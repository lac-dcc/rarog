#include "FirstFitAllocation.h"
#include "NaiveAllocation.h"
#include "mlir/Dialect/LLVMIR/LLVMDialect.h"
#include "mlir/IR/Block.h"
#include "mlir/IR/BuiltinOps.h"
#include "mlir/IR/BuiltinTypes.h"
#include "mlir/Pass/Pass.h"
#include "llvm/IR/Type.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/raw_ostream.h"
#include <fstream>
#include <list>
#include <unordered_set>

using namespace mlir;

namespace rarog {

namespace {

struct StaticAllocationPass
    : public PassWrapper<StaticAllocationPass, OperationPass<ModuleOp>> {

public:
  StaticAllocationPass(std::string resultFilename,
                       std::string allocationHeuristic)
      : ResultFilename(resultFilename),
        AllocationHeuristic(allocationHeuristic) {}

  void runOnOperation() override {
    if (!llvm::sys::fs::exists(ResultFilename)) {
      getOperation().emitError() << "Missing result file: " << ResultFilename;
      return signalPassFailure();
    }

    ModuleOp module = getOperation();

    LLVM::LLVMFuncOp targetFunc = nullptr;
    for (auto func : module.getOps<LLVM::LLVMFuncOp>()) {
      if (func.getName() == "torch_jit" || func.getName() == "tf2onnx") {
        targetFunc = func;
        break;
      }
    }

    if (!targetFunc)
      return;

    // Get the LLVM pointer type
    auto ptrType = LLVM::LLVMPointerType::get(module.getContext());

    // Get the LLVM void type
    auto voidType = LLVM::LLVMVoidType::get(module.getContext());

    auto i64Type = IntegerType::get(module.getContext(), 64);
    // Declare rarog_malloc(ptr, i64)
    // ptr is the pointer to the big allocated buffer
    // i64 is the offset from the ptr to allocate the memory
    declareFunction(module, "rarog_malloc", {ptrType, i64Type}, ptrType);

    // // Declare rarog_free(ptr, ptr)
    // // The first ptr is the pointer to the big allocated buffer
    // // The second ptr is the pointer to the buffer being deallocated
    // declareFunction(
    //   module,
    //   "rarog_free",
    //   {ptrType, ptrType},
    //   voidType
    // );

    // Declare instrumented_malloc
    declareFunction(module, "instrumented_malloc", {i64Type},
                    LLVM::LLVMPointerType::get(module.getContext()));

    // Declare instrumented_free
    declareFunction(module, "instrumented_free",
                    {LLVM::LLVMPointerType::get(module.getContext())},
                    LLVM::LLVMVoidType::get(module.getContext()));

    OpBuilder functionBuilder(module.getContext());
    Block &entryBlock = targetFunc.getBlocks().front();
    OpBuilder::InsertionGuard guard(functionBuilder);
    functionBuilder.setInsertionPointToStart(&entryBlock);

    Location loc = module.getLoc();

    // Compute the size of the allocated buffers for the model
    parseResultFile();

    size_t curOperation = 0;
    size_t curBuffer = 0;

    // Map associating pointers to indexes
    llvm::DenseMap<Value, size_t> ptrIndex;

    // Map associating pointer indexes to pointer metadata
    // Pointer metadata contains: alloc position, free position and buffer size
    llvm::DenseMap<size_t, std::tuple<size_t, size_t, size_t>> ptrMetadata;

    // Create instance of the static memory allocation problem
    targetFunc.walk([&](LLVM::CallOp callOp) {
      auto callee = callOp.getCallee();
      if (callee && *callee == "malloc") {
        // Find if the current malloc will be deallocated
        // If yes, add it to the packing problem
        // If not, it should be kept as a normal malloc
        std::unordered_set<Operation *> visited;
        if (!isFreed(callOp.getResult(), visited))
          return;

        // Associate the buffer with an index
        ptrIndex[callOp.getResult()] = curBuffer;

        // Get the operation index and size of the buffer
        ptrMetadata[curBuffer] = {curOperation, 0, bufferSizes[curBuffer]};

        // Update curBuffer and curOperation
        ++curBuffer;
        ++curOperation;
      } else if (callee && *callee == "free") {
        // Find the pointer index of the deallocated buffer
        Value deallocatedPtr = callOp.getOperand(0);
        size_t ptrIdx = ptrIndex.at(deallocatedPtr);

        // Update the free position of the pointer metadata
        auto [allocPos, freePos, size] = ptrMetadata.at(ptrIdx);
        ptrMetadata[ptrIdx] = {allocPos, curOperation, size};

        ++curOperation;
      }
    });

    llvm::SmallVector<std::tuple<size_t, size_t, size_t>> buffers;
    llvm::sort(buffers.begin(), buffers.end());
    for (size_t i = 0; i < curBuffer; ++i) {
      buffers.emplace_back(ptrMetadata.at(i));
    }

    auto [allocations, neededSize] = run_static_allocation(buffers);

    // Define mallocSize as the needed size to allocate the buffers in the
    // selected heuristic
    Value mallocSize = functionBuilder.create<LLVM::ConstantOp>(
        loc, functionBuilder.getI64Type(), neededSize);

    // Create a call to malloc with size mallocSize
    Value mallocPtr = functionBuilder
                          .create<LLVM::CallOp>(loc, ptrType,
                                                functionBuilder.getStringAttr(
                                                    "instrumented_malloc"),
                                                mallocSize)
                          .getResult();

    curBuffer = 0;

    // modify calls in the function body
    targetFunc.walk([&](LLVM::CallOp callOp) {
      // We don't want to modify the first malloc call
      if (callOp.getResult() == mallocPtr)
        return;

      auto callee = callOp.getCallee();
      if (callee && *callee == "malloc") {
        // Find if the current malloc will be deallocated
        // If yes, change it to rarog_malloc
        // If not, keep it as a normal malloc
        std::unordered_set<Operation *> visited;
        if (!isFreed(callOp.getResult(), visited))
          return;

        OpBuilder builder(callOp);

        // Get the offset from the allocations vector
        size_t offset = allocations[curBuffer];

        // Set a constant for the offset
        auto cstSize = builder.create<LLVM::ConstantOp>(
            callOp.getLoc(), builder.getI64Type(), offset);

        // Create operands vector
        llvm::SmallVector<Value> operands = {mallocPtr, cstSize};

        // Create a call to the rarog_malloc function
        auto newCall = builder.create<LLVM::CallOp>(
            callOp.getLoc(), callOp.getResultTypes(),
            builder.getStringAttr("rarog_malloc"), operands);

        // Replace the previous uses of malloc for the rarog_malloc result
        callOp.replaceAllUsesWith(newCall.getResults());

        // Erase the previous malloc call
        callOp.erase();

        // Update curBuffer
        ++curBuffer;
      } else if (callee && *callee == "free") {
        // Delete the free call since it's processed during the static
        // allocation
        callOp.erase();
      }
    });

    // Create free call to dealloc created malloc before each return instruction
    // of the function
    for (auto &block : targetFunc.getBlocks()) {
      auto *terminator = block.getTerminator();

      // Check if block terminator is a return instruction
      if (terminator && isa<LLVM::ReturnOp>(terminator)) {
        OpBuilder builder(terminator);

        builder.create<LLVM::CallOp>(
            loc, TypeRange{},
            functionBuilder.getStringAttr("instrumented_free"), mallocPtr);
      }
    }
  }

private:
  std::string ResultFilename;
  std::string AllocationHeuristic;
  llvm::SmallVector<size_t> bufferSizes;

  // Input: vector of triples containing, for each buffer: alloc position, free
  // position and buffer size Output: vector of offset for each buffer and
  // memory needed to allocate all the vectors
  std::pair<llvm::SmallVector<size_t>, size_t> run_static_allocation(
      llvm::SmallVector<std::tuple<size_t, size_t, size_t>> buffers) {
    if (AllocationHeuristic == "no-free") {
      return naive_allocation(buffers);
    } else {
      return first_fit_allocation(buffers);
    }
  }

  bool isFreed(Value pointer, std::unordered_set<Operation *> &visited) {
    for (Operation *user : pointer.getUsers()) {
      if (visited.count(user))
        continue;
      visited.insert(user);

      if (auto call = dyn_cast<LLVM::CallOp>(user)) {
        auto callee = call.getCallee();
        // Check if user callee is a free and, if so, stop modifying the
        // current malloc call
        if (callee && *callee == "free") {
          return true;
        }
      } else if (auto bitcast = dyn_cast<LLVM::BitcastOp>(user)) {
        return isFreed(bitcast.getResult(), visited);
      } else if (auto gep = dyn_cast<LLVM::GEPOp>(user)) {
        return isFreed(gep.getResult(), visited);
      } else if (auto insertValue = dyn_cast<LLVM::InsertValueOp>(user)) {
        return isFreed(insertValue.getResult(), visited);
      } else if (auto extractValue = dyn_cast<LLVM::ExtractValueOp>(user)) {
        return isFreed(extractValue.getResult(), visited);
      }
    }

    return false;
  }

  void declareFunction(ModuleOp module, llvm::StringRef name,
                       llvm::ArrayRef<Type> argTypes, Type retType) {
    for (auto func : module.getOps<LLVM::LLVMFuncOp>()) {
      if (func.getName() == name)
        return;
    }

    OpBuilder builder(module.getBodyRegion());
    auto funcType = LLVM::LLVMFunctionType::get(retType, argTypes);
    builder.create<LLVM::LLVMFuncOp>(module.getLoc(), name, funcType);
  }

  void parseResultFile() {
    std::ifstream resultFile(ResultFilename);

    std::string op;
    while (resultFile >> op) {
      if (op == "malloc") {
        std::string ptr;
        size_t size;
        resultFile >> ptr >> size;
        bufferSizes.emplace_back(size);
      } else {
        std::string ptr;
        resultFile >> ptr;
      }
    }
  }
};

} // namespace

std::unique_ptr<mlir::Pass>
createStaticAllocationPass(std::string resultFilename,
                           std::string allocationHeuristic) {
  return std::make_unique<StaticAllocationPass>(resultFilename,
                                                allocationHeuristic);
}

} // namespace rarog
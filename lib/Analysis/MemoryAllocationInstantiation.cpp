#include "mlir/Dialect/Func/IR/FuncOps.h"
#include "mlir/Dialect/LLVMIR/LLVMDialect.h"
#include "mlir/Dialect/MemRef/IR/MemRef.h"
#include "mlir/IR/Block.h"
#include "mlir/IR/BuiltinOps.h"
#include "mlir/IR/BuiltinTypes.h"
#include "mlir/Pass/Pass.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/raw_ostream.h"
#include <fstream>
#include <unordered_set>

using namespace mlir;

namespace rarog {

namespace {

struct MemoryAllocationInstantiationPass
    : public PassWrapper<MemoryAllocationInstantiationPass,
                         OperationPass<ModuleOp>> {

public:
  MemoryAllocationInstantiationPass(std::string resultFilename)
      : ResultFilename(resultFilename) {}

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

    parseResultFile();

    // Function name
    llvm::outs() << "Function: " << targetFunc.getName() << "\n";

    llvm::DenseMap<Operation *, int64_t> instructionMapping;
    int64_t idx = 0;
    targetFunc.walk([&](Operation *op) { instructionMapping[op] = idx++; });
    int64_t numInst = idx;

    llvm::DenseMap<Value, size_t> ptrIndex;
    llvm::DenseMap<int64_t, llvm::SmallVector<std::pair<int8_t, int64_t>>>
        opBufferMapping;

    idx = 0;
    targetFunc.walk([&](Operation *op) {
      if (auto callOp = dyn_cast<LLVM::CallOp>(op)) {
        auto callee = callOp.getCallee();
        if (callee && *callee == "malloc") {
          std::unordered_set<Operation *> visited;
          if (isFreed(callOp.getResult(), visited)) {
            ptrIndex[callOp.getResult()] = idx;
            opBufferMapping[instructionMapping.at(op)].emplace_back(0, idx);
            ++idx;
          }
        } else if (callee && *callee == "free") {
          Value deallocatedPtr = callOp.getOperand(0);
          size_t ptrIdx = ptrIndex.at(deallocatedPtr);
          opBufferMapping[instructionMapping.at(op)].emplace_back(2, ptrIdx);
        }
      }
    });

    for (size_t i = 0; i < numInst; i++) {
      if (opBufferMapping.contains(i)) {
        llvm::outs() << i << "\n";
        llvm::sort(opBufferMapping.at(i).begin(), opBufferMapping.at(i).end());
        for (auto [op, id] : opBufferMapping.at(i)) {
          if (op == 0) {
            llvm::outs() << "+ B" << id << " " << bufferSizes[id] << "\n";
          } else if (op == 1) {
            llvm::outs() << "* B" << id << "\n";
          } else {
            llvm::outs() << "- B" << id << "\n";
          }
        }
      }
    }
  }

private:
  std::string ResultFilename;
  llvm::SmallVector<size_t> bufferSizes;

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
};
} // namespace

std::unique_ptr<mlir::Pass>
createMemoryAllocationInstantiationPass(std::string resultFilename) {
  return std::make_unique<MemoryAllocationInstantiationPass>(resultFilename);
}

} // namespace rarog
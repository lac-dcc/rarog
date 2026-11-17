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

    if (llvm::Error err = parseResultFile()) {
      llvm::errs() << llvm::toString(std::move(err)) << "\n";
      signalPassFailure();
      return;
    }

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

    // Collect start (malloc) and end (free) instruction indices per buffer.
    llvm::SmallVector<int64_t> bufferStart(idx, -1);
    llvm::SmallVector<int64_t> bufferEnd(idx, -1);

    for (size_t i = 0; i < numInst; i++) {
      if (not opBufferMapping.contains(i))
        continue;

      llvm::sort(opBufferMapping.at(i).begin(), opBufferMapping.at(i).end());
      for (auto [op, id] : opBufferMapping.at(i)) {
        if (op == 0) {
          bufferStart[id] = static_cast<int64_t>(i);
        } else if (op == 2) {
          bufferEnd[id] = static_cast<int64_t>(i);
        }
      }
    }

    for (size_t id = 0; id < idx; id++) {
      assert(bufferSizes.size() > id);
      llvm::outs() << "B" << id << " " << bufferStart[id] << " "
                   << bufferEnd[id] << " " << bufferSizes[id] << "\n";
    }
  }

private:
  std::string ResultFilename;
  llvm::SmallVector<size_t> bufferSizes;

  llvm::Error parseResultFile() {
    // TODO: Unit tests to cover this parsing better
    std::ifstream resultFile(ResultFilename);
    if (!resultFile)
      return llvm::createStringError(
          llvm::inconvertibleErrorCode(),
          "parseResultFile: could not open result file '" + ResultFilename +
              "'");

    // Results should have the expected format on every line, otherwise we
    // need to report an error.

    std::string op;
    unsigned lineNo = 0;
    while (resultFile >> op) {
      ++lineNo;

      if (op == "malloc") {
        // Expected:  malloc <ptr> <size>
        std::string ptr;
        size_t size = 0;

        if (!(resultFile >> ptr)) {
          return llvm::createStringError(
              llvm::inconvertibleErrorCode(),
              "parseResultFile: line " + std::to_string(lineNo) +
                  ": expected '<ptr>' after 'malloc', but input ended");
        }
        if (!(resultFile >> size)) {
          return llvm::createStringError(
              llvm::inconvertibleErrorCode(),
              "parseResultFile: line " + std::to_string(lineNo) +
                  ": expected '<size>' after 'malloc " + ptr +
                  "', but input ended or size is not a number");
        }

        // TODO: Get a richer relation of buffer ids and their size, order
        // could be an issue.
        bufferSizes.emplace_back(size);
      } else if (op == "free") {
        // Expected:  free <ptr>
        std::string ptr;

        if (!(resultFile >> ptr)) {
          return llvm::createStringError(
              llvm::inconvertibleErrorCode(),
              "parseResultFile: line " + std::to_string(lineNo) +
                  ": expected '<ptr>' after 'free', but input ended");
        }
      } else {
        return llvm::createStringError(
            llvm::inconvertibleErrorCode(),
            "parseResultFile: line " + std::to_string(lineNo) +
                ": unexpected token '" + op +
                "' (expected 'malloc' or 'free')" + " while reading '" +
                ResultFilename + "'");
      }
    }

    // Distinguish a genuine I/O failure from a clean EOF.
    if (resultFile.bad()) {
      return llvm::createStringError(
          llvm::inconvertibleErrorCode(),
          "parseResultFile: I/O error while reading '" + ResultFilename +
              "' after line " + std::to_string(lineNo));
    }

    return llvm::Error::success();
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
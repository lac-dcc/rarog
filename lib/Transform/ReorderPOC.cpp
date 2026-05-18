#include "ReorderPOC.h"
#include "mlir/Dialect/Func/IR/FuncOps.h"
#include "mlir/Dialect/LLVMIR/LLVMDialect.h"
#include "mlir/Dialect/MemRef/IR/MemRef.h"
#include "mlir/IR/Block.h"
#include "mlir/IR/BuiltinOps.h"
#include "mlir/IR/BuiltinTypes.h"
#include "mlir/Pass/Pass.h"

using namespace mlir;

namespace rarog {

namespace {

struct ReorderPOCPass
    : public PassWrapper<ReorderPOCPass, OperationPass<ModuleOp>> {

  void runOnOperation() override {
    // TODO: Invoke Shuffling Number Analysis to get the ShufflingGraph
    llvm::outs() << "Hello World!\n";
  }
};

} // namespace

std::unique_ptr<mlir::Pass> rarog::createReorderPOCPass() {
  return std::make_unique<ReorderPOCPass>();
}

} // namespace rarog
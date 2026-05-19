#include "ReorderPOC.h"
/*
/rarog/lib/Transform/ReorderPOC.cpp:2:10: fatal error: 'ShufflingNumber.h' file
not found
    2 | #include "ShufflingNumber.h"
      |          ^~~~~~~~~~~~~~~~~~~
1 error generated.
ninja: build stopped: subcommand failed.
*/
#include "ShufflingNumber.h"
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
    auto &analysis = getAnalysis<ShufflingNumberGraphAnalysis>();
    Graph G = analysis.getShufflingNumberGraph();

    llvm::outs() << "Hello World!\n";
  }
};

} // namespace

std::unique_ptr<mlir::Pass> rarog::createReorderPOCPass() {
  return std::make_unique<ReorderPOCPass>();
}

} // namespace rarog
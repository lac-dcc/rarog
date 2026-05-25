#include "ReorderPOC.h"
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
    // ? Check if the correct function is analyzed, otherwise change to ModuleOp
    // and invoke against the Func inside it
    : public PassWrapper<ReorderPOCPass, OperationPass<func::FuncOp>> {

  void runOnOperation() override {
    auto &analysis = getAnalysis<ShufflingNumberGraphAnalysis>();
    Graph G = analysis.getShufflingNumberGraph();

    // *All the vertices
    llvm::outs() << "V = {";
    for (auto v : G.V) {
      llvm::outs() << " " << v->idx;
    }
    llvm::outs() << " }\n";

    llvm::outs() << "Current sources: ";
    for (auto src : G.get_sources()) {
      llvm::outs() << src->idx << " ";
    }
    llvm::outs() << "\n";

    // TODO: Use the info obtained from the graph to swap instructions that
    // aren't conflicting
  }
};

} // namespace

std::unique_ptr<mlir::Pass> createReorderPOCPass() {
  return std::make_unique<ReorderPOCPass>();
}

} // namespace rarog
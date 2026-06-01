#include "mlir/Dialect/Func/IR/FuncOps.h"
#include "mlir/Dialect/LLVMIR/LLVMDialect.h"
#include "mlir/Dialect/MemRef/IR/MemRef.h"
#include "mlir/IR/Block.h"
#include "mlir/IR/BuiltinOps.h"
#include "mlir/IR/BuiltinTypes.h"
#include "mlir/Pass/Pass.h"
#include "llvm/Support/raw_ostream.h"
#include <queue>

using namespace mlir;

namespace rarog {

namespace {

struct HoistAllocPass
    : public PassWrapper<HoistAllocPass, OperationPass<ModuleOp>> {

  void runOnOperation() override {
    ModuleOp module = getOperation();

    func::FuncOp targetFunc = nullptr;
    for (auto func : module.getOps<func::FuncOp>()) {
      if (func.getName() == "torch_jit" || func.getName() == "tf2onnx") {
        targetFunc = func;
        break;
      }
    }

    if (!targetFunc)
      return;
    if (targetFunc.getBlocks().size() != 1)
      return;

    // Map instructions to their index
    llvm::DenseMap<Operation *, int64_t> instructionMapping;
    int64_t idx = 0;
    targetFunc.walk([&](Operation *op) {
      // llvm::outs() << "    " << *op << "\n";
      instructionMapping[op] = idx++;
    });
    int64_t numInst = idx;

    //   // Collect allocation instructions
    //   llvm::SmallVector<memref::AllocOp> allocOps;
    //   targetFunc.walk([&](memref::AllocOp allocOp){
    //       allocOps.emplace_back(allocOp);
    //   });

    // Hoist memref.alloc instructions
    targetFunc.walk([&](memref::AllocOp allocOp) {
      if (!allocOp->getUsers().empty()) {

        // for (auto op : allocOp.getOperands()) {
        //     llvm::outs() << op << "\n";
        // }
        Operation *firstUser = *allocOp->getUsers().begin();
        for (auto user : allocOp->getUsers()) {
          if (instructionMapping[user] < instructionMapping[firstUser])
            firstUser = user;
        }

        OpBuilder builder(firstUser->getBlock(), Block::iterator(firstUser));
        auto newAllocOp = builder.create<memref::AllocOp>(
            allocOp.getLoc(), allocOp.getType(), allocOp.getOperands());
        allocOp.replaceAllUsesWith(newAllocOp.getResult());
        allocOp.erase();
      }
    });

    //   // erase memref.alloc at the end of the function
    //   for (auto allocOp : allocOps) {
    //     allocOp.erase();
    //   }
  }
};

} // namespace

std::unique_ptr<mlir::Pass> createHoistAllocPass() {
  return std::make_unique<HoistAllocPass>();
}

} // namespace rarog
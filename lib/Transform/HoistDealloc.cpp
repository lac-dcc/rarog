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

struct HoistDeallocPass
    : public PassWrapper<HoistDeallocPass, OperationPass<ModuleOp>> {

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

    // Collect deallocation instructions
    llvm::SmallVector<memref::DeallocOp> deallocOps;
    targetFunc.walk([&](memref::DeallocOp deallocOp) {
      deallocOps.emplace_back(deallocOp);
    });

    // Hoist memref.dealloc instructions
    targetFunc.walk([&](memref::AllocOp allocOp) {
      // Check if there is a deallocation instruction for allocOp
      bool hasDeallocInst = false;
      targetFunc.walk([&](memref::DeallocOp deallocOp) {
        if (deallocOp.getMemref() == allocOp.getResult())
          hasDeallocInst = true;
      });

      if (!hasDeallocInst)
        return;

      auto result = allocOp.getResult();
      std::queue<Operation *> users;
      Operation *lastUser = allocOp;

      users.emplace(allocOp);

      while (!users.empty()) {
        Operation *currentUser = users.front();
        users.pop();
        // llvm::errs() << "User for " << instructionMapping[allocOp] << " at "
        // << instructionMapping[currentUser] << "\n";

        if (instructionMapping[currentUser] > instructionMapping[lastUser])
          lastUser = currentUser;

        // Check if instruction create an alias for currentUser
        if (currentUser == allocOp || dyn_cast<memref::ViewOp>(currentUser) ||
            dyn_cast<memref::SubViewOp>(currentUser) ||
            dyn_cast<memref::ExpandShapeOp>(currentUser) ||
            dyn_cast<memref::CollapseShapeOp>(currentUser) ||
            dyn_cast<memref::ReshapeOp>(currentUser) ||
            dyn_cast<memref::ReinterpretCastOp>(currentUser) ||
            dyn_cast<memref::TransposeOp>(currentUser) ||
            dyn_cast<memref::CastOp>(currentUser) ||
            dyn_cast<memref::AssumeAlignmentOp>(currentUser)) {
          for (auto user : currentUser->getUsers()) {
            if (dyn_cast<memref::DeallocOp>(user)) {
              continue;
            }
            users.emplace(user);
          }
        }
      }

      // llvm::errs() << "The buffer " << *allocOp << "\n";
      // llvm::errs() << "Is last used by " << *lastUser << "\n";
      // create memref.dealloc after last use
      OpBuilder builder(lastUser->getBlock(), ++Block::iterator(lastUser));
      builder.create<memref::DeallocOp>(allocOp.getLoc(), allocOp.getResult());
    });

    // erase memref.dealloc at the end of the function
    for (auto deallocOp : deallocOps) {
      deallocOp.erase();
    }
  }
};

} // namespace

std::unique_ptr<mlir::Pass> createHoistDeallocPass() {
  return std::make_unique<HoistDeallocPass>();
}

} // namespace rarog
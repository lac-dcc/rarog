#include "mlir/Dialect/LLVMIR/LLVMDialect.h"
#include "mlir/IR/Block.h"
#include "mlir/IR/BuiltinOps.h"
#include "mlir/IR/BuiltinTypes.h"
#include "mlir/Pass/Pass.h"
#include "llvm/Support/raw_ostream.h"

using namespace mlir;

namespace rarog {

namespace {

struct InstrumentMallocPass
    : public PassWrapper<InstrumentMallocPass, OperationPass<ModuleOp>> {

  void runOnOperation() override {
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

    // Declare instrumented_malloc
    declareFunction(module, "instrumented_malloc",
                    {IntegerType::get(module.getContext(), 64)},
                    LLVM::LLVMPointerType::get(module.getContext()));

    // Declare instrumented_free
    declareFunction(module, "instrumented_free",
                    {LLVM::LLVMPointerType::get(module.getContext())},
                    LLVM::LLVMVoidType::get(module.getContext()));

    // modify calls in tf2onnx function
    targetFunc.walk([&](LLVM::CallOp callOp) {
      auto callee = callOp.getCallee();
      if (callee && *callee == "malloc") {
        OpBuilder builder(callOp);
        auto newCall = builder.create<LLVM::CallOp>(
            callOp.getLoc(), callOp.getResultTypes(),
            builder.getStringAttr("instrumented_malloc"), callOp.getOperands());
        callOp.replaceAllUsesWith(newCall.getResults());
        callOp.erase();
      } else if (callee && *callee == "free") {
        OpBuilder builder(callOp);
        auto newCall = builder.create<LLVM::CallOp>(
            callOp.getLoc(), callOp.getResultTypes(),
            builder.getStringAttr("instrumented_free"), callOp.getOperands());
        callOp.replaceAllUsesWith(newCall.getResults());
        callOp.erase();
      }
    });
  }

private:
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
};

} // namespace

std::unique_ptr<mlir::Pass> createInstrumentMallocPass() {
  return std::make_unique<InstrumentMallocPass>();
}

} // namespace rarog
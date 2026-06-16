#include "mlir/Dialect/Func/IR/FuncOps.h"
#include "mlir/Dialect/LLVMIR/LLVMDialect.h"
#include "mlir/Dialect/Linalg/IR/Linalg.h"
#include "mlir/Dialect/MemRef/IR/MemRef.h"
#include "mlir/Dialect/Tensor/IR/Tensor.h"
#include "mlir/IR/Block.h"
#include "mlir/IR/BuiltinOps.h"
#include "mlir/IR/BuiltinTypes.h"
#include "mlir/Pass/Pass.h"
#include "llvm/Support/raw_ostream.h"

using namespace mlir;

namespace rarog {

namespace {

struct AddRarogMainFunctionPass
    : public PassWrapper<AddRarogMainFunctionPass, OperationPass<ModuleOp>> {

  void runOnOperation() override {
    ModuleOp module = getOperation();
    OpBuilder builder(module.getContext());

    Location loc = module.getLoc();

    func::FuncOp targetFunc = nullptr;
    for (auto func : module.getOps<func::FuncOp>()) {
      if (func.getName() == "tf2onnx" || func.getName() == "torch_jit") {
        targetFunc = func;
        break;
      }
    }

    if (!targetFunc)
      return;

    if (!module.lookupSymbol<func::FuncOp>("main")) {
      auto mainFuncType = FunctionType::get(builder.getContext(), {}, {});
      auto mainFunc = builder.create<func::FuncOp>(loc, "main", mainFuncType);
      // mainFunc.addEntryBlock();

      OpBuilder::InsertionGuard guard(builder);
      builder.setInsertionPointToStart(mainFunc.addEntryBlock());

      // %c0 = arith.constant 1.0 : f32
      Value c0 =
          builder.create<arith::ConstantOp>(loc, builder.getF32FloatAttr(1.0f));

      // %i0 = tensor.empty() : tensor<1x32x32x3xf32>
      RankedTensorType emptyType;
      if (targetFunc.getName() == "tf2onnx") {
        emptyType = RankedTensorType::get({1, 32, 32, 3}, builder.getF32Type());
      } else {
        emptyType =
            RankedTensorType::get({1, 3, 224, 224}, builder.getF32Type());
      }
      Value i0 = builder.create<tensor::EmptyOp>(loc, emptyType.getShape(),
                                                 emptyType.getElementType());

      // %i1 = linalg.fill ins(%c0 : f32) outs(%i0 : tensor<1x32x32x3xf32>) ->
      // tensor<1x32x32x3xf32>
      auto fillOp = builder.create<linalg::FillOp>(loc, c0, i0);
      Value i1 = fillOp.getResult(0);

      // %cast = tensor.cast %i1 : tensor<1x32x32x3xf32> to
      // tensor<?x32x32x3xf32>
      RankedTensorType dynType;
      if (targetFunc.getName() == "tf2onnx") {
        dynType = RankedTensorType::get({ShapedType::kDynamic, 32, 32, 3},
                                        builder.getF32Type());
      } else {
        dynType = RankedTensorType::get({1, 3, 224, 224}, builder.getF32Type());
      }
      Value cast = builder.create<tensor::CastOp>(loc, dynType, i1);

      // %i2 = func.call @torchJit(%cast) : (tensor<?x32x32x3xf32>) ->
      // tensor<?x10xf32>
      FunctionType callType;
      if (targetFunc.getName() == "tf2onnx") {
        callType =
            FunctionType::get(module.getContext(), {dynType},
                              {RankedTensorType::get({ShapedType::kDynamic, 10},
                                                     builder.getF32Type())});
      } else {
        callType = FunctionType::get(
            module.getContext(), {dynType},
            {RankedTensorType::get({1, 1000}, builder.getF32Type())});
      }

      func::CallOp callFunc;

      if (targetFunc.getName() == "tf2onnx") {
        callFunc = builder.create<func::CallOp>(loc, "tf2onnx",
                                                callType.getResults(), cast);
      } else {
        callFunc = builder.create<func::CallOp>(loc, "torch_jit",
                                                callType.getResults(), cast);
      }
      Value i2 = callFunc.getResult(0);

      // %cast_0 = tensor.cast %i2 : tensor<?x10xf32> to tensor<*xf32>
      auto unrankedType = UnrankedTensorType::get(builder.getF32Type());
      Value cast_0 = builder.create<tensor::CastOp>(loc, unrankedType, i2);

      // func.call @printMemrefF32(%cast_0) : (tensor<*xf32>) -> ()
      builder.create<func::CallOp>(loc, "printMemrefF32", TypeRange{}, cast_0);

      // return
      builder.create<func::ReturnOp>(loc);

      module.push_back(mainFunc);
    }

    if (!module.lookupSymbol<func::FuncOp>("printMemrefF32")) {
      auto printType = FunctionType::get(
          builder.getContext(), {UnrankedTensorType::get(builder.getF32Type())},
          {});
      auto printFunc =
          builder.create<func::FuncOp>(loc, "printMemrefF32", printType);
      printFunc.setPrivate();
      module.push_back(printFunc);
    }
  }
};
} // namespace

std::unique_ptr<mlir::Pass> createAddRarogMainFunctionPass() {
  return std::make_unique<AddRarogMainFunctionPass>();
}

} // namespace rarog
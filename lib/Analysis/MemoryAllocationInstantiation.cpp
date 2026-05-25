#include "mlir/Dialect/Func/IR/FuncOps.h"
#include "mlir/Dialect/MemRef/IR/MemRef.h"
#include "mlir/IR/Block.h"
#include "mlir/IR/BuiltinOps.h"
#include "mlir/IR/BuiltinTypes.h"
#include "mlir/Pass/Pass.h"
#include "llvm/Support/raw_ostream.h"

using namespace mlir;

namespace rarog {

namespace {

struct MemoryAllocationInstantiationPass
    : public PassWrapper<MemoryAllocationInstantiationPass,
                         OperationPass<ModuleOp>> {

  void runOnOperation() override {
    ModuleOp module = getOperation();

    // Iterate over all functions in the module
    for (auto funcOp : module.getOps<func::FuncOp>()) {
      // Function name
      llvm::outs() << "Function: " << funcOp.getName() << "\n";

      llvm::DenseMap<Operation *, int64_t> instructionMapping;
      int64_t idx = 0;
      funcOp.walk([&](Operation *op) {
        // llvm::outs() << "    " << *op << "\n";
        instructionMapping[op] = idx++;
      });
      int64_t numInst = idx;

      llvm::DenseMap<int64_t, size_t> bufferSizeMapping;
      llvm::DenseMap<int64_t, llvm::SmallVector<std::pair<int8_t, int64_t>>>
          opBufferMapping;

      idx = 0;
      funcOp.walk([&](Operation *op) {
        if (auto allocOp = dyn_cast<memref::AllocOp>(op)) {
          // llvm::outs() << "Found memref.alloc instruction at idx: " <<
          // instructionMapping.at(op) << "\n";

          auto result = allocOp.getResult();
          auto type = result.getType();

          // Usage information
          llvm::SmallVector<int64_t> uses;
          for (auto user : result.getUsers()) {
            uses.emplace_back(instructionMapping.at(user));
          }
          llvm::sort(uses.begin(), uses.end());

          // Buffer has some uses
          if (uses.size() > 0) {

            // Size computation
            int64_t numElements = 1;
            for (auto dim : type.getShape()) {
              if (dim == ShapedType::kDynamic)
                dim = 1;
              numElements *= dim;
            }

            auto elemType = type.getElementType();
            auto typeSize = elemType.getIntOrFloatBitWidth() / 8;

            // Buffer [idx] has size numElements*typeSize
            bufferSizeMapping[idx] = numElements * typeSize;
            opBufferMapping[instructionMapping.at(op)].emplace_back(0, idx);

            // Size information
            // llvm::outs() << "\tSize: " << numElements*typeSize << "\n";

            // llvm::outs() << "\t\tType: " << type << "\n";
            // llvm::outs() << "\t\tElement Type: " << elemType << "\n";
            // llvm::outs() << "\t\tNumber of Elements: " << numElements <<
            // "\n"; llvm::outs() << "\t\tType size: " << typeSize << "\n\n";

            // llvm::outs() << "\tUses:";
            // for (auto use : uses) {
            //   llvm::outs() << " " << use;
            // }
            // llvm::outs() << "\n\n";
            for (size_t i = 0; i < uses.size() - 1; i++) {
              opBufferMapping[uses[i]].emplace_back(1, idx);
            }
            opBufferMapping[uses[uses.size() - 1]].emplace_back(2, idx);
            ++idx;
          }
        }
      });

      for (size_t i = 0; i < numInst; i++) {
        if (opBufferMapping.contains(i)) {
          llvm::outs() << i << "\n";
          llvm::sort(opBufferMapping.at(i).begin(),
                     opBufferMapping.at(i).end());
          for (auto [op, id] : opBufferMapping.at(i)) {
            if (op == 0) {
              llvm::outs() << "+ B" << id << " " << bufferSizeMapping.at(id)
                           << "\n";
            } else if (op == 1) {
              llvm::outs() << "* B" << id << "\n";
            } else {
              llvm::outs() << "- B" << id << "\n";
            }
          }
        }
      }
    }
  }
};
} // namespace

std::unique_ptr<mlir::Pass> createMemoryAllocationInstantiationPass() {
  return std::make_unique<MemoryAllocationInstantiationPass>();
}

} // namespace rarog
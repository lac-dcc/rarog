#include "Pipeline.h"
#include "MemoryAllocationInstantiation.h"
#include "ShufflingNumber.h"

#include "mlir/Conversion/ArithToLLVM/ArithToLLVM.h"
#include "mlir/Conversion/LLVMCommon/TypeConverter.h"
#include "mlir/Conversion/Passes.h"
#include "mlir/Conversion/SCFToControlFlow/SCFToControlFlow.h"
#include "mlir/Conversion/VectorToLLVM/ConvertVectorToLLVM.h"
#include "mlir/Dialect/Affine/Transforms/Passes.h"
#include "mlir/Dialect/Bufferization/Pipelines/Passes.h"
#include "mlir/Dialect/Bufferization/Transforms/Passes.h"
#include "mlir/Dialect/Linalg/Passes.h"
#include "mlir/Dialect/MemRef/Transforms/Passes.h"
#include "mlir/Pass/PassManager.h"
#include "mlir/Pass/PassOptions.h"
#include "mlir/Pass/PassRegistry.h"
#include "mlir/Transforms/Passes.h"
#include "llvm/Support/raw_ostream.h"

using namespace mlir;

struct MemoryAllocationInstantiationPipelineOptions
    : public PassPipelineOptions<MemoryAllocationInstantiationPipelineOptions> {
  Option<std::string> resultFilename{
      *this, "result-file",
      llvm::cl::desc("<Malloc instrumentation result file>"),
      llvm::cl::Required};
};

struct ShufflingNumberPipelineOptions
    : public PassPipelineOptions<ShufflingNumberPipelineOptions> {
  Option<bool> verbose{*this, "verbose",
                       llvm::cl::desc("<Enable verbose logging>"),
                       llvm::cl::Optional};
};

namespace {

void addMemoryAllocationInstantiationPipeline(
    OpPassManager &pm,
    const MemoryAllocationInstantiationPipelineOptions &options) {
  // --memory-AllocationInstantiation
  pm.addPass(
      rarog::createMemoryAllocationInstantiationPass(options.resultFilename));
}

void addShufflingNumberPipeline(OpPassManager &pm,
                                const ShufflingNumberPipelineOptions &options) {
  // --shuffling-number-pass="verbose"
  pm.addPass(rarog::createShufflingNumberPass(options.verbose));
}

} // namespace

namespace rarog {

void registerMemoryAllocationInstantiationPipeline() {
  PassPipelineRegistration<MemoryAllocationInstantiationPipelineOptions>(
      "memory-allocation-instantiation",
      "Instantiate memory allocation problem",
      addMemoryAllocationInstantiationPipeline);
}

void registerShufflingNumberPipeline() {
  PassPipelineRegistration<ShufflingNumberPipelineOptions>(
      "shuffling-number-pass", "Calculate the shuffling number of a function",
      addShufflingNumberPipeline);
}

} // namespace rarog
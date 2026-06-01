#include "Pipeline.h"
#include "AddNasbenchMainFunction.h"
#include "InstrumentMalloc.h"
#include "ReorderFrees.h"
#include "StaticAllocation.h"

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

struct NasbenchLoweringPipelineOptions
    : public PassPipelineOptions<NasbenchLoweringPipelineOptions> {
  Option<bool> enableReorderFrees{*this, "enable-reorder-frees",
                                  llvm::cl::desc("Enable reorder-frees pass"),
                                  llvm::cl::init(false)};
};

struct StaticAllocationPipelineOptions
    : public PassPipelineOptions<StaticAllocationPipelineOptions> {
  Option<std::string> resultFilename{
      *this, "result-file",
      llvm::cl::desc("<Malloc instrumentation result file>"),
      llvm::cl::Required};

  Option<std::string> allocationHeuristic{
      *this, "allocation-heuristic",
      llvm::cl::desc(
          "<Available allocation heuristics: no-free, first-fit, ilp>"),
      llvm::cl::init("first-fit")};
};

namespace {

void addNasbenchLoweringPipeline(
    OpPassManager &pm, const NasbenchLoweringPipelineOptions &options) {
  pm.addPass(rarog::createAddNasbenchMainFunctionPass());

  // --one-shot-bufferize="bufferize-function-boundaries"
  bufferization::OneShotBufferizePassOptions bufferizationOptions;
  bufferizationOptions.bufferizeFunctionBoundaries = true;
  pm.addPass(bufferization::createOneShotBufferizePass(bufferizationOptions));

  // --buffer-deallocation-pipeline
  // funcPM.addPass(bufferization::createBufferLoopHoistingPass());
  bufferization::BufferDeallocationPipelineOptions bufferDeallocOptions;
  mlir::bufferization::buildBufferDeallocationPipeline(pm,
                                                       bufferDeallocOptions);
  // funcPM.addPass(mlir::bufferization::createOptimizeAllocationLivenessPass());
  // funcPM.addPass(mlir::createConvertBufferizationToMemRefPass());

  if (options.enableReorderFrees) {
    // Hoist memref.realloc instructions close to last use of deallocated buffer
    pm.addPass(rarog::createReorderFreesPass());

    pm.addPass(createCanonicalizerPass());

    pm.addPass(createCSEPass());
  }

  // --convert-linalg-to-loops
  pm.addPass(createConvertLinalgToLoopsPass());
  // --expand-strided-metadata
  pm.addPass(memref::createExpandStridedMetadataPass());
  // --lower-affine
  pm.addPass(createLowerAffinePass());
  // --convert-vector-to-llvm
  pm.addPass(createConvertVectorToLLVMPass());
  // --convert-math-to-llvm
  pm.addPass(createConvertMathToLLVMPass());
  // --convert-math-to-libm
  pm.addPass(createConvertMathToLibmPass());
  // --convert-scf-to-cf
  pm.addPass(createSCFToControlFlowPass());
  // --convert-arith-to-llvm
  pm.addPass(createArithToLLVMConversionPass());
  // --convert-func-to-llvm
  pm.addPass(createConvertFuncToLLVMPass());
  // --convert-cf-to-llvm
  pm.addPass(createConvertControlFlowToLLVMPass());
  // --finalize-memref-to-llvm
  pm.addPass(createFinalizeMemRefToLLVMConversionPass());
  // --reconcile-unrealized-casts
  pm.addPass(createReconcileUnrealizedCastsPass());
  // --canonicalize
  pm.addPass(createCanonicalizerPass());
}

void addInstrumentMallocPipeline(OpPassManager &pm) {
  pm.addPass(rarog::createInstrumentMallocPass());
}

void addStaticAllocationPipeline(
    OpPassManager &pm, const StaticAllocationPipelineOptions &options) {
  // pm.addPass(createCanonicalizerPass());

  pm.addPass(rarog::createStaticAllocationPass(options.resultFilename,
                                               options.allocationHeuristic));

  // --canonicalize
  pm.addPass(createCanonicalizerPass());
}

void addReorderFreesPipeline(OpPassManager &pm) {
  // --one-shot-bufferize="bufferize-function-boundaries"
  bufferization::OneShotBufferizePassOptions bufferizationOptions;
  bufferizationOptions.bufferizeFunctionBoundaries = true;
  pm.addPass(bufferization::createOneShotBufferizePass(bufferizationOptions));

  // --buffer-deallocation-pipeline
  // funcPM.addPass(bufferization::createBufferLoopHoistingPass());
  bufferization::BufferDeallocationPipelineOptions bufferDeallocOptions;
  mlir::bufferization::buildBufferDeallocationPipeline(pm,
                                                       bufferDeallocOptions);

  // Hoist memref.realloc instructions close to last use of deallocated buffer
  pm.addPass(rarog::createReorderFreesPass());

  pm.addPass(createCanonicalizerPass());

  pm.addPass(createCSEPass());
}

} // namespace

namespace rarog {

void registerNasbenchLoweringPipeline() {
  PassPipelineRegistration<NasbenchLoweringPipelineOptions>(
      "nasbench-lowering-pipeline",
      "Insert main function and lower nasbench model to llvm",
      addNasbenchLoweringPipeline);
}

void registerInstrumentMallocPipeline() {
  PassPipelineRegistration<>(
      "instrument-malloc",
      "Change calls to malloc into calls to instrument_malloc",
      addInstrumentMallocPipeline);
}

void registerReorderFreesPipeline() {
  PassPipelineRegistration<>("reorder-frees",
                             "Hoist memref.realloc instructions close to last "
                             "use of deallocated buffer",
                             addReorderFreesPipeline);
}

void registerStaticAllocationPipeline() {
  PassPipelineRegistration<StaticAllocationPipelineOptions>(
      "static-allocation", "Add static memory allocation to ML models",
      addStaticAllocationPipeline);
}

} // namespace rarog
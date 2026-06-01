/**
 * @file rarog-opt.cpp
 * @brief Standalone driver for Rarog passes.
 */

// #include "MemoryVisualizerPass.h"
#include "Analysis/Pipeline.h"
#include "Transform/Pipeline.h"

#include "mlir/InitAllDialects.h"
#include "mlir/InitAllPasses.h"
#include "mlir/Pass/PassManager.h"
#include "mlir/Pass/PassOptions.h"
#include "mlir/Pass/PassRegistry.h"
#include "mlir/Tools/mlir-opt/MlirOptMain.h"

int main(int argc, char **argv) {
  mlir::registerAllPasses();

  rarog::registerMemoryAllocationInstantiationPipeline();
  rarog::registerShufflingNumberPipeline();
  rarog::registerRarogLoweringPipeline();
  rarog::registerInstrumentMallocPipeline();
  rarog::registerHoistDeallocPipeline();
  rarog::registerStaticAllocationPipeline();

  mlir::DialectRegistry registry;
  mlir::registerAllDialects(registry);

  return failed(mlir::MlirOptMain(argc, argv, "Rarog memory visualizer test\n",
                                  registry));
}
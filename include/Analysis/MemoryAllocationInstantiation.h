#ifndef RAROG_INCLUDE_ANALYSIS_MEMORYALLOCATIONINSTANTIATION_H
#define RAROG_INCLUDE_ANALYSIS_MEMORYALLOCATIONINSTANTIATION_H

#include "mlir/Pass/Pass.h"

namespace rarog {

std::unique_ptr<mlir::Pass> createMemoryAllocationInstantiationPass();

} // namespace rarog

#endif // RAROG_INCLUDE_ANALYSIS_MEMORYALLOCATIONINSTANTIATION_H
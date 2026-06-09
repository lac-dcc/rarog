#ifndef RAROG_INCLUDE_TRANSFORM_HOISTALLOC_H
#define RAROG_INCLUDE_TRANSFORM_HOISTALLOC_H

#include "mlir/Pass/Pass.h"

namespace rarog {

std::unique_ptr<mlir::Pass> createHoistAllocPass();

} // namespace rarog

#endif // RAROG_INCLUDE_TRANSFORM_HOISTALLOC_H
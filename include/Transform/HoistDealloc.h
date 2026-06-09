#ifndef RAROG_INCLUDE_TRANSFORM_HOISTDEALLOC_H
#define RAROG_INCLUDE_TRANSFORM_HOISTDEALLOC_H

#include "mlir/Pass/Pass.h"

namespace rarog {

std::unique_ptr<mlir::Pass> createHoistDeallocPass();

} // namespace rarog

#endif // RAROG_INCLUDE_TRANSFORM_HOISTDEALLOC_H
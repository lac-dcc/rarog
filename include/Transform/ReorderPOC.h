#ifndef RAROG_INCLUDE_TRANSFORM_REORDERPOC_H
#define RAROG_INCLUDE_TRANSFORM_REORDERPOC_H

#include "mlir/Pass/Pass.h"

namespace rarog {

std::unique_ptr<mlir::Pass> createReorderPOCPass();

} // namespace rarog

#endif // RAROG_INCLUDE_TRANSFORM_REORDERPOC_H
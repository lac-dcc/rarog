#ifndef RAROG_INCLUDE_TRANSFORM_REORDERFREES_H
#define RAROG_INCLUDE_TRANSFORM_REORDERFREES_H

#include "mlir/Pass/Pass.h"

namespace rarog {

std::unique_ptr<mlir::Pass> createReorderFreesPass();

} // namespace rarog

#endif // RAROG_INCLUDE_TRANSFORM_REORDERFREES_H
#ifndef RAROG_INCLUDE_TRANSFORM_ADDRAROGMAINFUNCTION_H
#define RAROG_INCLUDE_TRANSFORM_ADDRAROGMAINFUNCTION_H

#include "mlir/Pass/Pass.h"

namespace rarog {

std::unique_ptr<mlir::Pass> createAddRarogMainFunctionPass();

} // namespace rarog

#endif // RAROG_INCLUDE_TRANSFORM_ADDRAROGMAINFUNCTION_H
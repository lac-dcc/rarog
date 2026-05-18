#ifndef RAROG_INCLUDE_TRANSFORM_INSTRUMENTMALLOC_H
#define RAROG_INCLUDE_TRANSFORM_INSTRUMENTMALLOC_H

#include "mlir/Pass/Pass.h"

namespace rarog {

std::unique_ptr<mlir::Pass> createInstrumentMallocPass();

} // namespace rarog

#endif // RAROG_INCLUDE_TRANSFORM_INSTRUMENTMALLOC_H
#ifndef RAROG_INCLUDE_UTILS_ILPALLOCATION_H
#define RAROG_INCLUDE_UTILS_ILPALLOCATION_H

#include "llvm/ADT/SmallVector.h"

namespace rarog {

std::pair<llvm::SmallVector<size_t>, size_t>
ilp_allocation(llvm::SmallVector<std::tuple<size_t, size_t, size_t>> buffers);

} // namespace rarog

#endif
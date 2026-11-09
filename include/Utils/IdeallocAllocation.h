#ifndef RAROG_INCLUDE_UTILS_IDEALLOCALLOCATION_H
#define RAROG_INCLUDE_UTILS_IDEALLOCALLOCATION_H

#include "llvm/ADT/SmallVector.h"

namespace rarog {

std::pair<llvm::SmallVector<size_t>, size_t> idealloc_allocation(llvm::SmallVector<std::tuple<size_t, size_t, size_t>> buffers);

} // namespace rarog

#endif //RAROG_INCLUDE_UTILS_IDEALLOCALLOCATION_H
#include "NaiveAllocation.h"

namespace rarog {

std::pair<llvm::SmallVector<size_t>, size_t> naive_allocation(
    llvm::SmallVector<std::tuple<size_t, size_t, size_t>> buffers) {
  llvm::SmallVector<size_t> allocationDecisions;

  size_t offset = 0;

  for (auto [allocPos, freePos, size] : buffers) {
    allocationDecisions.emplace_back(offset);
    offset += size;
  }

  return {allocationDecisions, offset};
}

} // namespace rarog
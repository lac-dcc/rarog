#include "FirstFitAllocation.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/Support/raw_ostream.h"
#include <algorithm>
#include <list>

size_t allocate(size_t bufferSize,
                std::list<std::pair<size_t, size_t>> &freeIntervals);
void deallocate(size_t startPos, size_t bufferSize,
                std::list<std::pair<size_t, size_t>> &freeIntervals);

namespace rarog {

std::pair<llvm::SmallVector<size_t>, size_t> first_fit_allocation(
    llvm::SmallVector<std::tuple<size_t, size_t, size_t>> buffers) {

  // event := {allocPos, allocPos, size} if its alloc
  // event := {allocPos, freePos, size} if its dealloc
  llvm::SmallVector<std::tuple<size_t, size_t, size_t>> events;
  llvm::DenseMap<size_t, size_t> bufferOffset;
  size_t worstCaseSize = 0;

  for (auto [allocPos, freePos, size] : buffers) {
    // Allocation event
    events.emplace_back(allocPos, allocPos, size);

    // Deallocation event
    events.emplace_back(freePos, allocPos, size);

    worstCaseSize += size;
  }

  // events sorted by pos
  llvm::sort(events.begin(), events.end());

  // Create list of free intervals
  std::list<std::pair<size_t, size_t>> freeIntervals = {{0, worstCaseSize}};
  size_t neededSize = 0;

  llvm::SmallVector<size_t> allocationDecisions;

  for (auto [pos, allocPos, size] : events) {
    if (pos == allocPos) {
      size_t offset = allocate(size, freeIntervals);
      llvm::outs() << "\nAllocated buffer of size " << size << " at position "
                   << offset << "\n";
      bufferOffset[allocPos] = offset;
      allocationDecisions.emplace_back(offset);
      neededSize = std::max(neededSize, offset + size);
    } else {
      size_t offset = bufferOffset.at(allocPos);
      deallocate(offset, size, freeIntervals);
      llvm::outs() << "\nDeallocated buffer of size " << size << " at position "
                   << offset << "\n";
    }
  }

  return {allocationDecisions, neededSize};
}

} // namespace rarog

size_t allocate(size_t bufferSize,
                std::list<std::pair<size_t, size_t>> &freeIntervals) {
  for (auto it = freeIntervals.begin(); it != freeIntervals.end(); ++it) {
    if (bufferSize <= it->second) {
      size_t startPos = it->first;
      size_t endPos = startPos + bufferSize;
      size_t newSize = it->second - bufferSize;
      if (newSize > 0) {
        freeIntervals.emplace(it, endPos, newSize);
      }
      freeIntervals.erase(it);
      return startPos;
    }
  }
  return -1;
}

void deallocate(size_t startPos, size_t bufferSize,
                std::list<std::pair<size_t, size_t>> &freeIntervals) {

  auto isAfter = [startPos](auto ps) { return ps.first >= startPos; };

  auto it = std::find_if(freeIntervals.begin(), freeIntervals.end(), isAfter);

  bool isFirst = it == freeIntervals.begin(),
       isLast = it == freeIntervals.end();

  // If found, insert interval (startPos,bufferSize) just before iterator
  if (it != freeIntervals.end()) {
    freeIntervals.emplace(it, startPos, bufferSize);
  } else { // Otherwise, insert at the end
    freeIntervals.push_back({startPos, bufferSize});
  }

  // ? Try to Coalesce neighboring intervals
  auto coalesce = [&freeIntervals](auto p1, auto p2) {
    auto [pos1, size1] = *p1;
    auto [pos2, size2] = *p2;

    if (pos1 + size1 != pos2)
      return; // Not neighbors

    // Change value of iterator p1
    *p1 = {pos1, size1 + size2};
    // Remove iterator p2
    freeIntervals.erase(p2);
  };

  // ? point to newly created interval
  auto curr = std::prev(it);

  // * If curr is NOT the first element, try coalescing with interval before.
  if (!isFirst) {
    auto before = std::prev(curr);
    coalesce(before, curr);
  }

  // * If curr is NOT the last element, try coalescing with interval after
  if (!isLast) {
    auto curr = std::prev(it);
    coalesce(curr, it);
  }
}
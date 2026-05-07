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
  llvm::SmallVector<std::tuple<size_t, size_t, size_t>> events;
  llvm::DenseMap<size_t, size_t> bufferOffset;

  size_t neededSize = 0;

  std::list<std::pair<size_t, size_t>> freeIntervals;

  for (auto [allocPos, freePos, size] : buffers) {
    // Allocation event
    events.emplace_back(allocPos, allocPos, size);

    // Deallocation event
    events.emplace_back(freePos, allocPos, size);

    neededSize += size;
  }
  llvm::sort(events.begin(), events.end());

  // Create list of free intervals
  freeIntervals = {{0, neededSize}};
  neededSize = 0;

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

#define debug(it) llvm::outs() << #it << " = " << it

void deallocate(size_t startPos, size_t bufferSize,
                std::list<std::pair<size_t, size_t>> &freeIntervals) {

  auto isAfter = [startPos](std::pair<size_t, size_t> ps) {
    return ps.first >= startPos;
  };

  auto it = std::find_if(freeIntervals.begin(), freeIntervals.end(), isAfter);

  // If found, insert interval (startPos,bufferSize) just before iterator
  if (it != freeIntervals.end()) {
    freeIntervals.emplace(it, startPos, bufferSize);
  } else { // Otherwise, insert at the end
    freeIntervals.push_back({startPos, bufferSize});
  }

  // ? Coalesce neighbouring intervals (it-1)(it)(it+1)

  // * If it is NOT the first element, try joining with the interval before.

  if (it == freeIntervals.begin()) {
    llvm::outs() << "It's begin \n";
  } else {
    llvm::outs() << "It's NOT begin \n";
  }

  // * If it is NOT the last element, try joining with the interval after
  if (it == freeIntervals.end()) {
    llvm::outs() << "It's end\n";
  } else {
    llvm::outs() << "It's NOT end\n";
  }

  // for (auto it = freeIntervals.begin(); it != freeIntervals.end(); ++it) {
  //   if (it->first > startPos) {
  //     freeIntervals.emplace(it, startPos, bufferSize);
  //     // TODO: merge_intervals(freeIntervals);
  //     return;
  //   }
  // }
  // TODO: Also treat when no freeInterval after deallocate
  // buffer
}
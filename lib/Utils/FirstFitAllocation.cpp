#include "FirstFitAllocation.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/Support/raw_ostream.h"
#include <list>

size_t allocate(size_t bufferSize,
                std::list<std::pair<size_t, size_t>> &freeIntervals);
void merge_intervals(std::list<std::pair<size_t, size_t>> &freeIntervals);
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
  llvm::outs() << "\nTrying to allocate buffer of size " << bufferSize << "\n";
  llvm::outs() << "Available intervals:\n";
  for (auto [startPos, sz] : freeIntervals) {
    llvm::outs() << startPos << " " << sz << "\n";
  }
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

// TODO: We can change this to check only for the imediate previous and next
// element of the list
void merge_intervals(std::list<std::pair<size_t, size_t>> &freeIntervals) {
  llvm::SmallVector<std::list<std::pair<size_t, size_t>>::iterator>
      mergedIntervals, toErase;
  size_t startPos, endPos;
  for (auto it = freeIntervals.begin(); it != freeIntervals.end(); ++it) {
    if (mergedIntervals.empty()) {

      mergedIntervals.emplace_back(it);
      startPos = it->first;
      endPos = it->second + startPos;
    } else {

      if (it->first == endPos) {
        mergedIntervals.emplace_back(it);
        endPos = it->first + it->second;
      } else {
        if (mergedIntervals.size() > 1) {
          freeIntervals.emplace(it, startPos, endPos - startPos);
          for (auto toEraseIt : mergedIntervals) {
            toErase.emplace_back(toEraseIt);
          }
        }
        mergedIntervals.clear();
        mergedIntervals.emplace_back(it);
        startPos = it->first;
        endPos = it->second + startPos;
      }
    }
  }

  if (mergedIntervals.size() > 1) {
    freeIntervals.emplace(mergedIntervals[0], startPos, endPos - startPos);
    for (auto toEraseIt : mergedIntervals) {
      toErase.emplace_back(toEraseIt);
    }
  }

  for (auto it : toErase) {
    freeIntervals.erase(it);
  }
}

void deallocate(size_t startPos, size_t bufferSize,
                std::list<std::pair<size_t, size_t>> &freeIntervals) {
  for (auto it = freeIntervals.begin(); it != freeIntervals.end(); ++it) {
    if (it->first > startPos) {
      freeIntervals.emplace(it, startPos, bufferSize);
      merge_intervals(freeIntervals);
      return;
    }
  }
  // TODO: Also treat when no freeInterval after deallocate buffer
}
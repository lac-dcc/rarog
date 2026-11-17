#include "IdeallocAllocation.h"
#include "algo/Mod.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/Support/raw_ostream.h"
#include <algorithm>
#include <list>

namespace rarog {

std::pair<llvm::SmallVector<size_t>, size_t> idealloc_allocation(
  llvm::SmallVector<std::tuple<size_t, size_t, size_t>> buffers) {

  llvm::SmallVector<size_t> allocationDecisions;

  std::vector<std::tuple<size_t, size_t, size_t>> _buffers;
  for (auto buffer : buffers) {
    _buffers.push_back(buffer);
  }

  RarogBuffersParser parser = RarogBuffersParser(_buffers);
  std::vector<Job> _jobs = parser.read_jobs(0);

  JobSet jobs = jobset::init(std::move(_jobs));

  double                max_frag = 1.0;
  std::size_t           start     = 0;
  std::uint32_t         max_lives = 30000;

  auto [placement, sz] = idealloc(std::move(jobs), max_frag, start, max_lives);

  llvm::outs() << "Idealloc found allocation with size " << sz << "\n";

  for (int i = 0; i < buffers.size(); i++) {
    allocationDecisions.emplace_back(placement[i].get()->offset);
  }

  return {allocationDecisions, sz};
}

} // namespace rarog
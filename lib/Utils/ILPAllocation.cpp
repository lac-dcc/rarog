#include "ILPAllocation.h"

#include "llvm/Support/raw_ostream.h"
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

namespace rarog {

/// @brief Consults a .out file that was produced by the ILP for the
/// allocationDecisions
std::pair<llvm::SmallVector<size_t>, size_t>
ilp_allocation(llvm::SmallVector<std::tuple<size_t, size_t, size_t>> buffers,
               std::string ilpFilename) {

  llvm::SmallVector<size_t> allocationDecisions(buffers.size(), 0);
  size_t pool_size = 0;

  std::string line;
  std::ifstream ilp_file(ilpFilename);

  if (ilp_file.is_open()) {
    // The first line contains the total pool size.
    if (getline(ilp_file, line))
      pool_size = std::stoull(line);

    while (getline(ilp_file, line)) {
      std::istringstream ss(line);
      std::string bufferName;
      size_t offset;
      if (!(ss >> bufferName >> offset))
        continue;

      // Since buffer names are "B{index}" we just get the 'B' out, as in
      // lib/Analysis/MemoryAllocationInstantiation.cpp L97
      size_t bufferIndex = std::stoull(bufferName.substr(1));
      allocationDecisions[bufferIndex] = offset;
    }

    ilp_file.close();
  } else
    llvm::outs() << "Unable to open ILP file \"" << ilpFilename << "\"!\n";

  return {allocationDecisions, pool_size};
}

} // namespace rarog
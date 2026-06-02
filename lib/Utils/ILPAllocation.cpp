#include "ILPAllocation.h"

#include "llvm/Support/raw_ostream.h"
#include <algorithm>
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

  llvm::SmallVector<size_t> allocationDecisions;
  size_t maximum_offset = 0;

  std::string line;
  std::ifstream ilp_file(ilpFilename);

  if (ilp_file.is_open()) {
    while (getline(ilp_file, line)) {
      // Split the line by spaces
      std::string tok;
      std::stringstream ss(line);

      // discard block info
      getline(ss, tok, ' ');

      // check method
      getline(ss, tok, ' ');
      if (tok == "free")
        continue;

      // get offset and size
      getline(ss, tok, ' ');
      size_t offset = stoi(tok);
      getline(ss, tok, ' ');
      size_t size = stoi(tok);

      allocationDecisions.emplace_back(offset);
      maximum_offset = std::max(maximum_offset, offset + size);
    }

    ilp_file.close();
  } else
    llvm::outs() << "Unable to open ILP file \"" << ilpFilename << "\"!\n";

  return {allocationDecisions, maximum_offset};
}

} // namespace rarog
#ifndef RAROG_INCLUDE_ANALYSIS_SHUFFLINGNUMBER_H
#define RAROG_INCLUDE_ANALYSIS_SHUFFLINGNUMBER_H

#include "mlir/Dialect/Func/IR/FuncOps.h"
#include "mlir/Pass/Pass.h"

namespace rarog {

std::unique_ptr<mlir::Pass> createShufflingNumberPass(bool verbose);

using namespace std;
using namespace mlir;
typedef unsigned long long ull;

struct Vertex {
  Vertex(string idx) : idx{idx}, in_deg{0}, is_active{true} {}

  bool operator<(const Vertex &u) const { return idx < u.idx; }

  string idx;
  int in_deg;
  bool is_active;
};

struct Graph {
  Graph() {}

  void add_edge(Vertex *u, Vertex *v);
  void add_vertex(Vertex *u);
  void delete_vertex(Vertex *u);
  ull count(set<Vertex *> sources);
  set<Vertex *> get_sources();

  set<Vertex *> V;
  map<Vertex *, set<Vertex *>> adj;
  map<set<Vertex *>, ull> dp;
  static inline const ull outOfBounds = 1ULL << 27; //  2^27
};

struct ShufflingNumberGraphAnalysis {

  ShufflingNumberGraphAnalysis(Operation *op) {
    this->fn = cast<FunctionOpInterface>(op);
  }

  Graph getShufflingNumberGraph(bool VERBOSE = false);

private:
  FunctionOpInterface fn;
};

} // namespace rarog

#endif

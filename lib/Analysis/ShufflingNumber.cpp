#include "mlir/Dialect/Func/IR/FuncOps.h"
#include "mlir/Dialect/OpenACC/OpenACCUtils.h"
#include "mlir/IR/BuiltinOps.h"
#include "mlir/Pass/Pass.h"

#define debug(x) llvm::outs() << #x << " = " << x << " "

using namespace mlir;
using namespace func;

namespace rarog {

using namespace std;
typedef unsigned long long ull;

// From utils/topological_ordering_count.cpp
// modified to have string idx
struct Vertex {
  Vertex(string idx) : idx{idx}, in_deg{0}, is_active{true} {}

  bool operator<(const Vertex &u) const { return idx < u.idx; }

  string idx;
  int in_deg;
  bool is_active;
};

struct Graph {
  Graph() {}

  void add_edge(Vertex *u, Vertex *v) {
    if (!V.count(u))
      V.insert(u);
    if (!V.count(v))
      V.insert(v);

    if (!adj[u].count(v)) {
      adj[u].emplace(v);
      ++v->in_deg;
    }
  }

  void add_vertex(Vertex *u) {
    V.emplace(u);
    u->is_active = true;
  }

  void delete_vertex(Vertex *u) {
    V.erase(u);
    u->is_active = false;
  }

  ull count(set<Vertex *> sources) {
    // If G has 0 vertices, it has exactly 1 topological sorting.
    if (V.size() <= 1)
      return 1ULL;

    // Otherwise...  Find the source vertices of G. (These are just the
    // vertices with indegree 0.)

    // If there are none, there are no topological sortings of G.
    if (sources.empty())
      return dp[sources] = 0ULL;

    if (dp.count(sources))
      return dp[sources];

    // For each source vertex s of G, let ts be the number of topological
    // sortings of the simpler graph Gs obtained by deleting s from G.
    set<Vertex *> aux = sources;

    // The  number you want is just the sum of the ts values.
    ull res = 0ULL;

    for (Vertex *s : sources) {

      // Remove s from the graph
      aux.erase(s);
      delete_vertex(s);
      // Recalculate what the sources are after removing s
      for (Vertex *v : adj[s]) {
        if (v->is_active) {
          --v->in_deg;
          if (v->in_deg == 0)
            aux.emplace(v);
        }
      }

      // Recursively calculate topo sort of smaller graph
      // * Possibly stop on a BIG number
      res = res + count(aux);

      // Restore s in the graph
      add_vertex(s);
      // Restore the in_deg of vertices and remove sources pointed by s
      for (Vertex *v : adj[s]) {
        if (v->is_active) {
          if (v->in_deg == 0)
            aux.erase(v);
          ++v->in_deg;
        }
      }
      aux.emplace(s);

      // If it is already too big, exit early
      if (res >= outOfBounds) {
        return dp[sources] = outOfBounds;
      }
    }

    return dp[sources] = res;
  }

  set<Vertex *> get_sources() {
    set<Vertex *> sources;
    for (Vertex *v : V) {
      if (v->is_active && v->in_deg == 0)
        sources.emplace(v);
    }
    return sources;
  }

  set<Vertex *> V;
  map<Vertex *, set<Vertex *>> adj;
  map<set<Vertex *>, ull> dp;
  static inline const ull outOfBounds = 1ULL << 27; //  2^27
};

struct ShufflingNumberGraphAnalysis {

  ShufflingNumberGraphAnalysis(Operation *op) {
    this->fn = cast<FunctionOpInterface>(op);
  }

  Graph getShufflingNumberGraph(bool VERBOSE = false) {

    auto fnName = fn.getName();

    if (VERBOSE)
      llvm::outs() << "Calculating shuffling number for '" << fnName << "'\n";

    Graph G;
    map<string, Vertex *> vertices;

    // * Create vertex if does not exist
    auto get_or_insert = [&](string name) -> Vertex * {
      if (vertices.count(name) > 0) {
        return vertices.at(name);
      } else {
        // insert a new vertex with this name
        auto v = new Vertex(name);
        vertices.insert({name, v});
        return v;
      }
    };

    // Debug with
    // https://www.techiedelight.com/find-all-possible-topological-orderings-of-dag/

    for (Block &blk : fn.getBlocks()) {
      for (Operation &op : blk.getOperations()) {
        // <results...> = <opName> <operands...>
        vector<string> resultNames, operandNames;

        auto opName = op.getName();
        for (Value result : op.getResults()) {
          auto valuePortName = getValuePortName(result);
          resultNames.push_back(valuePortName);
        }

        for (Value operand : op.getOperands()) {
          auto valuePortName = getValuePortName(operand);
          operandNames.push_back(valuePortName);
        }

        // Create edge for each operandName -> resultName
        for (auto resultName : resultNames) {
          for (auto operandName : operandNames) {
            Vertex *opV = get_or_insert(operandName);
            Vertex *rsV = get_or_insert(resultName);

            // G.add_edge between the references to the vertices
            G.add_edge(opV, rsV);
          }
        }
      }
    }

    // * First step is to remove the sources from the graph, as we don't care
    //   about shuffling between the inputs and constants
    if (VERBOSE)
      llvm::outs() << "Removing sources: ";
    for (auto src : G.get_sources()) {
      if (VERBOSE)
        llvm::outs() << src->idx << " ";
      G.delete_vertex(src);
      for (Vertex *v : G.adj[src]) {
        v->in_deg--;
      }
    }
    if (VERBOSE)
      llvm::outs() << "\n";

    // *All the vertices
    if (VERBOSE) {
      llvm::outs() << "V = {";
      for (auto v : G.V) {
        llvm::outs() << " " << v->idx;
      }
      llvm::outs() << " }\n";

      llvm::outs() << "Current sources: ";
      for (auto src : G.get_sources()) {
        llvm::outs() << src->idx << " ";
      }
      llvm::outs() << "\n";
    }

    return G;
  }

private:
  FunctionOpInterface fn;

  // Obtained from pass --view-op-graph
  // https://github.com/llvm/llvm-project/blob/main/mlir/lib/Transforms/ViewOpGraph.cpp#L293
  std::string getValuePortName(Value operand) {
    string buf;
    llvm::raw_string_ostream os(buf);
    operand.printAsOperand(os, OpPrintingFlags());
    return buf;
  }
};

struct ShufflingNumberPass
    : public PassWrapper<ShufflingNumberPass, OperationPass<FuncOp>> {

public:
  ShufflingNumberPass(bool verbose) : VERBOSE(verbose) {}

  void runOnOperation() override {

    int numInst = 0;
    for (Block &blk : getOperation().getBlocks()) {
      numInst += blk.getOperations().size();
    }

    auto &analysis = getAnalysis<ShufflingNumberGraphAnalysis>();
    Graph G = analysis.getShufflingNumberGraph(VERBOSE);

    auto srcs = G.get_sources();
    int numVars = G.V.size();
    ull shufflingNumber = G.count(srcs);

    // Number of Variables, Number of Instructions, Shuffling Number
    llvm::outs() << numVars << "," << numInst << "," << shufflingNumber << "\n";
  }

private:
  bool VERBOSE;
};

std::unique_ptr<mlir::Pass> createShufflingNumberPass(bool verbose) {
  return std::make_unique<ShufflingNumberPass>(verbose);
}
}; // namespace rarog

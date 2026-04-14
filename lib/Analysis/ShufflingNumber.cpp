#include "mlir/Dialect/Func/IR/FuncOps.h"
#include "mlir/Dialect/Linalg/IR/Linalg.h"
#include "mlir/Dialect/OpenACC/OpenACCUtils.h"
#include "mlir/Dialect/Tensor/IR/Tensor.h"
#include "mlir/IR/BuiltinOps.h"
#include "mlir/Pass/Pass.h"

#define debug(x) llvm::outs() << #x << " = " << x << " "

using namespace mlir;
using namespace func;

namespace rarog {

using namespace std;

// https://www.geeksforgeeks.org/cpp/how-to-handle-large-numbers-in-cpp/
// Class to handle large numbers
class LargeNumber {
private:
  // The large number represented as a string
  string number;

public:
  LargeNumber() : number("0") {}
  LargeNumber(const string &num) : number(num) {}

  // Overloaded operator+ to add two LargeNumber objects
  LargeNumber operator+(const LargeNumber &other) const {
    string result;
    int carry = 0;
    int maxLength = max(number.length(), other.number.length());

    for (int i = 0; i < maxLength || carry; ++i) {
      int digit1 =
          i < number.length() ? number[number.length() - 1 - i] - '0' : 0;
      int digit2 = i < other.number.length()
                       ? other.number[other.number.length() - 1 - i] - '0'
                       : 0;

      int sum = digit1 + digit2 + carry;
      result.push_back(sum % 10 + '0');
      carry = sum / 10;
    }

    // Since the result is reversed, reverse it back to
    // get the correct number
    reverse(result.begin(), result.end());
    return LargeNumber(result);
  }

  // Overloaded operator<< to print a LargeNumber object
  friend llvm::raw_ostream &operator<<(llvm::raw_ostream &os,
                                       const LargeNumber &num) {
    return os << num.number;
  }
};

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

  LargeNumber count(set<Vertex *> sources) {
    // If G has 0 vertices, it has exactly 1 topological sorting.
    if (V.size() <= 1)
      return LargeNumber("1");

    // Otherwise...  Find the source vertices of G. (These are just the
    // vertices with indegree 0.)

    // If there are none, there are no topological sortings of G.
    if (sources.empty())
      return dp[sources] = LargeNumber("0");

    if (dp.count(sources))
      return dp[sources];

    // For each source vertex s of G, let ts be the number of topological
    // sortings of the simpler graph Gs obtained by deleting s from G.
    set<Vertex *> aux = sources;

    // The  number you want is just the sum of the ts values.
    LargeNumber res = LargeNumber("0");

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
  map<set<Vertex *>, LargeNumber> dp;
};

struct ShufflingNumberPass
    : public PassWrapper<ShufflingNumberPass, OperationPass<FuncOp>> {

  void runOnOperation() override {
    FuncOp fn = getOperation();
    auto fnName = fn.getName();

    // TODO: Will work on any function, after this base case tf2onnx works
    if (fnName != "tf2onnx") {
      return;
    }

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

    fn.walk([&](Operation *op) -> WalkResult {
      // <results...> = <opName> <operands...>
      vector<string> resultNames, operandNames;

      op->cloneWithoutRegions();

      if (auto linalgTranspose = dyn_cast<linalg::TransposeOp>(op)) {
        llvm::outs() << linalgTranspose << "\n";
      }

      if (auto tensorPad = dyn_cast<tensor::PadOp>(op)) {
        // llvm::outs() << tensorPad << "\n";
        auto modTensorPad = tensorPad.cloneWithoutRegions();
        llvm::outs() << modTensorPad << "\n";
      }

      auto opName = op->getName();
      for (Value result : op->getResults()) {
        auto valuePortName = getValuePortName(result);
        resultNames.push_back(valuePortName);
      }

      for (Value operand : op->getOperands()) {
        auto valuePortName = getValuePortName(operand);
        operandNames.push_back(valuePortName);
      }

      // Create edge for each operandName -> resultName
      for (auto resultName : resultNames) {
        for (auto operandName : operandNames) {
          Vertex *opV = get_or_insert(operandName);
          Vertex *rsV = get_or_insert(resultName);

          // G.add_edge between the references to the vertices
          llvm::outs() << "Edge " << opV->idx << " -> " << rsV->idx << "\n";
          G.add_edge(opV, rsV);
        }
      }

      return WalkResult::advance();
    });

    // * First step is to remove the sources from the graph, as we don't care
    //   about shuffling between the inputs and constants
    llvm::outs() << "Removing sources: ";
    for (auto src : G.get_sources()) {
      llvm::outs() << src->idx << " ";
      G.delete_vertex(src);
      for (Vertex *v : G.adj[src]) {
        v->in_deg--;
      }
    }
    llvm::outs() << "\n";

    // * All the vertices
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

    LargeNumber shufflingNumber = G.count(G.get_sources());

    debug(shufflingNumber) << "\n";
  }

private:
  // https://github.com/llvm/llvm-project/blob/42804379944cd0b221f9557ce219d4dc77a6055a/mlir/lib/Transforms/ViewOpGraph.cpp#L45-L50
  /// Return all values printed onto a stream as a string.
  static std::string strFromOs(function_ref<void(raw_ostream &)> func) {
    std::string buf;
    llvm::raw_string_ostream os(buf);
    func(os);
    return buf;
  }

  // https://github.com/llvm/llvm-project/blob/fbd0bcf55447c94dfa27bafb096f32e9c083f7ee/mlir/lib/Transforms/ViewOpGraph.cpp#L293-L302
  std::string getValuePortName(Value operand) {
    // Print value as an operand and omit the leading '%' character.
    auto str = strFromOs([&](raw_ostream &os) {
      operand.printAsOperand(os, OpPrintingFlags());
    });
    // ? Replace % and # with _
    // llvm::replace(str, '%', '_');
    // llvm::replace(str, '#', '_');
    return str;
  }
};

std::unique_ptr<mlir::Pass> createShufflingNumberPass() {
  return std::make_unique<ShufflingNumberPass>();
}
}; // namespace rarog

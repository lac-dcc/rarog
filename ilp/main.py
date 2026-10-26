# main.py
import sys

from tools import parse_trace, build_interference_graph
from model_ilp import build_weighted_model
from model_ilp import retrieve_weighted_solution


def main():
    if len(sys.argv) < 3:
        print("Usage: python main.py input.txt output.txt")
        return

    filein = sys.argv[1]
    fileout = sys.argv[2]

    trace = parse_trace(filein)

    print(f"File {filein}")

    graph = build_interference_graph(trace)

    mdl, x, y = build_weighted_model(graph)

    mdl.setParam("TimeLimit", 3600)

    mdl.optimize()

    if mdl.SolCount > 0:
        print("Best solution =", mdl.ObjVal)
        print("Best bound =", mdl.ObjBound)
        print("Runtime =", mdl.Runtime)
        print("Nodes =", mdl.NodeCount)

        retrieve_weighted_solution(mdl, x, y, graph, trace, fileout)
    else:
        print("No feasible solution found")


if __name__ == "__main__":
    main()

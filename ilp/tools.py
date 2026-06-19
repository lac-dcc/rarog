# tools.py

from dataclasses import dataclass, field
from typing import Dict, List, Optional, Tuple


@dataclass
class Buffer:
    name: str
    size: int
    start: int
    ends: Optional[int]
    uses: List[int] = field(default_factory=list)


@dataclass
class Trace:
    function_name: str
    buffers: Dict[str, Buffer]
    instructions: List[int]


class MyGraph:
    def __init__(self, n: int):
        self.n = n
        self.m = 0

        self.ids: Dict[str, int] = {}
        self.ids_inv: Dict[int, str] = {}

        self.edges: List[List[int]] = [[] for _ in range(n)]
        self.weights: List[int] = [0 for _ in range(n)]

    def has_edge(self, v: int, u: int) -> bool:
        return u in self.edges[v]

    def add_edge(self, v: int, u: int):
        if not self.has_edge(v, u):
            self.m += 1
            self.edges[v].append(u)
            self.edges[u].append(v)

    def get_anti(self, v: int):
        return [u for u in range(self.n) if not self.has_edge(v, u)]


def parse_trace(filename: str) -> Trace:
    buffers: Dict[str, Buffer] = {}
    instructions: List[int] = []

    current_instruction = None
    function_name = ""

    with open(filename, "r") as f:
        for line in f:
            line = line.strip()

            if not line:
                continue

            if line.startswith("Function:"):
                function_name = line.split(":")[1].strip()
                continue

            tokens = line.split()

            # instrução
            if len(tokens) == 1 and tokens[0].isdigit():
                current_instruction = int(tokens[0])
                instructions.append(current_instruction)
                continue

            # criação
            if tokens[0] == "+":
                name = tokens[1]
                size = int(tokens[2])

                buffers[name] = Buffer(
                    name=name, size=size, start=current_instruction, ends=None, uses=[]
                )
                continue

            # uso
            if tokens[0] == "*":
                name = tokens[1]
                buffers[name].uses.append(current_instruction)
                continue

            # remoção
            if tokens[0] == "-":
                name = tokens[1]
                buffers[name].ends = current_instruction
                continue

    return Trace(function_name, buffers, instructions)


def interfere(b1: Buffer, b2: Buffer) -> bool:
    return not (b1.ends < b2.start or b2.ends < b1.start)


def build_interference_graph(trace: Trace) -> MyGraph:
    graph = MyGraph(len(trace.buffers))

    # inicialização dos vértices
    idx = 0
    for name, buf in trace.buffers.items():
        graph.ids[name] = idx
        graph.ids_inv[idx] = name
        graph.weights[idx] = buf.size
        idx += 1

    buffers = list(trace.buffers.values())

    for i in range(len(buffers)):
        b1 = buffers[i]

        if b1.ends is None:
            continue

        for j in range(i + 1, len(buffers)):
            b2 = buffers[j]

            if b2.ends is None:
                continue

            if interfere(b1, b2):
                v = graph.ids[b1.name]
                u = graph.ids[b2.name]
                graph.add_edge(v, u)

    return graph

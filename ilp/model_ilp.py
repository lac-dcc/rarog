# model_ilp.py

from gurobipy import Model, GRB, quicksum


def build_weighted_model(graph):
    mdl = Model("DSA_ILP")
    n = graph.n

    # variáveis y
    y = {i: mdl.addVar(vtype=GRB.CONTINUOUS, lb=0, name=f"y_{i}") for i in range(n)}

    # variáveis x
    x = {}
    for i in range(n):
        for j in range(n):
            if not graph.has_edge(i, j):
                x[i, j] = mdl.addVar(vtype=GRB.BINARY, name=f"x_{i}_{j}")

    mdl.update()

    # restrições
    for v in range(n):
        lst = [u for u in range(n) if not graph.has_edge(u, v)]

        mdl.addConstr(quicksum(x[u, v] for u in lst) == 1, name=f"assign_{v}")

        for u in graph.get_anti(v):

            mdl.addConstr(x[v, u] <= x[v, v])

            mdl.addConstr(graph.weights[u] * x[v, u] <= y[v])

            lst2 = [
                w
                for w in range(n)
                if (not graph.has_edge(v, w)) and graph.has_edge(u, w)
            ]

            if len(lst2) > 0:
                mdl.addConstr(quicksum(x[v, w] for w in lst2) >= x[v, v] + x[u, u] - 1)

            for w in graph.get_anti(v):
                if w != u and graph.has_edge(u, w):
                    mdl.addConstr(x[v, u] + x[v, w] <= x[v, v])

    # objetivo
    mdl.setObjective(quicksum(y[i] for i in range(n)), GRB.MINIMIZE)

    mdl.update()

    return mdl, x, y


def retrieve_weighted_solution(mdl, x, y, graph, trace, output_file):
    offset = 0
    alloc = {}

    for v in range(graph.n):
        lst = [u for u in range(graph.n) if not graph.has_edge(u, v) and u != v]

        if x[v, v].X >= 0.5:
            val = int(round(y[v].X))

            alloc[graph.ids_inv[v]] = (offset, val)

            for u in lst:
                if x[v, u].X >= 0.5:
                    alloc[graph.ids_inv[u]] = (offset, val)

            offset += val

    events = []

    for name, buf in trace.buffers.items():
        a, b = alloc[name]

        events.append((buf.start, f"{name}: malloc {a} {b}"))

        if len(buf.uses) > 0:
            last_use = max(buf.uses)
            events.append((last_use, f"{name}: free {a}"))

    events.sort(key=lambda z: z[0])

    with open(output_file, "w") as f:
        for time, msg in events:
            f.write(f"{time}:{msg}\n")


def build_continuous_model(graph, factor):
    mdl = Model("DSA_CONTINUOUS")

    max_weight = max(graph.weights)

    # ignorando vértices muito pequenos
    v_nig = []
    w_nig = {}

    for v in range(graph.n):

        if floor(graph.weights[v] / factor) > 0.5:
            v_nig.append(v)
            w_nig[v] = int(ceil(graph.weights[v] / factor))

    # gcd dos pesos
    vals = list(w_nig.values())

    if len(vals) == 0:
        raise ValueError("No valid vertices after scaling.")

    mult = reduce(gcd, vals)

    print("gcd =", mult)

    max_scaled = max(vals)

    # conjunto de índices
    I = list(range(1, int(max_scaled / mult) + 1))

    print("I =", I)
    print("Vertices =", v_nig)

    # variáveis
    y = mdl.addVar(vtype=GRB.CONTINUOUS, lb=0, name="y")

    x = {}

    for v in v_nig:
        for i in I:
            x[v, i] = mdl.addVar(vtype=GRB.BINARY, name=f"x_{v}_{i}")

    mdl.update()

    # restrições
    for v in v_nig:

        # cada vértice recebe exatamente uma posição
        mdl.addConstr(quicksum(x[v, i] for i in I) == 1, name=f"assign_{v}")

        # vizinhos interferentes
        lst = [u for u in v_nig if graph.has_edge(u, v)]

        for i in I:
            # limite superior do intervalo
            mdl.addConstr(y >= (i * x[v, i] + (w_nig[v] / mult)), name=f"bound_{v}_{i}")

            # restrições de não sobreposição
            for u in lst:

                auxlist = [
                    i + j
                    for j in range(0, int(w_nig[v] / mult) + 1)
                    if (i + j) <= int(max_scaled / mult)
                ]

                if len(auxlist) > 0:
                    mdl.addConstr(
                        quicksum(x[u, j] for j in auxlist) <= x[v, i],
                        name=f"overlap_{v}_{u}_{i}",
                    )

    # objetivo
    mdl.setObjective(y, GRB.MINIMIZE)

    mdl.update()

    return mdl, x, y


def retrieve_continuous_solution(mdl, x, y, graph, factor, trace, output_file):

    max_weight = max(graph.weights)

    # ignorando vértices muito pequenos
    v_nig = []
    w_nig = {}

    for v in range(graph.n):

        if floor(graph.weights[v] / factor) > 0.5:
            v_nig.append(v)
            w_nig[v] = int(ceil(graph.weights[v] / factor))

    # gcd dos pesos
    vals = list(w_nig.values())

    if len(vals) == 0:
        raise ValueError("No valid vertices after scaling.")

    mult = reduce(gcd, vals)

    print("gcd =", mult)

    max_scaled = max(vals)

    # conjunto de índices
    I = list(range(1, int(max_scaled / mult) + 1))

    print("I =", I)
    print("Vertices =", v_nig)

    alloc = {}

    for v in v_nig:
        for i in I:
            if x[v, i].X >= 0.5:
                alloc[graph.ids_inv[v]] = (i, g.weights[v])

    events = []

    for name, buf in trace.buffers.items():
        a, b = alloc[name]

        events.append((buf.start, f"{name}: malloc {a} {b}"))

        if len(buf.uses) > 0:
            last_use = max(buf.uses)
            events.append((last_use, f"{name}: free {a}"))

    events.sort(key=lambda z: z[0])

    with open(output_file, "w") as f:
        for time, msg in events:
            f.write(f"{time}:{msg}\n")

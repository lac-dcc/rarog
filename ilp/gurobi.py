import sys
import gurobipy as gp
from gurobipy import GRB, Var
from dataclasses import dataclass

# 1. Data Definition
# Load buffers from file. File format: one buffer per line:
#     <id> <birth> <death> <size>
# Example (model_1.in):
#     B0 58 186 12352
#     B1 123 338 13936
#     ...
input_path = sys.argv[1] if len(sys.argv) > 1 else "model_1.in"
output_path = (
    sys.argv[2] if len(sys.argv) > 2 else input_path.rsplit(".", 1)[0] + ".out"
)

VERBOSE = False


@dataclass(frozen=True)
class Buffer:
    name: str
    birth: int
    death: int
    size: int


buffers: list[Buffer] = []
with open(input_path, "r") as f:
    for lineno, line in enumerate(f, start=1):
        line = line.strip()
        if not line or line.startswith("#"):
            continue
        parts = line.split()
        if len(parts) != 4:
            raise ValueError(
                f"{input_path}:{lineno}: expected 4 fields "
                f"(id birth death size), got: {line!r}"
            )
        buf_id, birth, death, size = parts
        buffers.append(
            Buffer(
                buf_id,
                int(birth),
                int(death),
                int(size),
            )
        )

if not buffers:
    raise ValueError(f"No buffers found in {input_path}")

# 2. Create Gurobi Model
model = gp.Model("OptimalMemoryAllocator")

# Numerical Precision Settings (Prevents Big-M tolerance leakage)
model.Params.IntegralityFocus = 1
model.Params.IntFeasTol = 1e-9
model.Params.FeasibilityTol = 1e-9

# 3. Decision Variables
# Starting memory address y[i] for each buffer
y: dict[str, Var] = {
    b.name: model.addVar(vtype=GRB.INTEGER, lb=0, name=f"addr_{b.name}")
    for b in buffers
}

# Variable representing the Maximum Memory Usage (MMU)
MMU = model.addVar(vtype=GRB.INTEGER, lb=0, name="MMU")

# Big-M value: upper bound on total memory size required
M = sum(b.size for b in buffers)

# Lookup helpers
buf_map = {b.name: b for b in buffers}
buf_ids = [b.name for b in buffers]

# 4. Binary relative positioning variables & Non-overlap constraints
x: dict[tuple[str, str], Var] = {}
for i in range(len(buf_ids)):
    for j in range(i + 1, len(buf_ids)):
        id1, id2 = buf_ids[i], buf_ids[j]
        b1, b2 = buf_map[id1], buf_map[id2]

        # Check if lifetimes overlap: max(birth1, birth2) < min(death1, death2)
        if max(b1.birth, b2.birth) < min(b1.death, b2.death):
            # x[id1, id2] = 1 if id1 is placed below id2, 0 otherwise
            M_ij = M - min(b1.size, b2.size)
            x[id1, id2] = model.addVar(vtype=GRB.BINARY, name=f"rel_{id1}_{id2}")

            # Constraint: id1 below id2 OR id2 below id1
            model.addConstr(
                y[id1] + b1.size <= y[id2] + M_ij * (1 - x[id1, id2]),
                name=f"no_overlap_{id1}_below_{id2}",
            )
            model.addConstr(
                y[id2] + b2.size <= y[id1] + M_ij * x[id1, id2],
                name=f"no_overlap_{id2}_below_{id1}",
            )

# 5. MMU Constraints
for b in buffers:
    # Buffer b occupies addresses from y[b] to y[b] + size - 1
    model.addConstr(MMU >= y[b.name] + b.size - 1, name=f"MMU_bound_{b.name}")

# 6. Set Objective
model.setObjective(MMU, GRB.MINIMIZE)

# 7. Solve Model
model.optimize()

# 8. Print / write results
if model.status == GRB.OPTIMAL:
    opt_MMU = int(round(MMU.X))
    mem_pool = opt_MMU + 1  # total bytes needed for the pool

    # Console output
    if VERBOSE:
        print(f"\n--- Optimal Solution Found ---")
        print(f"Optimal MMU: {opt_MMU}")
        print(f"Minimum Memory Pool Required: {mem_pool} bytes\n")
        print("Buffer Allocations:")
        for b in buffers:
            start = int(round(y[b.name].X))
            end = start + b.size - 1
            print(
                f"Buffer {b.name}: Address {start}..{end} "
                f"(Start: {start}, Size: {b.size})"
            )

    # Machine-readable output file:
    #   line 1: <M>            total memory pool size (bytes)
    #   lines:  <Bi> <offset>  start address of each buffer
    with open(output_path, "w") as f:
        f.write(f"{mem_pool}\n")
        for b in buffers:
            start = int(round(y[b.name].X))
            f.write(f"{b.name} {start}\n")

    print(f"\nWrote allocation map to: {output_path}")

else:
    print("TODO: Handle suboptimal result.")

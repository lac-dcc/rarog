import json
import os
from typing import Literal

tmp_results_path = os.environ.get("TMP_RESULTS_PATH", "tmp")
json_results_path = os.environ.get("JSON_RESULTS_PATH", ".")
model_name = os.environ.get("MODEL_NAME", "model_1")

d_model = tmp_results_path + "/" + model_name + "_lowered"
s_model = tmp_results_path + "/" + model_name + "_static_allocation"

d_mlir_log = d_model + ".mlir.log"
d_compile_logs = [d_model + ".ll.log", d_model + ".log"]
d_execution_log = d_model + ".exe.log"
d_memory_log = tmp_results_path + "/" + model_name + ".out"

s_mlir_log = s_model + ".mlir.log"
s_compile_logs = [s_model + ".ll.log", s_model + ".log"]
s_execution_log = s_model + ".exe.log"
s_memory_log = s_model + ".out"

Stats = Literal[
    "mlir_lowering_time",
    "total_compilation_time",
    "execution_time",
    "total_memory",
    "total_buffer_memory",
    "buffer_memory",
]

dt: dict[
    Literal["dynamic", "static"],
    dict[
        Stats,
        float,
    ],
] = {
    "dynamic": {},
    "static": {},
}


### Dynamic statistics

with open(d_mlir_log, "r") as f:
    a = f.readlines()
    t = float(a[0].split()[-1])
    dt["dynamic"]["mlir_lowering_time"] = t
    dt["dynamic"]["total_compilation_time"] = t

for i in d_compile_logs:
    with open(i, "r") as f:
        a = f.readlines()
        dt["dynamic"]["total_compilation_time"] += float(a[0].split()[-1])

with open(d_execution_log, "r") as f:
    a = f.readlines()
    dt["dynamic"]["execution_time"] = float(a[0].split()[-1])
    dt["dynamic"]["total_memory"] = round(float(a[1].split()[-1]) / 1024.0, 2)

with open(d_memory_log, "r") as f:
    a = f.readlines()
    min_pos = 0xFFFFFFFFFFFFFFFF
    max_pos = 0
    intervals: list[tuple[int, int]] = []
    for l in a:
        if l.split()[0] == "malloc":
            st = int(l.split()[1], 0)
            ed = st + int(l.split()[2])
            min_pos = min(min_pos, st)
            max_pos = max(max_pos, ed)
            intervals.append((st, ed))
    aux_l = 0
    aux_r = 0
    used_memory = 0
    for l, r in sorted(intervals):
        if l > aux_r:
            used_memory += aux_r - aux_l
            aux_l = l
            aux_r = r
        else:
            aux_r = max(r, aux_r)
    dt["dynamic"]["total_buffer_memory"] = round(
        float(max_pos - min_pos) / (1024.0 * 1024.0), 2
    )
    dt["dynamic"]["buffer_memory"] = round(
        float(used_memory + aux_r - aux_l) / (1024.0 * 1024.0), 2
    )


### Static statistics

with open(s_mlir_log, "r") as f:
    a = f.readlines()
    t = float(a[0].split()[-1])
    dt["static"]["mlir_lowering_time"] = t
    dt["static"]["total_compilation_time"] = t

for i in s_compile_logs:
    with open(i, "r") as f:
        a = f.readlines()
        dt["static"]["total_compilation_time"] += float(a[0].split()[-1])

with open(s_execution_log, "r") as f:
    a = f.readlines()
    dt["static"]["execution_time"] = float(a[0].split()[-1])
    dt["static"]["total_memory"] = round(float(a[1].split()[-1]) / 1024.0, 2)

with open(s_memory_log, "r") as f:
    a = f.readlines()
    dt["static"]["total_buffer_memory"] = round(
        float(int(a[0].split()[-1])) / (1024.0 * 1024.0), 2
    )
    dt["static"]["buffer_memory"] = dt["static"]["total_buffer_memory"]

json.dump(dt, open(json_results_path + "/" + model_name + ".json", "w"))

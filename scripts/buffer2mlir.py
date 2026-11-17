#!/usr/bin/env python3
"""
buffers2mlir.py - synthesize an MLIR program from buffer lifetime descriptions.

Input file: one buffer per line, `id, birth, death, size`
  - blank lines and lines starting with '#' are ignored
  - an optional header line (non-numeric birth/death/size) is skipped
  - birth/death must be distinct across ALL buffers; birth < death
  - size is in bytes

Each buffer becomes:
    %bufN = memref.alloc() : memref<SIZExi8>
    %dynN = memref.cast %bufN : ... to memref<?xi8>
    call @sink(%dynN)                      (right after alloc)
    ...
    call @sink(%dynN)                      (right before free)
    memref.dealloc %bufN

Only the relative order of events is preserved. The calls to the external,
opaque @sink function stop MLIR canonicalization and LLVM from deleting the
buffers as dead allocations.

Usage:
    python buffers2mlir.py buffers.txt -o prog.mlir --sink-c sink.c
"""

import argparse
import re
import sys


def parse_buffers(path):
    buffers = []
    with open(path) as f:
        for lineno, raw in enumerate(f, 1):
            line = raw.split("#", 1)[0].strip()
            if not line:
                continue
            parts = [p.strip() for p in line.split(",")]
            if len(parts) != 4:
                sys.exit(f"{path}:{lineno}: expected 4 fields "
                         f"`id, birth, death, size`, got {len(parts)}")
            id_, birth_s, death_s, size_s = parts
            try:
                birth, death, size = float(birth_s), float(death_s), int(size_s)
            except ValueError:
                if not buffers:  # tolerate a header line
                    continue
                sys.exit(f"{path}:{lineno}: non-numeric birth/death/size")
            if birth >= death:
                sys.exit(f"{path}:{lineno}: buffer {id_}: birth must be < death")
            if size < 0:
                sys.exit(f"{path}:{lineno}: buffer {id_}: negative size")
            buffers.append((id_, birth, death, size))
    return buffers


def validate(buffers):
    seen_ids = {}
    for id_, *_ in buffers:
        name = re.sub(r"\W", "_", id_)
        if name in seen_ids:
            sys.exit(f"duplicate buffer id (after sanitizing): {id_!r}")
        seen_ids[name] = id_
    times = {}
    for id_, birth, death, _ in buffers:
        for t, what in ((birth, "birth"), (death, "death")):
            if t in times:
                sys.exit(f"time {t} is not distinct: {what} of {id_!r} "
                         f"collides with {times[t]}")
            times[t] = f"{what} of {id_!r}"


def synthesize(buffers, func_name="main", alignment=None):
    events = []
    for id_, birth, death, size in buffers:
        events.append((birth, "alloc", id_, size))
        events.append((death, "free", id_, size))
    events.sort(key=lambda e: e[0])

    align = f" {{alignment = {alignment}}}" if alignment else ""
    out = [
        "func.func private @sink(memref<?xi8>)",
        "",
        f"func.func @{func_name}() {{",
    ]
    for _, kind, id_, size in events:
        n = re.sub(r"\W", "_", id_)
        ty = f"memref<{size}xi8>"
        if kind == "alloc":
            out.append(f"  %buf{n} = memref.alloc(){align} : {ty}")
            out.append(f"  %dyn{n} = memref.cast %buf{n} : {ty} to memref<?xi8>")
            out.append(f"  func.call @sink(%dyn{n}) : (memref<?xi8>) -> ()")
        else:
            out.append(f"  func.call @sink(%dyn{n}) : (memref<?xi8>) -> ()")
            out.append(f"  memref.dealloc %buf{n} : {ty}")
    out += ["  return", "}", ""]
    return "\n".join(out)


SINK_C = """\
#include <stdint.h>

/* Opaque sink for memref<?xi8> (lowered to the expanded descriptor:
   allocated ptr, aligned ptr, offset, size, stride). Keep this in a
   separate translation unit so nothing can inline it away. */
void sink(void *allocated, void *aligned, int64_t offset,
          int64_t size, int64_t stride) {
  (void)allocated; (void)aligned; (void)offset; (void)size; (void)stride;
}
"""


def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("input", help="buffer description file")
    ap.add_argument("-o", "--output", help="output .mlir file (default: stdout)")
    ap.add_argument("--func-name", default="main", help="generated function name")
    ap.add_argument("--alignment", type=int, help="alignment attribute on allocs")
    ap.add_argument("--sink-c", metavar="FILE", help="also write the C stub for @sink")
    args = ap.parse_args()

    buffers = parse_buffers(args.input)
    if not buffers:
        sys.exit("no buffers found in input")
    validate(buffers)

    mlir = synthesize(buffers, args.func_name, args.alignment)
    if args.output:
        with open(args.output, "w") as f:
            f.write(mlir)
    else:
        sys.stdout.write(mlir)

    if args.sink_c:
        with open(args.sink_c, "w") as f:
            f.write(SINK_C)


if __name__ == "__main__":
    main()
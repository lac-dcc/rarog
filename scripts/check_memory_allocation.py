import argparse
import sys
import tempfile
import unittest
from dataclasses import dataclass
from pathlib import Path


@dataclass(frozen=True)
class Buffer:
    name: str
    birth: int
    death: int
    size: int


def read_buffers(path: Path) -> list[Buffer]:
    """
    Check that `path` content has the format:
    ```
    B0 birth_0 death_0 size_0
    ...
    BN birth_N death_N size_N
    ```
    that names are unique, values are positive and birth is less-equal death
    """
    buffers: list[Buffer] = []
    names: set[str] = set()

    with path.open() as file:
        for line_number, line in enumerate(file, start=1):
            line = line.strip()
            if not line or line.startswith("#"):
                continue

            fields = line.split()
            if len(fields) != 4:
                raise ValueError(
                    f"{path}:{line_number}: expected 4 fields "
                    f"(name birth death size), got {line!r}"
                )

            name = fields[0]
            try:
                birth, death, size = map(int, fields[1:])
            except ValueError as error:
                raise ValueError(
                    f"{path}:{line_number}: birth, death, and size must be integers"
                ) from error

            if name in names:
                raise ValueError(
                    f"{path}:{line_number}: duplicate buffer name {name!r}"
                )
            if birth > death:
                raise ValueError(
                    f"{path}:{line_number}: death precedes birth for {name}"
                )
            if size < 0:
                raise ValueError(
                    f"{path}:{line_number}: size must not be negative for {name}"
                )

            names.add(name)
            buffers.append(Buffer(name, birth, death, size))

    if not buffers:
        raise ValueError(f"{path}: no buffers found")
    return buffers


def read_allocation(path: Path) -> tuple[int, dict[str, int]]:
    """
    Check that `path` content has the format:
    ```
    pool_size
    B0 buf_size_0
    ...

    BN buf_size_N
    ```
    that names are unique, all buffers fit in pool_size, and have non-negative size
    """
    with path.open() as file:
        lines = [
            (line_number, line.strip())
            for line_number, line in enumerate(file, start=1)
        ]

    lines = [
        (number, line) for number, line in lines if line and not line.startswith("#")
    ]
    if not lines:
        raise ValueError(f"{path}: no memory pool size found")

    line_number, line = lines[0]
    fields = line.split()
    if len(fields) != 1:
        raise ValueError(
            f"{path}:{line_number}: expected one field for memory pool size"
        )
    try:
        pool_size = int(fields[0])
    except ValueError as error:
        raise ValueError(
            f"{path}:{line_number}: memory pool size must be an integer"
        ) from error
    if pool_size < 0:
        raise ValueError(f"{path}:{line_number}: memory pool size must not be negative")

    offsets: dict[str, int] = {}
    for line_number, line in lines[1:]:
        fields = line.split()
        if len(fields) != 2:
            raise ValueError(
                f"{path}:{line_number}: expected 2 fields (name offset), got {line!r}"
            )
        name = fields[0]
        try:
            offset = int(fields[1])
        except ValueError as error:
            raise ValueError(
                f"{path}:{line_number}: offset must be an integer"
            ) from error
        if name in offsets:
            raise ValueError(f"{path}:{line_number}: duplicate allocation for {name!r}")
        if offset < 0:
            raise ValueError(
                f"{path}:{line_number}: offset must not be negative for {name}"
            )
        offsets[name] = offset

    return pool_size, offsets


def check_allocation(
    buffers: list[Buffer], pool_size: int, offsets: dict[str, int]
) -> list[str]:
    """
    Compares buffers to allocated offsets, checks if
    * All buffers fit inside pool
    * No buffers alive together share memory addresses
    """
    errors: list[str] = []
    buffers_by_name = {buffer.name: buffer for buffer in buffers}

    # All names are accounted for
    for name in sorted(offsets.keys() - buffers_by_name.keys()):
        errors.append(f"allocation contains unknown buffer {name}")
    for name in sorted(buffers_by_name.keys() - offsets.keys()):
        errors.append(f"allocation is missing buffer {name}")

    allocated_buffers = [buffer for buffer in buffers if buffer.name in offsets]
    for buffer in allocated_buffers:
        end = offsets[buffer.name] + buffer.size
        if end > pool_size:
            errors.append(
                f"{buffer.name} ends at byte {end}, beyond memory pool size {pool_size}"
            )

    for index, first in enumerate(allocated_buffers):
        first_start = offsets[first.name]
        first_end = first_start + first.size
        for second in allocated_buffers[index + 1 :]:
            if max(first.birth, second.birth) >= min(first.death, second.death):
                continue  # These aren't both alive at any given time

            second_start = offsets[second.name]
            second_end = second_start + second.size
            overlap_start = max(first_start, second_start)
            overlap_end = min(first_end, second_end)
            if overlap_start < overlap_end:
                errors.append(
                    f"{first.name} and {second.name} are both alive and overlap in "
                    f"memory at bytes [{overlap_start}, {overlap_end})"
                )

    return errors


# Run with python -m unittest -v check_memory_allocation.py
class TestCheckAllocation(unittest.TestCase):
    def test_allows_disjoint_ranges_for_live_buffers(self):
        buffers = [Buffer("A", 0, 5, 10), Buffer("B", 1, 4, 5)]

        errors = check_allocation(buffers, 16, {"A": 0, "B": 11})
        self.assertEqual(len(errors), 0)

    def test_rejects_overlapping_ranges_for_live_buffers(self):
        buffers = [Buffer("A", 0, 5, 10), Buffer("B", 1, 4, 5)]

        errors = check_allocation(buffers, 14, {"A": 0, "B": 9})

        self.assertEqual(len(errors), 1)
        self.assertIn("A and B", errors[0])
        self.assertIn("[9, 10)", errors[0])

    def test_allows_adjacent_memory_ranges_for_live_buffers(self):
        buffers = [Buffer("A", 0, 5, 10), Buffer("B", 1, 4, 5)]

        errors = check_allocation(buffers, 15, {"A": 0, "B": 10})
        self.assertEqual(len(errors), 0)

    def test_allows_memory_reuse_when_lifetimes_touch(self):
        buffers = [Buffer("A", 0, 5, 10), Buffer("B", 5, 8, 10)]

        errors = check_allocation(buffers, 10, {"A": 0, "B": 0})
        self.assertEqual(len(errors), 0)

    def test_accepts_buffer_ending_at_pool_boundary(self):
        buffers = [Buffer("A", 0, 1, 10)]

        errors = check_allocation(buffers, 10, {"A": 0})
        self.assertEqual(len(errors), 0)

    def test_rejects_buffer_ending_past_pool_boundary(self):
        buffers = [Buffer("A", 0, 1, 10)]

        errors = check_allocation(buffers, 10, {"A": 1})

        self.assertTrue(any("beyond memory pool size 10" in error for error in errors))

    def test_reports_missing_and_unknown_buffers(self):
        buffers = [Buffer("A", 0, 1, 1)]

        errors = check_allocation(buffers, 1, {"unknown": 0})

        self.assertTrue(any("missing buffer A" in error for error in errors))
        self.assertTrue(any("unknown buffer unknown" in error for error in errors))

    def test_alexnet_allocation_ok(self):
        buffers = [
            Buffer("B0", 104, 280, 623872),
            Buffer("B1", 170, 431, 774464),
            Buffer("B2", 322, 498, 186688),
            Buffer("B3", 371, 1528, 100),
            Buffer("B4", 435, 610, 246080),
            Buffer("B5", 502, 757, 559936),
            Buffer("B6", 652, 824, 129856),
            Buffer("B7", 761, 947, 172864),
            Buffer("B8", 828, 1052, 259648),
            Buffer("B9", 989, 1175, 345664),
            Buffer("B10", 1056, 1527, 173120),
            Buffer("B11", 1217, 1381, 230464),
            Buffer("B12", 1423, 1737, 36928),
            Buffer("B13", 1433, 1624, 36928),
            Buffer("B14", 1560, 2157, 68),
            Buffer("B15", 1628, 1736, 150995008),
            Buffer("B16", 1656, 1977, 16448),
            Buffer("B17", 1666, 1863, 16448),
            Buffer("B18", 1690, 1759, 16448),
            Buffer("B19", 1783, 1841, 67108928),
            Buffer("B20", 1887, 1976, 16384064),
            Buffer("B21", 2002, 2058, 4064),
            Buffer("B22", 2019, 2081, 72),
        ]
        pool_size = 151081348
        offsets = {
            "B0": 3251848,
            "B1": 2477384,
            "B2": 2290696,
            "B3": 2290596,
            "B4": 2044516,
            "B5": 1484580,
            "B6": 1354724,
            "B7": 1181860,
            "B8": 922212,
            "B9": 576548,
            "B10": 230464,
            "B11": 0,
            "B12": 49412,
            "B13": 86340,
            "B14": 49344,
            "B15": 86340,
            "B16": 32896,
            "B17": 16448,
            "B18": 0,
            "B19": 49412,
            "B20": 49412,
            "B21": 72,
            "B22": 0,
        }
        errors = check_allocation(buffers, pool_size, offsets)
        self.assertEqual(len(errors), 0)


class TestFileReaders(unittest.TestCase):
    def test_reads_buffer_file(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "model.in"
            path.write_text("# comment\n\nA 0 5 10\n", encoding="utf-8")

            self.assertEqual(read_buffers(path), [Buffer("A", 0, 5, 10)])

    def test_reads_allocation_file(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "model.out"
            path.write_text("10\nA 0\n", encoding="utf-8")

            self.assertEqual(read_allocation(path), (10, {"A": 0}))

    def test_rejects_duplicate_allocation_names(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "model.out"
            path.write_text("10\nA 0\nA 1\n", encoding="utf-8")

            with self.assertRaisesRegex(ValueError, "duplicate allocation"):
                read_allocation(path)


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Check that a memory allocation output is valid for its input."
    )
    parser.add_argument("input", type=Path, help="buffer input file (.in)")
    parser.add_argument("output", type=Path, help="allocation output file (.out)")
    args = parser.parse_args()

    try:
        buffers = read_buffers(args.input)
        pool_size, offsets = read_allocation(args.output)
    except (OSError, ValueError) as error:
        print(f"Invalid file: {error}", file=sys.stderr)
        return 2

    errors = check_allocation(buffers, pool_size, offsets)
    if errors:
        print("Invalid allocation:")
        for error in errors:
            print(f"- {error}")
        return 1

    print(f"Valid allocation: {len(buffers)} buffers fit in a {pool_size}-byte pool.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

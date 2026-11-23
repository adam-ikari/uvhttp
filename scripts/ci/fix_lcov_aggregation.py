#!/usr/bin/env python3
"""Merge duplicate lcov source-file records produced by per-target gcda.

Each test target compiles the library sources independently, so ``lcov
--capture`` emits many records with the same ``SF:`` path. lcov's own merge
keeps only one of them for line coverage, which understates files exercised
mainly by mock targets (connection.c reported 45.3% vs ~79.6% actual).

This script sums DA/BRDA/FNDA hit counters across all records of a source file,
so a line is uncovered only if no target ever executed it.

Usage:
    python3 fix_lcov_aggregation.py -i coverage.info -o coverage-fixed.info
"""
import argparse
from collections import defaultdict


def parse_records(text):
    """Yield (sf, lines) for each end_of_record-delimited block."""
    block = []
    for line in text.splitlines():
        block.append(line)
        if line == "end_of_record":
            sf = next((l[3:] for l in block if l.startswith("SF:")), None)
            if sf is not None:
                yield sf, block
            block = []


def merge(blocks):
    """Sum hit counters across blocks belonging to the same source file."""
    da = defaultdict(int)
    brda = {}
    fnda = {}
    header = []
    source_lines = set()
    fn = set()
    fnab = set()

    for block in blocks:
        for line in block:
            if line.startswith("DA:"):
                parts = line[3:].split(",", 2)
                line_no = parts[0]
                hits = int(parts[1]) if len(parts) > 1 else 0
                da[line_no] += hits
                if len(parts) > 2:
                    source_lines.add((line_no, parts[2]))
            elif line.startswith("BRDA:"):
                parts = line[5:].split(",")
                key = tuple(parts[:3])
                taken = parts[3] if len(parts) > 3 else "0"
                prev = brda.get(key, "0")
                if taken == "-" or prev == "-":
                    brda[key] = "-"
                else:
                    brda[key] = str(int(prev) + int(taken))
            elif line.startswith("FNDA:"):
                rest = line[5:]
                hits, name = rest.split(",", 1)
                fnda[name] = fnda.get(name, 0) + int(hits)
            elif line.startswith("FN:"):
                fn.add(line)
            elif line.startswith("FNA:"):
                fnab.add(line)
            elif line.startswith(("LF:", "LH:", "BRF:", "BRH:", "FNF:",
                                  "FNH:", "VER:", "TN:")):
                continue  # recomputed below
            elif not line.startswith(("SF:", "end_of_record")):
                header.append(line)

    out = []
    out.extend(sorted(fn))
    out.extend(sorted(fnab))
    for name, hits in sorted(fnda.items()):
        out.append(f"FNDA:{hits},{name}")
    out.append(f"FNF:{len(fnda)}")
    out.append(f"FNH:{sum(1 for h in fnda.values() if h > 0)}")
    for line_no in sorted(da, key=int):
        checksum = next((c for n, c in source_lines if n == line_no), None)
        out.append(f"DA:{line_no},{da[line_no]}" +
                   (f",{checksum}" if checksum else ""))
    out.append(f"LF:{len(da)}")
    out.append(f"LH:{sum(1 for h in da.values() if h > 0)}")
    for key in sorted(brda):
        out.append("BRDA:" + ",".join(key) + "," + brda[key])
    brf = len(brda)
    brh = sum(1 for v in brda.values() if v != "-" and int(v) > 0)
    out.append(f"BRF:{brf}")
    out.append(f"BRH:{brh}")
    return out


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("-i", "--input", required=True)
    ap.add_argument("-o", "--output", required=True)
    args = ap.parse_args()

    text = open(args.input, encoding="utf-8", errors="replace").read()
    grouped = defaultdict(list)
    for sf, block in parse_records(text):
        grouped[sf].append(block)

    merged = []
    for sf in sorted(grouped):
        merged.append(f"SF:{sf}")
        merged.extend(merge(grouped[sf]))
        merged.append("end_of_record")

    with open(args.output, "w", encoding="utf-8") as f:
        f.write("\n".join(merged) + "\n")

    print(f"merged {sum(len(v) for v in grouped.values())} records "
          f"into {len(grouped)} source files")


if __name__ == "__main__":
    main()

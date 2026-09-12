#!/usr/bin/env python3
"""
Performance regression check script.
Compares benchmark results against baseline and fails if regression exceeds threshold.

Usage:
    python3 regression_check.py benchmark-raw.csv --baseline baseline.json --threshold 0.10
"""

import csv
import json
import sys
import argparse
import statistics

# Default baseline (CI runner, 83K RPS tier)
DEFAULT_BASELINE = {
    "/": 83000,
    "/json": 81000,
    "/large": 8800,  # v2.7.2 零拷贝优化后基线（#378）
}

DEFAULT_THRESHOLD = 0.10  # 10% regression threshold

def parse_benchmark_csv(csv_path):
    """Parse benchmark-raw.csv and compute per-endpoint stats."""
    results = {}
    with open(csv_path, 'r') as f:
        reader = csv.DictReader(f)
        for row in reader:
            ep = row['endpoint']
            try:
                rps = float(row['rps'])
            except (ValueError, KeyError):
                continue
            results.setdefault(ep, []).append(rps)
    
    stats = {}
    for ep, values in results.items():
        if len(values) < 3:
            print(f"WARNING: Only {len(values)} rounds for {ep}, using all as median")
        stats[ep] = {
            'median': statistics.median(values),
            'mean': statistics.mean(values),
            'min': min(values),
            'max': max(values),
            'n': len(values),
            'values': values,
        }
    return stats

def check_regression(results, baseline, threshold):
    """Check if any endpoint regresses beyond threshold.

    Endpoints in `results` without a baseline entry are reported but never
    gated: they have no reference value yet (e.g. newly added benchmark
    targets /sse, /stream, /ws_connect, /ws_echo). Once a baseline is added
    to DEFAULT_BASELINE (or a baseline JSON), they start gating normally.
    """
    failures = []
    for ep in sorted(results):
        if ep not in baseline:
            med = results[ep]['median']
            print(f"INFO: {ep} — {med:.0f} RPS (no baseline, report only)")
    for ep, base_rps in baseline.items():
        if ep not in results:
            print(f"SKIP: {ep} not in benchmark results")
            continue
        actual = results[ep]['median']
        limit = base_rps * (1 - threshold)
        ratio = actual / base_rps * 100
        if actual < limit:
            failures.append({
                'endpoint': ep,
                'baseline': base_rps,
                'actual': actual,
                'limit': limit,
                'ratio': ratio,
            })
            print(f"FAIL: {ep} — {actual:.0f} RPS (baseline {base_rps:.0f}, limit {limit:.0f}, {ratio:.1f}%)")
        else:
            print(f"PASS: {ep} — {actual:.0f} RPS ({ratio:.1f}% of baseline)")
    return failures

def main():
    parser = argparse.ArgumentParser(description='Performance regression check')
    parser.add_argument('csv', help='Path to benchmark-raw.csv')
    parser.add_argument('--baseline', help='Path to baseline JSON (default: built-in)')
    parser.add_argument('--threshold', type=float, default=DEFAULT_THRESHOLD,
                        help=f'Regression threshold (default: {DEFAULT_THRESHOLD})')
    args = parser.parse_args()
    
    # Load baseline
    if args.baseline:
        with open(args.baseline) as f:
            baseline = json.load(f)
    else:
        baseline = DEFAULT_BASELINE
    
    # Parse results
    results = parse_benchmark_csv(args.csv)
    
    if not results:
        print("ERROR: No benchmark results found in CSV")
        sys.exit(2)
    
    print(f"\n=== Regression Check (threshold: {args.threshold*100:.0f}%) ===\n")
    failures = check_regression(results, baseline, args.threshold)
    
    if failures:
        print(f"\n=== FAILED: {len(failures)} endpoint(s) regressed ===")
        sys.exit(1)
    else:
        print(f"\n=== ALL PASS ===")
        sys.exit(0)

if __name__ == '__main__':
    main()

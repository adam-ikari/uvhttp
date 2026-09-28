#!/usr/bin/env python3
"""
Performance regression check script.

Modes:
    paired (what the PR/release gate should use) — head and base are measured
    on the SAME runner, interleaved in the same job, and an endpoint fails
    when the median of its per-round head/base ratios drops below the
    threshold. The absolute baseline is reported but does not decide the gate:
    GitHub-hosted runners show ~40% between-run variance on these endpoints
    (within-run CV stays under 5%), so a fixed RPS baseline ends up gating
    runner luck instead of code.
    python3 regression_check.py head.csv --compare base.csv --threshold 0.10

    absolute (legacy default) — compare against built-in or JSON baseline:
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
MIN_PAIRED_SAMPLES = 3    # fewer paired rounds than this is noise, report only

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

def parse_by_round(csv_path):
    """Parse benchmark-raw.csv into {endpoint: {round: rps}}.

    The round key is what pairs a head sample with the base sample measured
    next to it; rows without a numeric round fall back to their position.
    """
    rounds = {}
    with open(csv_path, 'r') as f:
        reader = csv.DictReader(f)
        for pos, row in enumerate(reader, start=1):
            ep = row.get('endpoint')
            try:
                rps = float(row['rps'])
            except (ValueError, KeyError, TypeError):
                continue
            try:
                key = int(float(row.get('round', '')))
            except (ValueError, TypeError):
                key = pos
            rounds.setdefault(ep, {})[key] = rps
    return rounds

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

def check_paired(head_rounds, base_rounds, threshold, gated):
    """Gate head against base measured on the same runner, round by round.

    Round i of head is paired with round i of base (the harness alternates
    the two binaries), so the runner's slow and fast phases hit both sides and
    cancel out. An endpoint fails when the median of its head/base ratios
    drops below 1 - threshold.
    """
    failures = []
    print(f"{'endpoint':<16} {'head med':>10} {'base med':>10} {'ratio':>8}   verdict")
    print("-" * 68)
    for ep in sorted(head_rounds):
        common = sorted(set(head_rounds[ep]) & set(base_rounds.get(ep, {})))
        head_all = [head_rounds[ep][r] for r in sorted(head_rounds[ep])]
        base_all = [base_rounds[ep][r] for r in sorted(base_rounds.get(ep, {}))]
        head_med = statistics.median(head_all)
        base_med = statistics.median(base_all) if base_all else None
        shown_base = f"{base_med:>10.0f}" if base_med is not None else f"{'-':>10}"
        if ep not in gated:
            print(f"{ep:<16} {head_med:>10.0f} {shown_base} {'-':>8}   report only")
            continue
        if base_med is None or base_med <= 0:
            failures.append({
                'endpoint': ep,
                'head': head_med,
                'base': None,
                'ratio': None,
                'pairs': 0,
                'reason': 'no base data',
            })
            print(f"{ep:<16} {head_med:>10.0f} {shown_base} {'-':>8}   FAIL (no base data)")
            continue
        ratios = [head_rounds[ep][r] / base_rounds[ep][r] for r in common
                  if base_rounds[ep][r] > 0]
        ratio = statistics.median(ratios) if ratios else head_med / base_med
        ratio_pct = ratio * 100
        limit_pct = (1 - threshold) * 100
        if len(ratios) < MIN_PAIRED_SAMPLES:
            # Fail closed: an inconclusive measurement is not a passing gate.
            failures.append({
                'endpoint': ep,
                'head': head_med,
                'base': base_med,
                'ratio': ratio_pct,
                'pairs': len(ratios),
                'reason': f'only {len(ratios)} paired round(s)',
            })
            print(f"{ep:<16} {head_med:>10.0f} {base_med:>10.0f} {ratio_pct:>7.1f}%"
                  f"   FAIL (only {len(ratios)} pairs, need {MIN_PAIRED_SAMPLES})")
            continue
        if ratio < 1 - threshold:
            failures.append({
                'endpoint': ep,
                'head': head_med,
                'base': base_med,
                'ratio': ratio_pct,
                'pairs': len(ratios),
                'reason': f'{ratio_pct:.1f}% of base, limit {limit_pct:.0f}%',
            })
            print(f"{ep:<16} {head_med:>10.0f} {base_med:>10.0f} {ratio_pct:>7.1f}%"
                  f"   FAIL (< {limit_pct:.0f}%)")
        else:
            print(f"{ep:<16} {head_med:>10.0f} {base_med:>10.0f} {ratio_pct:>7.1f}%"
                  f"   PASS ({len(ratios)} pairs)")
    return failures

def main():
    parser = argparse.ArgumentParser(description='Performance regression check')
    parser.add_argument('csv', help='Path to benchmark-raw.csv')
    parser.add_argument('--baseline', help='Path to baseline JSON (default: built-in)')
    parser.add_argument('--compare', metavar='BASE_CSV',
                        help='benchmark-raw.csv of the base revision measured on '
                             'the same runner; enables paired gating')
    parser.add_argument('--threshold', type=float, default=DEFAULT_THRESHOLD,
                        help=f'Regression threshold (default: {DEFAULT_THRESHOLD})')
    parser.add_argument('--gate', default=','.join(DEFAULT_BASELINE),
                        help='Comma-separated endpoints paired mode gates on; the '
                             'rest are reported (default: built-in baseline keys)')
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
    
    if args.compare:
        head_rounds = parse_by_round(args.csv)
        base_rounds = parse_by_round(args.compare)
        if not head_rounds or not base_rounds:
            print("ERROR: Paired mode needs round data in both CSVs")
            sys.exit(2)
        gated = [ep.strip() for ep in args.gate.split(',') if ep.strip()]
        print(f"\n=== Paired gate: head vs {args.compare} (threshold: "
              f"{args.threshold*100:.0f}%) ===\n")
        failures = check_paired(head_rounds, base_rounds, args.threshold, gated)
        print("\n=== Absolute baseline (report only) ===\n")
        check_regression(results, baseline, args.threshold)
    else:
        print(f"\n=== Regression Check (threshold: {args.threshold*100:.0f}%) ===\n")
        failures = check_regression(results, baseline, args.threshold)
    
    if failures:
        print(f"\n=== FAILED: {len(failures)} endpoint(s) ===")
        for f in failures:
            if 'pairs' in f:
                print(f"  - {f['endpoint']}: {f.get('reason', 'regressed')}")
        sys.exit(1)
    else:
        print(f"\n=== ALL PASS ===")
        sys.exit(0)

if __name__ == '__main__':
    main()

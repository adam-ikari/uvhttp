#!/usr/bin/env python3
"""
Performance regression detection script for UVHTTP

This script compares current performance results with baseline
and detects performance regressions and improvements.
"""

import json
import sys
import argparse
from typing import Dict, List, Any, Optional
from dataclasses import dataclass


@dataclass
class RegressionResult:
    """Regression detection result"""
    scenario: str
    metric: str
    baseline: float
    current: float
    change: float
    severity: str  # 'failure', 'warning', 'info'


@dataclass
class PerformanceReport:
    """Performance regression report"""
    regressions: List[RegressionResult]
    improvements: List[RegressionResult]
    warnings: List[RegressionResult]
    info: List[RegressionResult]

    def has_failure(self) -> bool:
        """Check if there are any failures"""
        return any(r.severity == 'failure' for r in self.regressions)

    def has_warning(self) -> bool:
        """Check if there are any warnings"""
        return len(self.warnings) > 0

    def has_improvement(self) -> bool:
        """Check if there are any improvements"""
        return len(self.improvements) > 0


class PerformanceBaseline:
    """Performance baseline manager"""

    def __init__(self, baseline_file: str):
        """Initialize baseline manager"""
        self.baseline_file = baseline_file
        self.baseline = self._load_baseline()

    def _load_baseline(self) -> Dict[str, Any]:
        """Load baseline from file"""
        try:
            with open(self.baseline_file, 'r') as f:
                return json.load(f)
        except FileNotFoundError:
            print(f"Warning: Baseline file not found: {self.baseline_file}")
            return {}
        except json.JSONDecodeError as e:
            print(f"Error: Invalid JSON in baseline file: {e}")
            return {}

    def get_scenario_baseline(self, scenario_name: str) -> Optional[Dict[str, Any]]:
        """Get baseline for a specific scenario"""
        scenarios = self.baseline.get('scenarios', {})
        return scenarios.get(scenario_name)


class PerformanceThresholds:
    """Performance thresholds configuration"""

    DEFAULT_THRESHOLDS = {
        'rps_warning': 0.10,   # 10% decrease triggers warning
        'rps_failure': 0.10,   # 10% decrease triggers failure
        'latency_warning': 0.10,  # 10% increase triggers warning
        'latency_failure': 0.20,  # 20% increase triggers failure
        'min_improvement': 0.05    # 5% improvement is considered significant
    }

    def __init__(self, thresholds: Optional[Dict[str, float]] = None):
        """Initialize thresholds"""
        self.thresholds = thresholds or self.DEFAULT_THRESHOLDS.copy()

    @classmethod
    def from_file(cls, file_path: str) -> 'PerformanceThresholds':
        """Load thresholds from file"""
        try:
            with open(file_path, 'r') as f:
                data = json.load(f)
                thresholds = data.get('thresholds', {})
                return cls(thresholds)
        except (FileNotFoundError, json.JSONDecodeError):
            return cls()


class PerformanceComparator:
    """Performance result comparator"""

    def __init__(
        self,
        baseline: PerformanceBaseline,
        thresholds: PerformanceThresholds
    ):
        """Initialize comparator"""
        self.baseline = baseline
        self.thresholds = thresholds

    def compare(self, current_results: Dict[str, Any]) -> PerformanceReport:
        """Compare current results with baseline"""
        report = PerformanceReport(
            regressions=[],
            improvements=[],
            warnings=[],
            info=[]
        )

        test_scenarios = current_results.get('test_scenarios', [])

        for scenario in test_scenarios:
            scenario_name = scenario['name']
            scenario_results = scenario.get('results', {})

            # Get baseline for this scenario
            baseline_scenario = self.baseline.get_scenario_baseline(scenario_name)
            if not baseline_scenario:
                continue

            # Check RPS
            self._check_rps(
                scenario_name,
                scenario_results,
                baseline_scenario,
                report
            )

            # Check latency
            self._check_latency(
                scenario_name,
                scenario_results,
                baseline_scenario,
                report
            )

        return report

    def _check_rps(
        self,
        scenario_name: str,
        current_results: Dict[str, Any],
        baseline_scenario: Dict[str, Any],
        report: PerformanceReport
    ):
        """Check RPS regression"""
        current_rps = current_results.get('rps', {}).get('value', 0)
        baseline_rps = baseline_scenario.get('rps', 0)

        if baseline_rps == 0:
            return

        rps_change = (current_rps - baseline_rps) / baseline_rps

        # Check for regression
        if rps_change < -self.thresholds.thresholds['rps_failure']:
            report.regressions.append(RegressionResult(
                scenario=scenario_name,
                metric='rps',
                baseline=baseline_rps,
                current=current_rps,
                change=rps_change * 100,
                severity='failure'
            ))
        elif rps_change < -self.thresholds.thresholds['rps_warning']:
            report.warnings.append(RegressionResult(
                scenario=scenario_name,
                metric='rps',
                baseline=baseline_rps,
                current=current_rps,
                change=rps_change * 100,
                severity='warning'
            ))
        # Check for improvement
        elif rps_change > self.thresholds.thresholds['min_improvement']:
            report.improvements.append(RegressionResult(
                scenario=scenario_name,
                metric='rps',
                baseline=baseline_rps,
                current=current_rps,
                change=rps_change * 100,
                severity='info'
            ))

    def _check_latency(
        self,
        scenario_name: str,
        current_results: Dict[str, Any],
        baseline_scenario: Dict[str, Any],
        report: PerformanceReport
    ):
        """Check latency regression"""
        # Check P99 latency
        current_p99 = current_results.get('latency_p99', {}).get('value', 0)
        baseline_p99 = baseline_scenario.get('latency_p99', 0)

        if baseline_p99 == 0:
            return

        latency_change = (current_p99 - baseline_p99) / baseline_p99

        # Check for regression
        if latency_change > self.thresholds.thresholds['latency_failure']:
            report.regressions.append(RegressionResult(
                scenario=scenario_name,
                metric='latency_p99',
                baseline=baseline_p99,
                current=current_p99,
                change=latency_change * 100,
                severity='failure'
            ))
        elif latency_change > self.thresholds.thresholds['latency_warning']:
            report.warnings.append(RegressionResult(
                scenario=scenario_name,
                metric='latency_p99',
                baseline=baseline_p99,
                current=current_p99,
                change=latency_change * 100,
                severity='warning'
            ))


class ReportGenerator:
    """Performance report generator"""

    @staticmethod
    def generate_markdown(report: PerformanceReport) -> str:
        """Generate markdown report"""
        md = "# Performance Regression Report\n\n"

        # Add regressions
        if report.regressions:
            md += "## ❌ Regressions Detected\n\n"
            md += "| Scenario | Metric | Baseline | Current | Change |\n"
            md += "|----------|--------|----------|---------|--------|\n"
            for reg in report.regressions:
                icon = "🔴" if reg.severity == 'failure' else "🟡"
                md += f"| {icon} {reg.scenario} | {reg.metric} | "
                md += f"{reg.baseline:.2f} | {reg.current:.2f} | "
                md += f"{reg.change:+.2f}% |\n"
            md += "\n"

        # Add warnings
        if report.warnings:
            md += "## ⚠️ Warnings\n\n"
            md += "| Scenario | Metric | Baseline | Current | Change |\n"
            md += "|----------|--------|----------|---------|--------|\n"
            for warn in report.warnings:
                md += f"| 🟡 {warn.scenario} | {warn.metric} | "
                md += f"{warn.baseline:.2f} | {warn.current:.2f} | "
                md += f"{warn.change:+.2f}% |\n"
            md += "\n"

        # Add improvements
        if report.improvements:
            md += "## ✅ Improvements\n\n"
            md += "| Scenario | Metric | Baseline | Current | Change |\n"
            md += "|----------|--------|----------|---------|--------|\n"
            for imp in report.improvements:
                md += f"| 🟢 {imp.scenario} | {imp.metric} | "
                md += f"{imp.baseline:.2f} | {imp.current:.2f} | "
                md += f"{imp.change:+.2f}% |\n"
            md += "\n"

        # Add summary
        md += "## Summary\n\n"
        md += f"- Regressions: {len(report.regressions)}\n"
        md += f"- Warnings: {len(report.warnings)}\n"
        md += f"- Improvements: {len(report.improvements)}\n"

        # Add overall status
        if report.has_failure():
            md += "\n**Overall Status**: ❌ FAILED - Performance regression detected\n"
        elif report.has_warning():
            md += "\n**Overall Status**: ⚠️ WARNING - Performance degradation detected\n"
        elif report.has_improvement():
            md += "\n**Overall Status**: ✅ PASSED - Performance improvements detected\n"
        else:
            md += "\n**Overall Status**: ✅ PASSED - No significant changes\n"

        return md

    @staticmethod
    def generate_json(report: PerformanceReport) -> Dict[str, Any]:
        """Generate JSON report"""
        return {
            'summary': {
                'regressions': len(report.regressions),
                'warnings': len(report.warnings),
                'improvements': len(report.improvements),
                'has_failure': report.has_failure(),
                'has_warning': report.has_warning(),
                'has_improvement': report.has_improvement()
            },
            'regressions': [
                {
                    'scenario': r.scenario,
                    'metric': r.metric,
                    'baseline': r.baseline,
                    'current': r.current,
                    'change': r.change,
                    'severity': r.severity
                }
                for r in report.regressions
            ],
            'warnings': [
                {
                    'scenario': w.scenario,
                    'metric': w.metric,
                    'baseline': w.baseline,
                    'current': w.current,
                    'change': w.change,
                    'severity': w.severity
                }
                for w in report.warnings
            ],
            'improvements': [
                {
                    'scenario': i.scenario,
                    'metric': i.metric,
                    'baseline': i.baseline,
                    'current': i.current,
                    'change': i.change,
                    'severity': i.severity
                }
                for i in report.improvements
            ]
        }


def load_json_file(filepath: str) -> Dict[str, Any]:
    """Load JSON file"""
    with open(filepath, 'r') as f:
        return json.load(f)


def main():
    """Main function"""
    parser = argparse.ArgumentParser(
        description='Detect performance regression in UVHTTP'
    )
    parser.add_argument(
        'current',
        help='Current performance results JSON file'
    )
    parser.add_argument(
        'baseline',
        help='Baseline performance results JSON file'
    )
    parser.add_argument(
        '--output',
        help='Output file (default: stdout)'
    )
    parser.add_argument(
        '--format',
        choices=['markdown', 'json', 'both'],
        default='markdown',
        help='Output format'
    )
    parser.add_argument(
        '--thresholds',
        help='Thresholds configuration file'
    )
    parser.add_argument(
        '--fail-on-regression',
        action='store_true',
        help='Exit with error if regression detected'
    )

    args = parser.parse_args()

    # Load current results
    try:
        current_results = load_json_file(args.current)
    except FileNotFoundError:
        print(f"Error: Current results file not found: {args.current}", file=sys.stderr)
        sys.exit(1)
    except json.JSONDecodeError as e:
        print(f"Error: Invalid JSON in current results file: {e}", file=sys.stderr)
        sys.exit(1)

    # Load baseline
    baseline = PerformanceBaseline(args.baseline)

    # Load thresholds
    if args.thresholds:
        thresholds = PerformanceThresholds.from_file(args.thresholds)
    else:
        thresholds = PerformanceThresholds()

    # Compare results
    comparator = PerformanceComparator(baseline, thresholds)
    report = comparator.compare(current_results)

    # Generate output
    if args.format in ['markdown', 'both']:
        markdown_report = ReportGenerator.generate_markdown(report)
        if args.output and args.format == 'markdown':
            with open(args.output, 'w') as f:
                f.write(markdown_report)
        else:
            print(markdown_report)

    if args.format in ['json', 'both']:
        json_report = ReportGenerator.generate_json(report)
        json_output = json.dumps(json_report, indent=2)
        if args.output and args.format == 'json':
            output_file = args.output
            if args.format == 'both':
                output_file = args.output.replace('.md', '.json')
            with open(output_file, 'w') as f:
                f.write(json_output)
        else:
            print(json_output)

    # Exit with error if regression detected
    if args.fail_on_regression and report.has_failure():
        sys.exit(1)


if __name__ == '__main__':
    main()
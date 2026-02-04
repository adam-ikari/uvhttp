#!/usr/bin/env python3
"""
UVHTTP 性能回归检测脚本

这个脚本比较当前测试结果与基准值，检测性能回归。
如果性能下降超过阈值，会发出警告或失败信号。
"""

import os
import sys
import json
import yaml
import argparse
from pathlib import Path
from datetime import datetime
from typing import Dict, List, Optional, Tuple

# 项目根目录
PROJECT_ROOT = Path(__file__).parent.parent
BASELINE_FILE = PROJECT_ROOT / "config" / "performance-baseline.yml"


class PerformanceRegressionDetector:
    def __init__(self, baseline_file: Path = BASELINE_FILE):
        self.baseline_file = baseline_file
        self.baseline = self._load_baseline()
        self.current_results = {}
        self.regressions = []
        self.warnings = []
        self.improvements = []

    def _load_baseline(self) -> Dict:
        """加载性能基准值"""
        if not self.baseline_file.exists():
            print(f"警告: 基准文件不存在: {self.baseline_file}")
            return {}

        with open(self.baseline_file, 'r') as f:
            return yaml.safe_load(f)

    def load_results(self, results_file: Path):
        """加载测试结果"""
        print(f"加载测试结果: {results_file}")

        if results_file.suffix == '.csv':
            self._load_csv_results(results_file)
        elif results_file.suffix == '.json':
            self._load_json_results(results_file)
        elif results_file.suffix == '.yml' or results_file.suffix == '.yaml':
            self._load_yaml_results(results_file)
        else:
            print(f"错误: 不支持的结果文件格式: {results_file.suffix}")
            sys.exit(1)

        print(f"加载完成，共加载 {len(self.current_results)} 个测试结果")

    def _load_csv_results(self, file: Path):
        """加载 CSV 格式的结果"""
        import csv

        with open(file, 'r') as f:
            reader = csv.DictReader(f)
            for row in reader:
                test_name = row.get('test_name', row.get('Endpoint', ''))
                rps = float(row.get('RPS', row.get('rps', 0)))
                if test_name and rps > 0:
                    self.current_results[test_name] = {'rps': rps}

    def _load_json_results(self, file: Path):
        """加载 JSON 格式的结果"""
        with open(file, 'r') as f:
            self.current_results = json.load(f)

    def _load_yaml_results(self, file: Path):
        """加载 YAML 格式的结果"""
        with open(file, 'r') as f:
            self.current_results = yaml.safe_load(f)

    def detect_regression(self):
        """检测性能回归"""
        print("检测性能回归...")

        if not self.baseline:
            print("警告: 没有基准值，跳过回归检测")
            return

        baseline_data = self.baseline.get('baseline', {})
        thresholds = self.baseline.get('thresholds', {})

        # 检查 RPS 回归
        self._check_rps_regression(baseline_data, thresholds)

        # 检查延迟回归
        self._check_latency_regression(baseline_data, thresholds)

        # 检查资源使用回归
        self._check_resource_regression(baseline_data, thresholds)

        print("回归检测完成")

    def _check_rps_regression(self, baseline_data: Dict, thresholds: Dict):
        """检查 RPS 回归"""
        rps_threshold = thresholds.get('rps', {}).get('failure', 0.10)

        # 检查低并发
        self._compare_metric(
            'low_concurrent',
            'rps',
            baseline_data.get('low_concurrent', {}).get('rps', 0),
            rps_threshold,
            higher_is_better=True
        )

        # 检查中等并发
        self._compare_metric(
            'medium_concurrent',
            'rps',
            baseline_data.get('medium_concurrent', {}).get('rps', 0),
            rps_threshold,
            higher_is_better=True
        )

        # 检查高并发
        self._compare_metric(
            'high_concurrent',
            'rps',
            baseline_data.get('high_concurrent', {}).get('rps', 0),
            rps_threshold,
            higher_is_better=True
        )

        # 检查极端并发
        self._compare_metric(
            'extreme_concurrent',
            'rps',
            baseline_data.get('extreme_concurrent', {}).get('rps', 0),
            rps_threshold,
            higher_is_better=True
        )

    def _check_latency_regression(self, baseline_data: Dict, thresholds: Dict):
        """检查延迟回归"""
        latency_threshold = thresholds.get('latency', {}).get('failure', 0.20)

        # 检查各并发级别的延迟
        for level in ['low_concurrent', 'medium_concurrent', 'high_concurrent', 'extreme_concurrent']:
            level_data = baseline_data.get(level, {})

            # 检查平均延迟
            self._compare_metric(
                f'{level}_avg_latency',
                'latency_avg',
                level_data.get('latency_avg', 0),
                latency_threshold,
                higher_is_better=False
            )

            # 检查 P99 延迟
            self._compare_metric(
                f'{level}_p99_latency',
                'latency_p99',
                level_data.get('latency_p99', 0),
                latency_threshold,
                higher_is_better=False
            )

    def _check_resource_regression(self, baseline_data: Dict, thresholds: Dict):
        """检查资源使用回归"""
        resource_threshold = thresholds.get('resources', {}).get('failure', 0.20)

        # 检查 CPU 使用率
        self._compare_metric(
            'cpu_usage',
            'cpu_usage',
            self.baseline.get('resources', {}).get('cpu_usage', {}).get('baseline', 0),
            resource_threshold,
            higher_is_better=False
        )

        # 检查内存使用
        self._compare_metric(
            'memory_usage',
            'memory_usage',
            self.baseline.get('resources', {}).get('memory_usage', {}).get('baseline', 0),
            resource_threshold,
            higher_is_better=False
        )

    def _compare_metric(
        self,
        metric_name: str,
        metric_key: str,
        baseline_value: float,
        threshold: float,
        higher_is_better: bool
    ):
        """比较指标值"""
        if baseline_value == 0:
            return

        # 查找当前值
        current_value = self._find_current_value(metric_name, metric_key)
        if current_value is None:
            return

        # 计算变化百分比
        if higher_is_better:
            change_percent = (baseline_value - current_value) / baseline_value
        else:
            change_percent = (current_value - baseline_value) / baseline_value

        # 判断是否回归
        if change_percent > threshold:
            self.regressions.append({
                'metric': metric_name,
                'baseline': baseline_value,
                'current': current_value,
                'change_percent': change_percent * 100,
                'threshold': threshold * 100,
                'higher_is_better': higher_is_better
            })
        elif change_percent > threshold * 0.5:
            self.warnings.append({
                'metric': metric_name,
                'baseline': baseline_value,
                'current': current_value,
                'change_percent': change_percent * 100,
                'threshold': threshold * 100,
                'higher_is_better': higher_is_better
            })
        elif change_percent < -threshold * 0.5:
            self.improvements.append({
                'metric': metric_name,
                'baseline': baseline_value,
                'current': current_value,
                'change_percent': change_percent * 100,
                'higher_is_better': higher_is_better
            })

    def _find_current_value(self, metric_name: str, metric_key: str) -> Optional[float]:
        """查找当前值"""
        # 尝试直接匹配
        if metric_name in self.current_results:
            if metric_key in self.current_results[metric_name]:
                return float(self.current_results[metric_name][metric_key])

        # 尝试模糊匹配
        for key, value in self.current_results.items():
            if metric_key in value:
                return float(value[metric_key])

        return None

    def generate_report(self, output_file: Optional[Path] = None):
        """生成回归检测报告"""
        print("生成回归检测报告...")

        report = {
            'timestamp': datetime.now().isoformat(),
            'baseline_file': str(self.baseline_file),
            'regressions': self.regressions,
            'warnings': self.warnings,
            'improvements': self.improvements,
            'summary': self._generate_summary()
        }

        # 打印报告
        self._print_report(report)

        # 保存报告
        if output_file:
            with open(output_file, 'w') as f:
                json.dump(report, f, indent=2)
            print(f"报告已保存: {output_file}")

        return report

    def _generate_summary(self) -> Dict:
        """生成摘要"""
        return {
            'total_regressions': len(self.regressions),
            'total_warnings': len(self.warnings),
            'total_improvements': len(self.improvements),
            'has_regression': len(self.regressions) > 0,
            'has_warning': len(self.warnings) > 0,
            'has_improvement': len(self.improvements) > 0,
            'status': self._get_status()
        }

    def _get_status(self) -> str:
        """获取状态"""
        if len(self.regressions) > 0:
            return 'failure'
        elif len(self.warnings) > 0:
            return 'warning'
        elif len(self.improvements) > 0:
            return 'improvement'
        else:
            return 'pass'

    def _print_report(self, report: Dict):
        """打印报告"""
        print("\n" + "=" * 80)
        print("性能回归检测报告")
        print("=" * 80)
        print(f"检测时间: {report['timestamp']}")
        print(f"基准文件: {report['baseline_file']}")
        print()

        # 打印回归
        if report['summary']['total_regressions'] > 0:
            print(" 性能回归 (需要修复):")
            print("-" * 80)
            for regression in report['regressions']:
                direction = "下降" if regression['higher_is_better'] else "增加"
                print(f"  指标: {regression['metric']}")
                print(f"  基准值: {regression['baseline']}")
                print(f"  当前值: {regression['current']}")
                print(f"  变化: {regression['change_percent']:.2f}% ({direction})")
                print(f"  阈值: {regression['threshold']:.2f}%")
                print()
        else:
            print(" 未检测到性能回归")

        # 打印警告
        if report['summary']['total_warnings'] > 0:
            print("\n  性能警告 (需要关注):")
            print("-" * 80)
            for warning in report['warnings']:
                direction = "下降" if warning['higher_is_better'] else "增加"
                print(f"  指标: {warning['metric']}")
                print(f"  基准值: {warning['baseline']}")
                print(f"  当前值: {warning['current']}")
                print(f"  变化: {warning['change_percent']:.2f}% ({direction})")
                print(f"  阈值: {warning['threshold']:.2f}%")
                print()

        # 打印改进
        if report['summary']['total_improvements'] > 0:
            print("\n🎉 性能改进:")
            print("-" * 80)
            for improvement in report['improvements']:
                direction = "提升" if improvement['higher_is_better'] else "降低"
                print(f"  指标: {improvement['metric']}")
                print(f"  基准值: {improvement['baseline']}")
                print(f"  当前值: {improvement['current']}")
                print(f"  变化: {abs(improvement['change_percent']):.2f}% ({direction})")
                print()

        # 打印摘要
        print("\n" + "=" * 80)
        print("摘要")
        print("=" * 80)
        print(f"回归数量: {report['summary']['total_regressions']}")
        print(f"警告数量: {report['summary']['total_warnings']}")
        print(f"改进数量: {report['summary']['total_improvements']}")
        print(f"状态: {report['summary']['status'].upper()}")
        print("=" * 80)

    def exit_with_status(self):
        """根据检测结果退出"""
        summary = self._generate_summary()

        if summary['has_regression']:
            print("\n 检测到性能回归，退出码: 1")
            sys.exit(1)
        elif summary['has_warning']:
            print("\n  检测到性能警告，退出码: 2")
            sys.exit(2)
        else:
            print("\n 性能检测通过，退出码: 0")
            sys.exit(0)


def main():
    parser = argparse.ArgumentParser(description='UVHTTP 性能回归检测')
    parser.add_argument('results_file', type=Path, help='测试结果文件 (CSV/JSON/YAML)')
    parser.add_argument('--baseline', type=Path, default=BASELINE_FILE,
                        help='基准文件路径 (默认: config/performance-baseline.yml)')
    parser.add_argument('--output', type=Path, help='输出报告文件路径')
    parser.add_argument('--fail-on-regression', action='store_true',
                        help='检测到回归时退出码为 1')

    args = parser.parse_args()

    # 创建检测器
    detector = PerformanceRegressionDetector(args.baseline)

    # 加载结果
    detector.load_results(args.results_file)

    # 检测回归
    detector.detect_regression()

    # 生成报告
    detector.generate_report(args.output)

    # 根据检测结果退出
    if args.fail_on_regression:
        detector.exit_with_status()


if __name__ == "__main__":
    main()
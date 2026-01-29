#!/usr/bin/env python3
"""
性能回归检测脚本
"""

import json
import sys
from typing import Dict, List, Tuple

# 性能回归阈值配置
THRESHOLDS = {
    'rps': {
        'warning': 0.10,  # 10% 下降
        'failure': 0.10   # 10% 下降
    },
    'latency': {
        'warning': 0.10,  # 10% 增加
        'failure': 0.20   # 20% 增加
    },
    'resources': {
        'warning': 0.10,  # 10% 增加
        'failure': 0.20   # 20% 增加
    }
}

# 性能基准值
BASELINE = {
    'low_concurrent': {
        'rps': 17798,
        'latency_avg': 518,  # μs
        'latency_p99': 1600,  # μs
    },
    'medium_concurrent': {
        'rps': 17209,
        'latency_avg': 2790,  # μs (2.79 ms)
        'latency_p99': 8500,  # μs (8.5 ms)
    },
    'high_concurrent': {
        'rps': 16623,
        'latency_avg': 12200,  # μs (12.2 ms)
        'latency_p99': 40000,  # μs (40 ms)
    }
}

def detect_regression(current_data: Dict) -> Tuple[List[Dict], List[Dict], List[Dict]]:
    """
    检测性能回归
    
    Args:
        current_data: 当前测试数据
    
    Returns:
        (failures, warnings, improvements)
    """
    failures = []
    warnings = []
    improvements = []
    
    for scenario in current_data.get('test_scenarios', []):
        scenario_name = scenario['name']
        results = scenario['results']
        
        if scenario_name not in BASELINE:
            continue
        
        baseline = BASELINE[scenario_name]
        
        # 检查吞吐量
        current_rps = results['rps']['value']
        baseline_rps = baseline['rps']
        rps_change = (current_rps - baseline_rps) / baseline_rps
        
        if rps_change < -THRESHOLDS['rps']['failure']:
            failures.append({
                'scenario': scenario_name,
                'metric': 'RPS',
                'current': current_rps,
                'baseline': baseline_rps,
                'change': rps_change * 100,
                'severity': 'failure'
            })
        elif rps_change < -THRESHOLDS['rps']['warning']:
            warnings.append({
                'scenario': scenario_name,
                'metric': 'RPS',
                'current': current_rps,
                'baseline': baseline_rps,
                'change': rps_change * 100,
                'severity': 'warning'
            })
        elif rps_change > THRESHOLDS['rps']['warning']:
            improvements.append({
                'scenario': scenario_name,
                'metric': 'RPS',
                'current': current_rps,
                'baseline': baseline_rps,
                'change': rps_change * 100,
                'severity': 'improvement'
            })
        
        # 检查平均延迟
        current_latency_avg = results['latency_avg']['value']
        baseline_latency_avg = baseline['latency_avg']
        latency_avg_change = (current_latency_avg - baseline_latency_avg) / baseline_latency_avg
        
        if latency_avg_change > THRESHOLDS['latency']['failure']:
            failures.append({
                'scenario': scenario_name,
                'metric': 'Average Latency',
                'current': current_latency_avg,
                'baseline': baseline_latency_avg,
                'change': latency_avg_change * 100,
                'severity': 'failure'
            })
        elif latency_avg_change > THRESHOLDS['latency']['warning']:
            warnings.append({
                'scenario': scenario_name,
                'metric': 'Average Latency',
                'current': current_latency_avg,
                'baseline': baseline_latency_avg,
                'change': latency_avg_change * 100,
                'severity': 'warning'
            })
        elif latency_avg_change < -THRESHOLDS['latency']['warning']:
            improvements.append({
                'scenario': scenario_name,
                'metric': 'Average Latency',
                'current': current_latency_avg,
                'baseline': baseline_latency_avg,
                'change': latency_avg_change * 100,
                'severity': 'improvement'
            })
    
    return failures, warnings, improvements

def generate_report(failures: List[Dict], warnings: List[Dict], improvements: List[Dict]) -> str:
    """
    生成回归检测报告
    
    Args:
        failures: 失败列表
        warnings: 警告列表
        improvements: 改进列表
    
    Returns:
        Markdown 格式的报告
    """
    report = []
    
    if failures:
        report.append("## 🚨 Performance Regressions Detected\n")
        for failure in failures:
            report.append(f"### {failure['scenario']}: {failure['metric']}\n")
            report.append(f"- **Current**: {failure['current']:.0f}\n")
            report.append(f"- **Baseline**: {failure['baseline']:.0f}\n")
            report.append(f"- **Change**: {failure['change']:.2f}%\n")
            report.append(f"- **Severity**: {failure['severity']}\n")
            report.append("\n")
    
    if warnings:
        report.append("## ⚠️ Performance Warnings\n")
        for warning in warnings:
            report.append(f"### {warning['scenario']}: {warning['metric']}\n")
            report.append(f"- **Current**: {warning['current']:.0f}\n")
            report.append(f"- **Baseline**: {warning['baseline']:.0f}\n")
            report.append(f"- **Change**: {warning['change']:.2f}%\n")
            report.append(f"- **Severity**: {warning['severity']}\n")
            report.append("\n")
    
    if improvements:
        report.append("## 🎉 Performance Improvements\n")
        for improvement in improvements:
            report.append(f"### {improvement['scenario']}: {improvement['metric']}\n")
            report.append(f"- **Current**: {improvement['current']:.0f}\n")
            report.append(f"- **Baseline**: {improvement['baseline']:.0f}\n")
            report.append(f"- **Change**: {improvement['change']:.2f}%\n")
            report.append(f"- **Severity**: {improvement['severity']}\n")
            report.append("\n")
    
    if not failures and not warnings and not improvements:
        report.append("## ✅ No Performance Changes Detected\n")
        report.append("All performance metrics are within acceptable ranges.\n")
    
    return ''.join(report)

def main():
    if len(sys.argv) < 2:
        print("Usage: python3 detect_regression.py <performance_results.json>")
        sys.exit(1)
    
    # 加载当前测试数据
    with open(sys.argv[1], 'r') as f:
        current_data = json.load(f)
    
    # 检测回归
    failures, warnings, improvements = detect_regression(current_data)
    
    # 生成报告
    report = generate_report(failures, warnings, improvements)
    print(report)
    
    # 保存报告
    with open('regression_report.md', 'w') as f:
        f.write(report)
    
    # 返回退出码
    if failures:
        sys.exit(1)  # 有回归，返回失败
    elif warnings:
        sys.exit(2)  # 有警告，返回警告
    else:
        sys.exit(0)  # 无问题，返回成功

if __name__ == '__main__':
    main()
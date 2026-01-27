#!/usr/bin/env python3
"""
PR 通知脚本
用于在 PR 中添加 CI/CD 结果评论
"""

import os
import sys
import json
import argparse


def load_performance_results(results_file):
    """加载性能测试结果"""
    try:
        with open(results_file, 'r') as f:
            return json.load(f)
    except Exception as e:
        print(f"Error loading performance results: {e}", file=sys.stderr)
        return None


def load_baseline(baseline_file):
    """加载性能基线"""
    try:
        with open(baseline_file, 'r') as f:
            return json.load(f)
    except Exception as e:
        print(f"Error loading baseline: {e}", file=sys.stderr)
        return None


def compare_performance(current, baseline):
    """比较当前性能与基线"""
    results = {
        'regression': False,
        'improvement': False,
        'details': []
    }
    
    if not current or not baseline:
        return results
    
    # 获取 RPS 数据
    current_rps = None
    baseline_rps = None
    
    if 'test_scenarios' in current:
        for scenario in current['test_scenarios']:
            if 'results' in scenario and 'rps' in scenario['results']:
                current_rps = scenario['results']['rps']['value']
                break
    
    if 'baseline' in baseline and 'rps' in baseline['baseline']:
        baseline_rps = baseline['baseline']['rps']
    
    if current_rps and baseline_rps:
        change = ((current_rps - baseline_rps) / baseline_rps) * 100
        
        results['details'].append({
            'metric': 'RPS',
            'baseline': baseline_rps,
            'current': current_rps,
            'change': change
        })
        
        # 回归阈值：RPS 下降超过 10%
        if change < -10:
            results['regression'] = True
        # 改进阈值：RPS 提升超过 10%
        elif change > 10:
            results['improvement'] = True
    
    return results


def generate_pr_comment(workflow_name, conclusion, run_id, run_number, 
                       performance_comparison=None, test_results=None):
    """生成 PR 评论内容"""
    comment = f"## 🤖 CI/CD PR Validation Results\n\n"
    comment += f"**Workflow**: {workflow_name}\n"
    comment += f"**Run**: #{run_number}\n"
    
    # 状态图标
    if conclusion == 'success':
        comment += f"**Status**: ✅ Passed\n\n"
    else:
        comment += f"**Status**: ❌ Failed\n\n"
    
    # 性能比较
    if performance_comparison:
        comment += "### 📊 Performance Comparison\n\n"
        
        if performance_comparison['regression']:
            comment += "⚠️ **Performance Regression Detected**\n\n"
        elif performance_comparison['improvement']:
            comment += "✨ **Performance Improvement Detected**\n\n"
        else:
            comment += "✅ **Performance Stable**\n\n"
        
        comment += "| Metric | Baseline | Current | Change |\n"
        comment += "|--------|----------|---------|--------|\n"
        
        for detail in performance_comparison['details']:
            change_pct = detail['change']
            change_str = f"{change_pct:+.2f}%"
            
            if change_pct < -10:
                change_str = f"🔴 {change_str}"
            elif change_pct > 10:
                change_str = f"🟢 {change_str}"
            
            comment += f"| {detail['metric']} | {detail['baseline']:.2f} | {detail['current']:.2f} | {change_str} |\n"
        
        comment += "\n"
    
    # 测试结果
    if test_results:
        comment += "### 🧪 Test Results\n\n"
        comment += f"- Total: {test_results.get('total', 0)}\n"
        comment += f"- Passed: {test_results.get('passed', 0)} ✅\n"
        comment += f"- Failed: {test_results.get('failed', 0)} ❌\n"
        comment += f"- Skipped: {test_results.get('skipped', 0)} ⏭️\n\n"
    
    # 详细链接
    comment += "### 🔗 Links\n\n"
    comment += f"- [View Workflow Run](https://github.com/adam-ikari/uvhttp/actions/runs/{run_id})\n"
    
    if conclusion == 'success':
        comment += "\nReady for review! 🎉"
    else:
        comment += "\nPlease fix the issues and push a new commit."
    
    return comment


def main():
    parser = argparse.ArgumentParser(description='Generate PR notification comment')
    parser.add_argument('--workflow-name', required=True, help='Workflow name')
    parser.add_argument('--conclusion', required=True, choices=['success', 'failure', 'cancelled', 'skipped'], help='Workflow conclusion')
    parser.add_argument('--run-id', required=True, help='Workflow run ID')
    parser.add_argument('--run-number', required=True, help='Workflow run number')
    parser.add_argument('--performance-results', help='Path to performance results JSON')
    parser.add_argument('--baseline', help='Path to baseline JSON')
    parser.add_argument('--test-results', help='Path to test results JSON')
    parser.add_argument('--output', '-o', help='Output file (default: stdout)')
    
    args = parser.parse_args()
    
    # 加载数据
    performance_data = None
    baseline_data = None
    test_data = None
    
    if args.performance_results:
        performance_data = load_performance_results(args.performance_results)
    
    if args.baseline:
        baseline_data = load_baseline(args.baseline)
    
    if args.test_results:
        with open(args.test_results, 'r') as f:
            test_data = json.load(f)
    
    # 比较性能
    performance_comparison = None
    if performance_data and baseline_data:
        performance_comparison = compare_performance(performance_data, baseline_data)
    
    # 生成评论
    comment = generate_pr_comment(
        args.workflow_name,
        args.conclusion,
        args.run_id,
        args.run_number,
        performance_comparison,
        test_data
    )
    
    # 输出
    if args.output:
        with open(args.output, 'w') as f:
            f.write(comment)
        print(f"Comment written to {args.output}")
    else:
        print(comment)


if __name__ == '__main__':
    main()
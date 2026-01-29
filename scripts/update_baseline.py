#!/usr/bin/env python3
"""
更新性能基线脚本
用于更新性能基线文件
"""

import os
import sys
import json
import argparse
from datetime import datetime
import shutil


def load_current_results(results_file):
    """加载当前性能测试结果"""
    try:
        with open(results_file, 'r') as f:
            return json.load(f)
    except Exception as e:
        print(f"Error loading results: {e}", file=sys.stderr)
        return None


def load_baseline(baseline_file):
    """加载当前基线"""
    try:
        with open(baseline_file, 'r') as f:
            return json.load(f)
    except Exception as e:
        print(f"Baseline file not found, will create new: {e}", file=sys.stderr)
        return None


def extract_baseline_data(results, version=None, commit=None):
    """从结果中提取基线数据"""
    if not results:
        return None
    
    baseline_data = {
        'version': version or 'unknown',
        'date': datetime.utcnow().isoformat() + 'Z',
        'commit': commit or 'unknown',
        'baseline': {}
    }
    
    # 提取 RPS 数据
    if 'test_scenarios' in results:
        for scenario in results['test_scenarios']:
            scenario_name = scenario.get('name', 'unknown')
            
            if 'results' in scenario:
                scenario_data = {}
                
                if 'rps' in scenario['results']:
                    scenario_data['rps'] = scenario['results']['rps']['value']
                
                if 'latency_avg' in scenario['results']:
                    latency = scenario['results']['latency_avg']['value']
                    if latency != 'N/A':
                        scenario_data['latency_avg'] = latency
                
                if scenario_data:
                    baseline_data['baseline'][scenario_name] = scenario_data
    
    # 提取环境信息
    if 'environment' in results:
        baseline_data['environment'] = {
            'os': results['environment'].get('os', 'unknown'),
            'runner': results['environment'].get('runner', 'unknown'),
            'cpu': results['environment'].get('cpu', 'unknown'),
            'compiler': results['environment'].get('compiler', 'unknown'),
            'compiler_version': results['environment'].get('compiler_version', 'unknown'),
            'build_type': results['environment'].get('build_type', 'Release'),
            'optimization': results['environment'].get('optimization', '-O2')
        }
    
    return baseline_data


def update_baseline_file(baseline_file, new_baseline, backup=True):
    """更新基线文件"""
    # 备份旧基线
    if backup and os.path.exists(baseline_file):
        backup_file = f"{baseline_file}.backup"
        shutil.copy2(baseline_file, backup_file)
        print(f"Baseline backed up to {backup_file}")
    
    # 写入新基线
    with open(baseline_file, 'w') as f:
        json.dump(new_baseline, f, indent=2)
    
    print(f"Baseline updated: {baseline_file}")
    print(f"Version: {new_baseline['version']}")
    print(f"Date: {new_baseline['date']}")
    
    return True


def update_history_file(history_file, new_baseline, max_entries=30):
    """更新历史记录文件"""
    history = []
    
    # 加载现有历史
    if os.path.exists(history_file):
        try:
            with open(history_file, 'r') as f:
                history = json.load(f)
        except:
            history = []
    
    # 添加新条目
    history.insert(0, new_baseline)
    
    # 限制条目数量
    history = history[:max_entries]
    
    # 写入历史文件
    with open(history_file, 'w') as f:
        json.dump(history, f, indent=2)
    
    print(f"History updated: {history_file} ({len(history)} entries)")
    
    return True


def compare_with_baseline(new_baseline, old_baseline):
    """比较新基线与旧基线"""
    if not old_baseline:
        print("No old baseline found for comparison")
        return
    
    print("\n=== Baseline Comparison ===\n")
    
    new_benchmarks = new_baseline.get('baseline', {})
    old_benchmarks = old_baseline.get('baseline', {})
    
    all_scenarios = set(new_benchmarks.keys()) | set(old_benchmarks.keys())
    
    for scenario in sorted(all_scenarios):
        new_data = new_benchmarks.get(scenario, {})
        old_data = old_benchmarks.get(scenario, {})
        
        print(f"Scenario: {scenario}")
        
        if 'rps' in new_data and 'rps' in old_data:
            change = ((new_data['rps'] - old_data['rps']) / old_data['rps']) * 100
            print(f"  RPS: {old_data['rps']:.2f} → {new_data['rps']:.2f} ({change:+.2f}%)")
        
        if 'latency_avg' in new_data and 'latency_avg' in old_data:
            change = ((new_data['latency_avg'] - old_data['latency_avg']) / old_data['latency_avg']) * 100
            print(f"  Latency: {old_data['latency_avg']:.2f}ms → {new_data['latency_avg']:.2f}ms ({change:+.2f}%)")
        
        print()


def main():
    parser = argparse.ArgumentParser(description='Update performance baseline')
    parser.add_argument('--results', '-r', required=True, help='Path to performance results JSON')
    parser.add_argument('--baseline', '-b', default='docs/performance/baseline.json', help='Path to baseline file')
    parser.add_argument('--history', '-H', default='docs/performance/baseline-history.json', help='Path to history file')
    parser.add_argument('--version', '-v', help='Version string (default: auto-detect)')
    parser.add_argument('--commit', '-c', help='Commit SHA (default: auto-detect)')
    parser.add_argument('--no-backup', action='store_true', help='Skip backup of old baseline')
    parser.add_argument('--no-history', action='store_true', help='Skip updating history file')
    parser.add_argument('--max-history', type=int, default=30, help='Maximum history entries')
    parser.add_argument('--compare', action='store_true', help='Compare with old baseline')
    
    args = parser.parse_args()
    
    # 加载结果
    results = load_current_results(args.results)
    if not results:
        print("Failed to load results", file=sys.stderr)
        sys.exit(1)
    
    # 提取版本和提交信息
    version = args.version
    commit = args.commit
    
    if not version:
        # 尝试从结果中获取版本
        version = results.get('version', 'unknown')
    
    if not commit:
        # 尝试从结果中获取提交信息
        if 'commit' in results:
            commit = results['commit'].get('sha', 'unknown')
        else:
            commit = 'unknown'
    
    # 提取基线数据
    new_baseline = extract_baseline_data(results, version, commit)
    if not new_baseline:
        print("Failed to extract baseline data", file=sys.stderr)
        sys.exit(1)
    
    # 加载旧基线
    old_baseline = None
    if args.compare or os.path.exists(args.baseline):
        old_baseline = load_baseline(args.baseline)
    
    # 比较基线
    if args.compare and old_baseline:
        compare_with_baseline(new_baseline, old_baseline)
    
    # 更新基线文件
    baseline_dir = os.path.dirname(args.baseline)
    if baseline_dir:
        os.makedirs(baseline_dir, exist_ok=True)
    
    update_baseline_file(args.baseline, new_baseline, backup=not args.no_backup)
    
    # 更新历史文件
    if not args.no_history:
        history_dir = os.path.dirname(args.history)
        if history_dir:
            os.makedirs(history_dir, exist_ok=True)
        
        update_history_file(args.history, new_baseline, max_entries=args.max_history)
    
    print("\n✅ Baseline update completed successfully")


if __name__ == '__main__':
    main()
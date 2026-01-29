#!/usr/bin/env python3
"""
UVHTTP 基准性能测试结果分析脚本

这个脚本分析基准测试结果，生成性能报告和趋势图表。
"""

import os
import sys
import json
import csv
from datetime import datetime
from pathlib import Path

class BenchmarkAnalyzer:
    def __init__(self, results_dir):
        self.results_dir = Path(results_dir)
        self.data = {}
        
    def load_results(self):
        """加载所有测试结果"""
        print(f"加载测试结果: {self.results_dir}")
        
        # 加载 RPS 结果
        self._load_rps_results()
        
        # 加载延迟结果
        self._load_latency_results()
        
        # 加载内存结果
        self._load_memory_results()
        
        # 加载连接结果
        self._load_connection_results()
        
        print(f"加载完成，共加载 {len(self.data)} 个测试结果")
        
    def _load_rps_results(self):
        """加载 RPS 测试结果"""
        rps_file = self.results_dir / "benchmark_rps.rps.csv"
        if rps_file.exists():
            with open(rps_file, 'r') as f:
                reader = csv.DictReader(f)
                for row in reader:
                    self.data['rps'] = self.data.get('rps', {})
                    self.data['rps'][row['test_name']] = float(row['rps'])
    
    def _load_latency_results(self):
        """加载延迟测试结果"""
        latency_log = self.results_dir / "benchmark_latency.server.log"
        if latency_log.exists():
            with open(latency_log, 'r') as f:
                content = f.read()
                # 解析延迟统计
                if "平均延迟" in content:
                    self.data['latency'] = {}
                    for line in content.split('\n'):
                        if '平均延迟' in line:
                            parts = line.split(':')
                            if len(parts) >= 2:
                                self.data['latency']['avg'] = parts[1].strip()
                        elif 'P50 延迟' in line:
                            parts = line.split(':')
                            if len(parts) >= 2:
                                self.data['latency']['p50'] = parts[1].strip()
                        elif 'P95 延迟' in line:
                            parts = line.split(':')
                            if len(parts) >= 2:
                                self.data['latency']['p95'] = parts[1].strip()
                        elif 'P99 延迟' in line:
                            parts = line.split(':')
                            if len(parts) >= 2:
                                self.data['latency']['p99'] = parts[1].strip()
    
    def _load_memory_results(self):
        """加载内存测试结果"""
        memory_log = self.results_dir / "benchmark_memory.server.log"
        if memory_log.exists():
            with open(memory_log, 'r') as f:
                content = f.read()
                if "峰值内存使用" in content:
                    self.data['memory'] = {}
                    for line in content.split('\n'):
                        if '峰值内存使用' in line:
                            parts = line.split(':')
                            if len(parts) >= 2:
                                self.data['memory']['peak'] = parts[1].strip()
    
    def _load_connection_results(self):
        """加载连接测试结果"""
        connection_log = self.results_dir / "benchmark_connection.server.log"
        if connection_log.exists():
            with open(connection_log, 'r') as f:
                content = f.read()
                if "成功率" in content:
                    self.data['connection'] = {}
                    for line in content.split('\n'):
                        if '成功率' in line:
                            parts = line.split(':')
                            if len(parts) >= 2:
                                self.data['connection']['success_rate'] = parts[1].strip()
    
    def analyze(self):
        """分析测试结果"""
        print("分析测试结果...")
        
        self.analysis = {
            'timestamp': datetime.now().isoformat(),
            'rps_analysis': self._analyze_rps(),
            'latency_analysis': self._analyze_latency(),
            'memory_analysis': self._analyze_memory(),
            'connection_analysis': self._analyze_connection(),
            'overall_assessment': self._assess_overall()
        }
        
        print("分析完成")
        
    def _analyze_rps(self):
        """分析 RPS 结果"""
        analysis = {
            'status': 'unknown',
            'details': []
        }
        
        if 'rps' not in self.data:
            analysis['status'] = 'no_data'
            return analysis
        
        rps_data = self.data['rps']
        
        # 检查低并发目标（2 线程 / 10 连接）
        low_concurrent_key = 'benchmark_rps_2t_10c'
        if low_concurrent_key in rps_data:
            rps = rps_data[low_concurrent_key]
            if rps >= 17000:
                analysis['details'].append(f"✅ 低并发: {rps:.0f} RPS (目标: ≥ 17,000)")
            elif rps >= 15000:
                analysis['details'].append(f"⚠️  低并发: {rps:.0f} RPS (目标: ≥ 17,000)")
            else:
                analysis['details'].append(f"❌ 低并发: {rps:.0f} RPS (目标: ≥ 17,000)")
        
        # 检查中等并发目标（4 线程 / 50 连接）
        medium_concurrent_key = 'benchmark_rps_4t_50c'
        if medium_concurrent_key in rps_data:
            rps = rps_data[medium_concurrent_key]
            if rps >= 17000:
                analysis['details'].append(f"✅ 中等并发: {rps:.0f} RPS (目标: ≥ 17,000)")
            elif rps >= 15000:
                analysis['details'].append(f"⚠️  中等并发: {rps:.0f} RPS (目标: ≥ 17,000)")
            else:
                analysis['details'].append(f"❌ 中等并发: {rps:.0f} RPS (目标: ≥ 17,000)")
        
        # 检查高并发目标（8 线程 / 200 连接）
        high_concurrent_key = 'benchmark_rps_8t_200c'
        if high_concurrent_key in rps_data:
            rps = rps_data[high_concurrent_key]
            if rps >= 16000:
                analysis['details'].append(f"✅ 高并发: {rps:.0f} RPS (目标: ≥ 16,000)")
            elif rps >= 14000:
                analysis['details'].append(f"⚠️  高并发: {rps:.0f} RPS (目标: ≥ 16,000)")
            else:
                analysis['details'].append(f"❌ 高并发: {rps:.0f} RPS (目标: ≥ 16,000)")
        
        # 判断整体状态
        all_pass = all('✅' in detail for detail in analysis['details'])
        if all_pass:
            analysis['status'] = 'pass'
        else:
            analysis['status'] = 'partial'
        
        return analysis
    
    def _analyze_latency(self):
        """分析延迟结果"""
        analysis = {
            'status': 'unknown',
            'details': []
        }
        
        if 'latency' not in self.data:
            analysis['status'] = 'no_data'
            return analysis
        
        latency_data = self.data['latency']
        
        # 检查平均延迟
        if 'avg' in latency_data:
            avg_str = latency_data['avg']
            avg_ms = float(avg_str.split()[0])
            if avg_ms < 15:
                analysis['details'].append(f"✅ 平均延迟: {avg_str} (目标: < 15ms)")
            elif avg_ms < 30:
                analysis['details'].append(f"⚠️  平均延迟: {avg_str} (目标: < 15ms)")
            else:
                analysis['details'].append(f"❌ 平均延迟: {avg_str} (目标: < 15ms)")
        
        # 检查 P99 延迟
        if 'p99' in latency_data:
            p99_str = latency_data['p99']
            p99_ms = float(p99_str.split()[0])
            if p99_ms < 50:
                analysis['details'].append(f"✅ P99 延迟: {p99_str} (目标: < 50ms)")
            elif p99_ms < 100:
                analysis['details'].append(f"⚠️  P99 延迟: {p99_str} (目标: < 50ms)")
            else:
                analysis['details'].append(f"❌ P99 延迟: {p99_str} (目标: < 50ms)")
        
        # 判断整体状态
        all_pass = all('✅' in detail for detail in analysis['details'])
        if all_pass:
            analysis['status'] = 'pass'
        else:
            analysis['status'] = 'partial'
        
        return analysis
    
    def _analyze_memory(self):
        """分析内存结果"""
        analysis = {
            'status': 'unknown',
            'details': []
        }
        
        if 'memory' not in self.data:
            analysis['status'] = 'no_data'
            return analysis
        
        memory_data = self.data['memory']
        
        if 'peak' in memory_data:
            analysis['details'].append(f"峰值内存: {memory_data['peak']}")
        
        analysis['status'] = 'pass'
        
        return analysis
    
    def _analyze_connection(self):
        """分析连接结果"""
        analysis = {
            'status': 'unknown',
            'details': []
        }
        
        if 'connection' not in self.data:
            analysis['status'] = 'no_data'
            return analysis
        
        connection_data = self.data['connection']
        
        if 'success_rate' in connection_data:
            rate_str = connection_data['success_rate']
            rate = float(rate_str.replace('%', ''))
            if rate >= 99.9:
                analysis['details'].append(f"✅ 成功率: {rate_str} (目标: > 99.9%)")
            elif rate >= 99.0:
                analysis['details'].append(f"⚠️  成功率: {rate_str} (目标: > 99.9%)")
            else:
                analysis['details'].append(f"❌ 成功率: {rate_str} (目标: > 99.9%)")
        
        # 判断整体状态
        all_pass = all('✅' in detail for detail in analysis['details'])
        if all_pass:
            analysis['status'] = 'pass'
        else:
            analysis['status'] = 'partial'
        
        return analysis
    
    def _assess_overall(self):
        """评估整体性能"""
        assessments = []
        
        if 'rps_analysis' in self.analysis:
            assessments.append(self.analysis['rps_analysis']['status'])
        
        if 'latency_analysis' in self.analysis:
            assessments.append(self.analysis['latency_analysis']['status'])
        
        if 'connection_analysis' in self.analysis:
            assessments.append(self.analysis['connection_analysis']['status'])
        
        if 'memory_analysis' in self.analysis:
            assessments.append(self.analysis['memory_analysis']['status'])
        
        # 判断整体状态
        if all(a == 'pass' for a in assessments):
            return 'excellent'
        elif all(a in ['pass', 'partial'] for a in assessments):
            return 'good'
        else:
            return 'needs_improvement'
    
    def generate_report(self):
        """生成分析报告"""
        print("生成分析报告...")
        
        report_file = self.results_dir / "analysis_report.md"
        
        with open(report_file, 'w') as f:
            f.write("# UVHTTP 基准性能测试分析报告\n\n")
            f.write(f"**生成时间**: {self.analysis['timestamp']}\n\n")
            
            # RPS 分析
            if 'rps_analysis' in self.analysis:
                f.write("## RPS 性能分析\n\n")
                for detail in self.analysis['rps_analysis']['details']:
                    f.write(f"- {detail}\n")
                f.write("\n")
            
            # 延迟分析
            if 'latency_analysis' in self.analysis:
                f.write("## 延迟性能分析\n\n")
                for detail in self.analysis['latency_analysis']['details']:
                    f.write(f"- {detail}\n")
                f.write("\n")
            
            # 内存分析
            if 'memory_analysis' in self.analysis:
                f.write("## 内存使用分析\n\n")
                for detail in self.analysis['memory_analysis']['details']:
                    f.write(f"- {detail}\n")
                f.write("\n")
            
            # 连接分析
            if 'connection_analysis' in self.analysis:
                f.write("## 连接管理分析\n\n")
                for detail in self.analysis['connection_analysis']['details']:
                    f.write(f"- {detail}\n")
                f.write("\n")
            
            # 整体评估
            f.write("## 整体性能评估\n\n")
            overall = self.analysis['overall_assessment']
            if overall == 'excellent':
                f.write("✅ **优秀** - 所有性能指标都达到或超过目标\n")
            elif overall == 'good':
                f.write("⚠️  **良好** - 大部分性能指标达到目标，有小部分需要改进\n")
            else:
                f.write("❌ **需要改进** - 有多个性能指标未达到目标\n")
            f.write("\n")
            
            # 建议
            f.write("## 改进建议\n\n")
            if 'rps_analysis' in self.analysis and self.analysis['rps_analysis']['status'] != 'pass':
                f.write("- 优化 RPS 性能：检查连接管理、内存分配算法\n")
            if 'latency_analysis' in self.analysis and self.analysis['latency_analysis']['status'] != 'pass':
                f.write("- 优化延迟性能：检查事件循环、I/O 操作\n")
            if 'connection_analysis' in self.analysis and self.analysis['connection_analysis']['status'] != 'pass':
                f.write("- 优化连接管理：检查连接建立和复用逻辑\n")
            f.write("\n")
        
        print(f"分析报告已生成: {report_file}")


def main():
    # 获取结果目录
    if len(sys.argv) > 1:
        results_dir = sys.argv[1]
    else:
        # 使用最新的结果目录
        results_base = Path(__file__).parent / "results"
        run_dirs = sorted([d for d in results_base.iterdir() if d.is_dir() and d.name.startswith("run_")])
        if run_dirs:
            results_dir = run_dirs[-1]
        else:
            print("错误: 没有找到测试结果目录")
            sys.exit(1)
    
    # 创建分析器
    analyzer = BenchmarkAnalyzer(results_dir)
    
    # 加载结果
    analyzer.load_results()
    
    # 分析结果
    analyzer.analyze()
    
    # 生成报告
    analyzer.generate_report()
    
    print("\n分析完成！")


if __name__ == "__main__":
    main()
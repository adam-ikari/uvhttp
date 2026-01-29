#!/usr/bin/env python3
"""
生成性能趋势图
"""

import json
import matplotlib.pyplot as plt
import matplotlib.dates as mdates
from datetime import datetime
import os
import sys

def generate_trend_chart(data_files, output_file):
    """
    生成性能趋势图
    
    Args:
        data_files: 历史数据文件列表
        output_file: 输出文件路径
    """
    # 加载历史数据
    historical_data = []
    for data_file in data_files:
        try:
            with open(data_file, 'r') as f:
                historical_data.append(json.load(f))
        except Exception as e:
            print(f"Warning: Failed to load {data_file}: {e}")
    
    if not historical_data:
        print("No valid data files found")
        return
    
    # 按日期排序
    historical_data.sort(key=lambda x: x['timestamp'])
    
    # 提取数据
    dates = [datetime.strptime(d['timestamp'], '%Y-%m-%dT%H:%M:%SZ') for d in historical_data]
    
    # 创建图表
    fig, axes = plt.subplots(2, 2, figsize=(16, 12))
    fig.suptitle('UVHTTP Performance Trends', fontsize=16, fontweight='bold')
    
    # 1. 吞吐量趋势
    ax1 = axes[0, 0]
    rps_low = []
    rps_medium = []
    rps_high = []
    
    for d in historical_data:
        for s in d.get('test_scenarios', []):
            if s['name'] == 'low_concurrent':
                rps_low.append(s['results']['rps']['value'])
            elif s['name'] == 'medium_concurrent':
                rps_medium.append(s['results']['rps']['value'])
            elif s['name'] == 'high_concurrent':
                rps_high.append(s['results']['rps']['value'])
    
    if rps_low:
        ax1.plot(dates, rps_low, marker='o', label='Low Concurrent (10)', linewidth=2)
    if rps_medium:
        ax1.plot(dates, rps_medium, marker='s', label='Medium Concurrent (50)', linewidth=2)
    if rps_high:
        ax1.plot(dates, rps_high, marker='^', label='High Concurrent (100)', linewidth=2)
    
    ax1.set_xlabel('Date', fontsize=12)
    ax1.set_ylabel('Requests Per Second', fontsize=12)
    ax1.set_title('Throughput Trends', fontsize=14, fontweight='bold')
    ax1.legend()
    ax1.grid(True, alpha=0.3)
    ax1.xaxis.set_major_formatter(mdates.DateFormatter('%Y-%m-%d'))
    plt.setp(ax1.xaxis.get_majorticklabels(), rotation=45, ha='right')
    
    # 2. 延迟趋势（简化版，只显示平均延迟）
    ax2 = axes[0, 1]
    latency_low = []
    latency_medium = []
    latency_high = []
    
    for d in historical_data:
        for s in d.get('test_scenarios', []):
            if s['name'] == 'low_concurrent':
                latency_low.append(float(s['results']['latency_avg']['value'].replace('us', '').replace('ms', '')))
            elif s['name'] == 'medium_concurrent':
                latency_medium.append(float(s['results']['latency_avg']['value'].replace('us', '').replace('ms', '')))
            elif s['name'] == 'high_concurrent':
                latency_high.append(float(s['results']['latency_avg']['value'].replace('us', '').replace('ms', '')))
    
    if latency_low:
        ax2.plot(dates, latency_low, marker='o', label='Low Concurrent', linewidth=2)
    if latency_medium:
        ax2.plot(dates, latency_medium, marker='s', label='Medium Concurrent', linewidth=2)
    if latency_high:
        ax2.plot(dates, latency_high, marker='^', label='High Concurrent', linewidth=2)
    
    ax2.set_xlabel('Date', fontsize=12)
    ax2.set_ylabel('Average Latency', fontsize=12)
    ax2.set_title('Latency Trends', fontsize=14, fontweight='bold')
    ax2.legend()
    ax2.grid(True, alpha=0.3)
    ax2.xaxis.set_major_formatter(mdates.DateFormatter('%Y-%m-%d'))
    plt.setp(ax2.xaxis.get_majorticklabels(), rotation=45, ha='right')
    
    # 3. P99 延迟趋势
    ax3 = axes[1, 0]
    p99_low = []
    p99_medium = []
    p99_high = []
    
    for d in historical_data:
        for s in d.get('test_scenarios', []):
            if s['name'] == 'low_concurrent':
                p99_value = s['results'].get('latency_p99', 'N/A')
                if p99_value != 'N/A':
                    p99_low.append(float(p99_value.replace('us', '').replace('ms', '')))
            elif s['name'] == 'medium_concurrent':
                p99_value = s['results'].get('latency_p99', 'N/A')
                if p99_value != 'N/A':
                    p99_medium.append(float(p99_value.replace('us', '').replace('ms', '')))
            elif s['name'] == 'high_concurrent':
                p99_value = s['results'].get('latency_p99', 'N/A')
                if p99_value != 'N/A':
                    p99_high.append(float(p99_value.replace('us', '').replace('ms', '')))
    
    if p99_low:
        ax3.plot(dates, p99_low, marker='o', label='Low Concurrent', linewidth=2)
    if p99_medium:
        ax3.plot(dates, p99_medium, marker='s', label='Medium Concurrent', linewidth=2)
    if p99_high:
        ax3.plot(dates, p99_high, marker='^', label='High Concurrent', linewidth=2)
    
    ax3.set_xlabel('Date', fontsize=12)
    ax3.set_ylabel('P99 Latency', fontsize=12)
    ax3.set_title('P99 Latency Trends', fontsize=14, fontweight='bold')
    ax3.legend()
    ax3.grid(True, alpha=0.3)
    ax3.xaxis.set_major_formatter(mdates.DateFormatter('%Y-%m-%d'))
    plt.setp(ax3.xaxis.get_majorticklabels(), rotation=45, ha='right')
    
    # 4. 测试场景数量趋势
    ax4 = axes[1, 1]
    total_scenarios = [len(d.get('test_scenarios', [])) for d in historical_data]
    
    ax4.plot(dates, total_scenarios, marker='o', color='blue', label='Total Scenarios', linewidth=2)
    ax4.set_xlabel('Date', fontsize=12)
    ax4.set_ylabel('Number of Scenarios', fontsize=12)
    ax4.set_title('Test Scenarios Trends', fontsize=14, fontweight='bold')
    ax4.legend()
    ax4.grid(True, alpha=0.3)
    ax4.xaxis.set_major_formatter(mdates.DateFormatter('%Y-%m-%d'))
    plt.setp(ax4.xaxis.get_majorticklabels(), rotation=45, ha='right')
    
    plt.tight_layout()
    plt.savefig(output_file, dpi=300, bbox_inches='tight')
    print(f"Performance trend chart saved to {output_file}")

if __name__ == '__main__':
    if len(sys.argv) < 3:
        print("Usage: python3 generate_trend_chart.py <data_files> <output_file>")
        sys.exit(1)
    
    data_files = sys.argv[1:-1]
    output_file = sys.argv[-1]
    generate_trend_chart(data_files, output_file)
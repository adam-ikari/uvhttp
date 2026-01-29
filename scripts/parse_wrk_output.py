#!/usr/bin/env python3
"""
Parse wrk output and generate performance JSON in the correct format
"""

import json
import re
import sys
from datetime import datetime

def parse_wrk_output(log_file):
    """Parse wrk output file"""
    with open(log_file, 'r') as f:
        content = f.read()
    
    # Extract RPS from each wrk run
    rps_values = []
    pattern = r'(\d+)\s+requests in\s+([\d.]+)s'
    matches = re.findall(pattern, content)
    
    for match in matches:
        requests = int(match[0])
        duration = float(match[1])
        rps = requests / duration
        rps_values.append(rps)
    
    return rps_values

def generate_performance_json(rps_values):
    """Generate performance JSON in baseline format"""
    if len(rps_values) < 3:
        print("Error: Need at least 3 RPS values", file=sys.stderr)
        return None
    
    # Map to scenarios
    scenarios = {
        "low_concurrent": {
            "rps": int(rps_values[0]),
            "latency_avg": 0,
            "latency_p50": 0,
            "latency_p95": 0,
            "latency_p99": 0,
            "latency_p999": 0,
            "transfer_rate": 0,
            "errors": 0
        },
        "medium_concurrent": {
            "rps": int(rps_values[1]),
            "latency_avg": 0,
            "latency_p50": 0,
            "latency_p95": 0,
            "latency_p99": 0,
            "latency_p999": 0,
            "transfer_rate": 0,
            "errors": 0
        },
        "high_concurrent": {
            "rps": int(rps_values[2]),
            "latency_avg": 0,
            "latency_p50": 0,
            "latency_p95": 0,
            "latency_p99": 0,
            "latency_p999": 0,
            "transfer_rate": 0,
            "errors": 0
        }
    }
    
    return {
        "version": "1.0.0",
        "updated_at": datetime.utcnow().strftime("%Y-%m-%dT%H:%M:%SZ"),
        "commit": {
            "sha": "current",
            "short_sha": "current",
            "message": "Current performance test",
            "branch": "pr"
        },
        "environment": {
            "os": "Linux",
            "runner": "GitHub Actions",
            "cpu": "Unknown",
            "cores": 0,
            "memory": "Unknown",
            "compiler": "GCC",
            "compiler_version": "Unknown",
            "build_type": "Release",
            "optimization": "-O2"
        },
        "scenarios": scenarios,
        "thresholds": {
            "rps_warning": 0.1,
            "rps_failure": 0.1,
            "latency_warning": 0.1,
            "latency_failure": 0.2,
            "min_improvement": 0.05
        }
    }

def main():
    if len(sys.argv) < 2:
        print("Usage: parse_wrk_output.py <wrk_log_file> [output_file]", file=sys.stderr)
        sys.exit(1)
    
    log_file = sys.argv[1]
    output_file = sys.argv[2] if len(sys.argv) > 2 else "performance.json"
    
    # Parse wrk output
    rps_values = parse_wrk_output(log_file)
    
    if len(rps_values) == 0:
        print("Error: No RPS values found in wrk output", file=sys.stderr)
        sys.exit(1)
    
    # Generate JSON
    performance_data = generate_performance_json(rps_values)
    
    if performance_data is None:
        sys.exit(1)
    
    # Write to file
    with open(output_file, 'w') as f:
        json.dump(performance_data, f, indent=2)
    
    print(f"Performance data written to {output_file}")
    print(f"RPS values: {rps_values}")

if __name__ == "__main__":
    main()
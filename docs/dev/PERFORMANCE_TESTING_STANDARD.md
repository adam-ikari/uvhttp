# UVHTTP Benchmark Performance Testing Standard

## Document Information

- **Version**: 1.0.0
- **Creation Date**: 2026-01-09
- **Last Updated**: 2026-01-09
- **Maintainer**: UVHTTP Development Team
- **Status**: Official Release

## 1. Purpose

This document defines the standard specification for benchmark performance testing in the UVHTTP project, ensuring that all performance tests are conducted under consistent conditions and that test results are reliable, repeatable, and comparable.

## 2. Scope

This standard applies to:
- Performance benchmarking of the UVHTTP core library
- Performance regression testing between versions
- Performance validation testing of new features
- Evaluation of performance optimization results

## 3. Test Environment Standards

### 3.1 Hardware Requirements

#### Minimum Configuration
- **CPU**: 4 cores, 2.0 GHz or higher
- **Memory**: 8GB or more
- **Network**: 1Gbps or higher
- **Disk**: SSD (recommended)

#### Recommended Configuration
- **CPU**: 8 cores, 3.0 GHz or higher
- **Memory**: 16GB or more
- **Network**: 1Gbps or higher
- **Disk**: NVMe SSD

#### Reference Test Environment
```
Operating system: Linux 6.14.11-2-pve
CPU: AMD Ryzen 7 5800H (16 cores, 8 cores online)
Memory: 12GB (11GB available)
```

### 3.2 Software Requirements

#### Operating System
- **Recommended**: Linux (Ubuntu 20.04+, CentOS 8+, Debian 11+)
- **Kernel version**: 5.4 or higher

#### Compiler
- **GCC**: 9.0 or higher
- **Clang**: 10.0 or higher

#### Dependency Libraries
- **libuv**: 1.40.0 or higher
- **mbedtls**: 2.28.0 or higher
- **mimalloc**: latest stable release (recommended for production testing)

### 3.3 System Configuration

#### CPU Performance Mode
```bash
# Set to performance mode
sudo cpupower frequency-set -g performance

# Verify the CPU frequency
cpupower frequency-info
```

#### Memory Configuration
```bash
# Disable swap (recommended)
sudo swapoff -a

# Verify the swap status
free -h
```

#### Network Configuration
```bash
# Increase the local connection limit
sudo sysctl -w net.core.somaxconn=65535
sudo sysctl -w net.ipv4.tcp_max_syn_backlog=8192
```

#### File Descriptor Limits
```bash
# Increase the file descriptor limit
ulimit -n 65535

# Verify
ulimit -n
```

## 4. Test Tool Standards

### 4.1 Required Tools

#### wrk (primary testing tool)
- **Version**: 4.2.0 or higher
- **Installation**: `sudo apt-get install wrk` or compile from source
- **Purpose**: HTTP stress testing

#### ab (optional)
- **Version**: 2.4 or higher
- **Purpose**: Apache Bench, used for comparison testing

### 4.2 Monitoring Tools

#### pidstat
- **Purpose**: monitor process resource usage
- **Installation**: `sudo apt-get install sysstat`

#### perf
- **Purpose**: performance analysis
- **Installation**: `sudo apt-get install linux-tools-common`

#### iostat
- **Purpose**: disk I/O monitoring
- **Installation**: `sudo apt-get install sysstat`

### 4.3 Tool Verification

Before starting the test, verify that all tools are installed correctly:
```bash
wrk --version
ab -V
pidstat -V
perf version
```

## 5. Test Methodology Standards

### 5.1 Compilation Configuration

#### Standard Configuration (for benchmarking)
```bash
cmake -DBUILD_WITH_WEBSOCKET=ON \
      -DBUILD_WITH_MIMALLOC=ON \
      -ENABLE_DEBUG=OFF \
      -ENABLE_COVERAGE=OFF \
      ..
```

#### Debug Configuration (development and debugging only)
```bash
cmake -DBUILD_WITH_WEBSOCKET=ON \
      -DBUILD_WITH_MIMALLOC=OFF \
      -ENABLE_DEBUG=ON \
      -ENABLE_COVERAGE=ON \
      ..
```

**Note**: performance benchmarks must use the standard configuration.

### 5.2 Compilation Commands
```bash
# Clean up the old build
make clean

# Compile (using all CPU cores)
make build

# Verify the compilation result
ls -lh build/dist/lib/libuvhttp.a
```

### 5.3 Test Server

#### Performance-Dedicated Test Server
- **File**: `examples/performance_test.c`
- **Requirements**:
  - No debug output (no printf)
  - Minimal response body
  - Optimized server configuration
  - Support for multiple test scenarios

#### Server Configuration Example
```c
// Recommended configuration
config.max_connections = 10000;
config.read_buffer_size = 16384;
config.write_buffer_size = 16384;
config.keepalive_timeout = 60;
config.connection_timeout = 30;
```

#### Starting the Server
```bash
# Set CPU affinity (optional)
taskset -c 0-7 ./build/dist/bin/performance_test

# Run in the background
nohup ./build/dist/bin/performance_test > /dev/null 2>&1 &
```

### 5.4 Test Scenarios

#### Basic Test Scenarios

##### Scenario 1: Low Concurrency Test
```bash
# Parameters
Number of threads: 2
Concurrent connections: 10
Test duration: 10 seconds
Number of runs: 5 (take the average)

# Command
wrk -t2 -c10 -d10s http://127.0.0.1:8080/
```

##### Scenario 2: Medium Concurrency Test
```bash
# Parameters
Number of threads: 4
Concurrent connections: 50
Test duration: 10 seconds
Number of runs: 5 (take the average)

# Command
wrk -t4 -c50 -d10s http://127.0.0.1:8080/
```

##### Scenario 3: High Concurrency Test
```bash
# Parameters
Number of threads: 8
Concurrent connections: 200
Test duration: 10 seconds
Number of runs: 5 (take the average)

# Command
wrk -t8 -c200 -d10s http://127.0.0.1:8080/
```

#### Extended Test Scenarios

##### Scenario 4: POST Request Test
```bash
# Create a POST data file
echo '{"test":"data"}' > post_data.json

# Run the test
wrk -t4 -c50 -d10s -s post.lua http://127.0.0.1:8080/api
```

##### Scenario 5: Large Response Body Test
```bash
# Test a 100KB response
wrk -t4 -c50 -d10s http://127.0.0.1:8080/large/100kb

# Test a 1MB response
wrk -t4 -c50 -d10s http://127.0.0.1:8080/large/1mb
```

##### Scenario 6: TLS Encryption Test
```bash
# Test HTTPS
wrk -t4 -c50 -d10s https://127.0.0.1:8443/
```

##### Scenario 7: WebSocket Test
```bash
# Use a dedicated WebSocket testing tool
# For example: wscat, websocket-bench
```

### 5.5 Test Execution Flow

#### Standard Test Flow
1. **Environment preparation**
   ```bash
   # Clear the system cache
   sudo sync && echo 3 | sudo tee /proc/sys/vm/drop_caches

   # Stop unnecessary background services
   sudo systemctl stop cron
   ```

2. **Start the test server**
   ```bash
   # Ensure the port is not in use
   lsof -i :8080

   # Start the server
   ./build/dist/bin/performance_test

   # Wait for the server to be ready
   sleep 2

   # Verify the server is running
   curl -I http://127.0.0.1:8080/
   ```

3. **Start resource monitoring**
   ```bash
   # Monitor CPU and memory
   pidstat -p <pid> -d -h -r -s -u -w 1 > perf_stats.log &

   # Monitor the network
   sar -n DEV 1 > network_stats.log &
   ```

4. **Execute the test**
   ```bash
   # Run the test script
   ./test/run_performance_tests.sh
   ```

5. **Stop monitoring**
   ```bash
   # Stop all monitoring processes
   pkill pidstat
   pkill sar
   ```

6. **Stop the server**
   ```bash
   pkill performance_test
   ```

7. **Collect the results**
   ```bash
   # Organize the test results
   ./test/collect_results.sh
   ```

### 5.6 Repeated Testing

#### Number of Runs Requirement
- **Basic scenarios**: run each scenario at least 5 times
- **Extended scenarios**: run each scenario at least 3 times
- **Regression tests**: run each scenario at least 10 times

#### Data Processing
```bash
# Calculate the average
awk '{sum+=$1; count++} END {print sum/count}' results.txt

# Calculate the standard deviation
awk '{sum+=$1; sumsq+=$1*$1; count++} END {print sqrt(sumsq/count - (sum/count)^2)}' results.txt

# Exclude outliers (using the IQR method)
# Q1 = 25th percentile
# Q3 = 75th percentile
# IQR = Q3 - Q1
# Outlier = < (Q1 - 1.5*IQR) or > (Q3 + 1.5*IQR)
```

## 6. Test Data Standards

### 6.1 Required Metrics

#### Performance Metrics
- **Throughput (RPS)**: number of requests processed per second
- **Average latency**: average response time across all requests
- **P50 latency**: response time of 50% of requests
- **P95 latency**: response time of 95% of requests
- **P99 latency**: response time of 99% of requests
- **P99.9 latency**: response time of 99.9% of requests
- **Transfer rate**: amount of data transferred per second (MB/s)

#### Resource Metrics
- **CPU usage**: average CPU utilization
- **Memory usage**: average memory used
- **Context switches**: context switches per second
- **System calls**: system calls per second

### 6.2 Data Recording Format

#### Test Result Records
```json
{
  "test_id": "PERF-2026-01-09-001",
  "timestamp": "2026-01-09T22:30:00Z",
  "environment": {
    "os": "Linux 6.14.11-2-pve",
    "cpu": "AMD Ryzen 7 5800H",
    "cores": 16,
    "memory": "12GB",
    "compiler": "GCC",
    "compiler_version": "11.4.0"
  },
  "configuration": {
    "build_with_mimalloc": true,
    "build_with_websocket": true,
    "debug": false,
    "optimization": "-O2"
  },
  "test_scenario": {
    "name": "Medium concurrency test",
    "threads": 4,
    "connections": 50,
    "duration": 10,
    "method": "GET",
    "url": "http://127.0.0.1:8080/"
  },
  "results": {
    "total_requests": 165440,
    "requests_per_second": 16544.0,
    "latency_avg_ms": 2.94,
    "latency_p50_ms": 2.8,
    "latency_p95_ms": 5.5,
    "latency_p99_ms": 8.2,
    "latency_p999_ms": 15.3,
    "transfer_rate_mbps": 5.33
  },
  "resources": {
    "cpu_usage_percent": 85.5,
    "memory_usage_mb": 128.5,
    "context_switches_per_sec": 12500,
    "system_calls_per_sec": 45000
  }
}
```

### 6.3 Data Validation

#### Reasonableness Checks
- Throughput should be within a reasonable range (1000-100000 RPS)
- Latency should increase with concurrency
- CPU usage should not exceed 100%
- Memory usage should remain stable

#### Consistency Checks
- The standard deviation of multiple test results should be less than 10% of the average
- No outliers should appear (outside the average ±3 standard deviations)

## 7. Test Report Standards

### 7.1 Report Structure

#### 1. Overview
- Test purpose
- Test scope
- Test date
- Testers

#### 2. Test Environment
- Hardware configuration
- Software configuration
- Network configuration

#### 3. Test Configuration
- Compilation configuration
- Server configuration
- Test tool configuration

#### 4. Test Results
- Basic scenario results
- Extended scenario results
- Resource usage
- Performance metric summary

#### 5. Analysis
- Result analysis
- Performance trends
- Bottleneck identification
- Optimization recommendations

#### 6. Conclusion
- Performance evaluation
- Comparison with baseline
- Improvement recommendations

### 7.2 Report Format

#### Markdown Format Example
```markdown
# UVHTTP Performance Test Report

## Test Overview
- **Test Date**: 2026-01-09
- **Tester**: John Doe
- **Test Version**: v1.2.0

## Test Environment
- **Operating System**: Linux 6.14.11-2-pve
- **CPU**: AMD Ryzen 7 5800H (16 cores)
- **Memory**: 12GB

## Test Results

### Scenario 1: Low Concurrency Test
| Metric | Value |
|-----|---|
| Throughput | 35,864 RPS |
| Average latency | 271.91 μs |
| P99 latency | 1.3 ms |

### Scenario 2: Medium Concurrency Test
| Metric | Value |
|-----|---|
| Throughput | 16,544 RPS |
| Average latency | 2.94 ms |
| P99 latency | 8.2 ms |

## Conclusion
...
```

### 7.3 Report Storage

#### Naming Convention
```
PERF-<YYYY-MM-DD>-<VERSION>-<SCENARIO>.md

For example:
PERF-2026-01-09-v1.2.0-baseline.md
PERF-2026-01-09-v1.2.0-regression.md
```

#### Storage Location
```
docs/performance/
├── reports/
│   ├── 2026-01-09/
│   │   ├── PERF-2026-01-09-v1.2.0-baseline.md
│   │   └── PERF-2026-01-09-v1.2.0-regression.md
│   └── 2026-01-10/
│       └── ...
└── data/
    ├── 2026-01-09/
    │   ├── baseline.json
    │   └── regression.json
    └── ...
```

## 8. Performance Benchmark Standards

### 8.1 Baseline Performance Metrics

> **Note**: The following baseline values are derived from actual test results.

#### Low Concurrency Scenario (2 threads / 10 connections)
| Metric | Minimum Requirement | Recommended Value | Actual Baseline |
|-----|---------|-------|---------|
| Throughput (RPS) | 10,000 | 15,000 | 24,439 |
| Average latency | < 1 ms | < 600 μs | 362 μs |
| P99 latency | < 10 ms | < 5 ms | 3.35 ms |

#### Medium Concurrency Scenario (4 threads / 50 connections)
| Metric | Minimum Requirement | Recommended Value | Actual Baseline |
|-----|---------|-------|---------|
| Throughput (RPS) | 10,000 | 15,000 | 23,959 |
| Average latency | < 10 ms | < 5 ms | 1.97 ms |
| P99 latency | < 50 ms | < 20 ms | 5.40 ms |

#### High Concurrency Scenario (8 threads / 200 connections)
| Metric | Minimum Requirement | Recommended Value | Actual Baseline |
|-----|---------|-------|---------|
| Throughput (RPS) | 10,000 | 15,000 | 23,273 |
| Average latency | < 50 ms | < 20 ms | 8.57 ms |
| P99 latency | < 200 ms | < 100 ms | 22.39 ms |

### 8.2 Performance Regression Thresholds

#### Throughput Regression
- **Warning**: a decrease of 5-10%
- **Failure**: a decrease > 10%

#### Latency Regression
- **Warning**: an increase of 10-20%
- **Failure**: an increase > 20%

#### Resource Usage Regression
- **Warning**: CPU usage increases by 10-20%
- **Failure**: CPU usage increases > 20%

### 8.3 Performance Optimization Goals

> **Note**: The following goals are set based on actual test results.

#### Short-Term Goals (1-2 weeks)
- Low concurrency throughput reaches 17,000+ RPS (already achieved 24,439 RPS)
- Medium concurrency throughput reaches 17,000+ RPS (already achieved 23,959 RPS)
- High concurrency throughput reaches 16,000+ RPS (already achieved 23,273 RPS)
- Average latency optimization completed (362 μs low concurrency, 1.97 ms medium concurrency, 8.57 ms high concurrency)

#### Medium-Term Goals (1-2 months)
- Establish a performance regression testing system
- Integrate into the CI/CD pipeline
- Execute performance tests on a regular basis
- Continuously optimize performance

#### Long-Term Goals (3-6 months)
- Expand test scenarios
- Support distributed testing
- Establish a performance baseline database
- Release performance-optimized versions

## 9. Quality Assurance

### 9.1 Pre-Test Checklist

- [ ] System configuration meets the standard
- [ ] All tools are installed and verified
- [ ] Standard compilation configuration is used
- [ ] Compilation is warning-free and error-free
- [ ] The test server has no debug output
- [ ] Server configuration is optimized
- [ ] Test scripts are verified
- [ ] Monitoring tools are started

### 9.2 In-Test Checklist

- [ ] The server is running normally
- [ ] Resource monitoring is running normally
- [ ] Tests execute as planned
- [ ] Test results are completely recorded
- [ ] No abnormal interruption occurred

### 9.3 Post-Test Checklist

- [ ] All test scenarios are completed
- [ ] Test data is collected
- [ ] Data reasonableness is verified
- [ ] Outliers are handled
- [ ] Test report is generated
- [ ] Report is stored

## 10. Change Management

### 10.1 Standard Change Process

1. Submit a change request
2. Assess the impact of the change
3. Obtain approval
4. Update the documentation
5. Notify relevant personnel
6. Train relevant personnel

### 10.2 Version History

| Version | Date | Changes | Author |
|-----|------|---------|------|
| 1.0.0 | 2026-01-09 | Initial version | UVHTTP Development Team |

## 11. Appendix

### 11.1 Glossary

- **RPS**: Requests Per Second, number of requests per second
- **P50/P95/P99**: percentiles, representing the response time of 50%/95%/99% of requests
- **IQR**: Interquartile Range
- **mimalloc**: Microsoft's memory allocator, which outperforms the system allocator

### 11.2 References

- [wrk Official Documentation](https://github.com/wg/wrk)
- [Apache Bench Documentation](https://httpd.apache.org/docs/2.4/programs/ab.html)
- [Linux Performance Tuning Guide](https://access.redhat.com/documentation/en-us/red_hat_enterprise_linux/)

### 11.3 Contact Information

---

## Appendix A: Performance Test Plan

> **Note**: This appendix contains the detailed contents of the performance test plan. For information about test execution flow, test scripts, and risk management, refer to the complete content.

### A.1 Test Objectives

This test plan aims to evaluate the performance of the UVHTTP project in various scenarios, including:

- Establishing performance baseline data
- Identifying performance bottlenecks
- Verifying the effect of optimizations
- Ensuring performance stability
- Supporting performance regression testing

### A.2 Test Scope

#### Functional Scope
- HTTP/1.1 request handling
- Keep-Alive connection management
- Routing functionality
- Response handling
- Middleware system
- TLS/HTTPS encrypted communication
- WebSocket communication
- Static file serving
- Rate limiting functionality
- CORS support

#### Scenario Scope
- Different concurrency levels (low, medium, high)
- Different request methods (GET, POST, PUT, DELETE)
- Different response body sizes
- Long-running stability
- Memory leak detection

### A.3 Test Phases

#### Phase 1: Environment Preparation
- Verify the test environment configuration
- Install and verify test tools
- Compile the UVHTTP project (standard configuration)
- Create a performance-dedicated test server
- Write test scripts
- Set up the monitoring system

#### Phase 2: Basic Performance Testing
- Low concurrency test (2 threads / 10 connections)
- Medium concurrency test (4 threads / 50 connections)
- High concurrency test (8 threads / 200 connections)
- Different thread count tests (1, 2, 4, 8, 16)
- Data analysis and reporting

#### Phase 3: Extended Performance Testing
- POST request tests
- Large response body tests (1KB, 10KB, 100KB, 1MB)
- TLS/HTTPS encryption tests
- WebSocket performance tests
- Static file serving tests
- Data analysis and reporting

#### Phase 4: Stability Testing
- Long-running test (30 minutes)
- Long-running test (1 hour)
- Long-running test (4 hours)
- Memory leak detection
- Data analysis and reporting

#### Phase 5: Optimization and Regression Testing
- Identify performance bottlenecks
- Implement performance optimizations
- Re-test to verify optimization results
- Establish a performance baseline database
- Write a performance testing guide
- Generate a final report

### A.4 Example Test Script

#### Main Test Script
```bash
#!/bin/bash
# test/run_performance_tests.sh

set -e

# Configuration
TEST_DIR="test/performance_results"
SERVER_PID=""
MONITOR_PID=""

# Create the results directory
mkdir -p $TEST_DIR

# Start the server
start_server() {
    echo "Starting the performance test server..."
    ./build/dist/bin/performance_test &
    SERVER_PID=$!
    sleep 2

    # Verify the server is running
    if ! curl -s -o /dev/null -w "%{http_code}" http://127.0.0.1:8080/ | grep -q "200"; then
        echo "Error: server failed to start"
        exit 1
    fi
    echo "Server started (PID: $SERVER_PID)"
}

# Run a test scenario
run_test() {
    local name=$1
    local threads=$2
    local connections=$3
    local duration=$4
    local iterations=$5

    echo "Running test scenario: $name"
    echo "Parameters: threads=$threads, connections=$connections, duration=${duration}s"

    for i in $(seq 1 $iterations); do
        echo "  Iteration $i/$iterations..."
        wrk -t$threads -c$connections -d${duration}s \
            http://127.0.0.1:8080/ \
            | tee $TEST_DIR/${name}_${i}.txt
    done
}

# Cleanup function
cleanup() {
    echo "Cleaning up resources..."
    stop_monitor
    stop_server
    exit 0
}

# Capture exit signals
trap cleanup EXIT INT TERM

# Main flow
main() {
    echo "=== UVHTTP Performance Test ==="
    echo "Start time: $(date)"
    echo ""

    # Start the server
    start_server

    # Run the test scenarios
    echo "=== Basic Performance Tests ==="
    run_test "low_concurrent" 2 10 10 5
    run_test "medium_concurrent" 4 50 10 5
    run_test "high_concurrent" 8 200 10 5

    echo ""
    echo "=== Test Complete ==="
    echo "End time: $(date)"
    echo "Results saved in: $TEST_DIR"
}

# Execute the main flow
main
```

### A.5 Risk Management

#### Potential Risks

**Risk 1: Unstable test environment**
- **Impact**: unreliable test results
- **Probability**: medium
- **Mitigation measures**:
  - Use a dedicated test server
  - Shut down unnecessary background services
  - Fix the CPU frequency

**Risk 2: Test tool issues**
- **Impact**: unable to complete the test
- **Probability**: low
- **Mitigation measures**:
  - Verify test tools in advance
  - Prepare backup test tools
  - Record detailed error logs

### A.6 Deliverables

#### Test Data
- Raw test results
- Resource monitoring data
- System logs

#### Test Reports
- Performance test report
- Performance analysis report
- Optimization recommendations report

#### Test Tools
- Test scripts
- Analysis scripts
- Monitoring scripts

---

**End of Document**

- **Maintainer**: UVHTTP Development Team
- **Email**: dev@uvhttp.org
- **Issue**: https://github.com/adam-ikari/uvhttp/issues

---

**End of Document**

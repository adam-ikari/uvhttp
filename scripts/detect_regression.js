#!/usr/bin/env node
/**
 * Performance regression detection script for UVHTTP
 * 
 * This script compares current performance results with baseline
 * and detects performance regressions and improvements.
 */

const fs = require('fs').promises;

/**
 * Performance thresholds configuration
 */
const THRESHOLDS = {
  rps: {
    warning: 0.10,  // 10% decrease triggers warning
    failure: 0.10   // 10% decrease triggers failure
  },
  latency: {
    warning: 0.10,  // 10% increase triggers warning
    failure: 0.20   // 20% increase triggers failure
  }
};

/**
 * Performance baseline values
 */
const BASELINE = {
  low_concurrent: {
    rps: 17798,
    latency_avg: 518,  // μs
    latency_p99: 1600  // μs
  },
  medium_concurrent: {
    rps: 17209,
    latency_avg: 2790,  // μs (2.79 ms)
    latency_p99: 8500   // μs (8.5 ms)
  },
  high_concurrent: {
    rps: 16623,
    latency_avg: 12200,  // μs (12.2 ms)
    latency_p99: 40000   // μs (40 ms)
  }
};

/**
 * Detect performance regression
 * @param {Object} currentData - Current test data
 * @returns {Object} Object containing failures, warnings, and improvements
 */
function detectRegression(currentData) {
  const failures = [];
  const warnings = [];
  const improvements = [];
  
  const testScenarios = currentData.test_scenarios || [];
  
  for (const scenario of testScenarios) {
    const scenarioName = scenario.name;
    const results = scenario.results || {};
    
    if (!BASELINE[scenarioName]) {
      continue;
    }
    
    const baseline = BASELINE[scenarioName];
    
    // Check RPS
    const currentRps = results.rps?.value || 0;
    const baselineRps = baseline.rps;
    const rpsChange = (currentRps - baselineRps) / baselineRps;
    
    if (rpsChange < -THRESHOLDS.rps.failure) {
      failures.push({
        scenario: scenarioName,
        metric: 'RPS',
        current: currentRps,
        baseline: baselineRps,
        change: rpsChange * 100,
        severity: 'failure'
      });
    } else if (rpsChange < -THRESHOLDS.rps.warning) {
      warnings.push({
        scenario: scenarioName,
        metric: 'RPS',
        current: currentRps,
        baseline: baselineRps,
        change: rpsChange * 100,
        severity: 'warning'
      });
    } else if (rpsChange > THRESHOLDS.rps.warning) {
      improvements.push({
        scenario: scenarioName,
        metric: 'RPS',
        current: currentRps,
        baseline: baselineRps,
        change: rpsChange * 100,
        severity: 'improvement'
      });
    }
    
    // Check average latency
    const currentLatencyAvg = results.latency_avg?.value || 0;
    const baselineLatencyAvg = baseline.latency_avg;
    const latencyChange = (currentLatencyAvg - baselineLatencyAvg) / baselineLatencyAvg;
    
    if (latencyChange > THRESHOLDS.latency.failure) {
      failures.push({
        scenario: scenarioName,
        metric: 'Average Latency',
        current: currentLatencyAvg,
        baseline: baselineLatencyAvg,
        change: latencyChange * 100,
        severity: 'failure'
      });
    } else if (latencyChange > THRESHOLDS.latency.warning) {
      warnings.push({
        scenario: scenarioName,
        metric: 'Average Latency',
        current: currentLatencyAvg,
        baseline: baselineLatencyAvg,
        change: latencyChange * 100,
        severity: 'warning'
      });
    } else if (latencyChange < -THRESHOLDS.latency.warning) {
      improvements.push({
        scenario: scenarioName,
        metric: 'Average Latency',
        current: currentLatencyAvg,
        baseline: baselineLatencyAvg,
        change: latencyChange * 100,
        severity: 'improvement'
      });
    }
  }
  
  return { failures, warnings, improvements };
}

/**
 * Generate regression detection report
 * @param {Array} failures - Array of failures
 * @param {Array} warnings - Array of warnings
 * @param {Array} improvements - Array of improvements
 * @returns {string} Markdown formatted report
 */
function generateReport(failures, warnings, improvements) {
  let report = '';
  
  if (failures.length > 0) {
    report += '## 🚨 Performance Regressions Detected\n\n';
    for (const failure of failures) {
      report += `### ${failure.scenario}: ${failure.metric}\n\n`;
      report += `- **Current**: ${failure.current.toFixed(0)}\n`;
      report += `- **Baseline**: ${failure.baseline.toFixed(0)}\n`;
      report += `- **Change**: ${failure.change.toFixed(2)}%\n`;
      report += `- **Severity**: ${failure.severity}\n\n`;
    }
  }
  
  if (warnings.length > 0) {
    report += '## ⚠️ Performance Warnings\n\n';
    for (const warning of warnings) {
      report += `### ${warning.scenario}: ${warning.metric}\n\n`;
      report += `- **Current**: ${warning.current.toFixed(0)}\n`;
      report += `- **Baseline**: ${warning.baseline.toFixed(0)}\n`;
      report += `- **Change**: ${warning.change.toFixed(2)}%\n`;
      report += `- **Severity**: ${warning.severity}\n\n`;
    }
  }
  
  if (improvements.length > 0) {
    report += '## 🎉 Performance Improvements\n\n';
    for (const improvement of improvements) {
      report += `### ${improvement.scenario}: ${improvement.metric}\n\n`;
      report += `- **Current**: ${improvement.current.toFixed(0)}\n`;
      report += `- **Baseline**: ${improvement.baseline.toFixed(0)}\n`;
      report += `- **Change**: ${improvement.change.toFixed(2)}%\n`;
      report += `- **Severity**: ${improvement.severity}\n\n`;
    }
  }
  
  if (failures.length === 0 && warnings.length === 0 && improvements.length === 0) {
    report += '## ✅ No Performance Changes Detected\n\n';
    report += 'All performance metrics are within acceptable ranges.\n';
  }
  
  return report;
}

/**
 * Main function
 */
async function main() {
  const args = process.argv.slice(2);
  
  if (args.length < 1) {
    console.error('Usage: node detect_regression.js <performance_results.json>');
    console.error('');
    console.error('Example:');
    console.error('  node detect_regression.js performance_results.json');
    process.exit(1);
  }
  
  const resultsFile = args[0];
  
  // Load current test data
  try {
    const content = await fs.readFile(resultsFile, 'utf-8');
    const currentData = JSON.parse(content);
    
    // Detect regression
    const { failures, warnings, improvements } = detectRegression(currentData);
    
    // Generate report
    const report = generateReport(failures, warnings, improvements);
    console.log(report);
    
    // Save report
    await fs.writeFile('regression_report.md', report, 'utf-8');
    
    // Return exit code
    if (failures.length > 0) {
      process.exit(1);
    } else if (warnings.length > 0) {
      process.exit(2);
    } else {
      process.exit(0);
    }
  } catch (error) {
    console.error('Error:', error.message);
    process.exit(1);
  }
}

main().catch(error => {
  console.error('Error:', error.message);
  process.exit(1);
});
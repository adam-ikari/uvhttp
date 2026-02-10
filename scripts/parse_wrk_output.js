#!/usr/bin/env node
/**
 * Parse wrk output and generate performance JSON in the correct format
 */

const fs = require('fs').promises;
const path = require('path');

/**
 * Parse wrk output file
 * @param {string} logFile - Path to wrk log file
 * @returns {Promise<number[]>} Array of RPS values
 */
async function parseWrkOutput(logFile) {
  try {
    const content = await fs.readFile(logFile, 'utf-8');
    
    // Extract RPS from each wrk run
    // Pattern: "12345 requests in 12.34s"
    const pattern = /(\d+)\s+requests in\s+([\d.]+)s/g;
    const matches = [];
    let match;
    
    while ((match = pattern.exec(content)) !== null) {
      const requests = parseInt(match[1], 10);
      const duration = parseFloat(match[2]);
      const rps = requests / duration;
      matches.push(rps);
    }
    
    return matches;
  } catch (error) {
    console.error(`Error reading file ${logFile}:`, error.message);
    process.exit(1);
  }
}

/**
 * Generate performance JSON in baseline format
 * @param {number[]} rpsValues - Array of RPS values
 * @returns {Object|null} Performance data object or null if invalid
 */
function generatePerformanceJson(rpsValues) {
  if (rpsValues.length < 3) {
    console.error('Error: Need at least 3 RPS values');
    return null;
  }
  
  // Map to scenarios
  const scenarios = {
    low_concurrent: {
      rps: Math.round(rpsValues[0]),
      latency_avg: 0,
      latency_p50: 0,
      latency_p95: 0,
      latency_p99: 0,
      latency_p999: 0,
      transfer_rate: 0,
      errors: 0
    },
    medium_concurrent: {
      rps: Math.round(rpsValues[1]),
      latency_avg: 0,
      latency_p50: 0,
      latency_p95: 0,
      latency_p99: 0,
      latency_p999: 0,
      transfer_rate: 0,
      errors: 0
    },
    high_concurrent: {
      rps: Math.round(rpsValues[2]),
      latency_avg: 0,
      latency_p50: 0,
      latency_p95: 0,
      latency_p99: 0,
      latency_p999: 0,
      transfer_rate: 0,
      errors: 0
    }
  };
  
  return {
    version: '1.0.0',
    updated_at: new Date().toISOString(),
    commit: {
      sha: 'current',
      short_sha: 'current',
      message: 'Current performance test',
      branch: 'pr'
    },
    environment: {
      os: 'Linux',
      runner: 'GitHub Actions',
      cpu: 'Unknown',
      cores: 0,
      memory: 'Unknown',
      compiler: 'GCC',
      compiler_version: 'Unknown',
      build_type: 'Release',
      optimization: '-O2'
    },
    scenarios,
    thresholds: {
      rps_warning: 0.1,
      rps_failure: 0.1,
      latency_warning: 0.1,
      latency_failure: 0.2,
      min_improvement: 0.05
    }
  };
}

/**
 * Main function
 */
async function main() {
  const args = process.argv.slice(2);
  
  if (args.length < 1) {
    console.error('Usage: node parse_wrk_output.js <wrk_log_file> [output_file]');
    console.error('');
    console.error('Example:');
    console.error('  node parse_wrk_output.js wrk_output.log performance.json');
    process.exit(1);
  }
  
  const logFile = args[0];
  const outputFile = args[1] || 'performance.json';
  
  // Parse wrk output
  const rpsValues = await parseWrkOutput(logFile);
  
  if (rpsValues.length === 0) {
    console.error('Error: No RPS values found in wrk output');
    process.exit(1);
  }
  
  // Generate JSON
  const performanceData = generatePerformanceJson(rpsValues);
  
  if (!performanceData) {
    process.exit(1);
  }
  
  // Write to file
  await fs.writeFile(outputFile, JSON.stringify(performanceData, null, 2), 'utf-8');
  
  console.log(` Performance data written to ${outputFile}`);
  console.log(`   RPS values: ${rpsValues.map(v => v.toFixed(2)).join(', ')}`);
}

main().catch(error => {
  console.error('Error:', error.message);
  process.exit(1);
});
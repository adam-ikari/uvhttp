#!/usr/bin/env node
/**
 * Update performance baseline script
 * 
 * This script updates the performance baseline file with current test results
 */

const fs = require('fs').promises;
const path = require('path');

/**
 * Load current performance test results
 * @param {string} resultsFile - Path to results file
 * @returns {Promise<Object|null>} Parsed results or null
 */
async function loadCurrentResults(resultsFile) {
  try {
    const content = await fs.readFile(resultsFile, 'utf-8');
    return JSON.parse(content);
  } catch (error) {
    console.error(`Error loading results: ${error.message}`);
    return null;
  }
}

/**
 * Load current baseline
 * @param {string} baselineFile - Path to baseline file
 * @returns {Promise<Object|null>} Parsed baseline or null
 */
async function loadBaseline(baselineFile) {
  try {
    const content = await fs.readFile(baselineFile, 'utf-8');
    return JSON.parse(content);
  } catch (error) {
    console.error(`Baseline file not found, will create new: ${error.message}`);
    return null;
  }
}

/**
 * Extract baseline data from results
 * @param {Object} results - Performance test results
 * @param {string} version - Version string
 * @param {string} commit - Commit SHA
 * @returns {Object|null} Baseline data or null
 */
function extractBaselineData(results, version, commit) {
  if (!results) {
    return null;
  }
  
  const baselineData = {
    version: version || 'unknown',
    date: new Date().toISOString(),
    commit: commit || 'unknown',
    baseline: {}
  };
  
  // Extract RPS data
  if (results.test_scenarios) {
    for (const scenario of results.test_scenarios) {
      const scenarioName = scenario.name || 'unknown';
      
      if (scenario.results) {
        const scenarioData = {};
        
        if (scenario.results.rps) {
          scenarioData.rps = scenario.results.rps.value;
        }
        
        if (scenario.results.latency_avg) {
          const latency = scenario.results.latency_avg.value;
          if (latency !== 'N/A') {
            scenarioData.latency_avg = latency;
          }
        }
        
        if (Object.keys(scenarioData).length > 0) {
          baselineData.baseline[scenarioName] = scenarioData;
        }
      }
    }
  }
  
  // Extract environment information
  if (results.environment) {
    baselineData.environment = {
      os: results.environment.os || 'unknown',
      runner: results.environment.runner || 'unknown',
      cpu: results.environment.cpu || 'unknown',
      compiler: results.environment.compiler || 'unknown',
      compiler_version: results.environment.compiler_version || 'unknown',
      build_type: results.environment.build_type || 'Release',
      optimization: results.environment.optimization || '-O2'
    };
  }
  
  return baselineData;
}

/**
 * Update baseline file
 * @param {string} baselineFile - Path to baseline file
 * @param {Object} newBaseline - New baseline data
 * @param {boolean} backup - Whether to backup old baseline
 * @returns {Promise<boolean>} Success status
 */
async function updateBaselineFile(baselineFile, newBaseline, backup = true) {
  try {
    // Backup old baseline
    if (backup) {
      try {
        await fs.copyFile(baselineFile, `${baselineFile}.backup`);
        console.log(`Baseline backed up to ${baselineFile}.backup`);
      } catch (error) {
        // File doesn't exist, skip backup
      }
    }
    
    // Write new baseline
    await fs.writeFile(baselineFile, JSON.stringify(newBaseline, null, 2), 'utf-8');
    
    console.log(`✅ Baseline updated: ${baselineFile}`);
    console.log(`   Version: ${newBaseline.version}`);
    console.log(`   Date: ${newBaseline.date}`);
    
    return true;
  } catch (error) {
    console.error(`Error updating baseline: ${error.message}`);
    return false;
  }
}

/**
 * Update history file
 * @param {string} historyFile - Path to history file
 * @param {Object} newBaseline - New baseline data
 * @param {number} maxEntries - Maximum history entries
 * @returns {Promise<boolean>} Success status
 */
async function updateHistoryFile(historyFile, newBaseline, maxEntries = 30) {
  try {
    let history = [];
    
    // Load existing history
    try {
      const content = await fs.readFile(historyFile, 'utf-8');
      history = JSON.parse(content);
    } catch (error) {
      history = [];
    }
    
    // Add new entry
    history.unshift(newBaseline);
    
    // Limit entries
    history = history.slice(0, maxEntries);
    
    // Write history file
    await fs.writeFile(historyFile, JSON.stringify(history, null, 2), 'utf-8');
    
    console.log(`✅ History updated: ${historyFile} (${history.length} entries)`);
    
    return true;
  } catch (error) {
    console.error(`Error updating history: ${error.message}`);
    return false;
  }
}

/**
 * Compare new baseline with old baseline
 * @param {Object} newBaseline - New baseline data
 * @param {Object} oldBaseline - Old baseline data
 */
function compareWithBaseline(newBaseline, oldBaseline) {
  if (!oldBaseline) {
    console.log('No old baseline found for comparison');
    return;
  }
  
  console.log('\n=== Baseline Comparison ===\n');
  
  const newBenchmarks = newBaseline.baseline || {};
  const oldBenchmarks = oldBaseline.baseline || {};
  
  const allScenarios = new Set([
    ...Object.keys(newBenchmarks),
    ...Object.keys(oldBenchmarks)
  ]);
  
  for (const scenario of Array.from(allScenarios).sort()) {
    const newData = newBenchmarks[scenario] || {};
    const oldData = oldBenchmarks[scenario] || {};
    
    console.log(`Scenario: ${scenario}`);
    
    if ('rps' in newData && 'rps' in oldData) {
      const change = ((newData.rps - oldData.rps) / oldData.rps) * 100;
      console.log(`  RPS: ${oldData.rps.toFixed(2)} → ${newData.rps.toFixed(2)} (${change >= 0 ? '+' : ''}${change.toFixed(2)}%)`);
    }
    
    if ('latency_avg' in newData && 'latency_avg' in oldData) {
      const change = ((newData.latency_avg - oldData.latency_avg) / oldData.latency_avg) * 100;
      console.log(`  Latency: ${oldData.latency_avg.toFixed(2)}ms → ${newData.latency_avg.toFixed(2)}ms (${change >= 0 ? '+' : ''}${change.toFixed(2)}%)`);
    }
    
    console.log('');
  }
}

/**
 * Main function
 */
async function main() {
  const args = process.argv.slice(2);
  
  // Parse arguments
  const options = {
    results: null,
    baseline: 'docs/performance/baseline.json',
    history: 'docs/performance/baseline-history.json',
    version: null,
    commit: null,
    noBackup: false,
    noHistory: false,
    maxHistory: 30,
    compare: false
  };
  
  for (let i = 0; i < args.length; i++) {
    switch (args[i]) {
      case '--results':
      case '-r':
        options.results = args[++i];
        break;
      case '--baseline':
      case '-b':
        options.baseline = args[++i];
        break;
      case '--history':
      case '-H':
        options.history = args[++i];
        break;
      case '--version':
      case '-v':
        options.version = args[++i];
        break;
      case '--commit':
      case '-c':
        options.commit = args[++i];
        break;
      case '--no-backup':
        options.noBackup = true;
        break;
      case '--no-history':
        options.noHistory = true;
        break;
      case '--max-history':
        options.maxHistory = parseInt(args[++i], 10);
        break;
      case '--compare':
        options.compare = true;
        break;
      default:
        console.error(`Unknown option: ${args[i]}`);
        process.exit(1);
    }
  }
  
  if (!options.results) {
    console.error('Error: --results is required');
    console.error('');
    console.error('Usage: node update_baseline.js --results <performance_results.json> [options]');
    console.error('');
    console.error('Options:');
    console.error('  --results, -r    Path to performance results JSON (required)');
    console.error('  --baseline, -b   Path to baseline file (default: docs/performance/baseline.json)');
    console.error('  --history, -H    Path to history file (default: docs/performance/baseline-history.json)');
    console.error('  --version, -v     Version string (default: auto-detect)');
    console.error('  --commit, -c      Commit SHA (default: auto-detect)');
    console.error('  --no-backup       Skip backup of old baseline');
    console.error('  --no-history      Skip updating history file');
    console.error('  --max-history N   Maximum history entries (default: 30)');
    console.error('  --compare         Compare with old baseline');
    console.error('');
    console.error('Example:');
    console.error('  node update_baseline.js --results performance.json');
    process.exit(1);
  }
  
  // Load results
  const results = await loadCurrentResults(options.results);
  if (!results) {
    console.error('Failed to load results');
    process.exit(1);
  }
  
  // Extract version and commit info
  let version = options.version;
  let commit = options.commit;
  
  if (!version) {
    version = results.version || 'unknown';
  }
  
  if (!commit) {
    if (results.commit) {
      commit = results.commit.sha || 'unknown';
    } else {
      commit = 'unknown';
    }
  }
  
  // Extract baseline data
  const newBaseline = extractBaselineData(results, version, commit);
  if (!newBaseline) {
    console.error('Failed to extract baseline data');
    process.exit(1);
  }
  
  // Load old baseline
  const oldBaseline = options.compare || await loadBaseline(options.baseline);
  
  // Compare baseline
  if (options.compare && oldBaseline) {
    compareWithBaseline(newBaseline, oldBaseline);
  }
  
  // Update baseline file
  const baselineDir = path.dirname(options.baseline);
  if (baselineDir) {
    await fs.mkdir(baselineDir, { recursive: true });
  }
  
  await updateBaselineFile(options.baseline, newBaseline, !options.noBackup);
  
  // Update history file
  if (!options.noHistory) {
    const historyDir = path.dirname(options.history);
    if (historyDir) {
      await fs.mkdir(historyDir, { recursive: true });
    }
    
    await updateHistoryFile(options.history, newBaseline, options.maxHistory);
  }
  
  console.log('\n✅ Baseline update completed successfully');
}

main().catch(error => {
  console.error('Error:', error.message);
  process.exit(1);
});
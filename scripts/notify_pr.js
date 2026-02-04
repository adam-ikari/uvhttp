#!/usr/bin/env node
/**
 * PR notification script
 * 
 * This script generates PR notification comments with CI/CD results,
 * performance comparisons, and test results
 */

const fs = require('fs').promises;

/**
 * Load performance test results
 * @param {string} resultsFile - Path to results file
 * @returns {Promise<Object|null>} Parsed results or null
 */
async function loadPerformanceResults(resultsFile) {
  try {
    const content = await fs.readFile(resultsFile, 'utf-8');
    return JSON.parse(content);
  } catch (error) {
    console.error(`Error loading performance results: ${error.message}`);
    return null;
  }
}

/**
 * Load performance baseline
 * @param {string} baselineFile - Path to baseline file
 * @returns {Promise<Object|null>} Parsed baseline or null
 */
async function loadBaseline(baselineFile) {
  try {
    const content = await fs.readFile(baselineFile, 'utf-8');
    return JSON.parse(content);
  } catch (error) {
    console.error(`Error loading baseline: ${error.message}`);
    return null;
  }
}

/**
 * Compare current performance with baseline
 * @param {Object} current - Current performance data
 * @param {Object} baseline - Baseline performance data
 * @returns {Object} Comparison results
 */
function comparePerformance(current, baseline) {
  const results = {
    regression: false,
    improvement: false,
    details: []
  };
  
  if (!current || !baseline) {
    return results;
  }
  
  // Get RPS data
  let currentRps = null;
  let baselineRps = null;
  
  if (current.test_scenarios) {
    for (const scenario of current.test_scenarios) {
      if (scenario.results && scenario.results.rps) {
        currentRps = scenario.results.rps.value;
        break;
      }
    }
  }
  
  if (baseline.baseline && baseline.baseline.rps) {
    baselineRps = baseline.baseline.rps;
  }
  
  if (currentRps && baselineRps) {
    const change = ((currentRps - baselineRps) / baselineRps) * 100;
    
    results.details.push({
      metric: 'RPS',
      baseline: baselineRps,
      current: currentRps,
      change: change
    });
    
    // Regression threshold: RPS decreases by more than 10%
    if (change < -10) {
      results.regression = true;
    }
    // Improvement threshold: RPS increases by more than 10%
    else if (change > 10) {
      results.improvement = true;
    }
  }
  
  return results;
}

/**
 * Generate PR comment content
 * @param {string} workflowName - Workflow name
 * @param {string} conclusion - Workflow conclusion
 * @param {string} runId - Workflow run ID
 * @param {string} runNumber - Workflow run number
 * @param {Object} performanceComparison - Performance comparison results
 * @param {Object} testResults - Test results
 * @returns {string} PR comment content
 */
function generatePRComment(workflowName, conclusion, runId, runNumber, performanceComparison, testResults) {
  let comment = `## 🤖 CI/CD PR Validation Results\n\n`;
  comment += `**Workflow**: ${workflowName}\n`;
  comment += `**Run**: #${runNumber}\n`;
  
  // Status icon
  if (conclusion === 'success') {
    comment += `**Status**:  Passed\n\n`;
  } else {
    comment += `**Status**:  Failed\n\n`;
  }
  
  // Performance comparison
  if (performanceComparison) {
    comment += `###  Performance Comparison\n\n`;
    
    if (performanceComparison.regression) {
      comment += ` **Performance Regression Detected**\n\n`;
    } else if (performanceComparison.improvement) {
      comment += `✨ **Performance Improvement Detected**\n\n`;
    } else {
      comment += ` **Performance Stable**\n\n`;
    }
    
    comment += `| Metric | Baseline | Current | Change |\n`;
    comment += `|--------|----------|---------|--------|\n`;
    
    for (const detail of performanceComparison.details) {
      const changePct = detail.change;
      let changeStr = `${changePct >= 0 ? '+' : ''}${changePct.toFixed(2)}%`;
      
      if (changePct < -10) {
        changeStr = `🔴 ${changeStr}`;
      } else if (changePct > 10) {
        changeStr = `🟢 ${changeStr}`;
      }
      
      comment += `| ${detail.metric} | ${detail.baseline.toFixed(2)} | ${detail.current.toFixed(2)} | ${changeStr} |\n`;
    }
    
    comment += `\n`;
  }
  
  // Test results
  if (testResults) {
    comment += `### 🧪 Test Results\n\n`;
    comment += `- Total: ${testResults.total || 0}\n`;
    comment += `- Passed: ${testResults.passed || 0} \n`;
    comment += `- Failed: ${testResults.failed || 0} \n`;
    comment += `- Skipped: ${testResults.skipped || 0} ⏭️\n\n`;
  }
  
  // Detailed links
  comment += `### 🔗 Links\n\n`;
  comment += `- [View Workflow Run](https://github.com/adam-ikari/uvhttp/actions/runs/${runId})\n`;
  
  if (conclusion === 'success') {
    comment += `\nReady for review! 🎉`;
  } else {
    comment += `\nPlease fix the issues and push a new commit.`;
  }
  
  return comment;
}

/**
 * Main function
 */
async function main() {
  const args = process.argv.slice(2);
  
  // Parse arguments
  const options = {
    workflowName: null,
    conclusion: null,
    runId: null,
    runNumber: null,
    performanceResults: null,
    baseline: null,
    testResults: null,
    output: null
  };
  
  for (let i = 0; i < args.length; i++) {
    switch (args[i]) {
      case '--workflow-name':
        options.workflowName = args[++i];
        break;
      case '--conclusion':
        options.conclusion = args[++i];
        break;
      case '--run-id':
        options.runId = args[++i];
        break;
      case '--run-number':
        options.runNumber = args[++i];
        break;
      case '--performance-results':
        options.performanceResults = args[++i];
        break;
      case '--baseline':
        options.baseline = args[++i];
        break;
      case '--test-results':
        options.testResults = args[++i];
        break;
      case '--output':
      case '-o':
        options.output = args[++i];
        break;
      default:
        console.error(`Unknown option: ${args[i]}`);
        process.exit(1);
    }
  }
  
  // Validate required arguments
  if (!options.workflowName || !options.conclusion || !options.runId || !options.runNumber) {
    console.error('Error: --workflow-name, --conclusion, --run-id, and --run-number are required');
    console.error('');
    console.error('Usage: node notify_pr.js --workflow-name <name> --conclusion <status>');
    console.error('                         --run-id <id> --run-number <num> [options]');
    console.error('');
    console.error('Options:');
    console.error('  --workflow-name <name>      Workflow name (required)');
    console.error('  --conclusion <status>       Workflow conclusion: success, failure, cancelled, skipped (required)');
    console.error('  --run-id <id>               Workflow run ID (required)');
    console.error('  --run-number <num>          Workflow run number (required)');
    console.error('  --performance-results <path> Path to performance results JSON');
    console.error('  --baseline <path>           Path to baseline JSON');
    console.error('  --test-results <path>       Path to test results JSON');
    console.error('  --output, -o <path>         Output file (default: stdout)');
    console.error('');
    console.error('Example:');
    console.error('  node notify_pr.js --workflow-name "CI" --conclusion success --run-id 123456 --run-number 42');
    process.exit(1);
  }
  
  // Validate conclusion
  const validConclusions = ['success', 'failure', 'cancelled', 'skipped'];
  if (!validConclusions.includes(options.conclusion)) {
    console.error(`Error: Invalid conclusion "${options.conclusion}"`);
    console.error(`Valid values: ${validConclusions.join(', ')}`);
    process.exit(1);
  }
  
  // Load data
  let performanceData = null;
  let baselineData = null;
  let testData = null;
  
  if (options.performanceResults) {
    performanceData = await loadPerformanceResults(options.performanceResults);
  }
  
  if (options.baseline) {
    baselineData = await loadBaseline(options.baseline);
  }
  
  if (options.testResults) {
    try {
      const content = await fs.readFile(options.testResults, 'utf-8');
      testData = JSON.parse(content);
    } catch (error) {
      console.error(`Error loading test results: ${error.message}`);
    }
  }
  
  // Compare performance
  let performanceComparison = null;
  if (performanceData && baselineData) {
    performanceComparison = comparePerformance(performanceData, baselineData);
  }
  
  // Generate comment
  const comment = generatePRComment(
    options.workflowName,
    options.conclusion,
    options.runId,
    options.runNumber,
    performanceComparison,
    testData
  );
  
  // Output
  if (options.output) {
    await fs.writeFile(options.output, comment, 'utf-8');
    console.log(`Comment written to ${options.output}`);
  } else {
    console.log(comment);
  }
}

main().catch(error => {
  console.error('Error:', error.message);
  process.exit(1);
});
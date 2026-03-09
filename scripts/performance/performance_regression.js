#!/usr/bin/env node
/**
 * Performance regression detection script for UVHTTP
 * 
 * This script compares current performance results with baseline
 * and detects performance regressions and improvements.
 */

const fs = require('fs').promises;

/**
 * Load JSON file
 * @param {string} filepath - Path to JSON file
 * @returns {Promise<Object>} Parsed JSON object
 */
async function loadJsonFile(filepath) {
  try {
    const content = await fs.readFile(filepath, 'utf-8');
    return JSON.parse(content);
  } catch (error) {
    throw new Error(`Error loading ${filepath}: ${error.message}`);
  }
}

/**
 * Performance baseline manager
 */
class PerformanceBaseline {
  /**
   * Initialize baseline manager
   * @param {string} baselineFile - Path to baseline file
   */
  constructor(baselineFile) {
    this.baselineFile = baselineFile;
    this.baseline = this._loadBaseline();
  }

  /**
   * Load baseline from file
   * @returns {Object} Baseline data
   */
  _loadBaseline() {
    try {
      const content = require('fs').readFileSync(this.baselineFile, 'utf-8');
      return JSON.parse(content);
    } catch (error) {
      if (error.code === 'ENOENT') {
        console.warn(`Warning: Baseline file not found: ${this.baselineFile}`);
      } else {
        console.error(`Error: Invalid JSON in baseline file: ${error.message}`);
      }
      return {};
    }
  }

  /**
   * Get baseline for a specific scenario
   * @param {string} scenarioName - Scenario name
   * @returns {Object|null} Baseline data or null
   */
  getScenarioBaseline(scenarioName) {
    const scenarios = this.baseline.scenarios || {};
    return scenarios[scenarioName] || null;
  }
}

/**
 * Performance thresholds configuration
 */
class PerformanceThresholds {
  /**
   * Default thresholds
   */
  static DEFAULT_THRESHOLDS = {
    rps_warning: 0.10,      // 10% decrease triggers warning
    rps_failure: 0.10,      // 10% decrease triggers failure
    latency_warning: 0.10,  // 10% increase triggers warning
    latency_failure: 0.20,  // 20% increase triggers failure
    min_improvement: 0.05   // 5% improvement is considered significant
  };

  /**
   * Initialize thresholds
   * @param {Object} thresholds - Custom thresholds
   */
  constructor(thresholds = null) {
    this.thresholds = thresholds || { ...PerformanceThresholds.DEFAULT_THRESHOLDS };
  }

  /**
   * Load thresholds from file
   * @param {string} filePath - Path to thresholds file
   * @returns {PerformanceThresholds} Thresholds instance
   */
  static async fromFile(filePath) {
    try {
      const data = await loadJsonFile(filePath);
      const thresholds = data.thresholds || {};
      return new PerformanceThresholds(thresholds);
    } catch (error) {
      return new PerformanceThresholds();
    }
  }
}

/**
 * Performance result comparator
 */
class PerformanceComparator {
  /**
   * Initialize comparator
   * @param {PerformanceBaseline} baseline - Baseline manager
   * @param {PerformanceThresholds} thresholds - Thresholds configuration
   */
  constructor(baseline, thresholds) {
    this.baseline = baseline;
    this.thresholds = thresholds;
  }

  /**
   * Compare current results with baseline
   * @param {Object} currentResults - Current performance results
   * @returns {Object} Performance report
   */
  compare(currentResults) {
    const report = {
      regressions: [],
      improvements: [],
      warnings: [],
      info: [],

      hasFailure() {
        return this.regressions.some(r => r.severity === 'failure');
      },

      hasWarning() {
        return this.warnings.length > 0;
      },

      hasImprovement() {
        return this.improvements.length > 0;
      }
    };

    const testScenarios = currentResults.test_scenarios || [];

    for (const scenario of testScenarios) {
      const scenarioName = scenario.name;
      const scenarioResults = scenario.results || {};

      // Get baseline for this scenario
      const baselineScenario = this.baseline.getScenarioBaseline(scenarioName);
      if (!baselineScenario) {
        continue;
      }

      // Check RPS
      this._checkRps(scenarioName, scenarioResults, baselineScenario, report);

      // Check latency
      this._checkLatency(scenarioName, scenarioResults, baselineScenario, report);
    }

    return report;
  }

  /**
   * Check RPS regression
   * @param {string} scenarioName - Scenario name
   * @param {Object} currentResults - Current results
   * @param {Object} baselineScenario - Baseline scenario
   * @param {Object} report - Report object
   */
  _checkRps(scenarioName, currentResults, baselineScenario, report) {
    const currentRps = currentResults.rps?.value || 0;
    const baselineRps = baselineScenario.rps || 0;

    if (baselineRps === 0) {
      return;
    }

    const rpsChange = (currentRps - baselineRps) / baselineRps;

    // Check for regression
    if (rpsChange < -this.thresholds.thresholds.rps_failure) {
      report.regressions.push({
        scenario: scenarioName,
        metric: 'rps',
        baseline: baselineRps,
        current: currentRps,
        change: rpsChange * 100,
        severity: 'failure'
      });
    } else if (rpsChange < -this.thresholds.thresholds.rps_warning) {
      report.warnings.push({
        scenario: scenarioName,
        metric: 'rps',
        baseline: baselineRps,
        current: currentRps,
        change: rpsChange * 100,
        severity: 'warning'
      });
    }
    // Check for improvement
    else if (rpsChange > this.thresholds.thresholds.min_improvement) {
      report.improvements.push({
        scenario: scenarioName,
        metric: 'rps',
        baseline: baselineRps,
        current: currentRps,
        change: rpsChange * 100,
        severity: 'info'
      });
    }
  }

  /**
   * Check latency regression
   * @param {string} scenarioName - Scenario name
   * @param {Object} currentResults - Current results
   * @param {Object} baselineScenario - Baseline scenario
   * @param {Object} report - Report object
   */
  _checkLatency(scenarioName, currentResults, baselineScenario, report) {
    // Check P99 latency
    const currentP99 = currentResults.latency_p99?.value || 0;
    const baselineP99 = baselineScenario.latency_p99 || 0;

    if (baselineP99 === 0) {
      return;
    }

    const latencyChange = (currentP99 - baselineP99) / baselineP99;

    // Check for regression
    if (latencyChange > this.thresholds.thresholds.latency_failure) {
      report.regressions.push({
        scenario: scenarioName,
        metric: 'latency_p99',
        baseline: baselineP99,
        current: currentP99,
        change: latencyChange * 100,
        severity: 'failure'
      });
    } else if (latencyChange > this.thresholds.thresholds.latency_warning) {
      report.warnings.push({
        scenario: scenarioName,
        metric: 'latency_p99',
        baseline: baselineP99,
        current: currentP99,
        change: latencyChange * 100,
        severity: 'warning'
      });
    }
  }
}

/**
 * Performance report generator
 */
class ReportGenerator {
  /**
   * Generate markdown report
   * @param {Object} report - Performance report
   * @returns {string} Markdown report
   */
  static generateMarkdown(report) {
    let md = '# Performance Regression Report\n\n';

    // Add regressions
    if (report.regressions.length > 0) {
      md += '##  Regressions Detected\n\n';
      md += '| Scenario | Metric | Baseline | Current | Change |\n';
      md += '|----------|--------|----------|---------|--------|\n';
      for (const reg of report.regressions) {
        const icon = reg.severity === 'failure' ? '🔴' : '🟡';
        md += `| ${icon} ${reg.scenario} | ${reg.metric} | `;
        md += `${reg.baseline.toFixed(2)} | ${reg.current.toFixed(2)} | `;
        md += `${reg.change >= 0 ? '+' : ''}${reg.change.toFixed(2)}% |\n`;
      }
      md += '\n';
    }

    // Add warnings
    if (report.warnings.length > 0) {
      md += '##  Warnings\n\n';
      md += '| Scenario | Metric | Baseline | Current | Change |\n';
      md += '|----------|--------|----------|---------|--------|\n';
      for (const warn of report.warnings) {
        md += `| 🟡 ${warn.scenario} | ${warn.metric} | `;
        md += `${warn.baseline.toFixed(2)} | ${warn.current.toFixed(2)} | `;
        md += `${warn.change >= 0 ? '+' : ''}${warn.change.toFixed(2)}% |\n`;
      }
      md += '\n';
    }

    // Add improvements
    if (report.improvements.length > 0) {
      md += '##  Improvements\n\n';
      md += '| Scenario | Metric | Baseline | Current | Change |\n';
      md += '|----------|--------|----------|---------|--------|\n';
      for (const imp of report.improvements) {
        md += `| 🟢 ${imp.scenario} | ${imp.metric} | `;
        md += `${imp.baseline.toFixed(2)} | ${imp.current.toFixed(2)} | `;
        md += `${imp.change >= 0 ? '+' : ''}${imp.change.toFixed(2)}% |\n`;
      }
      md += '\n';
    }

    // Add summary
    md += '## Summary\n\n';
    md += `- Regressions: ${report.regressions.length}\n`;
    md += `- Warnings: ${report.warnings.length}\n`;
    md += `- Improvements: ${report.improvements.length}\n`;

    // Add overall status
    if (report.hasFailure()) {
      md += '\n**Overall Status**:  FAILED - Performance regression detected\n';
    } else if (report.hasWarning()) {
      md += '\n**Overall Status**:  WARNING - Performance degradation detected\n';
    } else if (report.hasImprovement()) {
      md += '\n**Overall Status**:  PASSED - Performance improvements detected\n';
    } else {
      md += '\n**Overall Status**:  PASSED - No significant changes\n';
    }

    return md;
  }

  /**
   * Generate JSON report
   * @param {Object} report - Performance report
   * @returns {Object} JSON report
   */
  static generateJson(report) {
    return {
      summary: {
        regressions: report.regressions.length,
        warnings: report.warnings.length,
        improvements: report.improvements.length,
        has_failure: report.hasFailure(),
        has_warning: report.hasWarning(),
        has_improvement: report.hasImprovement()
      },
      regressions: report.regressions.map(r => ({
        scenario: r.scenario,
        metric: r.metric,
        baseline: r.baseline,
        current: r.current,
        change: r.change,
        severity: r.severity
      })),
      warnings: report.warnings.map(w => ({
        scenario: w.scenario,
        metric: w.metric,
        baseline: w.baseline,
        current: w.current,
        change: w.change,
        severity: w.severity
      })),
      improvements: report.improvements.map(i => ({
        scenario: i.scenario,
        metric: i.metric,
        baseline: i.baseline,
        current: i.current,
        change: i.change,
        severity: i.severity
      }))
    };
  }
}

/**
 * Main function
 */
async function main() {
  const args = process.argv.slice(2);

  // Parse arguments
  const options = {
    current: null,
    baseline: null,
    output: null,
    format: 'markdown',
    thresholds: null,
    failOnRegression: false
  };

  for (let i = 0; i < args.length; i++) {
    switch (args[i]) {
      case '--output':
        options.output = args[++i];
        break;
      case '--format':
        options.format = args[++i];
        break;
      case '--thresholds':
        options.thresholds = args[++i];
        break;
      case '--fail-on-regression':
        options.failOnRegression = true;
        break;
      default:
        if (!options.current) {
          options.current = args[i];
        } else if (!options.baseline) {
          options.baseline = args[i];
        } else {
          console.error(`Unknown option: ${args[i]}`);
          process.exit(1);
        }
    }
  }

  // Validate required arguments
  if (!options.current || !options.baseline) {
    console.error('Error: Current and baseline files are required');
    console.error('');
    console.error('Usage: node performance_regression.js <current.json> <baseline.json> [options]');
    console.error('');
    console.error('Options:');
    console.error('  --output <path>          Output file (default: stdout)');
    console.error('  --format <format>        Output format: markdown, json, both (default: markdown)');
    console.error('  --thresholds <path>      Thresholds configuration file');
    console.error('  --fail-on-regression     Exit with error if regression detected');
    console.error('');
    console.error('Example:');
    console.error('  node performance_regression.js current.json baseline.json --output report.md');
    process.exit(1);
  }

  // Validate format
  const validFormats = ['markdown', 'json', 'both'];
  if (!validFormats.includes(options.format)) {
    console.error(`Error: Invalid format "${options.format}"`);
    console.error(`Valid values: ${validFormats.join(', ')}`);
    process.exit(1);
  }

  // Load current results
  let currentResults;
  try {
    currentResults = await loadJsonFile(options.current);
  } catch (error) {
    console.error(error.message);
    process.exit(1);
  }

  // Load baseline
  const baseline = new PerformanceBaseline(options.baseline);

  // Load thresholds
  let thresholds;
  if (options.thresholds) {
    thresholds = await PerformanceThresholds.fromFile(options.thresholds);
  } else {
    thresholds = new PerformanceThresholds();
  }

  // Compare results
  const comparator = new PerformanceComparator(baseline, thresholds);
  const report = comparator.compare(currentResults);

  // Generate output
  if (options.format === 'markdown' || options.format === 'both') {
    const markdownReport = ReportGenerator.generateMarkdown(report);
    if (options.output && options.format === 'markdown') {
      await fs.writeFile(options.output, markdownReport, 'utf-8');
      console.log(`Markdown report written to ${options.output}`);
    } else {
      console.log(markdownReport);
    }
  }

  if (options.format === 'json' || options.format === 'both') {
    const jsonReport = ReportGenerator.generateJson(report);
    const jsonOutput = JSON.stringify(jsonReport, null, 2);
    if (options.output && options.format === 'json') {
      await fs.writeFile(options.output, jsonOutput, 'utf-8');
      console.log(`JSON report written to ${options.output}`);
    } else if (options.format === 'both') {
      const jsonFile = options.output.replace('.md', '.json');
      await fs.writeFile(jsonFile, jsonOutput, 'utf-8');
      console.log(`JSON report written to ${jsonFile}`);
    } else {
      console.log(jsonOutput);
    }
  }

  // Exit with error if regression detected
  if (options.failOnRegression && report.hasFailure()) {
    process.exit(1);
  }
}

main().catch(error => {
  console.error('Error:', error.message);
  process.exit(1);
});

#!/usr/bin/env node
/**
 * Generate performance trend chart using Mermaid
 * 
 * This script generates Mermaid charts for performance trends
 * including throughput, latency, and P99 latency trends
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
    console.warn(`Warning: Failed to load ${filepath}: ${error.message}`);
    return null;
  }
}

/**
 * Parse date string to timestamp
 * @param {string} dateString - Date string in ISO format
 * @returns {number} Unix timestamp
 */
function parseDate(dateString) {
  const date = new Date(dateString);
  return date.getTime();
}

/**
 * Format date for display
 * @param {string} dateString - Date string in ISO format
 * @returns {string} Formatted date string
 */
function formatDate(dateString) {
  const date = new Date(dateString);
  return date.toISOString().split('T')[0]; // YYYY-MM-DD
}

/**
 * Extract scenario data from historical data
 * @param {Array} historicalData - Historical performance data
 * @param {string} scenarioName - Scenario name
 * @param {string} metric - Metric name (rps, latency_avg, latency_p99)
 * @returns {Array} Array of { date, value }
 */
function extractScenarioData(historicalData, scenarioName, metric) {
  const data = [];
  for (const d of historicalData) {
    const date = formatDate(d.timestamp || d.date || d.updated_at);
    for (const s of d.test_scenarios || []) {
      if (s.name === scenarioName) {
        let value = null;
        if (metric === 'rps') {
          value = s.results?.rps?.value;
        } else if (metric === 'latency_avg') {
          value = s.results?.latency_avg?.value;
        } else if (metric === 'latency_p99') {
          value = s.results?.latency_p99?.value;
        }
        
        if (value !== null && value !== undefined && value !== 'N/A') {
          // Remove unit suffixes (us, ms)
          if (typeof value === 'string') {
            value = parseFloat(value.replace('us', '').replace('ms', ''));
          }
          data.push({ date, value });
        }
      }
    }
  }
  return data;
}

/**
 * Generate Mermaid xychart for throughput trends
 * @param {Array} historicalData - Historical performance data
 * @returns {string} Mermaid xychart code
 */
function generateThroughputChart(historicalData) {
  const rpsLow = extractScenarioData(historicalData, 'low_concurrent', 'rps');
  const rpsMedium = extractScenarioData(historicalData, 'medium_concurrent', 'rps');
  const rpsHigh = extractScenarioData(historicalData, 'high_concurrent', 'rps');
  
  // Find min and max values for y-axis
  const allValues = [
    ...rpsLow.map(d => d.value),
    ...rpsMedium.map(d => d.value),
    ...rpsHigh.map(d => d.value)
  ];
  const minValue = Math.min(...allValues) * 0.9;
  const maxValue = Math.max(...allValues) * 1.1;
  
  let chart = '```mermaid\nxychart-beta\n    title "Throughput Trends"\n';
  chart += `    x-axis [${rpsLow.map(d => d.date).join(', ')}]\n`;
  chart += `    y-axis "${minValue}" --> "${maxValue}"\n`;
  chart += '    line [';
  
  if (rpsLow.length > 0) {
    chart += rpsLow.map(d => d.value).join(', ');
  } else {
    chart += '0';
  }
  chart += ']\n';
  
  if (rpsMedium.length > 0) {
    chart += '    line [' + rpsMedium.map(d => d.value).join(', ') + ']\n';
  }
  
  if (rpsHigh.length > 0) {
    chart += '    line [' + rpsHigh.map(d => d.value).join(', ') + ']\n';
  }
  
  chart += '```\n';
  
  return chart;
}

/**
 * Generate Mermaid xychart for latency trends
 * @param {Array} historicalData - Historical performance data
 * @returns {string} Mermaid xychart code
 */
function generateLatencyChart(historicalData) {
  const latencyLow = extractScenarioData(historicalData, 'low_concurrent', 'latency_avg');
  const latencyMedium = extractScenarioData(historicalData, 'medium_concurrent', 'latency_avg');
  const latencyHigh = extractScenarioData(historicalData, 'high_concurrent', 'latency_avg');
  
  // Find min and max values for y-axis
  const allValues = [
    ...latencyLow.map(d => d.value),
    ...latencyMedium.map(d => d.value),
    ...latencyHigh.map(d => d.value)
  ];
  const minValue = Math.min(...allValues) * 0.9;
  const maxValue = Math.max(...allValues) * 1.1;
  
  let chart = '```mermaid\nxychart-beta\n    title "Average Latency Trends"\n';
  chart += `    x-axis [${latencyLow.map(d => d.date).join(', ')}]\n`;
  chart += `    y-axis "${minValue}" --> "${maxValue}"\n`;
  chart += '    line [';
  
  if (latencyLow.length > 0) {
    chart += latencyLow.map(d => d.value).join(', ');
  } else {
    chart += '0';
  }
  chart += ']\n';
  
  if (latencyMedium.length > 0) {
    chart += '    line [' + latencyMedium.map(d => d.value).join(', ') + ']\n';
  }
  
  if (latencyHigh.length > 0) {
    chart += '    line [' + latencyHigh.map(d => d.value).join(', ') + ']\n';
  }
  
  chart += '```\n';
  
  return chart;
}

/**
 * Generate Mermaid xychart for P99 latency trends
 * @param {Array} historicalData - Historical performance data
 * @returns {string} Mermaid xychart code
 */
function generateP99LatencyChart(historicalData) {
  const p99Low = extractScenarioData(historicalData, 'low_concurrent', 'latency_p99');
  const p99Medium = extractScenarioData(historicalData, 'medium_concurrent', 'latency_p99');
  const p99High = extractScenarioData(historicalData, 'high_concurrent', 'latency_p99');
  
  // Find min and max values for y-axis
  const allValues = [
    ...p99Low.map(d => d.value),
    ...p99Medium.map(d => d.value),
    ...p99High.map(d => d.value)
  ];
  const minValue = Math.min(...allValues) * 0.9;
  const maxValue = Math.max(...allValues) * 1.1;
  
  let chart = '```mermaid\nxychart-beta\n    title "P99 Latency Trends"\n';
  chart += `    x-axis [${p99Low.map(d => d.date).join(', ')}]\n`;
  chart += `    y-axis "${minValue}" --> "${maxValue}"\n`;
  chart += '    line [';
  
  if (p99Low.length > 0) {
    chart += p99Low.map(d => d.value).join(', ');
  } else {
    chart += '0';
  }
  chart += ']\n';
  
  if (p99Medium.length > 0) {
    chart += '    line [' + p99Medium.map(d => d.value).join(', ') + ']\n';
  }
  
  if (p99High.length > 0) {
    chart += '    line [' + p99High.map(d => d.value).join(', ') + ']\n';
  }
  
  chart += '```\n';
  
  return chart;
}

/**
 * Generate Mermaid xychart for test scenarios trend
 * @param {Array} historicalData - Historical performance data
 * @returns {string} Mermaid xychart code
 */
function generateScenariosChart(historicalData) {
  const dates = historicalData.map(d => formatDate(d.timestamp || d.date || d.updated_at));
  const scenarioCounts = historicalData.map(d => (d.test_scenarios || []).length);
  
  const minValue = Math.min(...scenarioCounts) * 0.9;
  const maxValue = Math.max(...scenarioCounts) * 1.1;
  
  let chart = '```mermaid\nxychart-beta\n    title "Test Scenarios Trends"\n';
  chart += `    x-axis [${dates.join(', ')}]\n`;
  chart += `    y-axis "${minValue}" --> "${maxValue}"\n`;
  chart += '    line [' + scenarioCounts.join(', ') + ']\n';
  chart += '```\n';
  
  return chart;
}

/**
 * Generate complete trend chart report
 * @param {Array} historicalData - Historical performance data
 * @returns {string} Complete Markdown report with Mermaid charts
 */
function generateTrendReport(historicalData) {
  let report = '# UVHTTP Performance Trends\n\n';
  report += '##  Performance Trend Analysis\n\n';
  report += 'This report shows the performance trends over time using Mermaid charts.\n\n';
  
  // Add charts
  report += '### Throughput Trends\n\n';
  report += generateThroughputChart(historicalData);
  report += '\n';
  
  report += '### Average Latency Trends\n\n';
  report += generateLatencyChart(historicalData);
  report += '\n';
  
  report += '### P99 Latency Trends\n\n';
  report += generateP99LatencyChart(historicalData);
  report += '\n';
  
  report += '### Test Scenarios Trends\n\n';
  report += generateScenariosChart(historicalData);
  report += '\n';
  
  // Add legend
  report += '## Legend\n\n';
  report += '- **Low Concurrent**: 10 concurrent connections\n';
  report += '- **Medium Concurrent**: 50 concurrent connections\n';
  report += '- **High Concurrent**: 100 concurrent connections\n';
  
  return report;
}

/**
 * Main function
 */
async function main() {
  const args = process.argv.slice(2);
  
  // Parse arguments
  const options = {
    dataFiles: [],
    outputFile: null
  };
  
  for (let i = 0; i < args.length; i++) {
    if (args[i] === '--output' || args[i] === '-o') {
      options.outputFile = args[++i];
    } else {
      options.dataFiles.push(args[i]);
    }
  }
  
  // Validate arguments
  if (options.dataFiles.length === 0) {
    console.error('Error: No data files provided');
    console.error('');
    console.error('Usage: node generate_trend_chart.js <data_files> --output <output_file>');
    console.error('');
    console.error('Options:');
    console.error('  --output, -o <path>    Output file path');
    console.error('');
    console.error('Example:');
    console.error('  node generate_trend_chart.js data1.json data2.json data3.json --output trends.md');
    process.exit(1);
  }
  
  if (!options.outputFile) {
    console.error('Error: --output is required');
    process.exit(1);
  }
  
  // Load historical data
  const historicalData = [];
  for (const dataFile of options.dataFiles) {
    const data = await loadJsonFile(dataFile);
    if (data) {
      historicalData.push(data);
    }
  }
  
  if (historicalData.length === 0) {
    console.error('Error: No valid data files found');
    process.exit(1);
  }
  
  // Sort by date
  historicalData.sort((a, b) => {
    const dateA = new Date(a.timestamp || a.date || a.updated_at);
    const dateB = new Date(b.timestamp || b.date || b.updated_at);
    return dateA - dateB;
  });
  
  console.log(`Loaded ${historicalData.length} data files`);
  
  // Generate trend report
  const report = generateTrendReport(historicalData);
  
  // Write to file
  await fs.writeFile(options.outputFile, report, 'utf-8');
  console.log(` Performance trend chart saved to ${options.outputFile}`);
}

main().catch(error => {
  console.error('Error:', error.message);
  process.exit(1);
});
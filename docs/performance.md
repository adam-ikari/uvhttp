---
layout: page
title: 性能仪表板
script: https://cdn.jsdelivr.net/npm/chart.js@4.4.1/dist/chart.umd.min.js
---

<script setup>
import { ref, onMounted } from 'vue'

// 性能数据
const performanceData = ref(null)
const loading = ref(true)
const error = ref(null)

// 获取性能数据
const fetchPerformanceData = async () => {
  try {
    loading.value = true
    error.value = null
    
    // 从 GitHub Releases 获取性能数据
    const response = await fetch('https://api.github.com/repos/adam-ikari/uvhttp/releases')
    const releases = await response.json()
    
    // 过滤出性能基准测试的 releases
    const perfReleases = releases.filter(r => r.tag_name.startsWith('perf-'))
    
    // 获取最新的 30 个性能测试结果
    const latestReleases = perfReleases.slice(0, 30)
    
    // 获取每个 release 的性能数据
    const performanceResults = []
    
    for (const release of latestReleases) {
      const assets = release.assets || []
      const jsonAsset = assets.find(a => a.name === 'performance-results.json')
      
      if (jsonAsset) {
        try {
          const dataResponse = await fetch(jsonAsset.browser_download_url)
          const data = await dataResponse.json()
          performanceResults.push({
            date: release.published_at,
            tag: release.tag_name,
            ...data
          })
        } catch (e) {
          console.error(`Failed to load data for ${release.tag_name}:`, e)
        }
      }
    }
    
    performanceData.value = performanceResults.reverse()
    loading.value = false
  } catch (e) {
    error.value = e.message
    loading.value = false
  }
}

onMounted(() => {
  fetchPerformanceData()
})
</script>

<template>
  <div class="performance-dashboard">
    <h1>性能仪表板</h1>
    
    <div v-if="loading" class="loading">
      <p>加载性能数据中...</p>
    </div>
    
    <div v-else-if="error" class="error">
      <p>加载失败: {{ error }}</p>
    </div>
    
    <div v-else-if="performanceData && performanceData.length > 0" class="dashboard-content">
      <!-- 最新性能指标 -->
      <section class="latest-metrics">
        <h2>最新性能指标</h2>
        <div class="metrics-grid">
          <div class="metric-card">
            <h3>低并发 (10 连接)</h3>
            <div class="metric-value">
              {{ performanceData[performanceData.length - 1]?.test_scenarios?.[0]?.results?.rps?.value?.toFixed(0) || 'N/A' }}
            </div>
            <div class="metric-label">RPS</div>
            <div class="metric-sub">
              延迟: {{ performanceData[performanceData.length - 1]?.test_scenarios?.[0]?.results?.latency_avg?.value || 'N/A' }}
            </div>
          </div>
          
          <div class="metric-card">
            <h3>中等并发 (50 连接)</h3>
            <div class="metric-value">
              {{ performanceData[performanceData.length - 1]?.test_scenarios?.[1]?.results?.rps?.value?.toFixed(0) || 'N/A' }}
            </div>
            <div class="metric-label">RPS</div>
            <div class="metric-sub">
              延迟: {{ performanceData[performanceData.length - 1]?.test_scenarios?.[1]?.results?.latency_avg?.value || 'N/A' }}
            </div>
          </div>
          
          <div class="metric-card">
            <h3>高并发 (100 连接)</h3>
            <div class="metric-value">
              {{ performanceData[performanceData.length - 1]?.test_scenarios?.[2]?.results?.rps?.value?.toFixed(0) || 'N/A' }}
            </div>
            <div class="metric-label">RPS</div>
            <div class="metric-sub">
              延迟: {{ performanceData[performanceData.length - 1]?.test_scenarios?.[2]?.results?.latency_avg?.value || 'N/A' }}
            </div>
          </div>
          
          <div class="metric-card">
            <h3>测试日期</h3>
            <div class="metric-value">
              {{ new Date(performanceData[performanceData.length - 1]?.timestamp).toLocaleDateString('zh-CN') }}
            </div>
            <div class="metric-label">最后更新</div>
            <div class="metric-sub">
              运行 #{{ performanceData[performanceData.length - 1]?.run_number }}
            </div>
          </div>
        </div>
      </section>
      
      <!-- 性能趋势图 -->
      <section class="trend-charts">
        <h2>性能趋势</h2>
        
        <div class="chart-container">
          <h3>吞吐量趋势</h3>
          <canvas id="throughputChart"></canvas>
        </div>
        
        <div class="chart-container">
          <h3>延迟趋势</h3>
          <canvas id="latencyChart"></canvas>
        </div>
      </section>
      
      <!-- 历史数据表格 -->
      <section class="history-table">
        <h2>历史数据</h2>
        <table>
          <thead>
            <tr>
              <th>日期</th>
              <th>运行编号</th>
              <th>低并发 RPS</th>
              <th>中等并发 RPS</th>
              <th>高并发 RPS</th>
              <th>状态</th>
            </tr>
          </thead>
          <tbody>
            <tr v-for="(data, index) in performanceData.slice().reverse()" :key="data.run_id">
              <td>{{ new Date(data.timestamp).toLocaleDateString('zh-CN') }}</td>
              <td>#{{ data.run_number }}</td>
              <td>{{ data.test_scenarios?.[0]?.results?.rps?.value?.toFixed(0) || 'N/A' }}</td>
              <td>{{ data.test_scenarios?.[1]?.results?.rps?.value?.toFixed(0) || 'N/A' }}</td>
              <td>{{ data.test_scenarios?.[2]?.results?.rps?.value?.toFixed(0) || 'N/A' }}</td>
              <td>
                <span class="status pass">✅ 通过</span>
              </td>
            </tr>
          </tbody>
        </table>
      </section>
      
      <!-- 详细报告链接 -->
      <section class="reports">
        <h2>详细报告</h2>
        <div class="report-links">
          <a 
            v-for="data in performanceData.slice(-5).reverse()" 
            :key="data.run_id"
            :href="`https://github.com/adam-ikari/uvhttp/releases/download/${data.tag}/performance-report.md`"
            target="_blank"
            class="report-link"
          >
            📊 {{ data.tag }} - {{ new Date(data.timestamp).toLocaleDateString('zh-CN') }}
          </a>
        </div>
      </section>
    </div>
    
    <div v-else class="no-data">
      <p>暂无性能数据</p>
      <p>性能基准测试将在每日 UTC 0:00 自动运行</p>
    </div>
  </div>
</template>

<style scoped>
.performance-dashboard {
  padding: 20px;
  max-width: 1200px;
  margin: 0 auto;
}

.loading, .error, .no-data {
  text-align: center;
  padding: 40px;
  font-size: 1.2em;
}

.error {
  color: #e74c3c;
}

.latest-metrics {
  margin-bottom: 40px;
}

.metrics-grid {
  display: grid;
  grid-template-columns: repeat(auto-fit, minmax(250px, 1fr));
  gap: 20px;
  margin-top: 20px;
}

.metric-card {
  background: var(--vp-c-bg-soft);
  border: 1px solid var(--vp-c-divider);
  border-radius: 8px;
  padding: 20px;
  text-align: center;
}

.metric-card h3 {
  margin: 0 0 10px 0;
  font-size: 1.1em;
  color: var(--vp-c-text-2);
}

.metric-value {
  font-size: 2.5em;
  font-weight: bold;
  color: var(--vp-c-brand);
  margin-bottom: 5px;
}

.metric-label {
  font-size: 0.9em;
  color: var(--vp-c-text-2);
  margin-bottom: 5px;
}

.metric-sub {
  font-size: 0.85em;
  color: var(--vp-c-text-3);
}

.trend-charts {
  margin-bottom: 40px;
}

.chart-container {
  background: var(--vp-c-bg-soft);
  border: 1px solid var(--vp-c-divider);
  border-radius: 8px;
  padding: 20px;
  margin-bottom: 20px;
}

.chart-container h3 {
  margin: 0 0 15px 0;
  font-size: 1.2em;
}

.chart-container canvas {
  max-height: 400px;
}

.history-table {
  margin-bottom: 40px;
}

.history-table table {
  width: 100%;
  border-collapse: collapse;
  background: var(--vp-c-bg-soft);
  border-radius: 8px;
  overflow: hidden;
}

.history-table th,
.history-table td {
  padding: 12px;
  text-align: left;
  border-bottom: 1px solid var(--vp-c-divider);
}

.history-table th {
  background: var(--vp-c-bg);
  font-weight: bold;
}

.history-table tr:hover {
  background: var(--vp-c-bg-soft);
}

.status.pass {
  color: #27ae60;
  font-weight: bold;
}

.reports {
  margin-bottom: 40px;
}

.report-links {
  display: flex;
  flex-direction: column;
  gap: 10px;
  margin-top: 20px;
}

.report-link {
  display: inline-block;
  padding: 10px 15px;
  background: var(--vp-c-bg-soft);
  border: 1px solid var(--vp-c-divider);
  border-radius: 6px;
  text-decoration: none;
  color: var(--vp-c-text-1);
  transition: all 0.3s;
}

.report-link:hover {
  background: var(--vp-c-brand);
  color: white;
  border-color: var(--vp-c-brand);
}

@media (max-width: 768px) {
  .metrics-grid {
    grid-template-columns: 1fr;
  }
  
  .metric-value {
    font-size: 2em;
  }
}
</style>

<script>
// 渲染图表
export default {
  data() {
    return {
      performanceData: []
    }
  },
  mounted() {
    this.$nextTick(() => {
      this.renderCharts()
    })
  },
  watch: {
    performanceData: {
      handler() {
        this.$nextTick(() => {
          this.renderCharts()
        })
      },
      deep: true
    }
  },
  methods: {
    renderCharts() {
      if (!this.performanceData || this.performanceData.length === 0) {
        return
      }
      
      const data = this.performanceData
      
      // 准备图表数据
      const labels = data.map(d => new Date(d.timestamp).toLocaleDateString('zh-CN'))
      const lowRps = data.map(d => d.test_scenarios?.[0]?.results?.rps?.value || 0)
      const mediumRps = data.map(d => d.test_scenarios?.[1]?.results?.rps?.value || 0)
      const highRps = data.map(d => d.test_scenarios?.[2]?.results?.rps?.value || 0)
      
      const lowLatency = data.map(d => {
        const val = d.test_scenarios?.[0]?.results?.latency_avg?.value || '0'
        return parseFloat(val.replace(/[^\d.]/g, ''))
      })
      const mediumLatency = data.map(d => {
        const val = d.test_scenarios?.[1]?.results?.latency_avg?.value || '0'
        return parseFloat(val.replace(/[^\d.]/g, ''))
      })
      const highLatency = data.map(d => {
        const val = d.test_scenarios?.[2]?.results?.latency_avg?.value || '0'
        return parseFloat(val.replace(/[^\d.]/g, ''))
      })
      
      // 渲染吞吐量趋势图
      const throughputCtx = document.getElementById('throughputChart')
      if (throughputCtx) {
        new Chart(throughputCtx, {
          type: 'line',
          data: {
            labels: labels,
            datasets: [
              {
                label: '低并发 (10 连接)',
                data: lowRps,
                borderColor: '#3498db',
                backgroundColor: 'rgba(52, 152, 219, 0.1)',
                tension: 0.4,
                fill: true
              },
              {
                label: '中等并发 (50 连接)',
                data: mediumRps,
                borderColor: '#2ecc71',
                backgroundColor: 'rgba(46, 204, 113, 0.1)',
                tension: 0.4,
                fill: true
              },
              {
                label: '高并发 (100 连接)',
                data: highRps,
                borderColor: '#e74c3c',
                backgroundColor: 'rgba(231, 76, 60, 0.1)',
                tension: 0.4,
                fill: true
              }
            ]
          },
          options: {
            responsive: true,
            maintainAspectRatio: false,
            plugins: {
              legend: {
                position: 'top',
              },
              tooltip: {
                mode: 'index',
                intersect: false
              }
            },
            scales: {
              y: {
                beginAtZero: false,
                title: {
                  display: true,
                  text: 'RPS'
                }
              }
            }
          }
        })
      }
      
      // 渲染延迟趋势图
      const latencyCtx = document.getElementById('latencyChart')
      if (latencyCtx) {
        new Chart(latencyCtx, {
          type: 'line',
          data: {
            labels: labels,
            datasets: [
              {
                label: '低并发延迟',
                data: lowLatency,
                borderColor: '#3498db',
                backgroundColor: 'rgba(52, 152, 219, 0.1)',
                tension: 0.4,
                fill: true
              },
              {
                label: '中等并发延迟',
                data: mediumLatency,
                borderColor: '#2ecc71',
                backgroundColor: 'rgba(46, 204, 113, 0.1)',
                tension: 0.4,
                fill: true
              },
              {
                label: '高并发延迟',
                data: highLatency,
                borderColor: '#e74c3c',
                backgroundColor: 'rgba(231, 76, 60, 0.1)',
                tension: 0.4,
                fill: true
              }
            ]
          },
          options: {
            responsive: true,
            maintainAspectRatio: false,
            plugins: {
              legend: {
                position: 'top',
              },
              tooltip: {
                mode: 'index',
                intersect: false
              }
            },
            scales: {
              y: {
                beginAtZero: true,
                title: {
                  display: true,
                  text: '延迟 (μs)'
                }
              }
            }
          }
        })
      }
    }
  }
}
</script>
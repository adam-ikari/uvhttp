<script setup lang="ts">
import { ref, onMounted } from 'vue'

interface Version {
  version: string
  status: string
  releaseDate: string
  url: string
  githubUrl: string
}

interface VersionConfig {
  current: string
  baseUrl: string
  versions: Version[]
  statusLabels: {
    current: string
    security: string
    limited: string
    deprecated: string
  }
}

// 默认版本配置
const DEFAULT_VERSION: Version = {
  version: '2.0.0',
  status: 'current',
  releaseDate: '2026-01-21',
  url: '/',
  githubUrl: 'https://github.com/adam-ikari/uvhttp/releases/tag/v2.0.0'
}

const versionsConfig = ref<VersionConfig | null>(null)
const currentVersion = ref<Version | null>(null)
const hasHistoryVersions = ref(false)

onMounted(async () => {
  try {
    const response = await fetch('/.vitepress/versions.json')
    if (response.ok) {
      const config = await response.json()
      versionsConfig.value = config
      currentVersion.value = config.versions.find((v: Version) => v.status === 'current') || config.versions[0]
      hasHistoryVersions.value = config.versions.length > 1
    }
  } catch (error) {
    console.warn('Failed to load versions config:', error)
    // 使用默认值
    currentVersion.value = DEFAULT_VERSION
  }
})

function handleVersionChange(event: Event) {
  const target = event.target as HTMLSelectElement
  const version = versionsConfig.value?.versions.find(v => v.version === target.value)
  if (version) {
    if (version.url.startsWith('http')) {
      window.open(version.url, '_blank')
    } else {
      window.location.href = version.url
    }
  }
}

function getVersionLabel(version: Version): string {
  if (!versionsConfig.value || !version.status) return ''
  const label = versionsConfig.value.statusLabels[version.status as keyof typeof versionsConfig.value.statusLabels]
  return label ? ` (${label})` : ''
}

function getVersionClass(status: string): string {
  return `version-status-${status}`
}
</script>

<template>
  <!-- 只有一个版本时显示徽章 -->
  <div v-if="!hasHistoryVersions && currentVersion" class="version-badge">
    <span class="version-text">v{{ currentVersion.version }}</span>
  </div>

  <!-- 有多个版本时显示下拉菜单 -->
  <div v-else-if="versionsConfig" class="version-select">
    <select
      :value="currentVersion?.version"
      @change="handleVersionChange"
      class="version-dropdown"
      aria-label="选择文档版本"
    >
      <option
        v-for="version in versionsConfig.versions"
        :key="version.version"
        :value="version.version"
        :class="getVersionClass(version.status)"
      >
        {{ version.version }}{{ getVersionLabel(version) }}
      </option>
    </select>
  </div>
</template>

<style scoped>
.version-badge {
  display: inline-flex;
  align-items: center;
  padding: 4px 10px;
  background-color: var(--vp-c-brand-soft);
  border-radius: 4px;
  font-size: 13px;
  font-weight: 500;
  color: var(--vp-c-brand);
  margin-left: 10px;
}

.version-text {
  font-family: var(--vp-font-family-mono);
}

.version-select {
  display: inline-block;
  margin-left: 10px;
}

.version-dropdown {
  padding: 6px 12px;
  font-size: 14px;
  border: 1px solid var(--vp-c-border);
  border-radius: 6px;
  background-color: var(--vp-c-bg);
  color: var(--vp-c-text-1);
  cursor: pointer;
  transition: border-color 0.2s, box-shadow 0.2s;
}

.version-dropdown:hover {
  border-color: var(--vp-c-brand);
  box-shadow: 0 0 0 3px rgba(84, 110, 122, 0.1);
}

.version-dropdown:focus {
  outline: none;
  border-color: var(--vp-c-brand);
  box-shadow: 0 0 0 3px rgba(84, 110, 122, 0.2);
}

.version-dropdown:active {
  transform: scale(0.98);
}

/* 版本状态样式 */
.version-status-current {
  font-weight: 600;
  color: var(--vp-c-brand);
}

.version-status-security {
  color: var(--vp-c-success);
}

.version-status-limited {
  color: var(--vp-c-text-2);
}

.version-status-deprecated {
  color: var(--vp-c-text-3);
  text-decoration: line-through;
}
</style>
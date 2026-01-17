<template>
  <div class="version-selector">
    <select v-model="selectedVersion" @change="onVersionChange" class="version-select" aria-label="Select documentation version" id="version-selector">
      <option v-for="version in versions" :key="version.label" :value="version.label">
        {{ version.label }} {{ version.isCurrent ? '(当前)' : '' }}
      </option>
    </select>
  </div>
</template>

<script setup lang="ts">
import { ref, onMounted } from 'vue'

interface Version {
  label: string
  link: string
  isCurrent: boolean
}

const versions = ref<Version[]>([
  { label: '1.5.0', link: '/v1.5.0/', isCurrent: true },
  { label: '1.4.0', link: '/v1.4.0/', isCurrent: false },
  { label: '1.3.0', link: '/v1.3.0/', isCurrent: false },
  { label: '1.2.0', link: '/v1.2.0/', isCurrent: false },
  { label: '1.1.0', link: '/v1.1.0/', isCurrent: false },
  { label: '1.0.0', link: '/v1.0.0/', isCurrent: false }
])

const selectedVersion = ref('1.5.0')

onMounted(() => {
  try {
    const path = window.location.pathname
    const versionMatch = path.match(/\/v(\d+\.\d+\.\d+)\//)
    if (versionMatch) {
      const version = versions.value.find(v => v.label === versionMatch[1])
      if (version) {
        selectedVersion.value = versionMatch[1]
      }
    }
  } catch (error) {
    console.error('Failed to detect version from URL:', error)
  }
})

const onVersionChange = () => {
  try {
    const version = versions.value.find(v => v.label === selectedVersion.value)
    if (version) {
      window.location.href = version.link
    }
  } catch (error) {
    console.error('Failed to change version:', error)
  }
}
</script>

<style scoped>
.version-selector {
  display: inline-block;
  margin-left: 10px;
}

.version-select {
  padding: 5px 10px;
  border: 1px solid #ccc;
  border-radius: 4px;
  background: #fff;
  color: #333;
  font-size: 14px;
  cursor: pointer;
}

.version-select:hover {
  border-color: #999;
}

.version-select:focus {
  outline: 2px solid #3b82f6;
  border-color: #3b82f6;
}
</style>
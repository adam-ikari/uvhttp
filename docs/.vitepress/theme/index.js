import DefaultTheme from 'vitepress/theme-without-fonts'
import { h } from 'vue'
import syncStatus from '../sync-status.json'
import './style.css'

export default {
  extends: DefaultTheme,
  Layout() {
    return h(DefaultTheme.Layout, null, {
      'content-top': () => null
    })
  },
  enhanceApp() {
    if (typeof window !== 'undefined') {
      if (document.readyState === 'loading') {
        document.addEventListener('DOMContentLoaded', checkOutdated)
      } else {
        checkOutdated()
      }
    }
  }
}

function checkOutdated() {
  var path = window.location.pathname
  if (!path.startsWith('/uvhttp/zh/')) return

  var enPath = path.replace('/uvhttp/zh/', '')
  var entry = syncStatus[enPath.replace(/\.html$/, '.md')] || syncStatus[enPath + '.md']
  if (!entry || !entry.outdated) return

  var existing = document.getElementById('outdated-banner')
  if (existing) existing.remove()

  var content = document.querySelector('.content-container')
  if (!content) return

  var banner = document.createElement('div')
  banner.id = 'outdated-banner'
  banner.style.cssText = 'background-color:#fff3cd;border:1px solid #ffc107;border-radius:6px;padding:12px 16px;margin-bottom:16px;color:#856404;font-size:14px;line-height:1.5'
  banner.innerHTML = '<strong>⚠️ 翻译可能已过时</strong> 此页面的英文版本已更新，中文翻译可能未及时同步。<br><a href="/uvhttp/' + enPath.replace(/\.md$/, '.html') + '" style="color:#856404;text-decoration:underline">查看英文原文</a>'

  content.prepend(banner)
}
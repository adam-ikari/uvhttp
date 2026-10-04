#!/usr/bin/env bash
# 发布检查清单强制器：把 docs/release-strategy.md 里靠人读的清单变成
# 脚本卡住的门禁。
#
# 用法：
#   scripts/ci/release_checklist.sh --pre      # 发布前：本地可验证项
#   scripts/ci/release_checklist.sh --post     # 发布后：GitHub / 站点可验证项
#
# 两阶段分开是因为部分项只能在发布动作完成后查（tag、release、线上
# versions.json）。pre 在合并版本 PR 前跑；post 在转正式 latest 后跑。
#
# 退出码：0=全过；1=有未过项（列出原因）；2=用法错。
set -euo pipefail

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
cd "$REPO_ROOT"

mode="${1:-}"
if [ "$mode" != "--pre" ] && [ "$mode" != "--post" ]; then
    echo "用法: $0 --pre | $0 --post" >&2
    exit 2
fi

ok=0
fail=0
pass() { printf '  \033[32m✓\033[0m %s\n' "$1"; ok=$((ok+1)); }
bad() { printf '  \033[31m✗\033[0m %s\n' "$1"; fail=$((fail+1)); }

# 从 VERSION 文件解析字段（容忍 # 注释）
vfield() {
    awk -F= -v k="$1" '$1==k && $2!="" {gsub(/"/,"",$2); print $2; exit}' VERSION
}

version="$(vfield VERSION)"
release_date="$(vfield RELEASE_DATE)"
previous="$(vfield PREVIOUS_VERSION)"

if [ -z "$version" ] || [ -z "$release_date" ] || [ -z "$previous" ]; then
    echo "VERSION 文件缺字段（VERSION/RELEASE_DATE/PREVIOUS_VERSION）" >&2
    exit 1
fi

echo "== 发布检查清单 ($mode) — v$version =="

# ---------------------------------------------------------------------------
# 共有项：VERSION / CHANGELOG 一致
# ---------------------------------------------------------------------------
echo "[VERSION/CHANGELOG]"
if grep -q "^## \[$version\]" docs/guide/CHANGELOG.md 2>/dev/null; then
    pass "EN CHANGELOG 含 [$version] 条目"
else
    bad "EN CHANGELOG 缺 [$version] 条目"
fi
if grep -q "^## \[$version\]" docs/zh/guide/CHANGELOG.md 2>/dev/null; then
    pass "ZH CHANGELOG 含 [$version] 条目"
else
    bad "ZH CHANGELOG 缺 [$version] 条目"
fi
if grep -q "^| v$version |" docs/release-strategy.md 2>/dev/null; then
    pass "release-strategy 版本历史含 v$version"
else
    bad "release-strategy 版本历史缺 v$version"
fi

# ---------------------------------------------------------------------------
if [ "$mode" = "--pre" ]; then
    echo "[构建/测试]"
    if [ -d build ]; then
        if (cd build && ctest --output-on-failure >/dev/null 2>&1); then
            pass "ctest 全部通过"
        else
            bad "ctest 有失败项（cd build && ctest --output-on-failure）"
        fi
    else
        bad "build/ 不存在（先 make build）"
    fi

    echo "[文档构建]"
    if [ -d docs/node_modules ]; then
        if (cd docs && npm run docs:build >/dev/null 2>&1); then
            pass "npm run docs:build 通过"
        else
            bad "npm run docs:build 失败（PR #446 同类问题）"
        fi
    else
        bad "docs/node_modules 不存在（先 cd docs && npm install）"
    fi

    echo "[静态检查]"
    # 只检查本次改动相对 main 的文件——全量检查由 CI 的 code-quality-check
    # job 负责，这里重复会混淆（且 main 上仍残留 #416 修复前空转期进入的
    # 既有违规，全量查会报红但与本次发布无关）。
    changed=$(git diff --name-only main...HEAD -- 'src/*.c' 'include/*.h' 'test/unit/*.cpp' 2>/dev/null | grep -E '\.(c|h|cpp)$' || true)
    if [ -z "$changed" ]; then
        pass "本次发布无 C/C++ 改动"
    elif clang-format --dry-run --Werror $changed >/dev/null 2>&1; then
        pass "本次改动文件 clang-format 合规（$changed）"
    else
        bad "本次改动文件 clang-format 有违规（全量门禁在 CI code-quality-check）"
    fi

    echo "[一致性]"
    # test_version_consistency 已把库自报版本钉到 VERSION，跑一下
    if [ -x build/dist/bin/test_version_consistency ]; then
        if build/dist/bin/test_version_consistency >/dev/null 2>&1; then
            pass "库自报版本与 VERSION 文件一致"
        else
            bad "库自报版本与 VERSION 不一致（可能 CMake 未注入宏）"
        fi
    else
        bad "test_version_consistency 未构建"
    fi

# ---------------------------------------------------------------------------
else  # --post：需要 gh CLI 与网络
    echo "[Git tag]"
    tag="v$version"
    # gh release view 的 --json 字段不统一（isLatest 在 view 不可用、在 list 可用），
    # 故统一走 release list。
    if gh release list --json tagName,isLatest,isPrerelease 2>/dev/null | \
       python3 -c "import json,sys; exit(0 if any(r['tagName']=='$tag' for r in json.load(sys.stdin)) else 1)"; then
        pass "远端存在 tag $tag"
    else
        bad "远端无 tag $tag（git push origin $tag）"
    fi

    echo "[GitHub Release]"
    release_json=$(gh release list --json tagName,isLatest,isPrerelease 2>/dev/null)
    tag_info=$(printf '%s' "$release_json" | python3 -c "import json,sys
rs=[r for r in json.load(sys.stdin) if r['tagName']=='$tag']
if not rs: print('none')
else: print(('latest' if rs[0].get('isLatest') else ('prerelease' if rs[0].get('isPrerelease') else 'released')))" 2>/dev/null)
    case "$tag_info" in
        latest)     pass "Release $tag 已转正式（latest）" ;;
        prerelease) bad "Release $tag 仍是 prerelease（gh release edit $tag --latest --prerelease=false）" ;;
        released)   bad "Release $tag 非 prerelease 也非 latest（状态异常）" ;;
        none)       bad "Release $tag 不存在（gh release create $tag）" ;;
    esac

    echo "[文档站点]"
    base_url="$(vfield MINIMUM_COMPATIBLE_VERSION >/dev/null 2>&1; echo "https://adam-ikari.github.io/uvhttp")"
    versions_json="$base_url/versions.json"
    if online=$(curl -sf --max-time 15 "$versions_json" 2>/dev/null); then
        online_current=$(printf '%s' "$online" | python3 -c 'import json,sys; print(json.load(sys.stdin).get("current",""))' 2>/dev/null || echo "")
        if [ "$online_current" = "$version" ]; then
            pass "线上 versions.json current=$version"
        else
            bad "线上 versions.json current=$online_current（期望 $version）—— 部署滞后或 #451 同类"
        fi
    else
        bad "取不到 $versions_json"
    fi
fi

echo
echo "== 结果：$ok 通过，$fail 未过 =="
if [ "$fail" -eq 0 ]; then exit 0; else exit 1; fi
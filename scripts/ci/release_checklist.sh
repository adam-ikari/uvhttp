#!/usr/bin/env bash
# 发布检查清单强制器：把 docs/release-strategy.md 里靠人读的清单变成
# 脚本卡住的门禁。
#
# 用法：
#   scripts/ci/release_checklist.sh --pre      # 发布前：本地可验证项
#   scripts/ci/release_checklist.sh --gates    # 转正式前：release 门禁确实跑过且全绿
#   scripts/ci/release_checklist.sh --post     # 发布后：GitHub / 站点可验证项
#
# 三段分开是因为部分项只能在发布动作的特定时点查（tag、release、线上
# versions.json）。pre 在合并版本 PR 前跑；gates 在 gh release create --prerelease
# 之后、--latest 之前跑；post 在转正式 latest 后跑。
#
# gates 这一段的唯一理由是「门禁没触发」不会以任何方式报错：release 事件的
# activity type 写错时 GitHub 只是不跑，看起来一切正常。v3.0.0 就撞上这个 ——
# 两条 release 门禁从 2026-10-16 起全哑：先写成无效的 [prerelease]，改成
# [prereleased] 后仍不跑（它对从 draft 发布的预发布不触发，而
# gh release create --prerelease 正是这样发布），正确类型是 [published]。
# pre-release 建好、页面正常、测试却一行没跑。流程写「等门禁全绿」而没人
# 去查它们跑没跑，那句话就是空的。
#
# 退出码：0=全过；1=有未过项（列出原因）；2=用法错。
set -euo pipefail

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
cd "$REPO_ROOT"

mode="${1:-}"
if [ "$mode" != "--pre" ] && [ "$mode" != "--gates" ] && [ "$mode" != "--post" ]; then
    echo "用法: $0 --pre | $0 --gates | $0 --post" >&2
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
elif [ "$mode" = "--post" ]; then  # 需要 gh CLI 与网络
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

# ---------------------------------------------------------------------------
else  # --gates：转正式前，确认 release 门禁真的因为这个 tag 跑过
    echo "[release 门禁运行记录]"
    tag="v$version"
    # 从远端解析 tag 提交，不用本地 ref：移动 tag 是文档化的恢复路径，本地
    # 未重新 fetch 时会拿旧 sha 匹配运行记录，把旧 tag 的绿当成新 tag 的绿。
    # 注解 tag 的 peeled 行（refs/tags/<tag>^{}）才是提交 sha。
    tag_refs="$(git ls-remote --tags origin "refs/tags/$tag*" 2>/dev/null || true)"
    sha="$(printf '%s\n' "$tag_refs" | awk -v r="refs/tags/$tag^{}" '$2==r {print $1; exit}')"
    if [ -z "$sha" ]; then
        sha="$(printf '%s\n' "$tag_refs" | awk -v r="refs/tags/$tag" '$2==r {print $1; exit}')"
    fi
    if [ -z "$sha" ]; then
        bad "解析不到远端 $tag 指向的提交（tag 未打、未 push 或网络不通）"
    else
        # release 事件的运行记录里 head_sha 就是 tag 指向的提交（v2.9.2/v2.9.1
        # 的运行都如此），head_branch 是 tag 名。两者都要匹配：只看 sha 时，
        # tag 被移回一个曾经全绿的提交会借旧运行假通过。
        # 只认 event=release：手动 workflow_dispatch 的基准不判红（见
        # perf-regression-gate），拿来当门禁通过等于自欺。
        # 多次运行（rerun/重触发都是新增、不替换）以 createdAt 最新的一条为准；
        # cancelled 不算证据 —— 否则任何一次取消都会让判定永久卡死。
        for wf in "ci-release-gate.yml|CI - Pre-Release Test Gate" "ci-benchmark.yml|CI - Performance Benchmark"; do
            wf_file="${wf%%|*}"; wf_name="${wf##*|}"
            state=$(gh run list --workflow "$wf_file" --event release --limit 50 \
                       --json headSha,headBranch,conclusion,createdAt 2>/dev/null | python3 -c "import json,sys
tag='$tag'; sha='$sha'
rs=[r for r in json.load(sys.stdin)
    if r['headSha']==sha and r.get('headBranch')==tag]
live=[r for r in rs if r['conclusion']!='cancelled']
if not rs: print('none')
elif not live: print('cancelled')
else:
    c=max(live, key=lambda r: r['createdAt'])['conclusion']
    if c=='success': print('success')
    elif c in ('', 'None', 'action_required', 'queued', 'in_progress', 'pending'): print('running')
    else: print('not-green')" 2>/dev/null || echo "query-failed")
            case "$state" in
                success)     pass "$wf_name：$tag（${sha:0:7}）最新的 release 运行成功" ;;
                running)     bad "$wf_name：$tag 最新的 release 运行还没结束，等它绿" ;;
                not-green)   bad "$wf_name：$tag 最新的 release 运行未通过 —— 用 gh run rerun <run-id> 重跑，不要移动 tag" ;;
                cancelled)   bad "$wf_name：$tag 只有被取消的 release 运行 —— 用 gh run rerun <run-id> 重跑，不要移动 tag" ;;
                none)        bad "$wf_name：没有 $tag 的 release 事件运行 —— 门禁根本没被触发，禁止转 latest" ;;
                *)           bad "$wf_name：查询失败（gh 未登录或网络）" ;;
            esac
        done
    fi
fi

echo
echo "== 结果：$ok 通过，$fail 未过 =="
if [ "$fail" -eq 0 ]; then exit 0; else exit 1; fi
#!/usr/bin/env python3
"""
测试有效性门禁：阻止「恒绿零验证」的测试进入仓库。

背景（2026-10-03 普查 3095 个测试）：发现 274 个零断言测试、101 个断言被
条件静默跳过、4 处断言被注释掉。其中 test_server_error_coverage.cpp 的
`check_rate_limit` 断言被注释后，把该函数返回值从 UVHTTP_OK 完全反转成
UVHTTP_ERROR_INVALID_PARAM，15 个测试仍然全绿——恒绿零验证。

本门禁只拦截**明确无效**的三类，不试图判定断言质量：

  [1] 断言被注释掉（// EXPECT_... 或 /* EXPECT_... */）—— 零容忍
  [2] 断言被包在 if 里且该 if 内无 GTEST_SKIP —— 零容忍新增
  [3] 零断言测试（函数体只有调用，验证的是「不崩溃」）—— 零容忍新增

[3] 类既有 274 个，多为 NULL 安全测试，在 ASan/UBSan 门禁下仍能捕获内存
错误，故不要求立即清零，只拦新增。[2] 类既有 101 个需逐项人工核实后治理，
先记基线、只拦新增。基线下调即为治理进度。

用法：
    python3 scripts/ci/check_test_validity.py            # 报告
    python3 scripts/ci/check_test_validity.py --strict   # 新增即退出 1
"""
import argparse
import re
import sys
from pathlib import Path

REPO = Path(__file__).resolve().parents[2]
TEST_DIR = REPO / "test" / "unit"

# 既有存量（2026-10-03 普查）。逐项修复后下调，下调即治理进度。
BASELINE_SILENT_SKIP = 18
BASELINE_ZERO_ASSERT = 270

ASSERT_CALL = re.compile(r'\b(EXPECT|ASSERT)_[A-Z]+\s*\(')
ASSERT_COMMENT = re.compile(r'^\s*(//|/\*)\s*(EXPECT|ASSERT)_[A-Z]+')
TEST_DEF = re.compile(
    r'^\s*(TEST|TEST_F|TEST_P)\s*\(\s*([A-Za-z_]\w*)\s*,\s*([A-Za-z_]\w*)\s*\)'
)


def split_test_functions(src):
    """按大括号配平把源文件切成 (名称, 行号, 函数体) 列表。"""
    out, lines, i = [], src.split("\n"), 0
    while i < len(lines):
        m = TEST_DEF.match(lines[i])
        if not m:
            i += 1
            continue
        name = f"{m.group(2)}.{m.group(3)}"
        start, body, depth, j = i, [], 0, i
        while j < len(lines):
            body.append(lines[j])
            depth += lines[j].count("{") - lines[j].count("}")
            j += 1
            if depth <= 0 and j > start + 1:
                break
        out.append((name, start + 1, "\n".join(body)))
        i = j
    return out


def is_silent_skip(body):
    """断言是否全部被无 GTEST_SKIP 的 if 罩住（条件不成立就永不执行）。"""
    if "GTEST_SKIP" in body:
        return False  # 跳过在输出里可见，合规
    total = len(ASSERT_CALL.findall(body))
    if total == 0:
        return False
    guarded, in_if, depth = 0, False, 0
    for line in body.split("\n"):
        if re.match(r'\s*if\s*\(', line) and not in_if:
            in_if, depth = True, line.count("{") - line.count("}")
            if ASSERT_CALL.search(line):
                guarded += 1
            continue
        if in_if:
            depth += line.count("{") - line.count("}")
            if ASSERT_CALL.search(line):
                guarded += 1
            if depth <= 0:
                in_if = False
    return guarded > 0 and guarded >= total


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--strict", action="store_true",
                    help="任一类超过基线即退出 1")
    args = ap.parse_args()

    files = sorted(TEST_DIR.glob("test_*.cpp"))
    commented, silent, zero, total = [], [], [], 0

    for f in files:
        src = f.read_text(encoding="utf-8", errors="replace")
        for n, line in enumerate(src.split("\n"), 1):
            if ASSERT_COMMENT.match(line):
                commented.append(f"{f.name}:{n}  {line.strip()[:70]}")
        funcs = split_test_functions(src)
        total += len(funcs)
        for name, line, body in funcs:
            entry = f"{f.name}:{line}  {name}"
            if not ASSERT_CALL.search(body):
                zero.append(entry)
            elif is_silent_skip(body):
                silent.append(entry)

    new_silent = max(0, len(silent) - BASELINE_SILENT_SKIP)
    new_zero = max(0, len(zero) - BASELINE_ZERO_ASSERT)

    print(f"扫描 {len(files)} 个文件 / {total} 个测试\n")
    print(f"[1] 断言被注释掉: {len(commented)}  [基线 0，零容忍]")
    for c in commented:
        print(f"    {c}")
    print(f"\n[2] 断言被条件静默跳过: {len(silent)}"
          f"  [基线 {BASELINE_SILENT_SKIP}，新增 {new_silent}]")
    for s in silent[:30]:
        print(f"    {s}")
    if len(silent) > 30:
        print(f"    ... 另 {len(silent)-30} 项")
    print(f"\n[3] 零断言测试: {len(zero)}"
          f"  [基线 {BASELINE_ZERO_ASSERT}，新增 {new_zero}]")

    if args.strict:
        problems = []
        if commented:
            problems.append(f"[1] {len(commented)} 处断言被注释掉")
        if new_silent:
            problems.append(f"[2] 新增 {new_silent} 个静默跳过")
        if new_zero:
            problems.append(f"[3] 新增 {new_zero} 个零断言测试")
        if problems:
            print("\nFAIL: " + "；".join(problems))
            return 1
    print("\nPASS")
    return 0


if __name__ == "__main__":
    sys.exit(main())
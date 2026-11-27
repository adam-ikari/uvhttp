#!/usr/bin/env python3
"""Reject hand-counted response body lengths that disagree with the literal.

A response body length must be the C string's byte count, not its character
count. In particular a UTF-8 literal like "中间件测试" is 15 bytes, not 5.
Passing the wrong number truncates or over-reads the body — the client sees
a truncated JSON document or trailing NUL bytes (JSON.parse fails).

Scope: examples/ only. Those files are the public, copy-pasteable usage
surface; library internals already use the byte buffer's real length, and
tests may deliberately pass mismatched lengths as negative cases.

Accepted forms:
    uvhttp_respond(resp, 200, "text/plain", "hello", 5);
    uvhttp_respond(resp, 200, "text/plain", "hello", strlen("hello"));
    uvhttp_respond_with_headers(resp, 200, hdrs, 2, "中文", strlen("中文"));

Rejected: a bare decimal literal whose value != the C string's byte count.

Exit 1 on any mismatch so CI blocks the PR.
"""

import pathlib
import re
import sys

ROOT = pathlib.Path(__file__).resolve().parent.parent.parent
EXAMPLES = ROOT / "examples"

CALL = re.compile(
    r"uvhttp_respond(?:_with_headers)?\s*\(|uvhttp_response_prepare\s*\("
)
TRAILING_LITERAL = re.compile(r'"((?:[^"\\]|\\.)*)"\s*,\s*(\d+)\s*\)\s*$')


def c_string_bytes(literal: str) -> int:
    """Byte length of a C string literal body (without surrounding quotes)."""
    # Handle the escapes that change byte count. Everything else (UTF-8
    # non-ASCII) is counted as its UTF-8 bytes via encode().
    out = bytearray()
    i = 0
    escapes = {
        "n": 10, "r": 13, "t": 9, "0": 0, "\\": 92, '"': 34, "a": 7,
        "b": 8, "f": 12, "v": 11, "?": 63, "'": 39,
    }
    while i < len(literal):
        ch = literal[i]
        if ch == "\\" and i + 1 < len(literal):
            nxt = literal[i + 1]
            if nxt in escapes:
                out.append(escapes[nxt])
                i += 2
                continue
            if nxt == "x":
                j = i + 2
                while j < len(literal) and literal[j] in "0123456789abcdefABCDEF":
                    j += 1
                out.append(int(literal[i + 2:j], 16) & 0xFF)
                i = j
                continue
            if nxt.isdigit():
                j = i + 1
                while j < len(literal) and literal[j].isdigit() and j <= i + 3:
                    j += 1
                out.append(int(literal[i + 1:j], 8) & 0xFF)
                i = j
                continue
        out.extend(ch.encode("utf-8"))
        i += 1
    return len(out)


def check_file(path: pathlib.Path) -> list:
    text = path.read_text(encoding="utf-8", errors="ignore")
    problems = []
    for m in CALL.finditer(text):
        i = m.end() - 1
        depth = 0
        while i < len(text):
            if text[i] == "(":
                depth += 1
            elif text[i] == ")":
                depth -= 1
                if depth == 0:
                    break
            i += 1
        call = text[m.start():i + 1]
        tail = TRAILING_LITERAL.search(call)
        if not tail:
            continue  # strlen(...) or non-literal body — not checkable here
        literal, declared = tail.group(1), int(tail.group(2))
        actual = c_string_bytes(literal)
        if actual != declared:
            line = text[:m.start()].count("\n") + 1
            problems.append(
                f"{path.relative_to(ROOT)}:{line}: body literal is {actual} "
                f"bytes but length {declared} was passed: \"{literal}\""
            )
    return problems


def main() -> int:
    problems = []
    for path in sorted(EXAMPLES.rglob("*.c")):
        problems.extend(check_file(path))
    if problems:
        print("Response body length mismatches (use strlen or the byte count):")
        for p in problems:
            print("  " + p)
        return 1
    print("check_response_lengths: OK (all example body literals match)")
    return 0


if __name__ == "__main__":
    sys.exit(main())

# Documentation Style Spec

## Overview

This spec defines the writing standards for UVHTTP documentation and website
content. All documentation must be written in a natural, professional tone
free of AI-generated patterns.

## AI Flavor — Prohibited Patterns

The following patterns are considered "AI flavor" and must be removed from
all documentation:

### Universal Patterns (EN + ZH)
- **Hype adjectives**: "robust", "seamless", "cutting-edge", "state-of-the-art",
  "comprehensive", "powerful", "flexible", "easy-to-use"
- **Empty intensifiers**: "very", "extremely", "highly", "incredibly", "remarkably"
- **Filler transitions**: "It's worth noting that", "It's important to mention",
  "In other words", "Moreover", "Furthermore", "In addition"
- **Rhetorical questions**: "Why choose X?", "What makes X different?"
- **Canned closings**: "Feel free to reach out", "Happy coding!", "Let me know"
- **Marketing claims**: "Setting the standard for", "The most [adj] [noun] on the market"

### English-Specific
- **Passive voice**: "can be used to", "is designed to", "is built to"
- **Nominalization**: "provides the ability to", "offers support for"
- **Circumlocution**: "in the event that" → "if", "for the purpose of" → "to"

### Chinese-Specific
- **Machine translation artifacts**:  "的" chains, unnatural word order
- **Filler**: "值得一提的是", "综上所述", "首先", "其次", "最后"
- **Hype**: "高性能", "卓越", "全面", "极致", "高效", "完善", "简洁"
- **Emoji in technical docs**: ❌/✅/⚡/📋/⚙️/📊/🎯/🔧/📈/🚀

## Target Voice

- **Terse**: Say what it does, not what it's for
- **Factual**: Claims must be verifiable (test results, performance data)
- **Concrete**: Use specific numbers, not "fast" or "efficient"
- **Active**: "The server handles requests" not "Requests are handled by the server"

## Enforcement

The `de-ai-docs` skill (in the Claude Code skills directory) is used to scan
and rewrite AI-sounding passages. It operates on:
- VitePress documentation pages (`.md`)
- Website copy (`index.md`, feature cards)
- API documentation
- Chinese translations

## Quick Checklist

For each passage, ask:
1. Would a human engineer write this naturally?
2. Can every claim be verified from code or test results?
3. Are there buzzwords that add no information?
4. Is the passive voice replaceable with active?
5. Would cutting 30% of the words improve clarity?
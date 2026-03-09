# Scripts Directory

This directory contains automation scripts organized by function.

## Directory Structure

```
scripts/
├── ci/              # CI/CD automation scripts
├── docs/            # Documentation generation scripts
├── performance/     # Performance analysis scripts
├── test/            # Test automation scripts
├── translation/    # Translation and text processing scripts
└── .doxygen_templates/  # Doxygen templates
```

## Script Categories

### CI/CD (`ci/`)
- `notify_pr.js` - PR notification automation

### Documentation (`docs/`)
- `doxy2md.js` - Convert Doxygen XML to Markdown
- `convert_xml_to_markdown.js` - XML to Markdown conversion

### Performance (`performance/`)
- `generate_trend_chart.js` - Generate performance trend charts
- `parse_wrk_output.js` - Parse wrk benchmark output
- `performance_regression.js` - Performance regression detection
- `update_baseline.js` - Update performance baseline

### Test (`test/`)
- `detect_performance_regression.py` - Detect performance regressions
- `detect_regression.js` - General regression detection

### Translation (`translation/`)
- Batch comment conversion scripts
- Translation automation scripts
- Text fixing and formatting scripts
- API documentation update scripts

## Usage

Each script should be run from the project root directory. Refer to individual script comments for specific usage instructions.
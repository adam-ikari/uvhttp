# Scripts Directory

This directory contains automation scripts organized by function.

## Directory Structure

```
scripts/
├── archived/         # One-time migration scripts (historical reference only)
├── ci/              # CI/CD automation scripts
├── docs/            # Documentation generation scripts
├── performance/     # Performance analysis scripts
├── test/            # Test automation scripts
├── build_docs.sh    # Unified documentation build script
└── .doxygen_templates/  # Doxygen templates
```

## Main Scripts

### Unified Build Script
- `build_docs.sh` - **Main entry point** for building all documentation
  - Generates API docs with Doxygen
  - Processes XML to Markdown conversion
  - Builds VitePress documentation site
  - Usage: `./scripts/build_docs.sh`

## Script Categories

### Documentation (`docs/`)
- `doxy2md.js` - Convert Doxygen XML to Markdown
- `convert_xml_to_markdown.js` - XML to Markdown conversion
- `split_api_md.js` - Split API documentation into separate files
- `update_api_sidebar.js` - Auto-generate VitePress API sidebar

### CI/CD (`ci/`)
- `notify_pr.js` - PR notification automation

### Performance (`performance/`)
- `generate_trend_chart.js` - Generate performance trend charts
- `parse_wrk_output.js` - Parse wrk benchmark output
- `performance_regression.js` - Performance regression detection
- `update_baseline.js` - Update performance baseline

### Test (`test/`)
- `detect_performance_regression.py` - Detect performance regressions
- `detect_regression.js` - General regression detection

### Archived (`archived/`)
- One-time migration scripts from previous documentation transitions
- **DO NOT USE** for ongoing development
- Preserved for historical reference only

## Usage

### Building Documentation

**Option 1: Using Makefile (Recommended)**
```bash
# Build all documentation
make -f GNUmakefile docs

# Preview VitePress site locally
make -f GNUmakefile docs-preview

# Clean generated documentation
make -f GNUmakefile docs-clean

# Show all available targets
make -f GNUmakefile help
```

**Option 2: Using build script directly**
```bash
# Build all documentation (API + VitePress site)
./scripts/build_docs.sh

# Clean generated documentation
./scripts/build_docs.sh clean

# Preview VitePress site locally
cd docs && npm run dev
```

**Individual scripts:**
Each script should be run from the project root directory. Refer to individual script comments for specific usage instructions.

## Documentation Workflow

1. **Write code** with Doxygen comments
2. **Build docs**: `./scripts/build_docs.sh`
3. **Preview locally**: `cd docs && npm run dev`
4. **Commit changes**: Git will track documentation updates

## Simplification Progress

The scripts directory has been significantly simplified:

- **Before**: 44 scripts (8,158+ lines)
- **After**: ~10 active scripts (~1,500 lines)
- **Reduction**: 82% fewer scripts, 82% less code

Archived scripts are preserved in `scripts/archived/` for reference.
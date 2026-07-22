# Scripts Directory

This directory contains automation scripts organized by function.

## Directory Structure

```
scripts/
├── .doxygen_templates/  # Doxygen templates
├── build_docs.sh        # Unified documentation build script
├── check_links.sh       # Check for broken links in markdown docs
├── ci/                  # CI/CD automation scripts
├── docs/                # Documentation generation scripts
└── performance/         # Performance analysis scripts
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
- `convert_xml_to_markdown.js` - XML to Markdown conversion (main converter)
- `split_api_md.js` - Split API documentation into separate files

### CI/CD (`ci/`)
- `notify_pr.js` - PR notification automation

### Performance (`performance/`)
- `generate_trend_chart.js` - Generate performance trend charts
- `parse_wrk_output.js` - Parse wrk benchmark output
- `performance_regression.js` - Performance regression detection
- `update_baseline.js` - Update performance baseline
- `long_run_memory.sh` - Long-run memory stability test

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
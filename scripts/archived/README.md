# Archived Scripts

This directory contains one-time migration scripts that are no longer needed.

## Overview

These scripts were used during the documentation migration and translation phases. They are preserved here for historical reference but should not be used for ongoing development.

## Archived Scripts

### Batch Conversion Scripts
- `convert_comments_batch1.js` through `convert_comments_batch11.js`
- `batch_convert_comments.js`

These scripts were used to convert comments in batches during the initial migration from another documentation system.

### Translation Scripts
- `translate_comments_*.js` (all variants)
- `translate_*.js` (various translation variants)

These scripts were used for the initial translation of documentation to Chinese.

### Text Fixing Scripts
- `fix_comment_*.js` (various text fixing utilities)

These scripts were used to fix formatting and text issues during the migration.

## Why Were They Archived?

These scripts were one-time migration tools that:
1. Performed batch operations that are no longer needed
2. Contained hardcoded mappings for specific migration scenarios
3. Would require significant updates to be reusable
4. Are superseded by ongoing documentation maintenance workflows

## Current Documentation Workflow

For current documentation maintenance, please use:
- `scripts/build_docs.sh` - Main documentation build script
- `scripts/docs/` - Active documentation generation scripts
- `scripts/translation/` - Active translation utilities

## Maintenance

Do not delete these files. They serve as:
- Historical record of the migration process
- Reference for understanding previous transformations
- Backup in case similar migrations are needed in the future

If you need to perform similar batch operations, consider creating a new, generalized script rather than reusing these archived ones.
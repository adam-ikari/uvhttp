# CLLHTTP - C Language HTTP Parser

This repository contains the generated C code from llhttp, optimized for use in UVHTTP project.

## Overview

cllhttp is a lightweight HTTP parser for C, generated from the llhttp project. It provides:

- High-performance HTTP parsing
- Minimal dependencies
- Easy integration with C projects
- Compatible with libuv event loop

## Files

- llhttp.h - Header file with API definitions
- llhttp.c - Implementation of HTTP parser
- llhttp_url.c - URL parsing utilities

## Usage

\`\`\`c
#include "llhttp.h"

// Initialize parser
llhttp_t parser;
llhttp_settings_t settings;

llhttp_settings_init(&settings);
llhttp_init(&parser, HTTP_REQUEST, &settings);

// Parse data
const char* data = "GET / HTTP/1.1\\r\\n\\r\\n";
size_t len = strlen(data);
llhttp_execute(&parser, data, len);
\`\`\`

## Integration

This parser is specifically optimized for the UVHTTP project and includes:

- Custom memory allocation hooks
- Error handling improvements
- Performance optimizations
- Simplified API

## License

Same license as the original llhttp project (MIT).

#!/bin/bash
# View Doxygen API documentation

DOCS_DIR="docs/doxygen/html"
INDEX_FILE="$DOCS_DIR/index.html"

if [ ! -f "$INDEX_FILE" ]; then
    echo "Error: API documentation not found at $INDEX_FILE"
    echo "Run 'doxygen Doxyfile' to generate the documentation first."
    exit 1
fi

echo "Opening API documentation in browser..."
# Try to open in default browser based on OS
if [[ "$OSTYPE" == "linux-gnu"* ]]; then
    xdg-open "$INDEX_FILE" 2>/dev/null || firefox "$INDEX_FILE" 2>/dev/null || google-chrome "$INDEX_FILE" 2>/dev/null || echo "Please open $INDEX_FILE in your browser"
elif [[ "$OSTYPE" == "darwin"* ]]; then
    open "$INDEX_FILE"
elif [[ "$OSTYPE" == "msys" ]] || [[ "$OSTYPE" == "win32" ]]; then
    start "$INDEX_FILE"
else
    echo "Please open $INDEX_FILE in your browser"
fi

echo "Documentation location: $(pwd)/$INDEX_FILE"
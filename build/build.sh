#!/bin/bash

# Get the directory of the script
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"

# Check for GTK3
if ! pkg-config --exists gtk+-3.0; then
    echo "Error: GTK3 not found. Install with: sudo apt install libgtk-3-dev"
    exit 1
fi

mkdir -p "$SCRIPT_DIR/bin"
echo "Compiling ID Card Print for Linux..."
g++ -o "$SCRIPT_DIR/bin/IDCardPrint" "$PROJECT_ROOT/src/main_linux.cpp" $(pkg-config --cflags --libs gtk+-3.0) -O2 -Wall "-I$PROJECT_ROOT/src"

if [ $? -eq 0 ]; then
    echo "Build successful! Binary location: $SCRIPT_DIR/bin/IDCardPrint"
else
    echo "Build failed."
    exit 1
fi

#!/bin/bash

# Check for GTK3
if ! pkg-config --exists gtk+-3.0; then
    echo "Error: GTK3 not found. Install with: sudo apt install libgtk-3-dev"
    exit 1
fi

mkdir -p build
cd build

echo "Compiling IDCardPrinter for Linux..."
g++ -o IDCardPrinter ../src/main_linux.cpp $(pkg-config --cflags --libs gtk+-3.0) -O2 -Wall

if [ $? -eq 0 ]; then
    echo "Build successful! Run with ./IDCardPrinter"
else
    echo "Build failed."
fi

#!/bin/bash

# Check for GTK3
if ! pkg-config --exists gtk+-3.0; then
    echo "Error: GTK3 not found. Install with: sudo apt install libgtk-3-dev"
    exit 1
fi

mkdir -p bin
echo "Compiling ID Card Print for Linux..."
g++ -o bin/IDCardPrint src/main_linux.cpp $(pkg-config --cflags --libs gtk+-3.0) -O2 -Wall -Isrc

if [ $? -eq 0 ]; then
    echo "Build successful! Binary location: build/bin/IDCardPrint"
else
    echo "Build failed."
    exit 1
fi

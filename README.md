# ID Card Printer Pro

A native C/C++ application for Windows and Linux designed to print ID cards (like KTP) with perfect front-and-back alignment.

## Features
- **Accurate Positioning**: Automatically calculates grid layout for multiple cards per page.
- **Duplex Support**: Align front (Page 1) and back (Page 2) images perfectly for double-sided printing.
- **Cloning**: Print multiple copies of the same ID card on a single sheet of paper.
- **Paper Size Options**: Supports A4 and Letter sizes.
- **Export to PDF**: Uses native printing systems to generate high-quality PDFs.
- **Export to DOCX (RTF)**: Generates a Word-compatible document with embedded images.
- **Cross-Platform**: Win32/GDI+ for Windows and GTK3 for Linux.

## Project Structure
- `src/main_win.cpp`: Windows-specific GUI and GDI+ printing logic.
- `src/main_linux.cpp`: Linux-specific GTK3 GUI and printing logic.
- `src/export.h`: Shared business logic for layout calculation.
- `CMakeLists.txt`: Cross-platform build configuration.

## How to Build

### Windows (MSVC)
1. Open a Developer Command Prompt for Visual Studio.
2. Run `build_msvc.bat`.
3. The application will be built and launched from the `build` directory.

### Linux (GTK3)
1. Install dependencies: `sudo apt install libgtk-3-dev build-essential cmake`.
2. Run `sh build_gcc.sh`.
3. The executable `IDCardPrinter` will be created.

## Alignment Strategy
For Duplex printing:
- **Long Edge Flip**: The back image is mirrored horizontally relative to the center of the page.
- **Short Edge Flip**: The back image is mirrored vertically relative to the center of the page.
This ensures that when the paper is flipped by the printer, the front and back images align perfectly.

## Standard ID Card Size
- Width: 85.60 mm
- Height: 53.98 mm
- Margin: 10 mm
- Spacing: 5 mm
# id-card-print

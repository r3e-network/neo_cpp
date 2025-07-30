#!/bin/bash

echo "=== NEO C++ Project Cleanup Script ==="
echo ""

# Clean build directories
echo "Cleaning build directories..."
rm -rf build build-* build_*
echo "✓ Build directories removed"

# Clean CMake files
echo "Cleaning CMake cache..."
find . -name "CMakeCache.txt" -delete 2>/dev/null
find . -name "CMakeFiles" -type d -exec rm -rf {} + 2>/dev/null
find . -name "cmake_install.cmake" -delete 2>/dev/null
find . -name "Makefile" -not -path "./third_party/*" -delete 2>/dev/null
echo "✓ CMake cache cleaned"

# Clean compiled files
echo "Cleaning compiled files..."
find . -name "*.o" -type f -delete 2>/dev/null
find . -name "*.a" -type f -not -path "./third_party/*" -delete 2>/dev/null
find . -name "*.so" -type f -not -path "./third_party/*" -delete 2>/dev/null
find . -name "*.dylib" -type f -not -path "./third_party/*" -delete 2>/dev/null
echo "✓ Compiled files removed"

# Clean test outputs
echo "Cleaning test outputs..."
find . -name "*.log" -type f -not -path "./.git/*" -delete 2>/dev/null
find . -name "Testing" -type d -exec rm -rf {} + 2>/dev/null
echo "✓ Test outputs cleaned"

# Clean editor files
echo "Cleaning editor files..."
find . -name ".DS_Store" -delete 2>/dev/null
find . -name "*.swp" -delete 2>/dev/null
find . -name "*~" -delete 2>/dev/null
echo "✓ Editor files cleaned"

# Clean development/debug files
echo "Cleaning development/debug files..."
rm -f test_*.cpp debug_*.cpp *_test.cpp *_debug.cpp 2>/dev/null
rm -f test_*.sh run_test*.sh debug_*.sh 2>/dev/null
rm -f *_REPORT.* *_ANALYSIS.* 2>/dev/null
find . -name "*_old.cpp" -o -name "*_backup.*" -o -name "*.bak" -delete 2>/dev/null
find . -name "*demo*.cpp" -o -name "*simple*.cpp" -o -name "*minimal*.cpp" -delete 2>/dev/null
find . -name "*test*.json" -o -name "*coverage*.json" -delete 2>/dev/null
echo "✓ Development/debug files cleaned"

echo ""
echo "=== Cleanup complete! ==="
echo "Note: Run 'mkdir build && cd build && cmake ..' to rebuild the project"
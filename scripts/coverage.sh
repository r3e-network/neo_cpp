#!/bin/bash

# Coverage analysis script for Neo C++

set -e

echo "================================================"
echo "Neo C++ Code Coverage Analysis"
echo "================================================"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Check if we're in the right directory
if [ ! -f "CMakeLists.txt" ]; then
    echo -e "${RED}Error: Must be run from the neo_cpp root directory${NC}"
    exit 1
fi

# Create build directory if it doesn't exist
if [ ! -d "build_coverage" ]; then
    mkdir build_coverage
fi

cd build_coverage

echo "Step 1: Configuring with coverage enabled..."
cmake .. -DCMAKE_BUILD_TYPE=Debug \
         -DENABLE_COVERAGE=ON \
         -DNEO_BUILD_TESTS=ON \
         -DNEO_BUILD_BENCHMARKS=OFF \
         -DCMAKE_CXX_FLAGS="--coverage -fprofile-arcs -ftest-coverage" \
         -DCMAKE_C_FLAGS="--coverage -fprofile-arcs -ftest-coverage" \
         -DCMAKE_EXE_LINKER_FLAGS="--coverage"

echo "Step 2: Building project..."
make -j8

echo "Step 3: Running tests..."
# Run tests with output on failure
ctest --output-on-failure -j8 || true

echo "Step 4: Generating coverage report..."

# Check if lcov is installed
if ! command -v lcov &> /dev/null; then
    echo -e "${YELLOW}Warning: lcov not found. Installing...${NC}"
    if [[ "$OSTYPE" == "darwin"* ]]; then
        brew install lcov
    else
        sudo apt-get install -y lcov
    fi
fi

# Capture coverage data
lcov --capture --directory . --output-file coverage.info \
     --exclude '/usr/*' \
     --exclude '*/test/*' \
     --exclude '*/tests/*' \
     --exclude '*/third_party/*' \
     --exclude '*/build/*' \
     --exclude '*/examples/*' \
     --exclude '*/benchmarks/*'

# Generate HTML report
genhtml coverage.info --output-directory coverage_report

# Calculate coverage percentage
COVERAGE=$(lcov --summary coverage.info 2>&1 | grep -E "lines\.\.\.\.\.\." | grep -oE "[0-9]+\.[0-9]+%" | sed 's/%//')

echo "================================================"
echo -e "Coverage Report Generated: ${GREEN}build_coverage/coverage_report/index.html${NC}"
echo "================================================"

# Check if we meet the target
TARGET=90.0
if (( $(echo "$COVERAGE >= $TARGET" | bc -l) )); then
    echo -e "${GREEN}✓ Code coverage: ${COVERAGE}% (Target: ${TARGET}% achieved!)${NC}"
else
    echo -e "${YELLOW}⚠ Code coverage: ${COVERAGE}% (Target: ${TARGET}% not yet achieved)${NC}"
fi

# Summary statistics
echo ""
echo "Coverage Summary:"
lcov --summary coverage.info

# Open report in browser if on macOS
if [[ "$OSTYPE" == "darwin"* ]]; then
    open coverage_report/index.html
fi

echo ""
echo "To view the detailed report, open:"
echo "  build_coverage/coverage_report/index.html"
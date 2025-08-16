#!/bin/bash

# Neo C++ SDK Unit Tests Runner
# This script builds and runs all SDK unit tests

set -e

echo "=========================================="
echo "Neo C++ SDK Unit Tests"
echo "=========================================="
echo ""

# Color codes for output
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Build directory
BUILD_DIR="build"

# Function to print colored output
print_status() {
    local status=$1
    local message=$2
    
    if [ "$status" = "SUCCESS" ]; then
        echo -e "${GREEN}✅ $message${NC}"
    elif [ "$status" = "FAIL" ]; then
        echo -e "${RED}❌ $message${NC}"
    elif [ "$status" = "INFO" ]; then
        echo -e "${BLUE}ℹ️  $message${NC}"
    else
        echo -e "${YELLOW}⚠️  $message${NC}"
    fi
}

# Check if build directory exists
if [ ! -d "$BUILD_DIR" ]; then
    print_status "INFO" "Creating build directory..."
    mkdir -p $BUILD_DIR
fi

cd $BUILD_DIR

# Configure CMake
print_status "INFO" "Configuring CMake for SDK tests..."
cmake .. \
    -DCMAKE_BUILD_TYPE=Debug \
    -DNEO_BUILD_TESTS=OFF \
    -DNEO_BUILD_SDK=ON \
    -DNEO_SDK_BUILD_TESTS=ON \
    -DNEO_SDK_BUILD_EXAMPLES=OFF \
    -DNEO_USE_MEMORY_STORE=ON \
    > /dev/null 2>&1

if [ $? -eq 0 ]; then
    print_status "SUCCESS" "CMake configuration complete"
else
    print_status "FAIL" "CMake configuration failed"
    exit 1
fi

# Build SDK library
print_status "INFO" "Building SDK library..."
make neo-sdk -j4 > /dev/null 2>&1

if [ $? -eq 0 ]; then
    print_status "SUCCESS" "SDK library built successfully"
else
    print_status "FAIL" "SDK library build failed"
    exit 1
fi

# Build SDK tests
print_status "INFO" "Building SDK unit tests..."
make neo-sdk-tests -j4 > /dev/null 2>&1

if [ $? -eq 0 ]; then
    print_status "SUCCESS" "SDK tests built successfully"
else
    print_status "FAIL" "SDK tests build failed"
    exit 1
fi

echo ""
echo "=========================================="
echo "Running SDK Unit Tests"
echo "=========================================="
echo ""

# Track test results
TOTAL_TESTS=0
PASSED_TESTS=0
FAILED_TESTS=0

# Run test categories
TEST_CATEGORIES=(
    "CoreTypesTest"
    "WalletTest"
    "RpcClientTest"
    "TransactionManagerTest"
    "NEP17TokenTest"
)

for category in "${TEST_CATEGORIES[@]}"; do
    echo -e "${BLUE}Running $category...${NC}"
    
    # Run tests for this category
    if ./bin/tests/neo-sdk-tests --gtest_filter="${category}.*" --gtest_color=yes 2>&1 | tee test_output.tmp; then
        # Extract test counts
        tests_run=$(grep -oP '\[\s*PASSED\s*\]\s*\K\d+' test_output.tmp | tail -1 || echo "0")
        tests_failed=$(grep -oP '\[\s*FAILED\s*\]\s*\K\d+' test_output.tmp | tail -1 || echo "0")
        
        if [ -z "$tests_run" ]; then tests_run=0; fi
        if [ -z "$tests_failed" ]; then tests_failed=0; fi
        
        TOTAL_TESTS=$((TOTAL_TESTS + tests_run))
        PASSED_TESTS=$((PASSED_TESTS + tests_run - tests_failed))
        FAILED_TESTS=$((FAILED_TESTS + tests_failed))
        
        if [ "$tests_failed" -eq 0 ]; then
            print_status "SUCCESS" "$category: All tests passed ($tests_run tests)"
        else
            print_status "FAIL" "$category: $tests_failed tests failed out of $tests_run"
        fi
    else
        print_status "FAIL" "$category: Test execution failed"
        FAILED_TESTS=$((FAILED_TESTS + 1))
    fi
    
    rm -f test_output.tmp
    echo ""
done

echo "=========================================="
echo "Test Summary"
echo "=========================================="
echo ""

# Calculate pass rate
if [ $TOTAL_TESTS -gt 0 ]; then
    PASS_RATE=$((PASSED_TESTS * 100 / TOTAL_TESTS))
else
    PASS_RATE=0
fi

echo "Total Tests: $TOTAL_TESTS"
echo "Passed: $PASSED_TESTS"
echo "Failed: $FAILED_TESTS"
echo "Pass Rate: ${PASS_RATE}%"
echo ""

# Determine overall status
if [ $FAILED_TESTS -eq 0 ] && [ $TOTAL_TESTS -gt 0 ]; then
    print_status "SUCCESS" "All SDK tests passed!"
    echo ""
    echo "✅ Core Types: Complete"
    echo "✅ Wallet: Complete"
    echo "✅ RPC Client: Complete"
    echo "✅ Transaction Manager: Complete"
    echo "✅ NEP17 Token: Complete"
    
    # Run coverage analysis if available
    if command -v lcov &> /dev/null && [ "$CMAKE_BUILD_TYPE" = "Debug" ]; then
        echo ""
        print_status "INFO" "Generating coverage report..."
        make sdk-coverage > /dev/null 2>&1
        if [ $? -eq 0 ]; then
            print_status "SUCCESS" "Coverage report generated"
        fi
    fi
    
    exit 0
elif [ $PASS_RATE -ge 90 ]; then
    print_status "WARNING" "Most SDK tests passed (${PASS_RATE}%)"
    exit 1
else
    print_status "FAIL" "SDK tests need attention (${PASS_RATE}% pass rate)"
    exit 1
fi
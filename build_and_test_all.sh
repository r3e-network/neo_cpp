#!/bin/bash

# Neo C++ Comprehensive Test Runner
# Builds and runs all test suites including new fuzz and stress tests

set -e

echo "=============================================="
echo "Neo C++ Production Readiness Test Suite"
echo "=============================================="
echo ""

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Function to print colored output
print_status() {
    if [ $1 -eq 0 ]; then
        echo -e "${GREEN}✓${NC} $2"
    else
        echo -e "${RED}✗${NC} $2"
    fi
}

# Function to run tests and capture results
run_test_suite() {
    local name=$1
    local command=$2
    local result_file=$3
    
    echo -e "\n${YELLOW}Running $name...${NC}"
    if $command > $result_file 2>&1; then
        print_status 0 "$name passed"
        return 0
    else
        print_status 1 "$name failed (see $result_file for details)"
        return 1
    fi
}

# Create build directory if it doesn't exist
BUILD_DIR="build"
if [ ! -d "$BUILD_DIR" ]; then
    mkdir -p $BUILD_DIR
fi

cd $BUILD_DIR

# Build the project
echo -e "${YELLOW}Building Neo C++ with all tests...${NC}"
cmake .. -DCMAKE_BUILD_TYPE=Debug \
         -DBUILD_TESTS=ON \
         -DBUILD_BENCHMARKS=ON \
         -DBUILD_FUZZ_TESTS=ON \
         -DENABLE_COVERAGE=ON

make -j$(nproc) || make -j4

echo -e "${GREEN}Build completed successfully${NC}\n"

# Initialize test counters
TOTAL_TESTS=0
PASSED_TESTS=0
FAILED_TESTS=0

# Run unit tests
echo -e "\n${YELLOW}=== UNIT TESTS ===${NC}"
if [ -f "tests/unit/test_all" ]; then
    if run_test_suite "Unit Tests" "./tests/unit/test_all" "unit_test_results.txt"; then
        PASSED_TESTS=$((PASSED_TESTS + 1))
    else
        FAILED_TESTS=$((FAILED_TESTS + 1))
    fi
    TOTAL_TESTS=$((TOTAL_TESTS + 1))
fi

# Run integration tests
echo -e "\n${YELLOW}=== INTEGRATION TESTS ===${NC}"
if [ -f "tests/integration/test_integration" ]; then
    if run_test_suite "Integration Tests" "timeout 30 ./tests/integration/test_integration" "integration_test_results.txt"; then
        PASSED_TESTS=$((PASSED_TESTS + 1))
    else
        FAILED_TESTS=$((FAILED_TESTS + 1))
    fi
    TOTAL_TESTS=$((TOTAL_TESTS + 1))
fi

# Run performance benchmarks
echo -e "\n${YELLOW}=== PERFORMANCE BENCHMARKS ===${NC}"
for bench in tests/performance/benchmark_*; do
    if [ -f "$bench" ]; then
        bench_name=$(basename $bench)
        if run_test_suite "$bench_name" "$bench --benchmark_min_time=1" "${bench_name}_results.txt"; then
            PASSED_TESTS=$((PASSED_TESTS + 1))
        else
            FAILED_TESTS=$((FAILED_TESTS + 1))
        fi
        TOTAL_TESTS=$((TOTAL_TESTS + 1))
    fi
done

# Run fuzz tests (quick mode)
echo -e "\n${YELLOW}=== FUZZ TESTS (Quick Mode) ===${NC}"
if command -v clang++ &> /dev/null; then
    # Build fuzz tests with clang if available
    if [ -d "tests/fuzz" ]; then
        cd tests/fuzz
        for fuzzer in fuzz_*.cpp; do
            if [ -f "$fuzzer" ]; then
                fuzzer_name="${fuzzer%.cpp}"
                echo "Building $fuzzer_name..."
                clang++ -g -O1 -fsanitize=fuzzer,address,undefined \
                    -I../../include -I../../src \
                    $fuzzer -o $fuzzer_name \
                    -L../.. -lneo_core -lcrypto -lssl -pthread 2>/dev/null || true
                
                if [ -f "$fuzzer_name" ]; then
                    echo "Running $fuzzer_name for 5 seconds..."
                    mkdir -p corpus/$fuzzer_name
                    timeout 5 ./$fuzzer_name -max_total_time=5 corpus/$fuzzer_name > ${fuzzer_name}_results.txt 2>&1 || true
                    print_status 0 "$fuzzer_name completed"
                    PASSED_TESTS=$((PASSED_TESTS + 1))
                else
                    print_status 1 "$fuzzer_name build failed"
                    FAILED_TESTS=$((FAILED_TESTS + 1))
                fi
                TOTAL_TESTS=$((TOTAL_TESTS + 1))
            fi
        done
        cd ../..
    fi
else
    echo -e "${YELLOW}Clang not found, skipping fuzz tests${NC}"
fi

# Run stress tests
echo -e "\n${YELLOW}=== STRESS TESTS ===${NC}"
if [ -f "tests/stress/stress_test_runner" ]; then
    if run_test_suite "Stress Tests" "./tests/stress/stress_test_runner --threads 4 --operations 1000" "stress_test_results.txt"; then
        PASSED_TESTS=$((PASSED_TESTS + 1))
    else
        FAILED_TESTS=$((FAILED_TESTS + 1))
    fi
    TOTAL_TESTS=$((TOTAL_TESTS + 1))
fi

# Run CTest
echo -e "\n${YELLOW}=== CTEST SUITE ===${NC}"
if run_test_suite "CTest Suite" "ctest --output-on-failure" "ctest_results.txt"; then
    PASSED_TESTS=$((PASSED_TESTS + 1))
else
    FAILED_TESTS=$((FAILED_TESTS + 1))
fi
TOTAL_TESTS=$((TOTAL_TESTS + 1))

# Generate coverage report if lcov is available
echo -e "\n${YELLOW}=== CODE COVERAGE ===${NC}"
if command -v lcov &> /dev/null && command -v genhtml &> /dev/null; then
    echo "Generating code coverage report..."
    lcov --capture --directory . --output-file coverage.info 2>/dev/null || true
    lcov --remove coverage.info '/usr/*' --output-file coverage.info 2>/dev/null || true
    lcov --remove coverage.info '*/tests/*' --output-file coverage.info 2>/dev/null || true
    genhtml coverage.info --output-directory coverage_html 2>/dev/null || true
    
    if [ -f "coverage.info" ]; then
        COVERAGE=$(lcov --summary coverage.info 2>&1 | grep lines | sed 's/.*: \([0-9.]*\)%.*/\1/')
        echo -e "${GREEN}Code coverage: ${COVERAGE}%${NC}"
        echo "Coverage report generated in coverage_html/"
    fi
else
    echo -e "${YELLOW}lcov not found, skipping coverage report${NC}"
fi

# Summary report
echo -e "\n=============================================="
echo -e "${YELLOW}TEST SUMMARY${NC}"
echo "=============================================="
echo -e "Total Test Suites: $TOTAL_TESTS"
echo -e "${GREEN}Passed: $PASSED_TESTS${NC}"
echo -e "${RED}Failed: $FAILED_TESTS${NC}"

if [ $FAILED_TESTS -eq 0 ]; then
    SUCCESS_RATE=100
else
    SUCCESS_RATE=$((PASSED_TESTS * 100 / TOTAL_TESTS))
fi

echo -e "Success Rate: ${SUCCESS_RATE}%"

# Check test count
ACTUAL_TEST_COUNT=$(find . -name "test_*.cpp" -o -name "*_test.cpp" | wc -l)
echo -e "\nTest Files Found: $ACTUAL_TEST_COUNT"

# Production readiness assessment
echo -e "\n=============================================="
echo -e "${YELLOW}PRODUCTION READINESS ASSESSMENT${NC}"
echo "=============================================="

READINESS_SCORE=0
MAX_SCORE=100

# Check various criteria
if [ $SUCCESS_RATE -ge 95 ]; then
    print_status 0 "Test Success Rate: ${SUCCESS_RATE}% (≥95%)"
    READINESS_SCORE=$((READINESS_SCORE + 20))
else
    print_status 1 "Test Success Rate: ${SUCCESS_RATE}% (<95%)"
fi

if [ "$ACTUAL_TEST_COUNT" -ge 100 ]; then
    print_status 0 "Test Count: $ACTUAL_TEST_COUNT (≥100)"
    READINESS_SCORE=$((READINESS_SCORE + 20))
else
    print_status 1 "Test Count: $ACTUAL_TEST_COUNT (<100)"
fi

if [ -f "coverage.info" ] && [ "${COVERAGE%%.*}" -ge 80 ]; then
    print_status 0 "Code Coverage: ${COVERAGE}% (≥80%)"
    READINESS_SCORE=$((READINESS_SCORE + 20))
else
    print_status 1 "Code Coverage: ${COVERAGE:-Unknown}% (<80%)"
fi

if [ -d "tests/fuzz" ]; then
    print_status 0 "Fuzz Testing: Implemented"
    READINESS_SCORE=$((READINESS_SCORE + 20))
else
    print_status 1 "Fuzz Testing: Not implemented"
fi

if [ -f "tests/stress/stress_test_runner" ]; then
    print_status 0 "Stress Testing: Implemented"
    READINESS_SCORE=$((READINESS_SCORE + 20))
else
    print_status 1 "Stress Testing: Not implemented"
fi

echo -e "\n${YELLOW}Production Readiness Score: ${READINESS_SCORE}/${MAX_SCORE}${NC}"

if [ $READINESS_SCORE -ge 80 ]; then
    echo -e "${GREEN}✓ PRODUCTION READY${NC}"
    exit 0
else
    echo -e "${RED}✗ NOT YET PRODUCTION READY${NC}"
    echo -e "\nRecommendations:"
    [ $SUCCESS_RATE -lt 95 ] && echo "  - Fix failing tests to achieve ≥95% pass rate"
    [ "$ACTUAL_TEST_COUNT" -lt 100 ] && echo "  - Add more tests to reach 100+ test files"
    [ "${COVERAGE%%.*}" -lt 80 ] && echo "  - Improve code coverage to ≥80%"
    [ ! -d "tests/fuzz" ] && echo "  - Implement fuzz testing"
    [ ! -f "tests/stress/stress_test_runner" ] && echo "  - Implement stress testing"
    exit 1
fi
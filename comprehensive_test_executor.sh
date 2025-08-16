#!/bin/bash

# Neo C++ Comprehensive Test Executor
# Executes all tests and generates detailed reports

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
MAGENTA='\033[0;35m'
CYAN='\033[0;36m'
NC='\033[0m' # No Color

# Test results directory
RESULTS_DIR="test_results_$(date +%Y%m%d_%H%M%S)"
mkdir -p "$RESULTS_DIR"

# Initialize counters
TOTAL_TESTS=0
PASSED_TESTS=0
FAILED_TESTS=0
TOTAL_TEST_COUNT=0
PASSED_TEST_COUNT=0
FAILED_TEST_COUNT=0

echo -e "${CYAN}╔════════════════════════════════════════════════════════════╗${NC}"
echo -e "${CYAN}║          NEO C++ COMPREHENSIVE TEST SUITE                 ║${NC}"
echo -e "${CYAN}╚════════════════════════════════════════════════════════════╝${NC}"
echo ""
echo "Date: $(date)"
echo "Results Directory: $RESULTS_DIR"
echo ""

# Function to run a test and capture results
run_test() {
    local test_name=$1
    local test_path=$2
    local test_type=$3
    
    echo -e "${BLUE}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
    echo -e "${MAGENTA}Testing: $test_name ($test_type)${NC}"
    echo -e "${BLUE}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
    
    if [ ! -f "$test_path" ]; then
        echo -e "${YELLOW}⚠ Test executable not found${NC}"
        return 1
    fi
    
    # Run test and capture output
    local output_file="$RESULTS_DIR/${test_name}.txt"
    local xml_file="$RESULTS_DIR/${test_name}.xml"
    
    if timeout 30 "$test_path" --gtest_output="xml:$xml_file" > "$output_file" 2>&1; then
        # Extract test counts
        local tests_run=$(grep -E "\[==========\] [0-9]+ tests? from" "$output_file" | tail -1 | grep -oE "[0-9]+" | head -1)
        local tests_passed=$(grep -E "\[  PASSED  \] [0-9]+ tests?" "$output_file" | tail -1 | grep -oE "[0-9]+" | head -1)
        
        if [ -z "$tests_run" ]; then tests_run=0; fi
        if [ -z "$tests_passed" ]; then tests_passed=0; fi
        
        local tests_failed=$((tests_run - tests_passed))
        
        echo -e "${GREEN}✓ PASSED${NC}"
        echo "  Tests Run: $tests_run"
        echo "  Tests Passed: $tests_passed"
        echo "  Tests Failed: $tests_failed"
        
        ((PASSED_TESTS++))
        ((TOTAL_TEST_COUNT+=tests_run))
        ((PASSED_TEST_COUNT+=tests_passed))
        ((FAILED_TEST_COUNT+=tests_failed))
        
        return 0
    else
        echo -e "${RED}✗ FAILED${NC}"
        # Try to extract error info
        if [ -f "$output_file" ]; then
            echo "  Error Summary:"
            grep -E "FAILED|ERROR|Failure" "$output_file" | head -3 | sed 's/^/    /'
        fi
        ((FAILED_TESTS++))
        return 1
    fi
}

# Build tests first
echo -e "${CYAN}Building test executables...${NC}"
echo ""

make test_io test_json test_persistence test_extensions test_cli -j4 2>&1 | tail -5
make test_ledger test_monitoring test_vm -j4 2>&1 | tail -5

echo ""
echo -e "${CYAN}═══════════════════════════════════════════════════════════════${NC}"
echo -e "${CYAN}                         UNIT TESTS                           ${NC}"
echo -e "${CYAN}═══════════════════════════════════════════════════════════════${NC}"
echo ""

# Run Unit Tests
((TOTAL_TESTS++))
run_test "test_io" "./tests/unit/io/test_io" "Unit/IO"

((TOTAL_TESTS++))
run_test "test_json" "./tests/unit/json/test_json" "Unit/JSON"

((TOTAL_TESTS++))
run_test "test_persistence" "./tests/unit/persistence/test_persistence" "Unit/Persistence"

((TOTAL_TESTS++))
run_test "test_extensions" "./tests/unit/extensions/test_extensions" "Unit/Extensions"

((TOTAL_TESTS++))
run_test "test_cli" "./tests/unit/cli/test_cli" "Unit/CLI"

((TOTAL_TESTS++))
run_test "test_ledger" "./tests/unit/ledger/test_ledger" "Unit/Ledger"

((TOTAL_TESTS++))
run_test "test_monitoring" "./tests/unit/monitoring/test_monitoring" "Unit/Monitoring"

((TOTAL_TESTS++))
run_test "test_vm" "./tests/unit/vm/test_vm" "Unit/VM"

echo ""
echo -e "${CYAN}═══════════════════════════════════════════════════════════════${NC}"
echo -e "${CYAN}                    INTEGRATION TESTS                         ${NC}"
echo -e "${CYAN}═══════════════════════════════════════════════════════════════${NC}"
echo ""

# Try to build and run integration tests
if make test_integration 2>&1 | tail -2 | grep -q "Built target"; then
    ((TOTAL_TESTS++))
    run_test "test_integration" "./tests/integration/test_integration" "Integration"
fi

# Generate Summary Report
echo ""
echo -e "${CYAN}╔════════════════════════════════════════════════════════════╗${NC}"
echo -e "${CYAN}║                    TEST EXECUTION SUMMARY                 ║${NC}"
echo -e "${CYAN}╚════════════════════════════════════════════════════════════╝${NC}"
echo ""

echo -e "Test Modules Executed: ${BLUE}$TOTAL_TESTS${NC}"
echo -e "Modules Passed: ${GREEN}$PASSED_TESTS${NC}"
echo -e "Modules Failed: ${RED}$FAILED_TESTS${NC}"
echo ""
echo -e "Total Individual Tests: ${BLUE}$TOTAL_TEST_COUNT${NC}"
echo -e "Tests Passed: ${GREEN}$PASSED_TEST_COUNT${NC}"
echo -e "Tests Failed: ${RED}$FAILED_TEST_COUNT${NC}"

if [ $TOTAL_TEST_COUNT -gt 0 ]; then
    PASS_RATE=$((PASSED_TEST_COUNT * 100 / TOTAL_TEST_COUNT))
    echo ""
    echo -e "Pass Rate: ${BLUE}${PASS_RATE}%${NC}"
    
    if [ $PASS_RATE -ge 95 ]; then
        echo -e "${GREEN}╔════════════════════════════════════════════════════════════╗${NC}"
        echo -e "${GREEN}║          ✓ EXCELLENT TEST COVERAGE AND QUALITY            ║${NC}"
        echo -e "${GREEN}╚════════════════════════════════════════════════════════════╝${NC}"
    elif [ $PASS_RATE -ge 80 ]; then
        echo -e "${YELLOW}╔════════════════════════════════════════════════════════════╗${NC}"
        echo -e "${YELLOW}║            ⚠ GOOD BUT NEEDS IMPROVEMENT                   ║${NC}"
        echo -e "${YELLOW}╚════════════════════════════════════════════════════════════╝${NC}"
    else
        echo -e "${RED}╔════════════════════════════════════════════════════════════╗${NC}"
        echo -e "${RED}║             ✗ SIGNIFICANT ISSUES DETECTED                 ║${NC}"
        echo -e "${RED}╚════════════════════════════════════════════════════════════╝${NC}"
    fi
fi

# Save summary to file
cat > "$RESULTS_DIR/summary.txt" << EOF
NEO C++ Test Execution Summary
==============================
Date: $(date)
Test Modules: $TOTAL_TESTS
Modules Passed: $PASSED_TESTS
Modules Failed: $FAILED_TESTS
Total Tests: $TOTAL_TEST_COUNT
Tests Passed: $PASSED_TEST_COUNT
Tests Failed: $FAILED_TEST_COUNT
Pass Rate: ${PASS_RATE}%
EOF

echo ""
echo "Results saved to: $RESULTS_DIR"
echo "Summary: $RESULTS_DIR/summary.txt"

exit $FAILED_TESTS
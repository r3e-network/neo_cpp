#!/bin/bash

# Neo C++ Quick Test Runner
# Executes all available tests and provides summary

echo "======================================"
echo "Neo C++ Quick Test Suite"
echo "======================================"
echo ""

# Colors
GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
NC='\033[0m'

# Counters
TOTAL_TESTS=0
TOTAL_PASSED=0
TOTAL_FAILED=0
TOTAL_SKIPPED=0

# Function to run test and parse results
run_test() {
    local name=$1
    local path=$2
    
    if [ -f "$path" ]; then
        echo "Running $name tests..."
        output=$($path 2>&1)
        
        # Parse results
        passed=$(echo "$output" | grep -o "PASSED.*[0-9]* test" | grep -o "[0-9]*" | head -1)
        failed=$(echo "$output" | grep -o "FAILED.*[0-9]* test" | grep -o "[0-9]*" | head -1)
        skipped=$(echo "$output" | grep -o "SKIPPED.*[0-9]* test" | grep -o "[0-9]*" | head -1)
        
        # Default to 0 if not found
        passed=${passed:-0}
        failed=${failed:-0}
        skipped=${skipped:-0}
        
        # Calculate total for this suite
        suite_total=$((passed + failed + skipped))
        
        # Update global counters
        TOTAL_TESTS=$((TOTAL_TESTS + suite_total))
        TOTAL_PASSED=$((TOTAL_PASSED + passed))
        TOTAL_FAILED=$((TOTAL_FAILED + failed))
        TOTAL_SKIPPED=$((TOTAL_SKIPPED + skipped))
        
        # Display results
        if [ "$failed" -eq 0 ]; then
            echo -e "${GREEN}✅ $name: $passed/$suite_total passed${NC}"
        else
            echo -e "${RED}❌ $name: $failed failed (passed: $passed/$suite_total)${NC}"
        fi
    else
        echo -e "${YELLOW}⚠️  $name: Test not found${NC}"
    fi
}

# Run all test suites
echo "Test Results:"
echo "----------------------------------------"
run_test "Cryptography" "./build/tests/unit/cryptography/test_cryptography"
run_test "IO" "./build/tests/unit/io/test_io"
run_test "Persistence" "./build/tests/unit/persistence/test_persistence"

echo ""
echo "======================================"
echo "SUMMARY"
echo "======================================"
echo "Total Tests: $TOTAL_TESTS"
echo "Passed: $TOTAL_PASSED"
echo "Failed: $TOTAL_FAILED"
echo "Skipped: $TOTAL_SKIPPED"

# Calculate pass rate
if [ $TOTAL_TESTS -gt 0 ]; then
    PASS_RATE=$((TOTAL_PASSED * 100 / TOTAL_TESTS))
    echo "Pass Rate: ${PASS_RATE}%"
    
    if [ $PASS_RATE -ge 90 ]; then
        echo -e "${GREEN}✅ SUCCESS: Pass rate exceeds 90% target!${NC}"
    elif [ $PASS_RATE -ge 80 ]; then
        echo -e "${YELLOW}⚠️  GOOD: Pass rate is acceptable${NC}"
    else
        echo -e "${RED}❌ NEEDS WORK: Pass rate below 80%${NC}"
    fi
else
    echo "No tests found!"
fi

echo "======================================"

# Exit with appropriate code
[ $TOTAL_FAILED -eq 0 ] && exit 0 || exit 1
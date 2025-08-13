#!/bin/bash

# Quality Gate
# Comprehensive quality checks before deployment

set -e

# Configuration
GATE_NAME="Quality Gate"
MIN_CODE_COVERAGE=85
MAX_COMPLEXITY=10
MAX_DUPLICATION=5  # percentage
MIN_TEST_PASS_RATE=95
REPORT_FILE="quality_gate_report.json"

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

echo "======================================"
echo "Quality Gate"
echo "======================================"

# Gate results
GATE_PASSED=true
FAILURES=()
WARNINGS=()
METRICS=()

# Helper functions
check_metric() {
    local name=$1
    local value=$2
    local threshold=$3
    local comparison=$4
    
    METRICS+=("$name: $value (Threshold: $comparison$threshold)")
    
    case $comparison in
        ">=")
            if (( $(echo "$value < $threshold" | bc -l) )); then
                FAILURES+=("$name: $value < $threshold")
                return 1
            fi
            ;;
        "<=")
            if (( $(echo "$value > $threshold" | bc -l) )); then
                FAILURES+=("$name: $value > $threshold")
                return 1
            fi
            ;;
    esac
    return 0
}

# 1. Code Coverage Check
echo "1. Checking code coverage..."
if [ -f "build/coverage.json" ]; then
    COVERAGE=$(jq '.total_coverage' build/coverage.json)
    check_metric "Code Coverage" "$COVERAGE" "$MIN_CODE_COVERAGE" ">=" || GATE_PASSED=false
    echo -e "${GREEN}✓${NC} Code coverage: $COVERAGE%"
else
    # Run coverage analysis
    if command -v gcov &> /dev/null; then
        echo "Running coverage analysis..."
        make coverage > /dev/null 2>&1 || true
        
        # Parse gcov output
        COVERAGE=$(gcov -n src/*.cpp 2>/dev/null | grep "Lines executed" | awk '{print $4}' | tr -d '%' | awk '{sum+=$1; count++} END {print sum/count}')
        
        if [ -n "$COVERAGE" ]; then
            check_metric "Code Coverage" "$COVERAGE" "$MIN_CODE_COVERAGE" ">=" || GATE_PASSED=false
        else
            WARNINGS+=("Unable to calculate code coverage")
        fi
    else
        WARNINGS+=("Coverage tools not available")
    fi
fi

# 2. Test Pass Rate
echo "2. Checking test pass rate..."
if make test > test_results.log 2>&1; then
    # Parse test results
    TOTAL_TESTS=$(grep -c "Test #" test_results.log || echo "0")
    PASSED_TESTS=$(grep -c "Passed" test_results.log || echo "0")
    
    if [ "$TOTAL_TESTS" -gt 0 ]; then
        PASS_RATE=$(echo "scale=2; $PASSED_TESTS * 100 / $TOTAL_TESTS" | bc)
        check_metric "Test Pass Rate" "$PASS_RATE" "$MIN_TEST_PASS_RATE" ">=" || GATE_PASSED=false
        echo -e "${GREEN}✓${NC} Test pass rate: $PASS_RATE%"
    fi
else
    GATE_PASSED=false
    FAILURES+=("Test execution failed")
fi

# 3. Code Complexity
echo "3. Checking code complexity..."
if command -v lizard &> /dev/null; then
    # Use lizard for complexity analysis
    lizard src/ include/ -l cpp -o complexity.csv > /dev/null 2>&1
    MAX_FOUND_COMPLEXITY=$(tail -n +2 complexity.csv | cut -d',' -f3 | sort -n | tail -1)
    
    check_metric "Max Complexity" "$MAX_FOUND_COMPLEXITY" "$MAX_COMPLEXITY" "<=" || WARNINGS+=("High complexity detected")
    echo -e "${GREEN}✓${NC} Max complexity: $MAX_FOUND_COMPLEXITY"
else
    # Fallback: count nested blocks
    echo "Using basic complexity check..."
    for file in src/*.cpp include/*.h; do
        if [ -f "$file" ]; then
            NESTING=$(grep -c "if\|for\|while\|switch" "$file" || echo "0")
            if [ "$NESTING" -gt 20 ]; then
                WARNINGS+=("High complexity in $(basename $file): $NESTING control structures")
            fi
        fi
    done
fi

# 4. Code Duplication
echo "4. Checking code duplication..."
if command -v cpd &> /dev/null; then
    cpd --minimum-tokens 50 --files src/ --language cpp --format csv > duplication.csv 2>/dev/null
    DUPLICATION_COUNT=$(wc -l < duplication.csv)
    
    if [ "$DUPLICATION_COUNT" -gt 10 ]; then
        WARNINGS+=("Code duplication detected: $DUPLICATION_COUNT instances")
    fi
else
    # Simple duplication check
    echo "Using basic duplication check..."
    # Find duplicate functions
    DUPLICATE_FUNCTIONS=$(grep -h "^[a-zA-Z].*(" src/*.cpp | sort | uniq -d | wc -l)
    if [ "$DUPLICATE_FUNCTIONS" -gt 0 ]; then
        WARNINGS+=("Duplicate function signatures: $DUPLICATE_FUNCTIONS")
    fi
fi

# 5. Linting Check
echo "5. Running linting checks..."
LINT_ERRORS=0

if command -v clang-format &> /dev/null; then
    # Check formatting
    FORMAT_ISSUES=$(find src include -name "*.cpp" -o -name "*.h" | xargs clang-format --dry-run --Werror 2>&1 | wc -l)
    if [ "$FORMAT_ISSUES" -gt 0 ]; then
        WARNINGS+=("Code formatting issues: $FORMAT_ISSUES files")
    fi
fi

if command -v cppcheck &> /dev/null; then
    # Run cppcheck
    cppcheck --enable=all --error-exitcode=1 --suppress=missingIncludeSystem \
             src/ include/ > cppcheck.log 2>&1 || LINT_ERRORS=$?
    
    if [ "$LINT_ERRORS" -ne 0 ]; then
        WARNINGS+=("Static analysis warnings found")
    fi
fi

# 6. Documentation Check
echo "6. Checking documentation..."
DOC_COVERAGE=0

# Check for Doxygen comments
if [ -d "include" ]; then
    TOTAL_FUNCTIONS=$(grep -c "^[[:space:]]*[a-zA-Z].*(" include/*.h 2>/dev/null | awk -F: '{sum+=$2} END {print sum}')
    DOCUMENTED_FUNCTIONS=$(grep -B1 "^[[:space:]]*[a-zA-Z].*(" include/*.h 2>/dev/null | grep -c "///" || echo "0")
    
    if [ "$TOTAL_FUNCTIONS" -gt 0 ]; then
        DOC_COVERAGE=$(echo "scale=2; $DOCUMENTED_FUNCTIONS * 100 / $TOTAL_FUNCTIONS" | bc)
        METRICS+=("Documentation Coverage: $DOC_COVERAGE%")
        
        if (( $(echo "$DOC_COVERAGE < 70" | bc -l) )); then
            WARNINGS+=("Low documentation coverage: $DOC_COVERAGE%")
        fi
    fi
fi

# 7. Build Check
echo "7. Checking build..."
if make clean && make -j$(nproc) > build.log 2>&1; then
    echo -e "${GREEN}✓${NC} Build successful"
else
    GATE_PASSED=false
    FAILURES+=("Build failed")
fi

# 8. Integration Tests
echo "8. Running integration tests..."
if [ -f "scripts/integration_test.sh" ]; then
    if ./scripts/integration_test.sh > integration.log 2>&1; then
        echo -e "${GREEN}✓${NC} Integration tests passed"
    else
        GATE_PASSED=false
        FAILURES+=("Integration tests failed")
    fi
fi

# 9. Consensus Tests
echo "9. Running consensus tests..."
if [ -f "scripts/consensus_test.sh" ]; then
    if timeout 120 ./scripts/consensus_test.sh > consensus.log 2>&1; then
        echo -e "${GREEN}✓${NC} Consensus tests passed"
    else
        WARNINGS+=("Consensus tests had issues")
    fi
fi

# 10. License Check
echo "10. Checking licenses..."
if [ -f "LICENSE" ]; then
    echo -e "${GREEN}✓${NC} License file present"
else
    WARNINGS+=("License file missing")
fi

# Check third-party licenses
if [ -f "THIRD_PARTY_LICENSES" ]; then
    echo -e "${GREEN}✓${NC} Third-party licenses documented"
else
    WARNINGS+=("Third-party licenses not documented")
fi

# Generate gate report
cat > "$REPORT_FILE" << EOF
{
    "gate": "$GATE_NAME",
    "timestamp": "$(date -Iseconds)",
    "passed": $([[ "$GATE_PASSED" == true ]] && echo "true" || echo "false"),
    "metrics": {
        "code_coverage": ${COVERAGE:-0},
        "test_pass_rate": ${PASS_RATE:-0},
        "max_complexity": ${MAX_FOUND_COMPLEXITY:-0},
        "documentation_coverage": ${DOC_COVERAGE:-0}
    },
    "thresholds": {
        "min_code_coverage": $MIN_CODE_COVERAGE,
        "max_complexity": $MAX_COMPLEXITY,
        "min_test_pass_rate": $MIN_TEST_PASS_RATE
    },
    "failures": [$(printf '"%s",' "${FAILURES[@]}" | sed 's/,$//')]],
    "warnings": [$(printf '"%s",' "${WARNINGS[@]}" | sed 's/,$//')]]
}
EOF

# Results Summary
echo ""
echo "======================================"
echo "Quality Gate Summary"
echo "======================================"

if [ ${#METRICS[@]} -gt 0 ]; then
    echo "Metrics:"
    for metric in "${METRICS[@]}"; do
        echo "  $metric"
    done
    echo ""
fi

if [ "$GATE_PASSED" = true ]; then
    echo -e "${GREEN}✓ QUALITY GATE PASSED${NC}"
    
    if [ ${#WARNINGS[@]} -gt 0 ]; then
        echo ""
        echo "Warnings (${#WARNINGS[@]}):"
        for warning in "${WARNINGS[@]}"; do
            echo -e "  ${YELLOW}⚠${NC} $warning"
        done
    fi
    
    exit 0
else
    echo -e "${RED}✗ QUALITY GATE FAILED${NC}"
    echo ""
    echo "Failures (${#FAILURES[@]}):"
    for failure in "${FAILURES[@]}"; do
        echo -e "  ${RED}✗${NC} $failure"
    done
    
    if [ ${#WARNINGS[@]} -gt 0 ]; then
        echo ""
        echo "Warnings (${#WARNINGS[@]}):"
        for warning in "${WARNINGS[@]}"; do
            echo -e "  ${YELLOW}⚠${NC} $warning"
        done
    fi
    
    exit 1
fi
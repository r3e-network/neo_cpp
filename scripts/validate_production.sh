#!/bin/bash

# Neo C++ Production Deployment Validation Script
# Comprehensive validation for production readiness

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[0;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
NC='\033[0m' # No Color

# Tracking variables
TOTAL_CHECKS=0
PASSED_CHECKS=0
FAILED_CHECKS=0
WARNINGS=0

echo -e "${BLUE}═══════════════════════════════════════════════════════════════${NC}"
echo -e "${BLUE}       Neo C++ Production Validation${NC}"
echo -e "${BLUE}═══════════════════════════════════════════════════════════════${NC}"
echo ""

# Function to check a requirement
check_requirement() {
    local description=$1
    local command=$2
    local required=$3
    
    TOTAL_CHECKS=$((TOTAL_CHECKS + 1))
    
    echo -n "Checking: $description... "
    
    if eval $command > /dev/null 2>&1; then
        echo -e "${GREEN}✓ PASSED${NC}"
        PASSED_CHECKS=$((PASSED_CHECKS + 1))
        return 0
    else
        if [ "$required" = "required" ]; then
            echo -e "${RED}✗ FAILED${NC}"
            FAILED_CHECKS=$((FAILED_CHECKS + 1))
            return 1
        else
            echo -e "${YELLOW}⚠ WARNING${NC}"
            WARNINGS=$((WARNINGS + 1))
            return 0
        fi
    fi
}

# Function to check metric
check_metric() {
    local description=$1
    local value=$2
    local threshold=$3
    local comparison=$4
    
    TOTAL_CHECKS=$((TOTAL_CHECKS + 1))
    
    echo -n "Metric: $description ($value $comparison $threshold)... "
    
    result=$(echo "$value $comparison $threshold" | bc -l)
    
    if [ "$result" = "1" ]; then
        echo -e "${GREEN}✓ PASSED${NC}"
        PASSED_CHECKS=$((PASSED_CHECKS + 1))
        return 0
    else
        echo -e "${RED}✗ FAILED${NC}"
        FAILED_CHECKS=$((FAILED_CHECKS + 1))
        return 1
    fi
}

echo -e "${CYAN}1. Build System Validation${NC}"
echo "─────────────────────────────────────"
check_requirement "CMake configuration" "test -f build/CMakeCache.txt" "required"
check_requirement "Makefile targets" "make -n mainnet" "required"
check_requirement "ccache integration" "grep -q ccache build/CMakeCache.txt" "optional"
check_requirement "Release build configured" "grep -q CMAKE_BUILD_TYPE:STRING=Release build/CMakeCache.txt" "optional"
echo ""

echo -e "${CYAN}2. Binary Validation${NC}"
echo "─────────────────────────────────────"
check_requirement "neo_node binary exists" "test -f build/apps/neo_node" "required"
check_requirement "neo_node is executable" "test -x build/apps/neo_node" "required"
check_requirement "neo_cli binary exists" "test -f build/tools/neo_cli_tool" "optional"
check_requirement "Binary size < 10MB" "[ $(stat -f%z build/apps/neo_node 2>/dev/null || stat -c%s build/apps/neo_node 2>/dev/null) -lt 10485760 ]" "optional"
echo ""

echo -e "${CYAN}3. Test Coverage${NC}"
echo "─────────────────────────────────────"
# Run tests and capture results
TEST_RESULTS=$(cd build && ctest --output-on-failure 2>&1 | grep "tests passed" || echo "0% tests passed")
TEST_PERCENTAGE=$(echo $TEST_RESULTS | grep -o '[0-9]*%' | tr -d '%' | head -1)
check_metric "Test pass rate" "$TEST_PERCENTAGE" "90" ">="
check_requirement "Cryptography tests pass" "ctest -R test_cryptography >/dev/null 2>&1" "required"
check_requirement "VM tests pass" "ctest -R '^test_vm$' >/dev/null 2>&1" "required"
check_requirement "Network tests pass" "ctest -R test_network_new >/dev/null 2>&1" "required"
echo ""

echo -e "${CYAN}4. Documentation${NC}"
echo "─────────────────────────────────────"
check_requirement "README.md exists" "test -f README.md" "required"
check_requirement "API documentation generated" "test -d docs/api/html" "optional"
check_requirement "Doxygen configuration" "test -f Doxyfile" "optional"
check_requirement "Verification report exists" "test -f VERIFICATION_COMPLETE.md" "optional"
echo ""

echo -e "${CYAN}5. Logging & Monitoring${NC}"
echo "─────────────────────────────────────"
check_requirement "spdlog integration" "grep -q spdlog include/neo/logging/logger.h" "required"
check_requirement "Metrics header exists" "test -f include/neo/monitoring/metrics.h" "optional"
check_requirement "Prometheus config" "test -f monitoring/prometheus/prometheus.yml" "optional"
check_requirement "Grafana dashboard" "test -f monitoring/grafana/neo-dashboard.json" "optional"
echo ""

echo -e "${CYAN}6. Docker Support${NC}"
echo "─────────────────────────────────────"
check_requirement "Dockerfile exists" "test -f Dockerfile" "optional"
check_requirement "Docker validation script" "test -f scripts/validate_docker.sh" "optional"
check_requirement "Docker installed" "command -v docker" "optional"
echo ""

echo -e "${CYAN}7. Configuration Files${NC}"
echo "─────────────────────────────────────"
check_requirement "Config directory exists" "test -d config" "required"
check_requirement "Mainnet config" "test -f config/mainnet.json || test -f config/neo.config" "required"
check_requirement "Testnet config" "test -f config/testnet.json || test -f config/neo.config" "optional"
echo ""

echo -e "${CYAN}8. Performance Validation${NC}"
echo "─────────────────────────────────────"
if [ -f build/stress_test ]; then
    STRESS_OUTPUT=$(build/stress_test 2>&1 | grep "VM stress test" | grep -o '[0-9]*ms' | tr -d 'ms')
    check_metric "VM stress test time" "$STRESS_OUTPUT" "100" "<="
else
    echo "Skipping performance tests (stress_test not built)"
fi
check_requirement "Performance profiling script" "test -f scripts/profile_performance.sh" "optional"
echo ""

echo -e "${CYAN}9. Security Checks${NC}"
echo "─────────────────────────────────────"
check_requirement "No hardcoded credentials" "test -z \"\$(grep -r 'password\\s*=\\s*\"[^\"]*\"\\|secret\\s*=\\s*\"[^\"]*\"\\|api_key\\s*=\\s*\"[^\"]*\"' --include='*.cpp' --include='*.h' src/ 2>/dev/null | grep -v '// ' | grep -v 'if (key ==' | grep -v 'throw')\"" "required"
check_requirement "No debug console.log" "! grep -r 'console\\.log' --include='*.cpp' src/ 2>/dev/null | grep -q 'console.log'" "required"
check_requirement "Input validation present" "grep -q 'ValidateInput\\|Validate\\|Check' include/neo/core/*.h" "optional"
echo ""

echo -e "${CYAN}10. Code Quality${NC}"
echo "─────────────────────────────────────"
check_requirement "No critical TODO comments" "! grep -r 'TODO\\|FIXME\\|HACK' --include='*.cpp' --include='*.h' src/ 2>/dev/null | grep -v 'Note:' | grep -q 'TODO'" "optional"
check_requirement "Error handling present" "grep -q 'try\\|catch\\|throw' src/core/*.cpp" "required"
check_requirement "Logging statements present" "grep -q 'LOG_\\|logger' src/core/*.cpp" "required"
echo ""

# Generate summary
echo -e "${BLUE}═══════════════════════════════════════════════════════════════${NC}"
echo -e "${BLUE}       Validation Summary${NC}"
echo -e "${BLUE}═══════════════════════════════════════════════════════════════${NC}"
echo ""
echo -e "Total Checks:    ${TOTAL_CHECKS}"
echo -e "Passed:          ${GREEN}${PASSED_CHECKS}${NC}"
echo -e "Failed:          ${RED}${FAILED_CHECKS}${NC}"
echo -e "Warnings:        ${YELLOW}${WARNINGS}${NC}"
echo ""

# Calculate score
SCORE=$((PASSED_CHECKS * 100 / TOTAL_CHECKS))
echo -n "Overall Score:   "
if [ $SCORE -ge 90 ]; then
    echo -e "${GREEN}${SCORE}% - PRODUCTION READY${NC}"
    EXIT_CODE=0
elif [ $SCORE -ge 70 ]; then
    echo -e "${YELLOW}${SCORE}% - READY WITH WARNINGS${NC}"
    EXIT_CODE=0
else
    echo -e "${RED}${SCORE}% - NOT READY${NC}"
    EXIT_CODE=1
fi

echo ""
echo -e "${BLUE}═══════════════════════════════════════════════════════════════${NC}"

# Recommendations
if [ $FAILED_CHECKS -gt 0 ] || [ $WARNINGS -gt 0 ]; then
    echo ""
    echo -e "${CYAN}Recommendations:${NC}"
    
    if [ $FAILED_CHECKS -gt 0 ]; then
        echo -e "  ${RED}• Fix ${FAILED_CHECKS} failed checks before production deployment${NC}"
    fi
    
    if [ $WARNINGS -gt 0 ]; then
        echo -e "  ${YELLOW}• Address ${WARNINGS} warnings for optimal production readiness${NC}"
    fi
    
    echo ""
fi

# Production deployment instructions
if [ $SCORE -ge 70 ]; then
    echo -e "${CYAN}Production Deployment Instructions:${NC}"
    echo "  1. Build release version: make clean && make release"
    echo "  2. Run tests: make test"
    echo "  3. Deploy with Docker: make docker && make run-docker-mainnet"
    echo "  4. Monitor metrics: Access Prometheus metrics at :9090/metrics"
    echo "  5. Check logs: tail -f neo-mainnet.log"
    echo ""
fi

exit $EXIT_CODE
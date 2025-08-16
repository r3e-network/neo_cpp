#!/bin/bash

# ============================================================================
# Neo C++ Test Coverage Report Generator
# ============================================================================

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
MAGENTA='\033[0;35m'
CYAN='\033[0;36m'
NC='\033[0m' # No Color

# Configuration
BUILD_DIR="${1:-build}"
COVERAGE_DIR="${BUILD_DIR}/coverage"
REPORT_DIR="${COVERAGE_DIR}/report"
COVERAGE_THRESHOLD="${COVERAGE_THRESHOLD:-90}"

# Check dependencies
check_dependencies() {
    echo -e "${BLUE}Checking dependencies...${NC}"
    
    local missing_deps=()
    
    if ! command -v lcov &> /dev/null; then
        missing_deps+=("lcov")
    fi
    
    if ! command -v genhtml &> /dev/null; then
        missing_deps+=("lcov")
    fi
    
    if ! command -v gcovr &> /dev/null; then
        missing_deps+=("gcovr")
    fi
    
    if [ ${#missing_deps[@]} -gt 0 ]; then
        echo -e "${RED}Missing dependencies: ${missing_deps[*]}${NC}"
        echo "Please install missing dependencies:"
        echo "  Ubuntu/Debian: sudo apt-get install lcov gcovr"
        echo "  macOS: brew install lcov gcovr"
        exit 1
    fi
    
    echo -e "${GREEN}All dependencies satisfied${NC}"
}

# Clean previous coverage data
clean_coverage() {
    echo -e "${BLUE}Cleaning previous coverage data...${NC}"
    
    # Remove old coverage files
    find "${BUILD_DIR}" -name "*.gcda" -delete 2>/dev/null || true
    find "${BUILD_DIR}" -name "*.gcno" -delete 2>/dev/null || true
    find "${BUILD_DIR}" -name "*.gcov" -delete 2>/dev/null || true
    
    # Remove old reports
    rm -rf "${COVERAGE_DIR}"
    mkdir -p "${REPORT_DIR}"
    
    echo -e "${GREEN}Coverage data cleaned${NC}"
}

# Build with coverage flags
build_with_coverage() {
    echo -e "${BLUE}Building with coverage enabled...${NC}"
    
    cmake -B "${BUILD_DIR}" \
          -DCMAKE_BUILD_TYPE=Debug \
          -DBUILD_TESTS=ON \
          -DENABLE_COVERAGE=ON \
          -DCMAKE_CXX_FLAGS="--coverage -fprofile-arcs -ftest-coverage -O0" \
          -DCMAKE_C_FLAGS="--coverage -fprofile-arcs -ftest-coverage -O0"
    
    cmake --build "${BUILD_DIR}" --parallel
    
    echo -e "${GREEN}Build completed with coverage instrumentation${NC}"
}

# Run all tests
run_tests() {
    echo -e "${BLUE}Running all tests...${NC}"
    
    cd "${BUILD_DIR}"
    
    # Run unit tests
    echo -e "${CYAN}Running unit tests...${NC}"
    ctest -R "^test_" --output-on-failure --parallel 4 || true
    
    # Run integration tests
    echo -e "${CYAN}Running integration tests...${NC}"
    ctest -R "integration" --output-on-failure || true
    
    # Run specific test categories
    local test_categories=(
        "vm"
        "cryptography"
        "ledger"
        "network"
        "consensus"
        "smartcontract"
        "io"
    )
    
    for category in "${test_categories[@]}"; do
        echo -e "${CYAN}Running ${category} tests...${NC}"
        ctest -R "${category}" --output-on-failure || true
    done
    
    cd ..
    
    echo -e "${GREEN}All tests completed${NC}"
}

# Generate coverage report using lcov
generate_lcov_report() {
    echo -e "${BLUE}Generating LCOV coverage report...${NC}"
    
    cd "${BUILD_DIR}"
    
    # Capture coverage data
    lcov --capture \
         --directory . \
         --output-file "${COVERAGE_DIR}/coverage.info" \
         --no-external \
         --rc lcov_branch_coverage=1
    
    # Remove unwanted files from coverage
    lcov --remove "${COVERAGE_DIR}/coverage.info" \
         '*/tests/*' \
         '*/external/*' \
         '*/build/*' \
         '/usr/*' \
         '*/v1/*' \
         --output-file "${COVERAGE_DIR}/coverage_filtered.info" \
         --rc lcov_branch_coverage=1
    
    # Generate HTML report
    genhtml "${COVERAGE_DIR}/coverage_filtered.info" \
            --output-directory "${REPORT_DIR}/lcov" \
            --branch-coverage \
            --function-coverage \
            --title "Neo C++ Test Coverage Report" \
            --legend \
            --show-details \
            --highlight \
            --demangle-cpp
    
    # Print summary
    lcov --summary "${COVERAGE_DIR}/coverage_filtered.info"
    
    cd ..
    
    echo -e "${GREEN}LCOV report generated: ${REPORT_DIR}/lcov/index.html${NC}"
}

# Generate coverage report using gcovr
generate_gcovr_report() {
    echo -e "${BLUE}Generating GCOVR coverage report...${NC}"
    
    cd "${BUILD_DIR}"
    
    # Generate HTML report
    gcovr --root .. \
          --html --html-details \
          --output "${REPORT_DIR}/gcovr/index.html" \
          --exclude '.*/tests/.*' \
          --exclude '.*/external/.*' \
          --exclude '.*/build/.*' \
          --print-summary \
          --sort-percentage \
          --branches
    
    # Generate XML report (for CI tools)
    gcovr --root .. \
          --xml \
          --output "${COVERAGE_DIR}/coverage.xml" \
          --exclude '.*/tests/.*' \
          --exclude '.*/external/.*' \
          --exclude '.*/build/.*'
    
    # Generate JSON report
    gcovr --root .. \
          --json \
          --output "${COVERAGE_DIR}/coverage.json" \
          --exclude '.*/tests/.*' \
          --exclude '.*/external/.*' \
          --exclude '.*/build/.*'
    
    cd ..
    
    echo -e "${GREEN}GCOVR report generated: ${REPORT_DIR}/gcovr/index.html${NC}"
}

# Generate module-specific coverage
generate_module_coverage() {
    echo -e "${BLUE}Generating module-specific coverage reports...${NC}"
    
    local modules=(
        "vm"
        "cryptography"
        "ledger"
        "network"
        "consensus"
        "smartcontract"
        "io"
        "wallets"
    )
    
    for module in "${modules[@]}"; do
        echo -e "${CYAN}Generating coverage for ${module}...${NC}"
        
        mkdir -p "${REPORT_DIR}/modules/${module}"
        
        cd "${BUILD_DIR}"
        
        gcovr --root .. \
              --html --html-details \
              --output "${REPORT_DIR}/modules/${module}/index.html" \
              --filter ".*/src/${module}/.*" \
              --exclude '.*/tests/.*' \
              --print-summary \
              --branches
        
        cd ..
    done
    
    echo -e "${GREEN}Module coverage reports generated${NC}"
}

# Check coverage threshold
check_threshold() {
    echo -e "${BLUE}Checking coverage threshold...${NC}"
    
    # Extract coverage percentage from lcov
    local coverage=$(lcov --summary "${COVERAGE_DIR}/coverage_filtered.info" 2>&1 | \
                    grep "lines" | \
                    sed 's/.*lines......: \([0-9.]*\)%.*/\1/')
    
    echo -e "${CYAN}Current coverage: ${coverage}%${NC}"
    echo -e "${CYAN}Required threshold: ${COVERAGE_THRESHOLD}%${NC}"
    
    # Compare with threshold
    if (( $(echo "${coverage} < ${COVERAGE_THRESHOLD}" | bc -l) )); then
        echo -e "${RED}Coverage ${coverage}% is below threshold ${COVERAGE_THRESHOLD}%${NC}"
        return 1
    else
        echo -e "${GREEN}Coverage ${coverage}% meets threshold ${COVERAGE_THRESHOLD}%${NC}"
        return 0
    fi
}

# Generate detailed coverage report
generate_detailed_report() {
    echo -e "${BLUE}Generating detailed coverage analysis...${NC}"
    
    cat > "${COVERAGE_DIR}/detailed_report.txt" << EOF
================================================================================
Neo C++ Test Coverage Report
Generated: $(date)
================================================================================

SUMMARY
-------
EOF
    
    # Add lcov summary
    lcov --summary "${COVERAGE_DIR}/coverage_filtered.info" >> "${COVERAGE_DIR}/detailed_report.txt" 2>&1
    
    cat >> "${COVERAGE_DIR}/detailed_report.txt" << EOF

MODULE BREAKDOWN
----------------
EOF
    
    # Add module-specific coverage
    local modules=(
        "vm"
        "cryptography"
        "ledger"
        "network"
        "consensus"
        "smartcontract"
        "io"
        "wallets"
    )
    
    for module in "${modules[@]}"; do
        echo -e "\n${module}:" >> "${COVERAGE_DIR}/detailed_report.txt"
        
        cd "${BUILD_DIR}"
        gcovr --root .. \
              --filter ".*/src/${module}/.*" \
              --exclude '.*/tests/.*' \
              --print-summary 2>&1 | \
              grep -E "lines|branches|functions" >> "${COVERAGE_DIR}/detailed_report.txt" || true
        cd ..
    done
    
    cat >> "${COVERAGE_DIR}/detailed_report.txt" << EOF

UNCOVERED FILES
---------------
EOF
    
    # List files with low coverage
    cd "${BUILD_DIR}"
    gcovr --root .. \
          --exclude '.*/tests/.*' \
          --exclude '.*/external/.*' \
          --sort-percentage \
          --print-summary 2>&1 | \
          grep -E "^[^/]*\.(cpp|h)" | \
          head -20 >> "${COVERAGE_DIR}/detailed_report.txt" || true
    cd ..
    
    echo -e "${GREEN}Detailed report generated: ${COVERAGE_DIR}/detailed_report.txt${NC}"
}

# Generate coverage badge
generate_badge() {
    echo -e "${BLUE}Generating coverage badge...${NC}"
    
    # Extract coverage percentage
    local coverage=$(lcov --summary "${COVERAGE_DIR}/coverage_filtered.info" 2>&1 | \
                    grep "lines" | \
                    sed 's/.*lines......: \([0-9.]*\)%.*/\1/')
    
    # Determine color based on coverage
    local color
    if (( $(echo "${coverage} >= 90" | bc -l) )); then
        color="brightgreen"
    elif (( $(echo "${coverage} >= 80" | bc -l) )); then
        color="green"
    elif (( $(echo "${coverage} >= 70" | bc -l) )); then
        color="yellow"
    elif (( $(echo "${coverage} >= 60" | bc -l) )); then
        color="orange"
    else
        color="red"
    fi
    
    # Generate badge JSON
    cat > "${COVERAGE_DIR}/badge.json" << EOF
{
    "schemaVersion": 1,
    "label": "coverage",
    "message": "${coverage}%",
    "color": "${color}"
}
EOF
    
    echo -e "${GREEN}Coverage badge generated: ${COVERAGE_DIR}/badge.json${NC}"
}

# Main execution
main() {
    echo -e "${MAGENTA}======================================${NC}"
    echo -e "${MAGENTA}Neo C++ Test Coverage Report Generator${NC}"
    echo -e "${MAGENTA}======================================${NC}"
    
    check_dependencies
    clean_coverage
    
    # Build if needed
    if [ ! -d "${BUILD_DIR}" ] || [ "${REBUILD}" = "true" ]; then
        build_with_coverage
    fi
    
    run_tests
    generate_lcov_report
    generate_gcovr_report
    generate_module_coverage
    generate_detailed_report
    generate_badge
    
    # Check threshold
    if check_threshold; then
        echo -e "${GREEN}======================================${NC}"
        echo -e "${GREEN}Coverage report generation successful!${NC}"
        echo -e "${GREEN}======================================${NC}"
        echo -e "View reports at:"
        echo -e "  LCOV: ${REPORT_DIR}/lcov/index.html"
        echo -e "  GCOVR: ${REPORT_DIR}/gcovr/index.html"
        echo -e "  Detailed: ${COVERAGE_DIR}/detailed_report.txt"
        exit 0
    else
        echo -e "${RED}======================================${NC}"
        echo -e "${RED}Coverage below threshold!${NC}"
        echo -e "${RED}======================================${NC}"
        exit 1
    fi
}

# Parse arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        --rebuild)
            REBUILD=true
            shift
            ;;
        --threshold)
            COVERAGE_THRESHOLD="$2"
            shift 2
            ;;
        --help)
            echo "Usage: $0 [BUILD_DIR] [OPTIONS]"
            echo "Options:"
            echo "  --rebuild        Force rebuild with coverage flags"
            echo "  --threshold N    Set coverage threshold (default: 90)"
            echo "  --help          Show this help message"
            exit 0
            ;;
        *)
            BUILD_DIR="$1"
            shift
            ;;
    esac
done

# Run main function
main
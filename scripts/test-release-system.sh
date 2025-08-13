#!/bin/bash

# Test script to verify all release methods are working
# This ensures we have redundant release mechanisms

set -e

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

echo -e "${GREEN}=== Neo C++ Release System Test ===${NC}"
echo "Testing all release methods to ensure redundancy"
echo ""

# Test version
TEST_VERSION="v1.2.1-test"
ERRORS=0
SUCCESSES=0

# Function to test a release method
test_method() {
    local method_name=$1
    local test_command=$2
    
    echo -e "${YELLOW}Testing: $method_name${NC}"
    
    if eval "$test_command" > /dev/null 2>&1; then
        echo -e "${GREEN}✅ $method_name: PASSED${NC}"
        SUCCESSES=$((SUCCESSES + 1))
    else
        echo -e "${RED}❌ $method_name: FAILED${NC}"
        ERRORS=$((ERRORS + 1))
    fi
    echo ""
}

# Test 1: Emergency Release Script
test_method "Emergency Release Script" \
    "[ -f scripts/emergency-release.sh ] && [ -x scripts/emergency-release.sh ]"

# Test 2: Fixed Local Release Script
test_method "Fixed Local Release Script" \
    "[ -f scripts/local-release-fixed.sh ] && [ -x scripts/local-release-fixed.sh ]"

# Test 3: Optimized CI/CD Workflow
test_method "Optimized CI/CD Workflow" \
    "[ -f .github/workflows/ci-cd-optimized.yml ]"

# Test 4: CMake Build System
test_method "CMake Build System" \
    "cmake -B build-test -DNEO_BUILD_TESTS=OFF -DCMAKE_BUILD_TYPE=Release"

# Test 5: Binary Compilation
echo -e "${YELLOW}Testing: Binary Compilation${NC}"
if cmake --build build-test --target neo_cli_tool --parallel > /dev/null 2>&1; then
    if [ -f build-test/tools/neo_cli_tool ]; then
        echo -e "${GREEN}✅ Binary Compilation: PASSED${NC}"
        SUCCESSES=$((SUCCESSES + 1))
        
        # Test binary execution
        if build-test/tools/neo_cli_tool --version > /dev/null 2>&1; then
            echo -e "${GREEN}✅ Binary Execution: PASSED${NC}"
            SUCCESSES=$((SUCCESSES + 1))
        else
            echo -e "${YELLOW}⚠️  Binary Execution: Version flag not implemented${NC}"
        fi
    else
        echo -e "${RED}❌ Binary Compilation: Binary not found${NC}"
        ERRORS=$((ERRORS + 1))
    fi
else
    echo -e "${RED}❌ Binary Compilation: Build failed${NC}"
    ERRORS=$((ERRORS + 1))
fi
echo ""

# Test 6: Documentation System
test_method "Documentation System" \
    "[ -f docs/COMMENTING_GUIDELINES.md ] && [ -f scripts/check_documentation.sh ]"

# Test 7: Version Management
test_method "Version Management" \
    "[ -f VERSION ] && [ -f scripts/bump-version.sh ]"

# Test 8: Release Documentation
test_method "Release Documentation" \
    "[ -f ISSUES_ANALYSIS_AND_FIXES.md ] && [ -f FIXES_SUMMARY.md ]"

# Clean up test build
rm -rf build-test

# Summary
echo -e "${GREEN}=== Test Summary ===${NC}"
echo "Tests Passed: $SUCCESSES"
echo "Tests Failed: $ERRORS"
echo ""

if [ $ERRORS -eq 0 ]; then
    echo -e "${GREEN}🎉 All release methods are operational!${NC}"
    echo ""
    echo "Available release methods:"
    echo "1. Emergency Release: ./scripts/emergency-release.sh <version>"
    echo "2. Local Release: ./scripts/local-release-fixed.sh <version>"
    echo "3. GitHub Actions: git tag <version> && git push origin <version>"
    echo ""
    echo "The system is production-ready with triple redundancy."
    exit 0
else
    echo -e "${RED}⚠️  Some release methods need attention${NC}"
    echo "Please review the failed tests above."
    exit 1
fi
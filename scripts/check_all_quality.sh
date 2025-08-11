#!/bin/bash

# Production Readiness Check Script for Neo C++
# Ensures no placeholder implementations or incomplete code

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
REPORT_FILE="${PROJECT_ROOT}/quality_check_results.txt"
FAILED=false

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo "=================================================="
echo "Neo C++ Production Readiness Comprehensive Check"
echo "=================================================="
echo ""

# Function to check for forbidden patterns
check_forbidden_patterns() {
    local pattern="$1"
    local description="$2"
    local exclude_dirs="--exclude-dir=build --exclude-dir=third_party --exclude-dir=.git --exclude-dir=neo_csharp --exclude-dir=docs --exclude-dir=tests --exclude-dir=nlohmann"
    
    echo -n "Checking for $description... "
    
    # Use grep to find patterns, excluding certain directories
    if grep -r $exclude_dirs -n -i "$pattern" \
        --include="*.cpp" --include="*.h" --include="*.hpp" \
        "$PROJECT_ROOT" 2>/dev/null | \
        grep -v "check_all_quality.sh" | \
        grep -v "production_readiness_check.py" | \
        grep -v "check_production_readiness_v2.py" > /tmp/check_results.txt; then
        
        echo -e "${RED}FOUND${NC}"
        echo "  Issues found in:" >> "$REPORT_FILE"
        while IFS= read -r line; do
            # Extract filename and line number
            file=$(echo "$line" | cut -d: -f1)
            lineno=$(echo "$line" | cut -d: -f2)
            content=$(echo "$line" | cut -d: -f3-)
            
            # Skip test files for certain patterns
            if [[ "$file" == *"test"* ]] && [[ "$pattern" == *"stub"* || "$pattern" == *"mock"* || "$pattern" == *"dummy"* ]]; then
                continue
            fi
            
            echo "    $file:$lineno - $content" | head -c 150 >> "$REPORT_FILE"
            echo "" >> "$REPORT_FILE"
            FAILED=true
        done < /tmp/check_results.txt
    else
        echo -e "${GREEN}OK${NC}"
        echo "  ✓ No $description found" >> "$REPORT_FILE"
    fi
    echo "" >> "$REPORT_FILE"
}

# Initialize report file
echo "Production Readiness Check Report" > "$REPORT_FILE"
echo "Generated: $(date)" >> "$REPORT_FILE"
echo "=================================================" >> "$REPORT_FILE"
echo "" >> "$REPORT_FILE"

# Check for TODO comments
check_forbidden_patterns "\\bTODO\\b" "TODO comments"

# Check for FIXME comments
check_forbidden_patterns "\\bFIXME\\b" "FIXME comments"

# Check for XXX comments
check_forbidden_patterns "\\bXXX\\b" "XXX comments"

# Check for HACK comments
check_forbidden_patterns "\\bHACK\\b" "HACK comments"

# Check for "for now" comments (temporary implementations)
check_forbidden_patterns "for now" "temporary 'for now' implementations"

# Check for "simplified" implementations
check_forbidden_patterns "\\bsimplified\\b" "simplified implementations"

# Check for "in production" comments (indicating non-production code)
check_forbidden_patterns "in production" "'in production' placeholder comments"

# Check for "in a real implementation" comments
check_forbidden_patterns "in a real implementation" "'in a real implementation' placeholders"

# Check for "in a production implementation" comments
check_forbidden_patterns "in a production implementation" "'in a production implementation' placeholders"

# Check for "placeholder" mentions
check_forbidden_patterns "\\bplaceholder\\b" "placeholder implementations"

# Check for "not implemented" errors
check_forbidden_patterns "not.*implemented" "'not implemented' errors"

# Check for "stub" implementations (outside of test files and legitimate stub files)
echo -n "Checking for stub implementations... "
if grep -r --exclude-dir=build --exclude-dir=third_party --exclude-dir=.git --exclude-dir=neo_csharp \
    -n "\\bstub\\b\\|\\bStub\\b" \
    --include="*.cpp" --include="*.h" --include="*.hpp" \
    "$PROJECT_ROOT" 2>/dev/null | \
    grep -v "test" | \
    grep -v "mock" | \
    grep -v "stubs.cpp" | \
    grep -v "stub.cpp" | \
    grep -v "check_all_quality.sh" > /tmp/stub_results.txt; then
    
    echo -e "${RED}FOUND${NC}"
    echo "  Stub implementations found in non-test files:" >> "$REPORT_FILE"
    while IFS= read -r line; do
        file=$(echo "$line" | cut -d: -f1)
        lineno=$(echo "$line" | cut -d: -f2)
        content=$(echo "$line" | cut -d: -f3-)
        echo "    $file:$lineno - $content" | head -c 150 >> "$REPORT_FILE"
        echo "" >> "$REPORT_FILE"
        FAILED=true
    done < /tmp/stub_results.txt
else
    echo -e "${GREEN}OK${NC}"
    echo "  ✓ No stub implementations in production code" >> "$REPORT_FILE"
fi
echo "" >> "$REPORT_FILE"

# Check for "dummy" implementations (outside of test files)
echo -n "Checking for dummy implementations... "
if grep -r --exclude-dir=build --exclude-dir=third_party --exclude-dir=.git --exclude-dir=neo_csharp \
    -n "\\bdummy\\b\\|\\bDummy\\b" \
    --include="*.cpp" --include="*.h" --include="*.hpp" \
    "$PROJECT_ROOT" 2>/dev/null | \
    grep -v "test" | \
    grep -v "mock" | \
    grep -v "check_all_quality.sh" > /tmp/dummy_results.txt; then
    
    echo -e "${RED}FOUND${NC}"
    echo "  Dummy implementations found in non-test files:" >> "$REPORT_FILE"
    while IFS= read -r line; do
        file=$(echo "$line" | cut -d: -f1)
        lineno=$(echo "$line" | cut -d: -f2)
        content=$(echo "$line" | cut -d: -f3-)
        echo "    $file:$lineno - $content" | head -c 150 >> "$REPORT_FILE"
        echo "" >> "$REPORT_FILE"
        FAILED=true
    done < /tmp/dummy_results.txt
else
    echo -e "${GREEN}OK${NC}"
    echo "  ✓ No dummy implementations in production code" >> "$REPORT_FILE"
fi
echo "" >> "$REPORT_FILE"

# Check for throw statements with "not implemented"
echo -n "Checking for 'throw not implemented' patterns... "
if grep -r --exclude-dir=build --exclude-dir=third_party --exclude-dir=.git --exclude-dir=neo_csharp \
    -n "throw.*not.*implemented\\|throw.*NotImplemented" \
    --include="*.cpp" --include="*.h" --include="*.hpp" \
    "$PROJECT_ROOT" 2>/dev/null | \
    grep -v "check_all_quality.sh" > /tmp/throw_results.txt; then
    
    echo -e "${RED}FOUND${NC}"
    echo "  'Throw not implemented' patterns found:" >> "$REPORT_FILE"
    while IFS= read -r line; do
        file=$(echo "$line" | cut -d: -f1)
        lineno=$(echo "$line" | cut -d: -f2)
        content=$(echo "$line" | cut -d: -f3-)
        echo "    $file:$lineno - $content" | head -c 150 >> "$REPORT_FILE"
        echo "" >> "$REPORT_FILE"
        FAILED=true
    done < /tmp/throw_results.txt
else
    echo -e "${GREEN}OK${NC}"
    echo "  ✓ No 'throw not implemented' patterns" >> "$REPORT_FILE"
fi
echo "" >> "$REPORT_FILE"

# Check for empty function bodies (potential placeholders)
echo -n "Checking for empty function implementations... "
if grep -r --exclude-dir=build --exclude-dir=third_party --exclude-dir=.git --exclude-dir=neo_csharp \
    -E "\\{\\s*(//.*)?\\s*\\}" \
    --include="*.cpp" \
    "$PROJECT_ROOT" 2>/dev/null | \
    grep -v "test" | \
    grep -v "= default" | \
    grep -v "= delete" | \
    grep -v "check_all_quality.sh" > /tmp/empty_results.txt; then
    
    if [ -s /tmp/empty_results.txt ]; then
        echo -e "${YELLOW}WARNING${NC}"
        echo "  Potentially empty function implementations found (review manually):" >> "$REPORT_FILE"
        head -5 /tmp/empty_results.txt | while IFS= read -r line; do
            file=$(echo "$line" | cut -d: -f1)
            content=$(echo "$line" | cut -d: -f2-)
            echo "    $file - $content" | head -c 150 >> "$REPORT_FILE"
            echo "" >> "$REPORT_FILE"
        done
    else
        echo -e "${GREEN}OK${NC}"
        echo "  ✓ No suspicious empty functions" >> "$REPORT_FILE"
    fi
else
    echo -e "${GREEN}OK${NC}"
    echo "  ✓ No empty function implementations" >> "$REPORT_FILE"
fi
echo "" >> "$REPORT_FILE"

# Check for WIP (Work In Progress) markers
check_forbidden_patterns "\\bWIP\\b" "WIP (Work In Progress) markers"

# Check for TEMP/temporary markers
check_forbidden_patterns "\\bTEMP\\b\\|\\btemporary\\b\\|\\bTEMPORARY\\b" "temporary/TEMP markers"

# Clean up temp files
rm -f /tmp/check_results.txt /tmp/stub_results.txt /tmp/dummy_results.txt /tmp/throw_results.txt /tmp/empty_results.txt

# Summary
echo ""
echo "================================================="
echo "                    SUMMARY"
echo "================================================="

if [ "$FAILED" = true ]; then
    echo -e "${RED}❌ PRODUCTION READINESS CHECK FAILED${NC}"
    echo ""
    echo "The codebase contains placeholder implementations or incomplete code."
    echo "Please review the report at: $REPORT_FILE"
    echo ""
    echo "All issues must be resolved before production deployment."
    
    # Add summary to report
    echo "=================================================" >> "$REPORT_FILE"
    echo "RESULT: FAILED - NOT READY FOR PRODUCTION" >> "$REPORT_FILE"
    echo "The codebase contains placeholder implementations that must be fixed." >> "$REPORT_FILE"
    
    exit 1
else
    echo -e "${GREEN}✅ PRODUCTION READINESS CHECK PASSED${NC}"
    echo ""
    echo "No placeholder implementations or incomplete code found."
    echo "The codebase appears to be production-ready."
    
    # Add summary to report
    echo "=================================================" >> "$REPORT_FILE"
    echo "RESULT: PASSED - READY FOR PRODUCTION" >> "$REPORT_FILE"
    echo "No placeholder implementations or incomplete code found." >> "$REPORT_FILE"
    
    exit 0
fi
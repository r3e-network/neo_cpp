#!/bin/bash

# Test script for production readiness checking
# This script scans for placeholder patterns and categorizes them

echo "Production Readiness Check - Detailed Analysis"
echo "=============================================="

# Colors for output
RED='\033[0;31m'
YELLOW='\033[1;33m'
GREEN='\033[0;32m'
NC='\033[0m' # No Color

# Directories to exclude from search
EXCLUDE_DIRS="./build ./third_party ./.git ./docs ./node_modules ./vendor ./.idea ./.vscode"
EXCLUDE_PATTERN=""
for dir in $EXCLUDE_DIRS; do
    EXCLUDE_PATTERN="$EXCLUDE_PATTERN -path $dir -prune -o"
done

# Function to search for patterns
search_pattern() {
    local pattern="$1"
    local description="$2"
    local file_types="$3"
    
    echo -e "\n${YELLOW}Searching for: $description${NC}"
    echo "Pattern: $pattern"
    echo "-------------------------------------------"
    
    # Search excluding certain directories
    local result=$(find . $EXCLUDE_PATTERN -type f \( $file_types \) -print0 2>/dev/null | \
                   xargs -0 grep -n "$pattern" 2>/dev/null | \
                   grep -v "production_readiness_check" | \
                   grep -v "test_readiness_check")
    
    if [ -z "$result" ]; then
        echo -e "${GREEN}✓ No issues found${NC}"
        return 0
    else
        echo -e "${RED}✗ Issues found:${NC}"
        echo "$result" | head -20
        local count=$(echo "$result" | wc -l)
        if [ $count -gt 20 ]; then
            echo "... and $((count - 20)) more occurrences"
        fi
        return 1
    fi
}

# Track total issues
TOTAL_ISSUES=0

echo -e "\n${YELLOW}=== TODO/FIXME Comments ===${NC}"
if search_pattern "TODO\|FIXME\|XXX\|HACK\|BUG\|OPTIMIZE" "TODO/FIXME/XXX/HACK comments" "-name '*.cpp' -o -name '*.h' -o -name '*.hpp' -o -name '*.cc'"; then
    :
else
    ((TOTAL_ISSUES++))
fi

echo -e "\n${YELLOW}=== NotImplemented Patterns ===${NC}"
if search_pattern "NotImplemented\|not implemented\|Not Implemented" "NotImplemented exceptions or comments" "-name '*.cpp' -o -name '*.h' -o -name '*.hpp'"; then
    :
else
    ((TOTAL_ISSUES++))
fi

echo -e "\n${YELLOW}=== Placeholder/Stub/Mock Implementations ===${NC}"
if search_pattern "placeholder\|stub\|Stub\|dummy\|Dummy\|mock" "Placeholder/Stub/Mock implementations" "-name '*.cpp' -o -name '*.h' -o -name '*.hpp'" | grep -v test | grep -v Test | grep -v gmock; then
    :
else
    ((TOTAL_ISSUES++))
fi

echo -e "\n${YELLOW}=== Simplified/Temporary Implementations ===${NC}"
if search_pattern "for now\|simplified\|in production\|in a real implementation\|temporary\|quick fix\|quick hack" "Simplified or temporary implementations" "-name '*.cpp' -o -name '*.h' -o -name '*.hpp'"; then
    :
else
    ((TOTAL_ISSUES++))
fi

echo -e "\n${YELLOW}=== Hardcoded Values ===${NC}"
if search_pattern "hardcoded\|hard-coded\|hard coded" "Hardcoded values" "-name '*.cpp' -o -name '*.h' -o -name '*.hpp'"; then
    :
else
    ((TOTAL_ISSUES++))
fi

echo -e "\n${YELLOW}=== Empty Catch Blocks ===${NC}"
if search_pattern "catch.*{[[:space:]]*}" "Empty catch blocks" "-name '*.cpp' -o -name '*.h' -o -name '*.hpp'"; then
    :
else
    ((TOTAL_ISSUES++))
fi

echo -e "\n${YELLOW}=== Debug Code ===${NC}"
if search_pattern "std::cout.*debug\|printf.*debug\|DEBUG_MODE\|test mode" "Debug code that should be removed" "-name '*.cpp' -o -name '*.h' -o -name '*.hpp'"; then
    :
else
    ((TOTAL_ISSUES++))
fi

# Summary
echo -e "\n${YELLOW}=============================================="
echo "PRODUCTION READINESS SUMMARY"
echo "=============================================="

if [ $TOTAL_ISSUES -eq 0 ]; then
    echo -e "${GREEN}✅ PASS: No production readiness issues found!${NC}"
    echo "The codebase appears to be production ready."
    exit 0
else
    echo -e "${RED}❌ FAIL: Found $TOTAL_ISSUES categories of issues${NC}"
    echo "These issues must be resolved before production deployment."
    exit 1
fi
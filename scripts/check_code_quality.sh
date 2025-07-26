#!/bin/bash

# Neo C++ Code Quality Checker
# This script checks for various code quality issues including:
# - TODOs, FIXMEs, and other markers
# - Placeholder implementations
# - Simplified implementations
# - Incomplete error handling
# - Inconsistencies with C# implementation

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Counters
TOTAL_ISSUES=0
CRITICAL_ISSUES=0
WARNINGS=0

# Project root (assuming script is in scripts/ directory)
PROJECT_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$PROJECT_ROOT"

echo -e "${BLUE}Neo C++ Code Quality Checker${NC}"
echo -e "${BLUE}=============================${NC}"
echo "Checking directory: $PROJECT_ROOT"
echo ""

# Function to print section header
print_section() {
    echo -e "\n${YELLOW}Checking: $1${NC}"
    echo "----------------------------------------"
}

# Function to check patterns in source files
check_pattern() {
    local pattern="$1"
    local description="$2"
    local severity="$3"  # "critical" or "warning"
    local exclude_dirs="$4"
    
    # Build exclude pattern
    local exclude_pattern=""
    if [ -n "$exclude_dirs" ]; then
        exclude_pattern="--exclude-dir={$exclude_dirs}"
    fi
    
    # Search for pattern
    local results
    if [ -n "$exclude_pattern" ]; then
        results=$(grep -rn "$pattern" --include="*.cpp" --include="*.h" --include="*.hpp" $exclude_pattern src/ include/ apps/ 2>/dev/null || true)
    else
        results=$(grep -rn "$pattern" --include="*.cpp" --include="*.h" --include="*.hpp" src/ include/ apps/ 2>/dev/null || true)
    fi
    
    if [ -n "$results" ]; then
        local count=$(echo "$results" | wc -l)
        TOTAL_ISSUES=$((TOTAL_ISSUES + count))
        
        if [ "$severity" = "critical" ]; then
            echo -e "${RED}Found $count $description:${NC}"
            CRITICAL_ISSUES=$((CRITICAL_ISSUES + count))
        else
            echo -e "${YELLOW}Found $count $description:${NC}"
            WARNINGS=$((WARNINGS + count))
        fi
        
        # Show first 10 results
        echo "$results" | head -10
        if [ $count -gt 10 ]; then
            echo "... and $((count - 10)) more"
        fi
        echo ""
    else
        echo -e "${GREEN}✓ No $description found${NC}"
    fi
}

# Function to check for specific code patterns
check_code_pattern() {
    local pattern="$1"
    local description="$2"
    local file_pattern="$3"
    
    local results=$(find src/ include/ apps/ -name "$file_pattern" -type f -exec grep -l "$pattern" {} \; 2>/dev/null || true)
    
    if [ -n "$results" ]; then
        local count=$(echo "$results" | wc -l)
        TOTAL_ISSUES=$((TOTAL_ISSUES + count))
        WARNINGS=$((WARNINGS + count))
        
        echo -e "${YELLOW}Found $count files with $description:${NC}"
        echo "$results" | head -10
        if [ $count -gt 10 ]; then
            echo "... and $((count - 10)) more"
        fi
        echo ""
    else
        echo -e "${GREEN}✓ No $description found${NC}"
    fi
}

# 1. Check for TODO/FIXME markers
print_section "TODO/FIXME markers"
check_pattern "TODO|FIXME|XXX|HACK|TEMP|TEMPORARY" "development markers" "warning" "tests,test"

# 2. Check for placeholder implementations
print_section "Placeholder implementations"
check_pattern "placeholder|Placeholder|PLACEHOLDER" "placeholder references" "critical" "tests,test"
check_pattern "stub|Stub|STUB" "stub implementations" "critical" "tests,test"
check_pattern "dummy|Dummy|DUMMY" "dummy implementations" "critical" "tests,test"
check_pattern "mock|Mock|MOCK" "mock implementations" "critical" "tests,test,mocks"

# 3. Check for simplified implementations
print_section "Simplified implementations"
check_pattern "simplified|Simplified|SIMPLIFIED" "simplified implementations" "critical" "tests,test"
check_pattern "simple implementation|Simple implementation" "simple implementation comments" "critical" "tests,test"
check_pattern "basic implementation|Basic implementation" "basic implementation comments" "warning" "tests,test"

# 4. Check for incomplete implementations
print_section "Incomplete implementations"
check_pattern "not implemented|Not implemented|NOT IMPLEMENTED" "not implemented features" "critical" "tests,test"
check_pattern "not fully implemented|Not fully implemented" "partially implemented features" "critical" "tests,test"
check_pattern "throw.*not.*implemented|runtime_error.*not.*implemented" "not implemented exceptions" "critical" "tests,test"
check_pattern "return.*not implemented" "not implemented return statements" "critical" "tests,test"

# 5. Check for temporary code
print_section "Temporary code"
check_pattern "for now|For now|FOR NOW" "temporary solutions" "warning" "tests,test"
check_pattern "temporary|Temporary|TEMPORARY" "temporary code" "warning" "tests,test"
check_pattern "workaround|Workaround|WORKAROUND" "workarounds" "warning" "tests,test"
check_pattern "quick fix|Quick fix|QUICK FIX" "quick fixes" "warning" "tests,test"

# 6. Check for hardcoded values
print_section "Hardcoded values"
check_pattern "hardcoded|Hardcoded|HARDCODED" "hardcoded value references" "warning" "tests,test"
check_pattern "magic number|Magic number|MAGIC NUMBER" "magic number references" "warning" "tests,test"

# 7. Check for missing error handling
print_section "Error handling issues"
check_pattern "catch\s*\(\s*\.\.\.\s*\)\s*{\s*}" "empty catch blocks" "critical" "tests,test"
check_pattern "catch\s*\(\s*\.\.\.\s*\)\s*{\s*//\s*ignore" "ignored exceptions" "warning" "tests,test"
check_pattern "// ignore error|// Ignore error" "ignored error comments" "warning" "tests,test"

# 8. Check for C# porting comments
print_section "C# porting references"
check_pattern "C# version|c# version|csharp version" "C# version references" "warning" "tests,test"
check_pattern "ported from|Ported from" "porting comments" "warning" "tests,test"
check_pattern "see C#|See C#|see c#" "C# reference comments" "warning" "tests,test"

# 9. Check for incomplete features
print_section "Incomplete features"
check_pattern "coming soon|Coming soon|COMING SOON" "coming soon features" "critical" "tests,test"
check_pattern "not available|Not available|NOT AVAILABLE" "unavailable features" "warning" "tests,test"
check_pattern "pending|Pending|PENDING" "pending implementations" "warning" "tests,test"
check_pattern "incomplete|Incomplete|INCOMPLETE" "incomplete features" "critical" "tests,test"

# 10. Check for debug code
print_section "Debug code"
check_pattern "console\.log|Console\.Log|printf.*debug|cout.*debug" "debug output" "warning" "tests,test"
check_pattern "DEBUG ONLY|Debug only|debug only" "debug-only code" "warning" "tests,test"
check_pattern "REMOVE BEFORE PRODUCTION|remove before production" "code to remove" "critical" "tests,test"

# 11. Check for specific Neo-related issues
print_section "Neo-specific implementation issues"
check_pattern "simplified.*verification|Simplified.*verification" "simplified verification" "critical" "tests,test"
check_pattern "simplified.*consensus|Simplified.*consensus" "simplified consensus" "critical" "tests,test"
check_pattern "simplified.*crypto|Simplified.*crypto" "simplified cryptography" "critical" "tests,test"

# 12. Check for assertions in production code
print_section "Assertions in production code"
check_code_pattern "assert\s*\(" "assertions in production code" "*.cpp"

# 13. Check for large functions (potential refactoring needed)
print_section "Code complexity indicators"
echo "Checking for large functions (>200 lines)..."
find src/ include/ apps/ -name "*.cpp" -type f -exec awk '
    /^[[:space:]]*[a-zA-Z_][a-zA-Z0-9_]*[[:space:]]+[a-zA-Z_][a-zA-Z0-9_]*[[:space:]]*\(/ {
        start=NR
        func=$0
    }
    /^[[:space:]]*\{/ && start {
        brace_count=1
        func_start=NR
    }
    brace_count > 0 {
        if (/\{/) brace_count+=gsub(/\{/, "")
        if (/\}/) brace_count-=gsub(/\}/, "")
        if (brace_count == 0) {
            lines = NR - func_start
            if (lines > 200) {
                print FILENAME ":" start ": Large function (" lines " lines)"
            }
            start=0
        }
    }
' {} \; 2>/dev/null | head -10 || true

# 14. Check for missing documentation
print_section "Documentation issues"
echo "Checking for undocumented public APIs..."
find include/ -name "*.h" -type f -exec grep -B1 "public:" {} \; | grep -v "///" | grep -v "^\s*/\*" | head -10 || true

# 15. Check specific files that commonly have issues
print_section "Known problematic areas"
echo "Checking specific files for common issues..."

# Check BigInteger implementations
if [ -f "src/extensions/biginteger_extensions.cpp" ]; then
    echo -n "BigInteger implementation: "
    if grep -q "not fully implemented\|Not fully implemented" "src/extensions/biginteger_extensions.cpp"; then
        echo -e "${RED}Has incomplete implementations${NC}"
        CRITICAL_ISSUES=$((CRITICAL_ISSUES + 1))
    else
        echo -e "${GREEN}✓ Complete${NC}"
    fi
fi

# Check consensus implementation
if [ -d "src/consensus" ]; then
    echo -n "Consensus implementation: "
    consensus_files=$(find src/consensus -name "*.cpp" -type f | wc -l)
    if [ $consensus_files -lt 5 ]; then
        echo -e "${YELLOW}Limited implementation (only $consensus_files files)${NC}"
        WARNINGS=$((WARNINGS + 1))
    else
        echo -e "${GREEN}✓ Appears complete${NC}"
    fi
fi

# Summary
echo -e "\n${BLUE}========== Summary ==========${NC}"
echo -e "Total issues found: ${TOTAL_ISSUES}"
echo -e "Critical issues: ${RED}${CRITICAL_ISSUES}${NC}"
echo -e "Warnings: ${YELLOW}${WARNINGS}${NC}"

if [ $CRITICAL_ISSUES -gt 0 ]; then
    echo -e "\n${RED}❌ Code has critical issues that need to be addressed${NC}"
    exit 1
elif [ $WARNINGS -gt 0 ]; then
    echo -e "\n${YELLOW}⚠️  Code has warnings that should be reviewed${NC}"
    exit 0
else
    echo -e "\n${GREEN}✅ Code quality check passed!${NC}"
    exit 0
fi
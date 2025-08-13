#!/bin/bash

# check_documentation.sh - Check code documentation coverage
# Analyzes header files for proper Doxygen documentation

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Counters
TOTAL_FILES=0
DOCUMENTED_FILES=0
TOTAL_CLASSES=0
DOCUMENTED_CLASSES=0
TOTAL_FUNCTIONS=0
DOCUMENTED_FUNCTIONS=0
WARNINGS=0

echo "======================================"
echo "   NEO C++ DOCUMENTATION CHECK"
echo "======================================"
echo ""

# Function to check file documentation
check_file_doc() {
    local file=$1
    local has_file_doc=1  # Default to not found (bash return 0 = success)
    
    # Check for @file documentation in first 30 lines
    if head -30 "$file" 2>/dev/null | grep -q "@file"; then
        has_file_doc=0  # Found it (bash return 0 = success)
        ((DOCUMENTED_FILES++))
    fi
    
    return $has_file_doc
}

# Function to check class documentation
check_class_doc() {
    local file=$1
    local classes=$(grep -E "^[[:space:]]*(class|struct)[[:space:]]+[A-Za-z_][A-Za-z0-9_]*[[:space:]*{]?" "$file" 2>/dev/null | wc -l)
    local documented=0
    
    if [ $classes -gt 0 ]; then
        # Look for @class or @brief before class definitions
        documented=$(grep -B5 -E "^[[:space:]]*(class|struct)[[:space:]]+" "$file" 2>/dev/null | grep -c "@class\|@brief" || true)
        TOTAL_CLASSES=$((TOTAL_CLASSES + classes))
        DOCUMENTED_CLASSES=$((DOCUMENTED_CLASSES + documented))
    fi
}

# Function to check function documentation  
check_function_doc() {
    local file=$1
    
    # Count public functions (rough estimate)
    local functions=$(grep -E "^[[:space:]]*(virtual[[:space:]]+)?\w+.*\(.*\)[[:space:]*;]?" "$file" 2>/dev/null | \
                     grep -v "^[[:space:]]*//" | \
                     grep -v "private:" | \
                     grep -v "protected:" | \
                     wc -l)
    
    if [ $functions -gt 0 ]; then
        # Count documented functions (those with @brief or /** before them)
        local documented=$(grep -B3 -E "^[[:space:]]*(virtual[[:space:]]+)?\w+.*\(.*\)[[:space:]*;]?" "$file" 2>/dev/null | \
                          grep -c "@brief\|/\*\*" || true)
        
        TOTAL_FUNCTIONS=$((TOTAL_FUNCTIONS + functions))
        DOCUMENTED_FUNCTIONS=$((DOCUMENTED_FUNCTIONS + documented/2))  # Divide by 2 as grep -B3 may count duplicates
    fi
}

# Function to check for common issues
check_issues() {
    local file=$1
    
    # Check for TODO without tracking
    if grep -q "TODO[^(]" "$file" 2>/dev/null; then
        echo -e "${YELLOW}  ⚠ TODO without issue tracking in $file${NC}"
        ((WARNINGS++))
    fi
    
    # Check for commented out code
    if grep -E "^[[:space:]]*//.*[{};]" "$file" 2>/dev/null | grep -qv "@\|///"; then
        echo -e "${YELLOW}  ⚠ Possible commented-out code in $file${NC}"
        ((WARNINGS++))
    fi
}

# Main documentation check
echo -e "${BLUE}Checking header files...${NC}"
echo ""

# Process all header files
for header in $(find include -name "*.h" -o -name "*.hpp" 2>/dev/null | grep -v third_party | sort); do
    ((TOTAL_FILES++))
    
    # Skip third-party files
    if [[ "$header" == *"third_party"* ]]; then
        continue
    fi
    
    # Check file documentation
    if check_file_doc "$header"; then
        echo -e "${GREEN}✓${NC} $header"
    else
        echo -e "${RED}✗${NC} $header - missing @file documentation"
    fi
    
    # Check class documentation
    check_class_doc "$header"
    
    # Check function documentation
    check_function_doc "$header"
    
    # Check for issues
    check_issues "$header"
done

echo ""
echo "======================================"
echo "   DOCUMENTATION COVERAGE REPORT"
echo "======================================"
echo ""

# Calculate percentages
FILE_COVERAGE=0
CLASS_COVERAGE=0
FUNCTION_COVERAGE=0

if [ $TOTAL_FILES -gt 0 ]; then
    FILE_COVERAGE=$((DOCUMENTED_FILES * 100 / TOTAL_FILES))
fi

if [ $TOTAL_CLASSES -gt 0 ]; then
    CLASS_COVERAGE=$((DOCUMENTED_CLASSES * 100 / TOTAL_CLASSES))
fi

if [ $TOTAL_FUNCTIONS -gt 0 ]; then
    FUNCTION_COVERAGE=$((DOCUMENTED_FUNCTIONS * 100 / TOTAL_FUNCTIONS))
fi

# Print report
echo "📁 File Documentation:"
echo "   Total Files: $TOTAL_FILES"
echo "   Documented: $DOCUMENTED_FILES"
echo -e "   Coverage: ${FILE_COVERAGE}%"
echo ""

echo "📦 Class Documentation:"
echo "   Total Classes: $TOTAL_CLASSES"
echo "   Documented: $DOCUMENTED_CLASSES"
echo -e "   Coverage: ${CLASS_COVERAGE}%"
echo ""

echo "🔧 Function Documentation:"
echo "   Estimated Functions: $TOTAL_FUNCTIONS"
echo "   Documented: $DOCUMENTED_FUNCTIONS"
echo -e "   Coverage: ${FUNCTION_COVERAGE}%"
echo ""

echo "⚠️  Warnings: $WARNINGS"
echo ""

# Overall assessment
OVERALL_COVERAGE=$(((FILE_COVERAGE + CLASS_COVERAGE + FUNCTION_COVERAGE) / 3))

echo "======================================"
echo -e "   OVERALL COVERAGE: ${OVERALL_COVERAGE}%"
echo "======================================"

if [ $OVERALL_COVERAGE -ge 80 ]; then
    echo -e "${GREEN}✅ Documentation coverage is GOOD${NC}"
    exit 0
elif [ $OVERALL_COVERAGE -ge 60 ]; then
    echo -e "${YELLOW}⚠️  Documentation coverage needs improvement${NC}"
    exit 0
else
    echo -e "${RED}❌ Documentation coverage is POOR${NC}"
    echo ""
    echo "Recommendations:"
    echo "1. Add @file documentation to all headers"
    echo "2. Document all public classes with @class and @brief"
    echo "3. Document all public methods with @brief, @param, @return"
    echo "4. Follow the guidelines in docs/COMMENTING_GUIDELINES.md"
    exit 1
fi
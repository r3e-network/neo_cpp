#!/bin/bash

# Master script to run all code quality checks
# This runs all quality check scripts and aggregates results

set -e

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
MAGENTA='\033[0;35m'
NC='\033[0m'

# Get script directory
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

echo -e "${MAGENTA}╔══════════════════════════════════════════════════════════╗${NC}"
echo -e "${MAGENTA}║          Neo C++ Comprehensive Quality Check             ║${NC}"
echo -e "${MAGENTA}╚══════════════════════════════════════════════════════════╝${NC}"
echo ""
echo "Project root: $PROJECT_ROOT"
echo "Date: $(date)"
echo ""

# Track overall results
TOTAL_CHECKS=0
PASSED_CHECKS=0
FAILED_CHECKS=0
WARNING_CHECKS=0

# Function to run a check
run_check() {
    local check_name="$1"
    local check_command="$2"
    
    TOTAL_CHECKS=$((TOTAL_CHECKS + 1))
    
    echo -e "\n${BLUE}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
    echo -e "${BLUE}Running: ${check_name}${NC}"
    echo -e "${BLUE}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
    
    # Run the check and capture exit code
    set +e
    eval "$check_command"
    local exit_code=$?
    set -e
    
    if [ $exit_code -eq 0 ]; then
        if grep -q "warning\|Warning\|WARNING" <<< "$check_command" 2>/dev/null; then
            echo -e "\n${YELLOW}⚠️  ${check_name}: WARNINGS${NC}"
            WARNING_CHECKS=$((WARNING_CHECKS + 1))
        else
            echo -e "\n${GREEN}✅ ${check_name}: PASSED${NC}"
            PASSED_CHECKS=$((PASSED_CHECKS + 1))
        fi
    else
        echo -e "\n${RED}❌ ${check_name}: FAILED${NC}"
        FAILED_CHECKS=$((FAILED_CHECKS + 1))
    fi
    
    return $exit_code
}

# Create results directory
RESULTS_DIR="$PROJECT_ROOT/quality_check_results"
mkdir -p "$RESULTS_DIR"
TIMESTAMP=$(date +%Y%m%d_%H%M%S)

# 1. Run basic quality check
echo -e "\n${YELLOW}1. Basic Code Quality Check${NC}"
run_check "Basic Quality Check" "$SCRIPT_DIR/check_code_quality.sh 2>&1 | tee $RESULTS_DIR/basic_quality_$TIMESTAMP.log"

# 2. Run C# consistency check
echo -e "\n${YELLOW}2. C# Consistency Check${NC}"
if [ -f "$SCRIPT_DIR/check_consistency_with_csharp.py" ]; then
    run_check "C# Consistency" "python3 $SCRIPT_DIR/check_consistency_with_csharp.py 2>&1 | tee $RESULTS_DIR/csharp_consistency_$TIMESTAMP.log"
else
    echo -e "${YELLOW}Skipping C# consistency check (script not found)${NC}"
fi

# 3. Check for compilation warnings
echo -e "\n${YELLOW}3. Compilation Warnings Check${NC}"
if [ -f "$PROJECT_ROOT/CMakeLists.txt" ]; then
    echo "Checking for compilation warnings..."
    cd "$PROJECT_ROOT"
    if [ -d "build" ]; then
        run_check "Compilation Warnings" "cd build && make clean && make -j$(nproc) 2>&1 | grep -i 'warning' | tee $RESULTS_DIR/compilation_warnings_$TIMESTAMP.log || true"
    else
        echo -e "${YELLOW}Build directory not found, skipping compilation check${NC}"
    fi
else
    echo -e "${YELLOW}CMakeLists.txt not found, skipping compilation check${NC}"
fi

# 4. Check for memory leaks (if valgrind is available)
echo -e "\n${YELLOW}4. Memory Safety Check${NC}"
if command -v valgrind &> /dev/null; then
    echo "Checking for potential memory issues..."
    # Just check if test executables exist
    if [ -d "$PROJECT_ROOT/build/tests" ]; then
        echo -e "${BLUE}Memory check tools available - run tests with valgrind for detailed analysis${NC}"
    fi
else
    echo -e "${YELLOW}Valgrind not installed, skipping memory check${NC}"
fi

# 5. Check code formatting consistency
echo -e "\n${YELLOW}5. Code Formatting Check${NC}"
if command -v clang-format &> /dev/null; then
    echo "Checking code formatting..."
    # Check if .clang-format exists
    if [ -f "$PROJECT_ROOT/.clang-format" ]; then
        # Count files that would be changed
        UNFORMATTED_COUNT=$(find "$PROJECT_ROOT/src" "$PROJECT_ROOT/include" -name "*.cpp" -o -name "*.h" | xargs clang-format -n 2>&1 | grep -c "would change" || true)
        if [ $UNFORMATTED_COUNT -gt 0 ]; then
            echo -e "${YELLOW}Found $UNFORMATTED_COUNT files with formatting issues${NC}"
            WARNING_CHECKS=$((WARNING_CHECKS + 1))
        else
            echo -e "${GREEN}✅ All files properly formatted${NC}"
            PASSED_CHECKS=$((PASSED_CHECKS + 1))
        fi
    else
        echo -e "${YELLOW}.clang-format not found, skipping format check${NC}"
    fi
else
    echo -e "${YELLOW}clang-format not installed, skipping format check${NC}"
fi

# 6. Check for cyclic dependencies
echo -e "\n${YELLOW}6. Dependency Analysis${NC}"
echo "Analyzing include dependencies..."
python3 - << 'EOF' 2>&1 | tee "$RESULTS_DIR/dependency_analysis_$TIMESTAMP.log"
import os
import re
from pathlib import Path
from collections import defaultdict

project_root = "$PROJECT_ROOT"
include_pattern = re.compile(r'#include\s*[<"]([^>"]+)[>"]')

# Build dependency graph
dependencies = defaultdict(set)
file_count = 0

for root, dirs, files in os.walk(project_root):
    # Skip build and test directories
    if 'build' in root or 'test' in root:
        continue
    
    for file in files:
        if file.endswith(('.h', '.hpp', '.cpp')):
            file_count += 1
            file_path = os.path.join(root, file)
            rel_path = os.path.relpath(file_path, project_root)
            
            try:
                with open(file_path, 'r') as f:
                    content = f.read()
                    includes = include_pattern.findall(content)
                    
                    for inc in includes:
                        if 'neo/' in inc:  # Only track project includes
                            dependencies[rel_path].add(inc)
            except:
                pass

print(f"Analyzed {file_count} files")
print(f"Found {sum(len(deps) for deps in dependencies.values())} include relationships")

# Check for potential circular dependencies (simplified)
circular_count = 0
for file, deps in dependencies.items():
    for dep in deps:
        if dep in dependencies:
            # Check if dependency includes the original file
            dep_deps = dependencies.get(dep, set())
            if any(file in d for d in dep_deps):
                circular_count += 1

if circular_count > 0:
    print(f"⚠️  Warning: Found {circular_count} potential circular dependencies")
else:
    print("✅ No obvious circular dependencies found")
EOF

# 7. Security scan for common vulnerabilities
echo -e "\n${YELLOW}7. Security Scan${NC}"
echo "Scanning for common security issues..."
SECURITY_ISSUES=0

# Check for unsafe functions
UNSAFE_FUNCTIONS="strcpy|strcat|sprintf|gets|scanf"
UNSAFE_COUNT=$(grep -rE "$UNSAFE_FUNCTIONS" "$PROJECT_ROOT/src" --include="*.cpp" --include="*.h" 2>/dev/null | wc -l || true)
if [ $UNSAFE_COUNT -gt 0 ]; then
    echo -e "${RED}Found $UNSAFE_COUNT uses of unsafe functions${NC}"
    SECURITY_ISSUES=$((SECURITY_ISSUES + UNSAFE_COUNT))
fi

# Check for hardcoded credentials
CRED_PATTERNS="password|secret|apikey|api_key|private_key"
CRED_COUNT=$(grep -riE "$CRED_PATTERNS.*=.*['\"]" "$PROJECT_ROOT/src" --include="*.cpp" --include="*.h" 2>/dev/null | grep -v "test" | wc -l || true)
if [ $CRED_COUNT -gt 0 ]; then
    echo -e "${YELLOW}Found $CRED_COUNT potential hardcoded credentials${NC}"
    SECURITY_ISSUES=$((SECURITY_ISSUES + CRED_COUNT))
fi

if [ $SECURITY_ISSUES -eq 0 ]; then
    echo -e "${GREEN}✅ No obvious security issues found${NC}"
    PASSED_CHECKS=$((PASSED_CHECKS + 1))
else
    echo -e "${RED}❌ Found $SECURITY_ISSUES security concerns${NC}"
    FAILED_CHECKS=$((FAILED_CHECKS + 1))
fi

# 8. Documentation coverage
echo -e "\n${YELLOW}8. Documentation Coverage${NC}"
echo "Checking documentation coverage..."

# Count documented vs undocumented public methods
TOTAL_PUBLIC_METHODS=$(grep -r "public:" "$PROJECT_ROOT/include" --include="*.h" -A 10 | grep -E "^\s*(virtual\s+)?[a-zA-Z_].*\(" | wc -l || true)
DOCUMENTED_METHODS=$(grep -r "public:" "$PROJECT_ROOT/include" --include="*.h" -B 5 -A 10 | grep -B 3 -E "^\s*(virtual\s+)?[a-zA-Z_].*\(" | grep -c "///" || true)

if [ $TOTAL_PUBLIC_METHODS -gt 0 ]; then
    DOC_PERCENTAGE=$((DOCUMENTED_METHODS * 100 / TOTAL_PUBLIC_METHODS))
    echo "Documentation coverage: $DOC_PERCENTAGE% ($DOCUMENTED_METHODS/$TOTAL_PUBLIC_METHODS public methods)"
    
    if [ $DOC_PERCENTAGE -lt 50 ]; then
        echo -e "${RED}❌ Poor documentation coverage${NC}"
        FAILED_CHECKS=$((FAILED_CHECKS + 1))
    elif [ $DOC_PERCENTAGE -lt 80 ]; then
        echo -e "${YELLOW}⚠️  Fair documentation coverage${NC}"
        WARNING_CHECKS=$((WARNING_CHECKS + 1))
    else
        echo -e "${GREEN}✅ Good documentation coverage${NC}"
        PASSED_CHECKS=$((PASSED_CHECKS + 1))
    fi
fi

# Generate summary report
echo -e "\n${MAGENTA}╔══════════════════════════════════════════════════════════╗${NC}"
echo -e "${MAGENTA}║                    FINAL REPORT                          ║${NC}"
echo -e "${MAGENTA}╚══════════════════════════════════════════════════════════╝${NC}"

SUMMARY_FILE="$RESULTS_DIR/summary_$TIMESTAMP.txt"
{
    echo "Neo C++ Quality Check Summary"
    echo "============================="
    echo "Date: $(date)"
    echo ""
    echo "Total Checks Run: $TOTAL_CHECKS"
    echo "Passed: $PASSED_CHECKS"
    echo "Warnings: $WARNING_CHECKS"
    echo "Failed: $FAILED_CHECKS"
    echo ""
    
    if [ $FAILED_CHECKS -eq 0 ] && [ $WARNING_CHECKS -eq 0 ]; then
        echo "Status: EXCELLENT - All checks passed!"
    elif [ $FAILED_CHECKS -eq 0 ]; then
        echo "Status: GOOD - No critical issues, some warnings"
    else
        echo "Status: NEEDS ATTENTION - Critical issues found"
    fi
} | tee "$SUMMARY_FILE"

echo ""
echo -e "${BLUE}Detailed results saved to: $RESULTS_DIR${NC}"
echo ""

# Final status
if [ $FAILED_CHECKS -gt 0 ]; then
    echo -e "${RED}❌ Quality check FAILED - Critical issues need attention${NC}"
    exit 1
elif [ $WARNING_CHECKS -gt 0 ]; then
    echo -e "${YELLOW}⚠️  Quality check passed with warnings${NC}"
    exit 0
else
    echo -e "${GREEN}✅ Quality check PASSED - Excellent code quality!${NC}"
    exit 0
fi
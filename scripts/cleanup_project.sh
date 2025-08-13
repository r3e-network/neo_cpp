#!/bin/bash

# cleanup_project.sh - Comprehensive project cleanup script
# Safe cleanup of temporary files, build artifacts, and redundant files

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Counters
FILES_REMOVED=0
BYTES_FREED=0
ERRORS=0

# Dry run mode
DRY_RUN=${1:-"--dry-run"}

echo "======================================"
echo "   NEO C++ PROJECT CLEANUP"
echo "======================================"
echo ""

if [ "$DRY_RUN" == "--dry-run" ]; then
    echo -e "${YELLOW}Running in DRY-RUN mode (no files will be deleted)${NC}"
    echo "To actually clean up, run: $0 --execute"
else
    echo -e "${RED}Running in EXECUTE mode (files will be deleted)${NC}"
    read -p "Are you sure? (y/N) " -n 1 -r
    echo
    if [[ ! $REPLY =~ ^[Yy]$ ]]; then
        echo "Cleanup cancelled."
        exit 0
    fi
fi

echo ""

# Function to remove files
remove_files() {
    local pattern=$1
    local description=$2
    
    echo -e "${BLUE}Cleaning $description...${NC}"
    
    local count=0
    local size=0
    
    while IFS= read -r file; do
        if [ -f "$file" ]; then
            local file_size=$(stat -f%z "$file" 2>/dev/null || stat -c%s "$file" 2>/dev/null || echo 0)
            size=$((size + file_size))
            count=$((count + 1))
            
            if [ "$DRY_RUN" == "--dry-run" ]; then
                echo "  Would remove: $file ($(numfmt --to=iec-i --suffix=B $file_size 2>/dev/null || echo "${file_size}B"))"
            else
                rm -f "$file"
                echo "  Removed: $file"
            fi
        fi
    done < <(find . -type f \( $pattern \) 2>/dev/null)
    
    if [ $count -gt 0 ]; then
        echo -e "${GREEN}  ✓ Found $count files ($((size / 1024))KB)${NC}"
        FILES_REMOVED=$((FILES_REMOVED + count))
        BYTES_FREED=$((BYTES_FREED + size))
    else
        echo "  No files found"
    fi
    echo ""
}

# Function to clean directories
clean_directory() {
    local dir=$1
    local description=$2
    
    if [ -d "$dir" ]; then
        local size=$(du -sk "$dir" 2>/dev/null | cut -f1)
        echo -e "${BLUE}Cleaning $description...${NC}"
        
        if [ "$DRY_RUN" == "--dry-run" ]; then
            echo "  Would remove: $dir (${size}KB)"
        else
            rm -rf "$dir"
            echo "  Removed: $dir (${size}KB)"
        fi
        
        BYTES_FREED=$((BYTES_FREED + size * 1024))
        echo -e "${GREEN}  ✓ Freed ${size}KB${NC}"
        echo ""
    fi
}

# 1. Clean build artifacts
echo -e "${YELLOW}=== Build Artifacts ===${NC}"
remove_files "-name '*.o' -o -name '*.a' -o -name '*.so' -o -name '*.dylib'" "object and library files"
remove_files "-name '*.d'" "dependency files"

# 2. Clean temporary files
echo -e "${YELLOW}=== Temporary Files ===${NC}"
remove_files "-name '*.tmp' -o -name '*.temp' -o -name '*~' -o -name '*.swp'" "temporary files"
remove_files "-name '.DS_Store'" "macOS metadata files"
remove_files "-name '*.bak' -o -name '*.orig'" "backup files"

# 3. Clean log files
echo -e "${YELLOW}=== Log Files ===${NC}"
remove_files "-name '*.log'" "log files"

# 4. Clean empty files (excluding important ones)
echo -e "${YELLOW}=== Empty Files ===${NC}"
while IFS= read -r file; do
    # Skip important empty files
    if [[ ! "$file" =~ \.git|__init__.py|\.gitkeep|LOCK|\.lock ]]; then
        if [ "$DRY_RUN" == "--dry-run" ]; then
            echo "  Would remove empty file: $file"
        else
            rm -f "$file"
            echo "  Removed empty file: $file"
        fi
        FILES_REMOVED=$((FILES_REMOVED + 1))
    fi
done < <(find . -type f -size 0 2>/dev/null | grep -v ".git")

# 5. Clean build directories (optional)
echo -e "${YELLOW}=== Build Directories ===${NC}"
if [ "$DRY_RUN" != "--dry-run" ]; then
    read -p "Remove build directories? (y/N) " -n 1 -r
    echo
    if [[ $REPLY =~ ^[Yy]$ ]]; then
        clean_directory "build" "build directory"
        clean_directory "build-debug" "debug build directory"
        clean_directory "cmake-build-debug" "CMake debug build"
        clean_directory "Testing" "testing directory"
    fi
else
    echo "  Would remove: build/, build-debug/, cmake-build-debug/, Testing/"
fi

# 6. Clean Python cache
echo -e "${YELLOW}=== Python Cache ===${NC}"
remove_files "-name '*.pyc' -o -name '*.pyo'" "Python bytecode files"
find . -type d -name "__pycache__" 2>/dev/null | while read -r dir; do
    if [ "$DRY_RUN" == "--dry-run" ]; then
        echo "  Would remove: $dir"
    else
        rm -rf "$dir"
        echo "  Removed: $dir"
    fi
    FILES_REMOVED=$((FILES_REMOVED + 1))
done

# 7. Consolidate redundant test runners
echo -e "${YELLOW}=== Redundant Scripts ===${NC}"
echo "Found multiple test runner scripts:"
echo "  - scripts/run_tests.sh (original)"
echo "  - scripts/run_all_tests.sh (comprehensive)"
echo "  - scripts/run_tests_simple.sh (simplified)"
echo "  - scripts/test_runner.sh (current)"
echo ""
echo "Recommendation: Keep only scripts/test_runner.sh as it's the most current"
if [ "$DRY_RUN" != "--dry-run" ]; then
    read -p "Remove redundant test runners? (y/N) " -n 1 -r
    echo
    if [[ $REPLY =~ ^[Yy]$ ]]; then
        rm -f scripts/run_tests.sh scripts/run_all_tests.sh scripts/run_tests_simple.sh
        echo -e "${GREEN}  ✓ Removed redundant test runners${NC}"
        FILES_REMOVED=$((FILES_REMOVED + 3))
    fi
fi

# 8. Consolidate status documentation
echo -e "${YELLOW}=== Documentation Consolidation ===${NC}"
echo "Found multiple status documents:"
echo "  - INFRASTRUCTURE_STATUS.md"
echo "  - ISSUES_RESOLVED.md"
echo "  - VALIDATION_SUCCESS.md"
echo "  - CMAKE_MAKEFILE_INTEGRATION.md"
echo ""
echo "These could be consolidated into a single PROJECT_STATUS.md"

# 9. Clean ccache
echo -e "${YELLOW}=== Ccache ===${NC}"
if command -v ccache &> /dev/null; then
    local cache_size=$(ccache -s | grep "cache size" | awk '{print $3}')
    echo "Current ccache size: $cache_size"
    if [ "$DRY_RUN" != "--dry-run" ]; then
        read -p "Clear ccache? (y/N) " -n 1 -r
        echo
        if [[ $REPLY =~ ^[Yy]$ ]]; then
            ccache -C
            echo -e "${GREEN}  ✓ Ccache cleared${NC}"
        fi
    fi
else
    echo "  ccache not installed"
fi

# Summary
echo ""
echo "======================================"
echo "   CLEANUP SUMMARY"
echo "======================================"
if [ "$DRY_RUN" == "--dry-run" ]; then
    echo -e "${YELLOW}DRY RUN RESULTS:${NC}"
    echo "  Files that would be removed: $FILES_REMOVED"
    echo "  Space that would be freed: $((BYTES_FREED / 1024))KB"
    echo ""
    echo "To execute cleanup, run: $0 --execute"
else
    echo -e "${GREEN}CLEANUP COMPLETED:${NC}"
    echo "  Files removed: $FILES_REMOVED"
    echo "  Space freed: $((BYTES_FREED / 1024))KB"
fi

# Additional recommendations
echo ""
echo -e "${BLUE}Additional Recommendations:${NC}"
echo "1. Run 'git gc' to optimize git repository"
echo "2. Consider using .gitignore for build artifacts"
echo "3. Set up pre-commit hooks to maintain cleanliness"
echo "4. Regular cleanup with: make clean or cmake --build build --target clean"
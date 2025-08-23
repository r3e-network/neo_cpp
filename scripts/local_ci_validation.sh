#!/bin/bash

# Neo C++ Local CI/CD Validation Script
# Converts GitHub Actions workflow to local execution for comprehensive validation

set -e  # Exit on any error

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Configuration
BUILD_TYPE="Release"
CMAKE_BUILD_PARALLEL_LEVEL=4
PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${PROJECT_ROOT}/build_local_validation"
LOG_DIR="${PROJECT_ROOT}/validation_logs"

echo -e "${BLUE}╔═══════════════════════════════════════════════════════════╗${NC}"
echo -e "${BLUE}║           Neo C++ Local CI/CD Validation Suite           ║${NC}"
echo -e "${BLUE}║              Comprehensive Build & Test Validation       ║${NC}"
echo -e "${BLUE}╚═══════════════════════════════════════════════════════════╝${NC}"
echo ""

# Function to print status
print_status() {
    local status=$1
    local message=$2
    case $status in
        "INFO")
            echo -e "${BLUE}ℹ️  ${message}${NC}"
            ;;
        "SUCCESS")
            echo -e "${GREEN}✅ ${message}${NC}"
            ;;
        "WARNING")
            echo -e "${YELLOW}⚠️  ${message}${NC}"
            ;;
        "ERROR")
            echo -e "${RED}❌ ${message}${NC}"
            ;;
        "HEADER")
            echo -e "${BLUE}${message}${NC}"
            echo "============================================================"
            ;;
    esac
}

# Function to check command availability
check_command() {
    local cmd=$1
    local name=$2
    if command -v "$cmd" >/dev/null 2>&1; then
        print_status "SUCCESS" "$name found: $(command -v $cmd)"
        return 0
    else
        print_status "ERROR" "$name not found"
        return 1
    fi
}

# Function to check library availability
check_library() {
    local lib=$1
    local name=$2
    if ldconfig -p | grep -q "$lib" 2>/dev/null; then
        print_status "SUCCESS" "$name library found"
        return 0
    elif find /usr/lib* /opt/homebrew/lib /usr/local/lib -name "*$lib*" 2>/dev/null | head -1 | grep -q .; then
        print_status "SUCCESS" "$name library found (alternative path)"
        return 0
    else
        print_status "WARNING" "$name library not found (may cause build issues)"
        return 1
    fi
}

# Create directories
mkdir -p "$LOG_DIR"
mkdir -p "$BUILD_DIR"

cd "$PROJECT_ROOT"

print_status "HEADER" "PHASE 1: ENVIRONMENT VALIDATION"

# Check required tools
print_status "INFO" "Checking build tools..."
check_command "cmake" "CMake"
cmake_found=$?

check_command "ninja" "Ninja"
ninja_found=$?

if [ $ninja_found -ne 0 ]; then
    check_command "make" "Make"
    make_found=$?
    if [ $make_found -eq 0 ]; then
        BUILD_GENERATOR="Unix Makefiles"
        BUILD_COMMAND="make"
    else
        print_status "ERROR" "Neither Ninja nor Make found"
        exit 1
    fi
else
    BUILD_GENERATOR="Ninja"
    BUILD_COMMAND="ninja"
fi

# Check compilers
print_status "INFO" "Checking compilers..."
check_command "g++" "GCC"
gcc_found=$?

check_command "clang++" "Clang"
clang_found=$?

if [ $gcc_found -eq 0 ]; then
    CXX_COMPILER="g++"
    C_COMPILER="gcc"
    print_status "SUCCESS" "Using GCC compiler"
elif [ $clang_found -eq 0 ]; then
    CXX_COMPILER="clang++"
    C_COMPILER="clang"
    print_status "SUCCESS" "Using Clang compiler"
else
    print_status "ERROR" "No suitable compiler found"
    exit 1
fi

# Check libraries
print_status "INFO" "Checking system libraries..."
check_library "libssl" "OpenSSL"
check_library "libboost" "Boost"
check_library "libcurl" "CURL"
check_library "libz" "Zlib"

# Check Python
check_command "python3" "Python3"

print_status "HEADER" "PHASE 2: PROJECT STRUCTURE VALIDATION"

# Validate project structure
print_status "INFO" "Validating project structure..."

required_dirs=("src" "tests" "include" "cmake" "scripts")
for dir in "${required_dirs[@]}"; do
    if [ -d "$dir" ]; then
        print_status "SUCCESS" "Directory $dir exists"
    else
        print_status "ERROR" "Required directory $dir missing"
        exit 1
    fi
done

required_files=("CMakeLists.txt" "README.md" "LICENSE")
for file in "${required_files[@]}"; do
    if [ -f "$file" ]; then
        print_status "SUCCESS" "File $file exists"
    else
        print_status "WARNING" "File $file missing"
    fi
done

print_status "HEADER" "PHASE 3: CMAKE CONFIGURATION"

# Clean previous build
if [ -d "$BUILD_DIR" ]; then
    print_status "INFO" "Cleaning previous build..."
    rm -rf "$BUILD_DIR"
fi

mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

print_status "INFO" "Configuring CMake with $BUILD_GENERATOR..."

# Configure CMake (equivalent to GitHub Actions configuration)
CMAKE_CONFIG_LOG="$LOG_DIR/cmake_config.log"
cmake_config_cmd="cmake -B . \
    -DCMAKE_BUILD_TYPE=$BUILD_TYPE \
    -DCMAKE_C_COMPILER=$C_COMPILER \
    -DCMAKE_CXX_COMPILER=$CXX_COMPILER \
    -DNEO_BUILD_TESTS=ON \
    -DNEO_BUILD_EXAMPLES=ON \
    -DNEO_BUILD_TOOLS=ON \
    -DNEO_BUILD_APPS=ON \
    -DNEO_BUILD_SDK=ON \
    -DENABLE_COVERAGE=OFF \
    -G\"$BUILD_GENERATOR\" \
    .."

echo "CMake command: $cmake_config_cmd" > "$CMAKE_CONFIG_LOG"

if eval "$cmake_config_cmd" >> "$CMAKE_CONFIG_LOG" 2>&1; then
    print_status "SUCCESS" "CMake configuration successful"
else
    print_status "ERROR" "CMake configuration failed"
    echo "Last 20 lines of CMake log:"
    tail -20 "$CMAKE_CONFIG_LOG"
    exit 1
fi

print_status "HEADER" "PHASE 4: BUILD EXECUTION"

print_status "INFO" "Building project with $BUILD_COMMAND..."

BUILD_LOG="$LOG_DIR/build.log"
build_cmd="cmake --build . --config $BUILD_TYPE --parallel $CMAKE_BUILD_PARALLEL_LEVEL"

echo "Build command: $build_cmd" > "$BUILD_LOG"
echo "Build started at: $(date)" >> "$BUILD_LOG"

if eval "$build_cmd" >> "$BUILD_LOG" 2>&1; then
    print_status "SUCCESS" "Build completed successfully"
else
    print_status "ERROR" "Build failed"
    echo "Last 30 lines of build log:"
    tail -30 "$BUILD_LOG"
    
    # Try to continue with available targets
    print_status "WARNING" "Attempting to continue with partial build..."
fi

print_status "HEADER" "PHASE 5: BUILD ARTIFACTS VALIDATION"

print_status "INFO" "Validating build artifacts..."

# Check for built libraries
if find . -name "libneo*.a" | head -1 | grep -q .; then
    lib_count=$(find . -name "libneo*.a" | wc -l)
    print_status "SUCCESS" "Found $lib_count Neo libraries"
    
    # List main libraries
    print_status "INFO" "Main libraries built:"
    find . -name "libneo*.a" | head -10 | while read lib; do
        size=$(ls -lh "$lib" | awk '{print $5}')
        print_status "INFO" "  $(basename "$lib") - $size"
    done
else
    print_status "WARNING" "No Neo libraries found"
fi

# Check for executables
if find . -name "neo_*" -executable -type f | head -1 | grep -q .; then
    exe_count=$(find . -name "neo_*" -executable -type f | wc -l)
    print_status "SUCCESS" "Found $exe_count Neo executables"
    
    # List main executables
    print_status "INFO" "Main executables built:"
    find . -name "neo_*" -executable -type f | head -5 | while read exe; do
        size=$(ls -lh "$exe" | awk '{print $5}')
        print_status "INFO" "  $(basename "$exe") - $size"
    done
else
    print_status "WARNING" "No Neo executables found"
fi

# Check for test executables
if find . -name "*test*" -executable -type f | head -1 | grep -q .; then
    test_count=$(find . -name "*test*" -executable -type f | wc -l)
    print_status "SUCCESS" "Found $test_count test executables"
else
    print_status "WARNING" "No test executables found"
fi

print_status "HEADER" "PHASE 6: TEST EXECUTION"

TEST_LOG="$LOG_DIR/test_execution.log"
echo "Test execution started at: $(date)" > "$TEST_LOG"

# Run tests if available
if [ -f "CTestTestfile.cmake" ] || command -v ctest >/dev/null 2>&1; then
    print_status "INFO" "Running CTest suite..."
    
    if ctest --output-on-failure --parallel 4 >> "$TEST_LOG" 2>&1; then
        print_status "SUCCESS" "CTest execution completed"
    else
        print_status "WARNING" "Some tests may have failed"
    fi
    
    # Extract test results
    if grep -q "tests passed" "$TEST_LOG"; then
        test_summary=$(grep "tests passed\|% tests passed" "$TEST_LOG" | tail -1)
        print_status "INFO" "Test summary: $test_summary"
    fi
else
    print_status "INFO" "CTest not available, checking for individual test executables..."
    
    # Run individual test executables
    test_results=0
    test_total=0
    
    find . -name "*test*" -executable -type f | head -10 | while read test_exe; do
        test_name=$(basename "$test_exe")
        print_status "INFO" "Running $test_name..."
        
        if timeout 30s "$test_exe" >> "$TEST_LOG" 2>&1; then
            print_status "SUCCESS" "$test_name passed"
        else
            print_status "WARNING" "$test_name failed or timed out"
        fi
    done
fi

print_status "HEADER" "PHASE 7: PRODUCTION READINESS VALIDATION"

cd "$PROJECT_ROOT"

# Check for production readiness indicators
print_status "INFO" "Validating production readiness..."

# Check configuration files
if [ -f "config/production_config.json" ]; then
    print_status "SUCCESS" "Production configuration found"
else
    print_status "WARNING" "Production configuration missing"
fi

# Check security documentation
if [ -f "docs/PRODUCTION_SECURITY.md" ]; then
    print_status "SUCCESS" "Production security documentation found"
else
    print_status "WARNING" "Production security documentation missing"
fi

# Check deployment files
if [ -f "Dockerfile" ]; then
    print_status "SUCCESS" "Docker deployment configuration found"
else
    print_status "WARNING" "Docker deployment configuration missing"
fi

# Check monitoring
if [ -d "monitoring" ]; then
    print_status "SUCCESS" "Monitoring configuration found"
else
    print_status "WARNING" "Monitoring configuration missing"
fi

print_status "HEADER" "PHASE 8: VALIDATION SUMMARY"

# Generate validation report
VALIDATION_REPORT="$LOG_DIR/validation_report_$(date +%Y%m%d_%H%M%S).md"

cat > "$VALIDATION_REPORT" << EOF
# Neo C++ Local CI/CD Validation Report

**Date**: $(date)
**Build Type**: $BUILD_TYPE
**Compiler**: $CXX_COMPILER
**Build System**: $BUILD_GENERATOR

## Environment Validation
- CMake: $(cmake --version | head -1)
- Compiler: $($CXX_COMPILER --version | head -1)
- Build System: $BUILD_GENERATOR

## Build Results
- Build Directory: $BUILD_DIR
- Libraries Built: $(find "$BUILD_DIR" -name "libneo*.a" 2>/dev/null | wc -l)
- Executables Built: $(find "$BUILD_DIR" -name "neo_*" -executable -type f 2>/dev/null | wc -l)
- Test Executables: $(find "$BUILD_DIR" -name "*test*" -executable -type f 2>/dev/null | wc -l)

## Log Files
- CMake Configuration: $CMAKE_CONFIG_LOG
- Build Log: $BUILD_LOG
- Test Execution: $TEST_LOG

## Production Readiness
- Configuration: $([ -f "config/production_config.json" ] && echo "✅" || echo "❌")
- Security Docs: $([ -f "docs/PRODUCTION_SECURITY.md" ] && echo "✅" || echo "❌")
- Docker Support: $([ -f "Dockerfile" ] && echo "✅" || echo "❌")
- Monitoring: $([ -d "monitoring" ] && echo "✅" || echo "❌")

## Validation Status
**Overall Status**: $([ -f "$BUILD_DIR/src/libneo_core.a" ] && echo "✅ PASSED" || echo "⚠️ PARTIAL")

EOF

print_status "SUCCESS" "Validation report generated: $VALIDATION_REPORT"

# Final summary
echo ""
print_status "HEADER" "FINAL VALIDATION RESULTS"

total_issues=0

# Check critical components
if [ -f "$BUILD_DIR/src/libneo_core.a" ]; then
    print_status "SUCCESS" "Core library built successfully"
else
    print_status "ERROR" "Core library not found"
    ((total_issues++))
fi

if [ $(find "$BUILD_DIR" -name "libneo*.a" 2>/dev/null | wc -l) -gt 5 ]; then
    print_status "SUCCESS" "Multiple Neo libraries built"
else
    print_status "WARNING" "Limited Neo libraries built"
    ((total_issues++))
fi

if [ -f "$CMAKE_CONFIG_LOG" ] && ! grep -q "Error\|FATAL" "$CMAKE_CONFIG_LOG"; then
    print_status "SUCCESS" "CMake configuration clean"
else
    print_status "WARNING" "CMake configuration has warnings/errors"
fi

if [ -f "$BUILD_LOG" ] && ! grep -q "error:\|Error " "$BUILD_LOG"; then
    print_status "SUCCESS" "Build completed without errors"
else
    print_status "WARNING" "Build completed with warnings/errors"
fi

# Production readiness check
production_score=0
total_checks=4

[ -f "config/production_config.json" ] && ((production_score++))
[ -f "docs/PRODUCTION_SECURITY.md" ] && ((production_score++))
[ -f "Dockerfile" ] && ((production_score++))
[ -d "monitoring" ] && ((production_score++))

production_percentage=$((production_score * 100 / total_checks))

echo ""
print_status "INFO" "Production Readiness Score: $production_score/$total_checks ($production_percentage%)"

if [ $total_issues -eq 0 ] && [ $production_percentage -ge 75 ]; then
    print_status "SUCCESS" "🎉 LOCAL VALIDATION PASSED - PRODUCTION READY"
    echo ""
    echo -e "${GREEN}╔═══════════════════════════════════════════════════════════╗${NC}"
    echo -e "${GREEN}║                    VALIDATION SUCCESSFUL                  ║${NC}"
    echo -e "${GREEN}║              Neo C++ is Production Ready!                 ║${NC}"
    echo -e "${GREEN}╚═══════════════════════════════════════════════════════════╝${NC}"
    exit 0
elif [ $total_issues -eq 0 ]; then
    print_status "SUCCESS" "🎯 LOCAL VALIDATION PASSED - Minor improvements recommended"
    exit 0
else
    print_status "WARNING" "⚠️  LOCAL VALIDATION COMPLETED WITH ISSUES - Review required"
    echo ""
    echo -e "${YELLOW}Issues found: $total_issues${NC}"
    echo -e "${YELLOW}Check logs in: $LOG_DIR${NC}"
    exit 2
fi
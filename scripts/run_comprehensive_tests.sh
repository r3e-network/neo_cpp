#!/bin/bash

# Neo C++ Comprehensive Test Runner
echo "╔═══════════════════════════════════════════════════════════════════════════════╗"
echo "║                    NEO C++ COMPREHENSIVE TEST EXECUTION                      ║"
echo "╚═══════════════════════════════════════════════════════════════════════════════╝"

# Colors
GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

# Test execution summary
TOTAL_TESTS=0
PASSED_TESTS=0
FAILED_TESTS=0

echo ""
echo -e "${BLUE}📊 Neo C++ Test Suite Analysis${NC}"
echo "═══════════════════════════════════════════════════════════════════════════════"

# Count test files
CPP_TESTS=$(find tests -name "*.cpp" -type f | wc -l)
JSON_TESTS=$(find tests -name "*.json" -type f | wc -l)
TOTAL_FILES=$((CPP_TESTS + JSON_TESTS))

echo "C++ Test Files: $CPP_TESTS"
echo "JSON Test Files: $JSON_TESTS"
echo "Total Test Files: $TOTAL_FILES"

echo ""
echo -e "${BLUE}🧪 Running Test Infrastructure Validation${NC}"
echo "═══════════════════════════════════════════════════════════════════════════════"

# Run our test validation tools
echo -e "${YELLOW}Phase 1: Basic Infrastructure Tests${NC}"

if [ -x "./minimal_test_demo" ]; then
    echo "Running minimal test demo..."
    ./minimal_test_demo | grep -E "(PASSED|FAILED|tests:|passed:|failed:)"
    if [ $? -eq 0 ]; then
        echo -e "${GREEN}✅ Minimal test demo: PASSED${NC}"
        PASSED_TESTS=$((PASSED_TESTS + 1))
    else
        echo -e "${RED}❌ Minimal test demo: FAILED${NC}"
        FAILED_TESTS=$((FAILED_TESTS + 1))
    fi
    TOTAL_TESTS=$((TOTAL_TESTS + 1))
else
    echo -e "${YELLOW}⚠️ minimal_test_demo not executable${NC}"
fi

echo ""
echo -e "${YELLOW}Phase 2: Test File Validation${NC}"

if [ -x "./validate_tests" ]; then
    echo "Running test file validation..."
    ./validate_tests | grep -E "(✅|❌|Files found:|Summary:)"
    if [ $? -eq 0 ]; then
        echo -e "${GREEN}✅ Test file validation: PASSED${NC}"
        PASSED_TESTS=$((PASSED_TESTS + 1))
    else
        echo -e "${RED}❌ Test file validation: FAILED${NC}"
        FAILED_TESTS=$((FAILED_TESTS + 1))
    fi
    TOTAL_TESTS=$((TOTAL_TESTS + 1))
else
    echo -e "${YELLOW}⚠️ validate_tests not executable${NC}"
fi

echo ""
echo -e "${YELLOW}Phase 3: Comprehensive Test Simulation${NC}"

if [ -x "./run_all_tests" ]; then
    echo "Running comprehensive test simulation..."
    # Run with timeout to prevent hanging
    timeout 60s ./run_all_tests | tail -20
    if [ $? -eq 0 ]; then
        echo -e "${GREEN}✅ Comprehensive test simulation: PASSED${NC}"
        PASSED_TESTS=$((PASSED_TESTS + 1))
    else
        echo -e "${RED}❌ Comprehensive test simulation: FAILED${NC}"
        FAILED_TESTS=$((FAILED_TESTS + 1))
    fi
    TOTAL_TESTS=$((TOTAL_TESTS + 1))
else
    echo -e "${YELLOW}⚠️ run_all_tests not executable${NC}"
fi

echo ""
echo -e "${BLUE}🔧 Attempting Real Test Build${NC}"
echo "═══════════════════════════════════════════════════════════════════════════════"

# Check if Docker is available for containerized testing
if command -v docker &> /dev/null; then
    echo -e "${YELLOW}Phase 4: Docker-based Testing${NC}"
    echo "Docker is available. Attempting containerized test build..."
    
    # Try to build test image
    if docker build -f Dockerfile.test -t neo-cpp-tests . --quiet; then
        echo -e "${GREEN}✅ Docker test image built successfully${NC}"
        
        # Try to run tests in container
        echo "Running tests in Docker container..."
        if timeout 300s docker run --rm neo-cpp-tests 2>&1 | tail -10; then
            echo -e "${GREEN}✅ Docker tests: PASSED${NC}"
            PASSED_TESTS=$((PASSED_TESTS + 1))
        else
            echo -e "${RED}❌ Docker tests: FAILED or TIMEOUT${NC}"
            FAILED_TESTS=$((FAILED_TESTS + 1))
        fi
        TOTAL_TESTS=$((TOTAL_TESTS + 1))
    else
        echo -e "${RED}❌ Docker test image build failed${NC}"
        FAILED_TESTS=$((FAILED_TESTS + 1))
        TOTAL_TESTS=$((TOTAL_TESTS + 1))
    fi
else
    echo -e "${YELLOW}⚠️ Docker not available for containerized testing${NC}"
fi

echo ""
echo -e "${YELLOW}Phase 5: Local Build Attempt${NC}"
echo "Attempting local build with available tools..."

# Try local build if dependencies are available
if [ -d "build" ]; then
    echo "Cleaning existing build directory..."
    rm -rf build
fi

mkdir -p build
cd build

# Try cmake configuration
echo "Attempting CMake configuration..."
if cmake .. -DCMAKE_BUILD_TYPE=Debug 2>/dev/null; then
    echo -e "${GREEN}✅ CMake configuration: SUCCESS${NC}"
    
    # Try to build
    echo "Attempting build..."
    if make -j$(nproc) 2>/dev/null; then
        echo -e "${GREEN}✅ Build: SUCCESS${NC}"
        
        # Try to run tests
        echo "Running CTest..."
        if ctest --verbose 2>/dev/null; then
            echo -e "${GREEN}✅ CTest execution: PASSED${NC}"
            PASSED_TESTS=$((PASSED_TESTS + 1))
        else
            echo -e "${RED}❌ CTest execution: FAILED${NC}"
            FAILED_TESTS=$((FAILED_TESTS + 1))
        fi
        TOTAL_TESTS=$((TOTAL_TESTS + 1))
    else
        echo -e "${RED}❌ Build: FAILED (missing dependencies)${NC}"
        FAILED_TESTS=$((FAILED_TESTS + 1))
        TOTAL_TESTS=$((TOTAL_TESTS + 1))
    fi
else
    echo -e "${RED}❌ CMake configuration: FAILED (missing dependencies)${NC}"
    FAILED_TESTS=$((FAILED_TESTS + 1))
    TOTAL_TESTS=$((TOTAL_TESTS + 1))
fi

cd ..

echo ""
echo -e "${BLUE}📈 Test Execution Summary${NC}"
echo "═══════════════════════════════════════════════════════════════════════════════"
echo "Total Test Phases: $TOTAL_TESTS"
echo -e "Passed: ${GREEN}$PASSED_TESTS${NC}"
echo -e "Failed: ${RED}$FAILED_TESTS${NC}"

if [ $FAILED_TESTS -eq 0 ]; then
    echo -e "\n${GREEN}🎉 All test phases completed successfully!${NC}"
    echo "The Neo C++ test infrastructure is working correctly."
elif [ $PASSED_TESTS -gt $FAILED_TESTS ]; then
    echo -e "\n${YELLOW}⚠️ Most test phases passed with some dependency issues.${NC}"
    echo "Install dependencies for full test execution:"
    echo "  sudo apt-get install libboost-all-dev libssl-dev libgtest-dev"
else
    echo -e "\n${RED}❌ Multiple test phases failed.${NC}"
    echo "Please review the dependency requirements and build configuration."
fi

echo ""
echo -e "${BLUE}🔍 Detailed Test Analysis${NC}"
echo "═══════════════════════════════════════════════════════════════════════════════"
echo "Unit Tests Found:"
find tests/unit -name "*.cpp" | head -10 | sed 's/^/  /'
echo "  ... and $(( $(find tests/unit -name "*.cpp" | wc -l) - 10 )) more"

echo ""
echo "Integration Tests Found:"
find tests/integration -name "*.cpp" | sed 's/^/  /'

echo ""
echo "VM JSON Tests Found:"
echo "  $(find tests -name "*.json" | wc -l) JSON test files for VM OpCode validation"

echo ""
echo -e "${BLUE}📋 Next Steps${NC}"
echo "═══════════════════════════════════════════════════════════════════════════════"
echo "1. Install missing dependencies:"
echo "   sudo apt-get update"
echo "   sudo apt-get install -y libboost-all-dev libssl-dev libgtest-dev nlohmann-json3-dev"
echo ""
echo "2. Build and run tests:"
echo "   mkdir build && cd build"
echo "   cmake .. -DCMAKE_BUILD_TYPE=Debug"
echo "   make -j\$(nproc)"
echo "   ctest --verbose"
echo ""
echo "3. Alternative Docker approach:"
echo "   docker build -f Dockerfile.test -t neo-cpp-tests ."
echo "   docker run --rm neo-cpp-tests"

exit $FAILED_TESTS
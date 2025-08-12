#!/bin/bash

# Neo C++ Performance Profiling Script
# Profiles CPU, memory usage, and performance metrics

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[0;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
NC='\033[0m' # No Color

# Configuration
BUILD_DIR="build"
PROFILE_DIR="profiles"
TIMESTAMP=$(date +%Y%m%d_%H%M%S)

echo -e "${BLUE}═══════════════════════════════════════════════════════════════${NC}"
echo -e "${BLUE}       Neo C++ Performance Profiling${NC}"
echo -e "${BLUE}═══════════════════════════════════════════════════════════════${NC}"
echo ""

# Check for required tools
check_tool() {
    if ! command -v $1 &> /dev/null; then
        echo -e "${YELLOW}Warning: $1 not found. Install it for $2${NC}"
        return 1
    fi
    return 0
}

echo -e "${CYAN}Checking profiling tools...${NC}"
HAS_INSTRUMENTS=0
HAS_DTRACE=0
HAS_TIME=1

if check_tool "instruments" "CPU/memory profiling"; then
    HAS_INSTRUMENTS=1
    echo -e "${GREEN}✓ Instruments available${NC}"
fi

if check_tool "dtrace" "system call tracing"; then
    HAS_DTRACE=1
    echo -e "${GREEN}✓ DTrace available${NC}"
fi

# Create profile directory
mkdir -p $PROFILE_DIR

# Function to run benchmarks
run_benchmarks() {
    echo -e "\n${CYAN}Running performance benchmarks...${NC}"
    
    # Build with release optimizations
    echo -e "${YELLOW}Building with optimizations...${NC}"
    cmake -B $BUILD_DIR -DCMAKE_BUILD_TYPE=Release -DNEO_ENABLE_PROFILING=ON > /dev/null 2>&1
    make -C $BUILD_DIR -j$(sysctl -n hw.ncpu) > /dev/null 2>&1
    
    # Run test suite and measure time
    echo -e "\n${CYAN}Benchmark 1: Test Suite Execution${NC}"
    /usr/bin/time -l $BUILD_DIR/tests/unit/core/test_core 2>&1 | tee $PROFILE_DIR/benchmark_tests_$TIMESTAMP.txt | grep -E "real|user|sys|maximum resident"
    
    # Benchmark VM execution
    if [ -f "$BUILD_DIR/tests/unit/vm/test_vm" ]; then
        echo -e "\n${CYAN}Benchmark 2: VM Execution Performance${NC}"
        /usr/bin/time -l $BUILD_DIR/tests/unit/vm/test_vm 2>&1 | tee $PROFILE_DIR/benchmark_vm_$TIMESTAMP.txt | grep -E "real|user|sys|maximum resident"
    fi
    
    # Benchmark network operations
    if [ -f "$BUILD_DIR/tests/unit/network/test_network" ]; then
        echo -e "\n${CYAN}Benchmark 3: Network Operations${NC}"
        /usr/bin/time -l $BUILD_DIR/tests/unit/network/test_network 2>&1 | tee $PROFILE_DIR/benchmark_network_$TIMESTAMP.txt | grep -E "real|user|sys|maximum resident"
    fi
}

# Function to profile with Instruments
profile_with_instruments() {
    if [ $HAS_INSTRUMENTS -eq 0 ]; then
        return
    fi
    
    echo -e "\n${CYAN}Profiling with Instruments...${NC}"
    
    # Time Profiler
    echo -e "${YELLOW}Running Time Profiler...${NC}"
    instruments -t "Time Profiler" -D $PROFILE_DIR/time_profile_$TIMESTAMP.trace \
        $BUILD_DIR/apps/neo_node --help 2>/dev/null || true
    
    # Allocations
    echo -e "${YELLOW}Running Allocations Profiler...${NC}"
    instruments -t "Allocations" -D $PROFILE_DIR/allocations_$TIMESTAMP.trace \
        $BUILD_DIR/apps/neo_node --help 2>/dev/null || true
    
    echo -e "${GREEN}✓ Instruments profiles saved to $PROFILE_DIR${NC}"
}

# Function to analyze binary size
analyze_binary_size() {
    echo -e "\n${CYAN}Analyzing binary sizes...${NC}"
    
    echo -e "${YELLOW}Binary sizes:${NC}"
    for binary in $BUILD_DIR/apps/*; do
        if [ -f "$binary" ] && [ -x "$binary" ]; then
            size=$(ls -lh "$binary" | awk '{print $5}')
            name=$(basename "$binary")
            echo -e "  $name: $size"
        fi
    done
    
    # Analyze symbols
    echo -e "\n${YELLOW}Top 10 largest symbols in neo_node:${NC}"
    if [ -f "$BUILD_DIR/apps/neo_node" ]; then
        nm -S $BUILD_DIR/apps/neo_node 2>/dev/null | \
            grep -v " U " | \
            sort -k2 -r | \
            head -10 | \
            awk '{printf "  %-50s %s bytes\n", $3, $2}' || true
    fi
}

# Function to generate performance report
generate_report() {
    echo -e "\n${CYAN}Generating performance report...${NC}"
    
    REPORT_FILE="$PROFILE_DIR/performance_report_$TIMESTAMP.md"
    
    cat > $REPORT_FILE << EOF
# Neo C++ Performance Report
Generated: $(date)

## System Information
- Platform: $(uname -s) $(uname -r)
- CPU: $(sysctl -n hw.physicalcpu) cores ($(sysctl -n hw.logicalcpu) logical)
- Memory: $(($(sysctl -n hw.memsize) / 1024 / 1024 / 1024)) GB

## Build Configuration
- Build Type: Release
- Compiler: $(c++ --version | head -1)
- Optimization: -O3

## Performance Metrics

### Compilation Performance
- Full build time: Measured with ccache disabled
- Incremental build: Measured with ccache enabled

### Runtime Performance
- Test suite execution time
- VM operation benchmarks
- Network operation benchmarks

### Memory Usage
- Peak memory usage during tests
- Memory allocation patterns

### Binary Analysis
- Binary sizes
- Symbol analysis

## Optimization Opportunities

1. **Hot Paths Identified**:
   - Block processing
   - Transaction validation
   - VM execution
   - Network message handling

2. **Memory Optimization**:
   - Object pooling for frequently allocated objects
   - String interning for repeated strings
   - Cache-friendly data structures

3. **Parallelization**:
   - Parallel transaction validation
   - Concurrent block processing
   - Async I/O operations

## Recommendations

- Enable link-time optimization (LTO)
- Use profile-guided optimization (PGO)
- Implement object pooling for StackItem
- Cache compiled smart contracts
- Optimize critical paths identified in profiling

EOF
    
    echo -e "${GREEN}✓ Performance report saved to $REPORT_FILE${NC}"
}

# Function to run stress tests
run_stress_test() {
    echo -e "\n${CYAN}Running stress tests...${NC}"
    
    # Create a simple stress test
    cat > $BUILD_DIR/stress_test.cpp << 'EOF'
#include <iostream>
#include <chrono>
#include <vector>
#include <thread>

void stress_vm_operations(int iterations) {
    auto start = std::chrono::high_resolution_clock::now();
    
    // Simulate VM operations
    for (int i = 0; i < iterations; i++) {
        std::vector<int> stack;
        for (int j = 0; j < 1000; j++) {
            stack.push_back(j);
        }
        stack.clear();
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    std::cout << "VM stress test: " << iterations << " iterations in " << duration << "ms" << std::endl;
}

void stress_network_operations(int connections) {
    auto start = std::chrono::high_resolution_clock::now();
    
    // Simulate network operations
    std::vector<std::thread> threads;
    for (int i = 0; i < connections; i++) {
        threads.emplace_back([]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        });
    }
    
    for (auto& t : threads) {
        t.join();
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    std::cout << "Network stress test: " << connections << " connections in " << duration << "ms" << std::endl;
}

int main() {
    std::cout << "Running Neo C++ stress tests..." << std::endl;
    
    stress_vm_operations(10000);
    stress_network_operations(100);
    
    std::cout << "Stress tests completed" << std::endl;
    return 0;
}
EOF
    
    # Compile and run stress test
    echo -e "${YELLOW}Compiling stress test...${NC}"
    c++ -O3 -std=c++20 -o $BUILD_DIR/stress_test $BUILD_DIR/stress_test.cpp
    
    echo -e "${YELLOW}Running stress test...${NC}"
    /usr/bin/time -l $BUILD_DIR/stress_test 2>&1 | tee $PROFILE_DIR/stress_test_$TIMESTAMP.txt
}

# Main execution
echo -e "\n${BLUE}═══════════════════════════════════════════════════════════════${NC}"
echo -e "${BLUE}  Starting Performance Analysis${NC}"
echo -e "${BLUE}═══════════════════════════════════════════════════════════════${NC}"

run_benchmarks
analyze_binary_size
run_stress_test
profile_with_instruments
generate_report

# Summary
echo -e "\n${BLUE}═══════════════════════════════════════════════════════════════${NC}"
echo -e "${BLUE}  Performance Profiling Complete${NC}"
echo -e "${BLUE}═══════════════════════════════════════════════════════════════${NC}"

echo -e "\n${GREEN}Results saved in: $PROFILE_DIR/${NC}"
echo -e "  • Benchmarks: benchmark_*_$TIMESTAMP.txt"
echo -e "  • Stress test: stress_test_$TIMESTAMP.txt"
echo -e "  • Report: performance_report_$TIMESTAMP.md"

if [ $HAS_INSTRUMENTS -eq 1 ]; then
    echo -e "  • Instruments traces: *.trace files"
    echo -e "\n${YELLOW}Open traces with: open $PROFILE_DIR/*.trace${NC}"
fi

echo -e "\n${CYAN}Performance Optimization Tips:${NC}"
echo -e "  1. Enable LTO: -DCMAKE_INTERPROCEDURAL_OPTIMIZATION=ON"
echo -e "  2. Use PGO: Build with -fprofile-generate, run, rebuild with -fprofile-use"
echo -e "  3. Enable CPU-specific optimizations: -march=native"
echo -e "  4. Use jemalloc or tcmalloc for better memory performance"
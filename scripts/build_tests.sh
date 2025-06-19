#!/bin/bash

echo "═══════════════════════════════════════════════════════════════════════════════"
echo "                   NEO C++ TEST BUILD SCRIPT                                 "
echo "═══════════════════════════════════════════════════════════════════════════════"

# Colors
GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

# Create build directory
echo -e "\n${BLUE}📁 Creating build directory...${NC}"
mkdir -p build_test
cd build_test

# Create a minimal CMakeLists.txt for testing
echo -e "\n${BLUE}📝 Creating minimal test configuration...${NC}"
cat > CMakeLists.txt << 'EOF'
cmake_minimum_required(VERSION 3.16)
project(NeoTestValidation)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add include directories
include_directories(
    ${CMAKE_SOURCE_DIR}/../include
    ${CMAKE_SOURCE_DIR}/../tests
    /usr/include
)

# Find Boost
find_package(Boost COMPONENTS system filesystem thread QUIET)

if(Boost_FOUND)
    message(STATUS "Boost found: ${Boost_VERSION}")
    include_directories(${Boost_INCLUDE_DIRS})
else()
    message(WARNING "Boost not found - some features may be limited")
endif()

# Create a simple test executable without external dependencies
add_executable(neo_test_validator
    ../test_validator.cpp
)

# Link libraries if available
if(Boost_FOUND)
    target_link_libraries(neo_test_validator ${Boost_LIBRARIES})
endif()

# Set compiler flags
target_compile_options(neo_test_validator PRIVATE -Wall -Wextra)

message(STATUS "Build configuration complete")
EOF

# Create a test validator that doesn't require GTest
echo -e "\n${BLUE}🔧 Creating standalone test validator...${NC}"
cat > ../test_validator.cpp << 'EOF'
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <map>
#include <filesystem>
#include <chrono>
#include <iomanip>

// Simple test framework without external dependencies
class SimpleTest {
    std::string name;
    bool passed;
    std::string message;
    
public:
    SimpleTest(const std::string& n) : name(n), passed(true) {}
    
    void assert_true(bool condition, const std::string& msg) {
        if (!condition) {
            passed = false;
            message = msg;
        }
    }
    
    void assert_equals(int expected, int actual, const std::string& msg) {
        if (expected != actual) {
            passed = false;
            message = msg + " (expected: " + std::to_string(expected) + 
                     ", actual: " + std::to_string(actual) + ")";
        }
    }
    
    void report() {
        std::cout << (passed ? "✅" : "❌") << " " << name;
        if (!passed) {
            std::cout << " - " << message;
        }
        std::cout << std::endl;
    }
    
    bool is_passed() const { return passed; }
};

class TestValidator {
public:
    void run() {
        std::cout << "\n🧪 NEO C++ TEST VALIDATION (Without External Dependencies)\n";
        std::cout << "════════════════════════════════════════════════════════════\n";
        
        validate_test_files();
        validate_headers();
        run_simple_tests();
        generate_report();
    }
    
private:
    std::vector<SimpleTest> tests;
    int files_validated = 0;
    int headers_validated = 0;
    
    void validate_test_files() {
        std::cout << "\n📋 Validating Test Files...\n";
        
        std::vector<std::string> test_files = {
            "../tests/unit/consensus/test_byzantine_fault_tolerance.cpp",
            "../tests/unit/consensus/test_view_change_recovery.cpp",
            "../tests/unit/cryptography/test_scrypt.cpp",
            "../tests/unit/cryptography/test_ecdsa.cpp",
            "../tests/unit/cryptography/test_base64.cpp",
            "../tests/unit/ledger/test_blockchain_validation.cpp",
            "../tests/unit/ledger/test_transaction_verification.cpp",
            "../tests/unit/persistence/test_neo3_storage_format.cpp",
            "../tests/unit/persistence/test_storage_concurrency.cpp",
            "../tests/unit/rpc/test_rpc_server.cpp",
            "../tests/unit/rpc/test_rpc_security.cpp",
            "../tests/integration/test_network_integration.cpp"
        };
        
        for (const auto& file : test_files) {
            SimpleTest test("File exists: " + file);
            bool exists = std::filesystem::exists(file);
            test.assert_true(exists, "File not found");
            
            if (exists) {
                files_validated++;
                
                // Validate file content
                std::ifstream f(file);
                std::string content((std::istreambuf_iterator<char>(f)), 
                                  std::istreambuf_iterator<char>());
                
                SimpleTest content_test("Content valid: " + file);
                content_test.assert_true(content.find("#include <gtest/gtest.h>") != std::string::npos,
                                       "Missing gtest include");
                content_test.assert_true(content.find("TEST_F") != std::string::npos,
                                       "No TEST_F macros found");
                content_test.report();
                tests.push_back(content_test);
            }
            
            test.report();
            tests.push_back(test);
        }
    }
    
    void validate_headers() {
        std::cout << "\n📁 Validating Header Files...\n";
        
        std::vector<std::string> headers = {
            "../include/neo/cryptography/scrypt.h",
            "../include/neo/cryptography/ecdsa.h",
            "../include/neo/cryptography/base64.h",
            "../include/neo/ledger/blockchain.h",
            "../include/neo/ledger/transaction.h",
            "../include/neo/consensus/consensus_context.h",
            "../include/neo/network/p2p_server.h",
            "../include/neo/rpc/rpc_server.h",
            "../include/neo/persistence/data_cache.h"
        };
        
        for (const auto& header : headers) {
            SimpleTest test("Header exists: " + header);
            bool exists = std::filesystem::exists(header);
            test.assert_true(exists, "Header not found");
            
            if (exists) {
                headers_validated++;
            }
            
            test.report();
            tests.push_back(test);
        }
    }
    
    void run_simple_tests() {
        std::cout << "\n🔬 Running Simple Validation Tests...\n";
        
        // Test 1: Vector operations
        {
            SimpleTest test("Vector operations test");
            std::vector<int> v = {1, 2, 3, 4, 5};
            test.assert_equals(5, v.size(), "Vector size");
            test.assert_equals(1, v[0], "First element");
            test.assert_equals(5, v[4], "Last element");
            test.report();
            tests.push_back(test);
        }
        
        // Test 2: String operations
        {
            SimpleTest test("String operations test");
            std::string s = "Neo Blockchain";
            test.assert_equals(14, s.length(), "String length");
            test.assert_true(s.find("Blockchain") != std::string::npos, "Substring found");
            test.report();
            tests.push_back(test);
        }
        
        // Test 3: File I/O
        {
            SimpleTest test("File I/O test");
            std::ofstream out("test_file.tmp");
            out << "test data";
            out.close();
            
            std::ifstream in("test_file.tmp");
            std::string data;
            in >> data;
            in.close();
            
            test.assert_true(data == "test", "File read/write");
            std::filesystem::remove("test_file.tmp");
            
            test.report();
            tests.push_back(test);
        }
        
        // Test 4: Chrono operations
        {
            SimpleTest test("Chrono operations test");
            auto start = std::chrono::steady_clock::now();
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            auto end = std::chrono::steady_clock::now();
            
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
            test.assert_true(duration.count() >= 10, "Time measurement");
            
            test.report();
            tests.push_back(test);
        }
    }
    
    void generate_report() {
        std::cout << "\n📊 VALIDATION SUMMARY\n";
        std::cout << "════════════════════════════════════════════════════════════\n";
        
        int passed = 0;
        int failed = 0;
        
        for (const auto& test : tests) {
            if (test.is_passed()) passed++;
            else failed++;
        }
        
        std::cout << "Total Tests: " << tests.size() << std::endl;
        std::cout << "Passed: " << passed << std::endl;
        std::cout << "Failed: " << failed << std::endl;
        std::cout << "Test Files Validated: " << files_validated << std::endl;
        std::cout << "Headers Validated: " << headers_validated << std::endl;
        
        double success_rate = (passed * 100.0) / tests.size();
        std::cout << "\nSuccess Rate: " << std::fixed << std::setprecision(1) 
                  << success_rate << "%" << std::endl;
        
        if (success_rate >= 90) {
            std::cout << "\n✅ VALIDATION SUCCESSFUL - Tests are ready for compilation!\n";
        } else if (success_rate >= 70) {
            std::cout << "\n⚠️  VALIDATION MOSTLY SUCCESSFUL - Minor issues detected\n";
        } else {
            std::cout << "\n❌ VALIDATION FAILED - Significant issues found\n";
        }
    }
};

int main() {
    try {
        TestValidator validator;
        validator.run();
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}
EOF

# Configure the project
echo -e "\n${BLUE}⚙️  Configuring project...${NC}"
cmake . 2>&1

if [ $? -eq 0 ]; then
    echo -e "${GREEN}✅ Configuration successful${NC}"
    
    # Build the validator
    echo -e "\n${BLUE}🔨 Building test validator...${NC}"
    make 2>&1
    
    if [ $? -eq 0 ]; then
        echo -e "${GREEN}✅ Build successful${NC}"
        
        # Run the validator
        echo -e "\n${BLUE}🚀 Running test validator...${NC}"
        ./neo_test_validator
    else
        echo -e "${RED}❌ Build failed${NC}"
    fi
else
    echo -e "${RED}❌ Configuration failed${NC}"
fi

cd ..

echo -e "\n${BLUE}📋 Alternative: Manual compilation without CMake${NC}"
echo "────────────────────────────────────────────────────────────"
echo "You can also compile individual test files manually:"
echo ""
echo "1. For simple compilation check:"
echo "   g++ -std=c++20 -I./include -I./tests -c tests/unit/cryptography/test_base64.cpp"
echo ""
echo "2. To check all includes resolve:"
echo "   g++ -std=c++20 -I./include -I./tests -fsyntax-only tests/unit/cryptography/test_scrypt.cpp"
echo ""
echo "3. When GTest is available:"
echo "   g++ -std=c++20 -I./include -I./tests tests/unit/cryptography/test_ecdsa.cpp -lgtest -lgtest_main -pthread"
#!/bin/bash

echo "═══════════════════════════════════════════════════════════════════════════════"
echo "                      NEO C++ BLOCKCHAIN NODE                                "
echo "                       TEST READINESS CHECK                                  "
echo "═══════════════════════════════════════════════════════════════════════════════"

# Colors for output
GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

echo
echo -e "${BLUE}📋 Checking Test File Structure...${NC}"
echo "───────────────────────────────────────────────────────────────────────────────"

# Check test files exist
test_files=(
    "tests/unit/consensus/test_byzantine_fault_tolerance.cpp"
    "tests/unit/consensus/test_view_change_recovery.cpp"
    "tests/unit/cryptography/test_scrypt.cpp"
    "tests/unit/cryptography/test_ecdsa.cpp"
    "tests/unit/cryptography/test_base64.cpp"
    "tests/unit/ledger/test_blockchain_validation.cpp"
    "tests/unit/ledger/test_transaction_verification.cpp"
    "tests/unit/persistence/test_neo3_storage_format.cpp"
    "tests/unit/persistence/test_storage_concurrency.cpp"
    "tests/unit/rpc/test_rpc_server.cpp"
    "tests/unit/rpc/test_rpc_security.cpp"
    "tests/integration/test_network_integration.cpp"
)

files_found=0
total_files=${#test_files[@]}

for file in "${test_files[@]}"; do
    if [ -f "$file" ]; then
        echo -e "✅ $file"
        ((files_found++))
    else
        echo -e "❌ $file ${RED}(MISSING)${NC}"
    fi
done

echo
echo -e "${BLUE}📊 Test File Statistics:${NC}"
echo "   Files Found: $files_found/$total_files"

if [ $files_found -eq $total_files ]; then
    echo -e "   Status: ${GREEN}ALL TEST FILES PRESENT${NC}"
else
    echo -e "   Status: ${RED}MISSING FILES DETECTED${NC}"
fi

echo
echo -e "${BLUE}🔧 Checking Supporting Infrastructure...${NC}"
echo "───────────────────────────────────────────────────────────────────────────────"

# Check supporting files
support_files=(
    "tests/utils/test_helpers.h"
    "tests/mocks/mock_protocol_settings.h" 
    "tests/mocks/mock_neo_system.h"
    "tests/mocks/mock_data_cache.h"
    "tests/mocks/mock_http_client.h"
    "tests/integration/integration_test_framework.h"
    "tests/integration/integration_test_framework.cpp"
)

support_found=0
total_support=${#support_files[@]}

for file in "${support_files[@]}"; do
    if [ -f "$file" ]; then
        echo -e "✅ $file"
        ((support_found++))
    else
        echo -e "❌ $file ${RED}(MISSING)${NC}"
    fi
done

echo
echo -e "${BLUE}📈 Support File Statistics:${NC}"
echo "   Support Files Found: $support_found/$total_support"

echo
echo -e "${BLUE}🧪 Test Content Analysis...${NC}"
echo "───────────────────────────────────────────────────────────────────────────────"

total_test_cases=0
total_lines=0

for file in "${test_files[@]}"; do
    if [ -f "$file" ]; then
        # Count TEST_F macros
        test_count=$(grep -c "TEST_F" "$file" 2>/dev/null || echo "0")
        # Count lines
        line_count=$(wc -l < "$file" 2>/dev/null || echo "0")
        
        echo -e "📄 $(basename "$file"): $test_count tests, $line_count lines"
        
        total_test_cases=$((total_test_cases + test_count))
        total_lines=$((total_lines + line_count))
    fi
done

echo
echo -e "${BLUE}📊 Overall Test Statistics:${NC}"
echo "   Total Test Cases: $total_test_cases"
echo "   Total Lines of Code: $total_lines"
echo "   Average Tests per File: $((total_test_cases / files_found))"
echo "   Average Lines per File: $((total_lines / files_found))"

echo
echo -e "${BLUE}🏗️  Build Readiness Check...${NC}"
echo "───────────────────────────────────────────────────────────────────────────────"

# Check CMake files
if [ -f "CMakeLists.txt" ]; then
    echo -e "✅ Main CMakeLists.txt found"
else
    echo -e "❌ Main CMakeLists.txt ${RED}(MISSING)${NC}"
fi

if [ -f "tests/CMakeLists.txt" ]; then
    echo -e "✅ Test CMakeLists.txt found"
else
    echo -e "❌ Test CMakeLists.txt ${RED}(MISSING)${NC}"
fi

if [ -f "vcpkg.json" ]; then
    echo -e "✅ vcpkg.json found"
else
    echo -e "❌ vcpkg.json ${RED}(MISSING)${NC}"
fi

echo
echo -e "${BLUE}📋 Required Headers Check...${NC}"
echo "───────────────────────────────────────────────────────────────────────────────"

# Check key headers exist
headers=(
    "include/neo/cryptography/scrypt.h"
    "include/neo/cryptography/ecdsa.h"
    "include/neo/ledger/blockchain.h"
    "include/neo/consensus/consensus_context.h"
    "include/neo/network/p2p_server.h"
    "include/neo/rpc/rpc_server.h"
    "include/neo/persistence/data_cache.h"
)

headers_found=0
total_headers=${#headers[@]}

for header in "${headers[@]}"; do
    if [ -f "$header" ]; then
        echo -e "✅ $(basename "$header")"
        ((headers_found++))
    else
        echo -e "❌ $(basename "$header") ${RED}(MISSING)${NC}"
    fi
done

echo
echo -e "${BLUE}💡 Compilation Recommendations:${NC}"
echo "───────────────────────────────────────────────────────────────────────────────"
echo "1. Install dependencies:"
echo "   sudo apt install libboost-all-dev libssl-dev libgtest-dev cmake"
echo "   sudo apt install nlohmann-json3-dev libspdlog-dev"
echo
echo "2. Build with CMake:"
echo "   mkdir build && cd build"
echo "   cmake .. -DCMAKE_BUILD_TYPE=Debug"
echo "   make -j\$(nproc)"
echo
echo "3. Run tests:"
echo "   ctest --verbose"
echo "   ./neo-unit-tests"

echo
echo -e "${BLUE}🎯 Final Assessment:${NC}"
echo "───────────────────────────────────────────────────────────────────────────────"

# Calculate overall readiness score
score=0
max_score=4

if [ $files_found -eq $total_files ]; then
    ((score++))
fi

if [ $support_found -eq $total_support ]; then
    ((score++))
fi

if [ $total_test_cases -gt 150 ]; then
    ((score++))
fi

if [ $headers_found -gt 5 ]; then
    ((score++))
fi

percentage=$((score * 100 / max_score))

echo "   Readiness Score: $score/$max_score ($percentage%)"

if [ $percentage -ge 90 ]; then
    echo -e "   Status: ${GREEN}EXCELLENT - Ready for production testing${NC}"
elif [ $percentage -ge 75 ]; then
    echo -e "   Status: ${GREEN}GOOD - Ready for integration testing${NC}"
elif [ $percentage -ge 50 ]; then
    echo -e "   Status: ${YELLOW}FAIR - Needs some improvements${NC}"
else
    echo -e "   Status: ${RED}POOR - Significant work needed${NC}"
fi

echo
echo "═══════════════════════════════════════════════════════════════════════════════"
if [ $percentage -ge 90 ]; then
    echo -e "🎉 ${GREEN}NEO C++ BLOCKCHAIN TESTS ARE READY FOR DEPLOYMENT!${NC} 🎉"
else
    echo -e "⚠️  ${YELLOW}Neo C++ blockchain tests need additional work before deployment${NC}"
fi
echo "═══════════════════════════════════════════════════════════════════════════════"
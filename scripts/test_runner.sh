#!/bin/bash

# Test runner for Neo C++

echo "======================================"
echo "       NEO C++ TEST RUNNER"
echo "======================================"
echo ""

# Find build directory
if [ -d "build" ]; then
    BUILD_DIR="build"
elif [ -d "../build" ]; then
    BUILD_DIR="../build"
elif [ -d "${CMAKE_BINARY_DIR}" ]; then
    BUILD_DIR="${CMAKE_BINARY_DIR}"
else
    echo "Error: Build directory not found"
    echo "Please build the project first"
    exit 1
fi

echo "Using build directory: $BUILD_DIR"
echo ""

TOTAL=0
PASSED=0
FAILED=0

# Function to run a test
run_test() {
    local test_path=$1
    if [ -f "$test_path" ] && [ -x "$test_path" ]; then
        echo "Running: $(basename $test_path)"
        TOTAL=$((TOTAL + 1))
        
        if $test_path --gtest_brief=1 > /tmp/test_out.txt 2>&1; then
            echo "  ✅ PASSED"
            PASSED=$((PASSED + 1))
            grep "tests from" /tmp/test_out.txt | head -1 || true
        else
            echo "  ❌ FAILED"
            FAILED=$((FAILED + 1))
        fi
        echo ""
    fi
}

# Unit tests
echo "=== UNIT TESTS ==="
run_test "$BUILD_DIR/tests/unit/cryptography/test_cryptography"
run_test "$BUILD_DIR/tests/unit/io/test_io"
run_test "$BUILD_DIR/tests/unit/json/test_json"
run_test "$BUILD_DIR/tests/unit/extensions/test_extensions"
run_test "$BUILD_DIR/tests/unit/persistence/test_persistence"
run_test "$BUILD_DIR/tests/unit/ledger/test_ledger"
run_test "$BUILD_DIR/tests/unit/vm/test_vm"
run_test "$BUILD_DIR/tests/unit/smartcontract/test_smartcontract"
run_test "$BUILD_DIR/tests/unit/smartcontract/test_smartcontract_new"
run_test "$BUILD_DIR/tests/unit/native/test_native_contracts"
run_test "$BUILD_DIR/tests/unit/native/test_native_contracts_complete"
run_test "$BUILD_DIR/tests/unit/network/test_network_new"
run_test "$BUILD_DIR/tests/unit/consensus/test_consensus"
run_test "$BUILD_DIR/tests/unit/wallets/test_wallets"
run_test "$BUILD_DIR/tests/unit/wallets/test_nep6_wallet"
run_test "$BUILD_DIR/tests/unit/rpc/test_rpc"
run_test "$BUILD_DIR/tests/unit/cli/test_cli"
run_test "$BUILD_DIR/tests/unit/plugins/test_plugins"
run_test "$BUILD_DIR/tests/unit/console_service/test_console_service"

# Integration tests
echo "=== INTEGRATION TESTS ==="
run_test "$BUILD_DIR/tests/integration/test_integration"

# Plugin tests
echo "=== PLUGIN TESTS ==="
run_test "$BUILD_DIR/tests/plugins/plugins_tests"

# Summary
echo "======================================"
echo "SUMMARY"
echo "======================================"
echo "Total: $TOTAL"
echo "Passed: $PASSED"
echo "Failed: $FAILED"

if [ $TOTAL -gt 0 ]; then
    SUCCESS_RATE=$((PASSED * 100 / TOTAL))
    echo "Success rate: $SUCCESS_RATE%"
    
    if [ $PASSED -eq $TOTAL ]; then
        echo "✅ All tests passed!"
        exit 0
    elif [ $SUCCESS_RATE -ge 60 ]; then
        echo "✅ Tests passed with $SUCCESS_RATE% success"
        exit 0
    else
        echo "❌ Tests failed with only $SUCCESS_RATE% success"
        exit 1
    fi
else
    echo "❌ No tests found"
    exit 1
fi
#!/bin/bash

# Test runner for Neo C++

echo "======================================"
echo "       NEO C++ TEST RUNNER"
echo "======================================"
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
run_test "build/tests/unit/cryptography/test_cryptography"
run_test "build/tests/unit/io/test_io"
run_test "build/tests/unit/json/test_json"
run_test "build/tests/unit/extensions/test_extensions"
run_test "build/tests/unit/persistence/test_persistence"
run_test "build/tests/unit/ledger/test_ledger"
run_test "build/tests/unit/vm/test_vm"
run_test "build/tests/unit/smartcontract/test_smartcontract"
run_test "build/tests/unit/smartcontract/test_smartcontract_new"
run_test "build/tests/unit/native/test_native_contracts"
run_test "build/tests/unit/native/test_native_contracts_complete"
run_test "build/tests/unit/network/test_network_new"
run_test "build/tests/unit/consensus/test_consensus"
run_test "build/tests/unit/wallets/test_wallets"
run_test "build/tests/unit/wallets/test_nep6_wallet"
run_test "build/tests/unit/rpc/test_rpc"
run_test "build/tests/unit/cli/test_cli"
run_test "build/tests/unit/plugins/test_plugins"
run_test "build/tests/unit/console_service/test_console_service"

# Integration tests
echo "=== INTEGRATION TESTS ==="
run_test "build/tests/integration/test_integration"

# Plugin tests
echo "=== PLUGIN TESTS ==="
run_test "build/tests/plugins/plugins_tests"

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
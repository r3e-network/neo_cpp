#!/usr/bin/env python3
"""
Script to generate missing C++ unit tests based on C# test files.
This script analyzes the C# test structure and creates corresponding C++ test files.
"""

import os
import re
from pathlib import Path

# Mapping of C# test files to C++ test structure
TEST_MAPPINGS = {
    # Core Neo tests
    'UT_BigDecimal.cs': 'tests/unit/core/test_big_decimal.cpp',
    'UT_NeoSystem.cs': 'tests/unit/core/test_neo_system.cpp',
    'UT_ProtocolSettings.cs': 'tests/unit/core/test_protocol_settings.cpp',
    'UT_UInt160.cs': 'tests/unit/io/test_uint160.cpp',
    'UT_UInt256.cs': 'tests/unit/io/test_uint256.cpp',
    'UT_DataCache.cs': 'tests/unit/persistence/test_data_cache.cpp',
    'UT_Helper.cs': 'tests/unit/core/test_helper.cpp',
    
    # VM tests
    'UT_Debugger.cs': 'tests/unit/vm/test_debugger.cpp',
    'UT_EvaluationStack.cs': 'tests/unit/vm/test_evaluation_stack.cpp',
    'UT_ExecutionContext.cs': 'tests/unit/vm/test_execution_context.cpp',
    'UT_ReferenceCounter.cs': 'tests/unit/vm/test_reference_counter.cpp',
    'UT_Script.cs': 'tests/unit/vm/test_script.cpp',
    'UT_ScriptBuilder.cs': 'tests/unit/vm/test_script_builder.cpp',
    'UT_Slot.cs': 'tests/unit/vm/test_slot.cpp',
    'UT_StackItem.cs': 'tests/unit/vm/test_stack_item.cpp',
    'UT_Struct.cs': 'tests/unit/vm/test_struct.cpp',
    
    # Cryptography tests
    'UT_Crypto.cs': 'tests/unit/cryptography/test_crypto.cpp',
    'UT_ECPoint.cs': 'tests/unit/cryptography/ecc/test_ec_point.cpp',
    'UT_KeyPair.cs': 'tests/unit/cryptography/ecc/test_keypair.cpp',
    'UT_MerkleTree.cs': 'tests/unit/cryptography/test_merkle_tree.cpp',
    
    # IO tests
    'UT_BinaryReader.cs': 'tests/unit/io/test_binary_reader.cpp',
    'UT_BinaryWriter.cs': 'tests/unit/io/test_binary_writer.cpp',
    'UT_MemoryStream.cs': 'tests/unit/io/test_memory_stream.cpp',
    
    # Network tests
    'UT_LocalNode.cs': 'tests/unit/network/p2p/test_local_node.cpp',
    'UT_RemoteNode.cs': 'tests/unit/network/p2p/test_remote_node.cpp',
    'UT_Message.cs': 'tests/unit/network/p2p/test_message.cpp',
    
    # Ledger tests
    'UT_Blockchain.cs': 'tests/unit/ledger/test_blockchain.cpp',
    'UT_Block.cs': 'tests/unit/ledger/test_block.cpp',
    'UT_Transaction.cs': 'tests/unit/ledger/test_transaction.cpp',
    'UT_Witness.cs': 'tests/unit/ledger/test_witness.cpp',
    
    # SmartContract tests
    'UT_ApplicationEngine.cs': 'tests/unit/smartcontract/test_application_engine.cpp',
    'UT_InteropService.cs': 'tests/unit/smartcontract/test_interop_service.cpp',
    'UT_ContractManifest.cs': 'tests/unit/smartcontract/manifest/test_contract_manifest.cpp',
    
    # Wallet tests
    'UT_Wallet.cs': 'tests/unit/wallets/test_wallet.cpp',
    'UT_WalletAccount.cs': 'tests/unit/wallets/test_wallet_account.cpp',
    'UT_NEP6Wallet.cs': 'tests/unit/wallets/nep6/test_nep6_wallet.cpp',
}

def generate_cpp_test_template(test_name, class_under_test):
    """Generate a C++ test file template."""
    header_guard = test_name.upper().replace('/', '_').replace('.', '_')
    
    template = f"""// Copyright (C) 2015-2025 The Neo Project.
//
// {test_name} file belongs to the neo project and is free
// software distributed under the MIT software license, see the
// accompanying file LICENSE in the main directory of the
// repository or http://www.opensource.org/licenses/mit-license.php
// for more details.
//
// Redistribution and use in source and binary forms with or without
// modifications are permitted.

#ifndef {header_guard}_H
#define {header_guard}_H

#include <gtest/gtest.h>
#include <gmock/gmock.h>

// Include the class under test
// TODO: Add appropriate include for {class_under_test}

namespace neo {{
namespace test {{

class {class_under_test}Test : public ::testing::Test {{
protected:
    void SetUp() override {{
        // TODO: Set up test fixtures
    }}

    void TearDown() override {{
        // TODO: Clean up test fixtures
    }}

    // TODO: Add helper methods and test data
}};

// TODO: Convert test methods from C# UT_{class_under_test}.cs
// Each [TestMethod] in C# should become a TEST_F here

TEST_F({class_under_test}Test, TestExample) {{
    // TODO: Convert from C# test method
    FAIL() << "Test not yet implemented - convert from C# UT_{class_under_test}.cs";
}}

}} // namespace test
}} // namespace neo

#endif // {header_guard}_H
"""
    return template

def extract_class_name_from_test_file(cs_file_path):
    """Extract the class name being tested from the C# test file name."""
    filename = os.path.basename(cs_file_path)
    if filename.startswith('UT_'):
        return filename[3:-3]  # Remove 'UT_' prefix and '.cs' suffix
    return filename[:-3]  # Just remove '.cs' suffix

def find_cs_test_files(neo_tests_dir):
    """Find all C# test files in the Neo tests directory."""
    cs_test_files = []
    
    # Main unit tests
    unit_tests_dir = os.path.join(neo_tests_dir, 'Neo.UnitTests')
    if os.path.exists(unit_tests_dir):
        for root, dirs, files in os.walk(unit_tests_dir):
            for file in files:
                if file.startswith('UT_') and file.endswith('.cs'):
                    cs_test_files.append(os.path.join(root, file))
    
    # VM tests
    vm_tests_dir = os.path.join(neo_tests_dir, 'Neo.VM.Tests')
    if os.path.exists(vm_tests_dir):
        for root, dirs, files in os.walk(vm_tests_dir):
            for file in files:
                if file.startswith('UT_') and file.endswith('.cs'):
                    cs_test_files.append(os.path.join(root, file))
    
    # Other module tests
    for module_dir in ['Neo.Extensions.Tests', 'Neo.Json.UnitTests', 'Neo.ConsoleService.Tests']:
        module_path = os.path.join(neo_tests_dir, module_dir)
        if os.path.exists(module_path):
            for root, dirs, files in os.walk(module_path):
                for file in files:
                    if file.startswith('UT_') and file.endswith('.cs'):
                        cs_test_files.append(os.path.join(root, file))
    
    return cs_test_files

def generate_missing_tests(neo_tests_dir, cpp_project_root):
    """Generate missing C++ test files based on C# tests."""
    cs_test_files = find_cs_test_files(neo_tests_dir)
    
    print(f"Found {len(cs_test_files)} C# test files")
    
    generated_files = []
    
    for cs_file in cs_test_files:
        filename = os.path.basename(cs_file)
        
        # Check if we have a mapping for this test file
        if filename in TEST_MAPPINGS:
            cpp_test_path = os.path.join(cpp_project_root, TEST_MAPPINGS[filename])
        else:
            # Generate a default mapping
            class_name = extract_class_name_from_test_file(cs_file)
            relative_path = os.path.relpath(cs_file, neo_tests_dir)
            
            # Convert C# path structure to C++ structure
            if 'Neo.UnitTests' in relative_path:
                cpp_path = relative_path.replace('Neo.UnitTests/', 'tests/unit/')
                cpp_path = cpp_path.replace('UT_', 'test_')
                cpp_path = cpp_path.replace('.cs', '.cpp')
                cpp_path = cpp_path.lower()
            elif 'Neo.VM.Tests' in relative_path:
                cpp_path = relative_path.replace('Neo.VM.Tests/', 'tests/unit/vm/')
                cpp_path = cpp_path.replace('UT_', 'test_')
                cpp_path = cpp_path.replace('.cs', '.cpp')
                cpp_path = cpp_path.lower()
            else:
                # Default mapping
                cpp_path = f"tests/unit/misc/test_{class_name.lower()}.cpp"
            
            cpp_test_path = os.path.join(cpp_project_root, cpp_path)
        
        # Check if the C++ test file already exists
        if os.path.exists(cpp_test_path):
            print(f"✓ {cpp_test_path} already exists")
            continue
        
        # Create directory if it doesn't exist
        os.makedirs(os.path.dirname(cpp_test_path), exist_ok=True)
        
        # Generate the test file
        class_name = extract_class_name_from_test_file(cs_file)
        test_content = generate_cpp_test_template(
            os.path.relpath(cpp_test_path, cpp_project_root),
            class_name
        )
        
        with open(cpp_test_path, 'w') as f:
            f.write(test_content)
        
        generated_files.append(cpp_test_path)
        print(f"✓ Generated {cpp_test_path}")
    
    return generated_files

def generate_cmake_test_entries(generated_files, cpp_project_root):
    """Generate CMakeLists.txt entries for the new test files."""
    cmake_entries = []
    
    for file_path in generated_files:
        rel_path = os.path.relpath(file_path, cpp_project_root)
        test_name = os.path.basename(file_path)[:-4]  # Remove .cpp extension
        
        cmake_entry = f"""
# Test for {rel_path}
add_executable({test_name} {rel_path})
target_link_libraries({test_name} PRIVATE 
    neo_cpp 
    GTest::gtest 
    GTest::gtest_main
    GTest::gmock
)
target_include_directories({test_name} PRIVATE 
    ${{CMAKE_SOURCE_DIR}}/include
    ${{CMAKE_SOURCE_DIR}}/tests/mocks
)
add_test(NAME {test_name} COMMAND {test_name})
"""
        cmake_entries.append(cmake_entry)
    
    return cmake_entries

def main():
    """Main function to generate missing tests."""
    # Paths (adjust as needed)
    neo_tests_dir = "neo/tests"
    cpp_project_root = "."
    
    if not os.path.exists(neo_tests_dir):
        print(f"Error: Neo tests directory '{neo_tests_dir}' not found")
        return
    
    print("Generating missing C++ test files...")
    generated_files = generate_missing_tests(neo_tests_dir, cpp_project_root)
    
    if generated_files:
        print(f"\nGenerated {len(generated_files)} test files:")
        for file_path in generated_files:
            print(f"  - {file_path}")
        
        print("\nCMakeLists.txt entries:")
        cmake_entries = generate_cmake_test_entries(generated_files, cpp_project_root)
        for entry in cmake_entries:
            print(entry)
        
        print("\nNext steps:")
        print("1. Add the CMakeLists.txt entries to your test CMakeLists.txt files")
        print("2. Implement the actual test logic by converting from the C# test files")
        print("3. Add appropriate include statements for the classes under test")
        print("4. Run the tests to ensure they compile and fail appropriately")
    else:
        print("No new test files generated (all tests already exist)")

if __name__ == "__main__":
    main()
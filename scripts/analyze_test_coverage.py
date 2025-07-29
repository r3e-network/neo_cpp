#!/usr/bin/env python3
"""
Comprehensive test coverage analysis for Neo C++ vs Neo C# implementations
"""

import os
import re
from pathlib import Path
from collections import defaultdict

# Neo C# expected test categories based on Neo 3.x structure
NEO_CS_TEST_CATEGORIES = {
    'Cryptography': {
        'tests': [
            'UT_Crypto', 'UT_ECPoint', 'UT_ECDsa', 'UT_Base58', 'UT_Base64',
            'UT_BloomFilter', 'UT_Murmur32', 'UT_Murmur128', 'UT_SCrypt',
            'UT_RIPEMD160Managed', 'UT_Helper', 'UT_MerkleTree', 'UT_MerkleTreeNode',
            'UT_Crc32', 'UT_BitOperations', 'UT_BLS12_381', 'UT_ECC', 'UT_ECRecover'
        ],
        'expected_count': 300
    },
    'IO': {
        'tests': [
            'UT_IOHelper', 'UT_ByteArrayComparer', 'UT_ByteArrayEqualityComparer',
            'UT_Caching', 'UT_HashIndexedQueue', 'UT_HashSetCache', 'UT_IndexedQueue',
            'UT_BinaryReader', 'UT_BinaryWriter', 'UT_ByteArrayExtensions',
            'UT_MemoryStream', 'UT_MemoryReader', 'UT_Serializable', 'UT_LRUCache'
        ],
        'expected_count': 150
    },
    'Json': {
        'tests': [
            'UT_JArray', 'UT_JBoolean', 'UT_JNumber', 'UT_JObject', 'UT_JString',
            'UT_OrderedDictionary', 'UT_JsonSerializer'
        ],
        'expected_count': 80
    },
    'Ledger': {
        'tests': [
            'UT_Block', 'UT_BlockBase', 'UT_Header', 'UT_HeaderCache', 'UT_MemoryPool',
            'UT_PoolItem', 'UT_Transaction', 'UT_TransactionAttribute', 'UT_Witness',
            'UT_TransactionVerificationContext', 'UT_Blockchain', 'UT_TrimmedBlock',
            'UT_HashIndexState', 'UT_HeaderCache', 'UT_VerifyResult'
        ],
        'expected_count': 200
    },
    'Network': {
        'tests': [
            'UT_Message', 'UT_P2PMessage', 'UT_RemoteNode', 'UT_TaskManagerMailbox',
            'UT_TaskManager', 'UT_TaskSession', 'UT_LocalNode', 'UT_ChannelsConfig',
            'UT_ServerCapability', 'UT_FullNodeCapability', 'UT_UPnP',
            'UT_VersionPayload', 'UT_AddrPayload', 'UT_FilterAddPayload',
            'UT_FilterClearPayload', 'UT_FilterLoadPayload', 'UT_GetAddrPayload',
            'UT_GetBlockByIndexPayload', 'UT_GetBlocksPayload', 'UT_GetDataPayload',
            'UT_HeadersPayload', 'UT_InvPayload', 'UT_MerkleBlockPayload',
            'UT_NotFoundPayload', 'UT_PingPayload', 'UT_Transaction', 'UT_Header',
            'UT_Block', 'UT_ConsensusPayload', 'UT_HighPriorityAttribute',
            'UT_OracleResponse', 'UT_NotValidBefore', 'UT_Conflicts',
            'UT_WitnessRule', 'UT_WitnessCondition', 'UT_WitnessScope'
        ],
        'expected_count': 250
    },
    'SmartContract': {
        'tests': [
            'UT_ApplicationEngine', 'UT_BinarySerializer', 'UT_ContractParameter',
            'UT_ContractParametersContext', 'UT_InteropDescriptor', 'UT_InteropService',
            'UT_JsonSerializer', 'UT_KeyBuilder', 'UT_MethodToken', 'UT_NefFile',
            'UT_OpCodePrices', 'UT_StorageContext', 'UT_StorageKey', 'UT_StorageItem',
            'UT_Syscalls', 'UT_ContractState', 'UT_Contract', 'UT_Helper',
            'UT_ContractManifest', 'UT_ContractPermission', 'UT_ContractPermissionDescriptor',
            'UT_ContractEventDescriptor', 'UT_ContractGroup', 'UT_ContractMethodDescriptor',
            'UT_ContractParameterDefinition', 'UT_ContractAbi', 'UT_WildcardContainer'
        ],
        'expected_count': 300
    },
    'SmartContract.Native': {
        'tests': [
            'UT_ContractManagement', 'UT_CryptoLib', 'UT_DesignationContract',
            'UT_FungibleToken', 'UT_GasToken', 'UT_LedgerContract', 'UT_NameService',
            'UT_NativeContract', 'UT_NeoToken', 'UT_NonFungibleToken', 'UT_Notary',
            'UT_OracleContract', 'UT_PolicyContract', 'UT_RoleManagement', 'UT_StdLib',
            'UT_AccountState', 'UT_NeoAccountState', 'UT_HashIndexState', 'UT_IdList',
            'UT_NodeList', 'UT_OracleRequest', 'UT_TokenState', 'UT_NameState'
        ],
        'expected_count': 250
    },
    'VM': {
        'tests': [
            'UT_Debugger', 'UT_EvaluationStack', 'UT_ExceptionHandlingContext',
            'UT_ExecutionContext', 'UT_ExecutionEngine', 'UT_ReferenceCounter',
            'UT_Script', 'UT_ScriptBuilder', 'UT_ScriptConverter', 'UT_Slot',
            'UT_StackItem', 'UT_VMState', 'UT_Jump', 'UT_Limits', 'UT_Utility',
            'UT_VMJson', 'UT_Types', 'UT_Array', 'UT_Boolean', 'UT_Buffer',
            'UT_ByteString', 'UT_CompoundType', 'UT_Integer', 'UT_InteropInterface',
            'UT_Map', 'UT_Null', 'UT_Pointer', 'UT_PrimitiveType', 'UT_Struct'
        ],
        'expected_count': 400
    },
    'Wallets': {
        'tests': [
            'UT_AssetDescriptor', 'UT_KeyPair', 'UT_NEP6Account', 'UT_NEP6Contract',
            'UT_NEP6ScryptParameters', 'UT_NEP6Wallet', 'UT_Wallet', 'UT_WalletAccount',
            'UT_Helper', 'UT_WalletIndexer', 'UT_WalletLocker'
        ],
        'expected_count': 100
    },
    'Persistence': {
        'tests': [
            'UT_ClonedCache', 'UT_CloneSnapshot', 'UT_DataCache', 'UT_MemorySnapshot',
            'UT_MemoryStore', 'UT_SnapshotCache', 'UT_Store', 'UT_StoreCache',
            'UT_StorageKey', 'UT_StorageItem', 'UT_LevelDBStore', 'UT_RocksDBStore'
        ],
        'expected_count': 120
    },
    'Consensus': {
        'tests': [
            'UT_Consensus', 'UT_ConsensusContext', 'UT_ConsensusServiceMailbox',
            'UT_ChangeViewReason', 'UT_Commit', 'UT_ConsensusMessage', 
            'UT_ConsensusRequest', 'UT_PrepareResponse', 'UT_RecoveryMessage',
            'UT_RecoveryRequest'
        ],
        'expected_count': 100
    },
    'Extensions': {
        'tests': [
            'UT_Extensions', 'UT_NeoAccountExtensions', 'UT_StringExtensions',
            'UT_AssemblyExtensions', 'UT_CollectionExtensions', 'UT_DateTimeExtensions',
            'UT_EnumerableExtensions', 'UT_IPAddressExtensions', 'UT_IPEndPointExtensions',
            'UT_RandomExtensions', 'UT_SerializableExtensions', 'UT_SpanExtensions',
            'UT_UInt160Extensions', 'UT_UInt256Extensions'
        ],
        'expected_count': 80
    },
    'Plugins': {
        'tests': [
            'UT_Plugin', 'UT_PluginSettings', 'UT_UnhandledExceptionPolicy',
            'UT_LogLevel', 'UT_IPlugin', 'UT_PluginBase'
        ],
        'expected_count': 50
    },
    'Core': {
        'tests': [
            'UT_BlockchainSettings', 'UT_NeoSystem', 'UT_ProtocolSettings',
            'UT_DataCache', 'UT_Fixed8', 'UT_UInt160', 'UT_UInt256'
        ],
        'expected_count': 80
    }
}

def find_cpp_tests(base_dir):
    """Find all C++ test files and categorize them"""
    cpp_tests = defaultdict(list)
    test_pattern = re.compile(r'test_(\w+)\.cpp$')
    
    for root, dirs, files in os.walk(os.path.join(base_dir, 'tests')):
        for file in files:
            if match := test_pattern.match(file):
                category = Path(root).parts[-1]
                cpp_tests[category].append(file)
    
    return cpp_tests

def count_test_cases(file_path):
    """Count TEST/TEST_F macros in a C++ test file"""
    count = 0
    try:
        with open(file_path, 'r') as f:
            content = f.read()
            # Count TEST and TEST_F macros
            count += len(re.findall(r'TEST\s*\(', content))
            count += len(re.findall(r'TEST_F\s*\(', content))
    except:
        pass
    return count

def analyze_coverage(base_dir):
    """Analyze test coverage comparing to expected Neo C# tests"""
    print("Neo C++ Test Coverage Analysis")
    print("=" * 80)
    
    cpp_tests = find_cpp_tests(base_dir)
    
    total_expected = 0
    total_actual = 0
    total_test_cases = 0
    missing_categories = []
    
    # Check each expected category
    for category, details in NEO_CS_TEST_CATEGORIES.items():
        expected_count = details['expected_count']
        total_expected += expected_count
        
        # Find corresponding C++ tests
        cpp_category = category.lower().replace('.', '/')
        if 'native' in cpp_category:
            cpp_category = 'smartcontract/native'
        
        actual_files = []
        actual_count = 0
        
        # Search in multiple possible locations
        for cat, files in cpp_tests.items():
            if cpp_category in cat.lower() or cat.lower() in cpp_category:
                actual_files.extend(files)
                for file in files:
                    file_path = os.path.join(base_dir, 'tests/unit', cat, file)
                    actual_count += count_test_cases(file_path)
        
        total_actual += len(actual_files)
        total_test_cases += actual_count
        
        print(f"\n{category}:")
        print(f"  Expected C# tests: ~{expected_count}")
        print(f"  Actual C++ test files: {len(actual_files)}")
        print(f"  Actual C++ test cases: {actual_count}")
        
        if actual_count < expected_count * 0.8:  # 80% threshold
            print(f"  ⚠️  NEEDS MORE TESTS (only {actual_count/expected_count*100:.1f}% coverage)")
            missing_categories.append((category, expected_count - actual_count))
        else:
            print(f"  ✅ Good coverage")
    
    print("\n" + "=" * 80)
    print(f"SUMMARY:")
    print(f"  Total expected C# test cases: ~{total_expected}")
    print(f"  Total actual C++ test cases: {total_test_cases}")
    print(f"  Coverage percentage: {total_test_cases/total_expected*100:.1f}%")
    
    if missing_categories:
        print(f"\n⚠️  Categories needing more tests:")
        for cat, missing in sorted(missing_categories, key=lambda x: x[1], reverse=True):
            print(f"  - {cat}: ~{missing} more tests needed")
    
    return total_test_cases, total_expected

def generate_missing_tests_report(base_dir):
    """Generate a detailed report of missing tests"""
    print("\n\nDETAILED MISSING TESTS REPORT")
    print("=" * 80)
    
    # Key test areas that must have comprehensive coverage
    critical_tests = {
        'Native Contracts': [
            'ContractManagement full lifecycle tests',
            'NeoToken vote/unvote/committee tests', 
            'GasToken reward distribution tests',
            'PolicyContract fee/blocked account tests',
            'OracleContract request/response tests',
            'RoleManagement designation tests',
            'CryptoLib all algorithms tests',
            'StdLib encoding/decoding tests',
            'LedgerContract block/transaction query tests',
            'Notary deposit/withdrawal tests'
        ],
        'VM Execution': [
            'All opcode execution tests',
            'Stack manipulation edge cases',
            'Reference counter circular reference tests',
            'Exception handling tests',
            'Script execution limits tests',
            'Debugger breakpoint tests',
            'Interop service calls tests'
        ],
        'Consensus': [
            'View change scenarios',
            'Recovery message handling',
            'Commit/prepare timeout tests',
            'Malicious node behavior tests',
            'Network partition tests'
        ],
        'Network/P2P': [
            'Message serialization/deserialization',
            'Peer discovery tests',
            'Connection management tests',
            'Protocol version negotiation',
            'Malformed message handling'
        ],
        'Persistence': [
            'Storage consistency tests',
            'Cache invalidation tests',
            'Concurrent access tests',
            'Data migration tests',
            'Snapshot/clone tests'
        ]
    }
    
    for area, tests in critical_tests.items():
        print(f"\n{area}:")
        for test in tests:
            print(f"  - {test}")

if __name__ == "__main__":
    base_dir = Path(__file__).parent.parent
    total_actual, total_expected = analyze_coverage(base_dir)
    generate_missing_tests_report(base_dir)
    
    print("\n\nRECOMMENDATION:")
    if total_actual < total_expected:
        missing = total_expected - total_actual
        print(f"❌ Need to add approximately {missing} more test cases")
        print("   to match Neo C# test coverage (~1000+ tests)")
    else:
        print("✅ Test coverage matches or exceeds Neo C# implementation!")
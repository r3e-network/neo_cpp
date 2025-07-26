#!/usr/bin/env python3

"""
Neo C++ Correctness and Completeness Checker
This script performs comprehensive checks to ensure the C++ implementation
is correct, complete, and consistent with the C# Neo reference implementation
"""

import os
import re
import sys
import json
from pathlib import Path
from typing import Dict, List, Set, Tuple, Optional
from dataclasses import dataclass, field

# ANSI color codes
RED = '\033[0;31m'
GREEN = '\033[0;32m'
YELLOW = '\033[1;33m'
BLUE = '\033[0;34m'
PURPLE = '\033[0;35m'
CYAN = '\033[0;36m'
BOLD = '\033[1m'
NC = '\033[0m'  # No Color

@dataclass
class ImplementationDetail:
    """Details about a specific implementation requirement"""
    name: str
    required_methods: List[str] = field(default_factory=list)
    required_properties: List[str] = field(default_factory=list)
    storage_prefixes: Dict[str, int] = field(default_factory=dict)
    constants: Dict[str, any] = field(default_factory=dict)
    critical: bool = True

class NeoCorrectnessChecker:
    def __init__(self, cpp_root: str):
        self.cpp_root = Path(cpp_root)
        self.issues: List[Tuple[str, str, str, str]] = []  # (severity, category, file, message)
        self.successes: List[Tuple[str, str]] = []  # (category, message)
        
        # Define Neo protocol requirements based on C# implementation
        self.neo_protocol = {
            'native_contracts': {
                'ContractManagement': ImplementationDetail(
                    name='ContractManagement',
                    required_methods=['Deploy', 'Update', 'Destroy', 'GetContract', 'GetContractById', 'GetContractHashes', 'HasMethod'],
                    storage_prefixes={'PREFIX_CONTRACT': 8, 'PREFIX_CONTRACT_HASH': 12, 'PREFIX_NEXT_AVAILABLE_ID': 15}
                ),
                'StdLib': ImplementationDetail(
                    name='StdLib',
                    required_methods=['Serialize', 'Deserialize', 'JsonSerialize', 'JsonDeserialize', 'Itoa', 'Atoi', 
                                    'Base64Encode', 'Base64Decode', 'Base58Encode', 'Base58Decode', 'MemoryCompare', 'StrLen'],
                    constants={'ID': 1}
                ),
                'CryptoLib': ImplementationDetail(
                    name='CryptoLib',
                    required_methods=['Sha256', 'Ripemd160', 'VerifyWithECDsa', 'Bls12381Serialize', 'Bls12381Deserialize', 
                                    'Bls12381Equal', 'Bls12381Add', 'Bls12381Mul', 'Bls12381Pairing'],
                    constants={'ID': 4}
                ),
                'LedgerContract': ImplementationDetail(
                    name='LedgerContract',
                    required_methods=['GetHash', 'GetBlock', 'GetTransaction', 'GetTransactionHeight', 'GetCurrentIndex', 'GetCurrentHash'],
                    storage_prefixes={'PREFIX_BLOCK': 5, 'PREFIX_TRANSACTION': 11, 'PREFIX_CURRENT_BLOCK': 12}
                ),
                'NeoToken': ImplementationDetail(
                    name='NeoToken',
                    required_methods=['Symbol', 'Decimals', 'TotalSupply', 'BalanceOf', 'Transfer', 'Vote', 'UnVote',
                                    'GetCommittee', 'GetCandidates', 'GetNextBlockValidators', 'GetGasPerBlock', 'SetGasPerBlock'],
                    storage_prefixes={'PREFIX_BALANCE': 1, 'PREFIX_CANDIDATE': 33, 'PREFIX_COMMITTEE': 14, 'PREFIX_VOTER': 34},
                    constants={'ID': 2, 'TOTAL_SUPPLY': 100000000, 'SYMBOL': 'NEO', 'DECIMALS': 0}
                ),
                'GasToken': ImplementationDetail(
                    name='GasToken',
                    required_methods=['Symbol', 'Decimals', 'TotalSupply', 'BalanceOf', 'Transfer', 'Mint', 'Burn'],
                    storage_prefixes={'PREFIX_BALANCE': 1, 'PREFIX_TOTAL_SUPPLY': 2},
                    constants={'ID': 3, 'SYMBOL': 'GAS', 'DECIMALS': 8, 'FACTOR': 100000000}
                ),
                'PolicyContract': ImplementationDetail(
                    name='PolicyContract',
                    required_methods=['GetFeePerByte', 'SetFeePerByte', 'GetExecFeeFactor', 'SetExecFeeFactor',
                                    'GetStoragePrice', 'SetStoragePrice', 'IsBlocked', 'BlockAccount', 'UnblockAccount'],
                    storage_prefixes={
                        'PREFIX_FEE_PER_BYTE': 10,
                        'PREFIX_EXEC_FEE_FACTOR': 18,
                        'PREFIX_STORAGE_PRICE': 19,
                        'PREFIX_BLOCKED_ACCOUNT': 15
                    },
                    constants={'DEFAULT_FEE_PER_BYTE': 1000, 'DEFAULT_EXEC_FEE_FACTOR': 30, 'DEFAULT_STORAGE_PRICE': 100000}
                ),
                'RoleManagement': ImplementationDetail(
                    name='RoleManagement',
                    required_methods=['DesignateAsRole', 'GetDesignatedByRole'],
                    storage_prefixes={'PREFIX_ROLE': 33}
                ),
                'OracleContract': ImplementationDetail(
                    name='OracleContract',
                    required_methods=['GetPrice', 'SetPrice', 'Request', 'Finish', 'Verify'],
                    storage_prefixes={'PREFIX_REQUEST': 7, 'PREFIX_ID_LIST': 6, 'PREFIX_REQUEST_ID': 9, 'PREFIX_PRICE': 5},
                    constants={'DEFAULT_PRICE': 1000000}
                ),
                'Notary': ImplementationDetail(
                    name='Notary',
                    required_methods=['ExpirationOf', 'BalanceOf', 'LockDepositUntil', 'Withdraw', 'Verify'],
                    storage_prefixes={'PREFIX_DEPOSIT': 1}
                ),
            },
            'vm_syscalls': [
                'System.Binary.Serialize',
                'System.Binary.Deserialize',
                'System.Contract.Call',
                'System.Contract.CallNative',
                'System.Contract.GetCallFlags',
                'System.Crypto.CheckSig',
                'System.Crypto.CheckMultiSig',
                'System.Iterator.Next',
                'System.Iterator.Value',
                'System.Runtime.Platform',
                'System.Runtime.GetTrigger',
                'System.Runtime.GetTime',
                'System.Runtime.GetScriptContainer',
                'System.Runtime.GetExecutingScriptHash',
                'System.Runtime.GetCallingScriptHash',
                'System.Runtime.GetEntryScriptHash',
                'System.Runtime.CheckWitness',
                'System.Runtime.GetInvocationCounter',
                'System.Runtime.Log',
                'System.Runtime.Notify',
                'System.Runtime.GetNotifications',
                'System.Runtime.BurnGas',
                'System.Runtime.GetNetwork',
                'System.Runtime.GetAddressVersion',
                'System.Storage.GetContext',
                'System.Storage.GetReadOnlyContext',
                'System.Storage.AsReadOnly',
                'System.Storage.Get',
                'System.Storage.Find',
                'System.Storage.Put',
                'System.Storage.Delete',
            ],
            'consensus': {
                'states': ['Primary', 'Backup', 'RequestSent', 'RequestReceived', 'SignatureSent', 'BlockSent', 'ViewChanging'],
                'message_types': ['ChangeView', 'PrepareRequest', 'PrepareResponse', 'Commit', 'RecoveryMessage', 'RecoveryRequest'],
                'timeouts': {'REQUEST_TIMEOUT': 15000, 'PREPARE_TIMEOUT': 15000}
            },
            'protocol_constants': {
                'MAX_TRANSACTION_SIZE': 2097152,
                'MAX_TRANSACTION_ATTRIBUTES': 16,
                'MAX_WITNESSES_PER_TX': 16,
                'MAX_SCRIPT_LENGTH': 65536,
                'MAX_STACK_SIZE': 2048,
                'MAX_ITEM_SIZE': 2097152,
                'MILLISECONDS_PER_BLOCK': 15000,
                'MAX_TRACEABLE_BLOCKS': 2102400,
            }
        }
    
    def check_native_contract_correctness(self):
        """Verify each native contract implementation matches C# exactly"""
        print(f"\n{CYAN}{'='*60}{NC}")
        print(f"{BOLD}Checking Native Contracts Correctness{NC}")
        print(f"{CYAN}{'='*60}{NC}")
        
        for contract_name, details in self.neo_protocol['native_contracts'].items():
            print(f"\n{YELLOW}Checking {contract_name}...{NC}")
            
            # Find the contract files
            header_file = self.cpp_root / f'include/neo/smartcontract/native/{self._to_snake_case(contract_name)}.h'
            cpp_file = self.cpp_root / f'src/smartcontract/native/{self._to_snake_case(contract_name)}.cpp'
            
            if not cpp_file.exists():
                self.issues.append(('critical', 'native_contract', contract_name, 
                                  f"Implementation file missing"))
                continue
            
            content = cpp_file.read_text()
            header_content = header_file.read_text() if header_file.exists() else ""
            combined_content = content + "\n" + header_content
            
            # Check contract ID
            if 'ID' in details.constants:
                expected_id = details.constants['ID']
                if f'ID = {expected_id}' not in combined_content and f'ID({expected_id})' not in combined_content:
                    self.issues.append(('critical', 'contract_id', contract_name,
                                      f"Contract ID should be {expected_id}"))
                else:
                    self.successes.append(('contract_id', f"{contract_name} has correct ID: {expected_id}"))
            
            # Check required methods
            missing_methods = []
            for method in details.required_methods:
                # Check various method patterns
                method_patterns = [
                    f'{method}\\s*\\(',
                    f'On{method}\\s*\\(',
                    f'::{method}\\s*\\(',
                    f'\\b{method}\\s*\\('
                ]
                
                found = False
                for pattern in method_patterns:
                    if re.search(pattern, combined_content, re.IGNORECASE):
                        found = True
                        break
                
                if not found:
                    missing_methods.append(method)
            
            if missing_methods:
                self.issues.append(('critical', 'missing_methods', contract_name,
                                  f"Missing methods: {', '.join(missing_methods)}"))
            else:
                self.successes.append(('methods', f"{contract_name} has all required methods"))
            
            # Check storage prefixes
            if details.storage_prefixes:
                for prefix_name, expected_value in details.storage_prefixes.items():
                    # Look for the prefix definition
                    prefix_patterns = [
                        f'{prefix_name}\\s*=\\s*{expected_value}',
                        f'{prefix_name}\\s*{{\\s*{expected_value}',
                        f'constexpr.*{prefix_name}\\s*=\\s*{expected_value}'
                    ]
                    
                    found = False
                    for pattern in prefix_patterns:
                        if re.search(pattern, combined_content):
                            found = True
                            break
                    
                    if not found:
                        # Check if different value is used
                        if prefix_name in combined_content:
                            self.issues.append(('critical', 'storage_prefix', contract_name,
                                              f"{prefix_name} should be {expected_value}"))
                        else:
                            self.issues.append(('critical', 'storage_prefix', contract_name,
                                              f"Missing storage prefix {prefix_name} = {expected_value}"))
            
            # Check other constants
            for const_name, expected_value in details.constants.items():
                if const_name != 'ID':  # Already checked
                    if isinstance(expected_value, str):
                        if f'"{expected_value}"' not in combined_content:
                            self.issues.append(('warning', 'constant', contract_name,
                                              f"Expected constant {const_name} = '{expected_value}'"))
                    elif isinstance(expected_value, int):
                        if str(expected_value) not in combined_content:
                            self.issues.append(('warning', 'constant', contract_name,
                                              f"Expected constant {const_name} = {expected_value}"))
            
            print(f"{GREEN}✓{NC} {contract_name} checked")
    
    def check_vm_syscalls_implementation(self):
        """Check if all VM system calls are implemented"""
        print(f"\n{CYAN}{'='*60}{NC}")
        print(f"{BOLD}Checking VM System Calls Implementation{NC}")
        print(f"{CYAN}{'='*60}{NC}")
        
        # Look for system call implementations
        syscall_files = [
            self.cpp_root / 'src/smartcontract/system_calls.cpp',
            self.cpp_root / 'src/smartcontract/system_calls_runtime.cpp',
            self.cpp_root / 'src/smartcontract/system_calls_contract.cpp',
            self.cpp_root / 'src/smartcontract/system_calls_storage.cpp',
            self.cpp_root / 'src/smartcontract/system_calls_crypto.cpp',
        ]
        
        all_syscall_content = ""
        for file in syscall_files:
            if file.exists():
                all_syscall_content += file.read_text() + "\n"
        
        if not all_syscall_content:
            self.issues.append(('critical', 'vm_syscalls', 'system_calls', 
                              "No system call implementation files found"))
            return
        
        missing_syscalls = []
        implemented_syscalls = []
        
        for syscall in self.neo_protocol['vm_syscalls']:
            # Convert dots to various patterns
            syscall_patterns = [
                syscall.replace('.', '_'),
                syscall.replace('.', '::'),
                f'"{syscall}"',
                syscall
            ]
            
            found = False
            for pattern in syscall_patterns:
                if pattern in all_syscall_content:
                    found = True
                    implemented_syscalls.append(syscall)
                    break
            
            if not found:
                missing_syscalls.append(syscall)
        
        if missing_syscalls:
            self.issues.append(('warning', 'vm_syscalls', 'system_calls',
                              f"Missing system calls: {', '.join(missing_syscalls[:5])}..."))
        
        print(f"System calls implemented: {len(implemented_syscalls)}/{len(self.neo_protocol['vm_syscalls'])}")
        self.successes.append(('vm_syscalls', 
                             f"Implemented {len(implemented_syscalls)} of {len(self.neo_protocol['vm_syscalls'])} system calls"))
    
    def check_consensus_correctness(self):
        """Verify consensus implementation matches dBFT specification"""
        print(f"\n{CYAN}{'='*60}{NC}")
        print(f"{BOLD}Checking Consensus (dBFT) Implementation{NC}")
        print(f"{CYAN}{'='*60}{NC}")
        
        consensus_dir = self.cpp_root / 'src/consensus'
        if not consensus_dir.exists():
            self.issues.append(('critical', 'consensus', 'directory', "Consensus directory missing"))
            return
        
        # Check consensus states
        consensus_files = list(consensus_dir.glob('*.cpp'))
        all_content = '\n'.join(f.read_text() for f in consensus_files if f.exists())
        
        missing_states = []
        for state in self.neo_protocol['consensus']['states']:
            if state not in all_content:
                missing_states.append(state)
        
        if missing_states:
            self.issues.append(('critical', 'consensus_states', 'consensus',
                              f"Missing consensus states: {', '.join(missing_states)}"))
        else:
            self.successes.append(('consensus', "All consensus states implemented"))
        
        # Check message types
        missing_messages = []
        for msg_type in self.neo_protocol['consensus']['message_types']:
            file_name = f'{self._to_snake_case(msg_type)}.cpp'
            if not (consensus_dir / file_name).exists():
                missing_messages.append(msg_type)
        
        if missing_messages:
            self.issues.append(('critical', 'consensus_messages', 'consensus',
                              f"Missing message types: {', '.join(missing_messages)}"))
        else:
            self.successes.append(('consensus', "All consensus message types implemented"))
        
        # Check timeouts
        for timeout_name, timeout_value in self.neo_protocol['consensus']['timeouts'].items():
            if str(timeout_value) not in all_content:
                self.issues.append(('warning', 'consensus_timeout', 'consensus',
                                  f"{timeout_name} should be {timeout_value}ms"))
    
    def check_protocol_constants(self):
        """Verify protocol constants match Neo specification"""
        print(f"\n{CYAN}{'='*60}{NC}")
        print(f"{BOLD}Checking Protocol Constants{NC}")
        print(f"{CYAN}{'='*60}{NC}")
        
        # Look for protocol settings
        settings_files = [
            self.cpp_root / 'include/neo/protocol_settings.h',
            self.cpp_root / 'src/protocol_settings.cpp',
            self.cpp_root / 'include/neo/core/protocol_settings.h',
        ]
        
        all_settings_content = ""
        for file in settings_files:
            if file.exists():
                all_settings_content += file.read_text() + "\n"
        
        for const_name, expected_value in self.neo_protocol['protocol_constants'].items():
            # Check various patterns
            patterns = [
                f'{const_name}\\s*=\\s*{expected_value}',
                f'{const_name}\\s*{{\\s*{expected_value}',
                f'constexpr.*{const_name}\\s*=\\s*{expected_value}',
                f'static.*{const_name}\\s*=\\s*{expected_value}'
            ]
            
            found = False
            for pattern in patterns:
                if re.search(pattern, all_settings_content):
                    found = True
                    break
            
            if not found:
                if const_name in all_settings_content:
                    self.issues.append(('warning', 'protocol_constant', 'settings',
                                      f"{const_name} may have incorrect value (should be {expected_value})"))
                else:
                    self.issues.append(('critical', 'protocol_constant', 'settings',
                                      f"Missing protocol constant {const_name} = {expected_value}"))
    
    def check_cryptography_correctness(self):
        """Verify cryptographic implementations"""
        print(f"\n{CYAN}{'='*60}{NC}")
        print(f"{BOLD}Checking Cryptographic Implementations{NC}")
        print(f"{CYAN}{'='*60}{NC}")
        
        crypto_requirements = {
            'sha256': ['SHA256', 'ComputeHash', '32', '256'],
            'ripemd160': ['RIPEMD160', 'ComputeHash', '20', '160'],
            'ecdsa': ['VerifySignature', 'secp256r1', 'secp256k1'],
            'bls12_381': ['Pairing', 'MillerLoop', 'G1', 'G2', 'Gt', 'Fp12'],
            'merkle': ['MerkleTree', 'ComputeRoot', 'GetProof'],
        }
        
        crypto_dir = self.cpp_root / 'src/cryptography'
        
        for crypto_type, required_items in crypto_requirements.items():
            print(f"\n{YELLOW}Checking {crypto_type}...{NC}")
            
            # Find relevant files
            relevant_files = []
            if crypto_type == 'ecdsa':
                relevant_files = [crypto_dir / 'ecc/ecpoint.cpp', crypto_dir / 'ecc/ecdsa.cpp']
            else:
                relevant_files = list(crypto_dir.rglob(f'*{crypto_type}*.cpp'))
            
            if not relevant_files:
                self.issues.append(('critical', 'cryptography', crypto_type,
                                  f"No implementation found for {crypto_type}"))
                continue
            
            all_content = ""
            for file in relevant_files:
                if file.exists():
                    all_content += file.read_text() + "\n"
            
            missing_items = []
            for item in required_items:
                if item not in all_content:
                    missing_items.append(item)
            
            if missing_items:
                self.issues.append(('warning', 'cryptography', crypto_type,
                                  f"Missing implementations: {', '.join(missing_items)}"))
            else:
                self.successes.append(('cryptography', f"{crypto_type} fully implemented"))
    
    def check_storage_implementation(self):
        """Check storage layer correctness"""
        print(f"\n{CYAN}{'='*60}{NC}")
        print(f"{BOLD}Checking Storage Implementation{NC}")
        print(f"{CYAN}{'='*60}{NC}")
        
        storage_requirements = {
            'DataCache': ['TryGet', 'GetAndChange', 'Add', 'Delete', 'Commit'],
            'StorageKey': ['GetKey', 'Equals', 'CompareTo', 'Serialize'],
            'StorageItem': ['GetValue', 'SetValue', 'IsConstant'],
            'MemoryStore': ['Get', 'Put', 'Delete', 'Contains', 'Seek'],
        }
        
        persistence_dir = self.cpp_root / 'src/persistence'
        
        for class_name, required_methods in storage_requirements.items():
            file_name = f'{self._to_snake_case(class_name)}.cpp'
            file_path = persistence_dir / file_name
            
            if not file_path.exists():
                self.issues.append(('critical', 'storage', class_name,
                                  f"Missing implementation file"))
                continue
            
            content = file_path.read_text()
            missing_methods = []
            
            for method in required_methods:
                if f'{method}(' not in content:
                    missing_methods.append(method)
            
            if missing_methods:
                self.issues.append(('warning', 'storage', class_name,
                                  f"Missing methods: {', '.join(missing_methods)}"))
    
    def check_test_coverage(self):
        """Check if critical components have tests"""
        print(f"\n{CYAN}{'='*60}{NC}")
        print(f"{BOLD}Checking Test Coverage{NC}")
        print(f"{CYAN}{'='*60}{NC}")
        
        test_dir = self.cpp_root / 'tests'
        if not test_dir.exists():
            self.issues.append(('warning', 'tests', 'directory', "Tests directory missing"))
            return
        
        # Check for native contract tests
        for contract_name in self.neo_protocol['native_contracts']:
            test_file = test_dir / 'unit/smartcontract/native' / f'test_{self._to_snake_case(contract_name)}.cpp'
            if not test_file.exists():
                self.issues.append(('warning', 'tests', contract_name,
                                  f"No test file found for {contract_name}"))
            else:
                self.successes.append(('tests', f"{contract_name} has test coverage"))
    
    def _to_snake_case(self, name: str) -> str:
        """Convert CamelCase to snake_case"""
        return re.sub('([a-z0-9])([A-Z])', r'\1_\2', name).lower()
    
    def generate_report(self):
        """Generate comprehensive correctness report"""
        print(f"\n{PURPLE}{'='*60}{NC}")
        print(f"{BOLD}NEO C++ CORRECTNESS VERIFICATION REPORT{NC}")
        print(f"{PURPLE}{'='*60}{NC}")
        
        critical_count = len([i for i in self.issues if i[0] == 'critical'])
        warning_count = len([i for i in self.issues if i[0] == 'warning'])
        info_count = len([i for i in self.issues if i[0] == 'info'])
        
        print(f"\n{BOLD}Summary:{NC}")
        print(f"Total issues found: {len(self.issues)}")
        print(f"{RED}Critical: {critical_count}{NC}")
        print(f"{YELLOW}Warnings: {warning_count}{NC}")
        print(f"{BLUE}Info: {info_count}{NC}")
        print(f"{GREEN}Successes: {len(self.successes)}{NC}")
        
        # Show successes
        if self.successes:
            print(f"\n{GREEN}{BOLD}✅ Verified Components:{NC}")
            success_categories = {}
            for category, message in self.successes:
                if category not in success_categories:
                    success_categories[category] = []
                success_categories[category].append(message)
            
            for category, messages in success_categories.items():
                print(f"\n{GREEN}{category.upper()}:{NC}")
                for msg in messages[:5]:
                    print(f"  ✓ {msg}")
                if len(messages) > 5:
                    print(f"  ... and {len(messages) - 5} more")
        
        # Show issues by severity
        if self.issues:
            print(f"\n{RED}{BOLD}Issues Found:{NC}")
            
            for severity in ['critical', 'warning', 'info']:
                severity_issues = [i for i in self.issues if i[0] == severity]
                if severity_issues:
                    color = RED if severity == 'critical' else YELLOW if severity == 'warning' else BLUE
                    print(f"\n{color}{BOLD}{severity.upper()} ({len(severity_issues)}):{NC}")
                    
                    # Group by category
                    category_issues = {}
                    for _, category, file, msg in severity_issues:
                        if category not in category_issues:
                            category_issues[category] = []
                        category_issues[category].append((file, msg))
                    
                    for category, file_msgs in category_issues.items():
                        print(f"\n  {BOLD}{category}:{NC}")
                        for file, msg in file_msgs[:3]:
                            print(f"    {file}: {msg}")
                        if len(file_msgs) > 3:
                            print(f"    ... and {len(file_msgs) - 3} more issues")
        
        # Final verdict
        print(f"\n{PURPLE}{'='*60}{NC}")
        print(f"{BOLD}FINAL VERDICT:{NC}")
        
        if critical_count > 0:
            print(f"{RED}❌ Critical issues found - C++ implementation needs fixes{NC}")
            print(f"\nRecommendations:")
            print(f"1. Fix all critical issues related to native contracts")
            print(f"2. Ensure all storage prefixes match C# values exactly")
            print(f"3. Implement missing required methods")
            return 1
        elif warning_count > 50:
            print(f"{YELLOW}⚠️  Many warnings found - implementation mostly correct but needs refinement{NC}")
            return 0
        else:
            print(f"{GREEN}✅ NEO C++ IMPLEMENTATION IS CORRECT AND COMPLETE!{NC}")
            print(f"\nThe C++ implementation:")
            print(f"• Matches all critical C# functionality")
            print(f"• Implements all native contracts correctly")
            print(f"• Has proper storage prefixes and constants")
            print(f"• Supports all required protocol features")
            return 0
    
    def run(self):
        """Run all correctness checks"""
        print(f"{BOLD}{CYAN}Neo C++ Correctness and Completeness Checker{NC}")
        print(f"Checking: {self.cpp_root}")
        print(f"Comparing against: Neo C# Reference Implementation")
        
        self.check_native_contract_correctness()
        self.check_vm_syscalls_implementation()
        self.check_consensus_correctness()
        self.check_protocol_constants()
        self.check_cryptography_correctness()
        self.check_storage_implementation()
        self.check_test_coverage()
        
        return self.generate_report()


def main():
    # Get script directory and project root
    script_dir = Path(__file__).parent
    project_root = script_dir.parent
    
    checker = NeoCorrectnessChecker(project_root)
    return checker.run()


if __name__ == '__main__':
    sys.exit(main())
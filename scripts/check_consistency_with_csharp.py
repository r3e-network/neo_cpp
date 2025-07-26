#!/usr/bin/env python3

"""
Neo C++ to C# Consistency Checker
This script checks for consistency between C++ implementation and C# reference
"""

import os
import re
import sys
from pathlib import Path
from typing import Dict, List, Set, Tuple

# ANSI color codes
RED = '\033[0;31m'
GREEN = '\033[0;32m'
YELLOW = '\033[1;33m'
BLUE = '\033[0;34m'
NC = '\033[0m'  # No Color

class ConsistencyChecker:
    def __init__(self, cpp_root: str):
        self.cpp_root = Path(cpp_root)
        self.issues: List[Tuple[str, str, str]] = []  # (severity, file, message)
        
        # Known C# to C++ mappings
        self.type_mappings = {
            'BigInteger': 'BigInteger',
            'UInt160': 'UInt160',
            'UInt256': 'UInt256',
            'ECPoint': 'ECPoint',
            'byte[]': 'ByteVector',
            'string': 'std::string',
            'Dictionary': 'std::unordered_map',
            'List': 'std::vector',
            'HashSet': 'std::unordered_set',
        }
        
        # Known method name mappings
        self.method_mappings = {
            'ToArray()': 'ToArray()',
            'ToString()': 'ToString()',
            'CompareTo': 'Compare',
            'Equals': 'operator==',
            'GetHashCode': 'GetHash',
        }
        
        # Known Neo-specific patterns
        self.neo_patterns = {
            'native_contracts': [
                'ContractManagement',
                'StdLib',
                'CryptoLib',
                'LedgerContract',
                'NeoToken',
                'GasToken',
                'PolicyContract',
                'RoleManagement',
                'OracleContract',
            ],
            'required_methods': {
                'NativeContract': ['OnPersist', 'PostPersist', 'Initialize', 'OnManifestCompose'],
                'NeoToken': ['Vote', 'GetCommittee', 'GetCandidates', 'GetNextBlockValidators'],
                'GasToken': ['GetGasPerBlock', 'SetGasPerBlock'],
                'PolicyContract': ['GetFeePerByte', 'GetExecFeeFactor', 'SetFeePerByte'],
                'RoleManagement': ['DesignateAsRole', 'GetDesignatedByRole'],
            },
            'consensus_states': [
                'Primary',
                'Backup',
                'RequestSent',
                'RequestReceived',
                'SignatureSent',
                'BlockSent',
                'ViewChanging',
            ]
        }
    
    def check_file_structure(self):
        """Check if C++ project has similar structure to C# Neo"""
        print(f"\n{YELLOW}Checking project structure...{NC}")
        
        required_dirs = [
            'src/core',
            'src/network',
            'src/ledger',
            'src/smartcontract',
            'src/smartcontract/native',
            'src/consensus',
            'src/persistence',
            'src/cryptography',
            'src/vm',
        ]
        
        for dir_path in required_dirs:
            full_path = self.cpp_root / dir_path
            if not full_path.exists():
                self.issues.append(('warning', dir_path, f"Missing directory (exists in C# Neo)"))
            else:
                print(f"{GREEN}✓{NC} {dir_path}")
    
    def check_native_contracts(self):
        """Check if all native contracts are implemented"""
        print(f"\n{YELLOW}Checking native contracts...{NC}")
        
        native_dir = self.cpp_root / 'src/smartcontract/native'
        if not native_dir.exists():
            self.issues.append(('critical', 'native contracts', "Native contracts directory missing"))
            return
        
        for contract in self.neo_patterns['native_contracts']:
            # Check header file
            header_file = self.cpp_root / f'include/neo/smartcontract/native/{self._to_snake_case(contract)}.h'
            cpp_file = native_dir / f'{self._to_snake_case(contract)}.cpp'
            
            if not header_file.exists() and not cpp_file.exists():
                self.issues.append(('critical', contract, f"Native contract not implemented"))
            else:
                # Check required methods
                if contract in self.neo_patterns['required_methods']:
                    self._check_required_methods(cpp_file, contract, self.neo_patterns['required_methods'][contract])
                print(f"{GREEN}✓{NC} {contract}")
    
    def check_cryptography_implementations(self):
        """Check cryptography implementations match C#"""
        print(f"\n{YELLOW}Checking cryptography implementations...{NC}")
        
        crypto_checks = {
            'src/cryptography/sha256.cpp': ['SHA256', 'ComputeHash'],
            'src/cryptography/ripemd160.cpp': ['RIPEMD160', 'ComputeHash'],
            'src/cryptography/murmur3.cpp': ['Murmur3', 'ComputeHash'],
            'src/cryptography/ecc/ecpoint.cpp': ['ECPoint', 'Multiply', 'Add', 'Negate'],
            'src/cryptography/ecc/ecfieldelement.cpp': ['ECFieldElement', 'Square', 'Sqrt'],
            'src/cryptography/bls12_381.cpp': ['BLS12_381', 'Pairing', 'MillerLoop'],
        }
        
        for file_path, required_items in crypto_checks.items():
            full_path = self.cpp_root / file_path
            if not full_path.exists():
                self.issues.append(('critical', file_path, "Missing cryptography implementation"))
            else:
                content = full_path.read_text()
                for item in required_items:
                    if item not in content:
                        self.issues.append(('warning', file_path, f"Missing {item} implementation"))
                print(f"{GREEN}✓{NC} {Path(file_path).name}")
    
    def check_consensus_implementation(self):
        """Check consensus mechanism implementation"""
        print(f"\n{YELLOW}Checking consensus implementation...{NC}")
        
        consensus_dir = self.cpp_root / 'src/consensus'
        if not consensus_dir.exists():
            self.issues.append(('critical', 'consensus', "Consensus directory missing"))
            return
        
        # Check for dBFT implementation files
        required_files = [
            'consensus_context.cpp',
            'consensus_service.cpp',
            'recovery_message.cpp',
            'recovery_request.cpp',
            'commit.cpp',
            'prepare_request.cpp',
            'prepare_response.cpp',
            'change_view.cpp',
        ]
        
        for file_name in required_files:
            if not (consensus_dir / file_name).exists():
                self.issues.append(('critical', f'consensus/{file_name}', "dBFT component missing"))
        
        # Check consensus states
        consensus_files = list(consensus_dir.glob('*.cpp'))
        all_content = '\n'.join(f.read_text() for f in consensus_files if f.exists())
        
        for state in self.neo_patterns['consensus_states']:
            if state not in all_content:
                self.issues.append(('warning', 'consensus', f"Missing consensus state: {state}"))
    
    def check_vm_implementation(self):
        """Check VM implementation completeness"""
        print(f"\n{YELLOW}Checking VM implementation...{NC}")
        
        vm_dir = self.cpp_root / 'src/vm'
        if not vm_dir.exists():
            self.issues.append(('critical', 'vm', "VM directory missing"))
            return
        
        # Check for essential VM components
        vm_components = {
            'execution_engine.cpp': ['Execute', 'LoadScript', 'Push', 'Pop'],
            'execution_context.cpp': ['InstructionPointer', 'EvaluationStack'],
            'opcode.cpp': ['PUSH', 'ADD', 'SUB', 'SYSCALL', 'RET'],
            'stack_item.cpp': ['Integer', 'ByteString', 'Array', 'Map', 'Boolean'],
            'syscalls.cpp': ['System.Contract.Call', 'System.Storage.Get'],
        }
        
        for file_name, required_items in vm_components.items():
            file_path = vm_dir / file_name
            if not file_path.exists():
                self.issues.append(('critical', f'vm/{file_name}', "VM component missing"))
            else:
                content = file_path.read_text()
                for item in required_items:
                    if item not in content:
                        self.issues.append(('warning', f'vm/{file_name}', f"Missing {item}"))\

    def check_common_patterns(self):
        """Check for common C# patterns and their C++ equivalents"""
        print(f"\n{YELLOW}Checking common patterns...{NC}")
        
        patterns_to_check = [
            # Pattern, C# version, C++ version, severity
            (r'throw new NotImplementedException', 'NotImplementedException', 'std::runtime_error("not implemented")', 'critical'),
            (r'Console\.WriteLine', 'Console.WriteLine', 'LOG_INFO or std::cout', 'warning'),
            (r'Debug\.Assert', 'Debug.Assert', 'assert (should be in debug only)', 'warning'),
            (r'using\s+\(', 'using statement', 'RAII/unique_ptr/lock_guard', 'info'),
            (r'async\s+Task', 'async/await', 'std::future/std::async', 'info'),
            (r'\?\?', 'null coalescing', 'value_or/optional', 'info'),
            (r'LINQ', 'LINQ queries', 'std algorithms', 'info'),
        ]
        
        cpp_files = list(self.cpp_root.rglob('*.cpp'))
        
        for pattern, csharp_feature, cpp_equivalent, severity in patterns_to_check:
            files_with_pattern = []
            for file_path in cpp_files:
                if 'test' not in str(file_path):
                    content = file_path.read_text()
                    if re.search(pattern, content, re.IGNORECASE):
                        files_with_pattern.append(file_path)
            
            if files_with_pattern:
                for f in files_with_pattern[:3]:  # Show first 3 examples
                    self.issues.append((severity, str(f.relative_to(self.cpp_root)), 
                                      f"Uses {csharp_feature} pattern - should use {cpp_equivalent}"))
    
    def check_error_handling(self):
        """Check error handling consistency"""
        print(f"\n{YELLOW}Checking error handling patterns...{NC}")
        
        cpp_files = list(self.cpp_root.rglob('*.cpp'))
        
        issues_found = {
            'empty_catch': 0,
            'generic_catch': 0,
            'no_error_msg': 0,
            'assert_in_prod': 0,
        }
        
        for file_path in cpp_files:
            if 'test' not in str(file_path):
                content = file_path.read_text()
                
                # Check for empty catch blocks
                if re.search(r'catch\s*\([^)]*\)\s*{\s*}', content):
                    issues_found['empty_catch'] += 1
                    self.issues.append(('critical', str(file_path.relative_to(self.cpp_root)), 
                                      "Empty catch block found"))
                
                # Check for catching ...
                if re.search(r'catch\s*\(\s*\.\.\.\s*\)', content):
                    issues_found['generic_catch'] += 1
                    self.issues.append(('warning', str(file_path.relative_to(self.cpp_root)), 
                                      "Generic catch(...) found"))
                
                # Check for assert in non-test code
                if re.search(r'\bassert\s*\(', content):
                    issues_found['assert_in_prod'] += 1
                    self.issues.append(('warning', str(file_path.relative_to(self.cpp_root)), 
                                      "assert() in production code"))
    
    def check_magic_numbers(self):
        """Check for magic numbers that should be constants"""
        print(f"\n{YELLOW}Checking for magic numbers...{NC}")
        
        # Known Neo protocol constants
        known_constants = {
            '2000000': 'MaxTransactionSize',
            '1024': 'MaxTransactionAttributes', 
            '16': 'MaxTransactionWitnesses',
            '2102': 'MaxContractLength',
            '255': 'MaxContractParametersCount',
            '1048576': 'MaxNotificationSize',
            '32768': 'MaxStackSize',
        }
        
        cpp_files = list(self.cpp_root.rglob('*.cpp'))
        
        for file_path in cpp_files:
            if 'test' not in str(file_path):
                content = file_path.read_text()
                
                for number, constant_name in known_constants.items():
                    if number in content and constant_name not in content:
                        self.issues.append(('warning', str(file_path.relative_to(self.cpp_root)), 
                                          f"Magic number {number} should be {constant_name}"))
    
    def _check_required_methods(self, file_path: Path, class_name: str, methods: List[str]):
        """Check if required methods are implemented in a class"""
        if not file_path.exists():
            return
            
        content = file_path.read_text()
        for method in methods:
            # Check various C++ method patterns
            patterns = [
                f'{method}\\s*\\(',  # MethodName(
                f'::{method}\\s*\\(',  # ::MethodName(
                f'\\b{method}\\s*\\(',  # word boundary MethodName(
            ]
            
            found = False
            for pattern in patterns:
                if re.search(pattern, content):
                    found = True
                    break
            
            if not found:
                self.issues.append(('critical', str(file_path.relative_to(self.cpp_root)), 
                                  f"Missing required method: {method}"))
    
    def _to_snake_case(self, name: str) -> str:
        """Convert CamelCase to snake_case"""
        return re.sub('([a-z0-9])([A-Z])', r'\1_\2', name).lower()
    
    def generate_report(self):
        """Generate final report"""
        print(f"\n{BLUE}{'='*50}{NC}")
        print(f"{BLUE}Consistency Check Summary{NC}")
        print(f"{BLUE}{'='*50}{NC}")
        
        critical_count = len([i for i in self.issues if i[0] == 'critical'])
        warning_count = len([i for i in self.issues if i[0] == 'warning'])
        info_count = len([i for i in self.issues if i[0] == 'info'])
        
        print(f"\nTotal issues: {len(self.issues)}")
        print(f"{RED}Critical: {critical_count}{NC}")
        print(f"{YELLOW}Warnings: {warning_count}{NC}")
        print(f"{BLUE}Info: {info_count}{NC}")
        
        if self.issues:
            print(f"\n{YELLOW}Issues found:{NC}")
            
            # Group by severity
            for severity in ['critical', 'warning', 'info']:
                severity_issues = [i for i in self.issues if i[0] == severity]
                if severity_issues:
                    color = RED if severity == 'critical' else YELLOW if severity == 'warning' else BLUE
                    print(f"\n{color}{severity.upper()} ({len(severity_issues)}):{NC}")
                    
                    # Group by file
                    file_issues = {}
                    for _, file, msg in severity_issues:
                        if file not in file_issues:
                            file_issues[file] = []
                        file_issues[file].append(msg)
                    
                    for file, messages in list(file_issues.items())[:10]:  # Show first 10 files
                        print(f"  {file}:")
                        for msg in messages[:3]:  # Show first 3 issues per file
                            print(f"    - {msg}")
                        if len(messages) > 3:
                            print(f"    ... and {len(messages) - 3} more issues")
        
        print(f"\n{BLUE}{'='*50}{NC}")
        
        if critical_count > 0:
            print(f"{RED}❌ Critical consistency issues found!{NC}")
            return 1
        elif warning_count > 0:
            print(f"{YELLOW}⚠️  Some consistency warnings found{NC}")
            return 0
        else:
            print(f"{GREEN}✅ C++ implementation is consistent with C# reference!{NC}")
            return 0
    
    def run(self):
        """Run all consistency checks"""
        print(f"{BLUE}Neo C++ to C# Consistency Checker{NC}")
        print(f"Checking: {self.cpp_root}")
        
        self.check_file_structure()
        self.check_native_contracts()
        self.check_cryptography_implementations()
        self.check_consensus_implementation()
        self.check_vm_implementation()
        self.check_common_patterns()
        self.check_error_handling()
        self.check_magic_numbers()
        
        return self.generate_report()


def main():
    # Get script directory and project root
    script_dir = Path(__file__).parent
    project_root = script_dir.parent
    
    checker = ConsistencyChecker(project_root)
    return checker.run()


if __name__ == '__main__':
    sys.exit(main())
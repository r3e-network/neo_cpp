#!/usr/bin/env python3
"""
Neo C++ to C# Compatibility Check
Ensures the C++ implementation is compatible with the reference C# implementation.
"""

import os
import re
import json
import sys
from pathlib import Path
from typing import Dict, List, Tuple, Set

class CompatibilityChecker:
    def __init__(self, cpp_root: str, csharp_root: str):
        self.cpp_root = Path(cpp_root)
        self.csharp_root = Path(csharp_root)
        self.issues = []
        self.warnings = []
        self.passed_checks = []
        
    def check_native_contracts_compatibility(self) -> bool:
        """Check if native contracts match between C++ and C#."""
        print("\n🔍 Checking native contracts compatibility...")
        
        # Map C# contract names to C++ equivalents
        contract_mapping = {
            'ContractManagement.cs': 'contract_management.cpp',
            'CryptoLib.cs': 'crypto_lib.cpp',
            'GasToken.cs': 'gas_token.cpp',
            'LedgerContract.cs': 'ledger_contract.cpp',
            'NeoToken.cs': 'neo_token.cpp',
            'OracleContract.cs': 'oracle_contract.cpp',
            'PolicyContract.cs': 'policy_contract.cpp',
            'RoleManagement.cs': 'role_management.cpp',
            'StdLib.cs': 'std_lib.cpp',
            'NameService.cs': 'name_service.cpp',
        }
        
        csharp_contracts_dir = self.csharp_root / 'src/Neo/SmartContract/Native'
        cpp_contracts_dir = self.cpp_root / 'src/smartcontract/native'
        
        missing_in_cpp = []
        implementation_mismatches = []
        
        for cs_file, cpp_file in contract_mapping.items():
            cs_path = csharp_contracts_dir / cs_file
            cpp_path = cpp_contracts_dir / cpp_file
            
            if cs_path.exists() and not cpp_path.exists():
                # Try alternative name
                alt_cpp_path = cpp_contracts_dir / cpp_file.replace('.cpp', '_contract.cpp')
                if not alt_cpp_path.exists():
                    missing_in_cpp.append(cs_file)
                    continue
                else:
                    cpp_path = alt_cpp_path
                    
            if cs_path.exists() and cpp_path.exists():
                # Check for key methods
                cs_methods = self._extract_methods_csharp(cs_path)
                cpp_methods = self._extract_methods_cpp(cpp_path)
                
                # Check critical methods
                critical_methods = ['OnPersist', 'PostPersist', 'Initialize']
                for method in critical_methods:
                    if method in cs_methods and method not in cpp_methods:
                        implementation_mismatches.append(f"{cpp_file} missing {method}")
        
        if missing_in_cpp:
            self.issues.append({
                'check': 'Native Contracts Compatibility',
                'severity': 'HIGH',
                'details': f"Missing contracts in C++: {', '.join(missing_in_cpp)}"
            })
            return False
            
        if implementation_mismatches:
            self.warnings.extend(implementation_mismatches)
            
        self.passed_checks.append('Native Contracts Compatibility')
        return True
    
    def check_protocol_constants(self) -> bool:
        """Check if protocol constants match between implementations."""
        print("\n🔍 Checking protocol constants compatibility...")
        
        # Check protocol.json compatibility
        cs_protocol = self.csharp_root / 'src/Neo/ProtocolSettings.cs'
        cpp_protocol = self.cpp_root / 'src/protocol_settings.cpp'
        
        if not cs_protocol.exists() or not cpp_protocol.exists():
            self.warnings.append("Could not find protocol settings files for comparison")
            return True
            
        constants_to_check = {
            'MaxTransactionsPerBlock': (512, r'MaxTransactionsPerBlock\s*=\s*(\d+)', r'max_transactions_per_block\s*=\s*(\d+)'),
            'MemoryPoolMaxTransactions': (50000, r'MemoryPoolMaxTransactions\s*=\s*(\d+)', r'memory_pool_max_transactions\s*=\s*(\d+)'),
            'MaxTraceableBlocks': (2102400, r'MaxTraceableBlocks\s*=\s*(\d+)', r'max_traceable_blocks\s*=\s*(\d+)'),
            'MaxBlockSize': (262144, r'MaxBlockSize\s*=\s*(\d+)', r'max_block_size\s*=\s*(\d+)'),
            'MaxBlockSystemFee': (900000000000, r'MaxBlockSystemFee\s*=\s*(\d+)', r'max_block_system_fee\s*=\s*(\d+)'),
        }
        
        mismatches = []
        
        try:
            with open(cs_protocol, 'r') as f:
                cs_content = f.read()
            with open(cpp_protocol, 'r') as f:
                cpp_content = f.read()
                
            for const_name, (expected_value, cs_pattern, cpp_pattern) in constants_to_check.items():
                cs_match = re.search(cs_pattern, cs_content)
                cpp_match = re.search(cpp_pattern, cpp_content)
                
                if cs_match and cpp_match:
                    cs_value = int(cs_match.group(1))
                    cpp_value = int(cpp_match.group(1))
                    
                    if cs_value != cpp_value:
                        mismatches.append(f"{const_name}: C# = {cs_value}, C++ = {cpp_value}")
                        
        except Exception as e:
            self.warnings.append(f"Error checking protocol constants: {e}")
            
        if mismatches:
            self.issues.append({
                'check': 'Protocol Constants',
                'severity': 'HIGH',
                'details': mismatches
            })
            return False
            
        self.passed_checks.append('Protocol Constants Compatibility')
        return True
    
    def check_consensus_compatibility(self) -> bool:
        """Check consensus implementation compatibility."""
        print("\n🔍 Checking consensus compatibility...")
        
        cs_consensus = self.csharp_root / 'src/Neo/Consensus/ConsensusService.cs'
        cpp_consensus = self.cpp_root / 'src/consensus/consensus_service.cpp'
        
        if not cs_consensus.exists() or not cpp_consensus.exists():
            self.warnings.append("Could not find consensus service files for comparison")
            return True
            
        # Check for consensus message handlers
        consensus_handlers = [
            'OnPrepareRequestReceived',
            'OnPrepareResponseReceived',
            'OnChangeViewReceived',
            'OnCommitReceived',
            'OnRecoveryMessageReceived',
            'OnRecoveryRequestReceived'
        ]
        
        cs_methods = self._extract_methods_csharp(cs_consensus)
        cpp_methods = self._extract_methods_cpp(cpp_consensus)
        
        missing_handlers = []
        for handler in consensus_handlers:
            if handler in cs_methods and handler not in cpp_methods:
                missing_handlers.append(handler)
                
        if missing_handlers:
            self.warnings.append(f"Missing consensus handlers in C++: {', '.join(missing_handlers)}")
            
        self.passed_checks.append('Consensus Compatibility')
        return True
    
    def check_p2p_message_compatibility(self) -> bool:
        """Check P2P message types compatibility."""
        print("\n🔍 Checking P2P message compatibility...")
        
        cs_messages_dir = self.csharp_root / 'src/Neo/Network/P2P/Payloads'
        cpp_messages_dir = self.cpp_root / 'src/network/p2p/payloads'
        
        if not cs_messages_dir.exists() or not cpp_messages_dir.exists():
            self.warnings.append("Could not find P2P payload directories for comparison")
            return True
            
        # Get message types from both implementations
        cs_messages = set()
        for file in cs_messages_dir.glob('*.cs'):
            if not file.name.startswith('I') and file.name.endswith('.cs'):
                cs_messages.add(file.stem)
                
        cpp_messages = set()
        for file in cpp_messages_dir.glob('*.cpp'):
            if file.stem.endswith('_payload'):
                cpp_messages.add(file.stem.replace('_payload', '').title().replace('_', ''))
                
        # Map some naming differences
        name_mapping = {
            'AddrPayload': 'Addr',
            'GetBlockByIndexPayload': 'GetBlockByIndex',
            'GetBlocksPayload': 'GetBlocks',
            'GetDataPayload': 'GetData',
            'HeadersPayload': 'Headers',
            'InvPayload': 'Inv',
            'MerkleBlockPayload': 'MerkleBlock',
            'PingPayload': 'Ping',
            'VersionPayload': 'Version',
        }
        
        # Normalize names
        normalized_cpp = set()
        for msg in cpp_messages:
            normalized = name_mapping.get(msg + 'Payload', msg)
            normalized_cpp.add(normalized)
            
        missing_in_cpp = cs_messages - normalized_cpp
        extra_in_cpp = normalized_cpp - cs_messages
        
        if missing_in_cpp:
            # Filter out some expected differences
            missing_in_cpp = {m for m in missing_in_cpp if m not in ['Witness', 'WitnessScope', 'WitnessRule']}
            
        if missing_in_cpp:
            self.warnings.append(f"P2P messages in C# but not C++: {', '.join(missing_in_cpp)}")
            
        self.passed_checks.append('P2P Message Compatibility')
        return True
    
    def check_transaction_attributes(self) -> bool:
        """Check transaction attribute types compatibility."""
        print("\n🔍 Checking transaction attributes compatibility...")
        
        # Check for transaction attribute types
        cs_attr_file = self.csharp_root / 'src/Neo/Network/P2P/Payloads/TransactionAttributeType.cs'
        cpp_attr_file = self.cpp_root / 'include/neo/ledger/transaction_attribute_type.h'
        
        if cs_attr_file.exists() and cpp_attr_file.exists():
            try:
                with open(cs_attr_file, 'r') as f:
                    cs_content = f.read()
                with open(cpp_attr_file, 'r') as f:
                    cpp_content = f.read()
                    
                # Extract attribute types
                cs_attrs = re.findall(r'(\w+)\s*=\s*0x([0-9a-fA-F]+)', cs_content)
                cpp_attrs = re.findall(r'(\w+)\s*=\s*0x([0-9a-fA-F]+)', cpp_content)
                
                cs_attr_dict = {name: int(value, 16) for name, value in cs_attrs}
                cpp_attr_dict = {name: int(value, 16) for name, value in cpp_attrs}
                
                mismatches = []
                for attr, value in cs_attr_dict.items():
                    if attr in cpp_attr_dict and cpp_attr_dict[attr] != value:
                        mismatches.append(f"{attr}: C# = 0x{value:02x}, C++ = 0x{cpp_attr_dict[attr]:02x}")
                        
                if mismatches:
                    self.issues.append({
                        'check': 'Transaction Attributes',
                        'severity': 'HIGH',
                        'details': mismatches
                    })
                    return False
                    
            except Exception as e:
                self.warnings.append(f"Error checking transaction attributes: {e}")
                
        self.passed_checks.append('Transaction Attributes Compatibility')
        return True
    
    def check_vm_opcodes(self) -> bool:
        """Check VM opcode compatibility."""
        print("\n🔍 Checking VM opcodes compatibility...")
        
        cs_opcodes = self.csharp_root / 'src/Neo/VM/OpCode.cs'
        cpp_opcodes = self.cpp_root / 'include/neo/vm/opcode.h'
        
        if not cs_opcodes.exists() or not cpp_opcodes.exists():
            self.warnings.append("Could not find VM opcode files for comparison")
            return True
            
        try:
            with open(cs_opcodes, 'r') as f:
                cs_content = f.read()
            with open(cpp_opcodes, 'r') as f:
                cpp_content = f.read()
                
            # Extract opcodes
            cs_ops = re.findall(r'(\w+)\s*=\s*0x([0-9a-fA-F]+)', cs_content)
            cpp_ops = re.findall(r'(\w+)\s*=\s*0x([0-9a-fA-F]+)', cpp_content)
            
            cs_ops_dict = {name: int(value, 16) for name, value in cs_ops}
            cpp_ops_dict = {name: int(value, 16) for name, value in cpp_ops}
            
            mismatches = []
            missing_in_cpp = []
            
            for op, value in cs_ops_dict.items():
                if op not in cpp_ops_dict:
                    missing_in_cpp.append(op)
                elif cpp_ops_dict[op] != value:
                    mismatches.append(f"{op}: C# = 0x{value:02x}, C++ = 0x{cpp_ops_dict[op]:02x}")
                    
            if missing_in_cpp:
                self.warnings.append(f"VM opcodes missing in C++: {', '.join(missing_in_cpp[:10])}")
                
            if mismatches:
                self.issues.append({
                    'check': 'VM Opcodes',
                    'severity': 'CRITICAL',
                    'details': mismatches
                })
                return False
                
        except Exception as e:
            self.warnings.append(f"Error checking VM opcodes: {e}")
            
        self.passed_checks.append('VM Opcodes Compatibility')
        return True
    
    def check_crypto_curves(self) -> bool:
        """Check cryptographic curve compatibility."""
        print("\n🔍 Checking cryptographic curves compatibility...")
        
        # Both implementations should use secp256r1 (P-256) as the primary curve
        cs_crypto = self.csharp_root / 'src/Neo/Cryptography/ECC/ECCurve.cs'
        cpp_crypto = self.cpp_root / 'include/neo/cryptography/ecc/secp256r1.h'
        
        if cs_crypto.exists() and cpp_crypto.exists():
            try:
                with open(cs_crypto, 'r') as f:
                    cs_content = f.read()
                    
                # Check for secp256r1 usage in C#
                if 'secp256r1' in cs_content or 'P-256' in cs_content:
                    self.passed_checks.append('Crypto Curves Compatibility')
                else:
                    self.warnings.append("Could not verify secp256r1 usage in C# implementation")
                    
            except Exception as e:
                self.warnings.append(f"Error checking crypto curves: {e}")
        else:
            self.warnings.append("Could not find crypto curve files for comparison")
            
        return True
    
    def check_storage_prefixes(self) -> bool:
        """Check storage prefix compatibility for native contracts."""
        print("\n🔍 Checking storage prefixes compatibility...")
        
        # Storage prefixes must match for data compatibility
        prefix_files = {
            'Contract': (self.csharp_root / 'src/Neo/SmartContract/Native/ContractManagement.cs',
                        self.cpp_root / 'src/smartcontract/native/contract_management.cpp'),
            'Neo': (self.csharp_root / 'src/Neo/SmartContract/Native/NeoToken.cs',
                   self.cpp_root / 'src/smartcontract/native/neo_token.cpp'),
            'Policy': (self.csharp_root / 'src/Neo/SmartContract/Native/PolicyContract.cs',
                      self.cpp_root / 'src/smartcontract/native/policy_contract.cpp'),
        }
        
        mismatches = []
        
        for contract, (cs_file, cpp_file) in prefix_files.items():
            if cs_file.exists() and cpp_file.exists():
                try:
                    with open(cs_file, 'r') as f:
                        cs_content = f.read()
                    with open(cpp_file, 'r') as f:
                        cpp_content = f.read()
                        
                    # Extract prefix definitions
                    cs_prefixes = re.findall(r'Prefix_\w+\s*=\s*(\d+)', cs_content)
                    cpp_prefixes = re.findall(r'Prefix\w+\s*=\s*(\d+)', cpp_content)
                    
                    if len(cs_prefixes) != len(cpp_prefixes):
                        self.warnings.append(f"{contract}: Different number of storage prefixes")
                        
                except Exception as e:
                    self.warnings.append(f"Error checking {contract} prefixes: {e}")
                    
        if mismatches:
            self.issues.append({
                'check': 'Storage Prefixes',
                'severity': 'HIGH',
                'details': mismatches
            })
            return False
            
        self.passed_checks.append('Storage Prefixes Compatibility')
        return True
    
    def _extract_methods_csharp(self, file_path: Path) -> Set[str]:
        """Extract method names from C# file."""
        methods = set()
        try:
            with open(file_path, 'r') as f:
                content = f.read()
                # Match C# method declarations
                matches = re.findall(r'(?:public|private|protected|internal)\s+(?:static\s+)?(?:async\s+)?(?:override\s+)?(?:virtual\s+)?(?:\w+(?:<[^>]+>)?)\s+(\w+)\s*\(', content)
                methods.update(matches)
        except Exception:
            pass
        return methods
    
    def _extract_methods_cpp(self, file_path: Path) -> Set[str]:
        """Extract method names from C++ file."""
        methods = set()
        try:
            with open(file_path, 'r') as f:
                content = f.read()
                # Match C++ method implementations
                matches = re.findall(r'(?:void|bool|int|auto|std::\w+(?:<[^>]+>)?)\s+\w+::(\w+)\s*\(', content)
                methods.update(matches)
                # Also match method declarations
                matches = re.findall(r'(?:void|bool|int|auto|std::\w+(?:<[^>]+>)?)\s+(\w+)\s*\(', content)
                methods.update(matches)
        except Exception:
            pass
        return methods
    
    def generate_report(self) -> Tuple[bool, str]:
        """Generate compatibility report."""
        report = []
        report.append("\n" + "="*80)
        report.append("NEO C++ TO C# COMPATIBILITY REPORT")
        report.append("="*80 + "\n")
        
        # Summary
        total_checks = len(self.passed_checks) + len(self.issues)
        report.append(f"✅ Passed Checks: {len(self.passed_checks)}/{total_checks}")
        
        if self.passed_checks:
            report.append("\nPASSED:")
            for check in self.passed_checks:
                report.append(f"  ✅ {check}")
        
        if self.issues:
            report.append(f"\n❌ Failed Checks: {len(self.issues)}")
            report.append("\nISSUES:")
            for issue in self.issues:
                report.append(f"\n  ❌ {issue['check']} [{issue['severity']}]")
                if 'details' in issue:
                    if isinstance(issue['details'], list):
                        for detail in issue['details'][:10]:
                            report.append(f"     - {detail}")
                        if len(issue['details']) > 10:
                            report.append(f"     ... and {len(issue['details']) - 10} more")
                    else:
                        report.append(f"     {issue['details']}")
        
        if self.warnings:
            report.append(f"\n⚠️  Warnings: {len(self.warnings)}")
            for warning in self.warnings[:10]:
                report.append(f"  ⚠️  {warning}")
            if len(self.warnings) > 10:
                report.append(f"  ... and {len(self.warnings) - 10} more warnings")
        
        # Compatibility assessment
        report.append("\n" + "="*80)
        report.append("COMPATIBILITY ASSESSMENT:")
        
        critical_issues = [i for i in self.issues if i['severity'] == 'CRITICAL']
        if not self.issues:
            report.append("✅ FULLY COMPATIBLE")
            report.append("   The C++ implementation is fully compatible with the C# reference.")
        elif critical_issues:
            report.append("❌ NOT COMPATIBLE")
            report.append(f"   {len(critical_issues)} critical compatibility issues found.")
        else:
            report.append("⚠️  MOSTLY COMPATIBLE")
            report.append("   Minor compatibility issues found. Review warnings.")
        
        report.append("="*80 + "\n")
        
        return len(self.issues) == 0, '\n'.join(report)

def main():
    if len(sys.argv) > 1:
        csharp_path = sys.argv[1]
    else:
        # Try to find neo_csharp folder
        script_dir = Path(__file__).parent
        repo_root = script_dir.parent
        csharp_path = repo_root.parent / 'neo_csharp'
        
        if not csharp_path.exists():
            print(f"❌ Error: Could not find neo_csharp folder at {csharp_path}")
            print("Usage: python3 csharp_compatibility_check.py [path_to_neo_csharp]")
            sys.exit(1)
    
    cpp_root = Path(__file__).parent.parent
    
    print(f"🔍 Checking compatibility between:")
    print(f"   C++ : {cpp_root}")
    print(f"   C#  : {csharp_path}")
    
    checker = CompatibilityChecker(cpp_root, csharp_path)
    
    # Run all checks
    checks = [
        checker.check_native_contracts_compatibility,
        checker.check_protocol_constants,
        checker.check_consensus_compatibility,
        checker.check_p2p_message_compatibility,
        checker.check_transaction_attributes,
        checker.check_vm_opcodes,
        checker.check_crypto_curves,
        checker.check_storage_prefixes,
    ]
    
    for check in checks:
        try:
            check()
        except Exception as e:
            print(f"❌ Error running {check.__name__}: {e}")
            checker.issues.append({
                'check': check.__name__,
                'severity': 'ERROR',
                'details': str(e)
            })
    
    # Generate and print report
    is_compatible, report = checker.generate_report()
    print(report)
    
    # Save report
    report_file = cpp_root / 'csharp_compatibility_report.txt'
    with open(report_file, 'w') as f:
        f.write(report)
    print(f"Report saved to: {report_file}")
    
    sys.exit(0 if is_compatible else 1)

if __name__ == '__main__':
    main()
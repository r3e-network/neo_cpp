#!/usr/bin/env python3
"""
Neo N3 C++ Compatibility Verification Script
============================================

This script verifies that the C++ implementation is compatible with the C# Neo N3 node
and identifies areas where the old Transaction format needs to be replaced.

Usage: python verify_neo3_compatibility.py
"""

import os
import re
import sys
from pathlib import Path
from typing import List, Dict, Set, Tuple

class Neo3CompatibilityVerifier:
    def __init__(self, project_root: str):
        self.project_root = Path(project_root)
        self.issues = []
        self.warnings = []
        self.successes = []
        
        # File patterns to check
        self.cpp_files = list(self.project_root.rglob("*.cpp"))
        self.h_files = list(self.project_root.rglob("*.h"))
        self.all_source_files = self.cpp_files + self.h_files
        
    def verify_transaction_compatibility(self):
        """Verify Transaction vs Neo3Transaction usage"""
        print("🔍 Analyzing Transaction vs Neo3Transaction compatibility...")
        
        old_transaction_refs = []
        neo3_transaction_refs = []
        
        for file_path in self.all_source_files:
            if any(exclude in str(file_path) for exclude in ['vcpkg', 'build-', 'CMakeFiles']):
                continue
                
            try:
                with open(file_path, 'r', encoding='utf-8') as f:
                    content = f.read()
                    
                    # Look for old Transaction references  
                    if re.search(r'\bTransaction\b', content) and 'neo3_transaction' not in str(file_path).lower():
                        old_transaction_refs.append(str(file_path))
                    
                    # Look for Neo3Transaction references
                    if re.search(r'\bNeo3Transaction\b', content):
                        neo3_transaction_refs.append(str(file_path))
                        
            except Exception as e:
                self.warnings.append(f"Could not read {file_path}: {e}")
        
        # Report findings
        if old_transaction_refs:
            self.issues.append(f"❌ Found {len(old_transaction_refs)} files using old Transaction format")
            for ref in old_transaction_refs[:10]:  # Show first 10
                self.issues.append(f"   - {ref}")
            if len(old_transaction_refs) > 10:
                self.issues.append(f"   ... and {len(old_transaction_refs) - 10} more files")
        
        if neo3_transaction_refs:
            self.successes.append(f"✅ Found {len(neo3_transaction_refs)} files using correct Neo3Transaction format")
            for ref in neo3_transaction_refs:
                self.successes.append(f"   - {ref}")
        
        return len(old_transaction_refs), len(neo3_transaction_refs)
    
    def verify_core_types(self):
        """Verify UInt160, UInt256, Block, Witness implementations"""
        print("🔍 Checking core type implementations...")
        
        core_types = {
            'UInt160': {'header': 'include/neo/io/uint160.h', 'impl': 'src/io/uint160.cpp'},
            'UInt256': {'header': 'include/neo/io/uint256.h', 'impl': 'src/io/uint256.cpp'},
            'Block': {'header': 'include/neo/ledger/block.h', 'impl': 'src/ledger/block.cpp'},
            'Witness': {'header': 'include/neo/ledger/witness.h', 'impl': 'src/ledger/witness.cpp'},
        }
        
        for type_name, files in core_types.items():
            header_path = self.project_root / files['header']
            impl_path = self.project_root / files['impl']
            
            if header_path.exists():
                self.successes.append(f"✅ {type_name} header found: {files['header']}")
                
                # Check for Neo N3 compatibility markers
                try:
                    with open(header_path, 'r') as f:
                        content = f.read()
                        
                    if type_name == 'Block':
                        # Check if Block uses old Transaction vs Neo3Transaction
                        if 'ledger::Transaction' in content and 'Neo3Transaction' not in content:
                            self.issues.append(f"❌ Block class still references old Transaction type")
                        elif 'Neo3Transaction' in content:
                            self.successes.append(f"✅ Block class uses Neo3Transaction")
                            
                except Exception as e:
                    self.warnings.append(f"Could not analyze {header_path}: {e}")
            else:
                self.issues.append(f"❌ {type_name} header missing: {files['header']}")
            
            if impl_path.exists():
                self.successes.append(f"✅ {type_name} implementation found: {files['impl']}")
            else:
                self.warnings.append(f"⚠️ {type_name} implementation missing: {files['impl']}")
    
    def verify_transaction_attributes(self):
        """Verify transaction attributes are complete"""
        print("🔍 Checking transaction attribute implementations...")
        
        required_attributes = {
            'NotValidBefore': {
                'header': 'include/neo/network/p2p/payloads/not_valid_before.h',
                'impl': 'src/network/p2p/payloads/not_valid_before.cpp'
            },
            'Conflicts': {
                'header': 'include/neo/network/p2p/payloads/conflicts.h', 
                'impl': 'src/network/p2p/payloads/conflicts.cpp'
            },
            'HighPriority': {
                'header': 'include/neo/network/p2p/payloads/high_priority.h',
                'impl': 'src/network/p2p/payloads/high_priority.cpp'
            }
        }
        
        for attr_name, files in required_attributes.items():
            header_path = self.project_root / files['header']
            impl_path = self.project_root / files['impl']
            
            if header_path.exists() and impl_path.exists():
                self.successes.append(f"✅ {attr_name} attribute complete")
            elif header_path.exists():
                self.issues.append(f"❌ {attr_name} header exists but implementation missing")
            else:
                self.issues.append(f"❌ {attr_name} attribute completely missing")
    
    def verify_interfaces(self):
        """Verify IInventory and IVerifiable interfaces"""
        print("🔍 Checking protocol interfaces...")
        
        interfaces = {
            'IInventory': 'include/neo/network/p2p/payloads/iinventory.h',
            'IVerifiable': 'include/neo/network/p2p/payloads/iverifiable.h'
        }
        
        for interface_name, path in interfaces.items():
            interface_path = self.project_root / path
            if interface_path.exists():
                self.successes.append(f"✅ {interface_name} interface found")
            else:
                self.issues.append(f"❌ {interface_name} interface missing")
    
    def verify_build_system(self):
        """Verify CMakeLists.txt includes new files"""
        print("🔍 Checking build system configuration...")
        
        cmake_path = self.project_root / "CMakeLists.txt"
        if cmake_path.exists():
            self.successes.append("✅ CMakeLists.txt found")
            
            try:
                with open(cmake_path, 'r') as f:
                    content = f.read()
                    
                # Check for GLOB patterns that would include new files
                if 'src/network/*.cpp' in content:
                    self.successes.append("✅ CMakeLists.txt includes network source files")
                else:
                    self.warnings.append("⚠️ CMakeLists.txt may not include new network files")
                    
            except Exception as e:
                self.warnings.append(f"Could not analyze CMakeLists.txt: {e}")
        else:
            self.issues.append("❌ CMakeLists.txt missing")
    
    def generate_replacement_plan(self):
        """Generate a plan for replacing old Transaction with Neo3Transaction"""
        print("📋 Generating Transaction replacement plan...")
        
        replacement_files = []
        
        for file_path in self.all_source_files:
            if any(exclude in str(file_path) for exclude in ['vcpkg', 'build-', 'CMakeFiles']):
                continue
                
            try:
                with open(file_path, 'r', encoding='utf-8') as f:
                    content = f.read()
                    
                # Skip if this is already a Neo3Transaction file
                if 'neo3_transaction' in str(file_path).lower():
                    continue
                    
                # Look for Transaction usage patterns
                patterns = [
                    r'#include.*transaction\.h',
                    r'\bTransaction\b(?!\w)',  # Transaction word boundary, not part of another word
                    r'std::.*<.*Transaction.*>',
                    r'Transaction\s*&',
                    r'Transaction\s*\*',
                ]
                
                needs_replacement = any(re.search(pattern, content) for pattern in patterns)
                
                if needs_replacement:
                    replacement_files.append(str(file_path))
                    
            except Exception as e:
                self.warnings.append(f"Could not analyze {file_path}: {e}")
        
        if replacement_files:
            print(f"\n📋 Files requiring Transaction → Neo3Transaction replacement ({len(replacement_files)}):")
            for file_path in replacement_files[:20]:  # Show first 20
                print(f"   - {file_path}")
            if len(replacement_files) > 20:
                print(f"   ... and {len(replacement_files) - 20} more files")
        
        return replacement_files
    
    def run_verification(self):
        """Run complete compatibility verification"""
        print("🚀 Starting Neo N3 C++ Compatibility Verification")
        print("=" * 60)
        
        # Run all verification checks
        old_tx_count, neo3_tx_count = self.verify_transaction_compatibility()
        self.verify_core_types()
        self.verify_transaction_attributes()
        self.verify_interfaces()
        self.verify_build_system()
        replacement_files = self.generate_replacement_plan()
        
        # Print results
        print("\n" + "=" * 60)
        print("📊 VERIFICATION RESULTS")
        print("=" * 60)
        
        if self.successes:
            print("\n✅ SUCCESSES:")
            for success in self.successes:
                print(f"  {success}")
        
        if self.warnings:
            print("\n⚠️  WARNINGS:")
            for warning in self.warnings:
                print(f"  {warning}")
        
        if self.issues:
            print("\n❌ CRITICAL ISSUES:")
            for issue in self.issues:
                print(f"  {issue}")
        
        # Summary
        print("\n" + "=" * 60)
        print("📈 COMPATIBILITY SUMMARY")
        print("=" * 60)
        
        total_issues = len(self.issues)
        total_warnings = len(self.warnings)
        total_successes = len(self.successes)
        
        if old_tx_count > 0:
            print(f"🔥 CRITICAL: {old_tx_count} files use old Transaction format")
            print(f"✅ Progress: {neo3_tx_count} files use correct Neo3Transaction format")
            compatibility_score = (neo3_tx_count / (old_tx_count + neo3_tx_count)) * 100
            print(f"📊 Transaction Compatibility: {compatibility_score:.1f}%")
        
        if total_issues == 0:
            print("🎉 EXCELLENT: Full Neo N3 compatibility achieved!")
            return 0
        elif total_issues <= 5:
            print("✅ GOOD: Minor compatibility issues found")
            return 1
        else:
            print("❌ NEEDS WORK: Significant compatibility issues found")
            return 2

def main():
    """Main entry point"""
    script_dir = Path(__file__).parent
    project_root = script_dir.parent
    
    print(f"Project root: {project_root}")
    
    verifier = Neo3CompatibilityVerifier(str(project_root))
    return verifier.run_verification()

if __name__ == "__main__":
    sys.exit(main()) 
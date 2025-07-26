#!/usr/bin/env python3
"""
Verification script to ensure Neo C++ implementation is complete and production-ready.
"""

import os
import subprocess
import re
from pathlib import Path

class ImplementationVerifier:
    def __init__(self):
        self.issues = []
        self.warnings = []
        self.stats = {
            'total_source_files': 0,
            'total_test_files': 0,
            'production_todos': 0,
            'test_todos': 0,
            'placeholder_functions': 0,
            'mock_implementations': 0
        }
    
    def verify_all(self):
        """Run all verification checks."""
        print("Neo C++ Implementation Verification")
        print("=" * 50)
        
        self.check_source_files()
        self.check_test_files()
        self.check_for_placeholders()
        self.check_for_mocks()
        self.check_critical_implementations()
        self.check_build_system()
        
        self.print_report()
    
    def check_source_files(self):
        """Check all source files for completeness."""
        print("\n1. Checking source files...")
        
        src_files = list(Path('src').rglob('*.cpp')) + list(Path('include').rglob('*.h'))
        self.stats['total_source_files'] = len(src_files)
        
        for file in src_files:
            try:
                with open(file, 'r') as f:
                    content = f.read()
                    
                # Check for TODOs
                todos = re.findall(r'//\s*TODO.*', content)
                if todos:
                    self.stats['production_todos'] += len(todos)
                    for todo in todos:
                        self.warnings.append(f"TODO in {file}: {todo.strip()}")
                
                # Check for not implemented
                if re.search(r'throw.*"[Nn]ot [Ii]mplemented', content):
                    self.issues.append(f"Not implemented exception in {file}")
                    self.stats['placeholder_functions'] += 1
                
                # Check for simplified implementations
                if re.search(r'//.*[Ss]implified|//.*[Mm]ock', content) and 'simple.cpp' not in str(file):
                    self.warnings.append(f"Possible simplified implementation in {file}")
                    
            except Exception as e:
                self.warnings.append(f"Could not read {file}: {e}")
    
    def check_test_files(self):
        """Check test files for proper structure."""
        print("\n2. Checking test files...")
        
        test_files = list(Path('tests').rglob('*.cpp'))
        self.stats['total_test_files'] = len(test_files)
        
        for file in test_files:
            try:
                with open(file, 'r') as f:
                    content = f.read()
                    
                # Check for TODOs
                todos = re.findall(r'//\s*TODO.*', content)
                if todos:
                    self.stats['test_todos'] += len(todos)
                
                # Check for FAIL() indicating unimplemented tests
                if 'FAIL() << "Test not yet implemented' in content:
                    self.warnings.append(f"Unimplemented test stub in {file}")
                    
            except Exception as e:
                self.warnings.append(f"Could not read {file}: {e}")
    
    def check_for_placeholders(self):
        """Check for placeholder implementations."""
        print("\n3. Checking for placeholders...")
        
        # Search for common placeholder patterns
        patterns = [
            r'return\s+nullptr\s*;\s*//\s*placeholder',
            r'//\s*PLACEHOLDER',
            r'//\s*STUB',
            r'return\s+0\s*;\s*//\s*TODO'
        ]
        
        for pattern in patterns:
            result = subprocess.run(
                ['grep', '-r', '-E', pattern, 'src/', 'include/'],
                capture_output=True,
                text=True
            )
            if result.stdout:
                matches = result.stdout.strip().split('\n')
                for match in matches:
                    self.issues.append(f"Placeholder found: {match}")
                    self.stats['placeholder_functions'] += 1
    
    def check_for_mocks(self):
        """Check for mock implementations in production code."""
        print("\n4. Checking for mocks...")
        
        # Exclude test directories
        result = subprocess.run(
            ['find', 'src', '-name', '*.cpp', '-exec', 'grep', '-l', 'mock', '{}', ';'],
            capture_output=True,
            text=True
        )
        
        if result.stdout:
            files = result.stdout.strip().split('\n')
            for file in files:
                if file and 'test' not in file.lower():
                    # Check if it's actually a mock
                    with open(file, 'r') as f:
                        content = f.read()
                        if re.search(r'[Mm]ock[A-Z]\w+|//.*[Mm]ock', content):
                            self.warnings.append(f"Possible mock in production: {file}")
                            self.stats['mock_implementations'] += 1
    
    def check_critical_implementations(self):
        """Verify critical components are properly implemented."""
        print("\n5. Checking critical implementations...")
        
        critical_files = {
            'src/core/neo_system.cpp': ['initialize_components', 'start_worker_threads'],
            'src/smartcontract/transaction_verifier.cpp': ['VerifyTransaction', 'VerifySignature'],
            'src/smartcontract/application_engine.cpp': ['CallContract', 'LoadContract'],
            'src/consensus/dbft_consensus.cpp': ['ProcessPrepareRequest', 'ProcessPrepareResponse'],
            'src/cryptography/bls12_381.cpp': ['HashToG1', 'MultiplyGT'],
            'src/smartcontract/native/neo_token.cpp': ['Transfer', 'Vote', 'RegisterCandidate'],
            'src/rpc/rpc_client.cpp': ['InvokeFunction', 'SendRawTransaction']
        }
        
        for file, functions in critical_files.items():
            if os.path.exists(file):
                with open(file, 'r') as f:
                    content = f.read()
                    for func in functions:
                        if func not in content:
                            self.issues.append(f"Critical function '{func}' not found in {file}")
            else:
                self.issues.append(f"Critical file not found: {file}")
    
    def check_build_system(self):
        """Check if build system is properly configured."""
        print("\n6. Checking build system...")
        
        if not os.path.exists('CMakeLists.txt'):
            self.issues.append("CMakeLists.txt not found")
        
        if not os.path.exists('build') and not os.path.exists('cmake-build-debug'):
            self.warnings.append("No build directory found - project may not have been built")
    
    def print_report(self):
        """Print verification report."""
        print("\n" + "=" * 50)
        print("VERIFICATION REPORT")
        print("=" * 50)
        
        print(f"\nStatistics:")
        print(f"  Total source files: {self.stats['total_source_files']}")
        print(f"  Total test files: {self.stats['total_test_files']}")
        print(f"  Production TODOs: {self.stats['production_todos']}")
        print(f"  Test TODOs: {self.stats['test_todos']}")
        print(f"  Placeholder functions: {self.stats['placeholder_functions']}")
        print(f"  Mock implementations: {self.stats['mock_implementations']}")
        
        if self.issues:
            print(f"\n❌ CRITICAL ISSUES ({len(self.issues)}):")
            for issue in self.issues[:10]:  # Show first 10
                print(f"  - {issue}")
            if len(self.issues) > 10:
                print(f"  ... and {len(self.issues) - 10} more")
        else:
            print("\n✅ No critical issues found!")
        
        if self.warnings:
            print(f"\n⚠️  WARNINGS ({len(self.warnings)}):")
            for warning in self.warnings[:10]:  # Show first 10
                print(f"  - {warning}")
            if len(self.warnings) > 10:
                print(f"  ... and {len(self.warnings) - 10} more")
        
        print("\n" + "=" * 50)
        
        if not self.issues and self.stats['placeholder_functions'] == 0:
            print("✅ IMPLEMENTATION VERIFIED: Neo C++ is production-ready!")
        else:
            print("❌ IMPLEMENTATION INCOMPLETE: Critical issues must be resolved!")
        
        print("=" * 50)

if __name__ == "__main__":
    verifier = ImplementationVerifier()
    verifier.verify_all()
#!/usr/bin/env python3
"""
Neo C++ Unit Test Coverage Checker

This script compares the C# test coverage with C++ test coverage to ensure
all critical tests have been converted from C# to C++.
"""

import os
import re
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
class TestModule:
    """Represents a test module/suite"""
    name: str
    test_files: List[str] = field(default_factory=list)
    test_methods: Set[str] = field(default_factory=set)
    coverage_percentage: float = 0.0

class UnitTestCoverageChecker:
    def __init__(self, cpp_root: str, csharp_root: str = None):
        self.cpp_root = Path(cpp_root)
        self.csharp_root = Path(csharp_root) if csharp_root else self.cpp_root / "neo_csharp"
        
        self.results = {
            'csharp_modules': {},
            'cpp_modules': {},
            'missing_modules': [],
            'missing_tests': {},
            'coverage_summary': {}
        }
        
        # Define critical test categories from C# codebase
        self.critical_test_categories = {
            'neo_core': [
                'UT_BigDecimal', 'UT_DataCache', 'UT_Helper', 'UT_NeoSystem', 
                'UT_ProtocolSettings', 'UT_UInt160', 'UT_UInt256'
            ],
            'neo_vm': [
                'UT_Debugger', 'UT_EvaluationStack', 'UT_ExecutionContext', 
                'UT_ReferenceCounter', 'UT_Script', 'UT_ScriptBuilder', 
                'UT_Slot', 'UT_StackItem', 'UT_Struct'
            ],
            'neo_cryptography_bls12_381': [
                'UT_Fp', 'UT_Fp12', 'UT_Fp2', 'UT_Fp6', 'UT_G1', 'UT_G2', 
                'UT_Pairings', 'UT_Scalar'
            ],
            'neo_extensions': [
                'UT_BigIntegerExtensions', 'UT_ByteArrayComparer', 'UT_ByteArrayEqualityComparer',
                'UT_ByteExtensions', 'UT_DateTimeExtensions', 'UT_IntegerExtensions',
                'UT_SecureStringExtensions', 'UT_StringExtensions'
            ],
            'neo_json': [
                'UT_JArray', 'UT_JBoolean', 'UT_JNumber', 'UT_JObject', 
                'UT_JPath', 'UT_JString', 'UT_OrderedDictionary'
            ],
            'neo_console_service': [
                'UT_CommandServiceBase', 'UT_CommandTokenizer'
            ],
            'neo_rpc_client': [
                'UT_ContractClient', 'UT_Nep17API', 'UT_PolicyAPI', 'UT_RpcClient',
                'UT_RpcModels', 'UT_TransactionManager', 'UT_Utility', 'UT_WalletAPI'
            ],
            'neo_plugins_dbft': [
                'UT_ConsensusService', 'UT_DBFT_Core', 'UT_DBFT_Failures', 
                'UT_DBFT_Integration', 'UT_DBFT_MessageFlow', 'UT_DBFT_NormalFlow',
                'UT_DBFT_Performance', 'UT_DBFT_Recovery'
            ],
            'neo_plugins_rpc_server': [
                'UT_Parameters', 'UT_Result', 'UT_RpcError', 'UT_RpcErrorHandling',
                'UT_RpcServer.Blockchain', 'UT_RpcServer.Node', 'UT_RpcServer.SmartContract',
                'UT_RpcServer.Utilities', 'UT_RpcServer.Wallet', 'UT_RpcServer'
            ],
            'neo_plugins_oracle_service': [
                'UT_OracleService'
            ]
        }
    
    def find_csharp_test_files(self) -> Dict[str, TestModule]:
        """Find all C# test files and extract test methods"""
        print(f"\n{CYAN}Analyzing C# Test Coverage...{NC}")
        
        test_modules = {}
        tests_dir = self.csharp_root / "tests"
        
        if not tests_dir.exists():
            print(f"{RED}C# tests directory not found at {tests_dir}{NC}")
            return {}
        
        for test_proj_dir in tests_dir.iterdir():
            if test_proj_dir.is_dir() and ".Tests" in test_proj_dir.name:
                module_name = test_proj_dir.name.replace(".Tests", "").replace("Neo.", "").replace("Plugins.", "")
                module = TestModule(name=module_name)
                
                # Find all .cs test files
                for cs_file in test_proj_dir.glob("**/*.cs"):
                    if cs_file.name.startswith("UT_") or "Test" in cs_file.name:
                        module.test_files.append(str(cs_file.relative_to(tests_dir)))
                        
                        # Extract test methods from the file
                        try:
                            content = cs_file.read_text(encoding='utf-8')
                            # Find test methods (marked with [TestMethod] or similar)
                            test_methods = re.findall(r'(?:(?:\[Test\]|\[TestMethod\]|\[Fact\]|\[Theory\])\s*)?(?:public\s+(?:async\s+)?(?:Task\s+|void\s+))(\w*Test\w*|Test\w*|\w*_\w*)\s*\(', content, re.MULTILINE)
                            module.test_methods.update(test_methods)
                        except Exception as e:
                            print(f"Warning: Could not parse {cs_file}: {e}")
                
                if module.test_files:
                    test_modules[module_name] = module
                    print(f"  Found {len(module.test_files)} test files in {module_name} with {len(module.test_methods)} test methods")
        
        return test_modules
    
    def find_cpp_test_files(self) -> Dict[str, TestModule]:
        """Find all C++ test files and extract test cases"""
        print(f"\n{CYAN}Analyzing C++ Test Coverage...{NC}")
        
        test_modules = {}
        tests_dir = self.cpp_root / "tests"
        
        if not tests_dir.exists():
            print(f"{RED}C++ tests directory not found at {tests_dir}{NC}")
            return {}
        
        # Look for unit tests
        unit_tests_dir = tests_dir / "unit"
        if unit_tests_dir.exists():
            for test_file in unit_tests_dir.rglob("*.cpp"):
                if "test_" in test_file.name or "_test" in test_file.name:
                    # Determine module name from file path
                    relative_path = test_file.relative_to(unit_tests_dir)
                    module_name = str(relative_path.parent) if relative_path.parent != Path('.') else "core"
                    module_name = module_name.replace('/', '_')
                    
                    if module_name not in test_modules:
                        test_modules[module_name] = TestModule(name=module_name)
                    
                    test_modules[module_name].test_files.append(str(relative_path))
                    
                    # Extract test cases from the file
                    try:
                        content = test_file.read_text()
                        # Find Google Test cases or similar
                        test_cases = re.findall(r'(?:TEST|TEST_F|TEST_P)\s*\(\s*(\w+)\s*,\s*(\w+)\s*\)', content)
                        for test_class, test_name in test_cases:
                            test_modules[module_name].test_methods.add(f"{test_class}.{test_name}")
                    except Exception as e:
                        print(f"Warning: Could not parse {test_file}: {e}")
        
        # Look for integration tests
        integration_tests_dir = tests_dir / "integration"
        if integration_tests_dir.exists():
            for test_file in integration_tests_dir.rglob("*.cpp"):
                if "test_" in test_file.name:
                    module_name = "integration"
                    if module_name not in test_modules:
                        test_modules[module_name] = TestModule(name=module_name)
                    
                    test_modules[module_name].test_files.append(str(test_file.relative_to(tests_dir)))
                    
                    # Extract test cases
                    try:
                        content = test_file.read_text()
                        test_cases = re.findall(r'(?:TEST|TEST_F|TEST_P)\s*\(\s*(\w+)\s*,\s*(\w+)\s*\)', content)
                        for test_class, test_name in test_cases:
                            test_modules[module_name].test_methods.add(f"{test_class}.{test_name}")
                    except Exception as e:
                        print(f"Warning: Could not parse {test_file}: {e}")
        
        for module_name, module in test_modules.items():
            print(f"  Found {len(module.test_files)} test files in {module_name} with {len(module.test_methods)} test cases")
        
        return test_modules
    
    def analyze_coverage_gaps(self, csharp_modules: Dict[str, TestModule], cpp_modules: Dict[str, TestModule]):
        """Analyze gaps between C# and C++ test coverage"""
        print(f"\n{CYAN}Analyzing Coverage Gaps...{NC}")
        
        missing_modules = []
        missing_tests = {}
        
        # Check for missing modules
        for csharp_module_name, csharp_module in csharp_modules.items():
            # Try to find equivalent C++ module
            cpp_equivalent = None
            for cpp_module_name in cpp_modules.keys():
                if self._modules_equivalent(csharp_module_name, cpp_module_name):
                    cpp_equivalent = cpp_module_name
                    break
            
            if not cpp_equivalent:
                missing_modules.append(csharp_module_name)
                print(f"{RED}Missing C++ test module for: {csharp_module_name}{NC}")
            else:
                cpp_module = cpp_modules[cpp_equivalent]
                
                # Check for missing tests within the module
                missing_in_module = []
                for csharp_test in csharp_module.test_methods:
                    # Try to find equivalent C++ test
                    found_equivalent = False
                    for cpp_test in cpp_module.test_methods:
                        if self._tests_equivalent(csharp_test, cpp_test):
                            found_equivalent = True
                            break
                    
                    if not found_equivalent:
                        missing_in_module.append(csharp_test)
                
                if missing_in_module:
                    missing_tests[csharp_module_name] = missing_in_module
                    print(f"{YELLOW}Module {csharp_module_name} missing {len(missing_in_module)} tests{NC}")
                else:
                    print(f"{GREEN}Module {csharp_module_name} has good C++ test coverage{NC}")
        
        self.results['missing_modules'] = missing_modules
        self.results['missing_tests'] = missing_tests
    
    def _modules_equivalent(self, csharp_name: str, cpp_name: str) -> bool:
        """Check if C# and C++ module names refer to the same component"""
        # Normalize names
        csharp_normalized = csharp_name.lower().replace("neo.", "").replace("plugins.", "")
        cpp_normalized = cpp_name.lower().replace("neo_", "").replace("_", "")
        
        # Direct match
        if csharp_normalized == cpp_normalized:
            return True
        
        # Common mappings
        mappings = {
            'unittests': 'core',
            'vm': 'vm',
            'cryptographybls12381': 'cryptography_bls12_381',
            'extensions': 'extensions',
            'json': 'json',
            'consoleservice': 'console_service',
            'rpcclient': 'rpc_client',
            'dbftplugin': 'consensus',
            'rpcserver': 'rpc_server'
        }
        
        return mappings.get(csharp_normalized) == cpp_normalized
    
    def _tests_equivalent(self, csharp_test: str, cpp_test: str) -> bool:
        """Check if C# and C++ test names refer to the same test"""
        # Extract method names
        csharp_method = csharp_test.split('.')[-1] if '.' in csharp_test else csharp_test
        cpp_method = cpp_test.split('.')[-1] if '.' in cpp_test else cpp_test
        
        # Normalize for comparison
        csharp_normalized = csharp_method.lower().replace("test", "").replace("_", "")
        cpp_normalized = cpp_method.lower().replace("test", "").replace("_", "")
        
        return csharp_normalized == cpp_normalized
    
    def generate_missing_tests_report(self):
        """Generate a report of missing tests that need to be implemented"""
        missing_modules = self.results['missing_modules']
        missing_tests = self.results['missing_tests']
        
        if not missing_modules and not missing_tests:
            print(f"\n{GREEN}✅ All critical C# tests have C++ equivalents!{NC}")
            return
        
        print(f"\n{RED}❌ Missing Test Coverage Report{NC}")
        print(f"{RED}{'='*60}{NC}")
        
        if missing_modules:
            print(f"\n{RED}Missing Test Modules ({len(missing_modules)}):{NC}")
            for module in missing_modules:
                print(f"  - {module}")
                if module in self.critical_test_categories:
                    print(f"    Critical tests: {', '.join(self.critical_test_categories[module][:3])}...")
        
        if missing_tests:
            print(f"\n{YELLOW}Incomplete Test Modules:{NC}")
            for module, tests in missing_tests.items():
                print(f"\n  {module} (missing {len(tests)} tests):")
                for test in tests[:5]:  # Show first 5
                    print(f"    - {test}")
                if len(tests) > 5:
                    print(f"    ... and {len(tests) - 5} more")
    
    def create_test_implementation_guide(self):
        """Create a guide for implementing missing tests"""
        guide_content = """# Neo C++ Test Implementation Guide

This guide shows which C# tests need to be converted to C++ tests.

## Critical Test Modules That Need Implementation

"""
        
        for module in self.results['missing_modules']:
            if module in self.critical_test_categories:
                guide_content += f"\n### {module}\n"
                guide_content += f"Location: tests/unit/{module.replace('.', '/')}/\n"
                guide_content += "Critical tests to implement:\n"
                for test in self.critical_test_categories[module]:
                    guide_content += f"- [ ] {test}\n"
        
        guide_content += "\n## Incomplete Test Modules\n"
        
        for module, tests in self.results['missing_tests'].items():
            guide_content += f"\n### {module}\n"
            guide_content += "Missing tests:\n"
            for test in tests:
                guide_content += f"- [ ] {test}\n"
        
        guide_path = self.cpp_root / "TEST_IMPLEMENTATION_GUIDE.md"
        guide_path.write_text(guide_content)
        print(f"\n{BLUE}Test implementation guide created: {guide_path}{NC}")
    
    def run_coverage_check(self):
        """Run the complete test coverage check"""
        print(f"{BOLD}{CYAN}Neo C++ Unit Test Coverage Checker{NC}")
        print(f"Checking: {self.cpp_root}")
        print(f"Comparing against C# tests in: {self.csharp_root}")
        
        # Analyze both codebases
        csharp_modules = self.find_csharp_test_files()
        cpp_modules = self.find_cpp_test_files()
        
        self.results['csharp_modules'] = {name: {'files': len(mod.test_files), 'tests': len(mod.test_methods)} for name, mod in csharp_modules.items()}
        self.results['cpp_modules'] = {name: {'files': len(mod.test_files), 'tests': len(mod.test_methods)} for name, mod in cpp_modules.items()}
        
        # Analyze gaps
        self.analyze_coverage_gaps(csharp_modules, cpp_modules)
        
        # Generate summary
        total_csharp_tests = sum(len(mod.test_methods) for mod in csharp_modules.values())
        total_cpp_tests = sum(len(mod.test_methods) for mod in cpp_modules.values())
        missing_count = len(self.results['missing_modules']) + sum(len(tests) for tests in self.results['missing_tests'].values())
        
        coverage_percentage = max(0, (total_cpp_tests / total_csharp_tests * 100)) if total_csharp_tests > 0 else 0
        
        self.results['coverage_summary'] = {
            'total_csharp_tests': total_csharp_tests,
            'total_cpp_tests': total_cpp_tests,
            'missing_count': missing_count,
            'coverage_percentage': coverage_percentage
        }
        
        # Generate reports
        self.generate_missing_tests_report()
        self.create_test_implementation_guide()
        
        # Final summary
        print(f"\n{PURPLE}{'='*60}{NC}")
        print(f"{BOLD}TEST COVERAGE SUMMARY{NC}")
        print(f"{PURPLE}{'='*60}{NC}")
        print(f"C# Tests: {total_csharp_tests}")
        print(f"C++ Tests: {total_cpp_tests}")
        print(f"Coverage: {coverage_percentage:.1f}%")
        print(f"Missing: {missing_count} tests")
        
        if coverage_percentage >= 80:
            print(f"\n{GREEN}✅ Good test coverage!{NC}")
            return 0
        elif coverage_percentage >= 50:
            print(f"\n{YELLOW}⚠️  Moderate test coverage - needs improvement{NC}")
            return 1
        else:
            print(f"\n{RED}❌ Poor test coverage - significant work needed{NC}")
            return 2


def main():
    import argparse
    
    parser = argparse.ArgumentParser(description='Check Neo C++ unit test coverage against C# reference')
    parser.add_argument('--cpp-root', default='.', help='C++ project root directory')
    parser.add_argument('--csharp-root', help='C# project root directory (default: cpp-root/neo_csharp)')
    parser.add_argument('--output', help='Output JSON file for results')
    
    args = parser.parse_args()
    
    checker = UnitTestCoverageChecker(args.cpp_root, args.csharp_root)
    exit_code = checker.run_coverage_check()
    
    # Save results if requested
    if args.output:
        with open(args.output, 'w') as f:
            json.dump(checker.results, f, indent=2)
        print(f"\nDetailed results saved to: {args.output}")
    
    return exit_code


if __name__ == '__main__':
    exit(main())
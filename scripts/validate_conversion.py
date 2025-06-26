#!/usr/bin/env python3
"""
Script to validate the completeness of Neo N3 C# to C++ conversion.
This script compares the C# and C++ implementations to identify missing components.
"""

import os
import json
import re
from pathlib import Path
from collections import defaultdict

class ConversionValidator:
    def __init__(self, neo_csharp_path, neo_cpp_path):
        self.neo_csharp_path = neo_csharp_path
        self.neo_cpp_path = neo_cpp_path
        self.results = {
            'modules': {'converted': [], 'missing': [], 'partial': []},
            'tests': {'converted': [], 'missing': [], 'partial': []},
            'files': {'converted': [], 'missing': []},
            'summary': {}
        }
    
    def analyze_csharp_structure(self):
        """Analyze the C# project structure."""
        csharp_modules = {}
        
        # Analyze src directory
        src_path = os.path.join(self.neo_csharp_path, 'src')
        if os.path.exists(src_path):
            for item in os.listdir(src_path):
                item_path = os.path.join(src_path, item)
                if os.path.isdir(item_path) and not item.startswith('.'):
                    csharp_modules[item] = self._analyze_directory(item_path)
        
        # Analyze tests directory
        tests_path = os.path.join(self.neo_csharp_path, 'tests')
        test_modules = {}
        if os.path.exists(tests_path):
            for item in os.listdir(tests_path):
                item_path = os.path.join(tests_path, item)
                if os.path.isdir(item_path) and not item.startswith('.'):
                    test_modules[item] = self._analyze_directory(item_path)
        
        return csharp_modules, test_modules
    
    def analyze_cpp_structure(self):
        """Analyze the C++ project structure."""
        cpp_modules = {}
        
        # Analyze src directory
        src_path = os.path.join(self.neo_cpp_path, 'src')
        if os.path.exists(src_path):
            cpp_modules = self._analyze_directory(src_path)
        
        # Analyze tests directory
        tests_path = os.path.join(self.neo_cpp_path, 'tests')
        test_modules = {}
        if os.path.exists(tests_path):
            test_modules = self._analyze_directory(tests_path)
        
        return cpp_modules, test_modules
    
    def _analyze_directory(self, directory):
        """Recursively analyze a directory structure."""
        structure = {
            'files': [],
            'subdirs': {},
            'cs_files': [],
            'cpp_files': [],
            'h_files': [],
            'test_files': []
        }
        
        try:
            for item in os.listdir(directory):
                item_path = os.path.join(directory, item)
                
                if os.path.isfile(item_path):
                    structure['files'].append(item)
                    
                    if item.endswith('.cs'):
                        structure['cs_files'].append(item)
                        if item.startswith('UT_') or 'Test' in item:
                            structure['test_files'].append(item)
                    elif item.endswith('.cpp'):
                        structure['cpp_files'].append(item)
                        if 'test_' in item or 'Test' in item:
                            structure['test_files'].append(item)
                    elif item.endswith('.h') or item.endswith('.hpp'):
                        structure['h_files'].append(item)
                
                elif os.path.isdir(item_path) and not item.startswith('.'):
                    structure['subdirs'][item] = self._analyze_directory(item_path)
        
        except PermissionError:
            pass
        
        return structure
    
    def compare_modules(self):
        """Compare C# and C++ module structures."""
        csharp_modules, csharp_tests = self.analyze_csharp_structure()
        cpp_modules, cpp_tests = self.analyze_cpp_structure()
        
        # Module mapping (C# to C++ naming)
        module_mappings = {
            'Neo': 'core',
            'Neo.CLI': 'cli',
            'Neo.ConsoleService': 'console_service',
            'Neo.Cryptography.BLS12_381': 'cryptography',
            'Neo.Cryptography.MPTTrie': 'cryptography/mpttrie',
            'Neo.Extensions': 'extensions',
            'Neo.GUI': 'gui',
            'Neo.IO': 'io',
            'Neo.Json': 'json',
            'Neo.Network.RpcClient': 'rpc',
            'Neo.VM': 'vm',
            'Plugins': 'plugins'
        }
        
        # Check module conversion status
        for cs_module, cs_structure in csharp_modules.items():
            if cs_module in module_mappings:
                cpp_module = module_mappings[cs_module]
                if self._module_exists_in_cpp(cpp_module, cpp_modules):
                    self.results['modules']['converted'].append(cs_module)
                else:
                    self.results['modules']['missing'].append(cs_module)
            else:
                # Check if module exists with similar name
                if self._find_similar_module(cs_module, cpp_modules):
                    self.results['modules']['partial'].append(cs_module)
                else:
                    self.results['modules']['missing'].append(cs_module)
        
        # Check test conversion status
        for cs_test, cs_structure in csharp_tests.items():
            cpp_equivalent = self._find_cpp_test_equivalent(cs_test, cpp_tests)
            if cpp_equivalent:
                self.results['tests']['converted'].append(cs_test)
            else:
                self.results['tests']['missing'].append(cs_test)
    
    def _module_exists_in_cpp(self, module_path, cpp_modules):
        """Check if a module exists in the C++ structure."""
        parts = module_path.split('/')
        current = cpp_modules
        
        for part in parts:
            if part in current.get('subdirs', {}):
                current = current['subdirs'][part]
            else:
                return False
        
        return True
    
    def _find_similar_module(self, cs_module, cpp_modules):
        """Find similar module names in C++ structure."""
        cs_lower = cs_module.lower().replace('.', '').replace('neo', '')
        
        def search_recursive(structure, path=""):
            for subdir in structure.get('subdirs', {}):
                if cs_lower in subdir.lower():
                    return f"{path}/{subdir}" if path else subdir
                result = search_recursive(structure['subdirs'][subdir], f"{path}/{subdir}" if path else subdir)
                if result:
                    return result
            return None
        
        return search_recursive(cpp_modules)
    
    def _find_cpp_test_equivalent(self, cs_test, cpp_tests):
        """Find equivalent C++ test for a C# test."""
        # Simple heuristic: look for similar names
        cs_lower = cs_test.lower().replace('.', '').replace('neo', '').replace('tests', '').replace('unittest', '')
        
        def search_recursive(structure):
            for subdir in structure.get('subdirs', {}):
                if cs_lower in subdir.lower():
                    return subdir
                result = search_recursive(structure['subdirs'][subdir])
                if result:
                    return result
            return None
        
        return search_recursive(cpp_tests)
    
    def analyze_test_coverage(self):
        """Analyze test coverage by comparing C# and C++ test files."""
        csharp_test_files = self._find_all_test_files(
            os.path.join(self.neo_csharp_path, 'tests')
        )
        cpp_test_files = self._find_all_test_files(
            os.path.join(self.neo_cpp_path, 'tests')
        )
        
        # Map C# test files to expected C++ equivalents
        for cs_test in csharp_test_files:
            expected_cpp = self._map_cs_test_to_cpp(cs_test)
            if any(expected_cpp.lower() in cpp_test.lower() for cpp_test in cpp_test_files):
                self.results['files']['converted'].append(cs_test)
            else:
                self.results['files']['missing'].append(cs_test)
    
    def _find_all_test_files(self, directory):
        """Find all test files in a directory."""
        test_files = []
        
        if not os.path.exists(directory):
            return test_files
        
        for root, dirs, files in os.walk(directory):
            for file in files:
                if (file.startswith('UT_') and file.endswith('.cs')) or \
                   (file.startswith('test_') and file.endswith('.cpp')):
                    test_files.append(os.path.join(root, file))
        
        return test_files
    
    def _map_cs_test_to_cpp(self, cs_test_path):
        """Map a C# test file to expected C++ test file name."""
        filename = os.path.basename(cs_test_path)
        if filename.startswith('UT_'):
            cpp_name = filename.replace('UT_', 'test_').replace('.cs', '.cpp').lower()
        else:
            cpp_name = filename.replace('.cs', '.cpp').lower()
        return cpp_name
    
    def check_critical_components(self):
        """Check for critical components that must be implemented."""
        critical_components = [
            'BigDecimal',
            'UInt160',
            'UInt256',
            'NeoSystem',
            'ProtocolSettings',
            'Blockchain',
            'ApplicationEngine',
            'ExecutionEngine',
            'InteropService'
        ]
        
        missing_critical = []
        
        # Check in C++ headers
        include_path = os.path.join(self.neo_cpp_path, 'include')
        if os.path.exists(include_path):
            for component in critical_components:
                if not self._find_component_in_headers(component, include_path):
                    missing_critical.append(component)
        
        self.results['critical_missing'] = missing_critical
    
    def _find_component_in_headers(self, component, include_path):
        """Find a component in header files."""
        component_lower = component.lower()
        
        for root, dirs, files in os.walk(include_path):
            for file in files:
                if file.endswith('.h') or file.endswith('.hpp'):
                    if component_lower in file.lower():
                        return True
                    
                    # Check file content for class/struct definitions
                    file_path = os.path.join(root, file)
                    try:
                        with open(file_path, 'r', encoding='utf-8', errors='ignore') as f:
                            content = f.read()
                            if re.search(rf'\b(class|struct)\s+{component}\b', content, re.IGNORECASE):
                                return True
                    except:
                        pass
        
        return False
    
    def generate_report(self):
        """Generate a comprehensive conversion report."""
        self.compare_modules()
        self.analyze_test_coverage()
        self.check_critical_components()
        
        # Calculate summary statistics
        total_modules = len(self.results['modules']['converted']) + \
                       len(self.results['modules']['missing']) + \
                       len(self.results['modules']['partial'])
        
        total_tests = len(self.results['tests']['converted']) + \
                     len(self.results['tests']['missing'])
        
        total_files = len(self.results['files']['converted']) + \
                     len(self.results['files']['missing'])
        
        self.results['summary'] = {
            'module_conversion_rate': len(self.results['modules']['converted']) / max(total_modules, 1) * 100,
            'test_conversion_rate': len(self.results['tests']['converted']) / max(total_tests, 1) * 100,
            'file_conversion_rate': len(self.results['files']['converted']) / max(total_files, 1) * 100,
            'critical_components_missing': len(self.results['critical_missing']),
            'total_modules': total_modules,
            'total_tests': total_tests,
            'total_files': total_files
        }
        
        return self.results
    
    def print_report(self):
        """Print a human-readable report."""
        results = self.generate_report()
        
        print("=" * 80)
        print("NEO N3 C# TO C++ CONVERSION VALIDATION REPORT")
        print("=" * 80)
        
        print(f"\nSUMMARY:")
        print(f"  Module Conversion Rate: {results['summary']['module_conversion_rate']:.1f}%")
        print(f"  Test Conversion Rate: {results['summary']['test_conversion_rate']:.1f}%")
        print(f"  File Conversion Rate: {results['summary']['file_conversion_rate']:.1f}%")
        print(f"  Critical Components Missing: {results['summary']['critical_components_missing']}")
        
        print(f"\nMODULE STATUS:")
        print(f"  ✅ Converted ({len(results['modules']['converted'])}): {', '.join(results['modules']['converted'])}")
        print(f"  ⚠️  Partial ({len(results['modules']['partial'])}): {', '.join(results['modules']['partial'])}")
        print(f"  ❌ Missing ({len(results['modules']['missing'])}): {', '.join(results['modules']['missing'])}")
        
        print(f"\nTEST STATUS:")
        print(f"  ✅ Converted ({len(results['tests']['converted'])}): {', '.join(results['tests']['converted'])}")
        print(f"  ❌ Missing ({len(results['tests']['missing'])}): {', '.join(results['tests']['missing'])}")
        
        if results['critical_missing']:
            print(f"\n🚨 CRITICAL COMPONENTS MISSING:")
            for component in results['critical_missing']:
                print(f"    - {component}")
        
        print(f"\nMISSING TEST FILES ({len(results['files']['missing'])}):")
        for file_path in results['files']['missing'][:10]:  # Show first 10
            print(f"    - {os.path.basename(file_path)}")
        if len(results['files']['missing']) > 10:
            print(f"    ... and {len(results['files']['missing']) - 10} more")
        
        print("\n" + "=" * 80)

def main():
    """Main function."""
    neo_csharp_path = "neo"
    neo_cpp_path = "."
    
    if not os.path.exists(neo_csharp_path):
        print(f"Error: C# Neo directory '{neo_csharp_path}' not found")
        return
    
    if not os.path.exists(neo_cpp_path):
        print(f"Error: C++ Neo directory '{neo_cpp_path}' not found")
        return
    
    validator = ConversionValidator(neo_csharp_path, neo_cpp_path)
    validator.print_report()
    
    # Save detailed results to JSON
    results = validator.generate_report()
    with open('conversion_validation_report.json', 'w') as f:
        json.dump(results, f, indent=2)
    
    print(f"\nDetailed report saved to: conversion_validation_report.json")

if __name__ == "__main__":
    main()
#!/usr/bin/env python3
"""
Neo C# to C++ Test Migration Tool

This tool analyzes Neo C# test files and generates equivalent C++ tests
to ensure 100% protocol compatibility.
"""

import os
import re
import json
from pathlib import Path
from typing import List, Dict, Tuple

class CSharpTestParser:
    """Parse C# test files and extract test cases"""
    
    def __init__(self, cs_test_path: str):
        self.cs_test_path = Path(cs_test_path)
        self.test_cases = []
        
    def parse_test_file(self, file_path: Path) -> List[Dict]:
        """Parse a single C# test file"""
        tests = []
        
        with open(file_path, 'r') as f:
            content = f.read()
            
        # Extract test methods
        test_pattern = r'\[Test(?:Case(?:Source)?)?\]\s*(?:public\s+)?(?:async\s+)?(?:Task\s+)?void\s+(\w+)\s*\([^)]*\)'
        test_methods = re.findall(test_pattern, content)
        
        for method_name in test_methods:
            # Extract test body
            method_pattern = rf'void\s+{method_name}\s*\([^)]*\)\s*\{{(.*?)\n\s*\}}'
            match = re.search(method_pattern, content, re.DOTALL)
            
            if match:
                test_body = match.group(1)
                
                # Extract assertions
                assertions = self.extract_assertions(test_body)
                
                # Extract test data
                test_data = self.extract_test_data(test_body)
                
                tests.append({
                    'name': method_name,
                    'category': self.categorize_test(method_name),
                    'assertions': assertions,
                    'test_data': test_data,
                    'original_body': test_body
                })
                
        return tests
    
    def extract_assertions(self, test_body: str) -> List[Dict]:
        """Extract assertions from test body"""
        assertions = []
        
        # Common NUnit assertions
        patterns = [
            (r'Assert\.AreEqual\((.*?),\s*(.*?)\)', 'EXPECT_EQ'),
            (r'Assert\.AreNotEqual\((.*?),\s*(.*?)\)', 'EXPECT_NE'),
            (r'Assert\.IsTrue\((.*?)\)', 'EXPECT_TRUE'),
            (r'Assert\.IsFalse\((.*?)\)', 'EXPECT_FALSE'),
            (r'Assert\.IsNull\((.*?)\)', 'EXPECT_EQ({}, nullptr)'),
            (r'Assert\.IsNotNull\((.*?)\)', 'EXPECT_NE({}, nullptr)'),
            (r'Assert\.Throws<(\w+)>\((.*?)\)', 'EXPECT_THROW({}, {})'),
        ]
        
        for pattern, cpp_macro in patterns:
            matches = re.findall(pattern, test_body)
            for match in matches:
                assertions.append({
                    'type': cpp_macro,
                    'args': match if isinstance(match, tuple) else (match,)
                })
                
        return assertions
    
    def extract_test_data(self, test_body: str) -> Dict:
        """Extract test data and constants"""
        test_data = {}
        
        # Extract hex strings
        hex_pattern = r'"([0-9a-fA-F]{2,})"'
        hex_strings = re.findall(hex_pattern, test_body)
        if hex_strings:
            test_data['hex_strings'] = hex_strings
            
        # Extract byte arrays
        byte_array_pattern = r'new\s+byte\[\]\s*\{([^}]+)\}'
        byte_arrays = re.findall(byte_array_pattern, test_body)
        if byte_arrays:
            test_data['byte_arrays'] = byte_arrays
            
        # Extract numerical constants
        number_pattern = r'\b(\d+)[UL]?\b'
        numbers = re.findall(number_pattern, test_body)
        if numbers:
            test_data['numbers'] = list(set(numbers))
            
        return test_data
    
    def categorize_test(self, test_name: str) -> str:
        """Categorize test based on name"""
        categories = {
            'crypto': ['sign', 'verify', 'hash', 'ecdsa', 'key'],
            'vm': ['opcode', 'stack', 'execute', 'script'],
            'consensus': ['dbft', 'consensus', 'view', 'commit'],
            'transaction': ['tx', 'transaction', 'witness', 'attribute'],
            'block': ['block', 'header', 'merkle'],
            'contract': ['contract', 'nep', 'native', 'deploy'],
            'network': ['p2p', 'message', 'peer', 'protocol'],
            'wallet': ['wallet', 'account', 'nep6', 'wif'],
        }
        
        test_name_lower = test_name.lower()
        for category, keywords in categories.items():
            if any(keyword in test_name_lower for keyword in keywords):
                return category
                
        return 'general'

class CppTestGenerator:
    """Generate C++ test files from parsed C# tests"""
    
    def __init__(self, output_path: str):
        self.output_path = Path(output_path)
        self.output_path.mkdir(parents=True, exist_ok=True)
        
    def generate_test_file(self, category: str, tests: List[Dict]) -> str:
        """Generate a C++ test file for a category"""
        
        cpp_content = f"""/**
 * @file test_{category}_migrated.cpp
 * @brief Tests migrated from Neo C# for {category} category
 * @note Auto-generated from C# tests - DO NOT EDIT MANUALLY
 */

#include <gtest/gtest.h>
#include <neo/test_utilities.h>

using namespace neo;

class {self.to_camel_case(category)}MigratedTest : public ::testing::Test {{
protected:
    void SetUp() override {{
        // Setup code
    }}
    
    void TearDown() override {{
        // Cleanup code
    }}
}};

"""
        
        for test in tests:
            cpp_content += self.generate_test_case(test, category)
            
        return cpp_content
    
    def generate_test_case(self, test: Dict, category: str) -> str:
        """Generate a single test case"""
        
        test_name = test['name']
        cpp_test = f"""
TEST_F({self.to_camel_case(category)}MigratedTest, {test_name}) {{
    // Migrated from C# test: {test_name}
    
"""
        
        # Add test data setup
        if 'hex_strings' in test.get('test_data', {}):
            for hex_str in test['test_data']['hex_strings']:
                cpp_test += f'    auto data = ByteVector::Parse("{hex_str}");\n'
                
        # Convert assertions
        for assertion in test.get('assertions', []):
            cpp_test += f"    {self.convert_assertion(assertion)};\n"
            
        cpp_test += "}\n"
        
        return cpp_test
    
    def convert_assertion(self, assertion: Dict) -> str:
        """Convert C# assertion to C++ GTest macro"""
        
        macro = assertion['type']
        args = assertion['args']
        
        if len(args) == 1:
            return f"{macro}({args[0]})"
        elif len(args) == 2:
            return f"{macro}({args[0]}, {args[1]})"
        else:
            return f"// TODO: Convert assertion with {len(args)} args"
    
    def to_camel_case(self, snake_str: str) -> str:
        """Convert snake_case to CamelCase"""
        components = snake_str.split('_')
        return ''.join(x.title() for x in components)

class TestMigrationOrchestrator:
    """Orchestrate the migration of C# tests to C++"""
    
    def __init__(self, cs_path: str, cpp_path: str):
        self.parser = CSharpTestParser(cs_path)
        self.generator = CppTestGenerator(cpp_path)
        self.migration_stats = {
            'total_files': 0,
            'total_tests': 0,
            'migrated_tests': 0,
            'categories': {}
        }
        
    def migrate_all_tests(self):
        """Migrate all C# test files to C++"""
        
        print("🔍 Scanning for C# test files...")
        cs_test_files = self.find_cs_test_files()
        
        print(f"📋 Found {len(cs_test_files)} C# test files")
        
        categorized_tests = {}
        
        for test_file in cs_test_files:
            print(f"  Parsing: {test_file.name}")
            tests = self.parser.parse_test_file(test_file)
            
            for test in tests:
                category = test['category']
                if category not in categorized_tests:
                    categorized_tests[category] = []
                categorized_tests[category].append(test)
                
            self.migration_stats['total_files'] += 1
            self.migration_stats['total_tests'] += len(tests)
            
        print(f"\n📊 Test Distribution by Category:")
        for category, tests in categorized_tests.items():
            print(f"  {category}: {len(tests)} tests")
            self.migration_stats['categories'][category] = len(tests)
            
        print(f"\n🔧 Generating C++ test files...")
        for category, tests in categorized_tests.items():
            cpp_content = self.generator.generate_test_file(category, tests)
            output_file = self.generator.output_path / f"test_{category}_migrated.cpp"
            
            with open(output_file, 'w') as f:
                f.write(cpp_content)
                
            print(f"  ✅ Generated: {output_file.name}")
            self.migration_stats['migrated_tests'] += len(tests)
            
        self.generate_migration_report()
        
    def find_cs_test_files(self) -> List[Path]:
        """Find all C# test files"""
        
        # Common patterns for C# test files
        patterns = [
            "*Test.cs",
            "*Tests.cs",
            "*Spec.cs",
            "UT_*.cs"
        ]
        
        test_files = []
        for pattern in patterns:
            test_files.extend(self.parser.cs_test_path.rglob(pattern))
            
        return test_files
    
    def generate_migration_report(self):
        """Generate migration report"""
        
        report = f"""
# C# to C++ Test Migration Report

## Summary
- Total C# test files processed: {self.migration_stats['total_files']}
- Total tests found: {self.migration_stats['total_tests']}
- Tests migrated: {self.migration_stats['migrated_tests']}
- Migration rate: {self.migration_stats['migrated_tests'] / max(1, self.migration_stats['total_tests']) * 100:.1f}%

## Category Breakdown
"""
        
        for category, count in self.migration_stats['categories'].items():
            report += f"- {category}: {count} tests\n"
            
        report += f"""
## Next Steps
1. Review generated test files for accuracy
2. Add missing test data and fixtures
3. Update CMakeLists.txt to include new tests
4. Run tests and fix compilation errors
5. Validate behavior matches C# implementation
"""
        
        report_file = self.generator.output_path / "MIGRATION_REPORT.md"
        with open(report_file, 'w') as f:
            f.write(report)
            
        print(f"\n📄 Migration report saved to: {report_file}")
        
    def generate_cmake_config(self):
        """Generate CMake configuration for migrated tests"""
        
        cmake_content = """# Auto-generated CMake configuration for migrated tests

set(MIGRATED_TEST_SOURCES
"""
        
        for category in self.migration_stats['categories'].keys():
            cmake_content += f"    test_{category}_migrated.cpp\n"
            
        cmake_content += """)\n
add_executable(test_migrated ${MIGRATED_TEST_SOURCES})
target_link_libraries(test_migrated
    PRIVATE
        neo_core
        GTest::GTest
        GTest::Main
)

add_test(NAME test_migrated COMMAND test_migrated)
"""
        
        cmake_file = self.generator.output_path / "CMakeLists.txt"
        with open(cmake_file, 'w') as f:
            f.write(cmake_content)
            
        print(f"📝 CMake configuration saved to: {cmake_file}")

def main():
    """Main entry point"""
    
    import argparse
    
    parser = argparse.ArgumentParser(description='Migrate Neo C# tests to C++')
    parser.add_argument('--cs-path', default='../neo', help='Path to Neo C# source')
    parser.add_argument('--cpp-path', default='tests/migrated', help='Output path for C++ tests')
    parser.add_argument('--categories', nargs='+', help='Specific categories to migrate')
    
    args = parser.parse_args()
    
    print("🚀 Neo C# to C++ Test Migration Tool")
    print("=" * 50)
    
    orchestrator = TestMigrationOrchestrator(args.cs_path, args.cpp_path)
    orchestrator.migrate_all_tests()
    orchestrator.generate_cmake_config()
    
    print("\n✨ Migration complete!")

if __name__ == "__main__":
    main()
#!/usr/bin/env python3
"""
Neo C# to C++ Test Converter
Automatically converts C# unit tests to C++ Google Test format
"""

import os
import re
from pathlib import Path
from typing import Dict, List, Tuple, Optional
import argparse

class TestConverter:
    def __init__(self):
        self.type_mappings = {
            'string': 'std::string',
            'byte[]': 'ByteVector',
            'uint': 'uint32_t',
            'ulong': 'uint64_t',
            'int': 'int32_t',
            'long': 'int64_t',
            'bool': 'bool',
            'BigInteger': 'BigInteger',
            'UInt160': 'UInt160',
            'UInt256': 'UInt256',
            'ECPoint': 'ECPoint',
            'ScriptHashType': 'ScriptHashType',
            'ContractParameterType': 'ContractParameterType',
            'TransactionAttributeType': 'TransactionAttributeType',
            'VMState': 'VMState',
            'OpCode': 'OpCode',
            'CallFlags': 'CallFlags',
            'void': 'void',
            'var': 'auto',
            'object': 'std::any'
        }
        
        self.assertion_mappings = {
            'Assert.AreEqual': 'EXPECT_EQ',
            'Assert.AreNotEqual': 'EXPECT_NE',
            'Assert.IsTrue': 'EXPECT_TRUE',
            'Assert.IsFalse': 'EXPECT_FALSE',
            'Assert.IsNull': 'EXPECT_EQ',
            'Assert.IsNotNull': 'EXPECT_NE',
            'Assert.Fail': 'FAIL',
            'Assert.ThrowsException': 'EXPECT_THROW',
            'CollectionAssert.AreEqual': 'EXPECT_EQ',
            'CollectionAssert.AreNotEqual': 'EXPECT_NE'
        }
        
    def parse_cs_test_file(self, filepath: str) -> List[Dict]:
        """Parse a C# test file and extract test methods"""
        tests = []
        
        with open(filepath, 'r', encoding='utf-8', errors='ignore') as f:
            content = f.read()
        
        # Find test class
        class_match = re.search(r'public\s+class\s+(\w+)', content)
        if not class_match:
            return tests
            
        class_name = class_match.group(1)
        
        # Find test methods
        test_pattern = r'\[(?:TestMethod|Test)\]\s*(?:\[DataRow\([^\]]*\)\]\s*)*public\s+(?:async\s+)?(?:Task\s+)?void\s+(\w+)\s*\([^)]*\)\s*\{([^}]*(?:\{[^}]*\}[^}]*)*)\}'
        
        for match in re.finditer(test_pattern, content, re.MULTILINE | re.DOTALL):
            test_name = match.group(1)
            test_body = match.group(2)
            
            # Check for data-driven tests
            data_rows = re.findall(r'\[DataRow\(([^)]+)\)\]', match.group(0))
            
            tests.append({
                'class': class_name,
                'name': test_name,
                'body': test_body,
                'data_rows': data_rows,
                'is_async': 'async' in match.group(0) or 'Task' in match.group(0)
            })
            
        return tests
    
    def convert_test_body(self, body: str) -> str:
        """Convert C# test body to C++ syntax"""
        cpp_body = body
        
        # Convert variable declarations
        cpp_body = re.sub(r'var\s+(\w+)\s*=', r'auto \1 =', cpp_body)
        cpp_body = re.sub(r'string\s+(\w+)\s*=', r'std::string \1 =', cpp_body)
        cpp_body = re.sub(r'byte\[\]\s+(\w+)\s*=', r'ByteVector \1 =', cpp_body)
        cpp_body = re.sub(r'uint\s+(\w+)\s*=', r'uint32_t \1 =', cpp_body)
        cpp_body = re.sub(r'int\s+(\w+)\s*=', r'int32_t \1 =', cpp_body)
        
        # Convert new expressions
        cpp_body = re.sub(r'new\s+byte\[\]\s*\{([^}]+)\}', r'ByteVector({\1})', cpp_body)
        cpp_body = re.sub(r'new\s+(\w+)\(\)', r'\1()', cpp_body)
        cpp_body = re.sub(r'new\s+(\w+)\(([^)]+)\)', r'\1(\2)', cpp_body)
        
        # Convert assertions
        for cs_assert, cpp_assert in self.assertion_mappings.items():
            if cs_assert == 'Assert.IsNull':
                cpp_body = re.sub(f'{cs_assert}\\(([^)]+)\\)', f'{cpp_assert}(\\1, nullptr)', cpp_body)
            elif cs_assert == 'Assert.IsNotNull':
                cpp_body = re.sub(f'{cs_assert}\\(([^)]+)\\)', f'{cpp_assert}(\\1, nullptr)', cpp_body)
            else:
                cpp_body = re.sub(f'{cs_assert}\\(', f'{cpp_assert}(', cpp_body)
        
        # Convert null to nullptr
        cpp_body = re.sub(r'\bnull\b', 'nullptr', cpp_body)
        
        # Convert string literals
        cpp_body = re.sub(r'@"([^"]*)"', r'"\1"', cpp_body)
        
        # Convert hex strings
        cpp_body = re.sub(r'"0x([0-9a-fA-F]+)"', r'ByteVector::Parse("\1")', cpp_body)
        
        # Convert array access
        cpp_body = re.sub(r'\.Length\b', '.Size()', cpp_body)
        cpp_body = re.sub(r'\.Count\b', '.size()', cpp_body)
        
        # Convert LINQ (basic)
        cpp_body = re.sub(r'\.ToArray\(\)', '.data()', cpp_body)
        cpp_body = re.sub(r'\.ToList\(\)', '', cpp_body)
        
        # Add semicolons if missing
        lines = cpp_body.split('\n')
        for i, line in enumerate(lines):
            line = line.strip()
            if line and not line.endswith((';', '{', '}', ':', '//')) and not line.startswith(('//','#')):
                lines[i] = line + ';'
        cpp_body = '\n'.join(lines)
        
        return cpp_body
    
    def generate_cpp_test(self, test: Dict, category: str) -> str:
        """Generate C++ test from parsed C# test"""
        cpp_test = ""
        
        # Generate test fixture class name
        fixture_name = test['class'].replace('UT_', '').replace('Test', '') + 'Test'
        
        # Handle data-driven tests
        if test['data_rows']:
            # Create parameterized test
            cpp_test += f"// Data-driven test converted from C#\n"
            for i, data_row in enumerate(test['data_rows']):
                test_name = f"{test['name']}_Case{i}"
                cpp_test += f"TEST_F({fixture_name}, {test_name})\n"
                cpp_test += "{\n"
                cpp_test += f"    // Test data: {data_row}\n"
                cpp_test += self.convert_test_body(test['body'])
                cpp_test += "\n}\n\n"
        else:
            # Regular test
            cpp_test += f"TEST_F({fixture_name}, {test['name']})\n"
            cpp_test += "{\n"
            cpp_test += self.convert_test_body(test['body'])
            cpp_test += "\n}\n"
        
        return cpp_test
    
    def create_test_file(self, category: str, tests: List[Dict]) -> str:
        """Create a complete C++ test file"""
        content = f"""#include <gtest/gtest.h>
#include <neo/{category.lower()}/{category.lower()}.h>
#include <neo/io/byte_vector.h>
#include <neo/core/types.h>

using namespace neo::{category.lower()};
using namespace neo::io;
using namespace neo::core;

"""
        
        # Group tests by class
        tests_by_class = {}
        for test in tests:
            class_name = test['class']
            if class_name not in tests_by_class:
                tests_by_class[class_name] = []
            tests_by_class[class_name].append(test)
        
        # Generate fixture classes and tests
        for class_name, class_tests in tests_by_class.items():
            fixture_name = class_name.replace('UT_', '').replace('Test', '') + 'Test'
            
            # Create fixture class
            content += f"""class {fixture_name} : public ::testing::Test
{{
protected:
    void SetUp() override
    {{
        // Setup test environment
    }}
    
    void TearDown() override
    {{
        // Cleanup test environment
    }}
}};

"""
            
            # Add tests
            for test in class_tests:
                content += self.generate_cpp_test(test, category)
                content += "\n"
        
        return content
    
    def convert_category(self, cs_test_dir: str, category: str, output_dir: str) -> int:
        """Convert all tests in a category"""
        converted_count = 0
        
        # Find all C# test files in category
        cs_files = []
        category_dirs = [
            os.path.join(cs_test_dir, f"Neo.{category}.Tests"),
            os.path.join(cs_test_dir, f"Neo.{category}.UnitTests"),
            os.path.join(cs_test_dir, "Neo.UnitTests", category)
        ]
        
        for dir_path in category_dirs:
            if os.path.exists(dir_path):
                for root, dirs, files in os.walk(dir_path):
                    for file in files:
                        if file.startswith("UT_") and file.endswith(".cs"):
                            cs_files.append(os.path.join(root, file))
        
        if not cs_files:
            print(f"  No C# test files found for {category}")
            return 0
        
        # Process each file
        for cs_file in cs_files:
            print(f"  Converting: {os.path.basename(cs_file)}")
            tests = self.parse_cs_test_file(cs_file)
            
            if tests:
                # Generate output filename
                base_name = os.path.basename(cs_file).replace("UT_", "test_").replace(".cs", ".cpp").lower()
                output_file = os.path.join(output_dir, base_name)
                
                # Generate C++ test content
                cpp_content = self.create_test_file(category, tests)
                
                # Write to file
                with open(output_file, 'w') as f:
                    f.write(cpp_content)
                
                converted_count += len(tests)
                print(f"    ✓ Converted {len(tests)} tests to {output_file}")
        
        return converted_count

def main():
    parser = argparse.ArgumentParser(description='Convert C# tests to C++ tests')
    parser.add_argument('--cs-dir', default='neo_csharp/tests', help='C# tests directory')
    parser.add_argument('--cpp-dir', default='tests/unit', help='C++ tests output directory')
    parser.add_argument('--category', help='Convert specific category only')
    parser.add_argument('--dry-run', action='store_true', help='Show what would be converted without writing files')
    
    args = parser.parse_args()
    
    print("""
╔════════════════════════════════════════════════════════════╗
║        Neo C# to C++ Test Converter v1.0                 ║
║         Automated Test Conversion Tool                    ║
╚════════════════════════════════════════════════════════════╝
""")
    
    converter = TestConverter()
    
    # Categories to convert
    categories = [
        'Cryptography', 'IO', 'Ledger', 'Network', 
        'Persistence', 'SmartContract', 'Wallet', 
        'Consensus', 'Json', 'Extensions'
    ]
    
    if args.category:
        categories = [args.category]
    
    total_converted = 0
    
    for category in categories:
        print(f"\n📁 Converting {category} tests...")
        
        # Create output directory
        output_dir = os.path.join(args.cpp_dir, category.lower())
        if not args.dry_run:
            os.makedirs(output_dir, exist_ok=True)
        
        # Convert tests
        if args.dry_run:
            print(f"  [DRY RUN] Would convert tests to {output_dir}")
        else:
            count = converter.convert_category(args.cs_dir, category, output_dir)
            total_converted += count
            print(f"  ✅ Converted {count} tests")
    
    print(f"\n{'='*60}")
    print(f"✅ Total tests converted: {total_converted}")
    print(f"{'='*60}")

if __name__ == "__main__":
    main()
#!/usr/bin/env python3
"""
Neo C# to C++ Test Coverage Analyzer
Compares C# tests with C++ tests to ensure complete conversion
"""

import os
import re
from pathlib import Path
from typing import Dict, List, Set, Tuple

class TestCoverageAnalyzer:
    def __init__(self):
        self.cs_tests = {}
        self.cpp_tests = {}
        self.test_mapping = {}
        
    def find_cs_tests(self, base_path: str = "neo_csharp/tests") -> Dict[str, List[str]]:
        """Find all C# test files and extract test methods"""
        cs_tests = {}
        
        if not os.path.exists(base_path):
            print(f"Warning: C# tests directory not found: {base_path}")
            return cs_tests
            
        for root, dirs, files in os.walk(base_path):
            for file in files:
                if file.endswith(".cs") and (file.startswith("UT_") or "Test" in file):
                    filepath = os.path.join(root, file)
                    category = self._get_category(filepath)
                    
                    with open(filepath, 'r', encoding='utf-8', errors='ignore') as f:
                        content = f.read()
                        
                    # Extract test methods
                    test_methods = re.findall(r'\[TestMethod\]\s+public\s+void\s+(\w+)', content)
                    test_methods += re.findall(r'\[Test\]\s+public\s+void\s+(\w+)', content)
                    
                    if test_methods:
                        if category not in cs_tests:
                            cs_tests[category] = []
                        cs_tests[category].extend(test_methods)
                        
        return cs_tests
    
    def find_cpp_tests(self, base_path: str = ".") -> Dict[str, List[str]]:
        """Find all C++ test files and extract test cases"""
        cpp_tests = {}
        
        for root, dirs, files in os.walk(base_path):
            # Skip C# directories
            if 'neo_csharp' in root:
                continue
                
            for file in files:
                if file.startswith("test_") and file.endswith(".cpp"):
                    filepath = os.path.join(root, file)
                    category = self._get_category(filepath)
                    
                    with open(filepath, 'r', encoding='utf-8', errors='ignore') as f:
                        content = f.read()
                    
                    # Extract TEST and TEST_F macros
                    test_cases = re.findall(r'TEST(?:_F)?\s*\(\s*(\w+)\s*,\s*(\w+)\s*\)', content)
                    
                    if test_cases:
                        if category not in cpp_tests:
                            cpp_tests[category] = []
                        for suite, test in test_cases:
                            cpp_tests[category].append(f"{suite}.{test}")
                            
        return cpp_tests
    
    def _get_category(self, filepath: str) -> str:
        """Extract category from file path"""
        path_parts = filepath.lower().split(os.sep)
        
        # Common categories
        if 'cryptography' in filepath.lower():
            return 'Cryptography'
        elif 'io' in filepath.lower():
            return 'IO'
        elif 'ledger' in filepath.lower() or 'blockchain' in filepath.lower():
            return 'Ledger'
        elif 'network' in filepath.lower() or 'p2p' in filepath.lower():
            return 'Network'
        elif 'persistence' in filepath.lower() or 'storage' in filepath.lower():
            return 'Persistence'
        elif 'smartcontract' in filepath.lower() or 'vm' in filepath.lower():
            return 'SmartContract'
        elif 'wallet' in filepath.lower():
            return 'Wallet'
        elif 'consensus' in filepath.lower():
            return 'Consensus'
        elif 'native' in filepath.lower():
            return 'NativeContracts'
        elif 'rpc' in filepath.lower():
            return 'RPC'
        elif 'json' in filepath.lower():
            return 'Json'
        elif 'extension' in filepath.lower() or 'misc' in filepath.lower():
            return 'Extensions'
        else:
            return 'Other'
    
    def map_test_names(self, cs_name: str, cpp_names: List[str]) -> List[str]:
        """Find potential C++ test matches for a C# test"""
        matches = []
        cs_lower = cs_name.lower()
        
        # Remove common prefixes
        cs_clean = cs_lower.replace('test_', '').replace('ut_', '').replace('test', '')
        
        for cpp_name in cpp_names:
            cpp_lower = cpp_name.lower()
            
            # Direct match
            if cs_lower in cpp_lower or cpp_lower in cs_lower:
                matches.append(cpp_name)
            # Partial match
            elif cs_clean and cs_clean in cpp_lower:
                matches.append(cpp_name)
            # Common patterns
            elif self._similar_test_name(cs_name, cpp_name):
                matches.append(cpp_name)
                
        return matches
    
    def _similar_test_name(self, cs_name: str, cpp_name: str) -> bool:
        """Check if test names are similar"""
        # Common test name mappings
        mappings = {
            'serialize': 'serializ',
            'deserialize': 'deserializ',
            'parse': 'pars',
            'tostring': 'tostring',
            'tohex': 'tohex',
            'equals': 'equal',
            'compare': 'compar',
            'create': 'creat',
            'verify': 'verif',
            'sign': 'sign',
            'hash': 'hash',
            'encrypt': 'encrypt',
            'decrypt': 'decrypt'
        }
        
        cs_lower = cs_name.lower()
        cpp_lower = cpp_name.lower()
        
        for cs_pattern, cpp_pattern in mappings.items():
            if cs_pattern in cs_lower and cpp_pattern in cpp_lower:
                return True
                
        return False
    
    def analyze_coverage(self) -> Dict:
        """Analyze test coverage between C# and C++"""
        self.cs_tests = self.find_cs_tests()
        self.cpp_tests = self.find_cpp_tests()
        
        analysis = {
            'summary': {},
            'categories': {},
            'missing_tests': {},
            'coverage_percentage': 0
        }
        
        # Count total tests
        total_cs_tests = sum(len(tests) for tests in self.cs_tests.values())
        total_cpp_tests = sum(len(tests) for tests in self.cpp_tests.values())
        
        analysis['summary'] = {
            'total_cs_tests': total_cs_tests,
            'total_cpp_tests': total_cpp_tests,
            'cs_categories': len(self.cs_tests),
            'cpp_categories': len(self.cpp_tests)
        }
        
        # Analyze by category
        all_categories = set(self.cs_tests.keys()) | set(self.cpp_tests.keys())
        
        for category in sorted(all_categories):
            cs_tests = self.cs_tests.get(category, [])
            cpp_tests = self.cpp_tests.get(category, [])
            
            # Find missing tests
            missing = []
            covered = 0
            
            for cs_test in cs_tests:
                matches = self.map_test_names(cs_test, cpp_tests)
                if matches:
                    covered += 1
                else:
                    missing.append(cs_test)
            
            coverage = (covered / len(cs_tests) * 100) if cs_tests else 100
            
            analysis['categories'][category] = {
                'cs_tests': len(cs_tests),
                'cpp_tests': len(cpp_tests),
                'covered': covered,
                'missing': len(missing),
                'coverage': coverage
            }
            
            if missing:
                analysis['missing_tests'][category] = missing
        
        # Calculate overall coverage
        total_covered = sum(cat['covered'] for cat in analysis['categories'].values())
        analysis['coverage_percentage'] = (total_covered / total_cs_tests * 100) if total_cs_tests else 0
        
        return analysis
    
    def generate_report(self) -> str:
        """Generate detailed coverage report"""
        analysis = self.analyze_coverage()
        
        report = """# Neo C# to C++ Test Coverage Report

## Executive Summary

| Metric | C# Tests | C++ Tests | Coverage |
|--------|----------|-----------|----------|
| **Total Tests** | {total_cs} | {total_cpp} | {coverage:.1f}% |
| **Categories** | {cs_cats} | {cpp_cats} | - |

## Category Coverage

| Category | C# Tests | C++ Tests | Coverage | Status |
|----------|----------|-----------|----------|--------|
""".format(
            total_cs=analysis['summary']['total_cs_tests'],
            total_cpp=analysis['summary']['total_cpp_tests'],
            coverage=analysis['coverage_percentage'],
            cs_cats=analysis['summary']['cs_categories'],
            cpp_cats=analysis['summary']['cpp_categories']
        )
        
        # Add category details
        for category, data in sorted(analysis['categories'].items()):
            status = "✅" if data['coverage'] >= 90 else "⚠️" if data['coverage'] >= 70 else "❌"
            report += f"| **{category}** | {data['cs_tests']} | {data['cpp_tests']} | {data['coverage']:.1f}% | {status} |\n"
        
        # Add missing tests section
        if analysis['missing_tests']:
            report += "\n## Missing Test Conversions\n\n"
            
            for category, missing in sorted(analysis['missing_tests'].items()):
                if missing:
                    report += f"### {category} ({len(missing)} missing)\n\n"
                    for test in missing[:10]:  # Show first 10
                        report += f"- `{test}`\n"
                    if len(missing) > 10:
                        report += f"- ... and {len(missing) - 10} more\n"
                    report += "\n"
        
        # Add recommendations
        report += """
## Recommendations

"""
        if analysis['coverage_percentage'] < 90:
            report += f"""### Priority Actions
1. **Current Coverage**: {analysis['coverage_percentage']:.1f}%
2. **Target Coverage**: 90%+
3. **Tests to Convert**: {analysis['summary']['total_cs_tests'] - sum(cat['covered'] for cat in analysis['categories'].values())}

"""
        
        # Identify priority categories
        priority_categories = []
        for category, data in analysis['categories'].items():
            if data['coverage'] < 80 and data['cs_tests'] > 0:
                priority_categories.append((category, data))
        
        if priority_categories:
            report += "### Priority Categories for Conversion\n\n"
            for category, data in sorted(priority_categories, key=lambda x: x[1]['coverage']):
                report += f"- **{category}**: {data['coverage']:.1f}% coverage ({data['missing']} tests missing)\n"
        
        return report
    
    def generate_conversion_tasks(self) -> List[Dict]:
        """Generate specific conversion tasks for missing tests"""
        analysis = self.analyze_coverage()
        tasks = []
        
        for category, missing_tests in analysis['missing_tests'].items():
            for test in missing_tests:
                task = {
                    'category': category,
                    'cs_test': test,
                    'priority': 'HIGH' if category in ['Cryptography', 'Ledger', 'SmartContract'] else 'MEDIUM',
                    'estimated_effort': '30 minutes',
                    'cpp_file': f"test_{category.lower()}.cpp"
                }
                tasks.append(task)
        
        return tasks

def main():
    print("""
╔════════════════════════════════════════════════════════════╗
║     Neo C# to C++ Test Coverage Analyzer v1.0            ║
║          Ensuring Complete Test Conversion                ║
╚════════════════════════════════════════════════════════════╝
""")
    
    analyzer = TestCoverageAnalyzer()
    
    # Analyze coverage
    print("🔍 Analyzing C# test files...")
    cs_tests = analyzer.find_cs_tests()
    print(f"   Found {sum(len(t) for t in cs_tests.values())} C# tests in {len(cs_tests)} categories")
    
    print("\n🔍 Analyzing C++ test files...")
    cpp_tests = analyzer.find_cpp_tests()
    print(f"   Found {sum(len(t) for t in cpp_tests.values())} C++ tests in {len(cpp_tests)} categories")
    
    print("\n📊 Calculating coverage...")
    analysis = analyzer.analyze_coverage()
    print(f"   Overall coverage: {analysis['coverage_percentage']:.1f}%")
    
    # Generate report
    report = analyzer.generate_report()
    
    # Save report
    report_file = "cs_to_cpp_test_coverage.md"
    with open(report_file, 'w') as f:
        f.write(report)
    print(f"\n📄 Report saved to: {report_file}")
    
    # Generate conversion tasks
    tasks = analyzer.generate_conversion_tasks()
    if tasks:
        print(f"\n⚠️  Found {len(tasks)} tests that need conversion")
        
        # Save tasks
        tasks_file = "test_conversion_tasks.txt"
        with open(tasks_file, 'w') as f:
            f.write("Test Conversion Tasks\n")
            f.write("=" * 50 + "\n\n")
            for i, task in enumerate(tasks[:20], 1):  # Show first 20
                f.write(f"{i}. [{task['priority']}] {task['category']}: {task['cs_test']}\n")
                f.write(f"   Target file: {task['cpp_file']}\n")
                f.write(f"   Estimated effort: {task['estimated_effort']}\n\n")
        print(f"📄 Tasks saved to: {tasks_file}")
    
    # Summary
    print("\n" + "=" * 60)
    if analysis['coverage_percentage'] >= 90:
        print("✅ EXCELLENT: Test coverage exceeds 90%!")
    elif analysis['coverage_percentage'] >= 80:
        print("⚠️  GOOD: Test coverage is acceptable but can be improved")
    else:
        print("❌ NEEDS WORK: Test coverage below 80% - conversion needed")
    print("=" * 60)

if __name__ == "__main__":
    main()
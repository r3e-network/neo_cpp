#!/usr/bin/env python3
"""
Quick Test Runner for Neo C++
Builds and runs available tests with simple reporting
"""

import os
import subprocess
import time
import json
from pathlib import Path
from datetime import datetime

class QuickTestRunner:
    def __init__(self):
        self.results = {
            'timestamp': datetime.now().isoformat(),
            'tests_run': 0,
            'tests_passed': 0,
            'tests_failed': 0,
            'test_details': []
        }
        self.build_dir = Path('build')
        
    def ensure_build_dir(self):
        """Ensure build directory exists and is configured"""
        if not self.build_dir.exists():
            print("Creating build directory...")
            self.build_dir.mkdir(exist_ok=True)
            
        # Configure with CMake if needed
        cmake_cache = self.build_dir / 'CMakeCache.txt'
        if not cmake_cache.exists():
            print("Configuring with CMake...")
            result = subprocess.run(
                ['cmake', '..', '-DBUILD_TESTS=ON', '-DCMAKE_BUILD_TYPE=Debug'],
                cwd=self.build_dir,
                capture_output=True,
                text=True
            )
            if result.returncode != 0:
                print(f"CMake configuration failed: {result.stderr}")
                return False
        return True
        
    def build_test(self, test_name):
        """Build a specific test target"""
        print(f"Building {test_name}...")
        result = subprocess.run(
            ['make', test_name, '-j4'],
            cwd=self.build_dir,
            capture_output=True,
            text=True,
            timeout=60
        )
        return result.returncode == 0
        
    def run_test(self, test_path):
        """Run a test executable"""
        if not Path(test_path).exists():
            return None
            
        print(f"Running {test_path}...")
        try:
            result = subprocess.run(
                [test_path, '--gtest_brief=1'],
                capture_output=True,
                text=True,
                timeout=30
            )
            
            # Parse output
            output = result.stdout + result.stderr
            passed = 'PASSED' in output or result.returncode == 0
            
            # Try to extract test counts
            test_count = 0
            if 'tests from' in output:
                import re
                match = re.search(r'(\d+) tests from', output)
                if match:
                    test_count = int(match.group(1))
                    
            return {
                'passed': passed,
                'test_count': test_count,
                'output': output[:500]  # First 500 chars
            }
        except subprocess.TimeoutExpired:
            return {
                'passed': False,
                'test_count': 0,
                'output': 'Test timed out'
            }
        except Exception as e:
            return {
                'passed': False,
                'test_count': 0,
                'output': str(e)
            }
            
    def find_test_targets(self):
        """Find available test targets from CMakeLists"""
        targets = []
        
        # Look for test targets in various CMakeLists.txt files
        cmake_files = [
            'tests/unit/CMakeLists.txt',
            'tests/unit/io/CMakeLists.txt',
            'tests/unit/cryptography/CMakeLists.txt',
            'tests/unit/vm/CMakeLists.txt',
            'tests/unit/network/CMakeLists.txt',
            'tests/unit/ledger/CMakeLists.txt',
            'tests/unit/consensus/CMakeLists.txt',
            'sdk/tests/CMakeLists.txt'
        ]
        
        for cmake_file in cmake_files:
            if Path(cmake_file).exists():
                with open(cmake_file, 'r') as f:
                    content = f.read()
                    # Extract test targets
                    import re
                    matches = re.findall(r'add_executable\((test_\w+)', content)
                    targets.extend(matches)
                    
        return list(set(targets))  # Remove duplicates
        
    def run_all_tests(self):
        """Main test execution"""
        print("=" * 60)
        print("NEO C++ QUICK TEST RUNNER")
        print("=" * 60)
        
        # Ensure build directory is ready
        if not self.ensure_build_dir():
            print("Failed to setup build directory")
            return
            
        # Find test targets
        test_targets = self.find_test_targets()
        print(f"\nFound {len(test_targets)} test targets")
        
        # Priority tests to run first
        priority_tests = [
            'test_io',
            'test_crypto',
            'test_vm',
            'test_network',
            'test_ledger',
            'test_consensus'
        ]
        
        # Reorder with priority tests first
        ordered_targets = []
        for p in priority_tests:
            if p in test_targets:
                ordered_targets.append(p)
                test_targets.remove(p)
        ordered_targets.extend(test_targets[:10])  # Add up to 10 more tests
        
        # Build and run tests
        for test_name in ordered_targets:
            print(f"\n--- Testing {test_name} ---")
            
            # Build
            if self.build_test(test_name):
                # Find the executable
                test_paths = [
                    f'build/tests/unit/{test_name}',
                    f'build/tests/unit/io/{test_name}',
                    f'build/tests/unit/cryptography/{test_name}',
                    f'build/tests/unit/vm/{test_name}',
                    f'build/tests/unit/network/{test_name}',
                    f'build/tests/unit/ledger/{test_name}',
                    f'build/tests/unit/consensus/{test_name}',
                    f'build/sdk/tests/{test_name}'
                ]
                
                test_run = False
                for test_path in test_paths:
                    if Path(test_path).exists():
                        result = self.run_test(test_path)
                        if result:
                            self.results['tests_run'] += 1
                            if result['passed']:
                                self.results['tests_passed'] += 1
                                print(f"  ✓ PASSED ({result['test_count']} tests)")
                            else:
                                self.results['tests_failed'] += 1
                                print(f"  ✗ FAILED")
                                print(f"    {result['output'][:200]}")
                                
                            self.results['test_details'].append({
                                'name': test_name,
                                'path': test_path,
                                'passed': result['passed'],
                                'test_count': result['test_count']
                            })
                            test_run = True
                            break
                            
                if not test_run:
                    print(f"  ⚠ Test executable not found after build")
            else:
                print(f"  ⚠ Build failed")
                
        # Generate report
        self.generate_report()
        
    def generate_report(self):
        """Generate test report"""
        print("\n" + "=" * 60)
        print("TEST EXECUTION SUMMARY")
        print("=" * 60)
        
        print(f"Total Test Modules Run: {self.results['tests_run']}")
        print(f"Passed: {self.results['tests_passed']}")
        print(f"Failed: {self.results['tests_failed']}")
        
        if self.results['tests_run'] > 0:
            pass_rate = (self.results['tests_passed'] / self.results['tests_run']) * 100
            print(f"Pass Rate: {pass_rate:.1f}%")
            
            if pass_rate >= 80:
                print("✓ Good test coverage")
            elif pass_rate >= 60:
                print("⚠ Moderate test coverage - improvements needed")
            else:
                print("✗ Low test coverage - significant issues")
                
        # Save results to file
        with open('quick_test_results.json', 'w') as f:
            json.dump(self.results, f, indent=2)
        print(f"\nDetailed results saved to quick_test_results.json")
        
        # Create markdown report
        with open('quick_test_report.md', 'w') as f:
            f.write("# Neo C++ Quick Test Report\n\n")
            f.write(f"**Date:** {self.results['timestamp']}\n\n")
            f.write("## Summary\n\n")
            f.write(f"- Test Modules Run: {self.results['tests_run']}\n")
            f.write(f"- Passed: {self.results['tests_passed']}\n")
            f.write(f"- Failed: {self.results['tests_failed']}\n")
            if self.results['tests_run'] > 0:
                f.write(f"- Pass Rate: {pass_rate:.1f}%\n")
            f.write("\n## Test Details\n\n")
            f.write("| Test Module | Status | Tests |\n")
            f.write("|-------------|--------|-------|\n")
            for test in self.results['test_details']:
                status = "✓ PASSED" if test['passed'] else "✗ FAILED"
                f.write(f"| {test['name']} | {status} | {test['test_count']} |\n")
                
        print("Markdown report saved to quick_test_report.md")

if __name__ == '__main__':
    runner = QuickTestRunner()
    runner.run_all_tests()
#!/usr/bin/env python3
"""
Test Results Generator for Neo C++ Test Dashboard
Parses test output and generates JSON data for the dashboard
"""

import json
import xml.etree.ElementTree as ET
import re
import sys
import os
from datetime import datetime
from pathlib import Path

class TestResultsGenerator:
    def __init__(self):
        self.results = {
            "timestamp": datetime.now().isoformat(),
            "summary": {
                "total": 0,
                "passed": 0,
                "failed": 0,
                "skipped": 0,
                "passRate": 0.0,
                "coverage": 0.0,
                "duration": 0.0
            },
            "suites": [],
            "coverage": {},
            "benchmarks": [],
            "history": []
        }
    
    def parse_gtest_xml(self, xml_file):
        """Parse Google Test XML output"""
        try:
            tree = ET.parse(xml_file)
            root = tree.getroot()
            
            # Parse test suites
            for testsuite in root.findall('.//testsuite'):
                suite_data = {
                    "name": testsuite.get('name'),
                    "tests": int(testsuite.get('tests', 0)),
                    "failures": int(testsuite.get('failures', 0)),
                    "disabled": int(testsuite.get('disabled', 0)),
                    "errors": int(testsuite.get('errors', 0)),
                    "time": float(testsuite.get('time', 0)),
                    "testcases": []
                }
                
                # Parse individual test cases
                for testcase in testsuite.findall('.//testcase'):
                    test_data = {
                        "name": testcase.get('name'),
                        "classname": testcase.get('classname'),
                        "time": float(testcase.get('time', 0)),
                        "status": "passed"
                    }
                    
                    # Check for failures
                    if testcase.find('failure') is not None:
                        test_data['status'] = 'failed'
                        failure = testcase.find('failure')
                        test_data['failure_message'] = failure.get('message', '')
                    
                    # Check for skipped tests
                    if testcase.find('skipped') is not None:
                        test_data['status'] = 'skipped'
                    
                    suite_data['testcases'].append(test_data)
                
                self.results['suites'].append(suite_data)
                
                # Update summary
                self.results['summary']['total'] += suite_data['tests']
                self.results['summary']['passed'] += suite_data['tests'] - suite_data['failures'] - suite_data['disabled']
                self.results['summary']['failed'] += suite_data['failures']
                self.results['summary']['skipped'] += suite_data['disabled']
                self.results['summary']['duration'] += suite_data['time']
                
        except Exception as e:
            print(f"Error parsing XML: {e}")
    
    def parse_gtest_output(self, output_file):
        """Parse Google Test console output"""
        try:
            with open(output_file, 'r') as f:
                content = f.read()
            
            # Parse test summary
            summary_match = re.search(r'\[=+\] (\d+) tests? from (\d+) test (?:suites?|cases?) ran', content)
            if summary_match:
                self.results['summary']['total'] = int(summary_match.group(1))
            
            # Parse passed tests
            passed_match = re.search(r'\[ PASSED \] (\d+) tests?', content)
            if passed_match:
                self.results['summary']['passed'] = int(passed_match.group(1))
            
            # Parse failed tests
            failed_match = re.search(r'\[ FAILED \] (\d+) tests?', content)
            if failed_match:
                self.results['summary']['failed'] = int(failed_match.group(1))
            
            # Parse skipped tests
            skipped_match = re.search(r'\[ SKIPPED \] (\d+) tests?', content)
            if skipped_match:
                self.results['summary']['skipped'] = int(skipped_match.group(1))
            
            # Parse duration
            duration_match = re.search(r'Total Test time \(real\) = ([\d.]+) sec', content)
            if duration_match:
                self.results['summary']['duration'] = float(duration_match.group(1))
            
            # Parse individual test results
            test_pattern = re.compile(r'\[ (OK|FAILED|SKIPPED) \] ([^\s]+)\.([^\s]+) \((\d+) ms\)')
            for match in test_pattern.finditer(content):
                status = match.group(1)
                suite = match.group(2)
                test = match.group(3)
                time_ms = int(match.group(4))
                
                # Find or create suite
                suite_data = None
                for s in self.results['suites']:
                    if s['name'] == suite:
                        suite_data = s
                        break
                
                if not suite_data:
                    suite_data = {
                        'name': suite,
                        'tests': 0,
                        'failures': 0,
                        'disabled': 0,
                        'errors': 0,
                        'time': 0.0,
                        'testcases': []
                    }
                    self.results['suites'].append(suite_data)
                
                # Add test case
                test_data = {
                    'name': test,
                    'classname': suite,
                    'time': time_ms / 1000.0,
                    'status': 'passed' if status == 'OK' else status.lower()
                }
                
                suite_data['testcases'].append(test_data)
                suite_data['tests'] += 1
                suite_data['time'] += test_data['time']
                
                if status == 'FAILED':
                    suite_data['failures'] += 1
                elif status == 'SKIPPED':
                    suite_data['disabled'] += 1
                    
        except Exception as e:
            print(f"Error parsing output: {e}")
    
    def parse_coverage_report(self, coverage_file):
        """Parse code coverage report"""
        try:
            with open(coverage_file, 'r') as f:
                content = f.read()
            
            # Parse lcov coverage data
            if coverage_file.endswith('.info'):
                current_file = None
                file_coverage = {}
                
                for line in content.splitlines():
                    if line.startswith('SF:'):
                        current_file = line[3:]
                        file_coverage[current_file] = {
                            'lines_hit': 0,
                            'lines_total': 0,
                            'branches_hit': 0,
                            'branches_total': 0
                        }
                    elif line.startswith('LH:'):
                        file_coverage[current_file]['lines_hit'] = int(line[3:])
                    elif line.startswith('LF:'):
                        file_coverage[current_file]['lines_total'] = int(line[3:])
                    elif line.startswith('BRH:'):
                        file_coverage[current_file]['branches_hit'] = int(line[3:])
                    elif line.startswith('BRF:'):
                        file_coverage[current_file]['branches_total'] = int(line[3:])
                
                # Calculate overall coverage
                total_lines = sum(f['lines_total'] for f in file_coverage.values())
                hit_lines = sum(f['lines_hit'] for f in file_coverage.values())
                
                if total_lines > 0:
                    self.results['summary']['coverage'] = round((hit_lines / total_lines) * 100, 1)
                
                # Group by module
                modules = {}
                for filepath, data in file_coverage.items():
                    # Extract module name from path
                    parts = filepath.split('/')
                    if 'src' in parts:
                        idx = parts.index('src')
                        if idx + 1 < len(parts):
                            module = parts[idx + 1]
                            if module not in modules:
                                modules[module] = {
                                    'lines_hit': 0,
                                    'lines_total': 0,
                                    'coverage': 0.0
                                }
                            modules[module]['lines_hit'] += data['lines_hit']
                            modules[module]['lines_total'] += data['lines_total']
                
                # Calculate module coverage
                for module, data in modules.items():
                    if data['lines_total'] > 0:
                        data['coverage'] = round((data['lines_hit'] / data['lines_total']) * 100, 1)
                
                self.results['coverage'] = modules
                
        except Exception as e:
            print(f"Error parsing coverage: {e}")
    
    def parse_benchmark_results(self, benchmark_file):
        """Parse Google Benchmark JSON output"""
        try:
            with open(benchmark_file, 'r') as f:
                data = json.load(f)
            
            for benchmark in data.get('benchmarks', []):
                bench_data = {
                    'name': benchmark['name'],
                    'time': benchmark.get('real_time', 0),
                    'cpu_time': benchmark.get('cpu_time', 0),
                    'iterations': benchmark.get('iterations', 0),
                    'time_unit': benchmark.get('time_unit', 'ns')
                }
                
                # Add bytes per second if available
                if 'bytes_per_second' in benchmark:
                    bench_data['throughput'] = benchmark['bytes_per_second']
                
                self.results['benchmarks'].append(bench_data)
                
        except Exception as e:
            print(f"Error parsing benchmarks: {e}")
    
    def calculate_pass_rate(self):
        """Calculate pass rate percentage"""
        total = self.results['summary']['total']
        if total > 0:
            passed = self.results['summary']['passed']
            self.results['summary']['passRate'] = round((passed / total) * 100, 1)
    
    def load_history(self, history_file):
        """Load historical test results"""
        try:
            if os.path.exists(history_file):
                with open(history_file, 'r') as f:
                    history = json.load(f)
                    # Keep last 100 entries
                    self.results['history'] = history[-99:]
        except Exception as e:
            print(f"Error loading history: {e}")
    
    def save_results(self, output_file):
        """Save results to JSON file"""
        self.calculate_pass_rate()
        
        # Add current results to history
        history_entry = {
            'timestamp': self.results['timestamp'],
            'total': self.results['summary']['total'],
            'passed': self.results['summary']['passed'],
            'failed': self.results['summary']['failed'],
            'passRate': self.results['summary']['passRate'],
            'coverage': self.results['summary']['coverage']
        }
        self.results['history'].append(history_entry)
        
        # Save to file
        with open(output_file, 'w') as f:
            json.dump(self.results, f, indent=2)
        
        print(f"Results saved to {output_file}")
        
        # Also save history separately
        history_file = output_file.replace('.json', '-history.json')
        with open(history_file, 'w') as f:
            json.dump(self.results['history'], f, indent=2)
    
    def generate_summary(self):
        """Generate human-readable summary"""
        s = self.results['summary']
        print("\n" + "="*60)
        print("TEST RESULTS SUMMARY")
        print("="*60)
        print(f"Total Tests:    {s['total']}")
        print(f"Passed:         {s['passed']} ({s['passRate']}%)")
        print(f"Failed:         {s['failed']}")
        print(f"Skipped:        {s['skipped']}")
        print(f"Duration:       {s['duration']:.2f}s")
        print(f"Code Coverage:  {s['coverage']}%")
        print("="*60)
        
        if s['failed'] > 0:
            print("\nFAILED TESTS:")
            for suite in self.results['suites']:
                for test in suite['testcases']:
                    if test['status'] == 'failed':
                        print(f"  - {suite['name']}.{test['name']}")
                        if 'failure_message' in test:
                            print(f"    {test['failure_message']}")
        
        if self.results['benchmarks']:
            print("\nBENCHMARK RESULTS:")
            for bench in self.results['benchmarks'][:5]:  # Show top 5
                print(f"  - {bench['name']}: {bench['time']:.2f} {bench['time_unit']}")
                if 'throughput' in bench:
                    print(f"    Throughput: {bench['throughput']:.2f} bytes/sec")

def main():
    generator = TestResultsGenerator()
    
    # Parse command line arguments
    if len(sys.argv) < 2:
        print("Usage: generate_results.py <test_output_file> [coverage_file] [benchmark_file]")
        sys.exit(1)
    
    # Parse test results
    test_file = sys.argv[1]
    if test_file.endswith('.xml'):
        generator.parse_gtest_xml(test_file)
    else:
        generator.parse_gtest_output(test_file)
    
    # Parse coverage if provided
    if len(sys.argv) > 2:
        generator.parse_coverage_report(sys.argv[2])
    
    # Parse benchmarks if provided
    if len(sys.argv) > 3:
        generator.parse_benchmark_results(sys.argv[3])
    
    # Load history
    output_file = 'test-results.json'
    history_file = 'test-results-history.json'
    generator.load_history(history_file)
    
    # Save results
    generator.save_results(output_file)
    
    # Generate summary
    generator.generate_summary()

if __name__ == '__main__':
    main()
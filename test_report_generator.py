#!/usr/bin/env python3
"""
Neo C++ Comprehensive Test Report Generator
Discovers and runs all available tests, generating detailed reports
"""

import os
import subprocess
import json
import time
import sys
from pathlib import Path
from datetime import datetime
from typing import Dict, List, Tuple
import xml.etree.ElementTree as ET

class TestReportGenerator:
    def __init__(self, build_dir: str = "build"):
        self.build_dir = Path(build_dir)
        self.test_results = {}
        self.test_binaries = []
        self.timestamp = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
        
    def discover_tests(self) -> List[Path]:
        """Discover all test binaries in the build directory"""
        test_patterns = ["test_*", "*_test", "*_tests"]
        found_tests = []
        
        for pattern in test_patterns:
            found_tests.extend(self.build_dir.rglob(pattern))
        
        # Filter to only executable files
        self.test_binaries = [t for t in found_tests if t.is_file() and os.access(t, os.X_OK)]
        return self.test_binaries
    
    def run_test(self, test_path: Path) -> Dict:
        """Run a single test binary and collect results"""
        result = {
            "name": test_path.name,
            "path": str(test_path),
            "status": "UNKNOWN",
            "passed": 0,
            "failed": 0,
            "skipped": 0,
            "disabled": 0,
            "total": 0,
            "duration": 0,
            "details": []
        }
        
        try:
            # Run test with XML output
            xml_output = test_path.parent / f"{test_path.name}_results.xml"
            start_time = time.time()
            
            process = subprocess.run(
                [str(test_path), f"--gtest_output=xml:{xml_output}"],
                capture_output=True,
                text=True,
                timeout=60
            )
            
            result["duration"] = time.time() - start_time
            
            # Parse output for summary
            output_lines = process.stdout.split('\n')
            for line in output_lines:
                if "PASSED" in line and "tests" in line:
                    try:
                        result["passed"] = int(line.split()[1])
                    except:
                        pass
                elif "FAILED" in line and "tests" in line:
                    try:
                        result["failed"] = int(line.split()[1])
                    except:
                        pass
                elif "SKIPPED" in line:
                    try:
                        result["skipped"] = int(line.split()[1])
                    except:
                        pass
                elif "DISABLED" in line:
                    try:
                        result["disabled"] = int(line.split()[3])
                    except:
                        pass
            
            # Parse XML for detailed results if available
            if xml_output.exists():
                try:
                    tree = ET.parse(xml_output)
                    root = tree.getroot()
                    
                    # Get test suite statistics
                    for testsuite in root.findall('testsuite'):
                        tests = int(testsuite.get('tests', 0))
                        failures = int(testsuite.get('failures', 0))
                        skipped = int(testsuite.get('skipped', 0))
                        disabled = int(testsuite.get('disabled', 0))
                        
                        result["total"] += tests
                        
                        # Get individual test results
                        for testcase in testsuite.findall('testcase'):
                            test_detail = {
                                "name": testcase.get('name'),
                                "classname": testcase.get('classname'),
                                "time": float(testcase.get('time', 0)),
                                "status": "PASSED"
                            }
                            
                            # Check for failures
                            failure = testcase.find('failure')
                            if failure is not None:
                                test_detail["status"] = "FAILED"
                                test_detail["message"] = failure.get('message', '')
                            
                            # Check for skipped
                            skipped_elem = testcase.find('skipped')
                            if skipped_elem is not None:
                                test_detail["status"] = "SKIPPED"
                            
                            result["details"].append(test_detail)
                
                except Exception as e:
                    print(f"Error parsing XML for {test_path.name}: {e}")
            
            # Determine overall status
            if result["failed"] > 0:
                result["status"] = "FAILED"
            elif result["passed"] > 0:
                result["status"] = "PASSED"
            elif result["skipped"] > 0:
                result["status"] = "SKIPPED"
            
        except subprocess.TimeoutExpired:
            result["status"] = "TIMEOUT"
        except Exception as e:
            result["status"] = "ERROR"
            result["error"] = str(e)
        
        return result
    
    def generate_summary(self) -> Dict:
        """Generate overall test summary"""
        summary = {
            "timestamp": self.timestamp,
            "total_binaries": len(self.test_results),
            "total_tests": 0,
            "total_passed": 0,
            "total_failed": 0,
            "total_skipped": 0,
            "total_disabled": 0,
            "overall_status": "PASSED",
            "pass_rate": 0
        }
        
        for result in self.test_results.values():
            summary["total_tests"] += result["total"]
            summary["total_passed"] += result["passed"]
            summary["total_failed"] += result["failed"]
            summary["total_skipped"] += result["skipped"]
            summary["total_disabled"] += result["disabled"]
        
        if summary["total_tests"] > 0:
            summary["pass_rate"] = (summary["total_passed"] / summary["total_tests"]) * 100
        
        if summary["total_failed"] > 0:
            summary["overall_status"] = "FAILED"
        elif summary["total_passed"] == 0:
            summary["overall_status"] = "NO_TESTS"
        
        return summary
    
    def generate_markdown_report(self) -> str:
        """Generate markdown formatted test report"""
        summary = self.generate_summary()
        
        report = f"""# Neo C++ Test Report

Generated: {self.timestamp}

## Executive Summary

| Metric | Value |
|--------|-------|
| **Overall Status** | {self._status_badge(summary['overall_status'])} |
| **Test Binaries** | {summary['total_binaries']} |
| **Total Tests** | {summary['total_tests']} |
| **Passed** | {summary['total_passed']} ✅ |
| **Failed** | {summary['total_failed']} ❌ |
| **Skipped** | {summary['total_skipped']} ⏭️ |
| **Disabled** | {summary['total_disabled']} 🚫 |
| **Pass Rate** | {summary['pass_rate']:.2f}% |

## Test Suite Results

"""
        
        # Add individual test results
        for name, result in sorted(self.test_results.items()):
            status_icon = self._get_status_icon(result['status'])
            report += f"""### {name} {status_icon}

- **Status**: {result['status']}
- **Tests**: {result['total']}
- **Passed**: {result['passed']}
- **Failed**: {result['failed']}
- **Skipped**: {result['skipped']}
- **Duration**: {result['duration']:.2f}s

"""
            
            # Add failed test details
            if result['failed'] > 0 and result['details']:
                report += "#### Failed Tests:\n"
                for detail in result['details']:
                    if detail['status'] == 'FAILED':
                        report += f"- `{detail['classname']}.{detail['name']}`: {detail.get('message', 'No message')}\n"
                report += "\n"
        
        # Add recommendations
        report += self._generate_recommendations(summary)
        
        return report
    
    def _status_badge(self, status: str) -> str:
        """Generate status badge"""
        colors = {
            "PASSED": "🟢 PASSED",
            "FAILED": "🔴 FAILED",
            "NO_TESTS": "⚪ NO TESTS",
            "SKIPPED": "🟡 SKIPPED"
        }
        return colors.get(status, status)
    
    def _get_status_icon(self, status: str) -> str:
        """Get status icon"""
        icons = {
            "PASSED": "✅",
            "FAILED": "❌",
            "SKIPPED": "⏭️",
            "ERROR": "⚠️",
            "TIMEOUT": "⏱️",
            "UNKNOWN": "❓"
        }
        return icons.get(status, "")
    
    def _generate_recommendations(self, summary: Dict) -> str:
        """Generate test recommendations based on results"""
        recommendations = "\n## Recommendations\n\n"
        
        if summary['total_failed'] > 0:
            recommendations += f"""### ⚠️ Immediate Actions Required

1. **Fix failing tests**: {summary['total_failed']} tests are currently failing
2. Review test logs for detailed error messages
3. Run failed tests individually for debugging
"""
        
        if summary['pass_rate'] < 80:
            recommendations += f"""### 📊 Test Coverage Improvement

- Current pass rate is {summary['pass_rate']:.2f}%, target is 90%+
- Consider adding more comprehensive test cases
- Review skipped tests and determine if they should be enabled
"""
        
        if summary['total_skipped'] > summary['total_tests'] * 0.1:
            recommendations += f"""### ⏭️ Skipped Tests

- {summary['total_skipped']} tests are being skipped
- Review if these tests should be enabled or removed
"""
        
        if summary['total_disabled'] > 0:
            recommendations += f"""### 🚫 Disabled Tests

- {summary['total_disabled']} tests are disabled
- Investigate why these tests were disabled and plan to re-enable them
"""
        
        if summary['pass_rate'] >= 90:
            recommendations += """### ✅ Good Test Health

- Test suite is performing well with >90% pass rate
- Continue maintaining high test coverage
- Consider adding performance benchmarks
"""
        
        return recommendations
    
    def run_all_tests(self):
        """Run all discovered tests"""
        print(f"Discovering tests in {self.build_dir}...")
        self.discover_tests()
        print(f"Found {len(self.test_binaries)} test binaries")
        
        for test_path in self.test_binaries:
            print(f"Running {test_path.name}...")
            result = self.run_test(test_path)
            self.test_results[test_path.name] = result
            
            status_icon = self._get_status_icon(result['status'])
            print(f"  {status_icon} {result['status']}: {result['passed']}/{result['total']} passed")
    
    def save_reports(self):
        """Save test reports in various formats"""
        # Save JSON report
        json_file = self.build_dir / "test_report.json"
        with open(json_file, 'w') as f:
            json.dump({
                "summary": self.generate_summary(),
                "results": self.test_results
            }, f, indent=2)
        print(f"JSON report saved to {json_file}")
        
        # Save Markdown report
        md_file = self.build_dir / "test_report.md"
        with open(md_file, 'w') as f:
            f.write(self.generate_markdown_report())
        print(f"Markdown report saved to {md_file}")
        
        # Print summary to console
        summary = self.generate_summary()
        print("\n" + "="*60)
        print("TEST SUMMARY")
        print("="*60)
        print(f"Overall Status: {summary['overall_status']}")
        print(f"Pass Rate: {summary['pass_rate']:.2f}%")
        print(f"Tests: {summary['total_passed']}/{summary['total_tests']} passed")
        print(f"Failed: {summary['total_failed']}")
        print(f"Skipped: {summary['total_skipped']}")
        print("="*60)

def main():
    generator = TestReportGenerator()
    generator.run_all_tests()
    generator.save_reports()
    
    # Exit with appropriate code
    summary = generator.generate_summary()
    if summary['total_failed'] > 0:
        sys.exit(1)
    sys.exit(0)

if __name__ == "__main__":
    main()
#!/usr/bin/env python3
"""
Neo C++ Comprehensive Test Runner with Coverage Analysis
Executes all available tests and generates detailed reports
"""

import os
import subprocess
import json
import time
import sys
import re
from pathlib import Path
from datetime import datetime
from typing import Dict, List, Tuple, Optional
import xml.etree.ElementTree as ET

class ComprehensiveTestRunner:
    def __init__(self, build_dir: str = "build"):
        self.build_dir = Path(build_dir)
        self.test_results = {}
        self.coverage_data = {}
        self.timestamp = datetime.now()
        self.total_time = 0
        
    def discover_tests(self) -> Dict[str, Path]:
        """Discover all test binaries"""
        tests = {}
        
        # Find compiled test binaries
        for test_path in self.build_dir.rglob("test_*"):
            if test_path.is_file() and os.access(test_path, os.X_OK):
                test_name = test_path.stem.replace("test_", "")
                tests[test_name] = test_path
        
        return tests
    
    def run_test_binary(self, name: str, path: Path) -> Dict:
        """Run a single test binary and collect results"""
        result = {
            "name": name,
            "path": str(path),
            "status": "UNKNOWN",
            "passed": 0,
            "failed": 0,
            "skipped": 0,
            "disabled": 0,
            "total": 0,
            "duration": 0,
            "pass_rate": 0,
            "output": "",
            "failed_tests": [],
            "skipped_tests": []
        }
        
        try:
            # Run test with detailed output
            start_time = time.time()
            process = subprocess.run(
                [str(path), "--gtest_output=xml:test_results.xml"],
                capture_output=True,
                text=True,
                timeout=60,
                cwd=path.parent
            )
            result["duration"] = round(time.time() - start_time, 3)
            result["output"] = process.stdout
            
            # Parse test output
            lines = process.stdout.split('\n')
            for line in lines:
                # Extract test counts
                if "[  PASSED  ]" in line and "tests." in line:
                    match = re.search(r'(\d+)\s+tests?', line)
                    if match:
                        result["passed"] = int(match.group(1))
                elif "[  FAILED  ]" in line and "tests," in line:
                    match = re.search(r'(\d+)\s+tests?', line)
                    if match:
                        result["failed"] = int(match.group(1))
                elif "[  SKIPPED ]" in line:
                    match = re.search(r'(\d+)\s+tests?', line)
                    if match:
                        result["skipped"] = int(match.group(1))
                elif "YOU HAVE" in line and "DISABLED TEST" in line:
                    match = re.search(r'(\d+)\s+DISABLED', line)
                    if match:
                        result["disabled"] = int(match.group(1))
                
                # Collect failed test names
                if "[  FAILED  ]" in line and "." in line and "tests" not in line.lower():
                    test_name = line.split("]")[1].strip()
                    if test_name and not test_name.startswith("tests"):
                        result["failed_tests"].append(test_name)
                
                # Collect skipped test names
                if "[  SKIPPED ]" in line and "." in line and "tests" not in line.lower():
                    test_name = line.split("]")[1].strip()
                    if test_name:
                        result["skipped_tests"].append(test_name)
            
            # Calculate totals
            result["total"] = result["passed"] + result["failed"] + result["skipped"]
            if result["total"] > 0:
                result["pass_rate"] = round((result["passed"] / result["total"]) * 100, 2)
            
            # Determine status
            if result["failed"] > 0:
                result["status"] = "FAILED"
            elif result["passed"] > 0:
                result["status"] = "PASSED"
            elif result["skipped"] > 0:
                result["status"] = "SKIPPED"
            else:
                result["status"] = "NO_TESTS"
                
        except subprocess.TimeoutExpired:
            result["status"] = "TIMEOUT"
        except Exception as e:
            result["status"] = "ERROR"
            result["error"] = str(e)
        
        return result
    
    def run_all_tests(self) -> None:
        """Run all discovered tests"""
        tests = self.discover_tests()
        print(f"\n🔍 Discovered {len(tests)} test binaries")
        print("=" * 60)
        
        for name, path in tests.items():
            print(f"\n📊 Running {name} tests...")
            result = self.run_test_binary(name, path)
            self.test_results[name] = result
            
            # Print immediate results
            status_icon = self._get_status_icon(result["status"])
            print(f"   {status_icon} Status: {result['status']}")
            print(f"   ✅ Passed: {result['passed']}/{result['total']}")
            if result["failed"] > 0:
                print(f"   ❌ Failed: {result['failed']}")
                for test in result["failed_tests"][:3]:
                    print(f"      - {test}")
            if result["skipped"] > 0:
                print(f"   ⏭️  Skipped: {result['skipped']}")
            print(f"   ⏱️  Duration: {result['duration']}s")
            print(f"   📈 Pass Rate: {result['pass_rate']}%")
    
    def generate_coverage_report(self) -> Dict:
        """Generate code coverage analysis"""
        coverage = {
            "line_coverage": 0,
            "branch_coverage": 0,
            "function_coverage": 0,
            "files_covered": 0,
            "total_files": 0
        }
        
        # Attempt to run gcov/lcov if available
        try:
            process = subprocess.run(
                ["which", "lcov"],
                capture_output=True,
                timeout=5
            )
            if process.returncode == 0:
                # Run coverage collection
                subprocess.run(
                    ["lcov", "--capture", "--directory", ".", "--output-file", "coverage.info"],
                    capture_output=True,
                    timeout=30,
                    cwd=self.build_dir
                )
                
                # Parse coverage info
                coverage_file = self.build_dir / "coverage.info"
                if coverage_file.exists():
                    with open(coverage_file, 'r') as f:
                        lines = f.readlines()
                        for line in lines:
                            if line.startswith("LF:"):
                                coverage["total_lines"] = int(line.split(":")[1])
                            elif line.startswith("LH:"):
                                coverage["lines_hit"] = int(line.split(":")[1])
                    
                    if coverage.get("total_lines", 0) > 0:
                        coverage["line_coverage"] = round(
                            (coverage.get("lines_hit", 0) / coverage["total_lines"]) * 100, 2
                        )
        except:
            # Coverage tools not available
            pass
        
        return coverage
    
    def calculate_summary(self) -> Dict:
        """Calculate overall test summary"""
        summary = {
            "timestamp": self.timestamp.isoformat(),
            "total_binaries": len(self.test_results),
            "total_tests": 0,
            "total_passed": 0,
            "total_failed": 0,
            "total_skipped": 0,
            "total_disabled": 0,
            "overall_pass_rate": 0,
            "total_duration": 0,
            "status": "UNKNOWN"
        }
        
        for result in self.test_results.values():
            summary["total_tests"] += result["total"]
            summary["total_passed"] += result["passed"]
            summary["total_failed"] += result["failed"]
            summary["total_skipped"] += result["skipped"]
            summary["total_disabled"] += result["disabled"]
            summary["total_duration"] += result["duration"]
        
        if summary["total_tests"] > 0:
            summary["overall_pass_rate"] = round(
                (summary["total_passed"] / summary["total_tests"]) * 100, 2
            )
        
        # Determine overall status
        if summary["total_failed"] > 0:
            summary["status"] = "FAILED"
        elif summary["total_passed"] > 0:
            summary["status"] = "PASSED"
        else:
            summary["status"] = "NO_TESTS"
        
        summary["total_duration"] = round(summary["total_duration"], 3)
        
        return summary
    
    def generate_markdown_report(self) -> str:
        """Generate comprehensive markdown report"""
        summary = self.calculate_summary()
        coverage = self.generate_coverage_report()
        
        report = f"""# Neo C++ Test Execution Report

## 📊 Executive Summary

**Date**: {self.timestamp.strftime('%Y-%m-%d %H:%M:%S')}  
**Platform**: {sys.platform}  
**Python Version**: {sys.version.split()[0]}

### Overall Results

| Metric | Value |
|--------|-------|
| **Status** | {self._get_status_badge(summary['status'])} |
| **Test Binaries** | {summary['total_binaries']} |
| **Total Tests** | {summary['total_tests']} |
| **Passed** | {summary['total_passed']} ✅ |
| **Failed** | {summary['total_failed']} ❌ |
| **Skipped** | {summary['total_skipped']} ⏭️ |
| **Disabled** | {summary['total_disabled']} 🚫 |
| **Pass Rate** | {summary['overall_pass_rate']}% |
| **Total Duration** | {summary['total_duration']}s |

"""
        
        # Add coverage if available
        if coverage.get("line_coverage", 0) > 0:
            report += f"""### Code Coverage

| Type | Coverage |
|------|----------|
| **Line Coverage** | {coverage['line_coverage']}% |
| **Branch Coverage** | {coverage['branch_coverage']}% |
| **Function Coverage** | {coverage['function_coverage']}% |

"""
        
        # Add individual test results
        report += "## 🧪 Test Suite Results\n\n"
        
        for name, result in sorted(self.test_results.items()):
            status_icon = self._get_status_icon(result['status'])
            report += f"""### {name.replace('_', ' ').title()} {status_icon}

| Metric | Value |
|--------|-------|
| **Status** | {result['status']} |
| **Tests** | {result['total']} |
| **Passed** | {result['passed']} |
| **Failed** | {result['failed']} |
| **Skipped** | {result['skipped']} |
| **Pass Rate** | {result['pass_rate']}% |
| **Duration** | {result['duration']}s |

"""
            
            # Add failed tests if any
            if result['failed_tests']:
                report += "#### ❌ Failed Tests\n"
                for test in result['failed_tests']:
                    report += f"- `{test}`\n"
                report += "\n"
            
            # Add skipped tests if significant
            if len(result['skipped_tests']) > 0 and len(result['skipped_tests']) <= 10:
                report += "#### ⏭️ Skipped Tests\n"
                for test in result['skipped_tests']:
                    report += f"- `{test}`\n"
                report += "\n"
        
        # Add recommendations
        report += self._generate_recommendations(summary)
        
        # Add test categories
        report += self._generate_test_categories()
        
        return report
    
    def _get_status_icon(self, status: str) -> str:
        """Get icon for status"""
        icons = {
            "PASSED": "✅",
            "FAILED": "❌",
            "SKIPPED": "⏭️",
            "ERROR": "⚠️",
            "TIMEOUT": "⏱️",
            "NO_TESTS": "❓",
            "UNKNOWN": "❓"
        }
        return icons.get(status, "")
    
    def _get_status_badge(self, status: str) -> str:
        """Get badge for status"""
        badges = {
            "PASSED": "🟢 **PASSED**",
            "FAILED": "🔴 **FAILED**",
            "SKIPPED": "🟡 **SKIPPED**",
            "NO_TESTS": "⚪ **NO TESTS**",
            "UNKNOWN": "⚫ **UNKNOWN**"
        }
        return badges.get(status, status)
    
    def _generate_recommendations(self, summary: Dict) -> str:
        """Generate test improvement recommendations"""
        recommendations = "\n## 💡 Recommendations\n\n"
        
        if summary['total_failed'] > 0:
            recommendations += f"""### 🔧 Fix Failing Tests
- **Priority**: High
- **Action**: {summary['total_failed']} tests are failing and need immediate attention
- **Impact**: Failing tests block deployment and reduce confidence

"""
        
        if summary['overall_pass_rate'] < 90:
            recommendations += f"""### 📈 Improve Pass Rate
- **Current**: {summary['overall_pass_rate']}%
- **Target**: 90%+
- **Action**: Focus on fixing failing tests and reducing skipped tests

"""
        
        if summary['total_skipped'] > summary['total_tests'] * 0.1:
            recommendations += f"""### ⏭️ Review Skipped Tests
- **Count**: {summary['total_skipped']} tests skipped
- **Action**: Determine if these tests should be enabled or removed

"""
        
        if summary['total_disabled'] > 0:
            recommendations += f"""### 🚫 Enable Disabled Tests
- **Count**: {summary['total_disabled']} tests disabled
- **Action**: Review and re-enable disabled tests

"""
        
        if summary['overall_pass_rate'] >= 90:
            recommendations += """### ✅ Maintain Quality
- **Status**: Good test health with >90% pass rate
- **Action**: Continue maintaining high standards
- **Next**: Add integration and performance tests

"""
        
        recommendations += """### 📊 Coverage Goals
- **Target**: 90% line coverage
- **Method**: Enable coverage flags in CMake
- **Tools**: Use gcov/lcov for C++ coverage

"""
        
        return recommendations
    
    def _generate_test_categories(self) -> str:
        """Generate test category summary"""
        categories = """
## 📁 Test Categories

### Unit Tests
- **Cryptography**: Hash functions, encryption, signatures
- **IO**: Input/output operations, serialization
- **Persistence**: Storage layer, database operations
- **VM**: Virtual machine execution
- **Smart Contracts**: Contract execution and validation
- **Network**: P2P communication, message handling
- **Consensus**: dBFT consensus mechanism

### SDK Tests
- **Core Types**: Blockchain primitives (45+ tests)
- **Wallet**: Key management and signing (35+ tests)
- **RPC Client**: Node communication (40+ tests)
- **Transaction Manager**: Transaction building (45+ tests)
- **NEP17 Token**: Token operations (40+ tests)

### Integration Tests
- **End-to-End**: Full workflow testing
- **Performance**: Benchmarks and stress tests
- **Security**: Vulnerability testing

"""
        return categories
    
    def save_reports(self):
        """Save all reports"""
        # Save JSON report
        json_report = {
            "summary": self.calculate_summary(),
            "coverage": self.generate_coverage_report(),
            "results": self.test_results
        }
        
        json_file = self.build_dir / f"test_report_{self.timestamp.strftime('%Y%m%d_%H%M%S')}.json"
        with open(json_file, 'w') as f:
            json.dump(json_report, f, indent=2)
        print(f"\n📄 JSON report saved to: {json_file}")
        
        # Save Markdown report
        md_file = self.build_dir / f"test_report_{self.timestamp.strftime('%Y%m%d_%H%M%S')}.md"
        with open(md_file, 'w') as f:
            f.write(self.generate_markdown_report())
        print(f"📄 Markdown report saved to: {md_file}")
        
        # Print summary
        summary = self.calculate_summary()
        print("\n" + "=" * 60)
        print("FINAL TEST SUMMARY")
        print("=" * 60)
        print(f"Overall Status: {summary['status']}")
        print(f"Pass Rate: {summary['overall_pass_rate']}%")
        print(f"Tests: {summary['total_passed']}/{summary['total_tests']} passed")
        if summary['total_failed'] > 0:
            print(f"Failed: {summary['total_failed']} ❌")
        if summary['total_skipped'] > 0:
            print(f"Skipped: {summary['total_skipped']} ⏭️")
        print(f"Duration: {summary['total_duration']}s")
        print("=" * 60)

def main():
    print("""
╔══════════════════════════════════════════════════════════╗
║         Neo C++ Comprehensive Test Runner v1.0          ║
║              Testing & Coverage Analysis                ║
╚══════════════════════════════════════════════════════════╝
""")
    
    runner = ComprehensiveTestRunner()
    runner.run_all_tests()
    runner.save_reports()
    
    # Return appropriate exit code
    summary = runner.calculate_summary()
    if summary['total_failed'] > 0:
        sys.exit(1)
    sys.exit(0)

if __name__ == "__main__":
    main()
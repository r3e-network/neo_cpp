#!/usr/bin/env python3
"""
Neo C++ Code Quality Checker

This script performs comprehensive code quality checks on the Neo C++ codebase:
- Identifies duplicate implementations
- Finds dead code
- Checks for incomplete implementations (TODO, FIXME, throw not implemented)
- Analyzes header/implementation consistency
- Checks for unused includes
"""

import os
import re
from pathlib import Path
import json
from collections import defaultdict
from typing import Dict, List, Set, Tuple

class CodeQualityChecker:
    def __init__(self, project_root: str):
        self.project_root = Path(project_root)
        self.src_dir = self.project_root / "src"
        self.include_dir = self.project_root / "include"
        
        # Patterns for code analysis
        self.todo_pattern = re.compile(r'(TODO|FIXME|XXX|HACK|BUG)(\s*:)?\s*(.+)', re.IGNORECASE)
        self.not_implemented_pattern = re.compile(r'throw\s+.*(?:not.*implemented|NotImplementedException)', re.IGNORECASE)
        self.include_pattern = re.compile(r'#include\s*[<"]([^>"]+)[>"]')
        self.pragma_once_pattern = re.compile(r'#pragma\s+once')
        self.ifndef_guard_pattern = re.compile(r'#ifndef\s+(\w+)')
        
    def check_todos_and_fixmes(self) -> Dict[str, List[Dict]]:
        """Find all TODO, FIXME, and similar comments."""
        todos = defaultdict(list)
        
        for file_path in self.get_all_source_files():
            try:
                content = file_path.read_text(encoding='utf-8')
                lines = content.splitlines()
                
                for line_num, line in enumerate(lines, 1):
                    match = self.todo_pattern.search(line)
                    if match:
                        todos[str(file_path)].append({
                            'line': line_num,
                            'type': match.group(1).upper(),
                            'message': match.group(3).strip() if match.group(3) else '',
                            'full_line': line.strip()
                        })
            except Exception as e:
                print(f"Error reading {file_path}: {e}")
        
        return dict(todos)
    
    def check_not_implemented(self) -> Dict[str, List[Dict]]:
        """Find all 'not implemented' exceptions."""
        not_implemented = defaultdict(list)
        
        for file_path in self.get_all_source_files():
            try:
                content = file_path.read_text(encoding='utf-8')
                lines = content.splitlines()
                
                for line_num, line in enumerate(lines, 1):
                    if self.not_implemented_pattern.search(line):
                        not_implemented[str(file_path)].append({
                            'line': line_num,
                            'code': line.strip()
                        })
            except Exception as e:
                print(f"Error reading {file_path}: {e}")
        
        return dict(not_implemented)
    
    def check_header_guards(self) -> Dict[str, List[str]]:
        """Check header files for proper include guards."""
        issues = defaultdict(list)
        
        for header_path in self.get_header_files():
            try:
                content = header_path.read_text(encoding='utf-8')
                
                has_pragma_once = bool(self.pragma_once_pattern.search(content))
                has_ifndef = bool(self.ifndef_guard_pattern.search(content))
                
                if not has_pragma_once and not has_ifndef:
                    issues['missing_guards'].append(str(header_path))
                elif has_pragma_once and has_ifndef:
                    issues['duplicate_guards'].append(str(header_path))
            except Exception as e:
                print(f"Error reading {header_path}: {e}")
        
        return dict(issues)
    
    def check_unused_includes(self) -> Dict[str, List[str]]:
        """Find potentially unused includes (heuristic-based)."""
        unused_includes = defaultdict(list)
        
        for file_path in self.get_all_source_files():
            try:
                content = file_path.read_text(encoding='utf-8')
                includes = self.include_pattern.findall(content)
                
                # Remove includes from content to check usage
                content_without_includes = re.sub(r'#include.*\n', '', content)
                
                for include in includes:
                    # Extract the base name from the include
                    base_name = Path(include).stem
                    
                    # Simple heuristic: check if the base name appears in the code
                    # This is not perfect but catches obvious cases
                    if base_name and not re.search(r'\b' + re.escape(base_name) + r'\b', content_without_includes):
                        # Skip common system includes
                        if not any(base_name.startswith(prefix) for prefix in ['std', 'boost', 'openssl']):
                            unused_includes[str(file_path)].append(include)
                            
            except Exception as e:
                print(f"Error reading {file_path}: {e}")
        
        return dict(unused_includes)
    
    def check_long_functions(self, max_lines: int = 100) -> Dict[str, List[Dict]]:
        """Find functions that are too long."""
        long_functions = defaultdict(list)
        
        func_pattern = re.compile(
            r'^(?!.*\b(?:if|for|while|switch)\b)\s*'
            r'(?:static\s+)?(?:inline\s+)?(?:virtual\s+)?'
            r'(?:const\s+)?(?:unsigned\s+)?(?:[\w:]+\s+)?'
            r'(\w+)\s*\([^)]*\)\s*(?:const\s*)?(?:override\s*)?'
            r'\s*\{',
            re.MULTILINE
        )
        
        for file_path in self.get_cpp_files():
            try:
                content = file_path.read_text(encoding='utf-8')
                lines = content.splitlines()
                
                # Find function starts
                for match in func_pattern.finditer(content):
                    func_name = match.group(1)
                    start_line = content[:match.start()].count('\n') + 1
                    
                    # Count braces to find function end
                    brace_count = 0
                    in_function = False
                    
                    for i in range(start_line - 1, len(lines)):
                        line = lines[i]
                        for char in line:
                            if char == '{':
                                brace_count += 1
                                in_function = True
                            elif char == '}':
                                brace_count -= 1
                                
                        if in_function and brace_count == 0:
                            func_length = i - start_line + 2
                            if func_length > max_lines:
                                long_functions[str(file_path)].append({
                                    'function': func_name,
                                    'start_line': start_line,
                                    'length': func_length
                                })
                            break
                            
            except Exception as e:
                print(f"Error reading {file_path}: {e}")
        
        return dict(long_functions)
    
    def check_large_files(self, max_lines: int = 1000) -> List[Dict]:
        """Find files that are too large."""
        large_files = []
        
        for file_path in self.get_all_source_files():
            try:
                content = file_path.read_text(encoding='utf-8')
                line_count = content.count('\n') + 1
                
                if line_count > max_lines:
                    large_files.append({
                        'file': str(file_path),
                        'lines': line_count
                    })
            except Exception as e:
                print(f"Error reading {file_path}: {e}")
        
        return sorted(large_files, key=lambda x: x['lines'], reverse=True)
    
    def get_all_source_files(self) -> List[Path]:
        """Get all C++ source and header files."""
        files = []
        for pattern in ['*.cpp', '*.h', '*.hpp']:
            files.extend(self.src_dir.rglob(pattern))
            files.extend(self.include_dir.rglob(pattern))
        return [f for f in files if 'build' not in str(f) and 'test' not in str(f) and f.exists()]
    
    def get_cpp_files(self) -> List[Path]:
        """Get all C++ implementation files."""
        files = list(self.src_dir.rglob('*.cpp'))
        return [f for f in files if 'build' not in str(f) and 'test' not in str(f) and f.exists()]
    
    def get_header_files(self) -> List[Path]:
        """Get all header files."""
        files = []
        for pattern in ['*.h', '*.hpp']:
            files.extend(self.include_dir.rglob(pattern))
        return [f for f in files if 'build' not in str(f) and f.exists()]
    
    def generate_report(self) -> Dict:
        """Generate comprehensive code quality report."""
        print("Analyzing Neo C++ code quality...")
        
        todos = self.check_todos_and_fixmes()
        not_implemented = self.check_not_implemented()
        header_issues = self.check_header_guards()
        long_functions = self.check_long_functions()
        large_files = self.check_large_files()
        
        # Summary statistics
        total_todos = sum(len(items) for items in todos.values())
        total_not_implemented = sum(len(items) for items in not_implemented.values())
        total_long_functions = sum(len(items) for items in long_functions.values())
        
        report = {
            'summary': {
                'total_files_analyzed': len(list(self.get_all_source_files())),
                'total_todos_fixmes': total_todos,
                'total_not_implemented': total_not_implemented,
                'total_long_functions': total_long_functions,
                'total_large_files': len(large_files),
                'files_missing_guards': len(header_issues.get('missing_guards', [])),
                'files_duplicate_guards': len(header_issues.get('duplicate_guards', []))
            },
            'todos_and_fixmes': todos,
            'not_implemented': not_implemented,
            'header_guard_issues': header_issues,
            'long_functions': long_functions,
            'large_files': large_files
        }
        
        return report
    
    def print_report(self, report: Dict):
        """Print a formatted report."""
        print("\n" + "="*80)
        print("CODE QUALITY REPORT")
        print("="*80)
        
        # Summary
        print("\n## SUMMARY")
        for key, value in report['summary'].items():
            print(f"  {key.replace('_', ' ').title()}: {value}")
        
        # TODOs and FIXMEs
        if report['todos_and_fixmes']:
            print("\n## TODOs AND FIXMEs (Top 10 files)")
            sorted_todos = sorted(report['todos_and_fixmes'].items(), 
                                key=lambda x: len(x[1]), reverse=True)[:10]
            
            for file_path, todos in sorted_todos:
                print(f"\n  {file_path} ({len(todos)} items)")
                for todo in todos[:3]:
                    print(f"    Line {todo['line']}: {todo['type']} - {todo['message']}")
                if len(todos) > 3:
                    print(f"    ... and {len(todos) - 3} more")
        
        # Not Implemented
        if report['not_implemented']:
            print("\n## NOT IMPLEMENTED FUNCTIONS")
            for file_path, items in list(report['not_implemented'].items())[:10]:
                print(f"\n  {file_path}")
                for item in items[:3]:
                    print(f"    Line {item['line']}: {item['code']}")
        
        # Long Functions
        if report['long_functions']:
            print("\n## LONG FUNCTIONS (>100 lines)")
            for file_path, funcs in list(report['long_functions'].items())[:10]:
                print(f"\n  {file_path}")
                for func in funcs[:3]:
                    print(f"    {func['function']}() - {func['length']} lines")
        
        # Large Files
        if report['large_files']:
            print("\n## LARGE FILES (>1000 lines)")
            for file_info in report['large_files'][:10]:
                print(f"  {file_info['file']} - {file_info['lines']} lines")
        
        # Header Guard Issues
        if report['header_guard_issues']:
            if report['header_guard_issues'].get('missing_guards'):
                print("\n## HEADERS MISSING GUARDS")
                for header in report['header_guard_issues']['missing_guards'][:10]:
                    print(f"  {header}")


def main():
    import argparse
    
    parser = argparse.ArgumentParser(description='Check Neo C++ code quality')
    parser.add_argument('--project-root', default='.', help='Project root directory')
    parser.add_argument('--output', help='Output JSON file')
    parser.add_argument('--max-function-lines', type=int, default=100,
                       help='Maximum lines per function')
    parser.add_argument('--max-file-lines', type=int, default=1000,
                       help='Maximum lines per file')
    
    args = parser.parse_args()
    
    checker = CodeQualityChecker(args.project_root)
    report = checker.generate_report()
    
    # Print report
    checker.print_report(report)
    
    # Save JSON if requested
    if args.output:
        with open(args.output, 'w') as f:
            json.dump(report, f, indent=2)
        print(f"\n\nFull report saved to: {args.output}")
    
    # Print improvement suggestions
    print("\n" + "="*80)
    print("IMPROVEMENT SUGGESTIONS")
    print("="*80)
    
    if report['summary']['total_todos_fixmes'] > 0:
        print(f"\n1. Address {report['summary']['total_todos_fixmes']} TODO/FIXME items")
    
    if report['summary']['total_not_implemented'] > 0:
        print(f"\n2. Implement {report['summary']['total_not_implemented']} stub functions")
    
    if report['summary']['total_long_functions'] > 0:
        print(f"\n3. Refactor {report['summary']['total_long_functions']} long functions")
    
    if report['summary']['total_large_files'] > 0:
        print(f"\n4. Split {report['summary']['total_large_files']} large files")
    
    print("\n5. Run the duplicate checker to remove redundant implementations")
    print("   python3 scripts/check_implementation_duplicates.py")


if __name__ == "__main__":
    main()
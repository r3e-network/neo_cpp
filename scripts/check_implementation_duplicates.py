#!/usr/bin/env python3
"""
Neo C++ Implementation Duplicate Checker

This script specifically checks for duplicate implementations in the Neo C++ codebase,
focusing on files that have multiple versions (complete, minimal, fixed, etc.)
"""

import os
import re
from pathlib import Path
import json
from collections import defaultdict
from typing import Dict, List, Set, Tuple

class ImplementationDuplicateChecker:
    def __init__(self, project_root: str):
        self.project_root = Path(project_root)
        self.src_dir = self.project_root / "src"
        
        # Known duplicate patterns
        self.duplicate_suffixes = [
            '_complete', '_minimal', '_fixed', '_broken', '_simple', 
            '_full', '_impl', '_old', '_new', '_stubs', '_partial',
            '_standalone', '_testnet'
        ]
        
        # Files to check in CMakeLists.txt
        self.cmake_files = []
        
    def find_implementation_files(self) -> Dict[str, List[Path]]:
        """Find all C++ implementation files grouped by base name."""
        impl_groups = defaultdict(list)
        
        # Find all .cpp files in src directory
        for cpp_file in self.src_dir.rglob('*.cpp'):
            if 'build' in str(cpp_file) or 'test' in str(cpp_file):
                continue
                
            file_stem = cpp_file.stem
            base_name = file_stem
            
            # Check if file has a duplicate suffix
            for suffix in self.duplicate_suffixes:
                if file_stem.endswith(suffix):
                    base_name = file_stem[:-len(suffix)]
                    break
            
            impl_groups[base_name].append(cpp_file)
        
        # Filter to only groups with multiple implementations
        return {k: v for k, v in impl_groups.items() if len(v) > 1}
    
    def check_cmake_usage(self, file_path: Path) -> List[Path]:
        """Check which CMakeLists.txt files reference this file."""
        cmake_files = []
        filename = file_path.name
        
        for cmake_file in self.project_root.rglob('CMakeLists.txt'):
            try:
                content = cmake_file.read_text()
                if filename in content:
                    cmake_files.append(cmake_file)
            except Exception as e:
                print(f"Error reading {cmake_file}: {e}")
        
        return cmake_files
    
    def analyze_file_contents(self, file_path: Path) -> Dict[str, any]:
        """Analyze file contents to extract key information."""
        try:
            content = file_path.read_text()
            
            # Count lines (excluding blank lines and comments)
            lines = content.splitlines()
            code_lines = 0
            for line in lines:
                stripped = line.strip()
                if stripped and not stripped.startswith('//') and not stripped.startswith('/*'):
                    code_lines += 1
            
            # Extract functions
            func_pattern = re.compile(r'^\s*(?:\w+\s+)*(\w+)\s*\([^)]*\)\s*(?:const\s*)?(?:override\s*)?(?:\{|;)', re.MULTILINE)
            functions = [m.group(1) for m in func_pattern.finditer(content) if m.group(1) not in ['if', 'for', 'while', 'switch', 'catch']]
            
            # Extract classes
            class_pattern = re.compile(r'^\s*(?:class|struct)\s+(\w+)', re.MULTILINE)
            classes = class_pattern.findall(content)
            
            # Check for main function
            has_main = 'int main(' in content
            
            # Check for stub/placeholder patterns
            has_todo = 'TODO' in content or 'FIXME' in content
            has_not_implemented = 'throw.*not.*implemented' in content.lower()
            
            return {
                'file_size': len(content),
                'code_lines': code_lines,
                'functions': functions,
                'classes': classes,
                'has_main': has_main,
                'has_todo': has_todo,
                'has_not_implemented': has_not_implemented,
                'last_modified': os.path.getmtime(file_path)
            }
        except Exception as e:
            return {'error': str(e)}
    
    def generate_report(self) -> Dict:
        """Generate a report of duplicate implementations."""
        print("Checking for duplicate implementations in Neo C++...")
        
        impl_groups = self.find_implementation_files()
        report = {
            'summary': {
                'total_duplicate_groups': len(impl_groups),
                'total_duplicate_files': sum(len(files) for files in impl_groups.values())
            },
            'duplicates': []
        }
        
        for base_name, files in sorted(impl_groups.items()):
            group_info = {
                'base_name': base_name,
                'files': []
            }
            
            for file_path in sorted(files):
                file_info = {
                    'path': str(file_path.relative_to(self.project_root)),
                    'name': file_path.name,
                    'cmake_usage': [str(f.relative_to(self.project_root)) for f in self.check_cmake_usage(file_path)]
                }
                
                # Add content analysis
                analysis = self.analyze_file_contents(file_path)
                file_info.update(analysis)
                
                group_info['files'].append(file_info)
            
            # Determine which file is likely the main implementation
            main_file = None
            for f in group_info['files']:
                if not any(suffix in f['name'] for suffix in self.duplicate_suffixes):
                    main_file = f['name']
                    break
            
            if not main_file:
                # Look for the one used in CMake
                for f in group_info['files']:
                    if f['cmake_usage']:
                        main_file = f['name']
                        break
            
            group_info['likely_main_file'] = main_file
            
            # Add recommendations
            recommendations = []
            for f in group_info['files']:
                if not f['cmake_usage'] and f['name'] != main_file:
                    recommendations.append(f"Remove unused file: {f['path']}")
                elif f.get('has_not_implemented'):
                    recommendations.append(f"Complete implementation in: {f['path']}")
            
            group_info['recommendations'] = recommendations
            report['duplicates'].append(group_info)
        
        return report
    
    def print_report(self, report: Dict):
        """Print a formatted report."""
        print("\n" + "="*80)
        print("DUPLICATE IMPLEMENTATION REPORT")
        print("="*80)
        
        print(f"\nTotal duplicate groups: {report['summary']['total_duplicate_groups']}")
        print(f"Total duplicate files: {report['summary']['total_duplicate_files']}")
        
        for group in report['duplicates']:
            print(f"\n{'='*60}")
            print(f"Base: {group['base_name']}")
            print(f"Main file: {group['likely_main_file'] or 'Unknown'}")
            
            for file_info in group['files']:
                status = "✓ USED" if file_info['cmake_usage'] else "✗ UNUSED"
                print(f"\n  [{status}] {file_info['name']}")
                print(f"    Path: {file_info['path']}")
                if 'code_lines' in file_info:
                    print(f"    Code lines: {file_info['code_lines']}")
                    print(f"    Functions: {len(file_info.get('functions', []))}")
                    print(f"    Classes: {len(file_info.get('classes', []))}")
                    if file_info.get('has_main'):
                        print(f"    Has main: Yes")
                    if file_info.get('has_not_implemented'):
                        print(f"    Has unimplemented functions: Yes")
                if file_info['cmake_usage']:
                    print(f"    Used in: {', '.join(file_info['cmake_usage'])}")
            
            if group['recommendations']:
                print(f"\n  Recommendations:")
                for rec in group['recommendations']:
                    print(f"    - {rec}")


def main():
    import argparse
    
    parser = argparse.ArgumentParser(description='Check for duplicate implementations in Neo C++')
    parser.add_argument('--project-root', default='.', help='Project root directory')
    parser.add_argument('--output', help='Output JSON file')
    parser.add_argument('--verbose', action='store_true', help='Verbose output')
    
    args = parser.parse_args()
    
    checker = ImplementationDuplicateChecker(args.project_root)
    report = checker.generate_report()
    
    # Print report
    checker.print_report(report)
    
    # Save JSON if requested
    if args.output:
        with open(args.output, 'w') as f:
            json.dump(report, f, indent=2)
        print(f"\nFull report saved to: {args.output}")
    
    # Print summary of actions needed
    print("\n" + "="*80)
    print("SUMMARY OF ACTIONS NEEDED:")
    print("="*80)
    
    total_unused = 0
    total_incomplete = 0
    
    for group in report['duplicates']:
        for f in group['files']:
            if not f['cmake_usage'] and f['name'] != group['likely_main_file']:
                total_unused += 1
            if f.get('has_not_implemented'):
                total_incomplete += 1
    
    print(f"\nUnused duplicate files to remove: {total_unused}")
    print(f"Files with incomplete implementations: {total_incomplete}")
    
    # Create cleanup script
    if total_unused > 0:
        cleanup_script = "#!/bin/bash\n"
        cleanup_script += "# Neo C++ Duplicate File Cleanup Script\n"
        cleanup_script += "# Generated by check_implementation_duplicates.py\n\n"
        
        for group in report['duplicates']:
            for f in group['files']:
                if not f['cmake_usage'] and f['name'] != group['likely_main_file']:
                    cleanup_script += f"# rm {f['path']}\n"
        
        with open('cleanup_duplicates.sh', 'w') as f:
            f.write(cleanup_script)
        
        print(f"\nCleanup script created: cleanup_duplicates.sh")
        print("Review and uncomment lines to remove files")


if __name__ == "__main__":
    main()
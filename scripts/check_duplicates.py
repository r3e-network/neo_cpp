#!/usr/bin/env python3
"""
Neo C++ Duplicate Implementation Checker

This script analyzes the Neo C++ codebase to identify:
1. Files with similar names that might be duplicates
2. Functions/methods that appear in multiple files
3. Classes defined in multiple locations
4. Similar code blocks across files
"""

import os
import re
import hashlib
import argparse
from collections import defaultdict
from pathlib import Path
import json
from typing import Dict, List, Set, Tuple

class DuplicateChecker:
    def __init__(self, project_root: str):
        self.project_root = Path(project_root)
        self.src_dir = self.project_root / "src"
        self.include_dir = self.project_root / "include"
        
        # Patterns to identify code elements
        self.function_pattern = re.compile(
            r'^\s*(?:static\s+)?(?:inline\s+)?(?:virtual\s+)?'
            r'(?:const\s+)?(?:unsigned\s+)?(?:[\w:]+\s+)?'
            r'(\w+)\s*\([^)]*\)\s*(?:const\s*)?(?:override\s*)?(?:noexcept\s*)?'
            r'\s*(?:->.*?)?\s*\{',
            re.MULTILINE
        )
        
        self.class_pattern = re.compile(
            r'^\s*(?:template\s*<[^>]+>\s*)?'
            r'(?:class|struct)\s+(\w+)'
            r'(?:\s*:\s*(?:public|private|protected)\s+[\w:]+)?'
            r'\s*\{',
            re.MULTILINE
        )
        
        self.namespace_pattern = re.compile(
            r'^\s*namespace\s+([\w:]+)\s*\{',
            re.MULTILINE
        )
        
    def find_all_cpp_files(self) -> List[Path]:
        """Find all C++ source and header files."""
        cpp_files = []
        for pattern in ['*.cpp', '*.h', '*.hpp']:
            cpp_files.extend(self.src_dir.rglob(pattern))
            cpp_files.extend(self.include_dir.rglob(pattern))
        return [f for f in cpp_files if 'build' not in str(f) and 'test' not in str(f)]
    
    def find_similar_filenames(self, files: List[Path]) -> Dict[str, List[Path]]:
        """Group files by their base name (without suffixes like _complete, _minimal, etc.)."""
        filename_groups = defaultdict(list)
        
        # Patterns for common suffixes that indicate duplicates
        suffix_pattern = re.compile(r'(_complete|_minimal|_fixed|_broken|_simple|_full|_impl|_test|_old|_new|_v\d+)')
        
        for file in files:
            # Remove common suffixes to find base name
            base_name = suffix_pattern.sub('', file.stem)
            filename_groups[base_name].append(file)
        
        # Filter out groups with only one file
        return {k: v for k, v in filename_groups.items() if len(v) > 1}
    
    def extract_functions(self, content: str) -> Set[str]:
        """Extract function signatures from file content."""
        functions = set()
        
        # Remove comments to avoid false positives
        content = self.remove_comments(content)
        
        matches = self.function_pattern.findall(content)
        for match in matches:
            if match and not match.startswith('~'):  # Skip destructors
                functions.add(match)
        
        return functions
    
    def extract_classes(self, content: str) -> Set[str]:
        """Extract class/struct definitions from file content."""
        content = self.remove_comments(content)
        return set(self.class_pattern.findall(content))
    
    def remove_comments(self, content: str) -> str:
        """Remove C++ comments from content."""
        # Remove single-line comments
        content = re.sub(r'//.*$', '', content, flags=re.MULTILINE)
        # Remove multi-line comments
        content = re.sub(r'/\*.*?\*/', '', content, flags=re.DOTALL)
        return content
    
    def calculate_similarity(self, content1: str, content2: str) -> float:
        """Calculate similarity between two pieces of code."""
        # Remove whitespace and comments for comparison
        clean1 = re.sub(r'\s+', '', self.remove_comments(content1))
        clean2 = re.sub(r'\s+', '', self.remove_comments(content2))
        
        if not clean1 or not clean2:
            return 0.0
        
        # Use set intersection for a simple similarity metric
        set1 = set(clean1.split())
        set2 = set(clean2.split())
        
        intersection = len(set1.intersection(set2))
        union = len(set1.union(set2))
        
        return intersection / union if union > 0 else 0.0
    
    def find_duplicate_implementations(self, files: List[Path]) -> Dict[str, List[Dict]]:
        """Find duplicate function and class implementations across files."""
        function_locations = defaultdict(list)
        class_locations = defaultdict(list)
        
        for file in files:
            try:
                content = file.read_text(encoding='utf-8')
                
                # Extract functions
                functions = self.extract_functions(content)
                for func in functions:
                    function_locations[func].append(str(file))
                
                # Extract classes
                classes = self.extract_classes(content)
                for cls in classes:
                    class_locations[cls].append(str(file))
                    
            except Exception as e:
                print(f"Error reading {file}: {e}")
        
        # Find duplicates
        duplicate_functions = {k: v for k, v in function_locations.items() if len(v) > 1}
        duplicate_classes = {k: v for k, v in class_locations.items() if len(v) > 1}
        
        return {
            'functions': duplicate_functions,
            'classes': duplicate_classes
        }
    
    def analyze_file_content_similarity(self, file_groups: Dict[str, List[Path]]) -> List[Dict]:
        """Analyze content similarity between files in the same group."""
        similarity_report = []
        
        for base_name, files in file_groups.items():
            if len(files) < 2:
                continue
                
            group_report = {
                'base_name': base_name,
                'files': [str(f) for f in files],
                'similarities': []
            }
            
            # Compare each pair of files
            for i in range(len(files)):
                for j in range(i + 1, len(files)):
                    try:
                        content1 = files[i].read_text(encoding='utf-8')
                        content2 = files[j].read_text(encoding='utf-8')
                        
                        similarity = self.calculate_similarity(content1, content2)
                        
                        if similarity > 0.5:  # Threshold for reporting
                            group_report['similarities'].append({
                                'file1': str(files[i]),
                                'file2': str(files[j]),
                                'similarity': round(similarity, 3)
                            })
                    except Exception as e:
                        print(f"Error comparing {files[i]} and {files[j]}: {e}")
            
            if group_report['similarities']:
                similarity_report.append(group_report)
        
        return similarity_report
    
    def check_cmake_usage(self) -> Dict[str, List[str]]:
        """Check which duplicate files are actually used in CMakeLists.txt."""
        cmake_files = list(self.project_root.rglob('CMakeLists.txt'))
        file_usage = defaultdict(list)
        
        for cmake_file in cmake_files:
            try:
                content = cmake_file.read_text(encoding='utf-8')
                
                # Find all .cpp file references
                cpp_files = re.findall(r'(\w+\.cpp)', content)
                
                for cpp_file in cpp_files:
                    file_usage[cpp_file].append(str(cmake_file.relative_to(self.project_root)))
                    
            except Exception as e:
                print(f"Error reading {cmake_file}: {e}")
        
        return dict(file_usage)
    
    def generate_report(self) -> Dict:
        """Generate a comprehensive duplicate analysis report."""
        print("Analyzing Neo C++ codebase for duplicates...")
        
        # Find all C++ files
        all_files = self.find_all_cpp_files()
        print(f"Found {len(all_files)} C++ files")
        
        # Find files with similar names
        similar_files = self.find_similar_filenames(all_files)
        print(f"Found {len(similar_files)} groups of similarly named files")
        
        # Find duplicate implementations
        duplicates = self.find_duplicate_implementations(all_files)
        print(f"Found {len(duplicates['functions'])} duplicate functions")
        print(f"Found {len(duplicates['classes'])} duplicate classes")
        
        # Analyze content similarity
        similarity_analysis = self.analyze_file_content_similarity(similar_files)
        
        # Check CMake usage
        cmake_usage = self.check_cmake_usage()
        
        # Generate report
        report = {
            'summary': {
                'total_files': len(all_files),
                'similar_filename_groups': len(similar_files),
                'duplicate_functions': len(duplicates['functions']),
                'duplicate_classes': len(duplicates['classes']),
                'files_with_high_similarity': sum(1 for g in similarity_analysis if g['similarities'])
            },
            'similar_filenames': {k: [str(f) for f in v] for k, v in similar_files.items()},
            'duplicate_functions': duplicates['functions'],
            'duplicate_classes': duplicates['classes'],
            'content_similarity': similarity_analysis,
            'cmake_usage': cmake_usage,
            'recommendations': self.generate_recommendations(similar_files, duplicates, cmake_usage)
        }
        
        return report
    
    def generate_recommendations(self, similar_files: Dict, duplicates: Dict, cmake_usage: Dict) -> List[str]:
        """Generate recommendations for cleaning up duplicates."""
        recommendations = []
        
        # Check for unused duplicate files
        for base_name, files in similar_files.items():
            for file in files:
                filename = file.name
                if filename not in cmake_usage:
                    recommendations.append(f"Consider removing unused file: {file}")
        
        # Check for multiple implementations of the same function
        for func_name, locations in duplicates['functions'].items():
            if len(locations) > 2:
                recommendations.append(
                    f"Function '{func_name}' is implemented in {len(locations)} files. "
                    f"Consider consolidating to a single implementation."
                )
        
        # Check for multiple class definitions
        for class_name, locations in duplicates['classes'].items():
            if len(locations) > 1 and not class_name.startswith('_'):
                recommendations.append(
                    f"Class '{class_name}' is defined in {len(locations)} files. "
                    f"This may cause linking issues."
                )
        
        return recommendations


def main():
    parser = argparse.ArgumentParser(description='Check for duplicate implementations in Neo C++ project')
    parser.add_argument('--project-root', default='.', help='Path to project root directory')
    parser.add_argument('--output', default='duplicate_analysis.json', help='Output file for report')
    parser.add_argument('--format', choices=['json', 'text'], default='text', help='Output format')
    
    args = parser.parse_args()
    
    checker = DuplicateChecker(args.project_root)
    report = checker.generate_report()
    
    if args.format == 'json':
        with open(args.output, 'w') as f:
            json.dump(report, f, indent=2)
        print(f"\nReport saved to {args.output}")
    else:
        # Print text report
        print("\n" + "="*80)
        print("NEO C++ DUPLICATE ANALYSIS REPORT")
        print("="*80)
        
        print("\n## SUMMARY")
        for key, value in report['summary'].items():
            print(f"  {key.replace('_', ' ').title()}: {value}")
        
        print("\n## SIMILAR FILENAMES")
        for base_name, files in list(report['similar_filenames'].items())[:10]:
            print(f"\n  Base name: {base_name}")
            for file in files:
                print(f"    - {file}")
        
        if len(report['similar_filenames']) > 10:
            print(f"\n  ... and {len(report['similar_filenames']) - 10} more groups")
        
        print("\n## DUPLICATE FUNCTIONS (Top 10)")
        for func_name, locations in list(report['duplicate_functions'].items())[:10]:
            print(f"\n  Function: {func_name}")
            for loc in locations[:3]:
                print(f"    - {loc}")
            if len(locations) > 3:
                print(f"    ... and {len(locations) - 3} more locations")
        
        print("\n## DUPLICATE CLASSES")
        for class_name, locations in list(report['duplicate_classes'].items())[:10]:
            print(f"\n  Class: {class_name}")
            for loc in locations:
                print(f"    - {loc}")
        
        print("\n## HIGH SIMILARITY FILES")
        for group in report['content_similarity'][:5]:
            print(f"\n  Group: {group['base_name']}")
            for sim in group['similarities']:
                print(f"    {sim['file1']} <-> {sim['file2']}: {sim['similarity']*100:.1f}% similar")
        
        print("\n## RECOMMENDATIONS")
        for i, rec in enumerate(report['recommendations'][:10], 1):
            print(f"  {i}. {rec}")
        
        if len(report['recommendations']) > 10:
            print(f"\n  ... and {len(report['recommendations']) - 10} more recommendations")
        
        print("\n" + "="*80)
        
        # Save full report as JSON anyway
        with open(args.output, 'w') as f:
            json.dump(report, f, indent=2)
        print(f"\nFull report saved to {args.output}")


if __name__ == "__main__":
    main()
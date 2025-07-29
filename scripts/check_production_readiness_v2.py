#!/usr/bin/env python3
"""
Production readiness checker for Neo C++ implementation.
Checks for common issues that indicate incomplete or non-production code.
"""

import os
import re
import sys
from pathlib import Path
from typing import List, Tuple, Set
import fnmatch

class ProductionReadinessChecker:
    def __init__(self, root_dir: Path):
        self.root_dir = root_dir
        self.issues = []
        self.ignore_patterns = self._load_ignore_patterns()
        
    def _load_ignore_patterns(self) -> Set[str]:
        """Load patterns from .production-check-ignore file."""
        ignore_file = self.root_dir / '.production-check-ignore'
        patterns = set()
        
        if ignore_file.exists():
            with open(ignore_file, 'r') as f:
                for line in f:
                    line = line.strip()
                    if line and not line.startswith('#'):
                        patterns.add(line)
        
        return patterns
    
    def _should_ignore(self, file_path: Path) -> bool:
        """Check if a file should be ignored based on patterns."""
        relative_path = file_path.relative_to(self.root_dir)
        str_path = str(relative_path)
        
        for pattern in self.ignore_patterns:
            if fnmatch.fnmatch(str_path, pattern):
                return True
            # Also check if any parent directory matches
            for parent in relative_path.parents:
                if fnmatch.fnmatch(str(parent) + '/', pattern):
                    return True
        
        return False
    
    def check_file(self, file_path: Path):
        """Check a single file for production readiness issues."""
        if self._should_ignore(file_path):
            return
            
        try:
            with open(file_path, 'r', encoding='utf-8', errors='ignore') as f:
                content = f.read()
                lines = content.split('\n')
                
            relative_path = file_path.relative_to(self.root_dir)
            
            # Check for various patterns line by line
            for i, line in enumerate(lines, 1):
                # Skip empty lines
                if not line.strip():
                    continue
                    
                # TODO comments (but not TODO: or @todo in documentation)
                if re.search(r'\bTODO\b(?!:)', line) and not re.search(r'[@\\]\s*todo\b', line, re.I):
                    self.issues.append((str(relative_path), i, "TODO comment found"))
                
                # Simplified/placeholder implementations
                if re.search(r'(simplified|placeholder|stub|mock)\s+(implementation|version)', line, re.I):
                    self.issues.append((str(relative_path), i, "Simplified implementation found"))
                
                # Not implemented
                if re.search(r'not\s+implemented|NotImplemented', line, re.I):
                    self.issues.append((str(relative_path), i, "Not implemented found"))
                
                # Ellipsis in code (not in comments or strings)
                if '...' in line:
                    # Check if it's in a comment
                    comment_pos = line.find('//')
                    ellipsis_pos = line.find('...')
                    if comment_pos == -1 or ellipsis_pos < comment_pos:
                        # Check if it's in a string
                        if not (re.search(r'"[^"]*\.\.\.[^"]*"', line) or re.search(r"'[^']*\.\.\.[^']*'", line)):
                            # Check if it's a valid C++ construct (variadic template)
                            if not re.search(r'(template|typename|class)\s*<.*\.\.\..*>', line) and \
                               not re.search(r'(Args|args|T|Types?)\s*&&?\s*\.\.\.\s*\w+', line) and \
                               not re.search(r'catch\s*\(\s*\.\.\.\s*\)', line):
                                self.issues.append((str(relative_path), i, "Ellipsis (...) found - possible incomplete code"))
                
                # Merge conflict markers
                if line.strip() in ['<<<<<<<', '=======', '>>>>>>>'] or \
                   re.match(r'^(<<<<<<<|=======|>>>>>>>)\s+\w+', line):
                    self.issues.append((str(relative_path), i, "Merge conflict markers found"))
                
                # XXX markers
                if re.search(r'\bXXX\b', line):
                    self.issues.append((str(relative_path), i, "XXX marker found"))
                
                # HACK comments
                if re.search(r'\bHACK\b', line, re.I):
                    self.issues.append((str(relative_path), i, "HACK comment found"))
                
                # Temporary code
                if re.search(r'\b(temp|temporary|TEMP|TEMPORARY)\b', line) and \
                   not re.search(r'(template|temperature)', line, re.I):
                    self.issues.append((str(relative_path), i, "Temporary code found"))
                
                # "For now" comments
                if re.search(r'for\s+now', line, re.I):
                    self.issues.append((str(relative_path), i, '"for now" comment found'))
                
        except Exception as e:
            print(f"Error reading {file_path}: {e}")
    
    def check_all(self):
        """Check all source files in the project."""
        extensions = {'.cpp', '.h', '.hpp', '.cc', '.cxx', '.c'}
        
        for root, dirs, files in os.walk(self.root_dir):
            # Skip hidden directories
            dirs[:] = [d for d in dirs if not d.startswith('.')]
            
            root_path = Path(root)
            for file in files:
                if any(file.endswith(ext) for ext in extensions):
                    self.check_file(root_path / file)
    
    def report(self):
        """Generate a report of all issues found."""
        if not self.issues:
            print("✅ No production readiness issues found!")
            return 0
        
        print(f"❌ Found {len(self.issues)} production readiness issues:\n")
        
        # Group by file
        by_file = {}
        for file, line, issue in self.issues:
            if file not in by_file:
                by_file[file] = []
            by_file[file].append((line, issue))
        
        # Report by file
        for file in sorted(by_file.keys()):
            print(f"\n{file}:")
            for line, issue in sorted(by_file[file]):
                print(f"  Line {line}: {issue}")
        
        return 1

def main():
    # Get the root directory
    if len(sys.argv) > 1:
        root_dir = Path(sys.argv[1])
    else:
        # Assume we're running from scripts directory
        root_dir = Path(__file__).parent.parent
    
    if not root_dir.exists():
        print(f"Error: Directory {root_dir} does not exist")
        return 1
    
    print(f"Checking production readiness in: {root_dir}")
    print(f"Using ignore file: {root_dir / '.production-check-ignore'}")
    
    checker = ProductionReadinessChecker(root_dir)
    checker.check_all()
    return checker.report()

if __name__ == "__main__":
    sys.exit(main())
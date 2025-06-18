#!/usr/bin/env python3
"""
Neo C++ Project Cleanup Script

This script removes all temporary files, intermediate documents, and 
test artifacts to prepare the project for GitHub repository.
"""

import os
import glob
import shutil
import sys
from pathlib import Path

def cleanup_project():
    """Clean up the Neo C++ project directory."""
    
    # Get the project root directory
    project_root = Path(__file__).parent
    os.chdir(project_root)
    
    print("🧹 Cleaning up Neo C++ project...")
    print(f"📁 Project root: {project_root}")
    
    # Files to remove (exact matches)
    files_to_remove = [
        # Intermediate documentation files
        "COMPREHENSIVE_MODULE_ANALYSIS.md",
        "CONVERSION_COMPLETION_PLAN.md", 
        "CONVERSION_REVIEW_SUMMARY.md",
        "DEVELOPMENT_GUIDE.md",
        "FINAL_COMPLETION_CORRECTIONS_REPORT.md",
        "FINAL_NEO_CPP_CONVERSION_REPORT.md",
        "FINAL_NEO_CPP_STATUS.md",
        "FINAL_VALIDATION_SCRIPT.cpp",
        "IMPLEMENTATION_STATUS.md",
        "NEO3_COMPATIBILITY_STATUS.md",
        "NEO_CPP_CLI_COMPLETE.md",
        "NEO_CPP_CLI_INTEGRATED_TEST_RESULTS.md",
        "NEO_CPP_COMPLETION_PLAN.md",
        "NEO_CPP_COMPLETION_STATUS_CURRENT.md",
        "NEO_CPP_REVIEW_SUMMARY.md",
        "PHASE1_PROGRESS_REPORT.md",
        "PHASE_2_COMPLETION_REPORT.md",
        "PHASE_3_1_COMPLETION_REPORT.md",
        "PHASE_3_2_COMPLETION_REPORT.md",
        "PHASE_4_1_BUILD_ANALYSIS.md",
        "PRODUCTION_READY_SUMMARY.md",
        "PROGRESS_SUMMARY.md",
        "QUICK_START_NEXT_SESSION.md",
        "TASK_4_INTEGRATION_TESTS_COMPLETE.md",
        "TASK_5_BUILD_DEPLOYMENT_SCRIPTS_COMPLETE.md",
        
        # Temporary executable files
        "neo-demo.exe",
        "neo-minimal.exe", 
        "neo-network-demo.exe",
        
        # Test and demo files
        "minimal_neo_node.cpp",
        "neo_network_demo.cpp",
        "simple_neo_cli_test.cpp",
        "simple_neo_node.cpp",
        "test_build.cpp",
        "test_cmake.txt",
        "test_neo_node.cpp",
        "test_node_connectivity.cpp",
        
        # Output and log files
        "neo_node_error.txt",
        "neo_node_output.txt",
        
        # Script files
        "build_and_run.sh",
        "run_demo.py",
        "run_neo_cli.bat",
        "run_neo_node.bat",
        
        # This cleanup script itself
        "cleanup_project.py"
    ]
    
    # Directories to remove
    dirs_to_remove = [
        "build/build",  # Nested build directory
        "scripts",      # Script directory with old scripts
    ]
    
    # Pattern-based removals
    patterns_to_remove = [
        "*.tmp",
        "*.temp", 
        "*.log",
        "*.cache",
        "build-*",
        "out/",
        "cmake-build-*/"
    ]
    
    removed_count = 0
    
    # Remove specific files
    print("\n🗑️  Removing intermediate files...")
    for file_path in files_to_remove:
        if os.path.exists(file_path):
            try:
                os.remove(file_path)
                print(f"   ✅ Removed: {file_path}")
                removed_count += 1
            except Exception as e:
                print(f"   ❌ Failed to remove {file_path}: {e}")
        else:
            print(f"   ⚪ Not found: {file_path}")
    
    # Remove directories
    print("\n📁 Removing temporary directories...")
    for dir_path in dirs_to_remove:
        if os.path.exists(dir_path):
            try:
                shutil.rmtree(dir_path)
                print(f"   ✅ Removed directory: {dir_path}")
                removed_count += 1
            except Exception as e:
                print(f"   ❌ Failed to remove {dir_path}: {e}")
        else:
            print(f"   ⚪ Not found: {dir_path}")
    
    # Remove pattern-based files
    print("\n🔍 Removing pattern-based files...")
    for pattern in patterns_to_remove:
        matches = glob.glob(pattern, recursive=True)
        for match in matches:
            try:
                if os.path.isfile(match):
                    os.remove(match)
                    print(f"   ✅ Removed: {match}")
                    removed_count += 1
                elif os.path.isdir(match):
                    shutil.rmtree(match)
                    print(f"   ✅ Removed directory: {match}")
                    removed_count += 1
            except Exception as e:
                print(f"   ❌ Failed to remove {match}: {e}")
    
    # Clean up examples directory
    print("\n📚 Cleaning examples directory...")
    examples_dir = "examples"
    if os.path.exists(examples_dir):
        # Remove test and demo files from examples
        example_patterns = [
            "examples/*test*.cpp",
            "examples/*demo*.cpp", 
            "examples/*comprehensive*.cpp"
        ]
        
        for pattern in example_patterns:
            matches = glob.glob(pattern)
            for match in matches:
                try:
                    os.remove(match)
                    print(f"   ✅ Removed example: {match}")
                    removed_count += 1
                except Exception as e:
                    print(f"   ❌ Failed to remove {match}: {e}")
    
    # Display final project structure
    print("\n📊 Final project structure:")
    
    # Key directories that should remain
    important_dirs = [
        "include/neo",
        "src", 
        "tests",
        "apps",
        "config",
        "examples",
        ".github/workflows"
    ]
    
    for dir_path in important_dirs:
        if os.path.exists(dir_path):
            file_count = sum(len(files) for _, _, files in os.walk(dir_path))
            print(f"   📁 {dir_path}/  ({file_count} files)")
        else:
            print(f"   ❌ Missing: {dir_path}/")
    
    # Key files that should remain
    important_files = [
        "README.md",
        "CONTRIBUTING.md", 
        "CHANGELOG.md",
        "LICENSE",
        "CMakeLists.txt",
        ".gitignore",
        ".clang-format",
        "vcpkg.json"
    ]
    
    print(f"\n📄 Important files:")
    for file_path in important_files:
        if os.path.exists(file_path):
            size = os.path.getsize(file_path)
            print(f"   ✅ {file_path} ({size} bytes)")
        else:
            print(f"   ❌ Missing: {file_path}")
    
    print(f"\n🎉 Cleanup complete!")
    print(f"📊 Removed {removed_count} files/directories")
    print(f"✨ Project is now ready for GitHub!")
    
    # Final recommendations
    print(f"\n📋 Next steps:")
    print(f"   1. Review the cleaned project structure")
    print(f"   2. Test the build: mkdir build && cd build && cmake .. && make")
    print(f"   3. Run tests: ctest")
    print(f"   4. Initialize git: git init")
    print(f"   5. Add files: git add .")
    print(f"   6. Commit: git commit -m 'Initial Neo C++ implementation'")
    print(f"   7. Push to GitHub repository")

if __name__ == "__main__":
    try:
        cleanup_project()
    except KeyboardInterrupt:
        print(f"\n⚠️  Cleanup interrupted by user")
        sys.exit(1)
    except Exception as e:
        print(f"\n❌ Error during cleanup: {e}")
        sys.exit(1)
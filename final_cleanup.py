#!/usr/bin/env python3
"""
Final cleanup script for Neo C++ project
Removes all intermediate files, documents, and temporary scripts
"""

import os
import shutil
import glob
from pathlib import Path

def cleanup_project():
    """Remove all intermediate and temporary files"""
    
    print("🧹 Final Neo C++ Project Cleanup")
    print("================================")
    
    # Change to project directory
    os.chdir('/home/neo/git/csahrp_cpp')
    
    # Files to remove completely
    files_to_remove = [
        # Intermediate documentation
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
        "GIT_PUSH_GUIDE.md",
        
        # Temporary scripts
        "cleanup_project.py",
        "git_push_commands.sh",
        "git_commands_jimmy.sh",
        "push_to_github.sh",
        "git_push_clean.sh",
        "remove_unwanted_files.sh",
        "fix_git_repo.sh",
        "build_and_run.sh",
        "run_demo.py",
        "final_cleanup.py",
        
        # Batch files
        "run_neo_cli.bat",
        "run_neo_node.bat",
        
        # Test executables
        "neo-demo.exe",
        "neo-minimal.exe",
        "neo-network-demo.exe",
        
        # Temporary test files
        "minimal_neo_node.cpp",
        "neo_network_demo.cpp",
        "simple_neo_cli_test.cpp",
        "simple_neo_node.cpp",
        "test_build.cpp",
        "test_cmake.txt",
        "test_neo_node.cpp",
        "test_node_connectivity.cpp",
        
        # Output files
        "neo_node_error.txt",
        "neo_node_output.txt",
        
        # CMake cache (will be regenerated)
        "CMakeCache.txt",
    ]
    
    # Directories to remove completely
    dirs_to_remove = [
        "scripts",  # Old scripts directory
        "build/build",  # Nested build directory
        "CMakeFiles",  # CMake temporary files
        ".cursor",  # Cursor editor files
        ".trae",  # Temporary files
    ]
    
    # Remove individual files
    print("\n📄 Removing intermediate files...")
    removed_count = 0
    for file_path in files_to_remove:
        if os.path.exists(file_path):
            try:
                os.remove(file_path)
                print(f"   ✅ Removed: {file_path}")
                removed_count += 1
            except Exception as e:
                print(f"   ❌ Failed to remove {file_path}: {e}")
    
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
    
    # Clean up examples directory
    print("\n📚 Cleaning examples directory...")
    example_files_to_remove = [
        "examples/crypto_comprehensive_test.cpp",
        "examples/crypto_comprehensive_test_improved.cpp",
        "examples/crypto_test.cpp",
        "examples/gtest_example.cpp",
        "examples/io_comprehensive_test.cpp",
        "examples/io_comprehensive_test_improved.cpp",
        "examples/io_test.cpp",
        "examples/ledger_comprehensive_test.cpp",
        "examples/network_comprehensive_test.cpp",
        "examples/simple_test.cpp",
        "examples/vm_comprehensive_test.cpp",
        "examples/vm_lte_gte_test.cpp",
        "examples/vm_test.cpp",
    ]
    
    for file_path in example_files_to_remove:
        if os.path.exists(file_path):
            try:
                os.remove(file_path)
                print(f"   ✅ Removed: {file_path}")
                removed_count += 1
            except Exception as e:
                print(f"   ❌ Failed to remove {file_path}: {e}")
    
    # Remove any .exe files in root
    print("\n🗑️  Removing executable files...")
    for exe_file in glob.glob("*.exe"):
        try:
            os.remove(exe_file)
            print(f"   ✅ Removed: {exe_file}")
            removed_count += 1
        except Exception as e:
            print(f"   ❌ Failed to remove {exe_file}: {e}")
    
    # Remove any temporary test files from tests directory
    print("\n🧪 Cleaning test files...")
    test_files_to_remove = [
        "tests/standalone_le_ge_test.cpp",
        "tests/standalone_lte_gte_test.cpp",
        "tests/network_compatibility.cpp",
    ]
    
    for file_path in test_files_to_remove:
        if os.path.exists(file_path):
            try:
                os.remove(file_path)
                print(f"   ✅ Removed: {file_path}")
                removed_count += 1
            except Exception as e:
                print(f"   ❌ Failed to remove {file_path}: {e}")
    
    print(f"\n✅ Cleanup complete! Removed {removed_count} files/directories")
    
    # Show final structure
    print("\n📊 Final project structure:")
    important_items = [
        ("📄", "README.md"),
        ("📄", "CONTRIBUTING.md"),
        ("📄", "CHANGELOG.md"),
        ("📄", "LICENSE"),
        ("📄", "CMakeLists.txt"),
        ("📄", ".gitignore"),
        ("📄", ".clang-format"),
        ("📄", "vcpkg.json"),
        ("📁", "include/neo/"),
        ("📁", "src/"),
        ("📁", "tests/"),
        ("📁", "apps/"),
        ("📁", "config/"),
        ("📁", "examples/"),
        ("📁", "plugins/"),
        ("📁", ".github/workflows/"),
        ("📁", "@docs/"),
    ]
    
    for icon, item in important_items:
        if os.path.exists(item):
            print(f"   {icon} {item} ✓")
        else:
            print(f"   {icon} {item} ✗")
    
    print("\n✨ Project is now clean and ready for GitHub!")
    print("\n📋 Next steps:")
    print("   1. Review the cleaned structure")
    print("   2. Run: git add .")
    print("   3. Run: git commit -m 'chore: final project cleanup'")
    print("   4. Run: git push origin master")

if __name__ == "__main__":
    cleanup_project()
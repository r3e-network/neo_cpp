#!/bin/bash

echo "🧹 Executing Final Cleanup for Neo C++ Project"
echo "=============================================="
echo ""

cd /home/neo/git/csahrp_cpp

# Remove intermediate documentation files
echo "📄 Removing intermediate documentation..."
rm -f COMPREHENSIVE_MODULE_ANALYSIS.md
rm -f CONVERSION_COMPLETION_PLAN.md
rm -f CONVERSION_REVIEW_SUMMARY.md
rm -f DEVELOPMENT_GUIDE.md
rm -f FINAL_COMPLETION_CORRECTIONS_REPORT.md
rm -f FINAL_NEO_CPP_CONVERSION_REPORT.md
rm -f FINAL_NEO_CPP_STATUS.md
rm -f FINAL_VALIDATION_SCRIPT.cpp
rm -f IMPLEMENTATION_STATUS.md
rm -f NEO3_COMPATIBILITY_STATUS.md
rm -f NEO_CPP_CLI_COMPLETE.md
rm -f NEO_CPP_CLI_INTEGRATED_TEST_RESULTS.md
rm -f NEO_CPP_COMPLETION_PLAN.md
rm -f NEO_CPP_COMPLETION_STATUS_CURRENT.md
rm -f NEO_CPP_REVIEW_SUMMARY.md
rm -f PHASE1_PROGRESS_REPORT.md
rm -f PHASE_2_COMPLETION_REPORT.md
rm -f PHASE_3_1_COMPLETION_REPORT.md
rm -f PHASE_3_2_COMPLETION_REPORT.md
rm -f PHASE_4_1_BUILD_ANALYSIS.md
rm -f PRODUCTION_READY_SUMMARY.md
rm -f PROGRESS_SUMMARY.md
rm -f QUICK_START_NEXT_SESSION.md
rm -f TASK_4_INTEGRATION_TESTS_COMPLETE.md
rm -f TASK_5_BUILD_DEPLOYMENT_SCRIPTS_COMPLETE.md
rm -f GIT_PUSH_GUIDE.md

# Remove temporary scripts
echo "🔧 Removing temporary scripts..."
rm -f cleanup_project.py
rm -f git_push_commands.sh
rm -f git_commands_jimmy.sh
rm -f push_to_github.sh
rm -f git_push_clean.sh
rm -f remove_unwanted_files.sh
rm -f fix_git_repo.sh
rm -f build_and_run.sh
rm -f run_demo.py
rm -f final_cleanup.py
rm -f execute_cleanup.sh

# Remove batch files
echo "🗑️  Removing batch files..."
rm -f run_neo_cli.bat
rm -f run_neo_node.bat

# Remove test executables
echo "🗑️  Removing test executables..."
rm -f neo-demo.exe
rm -f neo-minimal.exe
rm -f neo-network-demo.exe

# Remove temporary test files
echo "🧪 Removing temporary test files..."
rm -f minimal_neo_node.cpp
rm -f neo_network_demo.cpp
rm -f simple_neo_cli_test.cpp
rm -f simple_neo_node.cpp
rm -f test_build.cpp
rm -f test_cmake.txt
rm -f test_neo_node.cpp
rm -f test_node_connectivity.cpp
rm -f neo_node_error.txt
rm -f neo_node_output.txt

# Remove scripts directory
echo "📁 Removing scripts directory..."
rm -rf scripts/

# Remove nested build directory
echo "📁 Removing nested build directory..."
rm -rf build/build/

# Remove CMake cache
rm -f CMakeCache.txt
rm -rf CMakeFiles/

# Clean examples directory
echo "📚 Cleaning examples directory..."
rm -f examples/crypto_comprehensive_test.cpp
rm -f examples/crypto_comprehensive_test_improved.cpp
rm -f examples/crypto_test.cpp
rm -f examples/gtest_example.cpp
rm -f examples/io_comprehensive_test.cpp
rm -f examples/io_comprehensive_test_improved.cpp
rm -f examples/io_test.cpp
rm -f examples/ledger_comprehensive_test.cpp
rm -f examples/network_comprehensive_test.cpp
rm -f examples/simple_test.cpp
rm -f examples/vm_comprehensive_test.cpp
rm -f examples/vm_lte_gte_test.cpp
rm -f examples/vm_test.cpp

# Clean test directory
echo "🧪 Cleaning test directory..."
rm -f tests/standalone_le_ge_test.cpp
rm -f tests/standalone_lte_gte_test.cpp
rm -f tests/network_compatibility.cpp

echo ""
echo "✅ Cleanup complete!"
echo ""
echo "📊 Remaining structure:"
ls -la | grep -E "^d|\.md$|\.txt$|\.json$|\.yml$|\.yaml$|CMakeLists.txt|LICENSE|\.gitignore|\.clang-format" | head -20

echo ""
echo "📋 Next steps:"
echo "1. git add ."
echo "2. git rm -r --cached .cursor/ .trae/ @docs/ neo/ vcpkg_installed/"
echo "3. git commit -m 'chore: final project cleanup - remove all intermediate files'"
echo "4. git push origin master --force"
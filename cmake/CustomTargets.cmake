# CustomTargets.cmake - Custom CMake targets for Neo C++
# This file defines custom targets that mirror Makefile functionality

# Get the number of processors for parallel builds
include(ProcessorCount)
ProcessorCount(N)
if(NOT N EQUAL 0)
    set(JOBS ${N})
else()
    set(JOBS 4)
endif()

# =============================================================================
# Help Target - List all available custom targets
# =============================================================================

add_custom_target(help-targets
    COMMAND ${CMAKE_COMMAND} -E echo "Available custom targets for Neo C++:"
    COMMAND ${CMAKE_COMMAND} -E echo ""
    COMMAND ${CMAKE_COMMAND} -E echo "Running Targets:"
    COMMAND ${CMAKE_COMMAND} -E echo "  make run           - Run Neo node with default TestNet settings"
    COMMAND ${CMAKE_COMMAND} -E echo "  make run-mainnet   - Run Neo node on MainNet"
    COMMAND ${CMAKE_COMMAND} -E echo "  make run-testnet   - Run Neo node on TestNet"
    COMMAND ${CMAKE_COMMAND} -E echo "  make run-private   - Run private network"
    COMMAND ${CMAKE_COMMAND} -E echo "  make run-rpc       - Start RPC server"
    COMMAND ${CMAKE_COMMAND} -E echo ""
    COMMAND ${CMAKE_COMMAND} -E echo "Testing Targets:"
    COMMAND ${CMAKE_COMMAND} -E echo "  make test          - Run all tests"
    COMMAND ${CMAKE_COMMAND} -E echo "  make test-all      - Run all tests with output on failure"
    COMMAND ${CMAKE_COMMAND} -E echo "  make test-unit     - Run unit tests only"
    COMMAND ${CMAKE_COMMAND} -E echo "  make test-verbose  - Run tests with verbose output"
    COMMAND ${CMAKE_COMMAND} -E echo ""
    COMMAND ${CMAKE_COMMAND} -E echo "Code Quality Targets:"
    COMMAND ${CMAKE_COMMAND} -E echo "  make format        - Format code with clang-format"
    COMMAND ${CMAKE_COMMAND} -E echo "  make format-check  - Check code format without modifying"
    COMMAND ${CMAKE_COMMAND} -E echo "  make tidy          - Run clang-tidy"
    COMMAND ${CMAKE_COMMAND} -E echo "  make check         - Run all code quality checks"
    COMMAND ${CMAKE_COMMAND} -E echo ""
    COMMAND ${CMAKE_COMMAND} -E echo "Build Targets:"
    COMMAND ${CMAKE_COMMAND} -E echo "  make libs          - Build core libraries only"
    COMMAND ${CMAKE_COMMAND} -E echo "  make examples      - Build and run examples"
    COMMAND ${CMAKE_COMMAND} -E echo "  make bench         - Run benchmarks"
    COMMAND ${CMAKE_COMMAND} -E echo "  make docs          - Generate documentation with Doxygen"
    COMMAND ${CMAKE_COMMAND} -E echo ""
    COMMAND ${CMAKE_COMMAND} -E echo "Utility Targets:"
    COMMAND ${CMAKE_COMMAND} -E echo "  make clean         - Clean build files"
    COMMAND ${CMAKE_COMMAND} -E echo "  make clean-all     - Clean everything including cache"
    COMMAND ${CMAKE_COMMAND} -E echo "  make install       - Install Neo C++"
    COMMAND ${CMAKE_COMMAND} -E echo "  make help          - Show all targets"
    COMMAND ${CMAKE_COMMAND} -E echo "  make help-targets  - Show this help message"
    COMMENT "Showing available custom targets..."
)

# =============================================================================
# Testing Targets
# =============================================================================

# Run all tests
add_custom_target(test-all
    COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure -j${JOBS}
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "Running all tests..."
)

# Run unit tests only
add_custom_target(test-unit
    COMMAND ${CMAKE_CTEST_COMMAND} -R "^test_" --output-on-failure -j${JOBS}
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "Running unit tests..."
)

# Run integration tests
add_custom_target(test-integration
    COMMAND ${CMAKE_CTEST_COMMAND} -R "integration" --output-on-failure -j${JOBS}
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "Running integration tests..."
)

# Run tests with verbose output
add_custom_target(test-verbose
    COMMAND ${CMAKE_CTEST_COMMAND} -V -j${JOBS}
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "Running tests with verbose output..."
)

# Run test runner script
add_custom_target(test-runner
    COMMAND ${CMAKE_SOURCE_DIR}/scripts/test_runner.sh
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "Running test runner script..."
)

# =============================================================================
# Code Quality Targets
# =============================================================================

# Format code using clang-format
add_custom_target(format
    COMMAND find ${CMAKE_SOURCE_DIR}/src ${CMAKE_SOURCE_DIR}/include
            -name "*.cpp" -o -name "*.h" | grep -v third_party | xargs clang-format -i
    COMMENT "Formatting code with clang-format..."
)

# Check code format without modifying
add_custom_target(format-check
    COMMAND find ${CMAKE_SOURCE_DIR}/src ${CMAKE_SOURCE_DIR}/include
            -name "*.cpp" -o -name "*.h" | grep -v third_party | xargs clang-format --dry-run --Werror
    COMMENT "Checking code format..."
)

# Run clang-tidy
if(CMAKE_EXPORT_COMPILE_COMMANDS)
    add_custom_target(tidy
        COMMAND run-clang-tidy -p ${CMAKE_BINARY_DIR} -quiet
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        COMMENT "Running clang-tidy..."
    )
else()
    add_custom_target(tidy
        COMMAND echo "Please set CMAKE_EXPORT_COMPILE_COMMANDS=ON to use clang-tidy"
        COMMENT "Clang-tidy requires compile_commands.json"
    )
endif()

# Run all code quality checks
add_custom_target(check
    DEPENDS format-check
    COMMENT "Running code quality checks..."
)

# =============================================================================
# Running Applications
# =============================================================================

# Main run target - runs the node with default settings
if(NEO_BUILD_APPS)
    add_custom_target(run
        COMMAND $<TARGET_FILE:neo_node> --config ${CMAKE_SOURCE_DIR}/config/testnet.json
        DEPENDS neo_node
        COMMENT "Starting Neo node with default settings (TestNet)..."
    )
endif()

# Run Neo node targets (only if neo_node will be built)
if(NEO_BUILD_APPS)
    # Run Neo node (mainnet)
    add_custom_target(run-mainnet
        COMMAND $<TARGET_FILE:neo_node> --network mainnet --config ${CMAKE_SOURCE_DIR}/config/mainnet.json
        DEPENDS neo_node
        COMMENT "Starting Neo node (MainNet)..."
    )

    # Run Neo node (testnet)
    add_custom_target(run-testnet
        COMMAND $<TARGET_FILE:neo_node> --network testnet --config ${CMAKE_SOURCE_DIR}/config/testnet.json
        DEPENDS neo_node
        COMMENT "Starting Neo node (TestNet)..."
    )

    # Run private network
    add_custom_target(run-private
        COMMAND $<TARGET_FILE:neo_node> --private --config ${CMAKE_SOURCE_DIR}/config/private.json
        DEPENDS neo_node
        COMMENT "Starting private network..."
    )
    
    # Run RPC server
    add_custom_target(run-rpc
        COMMAND $<TARGET_FILE:neo_node> --rpc --config ${CMAKE_SOURCE_DIR}/config/rpc.json
        DEPENDS neo_node
        COMMENT "Starting RPC server..."
    )
    
    # Aliases for convenience
    add_custom_target(mainnet DEPENDS run-mainnet)
    add_custom_target(testnet DEPENDS run-testnet)
else()
    # Fallback targets when apps not built
    add_custom_target(run-mainnet
        COMMAND ${CMAKE_COMMAND} -E echo "Neo node not built. Set NEO_BUILD_APPS=ON and rebuild."
        COMMENT "Neo node not available"
    )
    add_custom_target(run-testnet
        COMMAND ${CMAKE_COMMAND} -E echo "Neo node not built. Set NEO_BUILD_APPS=ON and rebuild."
        COMMENT "Neo node not available"
    )
    add_custom_target(mainnet DEPENDS run-mainnet)
    add_custom_target(testnet DEPENDS run-testnet)
endif()

# Run Neo CLI
if(TARGET neo-cli)
    add_custom_target(run-cli
        COMMAND $<TARGET_FILE:neo-cli>
        DEPENDS neo-cli
        COMMENT "Starting Neo CLI..."
    )
endif()


# =============================================================================
# Build Utilities
# =============================================================================

# Clean everything including CMake cache
add_custom_target(clean-all
    COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_BINARY_DIR}/CMakeFiles
    COMMAND ${CMAKE_COMMAND} -E remove ${CMAKE_BINARY_DIR}/CMakeCache.txt
    COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_BINARY_DIR}/Testing
    COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_BINARY_DIR}/_deps
    COMMAND ${CMAKE_COMMAND} -E echo "Cleaned all build files and cache"
    COMMENT "Cleaning all build files and CMake cache..."
)

# Build core libraries only
add_custom_target(libs
    DEPENDS neo_core neo_io neo_cryptography neo_vm neo_ledger neo_network
    COMMENT "Building core libraries..."
)

# Build and run examples
if(NEO_BUILD_EXAMPLES)
    add_custom_target(examples
        COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --target all
        COMMAND for example in ${CMAKE_BINARY_DIR}/examples/neo_example_* \; do
                if [ -f "$$example" ] \; then
                    echo "Running $$(basename $$example)..." \;
                    $$example || true \;
                fi \;
                done
        COMMENT "Building and running examples..."
    )
endif()

# Run benchmarks
add_custom_target(bench
    COMMAND ${CMAKE_BINARY_DIR}/benchmarks/neo_benchmarks
    COMMENT "Running benchmarks..."
)

# =============================================================================
# Coverage
# =============================================================================

if(CMAKE_BUILD_TYPE STREQUAL "Debug" AND NEO_BUILD_TESTS)
    add_custom_target(coverage
        COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
        COMMAND lcov --capture --directory . --output-file coverage.info
        COMMAND lcov --remove coverage.info '/usr/*' --output-file coverage.info
        COMMAND lcov --list coverage.info
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        COMMENT "Generating test coverage report..."
    )
endif()

# =============================================================================
# Documentation
# =============================================================================

# Generate documentation with Doxygen
find_package(Doxygen)
if(DOXYGEN_FOUND)
    add_custom_target(docs
        COMMAND ${DOXYGEN_EXECUTABLE} ${CMAKE_SOURCE_DIR}/Doxyfile
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        COMMENT "Generating documentation with Doxygen..."
    )
else()
    add_custom_target(docs
        COMMAND echo "Doxygen not found. Please install Doxygen to generate documentation."
        COMMENT "Doxygen not found"
    )
endif()

# =============================================================================
# Infrastructure Scripts
# =============================================================================

# Run infrastructure validation
add_custom_target(validate-infrastructure
    COMMAND ${CMAKE_SOURCE_DIR}/scripts/validate_infrastructure.sh
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "Validating infrastructure..."
)

# Run integration tests
add_custom_target(integration-test
    COMMAND ${CMAKE_SOURCE_DIR}/scripts/integration_test.sh
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "Running integration tests..."
)

# Run consensus tests
add_custom_target(consensus-test
    COMMAND ${CMAKE_SOURCE_DIR}/scripts/consensus_test.sh
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "Running consensus tests..."
)

# Run network partition tests
add_custom_target(partition-test
    COMMAND ${CMAKE_SOURCE_DIR}/scripts/partition_test.sh
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "Running network partition tests..."
)

# Run performance tests
add_custom_target(performance-test
    COMMAND ${CMAKE_SOURCE_DIR}/scripts/performance_test.sh
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "Running performance tests..."
)

# Run security audit
add_custom_target(security-audit
    COMMAND ${CMAKE_SOURCE_DIR}/scripts/security_audit.sh
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "Running security audit..."
)

# Deploy
add_custom_target(deploy
    COMMAND ${CMAKE_SOURCE_DIR}/scripts/deployment/deploy.sh
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "Deploying Neo C++..."
)

# =============================================================================
# Docker Targets
# =============================================================================

# Build Docker image
add_custom_target(docker
    COMMAND docker build
            --build-arg VERSION=${PROJECT_VERSION}
            --build-arg BUILD_TYPE=${CMAKE_BUILD_TYPE}
            -t neo-cpp:${PROJECT_VERSION}
            -t neo-cpp:latest
            -f ${CMAKE_SOURCE_DIR}/Dockerfile .
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "Building Docker image..."
)

# Run Docker container
add_custom_target(docker-run
    COMMAND docker run -it --rm
            --name neo-node
            -p 10332:10332
            -p 10333:10333
            -p 10334:10334
            -v neo-data:/data
            -e NETWORK=mainnet
            neo-cpp:latest
    COMMENT "Running Neo node in Docker..."
)

# Run tests in Docker
add_custom_target(docker-test
    COMMAND docker run --rm neo-cpp:latest make test
    COMMENT "Running tests in Docker..."
)

# =============================================================================
# Utility Targets
# =============================================================================

# Show build status
add_custom_target(status
    COMMAND ${CMAKE_COMMAND} -E echo "Build Status:"
    COMMAND ${CMAKE_COMMAND} -E echo "  Build Directory: ${CMAKE_BINARY_DIR}"
    COMMAND ${CMAKE_COMMAND} -E echo "  Build Type: ${CMAKE_BUILD_TYPE}"
    COMMAND ${CMAKE_COMMAND} -E echo "  C++ Standard: ${CMAKE_CXX_STANDARD}"
    COMMAND ${CMAKE_COMMAND} -E echo "  Tests: ${NEO_BUILD_TESTS}"
    COMMAND ${CMAKE_COMMAND} -E echo "  Examples: ${NEO_BUILD_EXAMPLES}"
    COMMAND ${CMAKE_COMMAND} -E echo "  Tools: ${NEO_BUILD_TOOLS}"
    COMMAND ${CMAKE_COMMAND} -E echo "  Apps: ${NEO_BUILD_APPS}"
    COMMENT "Showing build status..."
)

# Show version
add_custom_target(version
    COMMAND ${CMAKE_COMMAND} -E echo "${PROJECT_VERSION}"
    COMMENT "Neo C++ version ${PROJECT_VERSION}"
)

# Clean ccache (check if not already defined)
if(NOT TARGET ccache-clean)
    add_custom_target(ccache-clean
        COMMAND ccache -C
        COMMENT "Clearing ccache..."
    )
endif()

# Show ccache stats (check if not already defined)
if(NOT TARGET ccache-stats)
    add_custom_target(ccache-stats
        COMMAND ccache -s
        COMMENT "Showing ccache statistics..."
    )
endif()

# CI pipeline
add_custom_target(ci
    COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --target clean
    COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --target all
    COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
    COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --target format-check
    COMMENT "Running CI pipeline..."
)

# Help target (using a script to avoid shell issues)
file(WRITE ${CMAKE_BINARY_DIR}/show_help.cmake "
message(\"Neo C++ Build System - CMake Targets\")
message(\"======================================\")
message(\"\")
message(\"Basic Build Targets:\")
message(\"  all              - Build everything\")
message(\"  clean            - Clean build artifacts\")
message(\"  libs             - Build core libraries only\")
message(\"  install          - Install to system\")
message(\"\")
message(\"Testing Targets:\")
message(\"  test             - Run CTest tests\")
message(\"  test-all         - Run all tests\")
message(\"  test-unit        - Run unit tests only\")
message(\"  test-integration - Run integration tests\")
message(\"  test-verbose     - Run tests with verbose output\")
message(\"  test-runner      - Run test runner script\")
message(\"  coverage         - Generate coverage report\")
message(\"\")
message(\"Quality Targets:\")
message(\"  format           - Format code with clang-format\")
message(\"  format-check     - Check code format\")
message(\"  tidy             - Run clang-tidy\")
message(\"  check            - Run all quality checks\")
message(\"\")
message(\"Running Targets:\")
message(\"  run-mainnet      - Run Neo node mainnet\")
message(\"  run-testnet      - Run Neo node testnet\")
message(\"  run-private      - Run private network\")
message(\"  run-cli          - Run Neo CLI\")
message(\"  run-rpc          - Start RPC server\")
message(\"  mainnet          - Alias for run-mainnet\")
message(\"  testnet          - Alias for run-testnet\")
message(\"\")
message(\"Infrastructure Targets:\")
message(\"  validate-infrastructure - Validate infrastructure\")
message(\"  integration-test        - Run integration tests\")
message(\"  consensus-test          - Run consensus tests\")
message(\"  partition-test          - Run partition tests\")
message(\"  performance-test        - Run performance tests\")
message(\"  security-audit          - Run security audit\")
message(\"  deploy                  - Deploy Neo C++\")
message(\"\")
message(\"Docker Targets:\")
message(\"  docker           - Build Docker image\")
message(\"  docker-run       - Run in Docker\")
message(\"  docker-test      - Run tests in Docker\")
message(\"\")
message(\"Utility Targets:\")
message(\"  status           - Show build status\")
message(\"  version          - Show version\")
message(\"  docs             - Generate documentation\")
message(\"  ccache-clean     - Clear ccache\")
message(\"  ccache-stats     - Show ccache stats\")
message(\"  ci               - Run CI pipeline\")
message(\"  help-neo         - Show this help\")
")

add_custom_target(help-neo
    COMMAND ${CMAKE_COMMAND} -P ${CMAKE_BINARY_DIR}/show_help.cmake
    COMMENT "Showing available CMake targets..."
)

# Message about custom targets
message(STATUS "Custom targets loaded. Run 'cmake --build . --target help' to see all available targets")
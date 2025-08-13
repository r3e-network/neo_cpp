# Neo C++ Build Commands Reference

This document shows how to use both `make` and `cmake` commands for building and managing the Neo C++ project.

## Quick Reference Table

| Task | Make Command | CMake Command |
|------|--------------|---------------|
| **Build** | `make` | `cmake --build build` |
| **Clean** | `make clean` | `cmake --build build --target clean` |
| **Run Tests** | `make test` | `cmake --build build --target test` |
| **Format Code** | `make format` | `cmake --build build --target format` |
| **Run Mainnet** | `make mainnet` | `cmake --build build --target mainnet` |
| **Run Testnet** | `make testnet` | `cmake --build build --target testnet` |
| **Show Help** | `make help` | `cmake --build build --target help` |

## Building the Project

### Using Make
```bash
# Configure and build (Release mode)
make

# Build in Debug mode
make debug

# Build in Release mode
make release

# Build core libraries only
make libs

# Clean and rebuild
make rebuild
```

### Using CMake
```bash
# Configure
cmake -B build -DCMAKE_BUILD_TYPE=Release

# Build
cmake --build build

# Build with parallel jobs
cmake --build build -j8

# Build specific target
cmake --build build --target neo_core

# Build core libraries
cmake --build build --target libs
```

## Running Tests

### Using Make
```bash
# Run all tests
make test

# Run unit tests only
make test-unit

# Run tests with verbose output
make test-verbose

# Generate coverage report
make coverage

# Run test runner script
./scripts/test_runner.sh
```

### Using CMake
```bash
# Run all tests
cmake --build build --target test

# Run all tests (alternative)
cmake --build build --target test-all

# Run unit tests only
cmake --build build --target test-unit

# Run integration tests
cmake --build build --target test-integration

# Run with verbose output
cmake --build build --target test-verbose

# Run test runner script
cmake --build build --target test-runner
```

### Using CTest Directly
```bash
cd build
ctest                           # Run all tests
ctest -R "test_"               # Run tests matching pattern
ctest --output-on-failure      # Show output on failure
ctest -V                       # Verbose output
ctest -j8                      # Run with 8 parallel jobs
```

## Code Quality

### Using Make
```bash
# Format code
make format

# Check format without modifying
make format-check

# Run clang-tidy
make tidy

# Run all checks
make check
```

### Using CMake
```bash
# Format code
cmake --build build --target format

# Check format
cmake --build build --target format-check

# Run clang-tidy
cmake --build build --target tidy

# Run all checks
cmake --build build --target check
```

## Running Neo Node

### Using Make
```bash
# Run mainnet node
make mainnet
# or
make run

# Run testnet node
make testnet
# or
make run-testnet

# Run private network
make run-private

# Run CLI
make run-cli

# Run RPC server
make run-rpc
```

### Using CMake
```bash
# Run mainnet node
cmake --build build --target run-mainnet
# or
cmake --build build --target mainnet

# Run testnet node
cmake --build build --target run-testnet
# or
cmake --build build --target testnet

# Run private network
cmake --build build --target run-private

# Run CLI
cmake --build build --target run-cli

# Run RPC server
cmake --build build --target run-rpc
```

## Infrastructure Testing

### Using Make
```bash
# Validate infrastructure
./scripts/validate_infrastructure.sh

# Run integration tests
./scripts/integration_test.sh

# Run consensus tests
./scripts/consensus_test.sh

# Run performance tests
./scripts/performance_test.sh

# Run security audit
./scripts/security_audit.sh
```

### Using CMake
```bash
# Validate infrastructure
cmake --build build --target validate-infrastructure

# Run integration tests
cmake --build build --target integration-test

# Run consensus tests
cmake --build build --target consensus-test

# Run performance tests
cmake --build build --target performance-test

# Run security audit
cmake --build build --target security-audit
```

## Docker Operations

### Using Make
```bash
# Build Docker image
make docker

# Run in Docker
make docker-run

# Run tests in Docker
make docker-test

# Push to registry
make docker-push

# Run mainnet in Docker
make run-docker-mainnet

# Run testnet in Docker
make run-docker-testnet
```

### Using CMake
```bash
# Build Docker image
cmake --build build --target docker

# Run in Docker
cmake --build build --target docker-run

# Run tests in Docker
cmake --build build --target docker-test
```

## Utility Commands

### Using Make
```bash
# Show build status
make status

# Show version
make version

# Show build info
make info

# Generate documentation
make docs

# Install
make install

# Uninstall
make uninstall

# Show help
make help
```

### Using CMake
```bash
# Show build status
cmake --build build --target status

# Show version
cmake --build build --target version

# Generate documentation
cmake --build build --target docs

# Install
cmake --install build

# Show help
cmake --build build --target help
```

## CI/CD Commands

### Using Make
```bash
# Run full CI pipeline
make ci

# Run benchmarks
make bench

# Run examples
make examples
```

### Using CMake
```bash
# Run full CI pipeline
cmake --build build --target ci

# Run benchmarks
cmake --build build --target bench

# Run examples (if built)
cmake --build build --target examples
```

## Cache Management

### Using Make
```bash
# Show ccache statistics
make ccache-stats

# Clear ccache
make ccache-clean
```

### Using CMake
```bash
# Show ccache statistics
cmake --build build --target ccache-stats

# Clear ccache
cmake --build build --target ccache-clean
```

## Advanced Build Options

### Configuration Options
```bash
# Configure with options (both Make and CMake)
cmake -B build \
  -DCMAKE_BUILD_TYPE=Debug \
  -DNEO_BUILD_TESTS=ON \
  -DNEO_BUILD_EXAMPLES=ON \
  -DNEO_BUILD_TOOLS=ON \
  -DNEO_BUILD_APPS=ON \
  -DENABLE_COVERAGE=OFF \
  -DENABLE_ASAN=OFF

# Using Make with options
make BUILD_TYPE=Debug ENABLE_TESTS=ON ENABLE_ASAN=ON
```

### Environment Variables
```bash
# Set build parallelism
export JOBS=8
make

# Set log level for running
export LOG_LEVEL=debug
make run

# Set installation prefix
export INSTALL_PREFIX=/opt/neo
make install
```

## Troubleshooting

### Build Issues
```bash
# Clean everything and rebuild
make distclean
make

# Or with CMake
rm -rf build
cmake -B build
cmake --build build
```

### Test Failures
```bash
# Run specific test with output
./build/tests/unit/cryptography/test_cryptography --gtest_filter="HashTest.*"

# Run with debugging
gdb ./build/tests/unit/ledger/test_ledger
```

### Missing Dependencies
```bash
# Install dependencies (macOS)
brew install cmake openssl boost rocksdb nlohmann-json spdlog gtest

# Install dependencies (Ubuntu/Debian)
sudo apt-get update
sudo apt-get install -y \
  cmake build-essential libssl-dev libboost-all-dev \
  librocksdb-dev nlohmann-json3-dev libspdlog-dev libgtest-dev
```

## Tips and Best Practices

1. **Use parallel builds**: Both `make -j8` and `cmake --build build -j8` speed up compilation
2. **Enable ccache**: Automatically enabled if ccache is installed
3. **Use build directories**: Keep source tree clean by building out-of-source
4. **Check status**: Run `make status` or `cmake --build build --target status` to see what's built
5. **Use shortcuts**: `make b` (build), `make t` (test), `make r` (run), `make c` (clean)
6. **Debug builds**: Use `CMAKE_BUILD_TYPE=Debug` for better debugging experience
7. **Verbose output**: Add `VERBOSE=1` to make commands or `-v` to cmake commands

## Summary

Both `make` and `cmake` commands are fully supported and provide the same functionality. The Makefile wraps CMake commands for convenience, while CMake custom targets provide direct access to all build operations. Choose the interface that best fits your workflow.
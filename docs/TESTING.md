# Neo C++ Testing Guide

## Overview

The Neo C++ project uses Google Test (GTest) framework for comprehensive testing across all components. Our test suite ensures code quality, reliability, and performance.

## Test Architecture

### Test Categories

#### 1. Unit Tests
- **Purpose**: Test individual components in isolation
- **Location**: `tests/unit/`
- **Coverage**: ~3,000+ test cases
- **Execution Time**: < 10 seconds
- **Run Command**: `ctest -E test_integration`

#### 2. Integration Tests
- **Purpose**: Test component interactions and workflows
- **Location**: `tests/integration/`
- **Coverage**: 84 test scenarios
- **Execution Time**: ~45 seconds
- **Run Command**: `ctest -R test_integration`

#### 3. Plugin Tests
- **Purpose**: Test plugin system functionality
- **Location**: `tests/plugins/`
- **Coverage**: Basic plugin operations
- **Execution Time**: < 1 second
- **Run Command**: `ctest -R plugins_tests`

## Quick Start

### Prerequisites
```bash
# Ubuntu/Debian
sudo apt-get install libgtest-dev cmake build-essential

# macOS
brew install googletest cmake

# Windows
vcpkg install gtest
```

### Building Tests
```bash
mkdir build && cd build
cmake .. -DBUILD_TESTS=ON
make -j8
```

### Running Tests

#### All Tests
```bash
# Using CTest
ctest

# Using test runner script
./test_runner.sh all

# Using Python monitor
python3 test_monitor.py run
```

#### Specific Test Categories
```bash
# Unit tests only
ctest -E test_integration

# Integration tests only
ctest -R test_integration

# Quick tests (exclude slow tests)
./test_runner.sh quick
```

#### Individual Test Suites
```bash
# Run specific test
ctest -R test_cryptography

# Run with verbose output
ctest -R test_cryptography -V

# Run with filter
./tests/unit/cryptography/test_cryptography --gtest_filter="HashTest.*"
```

## Test Organization

### Directory Structure
```
tests/
├── unit/                    # Unit tests
│   ├── cryptography/       # Cryptographic operations
│   ├── io/                 # I/O and serialization
│   ├── json/               # JSON handling
│   ├── extensions/         # Extension methods
│   ├── persistence/        # Data persistence
│   ├── network/            # Network layer
│   ├── ledger/             # Blockchain ledger
│   ├── vm/                 # Virtual machine
│   ├── smartcontract/      # Smart contracts
│   ├── wallets/            # Wallet management
│   ├── consensus/          # Consensus mechanism
│   ├── rpc/                # RPC server
│   └── cli/                # CLI tool
├── integration/            # Integration tests
│   └── test_integration    # All integration scenarios
└── plugins/                # Plugin tests
    └── plugins_tests       # Plugin functionality

```

### Test Naming Conventions
- **Test Suite**: `TestComponentName` (e.g., `HashTest`)
- **Test Case**: `TestSpecificFunctionality` (e.g., `TestSha256`)
- **File Name**: `test_component.cpp` (e.g., `test_cryptography.cpp`)

## Writing Tests

### Basic Test Structure
```cpp
#include <gtest/gtest.h>
#include <neo/component.h>

// Test fixture for shared setup
class ComponentTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Initialize test environment
    }
    
    void TearDown() override {
        // Clean up test environment
    }
    
    // Shared test data
    Component component_;
};

// Basic test
TEST_F(ComponentTest, TestBasicFunctionality) {
    // Arrange
    auto input = CreateTestInput();
    
    // Act
    auto result = component_.Process(input);
    
    // Assert
    EXPECT_TRUE(result.IsValid());
    EXPECT_EQ(result.GetValue(), expected_value);
}

// Parameterized test
TEST_P(ComponentTest, TestWithParameters) {
    auto param = GetParam();
    auto result = component_.Process(param);
    EXPECT_TRUE(result.IsValid());
}

INSTANTIATE_TEST_SUITE_P(
    ComponentTestCases,
    ComponentTest,
    ::testing::Values(param1, param2, param3)
);
```

### Test Utilities
```cpp
// Test data factory
class TestDataFactory {
public:
    static Block CreateValidBlock(uint32_t height = 0);
    static Transaction CreateValidTransaction();
    static Witness CreateValidWitness();
};

// Test helpers
namespace TestHelpers {
    void SetupTestBlockchain();
    void CleanupTestData();
    bool CompareBlocks(const Block& a, const Block& b);
}
```

## Test Coverage

### Current Coverage
- **Core Components**: ~90%
- **Cryptography**: 95%
- **Virtual Machine**: 85%
- **Smart Contracts**: 80%
- **Network Layer**: 75%

### Generating Coverage Reports
```bash
# Build with coverage
cmake .. -DCMAKE_BUILD_TYPE=Debug -DENABLE_COVERAGE=ON
make

# Run tests
ctest

# Generate report
lcov --capture --directory . --output-file coverage.info
lcov --remove coverage.info '/usr/*' '*/tests/*' --output-file coverage_filtered.info
genhtml coverage_filtered.info --output-directory coverage_report

# View report
open coverage_report/index.html
```

## Performance Testing

### Benchmark Tests
```cpp
#include <benchmark/benchmark.h>

static void BM_BlockProcessing(benchmark::State& state) {
    auto block = CreateTestBlock();
    for (auto _ : state) {
        ProcessBlock(block);
    }
}
BENCHMARK(BM_BlockProcessing);
```

### Performance Metrics
- **Block Processing**: < 10ms per block
- **Transaction Verification**: < 1ms per transaction
- **VM Execution**: < 100μs per operation
- **Cryptographic Operations**: < 1ms per operation

## Continuous Integration

### GitHub Actions
Tests run automatically on:
- Every push to main/develop branches
- Every pull request
- Daily scheduled runs
- Manual workflow dispatch

### Test Matrix
- **Operating Systems**: Ubuntu, macOS
- **Compilers**: GCC, Clang
- **Build Types**: Debug, Release

## Known Issues

### Disabled Tests
8 tests in `BlockSyncTest` are disabled due to threading issues:
- `DISABLED_TestBlockDownloadAndProcessing`
- `DISABLED_TestConcurrentBlockProcessing`
- `DISABLED_TestOrphanBlockHandling`
- `DISABLED_TestBlockInventoryHandling`
- `DISABLED_TestSyncProgressTracking`
- `DISABLED_TestMultiplePeerSync`
- `DISABLED_TestPerformanceMetrics`
- `DISABLED_TestErrorRecoveryAndResilience`

### Skipped Tests
6 tests in `BLS12381CompleteTest` are skipped pending implementation:
- `G1PointArithmetic`
- `G2PointConstruction`
- `G2PointArithmetic`
- `BLSSignatures`
- `HelperFunctions`
- `FieldArithmeticConsistency`

## Troubleshooting

### Common Issues

#### Tests Hang
```bash
# Run with timeout
ctest --timeout 60

# Kill hanging test
pkill -f test_integration
```

#### Segmentation Fault
```bash
# Run with debugger
gdb ./tests/integration/test_integration
run
bt  # backtrace when it crashes
```

#### Memory Leaks
```bash
# Run with valgrind
valgrind --leak-check=full ./tests/unit/test_name
```

### Debug Mode
```bash
# Build in debug mode
cmake .. -DCMAKE_BUILD_TYPE=Debug
make

# Run with verbose output
ctest -V

# Run with test output on failure
ctest --output-on-failure
```

## Test Tools

### 1. test_runner.sh
Convenient bash script for test execution:
```bash
./test_runner.sh all        # Run all tests
./test_runner.sh unit       # Run unit tests
./test_runner.sh parallel   # Run in parallel
./test_runner.sh coverage   # Generate coverage
```

### 2. test_monitor.py
Python script for test monitoring and analysis:
```bash
python3 test_monitor.py discover  # List all tests
python3 test_monitor.py run       # Run with analysis
python3 test_monitor.py watch     # Real-time monitoring
```

### 3. CTest Commands
```bash
ctest -N                    # List tests without running
ctest -j8                   # Run in parallel with 8 jobs
ctest --rerun-failed        # Rerun only failed tests
ctest --schedule-random     # Run in random order
```

## Best Practices

### Do's
- ✅ Write tests before implementing features (TDD)
- ✅ Keep tests independent and isolated
- ✅ Use descriptive test names
- ✅ Test both success and failure cases
- ✅ Mock external dependencies
- ✅ Keep tests fast (< 1 second per test)
- ✅ Use test fixtures for shared setup

### Don'ts
- ❌ Don't use global state in tests
- ❌ Don't rely on test execution order
- ❌ Don't test implementation details
- ❌ Don't ignore flaky tests
- ❌ Don't commit failing tests
- ❌ Don't use production data in tests

## Contributing

### Adding New Tests
1. Create test file in appropriate directory
2. Follow naming conventions
3. Include necessary headers
4. Write comprehensive test cases
5. Ensure tests pass locally
6. Update CMakeLists.txt
7. Submit pull request

### Test Review Checklist
- [ ] Tests are meaningful and test actual functionality
- [ ] Tests cover edge cases
- [ ] Tests are independent
- [ ] Tests have descriptive names
- [ ] Tests run quickly
- [ ] Tests don't use hardcoded paths
- [ ] Tests clean up after themselves

## Resources

### Documentation
- [Google Test Documentation](https://google.github.io/googletest/)
- [Google Test Primer](https://google.github.io/googletest/primer.html)
- [Advanced Guide](https://google.github.io/googletest/advanced.html)

### Tools
- [lcov](http://ltp.sourceforge.net/coverage/lcov.php) - Coverage reporting
- [valgrind](https://valgrind.org/) - Memory leak detection
- [gdb](https://www.gnu.org/software/gdb/) - Debugging
- [benchmark](https://github.com/google/benchmark) - Performance testing

---
*Last Updated: 2025-08-13 | Neo C++ Testing Guide v1.0*
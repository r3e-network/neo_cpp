# Neo C++ Make Targets Documentation

## Overview
After configuring the project with CMake, you can use various `make` commands to build, test, and run the Neo C++ node. This document lists all available targets and their usage.

## Setup
```bash
# Create build directory and configure
mkdir build
cd build
cmake ..

# Or with specific options
cmake .. -DCMAKE_BUILD_TYPE=Release -DNEO_BUILD_TESTS=ON
```

## Available Make Targets

### Quick Help
```bash
make help-targets    # Show all custom targets with descriptions
make help           # Show all targets (including CMake built-ins)
```

### Building

| Command | Description |
|---------|-------------|
| `make` or `make all` | Build everything (default target) |
| `make neo_node` | Build only the Neo node executable |
| `make neo_cli_tool` | Build only the CLI tool |
| `make libs` | Build core libraries only |
| `make -j8` | Build with 8 parallel jobs (faster) |
| `make clean` | Clean build files |
| `make clean-all` | Clean everything including CMake cache |

### Running the Node

| Command | Description |
|---------|-------------|
| `make run` | Run Neo node with default TestNet settings |
| `make run-mainnet` | Run Neo node on MainNet |
| `make run-testnet` | Run Neo node on TestNet |
| `make run-private` | Run private network |
| `make run-rpc` | Start RPC server |
| `make mainnet` | Alias for run-mainnet |
| `make testnet` | Alias for run-testnet |

**Example:**
```bash
# Build and run the node
make neo_node
make run

# Or run on mainnet
make run-mainnet
```

### Testing

| Command | Description |
|---------|-------------|
| `make test` | Run all tests (CMake built-in) |
| `make test-all` | Run all tests with output on failure |
| `make test-unit` | Run unit tests only |
| `make test-verbose` | Run tests with verbose output |
| `make test-integration` | Run integration tests |
| `make coverage` | Generate test coverage report (Debug build only) |

**Example:**
```bash
# Build tests and run them
make test_cryptography test_io test_vm
make test

# Run with verbose output
make test-verbose
```

### Code Quality

| Command | Description |
|---------|-------------|
| `make format` | Format code with clang-format |
| `make format-check` | Check code format without modifying |
| `make tidy` | Run clang-tidy static analysis |
| `make check` | Run all code quality checks |

**Example:**
```bash
# Format all code
make format

# Check if code is properly formatted
make format-check
```

### Documentation

| Command | Description |
|---------|-------------|
| `make docs` | Generate documentation with Doxygen |
| `make help-neo` | Show Neo-specific help |

### Advanced Targets

| Command | Description |
|---------|-------------|
| `make examples` | Build and run examples |
| `make bench` | Run performance benchmarks |
| `make install` | Install Neo C++ system-wide |
| `make package` | Create distribution package |

### Infrastructure & CI/CD

| Command | Description |
|---------|-------------|
| `make docker` | Build Docker image |
| `make docker-run` | Run in Docker container |
| `make docker-test` | Run tests in Docker |
| `make ci` | Run full CI pipeline locally |
| `make deploy` | Deploy to production |

### Specialized Testing

| Command | Description |
|---------|-------------|
| `make consensus-test` | Run consensus protocol tests |
| `make partition-test` | Run network partition tests |
| `make performance-test` | Run performance benchmarks |
| `make integration-test` | Run integration test suite |

## Common Workflows

### 1. Build and Run Node
```bash
mkdir build && cd build
cmake ..
make -j8
make run
```

### 2. Development Workflow
```bash
# Make changes to code
make format          # Format code
make -j8            # Build changes
make test           # Run tests
make run            # Test locally
```

### 3. Testing Workflow
```bash
cmake .. -DNEO_BUILD_TESTS=ON
make -j8
make test-all       # Run all tests
make test-verbose   # Debug failing tests
```

### 4. Clean Rebuild
```bash
make clean-all      # Clean everything
cmake ..            # Reconfigure
make -j8            # Fresh build
```

### 5. Release Build
```bash
cmake .. -DCMAKE_BUILD_TYPE=Release -DNEO_BUILD_TESTS=OFF
make -j8
make install
```

## Troubleshooting

### "No rule to make target"
If you get this error, the target might not be available because:
1. The project wasn't configured with the required options
2. Dependencies are missing
3. You need to run `cmake ..` first

**Solution:**
```bash
# Reconfigure with all features
cmake .. -DNEO_BUILD_TESTS=ON -DNEO_BUILD_APPS=ON -DNEO_BUILD_TOOLS=ON
make <target>
```

### Tests Not Found
If `make test` reports missing test executables:
```bash
# Build test executables first
make test_cryptography test_io test_vm test_ledger
make test
```

### Target Not Available
Some targets require specific CMake options:
- Tests require: `-DNEO_BUILD_TESTS=ON`
- Examples require: `-DNEO_BUILD_EXAMPLES=ON`
- Apps require: `-DNEO_BUILD_APPS=ON`

## Performance Tips

1. **Use parallel builds**: `make -j$(nproc)` on Linux or `make -j$(sysctl -n hw.ncpu)` on macOS
2. **Use ccache**: Automatically enabled if installed
3. **Release builds**: Use `-DCMAKE_BUILD_TYPE=Release` for production
4. **Minimal builds**: Disable tests and examples for faster builds

## Custom Targets

You can add your own targets by editing `cmake/CustomTargets.cmake`:

```cmake
add_custom_target(my-target
    COMMAND echo "Running my custom target"
    DEPENDS some_dependency
    COMMENT "Description of my target"
)
```

## Summary

The Neo C++ build system provides comprehensive make targets for:
- **Building**: Flexible build options with parallel support
- **Running**: Multiple network configurations
- **Testing**: Comprehensive test suite with coverage
- **Quality**: Code formatting and static analysis
- **Documentation**: Automated documentation generation

Use `make help-targets` to see all available custom targets at any time.

---
*Last Updated: August 2025*
*Neo C++ Version: 1.2.0*
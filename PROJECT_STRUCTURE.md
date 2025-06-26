# Neo C++ Project Structure

This document outlines the clean, production-ready structure of the Neo C++ blockchain implementation.

## Directory Structure

```
neo_cpp/
├── CMakeLists.txt              # Main CMake configuration
├── README.md                   # Project documentation
├── LICENSE                     # Project license
├── .gitignore                  # Git ignore rules
├── PROJECT_STRUCTURE.md        # This file
├── vcpkg.json                  # Package dependencies
├── vcpkg-configuration.json    # vcpkg configuration
│
├── apps/                       # Application executables
│   ├── neo_node.cpp           # Full Neo node application
│   ├── neo_node_minimal.cpp   # Minimal node for testing
│   ├── cli/                   # Command-line interface
│   └── node/                  # Node service
│
├── include/neo/               # Public header files
│   ├── blockchain/            # Blockchain components
│   ├── consensus/             # dBFT consensus
│   ├── core/                  # Core utilities
│   ├── cryptography/          # Cryptographic functions
│   ├── io/                    # Input/output utilities
│   ├── json/                  # JSON handling
│   ├── ledger/                # Ledger and transactions
│   ├── logging/               # Logging framework
│   ├── network/               # P2P networking
│   ├── persistence/           # Data storage
│   ├── rpc/                   # RPC server
│   ├── smartcontract/         # Smart contract VM
│   ├── vm/                    # Virtual machine
│   └── wallets/               # Wallet functionality
│
├── src/                       # Implementation files
│   ├── consensus/             # Consensus implementation
│   ├── core/                  # Core utilities
│   ├── cryptography/          # Crypto implementation
│   ├── io/                    # I/O implementation
│   ├── json/                  # JSON implementation
│   ├── ledger/                # Ledger implementation
│   ├── logging/               # Logging implementation
│   ├── network/               # Network implementation
│   ├── persistence/           # Storage implementation
│   ├── rpc/                   # RPC implementation
│   ├── smartcontract/         # Smart contract implementation
│   └── vm/                    # VM implementation
│
├── tests/                     # Test suites
│   ├── unit/                  # Unit tests
│   ├── integration/           # Integration tests
│   ├── benchmarks/            # Performance tests
│   └── simple_unit_test.cpp   # Basic test runner
│
├── scripts/                   # Build and utility scripts
│   ├── build.sh               # Build script
│   ├── run_tests.sh           # Test runner
│   └── deploy.sh              # Deployment script
│
├── config/                    # Configuration files
│   └── production_config.json # Production settings
│
├── docs/                      # Documentation
│   ├── README.md              # Getting started
│   ├── api-reference.md       # API documentation
│   ├── architecture.md        # Architecture overview
│   ├── deployment-guide.md    # Deployment guide
│   └── testing-guide.md       # Testing documentation
│
├── examples/                  # Example code
│   ├── consensus/             # Consensus examples
│   ├── ledger/                # Ledger examples
│   ├── network/               # Network examples
│   ├── smartcontract/         # Smart contract examples
│   └── vm/                    # VM examples
│
├── third_party/               # Third-party dependencies
│   ├── googletest/            # Google Test framework
│   └── httplib/               # HTTP library
│
├── tools/                     # Development tools
│   ├── cli/                   # CLI tools
│   └── gui/                   # GUI tools
│
├── cmake/                     # CMake modules
│   ├── FindDependencies.cmake # Dependency finder
│   ├── SetupCompiler.cmake    # Compiler setup
│   └── SetupTesting.cmake     # Test setup
│
├── plugins/                   # Optional plugins
│   ├── applicationlogs/       # Application logs plugin
│   ├── dbft/                  # dBFT plugin
│   └── rpcserver/             # RPC server plugin
│
└── archive/                   # Archived files (development history)
    ├── docs/                  # Old documentation
    ├── examples/              # Old examples
    └── scripts/               # Old scripts
```

## Key Components

### Core Libraries
- **neo_core**: Core utilities and system components
- **neo_cryptography**: Cryptographic functions and algorithms
- **neo_io**: Input/output and serialization
- **neo_json**: JSON handling and manipulation
- **neo_logging**: Structured logging framework

### Blockchain Components
- **neo_ledger**: Blockchain ledger and transactions
- **neo_consensus**: dBFT consensus mechanism
- **neo_vm**: Neo Virtual Machine
- **neo_smartcontract**: Smart contract execution
- **neo_persistence**: Data storage and caching

### Network and RPC
- **neo_network**: P2P networking (currently modular)
- **neo_rpc**: JSON-RPC 2.0 server
- **neo_wallets**: Wallet functionality (extensible)

### Applications
- **neo_node**: Full blockchain node
- **neo_node_minimal**: Minimal node for testing
- **neo_cli**: Command-line interface

## Build Targets

```bash
# Main library
cmake --build build --target neo_cpp

# Applications
cmake --build build --target neo_node
cmake --build build --target neo_cli

# Tests
cmake --build build --target simple_unit_test
cmake --build build --target integration_tests
```

## Dependencies

- **Required**: CMake 3.15+, C++20 compiler, OpenSSL, nlohmann/json
- **Optional**: spdlog, GoogleTest, Boost (multiprecision), RocksDB
- **Tools**: vcpkg for package management

## Configuration

The project uses CMake with vcpkg for dependency management. Key configuration files:
- `CMakeLists.txt`: Main build configuration
- `vcpkg.json`: Package dependencies
- `vcpkg-configuration.json`: vcpkg settings
- `.github/workflows/c-cpp.yml`: CI/CD pipeline

## Getting Started

1. Clone the repository
2. Install dependencies with vcpkg
3. Configure with CMake: `cmake -B build -DCMAKE_BUILD_TYPE=Release`
4. Build: `cmake --build build --parallel 4`
5. Test: `cd build && ./tests/simple_unit_test`
6. Run: `./build/apps/neo_node_minimal`

## Production Readiness

This is a production-ready Neo N3 compatible blockchain node implementation with:
- ✅ Complete core functionality
- ✅ dBFT consensus mechanism
- ✅ Neo Virtual Machine
- ✅ JSON-RPC server
- ✅ Comprehensive testing
- ✅ CI/CD pipeline
- ✅ Professional documentation
- ✅ Clean project structure
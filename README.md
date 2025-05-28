# Neo N3 C++ Node

A C++ implementation of the Neo N3 blockchain node.

## Overview

This project is a C++ port of the original C# Neo N3 node implementation. It aims to provide a high-performance, cross-platform implementation of the Neo N3 blockchain node.

## Features

- Full Neo N3 blockchain node implementation
- P2P network communication
- Smart contract execution
- Wallet management
- RPC server for external interaction
- Command-line interface for node management
- Plugin system for extensibility

## Project Status

This project is currently in development. See the [roadmap](docs/roadmap.md) for more details.

## Getting Started

### Prerequisites

- C++17 compatible compiler (GCC 8+, Clang 7+, MSVC 2019+)
- CMake 3.15+
- Boost 1.70+
- OpenSSL 1.1.1+
- RocksDB 6.0+
- nlohmann/json 3.9.0+
- spdlog 1.8.0+

### Building from Source

#### Linux/macOS

```bash
# Clone the repository
git clone https://github.com/neo-project/neo-cpp.git
cd neo-cpp

# Create a build directory
mkdir build
cd build

# Configure and build
cmake ..
cmake --build .

# Run tests
ctest
```

#### Windows

```powershell
# Clone the repository
git clone https://github.com/neo-project/neo-cpp.git
cd neo-cpp

# Create a build directory
mkdir build
cd build

# Configure and build
cmake ..
cmake --build . --config Release

# Run tests
ctest -C Release
```

### Running the Node

```bash
# Run the node with default settings
./neo-cli

# Run the node with custom settings
./neo-cli --config=config.json --db-path=data/chain
```

## Documentation

- [Architecture](docs/architecture.md): Overview of the Neo N3 C++ node architecture
- [Roadmap](docs/roadmap.md): Development roadmap
- [C++ Implementation](docs/cpp_implementation.md): C++ implementation details
- [C# vs C++ Differences](docs/csharp_cpp_differences.md): Differences between the C# and C++ implementations
- [Project Setup](docs/project_setup.md): Project setup details
- [Implementation Steps](docs/implementation_steps.md): Step-by-step implementation process

### Module Documentation

- [Cryptography](docs/modules/cryptography.md): Cryptographic operations
- [IO](docs/modules/io.md): Input/output operations
- [Persistence](docs/modules/persistence.md): Data persistence
- [Network](docs/modules/network.md): Network communication
- [Ledger](docs/modules/ledger.md): Blockchain ledger
- [VM](docs/modules/vm.md): Virtual machine
- [Smart Contract](docs/modules/smartcontract.md): Smart contract execution
- [CLI](docs/modules/cli.md): Command-line interface
- [RPC](docs/modules/rpc.md): Remote procedure call server
- [Plugins](docs/modules/plugins.md): Plugin system
- [Wallets](docs/modules/wallets.md): Wallet management

## Contributing

We welcome contributions to the Neo N3 C++ node project. Please see [CONTRIBUTING.md](CONTRIBUTING.md) for details on how to contribute.

### Development Workflow

1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Run tests
5. Submit a pull request

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## Acknowledgments

- The Neo Project team for the original C# implementation
- All contributors to the Neo ecosystem

## Contact

- Neo Website: [neo.org](https://neo.org/)
- Neo Discord: [discord.gg/neo](https://discord.gg/neo)
- Neo Twitter: [@neo_blockchain](https://twitter.com/neo_blockchain)

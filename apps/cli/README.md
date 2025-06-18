# Neo C++ CLI

A complete command-line interface for the Neo blockchain written in C++. This CLI provides full node functionality including network connectivity, block synchronization, wallet management, and blockchain operations.

## Features

### Core Functionality
- **Full Node Operation**: Complete Neo blockchain node with P2P networking
- **Network Synchronization**: Automatic block and header synchronization from the Neo network
- **RPC Server**: Built-in JSON-RPC server for external integrations
- **Wallet Management**: Create, open, and manage Neo wallets
- **Transaction Operations**: Send transactions, transfer assets, and manage balances
- **Blockchain Queries**: Query blocks, transactions, and network state

### Network Capabilities
- **P2P Connectivity**: Connect to Neo mainnet/testnet
- **Block Synchronization**: Real-time synchronization with the network
- **Peer Management**: Automatic peer discovery and connection management
- **Memory Pool**: Transaction pool management and relay

### Wallet Features
- **NEP-6 Wallet Support**: Standard Neo wallet format
- **Multi-Account**: Support for multiple accounts per wallet
- **Asset Management**: NEO and GAS token support
- **Transaction Creation**: Create and sign transactions
- **Balance Queries**: Check account balances

## Installation

### Prerequisites
- CMake 3.20 or higher
- C++20 compatible compiler (GCC 10+, Clang 12+, MSVC 2019+)
- vcpkg package manager
- Required dependencies (automatically installed via vcpkg):
  - Boost
  - OpenSSL
  - RocksDB
  - nlohmann/json
  - fmt
  - spdlog

### Build Instructions

1. **Clone the repository:**
   ```bash
   git clone <repository-url>
   cd csahrp_cpp
   ```

2. **Initialize vcpkg (if not already done):**
   ```bash
   git submodule update --init --recursive
   ./vcpkg/bootstrap-vcpkg.sh  # Linux/macOS
   ./vcpkg/bootstrap-vcpkg.bat # Windows
   ```

3. **Build the project:**
   ```bash
   mkdir build
   cd build
   cmake .. -DCMAKE_TOOLCHAIN_FILE=../vcpkg/scripts/buildsystems/vcpkg.cmake
   cmake --build . --config Release
   ```

4. **Install (optional):**
   ```bash
   cmake --install . --prefix /usr/local  # Linux/macOS
   ```

## Usage

### Starting the CLI

#### Interactive Mode
```bash
./neo-cli
```

#### Command Line Mode
```bash
./neo-cli --config config.json --wallet wallet.json
```

### Command Line Options

- `--config <file>`: Specify configuration file (default: config.json)
- `--wallet <file>`: Auto-open wallet file
- `--password <pass>`: Wallet password (use with caution)
- `--db-engine <engine>`: Database engine (RocksDB, LevelDB)
- `--db-path <path>`: Database storage path
- `--noverify`: Skip block verification (faster sync, less secure)
- `--verbose <level>`: Logging verbosity (0-5)

### Available Commands

#### Base Commands
- `help [category]`: Show help information
- `version`: Show version information
- `clear`: Clear the screen
- `exit`: Exit the CLI

#### Blockchain Commands
- `showblock <index|hash>`: Display block information
- `showheader <index|hash>`: Display block header information
- `showtx <hash>`: Display transaction information

#### Node Commands
- `showstate`: Display current node state and synchronization status
- `showpool`: Display memory pool transactions
- `showpeers`: Display connected peers

#### Wallet Commands
- `openwallet <path> [password]`: Open a wallet file
- `closewallet`: Close the current wallet
- `showbalance`: Display wallet balances
- `showaddress`: Display wallet addresses
- `transfer <asset> <address> <amount>`: Transfer assets

### Configuration

The CLI uses a JSON configuration file (default: `config.json`) with the following structure:

```json
{
    "Protocol": {
        "Network": 860833102,
        "MillisecondsPerBlock": 15000,
        "SeedList": [
            "seed1.neo.org:10333",
            "seed2.neo.org:10333"
        ]
    },
    "Storage": {
        "Engine": "RocksDB",
        "Path": "data"
    },
    "P2P": {
        "Port": 10333,
        "MaxConnections": 40
    },
    "RPC": {
        "Enabled": true,
        "Port": 10332,
        "BindAddress": "127.0.0.1"
    }
}
```

### Network Connectivity

The CLI automatically connects to the Neo network and begins synchronization:

1. **Peer Discovery**: Connects to seed nodes and discovers peers
2. **Header Sync**: Downloads and validates block headers
3. **Block Sync**: Downloads and validates full blocks
4. **Real-time Updates**: Receives new blocks and transactions

Monitor synchronization status with:
```
neo> showstate
```

### Wallet Operations

#### Creating a New Wallet
```bash
# Interactive wallet creation
neo> createwallet wallet.json

# Command line wallet creation
./neo-cli --wallet new_wallet.json
```

#### Opening an Existing Wallet
```bash
# Interactive
neo> openwallet wallet.json
Password: ********

# Command line
./neo-cli --wallet wallet.json --password mypassword
```

#### Checking Balances
```bash
neo> showbalance
Account: NXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXx
  NEO: 100
  GAS: 1500.50
```

#### Transferring Assets
```bash
# Transfer NEO
neo> transfer neo NXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXx 10

# Transfer GAS
neo> transfer gas NXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXx 100.5
```

### RPC Server

When enabled, the CLI runs a JSON-RPC server compatible with Neo's standard RPC interface:

- **Endpoint**: `http://localhost:10332`
- **Methods**: Standard Neo RPC methods (getblock, gettransaction, etc.)
- **Authentication**: Optional (configurable)

Example RPC call:
```bash
curl -X POST http://localhost:10332 \
  -H "Content-Type: application/json" \
  -d '{"jsonrpc":"2.0","method":"getblockcount","params":[],"id":1}'
```

## Architecture

### Components
- **MainService**: Core CLI service and command dispatcher
- **NeoSystem**: Complete Neo node implementation
- **NetworkSynchronizer**: P2P networking and block synchronization
- **Blockchain**: Block and transaction storage and validation
- **Wallet**: Account and transaction management
- **RPC Server**: JSON-RPC interface

### Threading Model
- **Main Thread**: CLI interface and user interaction
- **Network Thread**: P2P communication and message handling
- **Sync Thread**: Block synchronization and validation
- **RPC Thread**: HTTP server for RPC requests

## Development

### Adding New Commands

1. **Register the command** in `MainService::InitializeCommands()`:
   ```cpp
   RegisterCommand("mycommand", [this](const std::vector<std::string>& args) {
       return HandleMyCommand(args);
   }, "Category");
   ```

2. **Implement the handler**:
   ```cpp
   bool MainService::HandleMyCommand(const std::vector<std::string>& args)
   {
       // Command implementation
       return true;
   }
   ```

### Extending Functionality

The CLI is designed to be extensible:
- **Plugin System**: Add custom functionality via plugins
- **Custom Commands**: Register application-specific commands
- **Event Handlers**: React to blockchain events
- **Custom RPC Methods**: Extend the RPC interface

## Troubleshooting

### Common Issues

#### Synchronization Problems
- Check network connectivity
- Verify seed nodes are accessible
- Check firewall settings for P2P port (10333)

#### Database Issues
- Ensure sufficient disk space
- Check database path permissions
- Try different database engine (LevelDB vs RocksDB)

#### Wallet Problems
- Verify wallet file format (NEP-6)
- Check password correctness
- Ensure wallet file permissions

### Logging

Enable verbose logging for debugging:
```bash
./neo-cli --verbose 5
```

Log levels:
- 0: Errors only
- 1: Warnings and errors
- 2: Info, warnings, and errors
- 3: Debug information
- 4: Trace information
- 5: All messages

## Contributing

1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Add tests if applicable
5. Submit a pull request

## License

This project is licensed under the MIT License - see the LICENSE file for details.

## Support

For support and questions:
- GitHub Issues: Report bugs and feature requests
- Documentation: Check the docs/ directory
- Community: Join the Neo developer community

## Acknowledgments

- Neo Project team for the original C# implementation
- Contributors to the C++ port
- Open source libraries used in this project 
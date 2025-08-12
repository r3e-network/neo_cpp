# Neo C++ SDK

A comprehensive C++ SDK for building applications on the Neo blockchain, leveraging the robust Neo C++ node implementation.

## Features

- 🔐 **Wallet Management** - Create, import, and manage NEP-6 wallets
- 💰 **Transaction Building** - Intuitive transaction construction and signing
- 📜 **Smart Contracts** - Deploy and invoke smart contracts
- 🌐 **RPC Client** - Full-featured RPC client for node communication
- 🔗 **P2P Networking** - Direct P2P network interaction
- 🔑 **Cryptography** - Comprehensive cryptographic operations
- 💾 **Storage** - Local blockchain data management
- 🎯 **NEP Standards** - Built-in support for NEP-17 tokens

## Installation

### Requirements

- C++17 or later
- CMake 3.16+
- Boost 1.70+
- OpenSSL 1.1.1+
- RocksDB (optional, for storage)

### Building from Source

```bash
# Clone the repository
git clone https://github.com/neo-project/neo-cpp.git
cd neo-cpp/sdk

# Create build directory
mkdir build && cd build

# Configure
cmake .. -DCMAKE_BUILD_TYPE=Release

# Build
make -j$(nproc)

# Install (optional)
sudo make install
```

### Using CMake

Add to your `CMakeLists.txt`:

```cmake
find_package(neo-sdk REQUIRED)
target_link_libraries(your_app neo::sdk)
```

## Quick Start

### Create a Wallet

```cpp
#include <neo/sdk.h>

using namespace neo::sdk;

int main() {
    // Initialize SDK
    neo::sdk::Initialize();
    
    // Create a new wallet
    auto wallet = wallet::Wallet::Create("mywallet.json", "password123");
    
    // Create an account
    auto account = wallet->CreateAccount("Main Account");
    std::cout << "Address: " << account.GetAddress() << std::endl;
    
    // Save wallet
    wallet->Save();
    
    return 0;
}
```

### Send NEO

```cpp
#include <neo/sdk.h>

using namespace neo::sdk;

int main() {
    // Open wallet
    auto wallet = wallet::Wallet::Open("mywallet.json", "password123");
    
    // Connect to TestNet
    rpc::RpcClient client("http://seed1.neo.org:20332");
    
    // Build transaction
    auto tx = tx::TransactionBuilder()
        .Transfer(
            wallet->GetDefaultAccount().GetScriptHash(),
            core::UInt160::FromAddress("NXV7ZhHiyM1aHXwpVsRZC6BwNFP2jghXAq"),
            "NEO",
            10
        )
        .BuildAndSign(*wallet);
    
    // Send transaction
    auto txid = client.SendRawTransaction(tx->ToHexString());
    std::cout << "Transaction sent: " << txid << std::endl;
    
    return 0;
}
```

### Invoke Smart Contract

```cpp
#include <neo/sdk.h>

using namespace neo::sdk;

int main() {
    // Contract hash
    auto contractHash = core::UInt160::Parse("0xef4073a0f2b305a38ec4050e4d3d28bc40ea63f5");
    
    // Test invoke
    auto result = contract::ContractInvoker::TestInvoke(
        contractHash,
        "balanceOf",
        {
            core::ContractParameter::FromAddress("NXV7ZhHiyM1aHXwpVsRZC6BwNFP2jghXAq")
        }
    );
    
    std::cout << "Balance: " << result.stack[0].ToString() << std::endl;
    std::cout << "Gas consumed: " << result.gasConsumed << std::endl;
    
    return 0;
}
```

## SDK Modules

### Core Module
Basic blockchain types and operations:
- UInt256, UInt160 hash types
- Transaction, Block, Header types
- Blockchain interface

### Wallet Module
NEP-6 wallet management:
- Create/open wallets
- Account management
- Key operations
- Transaction signing

### Transaction Builder
Fluent interface for building transactions:
- Transfer assets
- Invoke contracts
- Set fees and attributes
- Multi-operation support

### Smart Contract Module
Contract deployment and invocation:
- Deploy contracts
- Test invocations
- Real invocations
- NEP-17 token support

### RPC Client
Communication with Neo nodes:
- Query blockchain data
- Send transactions
- Invoke functions
- Custom RPC calls

### Network Module
P2P network interaction:
- Connect to peers
- Broadcast transactions
- Subscribe to events
- Network presets (MainNet, TestNet)

### Cryptography Module
Cryptographic operations:
- Key pair generation
- Signing and verification
- Hashing functions
- Address generation

### Storage Module
Local blockchain storage:
- Store blocks and transactions
- Query blockchain data
- State management

## Examples

The `examples/` directory contains complete working examples:

- `wallet_example.cpp` - Wallet operations
- `transaction_example.cpp` - Transaction building and sending
- `contract_example.cpp` - Smart contract interaction
- `rpc_example.cpp` - RPC client usage
- `network_example.cpp` - P2P networking

Build examples:

```bash
cmake .. -DNEO_SDK_BUILD_EXAMPLES=ON
make
./examples/wallet_example
```

## API Documentation

Generate API documentation:

```bash
cmake .. -DNEO_SDK_BUILD_DOCS=ON
make neo-sdk-docs
```

View documentation at `build/docs/html/index.html`

## Testing

Run tests:

```bash
cmake .. -DNEO_SDK_BUILD_TESTS=ON
make
ctest
```

## Network Configuration

### MainNet
```cpp
rpc::RpcClient client("http://seed1.neo.org:10332");
```

### TestNet
```cpp
rpc::RpcClient client("http://seed1.neo.org:20332");
```

### Private Net
```cpp
network::NetworkConfig config;
config.magic = 0x4E454F4E;
config.seedList = {"127.0.0.1:20333"};
network::NetworkClient client(config);
```

## Contributing

We welcome contributions! Please see [CONTRIBUTING.md](CONTRIBUTING.md) for details.

### Development Setup

1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Add tests
5. Update documentation
6. Submit a pull request

## License

This project is licensed under the MIT License - see [LICENSE](LICENSE) for details.

## Support

- 📖 [Documentation](https://docs.neo.org)
- 💬 [Discord](https://discord.gg/neo)
- 🐛 [Issue Tracker](https://github.com/neo-project/neo-cpp/issues)
- 📧 [Email](mailto:dev@neo.org)

## Acknowledgments

Built on top of the Neo C++ node implementation, leveraging years of development and testing.

## Version History

- **1.0.0** - Initial release with core functionality
- **0.9.0** - Beta release for testing
- **0.1.0** - Alpha release

---

Made with ❤️ by the Neo community
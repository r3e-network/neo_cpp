# Neo C++ SDK Design Document

## Executive Summary

The Neo C++ SDK provides a comprehensive, easy-to-use library for developers to build applications on the Neo blockchain using C++. By leveraging the existing Neo C++ node codebase, we extract and organize core functionality into a modular, well-documented SDK that enables developers to:

- Connect to Neo networks (MainNet, TestNet, Private)
- Create and manage wallets
- Build and send transactions
- Deploy and invoke smart contracts
- Query blockchain data
- Implement custom Neo services

## Architecture Overview

```
┌─────────────────────────────────────────────────────┐
│                   Neo C++ SDK                        │
├─────────────────────────────────────────────────────┤
│                  High-Level APIs                     │
├─────────────────────────────────────────────────────┤
│   Wallet  │  Transaction  │  Contract  │   RPC      │
│   Module  │    Builder    │   Module   │  Client    │
├───────────┴────────────────┴───────────┴────────────┤
│                   Core Services                      │
├─────────────────────────────────────────────────────┤
│  Crypto   │   Network    │  Storage   │  Consensus  │
│  Service  │   Service    │  Service   │   Service   │
├───────────┴──────────────┴────────────┴─────────────┤
│              Low-Level Components                    │
├─────────────────────────────────────────────────────┤
│    VM     │   P2P        │  Ledger    │   Utils     │
│  Engine   │  Protocol    │   Core     │  & Helpers  │
└───────────┴──────────────┴────────────┴─────────────┘
```

## SDK Modules

### 1. Core Module (`neo::sdk::core`)

**Purpose**: Fundamental blockchain types and operations

```cpp
namespace neo::sdk::core {
    // Basic types from existing codebase
    using UInt256 = neo::io::UInt256;
    using UInt160 = neo::io::UInt160;
    using ECPoint = neo::cryptography::ECPoint;
    
    // Transaction types
    using Transaction = neo::core::Transaction;
    using Block = neo::ledger::Block;
    using Header = neo::ledger::Header;
    
    // High-level blockchain interface
    class Blockchain {
    public:
        static std::shared_ptr<Block> GetBlock(const UInt256& hash);
        static std::shared_ptr<Block> GetBlock(uint32_t height);
        static std::shared_ptr<Transaction> GetTransaction(const UInt256& hash);
        static uint32_t GetCurrentHeight();
        static std::shared_ptr<Header> GetHeader(uint32_t height);
    };
}
```

### 2. Wallet Module (`neo::sdk::wallet`)

**Purpose**: Wallet management and key operations

```cpp
namespace neo::sdk::wallet {
    // Leverage existing wallet implementation
    using Account = neo::wallets::Account;
    using NEP6Wallet = neo::wallets::NEP6Wallet;
    
    // Simplified wallet interface
    class Wallet {
    private:
        std::unique_ptr<NEP6Wallet> wallet_;
        
    public:
        // Creation and loading
        static std::shared_ptr<Wallet> Create(const std::string& path, const std::string& password);
        static std::shared_ptr<Wallet> Open(const std::string& path, const std::string& password);
        
        // Account management
        Account CreateAccount();
        Account ImportAccount(const std::string& wif);
        std::vector<Account> GetAccounts() const;
        
        // Balance queries
        uint64_t GetBalance(const std::string& asset);
        std::vector<UTXO> GetUTXOs(const UInt160& scriptHash);
        
        // Signing
        std::vector<uint8_t> Sign(const std::vector<uint8_t>& message, const Account& account);
    };
}
```

### 3. Transaction Builder (`neo::sdk::tx`)

**Purpose**: Simplified transaction construction

```cpp
namespace neo::sdk::tx {
    class TransactionBuilder {
    private:
        std::unique_ptr<neo::core::Transaction> tx_;
        
    public:
        TransactionBuilder();
        
        // Fluid interface for building transactions
        TransactionBuilder& SetSender(const UInt160& sender);
        TransactionBuilder& SetSystemFee(uint64_t fee);
        TransactionBuilder& SetNetworkFee(uint64_t fee);
        TransactionBuilder& SetValidUntilBlock(uint32_t block);
        TransactionBuilder& AddAttribute(const TransactionAttribute& attr);
        TransactionBuilder& AddWitness(const Witness& witness);
        
        // Contract invocation
        TransactionBuilder& InvokeContract(
            const UInt160& scriptHash,
            const std::string& method,
            const std::vector<ContractParameter>& params
        );
        
        // Asset transfers
        TransactionBuilder& Transfer(
            const UInt160& from,
            const UInt160& to,
            const std::string& asset,
            uint64_t amount
        );
        
        // Build and sign
        std::shared_ptr<Transaction> Build();
        std::shared_ptr<Transaction> BuildAndSign(const Wallet& wallet);
    };
}
```

### 4. Smart Contract Module (`neo::sdk::contract`)

**Purpose**: Smart contract deployment and invocation

```cpp
namespace neo::sdk::contract {
    // Contract deployment
    class ContractDeployer {
    public:
        struct DeploymentOptions {
            std::vector<uint8_t> script;
            std::string manifest;
            uint32_t storageFee = 0;
            std::vector<ContractParameter> data;
        };
        
        static std::shared_ptr<Transaction> Deploy(
            const DeploymentOptions& options,
            const Wallet& wallet
        );
    };
    
    // Contract invocation
    class ContractInvoker {
    public:
        // Test invocation (no blockchain state change)
        static InvocationResult TestInvoke(
            const UInt160& scriptHash,
            const std::string& method,
            const std::vector<ContractParameter>& params
        );
        
        // Real invocation (creates transaction)
        static std::shared_ptr<Transaction> Invoke(
            const UInt160& scriptHash,
            const std::string& method,
            const std::vector<ContractParameter>& params,
            const Wallet& wallet
        );
    };
    
    // NEP standards support
    class NEP17Token {
    private:
        UInt160 contractHash_;
        
    public:
        NEP17Token(const UInt160& contractHash);
        
        std::string Symbol();
        uint8_t Decimals();
        uint64_t TotalSupply();
        uint64_t BalanceOf(const UInt160& account);
        
        std::shared_ptr<Transaction> Transfer(
            const UInt160& from,
            const UInt160& to,
            uint64_t amount,
            const Wallet& wallet
        );
    };
}
```

### 5. RPC Client (`neo::sdk::rpc`)

**Purpose**: Communication with Neo nodes

```cpp
namespace neo::sdk::rpc {
    class RpcClient {
    private:
        std::string endpoint_;
        std::unique_ptr<neo::rpc::RpcClient> client_;
        
    public:
        RpcClient(const std::string& endpoint);
        
        // Node information
        std::string GetVersion();
        uint32_t GetBlockCount();
        std::string GetBestBlockHash();
        
        // Block queries
        json GetBlock(const std::string& hash, bool verbose = true);
        json GetBlockHeader(const std::string& hash, bool verbose = true);
        
        // Transaction operations
        json GetRawTransaction(const std::string& txid, bool verbose = true);
        std::string SendRawTransaction(const std::string& hex);
        
        // Contract operations
        json InvokeFunction(
            const std::string& scriptHash,
            const std::string& method,
            const std::vector<json>& params
        );
        
        // State queries
        json GetNep17Balances(const std::string& address);
        json GetStorage(const std::string& contractHash, const std::string& key);
        
        // Custom RPC calls
        json Call(const std::string& method, const std::vector<json>& params);
    };
}
```

### 6. Network Module (`neo::sdk::network`)

**Purpose**: P2P network interaction

```cpp
namespace neo::sdk::network {
    class NetworkClient {
    private:
        std::unique_ptr<neo::network::P2PServer> p2p_;
        
    public:
        NetworkClient(const NetworkConfig& config);
        
        // Connection management
        void Connect(const std::string& host, uint16_t port);
        void Disconnect();
        std::vector<PeerInfo> GetConnectedPeers();
        
        // Message handling
        void Broadcast(const Transaction& tx);
        void SubscribeToBlocks(std::function<void(const Block&)> callback);
        void SubscribeToTransactions(std::function<void(const Transaction&)> callback);
    };
    
    // Network presets
    struct Networks {
        static const NetworkConfig MainNet;
        static const NetworkConfig TestNet;
        static const NetworkConfig PrivateNet;
    };
}
```

### 7. Cryptography Module (`neo::sdk::crypto`)

**Purpose**: Cryptographic operations

```cpp
namespace neo::sdk::crypto {
    // Key management
    class KeyPair {
    private:
        neo::cryptography::KeyPair keypair_;
        
    public:
        static KeyPair Generate();
        static KeyPair FromWIF(const std::string& wif);
        
        std::string GetWIF() const;
        ECPoint GetPublicKey() const;
        std::vector<uint8_t> GetPrivateKey() const;
        UInt160 GetScriptHash() const;
        std::string GetAddress() const;
        
        std::vector<uint8_t> Sign(const std::vector<uint8_t>& message) const;
        bool Verify(const std::vector<uint8_t>& message, const std::vector<uint8_t>& signature) const;
    };
    
    // Hashing utilities
    class Hash {
    public:
        static UInt256 SHA256(const std::vector<uint8_t>& data);
        static UInt160 Hash160(const std::vector<uint8_t>& data);
        static UInt256 Hash256(const std::vector<uint8_t>& data);
    };
}
```

### 8. Storage Module (`neo::sdk::storage`)

**Purpose**: Local blockchain data storage

```cpp
namespace neo::sdk::storage {
    class BlockchainStorage {
    private:
        std::unique_ptr<neo::persistence::Store> store_;
        
    public:
        BlockchainStorage(const std::string& path);
        
        // Block storage
        void StoreBlock(const Block& block);
        std::shared_ptr<Block> GetBlock(const UInt256& hash);
        std::shared_ptr<Block> GetBlock(uint32_t height);
        
        // Transaction storage
        void StoreTransaction(const Transaction& tx);
        std::shared_ptr<Transaction> GetTransaction(const UInt256& hash);
        
        // State management
        void UpdateState(const StateUpdate& update);
        std::vector<uint8_t> GetState(const StorageKey& key);
    };
}
```

## Usage Examples

### Example 1: Creating a Wallet and Checking Balance

```cpp
#include <neo/sdk.h>

int main() {
    using namespace neo::sdk;
    
    // Create a new wallet
    auto wallet = wallet::Wallet::Create("mywallet.json", "password123");
    
    // Create a new account
    auto account = wallet->CreateAccount();
    std::cout << "Address: " << account.GetAddress() << std::endl;
    
    // Connect to TestNet
    rpc::RpcClient client("http://seed1.neo.org:20332");
    
    // Check NEO balance
    auto balances = client.GetNep17Balances(account.GetAddress());
    std::cout << "Balance: " << balances["NEO"] << std::endl;
    
    return 0;
}
```

### Example 2: Sending NEO

```cpp
#include <neo/sdk.h>

int main() {
    using namespace neo::sdk;
    
    // Open existing wallet
    auto wallet = wallet::Wallet::Open("mywallet.json", "password123");
    
    // Build transfer transaction
    auto tx = tx::TransactionBuilder()
        .Transfer(
            wallet->GetAccounts()[0].GetScriptHash(),
            UInt160::Parse("NXV7ZhHiyM1aHXwpVsRZC6BwNFP2jghXAq"),
            "NEO",
            100
        )
        .BuildAndSign(*wallet);
    
    // Send transaction
    rpc::RpcClient client("http://seed1.neo.org:20332");
    auto txid = client.SendRawTransaction(tx->ToHexString());
    std::cout << "Transaction sent: " << txid << std::endl;
    
    return 0;
}
```

### Example 3: Deploying a Smart Contract

```cpp
#include <neo/sdk.h>

int main() {
    using namespace neo::sdk;
    
    // Open wallet
    auto wallet = wallet::Wallet::Open("mywallet.json", "password123");
    
    // Read contract files
    auto nef = ReadFile("contract.nef");
    auto manifest = ReadFile("contract.manifest.json");
    
    // Deploy contract
    contract::ContractDeployer::DeploymentOptions options;
    options.script = nef;
    options.manifest = manifest;
    
    auto tx = contract::ContractDeployer::Deploy(options, *wallet);
    
    // Send deployment transaction
    rpc::RpcClient client("http://seed1.neo.org:20332");
    auto txid = client.SendRawTransaction(tx->ToHexString());
    std::cout << "Contract deployed: " << txid << std::endl;
    
    return 0;
}
```

### Example 4: Invoking a Smart Contract

```cpp
#include <neo/sdk.h>

int main() {
    using namespace neo::sdk;
    
    // Contract hash
    auto contractHash = UInt160::Parse("0x1234567890abcdef1234567890abcdef12345678");
    
    // Test invoke (no gas consumed)
    auto result = contract::ContractInvoker::TestInvoke(
        contractHash,
        "transfer",
        {
            ContractParameter::FromAddress("NXV7ZhHiyM1aHXwpVsRZC6BwNFP2jghXAq"),
            ContractParameter::FromAddress("NZs2zXSPuuv9ZF6TDGSWT1RBmE8rfGj7UW"),
            ContractParameter::FromInteger(100),
            ContractParameter::Null()
        }
    );
    
    std::cout << "Gas consumed: " << result.gasConsumed << std::endl;
    std::cout << "Result: " << result.stack[0].ToString() << std::endl;
    
    // Real invoke (creates transaction)
    auto wallet = wallet::Wallet::Open("mywallet.json", "password123");
    auto tx = contract::ContractInvoker::Invoke(
        contractHash,
        "transfer",
        { /* parameters */ },
        *wallet
    );
    
    return 0;
}
```

### Example 5: Subscribing to Blockchain Events

```cpp
#include <neo/sdk.h>

int main() {
    using namespace neo::sdk;
    
    // Create network client
    network::NetworkClient client(network::Networks::TestNet);
    
    // Subscribe to new blocks
    client.SubscribeToBlocks([](const core::Block& block) {
        std::cout << "New block: " << block.GetHash().ToString() 
                  << " Height: " << block.Index << std::endl;
    });
    
    // Subscribe to new transactions
    client.SubscribeToTransactions([](const core::Transaction& tx) {
        std::cout << "New transaction: " << tx.GetHash().ToString() << std::endl;
    });
    
    // Connect to network
    client.Connect("seed1.neo.org", 20333);
    
    // Keep running
    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    
    return 0;
}
```

## SDK Package Structure

```
neo-cpp-sdk/
├── include/
│   └── neo/
│       ├── sdk.h                 # Main header file
│       ├── core/
│       │   ├── blockchain.h
│       │   ├── transaction.h
│       │   └── block.h
│       ├── wallet/
│       │   ├── wallet.h
│       │   └── account.h
│       ├── tx/
│       │   └── builder.h
│       ├── contract/
│       │   ├── deployer.h
│       │   ├── invoker.h
│       │   └── nep17.h
│       ├── rpc/
│       │   └── client.h
│       ├── network/
│       │   └── client.h
│       ├── crypto/
│       │   ├── keypair.h
│       │   └── hash.h
│       └── storage/
│           └── storage.h
├── lib/
│   ├── libneo-sdk.a             # Static library
│   └── libneo-sdk.so            # Shared library
├── examples/
│   ├── wallet_example.cpp
│   ├── transaction_example.cpp
│   ├── contract_example.cpp
│   └── CMakeLists.txt
├── docs/
│   ├── getting_started.md
│   ├── api_reference.md
│   └── tutorials/
│       ├── create_wallet.md
│       ├── send_assets.md
│       └── deploy_contract.md
├── cmake/
│   └── neo-sdk-config.cmake
├── CMakeLists.txt
└── README.md
```

## Build Configuration

### CMakeLists.txt for SDK

```cmake
cmake_minimum_required(VERSION 3.16)
project(neo-cpp-sdk VERSION 1.0.0)

# C++ Standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Options
option(BUILD_SHARED_LIBS "Build shared library" ON)
option(BUILD_EXAMPLES "Build example programs" ON)
option(BUILD_TESTS "Build unit tests" ON)

# Find dependencies (from existing node)
find_package(Boost REQUIRED COMPONENTS system filesystem thread)
find_package(OpenSSL REQUIRED)
find_package(RocksDB REQUIRED)

# SDK library sources
set(SDK_SOURCES
    src/core/blockchain.cpp
    src/wallet/wallet.cpp
    src/tx/builder.cpp
    src/contract/deployer.cpp
    src/contract/invoker.cpp
    src/rpc/client.cpp
    src/network/client.cpp
    src/crypto/keypair.cpp
    src/storage/storage.cpp
)

# Create library
add_library(neo-sdk ${SDK_SOURCES})

# Include directories
target_include_directories(neo-sdk
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# Link dependencies
target_link_libraries(neo-sdk
    PUBLIC
        neo_cpp  # Link to existing node library
        Boost::system
        Boost::filesystem
        OpenSSL::SSL
        OpenSSL::Crypto
)

# Installation
install(TARGETS neo-sdk
    EXPORT neo-sdk-targets
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
)

install(DIRECTORY include/neo
    DESTINATION include
)

# Package configuration
install(EXPORT neo-sdk-targets
    FILE neo-sdk-targets.cmake
    NAMESPACE neo::
    DESTINATION lib/cmake/neo-sdk
)

# Examples
if(BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

# Tests
if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
```

## Integration with Existing Codebase

### Extraction Strategy

1. **Minimal Modification**: Keep existing node code intact, create SDK as a layer on top
2. **Selective Export**: Only expose stable, well-tested APIs
3. **Abstraction Layer**: Add simplified interfaces that hide complexity
4. **Backward Compatibility**: Ensure SDK updates don't break existing code

### Implementation Plan

1. **Phase 1: Core Infrastructure** (Week 1-2)
   - Extract basic types and utilities
   - Create SDK project structure
   - Set up build system

2. **Phase 2: Essential Modules** (Week 3-4)
   - Implement Wallet module
   - Implement Transaction Builder
   - Implement RPC Client

3. **Phase 3: Advanced Features** (Week 5-6)
   - Smart Contract module
   - Network module
   - Storage module

4. **Phase 4: Polish & Documentation** (Week 7-8)
   - Complete API documentation
   - Create comprehensive examples
   - Write tutorials
   - Package for distribution

## Testing Strategy

### Unit Tests
- Test each module independently
- Mock external dependencies
- Achieve >90% code coverage

### Integration Tests
- Test module interactions
- Use TestNet for real transactions
- Validate against Neo reference implementation

### Example Test

```cpp
#include <gtest/gtest.h>
#include <neo/sdk.h>

TEST(WalletTest, CreateAccount) {
    auto wallet = neo::sdk::wallet::Wallet::Create("test.json", "password");
    auto account = wallet->CreateAccount();
    
    EXPECT_FALSE(account.GetAddress().empty());
    EXPECT_EQ(account.GetAddress().substr(0, 1), "N");
}

TEST(TransactionTest, BuildTransfer) {
    neo::sdk::tx::TransactionBuilder builder;
    auto tx = builder
        .SetSystemFee(100000)
        .SetNetworkFee(100000)
        .Build();
    
    EXPECT_NE(tx, nullptr);
    EXPECT_EQ(tx->GetSystemFee(), 100000);
}
```

## Documentation Requirements

### API Documentation
- Doxygen comments for all public APIs
- Clear parameter descriptions
- Usage examples in comments
- Return value documentation

### User Guide
- Getting started guide
- Installation instructions
- Basic usage tutorials
- Advanced topics

### Reference Documentation
- Complete API reference
- Architecture overview
- Best practices
- Troubleshooting guide

## Conclusion

This SDK design leverages the robust Neo C++ node implementation to provide developers with a powerful, easy-to-use library for building Neo blockchain applications. By extracting and organizing existing functionality into well-defined modules, we create a professional SDK that maintains compatibility with the Neo ecosystem while providing a superior developer experience.

The modular architecture ensures that developers can use only the components they need, while the comprehensive examples and documentation make it easy to get started quickly. The SDK will significantly lower the barrier to entry for C++ developers wanting to build on Neo.
# Neo N3 C++ API Reference

## Overview

This document provides a comprehensive API reference for the Neo N3 C++ implementation, covering all major modules and their public interfaces.

## Core Modules

### Virtual Machine (VM)

#### ExecutionEngine
```cpp
namespace neo::vm {
    class ExecutionEngine {
    public:
        ExecutionEngine();
        
        // Execution control
        VMState Execute();
        void StepInto();
        void StepOut();
        void StepOver();
        
        // Stack operations
        void Push(const StackItem& item);
        StackItem Pop();
        StackItem Peek(int index = 0);
        
        // Script management
        void LoadScript(std::span<const uint8_t> script);
        void LoadScript(const Script& script);
        
        // State management
        VMState GetState() const;
        void Reset();
        
        // Properties
        int GetInstructionPointer() const;
        std::span<const uint8_t> GetCurrentInstruction() const;
        const EvaluationStack& GetEvaluationStack() const;
    };
}
```

#### StackItem Types
```cpp
namespace neo::vm {
    class StackItem {
    public:
        virtual StackItemType GetType() const = 0;
        virtual bool GetBoolean() const;
        virtual BigInteger GetInteger() const;
        virtual std::vector<uint8_t> GetSpan() const;
        virtual std::string GetString() const;
    };
    
    class Integer : public StackItem;
    class Boolean : public StackItem;
    class ByteString : public StackItem;
    class Array : public StackItem;
    class Map : public StackItem;
    class Struct : public StackItem;
    class Pointer : public StackItem;
    class Buffer : public StackItem;
}
```

### Cryptography

#### Hash Functions
```cpp
namespace neo::cryptography {
    class Hash {
    public:
        static UInt160 Hash160(std::span<const uint8_t> value);
        static UInt256 Hash256(std::span<const uint8_t> value);
        static UInt256 Sha256(std::span<const uint8_t> value);
        static UInt160 Ripemd160(std::span<const uint8_t> value);
        static std::vector<uint8_t> Murmur3(std::span<const uint8_t> value, uint32_t seed);
    };
}
```

#### Elliptic Curve Cryptography
```cpp
namespace neo::cryptography::ecc {
    class ECPoint {
    public:
        static ECPoint FromBytes(std::span<const uint8_t> data);
        static ECPoint Parse(const std::string& hex);
        static ECPoint Generator();
        
        std::vector<uint8_t> EncodePoint(bool compressed) const;
        bool VerifySignature(std::span<const uint8_t> message, 
                           std::span<const uint8_t> signature) const;
        
        ECPoint Add(const ECPoint& other) const;
        ECPoint Multiply(const BigInteger& scalar) const;
        
        bool IsInfinity() const;
        bool IsValid() const;
    };
    
    class ECDsa {
    public:
        static std::vector<uint8_t> Sign(std::span<const uint8_t> message,
                                       std::span<const uint8_t> private_key);
        static bool Verify(std::span<const uint8_t> message,
                          std::span<const uint8_t> signature,
                          const ECPoint& public_key);
    };
}
```

### Network

#### P2P Protocol
```cpp
namespace neo::network::p2p {
    class LocalNode {
    public:
        void Start(int port = 10333);
        void Stop();
        
        void ConnectToPeer(const std::string& endpoint);
        void DisconnectPeer(const std::string& endpoint);
        
        void Relay(const IInventory& inventory);
        void SendMessage(const Message& message);
        
        std::vector<RemoteNode> GetConnectedPeers() const;
        bool IsRunning() const;
    };
    
    class Message {
    public:
        MessageCommand GetCommand() const;
        std::vector<uint8_t> GetPayload() const;
        uint32_t GetChecksum() const;
        
        static Message Create(MessageCommand command, 
                            std::span<const uint8_t> payload);
    };
}
```

### RPC Server

#### JSON-RPC Interface
```cpp
namespace neo::rpc {
    class RpcServer {
    public:
        RpcServer(const RpcServerSettings& settings);
        
        void Start();
        void Stop();
        
        void RegisterMethod(const std::string& name, RpcMethodHandler handler);
        void UnregisterMethod(const std::string& name);
        
        bool IsRunning() const;
    };
    
    class RpcRequest {
    public:
        std::string GetJsonRpc() const;
        std::string GetMethod() const;
        nlohmann::json GetParams() const;
        nlohmann::json GetId() const;
        
        static RpcRequest FromJson(const nlohmann::json& json);
    };
    
    class RpcResponse {
    public:
        void SetResult(const nlohmann::json& result);
        void SetError(const RpcResponseError& error);
        
        nlohmann::json ToJson() const;
        static RpcResponse FromJson(const nlohmann::json& json);
    };
}
```

### RPC Client

#### Client Interface
```cpp
namespace neo::rpc {
    class RpcClient {
    public:
        RpcClient(const std::string& base_url);
        RpcClient(const std::string& base_url, 
                 const std::string& username, 
                 const std::string& password);
        
        // Blockchain methods
        std::string GetBestBlockHash();
        uint32_t GetBlockCount();
        json::JToken GetBlock(const std::string& hash, bool verbose = true);
        json::JToken GetBlock(uint32_t index, bool verbose = true);
        json::JToken GetTransaction(const std::string& hash, bool verbose = true);
        
        // Transaction methods
        std::string SendRawTransaction(const std::string& hex);
        json::JToken InvokeFunction(const std::string& script_hash,
                                   const std::string& operation,
                                   const std::vector<json::JToken>& params = {});
        
        // Async versions
        std::future<std::string> GetBestBlockHashAsync();
        std::future<uint32_t> GetBlockCountAsync();
        std::future<json::JToken> GetBlockAsync(const std::string& hash, bool verbose = true);
        
        // Utility methods
        json::JToken GetVersion();
        json::JToken RpcSend(const std::string& method, 
                            const std::vector<json::JToken>& params = {});
    };
}
```

### Wallets

#### Wallet Management
```cpp
namespace neo::wallets {
    class Wallet {
    public:
        virtual bool Contains(const UInt160& script_hash) const = 0;
        virtual WalletAccount CreateAccount(std::span<const uint8_t> private_key) = 0;
        virtual WalletAccount CreateAccount(const std::string& script_hash) = 0;
        virtual bool DeleteAccount(const UInt160& script_hash) = 0;
        virtual WalletAccount GetAccount(const UInt160& script_hash) const = 0;
        virtual std::vector<WalletAccount> GetAccounts() const = 0;
        
        virtual bool ChangePassword(const std::string& old_password,
                                   const std::string& new_password) = 0;
        virtual bool VerifyPassword(const std::string& password) const = 0;
        
        virtual Transaction MakeTransaction(std::span<const TransferOutput> outputs,
                                          const UInt160& from = UInt160::Zero()) = 0;
        virtual Transaction Sign(const ContractParametersContext& context) = 0;
    };
    
    class NEP6Wallet : public Wallet {
    public:
        static std::unique_ptr<NEP6Wallet> Create(const std::string& path,
                                                 const std::string& password,
                                                 const ProtocolSettings& settings);
        static std::unique_ptr<NEP6Wallet> Open(const std::string& path,
                                               const std::string& password,
                                               const ProtocolSettings& settings);
        
        void Save();
        void SaveAs(const std::string& path);
    };
}
```

### Smart Contracts

#### Contract Execution
```cpp
namespace neo::smartcontract {
    class ApplicationEngine : public ExecutionEngine {
    public:
        ApplicationEngine(TriggerType trigger,
                         const IVerifiable& container,
                         const DataCache& snapshot,
                         const Block& persistingBlock,
                         const ProtocolSettings& settings,
                         long gas);
        
        VMState Execute();
        
        // Gas management
        long GetGasConsumed() const;
        void AddGas(long gas);
        
        // Storage operations
        StorageItem GetStorageItem(const StorageKey& key);
        void PutStorageItem(const StorageKey& key, const StorageItem& item);
        void DeleteStorageItem(const StorageKey& key);
        
        // Contract management
        ContractState GetContract(const UInt160& hash);
        void DeployContract(const ContractState& contract);
        void UpdateContract(const UInt160& hash, const ContractState& contract);
        void DestroyContract(const UInt160& hash);
        
        // Notifications
        void SendNotification(const UInt160& script_hash,
                            const std::string& event_name,
                            const Array& state);
    };
}
```

### JSON Processing

#### JSON Token System
```cpp
namespace neo::json {
    class JToken {
    public:
        virtual JTokenType GetType() const = 0;
        virtual std::string ToString() const = 0;
        virtual JToken Clone() const = 0;
        
        // Type checking
        bool IsNull() const;
        bool IsBoolean() const;
        bool IsNumber() const;
        bool IsString() const;
        bool IsArray() const;
        bool IsObject() const;
        
        // Value access
        bool AsBoolean() const;
        double AsNumber() const;
        std::string AsString() const;
        
        // Parsing
        static JToken Parse(const std::string& json);
        static JToken Parse(std::istream& stream);
    };
    
    class JObject : public JToken {
    public:
        JToken& operator[](const std::string& key);
        const JToken& operator[](const std::string& key) const;
        
        void Add(const std::string& key, const JToken& value);
        bool Remove(const std::string& key);
        bool ContainsKey(const std::string& key) const;
        
        std::vector<std::string> GetKeys() const;
        size_t Count() const;
    };
    
    class JArray : public JToken {
    public:
        JToken& operator[](size_t index);
        const JToken& operator[](size_t index) const;
        
        void Add(const JToken& item);
        void Insert(size_t index, const JToken& item);
        bool Remove(const JToken& item);
        void RemoveAt(size_t index);
        
        size_t Count() const;
    };
}
```

### Console Service

#### Interactive Console
```cpp
namespace neo::console_service {
    class ConsoleServiceBase {
    public:
        virtual std::string GetServiceName() const = 0;
        virtual std::string GetPrompt() const;
        
        void Run(const std::vector<std::string>& args);
        
        bool GetShowPrompt() const;
        void SetShowPrompt(bool show_prompt);
        
        template<typename T>
        void RegisterCommandHandler(std::function<T(std::vector<std::shared_ptr<CommandToken>>&, bool)> handler);
        
    protected:
        virtual bool OnStart(const std::vector<std::string>& args);
        virtual void OnStop();
        
        bool OnCommand(const std::string& command_line);
        void OnHelpCommand(const std::string& key = "");
    };
    
    class ConsoleHelper {
    public:
        static void Info(const std::string& tag, const std::string& message);
        static void Warning(const std::string& msg);
        static void Error(const std::string& msg);
        
        static std::string ReadUserInput(const std::string& prompt = "", bool password = false);
        static std::string ReadSecureString(const std::string& prompt = "");
        
        static void SetForegroundColor(ConsoleColor color);
        static void SetBackgroundColor(ConsoleColor color);
        static void ResetColor();
        static void Clear();
    };
}
```

## Usage Examples

### Basic VM Execution
```cpp
#include <neo/vm/execution_engine.h>

using namespace neo::vm;

// Create and execute a simple script
ExecutionEngine engine;
std::vector<uint8_t> script = {OpCode::PUSH1, OpCode::PUSH2, OpCode::ADD};
engine.LoadScript(script);

VMState result = engine.Execute();
if (result == VMState::HALT) {
    auto value = engine.Pop();
    std::cout << "Result: " << value.GetInteger() << std::endl; // Output: 3
}
```

### RPC Client Usage
```cpp
#include <neo/rpc/rpc_client.h>

using namespace neo::rpc;

// Connect to a Neo node
RpcClient client("http://localhost:10332");

// Get blockchain information
auto version = client.GetVersion();
auto block_count = client.GetBlockCount();
auto best_hash = client.GetBestBlockHash();

std::cout << "Block count: " << block_count << std::endl;
std::cout << "Best block: " << best_hash << std::endl;
```

### Wallet Operations
```cpp
#include <neo/wallets/nep6_wallet.h>

using namespace neo::wallets;

// Create a new wallet
auto wallet = NEP6Wallet::Create("wallet.json", "password", settings);

// Create an account
auto account = wallet->CreateAccount();
std::cout << "Address: " << account.GetAddress() << std::endl;

// Save the wallet
wallet->Save();
```

This API reference provides the essential interfaces for working with the Neo N3 C++ implementation. For detailed implementation examples and advanced usage, refer to the unit tests and example applications in the repository.

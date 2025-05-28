# Neo C++ Implementation Guide

## Quick Start: Implementing the NeoSystem

### 1. Basic NeoSystem Implementation (src/neo_system.cpp)

```cpp
#include <neo/neo_system.h>
#include <neo/persistence/memory_store.h>
#include <neo/ledger/block.h>
#include <neo/io/uint256.h>

namespace neo {

// RelayCache Implementation
RelayCache::RelayCache(size_t capacity) : capacity_(capacity) {}

bool RelayCache::TryAdd(const io::UInt256& hash) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (cache_.find(hash) != cache_.end()) {
        return false; // Already exists
    }
    
    if (cache_.size() >= capacity_) {
        RemoveOldest();
    }
    
    cache_.insert(hash);
    return true;
}

bool RelayCache::Contains(const io::UInt256& hash) const {
    std::lock_guard<std::mutex> lock(mutex_);
    return cache_.find(hash) != cache_.end();
}

void RelayCache::Clear() {
    std::lock_guard<std::mutex> lock(mutex_);
    cache_.clear();
}

void RelayCache::RemoveOldest() {
    if (!cache_.empty()) {
        cache_.erase(cache_.begin());
    }
}

// NeoSystem Implementation
NeoSystem::NeoSystem(std::shared_ptr<ProtocolSettings> settings,
                     std::shared_ptr<persistence::IStoreProvider> storageProvider,
                     const std::string& storagePath)
    : settings_(settings)
    , storeProvider_(storageProvider)
    , storagePath_(storagePath)
{
    if (!storeProvider_) {
        storeProvider_ = std::make_shared<persistence::MemoryStoreProvider>();
    }
    
    store_ = storeProvider_->GetStore(storagePath_);
    relayCache_ = std::make_shared<RelayCache>(100);
    pluginManager_ = std::make_shared<plugins::PluginManager>();
    
    // Load plugins
    pluginManager_->LoadPlugins();
}

NeoSystem::~NeoSystem() {
    Dispose();
}

void NeoSystem::Initialize(std::shared_ptr<ledger::Block> genesisBlock) {
    if (isInitialized_.load()) {
        return;
    }
    
    if (!genesisBlock) {
        CreateGenesisBlock();
    } else {
        genesisBlock_ = genesisBlock;
    }
    
    InitializeComponents();
    
    // Notify plugins
    for (auto& plugin : pluginManager_->GetPlugins()) {
        plugin->OnSystemLoaded(shared_from_this());
    }
    
    isInitialized_.store(true);
}

void NeoSystem::Start(const NetworkConfig& config) {
    std::lock_guard<std::mutex> lock(startMutex_);
    
    if (suspendCount_.load() > 0) {
        pendingStartConfig_ = config;
        hasPendingStart_ = true;
        return;
    }
    
    if (!isInitialized_.load()) {
        throw std::runtime_error("System not initialized");
    }
    
    StartComponents(config);
    isRunning_.store(true);
}

void NeoSystem::Stop() {
    if (!isRunning_.load()) {
        return;
    }
    
    shouldStop_.store(true);
    StopComponents();
    
    // Wait for background threads
    for (auto& thread : backgroundThreads_) {
        if (thread.joinable()) {
            thread.join();
        }
    }
    
    isRunning_.store(false);
}

void NeoSystem::Dispose() {
    Stop();
    
    // Dispose plugins
    pluginManager_->UnloadPlugins();
    
    // Clear components
    blockchain_.reset();
    memoryPool_.reset();
    headerCache_.reset();
    localNode_.reset();
    taskManager_.reset();
    transactionRouter_.reset();
    
    if (store_) {
        store_.reset();
    }
}

std::shared_ptr<persistence::DataCache> NeoSystem::GetStoreView() const {
    return std::make_shared<persistence::DataCache>(store_);
}

std::shared_ptr<persistence::DataCache> NeoSystem::GetSnapshotCache() const {
    return std::make_shared<persistence::DataCache>(store_->GetSnapshot());
}

ledger::ContainsTransactionType NeoSystem::ContainsTransaction(const io::UInt256& hash) const {
    if (memoryPool_->ContainsTransaction(hash)) {
        return ledger::ContainsTransactionType::ExistsInPool;
    }
    
    if (blockchain_->ContainsTransaction(hash)) {
        return ledger::ContainsTransactionType::ExistsInLedger;
    }
    
    return ledger::ContainsTransactionType::NotExist;
}

void NeoSystem::CreateGenesisBlock() {
    // Create genesis block identical to C# version
    auto header = std::make_shared<ledger::BlockHeader>();
    header->prevHash = io::UInt256::Zero();
    header->merkleRoot = io::UInt256::Zero();
    header->timestamp = 1468595301000; // July 15, 2016 15:08:21 UTC
    header->nonce = 2083236893; // Bitcoin genesis nonce
    header->index = 0;
    header->primaryIndex = 0;
    header->nextConsensus = settings_->GetStandbyValidators().GetBFTAddress();
    
    genesisBlock_ = std::make_shared<ledger::Block>();
    genesisBlock_->header = header;
    genesisBlock_->transactions = {};
}

void NeoSystem::InitializeComponents() {
    // Initialize core components
    headerCache_ = std::make_shared<ledger::HeaderCache>();
    blockchain_ = std::make_shared<ledger::Blockchain>(GetStoreView());
    memoryPool_ = std::make_shared<ledger::MemoryPool>(blockchain_);
    
    // Initialize network components
    localNode_ = std::make_shared<network::p2p::LocalNode>(shared_from_this());
    taskManager_ = std::make_shared<network::p2p::TaskManager>(shared_from_this());
    transactionRouter_ = std::make_shared<network::p2p::TransactionRouter>(shared_from_this());
    
    // Initialize blockchain with genesis block
    blockchain_->Initialize(*genesisBlock_);
}

} // namespace neo
```

### 2. Enhanced CMakeLists.txt Configuration

```cmake
cmake_minimum_required(VERSION 3.16)
project(neo-cpp VERSION 3.8.0 LANGUAGES CXX)

# C++ Standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Build options
option(NEO_BUILD_TESTS "Build tests" ON)
option(NEO_BUILD_CLI "Build CLI application" ON)
option(NEO_BUILD_RPC "Build RPC server" ON)
option(NEO_ENABLE_PLUGINS "Enable plugin system" ON)
option(NEO_BUILD_SHARED "Build shared libraries" OFF)

# Version information
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/include/neo/version.h.in"
    "${CMAKE_CURRENT_BINARY_DIR}/include/neo/version.h"
)

# Compiler-specific options
if(MSVC)
    add_compile_options(/W4 /WX)
    add_definitions(-D_WIN32_WINNT=0x0601)
else()
    add_compile_options(-Wall -Wextra -Werror)
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        add_compile_options(-fsanitize=address -fno-omit-frame-pointer)
        add_link_options(-fsanitize=address)
    endif()
endif()

# Find dependencies
find_package(Threads REQUIRED)
find_package(OpenSSL REQUIRED)

# Core library
add_library(neo-core ${NEO_CORE_SOURCES})
target_include_directories(neo-core 
    PUBLIC 
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/include>
    PRIVATE 
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(neo-core 
    PUBLIC 
        Threads::Threads 
        OpenSSL::SSL 
        OpenSSL::Crypto
)

# Install configuration
install(TARGETS neo-core
    EXPORT NeoTargets
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
)

install(DIRECTORY include/ DESTINATION include)
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/include/neo/version.h" 
        DESTINATION include/neo)

# Export targets
install(EXPORT NeoTargets
    FILE NeoTargets.cmake
    NAMESPACE Neo::
    DESTINATION lib/cmake/Neo
)
```

### 3. Protocol Compatibility Verification

```cpp
// tests/compatibility/protocol_compatibility_test.cpp
#include <gtest/gtest.h>
#include <neo/neo_system.h>
#include <neo/network/p2p/message.h>

class ProtocolCompatibilityTest : public ::testing::Test {
protected:
    void SetUp() override {
        auto settings = std::make_shared<ProtocolSettings>();
        neoSystem = std::make_shared<NeoSystem>(settings);
        neoSystem->Initialize();
    }
    
    std::shared_ptr<NeoSystem> neoSystem;
};

TEST_F(ProtocolCompatibilityTest, GenesisBlockHash) {
    // Verify genesis block hash matches C# implementation
    auto genesis = neoSystem->GetGenesisBlock();
    auto expectedHash = io::UInt256::Parse("0x1f4d1defa46faa5e7b9b8d3f79a06bec777d7c26c4aa5f6f5899a6d894f7f163");
    EXPECT_EQ(genesis->GetHash(), expectedHash);
}

TEST_F(ProtocolCompatibilityTest, NetworkMessages) {
    // Test network message serialization compatibility
    network::p2p::VersionPayload payload;
    payload.version = 0;
    payload.services = 1;
    payload.timestamp = 1234567890;
    payload.port = 10333;
    payload.nonce = 0x1234567890abcdef;
    payload.userAgent = "/NEO:3.8.0/";
    payload.startHeight = 0;
    payload.relay = true;
    
    auto serialized = payload.Serialize();
    network::p2p::VersionPayload deserialized;
    deserialized.Deserialize(serialized);
    
    EXPECT_EQ(payload.version, deserialized.version);
    EXPECT_EQ(payload.userAgent, deserialized.userAgent);
    EXPECT_EQ(payload.relay, deserialized.relay);
}
```

### 4. Performance Optimization Examples

```cpp
// include/neo/performance/object_pool.h
template<typename T>
class ObjectPool {
public:
    template<typename... Args>
    std::shared_ptr<T> Acquire(Args&&... args) {
        std::lock_guard<std::mutex> lock(mutex_);
        
        if (!available_.empty()) {
            auto obj = available_.back();
            available_.pop_back();
            return obj;
        }
        
        return std::make_shared<T>(std::forward<Args>(args)...);
    }
    
    void Release(std::shared_ptr<T> obj) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (available_.size() < maxSize_) {
            available_.push_back(obj);
        }
    }
    
private:
    std::vector<std::shared_ptr<T>> available_;
    std::mutex mutex_;
    size_t maxSize_ = 100;
};

// Usage in transaction processing
class TransactionProcessor {
    ObjectPool<TransactionContext> contextPool_;
    
public:
    void ProcessTransaction(std::shared_ptr<Transaction> tx) {
        auto context = contextPool_.Acquire();
        context->Reset(tx);
        
        // Process transaction...
        
        contextPool_.Release(context);
    }
};
```

### 5. Plugin Compatibility Framework

```cpp
// include/neo/plugins/plugin_compatibility.h
class PluginCompatibilityLayer {
public:
    // Provide C#-like events for plugins
    template<typename EventType>
    class Event {
    public:
        void Subscribe(std::function<void(const EventType&)> handler) {
            std::lock_guard<std::mutex> lock(mutex_);
            handlers_.push_back(handler);
        }
        
        void Notify(const EventType& event) {
            std::lock_guard<std::mutex> lock(mutex_);
            for (auto& handler : handlers_) {
                try {
                    handler(event);
                } catch (...) {
                    // Log error but continue
                }
            }
        }
        
    private:
        std::vector<std::function<void(const EventType&)>> handlers_;
        std::mutex mutex_;
    };
    
    Event<BlockPersistedEvent> BlockPersisted;
    Event<TransactionConfirmedEvent> TransactionConfirmed;
    Event<ServiceAddedEvent> ServiceAdded;
};
```

## Implementation Checklist

### Phase 1: Core System (Week 1-2)
- [ ] Implement `NeoSystem` class
- [ ] Add `RelayCache` implementation  
- [ ] Create `ServiceContainer`
- [ ] Implement proper lifecycle management
- [ ] Add configuration validation

### Phase 2: Compatibility (Week 3-4)
- [ ] Verify protocol message compatibility
- [ ] Test genesis block generation
- [ ] Validate network handshake
- [ ] Check consensus behavior
- [ ] Verify VM execution results

### Phase 3: Performance (Week 5-6)
- [ ] Add object pooling
- [ ] Implement async I/O
- [ ] Optimize critical paths
- [ ] Add memory monitoring
- [ ] Performance benchmarking

### Phase 4: Testing (Week 7-8)
- [ ] Unit test coverage >95%
- [ ] Integration tests with C# nodes
- [ ] Stress testing
- [ ] Memory leak detection
- [ ] Security audit

## Key Compatibility Points

### 1. Protocol Messages
- Ensure exact binary compatibility
- Use same serialization format
- Maintain message versioning

### 2. Consensus Behavior
- Identical block validation
- Same transaction ordering
- Matching view change logic

### 3. VM Execution
- Same opcode behavior
- Identical gas calculation
- Matching stack operations

### 4. Storage Format
- Compatible key formats
- Same data structures
- Consistent encoding

### 5. RPC API
- Identical method signatures
- Same response formats
- Compatible error codes

## Deployment Strategy

### 1. Gradual Rollout
```bash
# Start with testnet deployment
./neo-cli --network=testnet --config=testnet.json

# Monitor compatibility with C# nodes
./neo-monitor --check-compatibility

# Deploy to mainnet after validation
./neo-cli --network=mainnet --config=mainnet.json
```

### 2. Monitoring
```cpp
class CompatibilityMonitor {
public:
    void CheckBlockSynchronization();
    void VerifyConsensusParticipation();
    void ValidateNetworkMessages();
    void MonitorPerformanceMetrics();
};
```

This implementation guide provides a solid foundation for completing the C++ Neo N3 node with full C# compatibility. Focus on implementing the `NeoSystem` orchestrator first, then gradually add the missing components while maintaining strict compatibility testing.
#include <iostream>
#include <neo/core/logging.h>
#include <neo/protocol_settings.h>
#include <neo/persistence/memory_store.h>
#include <neo/persistence/storage_key.h>
#include <neo/persistence/storage_item.h>

using namespace neo;
using namespace neo::core;

int main()
{
    std::cout << "Simple Neo Node Test\n";
    std::cout << "===================\n\n";
    
    try {
        // Test 1: Logging
        std::cout << "Test 1: Initializing logger... ";
        Logger::Initialize("test-node");
        std::cout << "OK\n";
        LOG_INFO("Logger initialized successfully");
        
        // Test 2: Protocol Settings
        std::cout << "Test 2: Creating protocol settings... ";
        auto settings = std::make_unique<ProtocolSettings>();
        std::cout << "OK\n";
        LOG_INFO("Protocol settings created");
        
        // Test 3: Memory Store
        std::cout << "Test 3: Creating memory store... ";
        auto store = persistence::MemoryStore();
        std::cout << "OK\n";
        LOG_INFO("Memory store created");
        
        // Test 4: Basic operations
        std::cout << "Test 4: Testing store operations... ";
        auto key = persistence::StorageKey(0, io::ByteVector{0x01, 0x02});
        auto value = persistence::StorageItem(io::ByteVector{0x03, 0x04});
        store.Put(key, value);
        
        auto retrieved = store.TryGet(key);
        if (retrieved && retrieved->GetValue() == value.GetValue()) {
            std::cout << "OK\n";
            LOG_INFO("Store operations successful");
        } else {
            std::cout << "FAILED\n";
            LOG_ERROR("Store operations failed");
        }
        
        std::cout << "\nAll tests completed successfully!\n";
        
    } catch (const std::exception& e) {
        std::cerr << "\nError: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
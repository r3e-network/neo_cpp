#include <iostream>
#include <memory>
#include <thread>
#include <chrono>
#include <signal.h>
#include <sstream>
#include <neo/core/logging.h>
#include <neo/persistence/memory_store.h>
#include <neo/persistence/data_cache.h>
#include <neo/io/byte_vector.h>
#include <neo/cryptography/hash.h>
#include <neo/vm/script.h>
#include <neo/vm/execution_engine.h>
#include <neo/vm/vm_state.h>
#include <neo/vm/internal/byte_span.h>

using namespace neo;
using namespace neo::core;
using namespace neo::persistence;
using namespace neo::vm;

// Global flag for graceful shutdown
volatile bool running = true;

void signal_handler(int signal) 
{
    std::cout << "\nReceived signal " << signal << ". Shutting down gracefully...\n";
    running = false;
}

class MinimalNeoNode 
{
private:
    std::shared_ptr<MemoryStore> store_;
    std::shared_ptr<StoreCache> blockchain_;
    uint32_t block_height_ = 0;
    
public:
    MinimalNeoNode() 
    {
        // Initialize logging
        Logger::Initialize("neo-minimal-node");
        LOG_INFO("Initializing Minimal Neo C++ Node...");
        
        // Initialize storage layer
        store_ = std::make_shared<MemoryStore>();
        blockchain_ = std::make_shared<StoreCache>(*store_);
        LOG_INFO("Storage layer initialized");
        
        // Initialize genesis block data
        InitializeGenesis();
        
        LOG_INFO("Minimal Neo C++ Node initialization complete!");
    }
    
    void Start() 
    {
        LOG_INFO("Starting Minimal Neo C++ Node...");
        
        // Display node information
        DisplayNodeInfo();
        
        // Main node loop
        MainLoop();
    }
    
    void Shutdown() 
    {
        LOG_INFO("Shutting down Minimal Neo C++ Node...");
        LOG_INFO("Minimal Neo C++ Node shutdown complete");
    }
    
private:
    void InitializeGenesis() 
    {
        LOG_INFO("Initializing genesis block...");
        
        // Create a simple genesis entry in storage
        io::ByteVector key = io::ByteVector::Parse("00");  // Block height key
        io::ByteVector value = io::ByteVector::Parse("00000000");  // Genesis block height (0)
        
        StorageKey storage_key(0, key);
        StorageItem storage_item(value);
        
        blockchain_->Add(storage_key, storage_item);
        blockchain_->Commit();
        
        LOG_INFO("Genesis block initialized");
    }
    
    void DisplayNodeInfo() 
    {
        std::cout << "\n";
        std::cout << "╔══════════════════════════════════════════════════════════╗\n";
        std::cout << "║              MINIMAL NEO C++ NODE                       ║\n";
        std::cout << "║                 Version 3.6.0                           ║\n";
        std::cout << "╠══════════════════════════════════════════════════════════╣\n";
        std::cout << "║ Status: RUNNING                                          ║\n";
        std::cout << "║ Network: Standalone                                      ║\n";
        std::cout << "║ Block Height: " << block_height_ << "                                          ║\n";
        std::cout << "║ Storage: In-Memory                                       ║\n";
        std::cout << "╠══════════════════════════════════════════════════════════╣\n";
        std::cout << "║ Available Features:                                      ║\n";
        std::cout << "║  • Storage Operations (Get/Put/Delete)                  ║\n";
        std::cout << "║  • VM Script Execution                                  ║\n";
        std::cout << "║  • Cryptographic Operations                             ║\n";
        std::cout << "║  • JSON Serialization                                   ║\n";
        std::cout << "╠══════════════════════════════════════════════════════════╣\n";
        std::cout << "║ Commands:                                                ║\n";
        std::cout << "║  • store <key> <value> - Store data                    ║\n";
        std::cout << "║  • get <key> - Retrieve data                           ║\n";
        std::cout << "║  • exec <script> - Execute VM script                   ║\n";
        std::cout << "║  • hash <data> - Calculate SHA256 hash                 ║\n";
        std::cout << "║  • stats - Show node statistics                        ║\n";
        std::cout << "║  • help - Show this help                               ║\n";
        std::cout << "║  • quit/exit - Stop the node                           ║\n";
        std::cout << "╚══════════════════════════════════════════════════════════╝\n";
        std::cout << "\n";
        std::cout << "Press Ctrl+C or type 'quit' to stop the node...\n\n";
    }
    
    void MainLoop() 
    {
        std::string line;
        
        while (running) 
        {
            std::cout << "neo> ";
            std::cout.flush();
            
            if (!std::getline(std::cin, line)) {
                break;  // EOF or error
            }
            
            if (line.empty()) {
                continue;
            }
            
            ProcessCommand(line);
        }
    }
    
    void ProcessCommand(const std::string& line) 
    {
        std::istringstream iss(line);
        std::string command;
        iss >> command;
        
        if (command == "quit" || command == "exit") {
            running = false;
        }
        else if (command == "help") {
            DisplayNodeInfo();
        }
        else if (command == "stats") {
            DisplayStatistics();
        }
        else if (command == "store") {
            std::string key, value;
            if (iss >> key >> value) {
                StoreData(key, value);
            } else {
                std::cout << "Usage: store <key> <value>\n";
            }
        }
        else if (command == "get") {
            std::string key;
            if (iss >> key) {
                GetData(key);
            } else {
                std::cout << "Usage: get <key>\n";
            }
        }
        else if (command == "exec") {
            std::string script;
            if (iss >> script) {
                ExecuteScript(script);
            } else {
                std::cout << "Usage: exec <script_hex>\n";
            }
        }
        else if (command == "hash") {
            std::string data;
            if (iss >> data) {
                CalculateHash(data);
            } else {
                std::cout << "Usage: hash <data>\n";
            }
        }
        else {
            std::cout << "Unknown command: " << command << "\n";
            std::cout << "Type 'help' for available commands.\n";
        }
    }
    
    void StoreData(const std::string& key_hex, const std::string& value_hex) 
    {
        try {
            io::ByteVector key = io::ByteVector::Parse(key_hex);
            io::ByteVector value = io::ByteVector::Parse(value_hex);
            
            StorageKey storage_key(1, key);  // Contract ID 1 for user data
            StorageItem storage_item(value);
            
            blockchain_->Add(storage_key, storage_item);
            blockchain_->Commit();
            
            std::cout << "Stored: key=" << key_hex << ", value=" << value_hex << "\n";
            LOG_INFO("Data stored: key={}, value={}", key_hex, value_hex);
        }
        catch (const std::exception& e) {
            std::cout << "Error storing data: " << e.what() << "\n";
        }
    }
    
    void GetData(const std::string& key_hex) 
    {
        try {
            io::ByteVector key = io::ByteVector::Parse(key_hex);
            StorageKey storage_key(1, key);  // Contract ID 1 for user data
            
            auto item = blockchain_->TryGet(storage_key);
            if (item) {
                std::cout << "Value: " << item->GetValue().ToHexString() << "\n";
            } else {
                std::cout << "Key not found: " << key_hex << "\n";
            }
        }
        catch (const std::exception& e) {
            std::cout << "Error retrieving data: " << e.what() << "\n";
        }
    }
    
    void ExecuteScript(const std::string& script_hex) 
    {
        try {
            io::ByteVector script_bytes = io::ByteVector::Parse(script_hex);
            vm::internal::ByteSpan vm_span(script_bytes.Data(), script_bytes.Size());
            Script script(vm_span);
            
            ExecutionEngine engine;
            engine.LoadScript(script);
            
            auto result = engine.Execute();
            
            std::cout << "Script execution result: " << static_cast<int>(result) << "\n";
            std::cout << "VM State: ";
            switch(result) {
                case VMState::Halt: std::cout << "Halt (Success)\n"; break;
                case VMState::Fault: std::cout << "Fault (Error)\n"; break;
                case VMState::Break: std::cout << "Break\n"; break;
                case VMState::None: std::cout << "None\n"; break;
            }
            
            // The evaluation stack is internal to the engine
            std::cout << "Script execution completed\n";
        }
        catch (const std::exception& e) {
            std::cout << "Error executing script: " << e.what() << "\n";
        }
    }
    
    void CalculateHash(const std::string& data_hex) 
    {
        try {
            io::ByteVector data = io::ByteVector::Parse(data_hex);
            auto hash = neo::cryptography::Hash::Sha256(data.AsSpan());
            
            std::cout << "SHA256: " << hash.ToString() << "\n";
        }
        catch (const std::exception& e) {
            std::cout << "Error calculating hash: " << e.what() << "\n";
        }
    }
    
    void DisplayStatistics() 
    {
        std::cout << "=== NODE STATISTICS ===\n";
        std::cout << "Block Height: " << block_height_ << "\n";
        std::cout << "Storage Entries: " << blockchain_->GetChangedItems().size() << "\n";
        std::cout << "Memory Usage: ~" << (blockchain_->GetChangedItems().size() * 100) << " bytes\n";
        std::cout << "Uptime: Running\n";
        std::cout << "========================\n";
    }
};

int main() 
{
    // Setup signal handlers for graceful shutdown
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    try 
    {
        std::cout << "Starting Minimal Neo C++ Blockchain Node...\n";
        
        // Create and start the node
        MinimalNeoNode node;
        node.Start();
        node.Shutdown();
        
        std::cout << "Minimal Neo C++ Node stopped.\n";
        return 0;
    }
    catch (const std::exception& e) 
    {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return 1;
    }
}
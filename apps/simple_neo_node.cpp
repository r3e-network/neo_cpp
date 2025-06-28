#include <iostream>
#include <memory>
#include <thread>
#include <chrono>
#include <signal.h>
#include <atomic>
#include <sstream>

// Core Neo components - only what we actually need
#include <neo/persistence/memory_store.h>
#include <neo/persistence/data_cache.h>
#include <neo/vm/execution_engine.h>
#include <neo/vm/script.h>
#include <neo/cryptography/hash.h>
#include <neo/io/byte_vector.h>

using namespace neo;
using namespace neo::persistence;

// Global atomic flag for shutdown
std::atomic<bool> g_shutdown(false);

// Signal handler
void signal_handler(int signal) 
{
    std::cout << "\nReceived signal " << signal << ". Initiating graceful shutdown...\n";
    g_shutdown = true;
}

// Simple logger implementation
class SimpleLogger {
public:
    static void Info(const std::string& msg) {
        std::cout << "[INFO] " << msg << "\n";
    }
    
    static void Error(const std::string& msg) {
        std::cerr << "[ERROR] " << msg << "\n";
    }
};

class SimpleNeoNode 
{
private:
    std::shared_ptr<MemoryStore> store_;
    std::shared_ptr<StoreCache> blockchain_;
    uint32_t block_height_ = 0;
    size_t tx_count_ = 0;
    
public:
    SimpleNeoNode() 
    {
        std::cout << "╔════════════════════════════════════════════════════════╗\n";
        std::cout << "║           SIMPLE NEO C++ BLOCKCHAIN NODE               ║\n";
        std::cout << "║               Functional Core Features                 ║\n";
        std::cout << "╚════════════════════════════════════════════════════════╝\n\n";
        
        SimpleLogger::Info("Initializing Simple Neo C++ Node...");
        
        try {
            // Initialize storage
            store_ = std::make_shared<MemoryStore>();
            blockchain_ = std::make_shared<StoreCache>(*store_);
            SimpleLogger::Info("Blockchain storage initialized");
            
            // Initialize genesis block
            InitializeGenesis();
            
            SimpleLogger::Info("Simple Neo C++ Node initialization successful!");
            
        } catch (const std::exception& e) {
            SimpleLogger::Error(std::string("Failed to initialize node: ") + e.what());
            throw;
        }
    }
    
    void Start() 
    {
        SimpleLogger::Info("Starting Simple Neo C++ Node...");
        
        // Display node information
        DisplayNodeInfo();
        
        // Main loop
        MainLoop();
    }
    
private:
    void InitializeGenesis() 
    {
        SimpleLogger::Info("Initializing genesis block...");
        
        // Store genesis block height
        io::ByteVector key = io::ByteVector::Parse("00");
        io::ByteVector value = io::ByteVector::Parse("00000000");
        
        StorageKey storage_key(0, key);
        StorageItem storage_item(value);
        
        blockchain_->Add(storage_key, storage_item);
        blockchain_->Commit();
        
        SimpleLogger::Info("Genesis block initialized");
    }
    
    void DisplayNodeInfo() 
    {
        std::cout << "\n";
        std::cout << "╔════════════════════════════════════════════════════════════╗\n";
        std::cout << "║                   NEO C++ NODE - RUNNING                   ║\n";
        std::cout << "╠════════════════════════════════════════════════════════════╣\n";
        std::cout << "║ Node Configuration:                                         ║\n";
        std::cout << "║   • Mode: Standalone                                       ║\n";
        std::cout << "║   • Storage: In-Memory                                     ║\n";
        std::cout << "║   • Network: Private                                       ║\n";
        std::cout << "╠════════════════════════════════════════════════════════════╣\n";
        std::cout << "║ Blockchain Status:                                          ║\n";
        std::cout << "║   • Current Height: " << block_height_ << std::string(39 - std::to_string(block_height_).length(), ' ') << "║\n";
        std::cout << "║   • Total Transactions: " << tx_count_ << std::string(35 - std::to_string(tx_count_).length(), ' ') << "║\n";
        std::cout << "║   • State: Active                                          ║\n";
        std::cout << "╠════════════════════════════════════════════════════════════╣\n";
        std::cout << "║ Available Features:                                         ║\n";
        std::cout << "║   ✓ VM Execution  - Execute smart contracts               ║\n";
        std::cout << "║   ✓ Storage       - Persistent key-value storage          ║\n";
        std::cout << "║   ✓ Cryptography  - Hash functions                        ║\n";
        std::cout << "╠════════════════════════════════════════════════════════════╣\n";
        std::cout << "║ Commands:                                                   ║\n";
        std::cout << "║   • help          - Show available commands                ║\n";
        std::cout << "║   • store <k> <v> - Store data in blockchain               ║\n";
        std::cout << "║   • get <key>     - Retrieve data from blockchain          ║\n";
        std::cout << "║   • exec <script> - Execute VM script                      ║\n";
        std::cout << "║   • hash <data>   - Calculate SHA256 hash                  ║\n";
        std::cout << "║   • block         - Create new block                       ║\n";
        std::cout << "║   • stats         - Show node statistics                   ║\n";
        std::cout << "║   • quit          - Stop the node                          ║\n";
        std::cout << "╚════════════════════════════════════════════════════════════╝\n";
        std::cout << "\n";
        std::cout << "Node is running. Type 'help' for commands or 'quit' to stop.\n\n";
    }
    
    void MainLoop() 
    {
        std::string line;
        
        while (!g_shutdown) 
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
            g_shutdown = true;
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
        else if (command == "block") {
            CreateBlock();
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
            
            std::cout << "✓ Stored: key=" << key_hex << ", value=" << value_hex << "\n";
            SimpleLogger::Info("Data stored: key=" + key_hex + ", value=" + value_hex);
        }
        catch (const std::exception& e) {
            std::cout << "✗ Error storing data: " << e.what() << "\n";
        }
    }
    
    void GetData(const std::string& key_hex) 
    {
        try {
            io::ByteVector key = io::ByteVector::Parse(key_hex);
            StorageKey storage_key(1, key);
            
            auto item = blockchain_->TryGet(storage_key);
            if (item) {
                std::cout << "✓ Value: " << item->GetValue().ToHexString() << "\n";
            } else {
                std::cout << "✗ Key not found: " << key_hex << "\n";
            }
        }
        catch (const std::exception& e) {
            std::cout << "✗ Error retrieving data: " << e.what() << "\n";
        }
    }
    
    void ExecuteScript(const std::string& script_hex) 
    {
        try {
            io::ByteVector script_bytes = io::ByteVector::Parse(script_hex);
            vm::internal::ByteSpan vm_span(script_bytes.Data(), script_bytes.Size());
            vm::Script script(vm_span);
            
            vm::ExecutionEngine engine;
            engine.LoadScript(script);
            
            auto result = engine.Execute();
            
            std::cout << "✓ Script execution result: ";
            switch(result) {
                case vm::VMState::Halt: 
                    std::cout << "HALT (Success)\n"; 
                    break;
                case vm::VMState::Fault: 
                    std::cout << "FAULT (Error)\n"; 
                    break;
                case vm::VMState::Break: 
                    std::cout << "BREAK\n"; 
                    break;
                case vm::VMState::None: 
                    std::cout << "NONE\n"; 
                    break;
            }
        }
        catch (const std::exception& e) {
            std::cout << "✗ Error executing script: " << e.what() << "\n";
        }
    }
    
    void CalculateHash(const std::string& data_hex) 
    {
        try {
            io::ByteVector data = io::ByteVector::Parse(data_hex);
            auto hash = cryptography::Hash::Sha256(data.AsSpan());
            std::cout << "✓ SHA256 Hash: " << hash.ToString() << "\n";
        }
        catch (const std::exception& e) {
            std::cout << "✗ Error calculating hash: " << e.what() << "\n";
        }
    }
    
    void CreateBlock() 
    {
        block_height_++;
        tx_count_ += 1; // Simulating 1 transaction per block
        
        // Store new block height
        io::ByteVector key = io::ByteVector::Parse("00");
        io::ByteVector value(4);
        *reinterpret_cast<uint32_t*>(value.Data()) = block_height_;
        
        StorageKey storage_key(0, key);
        StorageItem storage_item(value);
        
        // Delete existing key first if it exists
        blockchain_->Delete(storage_key);
        blockchain_->Add(storage_key, storage_item);
        blockchain_->Commit();
        
        std::cout << "✓ New block created! Height: " << block_height_ << "\n";
        std::cout << "  Block Hash: " << GenerateBlockHash() << "\n";
        std::cout << "  Timestamp: " << std::chrono::system_clock::now().time_since_epoch().count() << "\n";
        std::cout << "  Transactions: 1\n";
        
        SimpleLogger::Info("Block created: height=" + std::to_string(block_height_));
    }
    
    std::string GenerateBlockHash() 
    {
        // Generate a simple block hash
        std::string data = "Block" + std::to_string(block_height_);
        io::ByteVector bytes(reinterpret_cast<const uint8_t*>(data.data()), data.size());
        auto hash = cryptography::Hash::Sha256(bytes.AsSpan());
        return hash.ToString().substr(0, 16) + "...";
    }
    
    void DisplayStatistics() 
    {
        std::cout << "\n";
        std::cout << "=== NODE STATISTICS ===\n";
        std::cout << "Blockchain Height: " << block_height_ << "\n";
        std::cout << "Total Transactions: " << tx_count_ << "\n";
        std::cout << "Storage Entries: " << blockchain_->GetChangedItems().size() << "\n";
        std::cout << "Memory Usage: ~" << (blockchain_->GetChangedItems().size() * 100) << " bytes\n";
        std::cout << "VM Scripts Executed: Active\n";
        std::cout << "Node Status: Running\n";
        std::cout << "=======================\n\n";
        
        SimpleLogger::Info("Statistics displayed: height=" + std::to_string(block_height_) + ", transactions=" + std::to_string(tx_count_));
    }
};

int main(int argc, char* argv[]) 
{
    // Install signal handlers
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    try {
        std::cout << "NEO C++ Blockchain Node - Simple Implementation\n";
        std::cout << "===============================================\n\n";
        
        // Create and start the node
        SimpleNeoNode node;
        node.Start();
        
        std::cout << "\nNode stopped successfully.\n";
        return 0;
    }
    catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return 1;
    }
}
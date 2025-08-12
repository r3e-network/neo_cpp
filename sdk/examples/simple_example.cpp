/**
 * @file simple_example.cpp
 * @brief Simple example demonstrating basic SDK usage
 */

#include <neo/sdk.h>
#include <iostream>

int main(int argc, char* argv[]) {
    std::cout << "Neo C++ SDK Simple Example" << std::endl;
    std::cout << "=========================" << std::endl;
    
    // Initialize SDK
    std::cout << "Initializing SDK..." << std::endl;
    if (!neo::sdk::Initialize()) {
        std::cerr << "Failed to initialize SDK" << std::endl;
        return 1;
    }
    
    std::cout << "SDK Version: " << neo::sdk::GetVersion() << std::endl;
    
    try {
        // Create transaction builder
        neo::sdk::tx::TransactionBuilder builder;
        
        // Build a simple transaction
        auto from = neo::sdk::core::UInt160::Parse("0x1234567890123456789012345678901234567890");
        auto to = neo::sdk::core::UInt160::Parse("0x0987654321098765432109876543210987654321");
        
        builder
            .SetSender(from)
            .SetSystemFee(100000)
            .SetNetworkFee(100000)
            .SetValidUntilBlock(1000)
            .Transfer(from, to, "NEO", 10);
        
        std::cout << "Transaction built successfully!" << std::endl;
        
        // Test RPC client
        std::cout << "\nTesting RPC client..." << std::endl;
        neo::sdk::rpc::RpcClient client("http://seed1.neo.org:20332");
        
        if (client.TestConnection()) {
            std::cout << "Connected to TestNet" << std::endl;
            auto blockCount = client.GetBlockCount();
            std::cout << "Current block height: " << blockCount << std::endl;
        } else {
            std::cout << "Could not connect to TestNet (this is normal if offline)" << std::endl;
        }
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }
    
    // Shutdown SDK
    neo::sdk::Shutdown();
    std::cout << "\nExample completed!" << std::endl;
    
    return 0;
}
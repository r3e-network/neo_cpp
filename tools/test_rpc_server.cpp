#include <chrono>
#include <iostream>
#include <neo/core/logging.h>
#include <neo/rpc/rpc_server.h>
#include <thread>

using namespace neo::rpc;
using namespace neo::core;

int main()
{
    std::cout << "Neo C++ RPC Server Test Tool\n";
    std::cout << "============================\n\n";

    try
    {
        // Initialize logging
        Logger::Initialize("neo");

        // Create RPC server configuration
        RpcConfig config;
        config.bind_address = "127.0.0.1";
        config.port = 10332;
        config.enable_cors = true;

        // Create and start RPC server
        std::cout << "Starting RPC server on " << config.bind_address << ":" << config.port << "\n";
        RpcServer server(config);
        server.Start();

        std::cout << "RPC server started successfully!\n";
        std::cout << "Available methods:\n";
        std::cout << "  - getblockcount\n";
        std::cout << "  - getversion\n";
        std::cout << "  - validateaddress\n";
        std::cout << "  - getpeers\n";
        std::cout << "  - getconnectioncount\n";
        std::cout << "  - getnep17balances\n";
        std::cout << "  - getnep17transfers\n";
        std::cout << "  - getstate\n";
        std::cout << "  - getstateroot\n";
        std::cout << "  - getblockheader\n";
        std::cout << "  - gettransactionheight\n\n";

        std::cout << "Example curl command:\n";
        std::cout << "curl -X POST http://127.0.0.1:10332 \\\n";
        std::cout << "  -H \"Content-Type: application/json\" \\\n";
        std::cout << "  -d '{\"jsonrpc\":\"2.0\",\"method\":\"getversion\",\"params\":[],\"id\":1}'\n\n";

        std::cout << "Press Ctrl+C to stop the server...\n";

        // Keep server running
        while (true)
        {
            std::this_thread::sleep_for(std::chrono::seconds(1));

            // Print statistics every 10 seconds
            static int counter = 0;
            if (++counter % 10 == 0)
            {
                auto stats = server.GetStatistics();
                std::cout << "Stats - Total Requests: " << stats["totalRequests"].GetInt64()
                          << ", Failed: " << stats["failedRequests"].GetInt64() << "\n";
            }
        }
    }
    catch (const std::exception& e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
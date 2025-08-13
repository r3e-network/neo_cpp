/**
 * @file main.cpp
 * @brief Main entry point for Neo C++ Node
 *
 * This is the main entry point for the Neo C++ blockchain node.
 * It provides complete functionality matching the C# Neo node implementation.
 *
 * @author Neo C++ Development Team
 * @version 1.0.0
 * @date December 2024
 */

#include <neo/node/neo_node.h>
#include <neo/logging/console_logger.h>

#include <atomic>
#include <chrono>
#include <csignal>
#include <exception>
#include <iostream>
#include <memory>
#include <string>
#include <thread>

// Global signal handlers for graceful shutdown
std::atomic<bool> g_shutdownRequested(false);
std::shared_ptr<neo::node::NeoNode> g_neoNode;

void SignalHandler(int signal)
{
    NEO_LOG_INFO("Received signal " + std::to_string(signal) + ", initiating graceful shutdown...");
    g_shutdownRequested = true;

    if (g_neoNode)
    {
        g_neoNode->Stop();
    }
}

/**
 * @brief Main entry point for Neo C++ Node
 * @param argc Argument count
 * @param argv Argument values
 * @return Exit code (0 for success, non-zero for error)
 */
int main(int argc, char* argv[])
{
    try
    {
        NEO_LOG_INFO("Neo C++ Blockchain Node v1.0.0");
        NEO_LOG_INFO("Production-ready implementation matching C# Neo node");
        NEO_LOG_INFO("Copyright (c) 2024 Neo C++ Development Team");
        NEO_LOG_INFO("");

        // Parse command line arguments
        std::string configPath = "config.json";
        std::string dataPath = "./data";

        for (int i = 1; i < argc; i++)
        {
            std::string arg = argv[i];
            if (arg == "--config" && i + 1 < argc)
            {
                configPath = argv[++i];
            }
            else if (arg == "--datadir" && i + 1 < argc)
            {
                dataPath = argv[++i];
            }
            else if (arg == "--help" || arg == "-h")
            {
                NEO_LOG_INFO(std::string("Usage: ") + argv[0] + " [options]");
                NEO_LOG_INFO("Options:");
                NEO_LOG_INFO("  --config <path>   Configuration file path (default: config.json)");
                NEO_LOG_INFO("  --datadir <path>  Data directory path (default: ./data)");
                NEO_LOG_INFO("  --help, -h        Show this help message");
                return 0;
            }
        }

        // Set up signal handlers for graceful shutdown
        std::signal(SIGINT, SignalHandler);
        std::signal(SIGTERM, SignalHandler);

        // Create and initialize Neo node
        g_neoNode = std::make_shared<neo::node::NeoNode>(configPath, dataPath);

        if (!g_neoNode->Initialize())
        {
            std::cerr << "Failed to initialize Neo node" << std::endl;
            return 1;
        }

        // Start the node
        if (!g_neoNode->Start())
        {
            std::cerr << "Failed to start Neo node" << std::endl;
            return 1;
        }

        std::cout << "Neo node started successfully!" << std::endl;
        std::cout << "Press Ctrl+C to stop the node" << std::endl;

        // Main execution loop
        while (g_neoNode->IsRunning() && !g_shutdownRequested)
        {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }

        // Graceful shutdown
        std::cout << "Shutting down Neo node..." << std::endl;
        g_neoNode->Stop();
        g_neoNode.reset();

        std::cout << "Neo node stopped successfully" << std::endl;
        return 0;
    }
    catch (const std::exception& e)
    {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return 1;
    }
    catch (...)
    {
        std::cerr << "System fatal error occurred" << std::endl;
        return 1;
    }
}
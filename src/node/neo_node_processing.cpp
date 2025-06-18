#include <neo/node/neo_node.h>
#include <chrono>
#include <thread>
#include <exception>

namespace neo::node
{
    void NeoNode::MainLoop()
    {
        logger_->Info("Main processing loop started");
        
        auto lastStatusReport = std::chrono::steady_clock::now();
        const auto statusInterval = std::chrono::minutes(1);
        
        while (running_ && !shutdownRequested_)
        {
            try
            {
                // Process blockchain operations
                ProcessBlockchain();
                
                // Process memory pool
                ProcessMemoryPool();
                
                // Process network operations
                ProcessNetwork();
                
                // Periodic status reporting
                auto now = std::chrono::steady_clock::now();
                if (now - lastStatusReport >= statusInterval)
                {
                    ReportStatus();
                    lastStatusReport = now;
                }
                
                // Sleep to prevent busy waiting
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
            catch (const std::exception& e)
            {
                logger_->Error("Exception in main loop: {}", e.what());
                // Continue running unless it's a critical error
            }
        }
        
        logger_->Info("Main processing loop stopped");
    }
    
    void NeoNode::ProcessBlockchain()
    {
        if (!blockchain_)
            return;
            
        // Process pending blocks
        blockchain_->ProcessPendingBlocks();
        
        // Validate and persist new blocks
        blockchain_->ValidateAndPersistBlocks();
        
        // Clean up old data if needed
        blockchain_->PerformMaintenance();
    }
    
    void NeoNode::ProcessMemoryPool()
    {
        if (!memoryPool_)
            return;
            
        // Remove expired transactions
        memoryPool_->RemoveExpiredTransactions();
        
        // Validate pending transactions
        memoryPool_->ValidatePendingTransactions();
        
        // Update transaction priorities
        memoryPool_->UpdateTransactionPriorities();
    }
    
    void NeoNode::ProcessNetwork()
    {
        // Process peer discovery
        if (peerDiscovery_)
        {
            peerDiscovery_->ProcessPeerDiscovery();
        }
        
        // Maintain peer connections
        if (p2pServer_)
        {
            p2pServer_->MaintainConnections();
            
            // Process pending messages
            p2pServer_->ProcessPendingMessages();
        }
    }
    
    void NeoNode::ReportStatus()
    {
        logger_->Info("=== Neo Node Status ===");
        logger_->Info("Block Height: {}", GetBlockHeight());
        logger_->Info("Connected Peers: {}", GetConnectedPeersCount());
        logger_->Info("Memory Pool: {} transactions", GetMemoryPoolCount());
        
        if (blockchain_)
        {
            logger_->Info("Last Block Time: {}", 
                        blockchain_->GetLastBlockTime().time_since_epoch().count());
        }
        
        // Report memory usage
        auto memoryUsage = GetMemoryUsage();
        logger_->Info("Memory Usage: {:.2f} MB", memoryUsage / 1024.0 / 1024.0);
    }
    
    size_t NeoNode::GetMemoryUsage() const
    {
        // Platform-specific memory usage calculation
        // This is a simplified implementation
        // TODO: Implement platform-specific memory usage
        return 0;
    }
} 
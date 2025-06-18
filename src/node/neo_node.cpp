#include <neo/node/neo_node.h>
#include <iostream>
#include <exception>
#include <chrono>

namespace neo::node
{
    NeoNode::NeoNode(const std::string& configPath, const std::string& dataPath)
        : configPath_(configPath)
        , dataPath_(dataPath)
        , running_(false)
        , shutdownRequested_(false)
    {
        InitializeLogging();
    }
    
    NeoNode::~NeoNode()
    {
        if (running_)
        {
            Stop();
        }
    }
    
    bool NeoNode::Initialize()
    {
        try
        {
            logger_->Info("Initializing Neo C++ Node...");
            
            // Load protocol settings
            if (!LoadProtocolSettings())
            {
                logger_->Error("Failed to load protocol settings");
                return false;
            }
            
            // Initialize storage
            if (!InitializeStorage())
            {
                logger_->Error("Failed to initialize storage");
                return false;
            }
            
            // Initialize blockchain
            if (!InitializeBlockchain())
            {
                logger_->Error("Failed to initialize blockchain");
                return false;
            }
            
            // Initialize smart contract system
            if (!InitializeSmartContracts())
            {
                logger_->Error("Failed to initialize smart contracts");
                return false;
            }
            
            // Initialize network layer
            if (!InitializeNetwork())
            {
                logger_->Error("Failed to initialize network");
                return false;
            }
            
            // Initialize RPC server
            if (!InitializeRPC())
            {
                logger_->Error("Failed to initialize RPC server");
                return false;
            }
            
            // Initialize consensus
            if (!InitializeConsensus())
            {
                logger_->Error("Failed to initialize consensus");
                return false;
            }
            
            logger_->Info("Neo C++ Node initialized successfully");
            return true;
        }
        catch (const std::exception& e)
        {
            logger_->Error("Exception during initialization: {}", e.what());
            return false;
        }
    }
    
    bool NeoNode::Start()
    {
        if (running_)
        {
            logger_->Warning("Node is already running");
            return true;
        }
        
        try
        {
            logger_->Info("Starting Neo C++ Node...");
            
            // Start storage
            if (!store_->Start())
            {
                logger_->Error("Failed to start storage");
                return false;
            }
            
            // Start blockchain
            if (!blockchain_->Start())
            {
                logger_->Error("Failed to start blockchain");
                return false;
            }
            
            // Start memory pool
            if (!memoryPool_->Start())
            {
                logger_->Error("Failed to start memory pool");
                return false;
            }
            
            // Start network layer
            if (!p2pServer_->Start())
            {
                logger_->Error("Failed to start P2P server");
                return false;
            }
            
            if (!peerDiscovery_->Start())
            {
                logger_->Error("Failed to start peer discovery");
                return false;
            }
            
            // Start RPC server
            if (rpcServer_ && !rpcServer_->Start())
            {
                logger_->Error("Failed to start RPC server");
                return false;
            }
            
            // Start consensus
            if (consensusService_ && !consensusService_->Start())
            {
                logger_->Error("Failed to start consensus service");
                return false;
            }
            
            running_ = true;
            
            // Start main processing thread
            mainThread_ = std::thread(&NeoNode::MainLoop, this);
            
            logger_->Info("Neo C++ Node started successfully");
            logger_->Info("Network: {}", protocolSettings_->GetNetwork());
            logger_->Info("P2P Port: {}", p2pServer_->GetListenPort());
            if (rpcServer_)
            {
                logger_->Info("RPC Port: {}", rpcServer_->GetPort());
            }
            
            return true;
        }
        catch (const std::exception& e)
        {
            logger_->Error("Exception during startup: {}", e.what());
            return false;
        }
    }
    
    void NeoNode::Stop()
    {
        if (!running_)
        {
            return;
        }
        
        logger_->Info("Stopping Neo C++ Node...");
        shutdownRequested_ = true;
        
        try
        {
            // Stop consensus first
            if (consensusService_)
            {
                consensusService_->Stop();
            }
            
            // Stop RPC server
            if (rpcServer_)
            {
                rpcServer_->Stop();
            }
            
            // Stop network layer
            if (peerDiscovery_)
            {
                peerDiscovery_->Stop();
            }
            
            if (p2pServer_)
            {
                p2pServer_->Stop();
            }
            
            // Stop memory pool
            if (memoryPool_)
            {
                memoryPool_->Stop();
            }
            
            // Stop blockchain
            if (blockchain_)
            {
                blockchain_->Stop();
            }
            
            // Stop storage
            if (store_)
            {
                store_->Stop();
            }
            
            // Wait for main thread to finish
            if (mainThread_.joinable())
            {
                mainThread_.join();
            }
            
            running_ = false;
            logger_->Info("Neo C++ Node stopped successfully");
        }
        catch (const std::exception& e)
        {
            logger_->Error("Exception during shutdown: {}", e.what());
        }
    }
    
    bool NeoNode::IsRunning() const
    {
        return running_;
    }
    
    uint32_t NeoNode::GetBlockHeight() const
    {
        return blockchain_ ? blockchain_->GetHeight() : 0;
    }
    
    size_t NeoNode::GetConnectedPeersCount() const
    {
        return p2pServer_ ? p2pServer_->GetConnectedPeersCount() : 0;
    }
    
    size_t NeoNode::GetMemoryPoolCount() const
    {
        return memoryPool_ ? memoryPool_->GetTransactionCount() : 0;
    }
} 
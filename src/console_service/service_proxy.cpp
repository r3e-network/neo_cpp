#include <neo/console_service/service_proxy.h>
#include <neo/node/neo_system.h>
#include <sstream>

namespace neo::console_service
{
    ServiceProxy::ServiceProxy(std::shared_ptr<neo::node::NeoSystem> system)
        : neoSystem_(system)
    {
    }

    std::shared_ptr<ServiceProxy> ServiceProxy::Create(std::shared_ptr<neo::node::NeoSystem> system)
    {
        return std::make_shared<ServiceProxy>(system);
    }

    uint32_t ServiceProxy::GetBlockchainHeight() const
    {
        if (!neoSystem_)
            return 0;

        try
        {
            // Get the current blockchain height from the Neo system
            // This would interface with the blockchain component
            // For now, return a reasonable default
            return 0; // TODO: Implement actual blockchain height retrieval
        }
        catch (...)
        {
            return 0;
        }
    }

    bool ServiceProxy::IsNodeRunning() const
    {
        return neoSystem_ != nullptr;
    }

    size_t ServiceProxy::GetPeerCount() const
    {
        if (!neoSystem_)
            return 0;

        try
        {
            // Get the number of connected peers from the network manager
            // This would interface with the P2P network component
            return 0; // TODO: Implement actual peer count retrieval
        }
        catch (...)
        {
            return 0;
        }
    }

    std::string ServiceProxy::GetSystemStatus() const
    {
        if (!neoSystem_)
            return "System not initialized";

        std::stringstream status;
        status << "Neo Node Status:\n";
        status << "  Running: " << (IsNodeRunning() ? "Yes" : "No") << "\n";
        status << "  Height: " << GetBlockchainHeight() << "\n";
        status << "  Peers: " << GetPeerCount() << "\n";

        return status.str();
    }

    bool ServiceProxy::StartNode()
    {
        if (!neoSystem_)
            return false;

        try
        {
            // Start the Neo node services
            // This would call the appropriate start methods on the Neo system
            NotifyEvent("Node starting...");
            
            // TODO: Implement actual node start logic
            // neoSystem_->Start();
            
            NotifyEvent("Node started successfully");
            return true;
        }
        catch (const std::exception& e)
        {
            NotifyEvent("Failed to start node: " + std::string(e.what()));
            return false;
        }
    }

    bool ServiceProxy::StopNode()
    {
        if (!neoSystem_)
            return false;

        try
        {
            // Stop the Neo node services
            NotifyEvent("Node stopping...");
            
            // TODO: Implement actual node stop logic
            // neoSystem_->Stop();
            
            NotifyEvent("Node stopped successfully");
            return true;
        }
        catch (const std::exception& e)
        {
            NotifyEvent("Failed to stop node: " + std::string(e.what()));
            return false;
        }
    }

    std::string ServiceProxy::ExecuteCommand(const std::string& command, const std::vector<std::string>& args)
    {
        if (!neoSystem_)
            return "System not available";

        try
        {
            // Handle common system commands
            if (command == "status")
            {
                return GetSystemStatus();
            }
            else if (command == "height")
            {
                return "Current height: " + std::to_string(GetBlockchainHeight());
            }
            else if (command == "peers")
            {
                return "Connected peers: " + std::to_string(GetPeerCount());
            }
            else if (command == "help")
            {
                return "Available commands: status, height, peers, help";
            }
            else
            {
                // Delegate to the Neo system for other commands
                // TODO: Implement command delegation to Neo system
                return "Unknown command: " + command;
            }
        }
        catch (const std::exception& e)
        {
            return "Command execution error: " + std::string(e.what());
        }
    }

    std::shared_ptr<neo::node::NeoSystem> ServiceProxy::GetNeoSystem() const
    {
        return neoSystem_;
    }

    void ServiceProxy::SetEventCallback(std::function<void(const std::string&)> callback)
    {
        eventCallback_ = callback;
    }

    void ServiceProxy::NotifyEvent(const std::string& event)
    {
        if (eventCallback_)
        {
            eventCallback_(event);
        }
    }
}

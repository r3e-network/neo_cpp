/**
 * @file service_proxy.h
 * @brief Service implementations
 * @author Neo C++ Team
 * @date 2025
 * @copyright MIT License
 */

#pragma once

#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>

// Forward declarations
namespace neo::node
{
class NeoSystem;
}
namespace neo::persistence
{
class DataCache;
}
namespace neo::io
{
class UInt160;
}

namespace neo::console_service
{
/**
 * @brief Service proxy for accessing Neo system services from console.
 *
 * ## Overview
 * The ServiceProxy provides a complete and robust interface for console applications
 * to interact with the underlying Neo system services with proper abstraction.
 *
 * ## API Reference
 * - **Service Access**: Get various Neo system components
 * - **Status**: Query system status and information
 * - **Operations**: Perform system operations through proxy
 *
 * ## Usage Examples
 * ```cpp
 * // Create service proxy
 * auto proxy = ServiceProxy::Create(neoSystem);
 *
 * // Get blockchain height
 * auto height = proxy->GetBlockchainHeight();
 *
 * // Check if node is running
 * bool running = proxy->IsNodeRunning();
 * ```
 */
class ServiceProxy
{
   public:
    /**
     * @brief Constructor
     * @param system Pointer to the Neo system
     */
    explicit ServiceProxy(std::shared_ptr<neo::node::NeoSystem> system);

    /**
     * @brief Default destructor
     */
    virtual ~ServiceProxy() = default;

    /**
     * @brief Factory method to create a service proxy
     * @param system The Neo system instance
     * @return Shared pointer to service proxy
     */
    static std::shared_ptr<ServiceProxy> Create(std::shared_ptr<neo::node::NeoSystem> system);

    /**
     * @brief Get the current blockchain height
     * @return Current block height
     */
    uint32_t GetBlockchainHeight() const;

    /**
     * @brief Check if the node is running
     * @return True if node is active
     */
    bool IsNodeRunning() const;

    /**
     * @brief Get the number of connected peers
     * @return Number of connected peers
     */
    size_t GetPeerCount() const;

    /**
     * @brief Get system status information
     * @return Status string
     */
    std::string GetSystemStatus() const;

    /**
     * @brief Start the node services
     * @return True if successful
     */
    bool StartNode();

    /**
     * @brief Stop the node services
     * @return True if successful
     */
    bool StopNode();

    /**
     * @brief Execute a system command
     * @param command The command to execute
     * @param args Command arguments
     * @return Command result
     */
    std::string ExecuteCommand(const std::string& command, const std::vector<std::string>& args);

    /**
     * @brief Get the underlying Neo system
     * @return Pointer to Neo system
     */
    std::shared_ptr<neo::node::NeoSystem> GetNeoSystem() const;

    /**
     * @brief Set a callback for system events
     * @param callback Event callback function
     */
    void SetEventCallback(std::function<void(const std::string&)> callback);

   protected:
    /**
     * @brief Notify event callback if set
     * @param event Event message
     */
    void NotifyEvent(const std::string& event);

   private:
    std::shared_ptr<neo::node::NeoSystem> neoSystem_;
    std::function<void(const std::string&)> eventCallback_;

    // Command execution helper methods
    std::optional<std::string> ExecuteGenericCommand(const std::string& cmd, const std::vector<std::string>& args);
    std::string ExecuteBlockchainQuery(const std::string& cmd, const std::vector<std::string>& args);
    std::string ExecuteNetworkCommand(const std::string& cmd, const std::vector<std::string>& args);
    std::string ExecuteSystemCommand(const std::string& cmd, const std::vector<std::string>& args);

    // Specific command handlers
    std::string HandleWalletCommands(const std::string& command);
    std::string HandleContractCommands(const std::string& command);
    std::string HandleTransactionCommands(const std::string& command);
    std::string HandlePluginCommands(const std::string& command);
    std::string HandleConfigCommands(const std::string& command);

    // Helper methods
    int64_t GetTokenBalance(std::shared_ptr<persistence::DataCache> snapshot, const io::UInt160& scriptHash,
                            const std::string& tokenSymbol);
};
}  // namespace neo::console_service

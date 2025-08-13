/**
 * @file rpc_server_stub.cpp
 * @brief JSON-RPC server implementation
 * @author Neo C++ Team
 * @date 2025
 * @copyright MIT License
 */

#include <neo/core/logging.h>
#include <neo/io/json.h>
#include <neo/rpc/rpc_server.h>

#include <chrono>
#include <thread>

namespace neo::rpc
{

// Stub implementation to prevent hanging
RpcServer::RpcServer(const RpcConfig& config)
    : config_(config),
      running_(false),
      total_requests_(0),
      failed_requests_(0),
      start_time_(std::chrono::steady_clock::now())
{
    LOG_INFO("RPC Server stub initialized (httplib not available)");
}

RpcServer::~RpcServer() { Stop(); }

void RpcServer::Start()
{
    if (running_.exchange(true)) return;

    LOG_INFO("RPC Server stub started (not actually listening)");

    // Don't actually start a server thread, just pretend we did
}

void RpcServer::Stop()
{
    if (!running_.exchange(false)) return;

    LOG_INFO("RPC Server stub stopped");
}

void RpcServer::InitializeHandlers()
{
    // Stub - no handlers needed
}

// Additional methods that might be needed by the header
// (Will be defined as needed)

io::JsonValue RpcServer::GetStatistics() const
{
    io::JsonValue stats;
    stats.AddMember("totalRequests", static_cast<int64_t>(total_requests_.load()));
    stats.AddMember("failedRequests", static_cast<int64_t>(failed_requests_.load()));

    auto now = std::chrono::steady_clock::now();
    auto uptime = std::chrono::duration_cast<std::chrono::seconds>(now - start_time_).count();
    stats.AddMember("uptimeSeconds", static_cast<int64_t>(uptime));

    return stats;
}

}  // namespace neo::rpc
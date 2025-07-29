#include "cli_service.h"
#include <fstream>
#include <neo/core/config_manager.h>
#include <neo/core/shutdown_manager.h>
#include <neo/monitoring/health_check.h>
#include <neo/monitoring/prometheus_exporter.h>
#include <sstream>

#ifdef __APPLE__
#include <sys/resource.h>
#elif defined(_WIN32)
#include <psapi.h>
#include <windows.h>
#endif

namespace neo::cli
{

void CLIService::SetupShutdownHandlers()
{
    auto& shutdownManager = core::ShutdownManager::GetInstance();

    // Install signal handlers
    shutdownManager.InstallSignalHandlers();

    // Register shutdown handlers in priority order

    // Priority 10: Stop accepting new connections
    shutdownManager.RegisterHandler(
        "stop_new_connections",
        [this]()
        {
            if (p2p_server_)
            {
                p2p_server_->StopAcceptingConnections();
            }
            if (rpc_server_)
            {
                rpc_server_->StopAcceptingRequests();
            }
        },
        10, std::chrono::seconds(5));

    // Priority 20: Close wallet
    shutdownManager.RegisterHandler("close_wallet", [this]() { CloseWallet(); }, 20, std::chrono::seconds(5));

    // Priority 30: Stop consensus
    shutdownManager.RegisterHandler(
        "stop_consensus",
        [this]()
        {
            if (consensus_)
            {
                consensus_->Stop();
            }
        },
        30, std::chrono::seconds(30));

    // Priority 40: Flush memory pool
    shutdownManager.RegisterHandler(
        "flush_mempool",
        [this]()
        {
            if (auto mempool = GetMemoryPool())
            {
                // Save unconfirmed transactions if needed
                mempool->Clear();
            }
        },
        40, std::chrono::seconds(10));

    // Priority 50: Stop P2P server
    shutdownManager.RegisterHandler(
        "stop_p2p",
        [this]()
        {
            if (p2p_server_)
            {
                p2p_server_->Stop();
            }
        },
        50, std::chrono::seconds(30));

    // Priority 60: Stop RPC server
    shutdownManager.RegisterHandler(
        "stop_rpc",
        [this]()
        {
            if (rpc_server_)
            {
                rpc_server_->Stop();
            }
        },
        60, std::chrono::seconds(10));

    // Priority 70: Stop monitoring
    shutdownManager.RegisterHandler(
        "stop_monitoring",
        [this]()
        {
            monitoring::PrometheusExporter::GetInstance().StopServer();
            monitoring::HealthCheckManager::GetInstance().StopPeriodicChecks();
        },
        70, std::chrono::seconds(5));

    // Priority 80: Flush and close storage
    shutdownManager.RegisterHandler(
        "close_storage",
        [this]()
        {
            if (store_)
            {
                store_->Flush();
                store_->Close();
            }
        },
        80, std::chrono::seconds(30));

    // Priority 90: Final cleanup
    shutdownManager.RegisterHandler(
        "final_cleanup",
        [this]()
        {
            running_ = false;
            if (status_thread_.joinable())
            {
                status_thread_.join();
            }
        },
        90, std::chrono::seconds(5));
}

void CLIService::InitializeMetrics()
{
    auto& exporter = monitoring::PrometheusExporter::GetInstance();

    // System metrics
    auto cpuUsage = PROMETHEUS_GAUGE("neo_cpu_usage_percent", "CPU usage percentage");
    auto memoryUsage = PROMETHEUS_GAUGE("neo_memory_usage_bytes", "Memory usage in bytes");
    auto diskUsage = PROMETHEUS_GAUGE("neo_disk_usage_bytes", "Disk usage in bytes");

    // Blockchain metrics
    auto blockHeight = PROMETHEUS_GAUGE("neo_block_height", "Current blockchain height");
    auto headerHeight = PROMETHEUS_GAUGE("neo_header_height", "Current header height");
    auto blockProcessingTime = PROMETHEUS_HISTOGRAM("neo_block_processing_seconds", "Time to process a block",
                                                    std::vector<double>{0.01, 0.05, 0.1, 0.5, 1, 5});

    // Network metrics
    auto peerCount = PROMETHEUS_GAUGE("neo_peer_count", "Number of connected peers");
    auto bytesReceived = PROMETHEUS_COUNTER("neo_bytes_received_total", "Total bytes received");
    auto bytesSent = PROMETHEUS_COUNTER("neo_bytes_sent_total", "Total bytes sent");

    // Transaction metrics
    auto mempoolSize = PROMETHEUS_GAUGE("neo_mempool_size", "Number of transactions in mempool");
    auto txProcessed = PROMETHEUS_COUNTER("neo_transactions_processed_total", "Total transactions processed");
    auto txFailed = PROMETHEUS_COUNTER("neo_transactions_failed_total", "Total transactions failed");

    // RPC metrics
    auto rpcRequests = PROMETHEUS_LABELED_COUNTER("neo_rpc_requests_total", "Total RPC requests", {"method"});
    auto rpcLatency = PROMETHEUS_LABELED_HISTOGRAM("neo_rpc_latency_seconds", "RPC request latency", {"method"});
    auto rpcErrors = PROMETHEUS_LABELED_COUNTER("neo_rpc_errors_total", "Total RPC errors", {"method", "error"});

    // Register all metrics
    exporter.RegisterMetric(cpuUsage);
    exporter.RegisterMetric(memoryUsage);
    exporter.RegisterMetric(diskUsage);
    exporter.RegisterMetric(blockHeight);
    exporter.RegisterMetric(headerHeight);
    exporter.RegisterMetric(blockProcessingTime);
    exporter.RegisterMetric(peerCount);
    exporter.RegisterMetric(bytesReceived);
    exporter.RegisterMetric(bytesSent);
    exporter.RegisterMetric(mempoolSize);
    exporter.RegisterMetric(txProcessed);
    exporter.RegisterMetric(txFailed);
    exporter.RegisterMetric(rpcRequests);
    exporter.RegisterMetric(rpcLatency);
    exporter.RegisterMetric(rpcErrors);

    // Update metrics periodically
    std::thread(
        [this, blockHeight, peerCount, mempoolSize]()
        {
            while (running_)
            {
                if (auto blockchain = GetBlockchain())
                {
                    blockHeight->Set(blockchain->GetHeight());
                }

                if (p2p_server_)
                {
                    peerCount->Set(p2p_server_->GetConnectedCount());
                }

                if (auto mempool = GetMemoryPool())
                {
                    mempoolSize->Set(mempool->GetCount());
                }

                std::this_thread::sleep_for(std::chrono::seconds(5));
            }
        })
        .detach();
}

void CLIService::InitializeHealthChecks()
{
    auto& healthManager = monitoring::HealthCheckManager::GetInstance();

    // Blockchain health check
    auto blockchainCheck = std::make_shared<monitoring::BlockchainHealthCheck>(
        [this]()
        {
            auto blockchain = GetBlockchain();
            return blockchain ? blockchain->GetHeight() : 0;
        },
        [this]()
        {
            auto blockchain = GetBlockchain();
            return blockchain ? blockchain->GetHeaderHeight() : 0;
        });

    // P2P health check
    auto p2pCheck = std::make_shared<monitoring::P2PHealthCheck>(
        [this]() { return p2p_server_ ? p2p_server_->GetConnectedCount() : 0; },
        3  // minimum peers
    );

    // Memory health check
    auto memoryCheck = std::make_shared<monitoring::MemoryHealthCheck>(
        []()
        {
            // Get current process memory usage
            size_t memoryUsage = 0;

#ifdef __linux__
            std::ifstream statusFile("/proc/self/status");
            std::string line;
            while (std::getline(statusFile, line))
            {
                if (line.substr(0, 6) == "VmRSS:")
                {
                    std::istringstream iss(line.substr(6));
                    size_t kb;
                    std::string unit;
                    iss >> kb >> unit;
                    memoryUsage = kb * 1024;  // Convert KB to bytes
                    break;
                }
            }
#elif defined(__APPLE__)
            struct rusage usage;
            if (getrusage(RUSAGE_SELF, &usage) == 0)
            {
                memoryUsage = usage.ru_maxrss;
#ifdef __APPLE__
// On macOS, ru_maxrss is in bytes
#else
                // On other BSD systems, it's in KB
                memoryUsage *= 1024;
#endif
            }
#elif defined(_WIN32)
            PROCESS_MEMORY_COUNTERS pmc;
            if (GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc)))
            {
                memoryUsage = pmc.WorkingSetSize;
            }
#else
            // Fallback: estimate based on available system memory
            memoryUsage = 512 * 1024 * 1024;  // 512MB default
#endif

            return memoryUsage;
        },
        8192  // 8GB max
    );

    // Custom RPC health check
    class RpcHealthCheck : public monitoring::HealthCheck
    {
      public:
        RpcHealthCheck(rpc::RpcServer* server) : HealthCheck("rpc"), server_(server) {}

        monitoring::HealthCheckResult Check() override
        {
            auto start = std::chrono::steady_clock::now();
            monitoring::HealthCheckResult result;
            result.name = name_;
            result.timestamp = std::chrono::system_clock::now();

            if (!server_ || !server_->IsRunning())
            {
                result.status = monitoring::HealthStatus::UNHEALTHY;
                result.message = "RPC server not running";
            }
            else
            {
                result.status = monitoring::HealthStatus::HEALTHY;
                result.message = "RPC server operational";
                result.details["port"] = std::to_string(server_->GetPort());
            }

            auto end = std::chrono::steady_clock::now();
            result.responseTime = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

            return result;
        }

      private:
        rpc::RpcServer* server_;
    };

    auto rpcCheck = std::make_shared<RpcHealthCheck>(rpc_server_.get());

    // Register health checks
    healthManager.RegisterHealthCheck(blockchainCheck);
    healthManager.RegisterHealthCheck(p2pCheck);
    healthManager.RegisterHealthCheck(memoryCheck);
    healthManager.RegisterHealthCheck(rpcCheck);
}

void CLIService::StartMonitoring()
{
    // Start Prometheus metrics server
    uint16_t metricsPort = core::Config::GetPort("ApplicationConfiguration.Prometheus.Port", 9090);
    monitoring::PrometheusExporter::GetInstance().StartServer(metricsPort);
    std::cout << "Prometheus metrics available at http://localhost:" << metricsPort << "/metrics" << std::endl;

    // Start periodic health checks
    monitoring::HealthCheckManager::GetInstance().StartPeriodicChecks(std::chrono::seconds(30));

    // Initialize rate limiter
    rateLimiter_ = std::make_unique<rpc::MethodRateLimiter>();

    // Initialize connection limits
    network::ConnectionLimits::Config limitsConfig;
    limitsConfig.maxConnectionsPerIP =
        core::Config::GetUInt32("ApplicationConfiguration.P2P.MaxConnectionsPerAddress", 3);
    limitsConfig.maxTotalConnections = core::Config::GetUInt32("ApplicationConfiguration.P2P.MaxConnections", 40);
    connectionLimits_ = std::make_unique<network::ConnectionLimits>(limitsConfig);

    // Initialize timeout manager
    timeoutManager_ = std::make_unique<network::TimeoutManager>();
    timeoutManager_->Start();
}

}  // namespace neo::cli
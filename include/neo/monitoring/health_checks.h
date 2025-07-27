#pragma once

#include <filesystem>
#include <memory>
#include <neo/monitoring/metrics.h>
#include <neo/network/p2p/local_node.h>
#include <neo/persistence/data_cache.h>

namespace neo::monitoring
{
/**
 * @brief Database health check
 */
class DatabaseHealthCheck : public IHealthCheck
{
  private:
    std::shared_ptr<persistence::DataCache> db_;

  public:
    explicit DatabaseHealthCheck(std::shared_ptr<persistence::DataCache> db) : db_(db) {}

    HealthCheckResult Check() override
    {
        auto start = std::chrono::steady_clock::now();
        HealthCheckResult result;
        result.details["type"] = "database";

        try
        {
            if (!db_)
            {
                result.status = HealthStatus::Unhealthy;
                result.message = "Database not initialized";
            }
            else
            {
                // Try a simple read operation
                auto height = db_->GetHeight();
                result.status = HealthStatus::Healthy;
                result.message = "Database operational";
                result.details["height"] = std::to_string(height);
            }
        }
        catch (const std::exception& e)
        {
            result.status = HealthStatus::Unhealthy;
            result.message = std::string("Database error: ") + e.what();
        }

        auto end = std::chrono::steady_clock::now();
        result.duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        return result;
    }

    std::string GetName() const override
    {
        return "database";
    }
};

/**
 * @brief Network connectivity health check
 */
class NetworkHealthCheck : public IHealthCheck
{
  private:
    std::shared_ptr<network::p2p::LocalNode> node_;
    size_t min_peers_;

  public:
    explicit NetworkHealthCheck(std::shared_ptr<network::p2p::LocalNode> node, size_t min_peers = 3)
        : node_(node), min_peers_(min_peers)
    {
    }

    HealthCheckResult Check() override
    {
        auto start = std::chrono::steady_clock::now();
        HealthCheckResult result;
        result.details["type"] = "network";

        try
        {
            if (!node_)
            {
                result.status = HealthStatus::Unhealthy;
                result.message = "Network node not initialized";
            }
            else
            {
                size_t peer_count = node_->GetConnectedPeerCount();
                result.details["peer_count"] = std::to_string(peer_count);
                result.details["min_peers"] = std::to_string(min_peers_);

                if (peer_count == 0)
                {
                    result.status = HealthStatus::Unhealthy;
                    result.message = "No connected peers";
                }
                else if (peer_count < min_peers_)
                {
                    result.status = HealthStatus::Degraded;
                    result.message = "Below minimum peer count";
                }
                else
                {
                    result.status = HealthStatus::Healthy;
                    result.message = "Network connectivity good";
                }
            }
        }
        catch (const std::exception& e)
        {
            result.status = HealthStatus::Unhealthy;
            result.message = std::string("Network error: ") + e.what();
        }

        auto end = std::chrono::steady_clock::now();
        result.duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        return result;
    }

    std::string GetName() const override
    {
        return "network";
    }
};

/**
 * @brief Disk space health check
 */
class DiskSpaceHealthCheck : public IHealthCheck
{
  private:
    std::string path_;
    uint64_t min_free_bytes_;
    uint64_t warning_free_bytes_;

  public:
    explicit DiskSpaceHealthCheck(const std::string& path, uint64_t min_free_gb = 10, uint64_t warning_free_gb = 50)
        : path_(path), min_free_bytes_(min_free_gb * 1024ULL * 1024ULL * 1024ULL),
          warning_free_bytes_(warning_free_gb * 1024ULL * 1024ULL * 1024ULL)
    {
    }

    HealthCheckResult Check() override
    {
        auto start = std::chrono::steady_clock::now();
        HealthCheckResult result;
        result.details["type"] = "disk_space";
        result.details["path"] = path_;

        try
        {
            auto space = std::filesystem::space(path_);
            uint64_t free_bytes = space.available;
            uint64_t total_bytes = space.capacity;
            double free_percentage = (double)free_bytes / total_bytes * 100.0;

            result.details["free_gb"] = std::to_string(free_bytes / (1024ULL * 1024ULL * 1024ULL));
            result.details["total_gb"] = std::to_string(total_bytes / (1024ULL * 1024ULL * 1024ULL));
            result.details["free_percentage"] = std::to_string(free_percentage);

            if (free_bytes < min_free_bytes_)
            {
                result.status = HealthStatus::Unhealthy;
                result.message = "Critical: Low disk space";
            }
            else if (free_bytes < warning_free_bytes_)
            {
                result.status = HealthStatus::Degraded;
                result.message = "Warning: Disk space running low";
            }
            else
            {
                result.status = HealthStatus::Healthy;
                result.message = "Disk space adequate";
            }
        }
        catch (const std::exception& e)
        {
            result.status = HealthStatus::Unhealthy;
            result.message = std::string("Disk check error: ") + e.what();
        }

        auto end = std::chrono::steady_clock::now();
        result.duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        return result;
    }

    std::string GetName() const override
    {
        return "disk_space";
    }
};

/**
 * @brief Memory usage health check
 */
class MemoryHealthCheck : public IHealthCheck
{
  private:
    double max_usage_percentage_;
    double warning_usage_percentage_;

  public:
    explicit MemoryHealthCheck(double max_usage_percentage = 90.0, double warning_usage_percentage = 80.0)
        : max_usage_percentage_(max_usage_percentage), warning_usage_percentage_(warning_usage_percentage)
    {
    }

    HealthCheckResult Check() override;
    std::string GetName() const override
    {
        return "memory";
    }
};

/**
 * @brief Consensus health check
 */
class ConsensusHealthCheck : public IHealthCheck
{
  private:
    std::shared_ptr<consensus::DbftConsensus> consensus_;

  public:
    explicit ConsensusHealthCheck(std::shared_ptr<consensus::DbftConsensus> consensus) : consensus_(consensus) {}

    HealthCheckResult Check() override
    {
        auto start = std::chrono::steady_clock::now();
        HealthCheckResult result;
        result.details["type"] = "consensus";

        try
        {
            if (!consensus_)
            {
                result.status = HealthStatus::Unhealthy;
                result.message = "Consensus not initialized";
            }
            else
            {
                auto state = consensus_->GetState();
                result.details["view_number"] = std::to_string(state.GetViewNumber());
                result.details["block_index"] = std::to_string(state.GetBlockIndex());

                result.status = HealthStatus::Healthy;
                result.message = "Consensus operational";
            }
        }
        catch (const std::exception& e)
        {
            result.status = HealthStatus::Unhealthy;
            result.message = std::string("Consensus error: ") + e.what();
        }

        auto end = std::chrono::steady_clock::now();
        result.duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        return result;
    }

    std::string GetName() const override
    {
        return "consensus";
    }
};

/**
 * @brief Blockchain sync health check
 */
class BlockchainSyncHealthCheck : public IHealthCheck
{
  private:
    std::shared_ptr<persistence::DataCache> db_;
    std::shared_ptr<network::p2p::LocalNode> node_;
    uint32_t max_blocks_behind_;

  public:
    explicit BlockchainSyncHealthCheck(std::shared_ptr<persistence::DataCache> db,
                                       std::shared_ptr<network::p2p::LocalNode> node, uint32_t max_blocks_behind = 10)
        : db_(db), node_(node), max_blocks_behind_(max_blocks_behind)
    {
    }

    HealthCheckResult Check() override;
    std::string GetName() const override
    {
        return "blockchain_sync";
    }
};
}  // namespace neo::monitoring
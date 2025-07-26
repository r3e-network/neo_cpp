#pragma once

#include <string>
#include <vector>
#include <functional>
#include <chrono>
#include <atomic>
#include <mutex>
#include <thread>
#include <unordered_map>
#include <nlohmann/json.hpp>

namespace neo::monitoring {

/**
 * @brief Health check status
 */
enum class HealthStatus {
    HEALTHY,
    DEGRADED,
    UNHEALTHY
};

/**
 * @brief Health check result
 */
struct HealthCheckResult {
    std::string name;
    HealthStatus status;
    std::string message;
    std::chrono::milliseconds responseTime;
    std::chrono::system_clock::time_point timestamp;
    std::unordered_map<std::string, std::string> details;
    
    nlohmann::json ToJson() const {
        nlohmann::json j;
        j["name"] = name;
        j["status"] = StatusToString(status);
        j["message"] = message;
        j["responseTime"] = responseTime.count();
        j["timestamp"] = std::chrono::duration_cast<std::chrono::seconds>(
            timestamp.time_since_epoch()).count();
        
        if (!details.empty()) {
            j["details"] = details;
        }
        
        return j;
    }
    
private:
    static std::string StatusToString(HealthStatus status) {
        switch (status) {
            case HealthStatus::HEALTHY: return "healthy";
            case HealthStatus::DEGRADED: return "degraded";
            case HealthStatus::UNHEALTHY: return "unhealthy";
        }
        return "unknown";
    }
};

/**
 * @brief Base class for health checks
 */
class HealthCheck {
public:
    explicit HealthCheck(const std::string& name) : name_(name) {}
    virtual ~HealthCheck() = default;
    
    /**
     * @brief Perform health check
     * @return Health check result
     */
    virtual HealthCheckResult Check() = 0;
    
    const std::string& GetName() const { return name_; }
    
protected:
    std::string name_;
};

/**
 * @brief Health check that verifies blockchain sync status
 */
class BlockchainHealthCheck : public HealthCheck {
public:
    BlockchainHealthCheck(std::function<uint32_t()> getHeight,
                         std::function<uint32_t()> getHeaderHeight)
        : HealthCheck("blockchain"),
          getHeight_(getHeight),
          getHeaderHeight_(getHeaderHeight) {
    }
    
    HealthCheckResult Check() override {
        auto start = std::chrono::steady_clock::now();
        HealthCheckResult result;
        result.name = name_;
        result.timestamp = std::chrono::system_clock::now();
        
        try {
            uint32_t height = getHeight_();
            uint32_t headerHeight = getHeaderHeight_();
            
            result.details["height"] = std::to_string(height);
            result.details["headerHeight"] = std::to_string(headerHeight);
            
            // Check if syncing
            if (height == 0) {
                result.status = HealthStatus::UNHEALTHY;
                result.message = "Blockchain not initialized";
            } else if (headerHeight > height + 100) {
                result.status = HealthStatus::DEGRADED;
                result.message = "Blockchain syncing";
                result.details["syncProgress"] = std::to_string(
                    (height * 100.0) / headerHeight) + "%";
            } else {
                result.status = HealthStatus::HEALTHY;
                result.message = "Blockchain synced";
            }
            
        } catch (const std::exception& e) {
            result.status = HealthStatus::UNHEALTHY;
            result.message = "Failed to check blockchain: " + std::string(e.what());
        }
        
        auto end = std::chrono::steady_clock::now();
        result.responseTime = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        
        return result;
    }
    
private:
    std::function<uint32_t()> getHeight_;
    std::function<uint32_t()> getHeaderHeight_;
};

/**
 * @brief Health check for P2P connectivity
 */
class P2PHealthCheck : public HealthCheck {
public:
    P2PHealthCheck(std::function<size_t()> getPeerCount,
                   size_t minPeers = 3)
        : HealthCheck("p2p"),
          getPeerCount_(getPeerCount),
          minPeers_(minPeers) {
    }
    
    HealthCheckResult Check() override {
        auto start = std::chrono::steady_clock::now();
        HealthCheckResult result;
        result.name = name_;
        result.timestamp = std::chrono::system_clock::now();
        
        try {
            size_t peerCount = getPeerCount_();
            result.details["peerCount"] = std::to_string(peerCount);
            result.details["minPeers"] = std::to_string(minPeers_);
            
            if (peerCount == 0) {
                result.status = HealthStatus::UNHEALTHY;
                result.message = "No peers connected";
            } else if (peerCount < minPeers_) {
                result.status = HealthStatus::DEGRADED;
                result.message = "Insufficient peers";
            } else {
                result.status = HealthStatus::HEALTHY;
                result.message = "P2P network healthy";
            }
            
        } catch (const std::exception& e) {
            result.status = HealthStatus::UNHEALTHY;
            result.message = "Failed to check P2P: " + std::string(e.what());
        }
        
        auto end = std::chrono::steady_clock::now();
        result.responseTime = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        
        return result;
    }
    
private:
    std::function<size_t()> getPeerCount_;
    size_t minPeers_;
};

/**
 * @brief Health check for memory usage
 */
class MemoryHealthCheck : public HealthCheck {
public:
    MemoryHealthCheck(std::function<size_t()> getMemoryUsage,
                      size_t maxMemoryMB = 8192)
        : HealthCheck("memory"),
          getMemoryUsage_(getMemoryUsage),
          maxMemoryMB_(maxMemoryMB) {
    }
    
    HealthCheckResult Check() override {
        auto start = std::chrono::steady_clock::now();
        HealthCheckResult result;
        result.name = name_;
        result.timestamp = std::chrono::system_clock::now();
        
        try {
            size_t memoryMB = getMemoryUsage_() / (1024 * 1024);
            double usagePercent = (memoryMB * 100.0) / maxMemoryMB_;
            
            result.details["memoryMB"] = std::to_string(memoryMB);
            result.details["maxMemoryMB"] = std::to_string(maxMemoryMB_);
            result.details["usagePercent"] = std::to_string(usagePercent);
            
            if (usagePercent > 95) {
                result.status = HealthStatus::UNHEALTHY;
                result.message = "Memory usage critical";
            } else if (usagePercent > 80) {
                result.status = HealthStatus::DEGRADED;
                result.message = "Memory usage high";
            } else {
                result.status = HealthStatus::HEALTHY;
                result.message = "Memory usage normal";
            }
            
        } catch (const std::exception& e) {
            result.status = HealthStatus::UNHEALTHY;
            result.message = "Failed to check memory: " + std::string(e.what());
        }
        
        auto end = std::chrono::steady_clock::now();
        result.responseTime = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        
        return result;
    }
    
private:
    std::function<size_t()> getMemoryUsage_;
    size_t maxMemoryMB_;
};

/**
 * @brief Health check manager
 */
class HealthCheckManager {
public:
    static HealthCheckManager& GetInstance() {
        static HealthCheckManager instance;
        return instance;
    }
    
    /**
     * @brief Register a health check
     * @param check Health check to register
     */
    void RegisterHealthCheck(std::shared_ptr<HealthCheck> check) {
        std::lock_guard<std::mutex> lock(mutex_);
        checks_[check->GetName()] = check;
    }
    
    /**
     * @brief Run all health checks
     * @return Overall health status
     */
    HealthStatus RunChecks() {
        std::lock_guard<std::mutex> lock(mutex_);
        
        lastResults_.clear();
        HealthStatus overallStatus = HealthStatus::HEALTHY;
        
        for (const auto& [name, check] : checks_) {
            auto result = check->Check();
            lastResults_[name] = result;
            
            // Update overall status
            if (result.status == HealthStatus::UNHEALTHY) {
                overallStatus = HealthStatus::UNHEALTHY;
            } else if (result.status == HealthStatus::DEGRADED && 
                      overallStatus != HealthStatus::UNHEALTHY) {
                overallStatus = HealthStatus::DEGRADED;
            }
        }
        
        lastCheckTime_ = std::chrono::system_clock::now();
        return overallStatus;
    }
    
    /**
     * @brief Get last health check results
     * @return Map of results by check name
     */
    std::unordered_map<std::string, HealthCheckResult> GetLastResults() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return lastResults_;
    }
    
    /**
     * @brief Get health status as JSON
     * @return JSON representation
     */
    nlohmann::json GetHealthJson() {
        RunChecks(); // Update results
        
        std::lock_guard<std::mutex> lock(mutex_);
        
        nlohmann::json j;
        j["status"] = StatusToString(GetOverallStatus());
        j["timestamp"] = std::chrono::duration_cast<std::chrono::seconds>(
            lastCheckTime_.time_since_epoch()).count();
        
        nlohmann::json checks = nlohmann::json::array();
        for (const auto& [name, result] : lastResults_) {
            checks.push_back(result.ToJson());
        }
        j["checks"] = checks;
        
        return j;
    }
    
    /**
     * @brief Start periodic health checks
     * @param interval Check interval
     */
    void StartPeriodicChecks(std::chrono::seconds interval = std::chrono::seconds(30)) {
        if (running_) return;
        
        running_ = true;
        checkThread_ = std::thread([this, interval]() {
            while (running_) {
                RunChecks();
                std::this_thread::sleep_for(interval);
            }
        });
    }
    
    /**
     * @brief Stop periodic health checks
     */
    void StopPeriodicChecks() {
        running_ = false;
        if (checkThread_.joinable()) {
            checkThread_.join();
        }
    }
    
    ~HealthCheckManager() {
        StopPeriodicChecks();
    }
    
private:
    HealthCheckManager() = default;
    
    HealthStatus GetOverallStatus() const {
        HealthStatus overall = HealthStatus::HEALTHY;
        
        for (const auto& [name, result] : lastResults_) {
            if (result.status == HealthStatus::UNHEALTHY) {
                return HealthStatus::UNHEALTHY;
            } else if (result.status == HealthStatus::DEGRADED) {
                overall = HealthStatus::DEGRADED;
            }
        }
        
        return overall;
    }
    
    static std::string StatusToString(HealthStatus status) {
        switch (status) {
            case HealthStatus::HEALTHY: return "healthy";
            case HealthStatus::DEGRADED: return "degraded";
            case HealthStatus::UNHEALTHY: return "unhealthy";
        }
        return "unknown";
    }
    
    std::unordered_map<std::string, std::shared_ptr<HealthCheck>> checks_;
    std::unordered_map<std::string, HealthCheckResult> lastResults_;
    std::chrono::system_clock::time_point lastCheckTime_;
    
    std::atomic<bool> running_{false};
    std::thread checkThread_;
    mutable std::mutex mutex_;
};

} // namespace neo::monitoring
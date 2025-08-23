#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#include <chrono>
#include <atomic>
#include <mutex>

namespace neo::core {

/**
 * @brief Performance optimization categories
 */
enum class OptimizationCategory {
    MemoryPool,
    Blockchain,
    VirtualMachine,
    Cryptography,
    Network,
    Storage,
    RPC
};

/**
 * @brief Performance metrics collection
 */
struct PerformanceMetrics {
    std::atomic<uint64_t> operations_count{0};
    std::atomic<uint64_t> total_time_microseconds{0};
    std::atomic<uint64_t> min_time_microseconds{UINT64_MAX};
    std::atomic<uint64_t> max_time_microseconds{0};
    std::atomic<uint64_t> error_count{0};
    
    double GetAverageTimeMs() const {
        uint64_t count = operations_count.load();
        if (count == 0) return 0.0;
        return (total_time_microseconds.load() / static_cast<double>(count)) / 1000.0;
    }
    
    double GetMinTimeMs() const {
        uint64_t min_us = min_time_microseconds.load();
        return min_us == UINT64_MAX ? 0.0 : min_us / 1000.0;
    }
    
    double GetMaxTimeMs() const {
        return max_time_microseconds.load() / 1000.0;
    }
    
    double GetErrorRate() const {
        uint64_t count = operations_count.load();
        if (count == 0) return 0.0;
        return (error_count.load() / static_cast<double>(count)) * 100.0;
    }
    
    void Reset() {
        operations_count = 0;
        total_time_microseconds = 0;
        min_time_microseconds = UINT64_MAX;
        max_time_microseconds = 0;
        error_count = 0;
    }
};

/**
 * @brief Performance bottleneck information
 */
struct PerformanceBottleneck {
    std::string component;
    std::string operation;
    OptimizationCategory category;
    double average_time_ms;
    uint64_t call_frequency;
    double impact_score;  // Calculated as frequency * average_time
    std::string recommendation;
    
    PerformanceBottleneck(const std::string& comp, const std::string& op, 
                         OptimizationCategory cat, double avg_time, 
                         uint64_t frequency, const std::string& rec = "")
        : component(comp), operation(op), category(cat), 
          average_time_ms(avg_time), call_frequency(frequency), 
          impact_score(frequency * avg_time), recommendation(rec) {}
};

/**
 * @brief Performance optimization recommendations
 */
struct OptimizationRecommendation {
    OptimizationCategory category;
    std::string title;
    std::string description;
    std::vector<std::string> action_items;
    double expected_improvement_percent;
    std::string implementation_difficulty;  // "Low", "Medium", "High"
    
    OptimizationRecommendation(OptimizationCategory cat, const std::string& t, 
                              const std::string& desc, double improvement = 0.0,
                              const std::string& difficulty = "Medium")
        : category(cat), title(t), description(desc), 
          expected_improvement_percent(improvement), 
          implementation_difficulty(difficulty) {}
};

/**
 * @brief Performance optimizer for identifying and resolving bottlenecks
 * 
 * Monitors system performance, identifies bottlenecks, and provides
 * optimization recommendations for Neo C++ blockchain implementation.
 */
class PerformanceOptimizer {
public:
    /**
     * @brief Constructor
     */
    PerformanceOptimizer();
    
    /**
     * @brief Destructor
     */
    ~PerformanceOptimizer();
    
    /**
     * @brief Start performance monitoring
     */
    void StartMonitoring();
    
    /**
     * @brief Stop performance monitoring
     */
    void StopMonitoring();
    
    /**
     * @brief Record operation performance
     * @param component Component name
     * @param operation Operation name
     * @param category Performance category
     * @param duration_microseconds Operation duration
     * @param success Operation success status
     */
    void RecordOperation(const std::string& component, 
                        const std::string& operation,
                        OptimizationCategory category,
                        uint64_t duration_microseconds,
                        bool success = true);
    
    /**
     * @brief Get performance metrics for component
     * @param component Component name
     * @return Performance metrics
     */
    PerformanceMetrics GetMetrics(const std::string& component) const;
    
    /**
     * @brief Get all performance metrics
     * @return Map of component to metrics
     */
    std::unordered_map<std::string, PerformanceMetrics> GetAllMetrics() const;
    
    /**
     * @brief Identify performance bottlenecks
     * @param top_n Number of top bottlenecks to return
     * @return Vector of bottlenecks sorted by impact
     */
    std::vector<PerformanceBottleneck> IdentifyBottlenecks(size_t top_n = 10) const;
    
    /**
     * @brief Get optimization recommendations
     * @param category Optional category filter
     * @return Vector of recommendations
     */
    std::vector<OptimizationRecommendation> GetOptimizationRecommendations(
        OptimizationCategory category = OptimizationCategory::MemoryPool) const;
    
    /**
     * @brief Generate performance report
     * @param include_recommendations Include optimization recommendations
     * @return Performance report as string
     */
    std::string GeneratePerformanceReport(bool include_recommendations = true) const;
    
    /**
     * @brief Reset all performance metrics
     */
    void ResetMetrics();
    
    /**
     * @brief Enable/disable specific category monitoring
     * @param category Category to control
     * @param enabled Enable or disable
     */
    void SetCategoryEnabled(OptimizationCategory category, bool enabled);
    
    /**
     * @brief Check if category monitoring is enabled
     * @param category Category to check
     * @return true if enabled
     */
    bool IsCategoryEnabled(OptimizationCategory category) const;

private:
    /**
     * @brief Initialize default optimization recommendations
     */
    void InitializeRecommendations();
    
    /**
     * @brief Calculate impact score for bottleneck
     * @param metrics Performance metrics
     * @return Impact score
     */
    double CalculateImpactScore(const PerformanceMetrics& metrics) const;
    
    /**
     * @brief Get category-specific recommendations
     * @param category Optimization category
     * @return Vector of recommendations
     */
    std::vector<OptimizationRecommendation> GetCategoryRecommendations(
        OptimizationCategory category) const;

private:
    mutable std::mutex metrics_mutex_;
    std::unordered_map<std::string, PerformanceMetrics> component_metrics_;
    std::unordered_map<OptimizationCategory, bool> category_enabled_;
    
    // Optimization recommendations database
    std::vector<OptimizationRecommendation> optimization_recommendations_;
    
    // Performance thresholds
    static constexpr double HIGH_LATENCY_THRESHOLD_MS = 100.0;
    static constexpr double MEDIUM_LATENCY_THRESHOLD_MS = 50.0;
    static constexpr uint64_t HIGH_FREQUENCY_THRESHOLD = 1000;
    static constexpr double HIGH_ERROR_RATE_THRESHOLD = 5.0;
    
    std::atomic<bool> monitoring_enabled_{false};
};

/**
 * @brief RAII performance timer for automatic measurement
 */
class PerformanceTimer {
public:
    PerformanceTimer(PerformanceOptimizer& optimizer,
                    const std::string& component,
                    const std::string& operation,
                    OptimizationCategory category)
        : optimizer_(optimizer), component_(component), 
          operation_(operation), category_(category),
          start_time_(std::chrono::high_resolution_clock::now()) {}
    
    ~PerformanceTimer() {
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(
            end_time - start_time_).count();
        
        optimizer_.RecordOperation(component_, operation_, category_, 
                                 static_cast<uint64_t>(duration), success_);
    }
    
    void SetSuccess(bool success) { success_ = success; }

private:
    PerformanceOptimizer& optimizer_;
    std::string component_;
    std::string operation_;
    OptimizationCategory category_;
    std::chrono::high_resolution_clock::time_point start_time_;
    bool success_{true};
};

/**
 * @brief Macro for easy performance measurement
 */
#define MEASURE_PERFORMANCE(optimizer, component, operation, category) \
    PerformanceTimer timer(optimizer, component, operation, category)

/**
 * @brief Get optimization category name
 * @param category Optimization category
 * @return Category name
 */
std::string GetOptimizationCategoryName(OptimizationCategory category);

} // namespace neo::core
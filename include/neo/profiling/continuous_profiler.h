/**
 * @file continuous_profiler.h
 * @brief Continuous profiling system for Neo C++
 * @details Provides always-on profiling with minimal overhead
 */

#pragma once

#include <atomic>
#include <chrono>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

namespace neo::profiling {

/**
 * @brief Sample types for profiling
 */
enum class SampleType {
    CPU,           // CPU usage samples
    Memory,        // Memory allocation samples
    IO,            // I/O operation samples
    Network,       // Network operation samples
    Lock,          // Lock contention samples
    GC,            // Garbage collection samples (if applicable)
    Custom         // User-defined samples
};

/**
 * @brief Profile sample data
 */
struct ProfileSample {
    SampleType type;
    std::chrono::steady_clock::time_point timestamp;
    std::string function_name;
    std::string file_name;
    int line_number;
    uint64_t value;  // Type-specific value (e.g., microseconds, bytes)
    std::thread::id thread_id;
    std::map<std::string, std::string> metadata;
};

/**
 * @brief Call stack frame for profiling
 */
struct StackFrame {
    std::string function_name;
    std::string file_name;
    int line_number;
    uintptr_t address;
};

/**
 * @brief Continuous profiler for production systems
 */
class ContinuousProfiler {
public:
    struct Config {
        bool enable_cpu_profiling = true;
        bool enable_memory_profiling = true;
        bool enable_io_profiling = true;
        bool enable_network_profiling = true;
        bool enable_lock_profiling = true;
        
        std::chrono::milliseconds sampling_interval{10};  // 10ms default
        size_t max_samples_per_type = 100000;  // Ring buffer size
        size_t stack_trace_depth = 20;
        
        bool enable_compression = true;
        bool enable_symbolication = true;
        
        std::string output_directory = "./profiles";
        std::chrono::minutes profile_rotation_interval{60};
    };
    
    explicit ContinuousProfiler(const Config& config = Config());
    ~ContinuousProfiler();
    
    // Profiler control
    void Start();
    void Stop();
    bool IsRunning() const { return running_.load(); }
    
    // Manual sampling
    void RecordSample(const ProfileSample& sample);
    void RecordCPUSample(const std::string& function_name, uint64_t microseconds);
    void RecordMemorySample(const std::string& function_name, size_t bytes);
    void RecordIOSample(const std::string& function_name, uint64_t bytes, uint64_t microseconds);
    void RecordNetworkSample(const std::string& function_name, uint64_t bytes, uint64_t microseconds);
    void RecordLockSample(const std::string& function_name, uint64_t wait_microseconds);
    
    // Stack trace capture
    std::vector<StackFrame> CaptureStackTrace(size_t max_depth = 20);
    
    // Profile data access
    struct ProfileData {
        std::vector<ProfileSample> samples;
        std::chrono::steady_clock::time_point start_time;
        std::chrono::steady_clock::time_point end_time;
        std::map<std::string, uint64_t> function_totals;
        std::map<std::string, uint64_t> function_counts;
    };
    
    ProfileData GetProfileData(SampleType type) const;
    ProfileData GetAggregatedProfile() const;
    
    // Export formats
    void ExportPprof(const std::string& filename);  // Google pprof format
    void ExportFlameGraph(const std::string& filename);  // FlameGraph format
    void ExportJSON(const std::string& filename);  // JSON format
    void ExportCSV(const std::string& filename);  // CSV format
    
    // Analysis
    struct HotSpot {
        std::string function_name;
        uint64_t total_time_us;
        uint64_t call_count;
        double percentage;
    };
    
    std::vector<HotSpot> GetHotSpots(SampleType type, size_t top_n = 10) const;
    
    // Memory leak detection
    struct MemoryLeak {
        std::string allocation_site;
        size_t leaked_bytes;
        size_t allocation_count;
        std::vector<StackFrame> stack_trace;
    };
    
    std::vector<MemoryLeak> DetectMemoryLeaks() const;
    
    // Performance regression detection
    bool DetectRegression(const ProfileData& baseline, double threshold = 0.1) const;
    
private:
    Config config_;
    std::atomic<bool> running_{false};
    std::thread sampling_thread_;
    std::thread analysis_thread_;
    
    // Ring buffers for samples
    struct RingBuffer {
        std::vector<ProfileSample> samples;
        std::atomic<size_t> write_index{0};
        size_t capacity;
        mutable std::mutex mutex;
        
        explicit RingBuffer(size_t cap) : samples(cap), capacity(cap) {}
        void Add(const ProfileSample& sample);
        std::vector<ProfileSample> GetAll() const;
    };
    
    std::unordered_map<SampleType, std::unique_ptr<RingBuffer>> sample_buffers_;
    
    // Sampling implementation
    void SamplingLoop();
    void AnalysisLoop();
    
    void SampleCPU();
    void SampleMemory();
    void SampleIO();
    void SampleNetwork();
    void SampleLocks();
    
    // Profile rotation
    void RotateProfiles();
    std::chrono::steady_clock::time_point last_rotation_;
    
    // Symbolication cache
    mutable std::unordered_map<uintptr_t, std::string> symbol_cache_;
    std::string Symbolicate(uintptr_t address) const;
};

/**
 * @brief RAII profiling scope
 */
class ProfileScope {
public:
    ProfileScope(ContinuousProfiler& profiler, 
                const std::string& function_name,
                SampleType type = SampleType::CPU);
    ~ProfileScope();
    
    // Disable copy
    ProfileScope(const ProfileScope&) = delete;
    ProfileScope& operator=(const ProfileScope&) = delete;
    
    // Enable move
    ProfileScope(ProfileScope&&) = default;
    ProfileScope& operator=(ProfileScope&&) = default;
    
private:
    ContinuousProfiler& profiler_;
    std::string function_name_;
    SampleType type_;
    std::chrono::steady_clock::time_point start_time_;
};

/**
 * @brief Macros for easy profiling
 */
#define PROFILE_FUNCTION(profiler) \
    neo::profiling::ProfileScope _profile_scope(profiler, __FUNCTION__)

#define PROFILE_BLOCK(profiler, name) \
    neo::profiling::ProfileScope _profile_scope(profiler, name)

#define PROFILE_CPU(profiler, name) \
    neo::profiling::ProfileScope _profile_scope(profiler, name, neo::profiling::SampleType::CPU)

#define PROFILE_MEMORY(profiler, name) \
    neo::profiling::ProfileScope _profile_scope(profiler, name, neo::profiling::SampleType::Memory)

/**
 * @brief Global profiler instance
 */
class GlobalProfiler {
public:
    static ContinuousProfiler& Instance() {
        static ContinuousProfiler profiler;
        return profiler;
    }
    
    static void Configure(const ContinuousProfiler::Config& config) {
        Instance().Stop();
        instance_config_ = config;
        Instance() = ContinuousProfiler(config);
    }
    
    static void Start() { Instance().Start(); }
    static void Stop() { Instance().Stop(); }
    
private:
    static ContinuousProfiler::Config instance_config_;
};

/**
 * @brief Memory allocation hooks for profiling
 */
class MemoryProfiler {
public:
    static void* ProfiledMalloc(size_t size, const char* file, int line);
    static void ProfiledFree(void* ptr);
    static void* ProfiledRealloc(void* ptr, size_t size, const char* file, int line);
    
    struct AllocationInfo {
        size_t size;
        std::string file;
        int line;
        std::chrono::steady_clock::time_point timestamp;
        std::vector<StackFrame> stack_trace;
    };
    
    static std::unordered_map<void*, AllocationInfo> GetAllocations();
    static size_t GetTotalAllocatedBytes();
    static size_t GetAllocationCount();
    
private:
    static std::unordered_map<void*, AllocationInfo> allocations_;
    static std::mutex allocation_mutex_;
    static std::atomic<size_t> total_allocated_;
    static std::atomic<size_t> allocation_count_;
};

/**
 * @brief Profiling statistics
 */
struct ProfilingStats {
    uint64_t total_samples;
    uint64_t samples_dropped;  // Due to buffer overflow
    double overhead_percentage;
    std::chrono::milliseconds profiling_duration;
    
    std::map<SampleType, uint64_t> samples_by_type;
    std::map<std::string, double> overhead_by_component;
};

ProfilingStats GetProfilingStats();

} // namespace neo::profiling
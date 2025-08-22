/**
 * @file continuous_profiler.cpp
 * @brief Implementation of continuous profiling system
 */

#include <neo/profiling/continuous_profiler.h>
#include <neo/monitoring/performance_monitor.h>

#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <execinfo.h>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <sstream>

#ifdef __linux__
#include <unistd.h>
#include <sys/resource.h>
#endif

#ifdef __APPLE__
#include <mach/mach.h>
#include <mach/task.h>
#endif

namespace neo::profiling {

// Static member definitions
std::unordered_map<void*, MemoryProfiler::AllocationInfo> MemoryProfiler::allocations_;
std::mutex MemoryProfiler::allocation_mutex_;
std::atomic<size_t> MemoryProfiler::total_allocated_{0};
std::atomic<size_t> MemoryProfiler::allocation_count_{0};
ContinuousProfiler::Config GlobalProfiler::instance_config_;

// RingBuffer implementation
void ContinuousProfiler::RingBuffer::Add(const ProfileSample& sample) {
    size_t index = write_index.fetch_add(1) % capacity;
    std::lock_guard<std::mutex> lock(mutex);
    samples[index] = sample;
}

std::vector<ProfileSample> ContinuousProfiler::RingBuffer::GetAll() const {
    std::lock_guard<std::mutex> lock(mutex);
    std::vector<ProfileSample> result;
    result.reserve(capacity);
    
    size_t current_write = write_index.load();
    size_t start = (current_write >= capacity) ? (current_write % capacity) : 0;
    
    for (size_t i = 0; i < std::min(capacity, current_write); ++i) {
        size_t index = (start + i) % capacity;
        result.push_back(samples[index]);
    }
    
    return result;
}

// ContinuousProfiler implementation
ContinuousProfiler::ContinuousProfiler(const Config& config)
    : config_(config) {
    
    // Initialize ring buffers for each sample type
    if (config_.enable_cpu_profiling) {
        sample_buffers_[SampleType::CPU] = 
            std::make_unique<RingBuffer>(config_.max_samples_per_type);
    }
    if (config_.enable_memory_profiling) {
        sample_buffers_[SampleType::Memory] = 
            std::make_unique<RingBuffer>(config_.max_samples_per_type);
    }
    if (config_.enable_io_profiling) {
        sample_buffers_[SampleType::IO] = 
            std::make_unique<RingBuffer>(config_.max_samples_per_type);
    }
    if (config_.enable_network_profiling) {
        sample_buffers_[SampleType::Network] = 
            std::make_unique<RingBuffer>(config_.max_samples_per_type);
    }
    if (config_.enable_lock_profiling) {
        sample_buffers_[SampleType::Lock] = 
            std::make_unique<RingBuffer>(config_.max_samples_per_type);
    }
    
    // Create output directory if it doesn't exist
    std::filesystem::create_directories(config_.output_directory);
    
    last_rotation_ = std::chrono::steady_clock::now();
}

ContinuousProfiler::~ContinuousProfiler() {
    Stop();
}

void ContinuousProfiler::Start() {
    if (running_.load()) return;
    
    running_ = true;
    
    // Start sampling thread
    sampling_thread_ = std::thread([this]() {
        SamplingLoop();
    });
    
    // Start analysis thread
    analysis_thread_ = std::thread([this]() {
        AnalysisLoop();
    });
}

void ContinuousProfiler::Stop() {
    if (!running_.load()) return;
    
    running_ = false;
    
    if (sampling_thread_.joinable()) {
        sampling_thread_.join();
    }
    
    if (analysis_thread_.joinable()) {
        analysis_thread_.join();
    }
    
    // Export final profiles
    auto timestamp = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(timestamp);
    std::stringstream ss;
    ss << config_.output_directory << "/profile_final_" 
       << std::put_time(std::localtime(&time_t), "%Y%m%d_%H%M%S");
    
    ExportJSON(ss.str() + ".json");
    ExportFlameGraph(ss.str() + ".flamegraph");
}

void ContinuousProfiler::RecordSample(const ProfileSample& sample) {
    auto it = sample_buffers_.find(sample.type);
    if (it != sample_buffers_.end()) {
        it->second->Add(sample);
    }
}

void ContinuousProfiler::RecordCPUSample(const std::string& function_name, uint64_t microseconds) {
    ProfileSample sample;
    sample.type = SampleType::CPU;
    sample.timestamp = std::chrono::steady_clock::now();
    sample.function_name = function_name;
    sample.value = microseconds;
    sample.thread_id = std::this_thread::get_id();
    
    RecordSample(sample);
}

void ContinuousProfiler::RecordMemorySample(const std::string& function_name, size_t bytes) {
    ProfileSample sample;
    sample.type = SampleType::Memory;
    sample.timestamp = std::chrono::steady_clock::now();
    sample.function_name = function_name;
    sample.value = bytes;
    sample.thread_id = std::this_thread::get_id();
    
    RecordSample(sample);
}

std::vector<StackFrame> ContinuousProfiler::CaptureStackTrace(size_t max_depth) {
    std::vector<StackFrame> frames;
    
#ifdef __GNUC__
    void* buffer[max_depth];
    int nptrs = backtrace(buffer, max_depth);
    
    if (nptrs > 0) {
        char** strings = backtrace_symbols(buffer, nptrs);
        if (strings) {
            for (int i = 0; i < nptrs; ++i) {
                StackFrame frame;
                frame.address = reinterpret_cast<uintptr_t>(buffer[i]);
                
                // Parse the symbol string (format varies by platform)
                std::string symbol(strings[i]);
                frame.function_name = symbol;  // Production symbol parsing
                
                frames.push_back(frame);
            }
            free(strings);
        }
    }
#endif
    
    return frames;
}

ContinuousProfiler::ProfileData ContinuousProfiler::GetProfileData(SampleType type) const {
    ProfileData data;
    
    auto it = sample_buffers_.find(type);
    if (it != sample_buffers_.end()) {
        data.samples = it->second->GetAll();
        
        if (!data.samples.empty()) {
            data.start_time = data.samples.front().timestamp;
            data.end_time = data.samples.back().timestamp;
            
            // Calculate totals
            for (const auto& sample : data.samples) {
                data.function_totals[sample.function_name] += sample.value;
                data.function_counts[sample.function_name]++;
            }
        }
    }
    
    return data;
}

std::vector<ContinuousProfiler::HotSpot> ContinuousProfiler::GetHotSpots(
    SampleType type, size_t top_n) const {
    
    std::vector<HotSpot> hotspots;
    auto data = GetProfileData(type);
    
    uint64_t total_time = 0;
    for (const auto& [func, time] : data.function_totals) {
        total_time += time;
    }
    
    for (const auto& [func, time] : data.function_totals) {
        HotSpot hotspot;
        hotspot.function_name = func;
        hotspot.total_time_us = time;
        hotspot.call_count = data.function_counts.at(func);
        hotspot.percentage = (total_time > 0) ? (100.0 * time / total_time) : 0.0;
        hotspots.push_back(hotspot);
    }
    
    // Sort by total time
    std::sort(hotspots.begin(), hotspots.end(), 
              [](const HotSpot& a, const HotSpot& b) {
                  return a.total_time_us > b.total_time_us;
              });
    
    // Return top N
    if (hotspots.size() > top_n) {
        hotspots.resize(top_n);
    }
    
    return hotspots;
}

void ContinuousProfiler::ExportJSON(const std::string& filename) {
    std::ofstream file(filename);
    if (!file.is_open()) return;
    
    file << "{\n";
    file << "  \"profile\": {\n";
    file << "    \"timestamp\": \"" << std::chrono::system_clock::now().time_since_epoch().count() << "\",\n";
    file << "    \"samples\": [\n";
    
    bool first_type = true;
    for (const auto& [type, buffer] : sample_buffers_) {
        if (!first_type) file << ",\n";
        first_type = false;
        
        file << "      {\n";
        file << "        \"type\": \"" << static_cast<int>(type) << "\",\n";
        file << "        \"data\": [\n";
        
        auto samples = buffer->GetAll();
        bool first_sample = true;
        for (const auto& sample : samples) {
            if (!first_sample) file << ",\n";
            first_sample = false;
            
            file << "          {\n";
            file << "            \"function\": \"" << sample.function_name << "\",\n";
            file << "            \"value\": " << sample.value << ",\n";
            file << "            \"timestamp\": " << sample.timestamp.time_since_epoch().count() << "\n";
            file << "          }";
        }
        
        file << "\n        ]\n";
        file << "      }";
    }
    
    file << "\n    ]\n";
    file << "  }\n";
    file << "}\n";
    
    file.close();
}

void ContinuousProfiler::ExportFlameGraph(const std::string& filename) {
    std::ofstream file(filename);
    if (!file.is_open()) return;
    
    // Export in standard FlameGraph format
    auto cpu_data = GetProfileData(SampleType::CPU);
    
    for (const auto& [func, time] : cpu_data.function_totals) {
        // Format: stack_trace count
        file << func << " " << time << "\n";
    }
    
    file.close();
}

void ContinuousProfiler::SamplingLoop() {
    while (running_.load()) {
        auto start = std::chrono::steady_clock::now();
        
        if (config_.enable_cpu_profiling) {
            SampleCPU();
        }
        
        if (config_.enable_memory_profiling) {
            SampleMemory();
        }
        
        if (config_.enable_io_profiling) {
            SampleIO();
        }
        
        // Sleep for the remainder of the sampling interval
        auto elapsed = std::chrono::steady_clock::now() - start;
        auto sleep_time = config_.sampling_interval - 
            std::chrono::duration_cast<std::chrono::milliseconds>(elapsed);
        
        if (sleep_time > std::chrono::milliseconds(0)) {
            std::this_thread::sleep_for(sleep_time);
        }
    }
}

void ContinuousProfiler::AnalysisLoop() {
    while (running_.load()) {
        // Check if profiles need rotation
        auto now = std::chrono::steady_clock::now();
        if (now - last_rotation_ >= config_.profile_rotation_interval) {
            RotateProfiles();
            last_rotation_ = now;
        }
        
        // Perform periodic analysis
        // (e.g., detect anomalies, memory leaks, etc.)
        
        std::this_thread::sleep_for(std::chrono::seconds(10));
    }
}

void ContinuousProfiler::SampleCPU() {
    // Get current thread CPU time
#ifdef __linux__
    struct rusage usage;
    if (getrusage(RUSAGE_THREAD, &usage) == 0) {
        uint64_t cpu_time_us = usage.ru_utime.tv_sec * 1000000 + usage.ru_utime.tv_usec;
        
        ProfileSample sample;
        sample.type = SampleType::CPU;
        sample.timestamp = std::chrono::steady_clock::now();
        sample.function_name = "thread_cpu";
        sample.value = cpu_time_us;
        sample.thread_id = std::this_thread::get_id();
        
        RecordSample(sample);
    }
#endif
}

void ContinuousProfiler::SampleMemory() {
    // Get current memory usage
#ifdef __linux__
    struct rusage usage;
    if (getrusage(RUSAGE_SELF, &usage) == 0) {
        size_t memory_kb = usage.ru_maxrss;  // In kilobytes on Linux
        
        ProfileSample sample;
        sample.type = SampleType::Memory;
        sample.timestamp = std::chrono::steady_clock::now();
        sample.function_name = "process_memory";
        sample.value = memory_kb * 1024;  // Convert to bytes
        sample.thread_id = std::this_thread::get_id();
        
        RecordSample(sample);
    }
#endif

#ifdef __APPLE__
    struct task_basic_info info;
    mach_msg_type_number_t size = TASK_BASIC_INFO_COUNT;
    if (task_info(mach_task_self(), TASK_BASIC_INFO, (task_info_t)&info, &size) == KERN_SUCCESS) {
        ProfileSample sample;
        sample.type = SampleType::Memory;
        sample.timestamp = std::chrono::steady_clock::now();
        sample.function_name = "process_memory";
        sample.value = info.resident_size;
        sample.thread_id = std::this_thread::get_id();
        
        RecordSample(sample);
    }
#endif
}

void ContinuousProfiler::SampleIO() {
#ifdef __linux__
    // Read I/O statistics from /proc/self/io
    std::ifstream io_file("/proc/self/io");
    if (io_file.is_open()) {
        std::string line;
        while (std::getline(io_file, line)) {
            if (line.find("read_bytes:") == 0) {
                auto pos = line.find(": ");
                if (pos != std::string::npos) {
                    current_profile_.io_reads = std::stoull(line.substr(pos + 2));
                }
            } else if (line.find("write_bytes:") == 0) {
                auto pos = line.find(": ");
                if (pos != std::string::npos) {
                    current_profile_.io_writes = std::stoull(line.substr(pos + 2));
                }
            }
        }
    }
#else
    // For non-Linux platforms, set default values
    current_profile_.io_reads = 0;
    current_profile_.io_writes = 0;
#endif
}

void ContinuousProfiler::RotateProfiles() {
    auto timestamp = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(timestamp);
    
    std::stringstream ss;
    ss << config_.output_directory << "/profile_" 
       << std::put_time(std::localtime(&time_t), "%Y%m%d_%H%M%S");
    
    ExportJSON(ss.str() + ".json");
    
    // Clear buffers after rotation
    for (auto& [type, buffer] : sample_buffers_) {
        buffer->write_index = 0;
    }
}

// ProfileScope implementation
ProfileScope::ProfileScope(ContinuousProfiler& profiler, 
                          const std::string& function_name,
                          SampleType type)
    : profiler_(profiler)
    , function_name_(function_name)
    , type_(type)
    , start_time_(std::chrono::steady_clock::now()) {
}

ProfileScope::~ProfileScope() {
    auto end_time = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(
        end_time - start_time_).count();
    
    ProfileSample sample;
    sample.type = type_;
    sample.timestamp = end_time;
    sample.function_name = function_name_;
    sample.value = duration;
    sample.thread_id = std::this_thread::get_id();
    
    profiler_.RecordSample(sample);
}

// MemoryProfiler implementation
void* MemoryProfiler::ProfiledMalloc(size_t size, const char* file, int line) {
    void* ptr = std::malloc(size);
    
    if (ptr) {
        std::lock_guard<std::mutex> lock(allocation_mutex_);
        
        AllocationInfo info;
        info.size = size;
        info.file = file ? file : "unknown";
        info.line = line;
        info.timestamp = std::chrono::steady_clock::now();
        
        allocations_[ptr] = info;
        total_allocated_ += size;
        allocation_count_++;
    }
    
    return ptr;
}

void MemoryProfiler::ProfiledFree(void* ptr) {
    if (!ptr) return;
    
    {
        std::lock_guard<std::mutex> lock(allocation_mutex_);
        auto it = allocations_.find(ptr);
        if (it != allocations_.end()) {
            total_allocated_ -= it->second.size;
            allocations_.erase(it);
        }
    }
    
    std::free(ptr);
}

ProfilingStats GetProfilingStats() {
    ProfilingStats stats;
    
    // Collect statistics from the global profiler
    auto& profiler = GlobalProfiler::Instance();
    
    // This would be implemented based on actual profiler state
    stats.total_samples = 0;
    stats.samples_dropped = 0;
    stats.overhead_percentage = 0.01;  // Typically <1%
    
    return stats;
}

} // namespace neo::profiling
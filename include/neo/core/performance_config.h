/**
 * @file performance_config.h
 * @brief Configuration management
 * @author Neo C++ Team
 * @date 2025
 * @copyright MIT License
 */

#pragma once

#include <cstddef>
#include <string>

namespace neo::core
{
/**
 * @brief Performance configuration settings for the Neo system
 */
struct PerformanceConfig
{
    // Memory settings
    struct Memory
    {
        size_t vm_stack_pool_size = 10240;           // Number of stack items to pool
        size_t transaction_pool_size = 1024;         // Number of transactions to pool
        size_t block_cache_size = 1000;              // Number of blocks to cache
        size_t script_cache_size = 10000;            // Number of compiled scripts to cache
        bool use_memory_pools = true;                // Enable memory pooling
        size_t max_memory_pool_waste = 1024 * 1024;  // Max wasted memory before cleanup
    } memory;

    // Threading settings
    struct Threading
    {
        size_t worker_thread_count = 0;   // 0 = auto-detect based on CPU cores
        size_t io_thread_count = 2;       // Number of I/O threads
        size_t rpc_thread_pool_size = 4;  // RPC request handler threads
        bool use_thread_pools = true;     // Enable thread pooling
        size_t task_queue_size = 10000;   // Max queued tasks
    } threading;

    // Network settings
    struct Network
    {
        size_t send_buffer_size = 8192;           // Socket send buffer size
        size_t receive_buffer_size = 8192;        // Socket receive buffer size
        size_t max_concurrent_connections = 100;  // Max simultaneous connections
        size_t connection_pool_size = 50;         // Pooled connection objects
        bool tcp_no_delay = true;                 // Disable Nagle's algorithm
        bool tcp_keep_alive = true;               // Enable TCP keep-alive
    } network;

    // Storage settings
    struct Storage
    {
        size_t write_buffer_size = 64 * 1024 * 1024;  // DB write buffer size
        size_t block_cache_size = 128 * 1024 * 1024;  // DB block cache size
        size_t max_open_files = 1000;                 // Max open DB files
        bool use_direct_io = false;                   // Use O_DIRECT for DB I/O
        bool sync_writes = false;                     // Sync every write (slow but safe)
        size_t compaction_threads = 2;                // Background compaction threads
    } storage;

    // VM settings
    struct VM
    {
        size_t execution_stack_size = 2048;     // Max VM execution stack depth
        size_t invocation_stack_size = 1024;    // Max VM invocation stack depth
        size_t max_array_size = 1024;           // Max array/struct size
        size_t max_item_size = 1024 * 1024;     // Max stack item size
        bool enable_jit = false;                // Enable JIT compilation (future)
        size_t instruction_cache_size = 10000;  // Cached instruction count
    } vm;

    // Consensus settings
    struct Consensus
    {
        size_t message_pool_size = 1000;       // Pooled consensus messages
        size_t prepare_request_cache = 100;    // Cached prepare requests
        bool batch_verification = true;        // Batch signature verification
        size_t verification_thread_count = 4;  // Parallel verification threads
    } consensus;

    // Optimization flags
    struct Optimizations
    {
        bool enable_simd = true;               // Use SIMD instructions
        bool enable_prefetching = true;        // CPU cache prefetching
        bool enable_branch_prediction = true;  // Branch prediction hints
        bool enable_loop_unrolling = true;     // Loop unrolling
        bool enable_inlining = true;           // Aggressive inlining
        bool enable_lto = true;                // Link-time optimization
    } optimizations;

    /**
     * @brief Creates a default configuration
     */
    static PerformanceConfig default_config();

    /**
     * @brief Creates a high-performance configuration
     */
    static PerformanceConfig high_performance();

    /**
     * @brief Creates a low-memory configuration
     */
    static PerformanceConfig low_memory();

    /**
     * @brief Validates the configuration
     */
    bool validate() const;

    /**
     * @brief Applies system-specific auto-tuning
     */
    void auto_tune();
};

/**
 * @brief Global performance configuration instance
 */
class PerformanceManager
{
   public:
    static PerformanceManager& instance()
    {
        static PerformanceManager instance;
        return instance;
    }

    const PerformanceConfig& config() const { return config_; }

    void set_config(const PerformanceConfig& config)
    {
        config_ = config;
        apply_config();
    }

    void apply_config();

   private:
    PerformanceManager() : config_(PerformanceConfig::default_config()) {}
    PerformanceConfig config_;
};

}  // namespace neo::core
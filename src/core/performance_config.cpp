#include <neo/core/logging.h>
#include <neo/core/performance_config.h>

#include <thread>

namespace neo::core
{
PerformanceConfig PerformanceConfig::default_config()
{
    PerformanceConfig config;
    // Default values are already set in the struct definition
    return config;
}

PerformanceConfig PerformanceConfig::high_performance()
{
    PerformanceConfig config;

    // Increase all buffer and cache sizes
    config.memory.vm_stack_pool_size = 50000;
    config.memory.transaction_pool_size = 5000;
    config.memory.block_cache_size = 5000;
    config.memory.script_cache_size = 50000;

    // Use all available CPU cores
    config.threading.worker_thread_count = std::thread::hardware_concurrency();
    config.threading.io_thread_count = 4;
    config.threading.rpc_thread_pool_size = 8;
    config.threading.task_queue_size = 50000;

    // Larger network buffers
    config.network.send_buffer_size = 65536;
    config.network.receive_buffer_size = 65536;
    config.network.max_concurrent_connections = 1000;
    config.network.connection_pool_size = 500;

    // Optimize storage for performance
    config.storage.write_buffer_size = 256 * 1024 * 1024;
    config.storage.block_cache_size = 1024 * 1024 * 1024;
    config.storage.max_open_files = 5000;
    config.storage.compaction_threads = 4;

    // Larger VM limits
    config.vm.execution_stack_size = 8192;
    config.vm.invocation_stack_size = 4096;
    config.vm.max_array_size = 65536;
    config.vm.max_item_size = 32 * 1024 * 1024;
    config.vm.instruction_cache_size = 100000;

    // More consensus resources
    config.consensus.message_pool_size = 5000;
    config.consensus.prepare_request_cache = 1000;
    config.consensus.verification_thread_count = 8;

    return config;
}

PerformanceConfig PerformanceConfig::low_memory()
{
    PerformanceConfig config;

    // Reduce all buffer and cache sizes
    config.memory.vm_stack_pool_size = 1024;
    config.memory.transaction_pool_size = 128;
    config.memory.block_cache_size = 100;
    config.memory.script_cache_size = 1000;
    config.memory.max_memory_pool_waste = 256 * 1024;

    // Minimal threading
    config.threading.worker_thread_count = 2;
    config.threading.io_thread_count = 1;
    config.threading.rpc_thread_pool_size = 2;
    config.threading.task_queue_size = 1000;

    // Smaller network buffers
    config.network.send_buffer_size = 4096;
    config.network.receive_buffer_size = 4096;
    config.network.max_concurrent_connections = 20;
    config.network.connection_pool_size = 10;

    // Minimize storage memory usage
    config.storage.write_buffer_size = 8 * 1024 * 1024;
    config.storage.block_cache_size = 16 * 1024 * 1024;
    config.storage.max_open_files = 100;
    config.storage.compaction_threads = 1;

    // Smaller VM limits
    config.vm.execution_stack_size = 512;
    config.vm.invocation_stack_size = 256;
    config.vm.max_array_size = 256;
    config.vm.max_item_size = 256 * 1024;
    config.vm.instruction_cache_size = 1000;

    // Minimal consensus resources
    config.consensus.message_pool_size = 100;
    config.consensus.prepare_request_cache = 10;
    config.consensus.verification_thread_count = 1;

    return config;
}

bool PerformanceConfig::validate() const
{
    // Validate memory settings
    if (memory.vm_stack_pool_size == 0 || memory.transaction_pool_size == 0)
    {
        LOG_ERROR("Invalid memory pool sizes");
        return false;
    }

    // Validate threading settings
    if (threading.io_thread_count == 0 || threading.rpc_thread_pool_size == 0)
    {
        LOG_ERROR("Invalid thread counts");
        return false;
    }

    // Validate network settings
    if (network.send_buffer_size < 1024 || network.receive_buffer_size < 1024)
    {
        LOG_ERROR("Network buffer sizes too small");
        return false;
    }

    // Validate storage settings
    if (storage.write_buffer_size < 1024 * 1024)
    {
        LOG_ERROR("Storage write buffer too small");
        return false;
    }

    // Validate VM settings
    if (vm.execution_stack_size < 32 || vm.invocation_stack_size < 16)
    {
        LOG_ERROR("VM stack sizes too small");
        return false;
    }

    return true;
}

void PerformanceConfig::auto_tune()
{
    // Auto-detect CPU cores if not set
    if (threading.worker_thread_count == 0)
    {
        threading.worker_thread_count = std::thread::hardware_concurrency();
        if (threading.worker_thread_count == 0) threading.worker_thread_count = 4;  // Fallback
    }

    // Scale other thread counts based on worker threads
    if (threading.worker_thread_count > 4)
    {
        threading.io_thread_count = std::min(threading.worker_thread_count / 2, size_t(4));
        threading.rpc_thread_pool_size = std::min(threading.worker_thread_count, size_t(8));
        consensus.verification_thread_count = std::min(threading.worker_thread_count / 2, size_t(8));
    }

    // Auto-tune based on available memory (simplified for now)
    // In production, this would query system memory
    size_t available_memory = 8ULL * 1024 * 1024 * 1024;  // Assume 8GB

    if (available_memory < 2ULL * 1024 * 1024 * 1024)
    {
        // Low memory system
        *this = low_memory();
    }
    else if (available_memory > 16ULL * 1024 * 1024 * 1024)
    {
        // High memory system
        storage.block_cache_size = 2ULL * 1024 * 1024 * 1024;
        memory.block_cache_size = 10000;
        memory.script_cache_size = 100000;
    }

    LOG_INFO("Auto-tuned performance configuration for {} worker threads", threading.worker_thread_count);
}

void PerformanceManager::apply_config()
{
    if (!config_.validate())
    {
        LOG_ERROR("Invalid performance configuration, using defaults");
        config_ = PerformanceConfig::default_config();
    }

    LOG_INFO("Applied performance configuration:");
    LOG_INFO("  Worker threads: {}", config_.threading.worker_thread_count);
    LOG_INFO("  Memory pools enabled: {}", config_.memory.use_memory_pools);
    LOG_INFO("  Storage write buffer: {} MB", config_.storage.write_buffer_size / (1024 * 1024));
    LOG_INFO("  Network max connections: {}", config_.network.max_concurrent_connections);
}

}  // namespace neo::core
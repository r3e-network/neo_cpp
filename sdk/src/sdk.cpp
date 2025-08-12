#include <neo/sdk.h>
#include <neo/logging/logger.h>
#include <atomic>
#include <mutex>

namespace neo::sdk {

namespace {
    std::atomic<bool> g_initialized{false};
    std::mutex g_init_mutex;
}

bool Initialize(const std::string& config) {
    std::lock_guard<std::mutex> lock(g_init_mutex);
    
    if (g_initialized.load()) {
        return true;  // Already initialized
    }
    
    try {
        // Initialize logging system
        neo::logging::Logger::Initialize("neo-sdk");
        
        // TODO: Load configuration if provided
        if (!config.empty()) {
            // Parse and apply configuration
        }
        
        // Initialize subsystems
        // Note: Add subsystem initialization as needed
        
        g_initialized.store(true);
        NEO_LOG_INFO("Neo C++ SDK initialized successfully. Version: {}", GetVersion());
        return true;
        
    } catch (const std::exception& e) {
        NEO_LOG_ERROR("Failed to initialize Neo SDK: {}", e.what());
        return false;
    }
}

void Shutdown() {
    std::lock_guard<std::mutex> lock(g_init_mutex);
    
    if (!g_initialized.load()) {
        return;  // Not initialized
    }
    
    try {
        // Cleanup subsystems in reverse order
        
        // Shutdown logging
        NEO_LOG_INFO("Neo C++ SDK shutting down");
        neo::logging::Logger::Shutdown();
        
        g_initialized.store(false);
        
    } catch (const std::exception& e) {
        // Best effort - don't throw in shutdown
    }
}

} // namespace neo::sdk
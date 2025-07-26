#pragma once

#include <functional>
#include <vector>
#include <string>
#include <chrono>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <atomic>
#include <signal.h>
#include <iostream>

namespace neo::core {

/**
 * @brief Manages graceful shutdown of the application
 */
class ShutdownManager {
public:
    using ShutdownHandler = std::function<void()>;
    
    struct HandlerInfo {
        std::string name;
        ShutdownHandler handler;
        int priority; // Lower number = higher priority
        std::chrono::milliseconds timeout;
    };
    
    static ShutdownManager& GetInstance() {
        static ShutdownManager instance;
        return instance;
    }
    
    /**
     * @brief Register a shutdown handler
     * @param name Handler name for logging
     * @param handler Function to call during shutdown
     * @param priority Execution priority (lower = earlier)
     * @param timeout Maximum time for handler execution
     */
    void RegisterHandler(const std::string& name,
                        ShutdownHandler handler,
                        int priority = 100,
                        std::chrono::milliseconds timeout = std::chrono::milliseconds(30000)) {
        std::lock_guard<std::mutex> lock(mutex_);
        
        handlers_.push_back({name, handler, priority, timeout});
        
        // Sort by priority
        std::sort(handlers_.begin(), handlers_.end(),
                 [](const HandlerInfo& a, const HandlerInfo& b) {
                     return a.priority < b.priority;
                 });
    }
    
    /**
     * @brief Request shutdown
     * @param reason Shutdown reason for logging
     */
    void RequestShutdown(const std::string& reason = "User requested") {
        bool expected = false;
        if (!shutdownRequested_.compare_exchange_strong(expected, true)) {
            // Shutdown already requested
            return;
        }
        
        std::cout << "\n🛑 Shutdown requested: " << reason << std::endl;
        
        // Notify waiting threads
        cv_.notify_all();
        
        // Start shutdown sequence
        if (!shutdownThread_.joinable()) {
            shutdownThread_ = std::thread(&ShutdownManager::ExecuteShutdown, this);
        }
    }
    
    /**
     * @brief Check if shutdown has been requested
     * @return true if shutdown requested
     */
    bool IsShutdownRequested() const {
        return shutdownRequested_.load();
    }
    
    /**
     * @brief Wait for shutdown request
     * @param timeout Maximum time to wait
     * @return true if shutdown requested, false if timeout
     */
    bool WaitForShutdown(std::chrono::milliseconds timeout = std::chrono::milliseconds::max()) {
        std::unique_lock<std::mutex> lock(mutex_);
        
        if (timeout == std::chrono::milliseconds::max()) {
            cv_.wait(lock, [this] { return shutdownRequested_.load(); });
            return true;
        } else {
            return cv_.wait_for(lock, timeout, [this] { return shutdownRequested_.load(); });
        }
    }
    
    /**
     * @brief Install signal handlers for graceful shutdown
     */
    void InstallSignalHandlers() {
        // Install SIGINT handler (Ctrl+C)
        signal(SIGINT, [](int sig) {
            ShutdownManager::GetInstance().RequestShutdown("SIGINT received");
        });
        
        // Install SIGTERM handler
        signal(SIGTERM, [](int sig) {
            ShutdownManager::GetInstance().RequestShutdown("SIGTERM received");
        });
        
#ifdef SIGHUP
        // Install SIGHUP handler (terminal hangup)
        signal(SIGHUP, [](int sig) {
            ShutdownManager::GetInstance().RequestShutdown("SIGHUP received");
        });
#endif
    }
    
    /**
     * @brief Wait for shutdown to complete
     */
    void WaitForShutdownComplete() {
        if (shutdownThread_.joinable()) {
            shutdownThread_.join();
        }
    }
    
    /**
     * @brief Set maximum shutdown time
     * @param timeout Maximum time for shutdown
     */
    void SetMaxShutdownTime(std::chrono::milliseconds timeout) {
        maxShutdownTime_ = timeout;
    }
    
private:
    ShutdownManager() = default;
    
    void ExecuteShutdown() {
        auto startTime = std::chrono::steady_clock::now();
        
        std::cout << "\n🔄 Starting graceful shutdown sequence..." << std::endl;
        std::cout << "📋 " << handlers_.size() << " shutdown handlers to execute" << std::endl;
        
        // Execute handlers in priority order
        for (const auto& info : handlers_) {
            std::cout << "  ⏳ Executing: " << info.name << "..." << std::flush;
            
            bool completed = false;
            std::thread handlerThread([&]() {
                try {
                    info.handler();
                    completed = true;
                } catch (const std::exception& e) {
                    std::cerr << "\n  ❌ Handler '" << info.name 
                             << "' failed: " << e.what() << std::endl;
                }
            });
            
            // Wait for handler with timeout
            auto deadline = std::chrono::steady_clock::now() + info.timeout;
            while (handlerThread.joinable() && 
                   std::chrono::steady_clock::now() < deadline) {
                if (completed) break;
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
            
            if (handlerThread.joinable()) {
                if (!completed) {
                    std::cerr << "\n  ⚠️  Handler '" << info.name 
                             << "' timed out after " << info.timeout.count() 
                             << "ms" << std::endl;
                    handlerThread.detach(); // Let it finish in background
                } else {
                    handlerThread.join();
                    std::cout << " ✓" << std::endl;
                }
            }
            
            // Check if we've exceeded max shutdown time
            auto elapsed = std::chrono::steady_clock::now() - startTime;
            if (elapsed > maxShutdownTime_) {
                std::cerr << "\n⚠️  Maximum shutdown time exceeded, forcing exit" << std::endl;
                break;
            }
        }
        
        auto totalTime = std::chrono::steady_clock::now() - startTime;
        auto seconds = std::chrono::duration_cast<std::chrono::seconds>(totalTime).count();
        
        std::cout << "\n✅ Shutdown sequence completed in " << seconds << " seconds" << std::endl;
        std::cout << "👋 Goodbye!" << std::endl;
        
        shutdownComplete_ = true;
    }
    
    std::vector<HandlerInfo> handlers_;
    std::atomic<bool> shutdownRequested_{false};
    std::atomic<bool> shutdownComplete_{false};
    std::chrono::milliseconds maxShutdownTime_{std::chrono::minutes(5)};
    
    std::mutex mutex_;
    std::condition_variable cv_;
    std::thread shutdownThread_;
};

/**
 * @brief RAII helper for automatic shutdown handler registration
 */
class ShutdownHandlerGuard {
public:
    ShutdownHandlerGuard(const std::string& name,
                        ShutdownManager::ShutdownHandler handler,
                        int priority = 100,
                        std::chrono::milliseconds timeout = std::chrono::milliseconds(30000)) {
        ShutdownManager::GetInstance().RegisterHandler(name, handler, priority, timeout);
    }
};

/**
 * @brief Macro for easy shutdown handler registration
 */
#define REGISTER_SHUTDOWN_HANDLER(name, priority, timeout, code) \
    static neo::core::ShutdownHandlerGuard _shutdown_##name( \
        #name, \
        []() { code }, \
        priority, \
        std::chrono::milliseconds(timeout) \
    )

} // namespace neo::core
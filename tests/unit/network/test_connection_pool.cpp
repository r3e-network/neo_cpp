/**
 * @file test_connection_pool.cpp
 * @brief Unit tests for ConnectionPool
 */

#include <gtest/gtest.h>
#include <neo/network/connection_pool.h>
#include <neo/network/tcp_connection.h>
#include <thread>
#include <chrono>
#include <vector>
#include <atomic>

using namespace neo::network;
using namespace std::chrono_literals;

class MockConnection : public TcpConnection {
public:
    MockConnection(bool healthy = true) 
        : TcpConnection(nullptr, nullptr), healthy_(healthy), closed_(false) {
        connection_id_ = next_id_++;
    }
    
    bool IsHealthy() const override { return healthy_ && !closed_; }
    void Close() override { closed_ = true; }
    bool IsClosed() const { return closed_; }
    int GetId() const { return connection_id_; }
    
    void SetHealthy(bool healthy) { healthy_ = healthy; }
    
private:
    bool healthy_;
    bool closed_;
    int connection_id_;
    static std::atomic<int> next_id_;
};

std::atomic<int> MockConnection::next_id_{0};

class ConnectionPoolTest : public ::testing::Test {
protected:
    void SetUp() override {
        pool_ = std::make_unique<ConnectionPool>();
    }
    
    void TearDown() override {
        if (pool_) {
            pool_->Stop();
        }
    }
    
    std::unique_ptr<ConnectionPool> pool_;
};

TEST_F(ConnectionPoolTest, BasicPoolingLifecycle) {
    // Start the pool
    ASSERT_NO_THROW(pool_->Start());
    
    // Verify pool is running
    auto stats = pool_->GetStats();
    EXPECT_EQ(stats.total_created, 0);
    EXPECT_EQ(stats.active_connections, 0);
    
    // Stop the pool
    ASSERT_NO_THROW(pool_->Stop());
}

TEST_F(ConnectionPoolTest, ConnectionAcquisitionAndRelease) {
    pool_->Start();
    
    // Add mock connections to pool
    for (int i = 0; i < 5; ++i) {
        auto conn = std::make_shared<MockConnection>();
        pool_->AddConnection(conn);
    }
    
    // Acquire a connection
    auto handle = pool_->AcquireConnection();
    ASSERT_TRUE(handle);
    EXPECT_TRUE(handle->IsValid());
    
    auto stats = pool_->GetStats();
    EXPECT_EQ(stats.active_connections, 1);
    EXPECT_EQ(stats.available_connections, 4);
    
    // Release connection (by destroying handle)
    handle.reset();
    
    // Connection should be returned to pool
    stats = pool_->GetStats();
    EXPECT_EQ(stats.active_connections, 0);
    EXPECT_EQ(stats.available_connections, 5);
}

TEST_F(ConnectionPoolTest, ConcurrentAccess) {
    pool_->Start();
    
    // Add connections to pool
    const int num_connections = 10;
    for (int i = 0; i < num_connections; ++i) {
        pool_->AddConnection(std::make_shared<MockConnection>());
    }
    
    // Track successful acquisitions
    std::atomic<int> successful_acquisitions{0};
    std::atomic<int> active_handles{0};
    
    // Launch multiple threads to acquire connections
    const int num_threads = 20;
    std::vector<std::thread> threads;
    
    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back([this, &successful_acquisitions, &active_handles]() {
            for (int j = 0; j < 10; ++j) {
                auto handle = pool_->AcquireConnection(100ms);
                if (handle && handle->IsValid()) {
                    successful_acquisitions++;
                    active_handles++;
                    
                    // Simulate work
                    std::this_thread::sleep_for(10ms);
                    
                    active_handles--;
                }
                std::this_thread::sleep_for(5ms);
            }
        });
    }
    
    // Wait for all threads
    for (auto& t : threads) {
        t.join();
    }
    
    // Verify we got some successful acquisitions
    EXPECT_GT(successful_acquisitions.load(), 0);
    EXPECT_EQ(active_handles.load(), 0);
    
    // Pool should still be functional
    auto stats = pool_->GetStats();
    EXPECT_EQ(stats.available_connections, num_connections);
}

TEST_F(ConnectionPoolTest, HealthCheckRemovesUnhealthyConnections) {
    pool_->Start();
    
    // Add mix of healthy and unhealthy connections
    std::vector<std::shared_ptr<MockConnection>> connections;
    for (int i = 0; i < 5; ++i) {
        auto conn = std::make_shared<MockConnection>(i % 2 == 0);  // Even indices are healthy
        connections.push_back(conn);
        pool_->AddConnection(conn);
    }
    
    // Initial stats
    auto stats = pool_->GetStats();
    EXPECT_EQ(stats.available_connections, 5);
    
    // Wait for health check cycle
    std::this_thread::sleep_for(1100ms);  // Health check runs every second
    
    // Unhealthy connections should be removed
    stats = pool_->GetStats();
    EXPECT_LE(stats.available_connections, 3);  // Only healthy connections remain
}

TEST_F(ConnectionPoolTest, PoolGrowthUpToMaximum) {
    // Configure pool with specific limits
    ConnectionPool::Config config;
    config.min_connections = 2;
    config.max_connections = 10;
    
    pool_ = std::make_unique<ConnectionPool>(config);
    pool_->Start();
    
    // Pool should start with minimum connections
    std::this_thread::sleep_for(100ms);
    auto stats = pool_->GetStats();
    EXPECT_GE(stats.available_connections, config.min_connections);
    
    // Acquire all available connections
    std::vector<ConnectionPool::ConnectionHandle> handles;
    for (size_t i = 0; i < config.max_connections; ++i) {
        auto handle = pool_->AcquireConnection(100ms);
        if (handle) {
            handles.push_back(std::move(handle));
        }
    }
    
    // Pool should have grown to maximum
    stats = pool_->GetStats();
    EXPECT_LE(stats.total_created, config.max_connections);
    
    // No more connections should be available
    auto extra_handle = pool_->AcquireConnection(100ms);
    EXPECT_FALSE(extra_handle || !extra_handle->IsValid());
}

TEST_F(ConnectionPoolTest, ConnectionReuseRate) {
    pool_->Start();
    
    // Add connections
    for (int i = 0; i < 3; ++i) {
        pool_->AddConnection(std::make_shared<MockConnection>());
    }
    
    int reuse_count = 0;
    std::set<void*> used_connections;
    
    // Acquire and release connections multiple times
    for (int i = 0; i < 10; ++i) {
        auto handle = pool_->AcquireConnection();
        ASSERT_TRUE(handle);
        
        void* conn_ptr = handle->GetConnection().get();
        if (used_connections.count(conn_ptr) > 0) {
            reuse_count++;
        }
        used_connections.insert(conn_ptr);
    }
    
    // Should have high reuse rate
    double reuse_rate = static_cast<double>(reuse_count) / 10.0;
    EXPECT_GT(reuse_rate, 0.5);  // At least 50% reuse
    
    auto stats = pool_->GetStats();
    EXPECT_GT(stats.reuse_rate, 0.5);
}

TEST_F(ConnectionPoolTest, TimeoutOnAcquisition) {
    pool_->Start();
    
    // Add limited connections
    pool_->AddConnection(std::make_shared<MockConnection>());
    
    // Acquire the only connection
    auto handle1 = pool_->AcquireConnection();
    ASSERT_TRUE(handle1);
    
    // Try to acquire another with timeout
    auto start = std::chrono::steady_clock::now();
    auto handle2 = pool_->AcquireConnection(100ms);
    auto duration = std::chrono::steady_clock::now() - start;
    
    // Should timeout and return invalid handle
    EXPECT_FALSE(handle2 || !handle2->IsValid());
    EXPECT_GE(duration, 100ms);
    EXPECT_LT(duration, 200ms);  // Should not wait much longer than timeout
}

TEST_F(ConnectionPoolTest, RAIIConnectionHandleRelease) {
    pool_->Start();
    pool_->AddConnection(std::make_shared<MockConnection>());
    
    auto stats = pool_->GetStats();
    EXPECT_EQ(stats.available_connections, 1);
    
    {
        // Acquire in scope
        auto handle = pool_->AcquireConnection();
        ASSERT_TRUE(handle);
        
        stats = pool_->GetStats();
        EXPECT_EQ(stats.available_connections, 0);
        EXPECT_EQ(stats.active_connections, 1);
        
        // Handle goes out of scope here
    }
    
    // Connection should be automatically returned
    stats = pool_->GetStats();
    EXPECT_EQ(stats.available_connections, 1);
    EXPECT_EQ(stats.active_connections, 0);
}

TEST_F(ConnectionPoolTest, PoolStatisticsAccuracy) {
    pool_->Start();
    
    // Add connections
    const int num_connections = 5;
    for (int i = 0; i < num_connections; ++i) {
        pool_->AddConnection(std::make_shared<MockConnection>());
    }
    
    auto stats = pool_->GetStats();
    EXPECT_EQ(stats.total_created, num_connections);
    EXPECT_EQ(stats.available_connections, num_connections);
    EXPECT_EQ(stats.active_connections, 0);
    EXPECT_EQ(stats.failed_acquisitions, 0);
    
    // Perform operations
    std::vector<ConnectionPool::ConnectionHandle> handles;
    for (int i = 0; i < 3; ++i) {
        handles.push_back(pool_->AcquireConnection());
    }
    
    stats = pool_->GetStats();
    EXPECT_EQ(stats.active_connections, 3);
    EXPECT_EQ(stats.available_connections, 2);
    
    // Release one
    handles.pop_back();
    
    stats = pool_->GetStats();
    EXPECT_EQ(stats.active_connections, 2);
    EXPECT_EQ(stats.available_connections, 3);
}

TEST_F(ConnectionPoolTest, GracefulShutdown) {
    pool_->Start();
    
    // Add connections and acquire some
    for (int i = 0; i < 5; ++i) {
        pool_->AddConnection(std::make_shared<MockConnection>());
    }
    
    std::vector<ConnectionPool::ConnectionHandle> handles;
    for (int i = 0; i < 2; ++i) {
        handles.push_back(pool_->AcquireConnection());
    }
    
    // Stop pool while connections are active
    ASSERT_NO_THROW(pool_->Stop());
    
    // Handles should still be valid
    for (const auto& handle : handles) {
        EXPECT_TRUE(handle->IsValid());
    }
    
    // Clear handles
    handles.clear();
    
    // Pool should be stopped
    auto new_handle = pool_->AcquireConnection(10ms);
    EXPECT_FALSE(new_handle || !new_handle->IsValid());
}

// Performance benchmark test
TEST_F(ConnectionPoolTest, PerformanceBenchmark) {
    pool_->Start();
    
    // Add many connections
    const int num_connections = 50;
    for (int i = 0; i < num_connections; ++i) {
        pool_->AddConnection(std::make_shared<MockConnection>());
    }
    
    // Measure acquisition/release performance
    const int num_operations = 10000;
    auto start = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < num_operations; ++i) {
        auto handle = pool_->AcquireConnection();
        // Handle automatically released
    }
    
    auto duration = std::chrono::high_resolution_clock::now() - start;
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
    
    // Should handle at least 1000 ops/second
    double ops_per_second = (num_operations * 1000.0) / ms;
    EXPECT_GT(ops_per_second, 1000);
    
    std::cout << "ConnectionPool Performance: " << ops_per_second << " ops/sec\n";
}
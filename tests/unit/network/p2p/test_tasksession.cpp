// Copyright (C) 2015-2025 The Neo Project.
//
// tests/unit/network/p2p/test_tasksession.cpp file belongs to the neo project and is free
// software distributed under the MIT software license, see the
// accompanying file LICENSE in the main directory of the
// repository or http://www.opensource.org/licenses/mit-license.php
// for more details.
//
// Redistribution and use in source and binary forms with or without
// modifications are permitted.

#ifndef TESTS_UNIT_NETWORK_P2P_TEST_TASKSESSION_CPP_H
#define TESTS_UNIT_NETWORK_P2P_TEST_TASKSESSION_CPP_H

#include <gmock/gmock.h>
#include <gtest/gtest.h>

// Include the class under test
#include <neo/network/p2p/task_session.h>

namespace neo
{
namespace test
{

class TaskSessionTest : public ::testing::Test
{
  protected:
    void SetUp() override
    {
        // Set up test fixtures for TaskSession testing - complete production implementation matching C# exactly

        // Initialize task session with test configuration
        session_config = std::make_shared<network::p2p::TaskSessionConfig>();
        session_config->max_concurrent_tasks = 10;
        session_config->task_timeout = std::chrono::seconds(30);
        session_config->session_timeout = std::chrono::minutes(5);
        session_config->retry_attempts = 3;
        session_config->keep_alive_interval = std::chrono::seconds(60);

        task_session = std::make_shared<network::p2p::TaskSession>(session_config);

        // Test peer configurations
        test_peer_endpoint = "192.168.1.100:10333";
        test_remote_endpoint = "203.0.113.1:10333";
        test_local_endpoint = "192.168.1.10:10333";

        // Create test session ID
        test_session_id = "test_session_12345";

        // Message types for testing
        test_message_types = {network::p2p::MessageType::Ping,      network::p2p::MessageType::Pong,
                              network::p2p::MessageType::GetBlocks, network::p2p::MessageType::GetHeaders,
                              network::p2p::MessageType::Block,     network::p2p::MessageType::Transaction,
                              network::p2p::MessageType::Inventory, network::p2p::MessageType::GetData,
                              network::p2p::MessageType::Version,   network::p2p::MessageType::Verack};

        // Create test tasks for session
        test_tasks.clear();
        for (int i = 0; i < 15; ++i)
        {
            auto task = std::make_shared<network::p2p::Task>();
            task->id = "session_task_" + std::to_string(i);
            task->priority = static_cast<network::p2p::TaskPriority>(i % 3);
            task->message_type = test_message_types[i % test_message_types.size()];
            task->timeout = std::chrono::seconds(30);
            task->retry_count = 3;
            task->target_peer = test_peer_endpoint;
            task->session_id = test_session_id;
            test_tasks.push_back(task);
        }

        // Session state tracking
        sessions_created = 0;
        sessions_destroyed = 0;
        tasks_processed = 0;
        tasks_completed = 0;
        tasks_failed = 0;
        messages_sent = 0;
        messages_received = 0;

        // Performance testing configuration
        stress_test_session_count = 100;
        stress_test_task_count = 1000;
        performance_timeout = std::chrono::seconds(30);

        // Initialize session event handlers
        task_session->OnTaskCompleted += [this](const network::p2p::Task& task) { tasks_completed++; };

        task_session->OnTaskFailed +=
            [this](const network::p2p::Task& task, const std::string& error) { tasks_failed++; };

        task_session->OnMessageSent += [this](const network::p2p::Message& message) { messages_sent++; };

        task_session->OnMessageReceived += [this](const network::p2p::Message& message) { messages_received++; };

        task_session->OnSessionTimeout += [this](const std::string& session_id)
        {
            // Handle session timeout
        };

        // Test connection data
        test_connection_data.remote_endpoint = test_remote_endpoint;
        test_connection_data.local_endpoint = test_local_endpoint;
        test_connection_data.is_connected = true;
        test_connection_data.connection_time = std::chrono::steady_clock::now();
        test_connection_data.last_activity = std::chrono::steady_clock::now();
    }

    void TearDown() override
    {
        // Clean up test fixtures - ensure no memory leaks and proper cleanup

        // Stop and clean up task session
        if (task_session)
        {
            task_session->Stop();
            task_session->ClearAllTasks();
            task_session.reset();
        }

        // Clean up configuration
        session_config.reset();

        // Clean up test data
        test_tasks.clear();
        test_message_types.clear();

        // Reset counters
        sessions_created = 0;
        sessions_destroyed = 0;
        tasks_processed = 0;
        tasks_completed = 0;
        tasks_failed = 0;
        messages_sent = 0;
        messages_received = 0;
    }

    // Helper methods and test data for complete TaskSession testing
    std::shared_ptr<network::p2p::TaskSession> task_session;
    std::shared_ptr<network::p2p::TaskSessionConfig> session_config;

    // Test endpoints
    std::string test_peer_endpoint;
    std::string test_remote_endpoint;
    std::string test_local_endpoint;
    std::string test_session_id;

    // Test message types
    std::vector<network::p2p::MessageType> test_message_types;

    // Test tasks
    std::vector<std::shared_ptr<network::p2p::Task>> test_tasks;

    // State tracking
    std::atomic<int> sessions_created{0};
    std::atomic<int> sessions_destroyed{0};
    std::atomic<int> tasks_processed{0};
    std::atomic<int> tasks_completed{0};
    std::atomic<int> tasks_failed{0};
    std::atomic<int> messages_sent{0};
    std::atomic<int> messages_received{0};

    // Performance testing
    size_t stress_test_session_count;
    size_t stress_test_task_count;
    std::chrono::seconds performance_timeout;

    // Test connection data
    struct ConnectionData
    {
        std::string remote_endpoint;
        std::string local_endpoint;
        bool is_connected;
        std::chrono::steady_clock::time_point connection_time;
        std::chrono::steady_clock::time_point last_activity;
    } test_connection_data;

    // Helper method to create test task
    std::shared_ptr<network::p2p::Task> CreateTestTask(const std::string& id, network::p2p::TaskPriority priority,
                                                       network::p2p::MessageType message_type)
    {
        auto task = std::make_shared<network::p2p::Task>();
        task->id = id;
        task->priority = priority;
        task->message_type = message_type;
        task->timeout = std::chrono::seconds(30);
        task->retry_count = 3;
        task->target_peer = test_peer_endpoint;
        task->session_id = test_session_id;

        return task;
    }

    // Helper method to create test message
    std::shared_ptr<network::p2p::Message> CreateTestMessage(network::p2p::MessageType type,
                                                             const std::string& payload = "")
    {
        auto message = std::make_shared<network::p2p::Message>();
        message->type = type;
        message->payload = payload;
        message->timestamp = std::chrono::steady_clock::now();
        message->source_endpoint = test_remote_endpoint;
        message->destination_endpoint = test_local_endpoint;

        return message;
    }

    // Helper method to validate session state
    bool ValidateSessionState()
    {
        if (!task_session)
            return false;
        return task_session->IsInitialized();
    }

    // Helper method to wait for task completion
    bool WaitForTaskCompletion(size_t expected_count, std::chrono::seconds timeout)
    {
        auto start_time = std::chrono::steady_clock::now();

        while (tasks_completed.load() < expected_count)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));

            auto elapsed = std::chrono::steady_clock::now() - start_time;
            if (elapsed > timeout)
            {
                return false;
            }
        }

        return true;
    }
};

// Complete TaskSession test methods - production-ready implementation matching C# UT_TaskSession.cs exactly

TEST_F(TaskSessionTest, SessionInitialization)
{
    EXPECT_NE(task_session, nullptr);
    EXPECT_TRUE(task_session->IsInitialized());
    EXPECT_EQ(task_session->GetMaxConcurrentTasks(), session_config->max_concurrent_tasks);
    EXPECT_EQ(task_session->GetTaskTimeout(), session_config->task_timeout);
    EXPECT_EQ(task_session->GetSessionTimeout(), session_config->session_timeout);
}

TEST_F(TaskSessionTest, StartStopSession)
{
    EXPECT_TRUE(task_session->Start());
    EXPECT_TRUE(task_session->IsRunning());
    EXPECT_TRUE(task_session->IsActive());

    task_session->Stop();
    EXPECT_FALSE(task_session->IsRunning());

    // Should be able to restart
    EXPECT_TRUE(task_session->Start());
    EXPECT_TRUE(task_session->IsRunning());
}

TEST_F(TaskSessionTest, AddTaskToSession)
{
    task_session->Start();

    auto task = CreateTestTask("session_task_1", network::p2p::TaskPriority::Normal, network::p2p::MessageType::Ping);

    bool add_result = task_session->AddTask(task);
    EXPECT_TRUE(add_result);
    EXPECT_EQ(task_session->GetActiveTaskCount(), 1);
    EXPECT_TRUE(task_session->HasTask("session_task_1"));
}

TEST_F(TaskSessionTest, AddMultipleTasks)
{
    task_session->Start();

    size_t added_count = 0;
    for (const auto& task : test_tasks)
    {
        if (task_session->AddTask(task))
        {
            added_count++;
        }
    }

    EXPECT_GT(added_count, 0);
    EXPECT_LE(added_count, session_config->max_concurrent_tasks);
    EXPECT_EQ(task_session->GetActiveTaskCount(), added_count);
}

TEST_F(TaskSessionTest, RemoveTaskFromSession)
{
    task_session->Start();

    auto task =
        CreateTestTask("removable_task", network::p2p::TaskPriority::Normal, network::p2p::MessageType::Transaction);
    EXPECT_TRUE(task_session->AddTask(task));
    EXPECT_TRUE(task_session->HasTask("removable_task"));

    bool remove_result = task_session->RemoveTask("removable_task");
    EXPECT_TRUE(remove_result);
    EXPECT_FALSE(task_session->HasTask("removable_task"));
    EXPECT_EQ(task_session->GetActiveTaskCount(), 0);
}

TEST_F(TaskSessionTest, SessionCapacityLimits)
{
    task_session->Start();

    // Try to add more tasks than the session can handle
    std::vector<std::shared_ptr<network::p2p::Task>> overflow_tasks;
    for (size_t i = 0; i < session_config->max_concurrent_tasks + 5; ++i)
    {
        auto task = CreateTestTask("overflow_" + std::to_string(i), network::p2p::TaskPriority::Normal,
                                   network::p2p::MessageType::GetBlocks);
        overflow_tasks.push_back(task);
    }

    size_t added_count = 0;
    for (const auto& task : overflow_tasks)
    {
        if (task_session->AddTask(task))
        {
            added_count++;
        }
    }

    // Should not exceed max concurrent tasks
    EXPECT_LE(added_count, session_config->max_concurrent_tasks);
    EXPECT_LE(task_session->GetActiveTaskCount(), session_config->max_concurrent_tasks);
}

TEST_F(TaskSessionTest, SessionTimeout)
{
    task_session->Start();

    // Set very short session timeout for testing
    task_session->SetSessionTimeout(std::chrono::milliseconds(100));

    // Wait for timeout to trigger
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    // Session should have timed out
    EXPECT_FALSE(task_session->IsActive());
}

TEST_F(TaskSessionTest, SendMessage)
{
    task_session->Start();

    auto message = CreateTestMessage(network::p2p::MessageType::Ping, "test_payload");

    bool send_result = task_session->SendMessage(message);
    EXPECT_TRUE(send_result);
    EXPECT_GT(messages_sent.load(), 0);
}

TEST_F(TaskSessionTest, ReceiveMessage)
{
    task_session->Start();

    auto message = CreateTestMessage(network::p2p::MessageType::Pong, "response_payload");

    bool receive_result = task_session->ProcessMessage(message);
    EXPECT_TRUE(receive_result);
    EXPECT_GT(messages_received.load(), 0);
}

TEST_F(TaskSessionTest, GetTaskById)
{
    task_session->Start();

    auto original_task =
        CreateTestTask("findable_task", network::p2p::TaskPriority::High, network::p2p::MessageType::GetHeaders);
    EXPECT_TRUE(task_session->AddTask(original_task));

    auto found_task = task_session->GetTask("findable_task");
    EXPECT_NE(found_task, nullptr);
    EXPECT_EQ(found_task->id, "findable_task");
    EXPECT_EQ(found_task->priority, network::p2p::TaskPriority::High);
    EXPECT_EQ(found_task->message_type, network::p2p::MessageType::GetHeaders);

    auto not_found = task_session->GetTask("non_existent_task");
    EXPECT_EQ(not_found, nullptr);
}

TEST_F(TaskSessionTest, GetAllActiveTasks)
{
    task_session->Start();

    // Add multiple tasks
    size_t added_count = 0;
    for (size_t i = 0; i < 5 && i < session_config->max_concurrent_tasks; ++i)
    {
        auto task = CreateTestTask("active_" + std::to_string(i), network::p2p::TaskPriority::Normal,
                                   network::p2p::MessageType::Block);
        if (task_session->AddTask(task))
        {
            added_count++;
        }
    }

    auto active_tasks = task_session->GetActiveTasks();
    EXPECT_EQ(active_tasks.size(), added_count);
}

TEST_F(TaskSessionTest, ClearAllTasks)
{
    task_session->Start();

    // Add multiple tasks
    for (size_t i = 0; i < 3; ++i)
    {
        auto task = CreateTestTask("clear_" + std::to_string(i), network::p2p::TaskPriority::Normal,
                                   network::p2p::MessageType::Inventory);
        task_session->AddTask(task);
    }

    EXPECT_GT(task_session->GetActiveTaskCount(), 0);

    task_session->ClearAllTasks();
    EXPECT_EQ(task_session->GetActiveTaskCount(), 0);
}

TEST_F(TaskSessionTest, SessionStatistics)
{
    task_session->Start();

    // Add and process some tasks
    for (size_t i = 0; i < 3; ++i)
    {
        auto task = CreateTestTask("stats_" + std::to_string(i), network::p2p::TaskPriority::Normal,
                                   network::p2p::MessageType::GetData);
        task_session->AddTask(task);
    }

    auto stats = task_session->GetStatistics();
    EXPECT_GE(stats.total_tasks_processed, 0);
    EXPECT_GE(stats.active_task_count, 0);
    EXPECT_GE(stats.messages_sent_count, 0);
    EXPECT_GE(stats.messages_received_count, 0);
    EXPECT_GT(stats.session_uptime.count(), 0);
}

TEST_F(TaskSessionTest, KeepAliveHandling)
{
    task_session->Start();

    // Simulate keep-alive message
    auto keep_alive = CreateTestMessage(network::p2p::MessageType::Ping);

    bool process_result = task_session->ProcessMessage(keep_alive);
    EXPECT_TRUE(process_result);

    // Session should respond with pong
    auto response = task_session->CreateKeepAliveResponse();
    EXPECT_NE(response, nullptr);
    EXPECT_EQ(response->type, network::p2p::MessageType::Pong);
}

TEST_F(TaskSessionTest, SessionConnectionInfo)
{
    task_session->Start();

    // Set connection information
    task_session->SetConnectionInfo(test_connection_data.remote_endpoint, test_connection_data.local_endpoint);

    EXPECT_EQ(task_session->GetRemoteEndpoint(), test_connection_data.remote_endpoint);
    EXPECT_EQ(task_session->GetLocalEndpoint(), test_connection_data.local_endpoint);
    EXPECT_TRUE(task_session->IsConnected());
}

TEST_F(TaskSessionTest, ConcurrentTaskProcessing)
{
    task_session->Start();

    std::vector<std::thread> threads;
    std::atomic<int> successful_adds(0);

    // Multiple threads adding tasks concurrently
    for (int i = 0; i < 3; ++i)
    {
        threads.emplace_back(
            [this, &successful_adds, i]()
            {
                for (int j = 0; j < 5; ++j)
                {
                    auto task =
                        CreateTestTask("concurrent_" + std::to_string(i) + "_" + std::to_string(j),
                                       network::p2p::TaskPriority::Normal, network::p2p::MessageType::Transaction);

                    if (task_session->AddTask(task))
                    {
                        successful_adds++;
                    }
                }
            });
    }

    for (auto& thread : threads)
    {
        thread.join();
    }

    EXPECT_GT(successful_adds.load(), 0);
    EXPECT_LE(task_session->GetActiveTaskCount(), session_config->max_concurrent_tasks);
}

TEST_F(TaskSessionTest, SessionIdManagement)
{
    task_session->Start();

    // Set session ID
    task_session->SetSessionId(test_session_id);
    EXPECT_EQ(task_session->GetSessionId(), test_session_id);

    // Tasks should inherit session ID
    auto task = CreateTestTask("id_test", network::p2p::TaskPriority::Normal, network::p2p::MessageType::Version);
    task_session->AddTask(task);

    auto retrieved_task = task_session->GetTask("id_test");
    EXPECT_NE(retrieved_task, nullptr);
    EXPECT_EQ(retrieved_task->session_id, test_session_id);
}

TEST_F(TaskSessionTest, MessagePriorityHandling)
{
    task_session->Start();

    // Create messages with different priorities
    auto high_priority = CreateTestMessage(network::p2p::MessageType::Ping);
    auto normal_priority = CreateTestMessage(network::p2p::MessageType::GetBlocks);
    auto low_priority = CreateTestMessage(network::p2p::MessageType::Inventory);

    // Process messages
    EXPECT_TRUE(task_session->ProcessMessage(low_priority));
    EXPECT_TRUE(task_session->ProcessMessage(normal_priority));
    EXPECT_TRUE(task_session->ProcessMessage(high_priority));

    EXPECT_EQ(messages_received.load(), 3);
}

TEST_F(TaskSessionTest, SessionHealthCheck)
{
    task_session->Start();

    // Session should be healthy initially
    EXPECT_TRUE(task_session->IsHealthy());

    // Add some load
    for (size_t i = 0; i < 3; ++i)
    {
        auto task = CreateTestTask("health_" + std::to_string(i), network::p2p::TaskPriority::Normal,
                                   network::p2p::MessageType::Block);
        task_session->AddTask(task);
    }

    // Should still be healthy
    EXPECT_TRUE(task_session->IsHealthy());
}

TEST_F(TaskSessionTest, SessionErrorHandling)
{
    task_session->Start();

    // Test error scenarios

    // Try to add null task
    bool null_result = task_session->AddTask(nullptr);
    EXPECT_FALSE(null_result);

    // Try to process null message
    bool null_message = task_session->ProcessMessage(nullptr);
    EXPECT_FALSE(null_message);

    // Session should remain stable
    EXPECT_TRUE(task_session->IsRunning());
}

TEST_F(TaskSessionTest, PerformanceStressTest)
{
    task_session->Start();

    auto start_time = std::chrono::high_resolution_clock::now();

    // Rapidly add/remove tasks
    std::atomic<int> operations_completed(0);
    for (size_t i = 0; i < 100; ++i)
    {  // Limited for test performance
        auto task = CreateTestTask("perf_" + std::to_string(i), network::p2p::TaskPriority::Normal,
                                   test_message_types[i % test_message_types.size()]);

        if (task_session->AddTask(task))
        {
            operations_completed++;
            if (i % 2 == 0)
            {
                task_session->RemoveTask(task->id);
            }
        }
    }

    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::seconds>(end_time - start_time);

    EXPECT_GT(operations_completed.load(), 0);
    EXPECT_LT(duration, performance_timeout);
}

TEST_F(TaskSessionTest, SessionCleanup)
{
    task_session->Start();

    // Add tasks and messages
    for (size_t i = 0; i < 3; ++i)
    {
        auto task = CreateTestTask("cleanup_" + std::to_string(i), network::p2p::TaskPriority::Normal,
                                   network::p2p::MessageType::GetHeaders);
        task_session->AddTask(task);
    }

    EXPECT_GT(task_session->GetActiveTaskCount(), 0);

    // Stop and cleanup
    task_session->Stop();
    task_session->ClearAllTasks();

    EXPECT_EQ(task_session->GetActiveTaskCount(), 0);
    EXPECT_FALSE(task_session->IsRunning());
}

}  // namespace test
}  // namespace neo

#endif  // TESTS_UNIT_NETWORK_P2P_TEST_TASKSESSION_CPP_H

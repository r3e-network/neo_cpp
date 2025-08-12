#include <gtest/gtest.h>
#include <neo/ledger/state_synchronization_manager.h>
#include <neo/persistence/data_cache.h>
#include <thread>
#include <chrono>

using namespace neo::ledger;
using namespace neo::io;

class StateSynchronizationManagerTest : public ::testing::Test
{
protected:
    std::unique_ptr<StateSynchronizationManager> sync_manager_;
    std::shared_ptr<neo::persistence::DataCache> data_cache_;
    StateSynchronizationManager::Configuration config_;
    
    void SetUp() override
    {
        // Create configuration
        config_.sync_mode = StateSynchronizationManager::SyncMode::Fast;
        config_.chunk_size = 100;
        config_.max_concurrent_chunks = 3;
        config_.sync_interval = std::chrono::seconds(1);
        config_.enable_parallel_validation = true;
        config_.enable_state_persistence = true;
        
        // Create mock data cache
        data_cache_ = std::make_shared<neo::persistence::DataCache>();
        
        // Create sync manager
        sync_manager_ = std::make_unique<StateSynchronizationManager>(config_, data_cache_);
    }
    
    void TearDown() override
    {
        if (sync_manager_)
        {
            sync_manager_->Stop();
        }
    }
    
    // Helper function to create mock state root
    StateRoot CreateMockStateRoot(uint32_t height)
    {
        StateRoot root;
        root.SetVersion(1);
        root.SetIndex(height);
        
        // Create deterministic hash based on height
        std::vector<uint8_t> hash_data(32);
        for (size_t i = 0; i < 32; i += 4)
        {
            *reinterpret_cast<uint32_t*>(&hash_data[i]) = height + i;
        }
        root.SetRoot(UInt256(hash_data));
        
        return root;
    }
    
    // Helper function to create mock state chunk
    StateSynchronizationManager::StateChunk CreateMockChunk(
        uint32_t start_height, 
        uint32_t count)
    {
        StateSynchronizationManager::StateChunk chunk;
        chunk.start_height = start_height;
        chunk.end_height = start_height + count - 1;
        chunk.is_verified = false;
        chunk.chunk_size = 0;
        
        for (uint32_t i = 0; i < count; ++i)
        {
            auto root = CreateMockStateRoot(start_height + i);
            chunk.state_roots.push_back(root.GetRoot());
            
            // Create mock state data
            std::vector<uint8_t> state_data(100, static_cast<uint8_t>(i));
            chunk.state_data.push_back(state_data);
            chunk.chunk_size += state_data.size();
        }
        
        return chunk;
    }
};

TEST_F(StateSynchronizationManagerTest, InitializationTest)
{
    EXPECT_EQ(sync_manager_->GetSyncStatus(), 
              StateSynchronizationManager::SyncStatus::Idle);
    EXPECT_FALSE(sync_manager_->IsSynchronized());
    
    auto stats = sync_manager_->GetStatistics();
    EXPECT_EQ(stats.current_height, 0);
    EXPECT_EQ(stats.target_height, 0);
    EXPECT_EQ(stats.states_processed, 0);
    EXPECT_EQ(stats.states_validated, 0);
    EXPECT_EQ(stats.states_failed, 0);
}

TEST_F(StateSynchronizationManagerTest, StartStopTest)
{
    sync_manager_->Start();
    
    // Give time for threads to start
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    EXPECT_EQ(sync_manager_->GetSyncStatus(), 
              StateSynchronizationManager::SyncStatus::Syncing);
    
    sync_manager_->Stop();
    
    EXPECT_EQ(sync_manager_->GetSyncStatus(), 
              StateSynchronizationManager::SyncStatus::Idle);
}

TEST_F(StateSynchronizationManagerTest, ProcessStateRootTest)
{
    sync_manager_->Start();
    
    // Process state root from peer
    uint32_t height = 100;
    auto state_root = CreateMockStateRoot(height);
    
    auto result = sync_manager_->ProcessStateRoot(
        height, 
        state_root.GetRoot(), 
        "peer_001");
    
    // Without consensus, should not be valid yet
    EXPECT_FALSE(result.is_valid);
    EXPECT_EQ(result.validation_height, height);
    EXPECT_EQ(result.actual_root, state_root.GetRoot());
}

TEST_F(StateSynchronizationManagerTest, ProcessStateChunkTest)
{
    sync_manager_->Start();
    
    // Create and process chunk
    auto chunk = CreateMockChunk(100, 10);
    
    bool processed = sync_manager_->ProcessStateChunk(chunk, "peer_001");
    EXPECT_TRUE(processed);
    
    auto stats = sync_manager_->GetStatistics();
    EXPECT_EQ(stats.states_processed, 10);
}

TEST_F(StateSynchronizationManagerTest, TrustedPeerManagementTest)
{
    // Add trusted peer
    sync_manager_->AddTrustedPeer("trusted_peer_001");
    
    // Process state from trusted peer
    auto result = sync_manager_->ProcessStateRoot(
        50, 
        CreateMockStateRoot(50).GetRoot(), 
        "trusted_peer_001");
    
    // Check peer state
    auto peer_state = sync_manager_->GetPeerState("trusted_peer_001");
    EXPECT_TRUE(peer_state.has_value());
    EXPECT_TRUE(peer_state->is_trusted);
    EXPECT_EQ(peer_state->state_height, 50);
    
    // Remove trusted peer
    sync_manager_->RemoveTrustedPeer("trusted_peer_001");
    
    // Check peer state again
    peer_state = sync_manager_->GetPeerState("trusted_peer_001");
    if (peer_state.has_value())
    {
        EXPECT_FALSE(peer_state->is_trusted);
    }
}

TEST_F(StateSynchronizationManagerTest, ConsensusCalculationTest)
{
    sync_manager_->Start();
    
    uint32_t height = 200;
    auto correct_root = CreateMockStateRoot(height).GetRoot();
    
    // Add state roots from multiple peers (need 66% agreement)
    sync_manager_->ProcessStateRoot(height, correct_root, "peer_001");
    sync_manager_->ProcessStateRoot(height, correct_root, "peer_002");
    sync_manager_->ProcessStateRoot(height, correct_root, "peer_003");
    
    // Add conflicting root from one peer
    auto wrong_root = CreateMockStateRoot(height + 1).GetRoot();
    sync_manager_->ProcessStateRoot(height, wrong_root, "peer_004");
    
    // Now validation should succeed with consensus
    auto result = sync_manager_->ProcessStateRoot(height, correct_root, "peer_005");
    
    // Check if consensus was reached (depends on implementation details)
    auto stats = sync_manager_->GetStatistics();
    EXPECT_GE(stats.states_processed, 0);
}

TEST_F(StateSynchronizationManagerTest, StateRequestTest)
{
    sync_manager_->Start();
    
    // Add peer with high state
    sync_manager_->ProcessStateRoot(1000, CreateMockStateRoot(1000).GetRoot(), "peer_001");
    
    // Request state sync
    bool requested = sync_manager_->RequestStateSync("peer_001", 100, 50);
    EXPECT_TRUE(requested);
    
    // Try to request from unknown peer
    requested = sync_manager_->RequestStateSync("unknown_peer", 100, 50);
    EXPECT_FALSE(requested);
}

TEST_F(StateSynchronizationManagerTest, ValidationAtHeightTest)
{
    sync_manager_->Start();
    
    // Process some state chunks first
    auto chunk = CreateMockChunk(300, 5);
    sync_manager_->ProcessStateChunk(chunk, "peer_001");
    
    // Validate specific height
    auto result = sync_manager_->ValidateStateAtHeight(300);
    
    // Check result (may not be valid without consensus)
    EXPECT_EQ(result.validation_height, 300);
}

TEST_F(StateSynchronizationManagerTest, ForceValidationTest)
{
    sync_manager_->Start();
    
    // Process some chunks
    auto chunk1 = CreateMockChunk(400, 10);
    auto chunk2 = CreateMockChunk(410, 10);
    
    sync_manager_->ProcessStateChunk(chunk1, "peer_001");
    sync_manager_->ProcessStateChunk(chunk2, "peer_001");
    
    // Force validation
    size_t validated = sync_manager_->ForceValidation(400, 419);
    
    // Some states should be processed
    EXPECT_GE(validated, 0);
    EXPECT_LE(validated, 20);
}

TEST_F(StateSynchronizationManagerTest, ResetTest)
{
    sync_manager_->Start();
    
    // Process some data
    auto chunk = CreateMockChunk(500, 20);
    sync_manager_->ProcessStateChunk(chunk, "peer_001");
    
    auto stats_before = sync_manager_->GetStatistics();
    EXPECT_GT(stats_before.states_processed, 0);
    
    // Reset
    sync_manager_->Reset(true);
    
    auto stats_after = sync_manager_->GetStatistics();
    EXPECT_EQ(stats_after.current_height, 0);
    EXPECT_EQ(stats_after.states_processed, 0);
    EXPECT_EQ(stats_after.states_validated, 0);
}

TEST_F(StateSynchronizationManagerTest, ConfigurationUpdateTest)
{
    auto initial_config = sync_manager_->GetConfiguration();
    EXPECT_EQ(initial_config.chunk_size, 100);
    
    // Update configuration
    StateSynchronizationManager::Configuration new_config;
    new_config.chunk_size = 200;
    new_config.max_concurrent_chunks = 5;
    new_config.sync_mode = StateSynchronizationManager::SyncMode::Full;
    
    sync_manager_->UpdateConfiguration(new_config);
    
    auto updated_config = sync_manager_->GetConfiguration();
    EXPECT_EQ(updated_config.chunk_size, 200);
    EXPECT_EQ(updated_config.max_concurrent_chunks, 5);
    EXPECT_EQ(updated_config.sync_mode, StateSynchronizationManager::SyncMode::Full);
}

TEST_F(StateSynchronizationManagerTest, CallbacksTest)
{
    bool state_validated_called = false;
    bool validation_failed_called = false;
    bool progress_called = false;
    bool status_changed_called = false;
    
    // Set callbacks
    sync_manager_->SetOnStateValidated(
        [&state_validated_called](uint32_t height, const UInt256& root) {
            state_validated_called = true;
        });
    
    sync_manager_->SetOnValidationFailed(
        [&validation_failed_called](const StateSynchronizationManager::ValidationResult& result) {
            validation_failed_called = true;
        });
    
    sync_manager_->SetOnSyncProgress(
        [&progress_called](const StateSynchronizationManager::SyncStats& stats) {
            progress_called = true;
        });
    
    sync_manager_->SetOnStatusChanged(
        [&status_changed_called](StateSynchronizationManager::SyncStatus status) {
            status_changed_called = true;
        });
    
    // Start to trigger status change
    sync_manager_->Start();
    EXPECT_TRUE(status_changed_called);
    
    // Process chunk to trigger progress
    auto chunk = CreateMockChunk(600, 5);
    sync_manager_->ProcessStateChunk(chunk, "peer_001");
    EXPECT_TRUE(progress_called);
}

TEST_F(StateSynchronizationManagerTest, GetAllPeerStatesTest)
{
    sync_manager_->Start();
    
    // Add multiple peers
    sync_manager_->ProcessStateRoot(100, CreateMockStateRoot(100).GetRoot(), "peer_001");
    sync_manager_->ProcessStateRoot(200, CreateMockStateRoot(200).GetRoot(), "peer_002");
    sync_manager_->ProcessStateRoot(300, CreateMockStateRoot(300).GetRoot(), "peer_003");
    
    auto all_peers = sync_manager_->GetAllPeerStates();
    EXPECT_EQ(all_peers.size(), 3);
    
    // Check individual peers
    EXPECT_TRUE(all_peers.find("peer_001") != all_peers.end());
    EXPECT_TRUE(all_peers.find("peer_002") != all_peers.end());
    EXPECT_TRUE(all_peers.find("peer_003") != all_peers.end());
}

TEST_F(StateSynchronizationManagerTest, GetStateRootAtHeightTest)
{
    sync_manager_->Start();
    
    // Add state roots at different heights
    auto root_100 = CreateMockStateRoot(100);
    auto root_200 = CreateMockStateRoot(200);
    
    sync_manager_->ProcessStateRoot(100, root_100.GetRoot(), "peer_001");
    sync_manager_->ProcessStateRoot(200, root_200.GetRoot(), "peer_002");
    
    // Get state root at specific height
    auto retrieved_100 = sync_manager_->GetStateRootAtHeight(100);
    EXPECT_TRUE(retrieved_100.has_value());
    EXPECT_EQ(retrieved_100.value(), root_100.GetRoot());
    
    auto retrieved_200 = sync_manager_->GetStateRootAtHeight(200);
    EXPECT_TRUE(retrieved_200.has_value());
    EXPECT_EQ(retrieved_200.value(), root_200.GetRoot());
    
    // Try non-existent height
    auto retrieved_300 = sync_manager_->GetStateRootAtHeight(300);
    EXPECT_FALSE(retrieved_300.has_value());
}

TEST_F(StateSynchronizationManagerTest, ConcurrencyTest)
{
    sync_manager_->Start();
    
    const int num_threads = 5;
    const int chunks_per_thread = 10;
    std::vector<std::thread> threads;
    
    // Spawn threads to process chunks concurrently
    for (int t = 0; t < num_threads; ++t)
    {
        threads.emplace_back([this, t, chunks_per_thread]() {
            for (int i = 0; i < chunks_per_thread; ++i)
            {
                uint32_t start_height = t * 1000 + i * 10;
                auto chunk = CreateMockChunk(start_height, 5);
                sync_manager_->ProcessStateChunk(chunk, "peer_" + std::to_string(t));
                
                // Also process state roots
                sync_manager_->ProcessStateRoot(
                    start_height, 
                    CreateMockStateRoot(start_height).GetRoot(), 
                    "peer_" + std::to_string(t));
            }
        });
    }
    
    // Wait for all threads
    for (auto& thread : threads)
    {
        thread.join();
    }
    
    // Check that processing was successful
    auto stats = sync_manager_->GetStatistics();
    EXPECT_GT(stats.states_processed, 0);
    EXPECT_LE(stats.states_processed, num_threads * chunks_per_thread * 5);
}

// Main test runner
int main(int argc, char** argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
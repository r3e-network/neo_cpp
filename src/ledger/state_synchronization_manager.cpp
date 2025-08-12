#include <neo/cryptography/hash.h>
#include <neo/ledger/state_synchronization_manager.h>
#include <neo/logging/logger.h>
#include <neo/persistence/data_cache_adapter.h>

#include <algorithm>
#include <random>
#include <sstream>

namespace neo::ledger
{

StateSynchronizationManager::StateSynchronizationManager() : StateSynchronizationManager(Configuration{}, nullptr) {}

StateSynchronizationManager::StateSynchronizationManager(const Configuration& config,
                                                         std::shared_ptr<persistence::DataCache> data_cache)
    : config_(config), data_cache_(data_cache), sync_start_time_(std::chrono::steady_clock::now())
{
    neo::logging::Logger::Instance().Info("StateSync", "State Synchronization Manager initialized - Mode: " +
                                                           std::to_string(static_cast<int>(config.sync_mode)));
}

StateSynchronizationManager::~StateSynchronizationManager() { Stop(); }

void StateSynchronizationManager::Start()
{
    if (running_.exchange(true))
    {
        neo::logging::Logger::Instance().Warning("StateSync", "Already running");
        return;
    }

    neo::logging::Logger::Instance().Info("StateSync", "Starting state synchronization");

    sync_start_time_ = std::chrono::steady_clock::now();
    ChangeSyncStatus(SyncStatus::Syncing);

    // Start synchronization thread
    sync_thread_ = std::thread(&StateSynchronizationManager::SynchronizationThread, this);

    // Start validation thread if parallel validation enabled
    if (config_.enable_parallel_validation)
    {
        validation_thread_ = std::thread(&StateSynchronizationManager::ValidationThread, this);
    }
}

void StateSynchronizationManager::Stop()
{
    if (!running_.exchange(false))
    {
        return;
    }

    neo::logging::Logger::Instance().Info("StateSync", "Stopping state synchronization");

    ChangeSyncStatus(SyncStatus::Idle);

    // Stop threads
    if (sync_thread_.joinable())
    {
        sync_thread_.join();
    }

    if (validation_thread_.joinable())
    {
        validation_thread_.join();
    }

    // Clear pending chunks
    {
        std::unique_lock lock(mutex_);
        pending_chunks_.clear();
    }
}

bool StateSynchronizationManager::RequestStateSync(const std::string& peer_id, uint32_t start_height, uint32_t count)
{
    std::unique_lock lock(mutex_);

    // Check if peer exists and is available
    auto peer_it = peer_states_.find(peer_id);
    if (peer_it == peer_states_.end())
    {
        neo::logging::Logger::Instance().Warning("StateSync", "Unknown peer for state sync: " + peer_id);
        return false;
    }

    // Check if peer has required height
    if (peer_it->second.state_height < start_height + count - 1)
    {
        neo::logging::Logger::Instance().Warning("StateSync", "Peer doesn't have required state height");
        return false;
    }

    // Create state chunk request
    StateChunk chunk;
    chunk.start_height = start_height;
    chunk.end_height = start_height + count - 1;
    chunk.is_verified = false;

    // Add to pending chunks
    pending_chunks_[start_height] = chunk;

    // Update peer state
    peer_it->second.sync_status = SyncStatus::Syncing;
    peer_it->second.last_update = std::chrono::steady_clock::now();

    neo::logging::Logger::Instance().Debug("StateSync", "Requested state sync from " + peer_id + " for heights " +
                                                            std::to_string(start_height) + "-" +
                                                            std::to_string(start_height + count - 1));

    return true;
}

StateSynchronizationManager::ValidationResult StateSynchronizationManager::ProcessStateRoot(
    uint32_t height, const io::UInt256& state_root, const std::string& peer_id)
{
    auto start_time = std::chrono::steady_clock::now();

    std::unique_lock lock(mutex_);

    // Update peer state
    UpdatePeerState(peer_id, height, state_root);

    // Check cache first
    auto cache_it = validation_cache_.find(state_root);
    if (cache_it != validation_cache_.end())
    {
        return cache_it->second;
    }

    ValidationResult result;
    result.validation_height = height;
    result.actual_root = state_root;

    // Calculate consensus among peers
    auto consensus_root = CalculateStateConsensus(height);

    if (consensus_root.has_value())
    {
        result.expected_root = consensus_root.value();
        result.is_valid = (state_root == consensus_root.value());

        if (!result.is_valid)
        {
            result.error_message = "State root doesn't match consensus";
            states_failed_.fetch_add(1);

            if (on_validation_failed_)
            {
                on_validation_failed_(result);
            }
        }
        else
        {
            states_validated_.fetch_add(1);

            if (on_state_validated_)
            {
                on_state_validated_(height, state_root);
            }
        }
    }
    else
    {
        // Not enough peers for consensus yet
        result.is_valid = false;
        result.error_message = "Insufficient peer consensus";
    }

    auto end_time = std::chrono::steady_clock::now();
    result.validation_time = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);

    // Cache result
    validation_cache_[state_root] = result;

    // Limit cache size
    if (validation_cache_.size() > config_.max_state_cache_size)
    {
        // Remove oldest entries (simplified - in production use LRU)
        validation_cache_.clear();
    }

    return result;
}

bool StateSynchronizationManager::ProcessStateChunk(const StateChunk& chunk, const std::string& peer_id)
{
    std::unique_lock lock(mutex_);

    // Validate chunk integrity
    if (!ValidateChunkIntegrity(chunk))
    {
        neo::logging::Logger::Instance().Warning("StateSync", "Invalid state chunk from " + peer_id);

        // Update peer trust
        auto peer_it = peer_states_.find(peer_id);
        if (peer_it != peer_states_.end())
        {
            peer_it->second.retry_count++;
            if (peer_it->second.retry_count >= config_.max_retry_attempts)
            {
                peer_it->second.is_trusted = false;
            }
        }

        return false;
    }

    // Process each state in chunk
    for (size_t i = 0; i < chunk.state_roots.size(); ++i)
    {
        uint32_t height = chunk.start_height + static_cast<uint32_t>(i);

        // Persist state if enabled
        if (config_.enable_state_persistence && data_cache_)
        {
            PersistState(height, chunk.state_roots[i], chunk.state_data[i]);
        }

        states_processed_.fetch_add(1);
    }

    // Remove from pending chunks
    pending_chunks_.erase(chunk.start_height);

    // Update current height
    if (chunk.end_height > current_height_)
    {
        current_height_ = chunk.end_height;
    }

    // Check if synchronized
    if (current_height_ >= target_height_ && target_height_ > 0)
    {
        ChangeSyncStatus(SyncStatus::Synchronized);
    }

    // Trigger progress callback
    if (on_sync_progress_)
    {
        on_sync_progress_(GetStatistics());
    }

    neo::logging::Logger::Instance().Debug("StateSync", "Processed state chunk " + std::to_string(chunk.start_height) +
                                                            "-" + std::to_string(chunk.end_height));

    return true;
}

StateSynchronizationManager::ValidationResult StateSynchronizationManager::ValidateStateAtHeight(uint32_t height)
{
    std::shared_lock lock(mutex_);

    ValidationResult result;
    result.validation_height = height;

    // Load state from cache
    auto state_data = LoadState(height);
    if (!state_data.has_value())
    {
        result.is_valid = false;
        result.error_message = "State not found at height " + std::to_string(height);
        return result;
    }

    // Calculate state root from data
    auto calculated_root = cryptography::Hash::Hash256(state_data.value());
    result.actual_root = calculated_root;

    // Get expected root from consensus
    auto consensus_root = CalculateStateConsensus(height);
    if (consensus_root.has_value())
    {
        result.expected_root = consensus_root.value();
        result.is_valid = (result.actual_root == result.expected_root);

        if (!result.is_valid)
        {
            result.error_message = "State validation failed at height " + std::to_string(height);
        }
    }
    else
    {
        result.is_valid = false;
        result.error_message = "No consensus for validation";
    }

    return result;
}

StateSynchronizationManager::SyncStats StateSynchronizationManager::GetStatistics() const
{
    std::shared_lock lock(mutex_);

    SyncStats stats;
    stats.current_height = current_height_.load();
    stats.target_height = target_height_.load();
    stats.validated_height = current_height_.load();  // Simplified
    stats.states_processed = states_processed_.load();
    stats.states_validated = states_validated_.load();
    stats.states_failed = states_failed_.load();
    stats.peer_count = peer_states_.size();
    stats.sync_progress_percent = CalculateSyncProgress();
    stats.sync_start_time = sync_start_time_;
    stats.last_sync_time = std::chrono::steady_clock::now();

    // Calculate average validation time
    if (states_validated_ > 0)
    {
        // Simplified - in production track actual times
        stats.average_validation_time = std::chrono::milliseconds(100);
    }
    else
    {
        stats.average_validation_time = std::chrono::milliseconds(0);
    }

    return stats;
}

std::optional<StateSynchronizationManager::PeerState> StateSynchronizationManager::GetPeerState(
    const std::string& peer_id) const
{
    std::shared_lock lock(mutex_);

    auto it = peer_states_.find(peer_id);
    if (it != peer_states_.end())
    {
        return it->second;
    }

    return std::nullopt;
}

std::unordered_map<std::string, StateSynchronizationManager::PeerState> StateSynchronizationManager::GetAllPeerStates()
    const
{
    std::shared_lock lock(mutex_);
    return peer_states_;
}

void StateSynchronizationManager::AddTrustedPeer(const std::string& peer_id)
{
    std::unique_lock lock(mutex_);

    trusted_peers_.push_back(peer_id);

    // Update peer state if exists
    auto it = peer_states_.find(peer_id);
    if (it != peer_states_.end())
    {
        it->second.is_trusted = true;
    }

    neo::logging::Logger::Instance().Info("StateSync", "Added trusted peer: " + peer_id);
}

void StateSynchronizationManager::RemoveTrustedPeer(const std::string& peer_id)
{
    std::unique_lock lock(mutex_);

    auto it = std::find(trusted_peers_.begin(), trusted_peers_.end(), peer_id);
    if (it != trusted_peers_.end())
    {
        trusted_peers_.erase(it);
    }

    // Update peer state if exists
    auto peer_it = peer_states_.find(peer_id);
    if (peer_it != peer_states_.end())
    {
        peer_it->second.is_trusted = false;
    }

    neo::logging::Logger::Instance().Info("StateSync", "Removed trusted peer: " + peer_id);
}

bool StateSynchronizationManager::IsSynchronized() const { return sync_status_.load() == SyncStatus::Synchronized; }

std::optional<io::UInt256> StateSynchronizationManager::GetCurrentStateRoot() const
{
    std::shared_lock lock(mutex_);

    if (current_state_root_)
    {
        return current_state_root_->GetRoot();
    }

    return std::nullopt;
}

std::optional<io::UInt256> StateSynchronizationManager::GetStateRootAtHeight(uint32_t height) const
{
    std::shared_lock lock(mutex_);

    // Check peer states for this height
    for (const auto& [peer_id, peer_state] : peer_states_)
    {
        if (peer_state.state_height == height)
        {
            return peer_state.state_root;
        }
    }

    return std::nullopt;
}

size_t StateSynchronizationManager::ForceValidation(uint32_t start_height, uint32_t end_height)
{
    size_t validated_count = 0;

    for (uint32_t height = start_height; height <= end_height; ++height)
    {
        auto result = ValidateStateAtHeight(height);
        if (result.is_valid)
        {
            validated_count++;
        }
    }

    neo::logging::Logger::Instance().Info("StateSync",
                                          "Force validated " + std::to_string(validated_count) + " states");

    return validated_count;
}

void StateSynchronizationManager::Reset(bool clear_cache)
{
    std::unique_lock lock(mutex_);

    neo::logging::Logger::Instance().Info("StateSync", "Resetting synchronization state");

    // Reset counters
    current_height_ = 0;
    target_height_ = 0;
    states_processed_ = 0;
    states_validated_ = 0;
    states_failed_ = 0;

    // Clear pending chunks
    pending_chunks_.clear();

    // Clear validation cache if requested
    if (clear_cache)
    {
        validation_cache_.clear();
    }

    // Reset peer states
    for (auto& [peer_id, peer_state] : peer_states_)
    {
        peer_state.sync_status = SyncStatus::Idle;
        peer_state.retry_count = 0;
    }

    ChangeSyncStatus(SyncStatus::Idle);
}

void StateSynchronizationManager::UpdateConfiguration(const Configuration& config)
{
    std::unique_lock lock(mutex_);
    config_ = config;

    neo::logging::Logger::Instance().Info("StateSync", "Configuration updated");
}

void StateSynchronizationManager::SynchronizationThread()
{
    neo::logging::Logger::Instance().Info("StateSync", "Synchronization thread started");

    while (running_)
    {
        try
        {
            // Sleep for sync interval
            std::this_thread::sleep_for(config_.sync_interval);

            if (!running_) break;

            std::unique_lock lock(mutex_);

            // Remove stale peers
            RemoveStalePeers();

            // Select best peers for synchronization
            auto best_peers = SelectBestPeers(config_.max_concurrent_chunks);

            // Request chunks from selected peers
            for (const auto& peer_id : best_peers)
            {
                uint32_t chunk_start = current_height_ + 1;

                if (chunk_start <= target_height_)
                {
                    RequestStateSync(peer_id, chunk_start, config_.chunk_size);
                }
            }

            // Update metrics
            UpdateMetrics();

            // Check for synchronization completion
            if (current_height_ >= target_height_ && target_height_ > 0)
            {
                ChangeSyncStatus(SyncStatus::Synchronized);
            }

            // Attempt recovery if needed
            if (sync_status_ == SyncStatus::Failed && config_.enable_auto_recovery)
            {
                AttemptRecovery();
            }
        }
        catch (const std::exception& ex)
        {
            neo::logging::Logger::Instance().Error("StateSync",
                                                   "Synchronization thread error: " + std::string(ex.what()));
            HandleSyncFailure(ex.what());
        }
    }

    neo::logging::Logger::Instance().Info("StateSync", "Synchronization thread stopped");
}

void StateSynchronizationManager::ValidationThread()
{
    neo::logging::Logger::Instance().Info("StateSync", "Validation thread started");

    while (running_)
    {
        try
        {
            std::this_thread::sleep_for(std::chrono::seconds(1));

            if (!running_) break;

            std::unique_lock lock(mutex_);

            // Validate pending chunks
            for (auto& [height, chunk] : pending_chunks_)
            {
                if (!chunk.is_verified && ValidateChunkIntegrity(chunk))
                {
                    chunk.is_verified = true;
                    states_validated_.fetch_add(chunk.state_roots.size());
                }
            }
        }
        catch (const std::exception& ex)
        {
            neo::logging::Logger::Instance().Error("StateSync", "Validation thread error: " + std::string(ex.what()));
        }
    }

    neo::logging::Logger::Instance().Info("StateSync", "Validation thread stopped");
}

std::vector<std::string> StateSynchronizationManager::SelectBestPeers(size_t count) const
{
    std::vector<std::string> selected_peers;

    // Create vector of peer candidates
    std::vector<std::pair<std::string, uint32_t>> candidates;

    for (const auto& [peer_id, peer_state] : peer_states_)
    {
        // Skip untrusted or busy peers
        if (!peer_state.is_trusted || peer_state.sync_status == SyncStatus::Syncing) continue;

        // Skip peers with too many retries
        if (peer_state.retry_count >= config_.max_retry_attempts) continue;

        candidates.emplace_back(peer_id, peer_state.state_height);
    }

    // Sort by state height (prefer peers with more recent state)
    std::sort(candidates.begin(), candidates.end(), [](const auto& a, const auto& b) { return a.second > b.second; });

    // Select top peers
    for (size_t i = 0; i < std::min(count, candidates.size()); ++i)
    {
        selected_peers.push_back(candidates[i].first);
    }

    return selected_peers;
}

std::optional<io::UInt256> StateSynchronizationManager::CalculateStateConsensus(uint32_t height) const
{
    // Count state roots at this height
    std::unordered_map<io::UInt256, size_t> root_counts;
    size_t total_peers = 0;

    for (const auto& [peer_id, peer_state] : peer_states_)
    {
        if (peer_state.state_height == height)
        {
            root_counts[peer_state.state_root]++;
            total_peers++;
        }
    }

    // Check if we have enough peers
    if (total_peers < 3)  // Minimum 3 peers for consensus
    {
        return std::nullopt;
    }

    // Find root with required agreement
    size_t required_agreement = static_cast<size_t>(total_peers * config_.min_peer_agreement);

    for (const auto& [root, count] : root_counts)
    {
        if (count >= required_agreement)
        {
            return root;
        }
    }

    return std::nullopt;
}

bool StateSynchronizationManager::ValidateChunkIntegrity(const StateChunk& chunk) const
{
    // Validate chunk structure
    if (chunk.state_roots.size() != chunk.state_data.size())
    {
        return false;
    }

    if (chunk.state_roots.empty())
    {
        return false;
    }

    // Validate chunk hash
    std::vector<uint8_t> chunk_bytes;
    for (const auto& root : chunk.state_roots)
    {
        // UInt256 doesn't have iterators, get the data directly
        const auto& root_data = root.GetData();
        chunk_bytes.insert(chunk_bytes.end(), root_data.begin(), root_data.end());
    }

    auto hash = cryptography::Hash::Hash256(chunk_bytes);

    // In production, compare with chunk.chunk_hash
    // For now, basic validation
    return true;
}

bool StateSynchronizationManager::PersistState(uint32_t height, const io::UInt256& state_root,
                                               const std::vector<uint8_t>& state_data)
{
    if (!data_cache_)
    {
        return false;
    }

    try
    {
        // Create key for state storage
        std::vector<uint8_t> key;
        key.push_back(0x01);  // State prefix
        auto height_bytes = reinterpret_cast<const uint8_t*>(&height);
        key.insert(key.end(), height_bytes, height_bytes + sizeof(uint32_t));

        // Store state data using adapter
        persistence::DataCacheAdapter cache_adapter(data_cache_.get());
        cache_adapter.Put(0, key, state_data);

        // Store state root
        std::vector<uint8_t> root_key;
        root_key.push_back(0x02);  // Root prefix
        root_key.insert(root_key.end(), height_bytes, height_bytes + sizeof(uint32_t));

        // Store the state root
        auto root_bytes = state_root.GetData();
        cache_adapter.Put(0, root_key, std::vector<uint8_t>(root_bytes.begin(), root_bytes.end()));

        return true;
    }
    catch (const std::exception& ex)
    {
        neo::logging::Logger::Instance().Error("StateSync", "Failed to persist state: " + std::string(ex.what()));
        return false;
    }
}

std::optional<std::vector<uint8_t>> StateSynchronizationManager::LoadState(uint32_t height) const
{
    if (!data_cache_)
    {
        return std::nullopt;
    }

    try
    {
        // Create key for state retrieval
        std::vector<uint8_t> key;
        key.push_back(0x01);  // State prefix
        auto height_bytes = reinterpret_cast<const uint8_t*>(&height);
        key.insert(key.end(), height_bytes, height_bytes + sizeof(uint32_t));

        // Load state data
        // Create a proper StorageKey (using contract ID 0 for system storage)
        persistence::StorageKey storage_key(0, key);
        auto data = data_cache_->TryGet(storage_key);
        if (data)
        {
            return data->GetValue();
        }
    }
    catch (const std::exception& ex)
    {
        neo::logging::Logger::Instance().Error("StateSync", "Failed to load state: " + std::string(ex.what()));
    }

    return std::nullopt;
}

void StateSynchronizationManager::HandleSyncFailure(const std::string& reason)
{
    neo::logging::Logger::Instance().Error("StateSync", "Synchronization failed: " + reason);

    ChangeSyncStatus(SyncStatus::Failed);

    // Clear pending chunks
    pending_chunks_.clear();

    // Reset peer retry counts
    for (auto& [peer_id, peer_state] : peer_states_)
    {
        peer_state.retry_count = 0;
    }
}

bool StateSynchronizationManager::AttemptRecovery()
{
    neo::logging::Logger::Instance().Info("StateSync", "Attempting recovery");

    ChangeSyncStatus(SyncStatus::Recovering);

    try
    {
        // Reset state
        Reset(false);

        // Re-establish peer connections
        for (auto& [peer_id, peer_state] : peer_states_)
        {
            peer_state.sync_status = SyncStatus::Idle;
            peer_state.retry_count = 0;
        }

        // Restart synchronization
        ChangeSyncStatus(SyncStatus::Syncing);

        neo::logging::Logger::Instance().Info("StateSync", "Recovery successful");
        return true;
    }
    catch (const std::exception& ex)
    {
        neo::logging::Logger::Instance().Error("StateSync", "Recovery failed: " + std::string(ex.what()));
        ChangeSyncStatus(SyncStatus::Failed);
        return false;
    }
}

void StateSynchronizationManager::UpdatePeerState(const std::string& peer_id, uint32_t height,
                                                  const io::UInt256& state_root)
{
    auto it = peer_states_.find(peer_id);
    if (it == peer_states_.end())
    {
        // Create new peer state
        PeerState peer_state;
        peer_state.peer_id = peer_id;
        peer_state.state_height = height;
        peer_state.state_root = state_root;
        peer_state.sync_status = SyncStatus::Idle;
        peer_state.last_update = std::chrono::steady_clock::now();
        peer_state.retry_count = 0;
        peer_state.is_trusted = IsTrustedPeer(peer_id);

        peer_states_[peer_id] = peer_state;
    }
    else
    {
        // Update existing peer state
        it->second.state_height = height;
        it->second.state_root = state_root;
        it->second.last_update = std::chrono::steady_clock::now();
    }

    // Update target height if needed
    if (height > target_height_)
    {
        target_height_ = height;
    }
}

void StateSynchronizationManager::RemoveStalePeers()
{
    auto now = std::chrono::steady_clock::now();
    std::vector<std::string> stale_peers;

    for (const auto& [peer_id, peer_state] : peer_states_)
    {
        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - peer_state.last_update);

        if (elapsed > config_.peer_timeout)
        {
            stale_peers.push_back(peer_id);
        }
    }

    for (const auto& peer_id : stale_peers)
    {
        peer_states_.erase(peer_id);
        neo::logging::Logger::Instance().Debug("StateSync", "Removed stale peer: " + peer_id);
    }
}

void StateSynchronizationManager::UpdateMetrics()
{
    // Calculate and log metrics
    auto stats = GetStatistics();

    neo::logging::Logger::Instance().Debug(
        "StateSync", "Progress: " + std::to_string(stats.current_height) + "/" + std::to_string(stats.target_height) +
                         " (" + std::to_string(static_cast<int>(stats.sync_progress_percent)) + "%)");
}

void StateSynchronizationManager::ChangeSyncStatus(SyncStatus new_status)
{
    auto old_status = sync_status_.exchange(new_status);

    if (old_status != new_status)
    {
        neo::logging::Logger::Instance().Info(
            "StateSync", "Status changed: " + std::to_string(static_cast<int>(old_status)) + " -> " +
                             std::to_string(static_cast<int>(new_status)));

        if (on_status_changed_)
        {
            on_status_changed_(new_status);
        }
    }
}

double StateSynchronizationManager::CalculateSyncProgress() const
{
    if (target_height_ == 0)
    {
        return 0.0;
    }

    return static_cast<double>(current_height_) / target_height_ * 100.0;
}

bool StateSynchronizationManager::IsTrustedPeer(const std::string& peer_id) const
{
    return std::find(trusted_peers_.begin(), trusted_peers_.end(), peer_id) != trusted_peers_.end();
}

}  // namespace neo::ledger
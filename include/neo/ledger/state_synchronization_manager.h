#pragma once

#include <neo/io/uint256.h>
#include <neo/ledger/block.h>
#include <neo/ledger/state_root.h>
#include <neo/network/p2p/payloads/state_root_payload.h>
#include <neo/persistence/data_cache.h>

#include <atomic>
#include <chrono>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <thread>
#include <unordered_map>
#include <vector>

namespace neo::ledger
{
/**
 * @brief State Synchronization Manager for blockchain state consistency
 *
 * This class manages the synchronization of blockchain state across the network,
 * ensuring consistency of state roots, MPT (Merkle Patricia Trie) data, and
 * state transitions between peers. It provides C# Neo compatibility while
 * adding monitoring and recovery capabilities.
 */
class StateSynchronizationManager
{
   public:
    /**
     * @brief Synchronization status enumeration
     */
    enum class SyncStatus
    {
        Idle,          // Not syncing
        Syncing,       // Active synchronization
        Validating,    // Validating received state
        Synchronized,  // Fully synchronized
        Failed,        // Synchronization failed
        Recovering     // Recovering from failure
    };

    /**
     * @brief State synchronization mode
     */
    enum class SyncMode
    {
        Full,    // Full state synchronization
        Fast,    // Fast sync with state roots only
        Light,   // Light client mode
        Archive  // Archive node with full history
    };

    /**
     * @brief State validation result
     */
    struct ValidationResult
    {
        bool is_valid;
        std::string error_message;
        io::UInt256 expected_root;
        io::UInt256 actual_root;
        uint32_t validation_height;
        std::chrono::milliseconds validation_time;
    };

    /**
     * @brief Peer state information
     */
    struct PeerState
    {
        std::string peer_id;
        uint32_t state_height;
        io::UInt256 state_root;
        SyncStatus sync_status;
        std::chrono::steady_clock::time_point last_update;
        uint32_t retry_count;
        bool is_trusted;
    };

    /**
     * @brief Synchronization statistics
     */
    struct SyncStats
    {
        uint32_t current_height;
        uint32_t target_height;
        uint32_t validated_height;
        size_t states_processed;
        size_t states_validated;
        size_t states_failed;
        size_t peer_count;
        double sync_progress_percent;
        std::chrono::milliseconds average_validation_time;
        std::chrono::steady_clock::time_point sync_start_time;
        std::chrono::steady_clock::time_point last_sync_time;
    };

    /**
     * @brief State chunk for incremental synchronization
     */
    struct StateChunk
    {
        uint32_t start_height;
        uint32_t end_height;
        std::vector<io::UInt256> state_roots;
        std::vector<std::vector<uint8_t>> state_data;
        io::UInt256 chunk_hash;
        size_t chunk_size;
        bool is_verified;
    };

    /**
     * @brief Configuration for state synchronization
     */
    struct Configuration
    {
        SyncMode sync_mode = SyncMode::Fast;
        uint32_t chunk_size = 1000;  // States per chunk
        uint32_t max_concurrent_chunks = 5;
        std::chrono::seconds sync_interval{30};
        std::chrono::seconds peer_timeout{60};
        std::chrono::seconds validation_timeout{10};
        uint32_t max_retry_attempts = 3;
        bool enable_parallel_validation = true;
        bool enable_state_persistence = true;
        bool enable_auto_recovery = true;
        size_t max_state_cache_size = 10000;
        double min_peer_agreement = 0.66;  // 66% peer agreement required
    };

   private:
    // Core components
    Configuration config_;
    std::shared_ptr<persistence::DataCache> data_cache_;
    std::shared_ptr<StateRoot> current_state_root_;

    // Synchronization state
    std::atomic<SyncStatus> sync_status_{SyncStatus::Idle};
    std::atomic<uint32_t> current_height_{0};
    std::atomic<uint32_t> target_height_{0};

    // Peer management
    std::unordered_map<std::string, PeerState> peer_states_;
    std::vector<std::string> trusted_peers_;

    // State chunks and validation
    std::unordered_map<uint32_t, StateChunk> pending_chunks_;
    std::unordered_map<io::UInt256, ValidationResult> validation_cache_;

    // Threading and synchronization
    std::thread sync_thread_;
    std::thread validation_thread_;
    std::atomic<bool> running_{false};
    mutable std::shared_mutex mutex_;

    // Metrics and monitoring
    std::atomic<size_t> states_processed_{0};
    std::atomic<size_t> states_validated_{0};
    std::atomic<size_t> states_failed_{0};
    std::chrono::steady_clock::time_point sync_start_time_;

    // Callbacks
    std::function<void(uint32_t, const io::UInt256&)> on_state_validated_;
    std::function<void(const ValidationResult&)> on_validation_failed_;
    std::function<void(const SyncStats&)> on_sync_progress_;
    std::function<void(SyncStatus)> on_status_changed_;

   public:
    /**
     * @brief Default constructor
     */
    StateSynchronizationManager();

    /**
     * @brief Constructor with configuration
     * @param config Synchronization configuration
     * @param data_cache Data cache for state persistence
     */
    explicit StateSynchronizationManager(const Configuration& config,
                                         std::shared_ptr<persistence::DataCache> data_cache = nullptr);

    /**
     * @brief Destructor - ensures threads are stopped
     */
    ~StateSynchronizationManager();

    /**
     * @brief Start state synchronization
     */
    void Start();

    /**
     * @brief Stop state synchronization
     */
    void Stop();

    /**
     * @brief Request state synchronization with specific peer
     * @param peer_id Target peer identifier
     * @param start_height Starting block height
     * @param count Number of states to sync
     * @return True if request initiated successfully
     */
    bool RequestStateSync(const std::string& peer_id, uint32_t start_height, uint32_t count);

    /**
     * @brief Process received state root
     * @param height Block height
     * @param state_root State root hash
     * @param peer_id Source peer
     * @return Validation result
     */
    ValidationResult ProcessStateRoot(uint32_t height, const io::UInt256& state_root, const std::string& peer_id);

    /**
     * @brief Process received state chunk
     * @param chunk State chunk data
     * @param peer_id Source peer
     * @return True if chunk processed successfully
     */
    bool ProcessStateChunk(const StateChunk& chunk, const std::string& peer_id);

    /**
     * @brief Validate state at specific height
     * @param height Block height to validate
     * @return Validation result
     */
    ValidationResult ValidateStateAtHeight(uint32_t height);

    /**
     * @brief Get current synchronization status
     * @return Current sync status
     */
    SyncStatus GetSyncStatus() const { return sync_status_.load(); }

    /**
     * @brief Get synchronization statistics
     * @return Current statistics
     */
    SyncStats GetStatistics() const;

    /**
     * @brief Get peer state information
     * @param peer_id Peer identifier
     * @return Optional peer state if found
     */
    std::optional<PeerState> GetPeerState(const std::string& peer_id) const;

    /**
     * @brief Get all peer states
     * @return Map of peer states
     */
    std::unordered_map<std::string, PeerState> GetAllPeerStates() const;

    /**
     * @brief Add trusted peer
     * @param peer_id Peer identifier to trust
     */
    void AddTrustedPeer(const std::string& peer_id);

    /**
     * @brief Remove trusted peer
     * @param peer_id Peer identifier to remove
     */
    void RemoveTrustedPeer(const std::string& peer_id);

    /**
     * @brief Check if state is synchronized
     * @return True if fully synchronized
     */
    bool IsSynchronized() const;

    /**
     * @brief Get current state root
     * @return Current state root if available
     */
    std::optional<io::UInt256> GetCurrentStateRoot() const;

    /**
     * @brief Get state root at specific height
     * @param height Block height
     * @return State root if available
     */
    std::optional<io::UInt256> GetStateRootAtHeight(uint32_t height) const;

    /**
     * @brief Force state validation
     * @param start_height Starting height
     * @param end_height Ending height
     * @return Number of states validated
     */
    size_t ForceValidation(uint32_t start_height, uint32_t end_height);

    /**
     * @brief Reset synchronization state
     * @param clear_cache Clear validation cache
     */
    void Reset(bool clear_cache = false);

    /**
     * @brief Update configuration
     * @param config New configuration
     */
    void UpdateConfiguration(const Configuration& config);

    /**
     * @brief Get current configuration
     * @return Current configuration
     */
    const Configuration& GetConfiguration() const { return config_; }

    // Callback setters
    void SetOnStateValidated(std::function<void(uint32_t, const io::UInt256&)> callback)
    {
        on_state_validated_ = callback;
    }

    void SetOnValidationFailed(std::function<void(const ValidationResult&)> callback)
    {
        on_validation_failed_ = callback;
    }

    void SetOnSyncProgress(std::function<void(const SyncStats&)> callback) { on_sync_progress_ = callback; }

    void SetOnStatusChanged(std::function<void(SyncStatus)> callback) { on_status_changed_ = callback; }

   private:
    /**
     * @brief Main synchronization thread function
     */
    void SynchronizationThread();

    /**
     * @brief Validation thread function
     */
    void ValidationThread();

    /**
     * @brief Select best peers for synchronization
     * @param count Number of peers to select
     * @return Vector of selected peer IDs
     */
    std::vector<std::string> SelectBestPeers(size_t count) const;

    /**
     * @brief Calculate state root consensus among peers
     * @param height Block height
     * @return Consensus state root if agreement reached
     */
    std::optional<io::UInt256> CalculateStateConsensus(uint32_t height) const;

    /**
     * @brief Validate state chunk integrity
     * @param chunk State chunk to validate
     * @return True if valid
     */
    bool ValidateChunkIntegrity(const StateChunk& chunk) const;

    /**
     * @brief Persist state to cache
     * @param height Block height
     * @param state_root State root hash
     * @param state_data State data
     * @return True if persisted successfully
     */
    bool PersistState(uint32_t height, const io::UInt256& state_root, const std::vector<uint8_t>& state_data);

    /**
     * @brief Load state from cache
     * @param height Block height
     * @return State data if available
     */
    std::optional<std::vector<uint8_t>> LoadState(uint32_t height) const;

    /**
     * @brief Handle synchronization failure
     * @param reason Failure reason
     */
    void HandleSyncFailure(const std::string& reason);

    /**
     * @brief Attempt recovery from failure
     * @return True if recovery successful
     */
    bool AttemptRecovery();

    /**
     * @brief Update peer state information
     * @param peer_id Peer identifier
     * @param height State height
     * @param state_root State root hash
     */
    void UpdatePeerState(const std::string& peer_id, uint32_t height, const io::UInt256& state_root);

    /**
     * @brief Remove stale peers
     */
    void RemoveStalePeers();

    /**
     * @brief Update synchronization metrics
     */
    void UpdateMetrics();

    /**
     * @brief Change synchronization status
     * @param new_status New status
     */
    void ChangeSyncStatus(SyncStatus new_status);

    /**
     * @brief Calculate synchronization progress
     * @return Progress percentage (0-100)
     */
    double CalculateSyncProgress() const;

    /**
     * @brief Check if peer is trusted
     * @param peer_id Peer identifier
     * @return True if trusted
     */
    bool IsTrustedPeer(const std::string& peer_id) const;
};

}  // namespace neo::ledger
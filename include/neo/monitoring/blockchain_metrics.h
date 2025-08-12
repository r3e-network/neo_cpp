#pragma once

#include <neo/monitoring/metrics_collector.h>
#include <memory>

namespace neo::monitoring {

/**
 * @brief Blockchain-specific metrics collector
 */
class BlockchainMetrics {
public:
    static BlockchainMetrics& GetInstance() {
        static BlockchainMetrics instance;
        return instance;
    }
    
    // Initialize all blockchain metrics
    void Initialize();
    
    // Block metrics
    void OnBlockReceived();
    void OnBlockProcessed(double processingTime);
    void OnBlockValidated(bool valid);
    void SetBlockHeight(uint32_t height);
    void SetBlockTime(uint32_t timestamp);
    
    // Transaction metrics
    void OnTransactionReceived();
    void OnTransactionProcessed(double processingTime);
    void OnTransactionValidated(bool valid);
    void SetMempoolSize(size_t size);
    void SetMempoolBytes(size_t bytes);
    
    // State metrics
    void SetStateHeight(uint32_t height);
    void SetAccountCount(uint64_t count);
    void SetContractCount(uint64_t count);
    void SetValidatorCount(size_t count);
    
    // Storage metrics
    void SetDatabaseSize(uint64_t bytes);
    void OnDatabaseRead(double duration);
    void OnDatabaseWrite(double duration);
    
    // Consensus metrics
    void OnConsensusStarted();
    void OnConsensusCompleted(double duration);
    void OnViewChange();
    void SetConsensusState(const std::string& state);
    
    // Smart contract metrics
    void OnContractInvocation();
    void OnContractDeployment();
    void OnVMExecution(double duration);
    void OnSystemCall(const std::string& name);
    
private:
    BlockchainMetrics() = default;
    ~BlockchainMetrics() = default;
    
    BlockchainMetrics(const BlockchainMetrics&) = delete;
    BlockchainMetrics& operator=(const BlockchainMetrics&) = delete;
    
    // Block metrics
    std::shared_ptr<Counter> blocks_received_;
    std::shared_ptr<Counter> blocks_processed_;
    std::shared_ptr<Counter> blocks_validated_;
    std::shared_ptr<Counter> blocks_invalid_;
    std::shared_ptr<Gauge> block_height_;
    std::shared_ptr<Gauge> block_time_;
    std::shared_ptr<Histogram> block_processing_time_;
    
    // Transaction metrics
    std::shared_ptr<Counter> transactions_received_;
    std::shared_ptr<Counter> transactions_processed_;
    std::shared_ptr<Counter> transactions_validated_;
    std::shared_ptr<Counter> transactions_invalid_;
    std::shared_ptr<Gauge> mempool_size_;
    std::shared_ptr<Gauge> mempool_bytes_;
    std::shared_ptr<Histogram> transaction_processing_time_;
    
    // State metrics
    std::shared_ptr<Gauge> state_height_;
    std::shared_ptr<Gauge> account_count_;
    std::shared_ptr<Gauge> contract_count_;
    std::shared_ptr<Gauge> validator_count_;
    
    // Storage metrics
    std::shared_ptr<Gauge> database_size_;
    std::shared_ptr<Histogram> database_read_duration_;
    std::shared_ptr<Histogram> database_write_duration_;
    
    // Consensus metrics
    std::shared_ptr<Counter> consensus_rounds_;
    std::shared_ptr<Counter> view_changes_;
    std::shared_ptr<Histogram> consensus_duration_;
    std::shared_ptr<Gauge> consensus_state_;
    
    // Smart contract metrics
    std::shared_ptr<Counter> contract_invocations_;
    std::shared_ptr<Counter> contract_deployments_;
    std::shared_ptr<Histogram> vm_execution_duration_;
    std::shared_ptr<Counter> system_calls_;
};

} // namespace neo::monitoring
/**
 * @file blockchain_metrics.cpp
 * @brief Core blockchain implementation
 * @author Neo C++ Team
 * @date 2025
 * @copyright MIT License
 */

#include <neo/monitoring/blockchain_metrics.h>

namespace neo::monitoring {

void BlockchainMetrics::Initialize() {
    auto& collector = MetricsCollector::GetInstance();
    
    // Initialize block metrics
    blocks_received_ = collector.RegisterCounter(
        "neo_blocks_received_total", 
        "Total number of blocks received");
    
    blocks_processed_ = collector.RegisterCounter(
        "neo_blocks_processed_total", 
        "Total number of blocks processed");
    
    blocks_validated_ = collector.RegisterCounter(
        "neo_blocks_validated_total", 
        "Total number of blocks validated successfully");
    
    blocks_invalid_ = collector.RegisterCounter(
        "neo_blocks_invalid_total", 
        "Total number of invalid blocks");
    
    block_height_ = collector.RegisterGauge(
        "neo_block_height", 
        "Current blockchain height");
    
    block_time_ = collector.RegisterGauge(
        "neo_block_timestamp", 
        "Timestamp of the latest block");
    
    block_processing_time_ = collector.RegisterHistogram(
        "neo_block_processing_duration_seconds", 
        "Time taken to process a block",
        {0.001, 0.005, 0.01, 0.025, 0.05, 0.1, 0.25, 0.5, 1.0, 2.5, 5.0});
    
    // Initialize transaction metrics
    transactions_received_ = collector.RegisterCounter(
        "neo_transactions_received_total", 
        "Total number of transactions received");
    
    transactions_processed_ = collector.RegisterCounter(
        "neo_transactions_processed_total", 
        "Total number of transactions processed");
    
    transactions_validated_ = collector.RegisterCounter(
        "neo_transactions_validated_total", 
        "Total number of transactions validated successfully");
    
    transactions_invalid_ = collector.RegisterCounter(
        "neo_transactions_invalid_total", 
        "Total number of invalid transactions");
    
    mempool_size_ = collector.RegisterGauge(
        "neo_mempool_size", 
        "Number of transactions in mempool");
    
    mempool_bytes_ = collector.RegisterGauge(
        "neo_mempool_bytes", 
        "Size of mempool in bytes");
    
    transaction_processing_time_ = collector.RegisterHistogram(
        "neo_transaction_processing_duration_seconds", 
        "Time taken to process a transaction",
        {0.0001, 0.0005, 0.001, 0.005, 0.01, 0.05, 0.1, 0.5, 1.0});
    
    // Initialize state metrics
    state_height_ = collector.RegisterGauge(
        "neo_state_height", 
        "Current state height");
    
    account_count_ = collector.RegisterGauge(
        "neo_accounts_total", 
        "Total number of accounts");
    
    contract_count_ = collector.RegisterGauge(
        "neo_contracts_total", 
        "Total number of deployed contracts");
    
    validator_count_ = collector.RegisterGauge(
        "neo_validators_total", 
        "Number of active validators");
    
    // Initialize storage metrics
    database_size_ = collector.RegisterGauge(
        "neo_database_size_bytes", 
        "Size of the database in bytes");
    
    database_read_duration_ = collector.RegisterHistogram(
        "neo_database_read_duration_seconds", 
        "Time taken for database read operations",
        {0.00001, 0.00005, 0.0001, 0.0005, 0.001, 0.005, 0.01, 0.05, 0.1});
    
    database_write_duration_ = collector.RegisterHistogram(
        "neo_database_write_duration_seconds", 
        "Time taken for database write operations",
        {0.00001, 0.00005, 0.0001, 0.0005, 0.001, 0.005, 0.01, 0.05, 0.1, 0.5});
    
    // Initialize consensus metrics
    consensus_rounds_ = collector.RegisterCounter(
        "neo_consensus_rounds_total", 
        "Total number of consensus rounds");
    
    view_changes_ = collector.RegisterCounter(
        "neo_consensus_view_changes_total", 
        "Total number of view changes");
    
    consensus_duration_ = collector.RegisterHistogram(
        "neo_consensus_duration_seconds", 
        "Time taken to reach consensus",
        {0.1, 0.5, 1.0, 2.5, 5.0, 10.0, 15.0, 30.0, 60.0});
    
    consensus_state_ = collector.RegisterGauge(
        "neo_consensus_state", 
        "Current consensus state (0=idle, 1=active, 2=view_changing)");
    
    // Initialize smart contract metrics
    contract_invocations_ = collector.RegisterCounter(
        "neo_contract_invocations_total", 
        "Total number of contract invocations");
    
    contract_deployments_ = collector.RegisterCounter(
        "neo_contract_deployments_total", 
        "Total number of contract deployments");
    
    vm_execution_duration_ = collector.RegisterHistogram(
        "neo_vm_execution_duration_seconds", 
        "Time taken for VM execution",
        {0.00001, 0.00005, 0.0001, 0.0005, 0.001, 0.005, 0.01, 0.05, 0.1, 0.5, 1.0});
    
    system_calls_ = collector.RegisterCounter(
        "neo_system_calls_total", 
        "Total number of system calls");
}

void BlockchainMetrics::OnBlockReceived() {
    if (blocks_received_) {
        blocks_received_->Increment();
    }
}

void BlockchainMetrics::OnBlockProcessed(double processingTime) {
    if (blocks_processed_) {
        blocks_processed_->Increment();
    }
    if (block_processing_time_) {
        block_processing_time_->Observe(processingTime);
    }
}

void BlockchainMetrics::OnBlockValidated(bool valid) {
    if (valid && blocks_validated_) {
        blocks_validated_->Increment();
    } else if (!valid && blocks_invalid_) {
        blocks_invalid_->Increment();
    }
}

void BlockchainMetrics::SetBlockHeight(uint32_t height) {
    if (block_height_) {
        block_height_->Set(static_cast<double>(height));
    }
}

void BlockchainMetrics::SetBlockTime(uint32_t timestamp) {
    if (block_time_) {
        block_time_->Set(static_cast<double>(timestamp));
    }
}

void BlockchainMetrics::OnTransactionReceived() {
    if (transactions_received_) {
        transactions_received_->Increment();
    }
}

void BlockchainMetrics::OnTransactionProcessed(double processingTime) {
    if (transactions_processed_) {
        transactions_processed_->Increment();
    }
    if (transaction_processing_time_) {
        transaction_processing_time_->Observe(processingTime);
    }
}

void BlockchainMetrics::OnTransactionValidated(bool valid) {
    if (valid && transactions_validated_) {
        transactions_validated_->Increment();
    } else if (!valid && transactions_invalid_) {
        transactions_invalid_->Increment();
    }
}

void BlockchainMetrics::SetMempoolSize(size_t size) {
    if (mempool_size_) {
        mempool_size_->Set(static_cast<double>(size));
    }
}

void BlockchainMetrics::SetMempoolBytes(size_t bytes) {
    if (mempool_bytes_) {
        mempool_bytes_->Set(static_cast<double>(bytes));
    }
}

void BlockchainMetrics::SetStateHeight(uint32_t height) {
    if (state_height_) {
        state_height_->Set(static_cast<double>(height));
    }
}

void BlockchainMetrics::SetAccountCount(uint64_t count) {
    if (account_count_) {
        account_count_->Set(static_cast<double>(count));
    }
}

void BlockchainMetrics::SetContractCount(uint64_t count) {
    if (contract_count_) {
        contract_count_->Set(static_cast<double>(count));
    }
}

void BlockchainMetrics::SetValidatorCount(size_t count) {
    if (validator_count_) {
        validator_count_->Set(static_cast<double>(count));
    }
}

void BlockchainMetrics::SetDatabaseSize(uint64_t bytes) {
    if (database_size_) {
        database_size_->Set(static_cast<double>(bytes));
    }
}

void BlockchainMetrics::OnDatabaseRead(double duration) {
    if (database_read_duration_) {
        database_read_duration_->Observe(duration);
    }
}

void BlockchainMetrics::OnDatabaseWrite(double duration) {
    if (database_write_duration_) {
        database_write_duration_->Observe(duration);
    }
}

void BlockchainMetrics::OnConsensusStarted() {
    if (consensus_rounds_) {
        consensus_rounds_->Increment();
    }
    if (consensus_state_) {
        consensus_state_->Set(1.0); // Active
    }
}

void BlockchainMetrics::OnConsensusCompleted(double duration) {
    if (consensus_duration_) {
        consensus_duration_->Observe(duration);
    }
    if (consensus_state_) {
        consensus_state_->Set(0.0); // Idle
    }
}

void BlockchainMetrics::OnViewChange() {
    if (view_changes_) {
        view_changes_->Increment();
    }
    if (consensus_state_) {
        consensus_state_->Set(2.0); // View changing
    }
}

void BlockchainMetrics::SetConsensusState(const std::string& state) {
    if (consensus_state_) {
        double value = 0.0;
        if (state == "active") value = 1.0;
        else if (state == "view_changing") value = 2.0;
        consensus_state_->Set(value);
    }
}

void BlockchainMetrics::OnContractInvocation() {
    if (contract_invocations_) {
        contract_invocations_->Increment();
    }
}

void BlockchainMetrics::OnContractDeployment() {
    if (contract_deployments_) {
        contract_deployments_->Increment();
    }
}

void BlockchainMetrics::OnVMExecution(double duration) {
    if (vm_execution_duration_) {
        vm_execution_duration_->Observe(duration);
    }
}

void BlockchainMetrics::OnSystemCall(const std::string& name) {
    if (system_calls_) {
        system_calls_->Increment();
    }
    
    // Could also track individual system call metrics
    auto& collector = MetricsCollector::GetInstance();
    auto counter = collector.RegisterCounter(
        "neo_system_call_" + name + "_total",
        "Number of " + name + " system calls");
    if (counter) {
        counter->Increment();
    }
}

} // namespace neo::monitoring
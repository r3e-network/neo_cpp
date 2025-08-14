/**
 * @file error_handling_example.cpp
 * @brief Example usage of standardized error handling framework
 */

#include <neo/core/error_handling.h>
#include <neo/network/tcp_connection.h>
#include <neo/ledger/transaction.h>
#include <iostream>

namespace neo {
namespace examples {

using namespace neo::error;

/**
 * @brief Example of using Result type for network operations
 */
Result<std::shared_ptr<network::TcpConnection>> ConnectToPeer(
    const std::string& host, 
    uint16_t port) {
    
    // Validate input
    if (host.empty()) {
        return Err<std::shared_ptr<network::TcpConnection>>(
            NeoException(ErrorCode::InvalidArgument, "Host cannot be empty")
        );
    }
    
    if (port == 0) {
        return Err<std::shared_ptr<network::TcpConnection>>(
            NeoException(ErrorCode::InvalidArgument, "Port cannot be zero")
        );
    }
    
    try {
        // Attempt connection
        auto connection = std::make_shared<network::TcpConnection>();
        
        // Simulated connection logic
        if (!connection->Connect(host, port)) {
            return Err<std::shared_ptr<network::TcpConnection>>(
                NeoException(ErrorCode::ConnectionFailed, 
                           std::format("Failed to connect to {}:{}", host, port))
            );
        }
        
        return Ok(connection);
        
    } catch (const std::exception& e) {
        return Err<std::shared_ptr<network::TcpConnection>>(
            NeoException(ErrorCode::NetworkTimeout, e.what())
        );
    }
}

/**
 * @brief Example of chaining Results with AndThen
 */
Result<ledger::Transaction> ProcessTransaction(const std::string& tx_data) {
    // Parse transaction
    auto parse_result = ParseTransaction(tx_data);
    
    // Validate transaction
    return parse_result.AndThen([](ledger::Transaction tx) -> Result<ledger::Transaction> {
        if (!ValidateTransaction(tx)) {
            return Err<ledger::Transaction>(
                NeoException(ErrorCode::InvalidTransaction, "Transaction validation failed")
            );
        }
        return Ok(tx);
    })
    // Apply transaction
    .AndThen([](ledger::Transaction tx) -> Result<ledger::Transaction> {
        if (!ApplyTransaction(tx)) {
            return Err<ledger::Transaction>(
                NeoException(ErrorCode::ValidationFailed, "Failed to apply transaction")
            );
        }
        return Ok(tx);
    });
}

/**
 * @brief Example using TRY macro for early returns
 */
Result<bool> ValidateBlock(const ledger::Block& block) {
    // Check block header
    TRY(ValidateBlockHeader(block.GetHeader()));
    
    // Check each transaction
    for (const auto& tx : block.GetTransactions()) {
        TRY(ValidateTransaction(tx));
    }
    
    // Check merkle root
    TRY(ValidateMerkleRoot(block));
    
    return Ok(true);
}

/**
 * @brief Example using TRY_ASSIGN macro
 */
Result<std::string> GetBlockHash(uint32_t height) {
    // Get block from storage
    TRY_ASSIGN(block, LoadBlockFromStorage(height));
    
    // Compute hash
    TRY_ASSIGN(hash, ComputeBlockHash(block));
    
    return Ok(hash.ToString());
}

/**
 * @brief Example using ErrorGuard for cleanup
 */
Result<bool> UpdateStorage(const std::string& key, const std::string& value) {
    // Acquire lock
    auto lock = AcquireStorageLock();
    
    // Setup error recovery
    ErrorGuard guard([&lock]() {
        // Release lock on error
        lock.Release();
        // Rollback any partial changes
        RollbackStorageChanges();
    });
    
    // Perform update
    if (!WriteToStorage(key, value)) {
        return Err<bool>(
            NeoException(ErrorCode::WriteError, "Failed to write to storage")
        );
    }
    
    // Commit changes
    if (!CommitStorageChanges()) {
        return Err<bool>(
            NeoException(ErrorCode::StorageCorrupted, "Failed to commit changes")
        );
    }
    
    // Success - dismiss the guard
    guard.Dismiss();
    return Ok(true);
}

/**
 * @brief Example using NEO_REQUIRE for preconditions
 */
void TransferFunds(const std::string& from, const std::string& to, uint64_t amount) {
    NEO_REQUIRE(!from.empty(), ErrorCode::InvalidArgument, "From address cannot be empty");
    NEO_REQUIRE(!to.empty(), ErrorCode::InvalidArgument, "To address cannot be empty");
    NEO_REQUIRE(amount > 0, ErrorCode::InvalidArgument, "Amount must be positive");
    
    auto balance = GetBalance(from);
    NEO_REQUIRE(balance >= amount, ErrorCode::InsufficientFunds, 
               std::format("Insufficient balance: {} < {}", balance, amount));
    
    // Perform transfer
    DeductBalance(from, amount);
    AddBalance(to, amount);
    
    // Ensure postcondition
    NEO_ENSURE(GetBalance(from) == balance - amount, 
              ErrorCode::ValidationFailed, "Balance mismatch after transfer");
}

/**
 * @brief Example error handler registration
 */
void SetupErrorHandling() {
    ErrorContext::Instance().SetErrorHandler([](const NeoException& error) {
        // Log error with severity based on error code
        Severity severity = Severity::Error;
        
        if (static_cast<int>(error.code()) >= 5000) {
            severity = Severity::Critical;
        }
        
        // Log to appropriate sink
        std::cerr << "[" << SeverityToString(severity) << "] " 
                  << error.what() << std::endl;
        
        // Send metrics
        RecordErrorMetric(error.code());
        
        // Alert if critical
        if (severity == Severity::Critical) {
            SendAlert(error);
        }
    });
}

/**
 * @brief Example of custom error types
 */
class NetworkError : public NeoException {
public:
    NetworkError(const std::string& peer, const std::string& reason)
        : NeoException(ErrorCode::NetworkTimeout, 
                      std::format("Network error with peer {}: {}", peer, reason))
        , peer_(peer) {}
    
    const std::string& peer() const { return peer_; }
    
private:
    std::string peer_;
};

/**
 * @brief Example usage in main application flow
 */
void ExampleMainFlow() {
    // Connect to peer
    auto conn_result = ConnectToPeer("localhost", 8080);
    if (conn_result.IsError()) {
        std::cerr << "Connection failed: " << conn_result.Error().what() << std::endl;
        return;
    }
    
    auto connection = conn_result.Value();
    
    // Process transaction with error handling
    auto tx_result = ProcessTransaction("{\"type\":\"transfer\"}");
    
    tx_result
        .Map([](const ledger::Transaction& tx) {
            std::cout << "Transaction processed: " << tx.GetHash().ToString() << std::endl;
            return tx;
        })
        .MapError([](const NeoException& error) {
            ErrorContext::Instance().HandleError(error);
            return error;
        });
    
    // Use traditional try-catch when needed
    try {
        TransferFunds("addr1", "addr2", 1000);
    } catch (const NeoException& e) {
        std::cerr << "Transfer failed: " << e.what() << std::endl;
        // Attempt recovery
        if (e.code() == ErrorCode::InsufficientFunds) {
            // Handle specific error
        }
    }
}

} // namespace examples
} // namespace neo
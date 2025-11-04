/**
 * @file transaction_validator.h
 * @brief Public helpers for transaction validation
 */

#pragma once

#include <memory>
#include <neo/ledger/transaction.h>

namespace neo::ledger
{

class Blockchain;
class MemoryPool;

/**
 * @brief Result codes returned when validating a transaction.
 */
enum class ValidationResult : uint8_t
{
    Valid = 0,
    InvalidFormat,
    InvalidSize,
    InvalidAttribute,
    InvalidScript,
    InvalidWitness,
    InsufficientFunds,
    InvalidSignature,
    AlreadyExists,
    Expired,
    InvalidSystemFee,
    InvalidNetworkFee,
    PolicyViolation,
    Unknown
};

/**
 * @brief Validate a Neo transaction against the current blockchain snapshot and optional mempool.
 * @param transaction Transaction to validate.
 * @param blockchain Blockchain instance providing current state.
 * @param mempool Optional mempool used to check conflicts.
 * @return Detailed validation result indicating success or failure reason.
 */
ValidationResult ValidateTransaction(const Transaction& transaction, std::shared_ptr<Blockchain> blockchain,
                                     std::shared_ptr<MemoryPool> mempool = nullptr);

}  // namespace neo::ledger

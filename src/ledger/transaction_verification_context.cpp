/**
 * @file transaction_verification_context.cpp
 * @brief Transaction types and processing
 * @author Neo C++ Team
 * @date 2025
 * @copyright MIT License
 */

#include <neo/ledger/transaction.h>
#include <neo/ledger/transaction_verification_context.h>

#include <sstream>

namespace neo::ledger
{
TransactionVerificationContext::TransactionVerificationContext() = default;

TransactionVerificationContext::~TransactionVerificationContext() = default;

bool TransactionVerificationContext::CheckTransaction(std::shared_ptr<Transaction> transaction)
{
    if (!transaction)
    {
        return false;
    }

    auto hash = transaction->GetHash();

    // Check if transaction is already in context
    if (transaction_hashes_.find(hash) != transaction_hashes_.end())
    {
        return false;
    }

    // Check for output conflicts
    if (HasOutputConflict(transaction))
    {
        return false;
    }

    // Check for account conflicts
    if (HasAccountConflict(transaction))
    {
        return false;
    }

    return true;
}

void TransactionVerificationContext::AddTransaction(std::shared_ptr<Transaction> transaction)
{
    if (!transaction)
    {
        return;
    }

    auto hash = transaction->GetHash();

    // Add transaction hash
    transaction_hashes_.insert(hash);

    // Track account conflicts (Neo N3 uses account-based model, not UTXO)
    for (const auto& signer : transaction->GetSigners())
    {
        account_conflicts_[signer.GetAccount()] = hash;
    }
}

bool TransactionVerificationContext::IsConflicted(std::shared_ptr<Transaction> transaction) const
{
    if (!transaction)
    {
        return false;
    }

    return HasOutputConflict(transaction) || HasAccountConflict(transaction);
}

void TransactionVerificationContext::Reset() { Clear(); }

void TransactionVerificationContext::Clear()
{
    account_conflicts_.clear();
    transaction_hashes_.clear();
}

size_t TransactionVerificationContext::GetTransactionCount() const { return transaction_hashes_.size(); }

bool TransactionVerificationContext::HasOutputConflict(std::shared_ptr<Transaction> transaction) const
{
    // Neo N3 uses account-based model, no UTXO inputs to check
    // Output conflicts are managed at the account level through signers
    return false;
}

bool TransactionVerificationContext::HasAccountConflict(std::shared_ptr<Transaction> transaction) const
{
    for (const auto& signer : transaction->GetSigners())
    {
        if (account_conflicts_.find(signer.GetAccount()) != account_conflicts_.end())
        {
            return true;
        }
    }
    return false;
}

// TransactionRemovedEventArgs is implemented in memory_pool.cpp
}  // namespace neo::ledger

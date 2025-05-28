#include <neo/ledger/transaction_verification_context.h>

namespace neo::ledger
{
    TransactionVerificationContext::TransactionVerificationContext()
    {
    }

    VerifyResult TransactionVerificationContext::AddTransaction(std::shared_ptr<Transaction> transaction)
    {
        if (!transaction)
        {
            return VerifyResult::Invalid;
        }

        auto hash = transaction->GetHash();

        // Check if transaction already exists
        if (Contains(hash))
        {
            return VerifyResult::AlreadyExists;
        }

        // Validate the transaction
        auto result = ValidateTransaction(transaction);
        if (result != VerifyResult::Succeed)
        {
            return result;
        }

        // Check for conflicts
        result = CheckConflicts(transaction);
        if (result != VerifyResult::Succeed)
        {
            return result;
        }

        // Add to context
        transaction_hashes_.insert(hash);
        transactions_[hash] = transaction;

        return VerifyResult::Succeed;
    }

    bool TransactionVerificationContext::RemoveTransaction(const io::UInt256& hash)
    {
        auto it = transactions_.find(hash);
        if (it != transactions_.end())
        {
            transactions_.erase(it);
            transaction_hashes_.erase(hash);
            return true;
        }
        return false;
    }

    bool TransactionVerificationContext::Contains(const io::UInt256& hash) const
    {
        return transaction_hashes_.count(hash) > 0;
    }

    size_t TransactionVerificationContext::Count() const
    {
        return transactions_.size();
    }

    void TransactionVerificationContext::Clear()
    {
        transaction_hashes_.clear();
        transactions_.clear();
    }

    VerifyResult TransactionVerificationContext::CheckTransaction(std::shared_ptr<Transaction> transaction) const
    {
        if (!transaction)
        {
            return VerifyResult::Invalid;
        }

        auto hash = transaction->GetHash();

        // Check if transaction already exists
        if (Contains(hash))
        {
            return VerifyResult::AlreadyExists;
        }

        // Validate the transaction
        auto result = ValidateTransaction(transaction);
        if (result != VerifyResult::Succeed)
        {
            return result;
        }

        // Check for conflicts
        return CheckConflicts(transaction);
    }

    std::unordered_set<io::UInt256> TransactionVerificationContext::GetTransactionHashes() const
    {
        return transaction_hashes_;
    }

    VerifyResult TransactionVerificationContext::CheckConflicts(std::shared_ptr<Transaction> transaction) const
    {
        // Check for transaction hash conflicts (double spending)
        auto txHash = transaction->GetHash();
        if (transaction_hashes_.find(txHash) != transaction_hashes_.end())
        {
            return VerifyResult::AlreadyExists;
        }

        // Check for oracle response conflicts
        auto oracleAttr = transaction->GetOracleResponse();
        if (oracleAttr)
        {
            auto oracleId = oracleAttr->GetId();
            // Check if any existing transaction has the same oracle response ID
            for (const auto& [existingHash, existingTx] : transactions_)
            {
                auto existingOracleAttr = existingTx->GetOracleResponse();
                if (existingOracleAttr && existingOracleAttr->GetId() == oracleId)
                {
                    return VerifyResult::HasConflicts;
                }
            }
        }

        return VerifyResult::Succeed;
    }

    VerifyResult TransactionVerificationContext::ValidateTransaction(std::shared_ptr<Transaction> transaction) const
    {
        // Basic transaction validation
        // This is a simplified implementation

        try
        {
            // Check transaction size
            if (transaction->GetSize() == 0)
            {
                return VerifyResult::Invalid;
            }

            // Check if transaction is expired
            // This would require access to current block height
            // For now, we'll assume it's valid

            // Check network fee
            if (transaction->GetNetworkFee() < 0)
            {
                return VerifyResult::InsufficientFunds;
            }

            // Check system fee
            if (transaction->GetSystemFee() < 0)
            {
                return VerifyResult::InsufficientFunds;
            }

            // Additional validations would go here

            return VerifyResult::Succeed;
        }
        catch (...)
        {
            return VerifyResult::Invalid;
        }
    }

    // TransactionRemovedEventArgs implementation
    TransactionRemovedEventArgs::TransactionRemovedEventArgs(std::shared_ptr<Transaction> transaction, TransactionRemovalReason reason)
        : transaction_(transaction), reason_(reason)
    {
    }

    std::shared_ptr<Transaction> TransactionRemovedEventArgs::GetTransaction() const
    {
        return transaction_;
    }

    TransactionRemovalReason TransactionRemovedEventArgs::GetReason() const
    {
        return reason_;
    }
}

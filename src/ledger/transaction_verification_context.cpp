#include <neo/ledger/transaction_verification_context.h>
#include <neo/ledger/transaction.h>
#include <sstream>

namespace neo::ledger
{
    TransactionVerificationContext::TransactionVerificationContext() = default;

    TransactionVerificationContext::~TransactionVerificationContext() = default;

    bool TransactionVerificationContext::CheckTransaction(std::shared_ptr<Transaction> transaction)
    {
        if (!transaction) {
            return false;
        }

        auto hash = transaction->GetHash();
        
        // Check if transaction is already in context
        if (transaction_hashes_.find(hash) != transaction_hashes_.end()) {
            return false;
        }

        // Check for output conflicts
        if (HasOutputConflict(transaction)) {
            return false;
        }

        // Check for account conflicts
        if (HasAccountConflict(transaction)) {
            return false;
        }

        return true;
    }

    void TransactionVerificationContext::AddTransaction(std::shared_ptr<Transaction> transaction)
    {
        if (!transaction) {
            return;
        }

        auto hash = transaction->GetHash();
        
        // Add transaction hash
        transaction_hashes_.insert(hash);

        // Track used outputs
        for (const auto& input : transaction->GetInputs()) {
            auto key = MakeOutputKey(input.GetPrevHash(), input.GetPrevIndex());
            used_outputs_[key] = hash;
        }

        // Track account conflicts
        for (const auto& signer : transaction->GetSigners()) {
            account_conflicts_[signer.account] = hash;
        }
    }

    bool TransactionVerificationContext::IsConflicted(std::shared_ptr<Transaction> transaction) const
    {
        if (!transaction) {
            return false;
        }

        return HasOutputConflict(transaction) || HasAccountConflict(transaction);
    }

    void TransactionVerificationContext::Reset()
    {
        Clear();
    }

    void TransactionVerificationContext::Clear()
    {
        used_outputs_.clear();
        account_conflicts_.clear();
        transaction_hashes_.clear();
    }

    size_t TransactionVerificationContext::GetTransactionCount() const
    {
        return transaction_hashes_.size();
    }

    std::string TransactionVerificationContext::MakeOutputKey(const io::UInt256& prev_hash, uint32_t index) const
    {
        std::ostringstream oss;
        oss << prev_hash.ToString() << ":" << index;
        return oss.str();
    }

    bool TransactionVerificationContext::HasOutputConflict(std::shared_ptr<Transaction> transaction) const
    {
        for (const auto& input : transaction->GetInputs()) {
            auto key = MakeOutputKey(input.GetPrevHash(), input.GetPrevIndex());
            if (used_outputs_.find(key) != used_outputs_.end()) {
                return true;
            }
        }
        return false;
    }

    bool TransactionVerificationContext::HasAccountConflict(std::shared_ptr<Transaction> transaction) const
    {
        for (const auto& signer : transaction->GetSigners()) {
            if (account_conflicts_.find(signer.account) != account_conflicts_.end()) {
                return true;
            }
        }
        return false;
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

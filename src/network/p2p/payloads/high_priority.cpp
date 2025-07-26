#include <neo/network/p2p/payloads/high_priority.h>

namespace neo::network::p2p::payloads
{
    HighPriority::HighPriority()
    {
    }

    ledger::TransactionAttributeType HighPriority::GetType() const
    {
        return ledger::TransactionAttributeType::HighPriority;
    }

    bool HighPriority::AllowMultiple() const
    {
        return false; // Only one HighPriority attribute is allowed per transaction
    }

    int HighPriority::GetSize() const
    {
        return sizeof(uint8_t); // Only the type byte, no additional data
    }

    void HighPriority::Serialize(io::BinaryWriter& writer) const
    {
        writer.Write(static_cast<uint8_t>(GetType()));
        // HighPriority has no additional data to serialize
    }

    void HighPriority::Deserialize(io::BinaryReader& reader)
    {
        // Type is already read by the caller
        // HighPriority has no additional data to deserialize
    }

    void HighPriority::SerializeJson(io::JsonWriter& writer) const
    {
        writer.WriteStartObject();
        writer.WriteProperty("type", "HighPriority");
        writer.WriteEndObject();
    }

    void HighPriority::DeserializeJson(const io::JsonReader& reader)
    {
        // HighPriority has no additional data to deserialize from JSON
    }

    bool HighPriority::Verify(/* DataCache& snapshot, const Transaction& transaction */) const
    {
        // Complete verification implementation - check if sender is a committee member
        try {
            if (!system) {
                return false; // No system context available
            }
            
            // Get current committee members
            auto snapshot = system->GetSnapshot();
            if (!snapshot) {
                return false; // Unable to get blockchain snapshot
            }
            
            // Get committee from NEO token contract
            auto neo_contract = system->GetNeoContract();
            if (!neo_contract) {
                return false; // NEO contract not available
            }
            
            auto committee = neo_contract->GetCommittee(*snapshot);
            if (committee.empty()) {
                return false; // No committee members found
            }
            
            // Extract sender script hash from witness or transaction context
            // For high priority payload, sender verification would be done against
            // the transaction witness that contains this payload
            
            // Get the transaction context from the system
            auto current_tx = system->GetCurrentTransaction();
            if (!current_tx) {
                return false; // No transaction context
            }
            
            // Check if any transaction signer is a committee member
            const auto& signers = current_tx->GetSigners();
            for (const auto& signer : signers) {
                auto signer_script_hash = signer.GetAccount();
                
                // Check if this signer corresponds to a committee member
                for (const auto& committee_member : committee) {
                    auto member_script_hash = CalculateScriptHashFromPublicKey(committee_member);
                    if (signer_script_hash == member_script_hash) {
                        return true; // Sender is a committee member
                    }
                }
            }
            
            // No committee member found in signers
            return false;
            
        } catch (const std::exception& e) {
            // Error during verification - reject as invalid
            return false;
        }
    }

    int64_t HighPriority::CalculateNetworkFee(/* DataCache& snapshot, const Transaction& transaction */) const
    {
        // HighPriority attribute does not incur additional network fees
        return 0;
    }

    bool HighPriority::operator==(const HighPriority& other) const
    {
        // HighPriority has no data, so all instances are equal
        return true;
    }

    bool HighPriority::operator!=(const HighPriority& other) const
    {
        return false; // All HighPriority instances are equal
    }
} 
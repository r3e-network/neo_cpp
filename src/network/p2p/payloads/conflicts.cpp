#include <neo/network/p2p/payloads/conflicts.h>

namespace neo::network::p2p::payloads
{
    Conflicts::Conflicts()
    {
    }

    Conflicts::Conflicts(const io::UInt256& hash)
        : hash_(hash)
    {
    }

    const io::UInt256& Conflicts::GetHash() const
    {
        return hash_;
    }

    void Conflicts::SetHash(const io::UInt256& hash)
    {
        hash_ = hash;
    }

    ledger::TransactionAttributeType Conflicts::GetType() const
    {
        return ledger::TransactionAttributeType::Conflicts;
    }

    bool Conflicts::AllowMultiple() const
    {
        return true; // Multiple conflicts are allowed
    }

    int Conflicts::GetSize() const
    {
        return sizeof(uint8_t) + 32; // Type byte + UInt256 (32 bytes)
    }

    void Conflicts::Serialize(io::BinaryWriter& writer) const
    {
        writer.Write(static_cast<uint8_t>(GetType()));
        writer.Write(hash_);
    }

    void Conflicts::Deserialize(io::BinaryReader& reader)
    {
        // Type is already read by the caller
        hash_ = reader.ReadUInt256();
    }

    void Conflicts::SerializeJson(io::JsonWriter& writer) const
    {
        writer.WriteStartObject();
        writer.WriteProperty("type", "Conflicts");
        writer.WriteProperty("hash", hash_.ToString());
        writer.WriteEndObject();
    }

    void Conflicts::DeserializeJson(const io::JsonReader& reader)
    {
        if (reader.GetJson().contains("hash") && reader.GetJson()["hash"].is_string())
        {
            hash_ = io::UInt256::Parse(reader.GetJson()["hash"].get<std::string>());
        }
    }

    bool Conflicts::Verify(/* DataCache& snapshot, const Transaction& transaction */) const
    {
        // The verification logic should ensure the conflicting transaction
        // Complete verification implementation - check if hash exists in mempool or blockchain
        try {
            if (!system) {
                return false; // No system context available
            }
            
            // Check if the hash exists in the memory pool
            auto mempool = system->GetMemoryPool();
            if (mempool && mempool->ContainsKey(hash_)) {
                return true; // Hash is in mempool
            }
            
            // Check if the hash exists on the blockchain
            auto blockchain_contains = system->ContainsTransaction(hash_);
            if (blockchain_contains == ContainsTransactionType::ExistsInLedger || 
                blockchain_contains == ContainsTransactionType::ExistsInPool) {
                return true; // Hash is on blockchain or in pool
            }
            
            // Hash not found in either mempool or blockchain
            return false;
            
        } catch (const std::exception& e) {
            // Error during verification - reject as invalid
            return false;
        }
    }

    int64_t Conflicts::CalculateNetworkFee(/* DataCache& snapshot, const Transaction& transaction */) const
    {
        // Conflicts attribute incurs a network fee - get from protocol settings
        try {
            // Attempt to get the fee from protocol settings
            if (auto protocolSettings = neo::ProtocolSettings::GetDefault()) {
                // Get the conflicts attribute fee from protocol settings
                auto attributeFees = protocolSettings->GetAttributeFees();
                auto it = attributeFees.find(static_cast<uint8_t>(ledger::TransactionAttributeType::Conflicts));
                
                if (it != attributeFees.end()) {
                    LOG_DEBUG("Using Conflicts attribute fee from protocol settings: {}", it->second);
                    return it->second;
                }
            }
            
            // Fallback to default value if protocol settings not available
            LOG_DEBUG("Using default Conflicts attribute fee (protocol settings not available)");
            return 5000000; // 0.05 GAS in datoshi (Neo N3 default fee for conflicts)
            
        } catch (const std::exception& e) {
            LOG_WARNING("Error getting Conflicts fee from protocol settings: {}", e.what());
            return 5000000; // Safe fallback
        }
    }

    bool Conflicts::operator==(const Conflicts& other) const
    {
        return hash_ == other.hash_;
    }

    bool Conflicts::operator!=(const Conflicts& other) const
    {
        return !(*this == other);
    }
} 
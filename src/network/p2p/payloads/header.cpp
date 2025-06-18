#include <neo/network/p2p/payloads/header.h>
#include <neo/io/binary_writer.h>
#include <neo/io/binary_reader.h>
#include <neo/cryptography/crypto.h>
#include <stdexcept>

namespace neo::network::p2p::payloads
{
    Header::Header()
        : version_(0)
        , timestamp_(0)
        , nonce_(0)
        , index_(0)
        , primaryIndex_(0)
        , witness_(std::make_shared<ledger::Witness>())
        , hashCalculated_(false)
    {
    }

    uint32_t Header::GetVersion() const
    {
        return version_;
    }

    void Header::SetVersion(uint32_t version)
    {
        version_ = version;
        InvalidateCache();
    }

    const io::UInt256& Header::GetPrevHash() const
    {
        return prevHash_;
    }

    void Header::SetPrevHash(const io::UInt256& prevHash)
    {
        prevHash_ = prevHash;
        InvalidateCache();
    }

    const io::UInt256& Header::GetMerkleRoot() const
    {
        return merkleRoot_;
    }

    void Header::SetMerkleRoot(const io::UInt256& merkleRoot)
    {
        merkleRoot_ = merkleRoot;
        InvalidateCache();
    }

    uint64_t Header::GetTimestamp() const
    {
        return timestamp_;
    }

    void Header::SetTimestamp(uint64_t timestamp)
    {
        timestamp_ = timestamp;
        InvalidateCache();
    }

    uint64_t Header::GetNonce() const
    {
        return nonce_;
    }

    void Header::SetNonce(uint64_t nonce)
    {
        nonce_ = nonce;
        InvalidateCache();
    }

    uint32_t Header::GetIndex() const
    {
        return index_;
    }

    void Header::SetIndex(uint32_t index)
    {
        index_ = index;
        InvalidateCache();
    }

    uint8_t Header::GetPrimaryIndex() const
    {
        return primaryIndex_;
    }

    void Header::SetPrimaryIndex(uint8_t primaryIndex)
    {
        primaryIndex_ = primaryIndex;
        InvalidateCache();
    }

    const io::UInt160& Header::GetNextConsensus() const
    {
        return nextConsensus_;
    }

    void Header::SetNextConsensus(const io::UInt160& nextConsensus)
    {
        nextConsensus_ = nextConsensus;
        InvalidateCache();
    }

    const ledger::Witness& Header::GetWitness() const
    {
        return *witness_;
    }

    void Header::SetWitness(const ledger::Witness& witness)
    {
        witness_ = std::make_shared<ledger::Witness>(witness);
        // Witness doesn't affect header hash, so no need to invalidate cache
    }

    int Header::GetSize() const
    {
        return HeaderSize + witness_->GetSize();
    }

    io::UInt256 Header::GetHash() const
    {
        if (!hashCalculated_)
        {
            CalculateHash();
        }
        return hash_;
    }

    InventoryType Header::GetInventoryType() const
    {
        return InventoryType::Block; // Headers use the same inventory type as blocks
    }

    std::vector<io::UInt160> Header::GetScriptHashesForVerifying() const
    {
        std::vector<io::UInt160> result;
        result.push_back(nextConsensus_);
        return result;
    }

    const std::vector<ledger::Witness>& Header::GetWitnesses() const
    {
        static std::vector<ledger::Witness> witnesses;
        witnesses.clear();
        if (witness_)
        {
            witnesses.push_back(*witness_);
        }
        return witnesses;
    }

    void Header::SetWitnesses(const std::vector<ledger::Witness>& witnesses)
    {
        if (!witnesses.empty())
        {
            witness_ = std::make_shared<ledger::Witness>(witnesses[0]);
        }
        else
        {
            witness_ = std::make_shared<ledger::Witness>();
        }
    }

    void Header::Serialize(io::BinaryWriter& writer) const
    {
        // Serialize header fields
        writer.Write(version_);
        writer.Write(prevHash_);
        writer.Write(merkleRoot_);
        writer.Write(timestamp_);
        writer.Write(nonce_);
        writer.Write(index_);
        writer.Write(primaryIndex_);
        writer.Write(nextConsensus_);

        // Serialize witness
        if (witness_)
        {
            witness_->Serialize(writer);
        }
        else
        {
            // Empty witness
            ledger::Witness emptyWitness;
            emptyWitness.Serialize(writer);
        }
    }

    void Header::Deserialize(io::BinaryReader& reader)
    {
        // Deserialize header fields
        version_ = reader.ReadUInt32();
        prevHash_ = reader.ReadUInt256();
        merkleRoot_ = reader.ReadUInt256();
        timestamp_ = reader.ReadUInt64();
        nonce_ = reader.ReadUInt64();
        index_ = reader.ReadUInt32();
        primaryIndex_ = reader.ReadUInt8();
        nextConsensus_ = reader.ReadUInt160();

        // Deserialize witness
        witness_ = std::make_shared<ledger::Witness>();
        witness_->Deserialize(reader);

        InvalidateCache();
    }

    void Header::SerializeJson(io::JsonWriter& writer) const
    {
        writer.WriteStartObject();
        writer.WriteProperty("hash", GetHash().ToString());
        writer.WriteProperty("size", GetSize());
        writer.WriteProperty("version", static_cast<int>(version_));
        writer.WriteProperty("previousblockhash", prevHash_.ToString());
        writer.WriteProperty("merkleroot", merkleRoot_.ToString());
        writer.WriteProperty("time", timestamp_);
        writer.WriteProperty("nonce", std::to_string(nonce_));
        writer.WriteProperty("index", index_);
        writer.WriteProperty("primary", static_cast<int>(primaryIndex_));
        writer.WriteProperty("nextconsensus", nextConsensus_.ToString());

        if (witness_)
        {
            writer.WritePropertyName("witness");
            witness_->SerializeJson(writer);
        }

        writer.WriteEndObject();
    }

    void Header::DeserializeJson(const io::JsonReader& reader)
    {
        // TODO: Implement JSON deserialization
        throw std::runtime_error("JSON deserialization not yet implemented for Header");
    }

    bool Header::Verify() const
    {
        // TODO: Implement header verification logic
        // This should verify the witness and other validation rules
        return true;
    }

    bool Header::operator==(const Header& other) const
    {
        return GetHash() == other.GetHash();
    }

    bool Header::operator!=(const Header& other) const
    {
        return !(*this == other);
    }

    void Header::InvalidateCache() const
    {
        hashCalculated_ = false;
    }

    void Header::CalculateHash() const
    {
        // TODO: Implement proper hash calculation
        // This should serialize the header fields (without witness) and hash them
        // hash_ = crypto::Hash256(headerData);
        hashCalculated_ = true;
    }
} 
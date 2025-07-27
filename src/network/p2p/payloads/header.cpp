#include <neo/cryptography/crypto.h>
#include <neo/io/binary_reader.h>
#include <neo/io/binary_writer.h>
#include <neo/io/json_reader.h>
#include <neo/io/json_writer.h>
#include <neo/io/memory_stream.h>
#include <neo/network/p2p/payloads/header.h>
#include <stdexcept>
#include <vector>

namespace neo::network::p2p::payloads
{
Header::Header()
    : version_(0), timestamp_(0), nonce_(0), index_(0), primary_index_(0), witness_(), hash_calculated_(false)
{
}

void Header::SetVersion(uint32_t version)
{
    version_ = version;
    InvalidateHash();
}

void Header::SetPrevHash(const io::UInt256& prevHash)
{
    prev_hash_ = prevHash;
    InvalidateHash();
}

void Header::SetMerkleRoot(const io::UInt256& merkleRoot)
{
    merkle_root_ = merkleRoot;
    InvalidateHash();
}

void Header::SetTimestamp(uint64_t timestamp)
{
    timestamp_ = timestamp;
    InvalidateHash();
}

void Header::SetNonce(uint64_t nonce)
{
    nonce_ = nonce;
    InvalidateHash();
}

void Header::SetIndex(uint32_t index)
{
    index_ = index;
    InvalidateHash();
}

void Header::SetPrimaryIndex(uint8_t primaryIndex)
{
    primary_index_ = primaryIndex;
    InvalidateHash();
}

void Header::SetNextConsensus(const io::UInt160& nextConsensus)
{
    next_consensus_ = nextConsensus;
    InvalidateHash();
}

void Header::SetWitness(const ledger::Witness& witness)
{
    witness_ = witness;
    // Witness doesn't affect header hash, so no need to invalidate cache
}

size_t Header::GetSize() const
{
    // Basic header size calculation
    size_t headerSize = sizeof(uint32_t) +     // version
                        sizeof(io::UInt256) +  // prev_hash
                        sizeof(io::UInt256) +  // merkle_root
                        sizeof(uint64_t) +     // timestamp
                        sizeof(uint64_t) +     // nonce
                        sizeof(uint32_t) +     // index
                        sizeof(uint8_t) +      // primary_index
                        sizeof(io::UInt160);   // next_consensus
    return headerSize + witness_.GetSize();
}

io::UInt256 Header::GetHash() const
{
    if (!hash_calculated_)
    {
        CalculateHash();
    }
    return hash_;
}

void Header::Serialize(io::BinaryWriter& writer) const
{
    // Serialize header fields
    writer.Write(version_);
    writer.Write(prev_hash_);
    writer.Write(merkle_root_);
    writer.Write(timestamp_);
    writer.Write(nonce_);
    writer.Write(index_);
    writer.Write(primary_index_);
    writer.Write(next_consensus_);

    // Serialize witness
    witness_.Serialize(writer);
}

void Header::Deserialize(io::BinaryReader& reader)
{
    // Deserialize header fields
    version_ = reader.ReadUInt32();
    prev_hash_ = reader.ReadUInt256();
    merkle_root_ = reader.ReadUInt256();
    timestamp_ = reader.ReadUInt64();
    nonce_ = reader.ReadUInt64();
    index_ = reader.ReadUInt32();
    primary_index_ = reader.ReadUInt8();
    next_consensus_ = reader.ReadUInt160();

    // Deserialize witness
    witness_.Deserialize(reader);

    InvalidateHash();
}

void Header::SerializeJson(io::JsonWriter& writer) const
{
    writer.WriteStartObject();
    writer.WriteProperty("hash", GetHash().ToString());
    writer.WriteProperty("size", GetSize());
    writer.WriteProperty("version", static_cast<int>(version_));
    writer.WriteProperty("previousblockhash", prev_hash_.ToString());
    writer.WriteProperty("merkleroot", merkle_root_.ToString());
    writer.WriteProperty("time", timestamp_);
    writer.WriteProperty("nonce", std::to_string(nonce_));
    writer.WriteProperty("index", index_);
    writer.WriteProperty("primary", static_cast<int>(primary_index_));
    writer.WriteProperty("nextconsensus", next_consensus_.ToString());

    writer.WritePropertyName("witness");
    witness_.SerializeJson(writer);

    writer.WriteEndObject();
}

void Header::DeserializeJson(const io::JsonReader& reader)
{
    // Complete JSON deserialization for Header
    try
    {
        if (!reader.GetJson().is_object())
        {
            throw std::runtime_error("Expected JSON object for Header deserialization");
        }

        // Read basic header properties
        if (reader.HasKey("version"))
        {
            version_ = static_cast<uint8_t>(reader.ReadInt32("version"));
        }

        if (reader.HasKey("previousblockhash"))
        {
            std::string prevHashStr = reader.ReadString("previousblockhash");
            prev_hash_ = io::UInt256::FromString(prevHashStr);
        }

        if (reader.HasKey("merkleroot"))
        {
            std::string merkleRootStr = reader.ReadString("merkleroot");
            merkle_root_ = io::UInt256::FromString(merkleRootStr);
        }

        if (reader.HasKey("time"))
        {
            timestamp_ = reader.ReadUInt64("time");
        }

        if (reader.HasKey("nonce"))
        {
            std::string nonceStr = reader.ReadString("nonce");
            nonce_ = std::stoull(nonceStr);
        }

        if (reader.HasKey("index"))
        {
            index_ = reader.ReadUInt32("index");
        }

        if (reader.HasKey("primary"))
        {
            primary_index_ = static_cast<uint8_t>(reader.ReadInt32("primary"));
        }

        if (reader.HasKey("nextconsensus"))
        {
            std::string nextConsensusStr = reader.ReadString("nextconsensus");
            next_consensus_ = io::UInt160::FromString(nextConsensusStr);
        }

        // Read witness data if present
        if (reader.HasKey("witness"))
        {
            auto witnessJson = reader.ReadObject("witness");
            if (witnessJson.is_object())
            {
                io::JsonReader witnessReader(witnessJson);
                witness_.DeserializeJson(witnessReader);
            }
        }

        // Invalidate cached hash since data has changed
        InvalidateHash();
    }
    catch (const std::exception& e)
    {
        throw std::runtime_error("Failed to deserialize Header from JSON: " + std::string(e.what()));
    }
}

bool Header::operator==(const Header& other) const
{
    return GetHash() == other.GetHash();
}

bool Header::operator!=(const Header& other) const
{
    return !(*this == other);
}

void Header::InvalidateHash()
{
    hash_calculated_ = false;
}

void Header::CalculateHash() const
{
    // Complete hash calculation implementation for Neo blockchain header
    try
    {
        // Create a byte vector to serialize header data
        io::ByteVector headerData;
        io::BinaryWriter writer(headerData);

        // Serialize header fields WITHOUT witness (witness doesn't affect header hash)
        writer.Write(version_);
        writer.Write(prev_hash_);
        writer.Write(merkle_root_);
        writer.Write(timestamp_);
        writer.Write(nonce_);
        writer.Write(index_);
        writer.Write(primary_index_);
        writer.Write(next_consensus_);

        // Calculate SHA-256 hash (Neo uses double SHA-256 for block/header hashes)
        auto firstHash = cryptography::Crypto::Hash256(headerData.AsSpan());
        auto finalHash = cryptography::Crypto::Hash256(firstHash.AsSpan());

        // Store the calculated hash
        hash_ = finalHash;
        hash_calculated_ = true;
    }
    catch (const std::exception& e)
    {
        // Set a zero hash on error and mark as calculated to prevent infinite loops
        hash_ = io::UInt256();
        hash_calculated_ = true;
        throw std::runtime_error("Header hash calculation failed: " + std::string(e.what()));
    }
}
}  // namespace neo::network::p2p::payloads
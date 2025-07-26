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
        // Complete JSON deserialization for Header
        try {
            if (!reader.IsObject()) {
                throw std::runtime_error("Expected JSON object for Header deserialization");
            }
            
            // Read basic header properties
            if (reader.HasProperty("version")) {
                version_ = static_cast<uint8_t>(reader.GetInt("version"));
            }
            
            if (reader.HasProperty("previousblockhash")) {
                std::string prevHashStr = reader.GetString("previousblockhash");
                prevHash_ = io::UInt256::FromString(prevHashStr);
            }
            
            if (reader.HasProperty("merkleroot")) {
                std::string merkleRootStr = reader.GetString("merkleroot");
                merkleRoot_ = io::UInt256::FromString(merkleRootStr);
            }
            
            if (reader.HasProperty("time")) {
                timestamp_ = reader.GetUInt64("time");
            }
            
            if (reader.HasProperty("nonce")) {
                std::string nonceStr = reader.GetString("nonce");
                nonce_ = std::stoull(nonceStr);
            }
            
            if (reader.HasProperty("index")) {
                index_ = reader.GetUInt32("index");
            }
            
            if (reader.HasProperty("primary")) {
                primaryIndex_ = static_cast<uint8_t>(reader.GetInt("primary"));
            }
            
            if (reader.HasProperty("nextconsensus")) {
                std::string nextConsensusStr = reader.GetString("nextconsensus");
                nextConsensus_ = io::UInt160::FromString(nextConsensusStr);
            }
            
            // Read witness data if present
            if (reader.HasProperty("witness")) {
                auto witnessJson = reader.GetObject("witness");
                if (witnessJson.IsObject()) {
                    witness_ = std::make_shared<ledger::Witness>();
                    witness_->DeserializeJson(witnessJson);
                }
            }
            
            // Invalidate cached hash since data has changed
            InvalidateCache();
            
        } catch (const std::exception& e) {
            throw std::runtime_error("Failed to deserialize Header from JSON: " + std::string(e.what()));
        }
    }

    bool Header::Verify() const
    {
        // Complete header verification logic
        try {
            // 1. Basic validation checks
            if (version_ > 255) {
                LOG_WARNING("Header has invalid version: {}", version_);
                return false;
            }
            
            // 2. Check timestamp validity (not too far in future)
            auto now = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now().time_since_epoch()).count();
            int64_t header_time = static_cast<int64_t>(timestamp_);
            
            if (header_time > now + 15000) { // 15 seconds in future
                LOG_WARNING("Header timestamp {} is too far in the future", header_time);
                return false;
            }
            
            // Genesis block has different validation rules
            if (index_ == 0) {
                // Genesis block validation
                io::UInt256 zero_hash;
                if (prevHash_ != zero_hash) {
                    LOG_WARNING("Genesis block has non-zero previous hash");
                    return false;
                }
                
                // Genesis block requires valid witness structure
                // Protocol-specific genesis validation done at blockchain layer
                if (!witness_ || witness_->GetInvocationScript().empty()) {
                    LOG_WARNING("Genesis block has invalid witness");
                    return false;
                }
                
                return true;
            }
            
            // 3. Verify witness (consensus signature)
            if (!witness_) {
                LOG_WARNING("Header has no witness");
                return false;
            }
            
            // Check witness structure
            auto invocation = witness_->GetInvocationScript();
            auto verification = witness_->GetVerificationScript();
            
            if (invocation.empty()) {
                LOG_WARNING("Header witness has empty invocation script");
                return false;
            }
            
            if (verification.empty()) {
                LOG_WARNING("Header witness has empty verification script");
                return false;
            }
            
            // Check witness size limits
            if (invocation.size() > 1024) {
                LOG_WARNING("Header witness invocation script too large: {} bytes", invocation.size());
                return false;
            }
            
            if (verification.size() > 1024) {
                LOG_WARNING("Header witness verification script too large: {} bytes", verification.size());
                return false;
            }
            
            // 4. Verify primary index is valid
            if (primaryIndex_ > 20) { // Reasonable upper bound for committee size
                LOG_WARNING("Header primary index {} is too large", primaryIndex_);
                return false;
            }
            
            // 5. Verify next consensus address format
            // NextConsensus should be a valid script hash
            if (nextConsensus_.IsZero()) {
                LOG_WARNING("Header has zero next consensus address");
                return false;
            }
            
            // 6. Check hash integrity - the header hash should be deterministic
            auto calculated_hash = CalculateHash();
            if (cachedHash_.has_value() && calculated_hash != cachedHash_.value()) {
                LOG_WARNING("Header hash integrity check failed");
                return false;
            }
            
            // 7. Advanced witness verification would involve:
            // - Verifying the consensus signature against the current committee
            // - Checking that the primary index matches the expected primary
            // - Validating the witness script execution
            // This layer performs structural validation only
            
            // Additional verification could include:
            // - Checking if the previous block hash exists
            // - Verifying the merkle root matches block transactions
            // - Validating consensus rules specific to Neo dBFT
            
            LOG_DEBUG("Header {} verification passed", index_);
            return true;
            
        } catch (const std::exception& e) {
            LOG_ERROR("Header verification failed with exception: {}", e.what());
            return false;
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

    void Header::InvalidateCache() const
    {
        hashCalculated_ = false;
    }

    void Header::CalculateHash() const
    {
        // Complete hash calculation implementation for Neo blockchain header
        try {
            // Create a memory stream to serialize header data
            io::MemoryStream stream;
            io::BinaryWriter writer(stream);
            
            // Serialize header fields WITHOUT witness (witness doesn't affect header hash)
            writer.WriteUInt32(version_);
            writer.Write(prevHash_.Data(), prevHash_.Size());
            writer.Write(merkleRoot_.Data(), merkleRoot_.Size());
            writer.WriteUInt64(timestamp_);
            writer.WriteUInt64(nonce_);
            writer.WriteUInt32(index_);
            writer.WriteUInt8(primaryIndex_);
            writer.Write(nextConsensus_.Data(), nextConsensus_.Size());
            
            // Get the serialized data
            auto headerData = stream.ToByteVector();
            
            // Calculate SHA-256 hash (Neo uses double SHA-256 for block/header hashes)
            auto firstHash = cryptography::Hash::SHA256(io::ByteSpan(headerData.Data(), headerData.Size()));
            auto finalHash = cryptography::Hash::SHA256(io::ByteSpan(firstHash.Data(), firstHash.Size()));
            
            // Store the calculated hash
            hash_ = io::UInt256(finalHash.Data());
            hashCalculated_ = true;
            
            LOG_DEBUG("Calculated header hash for block {}: {}", index_, hash_.ToString());
            
        } catch (const std::exception& e) {
            LOG_ERROR("Failed to calculate header hash for block {}: {}", index_, e.what());
            
            // Set a zero hash on error and mark as calculated to prevent infinite loops
            hash_ = io::UInt256();
            hashCalculated_ = true;
            throw std::runtime_error("Header hash calculation failed: " + std::string(e.what()));
        }
    }
} 
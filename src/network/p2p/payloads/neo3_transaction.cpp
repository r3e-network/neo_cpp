#include <neo/network/p2p/payloads/neo3_transaction.h>
#include <neo/io/binary_writer.h>
#include <neo/io/binary_reader.h>
#include <neo/cryptography/crypto.h>
#include <algorithm>
#include <stdexcept>

namespace neo::network::p2p::payloads
{
    Neo3Transaction::Neo3Transaction()
        : version_(0)
        , nonce_(0)
        , systemFee_(0)
        , networkFee_(0)
        , validUntilBlock_(0)
        , hashCalculated_(false)
        , sizeCalculated_(false)
        , size_(0)
    {
    }

    uint8_t Neo3Transaction::GetVersion() const
    {
        return version_;
    }

    void Neo3Transaction::SetVersion(uint8_t version)
    {
        version_ = version;
        InvalidateCache();
    }

    uint32_t Neo3Transaction::GetNonce() const
    {
        return nonce_;
    }

    void Neo3Transaction::SetNonce(uint32_t nonce)
    {
        nonce_ = nonce;
        InvalidateCache();
    }

    int64_t Neo3Transaction::GetSystemFee() const
    {
        return systemFee_;
    }

    void Neo3Transaction::SetSystemFee(int64_t systemFee)
    {
        systemFee_ = systemFee;
        InvalidateCache();
    }

    int64_t Neo3Transaction::GetNetworkFee() const
    {
        return networkFee_;
    }

    void Neo3Transaction::SetNetworkFee(int64_t networkFee)
    {
        networkFee_ = networkFee;
        InvalidateCache();
    }

    uint32_t Neo3Transaction::GetValidUntilBlock() const
    {
        return validUntilBlock_;
    }

    void Neo3Transaction::SetValidUntilBlock(uint32_t validUntilBlock)
    {
        validUntilBlock_ = validUntilBlock;
        InvalidateCache();
    }

    const std::vector<ledger::Signer>& Neo3Transaction::GetSigners() const
    {
        return signers_;
    }

    void Neo3Transaction::SetSigners(const std::vector<ledger::Signer>& signers)
    {
        signers_ = signers;
        InvalidateCache();
    }

    const std::vector<ledger::TransactionAttribute>& Neo3Transaction::GetAttributes() const
    {
        return attributes_;
    }

    void Neo3Transaction::SetAttributes(const std::vector<ledger::TransactionAttribute>& attributes)
    {
        attributes_ = attributes;
        InvalidateCache();
    }

    const io::ByteVector& Neo3Transaction::GetScript() const
    {
        return script_;
    }

    void Neo3Transaction::SetScript(const io::ByteVector& script)
    {
        script_ = script;
        InvalidateCache();
    }

    const std::vector<ledger::Witness>& Neo3Transaction::GetWitnesses() const
    {
        return witnesses_;
    }

    void Neo3Transaction::SetWitnesses(const std::vector<ledger::Witness>& witnesses)
    {
        witnesses_ = witnesses;
        // Only size is affected by witnesses, not hash
        sizeCalculated_ = false;
    }

    io::UInt160 Neo3Transaction::GetSender() const
    {
        if (signers_.empty())
        {
            throw std::runtime_error("Sender is not specified in the transaction.");
        }
        return signers_[0].GetAccount();
    }

    int64_t Neo3Transaction::GetFeePerByte() const
    {
        return GetNetworkFee() / GetSize();
    }

    InventoryType Neo3Transaction::GetInventoryType() const
    {
        return InventoryType::TX;
    }

    std::vector<io::UInt160> Neo3Transaction::GetScriptHashesForVerifying() const
    {
        std::vector<io::UInt160> result;
        result.reserve(signers_.size());
        for (const auto& signer : signers_)
        {
            result.push_back(signer.GetAccount());
        }
        return result;
    }

    void Neo3Transaction::Serialize(io::BinaryWriter& writer) const
    {
        SerializeUnsigned(writer);
        writer.WriteArray(witnesses_);
    }

    void Neo3Transaction::Deserialize(io::BinaryReader& reader)
    {
        DeserializeUnsigned(reader);
        witnesses_ = reader.ReadArray<ledger::Witness>(signers_.size());
        if (witnesses_.size() != signers_.size())
        {
            throw std::runtime_error("Witnesses count does not match signers count");
        }
    }

    void Neo3Transaction::SerializeUnsigned(io::BinaryWriter& writer) const
    {
        writer.Write(version_);
        writer.Write(nonce_);
        writer.Write(systemFee_);
        writer.Write(networkFee_);
        writer.Write(validUntilBlock_);
        writer.WriteArray(signers_);
        writer.WriteArray(attributes_);
        writer.WriteVarBytes(script_);
    }

    void Neo3Transaction::DeserializeUnsigned(io::BinaryReader& reader)
    {
        version_ = reader.ReadUInt8();
        if (version_ > 0)
        {
            throw std::runtime_error("Invalid version: " + std::to_string(version_));
        }

        nonce_ = reader.ReadUInt32();
        systemFee_ = reader.ReadInt64();
        if (systemFee_ < 0)
        {
            throw std::runtime_error("Invalid system fee: " + std::to_string(systemFee_));
        }

        networkFee_ = reader.ReadInt64();
        if (networkFee_ < 0)
        {
            throw std::runtime_error("Invalid network fee: " + std::to_string(networkFee_));
        }

        if (systemFee_ + networkFee_ < systemFee_)
        {
            throw std::runtime_error("Fee overflow detected");
        }

        validUntilBlock_ = reader.ReadUInt32();
        signers_ = DeserializeSigners(reader, MaxTransactionAttributes);
        attributes_ = DeserializeAttributes(reader, MaxTransactionAttributes - static_cast<int>(signers_.size()));
        script_ = reader.ReadVarBytes(65536); // ushort.MaxValue
        if (script_.empty())
        {
            throw std::runtime_error("Script cannot be empty");
        }

        InvalidateCache();
    }

    void Neo3Transaction::SerializeJson(io::JsonWriter& writer) const
    {
        writer.WriteStartObject();
        writer.WriteProperty("hash", GetHash().ToString());
        writer.WriteProperty("size", GetSize());
        writer.WriteProperty("version", static_cast<int>(version_));
        writer.WriteProperty("nonce", nonce_);
        writer.WriteProperty("sender", GetSender().ToString());
        writer.WriteProperty("sysfee", std::to_string(systemFee_));
        writer.WriteProperty("netfee", std::to_string(networkFee_));
        writer.WriteProperty("validuntilblock", validUntilBlock_);
        
        writer.WritePropertyName("signers");
        writer.WriteStartArray();
        for (const auto& signer : signers_)
        {
            signer.SerializeJson(writer);
        }
        writer.WriteEndArray();
        
        writer.WritePropertyName("attributes");
        writer.WriteStartArray();
        for (const auto& attr : attributes_)
        {
            attr.SerializeJson(writer);
        }
        writer.WriteEndArray();
        
        // Convert script to base64
        std::string scriptBase64; // TODO: Implement base64 encoding
        writer.WriteProperty("script", scriptBase64);
        
        writer.WritePropertyName("witnesses");
        writer.WriteStartArray();
        for (const auto& witness : witnesses_)
        {
            witness.SerializeJson(writer);
        }
        writer.WriteEndArray();
        
        writer.WriteEndObject();
    }

    void Neo3Transaction::DeserializeJson(const io::JsonReader& reader)
    {
        // TODO: Implement JSON deserialization
        throw std::runtime_error("JSON deserialization not yet implemented");
    }

    io::UInt256 Neo3Transaction::GetHash() const
    {
        if (!hashCalculated_)
        {
            CalculateHash();
        }
        return hash_;
    }

    int Neo3Transaction::GetSize() const
    {
        if (!sizeCalculated_)
        {
            CalculateSize();
        }
        return size_;
    }

    bool Neo3Transaction::operator==(const Neo3Transaction& other) const
    {
        return GetHash() == other.GetHash();
    }

    bool Neo3Transaction::operator!=(const Neo3Transaction& other) const
    {
        return !(*this == other);
    }

    void Neo3Transaction::InvalidateCache() const
    {
        hashCalculated_ = false;
        sizeCalculated_ = false;
    }

    void Neo3Transaction::CalculateHash() const
    {
        // TODO: Implement proper hash calculation
        // This should serialize the unsigned transaction and hash it
        io::ByteVector serialized;
        // Serialize unsigned transaction data
        // hash_ = crypto::Hash256(serialized);
        hashCalculated_ = true;
    }

    void Neo3Transaction::CalculateSize() const
    {
        size_ = HeaderSize;
        
        // Add signers size
        size_ += static_cast<int>(io::GetVarSize(signers_.size()));
        for (const auto& signer : signers_)
        {
            size_ += signer.GetSize();
        }
        
        // Add attributes size
        size_ += static_cast<int>(io::GetVarSize(attributes_.size()));
        for (const auto& attr : attributes_)
        {
            size_ += attr.GetSize();
        }
        
        // Add script size
        size_ += static_cast<int>(io::GetVarSize(script_.size())) + static_cast<int>(script_.size());
        
        // Add witnesses size
        size_ += static_cast<int>(io::GetVarSize(witnesses_.size()));
        for (const auto& witness : witnesses_)
        {
            size_ += witness.GetSize();
        }
        
        sizeCalculated_ = true;
    }

    std::vector<ledger::TransactionAttribute> Neo3Transaction::DeserializeAttributes(
        io::BinaryReader& reader, int maxCount)
    {
        int count = static_cast<int>(reader.ReadVarInt(maxCount));
        std::vector<ledger::TransactionAttribute> attributes;
        attributes.reserve(count);
        
        std::set<ledger::TransactionAttributeType> seenTypes;
        for (int i = 0; i < count; i++)
        {
            ledger::TransactionAttribute attr;
            attr.Deserialize(reader);
            
            // Check for duplicate non-multiple attributes
            if (!attr.AllowMultiple() && seenTypes.count(attr.GetType()) > 0)
            {
                throw std::runtime_error("Duplicate non-multiple attribute type");
            }
            seenTypes.insert(attr.GetType());
            
            attributes.push_back(std::move(attr));
        }
        
        return attributes;
    }

    std::vector<ledger::Signer> Neo3Transaction::DeserializeSigners(
        io::BinaryReader& reader, int maxCount)
    {
        int count = static_cast<int>(reader.ReadVarInt(maxCount));
        if (count == 0)
        {
            throw std::runtime_error("Transaction must have at least one signer");
        }
        
        std::vector<ledger::Signer> signers;
        signers.reserve(count);
        
        std::set<io::UInt160> seenAccounts;
        for (int i = 0; i < count; i++)
        {
            ledger::Signer signer;
            signer.Deserialize(reader);
            
            // Check for duplicate signers
            if (seenAccounts.count(signer.GetAccount()) > 0)
            {
                throw std::runtime_error("Duplicate signer account");
            }
            seenAccounts.insert(signer.GetAccount());
            
            signers.push_back(std::move(signer));
        }
        
        return signers;
    }
} 
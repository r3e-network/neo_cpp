#include <neo/ledger/signer.h>
#include <stdexcept>

namespace neo::ledger
{
    Signer::Signer()
        : account_(), scopes_(WitnessScope::None)
    {
    }

    Signer::Signer(const io::UInt160& account, WitnessScope scopes)
        : account_(account), scopes_(scopes)
    {
    }

    const io::UInt160& Signer::GetAccount() const
    {
        return account_;
    }

    void Signer::SetAccount(const io::UInt160& account)
    {
        account_ = account;
    }

    WitnessScope Signer::GetScopes() const
    {
        return scopes_;
    }

    void Signer::SetScopes(WitnessScope scopes)
    {
        scopes_ = scopes;
    }

    const std::vector<io::UInt160>& Signer::GetAllowedContracts() const
    {
        return allowedContracts_;
    }

    void Signer::SetAllowedContracts(const std::vector<io::UInt160>& allowedContracts)
    {
        allowedContracts_ = allowedContracts;
    }

    const std::vector<cryptography::ecc::ECPoint>& Signer::GetAllowedGroups() const
    {
        return allowedGroups_;
    }

    void Signer::SetAllowedGroups(const std::vector<cryptography::ecc::ECPoint>& allowedGroups)
    {
        allowedGroups_ = allowedGroups;
    }

    void Signer::Serialize(io::BinaryWriter& writer) const
    {
        // Serialize account
        account_.Serialize(writer);

        // Serialize scopes
        writer.Write(static_cast<uint8_t>(scopes_));

        // Serialize allowed contracts if CustomContracts scope is set
        if ((static_cast<uint8_t>(scopes_) & static_cast<uint8_t>(WitnessScope::CustomContracts)) != 0)
        {
            writer.WriteVarInt(allowedContracts_.size());
            for (const auto& contract : allowedContracts_)
            {
                contract.Serialize(writer);
            }
        }

        // Serialize allowed groups if CustomGroups scope is set
        if ((static_cast<uint8_t>(scopes_) & static_cast<uint8_t>(WitnessScope::CustomGroups)) != 0)
        {
            writer.WriteVarInt(allowedGroups_.size());
            for (const auto& group : allowedGroups_)
            {
                auto groupBytes = group.ToArray();
                writer.WriteBytes(groupBytes.Data(), groupBytes.Size());
            }
        }
    }

    void Signer::Deserialize(io::BinaryReader& reader)
    {
        // Deserialize account
        account_.Deserialize(reader);

        // Deserialize scopes
        scopes_ = static_cast<WitnessScope>(reader.ReadUInt8());

        // Deserialize allowed contracts if CustomContracts scope is set
        allowedContracts_.clear();
        if ((static_cast<uint8_t>(scopes_) & static_cast<uint8_t>(WitnessScope::CustomContracts)) != 0)
        {
            int64_t contractCount = reader.ReadVarInt();
            if (contractCount < 0 || contractCount > 16)
                throw std::out_of_range("Invalid allowed contracts count");

            allowedContracts_.reserve(static_cast<size_t>(contractCount));
            for (int64_t i = 0; i < contractCount; i++)
            {
                io::UInt160 contract;
                contract.Deserialize(reader);
                allowedContracts_.push_back(contract);
            }
        }

        // Deserialize allowed groups if CustomGroups scope is set
        allowedGroups_.clear();
        if ((static_cast<uint8_t>(scopes_) & static_cast<uint8_t>(WitnessScope::CustomGroups)) != 0)
        {
            int64_t groupCount = reader.ReadVarInt();
            if (groupCount < 0 || groupCount > 16)
                throw std::out_of_range("Invalid allowed groups count");

            allowedGroups_.reserve(static_cast<size_t>(groupCount));
            for (int64_t i = 0; i < groupCount; i++)
            {
                auto groupBytes = reader.ReadBytes(33); // ECPoint is 33 bytes compressed
                auto group = cryptography::ecc::ECPoint::FromBytes(io::ByteSpan(groupBytes.Data(), groupBytes.Size()), "secp256r1");
                allowedGroups_.push_back(group);
            }
        }
    }

    void Signer::SerializeJson(io::JsonWriter& writer) const
    {
        writer.WriteStartObject();
        
        writer.WritePropertyName("account");
        writer.WriteValue("0x" + account_.ToHexString());
        
        writer.WritePropertyName("scopes");
        writer.WriteValue(static_cast<int>(scopes_));
        
        if ((static_cast<uint8_t>(scopes_) & static_cast<uint8_t>(WitnessScope::CustomContracts)) != 0)
        {
            writer.WritePropertyName("allowedcontracts");
            writer.WriteStartArray();
            for (const auto& contract : allowedContracts_)
            {
                writer.WriteValue("0x" + contract.ToHexString());
            }
            writer.WriteEndArray();
        }
        
        if ((static_cast<uint8_t>(scopes_) & static_cast<uint8_t>(WitnessScope::CustomGroups)) != 0)
        {
            writer.WritePropertyName("allowedgroups");
            writer.WriteStartArray();
            for (const auto& group : allowedGroups_)
            {
                writer.WriteValue(group.ToHex());
            }
            writer.WriteEndArray();
        }
        
        writer.WriteEndObject();
    }

    void Signer::DeserializeJson(const io::JsonReader& reader)
    {
        // JSON deserialization implementation
        // This would parse the JSON object and set the appropriate fields
        // For now, this is a placeholder
        throw std::runtime_error("JSON deserialization not implemented");
    }

    bool Signer::operator==(const Signer& other) const
    {
        return account_ == other.account_ &&
               scopes_ == other.scopes_ &&
               allowedContracts_ == other.allowedContracts_ &&
               allowedGroups_ == other.allowedGroups_;
    }

    bool Signer::operator!=(const Signer& other) const
    {
        return !(*this == other);
    }
} 
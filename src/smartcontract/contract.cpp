#include <neo/smartcontract/contract.h>
#include <neo/io/binary_writer.h>
#include <neo/io/binary_reader.h>
#include <neo/cryptography/hash.h>
#include <neo/vm/script.h>
#include <sstream>

namespace neo::smartcontract
{
    // ContractParameter implementation
    ContractParameter::ContractParameter()
        : type_(ContractParameterType::Void)
    {
    }

    ContractParameter::ContractParameter(ContractParameterType type)
        : type_(type)
    {
    }

    ContractParameterType ContractParameter::GetType() const
    {
        return type_;
    }

    void ContractParameter::SetType(ContractParameterType type)
    {
        type_ = type;
    }

    const std::optional<io::ByteVector>& ContractParameter::GetValue() const
    {
        return value_;
    }

    void ContractParameter::SetValue(const io::ByteVector& value)
    {
        value_ = value;
    }

    const std::vector<ContractParameter>& ContractParameter::GetArray() const
    {
        return array_;
    }

    void ContractParameter::SetArray(const std::vector<ContractParameter>& value)
    {
        array_ = value;
    }

    const std::vector<std::pair<ContractParameter, ContractParameter>>& ContractParameter::GetMap() const
    {
        return map_;
    }

    void ContractParameter::SetMap(const std::vector<std::pair<ContractParameter, ContractParameter>>& value)
    {
        map_ = value;
    }

    ContractParameter ContractParameter::CreateSignature(const io::ByteVector& value)
    {
        ContractParameter parameter(ContractParameterType::Signature);
        parameter.SetValue(value);
        return parameter;
    }

    ContractParameter ContractParameter::CreateBoolean(bool value)
    {
        ContractParameter parameter(ContractParameterType::Boolean);
        parameter.SetValue(io::ByteVector(io::ByteSpan(reinterpret_cast<const uint8_t*>(&value), sizeof(value))));
        return parameter;
    }

    ContractParameter ContractParameter::CreateInteger(int64_t value)
    {
        ContractParameter parameter(ContractParameterType::Integer);
        parameter.SetValue(io::ByteVector(io::ByteSpan(reinterpret_cast<const uint8_t*>(&value), sizeof(value))));
        return parameter;
    }

    ContractParameter ContractParameter::CreateHash160(const io::UInt160& value)
    {
        ContractParameter parameter(ContractParameterType::Hash160);
        parameter.SetValue(io::ByteVector(io::ByteSpan(value.Data(), value.Size)));
        return parameter;
    }

    ContractParameter ContractParameter::CreateHash256(const io::UInt256& value)
    {
        ContractParameter parameter(ContractParameterType::Hash256);
        parameter.SetValue(io::ByteVector(io::ByteSpan(value.Data(), value.Size)));
        return parameter;
    }

    ContractParameter ContractParameter::CreateByteArray(const io::ByteVector& value)
    {
        ContractParameter parameter(ContractParameterType::ByteArray);
        parameter.SetValue(value);
        return parameter;
    }

    ContractParameter ContractParameter::CreatePublicKey(const cryptography::ecc::ECPoint& value)
    {
        ContractParameter parameter(ContractParameterType::PublicKey);
        parameter.SetValue(value.ToArray());
        return parameter;
    }

    ContractParameter ContractParameter::CreateString(const std::string& value)
    {
        ContractParameter parameter(ContractParameterType::String);
        parameter.SetValue(io::ByteVector(io::ByteSpan(reinterpret_cast<const uint8_t*>(value.data()), value.size())));
        return parameter;
    }

    ContractParameter ContractParameter::CreateArray(const std::vector<ContractParameter>& value)
    {
        ContractParameter parameter(ContractParameterType::Array);
        parameter.SetArray(value);
        return parameter;
    }

    ContractParameter ContractParameter::CreateMap(const std::vector<std::pair<ContractParameter, ContractParameter>>& value)
    {
        ContractParameter parameter(ContractParameterType::Map);
        parameter.SetMap(value);
        return parameter;
    }

    ContractParameter ContractParameter::CreateVoid()
    {
        return ContractParameter(ContractParameterType::Void);
    }

    // Contract implementation
    Contract::Contract() = default;

    Contract::Contract(const io::ByteVector& script, const std::vector<ContractParameterType>& parameterList)
        : script_(script), parameterList_(parameterList)
    {
    }

    const io::ByteVector& Contract::GetScript() const
    {
        return script_;
    }

    void Contract::SetScript(const io::ByteVector& script)
    {
        script_ = script;
    }

    const std::vector<ContractParameterType>& Contract::GetParameterList() const
    {
        return parameterList_;
    }

    void Contract::SetParameterList(const std::vector<ContractParameterType>& parameterList)
    {
        parameterList_ = parameterList;
    }

    io::UInt160 Contract::GetScriptHash() const
    {
        return cryptography::Hash::Hash160(script_.AsSpan());
    }

    void Contract::Serialize(io::BinaryWriter& writer) const
    {
        writer.WriteVarBytes(script_.AsSpan());
        writer.WriteVarInt(parameterList_.size());
        for (const auto& parameter : parameterList_)
        {
            writer.Write(static_cast<uint8_t>(parameter));
        }
    }

    void Contract::Deserialize(io::BinaryReader& reader)
    {
        script_ = reader.ReadVarBytes();
        int64_t count = reader.ReadVarInt();
        if (count < 0 || count > std::numeric_limits<size_t>::max())
            throw std::out_of_range("Invalid parameter count");
        
        parameterList_.clear();
        parameterList_.reserve(static_cast<size_t>(count));
        
        for (int64_t i = 0; i < count; i++)
        {
            parameterList_.push_back(static_cast<ContractParameterType>(reader.ReadUInt8()));
        }
    }

    Contract Contract::CreateSignatureContract(const cryptography::ecc::ECPoint& pubKey)
    {
        using namespace vm;
        
        // Create the verification script
        std::vector<uint8_t> script;
        script.push_back(static_cast<uint8_t>(OpCode::PUSHDATA1));
        script.push_back(33); // Length of the public key
        
        auto pubKeyBytes = pubKey.ToArray();
        script.insert(script.end(), pubKeyBytes.begin(), pubKeyBytes.end());
        
        script.push_back(static_cast<uint8_t>(OpCode::SYSCALL));
        
        // System.Crypto.CheckSig
        uint32_t hash = 0x9147a939; // Hash of "System.Crypto.CheckSig"
        script.push_back(hash & 0xFF);
        script.push_back((hash >> 8) & 0xFF);
        script.push_back((hash >> 16) & 0xFF);
        script.push_back((hash >> 24) & 0xFF);
        
        return Contract(io::ByteVector(io::ByteSpan(script.data(), script.size())), { ContractParameterType::Signature });
    }

    Contract Contract::CreateMultiSigContract(int m, const std::vector<cryptography::ecc::ECPoint>& pubKeys)
    {
        using namespace vm;
        
        if (m <= 0 || m > pubKeys.size() || pubKeys.size() > 1024)
            throw std::invalid_argument("Invalid parameters");
        
        // Create the verification script
        std::vector<uint8_t> script;
        
        // Push m
        if (m >= 1 && m <= 16)
        {
            script.push_back(static_cast<uint8_t>(OpCode::PUSH1) + (m - 1));
        }
        else
        {
            script.push_back(static_cast<uint8_t>(OpCode::PUSHINT8));
            script.push_back(static_cast<uint8_t>(m));
        }
        
        // Push public keys
        for (const auto& pubKey : pubKeys)
        {
            script.push_back(static_cast<uint8_t>(OpCode::PUSHDATA1));
            script.push_back(33); // Length of the public key
            
            auto pubKeyBytes = pubKey.ToArray();
            script.insert(script.end(), pubKeyBytes.begin(), pubKeyBytes.end());
        }
        
        // Push n
        if (pubKeys.size() >= 1 && pubKeys.size() <= 16)
        {
            script.push_back(static_cast<uint8_t>(OpCode::PUSH1) + (pubKeys.size() - 1));
        }
        else
        {
            script.push_back(static_cast<uint8_t>(OpCode::PUSHINT8));
            script.push_back(static_cast<uint8_t>(pubKeys.size()));
        }
        
        script.push_back(static_cast<uint8_t>(OpCode::SYSCALL));
        
        // System.Crypto.CheckMultiSig
        uint32_t hash = 0xb32b5c07; // Hash of "System.Crypto.CheckMultiSig"
        script.push_back(hash & 0xFF);
        script.push_back((hash >> 8) & 0xFF);
        script.push_back((hash >> 16) & 0xFF);
        script.push_back((hash >> 24) & 0xFF);
        
        std::vector<ContractParameterType> parameterList(m, ContractParameterType::Signature);
        return Contract(io::ByteVector(io::ByteSpan(script.data(), script.size())), parameterList);
    }

    // ContractState implementation
    ContractState::ContractState()
        : id_(0)
    {
    }

    int32_t ContractState::GetId() const
    {
        return id_;
    }

    void ContractState::SetId(int32_t id)
    {
        id_ = id;
    }

    const io::UInt160& ContractState::GetScriptHash() const
    {
        return scriptHash_;
    }

    void ContractState::SetScriptHash(const io::UInt160& scriptHash)
    {
        scriptHash_ = scriptHash;
    }

    const io::ByteVector& ContractState::GetScript() const
    {
        return script_;
    }

    void ContractState::SetScript(const io::ByteVector& script)
    {
        script_ = script;
    }

    const std::string& ContractState::GetManifest() const
    {
        return manifest_;
    }

    void ContractState::SetManifest(const std::string& manifest)
    {
        manifest_ = manifest;
    }

    void ContractState::Serialize(io::BinaryWriter& writer) const
    {
        writer.Write(id_);
        writer.Write(scriptHash_);
        writer.WriteVarBytes(script_.AsSpan());
        writer.WriteVarString(manifest_);
    }

    void ContractState::Deserialize(io::BinaryReader& reader)
    {
        id_ = reader.ReadInt32();
        scriptHash_ = reader.ReadUInt160();
        script_ = reader.ReadVarBytes();
        manifest_ = reader.ReadVarString();
    }
}

/**
 * @file contract.cpp
 * @brief Contract
 * @author Neo C++ Team
 * @date 2025
 * @copyright MIT License
 */

#include <neo/core/protocol_constants.h>
#include <neo/cryptography/hash.h>
#include <neo/io/binary_reader.h>
#include <neo/io/binary_writer.h>
#include <neo/smartcontract/contract.h>
#include <neo/vm/script.h>
#include <neo/vm/script_builder.h>

#include <sstream>
#include <limits>

namespace neo::smartcontract
{
// ContractParameter implementation
ContractParameter::ContractParameter() : type_(ContractParameterType::Void) {}

ContractParameter::ContractParameter(ContractParameterType type) : type_(type) {}

ContractParameterType ContractParameter::GetType() const { return type_; }

void ContractParameter::SetType(ContractParameterType type) { type_ = type; }

const std::optional<io::ByteVector>& ContractParameter::GetValue() const { return value_; }

void ContractParameter::SetValue(const io::ByteVector& value) { value_ = value; }

const std::vector<ContractParameter>& ContractParameter::GetArray() const { return array_; }

void ContractParameter::SetArray(const std::vector<ContractParameter>& value) { array_ = value; }

const std::vector<std::pair<ContractParameter, ContractParameter>>& ContractParameter::GetMap() const { return map_; }

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

ContractParameter ContractParameter::CreateMap(
    const std::vector<std::pair<ContractParameter, ContractParameter>>& value)
{
    ContractParameter parameter(ContractParameterType::Map);
    parameter.SetMap(value);
    return parameter;
}

ContractParameter ContractParameter::CreateVoid() { return ContractParameter(ContractParameterType::Void); }

// Contract implementation
Contract::Contract() = default;

Contract::Contract(const io::ByteVector& script, const std::vector<ContractParameterType>& parameterList)
    : script_(script), parameterList_(parameterList)
{
}

const io::ByteVector& Contract::GetScript() const { return script_; }

void Contract::SetScript(const io::ByteVector& script) { script_ = script; }

const std::vector<ContractParameterType>& Contract::GetParameterList() const { return parameterList_; }

void Contract::SetParameterList(const std::vector<ContractParameterType>& parameterList)
{
    parameterList_ = parameterList;
}

io::UInt160 Contract::GetScriptHash() const { return cryptography::Hash::Hash160(script_.AsSpan()); }

void Contract::Serialize(io::BinaryWriter& writer) const
{
    writer.WriteVarBytes(script_.AsSpan());
    writer.WriteVarInt(static_cast<uint64_t>(parameterList_.size()));
    for (const auto& parameter : parameterList_)
    {
        writer.Write(static_cast<uint8_t>(parameter));
    }
}

void Contract::Deserialize(io::BinaryReader& reader)
{
    script_ = reader.ReadVarBytes();
    int64_t count = reader.ReadVarInt();
    if (count < 0 || static_cast<uint64_t>(count) > std::numeric_limits<size_t>::max())
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
    vm::ScriptBuilder builder;
    auto pubKeyBytes = pubKey.ToArray();
    builder.EmitPush(pubKeyBytes.AsSpan());
    builder.EmitSysCall("System.Crypto.CheckSig");
    auto script = builder.ToArray();
    return Contract(script, {ContractParameterType::Signature});
}

Contract Contract::CreateMultiSigContract(int m, const std::vector<cryptography::ecc::ECPoint>& pubKeys)
{
    if (m <= 0 || static_cast<size_t>(m) > pubKeys.size() || pubKeys.size() > 1024)
        throw std::invalid_argument("Invalid parameters");

    vm::ScriptBuilder builder;
    builder.EmitPush(static_cast<int64_t>(m));
    std::vector<ContractParameterType> parameterList(static_cast<size_t>(m), ContractParameterType::Signature);

    for (const auto& pubKey : pubKeys)
    {
        auto pubKeyBytes = pubKey.ToArray();
        builder.EmitPush(pubKeyBytes.AsSpan());
    }

    builder.EmitPush(static_cast<int64_t>(pubKeys.size()));
    builder.EmitSysCall("System.Crypto.CheckMultisig");
    auto script = builder.ToArray();
    return Contract(script, parameterList);
}

}  // namespace neo::smartcontract

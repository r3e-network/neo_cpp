/**
 * @file witness.cpp
 * @brief Witness
 * @author Neo C++ Team
 * @date 2025
 * @copyright MIT License
 */

#include <neo/cryptography/hash.h>
#include <neo/io/binary_reader.h>
#include <neo/io/binary_writer.h>
#include <neo/io/json_reader.h>
#include <neo/io/json_writer.h>
#include <neo/ledger/witness.h>

namespace neo::ledger
{
Witness::Witness() = default;

Witness::Witness(const io::ByteVector& invocationScript, const io::ByteVector& verificationScript)
    : invocationScript_(invocationScript), verificationScript_(verificationScript)
{
}

const io::ByteVector& Witness::GetInvocationScript() const { return invocationScript_; }

void Witness::SetInvocationScript(const io::ByteVector& invocationScript) { invocationScript_ = invocationScript; }

const io::ByteVector& Witness::GetVerificationScript() const { return verificationScript_; }

void Witness::SetVerificationScript(const io::ByteVector& verificationScript)
{
    verificationScript_ = verificationScript;
}

io::UInt160 Witness::GetScriptHash() const { return cryptography::Hash::Hash160(verificationScript_.AsSpan()); }

int Witness::GetSize() const
{
    return static_cast<int>(invocationScript_.Size() + verificationScript_.Size() + 2);  // +2 for length prefixes
}

void Witness::Serialize(io::BinaryWriter& writer) const
{
    writer.WriteVarBytes(invocationScript_.AsSpan());
    writer.WriteVarBytes(verificationScript_.AsSpan());
}

void Witness::Deserialize(io::BinaryReader& reader)
{
    invocationScript_ = reader.ReadVarBytes();
    verificationScript_ = reader.ReadVarBytes();
}

void Witness::SerializeJson(io::JsonWriter& writer) const
{
    writer.WriteStartObject();
    writer.WriteProperty("invocation", invocationScript_.ToHexString());
    writer.WriteProperty("verification", verificationScript_.ToHexString());
    writer.WriteEndObject();
}

void Witness::DeserializeJson(const io::JsonReader& reader)
{
    // Read invocation script as hex string
    std::string invocationHex = reader.ReadString("invocation");
    invocationScript_ = io::ByteVector::ParseHex(invocationHex);

    // Read verification script as hex string
    std::string verificationHex = reader.ReadString("verification");
    verificationScript_ = io::ByteVector::ParseHex(verificationHex);
}

bool Witness::operator==(const Witness& other) const
{
    return invocationScript_ == other.invocationScript_ && verificationScript_ == other.verificationScript_;
}

bool Witness::operator!=(const Witness& other) const { return !(*this == other); }
}  // namespace neo::ledger

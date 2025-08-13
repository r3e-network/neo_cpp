/**
 * @file contract_state.cpp
 * @brief Contract State
 * @author Neo C++ Team
 * @date 2025
 * @copyright MIT License
 */

#include <neo/cryptography/hash.h>
#include <neo/io/binary_reader.h>
#include <neo/io/binary_writer.h>
#include <neo/smartcontract/contract_state.h>

#include <sstream>

namespace neo::smartcontract
{
ContractState::ContractState() : id_(0), updateCounter_(0), scriptHash_(), script_(), manifest_() {}

int32_t ContractState::GetId() const { return id_; }

void ContractState::SetId(int32_t id) { id_ = id; }

const io::UInt160& ContractState::GetScriptHash() const { return scriptHash_; }

void ContractState::SetScriptHash(const io::UInt160& scriptHash) { scriptHash_ = scriptHash; }

const io::ByteVector& ContractState::GetScript() const { return script_; }

void ContractState::SetScript(const io::ByteVector& script) { script_ = script; }

const std::string& ContractState::GetManifest() const { return manifest_; }

void ContractState::SetManifest(const std::string& manifest) { manifest_ = manifest; }

uint16_t ContractState::GetUpdateCounter() const { return updateCounter_; }

void ContractState::SetUpdateCounter(uint16_t updateCounter) { updateCounter_ = updateCounter; }

void ContractState::Serialize(io::BinaryWriter& writer) const
{
    writer.Write(id_);
    writer.Write(updateCounter_);
    writer.Write(scriptHash_);
    writer.WriteVarBytes(script_.AsSpan());
    writer.WriteVarString(manifest_);
}

void ContractState::Deserialize(io::BinaryReader& reader)
{
    id_ = reader.ReadInt32();
    updateCounter_ = reader.ReadUInt16();
    scriptHash_ = reader.ReadSerializable<io::UInt160>();
    script_ = reader.ReadVarBytes();
    manifest_ = reader.ReadVarString();
}
}  // namespace neo::smartcontract

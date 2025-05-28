#include <neo/smartcontract/contract_state.h>
#include <neo/io/binary_reader.h>
#include <neo/io/binary_writer.h>
#include <neo/cryptography/hash.h>
#include <sstream>

namespace neo::smartcontract
{
    ContractState::ContractState()
        : id_(0), updateCounter_(0), manifest_(new manifest::ContractManifest())
    {
    }

    ContractState::~ContractState()
    {
        delete manifest_;
    }

    uint32_t ContractState::GetId() const
    {
        return id_;
    }

    void ContractState::SetId(uint32_t id)
    {
        id_ = id;
    }

    uint16_t ContractState::GetUpdateCounter() const
    {
        return updateCounter_;
    }

    void ContractState::SetUpdateCounter(uint16_t updateCounter)
    {
        updateCounter_ = updateCounter;
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

    const manifest::ContractManifest& ContractState::GetManifest() const
    {
        return *manifest_;
    }

    void ContractState::SetManifest(const manifest::ContractManifest& manifest)
    {
        *manifest_ = manifest;
    }

    void ContractState::SetManifestFromJson(const std::string& manifestJson)
    {
        *manifest_ = manifest::ContractManifest::Parse(manifestJson);
    }

    io::UInt160 ContractState::CalculateHash(const io::UInt160& sender, uint32_t nefChecksum, const std::string& name)
    {
        std::ostringstream stream;
        io::BinaryWriter writer(stream);
        writer.Write(sender);
        writer.Write(nefChecksum);
        writer.WriteVarString(name);
        std::string data = stream.str();
        return cryptography::Hash::Hash160(io::ByteSpan(reinterpret_cast<const uint8_t*>(data.data()), data.size()));
    }

    void ContractState::Serialize(io::BinaryWriter& writer) const
    {
        writer.Write(id_);
        writer.Write(updateCounter_);
        writer.Write(scriptHash_);
        writer.WriteVarBytes(script_.Data(), script_.Size());
        writer.WriteVarString(manifest_->ToJson());
    }

    void ContractState::Deserialize(io::BinaryReader& reader)
    {
        id_ = reader.ReadUInt32();
        updateCounter_ = reader.ReadUInt16();
        scriptHash_ = reader.ReadSerializable<io::UInt160>();
        script_ = reader.ReadVarBytes();
        std::string manifestJson = reader.ReadVarString();
        *manifest_ = manifest::ContractManifest::Parse(manifestJson);
    }
}

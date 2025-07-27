#include <neo/blockchain/header.h>
#include <neo/io/binary_reader.h>
#include <neo/io/binary_writer.h>
#include <neo/io/json_reader.h>
#include <neo/io/json_writer.h>

namespace neo::blockchain
{
Header::Header()
    : version_(0), prevHash_(), merkleRoot_(), timestamp_(0), nonce_(0), index_(0), primaryIndex_(0), nextConsensus_(),
      witness_(), hash_()
{
}

uint32_t Header::GetVersion() const
{
    return version_;
}

void Header::SetVersion(uint32_t version)
{
    version_ = version;
}

const io::UInt256& Header::GetPrevHash() const
{
    return prevHash_;
}

void Header::SetPrevHash(const io::UInt256& prevHash)
{
    prevHash_ = prevHash;
}

const io::UInt256& Header::GetMerkleRoot() const
{
    return merkleRoot_;
}

void Header::SetMerkleRoot(const io::UInt256& merkleRoot)
{
    merkleRoot_ = merkleRoot;
}

uint64_t Header::GetTimestamp() const
{
    return timestamp_;
}

void Header::SetTimestamp(uint64_t timestamp)
{
    timestamp_ = timestamp;
}

uint32_t Header::GetIndex() const
{
    return index_;
}

void Header::SetIndex(uint32_t index)
{
    index_ = index;
}

const io::UInt160& Header::GetNextConsensus() const
{
    return nextConsensus_;
}

void Header::SetNextConsensus(const io::UInt160& nextConsensus)
{
    nextConsensus_ = nextConsensus;
}

const ledger::Witness& Header::GetWitness() const
{
    return witness_;
}

void Header::SetWitness(const ledger::Witness& witness)
{
    witness_ = witness;
}

void Header::Serialize(io::BinaryWriter& writer) const
{
    writer.Write(static_cast<uint8_t>(version_));
    writer.Write(prevHash_);
    writer.Write(merkleRoot_);
    writer.Write(timestamp_);
    writer.Write(index_);
    writer.Write(nextConsensus_);
    witness_.Serialize(writer);
}

void Header::Deserialize(io::BinaryReader& reader)
{
    version_ = reader.ReadUInt8();
    prevHash_ = reader.ReadSerializable<io::UInt256>();
    merkleRoot_ = reader.ReadSerializable<io::UInt256>();
    timestamp_ = reader.ReadUInt64();
    index_ = reader.ReadUInt32();
    nextConsensus_ = reader.ReadSerializable<io::UInt160>();
    witness_.Deserialize(reader);
}

void Header::SerializeJson(io::JsonWriter& writer) const
{
    writer.WriteStartObject();
    writer.WriteProperty("version", static_cast<int>(version_));
    writer.WriteProperty("prevhash", prevHash_.ToString());
    writer.WriteProperty("merkleroot", merkleRoot_.ToString());
    writer.WriteProperty("timestamp", timestamp_);
    writer.WriteProperty("index", index_);
    writer.WriteProperty("nextconsensus", nextConsensus_.ToString());

    writer.WritePropertyName("witness");
    witness_.SerializeJson(writer);

    writer.WriteEndObject();
}

void Header::DeserializeJson(const io::JsonReader& reader)
{
    version_ = reader.ReadUInt32("version");
    prevHash_ = io::UInt256::Parse(reader.ReadString("prevhash"));
    merkleRoot_ = io::UInt256::Parse(reader.ReadString("merkleroot"));
    timestamp_ = reader.ReadUInt64("timestamp");
    index_ = reader.ReadUInt32("index");
    nextConsensus_ = io::UInt160::Parse(reader.ReadString("nextconsensus"));

    if (reader.HasKey("witness"))
    {
        auto witnessJson = reader.GetJson()["witness"];
        io::JsonReader witnessReader(witnessJson);
        witness_.DeserializeJson(witnessReader);
    }
}
}  // namespace neo::blockchain
#include <neo/network/payloads/merkle_block_payload.h>
#include <stdexcept>
#include <limits>

namespace neo::network::payloads
{
    MerkleBlockPayload::MerkleBlockPayload()
        : transactionCount_(0)
    {
    }

    MerkleBlockPayload::MerkleBlockPayload(std::shared_ptr<blockchain::Header> header, uint32_t transactionCount,
                                         const std::vector<io::UInt256>& hashes, const io::ByteVector& flags)
        : header_(header), transactionCount_(transactionCount), hashes_(hashes), flags_(flags)
    {
    }

    std::shared_ptr<blockchain::Header> MerkleBlockPayload::GetHeader() const
    {
        return header_;
    }

    void MerkleBlockPayload::SetHeader(std::shared_ptr<blockchain::Header> header)
    {
        header_ = header;
    }

    uint32_t MerkleBlockPayload::GetTransactionCount() const
    {
        return transactionCount_;
    }

    void MerkleBlockPayload::SetTransactionCount(uint32_t count)
    {
        transactionCount_ = count;
    }

    const std::vector<io::UInt256>& MerkleBlockPayload::GetHashes() const
    {
        return hashes_;
    }

    void MerkleBlockPayload::SetHashes(const std::vector<io::UInt256>& hashes)
    {
        hashes_ = hashes;
    }

    const io::ByteVector& MerkleBlockPayload::GetFlags() const
    {
        return flags_;
    }

    void MerkleBlockPayload::SetFlags(const io::ByteVector& flags)
    {
        flags_ = flags;
    }

    void MerkleBlockPayload::Serialize(io::BinaryWriter& writer) const
    {
        if (header_)
        {
            header_->Serialize(writer);
        }
        else
        {
            // Serialize an empty header if header_ is null
            blockchain::Header emptyHeader;
            emptyHeader.Serialize(writer);
        }

        writer.Write(transactionCount_);
        writer.WriteVarInt(hashes_.size());
        for (const auto& hash : hashes_)
        {
            writer.Write(hash);
        }
        writer.WriteVarBytes(flags_.AsSpan());
    }

    void MerkleBlockPayload::Deserialize(io::BinaryReader& reader)
    {
        header_ = std::make_shared<blockchain::Header>();
        header_->Deserialize(reader);
        
        transactionCount_ = reader.ReadUInt32();
        
        int64_t hashCount = reader.ReadVarInt();
        if (hashCount < 0 || hashCount > std::numeric_limits<size_t>::max())
            throw std::out_of_range("Invalid hash count");
        
        hashes_.clear();
        hashes_.reserve(static_cast<size_t>(hashCount));
        
        for (int64_t i = 0; i < hashCount; i++)
        {
            io::UInt256 hash = reader.ReadUInt256();
            hashes_.push_back(hash);
        }
        
        flags_ = reader.ReadVarBytes(8192); // Reasonable maximum size for flags
    }

    void MerkleBlockPayload::SerializeJson(io::JsonWriter& writer) const
    {
        writer.WriteStartObject("header");
        if (header_)
        {
            header_->SerializeJson(writer);
        }
        writer.WriteEndObject();
        
        writer.Write("transactionCount", transactionCount_);
        
        writer.WriteStartArray("hashes");
        for (const auto& hash : hashes_)
        {
            writer.Write(hash);
        }
        writer.WriteEndArray();
        
        writer.Write("flags", flags_);
    }

    void MerkleBlockPayload::DeserializeJson(const io::JsonReader& reader)
    {
        io::JsonReader headerReader = reader.ReadObject("header");
        header_ = std::make_shared<blockchain::Header>();
        header_->DeserializeJson(headerReader);
        
        transactionCount_ = reader.ReadUInt32("transactionCount");
        
        auto hashesArray = reader.ReadArray("hashes");
        hashes_.clear();
        hashes_.reserve(hashesArray.size());
        
        for (const auto& hashJson : hashesArray)
        {
            io::JsonReader hashReader(hashJson);
            io::UInt256 hash = hashReader.ReadUInt256();
            hashes_.push_back(hash);
        }
        
        flags_ = reader.ReadByteVector("flags");
    }
}

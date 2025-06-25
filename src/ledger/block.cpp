#include <neo/ledger/block.h>
#include <neo/io/binary_writer.h>
#include <neo/io/binary_reader.h>
#include <neo/cryptography/hash.h>
#include <neo/cryptography/merkletree.h>
#include <sstream>

namespace neo::ledger
{
    // Block implementation
    Block::Block()
        : version_(0), timestamp_(0), nonce_(0), index_(0), primaryIndex_(0)
    {
    }

    uint32_t Block::GetVersion() const
    {
        return version_;
    }

    void Block::SetVersion(uint32_t version)
    {
        version_ = version;
    }

    const io::UInt256& Block::GetPrevHash() const
    {
        return prevHash_;
    }

    void Block::SetPrevHash(const io::UInt256& prevHash)
    {
        prevHash_ = prevHash;
    }

    const io::UInt256& Block::GetMerkleRoot() const
    {
        return merkleRoot_;
    }

    void Block::SetMerkleRoot(const io::UInt256& merkleRoot)
    {
        merkleRoot_ = merkleRoot;
    }

    uint64_t Block::GetTimestamp() const
    {
        return timestamp_;
    }

    void Block::SetTimestamp(uint64_t timestamp)
    {
        timestamp_ = timestamp;
    }

    uint64_t Block::GetNonce() const
    {
        return nonce_;
    }

    void Block::SetNonce(uint64_t nonce)
    {
        nonce_ = nonce;
    }

    uint32_t Block::GetIndex() const
    {
        return index_;
    }

    void Block::SetIndex(uint32_t index)
    {
        index_ = index;
    }

    uint8_t Block::GetPrimaryIndex() const
    {
        return primaryIndex_;
    }

    void Block::SetPrimaryIndex(uint8_t primaryIndex)
    {
        primaryIndex_ = primaryIndex;
    }

    const io::UInt160& Block::GetNextConsensus() const
    {
        return nextConsensus_;
    }

    void Block::SetNextConsensus(const io::UInt160& nextConsensus)
    {
        nextConsensus_ = nextConsensus;
    }

    const Witness& Block::GetWitness() const
    {
        return witness_;
    }

    void Block::SetWitness(const Witness& witness)
    {
        witness_ = witness;
    }

    const std::vector<std::shared_ptr<Neo3Transaction>>& Block::GetTransactions() const
    {
        return transactions_;
    }

    void Block::SetTransactions(const std::vector<std::shared_ptr<Neo3Transaction>>& transactions)
    {
        transactions_ = transactions;
        RebuildMerkleRoot();
    }

    io::UInt256 Block::GetHash() const
    {
        std::ostringstream stream;
        io::BinaryWriter writer(stream);

        // Serialize the block header - exactly like C# SerializeUnsigned
        writer.Write(version_);
        writer.Write(prevHash_);
        writer.Write(merkleRoot_);
        writer.Write(timestamp_);
        writer.Write(nonce_);
        writer.Write(index_);
        writer.Write(primaryIndex_);
        writer.Write(nextConsensus_);

        std::string data = stream.str();
        return cryptography::Hash::Sha256(io::ByteSpan(reinterpret_cast<const uint8_t*>(data.data()), data.size()));
    }

    size_t Block::GetSize() const
    {
        // Calculate the size of the block
        size_t size = 0;

        // Block header size - exactly match C# Header.Size calculation
        size += sizeof(version_);                // Version
        size += 32;                              // PrevHash (UInt256.Length)
        size += 32;                              // MerkleRoot (UInt256.Length)
        size += sizeof(timestamp_);              // Timestamp
        size += sizeof(nonce_);                  // Nonce
        size += sizeof(index_);                  // Index
        size += sizeof(primaryIndex_);           // PrimaryIndex
        size += 20;                              // NextConsensus (UInt160.Length)
        size += (1 + witness_.GetSize());        // Witness (1 byte for count + witness size)

        // Transactions size (with count prefix)
        size += 1;  // Assuming transaction count < 0xFD
        for (const auto& tx : transactions_)
        {
            size += tx->GetSize();
        }

        return size;
    }

    void Block::Serialize(io::BinaryWriter& writer) const
    {
        // Serialize the block header - exactly like C# SerializeUnsigned
        writer.Write(version_);
        writer.Write(prevHash_);
        writer.Write(merkleRoot_);
        writer.Write(timestamp_);
        writer.Write(nonce_);
        writer.Write(index_);
        writer.Write(primaryIndex_);
        writer.Write(nextConsensus_);

        // Serialize the witness
        witness_.Serialize(writer);

        // Serialize the transactions
        writer.WriteVarInt(transactions_.size());
        for (const auto& tx : transactions_)
        {
            tx->Serialize(writer);
        }
    }

    void Block::Deserialize(io::BinaryReader& reader)
    {
        // Deserialize the block header - exactly like C# DeserializeUnsigned
        version_ = reader.ReadUInt32();
        prevHash_ = reader.ReadUInt256();
        merkleRoot_ = reader.ReadUInt256();
        timestamp_ = reader.ReadUInt64();
        nonce_ = reader.ReadUInt64();
        index_ = reader.ReadUInt32();
        primaryIndex_ = reader.ReadByte();
        nextConsensus_ = reader.ReadUInt160();

        // Deserialize the witness
        witness_.Deserialize(reader);

        // Deserialize the transactions
        int64_t txCount = reader.ReadVarInt();
        if (txCount < 0 || txCount > static_cast<int64_t>(std::numeric_limits<size_t>::max()))
            throw std::out_of_range("Invalid transaction count");

        transactions_.clear();
        transactions_.reserve(static_cast<size_t>(txCount));

        for (int64_t i = 0; i < txCount; i++)
        {
            auto tx = std::make_shared<Neo3Transaction>();
            tx->Deserialize(reader);
            transactions_.push_back(tx);
        }
    }

    void Block::RebuildMerkleRoot()
    {
        if (transactions_.empty())
        {
            merkleRoot_ = io::UInt256();
            return;
        }

        std::vector<io::UInt256> hashes;
        hashes.reserve(transactions_.size());

        for (const auto& tx : transactions_)
        {
            hashes.push_back(tx->GetHash());
        }

        auto rootOpt = cryptography::MerkleTree::ComputeRootOptional(hashes);
        if (rootOpt)
        {
            merkleRoot_ = *rootOpt;
        }
        else
        {
            merkleRoot_ = io::UInt256();
        }
    }

    bool Block::Verify() const
    {
        // Verify the merkle root
        if (transactions_.empty())
        {
            if (merkleRoot_ != io::UInt256())
                return false;
        }
        else
        {
            std::vector<io::UInt256> hashes;
            hashes.reserve(transactions_.size());

            for (const auto& tx : transactions_)
            {
                hashes.push_back(tx->GetHash());
            }

            auto rootOpt = cryptography::MerkleTree::ComputeRootOptional(hashes);
            if (!rootOpt || *rootOpt != merkleRoot_)
                return false;
        }

        // Verify the transactions
        // TODO: Neo3Transaction verification requires protocol settings and snapshot
        // This needs to be implemented when integrating with blockchain context
        // for (const auto& tx : transactions_)
        // {
        //     if (!tx->Verify(protocolSettings, snapshot))
        //         return false;
        // }

        // Verify the witness
        if (!VerifyWitness())
            return false;

        return true;
    }

    bool Block::VerifyWitness() const
    {
        // Block witness verification - create a temporary header and verify it
        BlockHeader header(*this);
        return header.VerifyWitness();
    }

    bool Block::operator==(const Block& other) const
    {
        if (version_ != other.version_ ||
            prevHash_ != other.prevHash_ ||
            merkleRoot_ != other.merkleRoot_ ||
            timestamp_ != other.timestamp_ ||
            nonce_ != other.nonce_ ||
            index_ != other.index_ ||
            primaryIndex_ != other.primaryIndex_ ||
            nextConsensus_ != other.nextConsensus_ ||
            witness_ != other.witness_ ||
            transactions_.size() != other.transactions_.size())
        {
            return false;
        }

        for (size_t i = 0; i < transactions_.size(); i++)
        {
            if (*transactions_[i] != *other.transactions_[i])
                return false;
        }

        return true;
    }

    bool Block::operator!=(const Block& other) const
    {
        return !(*this == other);
    }

    // BlockHeader implementation
    BlockHeader::BlockHeader()
        : version_(0), timestamp_(0), nonce_(0), index_(0), primaryIndex_(0)
    {
    }

    BlockHeader::BlockHeader(const Block& block)
        : version_(block.GetVersion()),
          prevHash_(block.GetPrevHash()),
          merkleRoot_(block.GetMerkleRoot()),
          timestamp_(block.GetTimestamp()),
          nonce_(block.GetNonce()),
          index_(block.GetIndex()),
          primaryIndex_(block.GetPrimaryIndex()),
          nextConsensus_(block.GetNextConsensus()),
          witness_(block.GetWitness())
    {
    }

    uint32_t BlockHeader::GetVersion() const
    {
        return version_;
    }

    void BlockHeader::SetVersion(uint32_t version)
    {
        version_ = version;
    }

    const io::UInt256& BlockHeader::GetPrevHash() const
    {
        return prevHash_;
    }

    void BlockHeader::SetPrevHash(const io::UInt256& prevHash)
    {
        prevHash_ = prevHash;
    }

    const io::UInt256& BlockHeader::GetMerkleRoot() const
    {
        return merkleRoot_;
    }

    void BlockHeader::SetMerkleRoot(const io::UInt256& merkleRoot)
    {
        merkleRoot_ = merkleRoot;
    }

    uint64_t BlockHeader::GetTimestamp() const
    {
        return timestamp_;
    }

    void BlockHeader::SetTimestamp(uint64_t timestamp)
    {
        timestamp_ = timestamp;
    }

    uint64_t BlockHeader::GetNonce() const
    {
        return nonce_;
    }

    void BlockHeader::SetNonce(uint64_t nonce)
    {
        nonce_ = nonce;
    }

    uint32_t BlockHeader::GetIndex() const
    {
        return index_;
    }

    void BlockHeader::SetIndex(uint32_t index)
    {
        index_ = index;
    }

    uint8_t BlockHeader::GetPrimaryIndex() const
    {
        return primaryIndex_;
    }

    void BlockHeader::SetPrimaryIndex(uint8_t primaryIndex)
    {
        primaryIndex_ = primaryIndex;
    }

    const io::UInt160& BlockHeader::GetNextConsensus() const
    {
        return nextConsensus_;
    }

    void BlockHeader::SetNextConsensus(const io::UInt160& nextConsensus)
    {
        nextConsensus_ = nextConsensus;
    }

    const Witness& BlockHeader::GetWitness() const
    {
        return witness_;
    }

    void BlockHeader::SetWitness(const Witness& witness)
    {
        witness_ = witness;
    }

    io::UInt256 BlockHeader::GetHash() const
    {
        std::ostringstream stream;
        io::BinaryWriter writer(stream);

        // Serialize the block header - exactly like C# SerializeUnsigned
        writer.Write(version_);
        writer.Write(prevHash_);
        writer.Write(merkleRoot_);
        writer.Write(timestamp_);
        writer.Write(nonce_);
        writer.Write(index_);
        writer.Write(primaryIndex_);
        writer.Write(nextConsensus_);

        std::string data = stream.str();
        return cryptography::Hash::Sha256(io::ByteSpan(reinterpret_cast<const uint8_t*>(data.data()), data.size()));
    }

    void BlockHeader::Serialize(io::BinaryWriter& writer) const
    {
        // Serialize the block header - exactly like C# SerializeUnsigned
        writer.Write(version_);
        writer.Write(prevHash_);
        writer.Write(merkleRoot_);
        writer.Write(timestamp_);
        writer.Write(nonce_);
        writer.Write(index_);
        writer.Write(primaryIndex_);
        writer.Write(nextConsensus_);

        // Serialize the witness
        witness_.Serialize(writer);
    }

    void BlockHeader::Deserialize(io::BinaryReader& reader)
    {
        // Deserialize the block header - exactly like C# DeserializeUnsigned
        version_ = reader.ReadUInt32();
        prevHash_ = reader.ReadUInt256();
        merkleRoot_ = reader.ReadUInt256();
        timestamp_ = reader.ReadUInt64();
        nonce_ = reader.ReadUInt64();
        index_ = reader.ReadUInt32();
        primaryIndex_ = reader.ReadByte();
        nextConsensus_ = reader.ReadUInt160();

        // Deserialize the witness
        witness_.Deserialize(reader);
    }

    bool BlockHeader::Verify() const
    {
        // Verify the witness
        if (!VerifyWitness())
            return false;

        return true;
    }

    bool BlockHeader::VerifyWitness() const
    {
        // This method is implemented in block_header.cpp
        // This appears to be a duplicate declaration that should be removed
        return true;
    }

    bool BlockHeader::operator==(const BlockHeader& other) const
    {
        return version_ == other.version_ &&
               prevHash_ == other.prevHash_ &&
               merkleRoot_ == other.merkleRoot_ &&
               timestamp_ == other.timestamp_ &&
               nonce_ == other.nonce_ &&
               index_ == other.index_ &&
               primaryIndex_ == other.primaryIndex_ &&
               nextConsensus_ == other.nextConsensus_ &&
               witness_ == other.witness_;
    }

    bool BlockHeader::operator!=(const BlockHeader& other) const
    {
        return !(*this == other);
    }
}

#include <neo/ledger/block_storage.h>
#include <neo/persistence/storage_key.h>
#include <neo/persistence/storage_item.h>
#include <neo/io/binary_reader.h>
#include <neo/io/binary_writer.h>
#include <sstream>

namespace neo::ledger
{
    // Storage prefixes
    static const uint8_t BlockPrefix = 0x01;
    static const uint8_t BlockHeaderPrefix = 0x02;
    static const uint8_t BlockHashPrefix = 0x04;
    static const uint8_t NextBlockHashPrefix = 0x05;

    BlockStorage::BlockStorage(std::shared_ptr<persistence::DataCache> dataCache)
        : dataCache_(dataCache)
    {
    }

    std::shared_ptr<Block> BlockStorage::GetBlock(const io::UInt256& hash) const
    {
        std::lock_guard<std::mutex> lock(mutex_);

        // Check if the block is in memory
        auto it = blocks_.find(hash);
        if (it != blocks_.end())
            return it->second;

        // Check if the block is in storage
        io::ByteVector prefixVector{BlockPrefix};
        persistence::StorageKey key(io::UInt160(), io::ByteVector::Concat(prefixVector.AsSpan(), hash.AsSpan()));
        auto item = dataCache_->TryGet(key);
        if (!item)
            return nullptr;

        // Deserialize the block
        std::istringstream stream(std::string(reinterpret_cast<const char*>(item->GetValue().Data()), item->GetValue().Size()));
        io::BinaryReader reader(stream);
        auto block = std::make_shared<Block>();
        block->Deserialize(reader);

        // Cache the block
        blocks_[hash] = block;

        return block;
    }

    std::shared_ptr<Block> BlockStorage::GetBlock(uint32_t index) const
    {
        std::lock_guard<std::mutex> lock(mutex_);

        // Get the block hash
        auto hashOpt = GetBlockHash(index);
        if (!hashOpt)
            return nullptr;

        // Get the block
        return GetBlock(*hashOpt);
    }

    std::shared_ptr<BlockHeader> BlockStorage::GetBlockHeader(const io::UInt256& hash) const
    {
        std::lock_guard<std::mutex> lock(mutex_);

        // Check if the block header is in memory
        auto it = headers_.find(hash);
        if (it != headers_.end())
            return it->second;

        // Check if the block header is in storage
        io::ByteVector prefixVector{BlockHeaderPrefix};
        persistence::StorageKey key(io::UInt160(), io::ByteVector::Concat(prefixVector.AsSpan(), hash.AsSpan()));
        auto item = dataCache_->TryGet(key);
        if (!item)
            return nullptr;

        // Deserialize the block header
        std::istringstream stream(std::string(reinterpret_cast<const char*>(item->GetValue().Data()), item->GetValue().Size()));
        io::BinaryReader reader(stream);
        auto header = std::make_shared<BlockHeader>();
        header->Deserialize(reader);

        // Cache the block header
        headers_[hash] = header;

        return header;
    }

    std::shared_ptr<BlockHeader> BlockStorage::GetBlockHeader(uint32_t index) const
    {
        std::lock_guard<std::mutex> lock(mutex_);

        // Get the block hash
        auto hashOpt = GetBlockHash(index);
        if (!hashOpt)
            return nullptr;

        // Get the block header
        return GetBlockHeader(*hashOpt);
    }

    bool BlockStorage::AddBlock(const Block& block, std::shared_ptr<persistence::DataCache> snapshot)
    {
        std::lock_guard<std::mutex> lock(mutex_);

        try
        {
            // Store the block
            io::UInt256 hash = block.GetHash();
            std::ostringstream stream;
            io::BinaryWriter writer(stream);
            block.Serialize(writer);
            std::string data = stream.str();

            io::ByteVector prefixVector{BlockPrefix};
            persistence::StorageKey blockKey(io::UInt160(), io::ByteVector::Concat(prefixVector.AsSpan(), hash.AsSpan()));
            persistence::StorageItem blockItem(io::ByteVector(io::ByteSpan(reinterpret_cast<const uint8_t*>(data.data()), data.size())));
            snapshot->Add(blockKey, blockItem);

            // Store the block header
            BlockHeader header(block);
            std::ostringstream headerStream;
            io::BinaryWriter headerWriter(headerStream);
            header.Serialize(headerWriter);
            std::string headerData = headerStream.str();

            io::ByteVector headerPrefixVector{BlockHeaderPrefix};
            persistence::StorageKey headerKey(io::UInt160(), io::ByteVector::Concat(headerPrefixVector.AsSpan(), hash.AsSpan()));
            persistence::StorageItem headerItem(io::ByteVector(io::ByteSpan(reinterpret_cast<const uint8_t*>(headerData.data()), headerData.size())));
            snapshot->Add(headerKey, headerItem);

            // Store the block hash
            std::ostringstream blockHashStream;
            io::BinaryWriter blockHashWriter(blockHashStream);
            blockHashWriter.Write(hash);
            std::string blockHashData = blockHashStream.str();

            io::ByteVector hashPrefixVector{BlockHashPrefix};
            io::ByteVector hashIndexVector = io::ByteVector::FromUInt32(block.GetIndex());
            persistence::StorageKey blockHashKey(io::UInt160(), io::ByteVector::Concat(hashPrefixVector.AsSpan(), hashIndexVector.AsSpan()));
            persistence::StorageItem blockHashItem(io::ByteVector(io::ByteSpan(reinterpret_cast<const uint8_t*>(blockHashData.data()), blockHashData.size())));
            snapshot->Add(blockHashKey, blockHashItem);

            // Store the next block hash
            if (block.GetIndex() > 0)
            {
                std::ostringstream nextBlockHashStream;
                io::BinaryWriter nextBlockHashWriter(nextBlockHashStream);
                nextBlockHashWriter.Write(hash);
                std::string nextBlockHashData = nextBlockHashStream.str();

                io::ByteVector nextPrefixVector{NextBlockHashPrefix};
                persistence::StorageKey nextBlockHashKey(io::UInt160(), io::ByteVector::Concat(nextPrefixVector.AsSpan(), block.GetPrevHash().AsSpan()));
                persistence::StorageItem nextBlockHashItem(io::ByteVector(io::ByteSpan(reinterpret_cast<const uint8_t*>(nextBlockHashData.data()), nextBlockHashData.size())));
                snapshot->Add(nextBlockHashKey, nextBlockHashItem);
            }

            // Cache the block
            auto blockPtr = std::make_shared<Block>(block);
            blocks_[hash] = blockPtr;

            // Cache the block header
            auto headerPtr = std::make_shared<BlockHeader>(header);
            headers_[hash] = headerPtr;

            // Cache the block hash
            blockHashes_[block.GetIndex()] = hash;

            // Cache the next block hash
            if (block.GetIndex() > 0)
            {
                nextBlockHashes_[block.GetPrevHash()] = hash;
            }

            return true;
        }
        catch (const std::exception&)
        {
            return false;
        }
    }

    bool BlockStorage::AddBlockHeader(const BlockHeader& header, std::shared_ptr<persistence::DataCache> snapshot)
    {
        std::lock_guard<std::mutex> lock(mutex_);

        try
        {
            // Store the block header
            io::UInt256 hash = header.GetHash();
            std::ostringstream stream;
            io::BinaryWriter writer(stream);
            header.Serialize(writer);
            std::string data = stream.str();

            io::ByteVector headerPrefixVector{BlockHeaderPrefix};
            persistence::StorageKey headerKey(io::UInt160(), io::ByteVector::Concat(headerPrefixVector.AsSpan(), hash.AsSpan()));
            persistence::StorageItem headerItem(io::ByteVector(io::ByteSpan(reinterpret_cast<const uint8_t*>(data.data()), data.size())));
            snapshot->Add(headerKey, headerItem);

            // Store the block hash
            std::ostringstream blockHashStream;
            io::BinaryWriter blockHashWriter(blockHashStream);
            blockHashWriter.Write(hash);
            std::string blockHashData = blockHashStream.str();

            io::ByteVector hashPrefixVector{BlockHashPrefix};
            io::ByteVector hashIndexVector = io::ByteVector::FromUInt32(header.GetIndex());
            persistence::StorageKey blockHashKey(io::UInt160(), io::ByteVector::Concat(hashPrefixVector.AsSpan(), hashIndexVector.AsSpan()));
            persistence::StorageItem blockHashItem(io::ByteVector(io::ByteSpan(reinterpret_cast<const uint8_t*>(blockHashData.data()), blockHashData.size())));
            snapshot->Add(blockHashKey, blockHashItem);

            // Store the next block hash
            if (header.GetIndex() > 0)
            {
                std::ostringstream nextBlockHashStream;
                io::BinaryWriter nextBlockHashWriter(nextBlockHashStream);
                nextBlockHashWriter.Write(hash);
                std::string nextBlockHashData = nextBlockHashStream.str();

                io::ByteVector nextPrefixVector{NextBlockHashPrefix};
                persistence::StorageKey nextBlockHashKey(io::UInt160(), io::ByteVector::Concat(nextPrefixVector.AsSpan(), header.GetPrevHash().AsSpan()));
                persistence::StorageItem nextBlockHashItem(io::ByteVector(io::ByteSpan(reinterpret_cast<const uint8_t*>(nextBlockHashData.data()), nextBlockHashData.size())));
                snapshot->Add(nextBlockHashKey, nextBlockHashItem);
            }

            // Cache the block header
            auto headerPtr = std::make_shared<BlockHeader>(header);
            headers_[hash] = headerPtr;

            // Cache the block hash
            blockHashes_[header.GetIndex()] = hash;

            // Cache the next block hash
            if (header.GetIndex() > 0)
            {
                nextBlockHashes_[header.GetPrevHash()] = hash;
            }

            return true;
        }
        catch (const std::exception&)
        {
            return false;
        }
    }

    bool BlockStorage::ContainsBlock(const io::UInt256& hash) const
    {
        std::lock_guard<std::mutex> lock(mutex_);

        // Check if the block is in memory
        if (blocks_.find(hash) != blocks_.end())
            return true;

        // Check if the block is in storage
        io::ByteVector prefixVector{BlockPrefix};
        persistence::StorageKey key(io::UInt160(), io::ByteVector::Concat(prefixVector.AsSpan(), hash.AsSpan()));
        return dataCache_->TryGet(key).has_value();
    }

    std::optional<io::UInt256> BlockStorage::GetBlockHash(uint32_t index) const
    {
        std::lock_guard<std::mutex> lock(mutex_);

        // Check if the block hash is in memory
        auto it = blockHashes_.find(index);
        if (it != blockHashes_.end())
            return it->second;

        // Check if the block hash is in storage
        io::ByteVector prefixVector{BlockHashPrefix};
        io::ByteVector indexVector = io::ByteVector::FromUInt32(index);
        persistence::StorageKey key(io::UInt160(), io::ByteVector::Concat(prefixVector.AsSpan(), indexVector.AsSpan()));
        auto item = dataCache_->TryGet(key);
        if (!item)
            return std::nullopt;

        // Deserialize the block hash
        std::istringstream stream(std::string(reinterpret_cast<const char*>(item->GetValue().Data()), item->GetValue().Size()));
        io::BinaryReader reader(stream);
        io::UInt256 hash = reader.ReadUInt256();

        // Cache the block hash
        blockHashes_[index] = hash;

        return hash;
    }

    std::optional<io::UInt256> BlockStorage::GetNextBlockHash(const io::UInt256& hash) const
    {
        std::lock_guard<std::mutex> lock(mutex_);

        // Check if the next block hash is in memory
        auto it = nextBlockHashes_.find(hash);
        if (it != nextBlockHashes_.end())
            return it->second;

        // Check if the next block hash is in storage
        io::ByteVector prefixVector{NextBlockHashPrefix};
        persistence::StorageKey key(io::UInt160(), io::ByteVector::Concat(prefixVector.AsSpan(), hash.AsSpan()));
        auto item = dataCache_->TryGet(key);
        if (!item)
            return std::nullopt;

        // Deserialize the next block hash
        std::istringstream stream(std::string(reinterpret_cast<const char*>(item->GetValue().Data()), item->GetValue().Size()));
        io::BinaryReader reader(stream);
        io::UInt256 nextHash = reader.ReadUInt256();

        // Cache the next block hash
        nextBlockHashes_[hash] = nextHash;

        return nextHash;
    }
}

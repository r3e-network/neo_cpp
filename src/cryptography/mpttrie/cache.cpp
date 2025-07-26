#include <neo/cryptography/mpttrie/cache.h>
#include <neo/io/binary_writer.h>
#include <neo/io/binary_reader.h>
#include <neo/io/memory_stream.h>

namespace neo::cryptography::mpttrie
{
    Cache::Cache(std::shared_ptr<persistence::IStoreSnapshot> store, uint8_t prefix)
        : store_(store), prefix_(prefix)
    {
    }

    std::unique_ptr<Node> Cache::Resolve(const io::UInt256& hash)
    {
        // Check if the node is already in the cache
        auto it = cache_.find(hash);
        if (it != cache_.end())
        {
            if (it->second && it->second->state != TrackState::Deleted)
            {
                // Clone the node to return
                if (it->second->node)
                {
                    return std::make_unique<Node>(*it->second->node);
                }
            }
            return nullptr;
        }

        // Not in cache, try to load from store
        auto key = CreateKey(hash);
        auto value = store_->TryGet(key);
        
        if (!value || value->empty())
        {
            return nullptr;
        }

        // Deserialize the node from storage
        try
        {
            io::MemoryStream stream(*value);
            io::BinaryReader reader(stream);
            
            auto node = std::make_unique<Node>();
            node->Deserialize(reader);
            
            // Add to cache for future access
            auto trackable = std::make_unique<Trackable>();
            trackable->node = std::make_unique<Node>(*node);
            trackable->state = TrackState::None;
            cache_[hash] = std::move(trackable);
            
            return node;
        }
        catch (const std::exception&)
        {
            // Failed to deserialize
            return nullptr;
        }
    }

    void Cache::PutNode(std::unique_ptr<Node> node)
    {
        if (!node)
        {
            return;
        }

        auto hash = node->GetHash();
        
        // Check if already in cache
        auto it = cache_.find(hash);
        if (it != cache_.end())
        {
            // Update existing entry
            it->second->node = std::move(node);
            if (it->second->state == TrackState::None)
            {
                it->second->state = TrackState::Changed;
            }
            // If it was Added or Changed, keep that state
            // If it was Deleted, change to Changed
            else if (it->second->state == TrackState::Deleted)
            {
                it->second->state = TrackState::Changed;
            }
        }
        else
        {
            // New entry
            auto trackable = std::make_unique<Trackable>();
            trackable->node = std::move(node);
            trackable->state = TrackState::Added;
            cache_[hash] = std::move(trackable);
        }
    }

    void Cache::DeleteNode(const io::UInt256& hash)
    {
        auto it = cache_.find(hash);
        if (it != cache_.end())
        {
            it->second->state = TrackState::Deleted;
            it->second->node.reset(); // Clear the node data
        }
        else
        {
            // Not in cache, create a delete marker
            auto trackable = std::make_unique<Trackable>();
            trackable->node = nullptr;
            trackable->state = TrackState::Deleted;
            cache_[hash] = std::move(trackable);
        }
    }

    void Cache::Commit()
    {
        // Process all cached entries
        for (const auto& [hash, trackable] : cache_)
        {
            if (!trackable)
            {
                continue;
            }

            auto key = CreateKey(hash);

            switch (trackable->state)
            {
                case TrackState::Added:
                case TrackState::Changed:
                {
                    if (trackable->node)
                    {
                        // Serialize the node
                        io::MemoryStream stream;
                        io::BinaryWriter writer(stream);
                        trackable->node->Serialize(writer);
                        
                        // Store in database
                        store_->Put(key, stream.ToByteVector());
                    }
                    break;
                }
                
                case TrackState::Deleted:
                {
                    // Remove from database
                    store_->Delete(key);
                    break;
                }
                
                case TrackState::None:
                {
                    // No changes needed
                    break;
                }
            }
        }

        // Clear the cache after commit
        cache_.clear();
    }

    io::ByteVector Cache::CreateKey(const io::UInt256& hash) const
    {
        io::ByteVector result;
        result.Resize(io::UInt256::Size + 1);
        result[0] = prefix_;
        std::memcpy(result.Data() + 1, hash.Data(), io::UInt256::Size);
        return result;
    }
}

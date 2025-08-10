#include <neo/persistence/memory_store.h>

#include <algorithm>

namespace neo::persistence
{
// MemoryStore implementation
MemoryStore::MemoryStore() = default;

MemoryStore::MemoryStore(const MemoryStore& other)
{
    std::lock_guard<std::mutex> lock(other.mutex_);
    store_ = other.store_;
}

std::optional<io::ByteVector> MemoryStore::TryGet(const io::ByteVector& key) const
{
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = store_.find(key);
    if (it == store_.end()) return std::nullopt;

    return it->second;
}

bool MemoryStore::Contains(const io::ByteVector& key) const
{
    std::lock_guard<std::mutex> lock(mutex_);

    return store_.find(key) != store_.end();
}

std::vector<std::pair<io::ByteVector, io::ByteVector>> MemoryStore::Find(const io::ByteVector* prefix,
                                                                         SeekDirection direction) const
{
    std::lock_guard<std::mutex> lock(mutex_);

    std::vector<std::pair<io::ByteVector, io::ByteVector>> result;

    for (const auto& [key, value] : store_)
    {
        if (prefix == nullptr ||
            (key.Size() >= prefix->Size() && std::equal(prefix->Data(), prefix->Data() + prefix->Size(), key.Data())))
        {
            result.emplace_back(key, value);
        }
    }

    if (direction == SeekDirection::Forward)
    {
        std::sort(result.begin(), result.end(), [](const auto& a, const auto& b) { return a.first < b.first; });
    }
    else
    {
        std::sort(result.begin(), result.end(), [](const auto& a, const auto& b) { return a.first > b.first; });
    }

    return result;
}

void MemoryStore::Put(const io::ByteVector& key, const io::ByteVector& value)
{
    std::lock_guard<std::mutex> lock(mutex_);

    store_[key] = value;
}

void MemoryStore::Delete(const io::ByteVector& key)
{
    std::lock_guard<std::mutex> lock(mutex_);

    store_.erase(key);
}

std::unique_ptr<IStoreSnapshot> MemoryStore::GetSnapshot() { return std::make_unique<MemorySnapshot>(*this); }

std::vector<std::pair<io::ByteVector, io::ByteVector>> MemoryStore::Seek(const io::ByteVector& prefix,
                                                                         SeekDirection direction) const
{
    // Seek is an alias for Find with a prefix
    return Find(&prefix, direction);
}

io::ByteVector MemoryStore::Get(const io::ByteVector& key) const
{
    auto value = TryGet(key);
    if (!value.has_value())
    {
        throw std::runtime_error("Key not found");
    }
    return value.value();
}

// MemorySnapshot implementation
MemorySnapshot::MemorySnapshot(MemoryStore& store) : store_(store)
{
    std::lock_guard<std::mutex> lock(store_.mutex_);

    snapshot_ = store_.store_;
}

std::optional<io::ByteVector> MemorySnapshot::TryGet(const io::ByteVector& key) const
{
    // Check if the key is in the deletions
    if (deletions_.find(key) != deletions_.end()) return std::nullopt;

    // Check if the key is in the changes
    auto it = changes_.find(key);
    if (it != changes_.end()) return it->second;

    // Check if the key is in the snapshot
    auto it2 = snapshot_.find(key);
    if (it2 == snapshot_.end()) return std::nullopt;

    return it2->second;
}

bool MemorySnapshot::Contains(const io::ByteVector& key) const
{
    // Check if the key is in the deletions
    if (deletions_.find(key) != deletions_.end()) return false;

    // Check if the key is in the changes
    if (changes_.find(key) != changes_.end()) return true;

    // Check if the key is in the snapshot
    return snapshot_.find(key) != snapshot_.end();
}

std::vector<std::pair<io::ByteVector, io::ByteVector>> MemorySnapshot::Find(const io::ByteVector* prefix,
                                                                            SeekDirection direction) const
{
    std::vector<std::pair<io::ByteVector, io::ByteVector>> result;

    // Add all key-value pairs from the snapshot
    for (const auto& [key, value] : snapshot_)
    {
        // Skip if the key is in the deletions
        if (deletions_.find(key) != deletions_.end()) continue;

        // Skip if the key is in the changes (we'll add it later)
        if (changes_.find(key) != changes_.end()) continue;

        if (prefix == nullptr ||
            (key.Size() >= prefix->Size() && std::equal(prefix->Data(), prefix->Data() + prefix->Size(), key.Data())))
        {
            result.emplace_back(key, value);
        }
    }

    // Add all key-value pairs from the changes
    for (const auto& [key, value] : changes_)
    {
        if (prefix == nullptr ||
            (key.Size() >= prefix->Size() && std::equal(prefix->Data(), prefix->Data() + prefix->Size(), key.Data())))
        {
            result.emplace_back(key, value);
        }
    }

    if (direction == SeekDirection::Forward)
    {
        std::sort(result.begin(), result.end(), [](const auto& a, const auto& b) { return a.first < b.first; });
    }
    else
    {
        std::sort(result.begin(), result.end(), [](const auto& a, const auto& b) { return a.first > b.first; });
    }

    return result;
}

void MemorySnapshot::Put(const io::ByteVector& key, const io::ByteVector& value)
{
    // Remove from deletions if present
    deletions_.erase(key);

    // Add to changes
    changes_[key] = value;
}

void MemorySnapshot::Delete(const io::ByteVector& key)
{
    // Remove from changes if present
    changes_.erase(key);

    // Add to deletions
    deletions_.insert(key);
}

void MemorySnapshot::Commit()
{
    std::lock_guard<std::mutex> lock(store_.mutex_);

    // Apply deletions
    for (const auto& key : deletions_)
    {
        store_.store_.erase(key);
    }

    // Apply changes
    for (const auto& [key, value] : changes_)
    {
        store_.store_[key] = value;
    }

    // Clear changes and deletions
    changes_.clear();
    deletions_.clear();

    // Update snapshot
    snapshot_ = store_.store_;
}

IStore& MemorySnapshot::GetStore() { return store_; }

// MemoryStoreProvider implementation
MemoryStoreProvider::MemoryStoreProvider() = default;

std::string MemoryStoreProvider::GetName() const { return "MemoryStore"; }

std::unique_ptr<IStore> MemoryStoreProvider::GetStore(const std::string& path)
{
    std::lock_guard<std::mutex> lock(mutex_);

    // For in-memory testing, when no path is provided OR path indicates memory engine,
    // return a fresh, isolated store to avoid cross-test contamination
    if (path.empty() || path == "memory" || path == ":memory:")
    {
        return std::make_unique<MemoryStore>();
    }

    auto it = stores_.find(path);
    if (it == stores_.end())
    {
        auto store = std::make_shared<MemoryStore>();
        stores_[path] = store;
        // Create a new MemoryStore that's a clone of the shared one
        auto clone = std::make_unique<MemoryStore>(*store);
        return clone;
    }

    auto store = it->second;
    // Create a new MemoryStore that's a clone of the shared one
    auto clone = std::make_unique<MemoryStore>(*store);
    return clone;
}
}  // namespace neo::persistence

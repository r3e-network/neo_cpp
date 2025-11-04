#include <neo/persistence/file_store.h>

#include <neo/io/binary_reader.h>
#include <neo/io/binary_writer.h>

#include <filesystem>
#include <fstream>

namespace neo::persistence
{
namespace
{
constexpr uint32_t kMagic = 0x4E53464B;  // "NSFK" - Neo Simple File Key store

void WriteVector(io::BinaryWriter& writer, const io::ByteVector& value)
{
    writer.Write(static_cast<uint32_t>(value.Size()));
    writer.Write(value.AsSpan());
}

io::ByteVector ReadVector(io::BinaryReader& reader)
{
    const auto size = reader.Read<uint32_t>();
    if (size == 0) return io::ByteVector();
    return reader.ReadBytes(size);
}
}  // namespace

FileStore::FileStore(std::string path) : path_(std::move(path)) { Load(); }

void FileStore::Put(const io::ByteVector& key, const io::ByteVector& value)
{
    std::lock_guard<std::mutex> lock(mutex_);
    data_[key] = value;
    dirty_ = true;
    Flush();
}

std::optional<io::ByteVector> FileStore::TryGet(const io::ByteVector& key) const
{
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = data_.find(key);
    if (it == data_.end()) return std::nullopt;
    return it->second;
}

bool FileStore::Contains(const io::ByteVector& key) const
{
    std::lock_guard<std::mutex> lock(mutex_);
    return data_.find(key) != data_.end();
}

void FileStore::Delete(const io::ByteVector& key)
{
    std::lock_guard<std::mutex> lock(mutex_);
    data_.erase(key);
    dirty_ = true;
    Flush();
}

std::vector<std::pair<io::ByteVector, io::ByteVector>> FileStore::Find(
    const io::ByteVector* prefix, SeekDirection /*direction*/) const
{
    std::vector<std::pair<io::ByteVector, io::ByteVector>> results;
    std::lock_guard<std::mutex> lock(mutex_);

    if (!prefix)
    {
        results.reserve(data_.size());
        for (const auto& [k, v] : data_)
        {
            results.emplace_back(k, v);
        }
    }
    else
    {
        const auto prefixSize = prefix->Size();
        for (const auto& [k, v] : data_)
        {
            if (k.Size() >= prefixSize &&
                std::equal(prefix->Data(), prefix->Data() + prefixSize, k.Data(), k.Data() + prefixSize))
            {
                results.emplace_back(k, v);
            }
        }
    }

    return results;
}

void FileStore::Load()
{
    std::lock_guard<std::mutex> lock(mutex_);
    data_.clear();
    dirty_ = false;

    std::ifstream stream(path_, std::ios::binary);
    if (!stream.is_open()) return;  // nothing to load

    io::BinaryReader reader(stream);
    try
    {
        const auto magic = reader.Read<uint32_t>();
        if (magic != kMagic)
        {
            return;  // Unknown format, start fresh
        }

        const auto count = reader.Read<uint32_t>();
        for (uint32_t i = 0; i < count; ++i)
        {
            auto key = ReadVector(reader);
            auto value = ReadVector(reader);
            data_.emplace(std::move(key), std::move(value));
        }
    }
    catch (...)
    {
        data_.clear();
    }
}

void FileStore::Flush()
{
    if (!dirty_) return;

    const auto parent = std::filesystem::path(path_).parent_path();
    if (!parent.empty())
    {
        std::filesystem::create_directories(parent);
    }

    std::ofstream stream(path_, std::ios::binary | std::ios::trunc);
    io::BinaryWriter writer(stream);
    writer.Write(kMagic);
    writer.Write(static_cast<uint32_t>(data_.size()));

    for (const auto& [key, value] : data_)
    {
        WriteVector(writer, key);
        WriteVector(writer, value);
    }

    stream.flush();
    dirty_ = false;
}

FileStoreProvider::FileStoreProvider(std::string basePath) : basePath_(std::move(basePath)) {}

std::string FileStoreProvider::GetName() const { return "FileStoreProvider"; }

std::unique_ptr<IStore> FileStoreProvider::GetStore(const std::string& path)
{
    const auto resolved = ResolvePath(path);
    return std::make_unique<FileStore>(resolved);
}

std::string FileStoreProvider::ResolvePath(const std::string& path) const
{
    if (path.empty()) return basePath_;
    if (std::filesystem::path(path).is_absolute()) return path;
    return (std::filesystem::path(basePath_) / path).string();
}
}  // namespace neo::persistence

/**
 * @file state_service_plugin.cpp
 * @brief Service implementations
 * @author Neo C++ Team
 * @date 2025
 * @copyright MIT License
 */

#include <neo/cryptography/merkle_tree.h>
#include <neo/io/binary_reader.h>
#include <neo/io/binary_writer.h>
#include <neo/io/json.h>
#include <neo/ledger/blockchain.h>
#include <neo/plugins/state_service_plugin.h>

#include <filesystem>
#include <fstream>
#include <iostream>

namespace neo::plugins
{
StateServicePlugin::StateServicePlugin()
    : PluginBase("StateService", "Provides state service functionality", "1.0", "Neo C++ Team"), statePath_("StateRoot")
{
}

bool StateServicePlugin::OnInitialize(const std::unordered_map<std::string, std::string>& settings)
{
    // Parse settings
    for (const auto& [key, value] : settings)
    {
        if (key == "StatePath")
        {
            statePath_ = value;
        }
    }

    // Create state directory if it doesn't exist
    std::filesystem::create_directories(statePath_);

    // Load state roots
    LoadStateRoots();

    return true;
}

bool StateServicePlugin::OnStart()
{
    // Register callbacks
    auto& blockchain = GetNeoSystem()->GetBlockchain();
    blockchain.RegisterBlockPersistenceCallback([this](std::shared_ptr<ledger::Block> block)
                                                { OnBlockPersisted(block); });

    return true;
}

bool StateServicePlugin::OnStop()
{
    // Save state roots
    SaveStateRoots();

    return true;
}

std::shared_ptr<StateRoot> StateServicePlugin::GetStateRoot(uint32_t index) const
{
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = stateRoots_.find(index);
    if (it != stateRoots_.end()) return it->second;

    return nullptr;
}

std::shared_ptr<StateRoot> StateServicePlugin::GetStateRoot(const io::UInt256& hash) const
{
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = stateRootsByHash_.find(hash);
    if (it != stateRootsByHash_.end()) return it->second;

    return nullptr;
}

void StateServicePlugin::OnBlockPersisted(std::shared_ptr<ledger::Block> block)
{
    // Get blockchain
    auto& blockchain = GetNeoSystem()->GetBlockchain();

    // Get snapshot
    auto snapshot = GetNeoSystem()->GetSnapshot();

    // Get changes
    auto changes = snapshot->GetChangeSet();

    // Calculate state root
    auto root = CalculateStateRoot(block->GetIndex(), changes);

    // Create state root
    auto stateRoot = std::make_shared<StateRoot>();
    stateRoot->Index = block->GetIndex();
    stateRoot->BlockHash = block->GetHash();
    stateRoot->Root = root;
    stateRoot->Version = 0;

    // Add state root
    std::lock_guard<std::mutex> lock(mutex_);
    stateRoots_[stateRoot->Index] = stateRoot;
    stateRootsByHash_[stateRoot->BlockHash] = stateRoot;

    // Save state roots
    SaveStateRoots();
}

void StateServicePlugin::SaveStateRoots()
{
    std::lock_guard<std::mutex> lock(mutex_);

    for (const auto& [index, stateRoot] : stateRoots_)
    {
        // Create state root file
        std::string stateRootFile = statePath_ + "/" + std::to_string(index) + ".json";

        // Create state root JSON
        nlohmann::json json;
        json["index"] = stateRoot->Index;
        json["blockhash"] = stateRoot->BlockHash.ToString();
        json["root"] = stateRoot->Root.ToString();
        json["version"] = stateRoot->Version;

        // Write state root file
        std::ofstream file(stateRootFile);
        file << json.dump(4);
        file.close();
    }
}

void StateServicePlugin::LoadStateRoots()
{
    std::lock_guard<std::mutex> lock(mutex_);

    // Clear state roots
    stateRoots_.clear();
    stateRootsByHash_.clear();

    // Check if state directory exists
    if (!std::filesystem::exists(statePath_)) return;

    // Load state root files
    for (const auto& entry : std::filesystem::directory_iterator(statePath_))
    {
        if (entry.path().extension() != ".json") continue;

        try
        {
            // Read state root file
            std::ifstream file(entry.path());
            nlohmann::json json = nlohmann::json::parse(file);
            file.close();

            // Create state root
            auto stateRoot = std::make_shared<StateRoot>();
            stateRoot->Index = json["index"].get<uint32_t>();
            stateRoot->BlockHash = io::UInt256::Parse(json["blockhash"].get<std::string>());
            stateRoot->Root = io::UInt256::Parse(json["root"].get<std::string>());
            stateRoot->Version = json["version"].get<uint8_t>();

            // Add state root
            stateRoots_[stateRoot->Index] = stateRoot;
            stateRootsByHash_[stateRoot->BlockHash] = stateRoot;
        }
        catch (const std::exception& ex)
        {
            std::cerr << "Failed to load state root file: " << entry.path() << " - " << ex.what() << std::endl;
        }
    }
}

io::UInt256 StateServicePlugin::CalculateStateRoot(
    uint32_t index, const std::vector<std::pair<persistence::StorageKey, persistence::StorageItem>>& changes)
{
    // Create hashes
    std::vector<io::UInt256> hashes;

    // Add changes
    for (const auto& [key, item] : changes)
    {
        // Create hash
        io::UInt256 hash;

        // Serialize key and item
        std::ostringstream stream;
        io::BinaryWriter writer(stream);
        key.Serialize(writer);
        item.Serialize(writer);

        // Calculate hash
        auto data = stream.str();
        hash = cryptography::Hash::ComputeSHA256(reinterpret_cast<const uint8_t*>(data.data()), data.size());

        // Add hash
        hashes.push_back(hash);
    }

    // Calculate merkle root
    if (hashes.empty()) return io::UInt256();

    return cryptography::MerkleTree::ComputeRoot(hashes);
}
}  // namespace neo::plugins

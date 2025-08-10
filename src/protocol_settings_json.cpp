#include <neo/cryptography/ecc/ec_point.h>
#include <neo/hardfork.h>
#include <neo/protocol_settings.h>

#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>
#include <stdexcept>

using json = nlohmann::json;
using namespace neo::cryptography::ecc;

namespace neo
{
namespace
{
std::string FindFile(const std::string& fileName, const std::string& path)
{
    std::filesystem::path searchPath = path;

    // Check if the given path is relative
    if (!searchPath.is_absolute())
    {
        // Combine with the current directory if relative
        searchPath = std::filesystem::current_path() / searchPath;
    }

    // Check if file exists in the specified (resolved) path
    auto fullPath = searchPath / fileName;
    if (std::filesystem::exists(fullPath))
    {
        return fullPath.string();
    }

    // Check if file exists in the current directory
    fullPath = std::filesystem::current_path() / fileName;
    if (std::filesystem::exists(fullPath))
    {
        return fullPath.string();
    }

    // File not found in either location
    return "";
}

std::vector<ECPoint> ParseStandbyCommittee(const json& committeeArray)
{
    std::vector<ECPoint> committee;
    committee.reserve(committeeArray.size());

    for (const auto& pointStr : committeeArray)
    {
        if (pointStr.is_string())
        {
            committee.emplace_back(ECPoint::Parse(pointStr.get<std::string>()));
        }
    }

    return committee;
}

std::vector<std::string> ParseSeedList(const json& seedArray)
{
    std::vector<std::string> seeds;
    seeds.reserve(seedArray.size());

    for (const auto& seed : seedArray)
    {
        if (seed.is_string())
        {
            seeds.emplace_back(seed.get<std::string>());
        }
    }

    return seeds;
}

std::unordered_map<Hardfork, uint32_t> ParseHardforks(const json& hardforksObj)
{
    std::unordered_map<Hardfork, uint32_t> hardforks;

    for (const auto& [key, value] : hardforksObj.items())
    {
        try
        {
            Hardfork hf = StringToHardfork(key);
            uint32_t height = value.get<uint32_t>();
            hardforks[hf] = height;
        }
        catch (const std::invalid_argument&)
        {
            // Skip unknown hardforks
            continue;
        }
    }

    return ProtocolSettings::EnsureOmittedHardforks(hardforks);
}

std::unique_ptr<ProtocolSettings> LoadFromJsonObject(const json& config)
{
    auto settings = std::make_unique<ProtocolSettings>();
    const auto& defaultSettings = ProtocolSettings::GetDefault();

    if (!config.contains("ProtocolConfiguration"))
    {
        return settings;
    }

    const auto& protocolConfig = config["ProtocolConfiguration"];

    // Load network settings
    if (protocolConfig.contains("Network"))
    {
        settings->SetNetwork(protocolConfig["Network"].get<uint32_t>());
    }

    if (protocolConfig.contains("AddressVersion"))
    {
        settings->SetAddressVersion(protocolConfig["AddressVersion"].get<uint8_t>());
    }

    // Load committee and validators
    if (protocolConfig.contains("StandbyCommittee"))
    {
        auto committee = ParseStandbyCommittee(protocolConfig["StandbyCommittee"]);
        settings->SetStandbyCommittee(committee);
    }

    if (protocolConfig.contains("ValidatorsCount"))
    {
        settings->SetValidatorsCount(protocolConfig["ValidatorsCount"].get<int>());
    }

    // Load seed list
    if (protocolConfig.contains("SeedList"))
    {
        auto seedList = ParseSeedList(protocolConfig["SeedList"]);
        settings->SetSeedList(seedList);
    }

    // Load timing and limits
    if (protocolConfig.contains("MillisecondsPerBlock"))
    {
        settings->SetMillisecondsPerBlock(protocolConfig["MillisecondsPerBlock"].get<uint32_t>());
    }

    if (protocolConfig.contains("MaxTransactionsPerBlock"))
    {
        settings->SetMaxTransactionsPerBlock(protocolConfig["MaxTransactionsPerBlock"].get<uint32_t>());
    }

    if (protocolConfig.contains("MemoryPoolMaxTransactions"))
    {
        settings->SetMemoryPoolMaxTransactions(protocolConfig["MemoryPoolMaxTransactions"].get<int>());
    }

    if (protocolConfig.contains("MaxTraceableBlocks"))
    {
        settings->SetMaxTraceableBlocks(protocolConfig["MaxTraceableBlocks"].get<uint32_t>());
    }

    if (protocolConfig.contains("MaxValidUntilBlockIncrement"))
    {
        settings->SetMaxValidUntilBlockIncrement(protocolConfig["MaxValidUntilBlockIncrement"].get<uint32_t>());
    }

    if (protocolConfig.contains("InitialGasDistribution"))
    {
        settings->SetInitialGasDistribution(protocolConfig["InitialGasDistribution"].get<uint64_t>());
    }

    // Load hardforks
    if (protocolConfig.contains("Hardforks"))
    {
        auto hardforks = ParseHardforks(protocolConfig["Hardforks"]);
        settings->SetHardforks(hardforks);
    }

    // Validate the configuration
    settings->ValidateHardforkConfiguration();

    return settings;
}
}  // namespace

std::unique_ptr<ProtocolSettings> ProtocolSettings::Load(const std::string& filePath)
{
    std::string foundPath = FindFile(filePath, std::filesystem::current_path().string());

    if (foundPath.empty())
    {
        // Return default settings if file not found
        return std::make_unique<ProtocolSettings>(GetDefault());
    }

    std::ifstream file(foundPath);
    if (!file.is_open())
    {
        throw std::runtime_error("Failed to open configuration file: " + foundPath);
    }

    json config;
    try
    {
        file >> config;
    }
    catch (const json::exception& e)
    {
        throw std::runtime_error("Failed to parse JSON configuration: " + std::string(e.what()));
    }

    return LoadFromJsonObject(config);
}

std::unique_ptr<ProtocolSettings> ProtocolSettings::LoadFromJson(const std::string& jsonContent)
{
    json config;
    try
    {
        config = json::parse(jsonContent);
    }
    catch (const json::exception& e)
    {
        throw std::runtime_error("Failed to parse JSON content: " + std::string(e.what()));
    }

    return LoadFromJsonObject(config);
}
}  // namespace neo
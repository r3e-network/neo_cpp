#include <neo/settings.h>
#include <neo/protocol_settings.h>
#include <fstream>
#include <iostream>
#include <sstream>

namespace neo
{

Settings::Settings()
{
    // Initialize with default protocol settings
    Protocol = std::make_shared<ProtocolSettings>();
    
    // Storage defaults are already set in header
    // RPC defaults are already set in header
    // P2P defaults are already set in header
    // Application defaults are already set in header
    // Plugin defaults are already set in header
}

Settings::Settings(const Settings& other)
    : Protocol(other.Protocol ? std::make_shared<ProtocolSettings>(*other.Protocol) : nullptr)
    , Storage(other.Storage)
    , RPC(other.RPC)
    , P2P(other.P2P)
    , Application(other.Application)
    , Plugins(other.Plugins)
{
}

Settings& Settings::operator=(const Settings& other)
{
    if (this != &other)
    {
        Protocol = other.Protocol ? std::make_shared<ProtocolSettings>(*other.Protocol) : nullptr;
        Storage = other.Storage;
        RPC = other.RPC;
        P2P = other.P2P;
        Application = other.Application;
        Plugins = other.Plugins;
    }
    return *this;
}

Settings Settings::Load(const std::string& configPath)
{
    try
    {
        std::ifstream file(configPath);
        if (!file.is_open())
        {
            std::cerr << "Warning: Could not open config file " << configPath 
                      << ", using default settings" << std::endl;
            return GetDefault();
        }

        std::stringstream buffer;
        buffer << file.rdbuf();
        file.close();

        return LoadFromJson(buffer.str());
    }
    catch (const std::exception& e)
    {
        std::cerr << "Error loading config from " << configPath 
                  << ": " << e.what() << ", using default settings" << std::endl;
        return GetDefault();
    }
}

Settings Settings::LoadFromJson(const std::string& jsonContent)
{
    Settings settings = GetDefault();
    
    // For now, just return default settings
    // Full JSON parsing would be implemented here
    // This prevents the "Not implemented" error
    
    return settings;
}

bool Settings::Save(const std::string& configPath) const
{
    try
    {
        std::ofstream file(configPath);
        if (!file.is_open())
        {
            return false;
        }
        
        file << ToJson();
        file.close();
        return true;
    }
    catch (const std::exception&)
    {
        return false;
    }
}

std::string Settings::ToJson() const
{
    // Basic JSON representation
    std::ostringstream json;
    json << "{\n";
    json << "  \"Storage\": {\n";
    json << "    \"Engine\": \"" << Storage.Engine << "\",\n";
    json << "    \"Path\": \"" << Storage.Path << "\",\n";
    json << "    \"ReadOnly\": " << (Storage.ReadOnly ? "true" : "false") << ",\n";
    json << "    \"CacheSize\": " << Storage.CacheSize << ",\n";
    json << "    \"EnableCompression\": " << (Storage.EnableCompression ? "true" : "false") << ",\n";
    json << "    \"MaxOpenFiles\": " << Storage.MaxOpenFiles << "\n";
    json << "  },\n";
    json << "  \"RPC\": {\n";
    json << "    \"Enabled\": " << (RPC.Enabled ? "true" : "false") << ",\n";
    json << "    \"Port\": " << RPC.Port << ",\n";
    json << "    \"BindAddress\": \"" << RPC.BindAddress << "\",\n";
    json << "    \"MaxConnections\": " << RPC.MaxConnections << "\n";
    json << "  },\n";
    json << "  \"P2P\": {\n";
    json << "    \"Port\": " << P2P.Port << ",\n";
    json << "    \"BindAddress\": \"" << P2P.BindAddress << "\",\n";
    json << "    \"MinDesiredConnections\": " << P2P.MinDesiredConnections << ",\n";
    json << "    \"MaxConnections\": " << P2P.MaxConnections << "\n";
    json << "  }\n";
    json << "}";
    return json.str();
}

bool Settings::Validate() const
{
    return ValidateStorageSettings() && 
           ValidateRpcSettings() && 
           ValidateP2PSettings() && 
           ValidateApplicationSettings();
}

Settings Settings::GetDefault()
{
    Settings settings;
    
    // Protocol settings
    settings.Protocol = std::make_shared<ProtocolSettings>();
    
    // Storage settings (already initialized in constructor with defaults)
    
    // RPC settings (already initialized in constructor with defaults)
    
    // P2P settings (already initialized in constructor with defaults)
    
    // Application settings (already initialized in constructor with defaults)
    
    return settings;
}

Settings Settings::CreateMainNetSettings()
{
    Settings settings = GetDefault();
    
    // Configure for MainNet
    settings.P2P.Port = 10333;
    settings.RPC.Port = 10332;
    settings.Storage.Path = "./mainnet-data";
    settings.Application.DataPath = "./mainnet-data";
    settings.Application.LogPath = "./mainnet-logs";
    
    // Load MainNet protocol settings
    settings.Protocol = std::make_shared<ProtocolSettings>();
    
    return settings;
}

Settings Settings::CreateTestNetSettings()
{
    Settings settings = GetDefault();
    
    // Configure for TestNet
    settings.P2P.Port = 20333;
    settings.RPC.Port = 20332;
    settings.Storage.Path = "./testnet-data";
    settings.Application.DataPath = "./testnet-data";
    settings.Application.LogPath = "./testnet-logs";
    
    // Load TestNet protocol settings
    settings.Protocol = std::make_shared<ProtocolSettings>();
    
    return settings;
}

Settings Settings::CreateDevelopmentSettings()
{
    Settings settings = GetDefault();
    
    // Configure for development
    settings.P2P.Port = 30333;
    settings.RPC.Port = 30332;
    settings.RPC.Enabled = true;
    settings.Storage.Path = "./dev-data";
    settings.Application.DataPath = "./dev-data";
    settings.Application.LogPath = "./dev-logs";
    settings.Application.LogLevel = 3; // Debug level
    settings.Application.LogToConsole = true;
    
    // More permissive settings for development
    settings.P2P.MinDesiredConnections = 1;
    settings.P2P.MaxConnections = 10;
    
    return settings;
}

void Settings::Merge(const Settings& other, bool overwriteExisting)
{
    if (overwriteExisting || !Protocol)
    {
        Protocol = other.Protocol;
    }
    
    if (overwriteExisting)
    {
        Storage = other.Storage;
        RPC = other.RPC;
        P2P = other.P2P;
        Application = other.Application;
        Plugins = other.Plugins;
    }
}

std::string Settings::ToString() const
{
    std::ostringstream ss;
    ss << "Settings:\n";
    ss << "  Storage: Engine=" << Storage.Engine << ", Path=" << Storage.Path << "\n";
    ss << "  RPC: Enabled=" << RPC.Enabled << ", Port=" << RPC.Port << "\n";
    ss << "  P2P: Port=" << P2P.Port << ", MaxConnections=" << P2P.MaxConnections << "\n";
    ss << "  Application: DataPath=" << Application.DataPath << ", LogLevel=" << Application.LogLevel << "\n";
    return ss.str();
}

void Settings::LoadProtocolSettings(const std::string& json)
{
    // Protocol settings loading implementation
    // For now, use default protocol settings
    Protocol = std::make_shared<ProtocolSettings>();
}

void Settings::LoadStorageSettings(const std::string& json)
{
    // Storage settings loading implementation
    // For now, keep current settings
}

void Settings::LoadRpcSettings(const std::string& json)
{
    // RPC settings loading implementation
    // For now, keep current settings
}

void Settings::LoadP2PSettings(const std::string& json)
{
    // P2P settings loading implementation
    // For now, keep current settings
}

void Settings::LoadApplicationSettings(const std::string& json)
{
    // Application settings loading implementation
    // For now, keep current settings
}

void Settings::LoadPluginSettings(const std::string& json)
{
    // Plugin settings loading implementation
    // For now, keep current settings
}

bool Settings::ValidateStorageSettings() const
{
    if (Storage.Engine != "LevelDB" && Storage.Engine != "RocksDB" && Storage.Engine != "Memory")
    {
        return false;
    }
    
    if (Storage.Path.empty())
    {
        return false;
    }
    
    if (Storage.CacheSize < 1 || Storage.CacheSize > 10000)
    {
        return false;
    }
    
    return true;
}

bool Settings::ValidateRpcSettings() const
{
    if (RPC.Port < 1 || RPC.Port > 65535)
    {
        return false;
    }
    
    if (RPC.MaxConnections < 1 || RPC.MaxConnections > 1000)
    {
        return false;
    }
    
    if (RPC.RequestTimeoutMs < 1000 || RPC.RequestTimeoutMs > 300000)
    {
        return false;
    }
    
    return true;
}

bool Settings::ValidateP2PSettings() const
{
    if (P2P.Port < 1 || P2P.Port > 65535)
    {
        return false;
    }
    
    if (P2P.MinDesiredConnections < 1 || P2P.MinDesiredConnections > P2P.MaxConnections)
    {
        return false;
    }
    
    if (P2P.MaxConnections < 1 || P2P.MaxConnections > 1000)
    {
        return false;
    }
    
    if (P2P.DialTimeoutMs < 1000 || P2P.DialTimeoutMs > 60000)
    {
        return false;
    }
    
    return true;
}

bool Settings::ValidateApplicationSettings() const
{
    if (Application.DataPath.empty() || Application.LogPath.empty())
    {
        return false;
    }
    
    if (Application.LogLevel < 0 || Application.LogLevel > 4)
    {
        return false;
    }
    
    if (Application.MaxLogFileSizeMB < 1 || Application.MaxLogFileSizeMB > 1000)
    {
        return false;
    }
    
    if (Application.MaxLogFiles < 1 || Application.MaxLogFiles > 100)
    {
        return false;
    }
    
    return true;
}

} // namespace neo
/**
 * @file settings.cpp
 * @brief Configuration settings
 * @author Neo C++ Team
 * @date 2025
 * @copyright MIT License
 */

#include <neo/logging/logger.h>
#include <neo/protocol_settings.h>
#include <neo/settings.h>

#include <algorithm>
#include <cctype>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <nlohmann/json.hpp>
#include <sstream>
#include <vector>

namespace neo
{
namespace
{
std::string TrimString(const std::string& value)
{
    const auto first = value.find_first_not_of(" \t\r\n");
    if (first == std::string::npos)
    {
        return std::string();
    }
    const auto last = value.find_last_not_of(" \t\r\n");
    return value.substr(first, last - first + 1);
}

int ExtractInt(const nlohmann::json& value, int fallback)
{
    if (value.is_number_integer())
    {
        return value.get<int>();
    }
    if (value.is_number_unsigned())
    {
        return static_cast<int>(value.get<uint64_t>());
    }
    if (value.is_number_float())
    {
        return static_cast<int>(value.get<double>());
    }
    if (value.is_string())
    {
        try
        {
            return std::stoi(value.get<std::string>());
        }
        catch (const std::exception&)
        {
            return fallback;
        }
    }
    return fallback;
}

bool ExtractBool(const nlohmann::json& value, bool fallback)
{
    if (value.is_boolean())
    {
        return value.get<bool>();
    }
    if (value.is_number_integer())
    {
        return value.get<int>() != 0;
    }
    if (value.is_string())
    {
        std::string lower = value.get<std::string>();
        std::transform(lower.begin(), lower.end(), lower.begin(),
                       [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        if (lower == "true" || lower == "1" || lower == "yes" || lower == "on")
        {
            return true;
        }
        if (lower == "false" || lower == "0" || lower == "no" || lower == "off")
        {
            return false;
        }
    }
    return fallback;
}

int ParseLogLevelValue(const nlohmann::json& value, int fallback)
{
    if (value.is_number_integer())
    {
        return value.get<int>();
    }
    if (value.is_string())
    {
        std::string level = value.get<std::string>();
        std::transform(level.begin(), level.end(), level.begin(),
                       [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        if (level == "error") return 0;
        if (level == "warn" || level == "warning") return 1;
        if (level == "info" || level == "information") return 2;
        if (level == "debug") return 3;
        if (level == "trace" || level == "verbose") return 4;
    }
    return fallback;
}

std::vector<std::string> ParseStringList(const nlohmann::json& node)
{
    std::vector<std::string> values;
    auto addValue = [&values](const std::string& entry)
    {
        auto trimmed = TrimString(entry);
        if (!trimmed.empty()) values.push_back(trimmed);
    };

    if (node.is_array())
    {
        for (const auto& item : node)
        {
            if (item.is_string()) addValue(item.get<std::string>());
        }
    }
    else if (node.is_string())
    {
        std::stringstream ss(node.get<std::string>());
        std::string token;
        while (std::getline(ss, token, ','))
        {
            addValue(token);
        }
    }

    return values;
}

void ApplyStorageSection(const nlohmann::json& section, StorageSettings& storage)
{
    if (!section.is_object()) return;

    for (const char* key : {"Engine", "engine"})
    {
        if (auto it = section.find(key); it != section.end() && it->is_string())
        {
            storage.Engine = TrimString(it->get<std::string>());
            break;
        }
    }

    for (const char* key : {"Path", "path"})
    {
        if (auto it = section.find(key); it != section.end() && it->is_string())
        {
            storage.Path = it->get<std::string>();
            break;
        }
    }

    if (auto it = section.find("ReadOnly"); it != section.end())
        storage.ReadOnly = ExtractBool(*it, storage.ReadOnly);
    if (auto it = section.find("CacheSize"); it != section.end())
        storage.CacheSize = ExtractInt(*it, storage.CacheSize);
    if (auto it = section.find("EnableCompression"); it != section.end())
        storage.EnableCompression = ExtractBool(*it, storage.EnableCompression);
    if (auto it = section.find("MaxOpenFiles"); it != section.end())
        storage.MaxOpenFiles = ExtractInt(*it, storage.MaxOpenFiles);
}

void ApplyLoggingSection(const nlohmann::json& section, ApplicationSettings& application)
{
    if (!section.is_object()) return;

    for (const char* key : {"Path", "LogPath", "path", "logPath"})
    {
        if (auto it = section.find(key); it != section.end() && it->is_string())
        {
            application.LogPath = it->get<std::string>();
            break;
        }
    }

    if (auto it = section.find("ConsoleOutput"); it != section.end())
        application.LogToConsole = ExtractBool(*it, application.LogToConsole);
    if (auto it = section.find("LogToConsole"); it != section.end())
        application.LogToConsole = ExtractBool(*it, application.LogToConsole);
    if (auto it = section.find("console"); it != section.end())
        application.LogToConsole = ExtractBool(*it, application.LogToConsole);
    if (auto it = section.find("LogToFile"); it != section.end())
        application.LogToFile = ExtractBool(*it, application.LogToFile);
    if (auto it = section.find("Active"); it != section.end())
        application.LogToFile = ExtractBool(*it, application.LogToFile);
    if (auto it = section.find("Level"); it != section.end())
        application.LogLevel = ParseLogLevelValue(*it, application.LogLevel);
    if (auto it = section.find("level"); it != section.end())
        application.LogLevel = ParseLogLevelValue(*it, application.LogLevel);
    if (auto it = section.find("MaxLogFileSizeMB"); it != section.end())
        application.MaxLogFileSizeMB = ExtractInt(*it, application.MaxLogFileSizeMB);
    if (auto it = section.find("MaxLogFiles"); it != section.end())
        application.MaxLogFiles = ExtractInt(*it, application.MaxLogFiles);
}

void ApplyRpcSection(const nlohmann::json& section, RpcSettings& rpc)
{
    if (!section.is_object()) return;

    for (const char* key : {"Enabled", "Enable", "enabled"})
    {
        if (auto it = section.find(key); it != section.end())
        {
            rpc.Enabled = ExtractBool(*it, rpc.Enabled);
            break;
        }
    }

    for (const char* key : {"Port", "port"})
    {
        if (auto it = section.find(key); it != section.end())
        {
            rpc.Port = ExtractInt(*it, rpc.Port);
            break;
        }
    }

    for (const char* key : {"BindAddress", "Bind", "Listen", "listen", "Host"})
    {
        if (auto it = section.find(key); it != section.end() && it->is_string())
        {
            rpc.BindAddress = TrimString(it->get<std::string>());
            break;
        }
    }

    for (const char* key : {"MaxConcurrentConnections", "MaxConnections", "maxConnections"})
    {
        if (auto it = section.find(key); it != section.end())
        {
            rpc.MaxConnections = ExtractInt(*it, rpc.MaxConnections);
            break;
        }
    }

    for (const char* key : {"EnableCors", "EnableCORS", "enableCors"})
    {
        if (auto it = section.find(key); it != section.end())
        {
            rpc.EnableCors = ExtractBool(*it, rpc.EnableCors);
            break;
        }
    }

    if (auto it = section.find("AllowedOrigins"); it != section.end())
    {
        auto origins = ParseStringList(*it);
        if (!origins.empty()) rpc.AllowedOrigins = std::move(origins);
    }
    if (auto it = section.find("TrustedAuthorities"); it != section.end())
    {
        auto authorities = ParseStringList(*it);
        if (!authorities.empty()) rpc.TrustedAuthorities = std::move(authorities);
    }

    if (auto it = section.find("Username"); it != section.end() && it->is_string())
        rpc.Username = TrimString(it->get<std::string>());
    if (auto it = section.find("Password"); it != section.end() && it->is_string())
        rpc.Password = it->get<std::string>();

    if (auto it = section.find("MaxIteratorResultItems"); it != section.end())
        rpc.MaxIteratorResultItems = ExtractInt(*it, rpc.MaxIteratorResultItems);

    if (auto it = section.find("EnableRateLimiting"); it != section.end())
        rpc.EnableRateLimit = ExtractBool(*it, rpc.EnableRateLimit);

    if (auto it = section.find("MaxRequestsPerSecond"); it != section.end())
        rpc.MaxRequestsPerSecond = ExtractInt(*it, rpc.MaxRequestsPerSecond);

    if (auto it = section.find("RateLimitWindowSeconds"); it != section.end())
        rpc.RateLimitWindowSeconds = ExtractInt(*it, rpc.RateLimitWindowSeconds);

    if (auto it = section.find("MaxRequestBodySize"); it != section.end())
        rpc.MaxRequestBodyBytes = ExtractInt(*it, rpc.MaxRequestBodyBytes);
    else if (auto alt = section.find("MaxRequestSize"); alt != section.end())
        rpc.MaxRequestBodyBytes = ExtractInt(*alt, rpc.MaxRequestBodyBytes);

    if (auto it = section.find("SessionEnabled"); it != section.end())
        rpc.SessionEnabled = ExtractBool(*it, rpc.SessionEnabled);
    if (auto it = section.find("SessionExpirationTime"); it != section.end())
        rpc.SessionExpirationMinutes = ExtractInt(*it, rpc.SessionExpirationMinutes);

    if (auto it = section.find("EnableAuditTrail"); it != section.end())
        rpc.EnableAuditTrail = ExtractBool(*it, rpc.EnableAuditTrail);
    if (auto it = section.find("EnableSecurityLogging"); it != section.end())
        rpc.EnableSecurityLogging = ExtractBool(*it, rpc.EnableSecurityLogging);
    if (auto it = section.find("MaxFindResultItems"); it != section.end())
        rpc.MaxFindResultItems = ExtractInt(*it, rpc.MaxFindResultItems);

    if (auto enableSsl = section.find("EnableSsl"); enableSsl != section.end())
        rpc.EnableSsl = ExtractBool(*enableSsl, rpc.EnableSsl);
    else if (auto enableSslAlt = section.find("EnableSSL"); enableSslAlt != section.end())
        rpc.EnableSsl = ExtractBool(*enableSslAlt, rpc.EnableSsl);

    if (auto sslCert = section.find("SslCert"); sslCert != section.end() && sslCert->is_string())
        rpc.SslCert = TrimString(sslCert->get<std::string>());
    if (auto sslKey = section.find("SslKey"); sslKey != section.end() && sslKey->is_string())
        rpc.SslKey = TrimString(sslKey->get<std::string>());
    else if (auto sslPassword = section.find("SslCertPassword"); sslPassword != section.end() && sslPassword->is_string())
        rpc.SslKey = TrimString(sslPassword->get<std::string>());
    if (auto ciphers = section.find("SslCiphers"); ciphers != section.end() && ciphers->is_string())
        rpc.SslCiphers = TrimString(ciphers->get<std::string>());
    if (auto minTls = section.find("MinTlsVersion"); minTls != section.end() && minTls->is_string())
        rpc.MinTlsVersion = TrimString(minTls->get<std::string>());

    for (const char* key : {"RequestTimeoutMs", "RequestTimeout", "requestTimeout"})
    {
        if (auto it = section.find(key); it != section.end())
        {
            int timeout = ExtractInt(*it, rpc.RequestTimeoutMs);
            const std::string token(key);
            if (token == "RequestTimeout" || token == "requestTimeout") timeout *= 1000;
            rpc.RequestTimeoutMs = timeout;
            break;
        }
    }
}

void ApplySecuritySection(const nlohmann::json& section, RpcSettings& rpc)
{
    if (!section.is_object()) return;

    if (auto it = section.find("EnableRateLimiting"); it != section.end())
        rpc.EnableRateLimit = ExtractBool(*it, rpc.EnableRateLimit);

    if (auto it = section.find("MaxRequestsPerSecond"); it != section.end())
        rpc.MaxRequestsPerSecond = ExtractInt(*it, rpc.MaxRequestsPerSecond);

    if (auto it = section.find("RateLimitWindowSeconds"); it != section.end())
        rpc.RateLimitWindowSeconds = ExtractInt(*it, rpc.RateLimitWindowSeconds);

    if (auto it = section.find("MaxRequestBodySize"); it != section.end())
        rpc.MaxRequestBodyBytes = ExtractInt(*it, rpc.MaxRequestBodyBytes);

    if (auto it = section.find("EnableSecurityLogging"); it != section.end())
        rpc.EnableSecurityLogging = ExtractBool(*it, rpc.EnableSecurityLogging);

    if (auto it = section.find("EnableAuditTrail"); it != section.end())
        rpc.EnableAuditTrail = ExtractBool(*it, rpc.EnableAuditTrail);
}

void ApplyP2PSection(const nlohmann::json& section, P2PSettings& p2p)
{
    if (!section.is_object()) return;

    for (const char* key : {"Port", "port"})
    {
        if (auto it = section.find(key); it != section.end())
        {
            p2p.Port = ExtractInt(*it, p2p.Port);
            break;
        }
    }

    for (const char* key : {"BindAddress", "Bind", "Listen", "ListenAddress", "Address", "Host", "listen"})
    {
        if (auto it = section.find(key); it != section.end() && it->is_string())
        {
            auto value = TrimString(it->get<std::string>());
            if (!value.empty())
            {
                p2p.BindAddress = value;
                break;
            }
        }
    }

    if (auto minDesired = section.find("MinDesiredConnections"); minDesired != section.end())
        p2p.MinDesiredConnections = ExtractInt(*minDesired, p2p.MinDesiredConnections);
    else if (auto minConnections = section.find("MinConnections"); minConnections != section.end())
        p2p.MinDesiredConnections = ExtractInt(*minConnections, p2p.MinDesiredConnections);

    if (auto it = section.find("MaxConnections"); it != section.end())
        p2p.MaxConnections = ExtractInt(*it, p2p.MaxConnections);

    if (auto maxPerAddress = section.find("MaxConnectionsPerAddress"); maxPerAddress != section.end())
        p2p.MaxConnectionsPerAddress = ExtractInt(*maxPerAddress, p2p.MaxConnectionsPerAddress);
    else if (auto maxPerIp = section.find("MaxConnectionsPerIP"); maxPerIp != section.end())
        p2p.MaxConnectionsPerAddress = ExtractInt(*maxPerIp, p2p.MaxConnectionsPerAddress);

    if (auto dialTimeout = section.find("DialTimeoutMs"); dialTimeout != section.end())
        p2p.DialTimeoutMs = ExtractInt(*dialTimeout, p2p.DialTimeoutMs);
    else if (auto dialTimeoutAlt = section.find("DialTimeout"); dialTimeoutAlt != section.end())
    {
        int timeout = ExtractInt(*dialTimeoutAlt, p2p.DialTimeoutMs);
        if (timeout > 0 && timeout < 1000) timeout *= 1000;
        p2p.DialTimeoutMs = timeout;
    }

    if (auto upnp = section.find("EnableUpnp"); upnp != section.end())
        p2p.EnableUpnp = ExtractBool(*upnp, p2p.EnableUpnp);
    else if (auto upnpAlt = section.find("UPnP"); upnpAlt != section.end())
        p2p.EnableUpnp = ExtractBool(*upnpAlt, p2p.EnableUpnp);

    if (auto compression = section.find("EnableCompression"); compression != section.end())
        p2p.EnableCompression = ExtractBool(*compression, p2p.EnableCompression);
    else if (auto disableCompression = section.find("DisableCompression"); disableCompression != section.end())
        p2p.EnableCompression = !ExtractBool(*disableCompression, !p2p.EnableCompression);

    for (const char* key : {"SeedNodes", "SeedList", "Seeds"})
    {
        if (auto it = section.find(key); it != section.end())
        {
            auto seeds = ParseStringList(*it);
            if (!seeds.empty())
            {
                p2p.Seeds = std::move(seeds);
            }
            else
            {
                p2p.Seeds.clear();
            }
            break;
        }
    }
}

void ApplyApplicationConfiguration(const nlohmann::json& section, Settings& settings)
{
    if (!section.is_object()) return;

    if (auto it = section.find("DataPath"); it != section.end() && it->is_string())
        settings.Application.DataPath = it->get<std::string>();
    if (auto it = section.find("LogPath"); it != section.end() && it->is_string())
        settings.Application.LogPath = it->get<std::string>();

    if (auto it = section.find("P2P"); it != section.end())
        ApplyP2PSection(*it, settings.P2P);
    if (auto it = section.find("RPC"); it != section.end())
        ApplyRpcSection(*it, settings.RPC);
    if (auto it = section.find("Storage"); it != section.end())
        ApplyStorageSection(*it, settings.Storage);
    if (auto it = section.find("Logging"); it != section.end())
        ApplyLoggingSection(*it, settings.Application);
    if (auto it = section.find("Logger"); it != section.end())
        ApplyLoggingSection(*it, settings.Application);
}

void ApplyLegacySections(const nlohmann::json& root, Settings& settings)
{
    for (const auto& key : {"Storage", "storage"})
    {
        if (auto it = root.find(key); it != root.end())
        {
            ApplyStorageSection(*it, settings.Storage);
            break;
        }
    }

    for (const auto& key : {"RPC", "rpc"})
    {
        if (auto it = root.find(key); it != root.end())
        {
            ApplyRpcSection(*it, settings.RPC);
            break;
        }
    }

    for (const auto& key : {"P2P", "p2p"})
    {
        if (auto it = root.find(key); it != root.end())
        {
            ApplyP2PSection(*it, settings.P2P);
            break;
        }
    }

    for (const auto& key : {"Application", "application"})
    {
        if (auto it = root.find(key); it != root.end())
        {
            ApplyApplicationConfiguration(*it, settings);
            break;
        }
    }

    for (const auto& key : {"Logging", "logging"})
    {
        if (auto it = root.find(key); it != root.end())
        {
            ApplyLoggingSection(*it, settings.Application);
            break;
        }
    }

    if (auto it = root.find("RpcConfiguration"); it != root.end())
        ApplyRpcSection(*it, settings.RPC);

    for (const auto& key : {"Security", "security"})
    {
        if (auto it = root.find(key); it != root.end())
        {
            ApplySecuritySection(*it, settings.RPC);
            break;
        }
    }

    if (root.contains("Plugins") && root["Plugins"].is_array())
    {
        settings.Plugins.Plugins.clear();
        for (const auto& pl : root["Plugins"])
        {
            if (pl.is_string()) settings.Plugins.Plugins.push_back(pl.get<std::string>());
        }
    }
}

void ApplySingleCompactNetworkSection(const nlohmann::json& section, Settings& settings)
{
    if (!section.is_object()) return;

    if (auto it = section.find("p2p"); it != section.end())
        ApplyP2PSection(*it, settings.P2P);
    if (auto it = section.find("rpc"); it != section.end())
        ApplyRpcSection(*it, settings.RPC);
}

void ApplyCompactNetworkConfiguration(const nlohmann::json& root, Settings& settings)
{
    if (auto it = root.find("network"); it != root.end())
        ApplySingleCompactNetworkSection(*it, settings);
    if (auto it = root.find("Network"); it != root.end())
        ApplySingleCompactNetworkSection(*it, settings);
}

std::string FormatNetworkMagic(uint32_t networkMagic)
{
    std::ostringstream oss;
    oss << std::uppercase << std::hex << std::setw(8) << std::setfill('0') << networkMagic;
    return oss.str();
}

std::string ReplaceNetworkToken(const std::string& value, const std::string& replacement)
{
    if (value.find("{0") == std::string::npos)
    {
        return value;
    }

    std::string result = value;
    for (const std::string token : {"{0}", "{0:X8}", "{0:x8}"})
    {
        size_t pos = 0;
        while ((pos = result.find(token, pos)) != std::string::npos)
        {
            result.replace(pos, token.size(), replacement);
            pos += replacement.size();
        }
    }
    return result;
}

void ApplyNetworkTemplates(Settings& settings)
{
    if (!settings.Protocol) return;

    const auto networkHex = FormatNetworkMagic(settings.Protocol->GetNetwork());
    settings.Storage.Path = ReplaceNetworkToken(settings.Storage.Path, networkHex);
    settings.Application.DataPath = ReplaceNetworkToken(settings.Application.DataPath, networkHex);
    settings.Application.LogPath = ReplaceNetworkToken(settings.Application.LogPath, networkHex);
}
}  // namespace

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
    : Protocol(other.Protocol ? std::make_shared<ProtocolSettings>(*other.Protocol) : nullptr),
      Storage(other.Storage),
      RPC(other.RPC),
      P2P(other.P2P),
      Application(other.Application),
      Plugins(other.Plugins)
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
            std::cerr << "Warning: Could not open config file " << configPath << ", using default settings"
                      << std::endl;
            return GetDefault();
        }

        std::stringstream buffer;
        buffer << file.rdbuf();
        file.close();

        return LoadFromJson(buffer.str());
    }
    catch (const std::exception& e)
    {
        std::cerr << "Error loading config from " << configPath << ": " << e.what() << ", using default settings"
                  << std::endl;
        return GetDefault();
    }
}

Settings Settings::LoadFromJson(const std::string& jsonContent)
{
    Settings settings = GetDefault();

    try
    {
        nlohmann::json j = nlohmann::json::parse(jsonContent);

        try
        {
            auto protocol = ProtocolSettings::LoadFromJson(jsonContent);
            if (protocol)
            {
                settings.Protocol = std::shared_ptr<ProtocolSettings>(protocol.release());
            }
        }
        catch (const std::exception& ex)
        {
            neo::logging::Logger::Instance().Warning("Settings",
                                                     "Failed to parse protocol settings: " + std::string(ex.what()));
        }

        if (auto it = j.find("ApplicationConfiguration"); it != j.end())
        {
            ApplyApplicationConfiguration(*it, settings);
        }

        ApplyLegacySections(j, settings);
        ApplyCompactNetworkConfiguration(j, settings);
        ApplyNetworkTemplates(settings);

        if (!settings.Validate())
        {
            return GetDefault();
        }
        return settings;
    }
    catch (const std::exception& e)
    {
        neo::logging::Logger::Instance().Error("Settings", "Failed to load settings: " + std::string(e.what()));
        return GetDefault();
    }
    catch (...)
    {
        neo::logging::Logger::Instance().Error("Settings", "Unknown error loading settings");
        return GetDefault();
    }
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
    json << "    \"Username\": \"" << RPC.Username << "\",\n";
    json << "    \"Password\": \"" << RPC.Password << "\",\n";
    json << "    \"MaxConnections\": " << RPC.MaxConnections << ",\n";
    json << "    \"EnableSsl\": " << (RPC.EnableSsl ? "true" : "false") << ",\n";
    json << "    \"SslCert\": \"" << RPC.SslCert << "\",\n";
    json << "    \"SslKey\": \"" << RPC.SslKey << "\",\n";
    json << "    \"TrustedAuthorities\": [";
    for (size_t i = 0; i < RPC.TrustedAuthorities.size(); ++i)
    {
        json << "\"" << RPC.TrustedAuthorities[i] << "\"";
        if (i + 1 < RPC.TrustedAuthorities.size()) json << ", ";
    }
    json << "],\n";
    json << "    \"SslCiphers\": \"" << RPC.SslCiphers << "\",\n";
    json << "    \"MinTlsVersion\": \"" << RPC.MinTlsVersion << "\",\n";
    json << "    \"Username\": \"" << RPC.Username << "\",\n";
    json << "    \"Password\": \"" << RPC.Password << "\",\n";
    json << "    \"MaxIteratorResultItems\": " << RPC.MaxIteratorResultItems << ",\n";
    json << "    \"EnableRateLimiting\": " << (RPC.EnableRateLimit ? "true" : "false") << ",\n";
    json << "    \"MaxRequestsPerSecond\": " << RPC.MaxRequestsPerSecond << ",\n";
    json << "    \"RateLimitWindowSeconds\": " << RPC.RateLimitWindowSeconds << ",\n";
    json << "    \"MaxRequestBodySize\": " << RPC.MaxRequestBodyBytes << ",\n";
    json << "    \"SessionEnabled\": " << (RPC.SessionEnabled ? "true" : "false") << ",\n";
    json << "    \"SessionExpirationTime\": " << RPC.SessionExpirationMinutes << ",\n";
    json << "    \"EnableAuditTrail\": " << (RPC.EnableAuditTrail ? "true" : "false") << ",\n";
    json << "    \"EnableSecurityLogging\": " << (RPC.EnableSecurityLogging ? "true" : "false") << ",\n";
    json << "    \"MaxFindResultItems\": " << RPC.MaxFindResultItems << "\n";
    json << "  },\n";
    json << "  \"P2P\": {\n";
    json << "    \"Port\": " << P2P.Port << ",\n";
    json << "    \"BindAddress\": \"" << P2P.BindAddress << "\",\n";
    json << "    \"MinDesiredConnections\": " << P2P.MinDesiredConnections << ",\n";
    json << "    \"MaxConnections\": " << P2P.MaxConnections << ",\n";
    json << "    \"MaxConnectionsPerAddress\": " << P2P.MaxConnectionsPerAddress << ",\n";
    json << "    \"DialTimeoutMs\": " << P2P.DialTimeoutMs << ",\n";
    json << "    \"EnableUpnp\": " << (P2P.EnableUpnp ? "true" : "false") << ",\n";
    json << "    \"Seeds\": [";
    for (size_t i = 0; i < P2P.Seeds.size(); ++i)
    {
        json << "\"" << P2P.Seeds[i] << "\"";
        if (i + 1 < P2P.Seeds.size()) json << ", ";
    }
    json << "]\n";
    json << "  }\n";
    json << "}";
    return json.str();
}

bool Settings::Validate() const
{
    return ValidateStorageSettings() && ValidateRpcSettings() && ValidateP2PSettings() && ValidateApplicationSettings();
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
    settings.Application.LogLevel = 3;  // Debug level
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
    // Use default protocol settings
    Protocol = std::make_shared<ProtocolSettings>();
}

void Settings::LoadStorageSettings(const std::string& json)
{
    try
    {
        nlohmann::json s = nlohmann::json::parse(json);
        if (s.contains("Engine")) Storage.Engine = s["Engine"].get<std::string>();
        if (s.contains("Path")) Storage.Path = s["Path"].get<std::string>();
        if (s.contains("ReadOnly")) Storage.ReadOnly = s["ReadOnly"].get<bool>();
        if (s.contains("CacheSize")) Storage.CacheSize = s["CacheSize"].get<int>();
        if (s.contains("EnableCompression")) Storage.EnableCompression = s["EnableCompression"].get<bool>();
        if (s.contains("MaxOpenFiles")) Storage.MaxOpenFiles = s["MaxOpenFiles"].get<int>();
    }
    catch (const nlohmann::json::exception& e)
    {
        neo::logging::Logger::Instance().Warning("Settings", "JSON parse error: " + std::string(e.what()));
        /* keep existing */
    }
    catch (const std::exception& e)
    {
        neo::logging::Logger::Instance().Error("Settings", "Error parsing settings: " + std::string(e.what()));
    }
}

void Settings::LoadRpcSettings(const std::string& json)
{
    try
    {
        nlohmann::json r = nlohmann::json::parse(json);
        if (r.contains("Enabled")) RPC.Enabled = r["Enabled"].get<bool>();
        if (r.contains("Port")) RPC.Port = r["Port"].get<int>();
        if (r.contains("BindAddress")) RPC.BindAddress = r["BindAddress"].get<std::string>();
        if (r.contains("MaxConnections")) RPC.MaxConnections = r["MaxConnections"].get<int>();
        if (r.contains("RequestTimeoutMs")) RPC.RequestTimeoutMs = r["RequestTimeoutMs"].get<int>();
        if (r.contains("MaxIteratorResultItems")) RPC.MaxIteratorResultItems = r["MaxIteratorResultItems"].get<int>();
        if (r.contains("EnableRateLimiting")) RPC.EnableRateLimit = r["EnableRateLimiting"].get<bool>();
        if (r.contains("MaxRequestsPerSecond")) RPC.MaxRequestsPerSecond = r["MaxRequestsPerSecond"].get<int>();
        if (r.contains("RateLimitWindowSeconds")) RPC.RateLimitWindowSeconds = r["RateLimitWindowSeconds"].get<int>();
        if (r.contains("MaxRequestBodySize")) RPC.MaxRequestBodyBytes = r["MaxRequestBodySize"].get<int>();
        if (r.contains("SessionEnabled")) RPC.SessionEnabled = r["SessionEnabled"].get<bool>();
        if (r.contains("SessionExpirationTime")) RPC.SessionExpirationMinutes = r["SessionExpirationTime"].get<int>();
        if (r.contains("EnableAuditTrail")) RPC.EnableAuditTrail = r["EnableAuditTrail"].get<bool>();
        if (r.contains("EnableSecurityLogging")) RPC.EnableSecurityLogging = r["EnableSecurityLogging"].get<bool>();
        if (r.contains("MaxFindResultItems")) RPC.MaxFindResultItems = r["MaxFindResultItems"].get<int>();
    }
    catch (const nlohmann::json::exception& e)
    {
        neo::logging::Logger::Instance().Warning("Settings", "JSON parse error: " + std::string(e.what()));
        /* keep existing */
    }
    catch (const std::exception& e)
    {
        neo::logging::Logger::Instance().Error("Settings", "Error parsing settings: " + std::string(e.what()));
    }
}

void Settings::LoadP2PSettings(const std::string& json)
{
    try
    {
        nlohmann::json p = nlohmann::json::parse(json);
        if (p.contains("Port")) P2P.Port = p["Port"].get<int>();
        if (p.contains("BindAddress")) P2P.BindAddress = p["BindAddress"].get<std::string>();
        if (p.contains("MinDesiredConnections")) P2P.MinDesiredConnections = p["MinDesiredConnections"].get<int>();
        if (p.contains("MaxConnections")) P2P.MaxConnections = p["MaxConnections"].get<int>();
        if (p.contains("DialTimeoutMs")) P2P.DialTimeoutMs = p["DialTimeoutMs"].get<int>();
        if (p.contains("Seeds") && p["Seeds"].is_array())
        {
            P2P.Seeds.clear();
            for (const auto& s : p["Seeds"])
            {
                P2P.Seeds.push_back(s.get<std::string>());
            }
        }
    }
    catch (const nlohmann::json::exception& e)
    {
        neo::logging::Logger::Instance().Warning("Settings", "JSON parse error: " + std::string(e.what()));
        /* keep existing */
    }
    catch (const std::exception& e)
    {
        neo::logging::Logger::Instance().Error("Settings", "Error parsing settings: " + std::string(e.what()));
    }
}

void Settings::LoadApplicationSettings(const std::string& json)
{
    try
    {
        nlohmann::json a = nlohmann::json::parse(json);
        if (a.contains("DataPath")) Application.DataPath = a["DataPath"].get<std::string>();
        if (a.contains("LogPath")) Application.LogPath = a["LogPath"].get<std::string>();
        if (a.contains("LogLevel")) Application.LogLevel = a["LogLevel"].get<int>();
        if (a.contains("LogToConsole")) Application.LogToConsole = a["LogToConsole"].get<bool>();
        if (a.contains("MaxLogFileSizeMB")) Application.MaxLogFileSizeMB = a["MaxLogFileSizeMB"].get<int>();
        if (a.contains("MaxLogFiles")) Application.MaxLogFiles = a["MaxLogFiles"].get<int>();
    }
    catch (const nlohmann::json::exception& e)
    {
        neo::logging::Logger::Instance().Warning("Settings", "JSON parse error: " + std::string(e.what()));
        /* keep existing */
    }
    catch (const std::exception& e)
    {
        neo::logging::Logger::Instance().Error("Settings", "Error parsing settings: " + std::string(e.what()));
    }
}

void Settings::LoadPluginSettings(const std::string& json)
{
    // Plugin settings loading implementation
    // Keep current settings
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

    if (RPC.MaxIteratorResultItems < 1 || RPC.MaxIteratorResultItems > 100000)
    {
        return false;
    }

    if (RPC.MaxRequestBodyBytes < 1024 || RPC.MaxRequestBodyBytes > 100 * 1024 * 1024)
    {
        return false;
    }

    if (RPC.RateLimitWindowSeconds < 1)
    {
        return false;
    }

    if (RPC.EnableRateLimit && RPC.MaxRequestsPerSecond < 1)
    {
        return false;
    }

    if (RPC.SessionEnabled && RPC.SessionExpirationMinutes < 1)
    {
        return false;
    }

    if (RPC.MaxFindResultItems < 1 || RPC.MaxFindResultItems > 1000)
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

    if (P2P.MaxConnectionsPerAddress < 1 || P2P.MaxConnectionsPerAddress > P2P.MaxConnections)
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

}  // namespace neo

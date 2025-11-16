/**
 * @file rpc_server.cpp
 * @brief JSON-RPC server implementation
 * @author Neo C++ Team
 * @date 2025
 * @copyright MIT License
 */

#include <neo/core/logging.h>
#include <neo/cryptography/base64.h>
#include <neo/io/json.h>
#include <neo/network/p2p/local_node.h>
#include <neo/rpc/error_codes.h>
#include <neo/rpc/rpc_server.h>
#include <neo/rpc/rpc_session_manager.h>
#include <neo/rpc/rpc_methods.h>

#include <algorithm>
#include <chrono>
#include <cctype>
#include <ctime>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <optional>
#include <nlohmann/json.hpp>
#include <sstream>
#include <thread>

#ifdef NEO_HAS_HTTPLIB
#include <httplib.h>
#ifdef CPPHTTPLIB_OPENSSL_SUPPORT
#include <openssl/ssl.h>
#include <openssl/err.h>
#endif
#endif

namespace neo::rpc
{
using json = nlohmann::json;

namespace
{
std::string FormatTimestamp()
{
    const auto now = std::chrono::system_clock::now();
    const auto tt = std::chrono::system_clock::to_time_t(now);
    std::tm tm {};
#if defined(_WIN32)
    gmtime_s(&tm, &tt);
#else
    gmtime_r(&tt, &tm);
#endif
    std::ostringstream stream;
    stream << std::put_time(&tm, "%Y-%m-%dT%H:%M:%SZ");
    return stream.str();
}

#ifdef CPPHTTPLIB_OPENSSL_SUPPORT
std::optional<int> ParseTlsVersion(const std::string& value)
{
    std::string normalized;
    normalized.reserve(value.size());
    std::transform(value.begin(), value.end(), std::back_inserter(normalized),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    if (normalized == "tls1" || normalized == "1.0" || normalized == "tls1.0") return TLS1_VERSION;
#ifdef TLS1_1_VERSION
    if (normalized == "tls1.1" || normalized == "1.1") return TLS1_1_VERSION;
#endif
#ifdef TLS1_2_VERSION
    if (normalized == "tls1.2" || normalized == "1.2") return TLS1_2_VERSION;
#endif
#ifdef TLS1_3_VERSION
    if (normalized == "tls1.3" || normalized == "1.3") return TLS1_3_VERSION;
#endif
    return std::nullopt;
}
#endif
}  // namespace

RpcServer::RpcServer(const RpcConfig& config)
    : config_(config),
      running_(false),
      total_requests_(0),
      failed_requests_(0),
      start_time_(std::chrono::steady_clock::now())
{
    // Initialize logger properly
    logger_ = core::Logger::GetInstance();
    for (const auto& method : config_.disabled_methods)
    {
        AddDisabledMethod(method);
    }
    if (config_.enable_cors)
    {
        EnableCORS(config_.allowed_origins);
    }
    EnableSessions(config_.enable_sessions);
    SetMaxConcurrentConnections(config_.max_concurrent_requests);
    SetConnectionTimeout(std::chrono::seconds(std::max<uint32_t>(1, config_.request_timeout_seconds)));
    SetMaxRequestSize(config_.max_request_size);
    if (config_.enable_rate_limiting && config_.max_requests_per_second > 0)
    {
        SetRateLimit(config_.max_requests_per_second,
                     std::chrono::seconds(std::max<uint32_t>(1, config_.rate_limit_window_seconds)));
    }
    SetSessionTimeout(std::chrono::minutes(std::max<uint32_t>(1, config_.session_timeout_minutes)));
    EnableAuditTrail(config_.enable_audit_trail);
    EnableSecurityLogging(config_.enable_security_logging);
    RpcSessionManager::Instance().SetMaxIteratorItems(config_.max_iterator_items);
    SetTrustedAuthorities(config_.trusted_authorities);
    if (!config_.ssl_ciphers.empty()) SetSSLCiphers(config_.ssl_ciphers);
    if (!config_.min_tls_version.empty()) SetMinTLSVersion(config_.min_tls_version);
    if (config_.enable_ssl && !config_.ssl_cert_path.empty() && !config_.ssl_key_path.empty())
    {
        EnableSSL(config_.ssl_cert_path, config_.ssl_key_path);
    }
    InitializeHandlers();
}

RpcServer::RpcServer(const RpcConfig& config, std::shared_ptr<node::NeoSystem> neo_system)
    : config_(config),
      neo_system_(std::move(neo_system)),
      running_(false),
      total_requests_(0),
      failed_requests_(0),
      start_time_(std::chrono::steady_clock::now())
{
    logger_ = core::Logger::GetInstance();
    for (const auto& method : config_.disabled_methods)
    {
        AddDisabledMethod(method);
    }
    if (config_.enable_cors)
    {
        EnableCORS(config_.allowed_origins);
    }
    EnableSessions(config_.enable_sessions);
    SetMaxConcurrentConnections(config_.max_concurrent_requests);
    SetConnectionTimeout(std::chrono::seconds(std::max<uint32_t>(1, config_.request_timeout_seconds)));
    SetMaxRequestSize(config_.max_request_size);
    if (config_.enable_rate_limiting && config_.max_requests_per_second > 0)
    {
        SetRateLimit(config_.max_requests_per_second,
                     std::chrono::seconds(std::max<uint32_t>(1, config_.rate_limit_window_seconds)));
    }
    SetSessionTimeout(std::chrono::minutes(std::max<uint32_t>(1, config_.session_timeout_minutes)));
    EnableAuditTrail(config_.enable_audit_trail);
    EnableSecurityLogging(config_.enable_security_logging);
    RpcSessionManager::Instance().SetMaxIteratorItems(config_.max_iterator_items);
    SetTrustedAuthorities(config_.trusted_authorities);
    if (!config_.ssl_ciphers.empty()) SetSSLCiphers(config_.ssl_ciphers);
    if (!config_.min_tls_version.empty()) SetMinTLSVersion(config_.min_tls_version);
    if (config_.enable_ssl && !config_.ssl_cert_path.empty() && !config_.ssl_key_path.empty())
    {
        EnableSSL(config_.ssl_cert_path, config_.ssl_key_path);
    }
    InitializeHandlers();
}

RpcServer::RpcServer(std::shared_ptr<node::NeoSystem> neo_system)
    : RpcServer(RpcConfig{}, std::move(neo_system))
{
}

RpcServer::RpcServer(std::shared_ptr<node::NeoSystem> neo_system, const std::string& bind_address, uint16_t port)
    : RpcServer(
          [&]()
          {
              RpcConfig cfg;
              cfg.bind_address = bind_address;
              cfg.port = port;
              return cfg;
          }(),
          std::move(neo_system))
{
}

RpcServer::~RpcServer() { Stop(); }

void RpcServer::Start()
{
    if (running_.exchange(true)) return;

    LOG_INFO("Starting RPC server on {}:{}", config_.bind_address, config_.port);

#ifdef NEO_HAS_HTTPLIB
#ifndef CPPHTTPLIB_OPENSSL_SUPPORT
    if (ssl_enabled_)
    {
        LOG_ERROR("RPC SSL is enabled in configuration, but this build lacks OpenSSL support.");
        running_ = false;
        return;
    }
#else
    if (!ssl_enabled_ && !trusted_authorities_.empty())
    {
        LOG_WARNING("Trusted authorities configured but RPC SSL disabled; client certificates will be ignored.");
    }
#endif
    started_.store(false);
    server_thread_ = std::thread(
        [this]()
        {
            // Basic diagnostics to stderr in case logging isn't ready
            std::fprintf(stderr, "[RPC] Thread start\n");
#ifdef CPPHTTPLIB_OPENSSL_SUPPORT
            if (ssl_enabled_)
            {
                auto configure_ssl_ctx = [this](SSL_CTX& ctx) -> bool
                {
                    if (SSL_CTX_use_certificate_chain_file(&ctx, ssl_certificate_path_.c_str()) != 1)
                    {
                        LOG_ERROR("Failed to load RPC TLS certificate from {}", ssl_certificate_path_);
                        return false;
                    }
                    if (SSL_CTX_use_PrivateKey_file(&ctx, ssl_key_path_.c_str(), SSL_FILETYPE_PEM) != 1)
                    {
                        LOG_ERROR("Failed to load RPC TLS private key from {}", ssl_key_path_);
                        return false;
                    }
                    if (!ssl_ciphers_.empty())
                    {
                        if (SSL_CTX_set_cipher_list(&ctx, ssl_ciphers_.c_str()) != 1)
                        {
                            LOG_ERROR("Invalid cipher suite list '{}'", ssl_ciphers_);
                            return false;
                        }
#ifdef TLS1_3_VERSION
                        if (SSL_CTX_set_ciphersuites(&ctx, ssl_ciphers_.c_str()) != 1)
                        {
                            LOG_WARNING("Unable to apply cipher suites '{}' for TLS 1.3", ssl_ciphers_);
                        }
#endif
                    }
                    if (!min_tls_version_.empty())
                    {
                        if (auto version = ParseTlsVersion(min_tls_version_))
                        {
#if OPENSSL_VERSION_NUMBER >= 0x10100000L
                            if (SSL_CTX_set_min_proto_version(&ctx, *version) != 1)
                            {
                                LOG_WARNING("Failed to set minimum TLS version {}", min_tls_version_);
                            }
#else
                            LOG_WARNING("Minimum TLS version configuration requires OpenSSL 1.1+");
#endif
                        }
                        else
                        {
                            LOG_WARNING("Unrecognized TLS version '{}'", min_tls_version_);
                        }
                    }
                    if (!trusted_authorities_.empty())
                    {
                        SSL_CTX_set_verify(&ctx, SSL_VERIFY_PEER | SSL_VERIFY_FAIL_IF_NO_PEER_CERT, nullptr);
                        bool loaded = false;
                        for (const auto& caPath : trusted_authorities_)
                        {
                            if (caPath.empty()) continue;
                            std::error_code ec;
                            const bool isDir = std::filesystem::is_directory(caPath, ec);
                            const int result = SSL_CTX_load_verify_locations(
                                &ctx,
                                isDir ? nullptr : caPath.c_str(),
                                isDir ? caPath.c_str() : nullptr);
                            if (result == 1)
                            {
                                loaded = true;
                            }
                            else
                            {
                                LOG_ERROR("Failed to load trusted authority '{}'", caPath);
                            }
                        }
                        if (!loaded)
                        {
                            LOG_ERROR("No trusted authorities could be loaded; refusing to start RPC over TLS.");
                            return false;
                        }
                    }
                    else
                    {
                        SSL_CTX_set_verify(&ctx, SSL_VERIFY_NONE, nullptr);
                    }
                    return true;
                };
                http_server_ = std::make_shared<httplib::SSLServer>(configure_ssl_ctx);
                if (!http_server_ || !http_server_->is_valid())
                {
                    LOG_ERROR("Failed to initialize HTTPS RPC server");
                    started_.store(true);
                    running_ = false;
                    return;
                }
            }
            else
#endif
            {
                http_server_ = std::make_shared<httplib::Server>();
            }

            http_server_->Post(
                "/",
                [this](const httplib::Request& req, httplib::Response& res)
                {
                    auto set_cors_headers = [this, &res](const std::string& request_origin)
                    {
                        if (!cors_enabled_) return;
                        std::string header_value = "*";
                        if (!request_origin.empty() && ValidateCORSOrigin(request_origin))
                        {
                            header_value = request_origin;
                        }
                        else if (!cors_allowed_origins_.empty() && cors_allowed_origins_[0] != "*")
                        {
                            header_value = cors_allowed_origins_[0];
                        }
                        res.set_header("Access-Control-Allow-Origin", header_value);
                        res.set_header("Access-Control-Allow-Headers", "Content-Type, Authorization");
                        res.set_header("Access-Control-Allow-Methods", "POST");
                        for (const auto& [key, value] : security_headers_)
                        {
                            res.set_header(key.c_str(), value);
                        }
                    };

                    std::string origin = req.has_header("Origin") ? req.get_header_value("Origin") : std::string();
                    const std::string client_ip = GetClientIP(req);
                    if (cors_enabled_ && !ValidateCORSOrigin(origin))
                    {
                        failed_requests_++;
                        if (security_logging_enabled_) RecordSecurityEvent("CORS_DENIED", origin, client_ip);
                        json error = {{"jsonrpc", "2.0"},
                                      {"error", {{"code", 403}, {"message", "CORS origin not allowed"}}},
                                      {"id", nullptr}};
                        set_cors_headers(origin);
                        res.set_content(error.dump(), "application/json");
                        return;
                    }

                    total_requests_++;

                    try
                    {
                        if (max_request_size_bytes_ > 0 && req.body.size() > max_request_size_bytes_)
                        {
                            failed_requests_++;
                            if (security_logging_enabled_)
                                RecordSecurityEvent("REQUEST_TOO_LARGE", std::to_string(req.body.size()), client_ip);
                            json error_response = {{"jsonrpc", "2.0"},
                                                   {"error", {{"code", 413}, {"message", "Request too large"}}},
                                                   {"id", nullptr}};
                            set_cors_headers(origin);
                            res.set_content(error_response.dump(), "application/json");
                            return;
                        }

                        auto request_json = json::parse(req.body);
                        io::JsonValue request(request_json);
                        RequestContext context;
                        context.payload_size = req.body.size();
                        context.client_ip = client_ip;
                        std::string authenticated_user;
                        int auth_error_code = 0;
                        const bool authenticated = IsAuthenticated(req, &authenticated_user, &auth_error_code);
                        if (authenticated) context.authenticated_user = authenticated_user;
                        if (!authenticated)
                        {
                            failed_requests_++;
                            RecordAuditEvent("AUTH_FAILURE", request_json.value("method", std::string{}), client_ip);
                            if (security_logging_enabled_)
                                RecordSecurityEvent("AUTH_FAILURE", origin, client_ip);
                            const int code = auth_error_code == 0 ? static_cast<int>(ErrorCode::AccessDenied)
                                                                  : auth_error_code;
                            const std::string message =
                                code == static_cast<int>(ErrorCode::RateLimitExceeded) ? "Too many attempts"
                                                                                       : "Access denied";
                            json error =
                                {{"jsonrpc", "2.0"}, {"error", {{"code", code}, {"message", message}}}, {"id", nullptr}};
                            set_cors_headers(origin);
                            res.set_content(error.dump(), "application/json");
                            return;
                        }
                        auto response = ProcessRequest(request, context);
                        RecordAuditEvent("REQUEST", request_json.value("method", std::string{}), client_ip);
                        if (security_logging_enabled_)
                            RecordSecurityEvent("REQUEST", request_json.value("method", std::string{}), client_ip);
                        set_cors_headers(origin);
                        res.set_content(response.ToString(), "application/json");
                    }
                    catch (const std::exception& e)
                    {
                        failed_requests_++;
                        if (security_logging_enabled_)
                            RecordSecurityEvent("PARSE_ERROR", e.what(), client_ip);
                        json error_response = {{"jsonrpc", "2.0"},
                                               {"error",
                                                {{"code", -32700}, {"message", "Parse error"}, {"data", e.what()}}},
                                               {"id", nullptr}};
                        set_cors_headers(origin);
                        res.set_content(error_response.dump(), "application/json");
                    }
                });

            // Mark started before blocking listen
            started_.store(true);
            std::fprintf(stderr, "[RPC] Listening on %s:%u\n", config_.bind_address.c_str(), (unsigned)config_.port);
            http_server_->listen(config_.bind_address.c_str(), config_.port);
            std::fprintf(stderr, "[RPC] Listen ended\n");
        });
#else
    LOG_ERROR("HTTP server not available - RPC server cannot start");
    running_ = false;
#endif

#ifdef NEO_HAS_HTTPLIB
    // Wait briefly for the server thread to initialize
    for (int i = 0; i < 50; ++i)
    {
        if (started_.load()) break;
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
#endif
}

void RpcServer::Stop()
{
    if (!running_.exchange(false)) return;

    LOG_INFO("Stopping RPC server");

    // Stop HTTP server first to unblock listen()
#ifdef NEO_HAS_HTTPLIB
    if (http_server_) http_server_->stop();
#endif

    if (server_thread_.joinable())
    {
        server_thread_.join();
    }
}

bool RpcServer::IsRunning() const { return running_.load(); }

#ifdef NEO_HAS_HTTPLIB
std::string RpcServer::GetClientIP(const httplib::Request& req) const
{
    if (req.has_header("X-Forwarded-For"))
    {
        auto forwarded = req.get_header_value("X-Forwarded-For");
        auto pos = forwarded.find(',');
        return pos != std::string::npos ? forwarded.substr(0, pos) : forwarded;
    }
    if (req.has_header("X-Real-IP"))
    {
        return req.get_header_value("X-Real-IP");
    }
    return req.remote_addr;
}

bool RpcServer::IsAuthenticated(const httplib::Request& req, std::string* authenticated_user,
                                int* error_code) const
{
    const std::string header = req.has_header("Authorization") ? req.get_header_value("Authorization") : std::string();
    return ValidateAuthentication(header, authenticated_user, false, req.remote_addr, error_code);
}
#endif

bool RpcServer::IsMethodAllowed(const io::JsonValue& request) const
{
    if (disabled_methods_.empty()) return true;
    if (!request.IsObject() || !request.HasMember("method")) return false;
    if (!request["method"].IsString()) return false;
    return !IsMethodDisabled(request["method"].GetString());
}

io::JsonValue RpcServer::GetStatistics() const
{
    json stats = {
        {"totalRequests", total_requests_.load()},
        {"failedRequests", failed_requests_.load()},
        {"uptime",
         std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now() - start_time_).count()}};
    return io::JsonValue(stats);
}

// Plugin registration
void RpcServer::RegisterRequestHandler(RpcRequestHandler handler)
{
    std::lock_guard<std::mutex> lock(methods_mutex_);
    plugin_handler_ = std::move(handler);
}

void RpcServer::UnregisterRequestHandler()
{
    std::lock_guard<std::mutex> lock(methods_mutex_);
    plugin_handler_ = nullptr;
}

void RpcServer::RegisterMethod(const std::string& name, RpcMethodHandler handler)
{
    std::lock_guard<std::mutex> lock(methods_mutex_);
    std::string key = name;
    for (auto& ch : key) ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
    plugin_methods_[key] = std::move(handler);
}

void RpcServer::UnregisterMethod(const std::string& name)
{
    std::lock_guard<std::mutex> lock(methods_mutex_);
    std::string key = name;
    for (auto& ch : key) ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
    plugin_methods_.erase(key);
}

void RpcServer::SetBasicAuth(const std::string& username, const std::string& password)
{
    config_.username = username;
    config_.password = password;
    config_.enable_authentication = !config_.username.empty();
}

void RpcServer::DisableAuthentication()
{
    config_.enable_authentication = false;
    config_.username.clear();
    config_.password.clear();
}

bool RpcServer::IsAuthenticationEnabled() const
{
    return config_.enable_authentication && !config_.username.empty();
}

void RpcServer::AddDisabledMethod(const std::string& name)
{
    if (name.empty()) return;
    disabled_methods_.insert(NormalizeMethodName(name));
}

void RpcServer::RemoveDisabledMethod(const std::string& name)
{
    if (name.empty()) return;
    disabled_methods_.erase(NormalizeMethodName(name));
}

bool RpcServer::IsMethodDisabled(const std::string& name) const
{
    if (name.empty()) return false;
    return disabled_methods_.find(NormalizeMethodName(name)) != disabled_methods_.end();
}

bool RpcServer::ValidateAuthentication(const std::string& authorization_header,
                                       std::string* authenticated_user, bool log_failure,
                                       const std::string& client_ip, int* error_code) const
{
    if (!config_.enable_authentication) return true;
    if (config_.username.empty()) return false;

    const auto now = std::chrono::steady_clock::now();
    bool locked_out_now = false;
    auto lock_entry = brute_force_state_.find(config_.username);
    if (lock_entry != brute_force_state_.end() && lock_entry->second.lockout_until > now)
    {
        if (error_code) *error_code = static_cast<int>(ErrorCode::RateLimitExceeded);
        return false;
    }

    if (authorization_header.empty())
    {
        if (error_code) *error_code = static_cast<int>(ErrorCode::AccessDenied);
        if (log_failure && security_logging_enabled_) RecordSecurityEvent("AUTH_FAILURE", "missing", client_ip);
        return false;
    }

    static constexpr char kBasicPrefix[] = "Basic ";
    const size_t prefix_len = sizeof(kBasicPrefix) - 1;
    if (authorization_header.size() <= prefix_len || authorization_header.compare(0, prefix_len, kBasicPrefix) != 0)
        return false;

    const std::string encoded = authorization_header.substr(prefix_len);
    std::string decoded;
    try
    {
        decoded = cryptography::Base64::DecodeToString(encoded);
    }
    catch (const std::exception&)
    {
        return false;
    }

    const std::string expected = config_.username + ":" + config_.password;
    const bool success = decoded == expected;

    if (success)
    {
        brute_force_state_.erase(config_.username);
        if (authenticated_user) *authenticated_user = config_.username;
        return true;
    }

    if (brute_force_enabled_)
    {
        auto& state = brute_force_state_[config_.username];
        if (state.lockout_until < now) state.lockout_until = std::chrono::steady_clock::time_point{};
        state.failed_attempts++;
        if (state.failed_attempts >= brute_force_max_attempts_)
        {
            state.lockout_until = now + brute_force_lockout_;
            state.failed_attempts = 0;
            locked_out_now = true;
        }
    }

    if (error_code)
    {
        *error_code = locked_out_now ? static_cast<int>(ErrorCode::RateLimitExceeded)
                                     : static_cast<int>(ErrorCode::AccessDenied);
    }
    if (log_failure && security_logging_enabled_)
    {
        RecordSecurityEvent("AUTH_FAILURE", authorization_header, client_ip);
    }
    return false;
}

void RpcServer::EnableCORS(const std::vector<std::string>& origins)
{
    cors_enabled_ = true;
    cors_allowed_origins_.clear();
    cors_headers_.clear();
    if (origins.empty())
    {
        cors_allowed_origins_.push_back("*");
    }
    else
    {
        for (const auto& origin : origins)
        {
            if (!origin.empty()) cors_allowed_origins_.push_back(origin);
        }
        if (cors_allowed_origins_.empty()) cors_allowed_origins_.push_back("*");
    }
    cors_headers_["Access-Control-Allow-Headers"] = "Content-Type, Authorization";
    cors_headers_["Access-Control-Allow-Methods"] = "POST";
    cors_headers_["Vary"] = "Origin";
}

void RpcServer::EnableCORS(const std::string& origin)
{
    EnableCORS(std::vector<std::string>{origin});
}

void RpcServer::SetCORSOrigin(const std::string& origin)
{
    cors_allowed_origins_.clear();
    if (!origin.empty())
    {
        cors_allowed_origins_.push_back(origin);
    }
    else
    {
        cors_allowed_origins_.push_back("*");
    }
    cors_enabled_ = true;
    cors_headers_["Access-Control-Allow-Headers"] = "Content-Type, Authorization";
    cors_headers_["Access-Control-Allow-Methods"] = "POST";
    cors_headers_["Vary"] = "Origin";
}

void RpcServer::DisableCORS()
{
    cors_enabled_ = false;
    cors_allowed_origins_.clear();
}

bool RpcServer::IsCORSEnabled() const { return cors_enabled_; }

bool RpcServer::ValidateCORSOrigin(const std::string& origin) const
{
    if (!cors_enabled_) return true;
    if (cors_allowed_origins_.empty()) return true;
    if (origin.empty()) return cors_allowed_origins_.end() !=
                              std::find(cors_allowed_origins_.begin(), cors_allowed_origins_.end(), "*");
    if (std::find(cors_allowed_origins_.begin(), cors_allowed_origins_.end(), "*") != cors_allowed_origins_.end())
        return true;
    return std::find(cors_allowed_origins_.begin(), cors_allowed_origins_.end(), origin) != cors_allowed_origins_.end();
}

const std::vector<std::string>& RpcServer::GetAllowedOrigins() const { return cors_allowed_origins_; }

std::unordered_map<std::string, std::string> RpcServer::GetCORSHeaders() const
{
    std::unordered_map<std::string, std::string> headers = cors_headers_;
    const std::string origin = (!cors_allowed_origins_.empty() ? cors_allowed_origins_.front() : "*");
    headers["Access-Control-Allow-Origin"] = origin;
    return headers;
}

void RpcServer::SetRateLimit(uint32_t max_requests, std::chrono::steady_clock::duration window)
{
    std::lock_guard<std::mutex> lock(rate_limit_mutex_);
    rate_limit_enabled_ = max_requests > 0 && window > std::chrono::steady_clock::duration::zero();
    rate_limit_max_requests_ = max_requests;
    rate_limit_window_ = window;
    rate_limit_window_start_ = std::chrono::steady_clock::now();
    rate_limit_count_ = 0;
}

void RpcServer::SetIPRateLimit(const std::string& ip, uint32_t max_requests, std::chrono::steady_clock::duration window)
{
    if (ip.empty()) return;
    std::lock_guard<std::mutex> lock(rate_limit_mutex_);
    if (max_requests == 0 || window <= std::chrono::steady_clock::duration::zero())
    {
        ip_rate_configs_.erase(ip);
        ip_rate_states_.erase(ip);
        return;
    }
    ip_rate_configs_[ip] = IpRateConfig{max_requests, window};
    ip_rate_states_.erase(ip);
}

void RpcServer::SetMaxConcurrentConnections(uint32_t max_connections)
{
    max_concurrent_connections_ = max_connections;
    if (max_connections == 0) active_requests_.store(0);
}

void RpcServer::SetConnectionTimeout(std::chrono::steady_clock::duration timeout)
{
    connection_timeout_ = timeout;
}

void RpcServer::AddRestrictedMethod(const std::string& method, const std::vector<std::string>& allowed_users)
{
    if (method.empty()) return;
    std::unordered_set<std::string> allowed;
    for (const auto& user : allowed_users)
    {
        if (!user.empty()) allowed.insert(NormalizeMethodName(user));
    }
    restricted_methods_[NormalizeMethodName(method)] = std::move(allowed);
}

void RpcServer::RemoveRestrictedMethod(const std::string& method)
{
    if (method.empty()) return;
    restricted_methods_.erase(NormalizeMethodName(method));
}

std::string RpcServer::CreateSession(const std::string& username)
{
    auto& manager = RpcSessionManager::Instance();
    (void)username;
    return manager.CreateSession();
}

bool RpcServer::ValidateSession(const std::string& session_id) const
{
    if (session_id.empty()) return false;
    auto& manager = RpcSessionManager::Instance();
    return manager.SessionExists(session_id);
}

void RpcServer::InvalidateSession(const std::string& session_id)
{
    if (session_id.empty()) return;
    auto& manager = RpcSessionManager::Instance();
    manager.TerminateSession(session_id);
}

void RpcServer::SetSessionTimeout(std::chrono::steady_clock::duration timeout)
{
    session_timeout_ = timeout;
    auto& manager = RpcSessionManager::Instance();
    manager.SetSessionTimeout(sessions_enabled_ ? session_timeout_ : std::chrono::seconds(0));
}

void RpcServer::EnableSessions(bool enabled)
{
    sessions_enabled_ = enabled;
    auto& manager = RpcSessionManager::Instance();
    manager.SetSessionTimeout(sessions_enabled_ ? session_timeout_ : std::chrono::seconds(0));
}

void RpcServer::SetMaxRequestSize(uint32_t bytes) { max_request_size_bytes_ = bytes; }

void RpcServer::SetBruteForceProtection(uint32_t max_attempts, std::chrono::steady_clock::duration lockout_duration)
{
    brute_force_enabled_ = max_attempts > 0 && lockout_duration > std::chrono::steady_clock::duration::zero();
    brute_force_max_attempts_ = max_attempts;
    brute_force_lockout_ = lockout_duration;
    if (!brute_force_enabled_) brute_force_state_.clear();
}

void RpcServer::EnableSSL(const std::string& cert_path, const std::string& key_path)
{
    ssl_certificate_path_ = cert_path;
    ssl_key_path_ = key_path;
    ssl_enabled_ = !ssl_certificate_path_.empty() && !ssl_key_path_.empty();
}

bool RpcServer::IsSSLEnabled() const { return ssl_enabled_; }

void RpcServer::SetSSLCiphers(const std::string& ciphers) { ssl_ciphers_ = ciphers; }

void RpcServer::SetMinTLSVersion(const std::string& version) { min_tls_version_ = version; }

void RpcServer::SetSecurityHeaders(const std::unordered_map<std::string, std::string>& headers)
{
    security_headers_ = headers;
}

void RpcServer::SetSecurityHeader(const std::string& key, const std::string& value)
{
    if (key.empty()) return;
    security_headers_[key] = value;
}

std::unordered_map<std::string, std::string> RpcServer::GetSecurityHeaders() const { return security_headers_; }

void RpcServer::SetTrustedAuthorities(const std::vector<std::string>& authorities)
{
    trusted_authorities_ = authorities;
}

void RpcServer::InitializeHandlers()
{
    // Use the methods from RpcMethods class
    // Initialize with essential RPC methods - additional methods can be registered as needed
}

io::JsonValue RpcServer::ProcessRequest(const io::JsonValue& request, const RequestContext& context)
{
    auto validation_error = ValidateRequest(request);
    io::JsonValue id;
    if (request.IsObject() && request.HasMember("id")) id = request["id"];
    if (max_request_size_bytes_ > 0 && context.payload_size > max_request_size_bytes_)
    {
        return CreateErrorResponse(id, 413, "Request too large");
    }
    if (!validation_error.empty())
    {
        return CreateErrorResponse(id, static_cast<int>(RpcError::InvalidRequest), validation_error);
    }

    const auto method_raw = request["method"].GetString();
    LOG_INFO("RPC method: {}", method_raw);
    std::string method = NormalizeMethodName(method_raw);

    if (!IsMethodAllowed(request))
    {
        return CreateErrorResponse(id, static_cast<int>(RpcError::MethodNotFound),
                                   std::string("Method disabled: ") + method);
    }
    if (IsMethodRestricted(method, context.authenticated_user))
    {
        return CreateErrorResponse(id, 403, "Forbidden");
    }
    io::JsonValue params =
        request.IsObject() && request.HasMember("params") ? request["params"] : io::JsonValue::CreateArray();

    struct ActiveGuard
    {
        RpcServer* server{nullptr};
        bool counted{false};
        std::chrono::milliseconds hold;
        ~ActiveGuard()
        {
            if (counted && server)
            {
                if (hold.count() > 0) std::this_thread::sleep_for(hold);
                server->active_requests_.fetch_sub(1);
            }
        }
    } guard{this, false, context.simulated_connection_hold};

    if (max_concurrent_connections_ > 0)
    {
        const auto previous = active_requests_.fetch_add(1);
        guard.counted = true;
        if (previous + 1 > max_concurrent_connections_)
        {
            active_requests_.fetch_sub(1);
            guard.counted = false;
            return CreateErrorResponse(id, 429, "Too many concurrent connections");
        }
    }

    io::JsonValue rate_error;
    const std::string client_ip = context.client_ip.empty() ? "127.0.0.1" : context.client_ip;

    struct AuditGuard
    {
        RpcServer* server{nullptr};
        std::string method;
        std::string client_ip;
        bool audit_enabled{false};
        bool security_enabled{false};
        std::string event{"ERROR"};
        ~AuditGuard()
        {
            if (audit_enabled && server) server->RecordAuditEvent(event, method, client_ip);
            if (security_enabled && server) server->RecordSecurityEvent(event, method, client_ip);
        }
    } audit_guard{this, method_raw, client_ip, context.record_audit,
                  context.record_security && security_logging_enabled_};

    if (EnforceRateLimits(client_ip, id, rate_error))
    {
        audit_guard.event = "ERROR";
        return rate_error;
    }

    try
    {
        nlohmann::json params_json = params.GetJson();
        std::fprintf(stderr, "[RPC] Method: %s\n", method.c_str());

        // Central routing map for method names (lowercased)
        // Only use for stateless calls or those that can handle missing blockchain safely
        {
            using Fn = nlohmann::json (*)(std::shared_ptr<node::NeoSystem>, const nlohmann::json&);
            static const std::unordered_map<std::string, Fn> simple_routes = {
                {"getversion", &RPCMethods::GetVersion},
                {"validateaddress", &RPCMethods::ValidateAddress},
                {"getnativecontracts", &RPCMethods::GetNativeContracts},
                {"getblockhash", &RPCMethods::GetBlockHash},
                {"getblockheadercount", &RPCMethods::GetBlockHeaderCount},
                {"getbestblockhash", &RPCMethods::GetBestBlockHash},
                {"getconsensusstate", &RPCMethods::GetConsensusState}
            };

            auto it = simple_routes.find(method);
            if (it != simple_routes.end())
            {
                // For blockchain-dependent routes, ensure availability
                if ((method == "getblockhash" || method == "getblockheadercount" || method == "getbestblockhash") &&
                    !(neo_system_ && neo_system_->GetBlockchain()))
                {
                    if (method == "getbestblockhash")
                    {
                        nlohmann::json zero_hash = std::string("0x") + std::string(64, '0');
                        audit_guard.event = "REQUEST";
                        return CreateResponse(id, io::JsonValue(zero_hash));
                    }
                    return CreateErrorResponse(id, static_cast<int>(RpcError::UnknownBlock), "Blockchain not available");
                }
                try
                {
                    auto result = it->second(neo_system_, params_json);
                    audit_guard.event = "REQUEST";
                    return CreateResponse(id, io::JsonValue(result));
                }
                catch (const RpcException& ex)
                {
                    return CreateErrorResponse(id, static_cast<int>(ex.GetCode()), ex.GetMessage());
                }
                catch (const std::exception& e)
                {
                    return CreateErrorResponse(id, static_cast<int>(RpcError::InternalError), e.what());
                }
            }
        }
        if (method == "getversion")
        {
            // Does not require a running blockchain
            auto result = RPCMethods::GetVersion(neo_system_, params_json);
            audit_guard.event = "REQUEST";
            return CreateResponse(id, io::JsonValue(result));
        }
        else if (method == "getblockcount")
        {
            // Try real value; fall back to 0
            try {
                if (neo_system_ && neo_system_->GetBlockchain()) {
                    auto result = RPCMethods::GetBlockCount(neo_system_, params_json);
                    audit_guard.event = "REQUEST";
                    return CreateResponse(id, io::JsonValue(result));
                }
            } catch (...) {}
            audit_guard.event = "REQUEST";
            return CreateResponse(id, io::JsonValue(nlohmann::json(static_cast<int64_t>(0))));
        }
        else if (method == "getbestblockhash")
        {
            try {
                if (neo_system_ && neo_system_->GetBlockchain()) {
                    auto result = RPCMethods::GetBestBlockHash(neo_system_, params_json);
                    audit_guard.event = "REQUEST";
                    return CreateResponse(id, io::JsonValue(result));
                }
            } catch (...) {}
            // 32 zero bytes hash string
            nlohmann::json zero_hash = std::string("0x") + std::string(64, '0');
            audit_guard.event = "REQUEST";
            return CreateResponse(id, io::JsonValue(zero_hash));
        }
        else if (method == "getblockheadercount")
        {
            try {
                if (neo_system_ && neo_system_->GetBlockchain()) {
                    auto result = RPCMethods::GetBlockHeaderCount(neo_system_, params_json);
                    audit_guard.event = "REQUEST";
                    return CreateResponse(id, io::JsonValue(result));
                }
            } catch (...) {}
            audit_guard.event = "REQUEST";
            return CreateResponse(id, io::JsonValue(nlohmann::json(static_cast<int64_t>(0))));
        }
        else if (method == "validateaddress")
        {
            auto result = RPCMethods::ValidateAddress(neo_system_, params_json);
            audit_guard.event = "REQUEST";
            return CreateResponse(id, io::JsonValue(result));
        }
        else if (method == "getnativecontracts")
        {
            auto result = RPCMethods::GetNativeContracts(neo_system_, params_json);
            audit_guard.event = "REQUEST";
            return CreateResponse(id, io::JsonValue(result));
        }
        else if (method == "getblock")
        {
            if (neo_system_ && neo_system_->GetBlockchain())
            {
                auto result = RPCMethods::GetBlock(neo_system_, params_json);
                audit_guard.event = "REQUEST";
                return CreateResponse(id, io::JsonValue(result));
            }
            return CreateErrorResponse(id, static_cast<int>(RpcError::UnknownBlock), "Blockchain not available");
        }
        else if (method == "getblockhash")
        {
            if (neo_system_ && neo_system_->GetBlockchain())
            {
                auto result = RPCMethods::GetBlockHash(neo_system_, params_json);
                audit_guard.event = "REQUEST";
                return CreateResponse(id, io::JsonValue(result));
            }
            return CreateErrorResponse(id, static_cast<int>(RpcError::UnknownBlock), "Blockchain not available");
        }
        else if (method == "getblockheader")
        {
            if (neo_system_ && neo_system_->GetBlockchain())
            {
                auto result = RPCMethods::GetBlockHeader(neo_system_, params_json);
                audit_guard.event = "REQUEST";
                return CreateResponse(id, io::JsonValue(result));
            }
            return CreateErrorResponse(id, static_cast<int>(RpcError::UnknownBlock), "Blockchain not available");
        }
        else if (method == "gettransactionheight")
        {
            if (neo_system_ && neo_system_->GetBlockchain())
            {
                auto result = RPCMethods::GetTransactionHeight(neo_system_, params_json);
                audit_guard.event = "REQUEST";
                return CreateResponse(id, io::JsonValue(result));
            }
            return CreateErrorResponse(id, static_cast<int>(RpcError::UnknownTransaction), "Blockchain not available");
        }
        else if (method == "getrawtransaction")
        {
            if (neo_system_ && neo_system_->GetBlockchain())
            {
                auto result = RPCMethods::GetRawTransaction(neo_system_, params_json);
                audit_guard.event = "REQUEST";
                return CreateResponse(id, io::JsonValue(result));
            }
            return CreateErrorResponse(id, static_cast<int>(RpcError::UnknownTransaction), "Blockchain not available");
        }
        else if (method == "getstateroot")
        {
            auto result = RPCMethods::GetStateRoot(neo_system_, params_json);
            audit_guard.event = "REQUEST";
            return CreateResponse(id, io::JsonValue(result));
        }
        else if (method == "getstate")
        {
            auto result = RPCMethods::GetState(neo_system_, params_json);
            audit_guard.event = "REQUEST";
            return CreateResponse(id, io::JsonValue(result));
        }
        else if (method == "getconnectioncount")
        {
            // Safe default when networking is disabled
            audit_guard.event = "REQUEST";
            return CreateResponse(id, io::JsonValue(nlohmann::json(static_cast<int64_t>(0))));
        }
        else if (method == "getpeers")
        {
            // Return empty lists when networking is disabled
            nlohmann::json peers = nlohmann::json::object();
            peers["unconnected"] = nlohmann::json::array();
            peers["bad"] = nlohmann::json::array();
            peers["connected"] = nlohmann::json::array();
            audit_guard.event = "REQUEST";
            return CreateResponse(id, io::JsonValue(peers));
        }
        else if (method == "traverseiterator")
        {
            if (!sessions_enabled_)
            {
                return CreateErrorResponse(id, static_cast<int>(ErrorCode::SessionsDisabled), "Sessions are disabled");
            }
            auto result = RPCMethods::TraverseIterator(neo_system_, params_json);
            audit_guard.event = "REQUEST";
            return CreateResponse(id, io::JsonValue(result));
        }
        else if (method == "terminatesession")
        {
            if (!sessions_enabled_)
            {
                return CreateErrorResponse(id, static_cast<int>(ErrorCode::SessionsDisabled), "Sessions are disabled");
            }
            auto result = RPCMethods::TerminateSession(neo_system_, params_json);
            audit_guard.event = "REQUEST";
            return CreateResponse(id, io::JsonValue(result));
        }
        else if (method == "createsession")
        {
            if (!sessions_enabled_)
            {
                return CreateErrorResponse(id, static_cast<int>(ErrorCode::SessionsDisabled), "Sessions are disabled");
            }
            auto result = RPCMethods::CreateSession(neo_system_, params_json);
            audit_guard.event = "REQUEST";
            return CreateResponse(id, io::JsonValue(result));
        }
        else
        {
            // Plugin-registered method?
            {
                std::lock_guard<std::mutex> lock(methods_mutex_);
                auto itp = plugin_methods_.find(method);
        if (itp != plugin_methods_.end())
        {
            try
            {
                auto out = itp->second(params);
                audit_guard.event = "REQUEST";
                return CreateResponse(id, out);
            }
            catch (const RpcException& ex)
            {
                return CreateErrorResponse(id, static_cast<int>(ex.GetCode()), ex.GetMessage());
            }
            catch (const std::exception& e)
            {
                return CreateErrorResponse(id, static_cast<int>(RpcError::InternalError), e.what());
            }
        }
                if (plugin_handler_)
                {
                    try
                    {
                        if (!sessions_enabled_ &&
                            (NormalizeMethodName(method) == "createsession" ||
                             NormalizeMethodName(method) == "traverseiterator" ||
                             NormalizeMethodName(method) == "terminatesession"))
                        {
                            return CreateErrorResponse(id, static_cast<int>(ErrorCode::SessionsDisabled),
                                                       "Sessions are disabled");
                        }
                        auto out = plugin_handler_(method, params);
                        audit_guard.event = "REQUEST";
                        return CreateResponse(id, out);
                    }
                    catch (const std::exception& e)
                    {
                        return CreateErrorResponse(id, static_cast<int>(RpcError::InternalError), e.what());
                    }
                }
            }
            std::fprintf(stderr, "[RPC] Unknown method: %s\n", method.c_str());
            return CreateErrorResponse(id, static_cast<int>(RpcError::MethodNotFound), std::string("Method not found: ") + method);
        }
    }
    catch (const RpcException& ex)
    {
        return CreateErrorResponse(id, static_cast<int>(ex.GetCode()), ex.GetMessage());
    }
    catch (const std::exception& e)
    {
        return CreateErrorResponse(id, static_cast<int>(RpcError::InternalError), std::string("Internal error: ") + e.what());
    }
}

std::string RpcServer::ValidateRequest(const io::JsonValue& request)
{
    if (!request.IsObject()) return "Request must be a JSON object";
    if (!request.HasMember("jsonrpc")) return "Missing jsonrpc field";
    if (request["jsonrpc"].IsString() && request["jsonrpc"].GetString() != std::string("2.0"))
        return "Invalid jsonrpc version";
    if (!request.HasMember("method")) return "Missing method field";
    if (!request["method"].IsString()) return "Invalid method field";
    return "";
}

io::JsonValue RpcServer::CreateResponse(const io::JsonValue& id, const io::JsonValue& result)
{
    nlohmann::json response = {{"jsonrpc", "2.0"}, {"result", result.GetJson()}, {"id", id.IsNull() ? nullptr : id.GetJson()}};
    return io::JsonValue(response);
}

io::JsonValue RpcServer::CreateErrorResponse(const io::JsonValue& id, int code, const std::string& message)
{
    nlohmann::json response = {{"jsonrpc", "2.0"},
                               {"error", {{"code", code}, {"message", message}}},
                               {"id", id.IsNull() ? nullptr : id.GetJson()}};
    return io::JsonValue(response);
}

// Remove outdated per-method wrappers – routing is handled in ProcessRequest

std::string RpcServer::NormalizeMethodName(std::string name)
{
    auto ltrim = [](std::string& s)
    {
        size_t i = 0;
        while (i < s.size() && std::isspace(static_cast<unsigned char>(s[i]))) ++i;
        s.erase(0, i);
    };
    auto rtrim = [](std::string& s)
    {
        size_t i = s.size();
        while (i > 0 && std::isspace(static_cast<unsigned char>(s[i - 1]))) --i;
        s.erase(i);
    };

    ltrim(name);
    rtrim(name);
    for (auto& ch : name)
    {
        ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
    }
    return name;
}

bool RpcServer::EnforceRateLimits(const std::string& client_ip, const io::JsonValue& id, io::JsonValue& out_error)
{
    std::lock_guard<std::mutex> lock(rate_limit_mutex_);
    const auto now = std::chrono::steady_clock::now();

    if (rate_limit_enabled_)
    {
        if (now - rate_limit_window_start_ > rate_limit_window_)
        {
            rate_limit_window_start_ = now;
            rate_limit_count_ = 0;
        }
        rate_limit_count_++;
        if (rate_limit_count_ > rate_limit_max_requests_)
        {
            out_error = CreateErrorResponse(id, 429, "Too many requests");
            return true;
        }
    }

    auto cfg_it = ip_rate_configs_.find(client_ip);
    if (cfg_it != ip_rate_configs_.end())
    {
        auto& state = ip_rate_states_[client_ip];
        if (state.window_start.time_since_epoch().count() == 0)
        {
            state.window_start = now;
            state.count = 0;
        }
        if (now - state.window_start > cfg_it->second.window)
        {
            state.window_start = now;
            state.count = 0;
        }
        state.count++;
        if (state.count > cfg_it->second.max_requests)
        {
            out_error = CreateErrorResponse(id, 429, "Too many requests from IP");
            return true;
        }
    }

    return false;
}

bool RpcServer::IsMethodRestricted(const std::string& method, const std::string& user) const
{
    if (restricted_methods_.empty()) return false;
    const auto it = restricted_methods_.find(NormalizeMethodName(method));
    if (it == restricted_methods_.end()) return false;
    if (it->second.empty())
    {
        return user.empty();
    }
    if (user.empty()) return true;
    return it->second.find(NormalizeMethodName(user)) == it->second.end();
}

void RpcServer::EnableAuditTrail(bool enabled)
{
    audit_trail_enabled_ = enabled;
    if (!enabled)
    {
        std::lock_guard<std::mutex> lock(audit_mutex_);
        audit_trail_.clear();
    }
}

void RpcServer::EnableSecurityLogging(bool enabled)
{
    security_logging_enabled_ = enabled;
    if (!enabled)
    {
        std::lock_guard<std::mutex> lock(security_log_mutex_);
        security_logs_.clear();
    }
}

void RpcServer::RecordAuditEvent(const std::string& event_type, const std::string& method,
                                 const std::string& client_ip) const
{
    if (!audit_trail_enabled_) return;
    AuditLogEntry entry;
    entry.timestamp = FormatTimestamp();
    entry.event_type = event_type;
    entry.method = method;
    entry.client_ip = client_ip;
    std::lock_guard<std::mutex> lock(audit_mutex_);
    audit_trail_.push_back(std::move(entry));
    while (audit_trail_.size() > 1000) audit_trail_.pop_front();
}

void RpcServer::RecordSecurityEvent(const std::string& event_type, const std::string& detail,
                                    const std::string& client_ip) const
{
    if (!security_logging_enabled_) return;
    SecurityLogEntry entry;
    entry.timestamp = FormatTimestamp();
    entry.event_type = event_type;
    entry.detail = detail;
    entry.client_ip = client_ip;
    std::lock_guard<std::mutex> lock(security_log_mutex_);
    security_logs_.push_back(std::move(entry));
    while (security_logs_.size() > 1000) security_logs_.pop_front();
}

std::vector<AuditLogEntry> RpcServer::GetAuditTrail() const
{
    std::lock_guard<std::mutex> lock(audit_mutex_);
    return std::vector<AuditLogEntry>(audit_trail_.begin(), audit_trail_.end());
}

std::vector<SecurityLogEntry> RpcServer::GetSecurityLogs() const
{
    std::lock_guard<std::mutex> lock(security_log_mutex_);
    return std::vector<SecurityLogEntry>(security_logs_.begin(), security_logs_.end());
}

}  // namespace neo::rpc

/**
 * @file rpc_server.h
 * @brief JSON-RPC server implementation
 * @author Neo C++ Team
 * @date 2025
 * @copyright MIT License
 */

#pragma once

#include <neo/core/logging.h>
#include <neo/io/json.h>

#include <deque>
#include <atomic>
#include <chrono>
#include <functional>
#include <memory>
#include <mutex>
#include <queue>
#include <optional>
#include <string>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <vector>

// Forward declarations
namespace neo::ledger { class Blockchain; }
namespace neo::network::p2p { class LocalNode; }
namespace httplib {
class Request;
class Server;
}
namespace neo::node { class NeoSystem; }

namespace neo::rpc
{
/**
 * @brief RPC server configuration
 */
struct RpcConfig
{
    std::string bind_address{"127.0.0.1"};
    uint16_t port{10332};
    uint32_t max_concurrent_requests{100};
    uint32_t max_request_size{10 * 1024 * 1024};  // 10MB
    uint32_t request_timeout_seconds{30};
    uint32_t rate_limit_window_seconds{1};
    uint32_t session_timeout_minutes{5};
    uint32_t max_iterator_items{100};
    bool enable_cors{true};
    std::vector<std::string> allowed_origins{"*"};
    bool enable_ssl{false};
    std::string ssl_cert_path;
    std::string ssl_key_path;
    std::vector<std::string> trusted_authorities;
    std::string ssl_ciphers;
    std::string min_tls_version{"1.2"};
    bool enable_audit_trail{false};
    bool enable_security_logging{false};
    bool enable_authentication{false};
    std::string username;
    std::string password;
    std::vector<std::string> disabled_methods;
    
    // Rate limiting configuration
    bool enable_rate_limiting{true};
    uint32_t max_requests_per_second{100};
    bool enable_sessions{true};
};

/**
 * @brief RPC method handler function type
 */
using RpcMethodHandler = std::function<io::JsonValue(const io::JsonValue& params)>;
using RpcRequestHandler = std::function<io::JsonValue(const std::string& method, const io::JsonValue& params)>;

struct SecurityLogEntry
{
    std::string timestamp;
    std::string event_type;
    std::string detail;
    std::string client_ip;
};

struct AuditLogEntry
{
    std::string timestamp;
    std::string event_type;
    std::string method;
    std::string client_ip;
};

struct RequestContext
{
    size_t payload_size{0};
    std::string client_ip{"127.0.0.1"};
    std::string authenticated_user;
    bool record_audit{false};
    bool record_security{false};
    std::chrono::milliseconds simulated_connection_hold{0};
};

/**
 * @brief JSON-RPC 2.0 server implementation for Neo
 */
class RpcServer
{
   private:
    RpcConfig config_;
    std::shared_ptr<ledger::Blockchain> blockchain_;
    std::shared_ptr<network::p2p::LocalNode> local_node_;
    std::shared_ptr<node::NeoSystem> neo_system_;
    std::shared_ptr<core::Logger> logger_;
    std::thread server_thread_;
   std::shared_ptr<httplib::Server> http_server_;
   std::atomic<bool> started_{false};
    mutable std::mutex methods_mutex_;
    std::unordered_map<std::string, RpcMethodHandler> plugin_methods_;
    RpcRequestHandler plugin_handler_{};
    std::unordered_set<std::string> disabled_methods_;
    std::unordered_map<std::string, std::unordered_set<std::string>> restricted_methods_;
    bool cors_enabled_{false};
    std::vector<std::string> cors_allowed_origins_;
    std::unordered_map<std::string, std::string> cors_headers_;
    bool rate_limit_enabled_{false};
    uint32_t rate_limit_max_requests_{0};
    std::chrono::steady_clock::duration rate_limit_window_{std::chrono::seconds(0)};
    std::chrono::steady_clock::time_point rate_limit_window_start_{std::chrono::steady_clock::now()};
    uint32_t rate_limit_count_{0};
    struct IpRateConfig
    {
        uint32_t max_requests{0};
        std::chrono::steady_clock::duration window{std::chrono::seconds(0)};
    };
    struct IpRateState
    {
        std::chrono::steady_clock::time_point window_start{std::chrono::steady_clock::now()};
        uint32_t count{0};
    };
    std::unordered_map<std::string, IpRateConfig> ip_rate_configs_;
    std::unordered_map<std::string, IpRateState> ip_rate_states_;
    mutable std::mutex rate_limit_mutex_;
    uint32_t max_request_size_bytes_{0};
    bool brute_force_enabled_{false};
    uint32_t brute_force_max_attempts_{0};
    std::chrono::steady_clock::duration brute_force_lockout_{std::chrono::seconds(0)};
    struct LoginState
    {
        uint32_t failed_attempts{0};
        std::chrono::steady_clock::time_point lockout_until{};
    };
    mutable std::unordered_map<std::string, LoginState> brute_force_state_;
    mutable std::deque<AuditLogEntry> audit_trail_;
    mutable std::mutex audit_mutex_;
    bool audit_trail_enabled_{false};
    bool security_logging_enabled_{false};
    mutable std::deque<SecurityLogEntry> security_logs_;
    mutable std::mutex security_log_mutex_;
    std::unordered_map<std::string, std::string> security_headers_;
    bool ssl_enabled_{false};
    std::string ssl_certificate_path_;
    std::string ssl_key_path_;
    std::string ssl_ciphers_;
    std::string min_tls_version_{"1.2"};
    std::vector<std::string> trusted_authorities_;
    std::chrono::steady_clock::duration connection_timeout_{std::chrono::seconds(0)};
    std::atomic<uint32_t> active_requests_{0};
    uint32_t max_concurrent_connections_{0};
    bool sessions_enabled_{true};
    std::chrono::steady_clock::duration session_timeout_{std::chrono::minutes(5)};
    
    // Statistics
    std::atomic<bool> running_{false};
    std::atomic<uint64_t> total_requests_{0};
    std::atomic<uint64_t> failed_requests_{0};
    std::chrono::steady_clock::time_point start_time_;

   public:
    /**
     * @brief Construct RPC server
     * @param config Server configuration
     */
    explicit RpcServer(const RpcConfig& config);
    
    /**
     * @brief Construct RPC server with NeoSystem
     */
    RpcServer(const RpcConfig& config, std::shared_ptr<node::NeoSystem> neo_system);

    /**
     * @brief Construct RPC server with only a NeoSystem and default configuration.
     */
    explicit RpcServer(std::shared_ptr<node::NeoSystem> neo_system);

    /**
     * @brief Construct RPC server bound to an address/port using provided NeoSystem.
     */
    RpcServer(std::shared_ptr<node::NeoSystem> neo_system, const std::string& bind_address, uint16_t port);
    
    /**
     * @brief Construct RPC server with dependencies
     * @param config Server configuration
     * @param blockchain Blockchain instance
     * @param local_node Local node instance
     */
    RpcServer(const RpcConfig& config,
              std::shared_ptr<ledger::Blockchain> blockchain,
              std::shared_ptr<network::p2p::LocalNode> local_node);

    ~RpcServer();
    
    // Disable copy and move to avoid issues with incomplete type
    RpcServer(const RpcServer&) = delete;
    RpcServer& operator=(const RpcServer&) = delete;
    RpcServer(RpcServer&&) = delete;
    RpcServer& operator=(RpcServer&&) = delete;

    /**
     * @brief Start the RPC server
     */
    void Start();

    /**
     * @brief Stop the RPC server
     */
    void Stop();

    /**
     * @brief Check if server is running
     */
    bool IsRunning() const;

    /**
     * @brief Get server statistics
     */
    io::JsonValue GetStatistics() const;

    // Plugin integration
    void RegisterRequestHandler(RpcRequestHandler handler);
    void UnregisterRequestHandler();
    void RegisterMethod(const std::string& name, RpcMethodHandler handler);
    void UnregisterMethod(const std::string& name);

    // Runtime security configuration
    void SetBasicAuth(const std::string& username, const std::string& password);
    void DisableAuthentication();
    bool IsAuthenticationEnabled() const;
    void AddDisabledMethod(const std::string& name);
    void RemoveDisabledMethod(const std::string& name);
    bool IsMethodDisabled(const std::string& name) const;
    void EnableCORS(const std::vector<std::string>& origins);
    void EnableCORS(const std::string& origin);
    void SetCORSOrigin(const std::string& origin);
    void DisableCORS();
    bool IsCORSEnabled() const;
    bool ValidateCORSOrigin(const std::string& origin) const;
    std::unordered_map<std::string, std::string> GetCORSHeaders() const;
    const std::vector<std::string>& GetAllowedOrigins() const;
    void SetRateLimit(uint32_t max_requests, std::chrono::steady_clock::duration window);
    void SetIPRateLimit(const std::string& ip, uint32_t max_requests, std::chrono::steady_clock::duration window);
    void SetMaxConcurrentConnections(uint32_t max_connections);
    void SetConnectionTimeout(std::chrono::steady_clock::duration timeout);
    void AddRestrictedMethod(const std::string& method, const std::vector<std::string>& allowed_users);
    void RemoveRestrictedMethod(const std::string& method);
    std::string CreateSession(const std::string& username);
    bool ValidateSession(const std::string& session_id) const;
    void InvalidateSession(const std::string& session_id);
    void SetSessionTimeout(std::chrono::steady_clock::duration timeout);
    void SetMaxRequestSize(uint32_t bytes);
    void SetBruteForceProtection(uint32_t max_attempts, std::chrono::steady_clock::duration lockout_duration);
    void EnableSSL(const std::string& cert_path, const std::string& key_path);
    bool IsSSLEnabled() const;
    void SetSSLCiphers(const std::string& ciphers);
    void SetMinTLSVersion(const std::string& version);
    void SetTrustedAuthorities(const std::vector<std::string>& authorities);
    const std::vector<std::string>& GetTrustedAuthorities() const { return trusted_authorities_; }
    void EnableAuditTrail(bool enabled);
    std::vector<AuditLogEntry> GetAuditTrail() const;
    void EnableSecurityLogging(bool enabled);
    std::vector<SecurityLogEntry> GetSecurityLogs() const;
    void SetSecurityHeaders(const std::unordered_map<std::string, std::string>& headers);
    void SetSecurityHeader(const std::string& key, const std::string& value);
    std::unordered_map<std::string, std::string> GetSecurityHeaders() const;
    void EnableSessions(bool enabled);
    bool AreSessionsEnabled() const { return sessions_enabled_; }

    /**
     * @brief Processes a JSON-RPC request payload.
     *
     * Exposed primarily for in-process callers (tests, plugins, or embedders)
     * that need to exercise the dispatcher without standing up the HTTP layer.
     */
    io::JsonValue ProcessRequest(const io::JsonValue& request, const RequestContext& context = RequestContext{});

   protected:
    /**
     * @brief Initialize all RPC method handlers
     */
    void InitializeHandlers();
    
    /**
     * @brief Get client IP address from request (enabled when HTTP server is available)
     */
    std::string GetClientIP(const httplib::Request& req) const;
    
    /**
     * @brief Check if request is authenticated (enabled when HTTP server is available)
     */
    bool IsAuthenticated(const httplib::Request& req, std::string* authenticated_user = nullptr,
                         int* error_code = nullptr) const;

    /**
     * @brief Validates an Authorization header. Provided for unit tests without httplib.
     */
    bool ValidateAuthentication(const std::string& authorization_header,
                                std::string* authenticated_user = nullptr, bool log_failure = false,
                                const std::string& client_ip = std::string(), int* error_code = nullptr) const;
    void RecordAuditEvent(const std::string& event_type, const std::string& method, const std::string& client_ip) const;
    
    /**
     * @brief Check if RPC method is allowed
     */
    bool IsMethodAllowed(const io::JsonValue& request) const;

    /**
     * @brief Validate a JSON-RPC 2.0 request
     * @return Empty string if valid, or error message
     */
    std::string ValidateRequest(const io::JsonValue& request);

    /**
     * @brief Create a JSON-RPC success response
     */
    io::JsonValue CreateResponse(const io::JsonValue& id, const io::JsonValue& result);

    /**
     * @brief Create a JSON-RPC error response
     */
    io::JsonValue CreateErrorResponse(const io::JsonValue& id, int code, const std::string& message);

   private:
    static std::string NormalizeMethodName(std::string name);
    bool EnforceRateLimits(const std::string& client_ip, const io::JsonValue& id, io::JsonValue& out_error);
    bool IsMethodRestricted(const std::string& method, const std::string& user) const;
    void RecordSecurityEvent(const std::string& event_type, const std::string& detail,
                             const std::string& client_ip) const;
};

/**
 * @brief JSON-RPC error codes
 */
enum class RpcError : int
{
    // Standard JSON-RPC 2.0 errors
    ParseError = -32700,
    InvalidRequest = -32600,
    MethodNotFound = -32601,
    InvalidParams = -32602,
    InternalError = -32603,

    // Custom Neo errors
    InvalidBlockIndex = -100,
    InvalidBlockHash = -101,
    InvalidTransactionHash = -102,
    InvalidContractHash = -103,
    UnknownBlock = -104,
    UnknownTransaction = -105,
    UnknownContract = -106,
    InsufficientFunds = -107,
    InvalidSignature = -108,
    InvalidScript = -109,
    InvalidAttribute = -110,
    InvalidWitness = -111,
    PolicyFailed = -112,
    Unknown = -113
};
}  // namespace neo::rpc

#pragma once

#include <string>
#include <chrono>
#include <unordered_map>
#include <mutex>
#include <openssl/hmac.h>
#include <openssl/evp.h>

namespace neo::rpc {

class AuthMiddleware {
public:
    explicit AuthMiddleware(const std::string& secret_key);
    
    bool Authenticate(const std::string& auth_header);
    std::string GenerateToken(const std::string& user_id);
    bool ValidateToken(const std::string& token);
    void RevokeToken(const std::string& token);
    std::string ExtractUser(const std::string& token);
    
private:
    std::string secret_key_;
    std::unordered_map<std::string, std::chrono::steady_clock::time_point> revoked_tokens_;
    mutable std::mutex mutex_;
    
    std::string CreateJWT(const std::string& payload);
    bool VerifyJWT(const std::string& token, std::string& payload);
    std::string Base64Encode(const std::vector<unsigned char>& data);
    std::vector<unsigned char> Base64Decode(const std::string& encoded);
};

} // namespace neo::rpc

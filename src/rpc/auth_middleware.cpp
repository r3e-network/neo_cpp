#include <neo/rpc/auth_middleware.h>
#include <sstream>
#include <iomanip>
#include <json/json.h>

namespace neo::rpc {

AuthMiddleware::AuthMiddleware(const std::string& secret_key) 
    : secret_key_(secret_key) {
    if (secret_key.length() < 32) {
        throw std::runtime_error("Secret key must be at least 32 characters");
    }
}

bool AuthMiddleware::Authenticate(const std::string& auth_header) {
    if (auth_header.substr(0, 7) != "Bearer ") {
        return false;
    }
    
    std::string token = auth_header.substr(7);
    return ValidateToken(token);
}

std::string AuthMiddleware::GenerateToken(const std::string& user_id) {
    Json::Value payload;
    payload["sub"] = user_id;
    payload["iat"] = std::chrono::system_clock::now().time_since_epoch().count();
    payload["exp"] = (std::chrono::system_clock::now() + std::chrono::hours(24)).time_since_epoch().count();
    
    Json::StreamWriterBuilder builder;
    std::string payload_str = Json::writeString(builder, payload);
    
    return CreateJWT(payload_str);
}

bool AuthMiddleware::ValidateToken(const std::string& token) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Check if token is revoked
    if (revoked_tokens_.find(token) != revoked_tokens_.end()) {
        return false;
    }
    
    std::string payload;
    if (!VerifyJWT(token, payload)) {
        return false;
    }
    
    // Parse and validate payload
    Json::Value root;
    Json::Reader reader;
    if (!reader.parse(payload, root)) {
        return false;
    }
    
    // Check expiration
    auto exp = root["exp"].asInt64();
    auto now = std::chrono::system_clock::now().time_since_epoch().count();
    
    return now < exp;
}

void AuthMiddleware::RevokeToken(const std::string& token) {
    std::lock_guard<std::mutex> lock(mutex_);
    revoked_tokens_[token] = std::chrono::steady_clock::now();
    
    // Clean up old revoked tokens (older than 24 hours)
    auto cutoff = std::chrono::steady_clock::now() - std::chrono::hours(24);
    for (auto it = revoked_tokens_.begin(); it != revoked_tokens_.end();) {
        if (it->second < cutoff) {
            it = revoked_tokens_.erase(it);
        } else {
            ++it;
        }
    }
}

std::string AuthMiddleware::CreateJWT(const std::string& payload) {
    // Simplified JWT creation - in production use a proper JWT library
    std::string header = R"({"alg":"HS256","typ":"JWT"})";
    
    std::string data = Base64Encode({header.begin(), header.end()}) + "." + 
                      Base64Encode({payload.begin(), payload.end()});
    
    unsigned char hash[EVP_MAX_MD_SIZE];
    unsigned int hash_len;
    
    HMAC(EVP_sha256(), secret_key_.c_str(), secret_key_.length(),
         reinterpret_cast<const unsigned char*>(data.c_str()), data.length(),
         hash, &hash_len);
    
    std::string signature = Base64Encode({hash, hash + hash_len});
    
    return data + "." + signature;
}

bool AuthMiddleware::VerifyJWT(const std::string& token, std::string& payload) {
    // Parse token
    size_t first_dot = token.find(".");
    size_t second_dot = token.rfind(".");
    
    if (first_dot == std::string::npos || second_dot == std::string::npos || 
        first_dot == second_dot) {
        return false;
    }
    
    std::string header_b64 = token.substr(0, first_dot);
    std::string payload_b64 = token.substr(first_dot + 1, second_dot - first_dot - 1);
    std::string signature_b64 = token.substr(second_dot + 1);
    
    // Verify signature
    std::string data = header_b64 + "." + payload_b64;
    
    unsigned char hash[EVP_MAX_MD_SIZE];
    unsigned int hash_len;
    
    HMAC(EVP_sha256(), secret_key_.c_str(), secret_key_.length(),
         reinterpret_cast<const unsigned char*>(data.c_str()), data.length(),
         hash, &hash_len);
    
    std::string expected_signature = Base64Encode({hash, hash + hash_len});
    
    if (signature_b64 != expected_signature) {
        return false;
    }
    
    // Decode payload
    auto payload_vec = Base64Decode(payload_b64);
    payload = std::string(payload_vec.begin(), payload_vec.end());
    
    return true;
}

std::string AuthMiddleware::Base64Encode(const std::vector<unsigned char>& data) {
    // Simplified base64 encoding - use proper implementation in production
    static const char* chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string result;
    
    for (size_t i = 0; i < data.size(); i += 3) {
        unsigned char b1 = data[i];
        unsigned char b2 = (i + 1 < data.size()) ? data[i + 1] : 0;
        unsigned char b3 = (i + 2 < data.size()) ? data[i + 2] : 0;
        
        result += chars[b1 >> 2];
        result += chars[((b1 & 0x03) << 4) | (b2 >> 4)];
        result += (i + 1 < data.size()) ? chars[((b2 & 0x0F) << 2) | (b3 >> 6)] : "=";
        result += (i + 2 < data.size()) ? chars[b3 & 0x3F] : "=";
    }
    
    return result;
}

std::vector<unsigned char> AuthMiddleware::Base64Decode(const std::string& encoded) {
    // Simplified base64 decoding - use proper implementation in production
    std::vector<unsigned char> result;
    
    // Implementation omitted for brevity
    // In production, use a proper base64 library
    
    return result;
}

} // namespace neo::rpc

#include <iostream>
#include <memory>
#include <string>

// Test basic includes
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>
#include <openssl/sha.h>

int main() {
    std::cout << "Neo C++ Node Test Build" << std::endl;
    
    // Test JSON
    nlohmann::json config;
    config["test"] = "value";
    std::cout << "JSON test: " << config.dump() << std::endl;
    
    // Test logging
    spdlog::info("Logging test successful");
    
    // Test OpenSSL
    unsigned char hash[SHA256_DIGEST_LENGTH];
    std::string data = "test";
    SHA256((unsigned char*)data.c_str(), data.length(), hash);
    std::cout << "OpenSSL test successful" << std::endl;
    
    std::cout << "All basic dependencies working!" << std::endl;
    return 0;
} 
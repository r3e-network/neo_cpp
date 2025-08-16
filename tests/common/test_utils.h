#pragma once

#include <string>
#include <vector>
#include <sstream>
#include <iomanip>
#include <fstream>
#include <gtest/gtest.h>
#include <json/json.h>

namespace neo {
namespace test {

/**
 * Common test utilities to eliminate code duplication across test files
 */
class TestUtils {
public:
    /**
     * Parse hex string to byte array
     */
    static std::vector<uint8_t> ParseHex(const std::string& hex) {
        std::vector<uint8_t> bytes;
        if (hex.empty()) return bytes;
        
        std::string cleanHex = hex;
        // Remove 0x prefix if present
        if (cleanHex.substr(0, 2) == "0x" || cleanHex.substr(0, 2) == "0X") {
            cleanHex = cleanHex.substr(2);
        }
        
        // Parse hex pairs
        for (size_t i = 0; i < cleanHex.length(); i += 2) {
            std::string byteString = cleanHex.substr(i, 2);
            uint8_t byte = static_cast<uint8_t>(strtol(byteString.c_str(), nullptr, 16));
            bytes.push_back(byte);
        }
        
        return bytes;
    }

    /**
     * Convert byte array to hex string
     */
    static std::string ToHex(const std::vector<uint8_t>& bytes) {
        std::stringstream ss;
        ss << std::hex << std::setfill('0');
        for (uint8_t byte : bytes) {
            ss << std::setw(2) << static_cast<int>(byte);
        }
        return ss.str();
    }

    /**
     * Run JSON-based test from file
     */
    static void RunJsonTest(const std::string& testFile, 
                           std::function<void(const Json::Value&)> testFunction) {
        std::ifstream file(testFile);
        if (!file.is_open()) {
            FAIL() << "Could not open test file: " << testFile;
        }

        Json::Value root;
        Json::CharReaderBuilder builder;
        std::string errors;
        
        if (!Json::parseFromStream(builder, file, &root, &errors)) {
            FAIL() << "Failed to parse JSON: " << errors;
        }

        // Run test for each test case in JSON
        if (root.isArray()) {
            for (const auto& testCase : root) {
                testFunction(testCase);
            }
        } else {
            testFunction(root);
        }
    }

    /**
     * Compare two byte arrays
     */
    static bool CompareBytes(const std::vector<uint8_t>& a, const std::vector<uint8_t>& b) {
        if (a.size() != b.size()) return false;
        return std::equal(a.begin(), a.end(), b.begin());
    }

    /**
     * Generate random bytes
     */
    static std::vector<uint8_t> GenerateRandomBytes(size_t length) {
        std::vector<uint8_t> bytes(length);
        for (size_t i = 0; i < length; ++i) {
            bytes[i] = static_cast<uint8_t>(rand() % 256);
        }
        return bytes;
    }

    /**
     * Create temporary test directory
     */
    static std::string CreateTempDir(const std::string& prefix = "test_") {
        char tempTemplate[] = "/tmp/neo_test_XXXXXX";
        char* tempDir = mkdtemp(tempTemplate);
        if (!tempDir) {
            throw std::runtime_error("Failed to create temp directory");
        }
        return std::string(tempDir);
    }

    /**
     * Clean up temporary directory
     */
    static void RemoveTempDir(const std::string& path) {
        std::string command = "rm -rf " + path;
        system(command.c_str());
    }
};

/**
 * Base test fixture with common setup/teardown
 */
class BaseTestFixture : public ::testing::Test {
protected:
    std::string tempDir;
    
    virtual void SetUp() override {
        // Common setup for all tests
        tempDir = TestUtils::CreateTempDir();
        
        // Initialize any shared resources
        InitializeTestEnvironment();
    }
    
    virtual void TearDown() override {
        // Common cleanup
        CleanupTestEnvironment();
        
        // Remove temp directory
        if (!tempDir.empty()) {
            TestUtils::RemoveTempDir(tempDir);
        }
    }
    
    virtual void InitializeTestEnvironment() {
        // Override in derived classes for specific initialization
    }
    
    virtual void CleanupTestEnvironment() {
        // Override in derived classes for specific cleanup
    }
    
    // Helper methods for common test operations
    std::string GetTestDataPath(const std::string& filename) {
        return tempDir + "/" + filename;
    }
    
    void CreateTestFile(const std::string& filename, const std::string& content) {
        std::string path = GetTestDataPath(filename);
        std::ofstream file(path);
        file << content;
        file.close();
    }
};

/**
 * VM test fixture with common VM setup
 */
class VMTestFixture : public BaseTestFixture {
protected:
    // Add VM-specific members here
    // neo::vm::ExecutionEngine* vm;
    
    virtual void InitializeTestEnvironment() override {
        BaseTestFixture::InitializeTestEnvironment();
        // Initialize VM
        // vm = new neo::vm::ExecutionEngine();
    }
    
    virtual void CleanupTestEnvironment() override {
        // Clean up VM
        // delete vm;
        BaseTestFixture::CleanupTestEnvironment();
    }
};

/**
 * Blockchain test fixture with blockchain setup
 */
class BlockchainTestFixture : public BaseTestFixture {
protected:
    // Add blockchain-specific members
    // neo::ledger::Blockchain* blockchain;
    
    virtual void InitializeTestEnvironment() override {
        BaseTestFixture::InitializeTestEnvironment();
        // Initialize blockchain
        // blockchain = new neo::ledger::Blockchain();
    }
    
    virtual void CleanupTestEnvironment() override {
        // Clean up blockchain
        // delete blockchain;
        BaseTestFixture::CleanupTestEnvironment();
    }
};

/**
 * Network test fixture with network setup
 */
class NetworkTestFixture : public BaseTestFixture {
protected:
    // Add network-specific members
    // neo::network::Server* server;
    
    virtual void InitializeTestEnvironment() override {
        BaseTestFixture::InitializeTestEnvironment();
        // Initialize network
        // server = new neo::network::Server();
    }
    
    virtual void CleanupTestEnvironment() override {
        // Clean up network
        // delete server;
        BaseTestFixture::CleanupTestEnvironment();
    }
};

} // namespace test
} // namespace neo
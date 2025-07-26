#pragma once

#include <string>
#include <vector>
#include <memory>
#include <fstream>
#include <filesystem>
#include <gtest/gtest.h>
#include <nlohmann/json.hpp>
#include "neo/vm/execution_engine.h"
#include "neo/vm/script.h"
#include "neo/vm/stack_item.h"

namespace neo::test {

using json = nlohmann::json;

/**
 * @brief JSON-based test runner for VM opcode tests
 * 
 * This class provides a framework for running VM tests defined in JSON files,
 * matching the C# test infrastructure for comprehensive opcode testing.
 */
class JsonTestRunner {
public:
    struct TestCase {
        std::string name;
        std::string script;
        std::vector<StackItem> initialStack;
        std::vector<StackItem> resultStack;
        VMState expectedState;
        std::string exceptionMessage;
        
        bool hasException() const { return !exceptionMessage.empty(); }
    };
    
    /**
     * @brief Load and execute all test files in a directory
     */
    static void RunTestDirectory(const std::string& directory) {
        namespace fs = std::filesystem;
        
        for (const auto& entry : fs::directory_iterator(directory)) {
            if (entry.path().extension() == ".json") {
                RunTestFile(entry.path().string());
            }
        }
    }
    
    /**
     * @brief Load and execute a single JSON test file
     */
    static void RunTestFile(const std::string& filePath) {
        std::ifstream file(filePath);
        if (!file.is_open()) {
            FAIL() << "Failed to open test file: " << filePath;
        }
        
        json testData;
        file >> testData;
        
        if (!testData.contains("tests") || !testData["tests"].is_array()) {
            FAIL() << "Invalid test file format: " << filePath;
        }
        
        for (const auto& test : testData["tests"]) {
            TestCase testCase = ParseTestCase(test);
            ExecuteTestCase(testCase, filePath);
        }
    }
    
private:
    /**
     * @brief Parse a test case from JSON
     */
    static TestCase ParseTestCase(const json& testJson) {
        TestCase testCase;
        
        // Parse test name
        testCase.name = testJson.value("name", "Unnamed Test");
        
        // Parse script (hex encoded)
        testCase.script = testJson.value("script", "");
        
        // Parse initial stack
        if (testJson.contains("initialStack") && testJson["initialStack"].is_array()) {
            for (const auto& item : testJson["initialStack"]) {
                testCase.initialStack.push_back(ParseStackItem(item));
            }
        }
        
        // Parse expected result stack
        if (testJson.contains("resultStack") && testJson["resultStack"].is_array()) {
            for (const auto& item : testJson["resultStack"]) {
                testCase.resultStack.push_back(ParseStackItem(item));
            }
        }
        
        // Parse expected state
        std::string stateStr = testJson.value("state", "HALT");
        testCase.expectedState = ParseVMState(stateStr);
        
        // Parse exception message (if any)
        testCase.exceptionMessage = testJson.value("exception", "");
        
        return testCase;
    }
    
    /**
     * @brief Parse a stack item from JSON
     */
    static StackItem ParseStackItem(const json& itemJson) {
        if (itemJson.is_null()) {
            return StackItem::Null();
        }
        
        if (itemJson.is_boolean()) {
            return StackItem::FromBoolean(itemJson.get<bool>());
        }
        
        if (itemJson.is_number_integer()) {
            return StackItem::FromInteger(itemJson.get<int64_t>());
        }
        
        if (itemJson.is_string()) {
            std::string value = itemJson.get<std::string>();
            
            // Check if it's hex encoded
            if (value.size() >= 2 && value[0] == '0' && (value[1] == 'x' || value[1] == 'X')) {
                return StackItem::FromHexString(value.substr(2));
            }
            
            return StackItem::FromString(value);
        }
        
        if (itemJson.is_array()) {
            std::vector<StackItem> array;
            for (const auto& element : itemJson) {
                array.push_back(ParseStackItem(element));
            }
            return StackItem::FromArray(array);
        }
        
        if (itemJson.is_object()) {
            if (itemJson.contains("type")) {
                std::string type = itemJson["type"];
                
                if (type == "ByteString" && itemJson.contains("value")) {
                    return StackItem::FromHexString(itemJson["value"]);
                }
                
                if (type == "Integer" && itemJson.contains("value")) {
                    return StackItem::FromInteger(itemJson["value"].get<int64_t>());
                }
                
                if (type == "Boolean" && itemJson.contains("value")) {
                    return StackItem::FromBoolean(itemJson["value"].get<bool>());
                }
                
                if (type == "Array" && itemJson.contains("value")) {
                    return ParseStackItem(itemJson["value"]);
                }
                
                if (type == "Map" && itemJson.contains("value")) {
                    std::map<StackItem, StackItem> map;
                    for (const auto& [key, value] : itemJson["value"].items()) {
                        map[StackItem::FromString(key)] = ParseStackItem(value);
                    }
                    return StackItem::FromMap(map);
                }
            }
        }
        
        // Default: treat as null
        return StackItem::Null();
    }
    
    /**
     * @brief Parse VM state from string
     */
    static VMState ParseVMState(const std::string& state) {
        if (state == "HALT") return VMState::HALT;
        if (state == "FAULT") return VMState::FAULT;
        if (state == "BREAK") return VMState::BREAK;
        return VMState::NONE;
    }
    
    /**
     * @brief Execute a single test case
     */
    static void ExecuteTestCase(const TestCase& testCase, const std::string& filePath) {
        SCOPED_TRACE("File: " + filePath + ", Test: " + testCase.name);
        
        try {
            // Create VM
            auto vm = std::make_unique<ExecutionEngine>();
            
            // Load initial stack
            for (const auto& item : testCase.initialStack) {
                vm->Push(item);
            }
            
            // Load and execute script
            Script script = Script::FromHexString(testCase.script);
            vm->LoadScript(script);
            vm->Execute();
            
            // Check final state
            EXPECT_EQ(vm->State(), testCase.expectedState) 
                << "VM state mismatch for test: " << testCase.name;
            
            // If we expect an exception, verify we got one
            if (testCase.hasException()) {
                EXPECT_EQ(vm->State(), VMState::FAULT) 
                    << "Expected FAULT state for test with exception: " << testCase.name;
                
                // TODO: Check specific exception message if available
            } else {
                // Verify result stack
                if (!testCase.resultStack.empty()) {
                    EXPECT_EQ(vm->ResultStack().size(), testCase.resultStack.size())
                        << "Result stack size mismatch for test: " << testCase.name;
                    
                    // Compare each stack item
                    for (size_t i = 0; i < testCase.resultStack.size(); ++i) {
                        auto actual = vm->ResultStack()[i];
                        auto expected = testCase.resultStack[i];
                        
                        EXPECT_TRUE(CompareStackItems(actual, expected))
                            << "Stack item mismatch at index " << i 
                            << " for test: " << testCase.name;
                    }
                }
            }
            
        } catch (const std::exception& e) {
            if (!testCase.hasException()) {
                FAIL() << "Unexpected exception in test '" << testCase.name << "': " << e.what();
            }
            // If we expect an exception, this is success
        }
    }
    
    /**
     * @brief Compare two stack items for equality
     */
    static bool CompareStackItems(const StackItem& a, const StackItem& b) {
        // Implementation would compare stack items based on type and value
        // Uses StackItem's built-in equality comparison
        return a.Equals(b);
    }
};

/**
 * @brief Macro for easily adding JSON test files to test suites
 */
#define RUN_JSON_VM_TEST(test_file_path) \
    TEST(VMOpcodeTests, test_file_path) { \
        JsonTestRunner::RunTestFile(test_file_path); \
    }

/**
 * @brief Macro for running all tests in a directory
 */
#define RUN_JSON_VM_TEST_DIRECTORY(test_dir) \
    TEST(VMOpcodeTests, test_dir) { \
        JsonTestRunner::RunTestDirectory(test_dir); \
    }

} // namespace neo::test
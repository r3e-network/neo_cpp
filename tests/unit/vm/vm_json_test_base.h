#pragma once

#include <filesystem>
#include <fstream>
#include <gtest/gtest.h>
#include <map>
#include <neo/io/json_reader.h>
#include <neo/io/json_writer.h>
#include <neo/vm/debugger.h>
#include <neo/vm/execution_engine.h>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

namespace neo::vm::tests
{
// Enum for VM test action types
enum class VMUTActionType
{
    Execute,
    StepInto,
    StepOut,
    StepOver
};

// Enum for VM test stack item types
enum class VMUTStackItemType
{
    Boolean,
    Integer,
    ByteString,
    Buffer,
    Array,
    Struct,
    Map,
    InteropInterface,
    Pointer,
    Any
};

// Forward declarations
class VMUTStackItem;
class VMUTExecutionContextState;
class VMUTExecutionEngineState;
class VMUTStep;
class VMUTTest;
class VMUT;

// Class for VM test stack item
class VMUTStackItem
{
  public:
    VMUTStackItemType type;
    nlohmann::json value;

    // Convert to StackItem
    std::shared_ptr<StackItem> ToStackItem() const;

    // Convert from StackItem
    static VMUTStackItem FromStackItem(const std::shared_ptr<StackItem>& item);
};

// Class for VM test execution context state
class VMUTExecutionContextState
{
  public:
    int instructionPointer;
    std::vector<VMUTStackItem> evaluationStack;
    std::vector<VMUTStackItem> staticFields;
    std::vector<VMUTStackItem> localVariables;
    std::vector<VMUTStackItem> arguments;
    io::ByteVector script;

    // Compare with actual execution context
    bool Equals(const ExecutionContext& context) const;
};

// Class for VM test execution engine state
class VMUTExecutionEngineState
{
  public:
    VMState state;
    std::vector<VMUTStackItem> resultStack;
    std::vector<VMUTExecutionContextState> invocationStack;
    std::string exceptionMessage;

    // Compare with actual execution engine
    bool Equals(const ExecutionEngine& engine) const;
};

// Class for VM test step
class VMUTStep
{
  public:
    std::string name;
    std::vector<VMUTActionType> actions;
    VMUTExecutionEngineState result;

    // Execute step
    void Execute(ExecutionEngine& engine, Debugger& debugger) const;
};

// Class for VM test
class VMUTTest
{
  public:
    std::string name;
    io::ByteVector script;
    std::vector<VMUTStep> steps;

    // Execute test
    void Execute() const;
};

// Class for VM test
class VMUT
{
  public:
    std::string category;
    std::string name;
    std::vector<VMUTTest> tests;

    // Load from JSON file
    static VMUT LoadFromFile(const std::string& filePath);

    // Execute all tests
    void Execute() const;
};

// Base class for VM JSON tests
class VMJsonTestBase : public ::testing::Test
{
  protected:
    // Execute test from JSON file
    void TestJson(const std::string& path);

    // Assert result
    void AssertResult(const VMUTExecutionEngineState& expected, const ExecutionEngine& engine,
                      const std::string& message);
};
}  // namespace neo::vm::tests

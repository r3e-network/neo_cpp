#include <fstream>
#include <gtest/gtest.h>
#include <neo/io/byte_span.h>
#include <neo/vm/execution_engine.h>
#include <neo/vm/opcode.h>
#include <neo/vm/script.h>
#include <neo/vm/script_builder.h>
#include <neo/vm/stack_item.h>
#include <nlohmann/json.hpp>

using namespace neo::vm;
using namespace neo::io;
using json = nlohmann::json;

class UT_OpCodes_Push : public testing::Test
{
  protected:
    void SetUp() override {}
    void TearDown() override {}

    void RunJsonTest(const std::string& jsonPath)
    {
        std::ifstream file(jsonPath);
        if (!file.is_open())
        {
            GTEST_SKIP() << "Test file not found: " << jsonPath;
            return;
        }

        json testData;
        file >> testData;

        // Parse and execute test based on JSON structure
        // This follows the same pattern as VM JSON tests
        for (const auto& test : testData["tests"])
        {
            ExecutionEngine engine;

            // Load script
            auto scriptHex = test["script"].get<std::string>();
            auto scriptBytes = ParseHex(scriptHex);
            Script script(scriptBytes);
            engine.LoadScript(script);

            // Execute
            auto result = engine.Execute();

            // Verify result
            auto expectedState = test["state"].get<std::string>();
            if (expectedState == "HALT")
            {
                EXPECT_EQ(result, VMState::Halt);
            }
            else if (expectedState == "FAULT")
            {
                EXPECT_EQ(result, VMState::Fault);
            }

            // Verify stack if specified
            if (test.contains("result_stack"))
            {
                // TODO: Implement stack verification
            }
        }
    }

    neo::vm::internal::ByteVector ParseHex(const std::string& hex)
    {
        neo::vm::internal::ByteVector result;
        for (size_t i = 0; i < hex.length(); i += 2)
        {
            std::string byte = hex.substr(i, 2);
            result.Push(static_cast<uint8_t>(std::stoul(byte, nullptr, 16)));
        }
        return result;
    }
};

TEST_F(UT_OpCodes_Push, PUSHA)
{
    // Test for PUSHA opcode
    RunJsonTest("/Users/jinghuiliao/git/r3e/neo_cpp/neo_csharp/tests/Neo.VM.Tests/Tests/OpCodes/Push/PUSHA.json");
}

TEST_F(UT_OpCodes_Push, PUSHM1_to_PUSH16)
{
    // Test for PUSHM1_to_PUSH16 opcode
    RunJsonTest(
        "/Users/jinghuiliao/git/r3e/neo_cpp/neo_csharp/tests/Neo.VM.Tests/Tests/OpCodes/Push/PUSHM1_to_PUSH16.json");
}

TEST_F(UT_OpCodes_Push, PUSHINT8_to_PUSHINT256)
{
    // Test for PUSHINT8_to_PUSHINT256 opcode
    RunJsonTest("/Users/jinghuiliao/git/r3e/neo_cpp/neo_csharp/tests/Neo.VM.Tests/Tests/OpCodes/Push/"
                "PUSHINT8_to_PUSHINT256.json");
}

TEST_F(UT_OpCodes_Push, PUSHDATA1)
{
    // Test for PUSHDATA1 opcode
    RunJsonTest("/Users/jinghuiliao/git/r3e/neo_cpp/neo_csharp/tests/Neo.VM.Tests/Tests/OpCodes/Push/PUSHDATA1.json");
}

TEST_F(UT_OpCodes_Push, PUSHDATA2)
{
    // Test for PUSHDATA2 opcode
    RunJsonTest("/Users/jinghuiliao/git/r3e/neo_cpp/neo_csharp/tests/Neo.VM.Tests/Tests/OpCodes/Push/PUSHDATA2.json");
}

TEST_F(UT_OpCodes_Push, PUSHNULL)
{
    // Test for PUSHNULL opcode
    RunJsonTest("/Users/jinghuiliao/git/r3e/neo_cpp/neo_csharp/tests/Neo.VM.Tests/Tests/OpCodes/Push/PUSHNULL.json");
}

TEST_F(UT_OpCodes_Push, PUSHDATA4)
{
    // Test for PUSHDATA4 opcode
    RunJsonTest("/Users/jinghuiliao/git/r3e/neo_cpp/neo_csharp/tests/Neo.VM.Tests/Tests/OpCodes/Push/PUSHDATA4.json");
}

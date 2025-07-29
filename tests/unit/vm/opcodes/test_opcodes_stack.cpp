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

class UT_OpCodes_Stack : public testing::Test
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

TEST_F(UT_OpCodes_Stack, XDROP)
{
    // Test for XDROP opcode
    RunJsonTest("/Users/jinghuiliao/git/r3e/neo_cpp/neo_csharp/tests/Neo.VM.Tests/Tests/OpCodes/Stack/XDROP.json");
}

TEST_F(UT_OpCodes_Stack, REVERSEN)
{
    // Test for REVERSEN opcode
    RunJsonTest("/Users/jinghuiliao/git/r3e/neo_cpp/neo_csharp/tests/Neo.VM.Tests/Tests/OpCodes/Stack/REVERSEN.json");
}

TEST_F(UT_OpCodes_Stack, REVERSE4)
{
    // Test for REVERSE4 opcode
    RunJsonTest("/Users/jinghuiliao/git/r3e/neo_cpp/neo_csharp/tests/Neo.VM.Tests/Tests/OpCodes/Stack/REVERSE4.json");
}

TEST_F(UT_OpCodes_Stack, CLEAR)
{
    // Test for CLEAR opcode
    RunJsonTest("/Users/jinghuiliao/git/r3e/neo_cpp/neo_csharp/tests/Neo.VM.Tests/Tests/OpCodes/Stack/CLEAR.json");
}

TEST_F(UT_OpCodes_Stack, REVERSE3)
{
    // Test for REVERSE3 opcode
    RunJsonTest("/Users/jinghuiliao/git/r3e/neo_cpp/neo_csharp/tests/Neo.VM.Tests/Tests/OpCodes/Stack/REVERSE3.json");
}

TEST_F(UT_OpCodes_Stack, ROT)
{
    // Test for ROT opcode
    RunJsonTest("/Users/jinghuiliao/git/r3e/neo_cpp/neo_csharp/tests/Neo.VM.Tests/Tests/OpCodes/Stack/ROT.json");
}

TEST_F(UT_OpCodes_Stack, PICK)
{
    // Test for PICK opcode
    RunJsonTest("/Users/jinghuiliao/git/r3e/neo_cpp/neo_csharp/tests/Neo.VM.Tests/Tests/OpCodes/Stack/PICK.json");
}

TEST_F(UT_OpCodes_Stack, NIP)
{
    // Test for NIP opcode
    RunJsonTest("/Users/jinghuiliao/git/r3e/neo_cpp/neo_csharp/tests/Neo.VM.Tests/Tests/OpCodes/Stack/NIP.json");
}

TEST_F(UT_OpCodes_Stack, ROLL)
{
    // Test for ROLL opcode
    RunJsonTest("/Users/jinghuiliao/git/r3e/neo_cpp/neo_csharp/tests/Neo.VM.Tests/Tests/OpCodes/Stack/ROLL.json");
}

TEST_F(UT_OpCodes_Stack, DEPTH)
{
    // Test for DEPTH opcode
    RunJsonTest("/Users/jinghuiliao/git/r3e/neo_cpp/neo_csharp/tests/Neo.VM.Tests/Tests/OpCodes/Stack/DEPTH.json");
}

TEST_F(UT_OpCodes_Stack, SWAP)
{
    // Test for SWAP opcode
    RunJsonTest("/Users/jinghuiliao/git/r3e/neo_cpp/neo_csharp/tests/Neo.VM.Tests/Tests/OpCodes/Stack/SWAP.json");
}

TEST_F(UT_OpCodes_Stack, TUCK)
{
    // Test for TUCK opcode
    RunJsonTest("/Users/jinghuiliao/git/r3e/neo_cpp/neo_csharp/tests/Neo.VM.Tests/Tests/OpCodes/Stack/TUCK.json");
}

TEST_F(UT_OpCodes_Stack, OVER)
{
    // Test for OVER opcode
    RunJsonTest("/Users/jinghuiliao/git/r3e/neo_cpp/neo_csharp/tests/Neo.VM.Tests/Tests/OpCodes/Stack/OVER.json");
}

TEST_F(UT_OpCodes_Stack, DROP)
{
    // Test for DROP opcode
    RunJsonTest("/Users/jinghuiliao/git/r3e/neo_cpp/neo_csharp/tests/Neo.VM.Tests/Tests/OpCodes/Stack/DROP.json");
}

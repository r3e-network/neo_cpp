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

class UT_OpCodes_Arrays : public testing::Test
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

TEST_F(UT_OpCodes_Arrays, CLEARITEMS)
{
    // Test for CLEARITEMS opcode
    RunJsonTest(
        "/Users/jinghuiliao/git/r3e/neo_cpp/neo_csharp/tests/Neo.VM.Tests/Tests/OpCodes/Arrays/CLEARITEMS.json");
}

TEST_F(UT_OpCodes_Arrays, PACK)
{
    // Test for PACK opcode
    RunJsonTest("/Users/jinghuiliao/git/r3e/neo_cpp/neo_csharp/tests/Neo.VM.Tests/Tests/OpCodes/Arrays/PACK.json");
}

TEST_F(UT_OpCodes_Arrays, NEWARRAY_T)
{
    // Test for NEWARRAY_T opcode
    RunJsonTest(
        "/Users/jinghuiliao/git/r3e/neo_cpp/neo_csharp/tests/Neo.VM.Tests/Tests/OpCodes/Arrays/NEWARRAY_T.json");
}

TEST_F(UT_OpCodes_Arrays, PICKITEM)
{
    // Test for PICKITEM opcode
    RunJsonTest("/Users/jinghuiliao/git/r3e/neo_cpp/neo_csharp/tests/Neo.VM.Tests/Tests/OpCodes/Arrays/PICKITEM.json");
}

TEST_F(UT_OpCodes_Arrays, PACKSTRUCT)
{
    // Test for PACKSTRUCT opcode
    RunJsonTest(
        "/Users/jinghuiliao/git/r3e/neo_cpp/neo_csharp/tests/Neo.VM.Tests/Tests/OpCodes/Arrays/PACKSTRUCT.json");
}

TEST_F(UT_OpCodes_Arrays, SETITEM)
{
    // Test for SETITEM opcode
    RunJsonTest("/Users/jinghuiliao/git/r3e/neo_cpp/neo_csharp/tests/Neo.VM.Tests/Tests/OpCodes/Arrays/SETITEM.json");
}

TEST_F(UT_OpCodes_Arrays, NEWSTRUCT0)
{
    // Test for NEWSTRUCT0 opcode
    RunJsonTest(
        "/Users/jinghuiliao/git/r3e/neo_cpp/neo_csharp/tests/Neo.VM.Tests/Tests/OpCodes/Arrays/NEWSTRUCT0.json");
}

TEST_F(UT_OpCodes_Arrays, REVERSEITEMS)
{
    // Test for REVERSEITEMS opcode
    RunJsonTest(
        "/Users/jinghuiliao/git/r3e/neo_cpp/neo_csharp/tests/Neo.VM.Tests/Tests/OpCodes/Arrays/REVERSEITEMS.json");
}

TEST_F(UT_OpCodes_Arrays, NEWMAP)
{
    // Test for NEWMAP opcode
    RunJsonTest("/Users/jinghuiliao/git/r3e/neo_cpp/neo_csharp/tests/Neo.VM.Tests/Tests/OpCodes/Arrays/NEWMAP.json");
}

TEST_F(UT_OpCodes_Arrays, HASKEY)
{
    // Test for HASKEY opcode
    RunJsonTest("/Users/jinghuiliao/git/r3e/neo_cpp/neo_csharp/tests/Neo.VM.Tests/Tests/OpCodes/Arrays/HASKEY.json");
}

TEST_F(UT_OpCodes_Arrays, NEWARRAY)
{
    // Test for NEWARRAY opcode
    RunJsonTest("/Users/jinghuiliao/git/r3e/neo_cpp/neo_csharp/tests/Neo.VM.Tests/Tests/OpCodes/Arrays/NEWARRAY.json");
}

TEST_F(UT_OpCodes_Arrays, KEYS)
{
    // Test for KEYS opcode
    RunJsonTest("/Users/jinghuiliao/git/r3e/neo_cpp/neo_csharp/tests/Neo.VM.Tests/Tests/OpCodes/Arrays/KEYS.json");
}

TEST_F(UT_OpCodes_Arrays, APPEND)
{
    // Test for APPEND opcode
    RunJsonTest("/Users/jinghuiliao/git/r3e/neo_cpp/neo_csharp/tests/Neo.VM.Tests/Tests/OpCodes/Arrays/APPEND.json");
}

TEST_F(UT_OpCodes_Arrays, REMOVE)
{
    // Test for REMOVE opcode
    RunJsonTest("/Users/jinghuiliao/git/r3e/neo_cpp/neo_csharp/tests/Neo.VM.Tests/Tests/OpCodes/Arrays/REMOVE.json");
}

TEST_F(UT_OpCodes_Arrays, PACKMAP)
{
    // Test for PACKMAP opcode
    RunJsonTest("/Users/jinghuiliao/git/r3e/neo_cpp/neo_csharp/tests/Neo.VM.Tests/Tests/OpCodes/Arrays/PACKMAP.json");
}

TEST_F(UT_OpCodes_Arrays, VALUES)
{
    // Test for VALUES opcode
    RunJsonTest("/Users/jinghuiliao/git/r3e/neo_cpp/neo_csharp/tests/Neo.VM.Tests/Tests/OpCodes/Arrays/VALUES.json");
}

TEST_F(UT_OpCodes_Arrays, NEWARRAY0)
{
    // Test for NEWARRAY0 opcode
    RunJsonTest("/Users/jinghuiliao/git/r3e/neo_cpp/neo_csharp/tests/Neo.VM.Tests/Tests/OpCodes/Arrays/NEWARRAY0.json");
}

TEST_F(UT_OpCodes_Arrays, UNPACK)
{
    // Test for UNPACK opcode
    RunJsonTest("/Users/jinghuiliao/git/r3e/neo_cpp/neo_csharp/tests/Neo.VM.Tests/Tests/OpCodes/Arrays/UNPACK.json");
}

TEST_F(UT_OpCodes_Arrays, SIZE)
{
    // Test for SIZE opcode
    RunJsonTest("/Users/jinghuiliao/git/r3e/neo_cpp/neo_csharp/tests/Neo.VM.Tests/Tests/OpCodes/Arrays/SIZE.json");
}

TEST_F(UT_OpCodes_Arrays, NEWSTRUCT)
{
    // Test for NEWSTRUCT opcode
    RunJsonTest("/Users/jinghuiliao/git/r3e/neo_cpp/neo_csharp/tests/Neo.VM.Tests/Tests/OpCodes/Arrays/NEWSTRUCT.json");
}

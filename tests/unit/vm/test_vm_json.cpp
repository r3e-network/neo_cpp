#include "vm_json_test_base.h"
#include <filesystem>
#include <gtest/gtest.h>
#include <neo/vm/debugger.h>
#include <neo/vm/execution_engine.h>

using namespace neo::vm;
using namespace neo::vm::tests;

class UT_VMJson : public VMJsonTestBase
{
  protected:
    void SetUp() override
    {
        // Create test directories if they don't exist
        std::filesystem::create_directories("Tests/Others");
        std::filesystem::create_directories("Tests/OpCodes/Arrays");
        std::filesystem::create_directories("Tests/OpCodes/Stack");
        std::filesystem::create_directories("Tests/OpCodes/Slot");
        std::filesystem::create_directories("Tests/OpCodes/Splice");
        std::filesystem::create_directories("Tests/OpCodes/Control");
        std::filesystem::create_directories("Tests/OpCodes/Push");
        std::filesystem::create_directories("Tests/OpCodes/Arithmetic");
        std::filesystem::create_directories("Tests/OpCodes/BitwiseLogic");
        std::filesystem::create_directories("Tests/OpCodes/Types");
    }
};

TEST_F(UT_VMJson, TestOthers)
{
    TestJson("Tests/Others");
}

TEST_F(UT_VMJson, TestOpCodesArrays)
{
    TestJson("Tests/OpCodes/Arrays");
}

TEST_F(UT_VMJson, TestOpCodesStack)
{
    TestJson("Tests/OpCodes/Stack");
}

TEST_F(UT_VMJson, TestOpCodesSlot)
{
    TestJson("Tests/OpCodes/Slot");
}

TEST_F(UT_VMJson, TestOpCodesSplice)
{
    TestJson("Tests/OpCodes/Splice");
}

TEST_F(UT_VMJson, TestOpCodesControl)
{
    TestJson("Tests/OpCodes/Control");
}

TEST_F(UT_VMJson, TestOpCodesPush)
{
    TestJson("Tests/OpCodes/Push");
}

TEST_F(UT_VMJson, TestOpCodesArithmetic)
{
    TestJson("Tests/OpCodes/Arithmetic");
}

TEST_F(UT_VMJson, TestOpCodesBitwiseLogic)
{
    TestJson("Tests/OpCodes/BitwiseLogic");
}

TEST_F(UT_VMJson, TestOpCodesTypes)
{
    TestJson("Tests/OpCodes/Types");
}

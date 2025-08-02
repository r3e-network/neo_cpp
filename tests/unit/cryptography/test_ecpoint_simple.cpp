#include <gtest/gtest.h>
#include <neo/cryptography/ecc/ecpoint.h>

using namespace neo::cryptography::ecc;

TEST(ECPointSimpleTest, BasicTests)
{
    // Test default construction
    ECPoint point1;
    EXPECT_TRUE(point1.IsInfinity());
    
    // Test curve construction
    ECPoint point2("secp256r1");
    EXPECT_TRUE(point2.IsInfinity());
    EXPECT_EQ(point2.GetCurveName(), "secp256r1");
    
    SUCCEED() << "Basic ECPoint tests pass";
}
#include <gtest/gtest.h>
#include <memory>
#include <neo/wallets/nep6/scrypt_parameters.h>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

using namespace neo::wallets::nep6;
using json = nlohmann::json;

/**
 * @brief Test fixture for ScryptParameters
 */
class ScryptParametersTest : public testing::Test
{
  protected:
    uint32_t testN;
    uint32_t testR;
    uint32_t testP;

    void SetUp() override
    {
        // Initialize test data with typical scrypt parameters
        testN = 16384;  // 2^14, typical value for N
        testR = 8;      // typical value for r
        testP = 1;      // typical value for p
    }
};

TEST_F(ScryptParametersTest, DefaultConstructor)
{
    ScryptParameters params;

    // Default values should be set
    EXPECT_GT(params.GetN(), 0u);
    EXPECT_GT(params.GetR(), 0u);
    EXPECT_GT(params.GetP(), 0u);
}

TEST_F(ScryptParametersTest, ParameterizedConstructor)
{
    ScryptParameters params(testN, testR, testP);

    EXPECT_EQ(testN, params.GetN());
    EXPECT_EQ(testR, params.GetR());
    EXPECT_EQ(testP, params.GetP());
}

TEST_F(ScryptParametersTest, GettersAndSetters)
{
    ScryptParameters params;

    // Test N parameter
    params.SetN(testN);
    EXPECT_EQ(testN, params.GetN());

    // Test R parameter
    params.SetR(testR);
    EXPECT_EQ(testR, params.GetR());

    // Test P parameter
    params.SetP(testP);
    EXPECT_EQ(testP, params.GetP());
}

TEST_F(ScryptParametersTest, DefaultStaticMethod)
{
    ScryptParameters defaultParams = ScryptParameters::Default();

    // Default parameters should be reasonable values
    EXPECT_GT(defaultParams.GetN(), 0u);
    EXPECT_GT(defaultParams.GetR(), 0u);
    EXPECT_GT(defaultParams.GetP(), 0u);

    // Typical defaults for NEP-6
    EXPECT_GE(defaultParams.GetN(), 16384u);  // At least 2^14
    EXPECT_GE(defaultParams.GetR(), 8u);      // At least 8
    EXPECT_GE(defaultParams.GetP(), 1u);      // At least 1
}

TEST_F(ScryptParametersTest, JsonSerialization)
{
    ScryptParameters original(testN, testR, testP);

    // Serialize to JSON
    json jsonObj = original.ToJson();

    // Deserialize from JSON
    ScryptParameters deserialized;
    deserialized.FromJson(jsonObj);

    // Compare
    EXPECT_EQ(original.GetN(), deserialized.GetN());
    EXPECT_EQ(original.GetR(), deserialized.GetR());
    EXPECT_EQ(original.GetP(), deserialized.GetP());
}

TEST_F(ScryptParametersTest, JsonFormat)
{
    ScryptParameters params(testN, testR, testP);

    json jsonObj = params.ToJson();

    // Check that JSON contains expected fields
    EXPECT_TRUE(jsonObj.contains("n"));
    EXPECT_TRUE(jsonObj.contains("r"));
    EXPECT_TRUE(jsonObj.contains("p"));

    // Check values
    EXPECT_EQ(testN, jsonObj["n"].get<uint32_t>());
    EXPECT_EQ(testR, jsonObj["r"].get<uint32_t>());
    EXPECT_EQ(testP, jsonObj["p"].get<uint32_t>());
}

TEST_F(ScryptParametersTest, JsonDeserialization)
{
    // Create JSON manually
    json jsonObj;
    jsonObj["n"] = testN;
    jsonObj["r"] = testR;
    jsonObj["p"] = testP;

    ScryptParameters params;
    params.FromJson(jsonObj);

    EXPECT_EQ(testN, params.GetN());
    EXPECT_EQ(testR, params.GetR());
    EXPECT_EQ(testP, params.GetP());
}

TEST_F(ScryptParametersTest, EdgeCaseValues)
{
    // Test with minimum values
    ScryptParameters minParams(1, 1, 1);
    EXPECT_EQ(1u, minParams.GetN());
    EXPECT_EQ(1u, minParams.GetR());
    EXPECT_EQ(1u, minParams.GetP());

    // Test with maximum values
    uint32_t maxValue = UINT32_MAX;
    ScryptParameters maxParams(maxValue, maxValue, maxValue);
    EXPECT_EQ(maxValue, maxParams.GetN());
    EXPECT_EQ(maxValue, maxParams.GetR());
    EXPECT_EQ(maxValue, maxParams.GetP());
}

TEST_F(ScryptParametersTest, CommonScryptValues)
{
    // Test common scrypt parameter combinations
    struct TestCase
    {
        uint32_t n;
        uint32_t r;
        uint32_t p;
        std::string description;
    };

    std::vector<TestCase> testCases = {
        {16384, 8, 1, "NEP-6 standard"},       // 2^14, 8, 1
        {32768, 8, 1, "Higher security"},      // 2^15, 8, 1
        {4096, 8, 1, "Fast for testing"},      // 2^12, 8, 1
        {1024, 1, 1, "Minimal security"},      // 2^10, 1, 1
        {65536, 8, 8, "High parallelization"}  // 2^16, 8, 8
    };

    for (const auto& tc : testCases)
    {
        ScryptParameters params(tc.n, tc.r, tc.p);
        EXPECT_EQ(tc.n, params.GetN()) << "Failed for: " << tc.description;
        EXPECT_EQ(tc.r, params.GetR()) << "Failed for: " << tc.description;
        EXPECT_EQ(tc.p, params.GetP()) << "Failed for: " << tc.description;
    }
}

TEST_F(ScryptParametersTest, PowerOfTwoN)
{
    // N should typically be a power of 2 for scrypt
    std::vector<uint32_t> powerOfTwo = {1,   2,    4,    8,    16,   32,    64,    128,  256,
                                        512, 1024, 2048, 4096, 8192, 16384, 32768, 65536};

    for (uint32_t n : powerOfTwo)
    {
        ScryptParameters params(n, testR, testP);
        EXPECT_EQ(n, params.GetN());

        // Verify it's actually a power of 2
        EXPECT_EQ(0u, n & (n - 1)) << "Value " << n << " is not a power of 2";
    }
}

TEST_F(ScryptParametersTest, UpdateAfterConstruction)
{
    ScryptParameters params;

    // Update all parameters
    for (uint32_t i = 1; i <= 10; ++i)
    {
        params.SetN(i * 1024);
        params.SetR(i);
        params.SetP(i);

        EXPECT_EQ(i * 1024, params.GetN());
        EXPECT_EQ(i, params.GetR());
        EXPECT_EQ(i, params.GetP());
    }
}

TEST_F(ScryptParametersTest, JsonRoundTrip)
{
    // Test multiple round trips
    ScryptParameters original(testN, testR, testP);

    for (int i = 0; i < 3; ++i)
    {
        json jsonObj = original.ToJson();

        ScryptParameters deserialized;
        deserialized.FromJson(jsonObj);

        // Update original for next iteration
        original = deserialized;

        // Verify consistency
        EXPECT_EQ(testN, deserialized.GetN());
        EXPECT_EQ(testR, deserialized.GetR());
        EXPECT_EQ(testP, deserialized.GetP());
    }
}

TEST_F(ScryptParametersTest, PerformanceImplications)
{
    // Test that parameters have expected relationships for performance
    ScryptParameters lowSecurity(1024, 1, 1);
    ScryptParameters mediumSecurity(16384, 8, 1);
    ScryptParameters highSecurity(65536, 8, 8);

    // Higher N means higher CPU/memory cost
    EXPECT_LT(lowSecurity.GetN(), mediumSecurity.GetN());
    EXPECT_LT(mediumSecurity.GetN(), highSecurity.GetN());

    // Higher R means larger block size
    EXPECT_LE(lowSecurity.GetR(), mediumSecurity.GetR());
    EXPECT_LE(mediumSecurity.GetR(), highSecurity.GetR());

    // Higher P means more parallelization
    EXPECT_LE(lowSecurity.GetP(), mediumSecurity.GetP());
    EXPECT_LE(mediumSecurity.GetP(), highSecurity.GetP());
}

TEST_F(ScryptParametersTest, InvalidJsonHandling)
{
    ScryptParameters params;

    // Test with missing fields
    json incompleteJson;
    incompleteJson["n"] = testN;
    // Missing r and p

    // FromJson should handle gracefully (implementation-dependent)
    EXPECT_NO_THROW(params.FromJson(incompleteJson));

    // Test with wrong types
    json wrongTypeJson;
    wrongTypeJson["n"] = "not_a_number";
    wrongTypeJson["r"] = testR;
    wrongTypeJson["p"] = testP;

    EXPECT_THROW(params.FromJson(wrongTypeJson), nlohmann::json::type_error);
}

TEST_F(ScryptParametersTest, NEP6Compliance)
{
    // Test NEP-6 standard parameters
    ScryptParameters nep6Standard = ScryptParameters::Default();

    // NEP-6 typically uses N=16384, r=8, p=1
    // These are reasonable defaults for wallet encryption
    EXPECT_GE(nep6Standard.GetN(), 16384u);
    EXPECT_GE(nep6Standard.GetR(), 8u);
    EXPECT_GE(nep6Standard.GetP(), 1u);

    // Verify JSON serialization matches expected format
    json jsonObj = nep6Standard.ToJson();
    EXPECT_TRUE(jsonObj.is_object());
    EXPECT_TRUE(jsonObj.contains("n"));
    EXPECT_TRUE(jsonObj.contains("r"));
    EXPECT_TRUE(jsonObj.contains("p"));
}

#include <gtest/gtest.h>
#include <memory>
#include <neo/io/uint160.h>
#include <neo/persistence/memory_store.h>
#include <neo/persistence/store_view.h>
#include <neo/smartcontract/application_engine.h>
#include <neo/smartcontract/native/policy_contract.h>
#include <neo/vm/stack_item.h>
#include <string>
#include <vector>

using namespace neo::smartcontract::native;
using namespace neo::smartcontract;
using namespace neo::persistence;
using namespace neo::io;
using namespace neo::vm;

class UT_PolicyContract : public testing::Test
{
  protected:
    void SetUp() override
    {
        // Create memory store for testing
        memoryStore_ = std::make_shared<MemoryStore>();
        storeView_ = std::make_shared<StoreView>(memoryStore_);

        // Get PolicyContract instance
        policyContract_ = PolicyContract::GetInstance();

        // Create test accounts
        testAccount1_ = UInt160::Parse("0x1234567890123456789012345678901234567890");
        testAccount2_ = UInt160::Parse("0xabcdefabcdefabcdefabcdefabcdefabcdefabcd");
        testAccount3_ = UInt160::Parse("0x1111111111111111111111111111111111111111");
    }

    void TearDown() override
    {
        // Cleanup
        storeView_.reset();
        memoryStore_.reset();
    }

  protected:
    std::shared_ptr<MemoryStore> memoryStore_;
    std::shared_ptr<StoreView> storeView_;
    std::shared_ptr<PolicyContract> policyContract_;
    UInt160 testAccount1_;
    UInt160 testAccount2_;
    UInt160 testAccount3_;
};

TEST_F(UT_PolicyContract, ConstructorAndConstants)
{
    // Test: Verify PolicyContract constructor and constants

    // Verify contract constants
    EXPECT_EQ(PolicyContract::ID, -7);
    EXPECT_STREQ(PolicyContract::NAME, "PolicyContract");

    // Verify storage prefixes
    EXPECT_EQ(PolicyContract::PREFIX_BLOCKED_ACCOUNT, 15);
    EXPECT_EQ(PolicyContract::PREFIX_FEE_PER_BYTE, 10);
    EXPECT_EQ(PolicyContract::PREFIX_EXEC_FEE_FACTOR, 18);
    EXPECT_EQ(PolicyContract::PREFIX_STORAGE_PRICE, 19);
    EXPECT_EQ(PolicyContract::PREFIX_ATTRIBUTE_FEE, 20);
    EXPECT_EQ(PolicyContract::PREFIX_MILLISECONDS_PER_BLOCK, 21);
    EXPECT_EQ(PolicyContract::PREFIX_MAX_VALID_UNTIL_BLOCK_INCREMENT, 22);
    EXPECT_EQ(PolicyContract::PREFIX_MAX_TRACEABLE_BLOCKS, 23);

    // Verify default values
    EXPECT_EQ(PolicyContract::DEFAULT_FEE_PER_BYTE, 1000u);
    EXPECT_EQ(PolicyContract::DEFAULT_EXEC_FEE_FACTOR, 30u);
    EXPECT_EQ(PolicyContract::DEFAULT_STORAGE_PRICE, 100000u);
    EXPECT_EQ(PolicyContract::DEFAULT_ATTRIBUTE_FEE, 0u);
    EXPECT_EQ(PolicyContract::DEFAULT_NOTARY_ASSISTED_ATTRIBUTE_FEE, 1000'0000u);

    // Verify maximum values
    EXPECT_EQ(PolicyContract::MAX_EXEC_FEE_FACTOR, 100u);
    EXPECT_EQ(PolicyContract::MAX_ATTRIBUTE_FEE, 10'0000'0000u);
    EXPECT_EQ(PolicyContract::MAX_STORAGE_PRICE, 10000000u);
    EXPECT_EQ(PolicyContract::MAX_MILLISECONDS_PER_BLOCK, 30000u);
    EXPECT_EQ(PolicyContract::MAX_MAX_VALID_UNTIL_BLOCK_INCREMENT, 86400u);
    EXPECT_EQ(PolicyContract::MAX_MAX_TRACEABLE_BLOCKS, 2102400u);

    // Verify event name
    EXPECT_STREQ(PolicyContract::MILLISECONDS_PER_BLOCK_CHANGED_EVENT, "MillisecondsPerBlockChanged");
}

TEST_F(UT_PolicyContract, SingletonInstance)
{
    // Test: Verify PolicyContract is a singleton

    auto instance1 = PolicyContract::GetInstance();
    auto instance2 = PolicyContract::GetInstance();

    // Both instances should be the same
    EXPECT_EQ(instance1, instance2);
    EXPECT_NE(instance1, nullptr);
    EXPECT_NE(instance2, nullptr);
}

TEST_F(UT_PolicyContract, GetFeePerByte_DefaultValue)
{
    // Test: Get default fee per byte value

    int64_t feePerByte = policyContract_->GetFeePerByte(storeView_);

    // Should return default value when not set
    EXPECT_EQ(feePerByte, PolicyContract::DEFAULT_FEE_PER_BYTE);
    EXPECT_GT(feePerByte, 0);  // Should be positive
}

TEST_F(UT_PolicyContract, GetExecFeeFactor_DefaultValue)
{
    // Test: Get default execution fee factor value

    uint32_t execFeeFactor = policyContract_->GetExecFeeFactor(storeView_);

    // Should return default value when not set
    EXPECT_EQ(execFeeFactor, PolicyContract::DEFAULT_EXEC_FEE_FACTOR);
    EXPECT_GT(execFeeFactor, 0u);                                   // Should be positive
    EXPECT_LE(execFeeFactor, PolicyContract::MAX_EXEC_FEE_FACTOR);  // Should be within limits
}

TEST_F(UT_PolicyContract, GetStoragePrice_DefaultValue)
{
    // Test: Get default storage price value

    uint32_t storagePrice = policyContract_->GetStoragePrice(storeView_);

    // Should return default value when not set
    EXPECT_EQ(storagePrice, PolicyContract::DEFAULT_STORAGE_PRICE);
    EXPECT_GT(storagePrice, 0u);                                 // Should be positive
    EXPECT_LE(storagePrice, PolicyContract::MAX_STORAGE_PRICE);  // Should be within limits
}

TEST_F(UT_PolicyContract, IsBlocked_DefaultBehavior)
{
    // Test: Check if accounts are blocked by default

    // Test with various accounts - none should be blocked initially
    EXPECT_FALSE(policyContract_->IsBlocked(storeView_, testAccount1_));
    EXPECT_FALSE(policyContract_->IsBlocked(storeView_, testAccount2_));
    EXPECT_FALSE(policyContract_->IsBlocked(storeView_, testAccount3_));

    // Test with zero account
    UInt160 zeroAccount = UInt160::Zero();
    EXPECT_FALSE(policyContract_->IsBlocked(storeView_, zeroAccount));
}

TEST_F(UT_PolicyContract, GetAttributeFee_DefaultValues)
{
    // Test: Get attribute fee for different types

    // Test default attribute fee (type 0)
    uint32_t defaultFee = policyContract_->GetAttributeFee(storeView_, 0);
    EXPECT_EQ(defaultFee, PolicyContract::DEFAULT_ATTRIBUTE_FEE);

    // Test various attribute types
    for (uint8_t attrType = 1; attrType <= 10; ++attrType)
    {
        uint32_t fee = policyContract_->GetAttributeFee(storeView_, attrType);
        EXPECT_LE(fee, PolicyContract::MAX_ATTRIBUTE_FEE);  // Should be within limits
    }

    // Test edge cases
    uint32_t maxTypeFee = policyContract_->GetAttributeFee(storeView_, 255);
    EXPECT_LE(maxTypeFee, PolicyContract::MAX_ATTRIBUTE_FEE);
}

TEST_F(UT_PolicyContract, GetMillisecondsPerBlock_DefaultValue)
{
    // Test: Get default milliseconds per block

    uint32_t millisecondsPerBlock = policyContract_->GetMillisecondsPerBlock(storeView_);

    // Should return a reasonable default value
    EXPECT_GT(millisecondsPerBlock, 0u);                                          // Should be positive
    EXPECT_LE(millisecondsPerBlock, PolicyContract::MAX_MILLISECONDS_PER_BLOCK);  // Should be within limits

    // Typical block time should be reasonable (between 5-30 seconds)
    EXPECT_GE(millisecondsPerBlock, 5000u);   // At least 5 seconds
    EXPECT_LE(millisecondsPerBlock, 30000u);  // At most 30 seconds
}

TEST_F(UT_PolicyContract, GetMaxValidUntilBlockIncrement_DefaultValue)
{
    // Test: Get default max valid until block increment

    uint32_t maxValidUntilBlockIncrement = policyContract_->GetMaxValidUntilBlockIncrement(storeView_);

    // Should return a reasonable default value
    EXPECT_GT(maxValidUntilBlockIncrement, 0u);  // Should be positive
    EXPECT_LE(maxValidUntilBlockIncrement,
              PolicyContract::MAX_MAX_VALID_UNTIL_BLOCK_INCREMENT);  // Should be within limits

    // Should be reasonable for transaction validity period
    EXPECT_GE(maxValidUntilBlockIncrement, 240u);  // At least 240 blocks (1 hour with 15s blocks)
}

TEST_F(UT_PolicyContract, GetMaxTraceableBlocks_DefaultValue)
{
    // Test: Get default max traceable blocks

    uint32_t maxTraceableBlocks = policyContract_->GetMaxTraceableBlocks(storeView_);

    // Should return a reasonable default value
    EXPECT_GT(maxTraceableBlocks, 0u);                                        // Should be positive
    EXPECT_LE(maxTraceableBlocks, PolicyContract::MAX_MAX_TRACEABLE_BLOCKS);  // Should be within limits

    // Should be reasonable for blockchain history tracking
    EXPECT_GE(maxTraceableBlocks, 86400u);  // At least 86400 blocks (approximately 15 days with 15s blocks)
}

TEST_F(UT_PolicyContract, ConstantValidation)
{
    // Test: Validate that constants are within expected ranges

    // Verify default values are within maximum limits
    EXPECT_LE(PolicyContract::DEFAULT_EXEC_FEE_FACTOR, PolicyContract::MAX_EXEC_FEE_FACTOR);
    EXPECT_LE(PolicyContract::DEFAULT_STORAGE_PRICE, PolicyContract::MAX_STORAGE_PRICE);
    EXPECT_LE(PolicyContract::DEFAULT_ATTRIBUTE_FEE, PolicyContract::MAX_ATTRIBUTE_FEE);
    EXPECT_LE(PolicyContract::DEFAULT_NOTARY_ASSISTED_ATTRIBUTE_FEE, PolicyContract::MAX_ATTRIBUTE_FEE);

    // Verify storage prefixes are unique
    std::vector<uint8_t> prefixes = {PolicyContract::PREFIX_BLOCKED_ACCOUNT,
                                     PolicyContract::PREFIX_FEE_PER_BYTE,
                                     PolicyContract::PREFIX_EXEC_FEE_FACTOR,
                                     PolicyContract::PREFIX_STORAGE_PRICE,
                                     PolicyContract::PREFIX_ATTRIBUTE_FEE,
                                     PolicyContract::PREFIX_MILLISECONDS_PER_BLOCK,
                                     PolicyContract::PREFIX_MAX_VALID_UNTIL_BLOCK_INCREMENT,
                                     PolicyContract::PREFIX_MAX_TRACEABLE_BLOCKS};

    // Check for uniqueness
    for (size_t i = 0; i < prefixes.size(); ++i)
    {
        for (size_t j = i + 1; j < prefixes.size(); ++j)
        {
            EXPECT_NE(prefixes[i], prefixes[j]) << "Duplicate prefix found at indices " << i << " and " << j;
        }
    }
}

TEST_F(UT_PolicyContract, ContractID_Validation)
{
    // Test: Verify contract ID is in the correct range for native contracts

    // Native contracts should have negative IDs
    EXPECT_LT(PolicyContract::ID, 0);

    // Should be a reasonable native contract ID
    EXPECT_GE(PolicyContract::ID, -10);  // Within reasonable range for native contracts
}

TEST_F(UT_PolicyContract, DefaultValues_ConsistencyCheck)
{
    // Test: Verify default values are consistent with Neo network expectations

    // Fee per byte should be reasonable (in datoshi, 1 datoshi = 1e-8 GAS)
    EXPECT_GE(PolicyContract::DEFAULT_FEE_PER_BYTE, 100u);    // At least 100 datoshi
    EXPECT_LE(PolicyContract::DEFAULT_FEE_PER_BYTE, 10000u);  // At most 10000 datoshi

    // Execution fee factor should be reasonable multiplier
    EXPECT_GE(PolicyContract::DEFAULT_EXEC_FEE_FACTOR, 1u);    // At least 1x
    EXPECT_LE(PolicyContract::DEFAULT_EXEC_FEE_FACTOR, 100u);  // At most 100x

    // Storage price should be reasonable for per-byte storage cost
    EXPECT_GE(PolicyContract::DEFAULT_STORAGE_PRICE, 1000u);     // At least 1000 datoshi per byte
    EXPECT_LE(PolicyContract::DEFAULT_STORAGE_PRICE, 1000000u);  // At most 1M datoshi per byte

    // Notary assisted fee should be significantly higher than default
    EXPECT_GT(PolicyContract::DEFAULT_NOTARY_ASSISTED_ATTRIBUTE_FEE, PolicyContract::DEFAULT_ATTRIBUTE_FEE);
    EXPECT_GE(PolicyContract::DEFAULT_NOTARY_ASSISTED_ATTRIBUTE_FEE, 1000000u);  // At least 1M datoshi
}

TEST_F(UT_PolicyContract, MaximumValues_BoundaryTesting)
{
    // Test: Verify maximum values represent reasonable upper bounds

    // Max exec fee factor should allow reasonable multipliers
    EXPECT_GE(PolicyContract::MAX_EXEC_FEE_FACTOR, 10u);    // At least 10x multiplier
    EXPECT_LE(PolicyContract::MAX_EXEC_FEE_FACTOR, 1000u);  // At most 1000x multiplier

    // Max attribute fee should be reasonable upper bound
    EXPECT_GE(PolicyContract::MAX_ATTRIBUTE_FEE, 1000'0000u);      // At least 10M datoshi
    EXPECT_LE(PolicyContract::MAX_ATTRIBUTE_FEE, 100'0000'0000u);  // At most 100B datoshi

    // Max storage price should allow reasonable storage costs
    EXPECT_GE(PolicyContract::MAX_STORAGE_PRICE, 100000u);     // At least 100K datoshi per byte
    EXPECT_LE(PolicyContract::MAX_STORAGE_PRICE, 100000000u);  // At most 100M datoshi per byte

    // Max milliseconds per block should represent reasonable block times
    EXPECT_GE(PolicyContract::MAX_MILLISECONDS_PER_BLOCK, 1000u);    // At least 1 second
    EXPECT_LE(PolicyContract::MAX_MILLISECONDS_PER_BLOCK, 300000u);  // At most 5 minutes

    // Max valid until block increment should allow reasonable transaction windows
    EXPECT_GE(PolicyContract::MAX_MAX_VALID_UNTIL_BLOCK_INCREMENT, 240u);      // At least 1 hour (240 blocks)
    EXPECT_LE(PolicyContract::MAX_MAX_VALID_UNTIL_BLOCK_INCREMENT, 5760000u);  // At most reasonable limit

    // Max traceable blocks should allow reasonable history depth
    EXPECT_GE(PolicyContract::MAX_MAX_TRACEABLE_BLOCKS, 5760u);      // At least 1 day (5760 blocks)
    EXPECT_LE(PolicyContract::MAX_MAX_TRACEABLE_BLOCKS, 10000000u);  // At most reasonable limit
}

TEST_F(UT_PolicyContract, MethodInterface_Validation)
{
    // Test: Verify all public methods are accessible and return expected types

    // Test getter methods don't throw with valid parameters
    EXPECT_NO_THROW({
        policyContract_->GetFeePerByte(storeView_);
        policyContract_->GetExecFeeFactor(storeView_);
        policyContract_->GetStoragePrice(storeView_);
        policyContract_->GetMillisecondsPerBlock(storeView_);
        policyContract_->GetMaxValidUntilBlockIncrement(storeView_);
        policyContract_->GetMaxTraceableBlocks(storeView_);
    });

    // Test IsBlocked method with various accounts
    EXPECT_NO_THROW({
        policyContract_->IsBlocked(storeView_, testAccount1_);
        policyContract_->IsBlocked(storeView_, testAccount2_);
        policyContract_->IsBlocked(storeView_, UInt160::Zero());
    });

    // Test GetAttributeFee with various types
    EXPECT_NO_THROW({
        policyContract_->GetAttributeFee(storeView_, 0);
        policyContract_->GetAttributeFee(storeView_, 1);
        policyContract_->GetAttributeFee(storeView_, 255);
    });
}

TEST_F(UT_PolicyContract, StoreView_NullHandling)
{
    // Test: Verify behavior with null store view

    std::shared_ptr<StoreView> nullStore = nullptr;

    // Methods should handle null store gracefully (either throw or return defaults)
    // The exact behavior depends on implementation, but should not crash
    EXPECT_NO_THROW({
        try
        {
            policyContract_->GetFeePerByte(nullStore);
        }
        catch (const std::exception&)
        {
            // Exception is acceptable for null store
        }
    });

    EXPECT_NO_THROW({
        try
        {
            policyContract_->IsBlocked(nullStore, testAccount1_);
        }
        catch (const std::exception&)
        {
            // Exception is acceptable for null store
        }
    });
}

TEST_F(UT_PolicyContract, UInt160_EdgeCases)
{
    // Test: Test account blocking with various UInt160 edge cases

    // Test with various account formats
    std::vector<UInt160> testAccounts = {
        UInt160::Zero(),                                               // All zeros
        UInt160::Parse("0xffffffffffffffffffffffffffffffffffffffff"),  // All ones
        UInt160::Parse("0x0000000000000000000000000000000000000001"),  // Minimal non-zero
        testAccount1_,
        testAccount2_,
        testAccount3_};

    for (const auto& account : testAccounts)
    {
        // Should not throw and should return false by default
        EXPECT_NO_THROW({
            bool isBlocked = policyContract_->IsBlocked(storeView_, account);
            EXPECT_FALSE(isBlocked);  // Default should be not blocked
        });
    }
}

TEST_F(UT_PolicyContract, ContractName_Validation)
{
    // Test: Verify contract name follows expected format

    std::string contractName = PolicyContract::NAME;

    // Should not be empty
    EXPECT_FALSE(contractName.empty());

    // Should be reasonable length
    EXPECT_GE(contractName.length(), 5u);
    EXPECT_LE(contractName.length(), 50u);

    // Should contain "Policy" and "Contract"
    EXPECT_NE(contractName.find("Policy"), std::string::npos);
    EXPECT_NE(contractName.find("Contract"), std::string::npos);

    // Should not contain invalid characters for contract names
    for (char c : contractName)
    {
        EXPECT_TRUE(std::isalnum(c) || c == '_') << "Invalid character in contract name: " << c;
    }
}
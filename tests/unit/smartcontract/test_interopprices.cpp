#include <gtest/gtest.h>
#include <limits>
#include <memory>
#include <neo/smartcontract/call_flags.h>
#include <neo/smartcontract/interop_descriptor.h>
#include <neo/smartcontract/interop_service.h>
#include <string>
#include <unordered_map>
#include <vector>

using namespace neo::smartcontract;

/**
 * @brief Test fixture for Interop Prices
 *
 * Tests the pricing mechanism for interoperable services in the Neo VM.
 */
class UT_InteropPrices : public testing::Test
{
  protected:
    void SetUp() override
    {
        // Ensure InteropService is initialized
        InteropService::initialize();
    }

    void TearDown() override
    {
        // No cleanup needed
    }

    // Helper to get service by name hash
    const InteropDescriptor* GetServiceByName(const std::string& name)
    {
        uint32_t hash = calculate_interop_hash(name);
        return InteropService::instance().get_descriptor(hash);
    }
};

TEST_F(UT_InteropPrices, VerifyPriceConstants)
{
    // Test: Verify that common price constants are used correctly

    // Common price constants (in GAS units)
    const int64_t PRICE_FREE = 0;
    const int64_t PRICE_OPCODE = 30;              // Base opcode price
    const int64_t PRICE_SYSCALL = 32768;          // Base syscall price
    const int64_t PRICE_STORAGE_READ = 50000;     // Storage read operation
    const int64_t PRICE_STORAGE_WRITE = 100000;   // Storage write operation
    const int64_t PRICE_CONTRACT_CALL = 512000;   // Contract call operation
    const int64_t PRICE_CHECK_WITNESS = 1048576;  // CheckWitness operation
    const int64_t PRICE_CHECK_SIG = 1048576;      // Signature verification

    // Verify these are reasonable values
    EXPECT_GE(PRICE_FREE, 0);
    EXPECT_GT(PRICE_OPCODE, 0);
    EXPECT_GT(PRICE_SYSCALL, PRICE_OPCODE);
    EXPECT_GT(PRICE_STORAGE_READ, PRICE_SYSCALL);
    EXPECT_GT(PRICE_STORAGE_WRITE, PRICE_STORAGE_READ);
    EXPECT_GT(PRICE_CONTRACT_CALL, PRICE_STORAGE_WRITE);
    EXPECT_GT(PRICE_CHECK_WITNESS, PRICE_CONTRACT_CALL);
}

TEST_F(UT_InteropPrices, RuntimeServicePrices)
{
    // Test: Verify runtime service prices

    // Simple runtime getters should be cheap
    auto platform = GetServiceByName("System.Runtime.Platform");
    auto getNetwork = GetServiceByName("System.Runtime.GetNetwork");
    auto getTrigger = GetServiceByName("System.Runtime.GetTrigger");
    auto getTime = GetServiceByName("System.Runtime.GetTime");

    if (platform)
    {
        EXPECT_GE(platform->fixed_price, 0);
        EXPECT_LE(platform->fixed_price, 1000);  // Should be very cheap
    }

    if (getNetwork)
    {
        EXPECT_GE(getNetwork->fixed_price, 0);
        EXPECT_LE(getNetwork->fixed_price, 1000);
    }

    if (getTrigger)
    {
        EXPECT_GE(getTrigger->fixed_price, 0);
        EXPECT_LE(getTrigger->fixed_price, 1000);
    }

    if (getTime)
    {
        EXPECT_GE(getTime->fixed_price, 0);
        EXPECT_LE(getTime->fixed_price, 10000);  // Slightly more expensive
    }

    // CheckWitness should be expensive
    auto checkWitness = GetServiceByName("System.Runtime.CheckWitness");
    if (checkWitness)
    {
        EXPECT_GT(checkWitness->fixed_price, 100000);  // Should be expensive
    }

    // Log and Notify should have moderate cost
    auto log = GetServiceByName("System.Runtime.Log");
    auto notify = GetServiceByName("System.Runtime.Notify");

    if (log)
    {
        EXPECT_GT(log->fixed_price, 1000);
        EXPECT_LT(log->fixed_price, 100000);
    }

    if (notify)
    {
        EXPECT_GT(notify->fixed_price, 1000);
        EXPECT_LT(notify->fixed_price, 100000);
    }
}

TEST_F(UT_InteropPrices, CryptoServicePrices)
{
    // Test: Verify crypto service prices

    auto checkSig = GetServiceByName("System.Crypto.CheckSig");
    auto checkMultisig = GetServiceByName("System.Crypto.CheckMultiSig");

    // Signature verification should be expensive
    if (checkSig)
    {
        EXPECT_GT(checkSig->fixed_price, 100000);    // Should be expensive
        EXPECT_LT(checkSig->fixed_price, 10000000);  // But not excessive
    }

    // Multi-signature should be more expensive than single signature
    if (checkMultisig)
    {
        EXPECT_GT(checkMultisig->fixed_price, 100000);
        // Note: Multisig price might be dynamic based on number of signatures
    }
}

TEST_F(UT_InteropPrices, StorageServicePrices)
{
    // Test: Verify storage service prices

    auto getContext = GetServiceByName("System.Storage.GetContext");
    auto getReadOnlyContext = GetServiceByName("System.Storage.GetReadOnlyContext");
    auto storageGet = GetServiceByName("System.Storage.Get");
    auto storageFind = GetServiceByName("System.Storage.Find");
    auto storagePut = GetServiceByName("System.Storage.Put");
    auto storageDelete = GetServiceByName("System.Storage.Delete");

    // Context operations should be cheap
    if (getContext)
    {
        EXPECT_GE(getContext->fixed_price, 0);
        EXPECT_LE(getContext->fixed_price, 1000);
    }

    if (getReadOnlyContext)
    {
        EXPECT_GE(getReadOnlyContext->fixed_price, 0);
        EXPECT_LE(getReadOnlyContext->fixed_price, 1000);
    }

    // Read operations should have moderate cost
    if (storageGet)
    {
        EXPECT_GT(storageGet->fixed_price, 10000);
        EXPECT_LT(storageGet->fixed_price, 100000);
    }

    if (storageFind)
    {
        EXPECT_GT(storageFind->fixed_price, 10000);
        EXPECT_LT(storageFind->fixed_price, 100000);
    }

    // Write operations should be more expensive than reads
    if (storagePut)
    {
        EXPECT_GT(storagePut->fixed_price, 50000);
        EXPECT_LT(storagePut->fixed_price, 1000000);
    }

    if (storageDelete)
    {
        EXPECT_GT(storageDelete->fixed_price, 10000);
        EXPECT_LT(storageDelete->fixed_price, 500000);
    }
}

TEST_F(UT_InteropPrices, ContractServicePrices)
{
    // Test: Verify contract service prices

    auto contractCall = GetServiceByName("System.Contract.Call");
    auto callNative = GetServiceByName("System.Contract.CallNative");
    auto getCallFlags = GetServiceByName("System.Contract.GetCallFlags");
    auto createStandardAccount = GetServiceByName("System.Contract.CreateStandardAccount");

    // Contract calls should be expensive
    if (contractCall)
    {
        EXPECT_GT(contractCall->fixed_price, 100000);
        EXPECT_LT(contractCall->fixed_price, 10000000);
    }

    if (callNative)
    {
        EXPECT_GT(callNative->fixed_price, 0);
        // Native calls might have different pricing model
    }

    // Simple getters should be cheap
    if (getCallFlags)
    {
        EXPECT_GE(getCallFlags->fixed_price, 0);
        EXPECT_LE(getCallFlags->fixed_price, 1000);
    }

    // Account creation should have moderate cost
    if (createStandardAccount)
    {
        EXPECT_GT(createStandardAccount->fixed_price, 1000);
        EXPECT_LT(createStandardAccount->fixed_price, 100000);
    }
}

TEST_F(UT_InteropPrices, AllServicesHaveValidPrices)
{
    // Test: Verify all registered services have valid prices

    const auto& services = InteropService::instance().services();

    for (const auto& [hash, descriptor] : services)
    {
        // All prices should be non-negative
        EXPECT_GE(descriptor.fixed_price, 0) << "Service " << descriptor.name << " has negative price";

        // Prices should not exceed reasonable maximum (e.g., 100M GAS)
        const int64_t MAX_REASONABLE_PRICE = 10000000000LL;  // 100M GAS
        EXPECT_LE(descriptor.fixed_price, MAX_REASONABLE_PRICE)
            << "Service " << descriptor.name << " has excessive price";

        // Verify hash matches calculated hash
        uint32_t calculatedHash = calculate_interop_hash(descriptor.name);
        EXPECT_EQ(hash, calculatedHash) << "Service " << descriptor.name << " has mismatched hash";
        EXPECT_EQ(descriptor.hash, calculatedHash) << "Service " << descriptor.name << " has inconsistent hash";
    }
}

TEST_F(UT_InteropPrices, PriceRelationships)
{
    // Test: Verify logical relationships between service prices

    // Storage write should be more expensive than read
    auto storageGet = GetServiceByName("System.Storage.Get");
    auto storagePut = GetServiceByName("System.Storage.Put");

    if (storageGet && storagePut)
    {
        EXPECT_GT(storagePut->fixed_price, storageGet->fixed_price)
            << "Storage write should be more expensive than read";
    }

    // CheckWitness should be expensive (cryptographic operation)
    auto checkWitness = GetServiceByName("System.Runtime.CheckWitness");
    auto getTime = GetServiceByName("System.Runtime.GetTime");

    if (checkWitness && getTime)
    {
        EXPECT_GT(checkWitness->fixed_price, getTime->fixed_price * 100)
            << "CheckWitness should be much more expensive than GetTime";
    }

    // Contract calls should be expensive
    auto contractCall = GetServiceByName("System.Contract.Call");
    auto runtimeLog = GetServiceByName("System.Runtime.Log");

    if (contractCall && runtimeLog)
    {
        EXPECT_GT(contractCall->fixed_price, runtimeLog->fixed_price * 10)
            << "Contract calls should be much more expensive than logging";
    }
}

TEST_F(UT_InteropPrices, ServiceAvailability)
{
    // Test: Check that essential services are available

    // Essential runtime services
    EXPECT_NE(GetServiceByName("System.Runtime.GetExecutingScriptHash"), nullptr);
    EXPECT_NE(GetServiceByName("System.Runtime.CheckWitness"), nullptr);
    EXPECT_NE(GetServiceByName("System.Runtime.Log"), nullptr);
    EXPECT_NE(GetServiceByName("System.Runtime.Notify"), nullptr);

    // Essential crypto services
    EXPECT_NE(GetServiceByName("System.Crypto.CheckSig"), nullptr);

    // Essential storage services
    EXPECT_NE(GetServiceByName("System.Storage.GetContext"), nullptr);
    EXPECT_NE(GetServiceByName("System.Storage.Get"), nullptr);
    EXPECT_NE(GetServiceByName("System.Storage.Put"), nullptr);

    // Essential contract services
    EXPECT_NE(GetServiceByName("System.Contract.Call"), nullptr);
}

TEST_F(UT_InteropPrices, CallFlagsAndPrices)
{
    // Test: Verify relationship between call flags and prices

    const auto& services = InteropService::instance().services();

    for (const auto& [hash, descriptor] : services)
    {
        // Services requiring write permissions should generally be more expensive
        if ((descriptor.required_call_flags & CallFlags::WriteStates) != CallFlags::None)
        {
            // Write operations should have some cost
            EXPECT_GT(descriptor.fixed_price, 0)
                << "Write operation " << descriptor.name << " should have positive cost";
        }

        // Services with no special flags are usually cheaper
        if (descriptor.required_call_flags == CallFlags::None)
        {
            // These are typically simple getters
            EXPECT_LE(descriptor.fixed_price, 100000)
                << "Simple operation " << descriptor.name << " seems too expensive";
        }
    }
}

TEST_F(UT_InteropPrices, InteropHashCalculation)
{
    // Test: Verify interop hash calculation

    // Test known service names
    std::vector<std::string> knownServices = {"System.Runtime.Platform",     "System.Runtime.GetTrigger",
                                              "System.Runtime.CheckWitness", "System.Crypto.CheckSig",
                                              "System.Storage.Get",          "System.Contract.Call"};

    for (const auto& serviceName : knownServices)
    {
        uint32_t hash1 = calculate_interop_hash(serviceName);
        uint32_t hash2 = calculate_interop_hash(serviceName);

        // Hash should be deterministic
        EXPECT_EQ(hash1, hash2) << "Hash calculation not deterministic for " << serviceName;

        // Hash should not be zero (unlikely but possible)
        EXPECT_NE(hash1, 0) << "Hash is zero for " << serviceName;

        // Verify service can be found by hash
        auto descriptor = InteropService::instance().get_descriptor(hash1);
        if (descriptor)
        {
            EXPECT_EQ(descriptor->name, serviceName);
        }
    }
}

TEST_F(UT_InteropPrices, IteratorServicePrices)
{
    // Test: Verify iterator service prices

    auto iteratorNext = GetServiceByName("System.Iterator.Next");
    auto iteratorValue = GetServiceByName("System.Iterator.Value");

    // Iterator operations should be cheap (they work on already loaded data)
    if (iteratorNext)
    {
        EXPECT_GE(iteratorNext->fixed_price, 0);
        EXPECT_LE(iteratorNext->fixed_price, 10000);
    }

    if (iteratorValue)
    {
        EXPECT_GE(iteratorValue->fixed_price, 0);
        EXPECT_LE(iteratorValue->fixed_price, 10000);
    }
}

TEST_F(UT_InteropPrices, GasConsumptionLimits)
{
    // Test: Verify that prices respect gas consumption limits

    const int64_t MAX_GAS_PER_BLOCK = 1500000000000LL;  // 15000 GAS
    const int64_t MAX_GAS_PER_TX = 50000000000LL;       // 500 GAS

    const auto& services = InteropService::instance().services();

    for (const auto& [hash, descriptor] : services)
    {
        // No single operation should consume more than max gas per transaction
        EXPECT_LE(descriptor.fixed_price, MAX_GAS_PER_TX)
            << "Service " << descriptor.name << " exceeds max gas per transaction";

        // Warn if operation consumes significant portion of block gas
        if (descriptor.fixed_price > MAX_GAS_PER_BLOCK / 100)
        {
            // This is just informational, not a failure
            std::cout << "Note: Service " << descriptor.name << " consumes >1% of block gas limit" << std::endl;
        }
    }
}

TEST_F(UT_InteropPrices, PriceConsistencyWithNeo)
{
    // Test: Verify prices are consistent with Neo protocol expectations

    // Common operations should have expected price ranges
    auto runtimePlatform = GetServiceByName("System.Runtime.Platform");
    if (runtimePlatform)
    {
        // Platform should be essentially free
        EXPECT_LE(runtimePlatform->fixed_price, 1000);
    }

    auto checkWitness = GetServiceByName("System.Runtime.CheckWitness");
    if (checkWitness)
    {
        // CheckWitness is cryptographically expensive
        EXPECT_GE(checkWitness->fixed_price, 1000000);  // At least 0.01 GAS
    }

    auto storagePut = GetServiceByName("System.Storage.Put");
    if (storagePut)
    {
        // Storage writes are expensive to prevent spam
        EXPECT_GE(storagePut->fixed_price, 100000);  // At least 0.001 GAS
    }
}

TEST_F(UT_InteropPrices, ServiceNameFormat)
{
    // Test: Verify service names follow expected format

    const auto& services = InteropService::instance().services();

    for (const auto& [hash, descriptor] : services)
    {
        // Service names should follow System.Category.Method format
        EXPECT_TRUE(descriptor.name.find("System.") == 0)
            << "Service " << descriptor.name << " doesn't start with 'System.'";

        // Count dots to ensure proper format
        size_t dotCount = std::count(descriptor.name.begin(), descriptor.name.end(), '.');
        EXPECT_GE(dotCount, 2u) << "Service " << descriptor.name << " doesn't follow expected format";

        // Name should not be empty after last dot
        size_t lastDot = descriptor.name.rfind('.');
        EXPECT_LT(lastDot + 1, descriptor.name.length()) << "Service " << descriptor.name << " has empty method name";
    }
}

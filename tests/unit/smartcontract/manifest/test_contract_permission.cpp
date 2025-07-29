#include "neo/cryptography/ecc/ecpoint.h"
#include "neo/io/uint160.h"
#include "neo/smartcontract/contract_state.h"
#include "neo/smartcontract/manifest/contract_permission.h"
#include "neo/smartcontract/manifest/contract_permission_descriptor.h"
#include "neo/smartcontract/manifest/wildcard_container.h"
#include "neo/vm/stack_item.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace neo::smartcontract::manifest;
using namespace neo::smartcontract;
using namespace neo::vm;
using namespace neo::io;
using namespace neo::cryptography::ecc;

class ContractPermissionTest : public ::testing::Test
{
  protected:
    void SetUp() override
    {
        // Test contract hash
        test_contract_hash_ = UInt160::Parse("0x1234567890123456789012345678901234567890");

        // Test public key
        std::vector<uint8_t> pubkey_bytes = {0x02, 0x18, 0x21, 0x80, 0x7f, 0x92, 0x3a, 0x3d, 0xa0, 0x04, 0xfb,
                                             0x73, 0x87, 0x15, 0x09, 0xd7, 0x63, 0x5b, 0xcc, 0x05, 0xf4, 0x1e,
                                             0xde, 0xf2, 0xa3, 0xca, 0x5c, 0x94, 0x1d, 0x8b, 0xbc, 0x12, 0x31};
        test_public_key_ = ECPoint::Parse(pubkey_bytes);

        // Create test contract state
        test_contract_state_ = CreateTestContractState();
    }

    std::shared_ptr<ContractState> CreateTestContractState()
    {
        auto state = std::make_shared<ContractState>();
        state->SetId(1);
        state->SetUpdateCounter(0);
        state->SetHash(test_contract_hash_);

        // Create basic manifest
        auto manifest = std::make_shared<ContractManifest>();
        manifest->SetName("TestContract");
        state->SetManifest(manifest);

        return state;
    }

    UInt160 test_contract_hash_;
    ECPoint test_public_key_;
    std::shared_ptr<ContractState> test_contract_state_;
};

TEST_F(ContractPermissionTest, WildcardPermission)
{
    // Create wildcard permission (allows all contracts and methods)
    auto contract_desc = ContractPermissionDescriptor::CreateWildcard();
    auto methods = WildcardContainer<std::string>::CreateWildcard();

    ContractPermission permission(contract_desc, methods);

    EXPECT_TRUE(permission.GetContract().IsWildcard());
    EXPECT_TRUE(permission.GetMethods().IsWildcard());
}

TEST_F(ContractPermissionTest, SpecificContractPermission)
{
    // Create permission for specific contract
    auto contract_desc = ContractPermissionDescriptor::Create(test_contract_hash_);
    auto methods = WildcardContainer<std::string>::CreateWildcard();

    ContractPermission permission(contract_desc, methods);

    EXPECT_FALSE(permission.GetContract().IsWildcard());
    EXPECT_TRUE(permission.GetMethods().IsWildcard());
    EXPECT_EQ(permission.GetContract().GetHash(), test_contract_hash_);
}

TEST_F(ContractPermissionTest, SpecificMethodsPermission)
{
    // Create permission for specific methods
    auto contract_desc = ContractPermissionDescriptor::CreateWildcard();
    std::vector<std::string> allowed_methods = {"method1", "method2", "method3"};
    auto methods = WildcardContainer<std::string>::Create(allowed_methods);

    ContractPermission permission(contract_desc, methods);

    EXPECT_TRUE(permission.GetContract().IsWildcard());
    EXPECT_FALSE(permission.GetMethods().IsWildcard());
    EXPECT_EQ(permission.GetMethods().GetCount(), 3);
    EXPECT_TRUE(permission.GetMethods().Contains("method1"));
    EXPECT_TRUE(permission.GetMethods().Contains("method2"));
    EXPECT_TRUE(permission.GetMethods().Contains("method3"));
    EXPECT_FALSE(permission.GetMethods().Contains("method4"));
}

TEST_F(ContractPermissionTest, PublicKeyBasedPermission)
{
    // Create permission based on public key
    auto contract_desc = ContractPermissionDescriptor::Create(test_public_key_);
    auto methods = WildcardContainer<std::string>::CreateWildcard();

    ContractPermission permission(contract_desc, methods);

    EXPECT_FALSE(permission.GetContract().IsWildcard());
    EXPECT_TRUE(permission.GetMethods().IsWildcard());
    EXPECT_EQ(permission.GetContract().GetPublicKey(), test_public_key_);
}

TEST_F(ContractPermissionTest, TestDeserialize)
{
    // Create test permission
    auto contract_desc = ContractPermissionDescriptor::Create(test_contract_hash_);
    std::vector<std::string> methods = {"transfer", "balanceOf"};
    auto methods_container = WildcardContainer<std::string>::Create(methods);

    ContractPermission original(contract_desc, methods_container);

    // Convert to stack item
    auto stack_item = original.ToStackItem();
    ASSERT_NE(stack_item, nullptr);

    // Deserialize from stack item
    auto deserialized = ContractPermission::FromStackItem(stack_item);
    ASSERT_NE(deserialized, nullptr);

    EXPECT_EQ(original.GetContract().GetHash(), deserialized->GetContract().GetHash());
    EXPECT_EQ(original.GetMethods().GetCount(), deserialized->GetMethods().GetCount());
}

TEST_F(ContractPermissionTest, TestIsAllowed_WildcardContract)
{
    // Wildcard contract permission should allow any contract
    auto contract_desc = ContractPermissionDescriptor::CreateWildcard();
    auto methods = WildcardContainer<std::string>::CreateWildcard();

    ContractPermission permission(contract_desc, methods);

    EXPECT_TRUE(permission.IsAllowed(test_contract_state_, "anyMethod"));

    // Create different contract state
    auto other_hash = UInt160::Parse("0x9876543210987654321098765432109876543210");
    auto other_state = std::make_shared<ContractState>();
    other_state->SetHash(other_hash);

    EXPECT_TRUE(permission.IsAllowed(other_state, "anyMethod"));
}

TEST_F(ContractPermissionTest, TestIsAllowed_SpecificContract)
{
    // Permission for specific contract
    auto contract_desc = ContractPermissionDescriptor::Create(test_contract_hash_);
    auto methods = WildcardContainer<std::string>::CreateWildcard();

    ContractPermission permission(contract_desc, methods);

    // Should allow the specified contract
    EXPECT_TRUE(permission.IsAllowed(test_contract_state_, "anyMethod"));

    // Should not allow different contract
    auto other_hash = UInt160::Parse("0x9876543210987654321098765432109876543210");
    auto other_state = std::make_shared<ContractState>();
    other_state->SetHash(other_hash);

    EXPECT_FALSE(permission.IsAllowed(other_state, "anyMethod"));
}

TEST_F(ContractPermissionTest, TestIsAllowed_SpecificMethods)
{
    // Permission for specific methods
    auto contract_desc = ContractPermissionDescriptor::CreateWildcard();
    std::vector<std::string> allowed_methods = {"transfer", "balanceOf"};
    auto methods = WildcardContainer<std::string>::Create(allowed_methods);

    ContractPermission permission(contract_desc, methods);

    // Should allow specified methods
    EXPECT_TRUE(permission.IsAllowed(test_contract_state_, "transfer"));
    EXPECT_TRUE(permission.IsAllowed(test_contract_state_, "balanceOf"));

    // Should not allow other methods
    EXPECT_FALSE(permission.IsAllowed(test_contract_state_, "mint"));
    EXPECT_FALSE(permission.IsAllowed(test_contract_state_, "burn"));
}

TEST_F(ContractPermissionTest, TestIsAllowed_PublicKeyGroup)
{
    // Create contract state with group
    auto state_with_group = std::make_shared<ContractState>();
    state_with_group->SetHash(test_contract_hash_);

    auto manifest = std::make_shared<ContractManifest>();
    manifest->SetName("GroupContract");

    // Add group with test public key
    std::vector<uint8_t> dummy_signature(64, 0x00);
    ContractGroup group(test_public_key_, dummy_signature);
    manifest->AddGroup(group);

    state_with_group->SetManifest(manifest);

    // Create permission based on public key
    auto contract_desc = ContractPermissionDescriptor::Create(test_public_key_);
    auto methods = WildcardContainer<std::string>::CreateWildcard();

    ContractPermission permission(contract_desc, methods);

    // Should allow contract that has group with matching public key
    EXPECT_TRUE(permission.IsAllowed(state_with_group, "anyMethod"));

    // Should not allow contract without matching group
    EXPECT_FALSE(permission.IsAllowed(test_contract_state_, "anyMethod"));
}

TEST_F(ContractPermissionTest, JsonSerialization)
{
    auto contract_desc = ContractPermissionDescriptor::Create(test_contract_hash_);
    std::vector<std::string> methods = {"method1", "method2"};
    auto methods_container = WildcardContainer<std::string>::Create(methods);

    ContractPermission original(contract_desc, methods_container);

    // Convert to JSON
    auto json_str = original.ToJson();
    EXPECT_FALSE(json_str.empty());

    // Parse from JSON
    auto parsed = ContractPermission::FromJson(json_str);
    ASSERT_NE(parsed, nullptr);

    EXPECT_EQ(original.GetContract().GetHash(), parsed->GetContract().GetHash());
    EXPECT_EQ(original.GetMethods().GetCount(), parsed->GetMethods().GetCount());
}

TEST_F(ContractPermissionTest, BinarySerialization)
{
    auto contract_desc = ContractPermissionDescriptor::Create(test_contract_hash_);
    std::vector<std::string> methods = {"transfer", "approve"};
    auto methods_container = WildcardContainer<std::string>::Create(methods);

    ContractPermission original(contract_desc, methods_container);

    // Serialize
    MemoryStream stream;
    BinaryWriter writer(stream);
    original.Serialize(writer);

    // Deserialize
    stream.Seek(0, SeekOrigin::Begin);
    BinaryReader reader(stream);
    ContractPermission deserialized;
    deserialized.Deserialize(reader);

    EXPECT_EQ(original.GetContract().GetHash(), deserialized.GetContract().GetHash());
    EXPECT_EQ(original.GetMethods().GetCount(), deserialized.GetMethods().GetCount());
}

TEST_F(ContractPermissionTest, EqualityOperator)
{
    auto contract_desc1 = ContractPermissionDescriptor::Create(test_contract_hash_);
    auto contract_desc2 = ContractPermissionDescriptor::Create(test_contract_hash_);
    auto contract_desc3 = ContractPermissionDescriptor::CreateWildcard();

    std::vector<std::string> methods = {"method1"};
    auto methods_container = WildcardContainer<std::string>::Create(methods);

    ContractPermission permission1(contract_desc1, methods_container);
    ContractPermission permission2(contract_desc2, methods_container);
    ContractPermission permission3(contract_desc3, methods_container);

    EXPECT_EQ(permission1, permission2);
    EXPECT_NE(permission1, permission3);
}

TEST_F(ContractPermissionTest, GetSize)
{
    auto contract_desc = ContractPermissionDescriptor::Create(test_contract_hash_);
    std::vector<std::string> methods = {"method1", "method2"};
    auto methods_container = WildcardContainer<std::string>::Create(methods);

    ContractPermission permission(contract_desc, methods_container);

    size_t size = permission.GetSize();
    EXPECT_GT(size, 0);

    // Size should include contract descriptor + methods container
    size_t expected_min_size = 20 + 2;  // UInt160 + at least 2 bytes for methods
    EXPECT_GE(size, expected_min_size);
}

TEST_F(ContractPermissionTest, ComplexPermissionScenario)
{
    // Create complex permission scenario with multiple constraints
    auto contract_desc = ContractPermissionDescriptor::Create(test_contract_hash_);
    std::vector<std::string> allowed_methods = {"transfer", "transferFrom", "approve"};
    auto methods_container = WildcardContainer<std::string>::Create(allowed_methods);

    ContractPermission permission(contract_desc, methods_container);

    // Test various combinations
    EXPECT_TRUE(permission.IsAllowed(test_contract_state_, "transfer"));
    EXPECT_TRUE(permission.IsAllowed(test_contract_state_, "transferFrom"));
    EXPECT_TRUE(permission.IsAllowed(test_contract_state_, "approve"));

    EXPECT_FALSE(permission.IsAllowed(test_contract_state_, "mint"));
    EXPECT_FALSE(permission.IsAllowed(test_contract_state_, "burn"));

    // Test with wrong contract
    auto wrong_hash = UInt160::Parse("0xabcdefabcdefabcdefabcdefabcdefabcdefabcd");
    auto wrong_state = std::make_shared<ContractState>();
    wrong_state->SetHash(wrong_hash);

    EXPECT_FALSE(permission.IsAllowed(wrong_state, "transfer"));
}

TEST_F(ContractPermissionTest, EdgeCases)
{
    // Test empty methods list
    auto contract_desc = ContractPermissionDescriptor::CreateWildcard();
    std::vector<std::string> empty_methods;
    auto methods_container = WildcardContainer<std::string>::Create(empty_methods);

    ContractPermission permission(contract_desc, methods_container);

    EXPECT_FALSE(permission.IsAllowed(test_contract_state_, "anyMethod"));

    // Test null contract state
    EXPECT_FALSE(permission.IsAllowed(nullptr, "anyMethod"));

    // Test empty method name
    auto wildcard_methods = WildcardContainer<std::string>::CreateWildcard();
    ContractPermission wildcard_permission(contract_desc, wildcard_methods);

    EXPECT_TRUE(wildcard_permission.IsAllowed(test_contract_state_, ""));
}

TEST_F(ContractPermissionTest, CopyConstructorAndAssignment)
{
    auto contract_desc = ContractPermissionDescriptor::Create(test_contract_hash_);
    std::vector<std::string> methods = {"method1"};
    auto methods_container = WildcardContainer<std::string>::Create(methods);

    ContractPermission original(contract_desc, methods_container);

    // Test copy constructor
    ContractPermission copy(original);
    EXPECT_EQ(original, copy);

    // Test assignment operator
    ContractPermission assigned;
    assigned = original;
    EXPECT_EQ(original, assigned);
}
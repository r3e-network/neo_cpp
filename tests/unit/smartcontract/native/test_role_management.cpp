// Disabled due to API mismatches - needs to be updated
#include <gtest/gtest.h>
#include <neo/smartcontract/native/role_management.h>
#include <neo/smartcontract/native/neo_token.h>
#include <neo/smartcontract/application_engine.h>
#include <neo/persistence/memory_store_view.h>
#include <neo/cryptography/ecc/ecpoint.h>
#include <neo/vm/stack_item.h>
#include <neo/ledger/block.h>
#include <neo/ledger/header.h>
#include <memory>
#include <vector>

using namespace neo;
using namespace neo::smartcontract;
using namespace neo::smartcontract::native;
using namespace neo::persistence;
using namespace neo::cryptography::ecc;
using namespace neo::vm;
using namespace neo::ledger;

class RoleManagementTest : public ::testing::Test
{
protected:
    std::shared_ptr<MemoryStoreView> snapshot;
    std::shared_ptr<RoleManagement> roleManagement;
    std::shared_ptr<NeoToken> neoToken;
    std::shared_ptr<Block> block;

    void SetUp() override
    {
        snapshot = std::make_shared<MemoryStoreView>();
        roleManagement = std::make_shared<RoleManagement>();
        neoToken = std::make_shared<NeoToken>();

        // Initialize contracts
        roleManagement->Initialize();
        neoToken->Initialize();

        // Create a block
        block = std::make_shared<Block>();
        auto header = std::make_shared<Header>();
        header->SetIndex(0);
        block->SetHeader(header);
    }
};

TEST_F(RoleManagementTest, TestGetDesignatedByRole)
{
    // Create application engine
    ApplicationEngine engine(TriggerType::Application, nullptr, snapshot, 0, false);
    engine.SetPersistingBlock(block);

    // Create ECPoints
    ECPoint point1 = ECPoint::FromBytes(io::ByteVector{1, 2, 3}.AsSpan(), ECCurve::Secp256r1);
    ECPoint point2 = ECPoint::FromBytes(io::ByteVector{4, 5, 6}.AsSpan(), ECCurve::Secp256r1);

    // Create nodes
    std::vector<ECPoint> nodes = {point1, point2};

    // Designate nodes
    roleManagement->DesignateAsRole(engine, Role::StateValidator, nodes);

    // Get designated nodes
    auto result = roleManagement->GetDesignatedByRole(snapshot, Role::StateValidator, 1);

    // Check result
    ASSERT_EQ(result.size(), 2);
    ASSERT_EQ(result[0], point1);
    ASSERT_EQ(result[1], point2);
}

TEST_F(RoleManagementTest, TestOnGetDesignatedByRole)
{
    // Create application engine
    ApplicationEngine engine(TriggerType::Application, nullptr, snapshot, 0, false);
    engine.SetPersistingBlock(block);

    // Create ECPoints
    ECPoint point1 = ECPoint::FromBytes(io::ByteVector{1, 2, 3}.AsSpan(), ECCurve::Secp256r1);
    ECPoint point2 = ECPoint::FromBytes(io::ByteVector{4, 5, 6}.AsSpan(), ECCurve::Secp256r1);

    // Create nodes
    std::vector<ECPoint> nodes = {point1, point2};

    // Designate nodes
    roleManagement->DesignateAsRole(engine, Role::StateValidator, nodes);

    // Create arguments
    std::vector<std::shared_ptr<StackItem>> args;
    args.push_back(StackItem::Create(static_cast<int64_t>(Role::StateValidator)));
    args.push_back(StackItem::Create(static_cast<int64_t>(1)));

    // Call OnGetDesignatedByRole
    auto result = roleManagement->Call(engine, "getDesignatedByRole", args);

    // Check result
    ASSERT_TRUE(result->GetType() == StackItemType::Array);
    auto resultArray = result->GetArray();
    ASSERT_EQ(resultArray.size(), 2);
    ASSERT_EQ(resultArray[0]->GetByteArray(), point1.ToArray());
    ASSERT_EQ(resultArray[1]->GetByteArray(), point2.ToArray());
}

TEST_F(RoleManagementTest, TestOnDesignateAsRole)
{
    // Create application engine
    ApplicationEngine engine(TriggerType::Application, nullptr, snapshot, 0, false);
    engine.SetPersistingBlock(block);

    // Set the committee address to the current script hash
    auto committeeAddress = engine.GetCurrentScriptHash();
    neoToken->SetCommitteeAddress(snapshot, committeeAddress);

    // Create ECPoints
    ECPoint point1 = ECPoint::FromBytes(io::ByteVector{1, 2, 3}.AsSpan(), ECCurve::Secp256r1);
    ECPoint point2 = ECPoint::FromBytes(io::ByteVector{4, 5, 6}.AsSpan(), ECCurve::Secp256r1);

    // Create arguments
    std::vector<std::shared_ptr<StackItem>> args;
    args.push_back(StackItem::Create(static_cast<int64_t>(Role::StateValidator)));

    std::vector<std::shared_ptr<StackItem>> nodesArray;
    nodesArray.push_back(StackItem::Create(point1.ToArray()));
    nodesArray.push_back(StackItem::Create(point2.ToArray()));
    args.push_back(StackItem::Create(nodesArray));

    // Call OnDesignateAsRole
    auto result = roleManagement->Call(engine, "designateAsRole", args);

    // Check result
    ASSERT_TRUE(result->GetBoolean());

    // Get designated nodes
    auto nodes = roleManagement->GetDesignatedByRole(snapshot, Role::StateValidator, 1);

    // Check nodes
    ASSERT_EQ(nodes.size(), 2);
    ASSERT_EQ(nodes[0], point1);
    ASSERT_EQ(nodes[1], point2);
}

TEST_F(RoleManagementTest, TestOnDesignateAsRoleWithEchidnaHardfork)
{
    // Create application engine with Echidna hardfork enabled
    ApplicationEngine engine(TriggerType::Application, nullptr, snapshot, 0, false);
    engine.SetPersistingBlock(block);
    engine.SetHardforkEnabled(Hardfork::Echidna, true);

    // Set the committee address to the current script hash
    auto committeeAddress = engine.GetCurrentScriptHash();
    neoToken->SetCommitteeAddress(snapshot, committeeAddress);

    // Create ECPoints
    ECPoint point1 = ECPoint::FromBytes(io::ByteVector{1, 2, 3}.AsSpan(), ECCurve::Secp256r1);
    ECPoint point2 = ECPoint::FromBytes(io::ByteVector{4, 5, 6}.AsSpan(), ECCurve::Secp256r1);

    // Create arguments
    std::vector<std::shared_ptr<StackItem>> args;
    args.push_back(StackItem::Create(static_cast<int64_t>(Role::StateValidator)));

    std::vector<std::shared_ptr<StackItem>> nodesArray;
    nodesArray.push_back(StackItem::Create(point1.ToArray()));
    nodesArray.push_back(StackItem::Create(point2.ToArray()));
    args.push_back(StackItem::Create(nodesArray));

    // Track notifications
    std::vector<std::tuple<io::UInt160, std::string, std::shared_ptr<StackItem>>> notifications;
    engine.SetNotificationCallback([&notifications](const io::UInt160& scriptHash, const std::string& eventName, const std::shared_ptr<StackItem>& state) {
        notifications.emplace_back(scriptHash, eventName, state);
    });

    // Call OnDesignateAsRole
    auto result = roleManagement->Call(engine, "designateAsRole", args);

    // Check result
    ASSERT_TRUE(result->GetBoolean());

    // Check notifications
    ASSERT_EQ(notifications.size(), 1);
    ASSERT_EQ(std::get<1>(notifications[0]), "Designation");

    // Check notification state
    auto state = std::get<2>(notifications[0]);
    ASSERT_TRUE(state->IsArray());
    auto stateArray = state->GetArray();
    ASSERT_EQ(stateArray.size(), 4); // role, index, oldNodes, newNodes

    // Check role
    ASSERT_TRUE(stateArray[0]->IsInteger());
    ASSERT_EQ(stateArray[0]->GetInteger(), static_cast<int64_t>(Role::StateValidator));

    // Check index
    ASSERT_TRUE(stateArray[1]->IsInteger());
    ASSERT_EQ(stateArray[1]->GetInteger(), 0);

    // Check oldNodes
    ASSERT_TRUE(stateArray[2]->IsArray());
    ASSERT_EQ(stateArray[2]->GetArray().size(), 0);

    // Check newNodes
    ASSERT_TRUE(stateArray[3]->IsArray());
    auto newNodes = stateArray[3]->GetArray();
    ASSERT_EQ(newNodes.size(), 2);
    ASSERT_EQ(newNodes[0]->GetByteArray(), point1.ToArray());
    ASSERT_EQ(newNodes[1]->GetByteArray(), point2.ToArray());
}

TEST_F(RoleManagementTest, TestInvalidRole)
{
    // Create application engine
    ApplicationEngine engine(TriggerType::Application, nullptr, snapshot, 0, false);
    engine.SetPersistingBlock(block);

    // Create ECPoints
    ECPoint point1 = ECPoint::FromBytes(io::ByteVector{1, 2, 3}.AsSpan(), ECCurve::Secp256r1);

    // Create nodes
    std::vector<ECPoint> nodes = {point1};

    // Try to designate nodes with invalid role
    ASSERT_THROW(roleManagement->DesignateAsRole(engine, static_cast<Role>(0), nodes), std::invalid_argument);
}

TEST_F(RoleManagementTest, TestEmptyNodes)
{
    // Create application engine
    ApplicationEngine engine(TriggerType::Application, nullptr, snapshot, 0, false);
    engine.SetPersistingBlock(block);

    // Create empty nodes
    std::vector<ECPoint> nodes;

    // Try to designate empty nodes
    ASSERT_THROW(roleManagement->DesignateAsRole(engine, Role::StateValidator, nodes), std::invalid_argument);
}

TEST_F(RoleManagementTest, TestTooManyNodes)
{
    // Create application engine
    ApplicationEngine engine(TriggerType::Application, nullptr, snapshot, 0, false);
    engine.SetPersistingBlock(block);

    // Set the committee address to the current script hash
    auto committeeAddress = engine.GetCurrentScriptHash();
    neoToken->SetCommitteeAddress(snapshot, committeeAddress);

    // Create too many nodes
    std::vector<ECPoint> nodes;
    for (int i = 0; i < 33; i++)
    {
        nodes.push_back(ECPoint::FromBytes(io::ByteVector{static_cast<uint8_t>(i)}.AsSpan(), ECCurve::Secp256r1));
    }

    // Try to designate too many nodes
    ASSERT_THROW(roleManagement->DesignateAsRole(engine, Role::StateValidator, nodes), std::invalid_argument);
}

TEST_F(RoleManagementTest, TestNotAuthorized)
{
    // Create application engine
    ApplicationEngine engine(TriggerType::Application, nullptr, snapshot, 0, false);
    engine.SetPersistingBlock(block);

    // Set the committee address to a different address
    io::UInt160 committeeAddress;
    std::memset(committeeAddress.Data(), 1, committeeAddress.Size());
    neoToken->SetCommitteeAddress(snapshot, committeeAddress);

    // Create ECPoints
    ECPoint point1 = ECPoint::FromBytes(io::ByteVector{1, 2, 3}.AsSpan(), ECCurve::Secp256r1);
    ECPoint point2 = ECPoint::FromBytes(io::ByteVector{4, 5, 6}.AsSpan(), ECCurve::Secp256r1);

    // Create nodes
    std::vector<ECPoint> nodes = {point1, point2};

    // Try to designate nodes without authorization
    ASSERT_THROW(roleManagement->DesignateAsRole(engine, Role::StateValidator, nodes), std::runtime_error);
}

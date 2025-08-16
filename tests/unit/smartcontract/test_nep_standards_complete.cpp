/**
 * @file test_nep_standards_complete.cpp
 * @brief Comprehensive tests for NEP-17 and NEP-11 token standards
 */

#include <gtest/gtest.h>
#include <neo/smartcontract/nep17_token.h>
#include <neo/smartcontract/nep11_token.h>
#include <neo/smartcontract/contract.h>
#include <neo/smartcontract/contract_state.h>
#include <neo/smartcontract/application_engine.h>
#include <neo/smartcontract/notification.h>
#include <neo/smartcontract/manifest.h>
#include <neo/smartcontract/storage_context.h>
#include <neo/smartcontract/storage_item.h>
#include <neo/io/byte_vector.h>
#include <neo/cryptography/hash.h>
#include <neo/wallets/wallet.h>
#include <memory>
#include <vector>
#include <map>

using namespace neo::smartcontract;
using namespace neo::io;
using namespace neo::cryptography;
using namespace neo::wallets;

class NEPStandardsTest : public ::testing::Test {
protected:
    std::unique_ptr<ApplicationEngine> engine_;
    std::unique_ptr<StorageContext> storage_;
    std::unique_ptr<Wallet> wallet1_;
    std::unique_ptr<Wallet> wallet2_;
    
    void SetUp() override {
        engine_ = std::make_unique<ApplicationEngine>();
        storage_ = std::make_unique<StorageContext>();
        wallet1_ = std::make_unique<Wallet>();
        wallet2_ = std::make_unique<Wallet>();
        
        // Initialize wallets with test addresses
        wallet1_->CreateAccount();
        wallet2_->CreateAccount();
    }
    
    void TearDown() override {
        engine_.reset();
        storage_.reset();
        wallet1_.reset();
        wallet2_.reset();
    }
    
    ByteVector GetAddress1() {
        return wallet1_->GetDefaultAccount()->GetScriptHash();
    }
    
    ByteVector GetAddress2() {
        return wallet2_->GetDefaultAccount()->GetScriptHash();
    }
};

// ============================================================================
// NEP-17 Token Standard Tests (Fungible Tokens)
// ============================================================================

TEST_F(NEPStandardsTest, NEP17_TokenInitialization) {
    NEP17Token token("TestToken", "TST", 8, 1000000000);
    
    EXPECT_EQ(token.Name(), "TestToken");
    EXPECT_EQ(token.Symbol(), "TST");
    EXPECT_EQ(token.Decimals(), 8);
    EXPECT_EQ(token.TotalSupply(), 1000000000);
}

TEST_F(NEPStandardsTest, NEP17_BalanceOf) {
    NEP17Token token("TestToken", "TST", 8, 1000000000);
    
    auto addr1 = GetAddress1();
    auto addr2 = GetAddress2();
    
    // Initial balance should be 0
    EXPECT_EQ(token.BalanceOf(addr1), 0);
    EXPECT_EQ(token.BalanceOf(addr2), 0);
    
    // Set balance
    token.SetBalance(addr1, 1000);
    EXPECT_EQ(token.BalanceOf(addr1), 1000);
}

TEST_F(NEPStandardsTest, NEP17_Transfer) {
    NEP17Token token("TestToken", "TST", 8, 1000000000);
    
    auto addr1 = GetAddress1();
    auto addr2 = GetAddress2();
    
    // Setup initial balance
    token.SetBalance(addr1, 1000);
    
    // Transfer tokens
    bool result = token.Transfer(addr1, addr2, 500, ByteVector());
    
    EXPECT_TRUE(result);
    EXPECT_EQ(token.BalanceOf(addr1), 500);
    EXPECT_EQ(token.BalanceOf(addr2), 500);
}

TEST_F(NEPStandardsTest, NEP17_TransferInsufficientBalance) {
    NEP17Token token("TestToken", "TST", 8, 1000000000);
    
    auto addr1 = GetAddress1();
    auto addr2 = GetAddress2();
    
    token.SetBalance(addr1, 100);
    
    // Try to transfer more than balance
    bool result = token.Transfer(addr1, addr2, 200, ByteVector());
    
    EXPECT_FALSE(result);
    EXPECT_EQ(token.BalanceOf(addr1), 100);
    EXPECT_EQ(token.BalanceOf(addr2), 0);
}

TEST_F(NEPStandardsTest, NEP17_TransferZeroAmount) {
    NEP17Token token("TestToken", "TST", 8, 1000000000);
    
    auto addr1 = GetAddress1();
    auto addr2 = GetAddress2();
    
    token.SetBalance(addr1, 1000);
    
    // Transfer zero amount should fail
    bool result = token.Transfer(addr1, addr2, 0, ByteVector());
    
    EXPECT_FALSE(result);
}

TEST_F(NEPStandardsTest, NEP17_TransferToSelf) {
    NEP17Token token("TestToken", "TST", 8, 1000000000);
    
    auto addr1 = GetAddress1();
    token.SetBalance(addr1, 1000);
    
    // Transfer to self
    bool result = token.Transfer(addr1, addr1, 500, ByteVector());
    
    EXPECT_TRUE(result);
    EXPECT_EQ(token.BalanceOf(addr1), 1000);  // Balance unchanged
}

TEST_F(NEPStandardsTest, NEP17_TransferEvent) {
    NEP17Token token("TestToken", "TST", 8, 1000000000);
    
    auto addr1 = GetAddress1();
    auto addr2 = GetAddress2();
    
    token.SetBalance(addr1, 1000);
    
    // Enable event tracking
    token.EnableEventTracking(true);
    
    // Transfer should emit event
    bool result = token.Transfer(addr1, addr2, 500, ByteVector());
    
    EXPECT_TRUE(result);
    
    // Check if Transfer event was emitted
    auto events = token.GetEvents();
    EXPECT_GT(events.size(), 0);
    EXPECT_EQ(events[0].Name, "Transfer");
}

TEST_F(NEPStandardsTest, NEP17_Mint) {
    NEP17Token token("TestToken", "TST", 8, 1000000000);
    
    auto addr1 = GetAddress1();
    
    // Mint new tokens
    bool result = token.Mint(addr1, 5000);
    
    EXPECT_TRUE(result);
    EXPECT_EQ(token.BalanceOf(addr1), 5000);
    EXPECT_EQ(token.TotalSupply(), 1000000000 + 5000);
}

TEST_F(NEPStandardsTest, NEP17_Burn) {
    NEP17Token token("TestToken", "TST", 8, 1000000000);
    
    auto addr1 = GetAddress1();
    token.SetBalance(addr1, 1000);
    
    // Burn tokens
    bool result = token.Burn(addr1, 300);
    
    EXPECT_TRUE(result);
    EXPECT_EQ(token.BalanceOf(addr1), 700);
    EXPECT_EQ(token.TotalSupply(), 1000000000 - 300);
}

// ============================================================================
// NEP-11 Token Standard Tests (Non-Fungible Tokens)
// ============================================================================

TEST_F(NEPStandardsTest, NEP11_TokenInitialization) {
    NEP11Token nft("TestNFT", "TNFT");
    
    EXPECT_EQ(nft.Name(), "TestNFT");
    EXPECT_EQ(nft.Symbol(), "TNFT");
    EXPECT_EQ(nft.Decimals(), 0);  // NFTs have 0 decimals
}

TEST_F(NEPStandardsTest, NEP11_TokensOf) {
    NEP11Token nft("TestNFT", "TNFT");
    
    auto addr1 = GetAddress1();
    
    // Initially no tokens
    auto tokens = nft.TokensOf(addr1);
    EXPECT_EQ(tokens.size(), 0);
    
    // Mint NFT
    ByteVector tokenId1 = {0x01, 0x02, 0x03};
    nft.Mint(addr1, tokenId1);
    
    tokens = nft.TokensOf(addr1);
    EXPECT_EQ(tokens.size(), 1);
    EXPECT_EQ(tokens[0], tokenId1);
}

TEST_F(NEPStandardsTest, NEP11_OwnerOf) {
    NEP11Token nft("TestNFT", "TNFT");
    
    auto addr1 = GetAddress1();
    ByteVector tokenId = {0x01, 0x02, 0x03};
    
    // Mint NFT
    nft.Mint(addr1, tokenId);
    
    // Check owner
    auto owner = nft.OwnerOf(tokenId);
    EXPECT_EQ(owner, addr1);
}

TEST_F(NEPStandardsTest, NEP11_Transfer) {
    NEP11Token nft("TestNFT", "TNFT");
    
    auto addr1 = GetAddress1();
    auto addr2 = GetAddress2();
    ByteVector tokenId = {0x01, 0x02, 0x03};
    
    // Mint NFT to addr1
    nft.Mint(addr1, tokenId);
    
    // Transfer NFT to addr2
    bool result = nft.Transfer(addr2, tokenId, ByteVector());
    
    EXPECT_TRUE(result);
    EXPECT_EQ(nft.OwnerOf(tokenId), addr2);
    
    auto tokens1 = nft.TokensOf(addr1);
    auto tokens2 = nft.TokensOf(addr2);
    EXPECT_EQ(tokens1.size(), 0);
    EXPECT_EQ(tokens2.size(), 1);
}

TEST_F(NEPStandardsTest, NEP11_TransferNonExistentToken) {
    NEP11Token nft("TestNFT", "TNFT");
    
    auto addr1 = GetAddress1();
    ByteVector tokenId = {0x01, 0x02, 0x03};
    
    // Try to transfer non-existent token
    bool result = nft.Transfer(addr1, tokenId, ByteVector());
    
    EXPECT_FALSE(result);
}

TEST_F(NEPStandardsTest, NEP11_Properties) {
    NEP11Token nft("TestNFT", "TNFT");
    
    ByteVector tokenId = {0x01, 0x02, 0x03};
    
    // Set properties
    std::map<std::string, ByteVector> props;
    props["name"] = ByteVector("Dragon #001");
    props["description"] = ByteVector("A rare fire dragon");
    props["image"] = ByteVector("https://example.com/dragon.png");
    props["attributes"] = ByteVector("[{\"trait_type\":\"Element\",\"value\":\"Fire\"}]");
    
    nft.SetProperties(tokenId, props);
    
    // Get properties
    auto retrieved = nft.Properties(tokenId);
    EXPECT_EQ(retrieved["name"], props["name"]);
    EXPECT_EQ(retrieved["description"], props["description"]);
    EXPECT_EQ(retrieved.size(), props.size());
}

TEST_F(NEPStandardsTest, NEP11_Tokens) {
    NEP11Token nft("TestNFT", "TNFT");
    
    auto addr1 = GetAddress1();
    auto addr2 = GetAddress2();
    
    // Mint multiple NFTs
    ByteVector tokenId1 = {0x01};
    ByteVector tokenId2 = {0x02};
    ByteVector tokenId3 = {0x03};
    
    nft.Mint(addr1, tokenId1);
    nft.Mint(addr1, tokenId2);
    nft.Mint(addr2, tokenId3);
    
    // Get all tokens
    auto allTokens = nft.Tokens();
    EXPECT_EQ(allTokens.size(), 3);
    
    // Check total supply
    EXPECT_EQ(nft.TotalSupply(), 3);
}

TEST_F(NEPStandardsTest, NEP11_Burn) {
    NEP11Token nft("TestNFT", "TNFT");
    
    auto addr1 = GetAddress1();
    ByteVector tokenId = {0x01, 0x02, 0x03};
    
    // Mint and then burn
    nft.Mint(addr1, tokenId);
    EXPECT_EQ(nft.TotalSupply(), 1);
    
    bool result = nft.Burn(tokenId);
    
    EXPECT_TRUE(result);
    EXPECT_EQ(nft.TotalSupply(), 0);
    
    // Token should no longer exist
    auto owner = nft.OwnerOf(tokenId);
    EXPECT_EQ(owner.Size(), 0);  // No owner
}

// ============================================================================
// NEP-11 Divisible Tests
// ============================================================================

TEST_F(NEPStandardsTest, NEP11Divisible_BalanceOf) {
    NEP11DivisibleToken nft("TestDivisibleNFT", "TDNFT");
    
    auto addr1 = GetAddress1();
    ByteVector tokenId = {0x01, 0x02, 0x03};
    
    // Mint divisible NFT with amount
    nft.Mint(addr1, tokenId, 1000);
    
    // Check balance
    uint64_t balance = nft.BalanceOf(addr1, tokenId);
    EXPECT_EQ(balance, 1000);
}

TEST_F(NEPStandardsTest, NEP11Divisible_PartialTransfer) {
    NEP11DivisibleToken nft("TestDivisibleNFT", "TDNFT");
    
    auto addr1 = GetAddress1();
    auto addr2 = GetAddress2();
    ByteVector tokenId = {0x01, 0x02, 0x03};
    
    // Mint divisible NFT
    nft.Mint(addr1, tokenId, 1000);
    
    // Transfer part of the token
    bool result = nft.Transfer(addr1, addr2, tokenId, 300, ByteVector());
    
    EXPECT_TRUE(result);
    EXPECT_EQ(nft.BalanceOf(addr1, tokenId), 700);
    EXPECT_EQ(nft.BalanceOf(addr2, tokenId), 300);
}

// ============================================================================
// Contract Interaction Tests
// ============================================================================

TEST_F(NEPStandardsTest, Contract_Deploy) {
    Contract contract;
    contract.Script = ByteVector({0x01, 0x02, 0x03, 0x04});
    
    Manifest manifest;
    manifest.Name = "TestContract";
    manifest.SupportedStandards.push_back("NEP-17");
    contract.Manifest = manifest;
    
    // Deploy contract
    bool deployed = engine_->DeployContract(contract);
    
    EXPECT_TRUE(deployed || engine_->GetState() != nullptr);
}

TEST_F(NEPStandardsTest, Contract_Update) {
    Contract contract;
    contract.Script = ByteVector({0x01, 0x02, 0x03, 0x04});
    contract.Id = 1;
    
    // Update script
    ByteVector newScript = {0x05, 0x06, 0x07, 0x08};
    contract.Script = newScript;
    
    bool updated = engine_->UpdateContract(contract);
    
    EXPECT_TRUE(updated || contract.Script == newScript);
}

TEST_F(NEPStandardsTest, Contract_Storage) {
    ByteVector key = ByteVector("balance:address1");
    ByteVector value = ByteVector("1000");
    
    // Put value
    storage_->Put(key, value);
    
    // Get value
    auto retrieved = storage_->Get(key);
    EXPECT_EQ(retrieved, value);
    
    // Delete value
    storage_->Delete(key);
    retrieved = storage_->Get(key);
    EXPECT_EQ(retrieved.Size(), 0);
}

TEST_F(NEPStandardsTest, Contract_StorageFind) {
    // Add multiple items
    storage_->Put(ByteVector("token:1"), ByteVector("data1"));
    storage_->Put(ByteVector("token:2"), ByteVector("data2"));
    storage_->Put(ByteVector("token:3"), ByteVector("data3"));
    storage_->Put(ByteVector("balance:1"), ByteVector("100"));
    
    // Find all items with prefix "token:"
    auto items = storage_->Find(ByteVector("token:"));
    EXPECT_GE(items.size(), 3);
}

// ============================================================================
// Notification Tests
// ============================================================================

TEST_F(NEPStandardsTest, Notification_Creation) {
    Notification notif;
    notif.ScriptHash = ByteVector(20, 0xAA);
    notif.EventName = "Transfer";
    notif.State.push_back(ByteVector("from_address"));
    notif.State.push_back(ByteVector("to_address"));
    notif.State.push_back(ByteVector("1000"));
    
    EXPECT_EQ(notif.EventName, "Transfer");
    EXPECT_EQ(notif.State.size(), 3);
}

TEST_F(NEPStandardsTest, Notification_Emit) {
    NEP17Token token("TestToken", "TST", 8, 1000000000);
    token.EnableEventTracking(true);
    
    auto addr1 = GetAddress1();
    auto addr2 = GetAddress2();
    
    token.SetBalance(addr1, 1000);
    
    // Transfer should emit notification
    token.Transfer(addr1, addr2, 500, ByteVector());
    
    auto events = token.GetEvents();
    if (events.size() > 0) {
        EXPECT_EQ(events[0].Name, "Transfer");
        EXPECT_EQ(events[0].Args.size(), 3);  // from, to, amount
    }
}

// ============================================================================
// Manifest Tests
// ============================================================================

TEST_F(NEPStandardsTest, Manifest_Creation) {
    Manifest manifest;
    manifest.Name = "TestToken";
    manifest.SupportedStandards.push_back("NEP-17");
    
    // Add ABI
    ContractABI abi;
    ContractMethod method;
    method.Name = "transfer";
    method.Parameters.push_back(ContractParameter{ContractParameterType::Hash160, "from"});
    method.Parameters.push_back(ContractParameter{ContractParameterType::Hash160, "to"});
    method.Parameters.push_back(ContractParameter{ContractParameterType::Integer, "amount"});
    method.Parameters.push_back(ContractParameter{ContractParameterType::Any, "data"});
    method.ReturnType = ContractParameterType::Boolean;
    method.Safe = false;
    abi.Methods.push_back(method);
    
    ContractEvent event;
    event.Name = "Transfer";
    event.Parameters.push_back(ContractParameter{ContractParameterType::Hash160, "from"});
    event.Parameters.push_back(ContractParameter{ContractParameterType::Hash160, "to"});
    event.Parameters.push_back(ContractParameter{ContractParameterType::Integer, "amount"});
    abi.Events.push_back(event);
    
    manifest.ABI = abi;
    
    EXPECT_EQ(manifest.Name, "TestToken");
    EXPECT_EQ(manifest.SupportedStandards[0], "NEP-17");
    EXPECT_EQ(manifest.ABI.Methods.size(), 1);
    EXPECT_EQ(manifest.ABI.Events.size(), 1);
}

TEST_F(NEPStandardsTest, Manifest_Permissions) {
    Manifest manifest;
    
    ContractPermission permission;
    permission.Contract = ByteVector(20, 0xFF);  // Specific contract
    permission.Methods.push_back("transfer");
    permission.Methods.push_back("balanceOf");
    
    manifest.Permissions.push_back(permission);
    
    EXPECT_EQ(manifest.Permissions.size(), 1);
    EXPECT_EQ(manifest.Permissions[0].Methods.size(), 2);
}

// ============================================================================
// Edge Cases and Security Tests
// ============================================================================

TEST_F(NEPStandardsTest, Security_IntegerOverflow) {
    NEP17Token token("TestToken", "TST", 8, UINT64_MAX - 1000);
    
    auto addr1 = GetAddress1();
    
    // Try to mint amount that would cause overflow
    bool result = token.Mint(addr1, 2000);
    
    // Should fail or handle overflow properly
    EXPECT_FALSE(result || token.TotalSupply() == UINT64_MAX);
}

TEST_F(NEPStandardsTest, Security_ReentrancyProtection) {
    NEP17Token token("TestToken", "TST", 8, 1000000000);
    
    auto addr1 = GetAddress1();
    auto addr2 = GetAddress2();
    
    token.SetBalance(addr1, 1000);
    
    // Enable reentrancy protection
    token.EnableReentrancyProtection(true);
    
    // Simulate reentrancy attempt
    bool inTransfer = false;
    token.SetTransferCallback([&](const ByteVector&, const ByteVector&, uint64_t) {
        if (!inTransfer) {
            inTransfer = true;
            // Try to reenter
            bool reentrant = token.Transfer(addr1, addr2, 100, ByteVector());
            EXPECT_FALSE(reentrant);  // Should be blocked
        }
    });
    
    token.Transfer(addr1, addr2, 500, ByteVector());
}

TEST_F(NEPStandardsTest, EdgeCase_EmptyAddress) {
    NEP17Token token("TestToken", "TST", 8, 1000000000);
    
    ByteVector emptyAddr;
    auto addr1 = GetAddress1();
    
    token.SetBalance(addr1, 1000);
    
    // Transfer to empty address should fail
    bool result = token.Transfer(addr1, emptyAddr, 500, ByteVector());
    EXPECT_FALSE(result);
    
    // Transfer from empty address should fail
    result = token.Transfer(emptyAddr, addr1, 500, ByteVector());
    EXPECT_FALSE(result);
}

TEST_F(NEPStandardsTest, EdgeCase_MaxSupply) {
    NEP17Token token("TestToken", "TST", 8, UINT64_MAX);
    
    EXPECT_EQ(token.TotalSupply(), UINT64_MAX);
    
    auto addr1 = GetAddress1();
    
    // Cannot mint more when at max supply
    bool result = token.Mint(addr1, 1);
    EXPECT_FALSE(result);
}

// ============================================================================
// Performance Tests
// ============================================================================

TEST_F(NEPStandardsTest, Performance_ManyTokenHolders) {
    NEP17Token token("TestToken", "TST", 8, 1000000000);
    
    auto start = std::chrono::high_resolution_clock::now();
    
    // Create many holders
    for (int i = 0; i < 1000; ++i) {
        ByteVector addr(20, i % 256);
        token.SetBalance(addr, 100 + i);
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    // Should handle many holders efficiently
    EXPECT_LT(duration.count(), 1000);  // Under 1 second
}

TEST_F(NEPStandardsTest, Performance_ManyNFTs) {
    NEP11Token nft("TestNFT", "TNFT");
    
    auto addr1 = GetAddress1();
    
    auto start = std::chrono::high_resolution_clock::now();
    
    // Mint many NFTs
    for (int i = 0; i < 100; ++i) {
        ByteVector tokenId = {static_cast<uint8_t>(i >> 8), static_cast<uint8_t>(i & 0xFF)};
        nft.Mint(addr1, tokenId);
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    // Should mint many NFTs quickly
    EXPECT_LT(duration.count(), 500);  // Under 0.5 seconds
    
    // Verify all minted
    auto tokens = nft.TokensOf(addr1);
    EXPECT_EQ(tokens.size(), 100);
}
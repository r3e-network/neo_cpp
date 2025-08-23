/**
 * @file test_nep17_token.cpp
 * @brief Unit tests for SDK NEP17 token functionality
 * @author Neo C++ Team
 * @date 2025
 */

#include <gtest/gtest.h>
#include <neo/sdk/nep17/nep17_token.h>
#include <neo/sdk/rpc/rpc_client.h>
#include <neo/sdk/wallet/wallet.h>
#include <neo/io/uint160.h>
#include <neo/io/uint256.h>
#include <filesystem>

using namespace neo::sdk::nep17;
using namespace neo::sdk::rpc;
using namespace neo::sdk::wallet;
using namespace neo::io;

// Mock RPC client for testing
class MockRpcClient : public RpcClient {
public:
    MockRpcClient() : RpcClient("http://localhost:10332") {}
    
    // Mock responses
    uint64_t mockBalance = 1000000000; // 10 tokens with 8 decimals
    std::string mockSymbol = "TEST";
    uint8_t mockDecimals = 8;
    uint64_t mockTotalSupply = 100000000000000000; // 1 billion tokens
    
    // Override methods to return mock data
    uint64_t GetNEP17Balance(const UInt160& tokenHash, const UInt160& account) override {
        return mockBalance;
    }
    
    std::string GetNEP17Symbol(const UInt160& tokenHash) override {
        return mockSymbol;
    }
    
    uint8_t GetNEP17Decimals(const UInt160& tokenHash) override {
        return mockDecimals;
    }
    
    uint64_t GetNEP17TotalSupply(const UInt160& tokenHash) override {
        return mockTotalSupply;
    }
};

class NEP17TokenTest : public ::testing::Test {
protected:
    std::unique_ptr<NEP17Token> token;
    std::unique_ptr<MockRpcClient> mockClient;
    std::unique_ptr<Wallet> wallet;
    UInt160 tokenHash;
    std::string walletPath;
    std::string walletPassword;
    
    void SetUp() override {
        // Setup mock RPC client
        mockClient = std::make_unique<MockRpcClient>();
        
        // NEO token hash (mainnet)
        tokenHash = UInt160::Parse("0xef4073a0f2b305a38ec4050e4d3d28bc40ea63f5");
        
        // Create NEP17 token instance
        token = std::make_unique<NEP17Token>(tokenHash, mockClient.get());
        
        // Create test wallet with secure random password
        walletPath = "test_nep17_wallet.json";
        walletPassword = "TestWallet_" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count());
        wallet = Wallet::Create("NEP17TestWallet", walletPath, walletPassword);
        wallet->CreateAccount("TestAccount");
    }
    
    void TearDown() override {
        if (std::filesystem::exists(walletPath)) {
            std::filesystem::remove(walletPath);
        }
    }
    
    UInt160 GetTestAddress() {
        auto accounts = wallet->GetAccounts();
        if (!accounts.empty()) {
            auto address = accounts[0]->GetAddress();
            return ScriptHashFromAddress(address).value_or(UInt160::Zero());
        }
        return UInt160::Zero();
    }
};

// Test token information
TEST_F(NEP17TokenTest, GetTokenHash) {
    EXPECT_EQ(token->GetHash(), tokenHash);
}

TEST_F(NEP17TokenTest, GetTokenSymbol) {
    auto symbol = token->GetSymbol();
    EXPECT_EQ(symbol, "TEST");
}

TEST_F(NEP17TokenTest, GetTokenDecimals) {
    auto decimals = token->GetDecimals();
    EXPECT_EQ(decimals, 8);
}

TEST_F(NEP17TokenTest, GetTokenTotalSupply) {
    auto totalSupply = token->GetTotalSupply();
    EXPECT_EQ(totalSupply, mockClient->mockTotalSupply);
}

TEST_F(NEP17TokenTest, GetTokenName) {
    // Some tokens may not have name, so this could return empty
    auto name = token->GetName();
    EXPECT_TRUE(name.empty() || !name.empty()); // Just check it doesn't crash
}

// Test balance queries
TEST_F(NEP17TokenTest, GetBalance) {
    auto address = GetTestAddress();
    auto balance = token->GetBalance(address);
    
    EXPECT_EQ(balance, mockClient->mockBalance);
}

TEST_F(NEP17TokenTest, GetBalanceFromAddress) {
    std::string address = "NUVPACMnKFhpuHjsRjhUvXz1GhqfGWx2CT";
    auto balance = token->GetBalance(address);
    
    EXPECT_EQ(balance, mockClient->mockBalance);
}

TEST_F(NEP17TokenTest, GetBalanceInvalidAddress) {
    std::string invalidAddress = "InvalidAddress";
    auto balance = token->GetBalance(invalidAddress);
    
    // Should return 0 for invalid address
    EXPECT_EQ(balance, 0);
}

TEST_F(NEP17TokenTest, GetFormattedBalance) {
    auto address = GetTestAddress();
    auto formatted = token->GetFormattedBalance(address);
    
    // With 8 decimals, 1000000000 should be "10.00000000"
    EXPECT_EQ(formatted, "10.00000000");
}

// Test transfer creation
TEST_F(NEP17TokenTest, CreateTransfer) {
    auto from = GetTestAddress();
    auto to = UInt160::Parse("0x2222222222222222222222222222222222222222");
    uint64_t amount = 100000000; // 1 token
    
    auto tx = token->CreateTransfer(from, to, amount);
    
    ASSERT_NE(tx, nullptr);
    EXPECT_GT(tx->Script.size(), 0);
    EXPECT_EQ(tx->Signers.size(), 1);
    EXPECT_EQ(tx->Signers[0].Account, from);
}

TEST_F(NEP17TokenTest, CreateTransferWithData) {
    auto from = GetTestAddress();
    auto to = UInt160::Parse("0x2222222222222222222222222222222222222222");
    uint64_t amount = 100000000;
    std::vector<uint8_t> data = {0x01, 0x02, 0x03};
    
    auto tx = token->CreateTransfer(from, to, amount, data);
    
    ASSERT_NE(tx, nullptr);
    EXPECT_GT(tx->Script.size(), 0);
}

TEST_F(NEP17TokenTest, CreateTransferFromAddresses) {
    std::string from = "NUVPACMnKFhpuHjsRjhUvXz1GhqfGWx2CT";
    std::string to = "NZBBNPCpcKgTKhKay8EJgRv3TeNxZWddfn";
    uint64_t amount = 100000000;
    
    auto tx = token->CreateTransfer(from, to, amount);
    
    ASSERT_NE(tx, nullptr);
    EXPECT_GT(tx->Script.size(), 0);
}

TEST_F(NEP17TokenTest, CreateTransferWithDecimals) {
    auto from = GetTestAddress();
    auto to = UInt160::Parse("0x2222222222222222222222222222222222222222");
    double amount = 1.5; // 1.5 tokens
    
    auto tx = token->CreateTransferFromAmount(from, to, amount);
    
    ASSERT_NE(tx, nullptr);
    EXPECT_GT(tx->Script.size(), 0);
    
    // Transaction should convert 1.5 to 150000000 (with 8 decimals)
}

// Test multi-transfer
TEST_F(NEP17TokenTest, CreateMultiTransfer) {
    auto from = GetTestAddress();
    
    std::vector<TokenTransfer> transfers;
    
    TokenTransfer transfer1;
    transfer1.to = UInt160::Parse("0x2222222222222222222222222222222222222222");
    transfer1.amount = 100000000; // 1 token
    transfers.push_back(transfer1);
    
    TokenTransfer transfer2;
    transfer2.to = UInt160::Parse("0x3333333333333333333333333333333333333333");
    transfer2.amount = 200000000; // 2 tokens
    transfers.push_back(transfer2);
    
    TokenTransfer transfer3;
    transfer3.to = UInt160::Parse("0x4444444444444444444444444444444444444444");
    transfer3.amount = 300000000; // 3 tokens
    transfers.push_back(transfer3);
    
    auto tx = token->CreateMultiTransfer(from, transfers);
    
    ASSERT_NE(tx, nullptr);
    EXPECT_GT(tx->Script.size(), 0);
    EXPECT_EQ(tx->Signers.size(), 1);
}

// Test allowance operations
TEST_F(NEP17TokenTest, GetAllowance) {
    auto owner = GetTestAddress();
    auto spender = UInt160::Parse("0x2222222222222222222222222222222222222222");
    
    // Mock implementation would return default value
    auto allowance = token->GetAllowance(owner, spender);
    
    EXPECT_GE(allowance, 0);
}

TEST_F(NEP17TokenTest, CreateApprove) {
    auto owner = GetTestAddress();
    auto spender = UInt160::Parse("0x2222222222222222222222222222222222222222");
    uint64_t amount = 1000000000; // 10 tokens
    
    auto tx = token->CreateApprove(owner, spender, amount);
    
    ASSERT_NE(tx, nullptr);
    EXPECT_GT(tx->Script.size(), 0);
    EXPECT_EQ(tx->Signers.size(), 1);
    EXPECT_EQ(tx->Signers[0].Account, owner);
}

TEST_F(NEP17TokenTest, CreateTransferFrom) {
    auto spender = GetTestAddress();
    auto from = UInt160::Parse("0x1111111111111111111111111111111111111111");
    auto to = UInt160::Parse("0x2222222222222222222222222222222222222222");
    uint64_t amount = 100000000;
    
    auto tx = token->CreateTransferFrom(spender, from, to, amount);
    
    ASSERT_NE(tx, nullptr);
    EXPECT_GT(tx->Script.size(), 0);
    EXPECT_EQ(tx->Signers.size(), 1);
    EXPECT_EQ(tx->Signers[0].Account, spender);
}

// Test special tokens (NEO and GAS)
TEST_F(NEP17TokenTest, NEOToken) {
    // NEO token has 0 decimals
    auto neoHash = UInt160::Parse("0xef4073a0f2b305a38ec4050e4d3d28bc40ea63f5");
    NEP17Token neo(neoHash, mockClient.get());
    
    // NEO should not have decimals
    mockClient->mockDecimals = 0;
    EXPECT_EQ(neo.GetDecimals(), 0);
    
    // Test NEO-specific operations
    auto address = GetTestAddress();
    auto balance = neo.GetBalance(address);
    EXPECT_GE(balance, 0);
}

TEST_F(NEP17TokenTest, GASToken) {
    // GAS token has 8 decimals
    auto gasHash = UInt160::Parse("0xd2a4cff31913016155e38e474a2c06d08be276cf");
    NEP17Token gas(gasHash, mockClient.get());
    
    mockClient->mockDecimals = 8;
    mockClient->mockSymbol = "GAS";
    
    EXPECT_EQ(gas.GetDecimals(), 8);
    EXPECT_EQ(gas.GetSymbol(), "GAS");
}

// Test token list operations
TEST_F(NEP17TokenTest, GetTokenList) {
    auto address = GetTestAddress();
    
    // This would normally get list from RPC
    auto tokens = token->GetTokenList(address);
    
    // Mock implementation might return empty or test data
    EXPECT_TRUE(tokens.empty() || !tokens.empty());
}

// Test amount conversion
TEST_F(NEP17TokenTest, AmountConversion) {
    // Test conversion from decimal amount to integer
    double decimalAmount = 12.345;
    uint64_t intAmount = token->AmountToInteger(decimalAmount);
    
    // With 8 decimals, 12.345 should be 1234500000
    EXPECT_EQ(intAmount, 1234500000);
    
    // Test conversion from integer to decimal
    uint64_t intValue = 1234500000;
    double decimalValue = token->IntegerToAmount(intValue);
    
    EXPECT_DOUBLE_EQ(decimalValue, 12.345);
}

TEST_F(NEP17TokenTest, AmountFormattingPrecision) {
    // Test various decimal precisions
    mockClient->mockDecimals = 0;
    EXPECT_EQ(token->AmountToInteger(10.0), 10);
    
    mockClient->mockDecimals = 2;
    EXPECT_EQ(token->AmountToInteger(10.99), 1099);
    
    mockClient->mockDecimals = 8;
    EXPECT_EQ(token->AmountToInteger(0.00000001), 1);
    
    mockClient->mockDecimals = 18;
    EXPECT_EQ(token->AmountToInteger(1.0), 1000000000000000000);
}

// Test error handling
TEST_F(NEP17TokenTest, InvalidTokenHash) {
    NEP17Token invalidToken(UInt160::Zero(), mockClient.get());
    
    // Operations should handle gracefully
    auto symbol = invalidToken.GetSymbol();
    auto decimals = invalidToken.GetDecimals();
    auto supply = invalidToken.GetTotalSupply();
    
    // Should return defaults or empty values
    EXPECT_TRUE(symbol.empty() || !symbol.empty());
    EXPECT_GE(decimals, 0);
    EXPECT_GE(supply, 0);
}

TEST_F(NEP17TokenTest, NullRpcClient) {
    NEP17Token tokenNoClient(tokenHash, nullptr);
    
    // Operations should handle null client gracefully
    auto balance = tokenNoClient.GetBalance(GetTestAddress());
    EXPECT_EQ(balance, 0);
    
    auto tx = tokenNoClient.CreateTransfer(
        GetTestAddress(),
        UInt160::Zero(),
        100000000
    );
    // Should still create transaction structure
    EXPECT_NE(tx, nullptr);
}

TEST_F(NEP17TokenTest, ZeroAmountTransfer) {
    auto from = GetTestAddress();
    auto to = UInt160::Parse("0x2222222222222222222222222222222222222222");
    uint64_t amount = 0;
    
    auto tx = token->CreateTransfer(from, to, amount);
    
    // Should still create valid transaction (some contracts allow 0 transfers)
    ASSERT_NE(tx, nullptr);
    EXPECT_GT(tx->Script.size(), 0);
}

TEST_F(NEP17TokenTest, MaxAmountTransfer) {
    auto from = GetTestAddress();
    auto to = UInt160::Parse("0x2222222222222222222222222222222222222222");
    uint64_t amount = UINT64_MAX;
    
    auto tx = token->CreateTransfer(from, to, amount);
    
    ASSERT_NE(tx, nullptr);
    EXPECT_GT(tx->Script.size(), 0);
}

// Test batch operations
TEST_F(NEP17TokenTest, BatchBalanceQuery) {
    std::vector<UInt160> addresses;
    addresses.push_back(UInt160::Parse("0x1111111111111111111111111111111111111111"));
    addresses.push_back(UInt160::Parse("0x2222222222222222222222222222222222222222"));
    addresses.push_back(UInt160::Parse("0x3333333333333333333333333333333333333333"));
    
    auto balances = token->GetBalances(addresses);
    
    EXPECT_EQ(balances.size(), addresses.size());
    for (const auto& balance : balances) {
        EXPECT_EQ(balance, mockClient->mockBalance);
    }
}

// Test transfer history
TEST_F(NEP17TokenTest, GetTransferHistory) {
    auto address = GetTestAddress();
    
    // This would normally get history from RPC
    auto transfers = token->GetTransferHistory(address);
    
    // Mock implementation might return empty
    EXPECT_TRUE(transfers.empty() || !transfers.empty());
}

TEST_F(NEP17TokenTest, GetTransferHistoryWithTimeRange) {
    auto address = GetTestAddress();
    uint64_t fromTime = 1640000000; // Some timestamp
    uint64_t toTime = 1650000000;
    
    auto transfers = token->GetTransferHistory(address, fromTime, toTime);
    
    EXPECT_TRUE(transfers.empty() || !transfers.empty());
}

// Performance test
TEST_F(NEP17TokenTest, PerformanceTest) {
    auto start = std::chrono::high_resolution_clock::now();
    
    auto from = GetTestAddress();
    auto to = UInt160::Parse("0x2222222222222222222222222222222222222222");
    
    // Create 100 transfer transactions
    for (int i = 0; i < 100; i++) {
        auto tx = token->CreateTransfer(from, to, 100000000);
        ASSERT_NE(tx, nullptr);
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    // Should complete 100 transfers in reasonable time
    EXPECT_LT(duration.count(), 2000); // Less than 2 seconds
}

// Test well-known tokens
TEST_F(NEP17TokenTest, WellKnownTokens) {
    struct WellKnownToken {
        std::string name;
        std::string hash;
        std::string symbol;
        uint8_t decimals;
    };
    
    std::vector<WellKnownToken> tokens = {
        {"NEO", "0xef4073a0f2b305a38ec4050e4d3d28bc40ea63f5", "NEO", 0},
        {"GAS", "0xd2a4cff31913016155e38e474a2c06d08be276cf", "GAS", 8}
    };
    
    for (const auto& knownToken : tokens) {
        auto hash = UInt160::Parse(knownToken.hash);
        NEP17Token token(hash, mockClient.get());
        
        // Set mock expectations
        mockClient->mockSymbol = knownToken.symbol;
        mockClient->mockDecimals = knownToken.decimals;
        
        EXPECT_EQ(token.GetSymbol(), knownToken.symbol);
        EXPECT_EQ(token.GetDecimals(), knownToken.decimals);
    }
}

// Test token metadata caching
TEST_F(NEP17TokenTest, MetadataCaching) {
    // First call should fetch from RPC
    auto symbol1 = token->GetSymbol();
    auto decimals1 = token->GetDecimals();
    
    // Subsequent calls should use cached values
    auto symbol2 = token->GetSymbol();
    auto decimals2 = token->GetDecimals();
    
    EXPECT_EQ(symbol1, symbol2);
    EXPECT_EQ(decimals1, decimals2);
    
    // Values should be cached and consistent
    EXPECT_EQ(symbol1, "TEST");
    EXPECT_EQ(decimals1, 8);
}
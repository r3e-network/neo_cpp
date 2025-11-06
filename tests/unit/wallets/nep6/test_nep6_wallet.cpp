#include <gtest/gtest.h>
#include <neo/cryptography/ecc/secp256r1.h>
#include <neo/wallets/nep6/nep6_wallet.h>

#include <filesystem>
#include <memory>
#include <random>

using namespace neo::wallets;
using namespace neo::wallets::nep6;
using namespace neo::cryptography::ecc;

namespace
{
std::string MakeTempWalletPath()
{
    auto base = std::filesystem::temp_directory_path();
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int> dis(0, std::numeric_limits<int>::max());
    std::filesystem::path dir = base / ("nep6_wallet_test_" + std::to_string(dis(gen)));
    std::filesystem::create_directories(dir);
    return (dir / "wallet.json").string();
}

void CleanupPath(const std::string& path)
{
    std::error_code ec;
    std::filesystem::remove_all(std::filesystem::path(path).parent_path(), ec);
}
}  // namespace

class Nep6WalletTest : public ::testing::Test
{
  protected:
    std::string walletPath_;
    std::unique_ptr<NEP6Wallet> wallet_;

    void SetUp() override
    {
        walletPath_ = MakeTempWalletPath();
        wallet_ = std::make_unique<NEP6Wallet>(walletPath_, "test-password", "unit-wallet");
        wallet_->SetScrypt(ScryptParameters(16384, 8, 8));
    }

    void TearDown() override
    {
        wallet_.reset();
        CleanupPath(walletPath_);
    }
};

TEST_F(Nep6WalletTest, CreateAccountProducesNEP2)
{
    auto account = wallet_->CreateAccount();
    ASSERT_NE(account, nullptr);

    auto nep6Account = std::dynamic_pointer_cast<NEP6Account>(account);
    ASSERT_NE(nep6Account, nullptr);

    const auto& nep2 = nep6Account->GetNEP2Key();
    EXPECT_FALSE(nep2.empty());
    EXPECT_TRUE(nep2.rfind("6P", 0) == 0);

    // Ensure decrypted private key round-trips
    auto privateKey = Secp256r1::FromNEP2(nep2, wallet_->GetPassword());
    auto reencoded = Secp256r1::ToNEP2(privateKey, wallet_->GetPassword());
    EXPECT_EQ(reencoded, nep2);
}

TEST_F(Nep6WalletTest, ChangePasswordReencryptsAccounts)
{
    auto account = wallet_->CreateAccount();
    auto nep6Account = std::dynamic_pointer_cast<NEP6Account>(account);
    ASSERT_NE(nep6Account, nullptr);

    auto originalNep2 = nep6Account->GetNEP2Key();
    ASSERT_FALSE(originalNep2.empty());

    std::string newPassword = "new-secret";
    ASSERT_TRUE(wallet_->ChangePassword("test-password", newPassword));

    auto updatedNep2 = nep6Account->GetNEP2Key();
    EXPECT_FALSE(updatedNep2.empty());
    EXPECT_NE(originalNep2, updatedNep2);

    // Old password should fail
    EXPECT_THROW(Secp256r1::FromNEP2(updatedNep2, "test-password"), std::exception);

    // New password should decrypt
    auto decrypted = Secp256r1::FromNEP2(updatedNep2, newPassword);
    EXPECT_EQ(decrypted.size(), 32u);
}

TEST_F(Nep6WalletTest, ImportFromNEP2AddsAccount)
{
    // Known vector from Neo test suite
    const std::string nep2 = "6PYKsHXhWUNUrWAYmTfL692qqmmrihFQVTQEXuDKpxss86FxxgurkvAwZN";
    const std::string password = "test123";

    auto imported = wallet_->ImportFromNEP2(nep2, password);
    ASSERT_NE(imported, nullptr);

    auto nep6Account = std::dynamic_pointer_cast<NEP6Account>(imported);
    ASSERT_NE(nep6Account, nullptr);
    EXPECT_EQ(nep6Account->GetNEP2Key(), nep2);

    // Verify password works through account helper
    EXPECT_TRUE(nep6Account->VerifyPassword(password, wallet_->GetScrypt()));

    // Wallet should now return this account via lookup
    auto fetched = wallet_->GetAccount(imported->GetScriptHash());
    EXPECT_EQ(fetched.get(), imported.get());
}

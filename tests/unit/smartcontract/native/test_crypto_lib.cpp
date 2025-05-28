#include <gtest/gtest.h>
#include <neo/smartcontract/native/crypto_lib.h>
#include <neo/smartcontract/application_engine.h>
#include <neo/persistence/memory_store_view.h>
#include <neo/cryptography/hash.h>
#include <neo/cryptography/ecc/secp256r1.h>
#include <neo/cryptography/bls12_381.h>
#include <neo/io/binary_reader.h>
#include <neo/io/binary_writer.h>
#include <sstream>

using namespace neo::smartcontract::native;
using namespace neo::persistence;
using namespace neo::io;
using namespace neo::vm;
using namespace neo::cryptography;

class CryptoLibTest : public ::testing::Test
{
protected:
    std::shared_ptr<MemoryStoreView> snapshot;
    std::shared_ptr<CryptoLib> cryptoLib;
    std::shared_ptr<ApplicationEngine> engine;

    void SetUp() override
    {
        snapshot = std::make_shared<MemoryStoreView>();
        cryptoLib = std::make_shared<CryptoLib>();
        engine = std::make_shared<ApplicationEngine>(TriggerType::Application, nullptr, snapshot, 0, false);
    }
};

TEST_F(CryptoLibTest, TestSha256)
{
    // Create input data
    ByteVector data = ByteVector::Parse("010203");

    // Create arguments
    std::vector<std::shared_ptr<StackItem>> args;
    args.push_back(StackItem::Create(data));

    // Call the method
    auto result = cryptoLib->Call(*engine, "sha256", args);

    // Check the result
    ASSERT_TRUE(result->IsBuffer());
    auto resultBytes = result->GetByteArray();

    // Calculate the expected hash
    auto expectedHash = Hash::Sha256(data.AsSpan());
    ByteVector expectedBytes(ByteSpan(expectedHash.Data(), expectedHash.Size()));

    // Compare the results
    ASSERT_EQ(resultBytes, expectedBytes);
}

TEST_F(CryptoLibTest, TestRipemd160)
{
    // Create input data
    ByteVector data = ByteVector::Parse("010203");

    // Create arguments
    std::vector<std::shared_ptr<StackItem>> args;
    args.push_back(StackItem::Create(data));

    // Call the method
    auto result = cryptoLib->Call(*engine, "ripemd160", args);

    // Check the result
    ASSERT_TRUE(result->IsBuffer());
    auto resultBytes = result->GetByteArray();

    // Calculate the expected hash
    auto expectedHash = Hash::Ripemd160(data.AsSpan());
    ByteVector expectedBytes(ByteSpan(expectedHash.Data(), expectedHash.Size()));

    // Compare the results
    ASSERT_EQ(resultBytes, expectedBytes);
}

TEST_F(CryptoLibTest, TestHash160)
{
    // Create input data
    ByteVector data = ByteVector::Parse("010203");

    // Create arguments
    std::vector<std::shared_ptr<StackItem>> args;
    args.push_back(StackItem::Create(data));

    // Call the method
    auto result = cryptoLib->Call(*engine, "hash160", args);

    // Check the result
    ASSERT_TRUE(result->IsBuffer());
    auto resultBytes = result->GetByteArray();

    // Calculate the expected hash
    auto expectedHash = Hash::Hash160(data.AsSpan());
    ByteVector expectedBytes(ByteSpan(expectedHash.Data(), expectedHash.Size()));

    // Compare the results
    ASSERT_EQ(resultBytes, expectedBytes);
}

TEST_F(CryptoLibTest, TestHash256)
{
    // Create input data
    ByteVector data = ByteVector::Parse("010203");

    // Create arguments
    std::vector<std::shared_ptr<StackItem>> args;
    args.push_back(StackItem::Create(data));

    // Call the method
    auto result = cryptoLib->Call(*engine, "hash256", args);

    // Check the result
    ASSERT_TRUE(result->IsBuffer());
    auto resultBytes = result->GetByteArray();

    // Calculate the expected hash
    auto expectedHash = Hash::Hash256(data.AsSpan());
    ByteVector expectedBytes(ByteSpan(expectedHash.Data(), expectedHash.Size()));

    // Compare the results
    ASSERT_EQ(resultBytes, expectedBytes);
}

TEST_F(CryptoLibTest, TestVerifySignature)
{
    // Generate a key pair
    ByteVector privateKey = ByteVector::Parse("0102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f20");
    auto publicKey = ecc::Secp256r1::GeneratePublicKey(privateKey.AsSpan());

    // Create a message
    ByteVector message = ByteVector::Parse("010203");

    // Sign the message
    auto signature = ecc::Secp256r1::Sign(message.AsSpan(), privateKey.AsSpan());

    // Create arguments
    std::vector<std::shared_ptr<StackItem>> args;
    args.push_back(StackItem::Create(message));
    args.push_back(StackItem::Create(publicKey.ToArray()));
    args.push_back(StackItem::Create(signature));

    // Call the method
    auto result = cryptoLib->Call(*engine, "verifySignature", args);

    // Check the result
    ASSERT_TRUE(result->GetBoolean());

    // Modify the message
    message = ByteVector::Parse("010204");
    args[0] = StackItem::Create(message);

    // Call the method again
    result = cryptoLib->Call(*engine, "verifySignature", args);

    // Check the result
    ASSERT_FALSE(result->GetBoolean());
}

TEST_F(CryptoLibTest, TestVerifyWithECDsa)
{
    // Generate a key pair
    ByteVector privateKey = ByteVector::Parse("0102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f20");
    auto publicKey = ecc::Secp256r1::GeneratePublicKey(privateKey.AsSpan());

    // Create a message
    ByteVector message = ByteVector::Parse("010203");

    // Sign the message
    auto signature = ecc::Secp256r1::Sign(message.AsSpan(), privateKey.AsSpan());

    // Create arguments
    std::vector<std::shared_ptr<StackItem>> args;
    args.push_back(StackItem::Create(message));
    args.push_back(StackItem::Create(publicKey.ToArray()));
    args.push_back(StackItem::Create(signature));
    args.push_back(StackItem::Create("secp256r1"));

    // Call the method
    auto result = cryptoLib->Call(*engine, "verifyWithECDsa", args);

    // Check the result
    ASSERT_TRUE(result->GetBoolean());

    // Modify the message
    message = ByteVector::Parse("010204");
    args[0] = StackItem::Create(message);

    // Call the method again
    result = cryptoLib->Call(*engine, "verifyWithECDsa", args);

    // Check the result
    ASSERT_FALSE(result->GetBoolean());
}

TEST_F(CryptoLibTest, TestBls12381SerializeDeserializeG1)
{
    // Create a G1 point
    auto g1 = std::make_shared<bls12_381::G1Point>(bls12_381::G1Point::Generator());

    // Create arguments
    std::vector<std::shared_ptr<StackItem>> args;
    args.push_back(StackItem::Create(g1));

    // Call the serialize method
    auto result = cryptoLib->Call(*engine, "bls12381Serialize", args);

    // Check the result
    ASSERT_TRUE(result->IsBuffer());
    auto serialized = result->GetByteArray();

    // Call the deserialize method
    args.clear();
    args.push_back(StackItem::Create(serialized));
    auto deserialized = cryptoLib->Call(*engine, "bls12381Deserialize", args);

    // Check the result
    ASSERT_TRUE(deserialized->IsInterop());
    auto deserializedG1 = std::dynamic_pointer_cast<bls12_381::G1Point>(deserialized->GetInterface());
    ASSERT_TRUE(deserializedG1);

    // Check if the points are equal
    ASSERT_EQ(*g1, *deserializedG1);
}

TEST_F(CryptoLibTest, TestBls12381SerializeDeserializeG2)
{
    // Create a G2 point
    auto g2 = std::make_shared<bls12_381::G2Point>(bls12_381::G2Point::Generator());

    // Create arguments
    std::vector<std::shared_ptr<StackItem>> args;
    args.push_back(StackItem::Create(g2));

    // Call the serialize method
    auto result = cryptoLib->Call(*engine, "bls12381Serialize", args);

    // Check the result
    ASSERT_TRUE(result->IsBuffer());
    auto serialized = result->GetByteArray();

    // Call the deserialize method
    args.clear();
    args.push_back(StackItem::Create(serialized));
    auto deserialized = cryptoLib->Call(*engine, "bls12381Deserialize", args);

    // Check the result
    ASSERT_TRUE(deserialized->IsInterop());
    auto deserializedG2 = std::dynamic_pointer_cast<bls12_381::G2Point>(deserialized->GetInterface());
    ASSERT_TRUE(deserializedG2);

    // Check if the points are equal
    ASSERT_EQ(*g2, *deserializedG2);
}

TEST_F(CryptoLibTest, TestBls12381Equal)
{
    // Create two G1 points
    auto g1a = std::make_shared<bls12_381::G1Point>(bls12_381::G1Point::Generator());
    auto g1b = std::make_shared<bls12_381::G1Point>(bls12_381::G1Point::Generator());
    auto g1c = std::make_shared<bls12_381::G1Point>(g1a->Add(*g1b));

    // Create arguments for equal points
    std::vector<std::shared_ptr<StackItem>> args;
    args.push_back(StackItem::Create(g1a));
    args.push_back(StackItem::Create(g1b));

    // Call the equal method
    auto result = cryptoLib->Call(*engine, "bls12381Equal", args);

    // Check the result
    ASSERT_TRUE(result->GetBoolean());

    // Create arguments for different points
    args.clear();
    args.push_back(StackItem::Create(g1a));
    args.push_back(StackItem::Create(g1c));

    // Call the equal method
    result = cryptoLib->Call(*engine, "bls12381Equal", args);

    // Check the result
    ASSERT_FALSE(result->GetBoolean());
}

TEST_F(CryptoLibTest, TestBls12381Add)
{
    // Create two G1 points
    auto g1a = std::make_shared<bls12_381::G1Point>(bls12_381::G1Point::Generator());
    auto g1b = std::make_shared<bls12_381::G1Point>(bls12_381::G1Point::Generator());

    // Create arguments
    std::vector<std::shared_ptr<StackItem>> args;
    args.push_back(StackItem::Create(g1a));
    args.push_back(StackItem::Create(g1b));

    // Call the add method
    auto result = cryptoLib->Call(*engine, "bls12381Add", args);

    // Check the result
    ASSERT_TRUE(result->IsInterop());
    auto resultG1 = std::dynamic_pointer_cast<bls12_381::G1Point>(result->GetInterface());
    ASSERT_TRUE(resultG1);

    // Calculate the expected result
    auto expected = g1a->Add(*g1b);

    // Check if the points are equal
    ASSERT_EQ(*resultG1, expected);
}

TEST_F(CryptoLibTest, TestBls12381Mul)
{
    // Create a G1 point
    auto g1 = std::make_shared<bls12_381::G1Point>(bls12_381::G1Point::Generator());

    // Create a scalar
    ByteVector scalar = ByteVector::Parse("0000000000000000000000000000000000000000000000000000000000000002");

    // Create arguments
    std::vector<std::shared_ptr<StackItem>> args;
    args.push_back(StackItem::Create(g1));
    args.push_back(StackItem::Create(scalar));
    args.push_back(StackItem::Create(false));

    // Call the mul method
    auto result = cryptoLib->Call(*engine, "bls12381Mul", args);

    // Check the result
    ASSERT_TRUE(result->IsInterop());
    auto resultG1 = std::dynamic_pointer_cast<bls12_381::G1Point>(result->GetInterface());
    ASSERT_TRUE(resultG1);

    // Calculate the expected result
    auto expected = g1->Multiply(scalar.AsSpan());

    // Check if the points are equal
    ASSERT_EQ(*resultG1, expected);
}

TEST_F(CryptoLibTest, TestBls12381Pairing)
{
    // Create a G1 point and a G2 point
    auto g1 = std::make_shared<bls12_381::G1Point>(bls12_381::G1Point::Generator());
    auto g2 = std::make_shared<bls12_381::G2Point>(bls12_381::G2Point::Generator());

    // Create arguments
    std::vector<std::shared_ptr<StackItem>> args;
    args.push_back(StackItem::Create(g1));
    args.push_back(StackItem::Create(g2));

    // Call the pairing method
    auto result = cryptoLib->Call(*engine, "bls12381Pairing", args);

    // Check the result
    ASSERT_TRUE(result->IsInterop());
    auto resultGT = std::dynamic_pointer_cast<bls12_381::GTPoint>(result->GetInterface());
    ASSERT_TRUE(resultGT);

    // Calculate the expected result
    auto expected = bls12_381::Pairing(*g1, *g2);

    // Check if the points are equal
    ASSERT_EQ(*resultGT, expected);
}

int main(int argc, char** argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

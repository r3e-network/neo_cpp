#include <iomanip>
#include <iostream>
#include <neo/cryptography/bls12_381.h>
#include <neo/cryptography/crypto.h>
#include <neo/cryptography/hash.h>
#include <neo/io/byte_vector.h>

using namespace neo::cryptography;
using namespace neo::cryptography::bls12_381;
using namespace neo::io;

void PrintBytes(const std::string& label, const ByteVector& bytes)
{
    std::cout << label << ": ";
    for (size_t i = 0; i < std::min(size_t(16), bytes.Size()); ++i)
    {
        std::cout << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(bytes[i]);
    }
    if (bytes.Size() > 16)
    {
        std::cout << "... (" << std::dec << bytes.Size() << " bytes total)";
    }
    std::cout << std::endl;
}

void TestG1Point()
{
    std::cout << "\n=== Testing G1Point ===" << std::endl;

    // Test infinity point
    G1Point infinity;
    std::cout << "Infinity point is infinity: " << (infinity.IsInfinity() ? "YES" : "NO") << std::endl;

    // Test generator
    G1Point generator = G1Point::Generator();
    std::cout << "Generator is infinity: " << (generator.IsInfinity() ? "YES" : "NO") << std::endl;

    // Test serialization
    ByteVector compressed = generator.ToBytes(true);
    ByteVector uncompressed = generator.ToBytes(false);
    std::cout << "Compressed size: " << compressed.Size() << " bytes" << std::endl;
    std::cout << "Uncompressed size: " << uncompressed.Size() << " bytes" << std::endl;
    PrintBytes("Compressed generator", compressed);

    // Test deserialization
    G1Point fromCompressed(compressed.AsSpan());
    std::cout << "Deserialization successful: " << (generator == fromCompressed ? "YES" : "NO") << std::endl;

    // Test arithmetic
    G1Point doubled = generator.Add(generator);
    std::cout << "G + G != G: " << (doubled != generator ? "YES" : "NO") << std::endl;

    // Test scalar multiplication
    ByteVector scalar = ByteVector(32);
    scalar[31] = 2;
    G1Point multiplied = generator.Multiply(scalar.AsSpan());
    std::cout << "2*G == G+G: " << (multiplied == doubled ? "YES" : "NO") << std::endl;
}

void TestG2Point()
{
    std::cout << "\n=== Testing G2Point ===" << std::endl;

    // Test generator
    G2Point generator = G2Point::Generator();
    std::cout << "Generator is infinity: " << (generator.IsInfinity() ? "YES" : "NO") << std::endl;

    // Test serialization
    ByteVector compressed = generator.ToBytes(true);
    ByteVector uncompressed = generator.ToBytes(false);
    std::cout << "Compressed size: " << compressed.Size() << " bytes" << std::endl;
    std::cout << "Uncompressed size: " << uncompressed.Size() << " bytes" << std::endl;
    PrintBytes("Compressed generator", compressed);

    // Test deserialization
    G2Point fromCompressed(compressed.AsSpan());
    std::cout << "Deserialization successful: " << (generator == fromCompressed ? "YES" : "NO") << std::endl;
}

void TestPairing()
{
    std::cout << "\n=== Testing Pairing ===" << std::endl;

    G1Point g1 = G1Point::Generator();
    G2Point g2 = G2Point::Generator();

    // Test basic pairing
    GTPoint e = Pairing(g1, g2);
    std::cout << "e(G1, G2) is identity: " << (e.IsIdentity() ? "YES" : "NO") << std::endl;

    // Test pairing with infinity
    G1Point inf1;
    GTPoint e_inf = Pairing(inf1, g2);
    std::cout << "e(O, G2) is identity: " << (e_inf.IsIdentity() ? "YES" : "NO") << std::endl;

    // Test GT serialization
    ByteVector gtBytes = e.ToBytes();
    std::cout << "GT size: " << gtBytes.Size() << " bytes" << std::endl;
}

void TestBLSSignature()
{
    std::cout << "\n=== Testing BLS Signatures ===" << std::endl;

    // Generate key pair
    ByteVector privateKey = Crypto::GenerateRandomBytes(32);
    G2Point publicKey = GeneratePublicKey(privateKey.AsSpan());
    PrintBytes("Private key", privateKey);

    // Sign a message
    ByteVector message = ByteVector::Parse("48656c6c6f204e656f");  // "Hello Neo"
    G1Point signature = Sign(privateKey.AsSpan(), message.AsSpan());

    // Verify signature
    bool valid = VerifySignature(publicKey, message.AsSpan(), signature);
    std::cout << "Signature valid: " << (valid ? "YES" : "NO") << std::endl;

    // Test with wrong message
    ByteVector wrongMessage = ByteVector::Parse("48656c6c6f20576f726c64");  // "Hello World"
    bool invalid = VerifySignature(publicKey, wrongMessage.AsSpan(), signature);
    std::cout << "Wrong message rejected: " << (!invalid ? "YES" : "NO") << std::endl;
}

void TestAggregateSignatures()
{
    std::cout << "\n=== Testing Aggregate Signatures ===" << std::endl;

    // Generate 3 key pairs
    std::vector<ByteVector> privateKeys;
    std::vector<G2Point> publicKeys;
    std::vector<ByteVector> messages;
    std::vector<G1Point> signatures;

    for (int i = 0; i < 3; i++)
    {
        privateKeys.push_back(Crypto::GenerateRandomBytes(32));
        publicKeys.push_back(GeneratePublicKey(privateKeys[i].AsSpan()));

        ByteVector msg(8);
        msg[0] = 'M';
        msg[1] = 's';
        msg[2] = 'g';
        msg[3] = ' ';
        msg[4] = '0' + i;
        messages.push_back(msg);

        signatures.push_back(Sign(privateKeys[i].AsSpan(), messages[i].AsSpan()));
    }

    // Aggregate signatures
    G1Point aggregateSig = AggregateSignatures(signatures);

    // Verify aggregate signature
    std::vector<neo::io::ByteSpan> messageSpans;
    for (const auto& msg : messages)
    {
        messageSpans.push_back(msg.AsSpan());
    }

    bool valid = VerifyAggregateSignature(publicKeys, messageSpans, aggregateSig);
    std::cout << "Aggregate signature valid: " << (valid ? "YES" : "NO") << std::endl;
}

void TestHashToG1()
{
    std::cout << "\n=== Testing Hash to G1 ===" << std::endl;

    ByteVector message1 = ByteVector::Parse("746573745f6d657373616765");  // "test_message"
    G1Point point1 = HashToG1(message1.AsSpan());
    std::cout << "Hash to G1 produces valid point: " << (!point1.IsInfinity() ? "YES" : "NO") << std::endl;

    ByteVector message2 = ByteVector::Parse("6f746865725f6d657373616765");  // "other_message"
    G1Point point2 = HashToG1(message2.AsSpan());
    std::cout << "Different messages produce different points: " << (point1 != point2 ? "YES" : "NO") << std::endl;
}

void TestNeoCompatibility()
{
    std::cout << "\n=== Testing Neo Protocol Compatibility ===" << std::endl;

    G1Point g1 = G1Point::Generator();
    G2Point g2 = G2Point::Generator();
    GTPoint gt = Pairing(g1, g2);

    std::cout << "G1 compressed size: " << g1.ToBytes(true).Size() << " (expected: 48)" << std::endl;
    std::cout << "G1 uncompressed size: " << g1.ToBytes(false).Size() << " (expected: 96)" << std::endl;
    std::cout << "G2 compressed size: " << g2.ToBytes(true).Size() << " (expected: 96)" << std::endl;
    std::cout << "G2 uncompressed size: " << g2.ToBytes(false).Size() << " (expected: 192)" << std::endl;
    std::cout << "GT size: " << gt.ToBytes().Size() << " (expected: 576)" << std::endl;

    // Test infinity encoding
    G1Point inf1;
    ByteVector infBytes = inf1.ToBytes(true);
    std::cout << "G1 infinity encoding: 0x" << std::hex << static_cast<int>(infBytes[0]) << " (expected: 0xC0)"
              << std::endl;
}

int main()
{
    std::cout << "BLS12-381 Functionality Test" << std::endl;
    std::cout << "===========================" << std::endl;

    try
    {
        TestG1Point();
        TestG2Point();
        TestPairing();
        TestBLSSignature();
        TestAggregateSignatures();
        TestHashToG1();
        TestNeoCompatibility();

        std::cout << "\n✅ All tests completed successfully!" << std::endl;
    }
    catch (const std::exception& e)
    {
        std::cout << "\n❌ Test failed with error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
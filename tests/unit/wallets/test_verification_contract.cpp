#include <gtest/gtest.h>
#include <neo/cryptography/ecc/ecpoint.h>
#include <neo/io/json_reader.h>
#include <neo/io/json_writer.h>
#include <neo/smartcontract/contract.h>
#include <neo/wallets/verification_contract.h>
#include <sstream>
#include <vector>

using namespace neo::wallets;
using namespace neo::smartcontract;
using namespace neo::cryptography::ecc;
using namespace neo::io;

class UT_VerificationContract : public testing::Test
{
  protected:
    void SetUp() override
    {
        // Create a public key for testing
        publicKey1 = ECPoint::FromHex("03b209fd4f53a7170ea4444e0cb0a6bb6a53c2bd016926989cf85f9b0fba17a70c");
        publicKey2 = ECPoint::FromHex("02a7834be9b32e2981d157cb5bbd3acb42cfd11ea5c3b10224d7a44e98c5910f1b");
        publicKey3 = ECPoint::FromHex("0214baf0ceea3a66f17e7e1e839ea25fd8bed6cd82e6bb6e68250189065f44ff01");
    }

    ECPoint publicKey1;
    ECPoint publicKey2;
    ECPoint publicKey3;
};

TEST_F(UT_VerificationContract, TestConstructorWithPublicKey)
{
    // Create a verification contract with a single public key
    VerificationContract contract(publicKey1);

    // Verify the contract
    EXPECT_TRUE(contract.IsSignatureContract());
    EXPECT_FALSE(contract.IsMultiSigContract());
    EXPECT_EQ(1, contract.GetPublicKeys().size());
    EXPECT_EQ(publicKey1, contract.GetPublicKeys()[0]);
    EXPECT_EQ(1, contract.GetM());

    // Verify the contract script
    auto script = contract.GetContract().GetScript();
    EXPECT_EQ(35, script.Size());
    EXPECT_EQ(0x21, script[0]);   // PUSHDATA1 for 33-byte public key
    EXPECT_EQ(0xac, script[34]);  // CHECKSIG

    // Verify the parameter list
    auto parameterList = contract.GetContract().GetParameterList();
    EXPECT_EQ(1, parameterList.size());
    EXPECT_EQ(ContractParameterType::Signature, parameterList[0]);
}

TEST_F(UT_VerificationContract, TestConstructorWithMultiSig)
{
    // Create a verification contract with multiple public keys
    std::vector<ECPoint> publicKeys = {publicKey1, publicKey2, publicKey3};
    VerificationContract contract(publicKeys, 2);

    // Verify the contract
    EXPECT_FALSE(contract.IsSignatureContract());
    EXPECT_TRUE(contract.IsMultiSigContract());
    EXPECT_EQ(3, contract.GetPublicKeys().size());
    EXPECT_EQ(publicKey1, contract.GetPublicKeys()[0]);
    EXPECT_EQ(publicKey2, contract.GetPublicKeys()[1]);
    EXPECT_EQ(publicKey3, contract.GetPublicKeys()[2]);
    EXPECT_EQ(2, contract.GetM());

    // Verify the contract script
    auto script = contract.GetContract().GetScript();
    EXPECT_EQ(108, script.Size());
    EXPECT_EQ(0x52, script[0]);    // PUSH2
    EXPECT_EQ(0x21, script[1]);    // PUSHDATA1 for 33-byte public key
    EXPECT_EQ(0x21, script[35]);   // PUSHDATA1 for 33-byte public key
    EXPECT_EQ(0x21, script[69]);   // PUSHDATA1 for 33-byte public key
    EXPECT_EQ(0x53, script[103]);  // PUSH3
    EXPECT_EQ(0xae, script[104]);  // CHECKMULTISIG

    // Verify the parameter list
    auto parameterList = contract.GetContract().GetParameterList();
    EXPECT_EQ(2, parameterList.size());
    EXPECT_EQ(ContractParameterType::Signature, parameterList[0]);
    EXPECT_EQ(ContractParameterType::Signature, parameterList[1]);
}

TEST_F(UT_VerificationContract, TestConstructorWithContract)
{
    // Create a contract
    Contract contract;

    // Create a signature contract script
    neo::vm::ScriptBuilder sb;
    sb.EmitPushData(publicKey1.ToBytes(true));
    sb.Emit(neo::vm::OpCode::CHECKSIG);
    contract.SetScript(sb.ToArray());
    contract.SetParameterList({ContractParameterType::Signature});

    // Create a verification contract from the contract
    VerificationContract verificationContract(contract);

    // Verify the verification contract
    EXPECT_TRUE(verificationContract.IsSignatureContract());
    EXPECT_FALSE(verificationContract.IsMultiSigContract());
    EXPECT_EQ(1, verificationContract.GetPublicKeys().size());
    EXPECT_EQ(publicKey1, verificationContract.GetPublicKeys()[0]);
    EXPECT_EQ(0, verificationContract.GetM());  // M is not set when constructing from a contract

    // Verify the contract script
    auto script = verificationContract.GetContract().GetScript();
    EXPECT_EQ(35, script.Size());
    EXPECT_EQ(0x21, script[0]);   // PUSHDATA1 for 33-byte public key
    EXPECT_EQ(0xac, script[34]);  // CHECKSIG

    // Verify the parameter list
    auto parameterList = verificationContract.GetContract().GetParameterList();
    EXPECT_EQ(1, parameterList.size());
    EXPECT_EQ(ContractParameterType::Signature, parameterList[0]);
}

TEST_F(UT_VerificationContract, TestGettersAndSetters)
{
    VerificationContract contract;

    // Test contract getter and setter
    Contract newContract;
    newContract.SetScript(ByteVector::FromHexString("21" + publicKey1.ToString() + "ac"));
    newContract.SetParameterList({ContractParameterType::Signature});
    contract.SetContract(newContract);
    EXPECT_EQ(newContract.GetScript(), contract.GetContract().GetScript());
    EXPECT_EQ(newContract.GetParameterList(), contract.GetContract().GetParameterList());

    // Test public keys getter and setter
    std::vector<ECPoint> publicKeys = {publicKey1, publicKey2};
    contract.SetPublicKeys(publicKeys);
    EXPECT_EQ(publicKeys, contract.GetPublicKeys());

    // Test parameter names getter and setter
    std::vector<std::string> parameterNames = {"signature1", "signature2"};
    contract.SetParameterNames(parameterNames);
    EXPECT_EQ(parameterNames, contract.GetParameterNames());

    // Test M getter and setter
    contract.SetM(2);
    EXPECT_EQ(2, contract.GetM());
}

TEST_F(UT_VerificationContract, TestJsonSerialization)
{
    // Create a verification contract
    std::vector<ECPoint> publicKeys = {publicKey1, publicKey2};
    VerificationContract contract(publicKeys, 1);

    // Set parameter names
    std::vector<std::string> parameterNames = {"signature1"};
    contract.SetParameterNames(parameterNames);

    // Serialize to JSON
    std::stringstream stream;
    JsonWriter writer(stream);
    contract.SerializeJson(writer);

    // Deserialize from JSON
    JsonReader reader(stream.str());
    VerificationContract deserializedContract;
    deserializedContract.DeserializeJson(reader);

    // Verify deserialized values
    EXPECT_EQ(contract.GetContract().GetScript(), deserializedContract.GetContract().GetScript());
    EXPECT_EQ(contract.GetContract().GetParameterList(), deserializedContract.GetContract().GetParameterList());
    EXPECT_EQ(contract.GetPublicKeys(), deserializedContract.GetPublicKeys());
    EXPECT_EQ(contract.GetParameterNames(), deserializedContract.GetParameterNames());
    EXPECT_EQ(contract.GetM(), deserializedContract.GetM());
}

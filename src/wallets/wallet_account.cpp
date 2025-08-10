#include <neo/cryptography/ecc/secp256r1.h>
#include <neo/cryptography/hash.h>
#include <neo/io/json.h>
#include <neo/wallets/wallet_account.h>

namespace neo::wallets
{
WalletAccount::WalletAccount() : locked_(false) {}

WalletAccount::WalletAccount(const cryptography::ecc::KeyPair& keyPair)
    : publicKey_(keyPair.PublicKey()), privateKey_(keyPair.PrivateKey()), locked_(false)
{
    // Calculate script hash
    scriptHash_ = cryptography::Hash::Hash160(publicKey_.ToArray().AsSpan());

    // TODO: Create contract when smartcontract dependency is resolved
    // contract_ = smartcontract::Contract::CreateSignatureContract(publicKey_);
}

WalletAccount::WalletAccount(const io::UInt160& scriptHash) : scriptHash_(scriptHash), locked_(false) {}

const io::UInt160& WalletAccount::GetScriptHash() const { return scriptHash_; }

void WalletAccount::SetScriptHash(const io::UInt160& scriptHash) { scriptHash_ = scriptHash; }

const cryptography::ecc::ECPoint& WalletAccount::GetPublicKey() const { return publicKey_; }

void WalletAccount::SetPublicKey(const cryptography::ecc::ECPoint& publicKey) { publicKey_ = publicKey; }

const std::vector<uint8_t>& WalletAccount::GetPrivateKey() const { return privateKey_; }

void WalletAccount::SetPrivateKey(const std::vector<uint8_t>& privateKey) { privateKey_ = privateKey; }

const smartcontract::Contract& WalletAccount::GetContract() const { return contract_; }

void WalletAccount::SetContract(const smartcontract::Contract& contract) { contract_ = contract; }

const std::string& WalletAccount::GetLabel() const { return label_; }

void WalletAccount::SetLabel(const std::string& label) { label_ = label; }

bool WalletAccount::IsLocked() const { return locked_; }

void WalletAccount::SetLocked(bool locked) { locked_ = locked; }

std::string WalletAccount::GetWIF() const
{
    if (privateKey_.empty()) return "";

    return cryptography::ecc::Secp256r1::ToWIF(privateKey_);
}

std::string WalletAccount::GetAddress() const { return scriptHash_.ToAddress(); }

bool WalletAccount::HasPrivateKey() const { return !privateKey_.empty(); }

nlohmann::json WalletAccount::ToJson() const
{
    nlohmann::json json;
    json["address"] = GetAddress();
    json["script_hash"] = scriptHash_.ToString();

    if (!publicKey_.IsInfinity()) json["public_key"] = publicKey_.ToString();

    if (!privateKey_.empty())
        json["private_key"] = io::ByteVector(io::ByteSpan(privateKey_.data(), privateKey_.size())).ToHexString();

    // TODO: Implement contract serialization when smartcontract dependency is resolved
    // if (!contract_.GetScript().IsEmpty())
    // {
    //     nlohmann::json contractJson;
    //     contractJson["script"] = contract_.GetScript().ToHexString();

    //     nlohmann::json parameterListJson = nlohmann::json::array();
    //     for (const auto& parameter : contract_.GetParameterList())
    //     {
    //         parameterListJson.push_back(static_cast<uint8_t>(parameter));
    //     }
    //     contractJson["parameters"] = parameterListJson;

    //     json["contract"] = contractJson;
    // }

    if (!label_.empty()) json["label"] = label_;

    json["locked"] = locked_;

    return json;
}

void WalletAccount::FromJson(const nlohmann::json& json)
{
    if (json.contains("script_hash"))
        scriptHash_ = io::UInt160::Parse(json["script_hash"].get<std::string>());
    else if (json.contains("address"))
        scriptHash_ = io::UInt160::FromAddress(json["address"].get<std::string>());

    if (json.contains("public_key"))
        publicKey_ = cryptography::ecc::ECPoint::Parse(json["public_key"].get<std::string>());

    if (json.contains("private_key"))
    {
        auto privateKeyHex = json["private_key"].get<std::string>();
        auto privateKeyBytes = io::ByteVector::Parse(privateKeyHex);
        privateKey_.resize(privateKeyBytes.Size());
        std::memcpy(privateKey_.data(), privateKeyBytes.Data(), privateKeyBytes.Size());
    }

    if (json.contains("contract"))
    {
        auto contractJson = json["contract"];

        io::ByteVector script;
        if (contractJson.contains("script")) script = io::ByteVector::Parse(contractJson["script"].get<std::string>());

        // TODO: Implement contract deserialization when smartcontract dependency is resolved
        // std::vector<smartcontract::ContractParameterType> parameterList;
        // if (contractJson.contains("parameters") && contractJson["parameters"].is_array())
        // {
        //     for (const auto& parameter : contractJson["parameters"])
        //     {
        //         parameterList.push_back(static_cast<smartcontract::ContractParameterType>(parameter.get<uint8_t>()));
        //     }
        // }
        // contract_ = smartcontract::Contract(script, parameterList);
    }

    if (json.contains("label")) label_ = json["label"].get<std::string>();

    if (json.contains("locked")) locked_ = json["locked"].get<bool>();
}
}  // namespace neo::wallets

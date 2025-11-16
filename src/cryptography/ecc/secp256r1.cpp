#include <neo/cryptography/base58.h>
#include <neo/cryptography/crypto.h>
#include <neo/cryptography/ecc/ecpoint.h>
#include <neo/cryptography/ecc/keypair.h>
#include <neo/cryptography/ecc/secp256r1.h>
#include <neo/cryptography/hash.h>
#include <neo/cryptography/scrypt.h>
#include <neo/io/byte_vector.h>
#include <neo/core/protocol_constants.h>
#include <neo/vm/opcode.h>
#include <neo/vm/script_builder.h>

#include <array>
#include <cstring>
#include <stdexcept>
#include <vector>
#include <sstream>
#include <iomanip>

#include <openssl/bn.h>
#include <openssl/ec.h>
#include <openssl/ecdsa.h>
#include <openssl/evp.h>
#include <openssl/obj_mac.h>
#include <openssl/rand.h>

namespace neo::cryptography::ecc
{
namespace
{
EC_GROUP* curve_group()
{
    static EC_GROUP* group = EC_GROUP_new_by_curve_name(NID_X9_62_prime256v1);
    if (!group)
    {
        throw std::runtime_error("Failed to create secp256r1 group");
    }
    return group;
}

void check(bool condition, const char* message)
{
    if (!condition)
    {
        throw std::runtime_error(message);
    }
}

void aes256_ecb_encrypt(const uint8_t* key, const uint8_t* input, uint8_t* output, size_t length)
{
    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    check(ctx != nullptr, "Failed to create AES context");

    check(EVP_EncryptInit_ex(ctx, EVP_aes_256_ecb(), nullptr, key, nullptr) == 1, "AES init failed");
    EVP_CIPHER_CTX_set_padding(ctx, 0);

    int outLen = 0;
    check(EVP_EncryptUpdate(ctx, output, &outLen, input, static_cast<int>(length)) == 1, "AES update failed");

    int finalLen = 0;
    check(EVP_EncryptFinal_ex(ctx, output + outLen, &finalLen) == 1, "AES final failed");
    check(static_cast<size_t>(outLen + finalLen) == length, "AES output length mismatch");

    EVP_CIPHER_CTX_free(ctx);
}

void aes256_ecb_decrypt(const uint8_t* key, const uint8_t* input, uint8_t* output, size_t length)
{
    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    check(ctx != nullptr, "Failed to create AES context");

    check(EVP_DecryptInit_ex(ctx, EVP_aes_256_ecb(), nullptr, key, nullptr) == 1, "AES init failed");
    EVP_CIPHER_CTX_set_padding(ctx, 0);

    int outLen = 0;
    check(EVP_DecryptUpdate(ctx, output, &outLen, input, static_cast<int>(length)) == 1, "AES update failed");

    int finalLen = 0;
    check(EVP_DecryptFinal_ex(ctx, output + outLen, &finalLen) == 1, "AES final failed");
    check(static_cast<size_t>(outLen + finalLen) == length, "AES output length mismatch");

    EVP_CIPHER_CTX_free(ctx);
}

io::ByteVector CreateSignatureVerificationScript(const io::ByteVector& publicKey)
{
    if (publicKey.IsEmpty())
    {
        throw std::invalid_argument("Public key cannot be empty");
    }

    vm::ScriptBuilder sb;
    sb.EmitPush(publicKey.AsSpan());
    sb.EmitSysCall("System.Crypto.CheckSig");
    return sb.ToArray();
}
}  // namespace

io::ByteVector Secp256r1::GeneratePrivateKey()
{
    io::ByteVector privateKey(PRIVATE_KEY_SIZE);
    check(RAND_bytes(privateKey.Data(), static_cast<int>(PRIVATE_KEY_SIZE)) == 1, "Failed to generate private key");

    BIGNUM* bn = BN_bin2bn(privateKey.Data(), static_cast<int>(PRIVATE_KEY_SIZE), nullptr);
    check(bn != nullptr, "Failed to create BIGNUM");

    const BIGNUM* order = EC_GROUP_get0_order(curve_group());
    check(order != nullptr, "Failed to get curve order");

    if (BN_is_zero(bn) || BN_cmp(bn, order) >= 0)
    {
        BN_free(bn);
        return GeneratePrivateKey();
    }

    BN_free(bn);
    return privateKey;
}

io::ByteVector Secp256r1::ComputePublicKey(const io::ByteVector& privateKey)
{
    if (!IsValidPrivateKey(privateKey)) throw std::invalid_argument("Invalid private key");

    EC_KEY* key = EC_KEY_new();
    check(key != nullptr, "Failed to allocate EC_KEY");

    check(EC_KEY_set_group(key, curve_group()) == 1, "Failed to set EC group");

    BIGNUM* priv = BN_bin2bn(privateKey.Data(), static_cast<int>(privateKey.Size()), nullptr);
    check(priv != nullptr, "Failed to create BIGNUM from private key");

    check(EC_KEY_set_private_key(key, priv) == 1, "Failed to set private key");

    EC_POINT* point = EC_POINT_new(curve_group());
    check(point != nullptr, "Failed to create EC_POINT");

    check(EC_POINT_mul(curve_group(), point, priv, nullptr, nullptr, nullptr) == 1, "Failed to compute public key");

    io::ByteVector publicKey(PUBLIC_KEY_SIZE);
    size_t written =
        EC_POINT_point2oct(curve_group(), point, POINT_CONVERSION_COMPRESSED, publicKey.Data(), PUBLIC_KEY_SIZE, nullptr);
    check(written == PUBLIC_KEY_SIZE, "Unexpected public key length");

    EC_POINT_free(point);
    BN_free(priv);
    EC_KEY_free(key);

    return publicKey;
}

io::ByteVector Secp256r1::Sign(const io::ByteVector& data, const io::ByteVector& privateKey)
{
    if (!IsValidPrivateKey(privateKey)) throw std::invalid_argument("Invalid private key");

    auto hash = Hash::Sha256(data.AsSpan());

    EC_KEY* key = EC_KEY_new();
    check(key != nullptr, "Failed to allocate EC_KEY");
    check(EC_KEY_set_group(key, curve_group()) == 1, "Failed to set EC group");

    BIGNUM* priv = BN_bin2bn(privateKey.Data(), static_cast<int>(privateKey.Size()), nullptr);
    check(priv != nullptr, "Failed to create BIGNUM from private key");
    check(EC_KEY_set_private_key(key, priv) == 1, "Failed to set private key");

    ECDSA_SIG* sig = ECDSA_do_sign(hash.Data(), static_cast<int>(hash.AsSpan().Size()), key);
    check(sig != nullptr, "Failed to produce signature");

    const BIGNUM* r = nullptr;
    const BIGNUM* s = nullptr;
    ECDSA_SIG_get0(sig, &r, &s);
    check(r != nullptr && s != nullptr, "Failed to extract r/s");

    io::ByteVector signature(64);
    check(BN_bn2binpad(r, signature.Data(), 32) == 32, "Failed to encode r");
    check(BN_bn2binpad(s, signature.Data() + 32, 32) == 32, "Failed to encode s");

    ECDSA_SIG_free(sig);
    BN_free(priv);
    EC_KEY_free(key);

    return signature;
}

bool Secp256r1::Verify(const io::ByteVector& data, const io::ByteVector& signature, const io::ByteVector& publicKey)
{
    if (!IsValidPublicKey(publicKey)) return false;

    auto hash = Hash::Sha256(data.AsSpan());

    EC_KEY* key = EC_KEY_new();
    if (!key) return false;
    if (EC_KEY_set_group(key, curve_group()) != 1)
    {
        EC_KEY_free(key);
        return false;
    }

    EC_POINT* point = EC_POINT_new(curve_group());
    if (!point)
    {
        EC_KEY_free(key);
        return false;
    }

    if (EC_POINT_oct2point(curve_group(), point, publicKey.Data(), publicKey.Size(), nullptr) != 1)
    {
        EC_POINT_free(point);
        EC_KEY_free(key);
        return false;
    }

    if (EC_KEY_set_public_key(key, point) != 1)
    {
        EC_POINT_free(point);
        EC_KEY_free(key);
        return false;
    }

    ECDSA_SIG* sig = nullptr;

    if (signature.Size() == 64)
    {
        // Raw (r||s) format
        BIGNUM* r = BN_bin2bn(signature.Data(), 32, nullptr);
        BIGNUM* s = BN_bin2bn(signature.Data() + 32, 32, nullptr);
        if (!r || !s)
        {
            BN_free(r);
            BN_free(s);
            EC_POINT_free(point);
            EC_KEY_free(key);
            return false;
        }
        sig = ECDSA_SIG_new();
        if (!sig || ECDSA_SIG_set0(sig, r, s) != 1)
        {
            BN_free(r);
            BN_free(s);
            ECDSA_SIG_free(sig);
            EC_POINT_free(point);
            EC_KEY_free(key);
            return false;
        }
    }
    else
    {
        // Fall back to DER decoding
        const unsigned char* sigPtr = signature.Data();
        sig = d2i_ECDSA_SIG(nullptr, &sigPtr, static_cast<long>(signature.Size()));
        if (!sig)
        {
            EC_POINT_free(point);
            EC_KEY_free(key);
            return false;
        }
    }

    int result = ECDSA_do_verify(hash.Data(), static_cast<int>(hash.AsSpan().Size()), sig, key);

    ECDSA_SIG_free(sig);
    EC_POINT_free(point);
    EC_KEY_free(key);

    return result == 1;
}

bool Secp256r1::IsValidPrivateKey(const io::ByteVector& privateKey)
{
    if (privateKey.Size() != PRIVATE_KEY_SIZE) return false;

    BIGNUM* value = BN_bin2bn(privateKey.Data(), static_cast<int>(privateKey.Size()), nullptr);
    if (!value) return false;

    const BIGNUM* order = EC_GROUP_get0_order(curve_group());
    bool valid = BN_is_zero(value) == 0 && BN_cmp(value, order) < 0;

    BN_free(value);
    return valid;
}

bool Secp256r1::IsValidPublicKey(const io::ByteVector& publicKey)
{
    if (publicKey.Size() != PUBLIC_KEY_SIZE) return false;
    return publicKey[0] == 0x02 || publicKey[0] == 0x03;
}

bool Secp256r1::IsZero(const io::ByteVector& value)
{
    for (size_t i = 0; i < value.Size(); ++i)
    {
        if (value[i] != 0) return false;
    }
    return true;
}

bool Secp256r1::IsOnCurve(const io::ByteVector& publicKey)
{
    if (!IsValidPublicKey(publicKey)) return false;

    EC_POINT* point = EC_POINT_new(curve_group());
    if (!point) return false;

    bool ok = EC_POINT_oct2point(curve_group(), point, publicKey.Data(), publicKey.Size(), nullptr) == 1 &&
              EC_POINT_is_on_curve(curve_group(), point, nullptr) == 1;

    EC_POINT_free(point);
    return ok;
}

KeyPair Secp256r1::GenerateKeyPair()
{
    auto priv = GeneratePrivateKey();
    return KeyPair(priv);
}

KeyPair Secp256r1::FromPrivateKey(const io::ByteVector& privateKey)
{
    if (!IsValidPrivateKey(privateKey)) throw std::invalid_argument("Invalid private key");
    return KeyPair(privateKey);
}

KeyPair Secp256r1::FromWIF(const std::string& wif)
{
    auto decoded = cryptography::Base58::DecodeToByteVector(wif);
    if (decoded.Size() < 34) throw std::invalid_argument("Invalid WIF");

    io::ByteVector priv(decoded.Data() + 1, 32);
    return KeyPair(priv);
}

std::string Secp256r1::ToWIF(const io::ByteVector& privateKey, bool)
{
    io::ByteVector payload;
    payload.Push(0x80);
    payload.Append(privateKey.AsSpan());
    payload.Push(0x01);
    return Base58::EncodeCheck(payload.AsSpan());
}

std::string Secp256r1::ToNEP2(const io::ByteVector& privateKey, const std::string& passphrase, int scryptN, int scryptR,
                              int scryptP)
{
    auto publicKey = ComputePublicKey(privateKey);
    auto script = CreateSignatureVerificationScript(publicKey);
    auto scriptHash = Hash::Hash160(script.AsSpan());
    auto address = scriptHash.ToAddress(static_cast<uint8_t>(core::ProtocolConstants::AddressVersion));
    std::vector<uint8_t> addressBytes(address.begin(), address.end());
    auto hash1 = Hash::Sha256(io::ByteSpan(addressBytes.data(), addressBytes.size()));
    auto hash2 = Hash::Sha256(hash1.AsSpan());

    std::vector<uint8_t> salt(hash2.Data(), hash2.Data() + 4);
    std::vector<uint8_t> pass(passphrase.begin(), passphrase.end());
    auto derived = cryptography::Scrypt::DeriveKey(pass, salt, static_cast<uint32_t>(scryptN), static_cast<uint32_t>(scryptR),
                                                   static_cast<uint32_t>(scryptP), 64);

    std::array<uint8_t, 32> block{};
    for (size_t i = 0; i < 32; ++i)
    {
        block[i] = privateKey[i] ^ derived[i];
    }

    std::array<uint8_t, 32> encrypted{};
    aes256_ecb_encrypt(derived.data() + 32, block.data(), encrypted.data(), encrypted.size());

    io::ByteVector payload;
    payload.Push(0x01);
    payload.Push(0x42);
    payload.Push(0xE0);
    payload.Append(io::ByteSpan(salt.data(), salt.size()));
    payload.Append(io::ByteSpan(encrypted.data(), encrypted.size()));

    return Base58::EncodeCheck(payload.AsSpan());
}

io::ByteVector Secp256r1::FromNEP2(const std::string& nep2, const std::string& passphrase)
{
    return FromNEP2(nep2, passphrase, 16384, 8, 8);
}

io::ByteVector Secp256r1::FromNEP2(const std::string& nep2, const std::string& passphrase, int scryptN, int scryptR,
                                   int scryptP)
{
    auto decoded = Base58::DecodeCheck(nep2);
    check(decoded.size() == 39, "Invalid NEP2 payload length");

    check(decoded[0] == 0x01 && decoded[1] == 0x42 && decoded[2] == 0xE0, "Invalid NEP2 flags");

    std::vector<uint8_t> salt(decoded.begin() + 3, decoded.begin() + 7);

    std::vector<uint8_t> pass(passphrase.begin(), passphrase.end());
    auto derived = cryptography::Scrypt::DeriveKey(pass, salt, static_cast<uint32_t>(scryptN), static_cast<uint32_t>(scryptR),
                                                   static_cast<uint32_t>(scryptP), 64);

    std::array<uint8_t, 32> decrypted{};
    aes256_ecb_decrypt(derived.data() + 32, decoded.data() + 7, decrypted.data(), decrypted.size());

    io::ByteVector privateKey(32);
    for (size_t i = 0; i < 32; ++i)
    {
        privateKey[i] = decrypted[i] ^ derived[i];
    }

    auto publicKey = ComputePublicKey(privateKey);
    auto script = CreateSignatureVerificationScript(publicKey);
    auto scriptHash = Hash::Hash160(script.AsSpan());

    auto address = scriptHash.ToAddress(static_cast<uint8_t>(core::ProtocolConstants::AddressVersion));
    std::vector<uint8_t> addressBytes(address.begin(), address.end());
    auto hash1 = Hash::Sha256(io::ByteSpan(addressBytes.data(), addressBytes.size()));
    auto hash2 = Hash::Sha256(hash1.AsSpan());

    if (std::memcmp(salt.data(), hash2.Data(), 4) != 0)
    {
        auto hashBytes = hash2.ToArray();
        std::ostringstream oss;
        oss << "Invalid passphrase (expected salt=";
        oss << std::hex << std::setfill('0');
        for (auto byte : salt) oss << std::setw(2) << static_cast<int>(byte);
        oss << ", computed=";
        for (int i = 0; i < 4; ++i) oss << std::setw(2) << static_cast<int>(hashBytes[i]);
        oss << ")";
        throw std::runtime_error(oss.str());
    }
    return privateKey;
}

io::ByteVector Secp256r1::DecryptPrivateKey(const std::string& wif) { return FromWIF(wif).GetPrivateKey(); }

io::ByteVector Secp256r1::DecryptPrivateKey(const std::string& nep2, const std::string& passphrase)
{
    return FromNEP2(nep2, passphrase);
}

io::ByteVector Secp256r1::DecryptPrivateKey(const std::string& nep2, const std::string& passphrase, int scryptN,
                                            int scryptR, int scryptP)
{
    return FromNEP2(nep2, passphrase, scryptN, scryptR, scryptP);
}
}  // namespace neo::cryptography::ecc

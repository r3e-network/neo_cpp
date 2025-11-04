/**
 * @file crypto.cpp
 * @brief Cryptographic operations
 * @author Neo C++ Team
 * @date 2025
 * @copyright MIT License
 */

#include <neo/cryptography/crypto.h>
#include <neo/cryptography/ecc/ecpoint.h>
#include <neo/cryptography/ecc/ecc_curve.h>
#include <neo/cryptography/hash.h>
#include <openssl/bio.h>
#include <openssl/buffer.h>
#include <openssl/ec.h>
#include <openssl/ecdsa.h>
#include <openssl/evp.h>
#include <openssl/hmac.h>
#include <openssl/rand.h>
#include <openssl/ripemd.h>
#include <openssl/sha.h>

#include <cstring>
#include <random>
#include <stdexcept>

// Suppress OpenSSL deprecation warnings for the entire file
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4996)  // OpenSSL deprecation warnings
#endif

namespace neo::cryptography
{
io::ByteVector Crypto::GenerateRandomBytes(size_t length)
{
    io::ByteVector result(length);
    if (RAND_bytes(result.Data(), static_cast<int>(length)) != 1)
    {
        throw std::runtime_error("Failed to generate random bytes");
    }
    return result;
}

io::UInt256 Crypto::Hash256(const uint8_t* data, size_t length)
{
    EVP_MD_CTX* ctx = EVP_MD_CTX_new();
    if (!ctx) throw std::runtime_error("Failed to create hash context");

    if (EVP_DigestInit_ex(ctx, EVP_sha256(), nullptr) != 1)
    {
        EVP_MD_CTX_free(ctx);
        throw std::runtime_error("Failed to initialize SHA256");
    }

    if (EVP_DigestUpdate(ctx, data, length) != 1)
    {
        EVP_MD_CTX_free(ctx);
        throw std::runtime_error("Failed to update SHA256");
    }

    unsigned char hash[SHA256_DIGEST_LENGTH];
    unsigned int hashLen;
    if (EVP_DigestFinal_ex(ctx, hash, &hashLen) != 1)
    {
        EVP_MD_CTX_free(ctx);
        throw std::runtime_error("Failed to finalize SHA256");
    }

    EVP_MD_CTX_free(ctx);
    return io::UInt256(io::ByteSpan(hash, SHA256_DIGEST_LENGTH));
}

io::UInt256 Crypto::Hash256(const io::ByteSpan& data) { return Hash256(data.Data(), data.Size()); }

io::UInt256 Crypto::Hash256(const io::ByteVector& data) { return Hash256(data.Data(), data.Size()); }

io::UInt160 Crypto::Hash160(const io::ByteSpan& data)
{
    // First compute SHA256
    io::UInt256 sha256Hash = Hash256(data);

    // Then compute RIPEMD160 of the SHA256 hash
    unsigned char ripemdHash[RIPEMD160_DIGEST_LENGTH];
    RIPEMD160(sha256Hash.Data(), SHA256_DIGEST_LENGTH, ripemdHash);

    return io::UInt160(io::ByteSpan(ripemdHash, RIPEMD160_DIGEST_LENGTH));
}

io::ByteVector Crypto::AesEncrypt(const io::ByteSpan& data, const io::ByteSpan& key, const io::ByteSpan& iv)
{
    if (key.Size() != 32) throw std::invalid_argument("Key must be 32 bytes");

    if (iv.Size() != 16) throw std::invalid_argument("IV must be 16 bytes");

    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    if (!ctx) throw std::runtime_error("Failed to create cipher context");

    if (EVP_EncryptInit_ex(ctx, EVP_aes_256_cbc(), nullptr, key.Data(), iv.Data()) != 1)
    {
        EVP_CIPHER_CTX_free(ctx);
        throw std::runtime_error("Failed to initialize encryption");
    }

    io::ByteVector result(data.Size() + EVP_CIPHER_block_size(EVP_aes_256_cbc()));
    int outLen1 = 0;
    if (EVP_EncryptUpdate(ctx, result.Data(), &outLen1, data.Data(), static_cast<int>(data.Size())) != 1)
    {
        EVP_CIPHER_CTX_free(ctx);
        throw std::runtime_error("Failed to encrypt data");
    }

    int outLen2 = 0;
    if (EVP_EncryptFinal_ex(ctx, result.Data() + outLen1, &outLen2) != 1)
    {
        EVP_CIPHER_CTX_free(ctx);
        throw std::runtime_error("Failed to finalize encryption");
    }

    EVP_CIPHER_CTX_free(ctx);

    result.Resize(outLen1 + outLen2);
    return result;
}

io::ByteVector Crypto::AesDecrypt(const io::ByteSpan& data, const io::ByteSpan& key, const io::ByteSpan& iv)
{
    if (key.Size() != 32) throw std::invalid_argument("Key must be 32 bytes");

    if (iv.Size() != 16) throw std::invalid_argument("IV must be 16 bytes");

    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    if (!ctx) throw std::runtime_error("Failed to create cipher context");

    if (EVP_DecryptInit_ex(ctx, EVP_aes_256_cbc(), nullptr, key.Data(), iv.Data()) != 1)
    {
        EVP_CIPHER_CTX_free(ctx);
        throw std::runtime_error("Failed to initialize decryption");
    }

    io::ByteVector result(data.Size());
    int outLen1 = 0;
    if (EVP_DecryptUpdate(ctx, result.Data(), &outLen1, data.Data(), static_cast<int>(data.Size())) != 1)
    {
        EVP_CIPHER_CTX_free(ctx);
        throw std::runtime_error("Failed to decrypt data");
    }

    int outLen2 = 0;
    if (EVP_DecryptFinal_ex(ctx, result.Data() + outLen1, &outLen2) != 1)
    {
        EVP_CIPHER_CTX_free(ctx);
        throw std::runtime_error("Failed to finalize decryption");
    }

    EVP_CIPHER_CTX_free(ctx);

    result.Resize(outLen1 + outLen2);
    return result;
}

io::ByteVector Crypto::PBKDF2(const io::ByteSpan& password, const io::ByteSpan& salt, int iterations, int keyLength)
{
    io::ByteVector result(keyLength);

    if (PKCS5_PBKDF2_HMAC(reinterpret_cast<const char*>(password.Data()), static_cast<int>(password.Size()),
                          salt.Data(), static_cast<int>(salt.Size()), iterations, EVP_sha256(), keyLength,
                          result.Data()) != 1)
    {
        throw std::runtime_error("Failed to derive key");
    }

    return result;
}

io::ByteVector Crypto::HmacSha256(const io::ByteSpan& key, const io::ByteSpan& data)
{
    unsigned int length = EVP_MAX_MD_SIZE;
    io::ByteVector result(length);

    HMAC_CTX* ctx = HMAC_CTX_new();
    if (!ctx) throw std::runtime_error("Failed to create HMAC context");

    // Handle empty key case - provide empty key buffer for OpenSSL 3.0 compatibility
    static const uint8_t emptyKey = 0;
    const void* keyPtr = key.Size() > 0 ? static_cast<const void*>(key.Data()) : static_cast<const void*>(&emptyKey);
    if (HMAC_Init_ex(ctx, keyPtr, static_cast<int>(key.Size()), EVP_sha256(), nullptr) != 1)
    {
        HMAC_CTX_free(ctx);
        throw std::runtime_error("Failed to initialize HMAC");
    }

    if (HMAC_Update(ctx, data.Data(), data.Size()) != 1)
    {
        HMAC_CTX_free(ctx);
        throw std::runtime_error("Failed to update HMAC");
    }

    if (HMAC_Final(ctx, result.Data(), &length) != 1)
    {
        HMAC_CTX_free(ctx);
        throw std::runtime_error("Failed to finalize HMAC");
    }

    HMAC_CTX_free(ctx);

    result.Resize(length);
    return result;
}

std::string Crypto::Base64Encode(const io::ByteSpan& data)
{
    BIO* b64 = BIO_new(BIO_f_base64());
    BIO* bmem = BIO_new(BIO_s_mem());
    b64 = BIO_push(b64, bmem);
    BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);
    BIO_write(b64, data.Data(), static_cast<int>(data.Size()));
    BIO_flush(b64);

    BUF_MEM* bptr;
    BIO_get_mem_ptr(b64, &bptr);

    std::string result(bptr->data, bptr->length);
    BIO_free_all(b64);

    return result;
}

io::ByteVector Crypto::Base64Decode(const std::string& base64)
{
    // Validate Base64 characters first
    for (char c : base64)
    {
        if (!(std::isalnum(c) || c == '+' || c == '/' || c == '='))
        {
            throw std::runtime_error("Failed to decode Base64");
        }
    }

    BIO* b64 = BIO_new(BIO_f_base64());
    BIO* bmem = BIO_new_mem_buf(base64.c_str(), static_cast<int>(base64.length()));
    bmem = BIO_push(b64, bmem);
    BIO_set_flags(bmem, BIO_FLAGS_BASE64_NO_NL);

    io::ByteVector result(base64.length());
    int decodedSize = BIO_read(bmem, result.Data(), static_cast<int>(result.Size()));
    BIO_free_all(bmem);

    if (decodedSize < 0) throw std::runtime_error("Failed to decode Base64");

    result.Resize(decodedSize);
    return result;
}

io::ByteVector Crypto::CreateSignatureRedeemScript(const ecc::ECPoint& publicKey)
{
    // Neo signature redeem script format:
    // PUSHDATA1 0x21 <33-byte compressed public key> CHECKSIG
    io::ByteVector script;
    script.Reserve(35);  // 1 + 1 + 33 bytes for public key + 1 for CHECKSIG

    // PUSHDATA1 (0x0C) followed by length (0x21 = 33 bytes)
    script.Push(0x0C);
    script.Push(0x21);

    // Add the compressed public key (33 bytes)
    auto pubKeyBytes = publicKey.ToArray();
    script.Append(pubKeyBytes.AsSpan());

    // CHECKSIG opcode (0x41)
    script.Push(0x41);

    return script;
}

bool Crypto::VerifySignature(const io::ByteSpan& message, const io::ByteSpan& signature, const ecc::ECPoint& publicKey)
{
    return VerifySignature(message, signature, publicKey, ecc::ECCCurve::Secp256r1());
}

bool Crypto::VerifySignature(const io::ByteSpan& message, const io::ByteSpan& signature, const ecc::ECPoint& publicKey,
                             const ecc::ECCCurve& curve)
{
    const auto fieldSize = curve.GetFieldSize();
    if (signature.Size() != fieldSize * 2) return false;

    try
    {
        EC_KEY* eckey = EC_KEY_new();
        if (!eckey) return false;

        if (EC_KEY_set_group(eckey, curve.GetGroup()) != 1)
        {
            EC_KEY_free(eckey);
            return false;
        }

        auto pubKeyBytes = publicKey.ToArray();
        EC_POINT* point = EC_POINT_new(curve.GetGroup());
        if (!point)
        {
            EC_KEY_free(eckey);
            return false;
        }

        if (EC_POINT_oct2point(curve.GetGroup(), point, pubKeyBytes.Data(), pubKeyBytes.Size(), nullptr) != 1)
        {
            EC_POINT_free(point);
            EC_KEY_free(eckey);
            return false;
        }

        if (EC_KEY_set_public_key(eckey, point) != 1)
        {
            EC_POINT_free(point);
            EC_KEY_free(eckey);
            return false;
        }

        ECDSA_SIG* sig = ECDSA_SIG_new();
        if (!sig)
        {
            EC_POINT_free(point);
            EC_KEY_free(eckey);
            return false;
        }

        const size_t componentSize = fieldSize;
        BIGNUM* r = BN_bin2bn(signature.Data(), static_cast<int>(componentSize), nullptr);
        BIGNUM* s = BN_bin2bn(signature.Data() + componentSize, static_cast<int>(componentSize), nullptr);

        if (!r || !s)
        {
            if (r) BN_free(r);
            if (s) BN_free(s);
            ECDSA_SIG_free(sig);
            EC_POINT_free(point);
            EC_KEY_free(eckey);
            return false;
        }

        ECDSA_SIG_set0(sig, r, s);

        const int result = ECDSA_do_verify(message.Data(), static_cast<int>(message.Size()), sig, eckey);

        ECDSA_SIG_free(sig);
        EC_POINT_free(point);
        EC_KEY_free(eckey);

        return result == 1;
    }
    catch (const std::runtime_error&)
    {
        return false;
    }
    catch (const std::bad_alloc&)
    {
        return false;
    }
}

io::ByteVector Crypto::Sign(const io::ByteSpan& message, const io::ByteSpan& privateKey)
{
    return Sign(message, privateKey, ecc::ECCCurve::Secp256r1());
}

io::ByteVector Crypto::Sign(const io::ByteSpan& message, const io::ByteSpan& privateKey, const ecc::ECCCurve& curve)
{
    const auto fieldSize = curve.GetFieldSize();
    if (privateKey.Size() != fieldSize)
    {
        throw std::invalid_argument("Private key must match the curve field size");
    }

    try
    {
        EC_KEY* eckey = EC_KEY_new();
        if (!eckey) throw std::runtime_error("Failed to create EC_KEY");

        if (EC_KEY_set_group(eckey, curve.GetGroup()) != 1)
        {
            EC_KEY_free(eckey);
            throw std::runtime_error("Failed to associate curve with EC_KEY");
        }

        BIGNUM* privKeyBN =
            BN_bin2bn(privateKey.Data(), static_cast<int>(privateKey.Size()), nullptr);
        if (!privKeyBN)
        {
            EC_KEY_free(eckey);
            throw std::runtime_error("Failed to create BIGNUM from private key");
        }

        if (EC_KEY_set_private_key(eckey, privKeyBN) != 1)
        {
            BN_free(privKeyBN);
            EC_KEY_free(eckey);
            throw std::runtime_error("Failed to set private key");
        }

        EC_POINT* pubKey = EC_POINT_new(curve.GetGroup());
        if (!pubKey)
        {
            BN_free(privKeyBN);
            EC_KEY_free(eckey);
            throw std::runtime_error("Failed to create EC_POINT");
        }

        if (EC_POINT_mul(curve.GetGroup(), pubKey, privKeyBN, nullptr, nullptr, nullptr) != 1)
        {
            EC_POINT_free(pubKey);
            BN_free(privKeyBN);
            EC_KEY_free(eckey);
            throw std::runtime_error("Failed to compute public key");
        }

        if (EC_KEY_set_public_key(eckey, pubKey) != 1)
        {
            EC_POINT_free(pubKey);
            BN_free(privKeyBN);
            EC_KEY_free(eckey);
            throw std::runtime_error("Failed to set public key");
        }

        ECDSA_SIG* sig = ECDSA_do_sign(message.Data(), static_cast<int>(message.Size()), eckey);
        if (!sig)
        {
            EC_POINT_free(pubKey);
            BN_free(privKeyBN);
            EC_KEY_free(eckey);
            throw std::runtime_error("Failed to sign message");
        }

        const BIGNUM* r = nullptr;
        const BIGNUM* s = nullptr;
        ECDSA_SIG_get0(sig, &r, &s);

        io::ByteVector signature(fieldSize * 2);

        const int rLen = BN_num_bytes(r);
        const int sLen = BN_num_bytes(s);
        const size_t totalSize = signature.Size();
        std::memset(signature.Data(), 0, totalSize);
        const size_t rOffset = fieldSize - static_cast<size_t>(rLen);
        const size_t sOffset = fieldSize - static_cast<size_t>(sLen);
        BN_bn2bin(r, signature.Data() + rOffset);
        BN_bn2bin(s, signature.Data() + fieldSize + sOffset);

        ECDSA_SIG_free(sig);
        EC_POINT_free(pubKey);
        BN_free(privKeyBN);
        EC_KEY_free(eckey);

        return signature;
    }
    catch (const std::runtime_error&)
    {
        throw;
    }
    catch (const std::bad_alloc&)
    {
        throw std::runtime_error("Failed to sign message: out of memory");
    }
    catch (const std::exception& e)
    {
        throw std::runtime_error("Failed to sign message: " + std::string(e.what()));
    }
}

ecc::ECPoint Crypto::ComputePublicKey(const io::ByteSpan& privateKey)
{
    return ComputePublicKey(privateKey, ecc::ECCCurve::Secp256r1());
}

ecc::ECPoint Crypto::ComputePublicKey(const io::ByteSpan& privateKey, const ecc::ECCCurve& curve)
{
    const auto fieldSize = curve.GetFieldSize();
    if (privateKey.Size() != fieldSize)
    {
        throw std::invalid_argument("Private key must match the curve field size");
    }

    try
    {
        BIGNUM* privKeyBN =
            BN_bin2bn(privateKey.Data(), static_cast<int>(privateKey.Size()), nullptr);
        if (!privKeyBN) throw std::runtime_error("Failed to create BIGNUM from private key");

        EC_POINT* pubKeyPoint = EC_POINT_new(curve.GetGroup());
        if (!pubKeyPoint)
        {
            BN_free(privKeyBN);
            throw std::runtime_error("Failed to create EC_POINT");
        }

        if (EC_POINT_mul(curve.GetGroup(), pubKeyPoint, privKeyBN, nullptr, nullptr, nullptr) != 1)
        {
            EC_POINT_free(pubKeyPoint);
            BN_free(privKeyBN);
            throw std::runtime_error("Failed to compute public key");
        }

        size_t pubKeyLen =
            EC_POINT_point2oct(curve.GetGroup(), pubKeyPoint, POINT_CONVERSION_COMPRESSED, nullptr, 0, nullptr);
        if (pubKeyLen == 0)
        {
            EC_POINT_free(pubKeyPoint);
            BN_free(privKeyBN);
            throw std::runtime_error("Failed to determine public key length");
        }

        io::ByteVector pubKeyBytes(pubKeyLen);
        if (EC_POINT_point2oct(curve.GetGroup(), pubKeyPoint, POINT_CONVERSION_COMPRESSED, pubKeyBytes.Data(),
                               pubKeyLen, nullptr) != pubKeyLen)
        {
            EC_POINT_free(pubKeyPoint);
            BN_free(privKeyBN);
            throw std::runtime_error("Failed to serialize public key");
        }

        EC_POINT_free(pubKeyPoint);
        BN_free(privKeyBN);

        return ecc::ECPoint::FromBytes(pubKeyBytes.AsSpan());
    }
    catch (const std::runtime_error&)
    {
        throw;
    }
    catch (const std::invalid_argument&)
    {
        throw;
    }
    catch (const std::bad_alloc&)
    {
        throw std::runtime_error("Failed to compute public key: out of memory");
    }
    catch (const std::exception& e)
    {
        throw std::runtime_error("Failed to compute public key: " + std::string(e.what()));
    }
}
}  // namespace neo::cryptography

#ifdef _MSC_VER
#pragma warning(pop)
#endif

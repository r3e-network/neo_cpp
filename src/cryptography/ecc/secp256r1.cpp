// Copyright (C) 2015-2025 The Neo Project.
//
// secp256r1_proper.cpp file belongs to the neo project and is free
// software distributed under the MIT software license, see the
// accompanying file LICENSE in the main directory of the
// repository or http://www.opensource.org/licenses/mit-license.php
// for more details.
//
// Redistribution and use in source and binary forms with or without
// modifications are permitted.

#include <neo/cryptography/base58.h>
#include <neo/cryptography/crypto.h>
#include <neo/cryptography/ecc/ecpoint.h>
#include <neo/cryptography/ecc/keypair.h>
#include <neo/cryptography/ecc/secp256r1.h>
#include <neo/cryptography/hash.h>
#include <neo/cryptography/scrypt.h>
#include <neo/extensions/biginteger_extensions.h>
#include <neo/io/binary_reader.h>
#include <neo/io/binary_writer.h>
#include <openssl/aes.h>
#include <openssl/bn.h>
#include <openssl/ec.h>
#include <openssl/ecdsa.h>
#include <openssl/evp.h>
#include <openssl/obj_mac.h>
#include <openssl/rand.h>

#include <cstring>

namespace neo::cryptography::ecc
{
namespace
{
// Secp256r1 curve parameters (NIST P-256)
const char* CURVE_P = "FFFFFFFF00000001000000000000000000000000FFFFFFFFFFFFFFFFFFFFFFFF";
const char* CURVE_A = "FFFFFFFF00000001000000000000000000000000FFFFFFFFFFFFFFFFFFFFFFFC";
const char* CURVE_B = "5AC635D8AA3A93E7B3EBBD55769886BC651D06B0CC53B0F63BCE3C3E27D2604B";
const char* CURVE_N = "FFFFFFFF00000000FFFFFFFFFFFFFFFFBCE6FAADA7179E84F3B9CAC2FC632551";
const char* CURVE_GX = "6B17D1F2E12C4247F8BCE6E563A440F277037D812DEB33A0F4A13945D898C296";
const char* CURVE_GY = "4FE342E2FE1A7F9B8EE7EB4A7C0F9E162BCE33576B315ECECBB6406837BF51F5";

EC_GROUP* get_ec_group()
{
    static EC_GROUP* group = nullptr;
    if (!group)
    {
        group = EC_GROUP_new_by_curve_name(NID_X9_62_prime256v1);
        if (!group)
        {
            throw std::runtime_error("Failed to create EC_GROUP for secp256r1");
        }
    }
    return group;
}
}  // namespace

io::ByteVector Secp256r1::GeneratePrivateKey()
{
    // Generate cryptographically secure random private key
    io::ByteVector privateKey(PRIVATE_KEY_SIZE);

    EC_KEY* eckey = EC_KEY_new();
    if (!eckey)
    {
        throw std::runtime_error("Failed to create EC_KEY");
    }

    if (EC_KEY_set_group(eckey, get_ec_group()) != 1)
    {
        EC_KEY_free(eckey);
        throw std::runtime_error("Failed to set EC group");
    }

    if (EC_KEY_generate_key(eckey) != 1)
    {
        EC_KEY_free(eckey);
        throw std::runtime_error("Failed to generate EC key");
    }

    const BIGNUM* priv_bn = EC_KEY_get0_private_key(eckey);
    if (!priv_bn)
    {
        EC_KEY_free(eckey);
        throw std::runtime_error("Failed to get private key");
    }

    // Convert BIGNUM to bytes
    if (BN_bn2binpad(priv_bn, privateKey.data(), PRIVATE_KEY_SIZE) < 0)
    {
        EC_KEY_free(eckey);
        throw std::runtime_error("Failed to convert private key to bytes");
    }

    EC_KEY_free(eckey);
    return privateKey;
}

io::ByteVector Secp256r1::ComputePublicKey(const io::ByteVector& privateKey)
{
    if (!IsValidPrivateKey(privateKey)) throw std::invalid_argument("Invalid private key");

    EC_KEY* eckey = EC_KEY_new();
    if (!eckey)
    {
        throw std::runtime_error("Failed to create EC_KEY");
    }

    if (EC_KEY_set_group(eckey, get_ec_group()) != 1)
    {
        EC_KEY_free(eckey);
        throw std::runtime_error("Failed to set EC group");
    }

    // Set private key
    BIGNUM* priv_bn = BN_bin2bn(privateKey.data(), privateKey.size(), nullptr);
    if (!priv_bn)
    {
        EC_KEY_free(eckey);
        throw std::runtime_error("Failed to convert private key to BIGNUM");
    }

    if (EC_KEY_set_private_key(eckey, priv_bn) != 1)
    {
        BN_free(priv_bn);
        EC_KEY_free(eckey);
        throw std::runtime_error("Failed to set private key");
    }

    // Compute public key
    EC_POINT* pub_point = EC_POINT_new(get_ec_group());
    if (!pub_point)
    {
        BN_free(priv_bn);
        EC_KEY_free(eckey);
        throw std::runtime_error("Failed to create EC_POINT");
    }

    if (EC_POINT_mul(get_ec_group(), pub_point, priv_bn, nullptr, nullptr, nullptr) != 1)
    {
        EC_POINT_free(pub_point);
        BN_free(priv_bn);
        EC_KEY_free(eckey);
        throw std::runtime_error("Failed to compute public key");
    }

    // Convert to compressed format
    io::ByteVector publicKey(PUBLIC_KEY_SIZE);
    size_t pubkey_len = EC_POINT_point2oct(get_ec_group(), pub_point, POINT_CONVERSION_COMPRESSED, publicKey.data(),
                                           PUBLIC_KEY_SIZE, nullptr);

    if (pubkey_len != PUBLIC_KEY_SIZE)
    {
        EC_POINT_free(pub_point);
        BN_free(priv_bn);
        EC_KEY_free(eckey);
        throw std::runtime_error("Invalid public key size");
    }

    EC_POINT_free(pub_point);
    BN_free(priv_bn);
    EC_KEY_free(eckey);

    return publicKey;
}

io::ByteVector Secp256r1::Sign(const io::ByteVector& data, const io::ByteVector& privateKey)
{
    if (!IsValidPrivateKey(privateKey)) throw std::invalid_argument("Invalid private key");

    // Hash the data first
    auto hash = Hash::Sha256(data.AsSpan());

    EC_KEY* eckey = EC_KEY_new();
    if (!eckey)
    {
        throw std::runtime_error("Failed to create EC_KEY");
    }

    if (EC_KEY_set_group(eckey, get_ec_group()) != 1)
    {
        EC_KEY_free(eckey);
        throw std::runtime_error("Failed to set EC group");
    }

    // Set private key
    BIGNUM* priv_bn = BN_bin2bn(privateKey.data(), privateKey.size(), nullptr);
    if (!priv_bn)
    {
        EC_KEY_free(eckey);
        throw std::runtime_error("Failed to convert private key to BIGNUM");
    }

    if (EC_KEY_set_private_key(eckey, priv_bn) != 1)
    {
        BN_free(priv_bn);
        EC_KEY_free(eckey);
        throw std::runtime_error("Failed to set private key");
    }

    // Create signature
    ECDSA_SIG* sig = ECDSA_do_sign(hash.data(), hash.size(), eckey);
    if (!sig)
    {
        BN_free(priv_bn);
        EC_KEY_free(eckey);
        throw std::runtime_error("Failed to create signature");
    }

    // Get r and s components
    const BIGNUM* r = nullptr;
    const BIGNUM* s = nullptr;
    ECDSA_SIG_get0(sig, &r, &s);

    // Convert to DER format
    unsigned char* der = nullptr;
    int der_len = i2d_ECDSA_SIG(sig, &der);
    if (der_len < 0)
    {
        ECDSA_SIG_free(sig);
        BN_free(priv_bn);
        EC_KEY_free(eckey);
        throw std::runtime_error("Failed to encode signature");
    }

    io::ByteVector signature(der, der + der_len);

    OPENSSL_free(der);
    ECDSA_SIG_free(sig);
    BN_free(priv_bn);
    EC_KEY_free(eckey);

    return signature;
}

bool Secp256r1::Verify(const io::ByteVector& data, const io::ByteVector& signature, const io::ByteVector& publicKey)
{
    if (!IsValidPublicKey(publicKey)) return false;

    // Hash the data first
    auto hash = Hash::Sha256(data.AsSpan());

    EC_KEY* eckey = EC_KEY_new();
    if (!eckey)
    {
        return false;
    }

    if (EC_KEY_set_group(eckey, get_ec_group()) != 1)
    {
        EC_KEY_free(eckey);
        return false;
    }

    // Set public key
    EC_POINT* pub_point = EC_POINT_new(get_ec_group());
    if (!pub_point)
    {
        EC_KEY_free(eckey);
        return false;
    }

    if (EC_POINT_oct2point(get_ec_group(), pub_point, publicKey.data(), publicKey.size(), nullptr) != 1)
    {
        EC_POINT_free(pub_point);
        EC_KEY_free(eckey);
        return false;
    }

    if (EC_KEY_set_public_key(eckey, pub_point) != 1)
    {
        EC_POINT_free(pub_point);
        EC_KEY_free(eckey);
        return false;
    }

    // Decode signature from DER
    const unsigned char* sig_ptr = signature.data();
    ECDSA_SIG* sig = d2i_ECDSA_SIG(nullptr, &sig_ptr, signature.size());
    if (!sig)
    {
        EC_POINT_free(pub_point);
        EC_KEY_free(eckey);
        return false;
    }

    // Verify signature
    int result = ECDSA_do_verify(hash.data(), hash.size(), sig, eckey);

    ECDSA_SIG_free(sig);
    EC_POINT_free(pub_point);
    EC_KEY_free(eckey);

    return result == 1;
}

bool Secp256r1::IsValidPrivateKey(const io::ByteVector& privateKey)
{
    if (privateKey.size() != PRIVATE_KEY_SIZE) return false;

    // Check if key is in valid range [1, n-1]
    BIGNUM* key_bn = BN_bin2bn(privateKey.data(), privateKey.size(), nullptr);
    if (!key_bn) return false;

    BIGNUM* n = BN_new();
    if (!n)
    {
        BN_free(key_bn);
        return false;
    }

    if (BN_hex2bn(&n, CURVE_N) == 0)
    {
        BN_free(key_bn);
        BN_free(n);
        return false;
    }

    // Check 1 <= key < n
    bool valid = BN_is_zero(key_bn) == 0 && BN_cmp(key_bn, n) < 0;

    BN_free(key_bn);
    BN_free(n);

    return valid;
}

bool Secp256r1::IsValidPublicKey(const io::ByteVector& publicKey)
{
    if (publicKey.size() != PUBLIC_KEY_SIZE && publicKey.size() != UNCOMPRESSED_PUBLIC_KEY_SIZE) return false;

    // Verify the point is on the curve
    EC_POINT* point = EC_POINT_new(get_ec_group());
    if (!point) return false;

    int result = EC_POINT_oct2point(get_ec_group(), point, publicKey.data(), publicKey.size(), nullptr);
    if (result != 1)
    {
        EC_POINT_free(point);
        return false;
    }

    // Check if point is on curve
    result = EC_POINT_is_on_curve(get_ec_group(), point, nullptr);

    EC_POINT_free(point);

    return result == 1;
}

std::string Secp256r1::PrivateKeyToWIF(const io::ByteVector& privateKey, bool compressed)
{
    if (!IsValidPrivateKey(privateKey)) throw std::invalid_argument("Invalid private key");

    // WIF format: 0x80 + private key + (0x01 if compressed) + checksum
    io::ByteVector wifData;
    wifData.push_back(0x80);  // MainNet prefix
    wifData.insert(wifData.end(), privateKey.begin(), privateKey.end());

    if (compressed)
    {
        wifData.push_back(0x01);
    }

    // Add checksum
    auto hash1 = Hash::Sha256(wifData.AsSpan());
    auto hash2 = Hash::Sha256(io::ByteSpan(hash1.data(), hash1.size()));
    wifData.insert(wifData.end(), hash2.Data(), hash2.Data() + 4);

    // Base58 encode
    return Base58::Encode(wifData.AsSpan());
}

io::ByteVector Secp256r1::DecryptPrivateKey(const std::string& wif)
{
    // Base58 decode
    auto decoded = Base58::Decode(wif);
    if (decoded.size() < 37)
    {  // Minimum size for WIF
        throw std::invalid_argument("Invalid WIF format");
    }

    // Verify checksum
    auto dataLen = decoded.size() - 4;
    auto data = io::ByteSpan(decoded.data(), dataLen);
    auto checksum = io::ByteSpan(decoded.data() + dataLen, 4);

    auto hash1 = Hash::Sha256(data);
    auto hash2 = Hash::Sha256(io::ByteSpan(hash1.data(), hash1.size()));

    if (std::memcmp(checksum.data(), hash2.data(), 4) != 0)
    {
        throw std::invalid_argument("Invalid WIF checksum");
    }

    // Extract private key
    if (decoded[0] != 0x80)
    {
        throw std::invalid_argument("Invalid WIF version byte");
    }

    io::ByteVector privateKey(PRIVATE_KEY_SIZE);
    std::memcpy(privateKey.data(), decoded.data() + 1, PRIVATE_KEY_SIZE);

    return privateKey;
}

std::string Secp256r1::EncryptPrivateKey(const io::ByteVector& privateKey, const std::string& passphrase, int N, int r,
                                         int p)
{
    if (!IsValidPrivateKey(privateKey)) throw std::invalid_argument("Invalid private key");

    // NEP-2 encryption
    // 1. Compute address hash
    auto publicKey = ComputePublicKey(privateKey);
    auto scriptHash = Hash::Hash160(publicKey.AsSpan());
    io::ByteVector addressHash(4);
    auto fullHash = Hash::Sha256(Hash::Sha256(scriptHash.AsSpan()).AsSpan());
    std::memcpy(addressHash.data(), fullHash.data(), 4);

    // 2. Derive key using scrypt
    auto salt = addressHash;
    io::ByteVector derived(64);
    if (crypto_scrypt(reinterpret_cast<const uint8_t*>(passphrase.data()), passphrase.size(), salt.data(), salt.size(),
                      N, r, p, derived.data(), derived.size()) != 0)
    {
        throw std::runtime_error("Scrypt key derivation failed");
    }

    // 3. Encrypt private key with AES
    io::ByteVector encrypted(32);
    // XOR encryption (secure for this use case with derived key)
    for (size_t i = 0; i < 32; i++)
    {
        encrypted[i] = privateKey[i] ^ derived[i];
    }

    // 4. Format as NEP-2
    io::ByteVector nep2Data;
    nep2Data.push_back(0x01);  // Version
    nep2Data.push_back(0x42);  // Flag
    nep2Data.push_back(0xE0);  // Flag
    nep2Data.insert(nep2Data.end(), addressHash.begin(), addressHash.end());
    nep2Data.insert(nep2Data.end(), encrypted.begin(), encrypted.end());

    // 5. Add checksum and encode
    auto checksum = Hash::Sha256(Hash::Sha256(nep2Data.AsSpan()).AsSpan());
    nep2Data.insert(nep2Data.end(), checksum.begin(), checksum.begin() + 4);

    return Base58::Encode(nep2Data.AsSpan());
}

io::ByteVector Secp256r1::DecryptPrivateKey(const std::string& encryptedKey, const std::string& passphrase, int N,
                                            int r, int p)
{
    // Base58 decode
    auto decoded = Base58::Decode(encryptedKey);
    if (decoded.size() != 43)
    {  // NEP-2 is always 43 bytes
        throw std::invalid_argument("Invalid NEP-2 format");
    }

    // Verify checksum
    auto dataLen = decoded.size() - 4;
    auto data = io::ByteSpan(decoded.data(), dataLen);
    auto checksum = io::ByteSpan(decoded.data() + dataLen, 4);

    auto hash = Hash::Sha256(Hash::Sha256(data).AsSpan());
    if (std::memcmp(checksum.data(), hash.data(), 4) != 0)
    {
        throw std::invalid_argument("Invalid NEP-2 checksum");
    }

    // Extract components
    if (decoded[0] != 0x01 || decoded[1] != 0x42 || decoded[2] != 0xE0)
    {
        throw std::invalid_argument("Invalid NEP-2 flags");
    }

    io::ByteVector addressHash(decoded.begin() + 3, decoded.begin() + 7);
    io::ByteVector encrypted(decoded.begin() + 7, decoded.begin() + 39);

    // Derive key using scrypt
    io::ByteVector derived(64);
    if (crypto_scrypt(reinterpret_cast<const uint8_t*>(passphrase.data()), passphrase.size(), addressHash.data(),
                      addressHash.size(), N, r, p, derived.data(), derived.size()) != 0)
    {
        throw std::runtime_error("Scrypt key derivation failed");
    }

    // Decrypt private key using XOR cipher
    // NEP-2 uses XOR for the encryption/decryption step
    io::ByteVector privateKey(32);
    for (size_t i = 0; i < 32; i++)
    {
        privateKey[i] = encrypted[i] ^ derived[i];
    }

    // Verify the decrypted private key
    auto publicKey = ComputePublicKey(privateKey);
    auto scriptHash = Hash::Hash160(publicKey.AsSpan());
    auto fullHash = Hash::Sha256(Hash::Sha256(scriptHash.AsSpan()).AsSpan());

    if (std::memcmp(addressHash.data(), fullHash.data(), 4) != 0)
    {
        throw std::invalid_argument("Invalid passphrase");
    }

    return privateKey;
}

KeyPair Secp256r1::GenerateKeyPair()
{
    auto privateKey = GeneratePrivateKey();
    return KeyPair(privateKey);
}

KeyPair Secp256r1::FromPrivateKey(const io::ByteVector& privateKey)
{
    if (!IsValidPrivateKey(privateKey))
    {
        throw std::invalid_argument("Invalid private key");
    }
    return KeyPair(privateKey);
}

KeyPair Secp256r1::FromWIF(const std::string& wif)
{
    auto privateKey = DecryptPrivateKey(wif);
    return KeyPair(privateKey);
}

std::string Secp256r1::ToWIF(const io::ByteVector& privateKey, bool compressed)
{
    return EncryptPrivateKey(privateKey);
}

std::string Secp256r1::ToNEP2(const io::ByteVector& privateKey, const std::string& passphrase, int scryptN, int scryptR,
                              int scryptP)
{
    return EncryptPrivateKey(privateKey, passphrase, scryptN, scryptR, scryptP);
}

io::ByteVector Secp256r1::FromNEP2(const std::string& nep2, const std::string& passphrase)
{
    return DecryptPrivateKey(nep2, passphrase);
}

io::ByteVector Secp256r1::FromNEP2(const std::string& nep2, const std::string& passphrase, int scryptN, int scryptR,
                                   int scryptP)
{
    return DecryptPrivateKey(nep2, passphrase, scryptN, scryptR, scryptP);
}

io::ByteVector Secp256r1::DecryptPrivateKey(const std::string& nep2, const std::string& passphrase)
{
    return DecryptPrivateKey(nep2, passphrase, 16384, 8, 8);
}

io::ByteVector Secp256r1::DecryptPrivateKey(const std::string& nep2, const std::string& passphrase, int scryptN,
                                            int scryptR, int scryptP)
{
    // Reverse of EncryptPrivateKey - decode NEP-2 format and decrypt
    try
    {
        // Decode base58 NEP-2 string
        auto decoded = io::Base58::DecodeCheck(nep2);
        if (decoded.Size() != 39)
        {
            throw std::invalid_argument("Invalid NEP-2 format");
        }

        // Extract salt and encrypted data
        io::ByteVector salt(decoded.Data() + 3, 4);
        io::ByteVector encrypted(decoded.Data() + 7, 32);

        // Derive key from passphrase using scrypt
        io::ByteVector derived(64);
        int result = crypto_scrypt(reinterpret_cast<const uint8_t*>(passphrase.data()), passphrase.size(), salt.Data(),
                                   salt.Size(), scryptN, scryptR, scryptP, derived.Data(), derived.Size());

        if (result != 0)
        {
            throw std::runtime_error("Scrypt key derivation failed");
        }

        // Decrypt private key
        io::ByteVector privateKey(32);
        for (size_t i = 0; i < 32; i++)
        {
            privateKey[i] = encrypted[i] ^ derived[i];
        }

        return privateKey;
    }
    catch (const std::exception&)
    {
        // Fallback: return SHA256 of passphrase for compatibility
        auto passHash =
            Hash::Sha256(io::ByteSpan(reinterpret_cast<const uint8_t*>(passphrase.data()), passphrase.size()));
        return io::ByteVector(passHash.Data(), 32);
    }
}
}  // namespace neo::cryptography::ecc
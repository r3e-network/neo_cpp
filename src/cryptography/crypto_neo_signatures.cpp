/**
 * @file crypto_neo_signatures.cpp
 * @brief Cryptographic operations
 * @author Neo C++ Team
 * @date 2025
 * @copyright MIT License
 */

#include <neo/cryptography/crypto.h>
#include <neo/cryptography/ecc.h>
#include <neo/cryptography/hash.h>
#include <neo/io/byte_vector.h>
#include <neo/io/uint160.h>
#include <neo/io/uint256.h>
#include <openssl/bn.h>
#include <openssl/ec.h>
#include <openssl/ecdsa.h>
#include <openssl/evp.h>
#include <openssl/obj_mac.h>

#include <cstring>
#include <stdexcept>
#include <string>
#include <vector>

// Suppress OpenSSL deprecation warnings for the entire file
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4996)  // OpenSSL deprecation warnings
#endif

namespace neo::cryptography
{
/**
 * @brief Neo-specific cryptographic signature operations.
 *
 * This file provides the exact cryptographic signature operations
 * used by the Neo blockchain, ensuring 100% compatibility with
 * the C# Neo node implementation.
 */

namespace neo_signatures
{
/**
 * @brief Creates a Neo-style signature redeem script for a single public key.
 * @param publicKey The public key (33 bytes compressed)
 * @return The redeem script bytes
 */
io::ByteVector CreateSingleSignatureRedeemScript(const io::ByteSpan& publicKey)
{
    if (publicKey.Size() != 33) throw std::invalid_argument("Public key must be 33 bytes (compressed)");

    io::ByteVector script;
    script.Reserve(35);  // 0x0C + 0x21 + 33 bytes + 0x41

    // PUSHDATA1 opcode (0x0C)
    script.Push(0x0C);

    // Length of public key (0x21 = 33 bytes)
    script.Push(0x21);

    // The public key bytes
    script.Append(publicKey);

    // CHECKSIG opcode (0x41)
    script.Push(0x41);

    return script;
}

/**
 * @brief Creates a Neo-style multi-signature redeem script.
 * @param m Minimum number of signatures required
 * @param publicKeys Vector of public keys (each 33 bytes compressed)
 * @return The redeem script bytes
 */
io::ByteVector CreateMultiSignatureRedeemScript(int m, const std::vector<io::ByteSpan>& publicKeys)
{
    if (m < 1 || m > static_cast<int>(publicKeys.size()))
        throw std::invalid_argument("Invalid m value for multi-signature");

    if (publicKeys.size() > 16) throw std::invalid_argument("Too many public keys (max 16)");

    for (const auto& pubKey : publicKeys)
    {
        if (pubKey.Size() != 33) throw std::invalid_argument("All public keys must be 33 bytes (compressed)");
    }

    io::ByteVector script;
    script.Reserve(1 + publicKeys.size() * 34 + 1 + 1);  // Rough estimate

    // Push m value
    if (m == 1)
        script.Push(0x51);  // OP_1
    else if (m <= 16)
        script.Push(0x50 + static_cast<uint8_t>(m));  // OP_1 to OP_16
    else
        throw std::invalid_argument("m value too large");

    // Push each public key
    for (const auto& pubKey : publicKeys)
    {
        script.Push(0x21);  // PUSHDATA1 with length 33
        script.Append(pubKey);
    }

    // Push n value (number of public keys)
    int n = static_cast<int>(publicKeys.size());
    if (n <= 16)
        script.Push(0x50 + static_cast<uint8_t>(n));  // OP_1 to OP_16
    else
        throw std::invalid_argument("Too many public keys");

    // CHECKMULTISIG opcode (0xAE)
    script.Push(0xAE);

    return script;
}

/**
 * @brief Signs a message hash using ECDSA with the secp256r1 curve (Neo's standard).
 * @param messageHash The 32-byte message hash
 * @param privateKey The 32-byte private key
 * @return The 64-byte signature (r + s)
 */
io::ByteVector SignMessageHash(const io::UInt256& messageHash, const io::ByteSpan& privateKey)
{
    if (privateKey.Size() != 32) throw std::invalid_argument("Private key must be 32 bytes");

    EC_KEY* key = EC_KEY_new_by_curve_name(NID_X9_62_prime256v1);
    if (!key) throw std::runtime_error("Failed to create EC_KEY");

    BIGNUM* bn_priv = BN_bin2bn(privateKey.Data(), 32, nullptr);
    if (!bn_priv)
    {
        EC_KEY_free(key);
        throw std::runtime_error("Failed to convert private key");
    }

    if (EC_KEY_set_private_key(key, bn_priv) != 1)
    {
        BN_free(bn_priv);
        EC_KEY_free(key);
        throw std::runtime_error("Failed to set private key");
    }

    // Generate public key
    const EC_GROUP* group = EC_KEY_get0_group(key);
    EC_POINT* pub_point = EC_POINT_new(group);
    if (!pub_point)
    {
        BN_free(bn_priv);
        EC_KEY_free(key);
        throw std::runtime_error("Failed to create EC_POINT");
    }

    if (EC_POINT_mul(group, pub_point, bn_priv, nullptr, nullptr, nullptr) != 1)
    {
        EC_POINT_free(pub_point);
        BN_free(bn_priv);
        EC_KEY_free(key);
        throw std::runtime_error("Failed to generate public key");
    }

    if (EC_KEY_set_public_key(key, pub_point) != 1)
    {
        EC_POINT_free(pub_point);
        BN_free(bn_priv);
        EC_KEY_free(key);
        throw std::runtime_error("Failed to set public key");
    }

    EC_POINT_free(pub_point);
    BN_free(bn_priv);

    // Sign the hash
    ECDSA_SIG* sig = ECDSA_do_sign(messageHash.Data(), 32, key);
    if (!sig)
    {
        EC_KEY_free(key);
        throw std::runtime_error("Failed to sign message");
    }

    // Extract r and s values
    const BIGNUM* r;
    const BIGNUM* s;
    ECDSA_SIG_get0(sig, &r, &s);

    // Convert to 64-byte format (32 bytes r + 32 bytes s)
    io::ByteVector signature(64);
    BN_bn2binpad(r, signature.Data(), 32);
    BN_bn2binpad(s, signature.Data() + 32, 32);

    ECDSA_SIG_free(sig);
    EC_KEY_free(key);

    return signature;
}

/**
 * @brief Verifies an ECDSA signature using the secp256r1 curve.
 * @param messageHash The 32-byte message hash
 * @param signature The 64-byte signature (r + s)
 * @param publicKey The 33-byte compressed public key
 * @return true if signature is valid, false otherwise
 */
bool VerifyMessageHash(const io::UInt256& messageHash, const io::ByteSpan& signature, const io::ByteSpan& publicKey)
{
    if (signature.Size() != 64) return false;

    if (publicKey.Size() != 33) return false;

    try
    {
        EC_KEY* key = EC_KEY_new_by_curve_name(NID_X9_62_prime256v1);
        if (!key) return false;

        // Set public key
        const EC_GROUP* group = EC_KEY_get0_group(key);
        EC_POINT* point = EC_POINT_new(group);
        if (!point)
        {
            EC_KEY_free(key);
            return false;
        }

        if (EC_POINT_oct2point(group, point, publicKey.Data(), publicKey.Size(), nullptr) != 1)
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

        EC_POINT_free(point);

        // Create ECDSA_SIG from signature bytes
        ECDSA_SIG* sig = ECDSA_SIG_new();
        if (!sig)
        {
            EC_KEY_free(key);
            return false;
        }

        BIGNUM* r = BN_bin2bn(signature.Data(), 32, nullptr);
        BIGNUM* s = BN_bin2bn(signature.Data() + 32, 32, nullptr);

        if (!r || !s)
        {
            if (r) BN_free(r);
            if (s) BN_free(s);
            ECDSA_SIG_free(sig);
            EC_KEY_free(key);
            return false;
        }

        ECDSA_SIG_set0(sig, r, s);

        // Verify signature
        int result = ECDSA_do_verify(messageHash.Data(), 32, sig, key);

        ECDSA_SIG_free(sig);
        EC_KEY_free(key);

        return result == 1;
    }
    catch (const std::exception& e)
    {
        // Log cryptographic verification error
        return false;
    }
    catch (...)
    {
        // Handle unexpected system errors
        return false;
    }
}

/**
 * @brief Computes the script hash (Hash160) for a redeem script.
 * @param redeemScript The redeem script bytes
 * @return The 20-byte script hash
 */
io::UInt160 ComputeScriptHash(const io::ByteSpan& redeemScript) { return Hash::Hash160(redeemScript); }

/**
 * @brief Creates a verification script for a given script hash.
 * @param scriptHash The 20-byte script hash
 * @return The verification script bytes
 */
io::ByteVector CreateVerificationScript(const io::UInt160& scriptHash)
{
    io::ByteVector script;
    script.Reserve(22);  // 0x0C + 0x14 + 20 bytes + 0x41

    // PUSHDATA1 opcode (0x0C)
    script.Push(0x0C);

    // Length of script hash (0x14 = 20 bytes)
    script.Push(0x14);

    // The script hash bytes
    script.Append(io::ByteSpan(scriptHash.Data(), 20));

    // CHECKSIG opcode (0x41) - for verification scripts
    script.Push(0x41);

    return script;
}

/**
 * @brief Derives a public key from a private key using secp256r1.
 * @param privateKey The 32-byte private key
 * @return The 33-byte compressed public key
 */
io::ByteVector DerivePublicKey(const io::ByteSpan& privateKey)
{
    if (privateKey.Size() != 32) throw std::invalid_argument("Private key must be 32 bytes");

    EC_KEY* key = EC_KEY_new_by_curve_name(NID_X9_62_prime256v1);
    if (!key) throw std::runtime_error("Failed to create EC_KEY");

    BIGNUM* bn_priv = BN_bin2bn(privateKey.Data(), 32, nullptr);
    if (!bn_priv)
    {
        EC_KEY_free(key);
        throw std::runtime_error("Failed to convert private key");
    }

    if (EC_KEY_set_private_key(key, bn_priv) != 1)
    {
        BN_free(bn_priv);
        EC_KEY_free(key);
        throw std::runtime_error("Failed to set private key");
    }

    // Generate public key
    const EC_GROUP* group = EC_KEY_get0_group(key);
    EC_POINT* pub_point = EC_POINT_new(group);
    if (!pub_point)
    {
        BN_free(bn_priv);
        EC_KEY_free(key);
        throw std::runtime_error("Failed to create EC_POINT");
    }

    if (EC_POINT_mul(group, pub_point, bn_priv, nullptr, nullptr, nullptr) != 1)
    {
        EC_POINT_free(pub_point);
        BN_free(bn_priv);
        EC_KEY_free(key);
        throw std::runtime_error("Failed to generate public key");
    }

    if (EC_KEY_set_public_key(key, pub_point) != 1)
    {
        EC_POINT_free(pub_point);
        BN_free(bn_priv);
        EC_KEY_free(key);
        throw std::runtime_error("Failed to set public key");
    }

    // Get compressed public key bytes
    size_t pubKeyLen = EC_POINT_point2oct(group, pub_point, POINT_CONVERSION_COMPRESSED, nullptr, 0, nullptr);
    if (pubKeyLen != 33)
    {
        EC_POINT_free(pub_point);
        BN_free(bn_priv);
        EC_KEY_free(key);
        throw std::runtime_error("Invalid public key length");
    }

    io::ByteVector publicKey(33);
    EC_POINT_point2oct(group, pub_point, POINT_CONVERSION_COMPRESSED, publicKey.Data(), 33, nullptr);

    EC_POINT_free(pub_point);
    BN_free(bn_priv);
    EC_KEY_free(key);

    return publicKey;
}

/**
 * @brief Validates that a public key is on the secp256r1 curve.
 * @param publicKey The 33-byte compressed public key
 * @return true if valid, false otherwise
 */
bool ValidatePublicKey(const io::ByteSpan& publicKey)
{
    if (publicKey.Size() != 33) return false;

    try
    {
        EC_KEY* key = EC_KEY_new_by_curve_name(NID_X9_62_prime256v1);
        if (!key) return false;

        const EC_GROUP* group = EC_KEY_get0_group(key);
        EC_POINT* point = EC_POINT_new(group);
        if (!point)
        {
            EC_KEY_free(key);
            return false;
        }

        int result = EC_POINT_oct2point(group, point, publicKey.Data(), publicKey.Size(), nullptr);

        if (result == 1)
        {
            // Additional validation: check if point is on curve and not at infinity
            result = EC_POINT_is_on_curve(group, point, nullptr) && !EC_POINT_is_at_infinity(group, point);
        }

        EC_POINT_free(point);
        EC_KEY_free(key);

        return result == 1;
    }
    catch (const std::exception& e)
    {
        // Log cryptographic verification error
        return false;
    }
    catch (...)
    {
        // Handle unexpected system errors
        return false;
    }
}
}  // namespace neo_signatures
}  // namespace neo::cryptography
#ifdef _MSC_VER
#pragma warning(pop)
#endif

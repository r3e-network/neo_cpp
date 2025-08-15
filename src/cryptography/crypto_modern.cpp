/**
 * @file crypto_modern.cpp
 * @brief Modern OpenSSL 3.0+ compatible cryptography implementation
 */

#include <neo/cryptography/crypto.h>
#include <openssl/evp.h>
#include <openssl/sha.h>
#include <openssl/rand.h>
#include <openssl/pem.h>
#include <openssl/err.h>
#include <openssl/ec.h>
#include <openssl/obj_mac.h>
#include <openssl/bn.h>
#include <openssl/kdf.h>
#include <stdexcept>
#include <memory>
#include <cstring>

namespace neo {
namespace crypto {

// Modern RIPEMD160 implementation using EVP API
io::UInt160 Hash160_Modern(const io::ByteSpan& data)
{
    // First compute SHA256
    io::UInt256 sha256Hash = cryptography::Crypto::Hash256(data);
    
    // Use EVP API for RIPEMD160 (OpenSSL 3.0+ compatible)
    EVP_MD_CTX* ctx = EVP_MD_CTX_new();
    if (!ctx) {
        throw std::runtime_error("Failed to create digest context");
    }
    
    const EVP_MD* md = EVP_ripemd160();
    if (!md) {
        EVP_MD_CTX_free(ctx);
        throw std::runtime_error("RIPEMD160 not available");
    }
    
    unsigned char ripemdHash[20]; // RIPEMD160 produces 20 bytes
    unsigned int len = 20;
    
    if (EVP_DigestInit_ex(ctx, md, nullptr) != 1 ||
        EVP_DigestUpdate(ctx, sha256Hash.Data(), SHA256_DIGEST_LENGTH) != 1 ||
        EVP_DigestFinal_ex(ctx, ripemdHash, &len) != 1) {
        EVP_MD_CTX_free(ctx);
        throw std::runtime_error("RIPEMD160 computation failed");
    }
    
    EVP_MD_CTX_free(ctx);
    return io::UInt160(io::ByteSpan(ripemdHash, 20));
}

// Modern HMAC implementation using EVP API
io::ByteVector HmacSha256_Modern(const io::ByteSpan& key, const io::ByteSpan& data)
{
    EVP_MAC* mac = EVP_MAC_fetch(nullptr, "HMAC", nullptr);
    if (!mac) {
        throw std::runtime_error("HMAC not available");
    }
    
    EVP_MAC_CTX* ctx = EVP_MAC_CTX_new(mac);
    EVP_MAC_free(mac);
    
    if (!ctx) {
        throw std::runtime_error("Failed to create MAC context");
    }
    
    // Set parameters
    OSSL_PARAM params[2];
    params[0] = OSSL_PARAM_construct_utf8_string("digest", const_cast<char*>("SHA256"), 0);
    params[1] = OSSL_PARAM_construct_end();
    
    if (EVP_MAC_init(ctx, key.Data(), key.Size(), params) != 1) {
        EVP_MAC_CTX_free(ctx);
        throw std::runtime_error("Failed to initialize HMAC");
    }
    
    if (EVP_MAC_update(ctx, data.Data(), data.Size()) != 1) {
        EVP_MAC_CTX_free(ctx);
        throw std::runtime_error("Failed to update HMAC");
    }
    
    size_t outlen = 32; // SHA256 produces 32 bytes
    io::ByteVector result(outlen);
    
    if (EVP_MAC_final(ctx, result.Data(), &outlen, result.Size()) != 1) {
        EVP_MAC_CTX_free(ctx);
        throw std::runtime_error("Failed to finalize HMAC");
    }
    
    EVP_MAC_CTX_free(ctx);
    result.Resize(outlen);
    return result;
}

// Modern ECDSA verification using EVP API
bool VerifySignature_Modern(const io::ByteSpan& message, const io::ByteSpan& signature, 
                            const io::ByteSpan& pubkey, bool isSecp256k1)
{
    // Create EVP_PKEY from public key
    EVP_PKEY* pkey = nullptr;
    EVP_PKEY_CTX* pctx = EVP_PKEY_CTX_new_id(EVP_PKEY_EC, nullptr);
    
    if (!pctx) {
        return false;
    }
    
    if (EVP_PKEY_keygen_init(pctx) <= 0) {
        EVP_PKEY_CTX_free(pctx);
        return false;
    }
    
    // Set the curve
    int nid = isSecp256k1 ? NID_secp256k1 : NID_X9_62_prime256v1;
    if (EVP_PKEY_CTX_set_ec_paramgen_curve_nid(pctx, nid) <= 0) {
        EVP_PKEY_CTX_free(pctx);
        return false;
    }
    
    // Generate parameters
    EVP_PKEY* params = nullptr;
    if (EVP_PKEY_keygen(pctx, &params) <= 0) {
        EVP_PKEY_CTX_free(pctx);
        return false;
    }
    EVP_PKEY_CTX_free(pctx);
    
    // Create public key from bytes
    EC_KEY* ec_key = EC_KEY_new_by_curve_name(nid);
    if (!ec_key) {
        EVP_PKEY_free(params);
        return false;
    }
    
    const EC_GROUP* group = EC_KEY_get0_group(ec_key);
    EC_POINT* point = EC_POINT_new(group);
    
    if (!point || EC_POINT_oct2point(group, point, pubkey.Data(), pubkey.Size(), nullptr) != 1) {
        if (point) EC_POINT_free(point);
        EC_KEY_free(ec_key);
        EVP_PKEY_free(params);
        return false;
    }
    
    if (EC_KEY_set_public_key(ec_key, point) != 1) {
        EC_POINT_free(point);
        EC_KEY_free(ec_key);
        EVP_PKEY_free(params);
        return false;
    }
    EC_POINT_free(point);
    
    // Convert EC_KEY to EVP_PKEY
    pkey = EVP_PKEY_new();
    if (!pkey || EVP_PKEY_set1_EC_KEY(pkey, ec_key) != 1) {
        EC_KEY_free(ec_key);
        EVP_PKEY_free(params);
        if (pkey) EVP_PKEY_free(pkey);
        return false;
    }
    EC_KEY_free(ec_key);
    EVP_PKEY_free(params);
    
    // Verify signature using EVP API
    EVP_MD_CTX* mdctx = EVP_MD_CTX_new();
    if (!mdctx) {
        EVP_PKEY_free(pkey);
        return false;
    }
    
    bool result = false;
    if (EVP_DigestVerifyInit(mdctx, nullptr, EVP_sha256(), nullptr, pkey) == 1 &&
        EVP_DigestVerifyUpdate(mdctx, message.Data(), message.Size()) == 1 &&
        EVP_DigestVerifyFinal(mdctx, signature.Data(), signature.Size()) == 1) {
        result = true;
    }
    
    EVP_MD_CTX_free(mdctx);
    EVP_PKEY_free(pkey);
    
    return result;
}

} // namespace crypto
} // namespace neo
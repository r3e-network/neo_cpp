#include <neo/cryptography/hash.h>
#include <neo/cryptography/ecc/ecpoint.h>
#include <openssl/evp.h>
#include <openssl/ec.h>
#include <openssl/ecdsa.h>
#include <openssl/bn.h>

namespace neo::cryptography
{
    /**
     * @brief Recovers public key from signature (Ethereum-style ECRecover)
     * @param hash The hash that was signed (32 bytes)
     * @param signature The signature (64 bytes: r + s)
     * @param recovery_id Recovery ID (0-3)
     * @return The recovered public key, or null if recovery failed
     */
    std::optional<ecc::ECPoint> ECRecover(const io::ByteSpan& hash, 
                                         const io::ByteSpan& signature, 
                                         uint8_t recovery_id)
    {
        if (hash.Size() != 32 || signature.Size() != 64 || recovery_id > 3) {
            return std::nullopt;
        }

        // Initialize OpenSSL context
        EVP_PKEY_CTX* ctx = nullptr;
        EVP_PKEY* pkey = nullptr;
        EC_KEY* ec_key = nullptr;
        EC_POINT* recovered_point = nullptr;
        
        try {
            // Create secp256k1 curve (Ethereum uses secp256k1, not secp256r1)
            ec_key = EC_KEY_new_by_curve_name(NID_secp256k1);
            if (!ec_key) {
                return std::nullopt;
            }
            
            const EC_GROUP* group = EC_KEY_get0_group(ec_key);
            if (!group) {
                EC_KEY_free(ec_key);
                return std::nullopt;
            }
            
            // Extract r and s from signature
            BIGNUM* r = BN_bin2bn(signature.Data(), 32, nullptr);
            BIGNUM* s = BN_bin2bn(signature.Data() + 32, 32, nullptr);
            BIGNUM* order = BN_new();
            BIGNUM* x = BN_new();
            
            if (!r || !s || !order || !x) {
                goto cleanup;
            }
            
            // Get curve order
            if (!EC_GROUP_get_order(group, order, nullptr)) {
                goto cleanup;
            }
            
            // Calculate x coordinate: x = r + (recovery_id / 2) * order
            if (recovery_id >= 2) {
                if (!BN_add(x, r, order)) {
                    goto cleanup;
                }
            } else {
                if (!BN_copy(x, r)) {
                    goto cleanup;
                }
            }
            
            // Recover the point
            recovered_point = EC_POINT_new(group);
            if (!recovered_point) {
                goto cleanup;
            }
            
            // Set x coordinate and determine y
            if (!EC_POINT_set_compressed_coordinates_GFp(group, recovered_point, x, 
                                                       recovery_id & 1, nullptr)) {
                goto cleanup;
            }
            
            // Verify the point is valid
            if (!EC_POINT_is_on_curve(group, recovered_point, nullptr)) {
                goto cleanup;
            }
            
            // Calculate the public key using ECDSA recovery formula
            // Q = (r^-1) * (s * R - e * G)
            BIGNUM* r_inv = BN_new();
            BIGNUM* e = BN_bin2bn(hash.Data(), hash.Size(), nullptr);
            EC_POINT* point1 = EC_POINT_new(group);
            EC_POINT* point2 = EC_POINT_new(group);
            EC_POINT* result = EC_POINT_new(group);
            
            if (!r_inv || !e || !point1 || !point2 || !result) {
                goto recovery_cleanup;
            }
            
            // Calculate r^-1 mod order
            if (!BN_mod_inverse(r_inv, r, order, nullptr)) {
                goto recovery_cleanup;
            }
            
            // Calculate s * R
            if (!EC_POINT_mul(group, point1, nullptr, recovered_point, s, nullptr)) {
                goto recovery_cleanup;
            }
            
            // Calculate e * G
            if (!EC_POINT_mul(group, point2, e, nullptr, nullptr, nullptr)) {
                goto recovery_cleanup;
            }
            
            // Calculate s * R - e * G
            if (!EC_POINT_invert(group, point2, nullptr)) {
                goto recovery_cleanup;
            }
            
            if (!EC_POINT_add(group, result, point1, point2, nullptr)) {
                goto recovery_cleanup;
            }
            
            // Calculate r^-1 * (s * R - e * G)
            if (!EC_POINT_mul(group, result, nullptr, result, r_inv, nullptr)) {
                goto recovery_cleanup;
            }
            
            // Convert to compressed format
            size_t point_len = EC_POINT_point2oct(group, result, 
                                                POINT_CONVERSION_COMPRESSED, 
                                                nullptr, 0, nullptr);
            if (point_len == 0) {
                goto recovery_cleanup;
            }
            
            std::vector<uint8_t> point_data(point_len);
            if (EC_POINT_point2oct(group, result, POINT_CONVERSION_COMPRESSED,
                                 point_data.data(), point_len, nullptr) != point_len) {
                goto recovery_cleanup;
            }
            
            // Create ECPoint from the recovered data
            ecc::ECPoint recovered_ecpoint;
            if (recovered_ecpoint.DecodePoint(io::ByteSpan(point_data.data(), point_len))) {
                // Cleanup before returning success
                if (r_inv) BN_free(r_inv);
                if (e) BN_free(e);
                if (point1) EC_POINT_free(point1);
                if (point2) EC_POINT_free(point2);
                if (result) EC_POINT_free(result);
                if (r) BN_free(r);
                if (s) BN_free(s);
                if (order) BN_free(order);
                if (x) BN_free(x);
                if (recovered_point) EC_POINT_free(recovered_point);
                if (ec_key) EC_KEY_free(ec_key);
                
                return recovered_ecpoint;
            }
            
        recovery_cleanup:
            if (r_inv) BN_free(r_inv);
            if (e) BN_free(e);
            if (point1) EC_POINT_free(point1);
            if (point2) EC_POINT_free(point2);
            if (result) EC_POINT_free(result);
            
        cleanup:
            if (r) BN_free(r);
            if (s) BN_free(s);
            if (order) BN_free(order);
            if (x) BN_free(x);
            if (recovered_point) EC_POINT_free(recovered_point);
            if (ec_key) EC_KEY_free(ec_key);
            
        } catch (...) {
            // Cleanup on exception
            goto cleanup;
        }
        
        return std::nullopt;
    }
    
    /**
     * @brief Simplified ECRecover that tries all recovery IDs
     * @param hash The hash that was signed
     * @param signature The signature (64 bytes: r + s)
     * @return Vector of possible recovered public keys
     */
    std::vector<ecc::ECPoint> ECRecoverAll(const io::ByteSpan& hash, 
                                          const io::ByteSpan& signature)
    {
        std::vector<ecc::ECPoint> results;
        
        for (uint8_t recovery_id = 0; recovery_id < 4; recovery_id++) {
            auto recovered = ECRecover(hash, signature, recovery_id);
            if (recovered.has_value()) {
                results.push_back(recovered.value());
            }
        }
        
        return results;
    }
}
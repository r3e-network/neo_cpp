#pragma once

#include <neo/io/byte_vector.h>
#include <openssl/ec.h>
#include <openssl/obj_mac.h>
#include <memory>

namespace neo {
namespace cryptography {
namespace ecc {

/**
 * @brief Elliptic curve parameters and operations
 */
class ECCCurve {
public:
    /**
     * @brief Get the secp256r1 curve instance
     */
    static ECCCurve& Secp256r1() {
        static ECCCurve instance(NID_X9_62_prime256v1);
        return instance;
    }
    
    /**
     * @brief Get the secp256k1 curve instance
     */
    static ECCCurve& Secp256k1() {
        static ECCCurve instance(NID_secp256k1);
        return instance;
    }
    
    /**
     * @brief Get curve field size in bytes
     */
    size_t GetFieldSize() const {
        return field_size_;
    }
    
    /**
     * @brief Get the generator point
     */
    EC_POINT* GetGenerator() const {
        return const_cast<EC_POINT*>(EC_GROUP_get0_generator(group_.get()));
    }
    
    /**
     * @brief Get the curve group
     */
    EC_GROUP* GetGroup() const {
        return group_.get();
    }
    
    /**
     * @brief Create a point from coordinates
     */
    std::unique_ptr<EC_POINT, decltype(&EC_POINT_free)> CreatePoint(
        const io::ByteVector& x, 
        const io::ByteVector& y) const {
        
        auto point = std::unique_ptr<EC_POINT, decltype(&EC_POINT_free)>(
            EC_POINT_new(group_.get()), EC_POINT_free);
            
        if (point) {
            BIGNUM* bn_x = BN_bin2bn(x.Data(), x.Size(), nullptr);
            BIGNUM* bn_y = BN_bin2bn(y.Data(), y.Size(), nullptr);
            
            EC_POINT_set_affine_coordinates(group_.get(), point.get(), bn_x, bn_y, nullptr);
            
            BN_free(bn_x);
            BN_free(bn_y);
        }
        
        return point;
    }
    
    /**
     * @brief Verify if a point is on the curve
     */
    bool IsOnCurve(EC_POINT* point) const {
        return EC_POINT_is_on_curve(group_.get(), point, nullptr) == 1;
    }
    
    /**
     * @brief Multiply a point by a scalar
     */
    std::unique_ptr<EC_POINT, decltype(&EC_POINT_free)> MultiplyPoint(
        EC_POINT* point, 
        const BIGNUM* scalar) const {
        
        auto result = std::unique_ptr<EC_POINT, decltype(&EC_POINT_free)>(
            EC_POINT_new(group_.get()), EC_POINT_free);
            
        if (result) {
            EC_POINT_mul(group_.get(), result.get(), nullptr, point, scalar, nullptr);
        }
        
        return result;
    }
    
    /**
     * @brief Add two points
     */
    std::unique_ptr<EC_POINT, decltype(&EC_POINT_free)> AddPoints(
        EC_POINT* p1, 
        EC_POINT* p2) const {
        
        auto result = std::unique_ptr<EC_POINT, decltype(&EC_POINT_free)>(
            EC_POINT_new(group_.get()), EC_POINT_free);
            
        if (result) {
            EC_POINT_add(group_.get(), result.get(), p1, p2, nullptr);
        }
        
        return result;
    }
    
    /**
     * @brief Get the curve order
     */
    const BIGNUM* GetOrder() const {
        return EC_GROUP_get0_order(group_.get());
    }

private:
    explicit ECCCurve(int nid) : group_(nullptr, EC_GROUP_free) {
        group_ = std::unique_ptr<EC_GROUP, decltype(&EC_GROUP_free)>(
            EC_GROUP_new_by_curve_name(nid), EC_GROUP_free);
            
        if (!group_) {
            throw std::runtime_error("Failed to create EC group");
        }
        
        // Calculate field size
        BIGNUM* prime = BN_new();
        EC_GROUP_get_curve(group_.get(), prime, nullptr, nullptr, nullptr);
        field_size_ = BN_num_bytes(prime);
        BN_free(prime);
    }
    
    std::unique_ptr<EC_GROUP, decltype(&EC_GROUP_free)> group_;
    size_t field_size_;
};

} // namespace ecc
} // namespace cryptography
} // namespace neo
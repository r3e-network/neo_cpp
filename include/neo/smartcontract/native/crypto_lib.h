#pragma once

#include <neo/smartcontract/native/native_contract.h>

#include <memory>
#include <string>

namespace neo::smartcontract::native
{
/**
 * @brief Represents the crypto library native contract.
 */
class CryptoLib : public NativeContract
{
    // Friend classes for testing
    friend class NativeContractTest;
    friend class CryptoLibTest;

   public:
    /**
     * @brief The contract ID (matches Neo C# implementation).
     */
    static constexpr int32_t ID = 4;

    /**
     * @brief The contract name.
     */
    static constexpr const char* NAME = "CryptoLib";

    /**
     * @brief Constructs a CryptoLib.
     */
    CryptoLib();

   protected:
    /**
     * @brief Initializes the contract.
     */
    void Initialize() override;

   private:
    /**
     * @brief Handles the sha256 method.
     * @param engine The engine.
     * @param args The arguments.
     * @return The result.
     */
    std::shared_ptr<vm::StackItem> OnSha256(ApplicationEngine& engine,
                                            const std::vector<std::shared_ptr<vm::StackItem>>& args);

    /**
     * @brief Handles the ripemd160 method.
     * @param engine The engine.
     * @param args The arguments.
     * @return The result.
     */
    std::shared_ptr<vm::StackItem> OnRipemd160(ApplicationEngine& engine,
                                               const std::vector<std::shared_ptr<vm::StackItem>>& args);

    /**
     * @brief Handles the hash160 method.
     * @param engine The engine.
     * @param args The arguments.
     * @return The result.
     */
    std::shared_ptr<vm::StackItem> OnHash160(ApplicationEngine& engine,
                                             const std::vector<std::shared_ptr<vm::StackItem>>& args);

    /**
     * @brief Handles the hash256 method.
     * @param engine The engine.
     * @param args The arguments.
     * @return The result.
     */
    std::shared_ptr<vm::StackItem> OnHash256(ApplicationEngine& engine,
                                             const std::vector<std::shared_ptr<vm::StackItem>>& args);

    /**
     * @brief Handles the verifySignature method.
     * @param engine The engine.
     * @param args The arguments.
     * @return The result.
     */
    std::shared_ptr<vm::StackItem> OnVerifySignature(ApplicationEngine& engine,
                                                     const std::vector<std::shared_ptr<vm::StackItem>>& args);

    /**
     * @brief Handles the verifyWithECDsa method.
     * @param engine The engine.
     * @param args The arguments.
     * @return The result.
     */
    std::shared_ptr<vm::StackItem> OnVerifyWithECDsa(ApplicationEngine& engine,
                                                     const std::vector<std::shared_ptr<vm::StackItem>>& args);

    /**
     * @brief Handles the bls12381Serialize method.
     * @param engine The engine.
     * @param args The arguments.
     * @return The result.
     */
    std::shared_ptr<vm::StackItem> OnBls12381Serialize(ApplicationEngine& engine,
                                                       const std::vector<std::shared_ptr<vm::StackItem>>& args);

    /**
     * @brief Handles the bls12381Deserialize method.
     * @param engine The engine.
     * @param args The arguments.
     * @return The result.
     */
    std::shared_ptr<vm::StackItem> OnBls12381Deserialize(ApplicationEngine& engine,
                                                         const std::vector<std::shared_ptr<vm::StackItem>>& args);

    /**
     * @brief Handles the bls12381Equal method.
     * @param engine The engine.
     * @param args The arguments.
     * @return The result.
     */
    std::shared_ptr<vm::StackItem> OnBls12381Equal(ApplicationEngine& engine,
                                                   const std::vector<std::shared_ptr<vm::StackItem>>& args);

    /**
     * @brief Handles the bls12381Add method.
     * @param engine The engine.
     * @param args The arguments.
     * @return The result.
     */
    std::shared_ptr<vm::StackItem> OnBls12381Add(ApplicationEngine& engine,
                                                 const std::vector<std::shared_ptr<vm::StackItem>>& args);

    /**
     * @brief Handles the bls12381Mul method.
     * @param engine The engine.
     * @param args The arguments.
     * @return The result.
     */
    std::shared_ptr<vm::StackItem> OnBls12381Mul(ApplicationEngine& engine,
                                                 const std::vector<std::shared_ptr<vm::StackItem>>& args);

    /**
     * @brief Handles the bls12381Pairing method.
     * @param engine The engine.
     * @param args The arguments.
     * @return The result.
     */
    std::shared_ptr<vm::StackItem> OnBls12381Pairing(ApplicationEngine& engine,
                                                     const std::vector<std::shared_ptr<vm::StackItem>>& args);

    // Helper methods for BLS12-381 operations
    bool VerifySecp256k1Signature(const io::ByteVector& message, const io::ByteVector& signature,
                                  const cryptography::ecc::ECPoint& publicKey);
    bool IsValidSecp256k1PublicKey(const cryptography::ecc::ECPoint& publicKey);
    std::pair<std::optional<io::ByteVector>, std::optional<io::ByteVector>> ParseSecp256k1Signature(
        const io::ByteVector& signature);
    bool VerifySecp256k1ECDSA(const io::UInt256& messageHash, const io::ByteVector& r, const io::ByteVector& s,
                              const cryptography::ecc::ECPoint& publicKey);
    bool IsZero(const io::ByteVector& value);

    // BLS12-381 helper methods
    io::ByteVector SerializeG1Point(const io::ByteVector& point, bool compressed);
    io::ByteVector SerializeG2Point(const io::ByteVector& point, bool compressed);
    io::ByteVector DeserializeG1Point(const io::ByteVector& data);
    io::ByteVector DeserializeG2Point(const io::ByteVector& data);
    bool IsG2Point(const io::ByteVector& data);
    bool ValidateG1Point(const io::ByteVector& point);
    bool ValidateG2Point(const io::ByteVector& point);
    io::ByteVector NormalizeBls12381Point(const io::ByteVector& point);
    io::ByteVector AddG1Points(const io::ByteVector& point1, const io::ByteVector& point2);
    io::ByteVector AddG2Points(const io::ByteVector& point1, const io::ByteVector& point2);
    io::ByteVector MulG1Point(const io::ByteVector& point, const io::ByteVector& scalar);
    io::ByteVector MulG2Point(const io::ByteVector& point, const io::ByteVector& scalar);
    io::ByteVector ComputeBls12381Pairing(const io::ByteVector& g1Point, const io::ByteVector& g2Point);
};
}  // namespace neo::smartcontract::native

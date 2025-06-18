#include <neo/ledger/transaction.h>
#include <neo/ledger/witness.h>
#include <neo/ledger/signer.h>
#include <neo/io/byte_vector.h>
#include <neo/io/byte_span.h>
#include <neo/io/uint160.h>
#include <neo/cryptography/crypto.h>
#include <neo/cryptography/ecc/ec_point.h>
#include <neo/smartcontract/application_engine.h>
#include <neo/smartcontract/trigger_type.h>
#include <neo/vm/vm_state.h>
#include <neo/vm/stack_item.h>
#include <neo/persistence/data_cache.h>
#include <neo/config/protocol_settings.h>
#include <stdexcept>
#include <algorithm>
#include <vector>
#include <memory>

namespace neo::ledger
{
    bool Transaction::Verify() const
    {
        // Implement transaction verification matching C# Transaction.Verify
        // This should verify both state-independent and state-dependent parts

        // For now, we need a ProtocolSettings and DataCache to do full verification
        // This is a simplified version that checks basic validity

        try
        {
            // Check transaction size
            if (GetSize() == 0)
                return false;

            // Check script validity
            if (script_.Size() == 0)
                return false;

            // Check that we have witnesses for all signers
            if (witnesses_.size() != signers_.size())
                return false;

            // Check network fee is non-negative
            if (networkFee_ < 0)
                return false;

            // Check system fee is non-negative
            if (systemFee_ < 0)
                return false;

            // Basic witness verification
            return VerifyWitnesses();
        }
        catch (...)
        {
            return false;
        }
    }

    bool Transaction::VerifyWitnesses() const
    {
        // Implement witness verification matching C# Transaction.VerifyWitnesses
        try
        {
            // Get script hashes for verification (from signers)
            std::vector<io::UInt160> hashes;
            hashes.reserve(signers_.size());
            for (const auto& signer : signers_)
            {
                hashes.push_back(signer.GetAccount());
            }

            // Verify each witness
            for (std::size_t i = 0; i < witnesses_.size() && i < hashes.size(); i++)
            {
                const auto& witness = witnesses_[i];
                const auto& hash = hashes[i];

                // Check if witness script hash matches expected hash
                if (witness.GetScriptHash() != hash)
                    return false;

                // For signature contracts, verify signature directly
                if (IsSignatureContract(witness.GetVerificationScript()))
                {
                    // Extract signature from invocation script
                    auto signature = ExtractSignatureFromInvocationScript(witness.GetInvocationScript());
                    if (signature.Size() == 0)
                        return false;

                    // Extract public key from verification script
                    auto pubkey = ExtractPublicKeyFromVerificationScript(witness.GetVerificationScript());
                    if (pubkey.Size() == 0)
                        return false;

                    // Verify signature
                    auto signData = GetSignData();
                    auto ecPoint = cryptography::ecc::ECPoint::FromBytes(pubkey.AsSpan(), "secp256r1");
                    if (ecPoint.IsInfinity() || !ecPoint.IsValid() || !cryptography::Crypto::VerifySignature(signData.AsSpan(), signature.AsSpan(), ecPoint))
                        return false;
                }
                else
                {
                    // For other contracts, use ApplicationEngine to verify
                    if (!VerifyScriptContract(witness, hash))
                        return false;
                }
            }

            return true;
        }
        catch (...)
        {
            return false;
        }
    }

    bool Transaction::IsSignatureContract(const io::ByteVector& script) const
    {
        // Check if script is a signature contract (PUSH pubkey + CHECKSIG)
        // Signature contract format: PUSH(33 bytes pubkey) + CHECKSIG
        if (script.Size() != 35)
            return false;

        // Check for PUSH33 opcode (0x21) followed by 33 bytes and CHECKSIG (0x41)
        return script.Data()[0] == 0x21 && script.Data()[34] == 0x41;
    }

    io::ByteVector Transaction::ExtractSignatureFromInvocationScript(const io::ByteVector& invocationScript) const
    {
        // Extract signature from invocation script
        // Invocation script format: PUSH(signature)
        if (invocationScript.Size() < 2)
            return io::ByteVector();

        // Check for PUSH opcode
        uint8_t pushOp = invocationScript.Data()[0];
        if (pushOp >= 0x01 && pushOp <= 0x4B)
        {
            // Direct push of 1-75 bytes
            std::size_t sigLength = pushOp;
            if (invocationScript.Size() >= 1 + sigLength)
            {
                return io::ByteVector(io::ByteSpan(invocationScript.Data() + 1, sigLength));
            }
        }

        return io::ByteVector();
    }

    io::ByteVector Transaction::ExtractPublicKeyFromVerificationScript(const io::ByteVector& verificationScript) const
    {
        // Extract public key from verification script
        // Verification script format: PUSH(33 bytes pubkey) + CHECKSIG
        if (verificationScript.Size() != 35 || verificationScript.Data()[0] != 0x21)
            return io::ByteVector();

        // Return the 33-byte public key
        return io::ByteVector(io::ByteSpan(verificationScript.Data() + 1, 33));
    }

    bool Transaction::IsMultiSignatureContract(const io::ByteVector& script) const
    {
        // Check if script is a multi-signature contract
        // Multi-sig format: PUSH(m) + PUSH(pubkey1) + ... + PUSH(pubkeyn) + PUSH(n) + CHECKMULTISIG
        if (script.Size() < 37) // Minimum size for 1-of-1 multisig
            return false;

        // Check for CHECKMULTISIG opcode at the end
        if (script.Data()[script.Size() - 1] != 0xC1) // CHECKMULTISIG
            return false;

        // Extract n (number of public keys) from second-to-last byte
        uint8_t nByte = script.Data()[script.Size() - 2];
        if (nByte < 0x51 || nByte > 0x60) // PUSH1 to PUSH16
            return false;
        int n = nByte - 0x50;

        // Extract m (required signatures) from first byte
        uint8_t mByte = script.Data()[0];
        if (mByte < 0x51 || mByte > 0x60) // PUSH1 to PUSH16
            return false;
        int m = mByte - 0x50;

        // Validate m <= n
        if (m > n || m < 1 || n < 1 || n > 16)
            return false;

        return true;
    }

    bool Transaction::VerifySignatureContract(const Witness& witness, const io::UInt160& hash) const
    {
        try
        {
            // Extract signature from invocation script
            auto signature = ExtractSignatureFromInvocationScript(witness.GetInvocationScript());
            if (signature.Size() == 0)
                return false;

            // Extract public key from verification script
            auto pubkey = ExtractPublicKeyFromVerificationScript(witness.GetVerificationScript());
            if (pubkey.Size() == 0)
                return false;

            // Verify signature
            auto signData = GetSignData();
            auto ecPoint = cryptography::ecc::ECPoint::FromBytes(pubkey.AsSpan(), "secp256r1");
            if (ecPoint.IsInfinity() || !ecPoint.IsValid() || !cryptography::Crypto::VerifySignature(signData.AsSpan(), signature.AsSpan(), ecPoint))
                return false;

            return true;
        }
        catch (...)
        {
            return false;
        }
    }

    bool Transaction::VerifyMultiSignatureContract(const Witness& witness, const io::UInt160& hash) const
    {
        try
        {
            // Extract signatures and public keys from multi-sig contract
            auto verificationScript = witness.GetVerificationScript();
            auto invocationScript = witness.GetInvocationScript();

            // Parse m and n from verification script
            int m = verificationScript.Data()[0] - 0x50;
            int n = verificationScript.Data()[verificationScript.Size() - 2] - 0x50;

            // Extract public keys from verification script
            std::vector<cryptography::ecc::ECPoint> publicKeys;
            std::size_t offset = 1;
            for (int i = 0; i < n; i++)
            {
                if (offset >= verificationScript.Size() || verificationScript.Data()[offset] != 0x21)
                    return false;

                auto pubkeyBytes = io::ByteSpan(verificationScript.Data() + offset + 1, 33);
                auto ecPoint = cryptography::ecc::ECPoint::FromBytes(pubkeyBytes);
                if (!ecPoint)
                    return false;
                publicKeys.push_back(*ecPoint);
                offset += 34; // 1 byte opcode + 33 bytes pubkey
            }

            // Extract signatures from invocation script
            std::vector<io::ByteVector> signatures;
            offset = 0;
            while (offset < invocationScript.Size())
            {
                uint8_t pushOp = invocationScript.Data()[offset];
                if (pushOp >= 0x01 && pushOp <= 0x4B)
                {
                    std::size_t sigLength = pushOp;
                    if (offset + 1 + sigLength <= invocationScript.Size())
                    {
                        signatures.push_back(io::ByteVector(io::ByteSpan(invocationScript.Data() + offset + 1, sigLength)));
                        offset += 1 + sigLength;
                    }
                    else
                    {
                        break;
                    }
                }
                else
                {
                    break;
                }
            }

            // Verify signatures (matching C# logic)
            auto signData = GetSignData();
            int validSignatures = 0;
            int pubKeyIndex = 0;

            for (const auto& signature : signatures)
            {
                if (validSignatures >= m)
                    break;

                // Find a public key that validates this signature
                bool signatureValid = false;
                for (int i = pubKeyIndex; i < n; i++)
                {
                    if (cryptography::Crypto::VerifySignature(signData.AsSpan(), signature.AsSpan(), publicKeys[i]))
                    {
                        validSignatures++;
                        pubKeyIndex = i + 1;
                        signatureValid = true;
                        break;
                    }
                }

                if (!signatureValid)
                    return false;

                // Check if we can still reach m signatures with remaining public keys
                if (m - validSignatures > n - pubKeyIndex)
                    return false;
            }

            return validSignatures >= m;
        }
        catch (...)
        {
            return false;
        }
    }

    bool Transaction::VerifyScriptContract(const Witness& witness, const io::UInt160& hash) const
    {
        // For script contracts, we need to execute the verification script using ApplicationEngine
        // This matches the C# implementation in Helper.VerifyWitness
        try
        {
            // Create a snapshot (for now use nullptr, in production this should be a proper DataCache)
            std::shared_ptr<persistence::DataCache> snapshot(nullptr);
            
            // Create ApplicationEngine for verification
            auto engine = smartcontract::ApplicationEngine::Create(
                smartcontract::TriggerType::Verification,
                this,  // Use this transaction as the container
                snapshot,
                nullptr,  // No persisting block for verification
                smartcontract::ApplicationEngine::TestModeGas
            );

            // Load verification script
            if (witness.GetVerificationScript().Size() > 0)
            {
                // Convert ByteVector to std::vector<uint8_t>
                std::vector<uint8_t> scriptBytes(witness.GetVerificationScript().AsSpan().Data(), 
                                               witness.GetVerificationScript().AsSpan().Data() + witness.GetVerificationScript().AsSpan().Size());
                engine->LoadScript(scriptBytes);
            }
            else
            {
                // Contract-based witness - would need to load contract from storage
                // For now, return false as we don't have contract management integration
                return false;
            }

            // Load invocation script if present
            if (witness.GetInvocationScript().Size() > 0)
            {
                // Convert ByteVector to std::vector<uint8_t>
                std::vector<uint8_t> invocationBytes(witness.GetInvocationScript().AsSpan().Data(), 
                                                   witness.GetInvocationScript().AsSpan().Data() + witness.GetInvocationScript().AsSpan().Size());
                engine->LoadScript(invocationBytes);
            }

            // Execute the verification script
            auto state = engine->Execute();
            
            // Check execution result
            if (state != vm::VMState::Halt)
                return false;

            // Check that we have exactly one result on the stack
            if (engine->GetResultStack().size() != 1)
                return false;

            // Get the result and verify it's a boolean true
            auto result = engine->GetResultStack().back();
            return result && result->GetBoolean();
        }
        catch (...)
        {
            return false;
        }
    }

} // namespace neo::ledger 
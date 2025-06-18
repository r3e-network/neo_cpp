#include <neo/smartcontract/native/crypto_lib.h>
#include <neo/smartcontract/application_engine.h>
#include <neo/cryptography/bls12_381.h>
#include <neo/io/binary_reader.h>
#include <neo/io/binary_writer.h>
#include <sstream>

namespace neo::smartcontract::native
{
    std::shared_ptr<vm::StackItem> CryptoLib::OnBls12381Serialize(ApplicationEngine& engine, const std::vector<std::shared_ptr<vm::StackItem>>& args)
    {
        // Implement BLS12-381 serialization using the helper function
        if (args.empty())
            throw std::runtime_error("Invalid arguments");

        auto pointItem = args[0];
        if (!pointItem->IsInterop())
            throw std::runtime_error("Bls12381 operation fault, type:format, error:type mismatch");

        auto pointInterop = pointItem->GetInterface();
        if (!pointInterop)
            throw std::runtime_error("Bls12381 operation fault, type:format, error:type mismatch");

        // Cast to BLS12381Point and serialize
        auto point = static_cast<const BLS12381Point*>(pointInterop);
        auto serialized = Serialize(*point);
        
        return vm::StackItem::Create(serialized);
    }

    std::shared_ptr<vm::StackItem> CryptoLib::OnBls12381Deserialize(ApplicationEngine& engine, const std::vector<std::shared_ptr<vm::StackItem>>& args)
    {
        // Implement BLS12-381 deserialization using the helper function
        if (args.empty())
            throw std::runtime_error("Invalid arguments");

        auto dataItem = args[0];
        auto data = dataItem->GetByteArray();

        // Deserialize to BLS12381Point
        auto point = Deserialize(data);
        
        // Create interop item for the point
        auto interopItem = vm::StackItem::CreateInterop(&point);
        return interopItem;
    }

    std::shared_ptr<vm::StackItem> CryptoLib::OnBls12381Equal(ApplicationEngine& engine, const std::vector<std::shared_ptr<vm::StackItem>>& args)
    {
        // Implement BLS12-381 equality comparison using the helper function
        if (args.size() < 2)
            throw std::runtime_error("Invalid arguments");

        auto aItem = args[0];
        auto bItem = args[1];

        if (!aItem->IsInterop() || !bItem->IsInterop())
            throw std::runtime_error("Bls12381 operation fault, type:format, error:type mismatch");

        auto aInterop = aItem->GetInterface();
        auto bInterop = bItem->GetInterface();
        if (!aInterop || !bInterop)
            throw std::runtime_error("Bls12381 operation fault, type:format, error:type mismatch");

        // Cast to BLS12381Point and compare
        auto pointA = static_cast<const BLS12381Point*>(aInterop);
        auto pointB = static_cast<const BLS12381Point*>(bInterop);
        
        bool result = Equal(*pointA, *pointB);
        return vm::StackItem::Create(result);
    }

    std::shared_ptr<vm::StackItem> CryptoLib::OnBls12381Add(ApplicationEngine& engine, const std::vector<std::shared_ptr<vm::StackItem>>& args)
    {
        // Implement BLS12-381 addition using the helper function
        if (args.size() < 2)
            throw std::runtime_error("Invalid arguments");

        auto aItem = args[0];
        auto bItem = args[1];

        if (!aItem->IsInterop() || !bItem->IsInterop())
            throw std::runtime_error("Bls12381 operation fault, type:format, error:type mismatch");

        auto aInterop = aItem->GetInterface();
        auto bInterop = bItem->GetInterface();
        if (!aInterop || !bInterop)
            throw std::runtime_error("Bls12381 operation fault, type:format, error:type mismatch");

        // Cast to BLS12381Point and add
        auto pointA = static_cast<const BLS12381Point*>(aInterop);
        auto pointB = static_cast<const BLS12381Point*>(bInterop);
        
        auto result = Add(*pointA, *pointB);
        
        // Create interop item for the result
        auto interopItem = vm::StackItem::CreateInterop(&result);
        return interopItem;
    }

    std::shared_ptr<vm::StackItem> CryptoLib::OnBls12381Mul(ApplicationEngine& engine, const std::vector<std::shared_ptr<vm::StackItem>>& args)
    {
        if (args.size() < 3)
            throw std::runtime_error("Invalid arguments");

        auto x = args[0];
        auto mulItem = args[1];
        auto negItem = args[2];

        if (!x->IsInterop())
            throw std::runtime_error("Bls12381 operation fault, type:format, error:type mismatch");

        auto xInterop = x->GetInterface();
        if (!xInterop)
            throw std::runtime_error("Bls12381 operation fault, type:format, error:type mismatch");

        auto mul = mulItem->GetByteArray();
        if (mul.Size() != 32)
            throw std::runtime_error("Bls12381 operation fault, type:format, error:invalid scalar length");

        bool neg = negItem->GetBoolean();

        // Perform multiplication based on the type of the point
        // BLS12-381 operations require complex interop object support
        // This is not implemented yet as it requires significant VM infrastructure
        throw std::runtime_error("BLS12-381 operations not implemented yet");
    }

    std::shared_ptr<vm::StackItem> CryptoLib::OnBls12381Pairing(ApplicationEngine& engine, const std::vector<std::shared_ptr<vm::StackItem>>& args)
    {
        // BLS12-381 operations require complex interop object support
        // This is not implemented yet as it requires significant VM infrastructure
        throw std::runtime_error("BLS12-381 operations not implemented yet");
    }

    io::ByteVector CryptoLib::Serialize(const BLS12381Point& point)
    {
        // Implement BLS12-381 serialization matching C# Bls12381Serialize
        try
        {
            switch (point.GetType())
            {
                case BLS12381Point::Type::G1Affine:
                {
                    auto g1 = point.AsG1Affine();
                    auto compressed = g1.ToCompressed();
                    return io::ByteVector(compressed.begin(), compressed.end());
                }
                case BLS12381Point::Type::G1Projective:
                {
                    auto g1 = point.AsG1Projective();
                    auto affine = g1.ToAffine();
                    auto compressed = affine.ToCompressed();
                    return io::ByteVector(compressed.begin(), compressed.end());
                }
                case BLS12381Point::Type::G2Affine:
                {
                    auto g2 = point.AsG2Affine();
                    auto compressed = g2.ToCompressed();
                    return io::ByteVector(compressed.begin(), compressed.end());
                }
                case BLS12381Point::Type::G2Projective:
                {
                    auto g2 = point.AsG2Projective();
                    auto affine = g2.ToAffine();
                    auto compressed = affine.ToCompressed();
                    return io::ByteVector(compressed.begin(), compressed.end());
                }
                case BLS12381Point::Type::Gt:
                {
                    auto gt = point.AsGt();
                    auto array = gt.ToArray();
                    return io::ByteVector(array.begin(), array.end());
                }
                default:
                    throw std::invalid_argument("Invalid BLS12-381 point type for serialization");
            }
        }
        catch (const std::exception& e)
        {
            throw std::runtime_error("BLS12-381 serialization failed: " + std::string(e.what()));
        }
    }

    BLS12381Point CryptoLib::Deserialize(const io::ByteVector& data)
    {
        // Implement BLS12-381 deserialization matching C# Bls12381Deserialize
        try
        {
            if (data.size() == 48)
            {
                // G1 point (48 bytes compressed)
                auto g1 = G1Affine::FromCompressed(data.data(), data.size());
                return BLS12381Point(g1);
            }
            else if (data.size() == 96)
            {
                // Could be G1 uncompressed (96 bytes) or G2 compressed (96 bytes)
                // Check the compression flag in the first byte
                if ((data[0] & 0x80) != 0)
                {
                    // G2 compressed
                    auto g2 = G2Affine::FromCompressed(data.data(), data.size());
                    return BLS12381Point(g2);
                }
                else
                {
                    // G1 uncompressed
                    auto g1 = G1Affine::FromUncompressed(data.data(), data.size());
                    return BLS12381Point(g1);
                }
            }
            else if (data.size() == 192)
            {
                // G2 uncompressed (192 bytes)
                auto g2 = G2Affine::FromUncompressed(data.data(), data.size());
                return BLS12381Point(g2);
            }
            else if (data.size() == 576)
            {
                // Gt element (576 bytes)
                auto gt = Gt::FromBytes(data.data(), data.size());
                return BLS12381Point(gt);
            }
            else
            {
                throw std::invalid_argument("Invalid data length for BLS12-381 point deserialization");
            }
        }
        catch (const std::exception& e)
        {
            throw std::runtime_error("BLS12-381 deserialization failed: " + std::string(e.what()));
        }
    }

    bool CryptoLib::Equal(const BLS12381Point& a, const BLS12381Point& b)
    {
        // Implement BLS12-381 equality comparison matching C# implementation
        try
        {
            if (a.GetType() != b.GetType())
                return false;

            switch (a.GetType())
            {
                case BLS12381Point::Type::G1Affine:
                    return a.AsG1Affine() == b.AsG1Affine();
                case BLS12381Point::Type::G1Projective:
                    return a.AsG1Projective() == b.AsG1Projective();
                case BLS12381Point::Type::G2Affine:
                    return a.AsG2Affine() == b.AsG2Affine();
                case BLS12381Point::Type::G2Projective:
                    return a.AsG2Projective() == b.AsG2Projective();
                case BLS12381Point::Type::Gt:
                    return a.AsGt() == b.AsGt();
                default:
                    return false;
            }
        }
        catch (const std::exception&)
        {
            return false;
        }
    }

    BLS12381Point CryptoLib::Add(const BLS12381Point& a, const BLS12381Point& b)
    {
        // Implement BLS12-381 addition matching C# implementation
        try
        {
            if (a.GetType() != b.GetType())
                throw std::invalid_argument("Cannot add BLS12-381 points of different types");

            switch (a.GetType())
            {
                case BLS12381Point::Type::G1Affine:
                {
                    auto result = a.AsG1Affine() + b.AsG1Affine();
                    return BLS12381Point(result);
                }
                case BLS12381Point::Type::G1Projective:
                {
                    auto result = a.AsG1Projective() + b.AsG1Projective();
                    return BLS12381Point(result);
                }
                case BLS12381Point::Type::G2Affine:
                {
                    auto result = a.AsG2Affine() + b.AsG2Affine();
                    return BLS12381Point(result);
                }
                case BLS12381Point::Type::G2Projective:
                {
                    auto result = a.AsG2Projective() + b.AsG2Projective();
                    return BLS12381Point(result);
                }
                case BLS12381Point::Type::Gt:
                {
                    auto result = a.AsGt() * b.AsGt(); // Gt uses multiplication for group operation
                    return BLS12381Point(result);
                }
                default:
                    throw std::invalid_argument("Invalid BLS12-381 point type for addition");
            }
        }
        catch (const std::exception& e)
        {
            throw std::runtime_error("BLS12-381 addition failed: " + std::string(e.what()));
        }
    }
}

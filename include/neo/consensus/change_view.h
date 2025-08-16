#pragma once

#include <neo/io/serializable.h>
#include <neo/io/binary_reader.h>
#include <neo/io/binary_writer.h>
#include <cstdint>

namespace neo {
namespace consensus {

/**
 * @brief Change view consensus message
 */
class ChangeView : public io::ISerializable {
public:
    /**
     * @brief View number to change to
     */
    uint8_t NewViewNumber = 0;
    
    /**
     * @brief Timestamp of the change view request
     */
    uint64_t Timestamp = 0;
    
    /**
     * @brief Reason for the view change
     */
    enum class Reason : uint8_t {
        Timeout = 0x00,
        InvalidBlock = 0x01,
        InvalidTransaction = 0x02,
        ConsensusPayloadTimeout = 0x03,
        Other = 0xFF
    };
    
    Reason ChangeReason = Reason::Timeout;
    
    /**
     * @brief Default constructor
     */
    ChangeView() = default;
    
    /**
     * @brief Constructor with view number
     */
    explicit ChangeView(uint8_t newView) : NewViewNumber(newView) {
        Timestamp = std::chrono::system_clock::now().time_since_epoch().count();
    }
    
    /**
     * @brief Get the size of the serialized data
     */
    size_t Size() const override {
        return sizeof(NewViewNumber) + sizeof(Timestamp) + sizeof(ChangeReason);
    }
    
    /**
     * @brief Serialize to binary writer
     */
    void Serialize(io::BinaryWriter& writer) const override {
        writer.Write(NewViewNumber);
        writer.Write(Timestamp);
        writer.Write(static_cast<uint8_t>(ChangeReason));
    }
    
    /**
     * @brief Deserialize from binary reader
     */
    void Deserialize(io::BinaryReader& reader) override {
        NewViewNumber = reader.ReadByte();
        Timestamp = reader.ReadUInt64();
        ChangeReason = static_cast<Reason>(reader.ReadByte());
    }
    
    /**
     * @brief Check if the change view is valid
     */
    bool IsValid() const {
        // View number should be reasonable
        return NewViewNumber < 100;
    }
    
    /**
     * @brief Compare two change view messages
     */
    bool operator==(const ChangeView& other) const {
        return NewViewNumber == other.NewViewNumber &&
               Timestamp == other.Timestamp &&
               ChangeReason == other.ChangeReason;
    }
};

} // namespace consensus
} // namespace neo
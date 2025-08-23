#pragma once

#include <memory>
#include <string>
#include <unordered_set>

#include "neo/network/p2p/message.h"
#include "neo/core/protocol_settings.h"
#include "neo/io/byte_vector.h"

namespace neo::network::p2p {

/**
 * @brief Message validation result
 */
enum class ValidationResult {
    Valid,
    InvalidMagic,
    InvalidCommand,
    InvalidChecksum,
    InvalidSize,
    InvalidPayload,
    UnsupportedVersion,
    NetworkMismatch
};

/**
 * @brief P2P message validator for protocol compliance
 * 
 * Validates incoming and outgoing P2P messages against Neo N3 protocol specifications:
 * - Network magic number verification
 * - Message size limits
 * - Checksum validation
 * - Command validation
 * - Payload structure validation
 */
class MessageValidator {
public:
    /**
     * @brief Constructor
     * @param protocol_settings Protocol configuration
     */
    explicit MessageValidator(std::shared_ptr<core::ProtocolSettings> protocol_settings);
    
    /**
     * @brief Validate incoming message
     * @param message_data Raw message data
     * @param source_address Source peer address
     * @return Validation result
     */
    ValidationResult ValidateIncoming(const io::ByteVector& message_data, const std::string& source_address);
    
    /**
     * @brief Validate outgoing message
     * @param message Message to validate
     * @param destination_address Destination peer address
     * @return Validation result
     */
    ValidationResult ValidateOutgoing(const Message& message, const std::string& destination_address);
    
    /**
     * @brief Validate message header
     * @param header_data Message header bytes
     * @return Validation result
     */
    ValidationResult ValidateHeader(const io::ByteVector& header_data);
    
    /**
     * @brief Validate message payload
     * @param command Message command
     * @param payload_data Payload bytes
     * @return Validation result
     */
    ValidationResult ValidatePayload(const std::string& command, const io::ByteVector& payload_data);
    
    /**
     * @brief Check if command is supported
     * @param command Message command
     * @return true if supported
     */
    bool IsCommandSupported(const std::string& command) const;
    
    /**
     * @brief Get maximum message size
     * @return Maximum allowed message size in bytes
     */
    uint32_t GetMaxMessageSize() const { return max_message_size_; }
    
    /**
     * @brief Get validation statistics
     * @return Statistics object
     */
    struct ValidationStats {
        uint64_t messages_validated = 0;
        uint64_t valid_messages = 0;
        uint64_t invalid_magic = 0;
        uint64_t invalid_checksum = 0;
        uint64_t invalid_size = 0;
        uint64_t invalid_payload = 0;
        uint64_t unsupported_version = 0;
    };
    
    ValidationStats GetStats() const { return stats_; }
    
    /**
     * @brief Reset validation statistics
     */
    void ResetStats();

private:
    /**
     * @brief Validate network magic number
     * @param magic Magic number from message
     * @return true if valid
     */
    bool ValidateNetworkMagic(uint32_t magic) const;
    
    /**
     * @brief Validate message checksum
     * @param payload_data Payload data
     * @param expected_checksum Expected checksum
     * @return true if valid
     */
    bool ValidateChecksum(const io::ByteVector& payload_data, uint32_t expected_checksum) const;
    
    /**
     * @brief Validate message size limits
     * @param size Message size
     * @param command Message command
     * @return true if valid
     */
    bool ValidateMessageSize(uint32_t size, const std::string& command) const;
    
    /**
     * @brief Validate version payload
     * @param payload_data Payload data
     * @return Validation result
     */
    ValidationResult ValidateVersionPayload(const io::ByteVector& payload_data);
    
    /**
     * @brief Validate inventory payload
     * @param payload_data Payload data
     * @return Validation result
     */
    ValidationResult ValidateInventoryPayload(const io::ByteVector& payload_data);
    
    /**
     * @brief Validate block payload
     * @param payload_data Payload data
     * @return Validation result
     */
    ValidationResult ValidateBlockPayload(const io::ByteVector& payload_data);
    
    /**
     * @brief Validate transaction payload
     * @param payload_data Payload data
     * @return Validation result
     */
    ValidationResult ValidateTransactionPayload(const io::ByteVector& payload_data);

private:
    std::shared_ptr<core::ProtocolSettings> protocol_settings_;
    
    // Validation configuration
    uint32_t max_message_size_;
    uint32_t max_inventory_count_;
    uint32_t max_block_size_;
    uint32_t max_transaction_size_;
    
    // Supported commands
    std::unordered_set<std::string> supported_commands_;
    
    // Command-specific size limits
    std::unordered_map<std::string, uint32_t> command_size_limits_;
    
    // Statistics
    mutable ValidationStats stats_;
    
    // Network validation
    uint32_t expected_network_magic_;
    std::unordered_set<uint32_t> allowed_protocol_versions_;
};

/**
 * @brief Get validation result description
 * @param result Validation result
 * @return Human-readable description
 */
std::string GetValidationResultDescription(ValidationResult result);

/**
 * @brief Message validation exception
 */
class MessageValidationException : public std::exception {
public:
    MessageValidationException(ValidationResult result, const std::string& details = "")
        : result_(result), details_(details) {
        message_ = GetValidationResultDescription(result);
        if (!details.empty()) {
            message_ += ": " + details;
        }
    }
    
    const char* what() const noexcept override {
        return message_.c_str();
    }
    
    ValidationResult GetResult() const { return result_; }
    const std::string& GetDetails() const { return details_; }

private:
    ValidationResult result_;
    std::string details_;
    std::string message_;
};

} // namespace neo::network::p2p
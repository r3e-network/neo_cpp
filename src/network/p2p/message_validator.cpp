#include "neo/network/p2p/message_validator.h"
#include "neo/core/common_logging.h"
#include "neo/cryptography/hash.h"

namespace neo::network::p2p {

MessageValidator::MessageValidator(std::shared_ptr<core::ProtocolSettings> protocol_settings)
    : protocol_settings_(protocol_settings),
      max_message_size_(1024 * 1024 * 2),  // 2MB default
      max_inventory_count_(65536),
      max_block_size_(1024 * 1024),        // 1MB
      max_transaction_size_(1024 * 64),    // 64KB
      expected_network_magic_(protocol_settings->GetMagic()) {
    
    // Initialize supported commands (Neo N3 protocol)
    supported_commands_ = {
        "version", "verack", "getaddr", "addr", "ping", "pong",
        "getheaders", "headers", "getblocks", "inv", "getdata",
        "block", "tx", "consensus", "reject", "filterload",
        "filteradd", "filterclear", "merkleblock", "alert",
        "mempool", "notfound", "getblocktxn", "blocktxn",
        "getcmpctblock", "cmpctblock"
    };
    
    // Command-specific size limits
    command_size_limits_ = {
        {"version", 1024},
        {"verack", 0},
        {"getaddr", 0},
        {"addr", 8192},      // Up to 1000 addresses
        {"ping", 4},
        {"pong", 4},
        {"getheaders", 8192},
        {"headers", max_block_size_ * 2},  // Multiple headers
        {"getblocks", 8192},
        {"inv", 65536},      // Inventory items
        {"getdata", 65536},  // Data requests
        {"block", max_block_size_},
        {"tx", max_transaction_size_},
        {"consensus", 1024 * 16},  // 16KB for consensus data
        {"reject", 1024},
        {"mempool", 0},
        {"notfound", 65536}
    };
    
    // Allowed protocol versions (Neo N3)
    allowed_protocol_versions_ = {0, 1, 2, 3};  // Neo N3 versions
    
    core::CommonLogging::Info("Message validator initialized for network magic: " + 
                            std::to_string(expected_network_magic_));
}

ValidationResult MessageValidator::ValidateIncoming(const io::ByteVector& message_data, 
                                                  const std::string& source_address) {
    stats_.messages_validated++;
    
    try {
        // Minimum header size check (4 + 12 + 4 + 4 = 24 bytes)
        if (message_data.size() < 24) {
            stats_.invalid_size++;
            return ValidationResult::InvalidSize;
        }
        
        // Validate header
        ValidationResult header_result = ValidateHeader(message_data);
        if (header_result != ValidationResult::Valid) {
            return header_result;
        }
        
        // Extract header components
        uint32_t magic = *reinterpret_cast<const uint32_t*>(message_data.data());
        std::string command(reinterpret_cast<const char*>(message_data.data() + 4), 12);
        command = command.substr(0, command.find('\0'));  // Remove null padding
        uint32_t payload_size = *reinterpret_cast<const uint32_t*>(message_data.data() + 16);
        uint32_t checksum = *reinterpret_cast<const uint32_t*>(message_data.data() + 20);
        
        // Validate network magic
        if (!ValidateNetworkMagic(magic)) {
            stats_.invalid_magic++;
            return ValidationResult::InvalidMagic;
        }
        
        // Validate command
        if (!IsCommandSupported(command)) {
            return ValidationResult::InvalidCommand;
        }
        
        // Validate message size
        if (!ValidateMessageSize(payload_size, command)) {
            stats_.invalid_size++;
            return ValidationResult::InvalidSize;
        }
        
        // Validate total message size
        if (message_data.size() != 24 + payload_size) {
            stats_.invalid_size++;
            return ValidationResult::InvalidSize;
        }
        
        // Extract and validate payload
        io::ByteVector payload_data(message_data.begin() + 24, message_data.end());
        
        // Validate checksum
        if (!ValidateChecksum(payload_data, checksum)) {
            stats_.invalid_checksum++;
            return ValidationResult::InvalidChecksum;
        }
        
        // Validate payload structure
        ValidationResult payload_result = ValidatePayload(command, payload_data);
        if (payload_result != ValidationResult::Valid) {
            stats_.invalid_payload++;
            return payload_result;
        }
        
        stats_.valid_messages++;
        return ValidationResult::Valid;
        
    } catch (const std::exception& e) {
        core::CommonLogging::Error("Message validation exception: " + std::string(e.what()));
        return ValidationResult::InvalidPayload;
    }
}

ValidationResult MessageValidator::ValidateOutgoing(const Message& message, 
                                                  const std::string& destination_address) {
    stats_.messages_validated++;
    
    try {
        // Validate command
        if (!IsCommandSupported(message.GetCommand())) {
            return ValidationResult::InvalidCommand;
        }
        
        // Validate network magic
        if (!ValidateNetworkMagic(message.GetMagic())) {
            stats_.invalid_magic++;
            return ValidationResult::InvalidMagic;
        }
        
        // Validate payload size
        uint32_t payload_size = message.GetPayloadSize();
        if (!ValidateMessageSize(payload_size, message.GetCommand())) {
            stats_.invalid_size++;
            return ValidationResult::InvalidSize;
        }
        
        // Validate payload structure
        io::ByteVector payload_data = message.GetPayloadData();
        ValidationResult payload_result = ValidatePayload(message.GetCommand(), payload_data);
        if (payload_result != ValidationResult::Valid) {
            stats_.invalid_payload++;
            return payload_result;
        }
        
        stats_.valid_messages++;
        return ValidationResult::Valid;
        
    } catch (const std::exception& e) {
        core::CommonLogging::Error("Outgoing message validation exception: " + std::string(e.what()));
        return ValidationResult::InvalidPayload;
    }
}

ValidationResult MessageValidator::ValidateHeader(const io::ByteVector& header_data) {
    if (header_data.size() < 24) {
        return ValidationResult::InvalidSize;
    }
    
    // Extract and validate magic number
    uint32_t magic = *reinterpret_cast<const uint32_t*>(header_data.data());
    if (!ValidateNetworkMagic(magic)) {
        return ValidationResult::InvalidMagic;
    }
    
    // Extract and validate command
    std::string command(reinterpret_cast<const char*>(header_data.data() + 4), 12);
    command = command.substr(0, command.find('\0'));
    
    if (!IsCommandSupported(command)) {
        return ValidationResult::InvalidCommand;
    }
    
    // Validate payload size
    uint32_t payload_size = *reinterpret_cast<const uint32_t*>(header_data.data() + 16);
    if (!ValidateMessageSize(payload_size, command)) {
        return ValidationResult::InvalidSize;
    }
    
    return ValidationResult::Valid;
}

ValidationResult MessageValidator::ValidatePayload(const std::string& command, 
                                                 const io::ByteVector& payload_data) {
    try {
        if (command == "version") {
            return ValidateVersionPayload(payload_data);
        } else if (command == "inv" || command == "getdata") {
            return ValidateInventoryPayload(payload_data);
        } else if (command == "block") {
            return ValidateBlockPayload(payload_data);
        } else if (command == "tx") {
            return ValidateTransactionPayload(payload_data);
        } else if (command == "verack" || command == "getaddr" || command == "mempool") {
            // These commands should have empty payloads
            return payload_data.empty() ? ValidationResult::Valid : ValidationResult::InvalidPayload;
        } else if (command == "ping" || command == "pong") {
            // Should contain 4-byte nonce
            return payload_data.size() == 4 ? ValidationResult::Valid : ValidationResult::InvalidPayload;
        }
        
        // For other commands, basic size validation is sufficient
        return ValidationResult::Valid;
        
    } catch (const std::exception& e) {
        core::CommonLogging::Error("Payload validation error for " + command + ": " + e.what());
        return ValidationResult::InvalidPayload;
    }
}

bool MessageValidator::IsCommandSupported(const std::string& command) const {
    return supported_commands_.find(command) != supported_commands_.end();
}

void MessageValidator::ResetStats() {
    stats_ = ValidationStats{};
}

bool MessageValidator::ValidateNetworkMagic(uint32_t magic) const {
    return magic == expected_network_magic_;
}

bool MessageValidator::ValidateChecksum(const io::ByteVector& payload_data, uint32_t expected_checksum) const {
    try {
        // Calculate SHA256 hash of payload
        io::ByteVector hash = cryptography::Hash::Sha256(payload_data);
        
        // Take first 4 bytes as checksum
        uint32_t calculated_checksum = *reinterpret_cast<const uint32_t*>(hash.data());
        
        return calculated_checksum == expected_checksum;
        
    } catch (const std::exception& e) {
        core::CommonLogging::Error("Checksum validation error: " + std::string(e.what()));
        return false;
    }
}

bool MessageValidator::ValidateMessageSize(uint32_t size, const std::string& command) const {
    // Check against global limit
    if (size > max_message_size_) {
        return false;
    }
    
    // Check against command-specific limit
    auto it = command_size_limits_.find(command);
    if (it != command_size_limits_.end() && size > it->second) {
        return false;
    }
    
    return true;
}

ValidationResult MessageValidator::ValidateVersionPayload(const io::ByteVector& payload_data) {
    // Version payload should contain version info
    if (payload_data.size() < 32) {  // Minimum size for version message
        return ValidationResult::InvalidPayload;
    }
    
    // Extract protocol version (first 4 bytes)
    uint32_t protocol_version = *reinterpret_cast<const uint32_t*>(payload_data.data());
    
    if (allowed_protocol_versions_.find(protocol_version) == allowed_protocol_versions_.end()) {
        return ValidationResult::UnsupportedVersion;
    }
    
    return ValidationResult::Valid;
}

ValidationResult MessageValidator::ValidateInventoryPayload(const io::ByteVector& payload_data) {
    if (payload_data.empty()) {
        return ValidationResult::InvalidPayload;
    }
    
    // First byte should be inventory count
    if (payload_data.size() < 1) {
        return ValidationResult::InvalidPayload;
    }
    
    uint8_t count = payload_data[0];
    
    // Validate count against limit
    if (count > max_inventory_count_) {
        return ValidationResult::InvalidPayload;
    }
    
    // Each inventory item is 33 bytes (1 byte type + 32 bytes hash)
    size_t expected_size = 1 + (count * 33);
    if (payload_data.size() != expected_size) {
        return ValidationResult::InvalidPayload;
    }
    
    return ValidationResult::Valid;
}

ValidationResult MessageValidator::ValidateBlockPayload(const io::ByteVector& payload_data) {
    if (payload_data.empty()) {
        return ValidationResult::InvalidPayload;
    }
    
    // Basic size check
    if (payload_data.size() > max_block_size_) {
        return ValidationResult::InvalidPayload;
    }
    
    // Block should start with version field (4 bytes)
    if (payload_data.size() < 4) {
        return ValidationResult::InvalidPayload;
    }
    
    return ValidationResult::Valid;
}

ValidationResult MessageValidator::ValidateTransactionPayload(const io::ByteVector& payload_data) {
    if (payload_data.empty()) {
        return ValidationResult::InvalidPayload;
    }
    
    // Basic size check
    if (payload_data.size() > max_transaction_size_) {
        return ValidationResult::InvalidPayload;
    }
    
    // Transaction should start with version field (1 byte)
    if (payload_data.size() < 1) {
        return ValidationResult::InvalidPayload;
    }
    
    return ValidationResult::Valid;
}

std::string GetValidationResultDescription(ValidationResult result) {
    switch (result) {
        case ValidationResult::Valid:
            return "Valid";
        case ValidationResult::InvalidMagic:
            return "Invalid network magic number";
        case ValidationResult::InvalidCommand:
            return "Invalid or unsupported command";
        case ValidationResult::InvalidChecksum:
            return "Invalid message checksum";
        case ValidationResult::InvalidSize:
            return "Invalid message size";
        case ValidationResult::InvalidPayload:
            return "Invalid message payload";
        case ValidationResult::UnsupportedVersion:
            return "Unsupported protocol version";
        case ValidationResult::NetworkMismatch:
            return "Network mismatch";
        default:
            return "Unknown validation error";
    }
}

} // namespace neo::network::p2p
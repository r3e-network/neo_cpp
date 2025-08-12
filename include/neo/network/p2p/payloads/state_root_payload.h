#pragma once

#include <neo/io/iserializable.h>
#include <neo/io/uint256.h>
#include <neo/ledger/state_root.h>

#include <cstdint>
#include <vector>

namespace neo::network::p2p::payloads
{
/**
 * @brief State root payload for network transmission
 *
 * This payload is used to transmit state roots between nodes for
 * state synchronization. It follows the C# Neo protocol format.
 */
class StateRootPayload : public io::ISerializable
{
   private:
    ledger::StateRoot state_root_;
    uint32_t request_id_;  // Request identifier for correlation
    bool is_response_;     // True if this is a response to a request

   public:
    /**
     * @brief Default constructor
     */
    StateRootPayload() : request_id_(0), is_response_(false) {}

    /**
     * @brief Constructor with state root
     * @param state_root State root to transmit
     * @param request_id Request identifier
     * @param is_response Whether this is a response
     */
    StateRootPayload(const ledger::StateRoot& state_root, uint32_t request_id = 0, bool is_response = false)
        : state_root_(state_root), request_id_(request_id), is_response_(is_response)
    {
    }

    /**
     * @brief Get the state root
     * @return State root
     */
    const ledger::StateRoot& GetStateRoot() const { return state_root_; }

    /**
     * @brief Set the state root
     * @param state_root State root to set
     */
    void SetStateRoot(const ledger::StateRoot& state_root) { state_root_ = state_root; }

    /**
     * @brief Get request identifier
     * @return Request ID
     */
    uint32_t GetRequestId() const { return request_id_; }

    /**
     * @brief Set request identifier
     * @param request_id Request ID
     */
    void SetRequestId(uint32_t request_id) { request_id_ = request_id; }

    /**
     * @brief Check if this is a response
     * @return True if response
     */
    bool IsResponse() const { return is_response_; }

    /**
     * @brief Set response flag
     * @param is_response Response flag
     */
    void SetIsResponse(bool is_response) { is_response_ = is_response; }

    /**
     * @brief Get the size of serialized data
     * @return Size in bytes
     */
    size_t GetSize() const;

    /**
     * @brief Serialize to writer
     * @param writer Binary writer
     */
    void Serialize(io::BinaryWriter& writer) const override;

    /**
     * @brief Deserialize from reader
     * @param reader Binary reader
     */
    void Deserialize(io::BinaryReader& reader) override;
};

/**
 * @brief Request for state roots at specific heights
 */
class GetStateRootsPayload : public io::ISerializable
{
   private:
    uint32_t request_id_;
    uint32_t start_height_;
    uint32_t count_;

   public:
    /**
     * @brief Default constructor
     */
    GetStateRootsPayload() : request_id_(0), start_height_(0), count_(0) {}

    /**
     * @brief Constructor with parameters
     * @param request_id Request identifier
     * @param start_height Starting block height
     * @param count Number of state roots to request
     */
    GetStateRootsPayload(uint32_t request_id, uint32_t start_height, uint32_t count)
        : request_id_(request_id), start_height_(start_height), count_(count)
    {
    }

    uint32_t GetRequestId() const { return request_id_; }
    void SetRequestId(uint32_t id) { request_id_ = id; }

    uint32_t GetStartHeight() const { return start_height_; }
    void SetStartHeight(uint32_t height) { start_height_ = height; }

    uint32_t GetCount() const { return count_; }
    void SetCount(uint32_t count) { count_ = count; }

    size_t GetSize() const;
    void Serialize(io::BinaryWriter& writer) const override;
    void Deserialize(io::BinaryReader& reader) override;
};

}  // namespace neo::network::p2p::payloads
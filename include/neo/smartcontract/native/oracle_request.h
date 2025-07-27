#pragma once

#include <memory>
#include <neo/io/byte_vector.h>
#include <neo/io/uint160.h>
#include <neo/io/uint256.h>
#include <neo/vm/stack_item.h>
#include <string>

namespace neo::smartcontract::native
{
/**
 * @brief Represents an Oracle request in smart contracts.
 */
class OracleRequest
{
  public:
    /**
     * @brief Constructs an OracleRequest.
     */
    OracleRequest();

    /**
     * @brief Constructs an OracleRequest with the specified parameters.
     * @param originalTxid The original transaction ID.
     * @param gasForResponse The gas for response.
     * @param url The URL.
     * @param filter The filter.
     * @param callbackContract The callback contract.
     * @param callbackMethod The callback method.
     * @param userData The user data.
     */
    OracleRequest(const io::UInt256& originalTxid, int64_t gasForResponse, const std::string& url,
                  const std::string& filter, const io::UInt160& callbackContract, const std::string& callbackMethod,
                  const io::ByteVector& userData);

    /**
     * @brief Gets the original transaction ID.
     * @return The original transaction ID.
     */
    const io::UInt256& GetOriginalTxid() const;

    /**
     * @brief Sets the original transaction ID.
     * @param originalTxid The original transaction ID.
     */
    void SetOriginalTxid(const io::UInt256& originalTxid);

    /**
     * @brief Gets the gas for response.
     * @return The gas for response.
     */
    int64_t GetGasForResponse() const;

    /**
     * @brief Sets the gas for response.
     * @param gasForResponse The gas for response.
     */
    void SetGasForResponse(int64_t gasForResponse);

    /**
     * @brief Gets the URL.
     * @return The URL.
     */
    const std::string& GetUrl() const;

    /**
     * @brief Sets the URL.
     * @param url The URL.
     */
    void SetUrl(const std::string& url);

    /**
     * @brief Gets the filter.
     * @return The filter.
     */
    const std::string& GetFilter() const;

    /**
     * @brief Sets the filter.
     * @param filter The filter.
     */
    void SetFilter(const std::string& filter);

    /**
     * @brief Gets the callback contract.
     * @return The callback contract.
     */
    const io::UInt160& GetCallbackContract() const;

    /**
     * @brief Sets the callback contract.
     * @param callbackContract The callback contract.
     */
    void SetCallbackContract(const io::UInt160& callbackContract);

    /**
     * @brief Gets the callback method.
     * @return The callback method.
     */
    const std::string& GetCallbackMethod() const;

    /**
     * @brief Sets the callback method.
     * @param callbackMethod The callback method.
     */
    void SetCallbackMethod(const std::string& callbackMethod);

    /**
     * @brief Gets the user data.
     * @return The user data.
     */
    const io::ByteVector& GetUserData() const;

    /**
     * @brief Sets the user data.
     * @param userData The user data.
     */
    void SetUserData(const io::ByteVector& userData);

    /**
     * @brief Converts the request to a stack item.
     * @return The stack item.
     */
    std::shared_ptr<vm::StackItem> ToStackItem() const;

    /**
     * @brief Initializes the request from a stack item.
     * @param item The stack item.
     */
    void FromStackItem(const std::shared_ptr<vm::StackItem>& item);

    /**
     * @brief Serializes the request to a binary writer.
     * @param writer The binary writer.
     */
    void Serialize(io::BinaryWriter& writer) const;

    /**
     * @brief Deserializes the request from a binary reader.
     * @param reader The binary reader.
     */
    void Deserialize(io::BinaryReader& reader);

  private:
    io::UInt256 originalTxid_;
    int64_t gasForResponse_;
    std::string url_;
    std::string filter_;
    io::UInt160 callbackContract_;
    std::string callbackMethod_;
    io::ByteVector userData_;
};
}  // namespace neo::smartcontract::native

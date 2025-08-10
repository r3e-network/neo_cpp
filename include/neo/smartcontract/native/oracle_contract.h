#pragma once

#include <neo/cryptography/ecc/ecpoint.h>
#include <neo/io/uint160.h>
#include <neo/io/uint256.h>
#include <neo/persistence/store_view.h>
#include <neo/smartcontract/native/id_list.h>
#include <neo/smartcontract/native/native_contract.h>
#include <neo/smartcontract/native/oracle_request.h>
#include <neo/vm/stack_item.h>

#include <memory>
#include <string>
#include <tuple>
#include <vector>

namespace neo::smartcontract::native
{
/**
 * @brief Represents the oracle native contract.
 */
class OracleContract : public NativeContract
{
   public:
    /**
     * @brief The contract ID.
     */
    static constexpr int32_t ID = -9;

    /**
     * @brief The contract name.
     */
    static constexpr const char* NAME = "OracleContract";

    /**
     * @brief The maximum URL length.
     */
    static constexpr int MAX_URL_LENGTH = 256;

    /**
     * @brief The maximum filter length.
     */
    static constexpr int MAX_FILTER_LENGTH = 128;

    /**
     * @brief The maximum callback length.
     */
    static constexpr int MAX_CALLBACK_LENGTH = 32;

    /**
     * @brief The maximum user data length.
     */
    static constexpr int MAX_USER_DATA_LENGTH = 512;

    /**
     * @brief The storage prefix for requests.
     */
    static constexpr uint8_t PREFIX_REQUEST = 7;

    /**
     * @brief The storage prefix for request ID.
     */
    static constexpr uint8_t PREFIX_REQUEST_ID = 9;

    /**
     * @brief The storage prefix for ID list.
     */
    static constexpr uint8_t PREFIX_ID_LIST = 6;

    /**
     * @brief The storage prefix for price.
     */
    static constexpr uint8_t PREFIX_PRICE = 5;

    /**
     * @brief The storage prefix for oracles.
     */
    static constexpr uint8_t PREFIX_ORACLE = 8;

    /**
     * @brief The storage prefix for responses.
     */
    static constexpr uint8_t PREFIX_RESPONSE = 10;

    /**
     * @brief The default price.
     */
    static constexpr int64_t DEFAULT_PRICE = 1000000;

    /**
     * @brief Constructs an OracleContract.
     */
    OracleContract();

    /**
     * @brief Gets the instance.
     * @return The instance.
     */
    static std::shared_ptr<OracleContract> GetInstance();

    /**
     * @brief Gets the price.
     * @param snapshot The snapshot.
     * @return The price.
     */
    int64_t GetPrice(std::shared_ptr<persistence::StoreView> snapshot) const;

    /**
     * @brief Sets the price.
     * @param snapshot The snapshot.
     * @param price The price.
     */
    void SetPrice(std::shared_ptr<persistence::StoreView> snapshot, int64_t price);

    /**
     * @brief Gets the oracles.
     * @param snapshot The snapshot.
     * @return The oracles.
     */
    std::vector<io::UInt160> GetOracles(std::shared_ptr<persistence::StoreView> snapshot) const;

    /**
     * @brief Gets a request by ID.
     * @param snapshot The snapshot.
     * @param id The request ID.
     * @return The request.
     */
    OracleRequest GetRequest(std::shared_ptr<persistence::StoreView> snapshot, uint64_t id) const;

    /**
     * @brief Gets the ID list for a URL.
     * @param snapshot The snapshot.
     * @param urlHash The URL hash.
     * @return The ID list.
     */
    IdList GetIdList(std::shared_ptr<persistence::StoreView> snapshot, const io::ByteVector& urlHash) const;

    /**
     * @brief Gets the URL hash.
     * @param url The URL.
     * @return The URL hash.
     */
    static io::UInt256 GetUrlHash(const std::string& url);

    /**
     * @brief Initializes the contract.
     * @param engine The engine.
     * @param hardfork The hardfork version.
     * @return True if successful, false otherwise.
     */
    bool InitializeContract(ApplicationEngine& engine, uint32_t hardfork);

    /**
     * @brief Handles the OnPersist event.
     * @param engine The engine.
     * @return True if successful, false otherwise.
     */
    bool OnPersist(ApplicationEngine& engine);

    /**
     * @brief Handles the PostPersist event.
     * @param engine The engine.
     * @return True if successful, false otherwise.
     */
    bool PostPersist(ApplicationEngine& engine);

    /**
     * @brief Sets the oracles.
     * @param snapshot The snapshot.
     * @param oracles The oracles.
     */
    void SetOracles(std::shared_ptr<persistence::StoreView> snapshot, const std::vector<io::UInt160>& oracles);

    /**
     * @brief Gets all requests.
     * @param snapshot The snapshot.
     * @return The requests.
     */
    std::vector<std::pair<uint64_t, OracleRequest>> GetRequests(std::shared_ptr<persistence::StoreView> snapshot) const;

    /**
     * @brief Gets requests by URL.
     * @param snapshot The snapshot.
     * @param url The URL.
     * @return The requests.
     */
    std::vector<std::pair<uint64_t, OracleRequest>> GetRequestsByUrl(std::shared_ptr<persistence::StoreView> snapshot,
                                                                     const std::string& url) const;

    /**
     * @brief Gets a response.
     * @param snapshot The snapshot.
     * @param id The ID.
     * @return The response (code, result).
     */
    std::tuple<uint8_t, std::string> GetResponse(std::shared_ptr<persistence::StoreView> snapshot, uint64_t id) const;

    /**
     * @brief Creates a request.
     * @param snapshot The snapshot.
     * @param url The URL.
     * @param filter The filter.
     * @param callback The callback contract.
     * @param callbackMethod The callback method.
     * @param gasForResponse The gas for response.
     * @param userData The user data.
     * @param originalTxid The original transaction ID.
     * @return The request ID.
     */
    uint64_t CreateRequest(std::shared_ptr<persistence::StoreView> snapshot, const std::string& url,
                           const std::string& filter, const io::UInt160& callback, const std::string& callbackMethod,
                           int64_t gasForResponse, const io::ByteVector& userData, const io::UInt256& originalTxid);

   protected:
    /**
     * @brief Initializes the contract.
     */
    void Initialize() override;

   private:
    /**
     * @brief Handles the getPrice method.
     * @param engine The engine.
     * @param args The arguments.
     * @return The result.
     */
    std::shared_ptr<vm::StackItem> OnGetPrice(ApplicationEngine& engine,
                                              const std::vector<std::shared_ptr<vm::StackItem>>& args);

    /**
     * @brief Handles the setPrice method.
     * @param engine The engine.
     * @param args The arguments.
     * @return The result.
     */
    std::shared_ptr<vm::StackItem> OnSetPrice(ApplicationEngine& engine,
                                              const std::vector<std::shared_ptr<vm::StackItem>>& args);

    /**
     * @brief Handles the getOracles method.
     * @param engine The engine.
     * @param args The arguments.
     * @return The result.
     */
    std::shared_ptr<vm::StackItem> OnGetOracles(ApplicationEngine& engine,
                                                const std::vector<std::shared_ptr<vm::StackItem>>& args);

    /**
     * @brief Handles the setOracles method.
     * @param engine The engine.
     * @param args The arguments.
     * @return The result.
     */
    std::shared_ptr<vm::StackItem> OnSetOracles(ApplicationEngine& engine,
                                                const std::vector<std::shared_ptr<vm::StackItem>>& args);

    /**
     * @brief Handles the request method.
     * @param engine The engine.
     * @param args The arguments.
     * @return The result.
     */
    std::shared_ptr<vm::StackItem> OnRequest(ApplicationEngine& engine,
                                             const std::vector<std::shared_ptr<vm::StackItem>>& args);

    /**
     * @brief Handles the finish method.
     * @param engine The engine.
     * @param args The arguments.
     * @return The result.
     */
    std::shared_ptr<vm::StackItem> OnFinish(ApplicationEngine& engine,
                                            const std::vector<std::shared_ptr<vm::StackItem>>& args);

    /**
     * @brief Handles the verify method.
     * @param engine The engine.
     * @param args The arguments.
     * @return The result.
     */
    std::shared_ptr<vm::StackItem> OnVerify(ApplicationEngine& engine,
                                            const std::vector<std::shared_ptr<vm::StackItem>>& args);

    /**
     * @brief Gets the next request ID.
     * @param snapshot The snapshot.
     * @return The next request ID.
     */
    uint64_t GetNextRequestId(std::shared_ptr<persistence::StoreView> snapshot) const;

    /**
     * @brief Adds a request to the ID list.
     * @param snapshot The snapshot.
     * @param id The ID.
     */
    void AddRequestToIdList(std::shared_ptr<persistence::StoreView> snapshot, uint64_t id);

    /**
     * @brief Removes a request from the ID list.
     * @param snapshot The snapshot.
     * @param id The ID.
     */
    void RemoveRequestFromIdList(std::shared_ptr<persistence::StoreView> snapshot, uint64_t id);

    /**
     * @brief Gets the ID list.
     * @param snapshot The snapshot.
     * @param urlHash The URL hash.
     * @return The ID list.
     */
    IdList GetIdList(std::shared_ptr<persistence::StoreView> snapshot, const io::UInt256& urlHash) const;

    /**
     * @brief Checks if the caller is a committee member.
     * @param engine The engine.
     * @return True if the caller is a committee member, false otherwise.
     */
    bool CheckCommittee(ApplicationEngine& engine) const;

    /**
     * @brief Checks if the caller is an oracle node.
     * @param engine The engine.
     * @return True if the caller is an oracle node, false otherwise.
     */
    bool CheckOracleNode(ApplicationEngine& engine) const;

    /**
     * @brief Gets the original transaction ID.
     * @param engine The engine.
     * @return The original transaction ID.
     */
    io::UInt256 GetOriginalTxid(ApplicationEngine& engine) const;
};
}  // namespace neo::smartcontract::native

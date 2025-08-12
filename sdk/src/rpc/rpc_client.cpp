#include <neo/sdk/rpc/rpc_client.h>
#include <neo/logging/logger.h>
#include <curl/curl.h>
#include <sstream>
#include <atomic>

namespace neo::sdk::rpc {

// Callback for CURL to write response data
static size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* response) {
    size_t totalSize = size * nmemb;
    response->append((char*)contents, totalSize);
    return totalSize;
}

class RpcClient::Impl {
public:
    std::string endpoint;
    uint32_t timeout = 30000;  // 30 seconds default
    std::atomic<uint64_t> requestId{1};
    
    Impl(const std::string& ep) : endpoint(ep) {
        // Initialize CURL globally (once per application)
        static bool curlInitialized = false;
        if (!curlInitialized) {
            curl_global_init(CURL_GLOBAL_DEFAULT);
            curlInitialized = true;
        }
    }
    
    json MakeRequest(const std::string& method, const std::vector<json>& params = {}) {
        // Build JSON-RPC request
        json request;
        request["jsonrpc"] = "2.0";
        request["method"] = method;
        request["params"] = params;
        request["id"] = requestId.fetch_add(1);
        
        std::string requestStr = request.dump();
        
        NEO_LOG_DEBUG("RPC Request: {}", requestStr);
        
        // Setup CURL
        CURL* curl = curl_easy_init();
        if (!curl) {
            throw std::runtime_error("Failed to initialize CURL");
        }
        
        std::string response;
        struct curl_slist* headers = nullptr;
        headers = curl_slist_append(headers, "Content-Type: application/json");
        
        curl_easy_setopt(curl, CURLOPT_URL, endpoint.c_str());
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, requestStr.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, timeout);
        
        // Perform request
        CURLcode res = curl_easy_perform(curl);
        
        // Cleanup
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
        
        if (res != CURLE_OK) {
            throw std::runtime_error("CURL request failed: " + std::string(curl_easy_strerror(res)));
        }
        
        NEO_LOG_DEBUG("RPC Response: {}", response);
        
        // Parse response
        json responseJson = json::parse(response);
        
        // Check for error
        if (responseJson.contains("error") && !responseJson["error"].is_null()) {
            auto error = responseJson["error"];
            std::string errorMsg = error.contains("message") ? 
                error["message"].get<std::string>() : "Unknown error";
            throw std::runtime_error("RPC error: " + errorMsg);
        }
        
        // Return result
        if (responseJson.contains("result")) {
            return responseJson["result"];
        }
        
        return json();
    }
};

RpcClient::RpcClient(const std::string& endpoint) 
    : impl_(std::make_unique<Impl>(endpoint)) {
    NEO_LOG_INFO("RPC Client initialized with endpoint: {}", endpoint);
}

RpcClient::~RpcClient() = default;

// Node information methods
std::string RpcClient::GetVersion() {
    auto result = impl_->MakeRequest("getversion");
    if (result.contains("useragent")) {
        return result["useragent"].get<std::string>();
    }
    return "";
}

uint32_t RpcClient::GetBlockCount() {
    auto result = impl_->MakeRequest("getblockcount");
    return result.get<uint32_t>();
}

std::string RpcClient::GetBestBlockHash() {
    auto result = impl_->MakeRequest("getbestblockhash");
    return result.get<std::string>();
}

uint32_t RpcClient::GetConnectionCount() {
    auto result = impl_->MakeRequest("getconnectioncount");
    return result.get<uint32_t>();
}

// Block queries
json RpcClient::GetBlock(const std::string& hash, bool verbose) {
    return impl_->MakeRequest("getblock", {hash, verbose});
}

json RpcClient::GetBlock(uint32_t index, bool verbose) {
    return impl_->MakeRequest("getblock", {index, verbose});
}

json RpcClient::GetBlockHeader(const std::string& hash, bool verbose) {
    return impl_->MakeRequest("getblockheader", {hash, verbose});
}

json RpcClient::GetBlockHeader(uint32_t index, bool verbose) {
    return impl_->MakeRequest("getblockheader", {index, verbose});
}

// Transaction operations
json RpcClient::GetRawTransaction(const std::string& txid, bool verbose) {
    return impl_->MakeRequest("getrawtransaction", {txid, verbose});
}

std::string RpcClient::SendRawTransaction(const std::string& hex) {
    auto result = impl_->MakeRequest("sendrawtransaction", {hex});
    return result.get<std::string>();
}

uint32_t RpcClient::GetTransactionHeight(const std::string& txid) {
    auto result = impl_->MakeRequest("gettransactionheight", {txid});
    return result.get<uint32_t>();
}

// Contract operations
json RpcClient::InvokeFunction(
    const std::string& scriptHash,
    const std::string& method,
    const std::vector<json>& params) {
    
    std::vector<json> rpcParams = {scriptHash, method};
    if (!params.empty()) {
        rpcParams.push_back(params);
    }
    
    return impl_->MakeRequest("invokefunction", rpcParams);
}

json RpcClient::InvokeScript(const std::string& script) {
    return impl_->MakeRequest("invokescript", {script});
}

json RpcClient::GetContractState(const std::string& scriptHash) {
    return impl_->MakeRequest("getcontractstate", {scriptHash});
}

// State queries
json RpcClient::GetNep17Balances(const std::string& address) {
    return impl_->MakeRequest("getnep17balances", {address});
}

json RpcClient::GetNep17Transfers(const std::string& address, uint64_t startTime, uint64_t endTime) {
    std::vector<json> params = {address};
    if (startTime > 0) {
        params.push_back(startTime);
        if (endTime > 0) {
            params.push_back(endTime);
        }
    }
    return impl_->MakeRequest("getnep17transfers", params);
}

json RpcClient::GetStorage(const std::string& contractHash, const std::string& key) {
    return impl_->MakeRequest("getstorage", {contractHash, key});
}

json RpcClient::FindStorage(const std::string& contractHash, const std::string& prefix) {
    return impl_->MakeRequest("findstorage", {contractHash, prefix});
}

// Account operations
json RpcClient::GetAccountState(const std::string& address) {
    return impl_->MakeRequest("getaccountstate", {address});
}

bool RpcClient::ValidateAddress(const std::string& address) {
    auto result = impl_->MakeRequest("validateaddress", {address});
    return result.contains("isvalid") && result["isvalid"].get<bool>();
}

uint64_t RpcClient::GetUnclaimedGas(const std::string& address) {
    auto result = impl_->MakeRequest("getunclaimedgas", {address});
    if (result.contains("unclaimed")) {
        return std::stoull(result["unclaimed"].get<std::string>());
    }
    return 0;
}

// Utility methods
std::vector<std::string> RpcClient::ListMethods() {
    auto result = impl_->MakeRequest("listmethods");
    std::vector<std::string> methods;
    for (const auto& method : result) {
        methods.push_back(method.get<std::string>());
    }
    return methods;
}

uint64_t RpcClient::CalculateNetworkFee(const std::string& tx) {
    auto result = impl_->MakeRequest("calculatenetworkfee", {tx});
    if (result.contains("networkfee")) {
        return std::stoull(result["networkfee"].get<std::string>());
    }
    return 0;
}

json RpcClient::GetApplicationLog(const std::string& txid) {
    return impl_->MakeRequest("getapplicationlog", {txid});
}

json RpcClient::GetStateRoot(uint32_t height) {
    return impl_->MakeRequest("getstateroot", {height});
}

json RpcClient::GetProof(const std::string& rootHash, const std::string& contractHash, const std::string& key) {
    return impl_->MakeRequest("getproof", {rootHash, contractHash, key});
}

// Custom RPC call
json RpcClient::Call(const std::string& method, const std::vector<json>& params) {
    return impl_->MakeRequest(method, params);
}

// Configuration
void RpcClient::SetTimeout(uint32_t timeoutMs) {
    impl_->timeout = timeoutMs;
}

std::string RpcClient::GetEndpoint() const {
    return impl_->endpoint;
}

bool RpcClient::TestConnection() {
    try {
        GetVersion();
        return true;
    } catch (...) {
        return false;
    }
}

} // namespace neo::sdk::rpc
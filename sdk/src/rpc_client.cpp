// rpc_client.cpp - Full implementation of Neo RPC client

#include <neo/sdk/rpc/rpc_client.h>
#include <curl/curl.h>
#include <sstream>
#include <stdexcept>
#include <chrono>
#include <thread>

namespace neo::sdk::rpc {

// Callback function for CURL
static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

// Implementation class
class RpcClient::Impl {
public:
    std::string endpoint_;
    uint32_t timeout_ms_ = 30000;
    CURL* curl_ = nullptr;
    
    Impl(const std::string& endpoint) : endpoint_(endpoint) {
        curl_global_init(CURL_GLOBAL_DEFAULT);
        curl_ = curl_easy_init();
        if (!curl_) {
            throw std::runtime_error("Failed to initialize CURL");
        }
    }
    
    ~Impl() {
        if (curl_) {
            curl_easy_cleanup(curl_);
        }
        curl_global_cleanup();
    }
    
    json makeRequest(const std::string& method, const std::vector<json>& params = {}) {
        // Build JSON-RPC request
        json request = {
            {"jsonrpc", "2.0"},
            {"method", method},
            {"params", params},
            {"id", 1}
        };
        
        std::string request_str = request.dump();
        std::string response_str;
        
        // Setup CURL
        curl_easy_setopt(curl_, CURLOPT_URL, endpoint_.c_str());
        curl_easy_setopt(curl_, CURLOPT_POSTFIELDS, request_str.c_str());
        curl_easy_setopt(curl_, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl_, CURLOPT_WRITEDATA, &response_str);
        curl_easy_setopt(curl_, CURLOPT_TIMEOUT_MS, timeout_ms_);
        
        // Set headers
        struct curl_slist* headers = nullptr;
        headers = curl_slist_append(headers, "Content-Type: application/json");
        curl_easy_setopt(curl_, CURLOPT_HTTPHEADER, headers);
        
        // Perform request
        CURLcode res = curl_easy_perform(curl_);
        curl_slist_free_all(headers);
        
        if (res != CURLE_OK) {
            throw std::runtime_error(std::string("CURL request failed: ") + curl_easy_strerror(res));
        }
        
        // Parse response
        json response = json::parse(response_str);
        
        // Check for error
        if (response.contains("error") && !response["error"].is_null()) {
            throw std::runtime_error(response["error"]["message"].get<std::string>());
        }
        
        return response["result"];
    }
};

// Constructor
RpcClient::RpcClient(const std::string& endpoint) 
    : impl_(std::make_unique<Impl>(endpoint)) {
}

// Destructor
RpcClient::~RpcClient() = default;

// Node information methods
std::string RpcClient::GetVersion() {
    json result = impl_->makeRequest("getversion");
    return result["useragent"].get<std::string>();
}

uint32_t RpcClient::GetBlockCount() {
    json result = impl_->makeRequest("getblockcount");
    return result.get<uint32_t>();
}

std::string RpcClient::GetBestBlockHash() {
    json result = impl_->makeRequest("getbestblockhash");
    return result.get<std::string>();
}

uint32_t RpcClient::GetConnectionCount() {
    json result = impl_->makeRequest("getconnectioncount");
    return result.get<uint32_t>();
}

// Block query methods
json RpcClient::GetBlock(const std::string& hash, bool verbose) {
    return impl_->makeRequest("getblock", {hash, verbose});
}

json RpcClient::GetBlock(uint32_t index, bool verbose) {
    return impl_->makeRequest("getblock", {index, verbose});
}

json RpcClient::GetBlockHeader(const std::string& hash, bool verbose) {
    return impl_->makeRequest("getblockheader", {hash, verbose});
}

json RpcClient::GetBlockHeader(uint32_t index, bool verbose) {
    return impl_->makeRequest("getblockheader", {index, verbose});
}

// Transaction methods
json RpcClient::GetRawTransaction(const std::string& txid, bool verbose) {
    return impl_->makeRequest("getrawtransaction", {txid, verbose});
}

std::string RpcClient::SendRawTransaction(const std::string& hex) {
    json result = impl_->makeRequest("sendrawtransaction", {hex});
    return result.get<std::string>();
}

uint32_t RpcClient::GetTransactionHeight(const std::string& txid) {
    json result = impl_->makeRequest("gettransactionheight", {txid});
    return result.get<uint32_t>();
}

// Contract methods
json RpcClient::InvokeFunction(const std::string& scriptHash, 
                               const std::string& method,
                               const std::vector<json>& params) {
    return impl_->makeRequest("invokefunction", {scriptHash, method, params});
}

json RpcClient::InvokeScript(const std::string& script) {
    return impl_->makeRequest("invokescript", {script});
}

json RpcClient::GetContractState(const std::string& scriptHash) {
    return impl_->makeRequest("getcontractstate", {scriptHash});
}

// State queries
json RpcClient::GetNep17Balances(const std::string& address) {
    return impl_->makeRequest("getnep17balances", {address});
}

json RpcClient::GetNep17Transfers(const std::string& address, uint64_t startTime, uint64_t endTime) {
    if (startTime == 0 && endTime == 0) {
        return impl_->makeRequest("getnep17transfers", {address});
    } else {
        return impl_->makeRequest("getnep17transfers", {address, startTime, endTime});
    }
}

json RpcClient::GetStorage(const std::string& contractHash, const std::string& key) {
    return impl_->makeRequest("getstorage", {contractHash, key});
}

json RpcClient::FindStorage(const std::string& contractHash, const std::string& prefix) {
    return impl_->makeRequest("findstorage", {contractHash, prefix});
}

// Account operations
json RpcClient::GetAccountState(const std::string& address) {
    // This method might not exist in newer versions, using getnep17balances instead
    return GetNep17Balances(address);
}

bool RpcClient::ValidateAddress(const std::string& address) {
    json result = impl_->makeRequest("validateaddress", {address});
    return result["isvalid"].get<bool>();
}

uint64_t RpcClient::GetUnclaimedGas(const std::string& address) {
    json result = impl_->makeRequest("getunclaimedgas", {address});
    return result["unclaimed"].get<uint64_t>();
}

// Utility methods
std::vector<std::string> RpcClient::ListMethods() {
    json result = impl_->makeRequest("listmethods");
    std::vector<std::string> methods;
    for (const auto& method : result) {
        methods.push_back(method.get<std::string>());
    }
    return methods;
}

uint64_t RpcClient::CalculateNetworkFee(const std::string& tx) {
    json result = impl_->makeRequest("calculatenetworkfee", {tx});
    return result["networkfee"].get<uint64_t>();
}

json RpcClient::GetApplicationLog(const std::string& txid) {
    return impl_->makeRequest("getapplicationlog", {txid});
}

json RpcClient::GetStateRoot(uint32_t height) {
    return impl_->makeRequest("getstateroot", {height});
}

json RpcClient::GetProof(const std::string& rootHash, 
                        const std::string& contractHash, 
                        const std::string& key) {
    return impl_->makeRequest("getproof", {rootHash, contractHash, key});
}

// Custom RPC call
json RpcClient::Call(const std::string& method, const std::vector<json>& params) {
    return impl_->makeRequest(method, params);
}

// Configuration
void RpcClient::SetTimeout(uint32_t timeoutMs) {
    impl_->timeout_ms_ = timeoutMs;
}

std::string RpcClient::GetEndpoint() const {
    return impl_->endpoint_;
}

bool RpcClient::TestConnection() {
    try {
        GetBlockCount();
        return true;
    } catch (...) {
        return false;
    }
}

} // namespace neo::sdk::rpc
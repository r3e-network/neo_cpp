#include <neo/core/logging.h>
#include <neo/network/ip_address.h>
#include <neo/network/p2p/local_node.h>
#include <neo/network/p2p/message.h>
#include <neo/network/p2p/payloads/get_data_payload.h>
#include <neo/network/p2p/task_manager.h>
#include <neo/network/p2p_server.h>
#include <neo/network/payload_factory.h>
#include <neo/network/tcp_connection.h>

namespace neo::network::p2p
{
// LocalNode implementation has been moved to local_node.cpp
// Commenting out stubs to avoid duplicate symbols

/*
// LocalNode implementation for P2P networking
class LocalNodeStub
{
  private:
    static LocalNodeStub instance;

  public:
    void Start(const ChannelsConfig&)
    {
        LOG_INFO("LocalNode stub Start()");
    }
    void Stop()
    {
        LOG_INFO("LocalNode stub Stop()");
    }
};

LocalNodeStub LocalNodeStub::instance;

bool LocalNode::Start(const ChannelsConfig& config)
{
    LOG_INFO("LocalNode stub Start()");
    return true;
}

void LocalNode::Stop()
{
    LOG_INFO("LocalNode stub Stop()");
}

void LocalNode::OnRemoteNodeHandshaked(RemoteNode* node)
{
    LOG_INFO("LocalNode stub OnRemoteNodeHandshaked()");
}

void LocalNode::OnVersionMessageReceived(RemoteNode* node, const payloads::VersionPayload& payload)
{
    LOG_INFO("LocalNode stub OnVersionMessageReceived()");
}

void LocalNode::OnAddrMessageReceived(RemoteNode* node, const payloads::AddrPayload& payload)
{
    LOG_INFO("LocalNode stub OnAddrMessageReceived()");
}

void LocalNode::OnPingMessageReceived(RemoteNode* node, const payloads::PingPayload& payload)
{
    LOG_INFO("LocalNode stub OnPingMessageReceived()");
}

void LocalNode::OnPongMessageReceived(RemoteNode* node, const payloads::PingPayload& payload)
{
    LOG_INFO("LocalNode stub OnPongMessageReceived()");
}

void LocalNode::OnInvMessageReceived(RemoteNode* node, const payloads::InvPayload& payload)
{
    LOG_INFO("LocalNode stub OnInvMessageReceived()");
}

void LocalNode::OnGetDataMessageReceived(RemoteNode* node, const payloads::GetDataPayload& payload)
{
    LOG_INFO("LocalNode stub OnGetDataMessageReceived()");
}

void LocalNode::OnGetBlocksMessageReceived(RemoteNode* node, const payloads::GetBlocksPayload& payload)
{
    LOG_INFO("LocalNode stub OnGetBlocksMessageReceived()");
}

void LocalNode::OnGetHeadersMessageReceived(RemoteNode* node, const payloads::GetBlocksPayload& payload)
{
    LOG_INFO("LocalNode stub OnGetHeadersMessageReceived()");
}

void LocalNode::OnHeadersMessageReceived(RemoteNode* node, const payloads::HeadersPayload& payload)
{
    LOG_INFO("LocalNode stub OnHeadersMessageReceived()");
}

void LocalNode::OnGetBlockByIndexMessageReceived(RemoteNode* node, const payloads::GetBlockByIndexPayload& payload)
{
    LOG_INFO("LocalNode stub OnGetBlockByIndexMessageReceived()");
}

void LocalNode::OnRemoteNodeDisconnected(RemoteNode* node)
{
    LOG_INFO("LocalNode stub OnRemoteNodeDisconnected()");
}

void LocalNode::OnMempoolMessageReceived(RemoteNode* node, const payloads::MempoolPayload& payload)
{
    LOG_INFO("LocalNode stub OnMempoolMessageReceived()");
}

void LocalNode::OnFilterAddMessageReceived(RemoteNode* node, const payloads::FilterAddPayload& payload)
{
    LOG_INFO("LocalNode stub OnFilterAddMessageReceived()");
}

void LocalNode::OnFilterClearMessageReceived(RemoteNode* node, const payloads::FilterClearPayload& payload)
{
    LOG_INFO("LocalNode stub OnFilterClearMessageReceived()");
}

void LocalNode::OnFilterLoadMessageReceived(RemoteNode* node, const payloads::FilterLoadPayload& payload)
{
    LOG_INFO("LocalNode stub OnFilterLoadMessageReceived()");
}

void LocalNode::OnRemoteNodeConnected(RemoteNode* node)
{
    LOG_INFO("LocalNode stub OnRemoteNodeConnected()");
}
*/

// TaskManager implementation for background tasks
TaskManager::TaskManager(std::shared_ptr<ledger::Blockchain>, std::shared_ptr<ledger::MemoryPool>)
{
    LOG_INFO("TaskManager stub constructor");
}

TaskManager::~TaskManager()
{
    LOG_INFO("TaskManager stub destructor");
}

// Message implementations moved to message_simple.cpp - stubs removed to avoid conflicts
}  // namespace neo::network::p2p

namespace neo::network::p2p
{

// PayloadFactory stub
std::shared_ptr<IPayload> PayloadFactory::DeserializePayload(MessageCommand command, io::BinaryReader& reader)
{
    // Returns nullptr when payload deserialization is not supported
    return nullptr;
}

}  // namespace neo::network::p2p

namespace neo::network
{

// P2PServer stub
size_t P2PServer::GetConnectedPeersCount() const
{
    LOG_WARNING("Stub: P2PServer::GetConnectedPeersCount() returning 0");
    return 0;
}

// TcpConnection stubs
TcpConnection::TcpConnection(boost::asio::io_context& context) : socket_(context) {}

TcpConnection::~TcpConnection() = default;

void TcpConnection::Start()
{
    LOG_INFO("TcpConnection::Start() stub");
}

void TcpConnection::Close()
{
    LOG_INFO("TcpConnection::Close() stub");
}

boost::asio::ip::tcp::socket& TcpConnection::GetSocket()
{
    return socket_;
}

void TcpConnection::SetConnectionClosedCallback(std::function<void()> callback)
{
    connectionClosedCallback_ = callback;
}

IPEndPoint TcpConnection::GetRemoteEndpoint() const
{
    try
    {
        auto endpoint = socket_.remote_endpoint();
        return IPEndPoint(IPAddress(endpoint.address().to_string()), endpoint.port());
    }
    catch (const std::exception& e)
    {
        // Log network endpoint conversion error
        return IPEndPoint(IPAddress::Any(), 0);
    }
}

}  // namespace neo::network
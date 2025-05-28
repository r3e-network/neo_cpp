#include <neo/network/p2p/local_node.h>
#include <neo/network/p2p/remote_node.h>
#include <neo/network/message.h>
#include <neo/network/p2p/payloads/version_payload.h>
#include <neo/network/p2p/payloads/ping_payload.h>
#include <neo/network/p2p/payloads/addr_payload.h>
#include <neo/network/p2p/payloads/inv_payload.h>
#include <neo/network/p2p/payloads/get_data_payload.h>
#include <neo/network/p2p/payloads/get_blocks_payload.h>
#include <neo/network/p2p/payloads/get_headers_payload.h>
#include <neo/network/p2p/payloads/get_block_by_index_payload.h>
#include <neo/network/p2p/payloads/headers_payload.h>
#include <neo/network/p2p/payloads/mempool_payload.h>
#include <neo/network/p2p/payloads/filter_add_payload.h>
#include <neo/network/p2p/payloads/filter_clear_payload.h>
#include <neo/network/p2p/payloads/filter_load_payload.h>
#include <chrono>

namespace neo::network::p2p
{
    void LocalNode::Broadcast(const Message& message, bool enableCompression)
    {
        std::vector<RemoteNode*> nodes = GetConnectedNodes();

        for (auto node : nodes)
        {
            if (node->IsHandshaked())
            {
                node->Send(message, enableCompression);
            }
        }
    }

    void LocalNode::BroadcastInv(InventoryType type, const std::vector<io::UInt256>& hashes)
    {
        if (hashes.empty())
            return;

        // Create an inv payload
        auto payload = std::make_shared<payloads::InvPayload>(type, hashes);

        // Create an inv message
        Message message = Message::Create(MessageCommand::Inv, payload);

        // Broadcast the message
        Broadcast(message);
    }

    void LocalNode::SetVersionMessageReceivedCallback(std::function<void(RemoteNode*, const payloads::VersionPayload&)> callback)
    {
        versionMessageReceivedCallback_ = callback;
    }

    void LocalNode::SetPingMessageReceivedCallback(std::function<void(RemoteNode*, const payloads::PingPayload&)> callback)
    {
        pingMessageReceivedCallback_ = callback;
    }

    void LocalNode::SetPongMessageReceivedCallback(std::function<void(RemoteNode*, const payloads::PingPayload&)> callback)
    {
        pongMessageReceivedCallback_ = callback;
    }

    void LocalNode::SetAddrMessageReceivedCallback(std::function<void(RemoteNode*, const payloads::AddrPayload&)> callback)
    {
        addrMessageReceivedCallback_ = callback;
    }

    void LocalNode::SetInvMessageReceivedCallback(std::function<void(RemoteNode*, const payloads::InvPayload&)> callback)
    {
        invMessageReceivedCallback_ = callback;
    }

    void LocalNode::SetGetDataMessageReceivedCallback(std::function<void(RemoteNode*, const payloads::GetDataPayload&)> callback)
    {
        getDataMessageReceivedCallback_ = callback;
    }

    void LocalNode::SetGetBlocksMessageReceivedCallback(std::function<void(RemoteNode*, const payloads::GetBlocksPayload&)> callback)
    {
        getBlocksMessageReceivedCallback_ = callback;
    }

    void LocalNode::SetGetBlockByIndexMessageReceivedCallback(std::function<void(RemoteNode*, const payloads::GetBlockByIndexPayload&)> callback)
    {
        getBlockByIndexMessageReceivedCallback_ = callback;
    }

    void LocalNode::SetGetHeadersMessageReceivedCallback(std::function<void(RemoteNode*, const payloads::GetBlocksPayload&)> callback)
    {
        getHeadersMessageReceivedCallback_ = callback;
    }

    void LocalNode::SetHeadersMessageReceivedCallback(std::function<void(RemoteNode*, const payloads::HeadersPayload&)> callback)
    {
        headersMessageReceivedCallback_ = callback;
    }

    void LocalNode::SetMempoolMessageReceivedCallback(std::function<void(RemoteNode*, const payloads::MempoolPayload&)> callback)
    {
        mempoolMessageReceivedCallback_ = callback;
    }

    void LocalNode::SetFilterAddMessageReceivedCallback(std::function<void(RemoteNode*, const payloads::FilterAddPayload&)> callback)
    {
        filterAddMessageReceivedCallback_ = callback;
    }

    void LocalNode::SetFilterClearMessageReceivedCallback(std::function<void(RemoteNode*, const payloads::FilterClearPayload&)> callback)
    {
        filterClearMessageReceivedCallback_ = callback;
    }

    void LocalNode::SetFilterLoadMessageReceivedCallback(std::function<void(RemoteNode*, const payloads::FilterLoadPayload&)> callback)
    {
        filterLoadMessageReceivedCallback_ = callback;
    }

    void LocalNode::SetRemoteNodeConnectedCallback(std::function<void(RemoteNode*)> callback)
    {
        remoteNodeConnectedCallback_ = callback;
    }

    void LocalNode::SetRemoteNodeDisconnectedCallback(std::function<void(RemoteNode*)> callback)
    {
        remoteNodeDisconnectedCallback_ = callback;
    }

    void LocalNode::SetRemoteNodeHandshakedCallback(std::function<void(RemoteNode*)> callback)
    {
        remoteNodeHandshakedCallback_ = callback;
    }

    void LocalNode::OnVersionMessageReceived(RemoteNode* remoteNode, const payloads::VersionPayload& payload)
    {
        if (versionMessageReceivedCallback_)
        {
            versionMessageReceivedCallback_(remoteNode, payload);
        }
    }

    void LocalNode::OnPingMessageReceived(RemoteNode* remoteNode, const payloads::PingPayload& payload)
    {
        if (pingMessageReceivedCallback_)
        {
            pingMessageReceivedCallback_(remoteNode, payload);
        }
    }

    void LocalNode::OnPongMessageReceived(RemoteNode* remoteNode, const payloads::PingPayload& payload)
    {
        if (pongMessageReceivedCallback_)
        {
            pongMessageReceivedCallback_(remoteNode, payload);
        }
    }

    void LocalNode::OnAddrMessageReceived(RemoteNode* remoteNode, const payloads::AddrPayload& payload)
    {
        if (addrMessageReceivedCallback_)
        {
            addrMessageReceivedCallback_(remoteNode, payload);
        }
    }

    void LocalNode::OnInvMessageReceived(RemoteNode* remoteNode, const payloads::InvPayload& payload)
    {
        if (invMessageReceivedCallback_)
        {
            invMessageReceivedCallback_(remoteNode, payload);
        }
    }

    void LocalNode::OnGetDataMessageReceived(RemoteNode* remoteNode, const payloads::GetDataPayload& payload)
    {
        if (getDataMessageReceivedCallback_)
        {
            getDataMessageReceivedCallback_(remoteNode, payload);
        }
    }

    void LocalNode::OnGetBlocksMessageReceived(RemoteNode* remoteNode, const payloads::GetBlocksPayload& payload)
    {
        if (getBlocksMessageReceivedCallback_)
        {
            getBlocksMessageReceivedCallback_(remoteNode, payload);
        }
    }

    void LocalNode::OnGetBlockByIndexMessageReceived(RemoteNode* remoteNode, const payloads::GetBlockByIndexPayload& payload)
    {
        if (getBlockByIndexMessageReceivedCallback_)
        {
            getBlockByIndexMessageReceivedCallback_(remoteNode, payload);
        }
    }

    void LocalNode::OnGetHeadersMessageReceived(RemoteNode* remoteNode, const payloads::GetBlocksPayload& payload)
    {
        if (getHeadersMessageReceivedCallback_)
        {
            getHeadersMessageReceivedCallback_(remoteNode, payload);
        }
    }

    void LocalNode::OnHeadersMessageReceived(RemoteNode* remoteNode, const payloads::HeadersPayload& payload)
    {
        if (headersMessageReceivedCallback_)
        {
            headersMessageReceivedCallback_(remoteNode, payload);
        }
    }

    void LocalNode::OnMempoolMessageReceived(RemoteNode* remoteNode, const payloads::MempoolPayload& payload)
    {
        if (mempoolMessageReceivedCallback_)
        {
            mempoolMessageReceivedCallback_(remoteNode, payload);
        }
    }

    void LocalNode::OnFilterAddMessageReceived(RemoteNode* remoteNode, const payloads::FilterAddPayload& payload)
    {
        if (filterAddMessageReceivedCallback_)
        {
            filterAddMessageReceivedCallback_(remoteNode, payload);
        }
    }

    void LocalNode::OnFilterClearMessageReceived(RemoteNode* remoteNode, const payloads::FilterClearPayload& payload)
    {
        if (filterClearMessageReceivedCallback_)
        {
            filterClearMessageReceivedCallback_(remoteNode, payload);
        }
    }

    void LocalNode::OnFilterLoadMessageReceived(RemoteNode* remoteNode, const payloads::FilterLoadPayload& payload)
    {
        if (filterLoadMessageReceivedCallback_)
        {
            filterLoadMessageReceivedCallback_(remoteNode, payload);
        }
    }

    void LocalNode::OnRemoteNodeConnected(RemoteNode* remoteNode)
    {
        // Mark the peer as connected
        MarkPeerConnected(remoteNode->GetRemoteEndPoint());

        if (remoteNodeConnectedCallback_)
        {
            remoteNodeConnectedCallback_(remoteNode);
        }
    }

    void LocalNode::OnRemoteNodeDisconnected(RemoteNode* remoteNode)
    {
        // Mark the peer as disconnected
        MarkPeerDisconnected(remoteNode->GetRemoteEndPoint());

        // Remove the remote node from the connected nodes
        RemoveConnectedNode(remoteNode->GetConnection()->GetId());

        if (remoteNodeDisconnectedCallback_)
        {
            remoteNodeDisconnectedCallback_(remoteNode);
        }
    }

    void LocalNode::OnRemoteNodeHandshaked(RemoteNode* remoteNode)
    {
        // Update the peer with the version information
        auto peer = peerList_.GetPeer(remoteNode->GetRemoteEndPoint());
        if (peer)
        {
            peer->SetVersion(remoteNode->GetVersion());
            peer->SetCapabilities(remoteNode->GetCapabilities());

            // Update the last seen time
            peer->SetLastSeenTime(std::chrono::duration_cast<std::chrono::seconds>(
                std::chrono::system_clock::now().time_since_epoch()).count());
        }

        if (remoteNodeHandshakedCallback_)
        {
            remoteNodeHandshakedCallback_(remoteNode);
        }
    }
}

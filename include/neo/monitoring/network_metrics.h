/**
 * @file network_metrics.h
 * @brief Performance metrics collection
 * @author Neo C++ Team
 * @date 2025
 * @copyright MIT License
 */

#pragma once

#include <neo/monitoring/metrics_collector.h>
#include <memory>
#include <string>

namespace neo::monitoring {

/**
 * @brief Network and P2P metrics collector
 */
class NetworkMetrics {
public:
    static NetworkMetrics& GetInstance() {
        static NetworkMetrics instance;
        return instance;
    }
    
    // Initialize all network metrics
    void Initialize();
    
    // Connection metrics
    void OnPeerConnected(const std::string& peer);
    void OnPeerDisconnected(const std::string& peer);
    void SetConnectedPeers(size_t count);
    void SetMaxPeers(size_t max);
    
    // Message metrics
    void OnMessageSent(const std::string& type, size_t bytes);
    void OnMessageReceived(const std::string& type, size_t bytes);
    void OnMessageProcessed(const std::string& type, double duration);
    
    // Bandwidth metrics
    void AddBytesReceived(uint64_t bytes);
    void AddBytesSent(uint64_t bytes);
    void SetBandwidthIn(double bytesPerSecond);
    void SetBandwidthOut(double bytesPerSecond);
    
    // Latency metrics
    void RecordPeerLatency(const std::string& peer, double latency);
    void RecordRoundTripTime(double rtt);
    
    // Error metrics
    void OnConnectionError();
    void OnMessageError(const std::string& type);
    void OnProtocolViolation();
    
    // RPC metrics
    void OnRpcRequest(const std::string& method);
    void OnRpcResponse(const std::string& method, double duration, bool success);
    void SetActiveRpcConnections(size_t count);
    
private:
    NetworkMetrics() = default;
    ~NetworkMetrics() = default;
    
    NetworkMetrics(const NetworkMetrics&) = delete;
    NetworkMetrics& operator=(const NetworkMetrics&) = delete;
    
    // Connection metrics
    std::shared_ptr<Counter> peers_connected_;
    std::shared_ptr<Counter> peers_disconnected_;
    std::shared_ptr<Gauge> peers_current_;
    std::shared_ptr<Gauge> peers_max_;
    
    // Message metrics
    std::shared_ptr<Counter> messages_sent_;
    std::shared_ptr<Counter> messages_received_;
    std::shared_ptr<Counter> bytes_sent_;
    std::shared_ptr<Counter> bytes_received_;
    std::shared_ptr<Histogram> message_processing_time_;
    
    // Bandwidth metrics
    std::shared_ptr<Gauge> bandwidth_in_;
    std::shared_ptr<Gauge> bandwidth_out_;
    
    // Latency metrics
    std::shared_ptr<Histogram> peer_latency_;
    std::shared_ptr<Histogram> round_trip_time_;
    
    // Error metrics
    std::shared_ptr<Counter> connection_errors_;
    std::shared_ptr<Counter> message_errors_;
    std::shared_ptr<Counter> protocol_violations_;
    
    // RPC metrics
    std::shared_ptr<Counter> rpc_requests_;
    std::shared_ptr<Counter> rpc_responses_success_;
    std::shared_ptr<Counter> rpc_responses_failure_;
    std::shared_ptr<Histogram> rpc_duration_;
    std::shared_ptr<Gauge> rpc_connections_active_;
};

} // namespace neo::monitoring
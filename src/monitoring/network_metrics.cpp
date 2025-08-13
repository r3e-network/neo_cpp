/**
 * @file network_metrics.cpp
 * @brief Performance metrics collection
 * @author Neo C++ Team
 * @date 2025
 * @copyright MIT License
 */

#include <neo/monitoring/network_metrics.h>

namespace neo::monitoring {

void NetworkMetrics::Initialize() {
    auto& collector = MetricsCollector::GetInstance();
    
    // Initialize connection metrics
    peers_connected_ = collector.RegisterCounter(
        "neo_peers_connected_total",
        "Total number of peer connections established");
    
    peers_disconnected_ = collector.RegisterCounter(
        "neo_peers_disconnected_total",
        "Total number of peer disconnections");
    
    peers_current_ = collector.RegisterGauge(
        "neo_peers_current",
        "Current number of connected peers");
    
    peers_max_ = collector.RegisterGauge(
        "neo_peers_max",
        "Maximum number of allowed peers");
    
    // Initialize message metrics
    messages_sent_ = collector.RegisterCounter(
        "neo_messages_sent_total",
        "Total number of messages sent");
    
    messages_received_ = collector.RegisterCounter(
        "neo_messages_received_total",
        "Total number of messages received");
    
    bytes_sent_ = collector.RegisterCounter(
        "neo_bytes_sent_total",
        "Total number of bytes sent");
    
    bytes_received_ = collector.RegisterCounter(
        "neo_bytes_received_total",
        "Total number of bytes received");
    
    message_processing_time_ = collector.RegisterHistogram(
        "neo_message_processing_duration_seconds",
        "Time taken to process network messages",
        {0.00001, 0.00005, 0.0001, 0.0005, 0.001, 0.005, 0.01, 0.05, 0.1});
    
    // Initialize bandwidth metrics
    bandwidth_in_ = collector.RegisterGauge(
        "neo_bandwidth_in_bytes_per_second",
        "Incoming bandwidth in bytes per second");
    
    bandwidth_out_ = collector.RegisterGauge(
        "neo_bandwidth_out_bytes_per_second",
        "Outgoing bandwidth in bytes per second");
    
    // Initialize latency metrics
    peer_latency_ = collector.RegisterHistogram(
        "neo_peer_latency_seconds",
        "Latency to peers in seconds",
        {0.001, 0.005, 0.01, 0.025, 0.05, 0.1, 0.25, 0.5, 1.0});
    
    round_trip_time_ = collector.RegisterHistogram(
        "neo_rtt_seconds",
        "Round trip time in seconds",
        {0.001, 0.005, 0.01, 0.025, 0.05, 0.1, 0.25, 0.5, 1.0, 2.5});
    
    // Initialize error metrics
    connection_errors_ = collector.RegisterCounter(
        "neo_connection_errors_total",
        "Total number of connection errors");
    
    message_errors_ = collector.RegisterCounter(
        "neo_message_errors_total",
        "Total number of message processing errors");
    
    protocol_violations_ = collector.RegisterCounter(
        "neo_protocol_violations_total",
        "Total number of protocol violations");
    
    // Initialize RPC metrics
    rpc_requests_ = collector.RegisterCounter(
        "neo_rpc_requests_total",
        "Total number of RPC requests");
    
    rpc_responses_success_ = collector.RegisterCounter(
        "neo_rpc_responses_success_total",
        "Total number of successful RPC responses");
    
    rpc_responses_failure_ = collector.RegisterCounter(
        "neo_rpc_responses_failure_total",
        "Total number of failed RPC responses");
    
    rpc_duration_ = collector.RegisterHistogram(
        "neo_rpc_duration_seconds",
        "RPC request duration in seconds",
        {0.001, 0.005, 0.01, 0.025, 0.05, 0.1, 0.25, 0.5, 1.0, 2.5, 5.0});
    
    rpc_connections_active_ = collector.RegisterGauge(
        "neo_rpc_connections_active",
        "Number of active RPC connections");
}

void NetworkMetrics::OnPeerConnected(const std::string& peer) {
    if (peers_connected_) {
        peers_connected_->Increment();
    }
    if (peers_current_) {
        peers_current_->Increment();
    }
}

void NetworkMetrics::OnPeerDisconnected(const std::string& peer) {
    if (peers_disconnected_) {
        peers_disconnected_->Increment();
    }
    if (peers_current_) {
        peers_current_->Decrement();
    }
}

void NetworkMetrics::SetConnectedPeers(size_t count) {
    if (peers_current_) {
        peers_current_->Set(static_cast<double>(count));
    }
}

void NetworkMetrics::SetMaxPeers(size_t max) {
    if (peers_max_) {
        peers_max_->Set(static_cast<double>(max));
    }
}

void NetworkMetrics::OnMessageSent(const std::string& type, size_t bytes) {
    if (messages_sent_) {
        messages_sent_->Increment();
    }
    if (bytes_sent_) {
        bytes_sent_->Increment(static_cast<double>(bytes));
    }
    
    // Track per-message-type metrics
    auto& collector = MetricsCollector::GetInstance();
    auto counter = collector.RegisterCounter(
        "neo_messages_sent_" + type + "_total",
        "Number of " + type + " messages sent");
    if (counter) {
        counter->Increment();
    }
}

void NetworkMetrics::OnMessageReceived(const std::string& type, size_t bytes) {
    if (messages_received_) {
        messages_received_->Increment();
    }
    if (bytes_received_) {
        bytes_received_->Increment(static_cast<double>(bytes));
    }
    
    // Track per-message-type metrics
    auto& collector = MetricsCollector::GetInstance();
    auto counter = collector.RegisterCounter(
        "neo_messages_received_" + type + "_total",
        "Number of " + type + " messages received");
    if (counter) {
        counter->Increment();
    }
}

void NetworkMetrics::OnMessageProcessed(const std::string& type, double duration) {
    if (message_processing_time_) {
        message_processing_time_->Observe(duration);
    }
    
    // Track per-message-type processing time
    auto& collector = MetricsCollector::GetInstance();
    auto histogram = collector.RegisterHistogram(
        "neo_message_processing_" + type + "_seconds",
        "Processing time for " + type + " messages");
    if (histogram) {
        histogram->Observe(duration);
    }
}

void NetworkMetrics::AddBytesReceived(uint64_t bytes) {
    if (bytes_received_) {
        bytes_received_->Increment(static_cast<double>(bytes));
    }
}

void NetworkMetrics::AddBytesSent(uint64_t bytes) {
    if (bytes_sent_) {
        bytes_sent_->Increment(static_cast<double>(bytes));
    }
}

void NetworkMetrics::SetBandwidthIn(double bytesPerSecond) {
    if (bandwidth_in_) {
        bandwidth_in_->Set(bytesPerSecond);
    }
}

void NetworkMetrics::SetBandwidthOut(double bytesPerSecond) {
    if (bandwidth_out_) {
        bandwidth_out_->Set(bytesPerSecond);
    }
}

void NetworkMetrics::RecordPeerLatency(const std::string& peer, double latency) {
    if (peer_latency_) {
        peer_latency_->Observe(latency);
    }
}

void NetworkMetrics::RecordRoundTripTime(double rtt) {
    if (round_trip_time_) {
        round_trip_time_->Observe(rtt);
    }
}

void NetworkMetrics::OnConnectionError() {
    if (connection_errors_) {
        connection_errors_->Increment();
    }
}

void NetworkMetrics::OnMessageError(const std::string& type) {
    if (message_errors_) {
        message_errors_->Increment();
    }
    
    // Track per-message-type errors
    auto& collector = MetricsCollector::GetInstance();
    auto counter = collector.RegisterCounter(
        "neo_message_errors_" + type + "_total",
        "Number of " + type + " message errors");
    if (counter) {
        counter->Increment();
    }
}

void NetworkMetrics::OnProtocolViolation() {
    if (protocol_violations_) {
        protocol_violations_->Increment();
    }
}

void NetworkMetrics::OnRpcRequest(const std::string& method) {
    if (rpc_requests_) {
        rpc_requests_->Increment();
    }
    
    // Track per-method metrics
    auto& collector = MetricsCollector::GetInstance();
    auto counter = collector.RegisterCounter(
        "neo_rpc_requests_" + method + "_total",
        "Number of " + method + " RPC requests");
    if (counter) {
        counter->Increment();
    }
}

void NetworkMetrics::OnRpcResponse(const std::string& method, double duration, bool success) {
    if (success && rpc_responses_success_) {
        rpc_responses_success_->Increment();
    } else if (!success && rpc_responses_failure_) {
        rpc_responses_failure_->Increment();
    }
    
    if (rpc_duration_) {
        rpc_duration_->Observe(duration);
    }
    
    // Track per-method response time
    auto& collector = MetricsCollector::GetInstance();
    auto histogram = collector.RegisterHistogram(
        "neo_rpc_duration_" + method + "_seconds",
        "Duration of " + method + " RPC calls");
    if (histogram) {
        histogram->Observe(duration);
    }
}

void NetworkMetrics::SetActiveRpcConnections(size_t count) {
    if (rpc_connections_active_) {
        rpc_connections_active_->Set(static_cast<double>(count));
    }
}

} // namespace neo::monitoring
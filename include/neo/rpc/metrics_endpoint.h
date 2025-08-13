/**
 * @file metrics_endpoint.h
 * @brief RPC endpoint for exposing performance metrics
 */

#pragma once

#include <string>
#include <memory>

namespace neo {
namespace rpc {

/**
 * @brief Metrics endpoint handler for RPC server
 */
class MetricsEndpoint {
public:
    /**
     * @brief Get metrics in Prometheus format
     * @return Prometheus-formatted metrics string
     */
    static std::string GetPrometheusMetrics();
    
    /**
     * @brief Get metrics in JSON format
     * @return JSON-formatted metrics string
     */
    static std::string GetJsonMetrics();
    
    /**
     * @brief Get health check status
     * @return JSON health status
     */
    static std::string GetHealthStatus();
    
    /**
     * @brief Register metrics endpoints with RPC server
     * @param server RPC server instance
     */
    template<typename RpcServer>
    static void RegisterEndpoints(RpcServer& server) {
        // Register /metrics endpoint (Prometheus format)
        server.Get("/metrics", [](const auto& req, auto& res) {
            res.set_content(GetPrometheusMetrics(), "text/plain; version=0.0.4");
        });
        
        // Register /metrics/json endpoint
        server.Get("/metrics/json", [](const auto& req, auto& res) {
            res.set_content(GetJsonMetrics(), "application/json");
        });
        
        // Register /health endpoint
        server.Get("/health", [](const auto& req, auto& res) {
            res.set_content(GetHealthStatus(), "application/json");
        });
    }
};

} // namespace rpc
} // namespace neo
#pragma once

#include <neo/network/ip_address.h>
#include <string>
#include <chrono>
#include <memory>

namespace neo::network
{
    /**
     * @brief Provides methods for interacting with UPnP devices.
     */
    class UPnP
    {
    public:
        /**
         * @brief Gets or sets the timeout for discovering the UPnP device.
         * @return The timeout for discovering the UPnP device.
         */
        static std::chrono::seconds GetTimeOut();

        /**
         * @brief Sets the timeout for discovering the UPnP device.
         * @param timeout The timeout for discovering the UPnP device.
         */
        static void SetTimeOut(std::chrono::seconds timeout);

        /**
         * @brief Sends an Udp broadcast message to discover the UPnP device.
         * @return true if the UPnP device is successfully discovered; otherwise, false.
         */
        static bool Discover();

        /**
         * @brief Attempt to create a port forwarding.
         * @param port The port to forward.
         * @param protocol The protocol of the port (TCP or UDP).
         * @param description The description of the forward.
         */
        static void ForwardPort(uint16_t port, const std::string& protocol, const std::string& description);

        /**
         * @brief Attempt to delete a port forwarding.
         * @param port The port to forward.
         * @param protocol The protocol of the port (TCP or UDP).
         */
        static void DeleteForwardingRule(uint16_t port, const std::string& protocol);

        /**
         * @brief Attempt to get the external IP address of the local host.
         * @return The external IP address of the local host.
         */
        static IPAddress GetExternalIP();

    private:
        /**
         * @brief The timeout for discovering the UPnP device.
         */
        static std::chrono::seconds timeOut_;

        /**
         * @brief The service URL of the UPnP device.
         */
        static std::string serviceUrl_;

        /**
         * @brief Gets the service URL from the UPnP device description.
         * @param resp The URL of the UPnP device description.
         * @return The service URL of the UPnP device.
         */
        static std::string GetServiceUrl(const std::string& resp);

        /**
         * @brief Combines two URLs.
         * @param baseUrl The base URL.
         * @param relativeUrl The relative URL.
         * @return The combined URL.
         */
        static std::string CombineUrls(const std::string& baseUrl, const std::string& relativeUrl);

        /**
         * @brief Sends a SOAP request to the UPnP device.
         * @param url The URL of the UPnP device.
         * @param soap The SOAP request.
         * @param function The function name.
         * @return The XML response from the UPnP device.
         */
        static std::string SOAPRequest(const std::string& url, const std::string& soap, const std::string& function);
    };
}

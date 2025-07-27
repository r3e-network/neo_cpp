#include <boost/algorithm/string.hpp>
#include <boost/asio.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <chrono>
#include <neo/logging/logger.h>
#include <neo/network/upnp.h>
#include <sstream>
#include <stdexcept>
#include <thread>

namespace neo::network
{
// Initialize static members
std::chrono::seconds UPnP::timeOut_ = std::chrono::seconds(3);
std::string UPnP::serviceUrl_;

std::chrono::seconds UPnP::GetTimeOut()
{
    return timeOut_;
}

void UPnP::SetTimeOut(std::chrono::seconds timeout)
{
    timeOut_ = timeout;
}

bool UPnP::Discover()
{
    try
    {
        // Create a UDP socket
        boost::asio::io_context io_context;
        boost::asio::ip::udp::socket socket(io_context, boost::asio::ip::udp::endpoint(boost::asio::ip::udp::v4(), 0));

        // Set socket options
        socket.set_option(boost::asio::socket_base::broadcast(true));
        // Set receive timeout using the new Boost ASIO API
        boost::asio::socket_base::receive_low_watermark option(1);
        socket.set_option(option);

        // Create the discovery message
        std::string req = "M-SEARCH * HTTP/1.1\r\n"
                          "HOST: 239.255.255.250:1900\r\n"
                          "ST:upnp:rootdevice\r\n"
                          "MAN:\"ssdp:discover\"\r\n"
                          "MX:3\r\n\r\n";

        // Send the discovery message
        boost::asio::ip::udp::endpoint broadcast_endpoint(boost::asio::ip::address_v4::broadcast(), 1900);
        socket.send_to(boost::asio::buffer(req), broadcast_endpoint);
        socket.send_to(boost::asio::buffer(req), broadcast_endpoint);
        socket.send_to(boost::asio::buffer(req), broadcast_endpoint);

        // Record the start time
        auto start = std::chrono::steady_clock::now();

        // Receive responses
        char buffer[4096];
        boost::asio::ip::udp::endpoint sender_endpoint;

        while (std::chrono::steady_clock::now() - start < timeOut_)
        {
            try
            {
                // Receive a response
                size_t length = socket.receive_from(boost::asio::buffer(buffer), sender_endpoint);

                // Convert the response to a string
                std::string resp(buffer, length);
                boost::algorithm::to_lower(resp);

                // Check if the response contains the root device
                if (resp.find("upnp:rootdevice") != std::string::npos)
                {
                    // Extract the location URL
                    size_t location_pos = resp.find("location:");
                    if (location_pos != std::string::npos)
                    {
                        location_pos += 9;  // Skip "location:"
                        size_t end_pos = resp.find('\r', location_pos);
                        if (end_pos != std::string::npos)
                        {
                            std::string location = resp.substr(location_pos, end_pos - location_pos);
                            boost::algorithm::trim(location);

                            // Get the service URL
                            serviceUrl_ = GetServiceUrl(location);
                            if (!serviceUrl_.empty())
                            {
                                return true;
                            }
                        }
                    }
                }
            }
            catch (const std::exception& e)
            {
                // Ignore errors and continue
                logging::Logger::Instance().Warning("Network",
                                                    std::string("Error receiving UPnP response: ") + e.what());
            }
        }

        return false;
    }
    catch (const std::exception& e)
    {
        logging::Logger::Instance().Error("Network", std::string("Error discovering UPnP device: ") + e.what());
        return false;
    }
}

std::string UPnP::GetServiceUrl(const std::string& resp)
{
    try
    {
        // Download the device description
        boost::asio::io_context io_context;
        boost::beast::tcp_stream stream(io_context);

        // Parse the URL
        std::string host;
        std::string port;
        std::string target;

        size_t protocol_end = resp.find("://");
        if (protocol_end != std::string::npos)
        {
            size_t host_start = protocol_end + 3;
            size_t host_end = resp.find(':', host_start);
            if (host_end != std::string::npos)
            {
                host = resp.substr(host_start, host_end - host_start);
                size_t port_start = host_end + 1;
                size_t port_end = resp.find('/', port_start);
                if (port_end != std::string::npos)
                {
                    port = resp.substr(port_start, port_end - port_start);
                    target = resp.substr(port_end);
                }
            }
            else
            {
                host_end = resp.find('/', host_start);
                if (host_end != std::string::npos)
                {
                    host = resp.substr(host_start, host_end - host_start);
                    port = "80";  // Default HTTP port
                    target = resp.substr(host_end);
                }
            }
        }

        if (host.empty() || port.empty() || target.empty())
        {
            logging::Logger::Instance().Error("Network", "Invalid UPnP device URL: " + resp);
            return "";
        }

        // Connect to the host
        boost::asio::ip::tcp::resolver resolver(io_context);
        auto const results = resolver.resolve(host, port);
        stream.connect(results);

        // Send the HTTP request
        boost::beast::http::request<boost::beast::http::string_body> req{boost::beast::http::verb::get, target, 11};
        req.set(boost::beast::http::field::host, host);
        req.set(boost::beast::http::field::user_agent, BOOST_BEAST_VERSION_STRING);

        boost::beast::http::write(stream, req);

        // Receive the HTTP response
        boost::beast::flat_buffer buffer;
        boost::beast::http::response<boost::beast::http::string_body> res;
        boost::beast::http::read(stream, buffer, res);

        // Close the connection
        stream.socket().shutdown(boost::asio::ip::tcp::socket::shutdown_both);

        // Parse the XML response
        std::istringstream xml_stream(res.body());
        boost::property_tree::ptree pt;
        boost::property_tree::read_xml(xml_stream, pt);

        // Check if the device is an Internet Gateway Device
        std::string device_type = pt.get<std::string>("root.device.deviceType", "");
        if (device_type.find("InternetGatewayDevice") == std::string::npos)
        {
            logging::Logger::Instance().Warning("Network", "UPnP device is not an Internet Gateway Device");
            return "";
        }

        // Find the WANIPConnection service
        std::string control_url;
        for (const auto& service : pt.get_child("root.device.serviceList"))
        {
            std::string service_type = service.second.get<std::string>("serviceType", "");
            if (service_type.find("WANIPConnection") != std::string::npos)
            {
                control_url = service.second.get<std::string>("controlURL", "");
                break;
            }
        }

        if (control_url.empty())
        {
            logging::Logger::Instance().Warning("Network", "UPnP device does not have a WANIPConnection service");
            return "";
        }

        // Combine the base URL with the control URL
        return CombineUrls(resp, control_url);
    }
    catch (const std::exception& e)
    {
        logging::Logger::Instance().Error("Network", std::string("Error getting UPnP service URL: ") + e.what());
        return "";
    }
}

std::string UPnP::CombineUrls(const std::string& baseUrl, const std::string& relativeUrl)
{
    if (relativeUrl.empty())
        return "";

    if (relativeUrl[0] == '/')
    {
        // Absolute path
        size_t protocol_end = baseUrl.find("://");
        if (protocol_end != std::string::npos)
        {
            size_t path_start = baseUrl.find('/', protocol_end + 3);
            if (path_start != std::string::npos)
            {
                return baseUrl.substr(0, path_start) + relativeUrl;
            }
            else
            {
                return baseUrl + relativeUrl;
            }
        }
    }

    // Relative path
    size_t last_slash = baseUrl.rfind('/');
    if (last_slash != std::string::npos)
    {
        return baseUrl.substr(0, last_slash + 1) + relativeUrl;
    }

    return baseUrl + "/" + relativeUrl;
}

void UPnP::ForwardPort(uint16_t port, const std::string& protocol, const std::string& description)
{
    if (serviceUrl_.empty())
        throw std::runtime_error("No UPnP service available or Discover() has not been called");

    // Get the local IP address
    boost::asio::io_context io_context;
    boost::asio::ip::udp::resolver resolver(io_context);
    auto results = resolver.resolve(boost::asio::ip::udp::v4(), "8.8.8.8", "53");
    boost::asio::ip::udp::endpoint endpoint = *results.begin();
    boost::asio::ip::udp::socket socket(io_context);
    socket.open(boost::asio::ip::udp::v4());
    socket.connect(endpoint);
    boost::asio::ip::address local_address = socket.local_endpoint().address();

    // Create the SOAP request
    std::string soap = "<u:AddPortMapping xmlns:u=\"urn:schemas-upnp-org:service:WANIPConnection:1\">"
                       "<NewRemoteHost></NewRemoteHost>"
                       "<NewExternalPort>" +
                       std::to_string(port) +
                       "</NewExternalPort>"
                       "<NewProtocol>" +
                       protocol +
                       "</NewProtocol>"
                       "<NewInternalPort>" +
                       std::to_string(port) +
                       "</NewInternalPort>"
                       "<NewInternalClient>" +
                       local_address.to_string() +
                       "</NewInternalClient>"
                       "<NewEnabled>1</NewEnabled>"
                       "<NewPortMappingDescription>" +
                       description +
                       "</NewPortMappingDescription>"
                       "<NewLeaseDuration>0</NewLeaseDuration>"
                       "</u:AddPortMapping>";

    // Send the SOAP request
    SOAPRequest(serviceUrl_, soap, "AddPortMapping");

    logging::Logger::Instance().Info("Network",
                                     "UPnP port forwarding created for " + protocol + " port " + std::to_string(port));
}

void UPnP::DeleteForwardingRule(uint16_t port, const std::string& protocol)
{
    if (serviceUrl_.empty())
        throw std::runtime_error("No UPnP service available or Discover() has not been called");

    // Create the SOAP request
    std::string soap = "<u:DeletePortMapping xmlns:u=\"urn:schemas-upnp-org:service:WANIPConnection:1\">"
                       "<NewRemoteHost></NewRemoteHost>"
                       "<NewExternalPort>" +
                       std::to_string(port) +
                       "</NewExternalPort>"
                       "<NewProtocol>" +
                       protocol +
                       "</NewProtocol>"
                       "</u:DeletePortMapping>";

    // Send the SOAP request
    SOAPRequest(serviceUrl_, soap, "DeletePortMapping");

    logging::Logger::Instance().Info("Network",
                                     "UPnP port forwarding deleted for " + protocol + " port " + std::to_string(port));
}

IPAddress UPnP::GetExternalIP()
{
    if (serviceUrl_.empty())
        throw std::runtime_error("No UPnP service available or Discover() has not been called");

    // Create the SOAP request
    std::string soap = "<u:GetExternalIPAddress xmlns:u=\"urn:schemas-upnp-org:service:WANIPConnection:1\">"
                       "</u:GetExternalIPAddress>";

    // Send the SOAP request
    std::string response = SOAPRequest(serviceUrl_, soap, "GetExternalIPAddress");

    // Parse the response
    std::istringstream xml_stream(response);
    boost::property_tree::ptree pt;
    boost::property_tree::read_xml(xml_stream, pt);

    // Extract the IP address
    std::string ip = pt.get<std::string>("s:Envelope.s:Body.u:GetExternalIPAddressResponse.NewExternalIPAddress", "");
    if (ip.empty())
        throw std::runtime_error("Failed to get external IP address");

    return IPAddress(ip);
}

std::string UPnP::SOAPRequest(const std::string& url, const std::string& soap, const std::string& function)
{
    try
    {
        // Create the SOAP envelope
        std::string req = "<?xml version=\"1.0\"?>"
                          "<s:Envelope xmlns:s=\"http://schemas.xmlsoap.org/soap/envelope/\" "
                          "s:encodingStyle=\"http://schemas.xmlsoap.org/soap/encoding/\">"
                          "<s:Body>" +
                          soap +
                          "</s:Body>"
                          "</s:Envelope>";

        // Parse the URL
        std::string host;
        std::string port;
        std::string target;

        size_t protocol_end = url.find("://");
        if (protocol_end != std::string::npos)
        {
            size_t host_start = protocol_end + 3;
            size_t host_end = url.find(':', host_start);
            if (host_end != std::string::npos)
            {
                host = url.substr(host_start, host_end - host_start);
                size_t port_start = host_end + 1;
                size_t port_end = url.find('/', port_start);
                if (port_end != std::string::npos)
                {
                    port = url.substr(port_start, port_end - port_start);
                    target = url.substr(port_end);
                }
            }
            else
            {
                host_end = url.find('/', host_start);
                if (host_end != std::string::npos)
                {
                    host = url.substr(host_start, host_end - host_start);
                    port = "80";  // Default HTTP port
                    target = url.substr(host_end);
                }
            }
        }

        if (host.empty() || port.empty() || target.empty())
        {
            throw std::runtime_error("Invalid URL: " + url);
        }

        // Connect to the host
        boost::asio::io_context io_context;
        boost::beast::tcp_stream stream(io_context);
        boost::asio::ip::tcp::resolver resolver(io_context);
        auto const results = resolver.resolve(host, port);
        stream.connect(results);

        // Send the HTTP request
        boost::beast::http::request<boost::beast::http::string_body> http_req{boost::beast::http::verb::post, target,
                                                                              11};
        http_req.set(boost::beast::http::field::host, host);
        http_req.set(boost::beast::http::field::user_agent, BOOST_BEAST_VERSION_STRING);
        http_req.set(boost::beast::http::field::content_type, "text/xml; charset=\"utf-8\"");
        http_req.set("SOAPACTION", "\"urn:schemas-upnp-org:service:WANIPConnection:1#" + function + "\"");
        http_req.body() = req;
        http_req.prepare_payload();

        boost::beast::http::write(stream, http_req);

        // Receive the HTTP response
        boost::beast::flat_buffer buffer;
        boost::beast::http::response<boost::beast::http::string_body> res;
        boost::beast::http::read(stream, buffer, res);

        // Close the connection
        stream.socket().shutdown(boost::asio::ip::tcp::socket::shutdown_both);

        // Check the response status
        if (res.result() != boost::beast::http::status::ok)
        {
            throw std::runtime_error("HTTP error: " + std::to_string(static_cast<int>(res.result())));
        }

        return res.body();
    }
    catch (const std::exception& e)
    {
        logging::Logger::Instance().Error("Network", std::string("Error sending SOAP request: ") + e.what());
        throw;
    }
}
}  // namespace neo::network

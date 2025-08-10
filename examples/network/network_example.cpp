#include <iostream>
#include <neo/network/p2p/ip_endpoint.h>
#include <neo/network/p2p/message.h>
#include <neo/network/p2p/payloads/version_payload.h>

using namespace neo::network::p2p;

int main()
{
    // Create an IP endpoint
    IPEndPoint endpoint("127.0.0.1", 10333);

    // Print the endpoint
    std::cout << "Endpoint: " << endpoint.ToString() << std::endl;

    // Create a version payload and wrap in a message
    auto payload = std::make_shared<payloads::VersionPayload>();
    Message message(MessageCommand::Version, payload);

    // Print the message
    std::cout << "Message command: " << static_cast<int>(message.GetCommand()) << std::endl;
    // Print payload type
    std::cout << "Message has payload" << std::endl;

    return 0;
}

#include <iostream>
#include <neo/network/p2p/ip_endpoint.h>
#include <neo/network/p2p/message.h>

using namespace neo::network::p2p;

int main() {
    // Create an IP endpoint
    IPEndPoint endpoint("127.0.0.1", 10333);
    
    // Print the endpoint
    std::cout << "Endpoint: " << endpoint.ToString() << std::endl;
    
    // Create a message
    Message message(MessageCommand::Version, std::vector<uint8_t>{1, 2, 3, 4});
    
    // Print the message
    std::cout << "Message command: " << static_cast<int>(message.GetCommand()) << std::endl;
    std::cout << "Message payload size: " << message.GetPayload().size() << std::endl;
    
    return 0;
}

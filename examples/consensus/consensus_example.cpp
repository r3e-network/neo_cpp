#include <iostream>
#include <neo/consensus/consensus_message.h>
#include <neo/consensus/consensus_service.h>

using namespace neo::consensus;

int main() {
    // Create a consensus message
    ChangeViewMessage message;
    message.SetViewNumber(1);
    message.SetNewViewNumber(2);
    
    // Print the message
    std::cout << "Message type: " << static_cast<int>(message.GetType()) << std::endl;
    std::cout << "View number: " << message.GetViewNumber() << std::endl;
    std::cout << "New view number: " << message.GetNewViewNumber() << std::endl;
    
    return 0;
}

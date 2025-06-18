#include <neo/hardfork.h>
#include <stdexcept>
#include <string>

namespace neo
{
    const char* HardforkToString(Hardfork hardfork)
    {
        switch (hardfork)
        {
            case Hardfork::HF_Aspidochelone:
                return "HF_Aspidochelone";
            case Hardfork::HF_Basilisk:
                return "HF_Basilisk";
            case Hardfork::HF_Cockatrice:
                return "HF_Cockatrice";
            case Hardfork::HF_Domovoi:
                return "HF_Domovoi";
            case Hardfork::HF_Echidna:
                return "HF_Echidna";
            default:
                throw std::invalid_argument("Unknown hardfork value");
        }
    }

    Hardfork StringToHardfork(const std::string& str)
    {
        if (str == "HF_Aspidochelone")
            return Hardfork::HF_Aspidochelone;
        else if (str == "HF_Basilisk")
            return Hardfork::HF_Basilisk;
        else if (str == "HF_Cockatrice")
            return Hardfork::HF_Cockatrice;
        else if (str == "HF_Domovoi")
            return Hardfork::HF_Domovoi;
        else if (str == "HF_Echidna")
            return Hardfork::HF_Echidna;
        else
            throw std::invalid_argument("Invalid hardfork string: " + str);
    }
} 
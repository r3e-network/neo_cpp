#include <neo/core/fixed8.h>

#include <cstdlib>
#include <stdexcept>
#include <string>

namespace neo::core
{
std::string Fixed8::ToString() const
{
    return std::to_string(ToDouble());
}

Fixed8 Fixed8::Parse(const std::string& str)
{
    try
    {
        return Fixed8(std::stod(str));
    }
    catch (const std::exception&)
    {
        return Fixed8();
    }
}
}  // namespace neo::core

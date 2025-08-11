// Build support file to ensure neo_ledger library is not empty
// This prevents CMake build issues with empty libraries

namespace neo::ledger
{
// Build support function to provide a symbol
void ledger_dummy_function()
{
    // Empty implementation
}
}  // namespace neo::ledger
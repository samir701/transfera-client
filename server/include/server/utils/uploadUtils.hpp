#pragma once

namespace server::utils
{

    // Same range as Java: [DYNAMIC_STARTING_PORT, DYNAMIC_ENDING_PORT)
    // i.e. 49152 .. 65534 inclusive.
    int generateCode();

} // namespace p2p::utils
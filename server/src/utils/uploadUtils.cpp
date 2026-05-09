#include "server/utils/uploadUtils.hpp"

#include <random>

namespace server::utils
{

    int generateCode()
    {
        static constexpr int kDynamicStartingPort = 49152;
        static constexpr int kDynamicEndingPort = 65535;
        static constexpr int kSpan = kDynamicEndingPort - kDynamicStartingPort;

        thread_local std::mt19937 rng{std::random_device{}()};
        std::uniform_int_distribution<int> dist(0, kSpan - 1);
        return dist(rng) + kDynamicStartingPort;
    }

} // namespace server::utils
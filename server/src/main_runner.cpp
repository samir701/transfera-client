#include "server/service/fileController.hpp"

#include "server/logging.hpp"

#include <chrono>
#include <csignal>
#include <cstdlib>
#include <atomic>
#include <iostream>
#include <thread>

namespace
{
    std::atomic<bool> g_keepRunning{true};

    void onSignal(int sig)
    {
        server::log::info("received signal " + std::to_string(sig) + ", shutting down");
        g_keepRunning = false;
    }

    int readPortFromEnv()
    {
        constexpr int kDefaultPort = 8080;
        if (const char *env = std::getenv("PORT"); env && *env)
        {
            try
            {
                const int port = std::stoi(env);
                if (port > 0 && port <= 65535)
                    return port;
            }
            catch (...)
            {
            }
            server::log::error("Invalid PORT env, using default " + std::to_string(kDefaultPort));
        }
        return kDefaultPort;
    }
} // namespace

int main()
{
    server::log::init();

    const int port = readPortFromEnv();
    server::log::info("starting Transfera API PORT=" + std::to_string(port));

    std::signal(SIGINT, onSignal);
    std::signal(SIGTERM, onSignal);

    server::services::FileController controller(port);
    if (!controller.start())
    {
        server::log::error("Failed to start Transfera API on port " + std::to_string(port));
        return 1;
    }

    while (g_keepRunning)
        std::this_thread::sleep_for(std::chrono::milliseconds(200));

    controller.stop();
    server::log::info("Transfera API exited cleanly");
    return 0;
}

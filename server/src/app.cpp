#include "server/app.hpp"

#include "server/service/fileController.hpp"

#include <chrono>
#include <csignal>
#include <cstdlib>
#include <atomic>
#include <iostream>
#include <thread>

namespace server
{

    namespace
    {
        std::atomic<bool> g_keepRunning{true};

        void onSignal(int) { g_keepRunning = false; }

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
            }
            return kDefaultPort;
        }
    } // namespace

    void run()
    {
        const int port = readPortFromEnv();

        std::signal(SIGINT, onSignal);
        std::signal(SIGTERM, onSignal);

        services::FileController controller(port);
        if (!controller.start())
        {
            std::cerr << "Failed to start Transfera API on port " << port << '\n';
            return;
        }

        while (g_keepRunning)
            std::this_thread::sleep_for(std::chrono::milliseconds(200));

        controller.stop();
    }

} // namespace server

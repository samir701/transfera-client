#include "server/service/fileSharer.hpp"

#include <atomic>
#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <thread>

int main()
{
    namespace fs = std::filesystem;

    const fs::path tmp = fs::temp_directory_path() / "p2p_test_file.txt";
    {
        std::cout << tmp.string() << '\n';
        std::ofstream out(tmp);
        out << "hello from file sharer\n";
    }

    server::service::FileSharer sharer;
    const int port = sharer.offerFile(tmp.string());

    std::atomic<bool> ready{false};
    std::thread server_thread([&]
                              {
    ready = true;
    sharer.startFileServer(port); });

    while (!ready)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    std::cout << "Starting client to receive file...\n";
    // const std::string cmd =
    //     "nc 127.0.0.1 " + std::to_string(port) + " > /tmp/p2p_received.bin";
    // if (std::system(cmd.c_str()) != 0)
    // {
    //     std::cerr << "nc failed (install netcat: brew install netcat)\n";
    // }

    server_thread.join();
    return 0;
}
#include "server/service/fileSharer.hpp"
#include "server/utils/uploadUtils.hpp"

#include <arpa/inet.h>
#include <cerrno>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <netinet/in.h>
#include <sys/socket.h>
#include <thread>
#include <unistd.h>

namespace server::service
{
    // namespace
    // {
    void run_file_sender(int client_fd, std::string file_path)
    {
        std::cout << "Starting to send file: " << file_path << '\n';
        std::ifstream input(file_path, std::ios::binary);
        if (!input)
        {
            std::cerr << "Failed to open file: " << file_path << '\n';
            close(client_fd);
            return;
        }

        char buffer[8192];
        // input.read(buffer, sizeof(buffer));
        // const auto n = input.gcount();
        // std::cout << "Reading " << n << " bytes from file...\n";
        while (input.read(buffer, sizeof(buffer)) || input.gcount() > 0)
        {
            std::cout << "Read " << input.gcount() << " bytes from file...\n";
            std::size_t remaining = static_cast<std::size_t>(input.gcount());
            const char *p = buffer;
            while (remaining > 0)
            {
                std::cout << "Sending " << remaining << " bytes...\n";
                const ssize_t sent =
                    ::send(client_fd, p, remaining, 0);
                if (sent <= 0)
                {
                    std::cerr << "Error sending file: " << std::strerror(errno) << '\n';
                    close(client_fd);
                    return;
                }
                p += static_cast<std::size_t>(sent);
                remaining -= static_cast<std::size_t>(sent);
            }
        }
        std::cout << "Finished sending file: " << file_path << '\n';
        close(client_fd);
    }

    // } // namespace

    int FileSharer::offerFile(const std::string &filePath)
    {
        while (true)
        {
            const int port = server::utils::generateCode();
            if (available_files_.find(port) == available_files_.end())
            {
                available_files_.emplace(port, filePath);
                return port;
            }
        }
    }

    void FileSharer::startFileServer(int port)
    {
        const auto it = available_files_.find(port);
        if (it == available_files_.end())
        {
            std::cerr << "No file associated with port: " << port << '\n';
            return;
        }
        const std::string filePath = it->second;

        const int server_fd = ::socket(AF_INET, SOCK_STREAM, 0);
        if (server_fd < 0)
        {
            std::cerr << "Error starting file server on port " << port << ": "
                      << std::strerror(errno) << '\n';
            return;
        }

        int opt = 1;
        if (::setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) <
            0)
        {
            std::cerr << "Error starting file server on port " << port << ": "
                      << std::strerror(errno) << '\n';
            ::close(server_fd);
            return;
        }

        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = htonl(INADDR_ANY);
        addr.sin_port = htons(static_cast<uint16_t>(port));

        if (::bind(server_fd, reinterpret_cast<sockaddr *>(&addr), sizeof(addr)) <
            0)
        // reinterpret_cast is a C++ operator that allows you to convert one pointer type to another, even if they are unrelated. In this case, it is used to convert the pointer to sockaddr_in (which is a specific structure for IPv4 addresses) to a pointer to sockaddr (which is a more generic structure used in socket programming). This is necessary because the bind function expects a pointer to sockaddr, but we have a sockaddr_in structure that contains the necessary information for binding the socket.
        {
            std::cerr << "Error starting file server on port " << port << ": "
                      << std::strerror(errno) << '\n';
            ::close(server_fd);
            return;
        }

        if (::listen(server_fd, 1) < 0)
        {
            std::cerr << "Error starting file server on port " << port << ": "
                      << std::strerror(errno) << '\n';
            ::close(server_fd);
            return;
        }

        std::cout << "Ready to serve file '"
                  << std::filesystem::path(filePath).filename().string()
                  << "' on port " << port << '\n';

        sockaddr_in client_addr{};
        socklen_t client_len = sizeof(client_addr);
        const int client_fd =
            ::accept(server_fd, reinterpret_cast<sockaddr *>(&client_addr), &client_len);
        if (client_fd < 0)
        {
            std::cerr << "Failed to accept connection on port " << port << ": "
                      << std::strerror(errno) << '\n';
            ::close(server_fd);
            return;
        }

        char client_ip[INET_ADDRSTRLEN]{};
        const char *ip = ::inet_ntop(AF_INET, &client_addr.sin_addr, client_ip,
                                     sizeof(client_ip));
        std::cout << "Client connected: " << (ip ? ip : "?") << '\n';

        std::thread sender(run_file_sender, client_fd, filePath);
        sender.join();
        ::close(server_fd);
    }

} // namespace server::service
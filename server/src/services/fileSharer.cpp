#include "server/service/fileSharer.hpp"
#include "server/logging.hpp"
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
    void fileSenderHandler(int client_fd, std::string file_path, std::string download_name)
    {
        server::log::info("Starting to send file: " + file_path);
        std::ifstream input(file_path, std::ios::binary);
        if (!input)
        {
            server::log::error("Error sending file to client: failed to open file '" + file_path +
                               "'");
            ::close(client_fd);
            return;
        }
        // Send original display name (preserves extension for the downloader)
        const std::string header = "Filename: " + download_name + "\n";
        {
            const char *p = header.data();
            std::size_t remaining = header.size();
            while (remaining > 0)
            {
                const ssize_t sent = ::send(client_fd, p, remaining, 0);
                if (sent <= 0)
                {
                    server::log::error(std::string("Error sending file to client: ") +
                                       std::strerror(errno));
                    ::close(client_fd);
                    return;
                }
                p += static_cast<std::size_t>(sent);
                remaining -= static_cast<std::size_t>(sent);
            }
        }
        // Send file content in 4096-byte chunks
        char buffer[4096];
        while (input.read(buffer, sizeof(buffer)) || input.gcount() > 0)
        {
            std::size_t remaining = static_cast<std::size_t>(input.gcount());
            const char *p = buffer;
            while (remaining > 0)
            {
                const ssize_t sent = ::send(client_fd, p, remaining, 0);
                if (sent <= 0)
                {
                    server::log::error(std::string("Error sending file to client: ") +
                                       std::strerror(errno));
                    ::close(client_fd);
                    return;
                }
                p += static_cast<std::size_t>(sent);
                remaining -= static_cast<std::size_t>(sent);
            }
        }
        server::log::info("File '" + download_name + "' sent to client");
        ::close(client_fd);
    }
    // } // namespace

    bool FileSharer::tryGetDownloadName(int port, std::string &outName) const
    {
        const auto it = available_files_.find(port);
        if (it == available_files_.end())
            return false;
        outName = it->second.downloadName;
        return !outName.empty();
    }

    bool FileSharer::tryGetSharedFile(int port, SharedFile &outFile) const
    {
        const auto it = available_files_.find(port);
        if (it == available_files_.end())
            return false;
        outFile = it->second;
        return !outFile.path.empty();
    }

    int FileSharer::offerFile(const std::string &filePath, const std::string &downloadName)
    {
        while (true)
        {
            const int port = server::utils::generateCode();
            if (available_files_.find(port) == available_files_.end())
            {
                available_files_.emplace(port, SharedFile{filePath, downloadName});
                return port;
            }
        }
    }

    void FileSharer::startFileServer(int port)
    {
        const auto it = available_files_.find(port);
        if (it == available_files_.end())
        {
            server::log::error("No file associated with port: " + std::to_string(port));
            return;
        }
        const SharedFile &shared = it->second;
        const std::string &filePath = shared.path;
        const std::string &downloadName = shared.downloadName;

        const int server_fd = ::socket(AF_INET, SOCK_STREAM, 0);
        if (server_fd < 0)
        {
            server::log::error("Error starting file server on port " + std::to_string(port) +
                               ": " + std::strerror(errno));
            return;
        }

        int opt = 1;
        if (::setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) <
            0)
        {
            server::log::error("Error starting file server on port " + std::to_string(port) +
                               ": " + std::strerror(errno));
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
            server::log::error("Error starting file server on port " + std::to_string(port) +
                               ": " + std::strerror(errno));
            ::close(server_fd);
            return;
        }

        if (::listen(server_fd, 1) < 0)
        {
            server::log::error("Error starting file server on port " + std::to_string(port) +
                               ": " + std::strerror(errno));
            ::close(server_fd);
            return;
        }

        server::log::info("Ready to serve file '" + downloadName + "' on port " +
                          std::to_string(port));

        sockaddr_in client_addr{};
        socklen_t client_len = sizeof(client_addr);
        const int client_fd =
            ::accept(server_fd, reinterpret_cast<sockaddr *>(&client_addr), &client_len);
        if (client_fd < 0)
        {
            server::log::error("Failed to accept connection on port " + std::to_string(port) +
                               ": " + std::strerror(errno));
            ::close(server_fd);
            return;
        }

        char client_ip[INET_ADDRSTRLEN]{};
        const char *ip = ::inet_ntop(AF_INET, &client_addr.sin_addr, client_ip,
                                     sizeof(client_ip));
        server::log::info(std::string("Client connected: ") + (ip ? ip : "?"));

        std::thread sender(fileSenderHandler, client_fd, filePath, downloadName);
        sender.join();
        ::close(server_fd);
    }

} // namespace server::service
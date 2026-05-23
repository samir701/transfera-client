#include "server/service/fileSharer.hpp"
#include "server/logging.hpp"
#include "server/utils/uploadUtils.hpp"

#include <algorithm>
#include <arpa/inet.h>
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <utility>
#include <vector>
#include <filesystem>
#include <fstream>
#include <future>
#include <iostream>
#include <memory>
#include <netinet/in.h>
#include <sstream>
#include <sys/socket.h>
#include <thread>
#include <unistd.h>

namespace server::service
{
    namespace
    {
        bool recvLine(int fd, std::string &outLine)
        {
            outLine.clear();
            char c = 0;
            while (true)
            {
                const ssize_t n = ::recv(fd, &c, 1, 0);
                if (n == 0)
                    return false;
                if (n < 0)
                    return false;
                if (c == '\n')
                    return true;
                outLine.push_back(c);
            }
        }

        bool recvAllToString(int fd, std::string &out)
        {
            out.clear();
            char buffer[4096];
            while (true)
            {
                const ssize_t n = ::recv(fd, buffer, sizeof(buffer), 0);
                if (n == 0)
                    break;
                if (n < 0)
                    return false;
                out.append(buffer, static_cast<std::size_t>(n));
            }
            return true;
        }

        bool fileSenderHandler(int client_fd, const std::string &file_path,
                               const std::string &download_name)
        {
            server::log::info("P2P send: opening " + file_path);
            std::ifstream input(file_path, std::ios::binary);
            if (!input)
            {
                server::log::error("P2P send: failed to open '" + file_path + "'");
                ::close(client_fd);
                return false;
            }

            const std::string header = "Filename: " + download_name + "\n";
            {
                const char *p = header.data();
                std::size_t remaining = header.size();
                while (remaining > 0)
                {
                    const ssize_t sent = ::send(client_fd, p, remaining, 0);
                    if (sent <= 0)
                    {
                        server::log::error(std::string("P2P send: header failed: ") +
                                           std::strerror(errno));
                        ::close(client_fd);
                        return false;
                    }
                    p += static_cast<std::size_t>(sent);
                    remaining -= static_cast<std::size_t>(sent);
                }
            }

            char buffer[4096];
            std::size_t total = 0;
            while (input.read(buffer, sizeof(buffer)) || input.gcount() > 0)
            {
                std::size_t remaining = static_cast<std::size_t>(input.gcount());
                const char *p = buffer;
                while (remaining > 0)
                {
                    const ssize_t sent = ::send(client_fd, p, remaining, 0);
                    if (sent <= 0)
                    {
                        server::log::error(std::string("P2P send: body failed: ") +
                                           std::strerror(errno));
                        ::close(client_fd);
                        return false;
                    }
                    p += static_cast<std::size_t>(sent);
                    remaining -= static_cast<std::size_t>(sent);
                    total += static_cast<std::size_t>(sent);
                }
            }
            server::log::info("P2P send: complete name='" + download_name +
                              "' bytes=" + std::to_string(total));
            ::close(client_fd);
            return true;
        }

    } // namespace

    FileSharer::FileSharer() = default;

    FileSharer::~FileSharer() { shutdown(); }

    void FileSharer::shutdown()
    {
        stopExpirySweeper_ = true;
        if (expirySweeperThread_.joinable())
            expirySweeperThread_.join();
    }

    std::chrono::seconds FileSharer::inviteTtl()
    {
        constexpr int kDefaultSeconds = 300; // 5 minutes
        if (const char *v = std::getenv("TRANSFERA_INVITE_TTL_SEC"); v && *v)
        {
            try
            {
                const long sec = std::stol(v);
                if (sec > 0)
                    return std::chrono::seconds(sec);
            }
            catch (...)
            {
            }
        }
        return std::chrono::seconds(kDefaultSeconds);
    }

    std::chrono::seconds FileSharer::expirySweepInterval()
    {
        return std::chrono::seconds(15);
    }

    void FileSharer::ensureExpirySweeperStarted()
    {
        if (expirySweeperStarted_.exchange(true))
            return;

        stopExpirySweeper_ = false;
        expirySweeperThread_ = std::thread([this] { expirySweeperLoop(); });
        if (server::log::verboseEnabled())
        {
            server::log::info("invite expiry sweeper started (ttl=" +
                              std::to_string(inviteTtl().count()) + "s)");
        }
    }

    void FileSharer::expirySweeperLoop()
    {
        while (!stopExpirySweeper_.load())
        {
            purgeExpiredInvites();
            const auto step = expirySweepInterval();
            for (int i = 0; i < step.count() && !stopExpirySweeper_.load(); ++i)
                std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }

    bool FileSharer::isInviteExpired(const SharedFile &file) const
    {
        if (file.downloadCount > 0)
            return false;
        const auto now = std::chrono::steady_clock::now();
        return (now - file.offeredAt) >= inviteTtl();
    }

    void FileSharer::purgeExpiredInvites()
    {
        std::vector<std::pair<int, std::string>> expired;
        {
            std::lock_guard lock(mapMutex_);
            const auto now = std::chrono::steady_clock::now();
            const auto ttl = inviteTtl();
            for (const auto &[port, file] : available_files_)
            {
                if (file.downloadCount > 0)
                    continue;
                if (file.transferInProgress)
                    continue;
                if ((now - file.offeredAt) >= ttl)
                    expired.emplace_back(port, file.path);
            }
        }

        for (const auto &[port, path] : expired)
            expireIdleInvite(port, path);
    }

    void FileSharer::expireIdleInvite(int port, const std::string &filePath)
    {
        {
            std::lock_guard lock(mapMutex_);
            const auto it = available_files_.find(port);
            if (it == available_files_.end())
                return;
            if (it->second.downloadCount > 0 || it->second.transferInProgress)
                return;
        }

        server::log::info("P2P invite expired invite_port=" + std::to_string(port) +
                          " (no download within " + std::to_string(inviteTtl().count()) +
                          "s)");
        consumeInvite(port, filePath);
    }

    std::mutex &FileSharer::transferMutexForPort(int port)
    {
        std::lock_guard lock(mapMutex_);
        auto &slot = portTransferMutexes_[port];
        if (!slot)
            slot = std::make_unique<std::mutex>();
        return *slot;
    }

    bool FileSharer::tryGetDownloadName(int port, std::string &outName) const
    {
        std::lock_guard lock(mapMutex_);
        const auto it = available_files_.find(port);
        if (it == available_files_.end())
            return false;
        outName = it->second.downloadName;
        return !outName.empty();
    }

    bool FileSharer::tryGetSharedFile(int port, SharedFile &outFile) const
    {
        std::lock_guard lock(mapMutex_);
        const auto it = available_files_.find(port);
        if (it == available_files_.end())
            return false;
        outFile = it->second;
        return !outFile.path.empty();
    }

    bool FileSharer::claimInviteForTransfer(int port, SharedFile &outFile, std::string &outError)
    {
        purgeExpiredInvites();

        std::string pathToExpire;
        bool claimed = false;
        {
            std::lock_guard lock(mapMutex_);
            const auto it = available_files_.find(port);
            if (it == available_files_.end())
            {
                outError = "invalid or expired invite code";
            }
            else if (isInviteExpired(it->second))
            {
                outError = "invalid or expired invite code";
                if (!it->second.transferInProgress)
                    pathToExpire = it->second.path;
            }
            else if (it->second.downloadCount >= it->second.maxDownloads)
            {
                outError = "invalid or expired invite code";
            }
            else if (it->second.transferInProgress)
            {
                outError = "download already in progress";
            }
            else
            {
                it->second.transferInProgress = true;
                outFile = it->second;
                claimed = true;
            }
        }

        if (!pathToExpire.empty())
            expireIdleInvite(port, pathToExpire);
        return claimed;
    }

    void FileSharer::releaseInviteClaim(int port)
    {
        std::lock_guard lock(mapMutex_);
        const auto it = available_files_.find(port);
        if (it != available_files_.end())
            it->second.transferInProgress = false;
    }

    void FileSharer::consumeInvite(int port, const std::string &filePath)
    {
        {
            std::lock_guard lock(mapMutex_);
            available_files_.erase(port);
            portTransferMutexes_.erase(port);
        }

        std::error_code ec;
        std::filesystem::remove(filePath, ec);
        if (ec && server::log::verboseEnabled())
        {
            server::log::info("P2P invite consumed invite_port=" + std::to_string(port) +
                              " (file remove: " + ec.message() + ")");
        }
        else
        {
            server::log::info("P2P invite consumed invite_port=" + std::to_string(port) +
                              " connection closed");
        }
    }

    void FileSharer::recordSuccessfulDownload(int port, const std::string &filePath,
                                              int &outDownloadsRemaining)
    {
        std::string pathToRemove = filePath;
        bool removeInvite = false;
        int completed = 0;
        int maxAllowed = 1;

        {
            std::lock_guard lock(mapMutex_);
            const auto it = available_files_.find(port);
            if (it == available_files_.end())
            {
                outDownloadsRemaining = 0;
                return;
            }
            it->second.transferInProgress = false;
            it->second.downloadCount += 1;
            completed = it->second.downloadCount;
            maxAllowed = it->second.maxDownloads;
            outDownloadsRemaining = std::max(0, maxAllowed - completed);
            if (completed >= maxAllowed)
            {
                removeInvite = true;
                pathToRemove = it->second.path;
            }
        }

        if (server::log::verboseEnabled())
        {
            server::log::info("P2P download recorded invite_port=" + std::to_string(port) +
                              " count=" + std::to_string(completed) + "/" +
                              std::to_string(maxAllowed) + " remaining=" +
                              std::to_string(outDownloadsRemaining));
        }

        if (removeInvite)
            consumeInvite(port, pathToRemove);
    }

    int FileSharer::offerFile(const std::string &filePath, const std::string &downloadName,
                              int maxDownloads)
    {
        if (maxDownloads < 1)
            maxDownloads = 1;

        ensureExpirySweeperStarted();
        purgeExpiredInvites();

        std::lock_guard lock(mapMutex_);
        while (true)
        {
            const int port = server::utils::generateCode();
            if (available_files_.find(port) == available_files_.end())
            {
                SharedFile entry;
                entry.path = filePath;
                entry.downloadName = downloadName;
                entry.maxDownloads = maxDownloads;
                entry.offeredAt = std::chrono::steady_clock::now();
                available_files_.emplace(port, std::move(entry));
                if (server::log::verboseEnabled())
                {
                    server::log::info("P2P offer: invite_port=" + std::to_string(port) +
                                      " max_downloads=" + std::to_string(maxDownloads) +
                                      " ttl_sec=" + std::to_string(inviteTtl().count()) +
                                      " path=" + filePath + " name=" + downloadName);
                }
                return port;
            }
        }
    }

    bool FileSharer::receiveViaLocalP2P(int port, std::string &outFilename,
                                        std::string &outBody, std::string &outError,
                                        int *outDownloadsRemaining)
    {
        SharedFile shared;
        if (!claimInviteForTransfer(port, shared, outError))
            return false;

        if (!std::filesystem::exists(shared.path) ||
            !std::filesystem::is_regular_file(shared.path))
        {
            releaseInviteClaim(port);
            outError = "file no longer on server";
            return false;
        }

        std::lock_guard portLock(transferMutexForPort(port));

        struct ClaimGuard
        {
            FileSharer &sharer;
            int invitePort;
            bool keep{false};
            ~ClaimGuard()
            {
                if (!keep)
                    sharer.releaseInviteClaim(invitePort);
            }
        } claimGuard{*this, port};

        server::log::info("P2P download start invite_port=" + std::to_string(port));

        int fds[2]{-1, -1};
        if (::socketpair(AF_UNIX, SOCK_STREAM, 0, fds) < 0)
        {
            outError = std::string("socketpair(): ") + std::strerror(errno);
            return false;
        }

        const int recvFd = fds[0];
        const int sendFd = fds[1];

        server::log::info("P2P channel ready invite_port=" + std::to_string(port));

        bool sendOk = false;
        std::thread senderThread(
            [&]()
            { sendOk = fileSenderHandler(sendFd, shared.path, shared.downloadName); });

        std::string filename =
            shared.downloadName.empty() ? "downloaded-file" : shared.downloadName;

        std::string header;
        if (!recvLine(recvFd, header))
        {
            outError = "failed to read filename header";
            ::close(recvFd);
            if (senderThread.joinable())
                senderThread.join();
            return false;
        }
        if (!header.empty() && header.back() == '\r')
            header.pop_back();

        const std::string prefix = "Filename: ";
        if (header.rfind(prefix, 0) == 0)
        {
            const std::string fromPeer = header.substr(prefix.size());
            if (!fromPeer.empty())
                filename = fromPeer;
        }

        server::log::info("P2P receiving invite_port=" + std::to_string(port) + " name=" +
                          filename);

        if (!recvAllToString(recvFd, outBody))
        {
            outError = "failed while reading P2P stream";
            ::close(recvFd);
            if (senderThread.joinable())
                senderThread.join();
            return false;
        }
        ::close(recvFd);

        if (senderThread.joinable())
            senderThread.join();

        if (!sendOk)
        {
            outError = "P2P sender failed";
            return false;
        }

        outFilename = filename;
        claimGuard.keep = true;
        int remaining = 0;
        recordSuccessfulDownload(port, shared.path, remaining);
        if (outDownloadsRemaining)
            *outDownloadsRemaining = remaining;
        server::log::info("P2P download complete invite_port=" + std::to_string(port) +
                          " name=" + filename + " bytes=" + std::to_string(outBody.size()) +
                          " remaining=" + std::to_string(remaining));
        return true;
    }

    void FileSharer::startFileServer(int port)
    {
        SharedFile shared;
        std::string err;
        if (!claimInviteForTransfer(port, shared, err))
        {
            server::log::error("remote P2P port " + std::to_string(port) + ": " + err);
            return;
        }

        const int server_fd = ::socket(AF_INET, SOCK_STREAM, 0);
        if (server_fd < 0)
            return;

        int opt = 1;
        ::setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = htonl(INADDR_ANY);
        addr.sin_port = htons(static_cast<uint16_t>(port));

        if (::bind(server_fd, reinterpret_cast<sockaddr *>(&addr), sizeof(addr)) < 0 ||
            ::listen(server_fd, 1) < 0)
        {
            ::close(server_fd);
            releaseInviteClaim(port);
            server::log::error("remote P2P bind/listen failed port " + std::to_string(port));
            return;
        }

        server::log::info("remote P2P ready invite_port=" + std::to_string(port));
        sockaddr_in client_addr{};
        socklen_t client_len = sizeof(client_addr);
        const int client_fd =
            ::accept(server_fd, reinterpret_cast<sockaddr *>(&client_addr), &client_len);
        ::close(server_fd);
        if (client_fd >= 0)
        {
            if (fileSenderHandler(client_fd, shared.path, shared.downloadName))
            {
                int remaining = 0;
                recordSuccessfulDownload(port, shared.path, remaining);
            }
            else
                releaseInviteClaim(port);
        }
        else
            releaseInviteClaim(port);
    }

} // namespace server::service

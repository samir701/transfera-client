#include "server/service/fileController.hpp"

#include <chrono>
#include <cstdlib>
#include <iostream>
#include <fstream>
#include <random>
#include <thread>
#include <sstream>
#include <stdexcept>

#include <arpa/inet.h>
#include <cerrno>
#include <cstring>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

namespace fs = std::filesystem;

namespace server::services
{

    namespace
    {
        fs::path getTempDir()
        {
            if (const char *t = std::getenv("TMPDIR"); t && *t)
                return fs::path(t);
            return fs::path("/tmp");
        }

        std::string randomHex(std::size_t len)
        {
            static const char *kHex = "0123456789abcdef";
            thread_local std::mt19937 rng{std::random_device{}()};
            std::uniform_int_distribution<int> dist(0, 15);

            std::string s;
            s.reserve(len);
            for (std::size_t i = 0; i < len; ++i)
                s.push_back(kHex[dist(rng)]);
            return s;
        }

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

        bool recvAllToFile(int fd, const fs::path &outFile)
        {
            std::ofstream fos(outFile, std::ios::binary);
            if (!fos)
                return false;

            char buffer[4096];
            while (true)
            {
                const ssize_t n = ::recv(fd, buffer, sizeof(buffer), 0);
                if (n == 0)
                    break;
                if (n < 0)
                    return false;
                fos.write(buffer, n);
                if (!fos)
                    return false;
            }
            return true;
        }

    } // namespace

    FileController::FileController(int port)
        : port_(port),
          uploadDir_(getTempDir() / "transfera-uploads")
    {
        std::error_code ec;
        fs::create_directories(uploadDir_, ec);

        // Use httplib default thread pool (avoids custom pool edge cases under load)
        server_.set_read_timeout(120, 0);
        server_.set_write_timeout(120, 0);

        registerRoutes();
    }

    FileController::~FileController() { stop(); }

    bool FileController::start()
    {
        if (running_.exchange(true))
            return listen_ok_.load();

        listen_ok_ = false;

        serverThread_ = std::thread([this]
                                    {
                                        if (!server_.bind_to_port("0.0.0.0", port_))
                                        {
                                            std::cerr << "ERROR: Could not bind to port " << port_
                                                      << " (is another server already running?)\n";
                                            std::cerr << "  Fix: lsof -ti :" << port_
                                                      << " | xargs kill -9\n";
                                            running_ = false;
                                            return;
                                        }

                                        listen_ok_ = true;
                                        if (!server_.listen_after_bind())
                                        {
                                            std::cerr << "ERROR: API server stopped on port "
                                                      << port_ << '\n';
                                        }
                                        listen_ok_ = false;
                                        running_ = false; });

        for (int i = 0; i < 50 && !listen_ok_.load(); ++i)
            std::this_thread::sleep_for(std::chrono::milliseconds(20));

        if (!listen_ok_.load())
        {
            running_ = false;
            server_.stop();
            if (serverThread_.joinable())
                serverThread_.join();
            return false;
        }

        std::cout << "Transfera API on http://0.0.0.0:" << port_ << '\n';
        std::cout << "  GET  http://0.0.0.0:" << port_ << "/api/health\n";
        std::cout << "  POST http://0.0.0.0:" << port_ << "/api/upload\n";
        std::cout << "  GET  http://0.0.0.0:" << port_ << "/api/download/<port>\n";
        return true;
    }

    void FileController::stop()
    {
        if (!running_.exchange(false))
        {
            if (serverThread_.joinable())
                serverThread_.join();
            return;
        }

        server_.stop();
        if (serverThread_.joinable())
            serverThread_.join();
        std::cout << "API server stopped\n";
    }

    void FileController::registerRoutes()
    {
        server_.Options(R"(/.*)", [this](const httplib::Request &req, httplib::Response &res)
                        { handleCorsOrNotFound(req, res); });

        // Health check for frontend / ops
        server_.Get("/api/health", [this](const httplib::Request &, httplib::Response &res)
                    {
                          applyCorsHeaders(res);
                          res.status = 200;
                          res.set_content(R"({"status":"ok"})", "application/json"); });

        // Routes match Next.js client: /api/upload, /api/download/:port
        server_.Post("/api/upload", [this](const httplib::Request &req, httplib::Response &res)
                     { handleUpload(req, res); });

        server_.Get(R"(/api/download/(\d+))", [this](const httplib::Request &req, httplib::Response &res)
                    { handleDownload(req, res); });

        // Legacy paths (Java-style)
        server_.Post("/upload", [this](const httplib::Request &req, httplib::Response &res)
                     { handleUpload(req, res); });

        server_.Get(R"(/download/(\d+))", [this](const httplib::Request &req, httplib::Response &res)
                    { handleDownload(req, res); });

        server_.set_error_handler([this](const httplib::Request &req, httplib::Response &res)
                                  {
                                      if (res.status == 404)
                                          handleCorsOrNotFound(req, res);
                                      else
                                          applyCorsHeaders(res); });
    }

    void FileController::applyCorsHeaders(httplib::Response &res) const
    {
        res.set_header("Access-Control-Allow-Origin", "*");
        res.set_header("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
        res.set_header("Access-Control-Allow-Headers",
                       "Content-Type, Authorization, Accept, X-Requested-With");
        res.set_header("Access-Control-Max-Age", "86400");
        // Required for axios/browser to read filename on cross-origin download (e.g. :3000 -> :8080)
        res.set_header("Access-Control-Expose-Headers",
                       "Content-Disposition, X-Filename, Content-Type, Content-Length");
    }

    void FileController::handleCorsOrNotFound(const httplib::Request &req,
                                              httplib::Response &res) const
    {
        applyCorsHeaders(res);

        if (req.method == "OPTIONS")
        {
            res.status = 204;
            return;
        }

        res.status = 404;
        res.set_content("Not Found", "text/plain");
    }

    std::string FileController::sanitizeDisplayFilename(const std::string &originalFilename)
    {
        fs::path base = fs::path(originalFilename).filename();
        std::string name = base.string();
        if (name.empty() || name == "." || name == "..")
            return "unnamed-file";

        // Hidden files like ".env" — keep leading dot as stem
        if (base.extension().empty() && !name.empty() && name.front() == '.')
            return name;

        return name;
    }

    std::string FileController::makeUniqueName(const std::string &originalFilename)
    {
        const std::string display = sanitizeDisplayFilename(originalFilename);
        const fs::path base = fs::path(display);
        const std::string ext = base.extension().string();
        std::string stem = base.stem().string();
        if (stem.empty())
            stem = "unnamed-file";
        return randomHex(32) + "_" + stem + ext;
    }

    void FileController::handleUpload(const httplib::Request &req, httplib::Response &res)
    {
        applyCorsHeaders(res);

        if (req.method != "POST")
        {
            res.status = 405;
            res.set_content("Method Not Allowed", "text/plain");
            return;
        }

        const std::string contentType = req.get_header_value("Content-Type");
        if (contentType.empty() ||
            contentType.find("multipart/form-data") == std::string::npos)
        {
            res.status = 400;
            res.set_content("Bad Request: Content-Type must be multipart/form-data", "text/plain");
            return;
        }

        try
        {
            if (!req.form.has_file("file"))
            {
                res.status = 400;
                res.set_content(
                    "Bad Request: missing multipart field 'file' (field name must be \"file\")",
                    "text/plain");
                return;
            }

            const httplib::FormData uploaded = req.form.get_file("file");
            if (uploaded.content.empty())
            {
                res.status = 400;
                res.set_content("Bad Request: empty file upload", "text/plain");
                return;
            }

            std::string filename = uploaded.filename;

            const std::string displayName = sanitizeDisplayFilename(filename);

            const std::string uniqueFilename = makeUniqueName(displayName);
            const fs::path filePath = uploadDir_ / uniqueFilename;

            {
                std::ofstream fos(filePath, std::ios::binary);
                fos.write(uploaded.content.data(),
                          static_cast<std::streamsize>(uploaded.content.size()));
                if (!fos)
                    throw std::runtime_error("failed to write uploaded file");
            }

            const int port = fileSharer_.offerFile(filePath.string(), displayName);

            std::thread([this, port]()
                        { fileSharer_.startFileServer(port); })
                .detach();

            const std::string jsonResponse = "{\"port\": " + std::to_string(port) + "}";
            res.status = 200;
            res.set_content(jsonResponse, "application/json");
        }
        catch (const std::exception &e)
        {
            std::cerr << "Error processing file upload: " << e.what() << '\n';
            res.status = 500;
            res.set_content(std::string("Server error: ") + e.what(), "text/plain");
        }
    }
    bool FileController::parsePortFromPath(const std::string &path, int &outPort)
    {
        const auto slash = path.find_last_of('/');
        if (slash == std::string::npos || slash + 1 >= path.size())
            return false;

        try
        {
            outPort = std::stoi(path.substr(slash + 1));
            return outPort > 0 && outPort <= 65535;
        }
        catch (...)
        {
            return false;
        }
    }

    void FileController::handleDownload(const httplib::Request &req, httplib::Response &res)
    {
        applyCorsHeaders(res);

        if (req.method != "GET")
        {
            res.status = 405;
            res.set_content("Method Not Allowed", "text/plain");
            return;
        }

        int peerPort = 0;
        if (!parsePortFromPath(req.path, peerPort))
        {
            res.status = 400;
            res.set_content("Bad Request: Invalid port number", "text/plain");
            return;
        }

        try
        {
            const int sock = ::socket(AF_INET, SOCK_STREAM, 0);
            if (sock < 0)
                throw std::runtime_error("socket() failed");

            sockaddr_in addr{};
            addr.sin_family = AF_INET;
            addr.sin_port = htons(static_cast<uint16_t>(peerPort));
            if (::inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr) != 1)
            {
                ::close(sock);
                throw std::runtime_error("inet_pton failed");
            }

            if (::connect(sock, reinterpret_cast<sockaddr *>(&addr), sizeof(addr)) < 0)
            {
                ::close(sock);
                throw std::runtime_error(std::strerror(errno));
            }

            std::string filename = "downloaded-file";
            std::string lookupName;
            if (fileSharer_.tryGetDownloadName(peerPort, lookupName))
                filename = lookupName;

            std::string header;
            if (!recvLine(sock, header))
            {
                ::close(sock);
                throw std::runtime_error("failed to read filename header");
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

            const fs::path tempFile =
                fs::temp_directory_path() / ("download-" + randomHex(8) + ".tmp");

            if (!recvAllToFile(sock, tempFile))
            {
                ::close(sock);
                std::error_code ec;
                fs::remove(tempFile, ec);
                throw std::runtime_error("failed while reading peer stream");
            }
            ::close(sock);

            std::ifstream fis(tempFile, std::ios::binary);
            if (!fis)
            {
                std::error_code ec;
                fs::remove(tempFile, ec);
                throw std::runtime_error("failed to open temp file");
            }

            std::ostringstream buffer;
            buffer << fis.rdbuf();

            res.status = 200;
            res.set_header("Content-Disposition",
                           "attachment; filename=\"" + filename + "\"");
            res.set_header("X-Filename", filename);
            res.set_content(buffer.str(), "application/octet-stream");

            std::error_code ec;
            fs::remove(tempFile, ec);
        }
        catch (const std::exception &e)
        {
            std::cerr << "Error downloading file from peer: " << e.what() << '\n';
            res.status = 500;
            res.set_content(std::string("Error downloading file: ") + e.what(), "text/plain");
        }
    }

} // namespace server::services
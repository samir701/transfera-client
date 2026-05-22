#include "server/service/fileController.hpp"

#include <cstdlib>
#include <iostream>
#include <fstream>
#include <optional>
#include <random>
#include <sstream>
#include <stdexcept>
#include <vector>

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

        struct ParseResult
        {
            std::string filename;
            std::string contentType;
            std::string fileContent;
        };

        class MultipartParser
        {
        public:
            MultipartParser(std::string data, std::string boundary)
                : data_(std::move(data)), boundary_(std::move(boundary)) {}

            std::optional<ParseResult> parse() const
            {
                try
                {
                    const std::string &dataAsString = data_;

                    const std::string filenameMarker = "filename=\"";
                    std::size_t filenameStart = dataAsString.find(filenameMarker);
                    if (filenameStart == std::string::npos)
                        return std::nullopt;

                    filenameStart += filenameMarker.size();
                    const std::size_t filenameEnd = dataAsString.find('"', filenameStart);
                    if (filenameEnd == std::string::npos)
                        return std::nullopt;

                    const std::string filename =
                        dataAsString.substr(filenameStart, filenameEnd - filenameStart);

                    const std::string contentTypeMarker = "Content-Type: ";
                    std::size_t contentTypeStart =
                        dataAsString.find(contentTypeMarker, filenameEnd);
                    std::string contentType = "application/octet-stream";

                    if (contentTypeStart != std::string::npos)
                    {
                        contentTypeStart += contentTypeMarker.size();
                        const std::size_t contentTypeEnd =
                            dataAsString.find("\r\n", contentTypeStart);
                        if (contentTypeEnd != std::string::npos)
                        {
                            contentType = dataAsString.substr(
                                contentTypeStart, contentTypeEnd - contentTypeStart);
                        }
                    }

                    const std::string headerEndMarker = "\r\n\r\n";
                    const std::size_t headerEnd = dataAsString.find(headerEndMarker);
                    if (headerEnd == std::string::npos)
                        return std::nullopt;

                    const std::size_t contentStart = headerEnd + headerEndMarker.size();

                    std::string closing = "\r\n--" + boundary_ + "--";
                    std::vector<unsigned char> boundaryBytes(closing.begin(), closing.end());
                    std::size_t contentEnd =
                        findSequence(data_, boundaryBytes, contentStart);

                    if (contentEnd == std::string::npos)
                    {
                        closing = "\r\n--" + boundary_;
                        boundaryBytes.assign(closing.begin(), closing.end());
                        contentEnd = findSequence(data_, boundaryBytes, contentStart);
                    }

                    if (contentEnd == std::string::npos || contentEnd <= contentStart)
                        return std::nullopt;

                    return ParseResult{
                        filename,
                        contentType,
                        data_.substr(contentStart, contentEnd - contentStart),
                    };
                }
                catch (const std::exception &e)
                {
                    std::cerr << "Error parsing multipart data: " << e.what() << '\n';
                    return std::nullopt;
                }
            }

        private:
            static std::size_t findSequence(const std::string &data,
                                            const std::vector<unsigned char> &sequence,
                                            std::size_t startPos)
            {
                if (sequence.empty() || startPos >= data.size())
                    return std::string::npos;

                for (std::size_t i = startPos; i + sequence.size() <= data.size(); ++i)
                {
                    bool match = true;
                    for (std::size_t j = 0; j < sequence.size(); ++j)
                    {
                        if (static_cast<unsigned char>(data[i + j]) != sequence[j])
                        {
                            match = false;
                            break;
                        }
                    }
                    if (match)
                        return i;
                }
                return std::string::npos;
            }

            std::string data_;
            std::string boundary_;
        };

        std::string extractBoundary(const std::string &contentType)
        {
            const std::string key = "boundary=";
            const auto pos = contentType.find(key);
            if (pos == std::string::npos)
                return {};

            std::string boundary = contentType.substr(pos + key.size());

            const auto semi = boundary.find(';');
            if (semi != std::string::npos)
                boundary = boundary.substr(0, semi);

            while (!boundary.empty() && (boundary.front() == ' ' || boundary.front() == '"'))
                boundary.erase(boundary.begin());
            while (!boundary.empty() && (boundary.back() == ' ' || boundary.back() == '"'))
                boundary.pop_back();

            return boundary;
        }

    } // namespace

    FileController::FileController(int port)
        : port_(port),
          uploadDir_(getTempDir() / "peerlink-uploads")
    {
        std::error_code ec;
        fs::create_directories(uploadDir_, ec);

        // Equivalent to Executors.newFixedThreadPool(10)
        server_.new_task_queue = []
        { return new httplib::ThreadPool(10); };

        // Equivalent to createContext("/upload"...), "/download", "/"
        registerRoutes();
    }

    FileController::~FileController() { stop(); }

    void FileController::start()
    {
        if (running_.exchange(true))
            return;

        serverThread_ = std::thread([this]
                                    {
                                        if (!server_.listen("0.0.0.0", port_))
                                        {
                                            std::cerr << "Failed to start API server on port " << port_ << '\n';
                                        }
                                        running_ = false; });

        std::cout << "PeerLink API listening on http://0.0.0.0:" << port_ << '\n';
        std::cout << "  POST http://localhost:" << port_ << "/api/upload\n";
        std::cout << "  GET  http://localhost:" << port_ << "/api/download/<port>\n";
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
                          res.set_content(R"({"status":"ok"})", "application/json");
                      });

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

    std::string FileController::makeUniqueName(const std::string &originalFilename)
    {
        const fs::path base = fs::path(originalFilename).filename();
        return randomHex(32) + "_" + base.string();
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
            std::string filename;
            std::string fileBytes;

            // Prefer httplib multipart parsing (reliable for browser uploads)
            if (req.form.has_file("file"))
            {
                const httplib::FormData uploaded = req.form.get_file("file");
                filename = uploaded.filename;
                fileBytes = uploaded.content;
            }
            else
            {
                const std::string boundary = extractBoundary(contentType);
                if (boundary.empty())
                {
                    res.status = 400;
                    res.set_content("Bad Request: missing multipart boundary", "text/plain");
                    return;
                }

                MultipartParser parser(req.body, boundary);
                const std::optional<ParseResult> result = parser.parse();

                if (!result)
                {
                    res.status = 400;
                    res.set_content("Bad Request: Could not parse file content", "text/plain");
                    return;
                }

                filename = result->filename;
                fileBytes = result->fileContent;
            }

            if (filename.empty())
                filename = "unnamed-file";

            const std::string uniqueFilename = makeUniqueName(filename);
            const fs::path filePath = uploadDir_ / uniqueFilename;

            {
                std::ofstream fos(filePath, std::ios::binary);
                fos.write(fileBytes.data(),
                          static_cast<std::streamsize>(fileBytes.size()));
                if (!fos)
                    throw std::runtime_error("failed to write uploaded file");
            }

            const int port = fileSharer_.offerFile(filePath.string());

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

            std::string header;
            if (!recvLine(sock, header))
            {
                ::close(sock);
                throw std::runtime_error("failed to read filename header");
            }
            if (!header.empty() && header.back() == '\r')
                header.pop_back();

            std::string filename = "downloaded-file";
            const std::string prefix = "Filename: ";
            if (header.rfind(prefix, 0) == 0)
                filename = header.substr(prefix.size());

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
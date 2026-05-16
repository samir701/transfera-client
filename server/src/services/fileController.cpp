#include "server/service/fileController.hpp"

#include <cstdlib>
#include <iostream>
#include <fstream>
#include <random>
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
                                        std::cout << "API server started on port " << port_ << '\n';
                                        if (!server_.listen("0.0.0.0", port_))
                                        {
                                            std::cerr << "Failed to start API server on port " << port_ << '\n';
                                        }
                                        running_ = false; });
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
        // Handle OPTIONS globally (preflight)
        server_.Options(R"(/.*)", [this](const httplib::Request &req, httplib::Response &res)
                        { handleCorsOrNotFound(req, res); });

        server_.Post("/upload", [this](const httplib::Request &req, httplib::Response &res)
                     { handleUpload(req, res); });

        // server_.Get(R"(/download/(\d+))", [this](const httplib::Request &req, httplib::Response &res)
        //             { handleDownload(req, res); });

        server_.set_error_handler([this](const httplib::Request &req, httplib::Response &res)
                                  {
                                      if (res.status == 404)
                                      {
                                          handleCorsOrNotFound(req, res);
                                      }
                                      else
                                      {
                                          applyCorsHeaders(res);
                                      } });
    }

    void FileController::applyCorsHeaders(httplib::Response &res) const
    {
        res.set_header("Access-Control-Allow-Origin", "*");
        res.set_header("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
        res.set_header("Access-Control-Allow-Headers", "Content-Type,Authorization");
    }

    void FileController::handleCorsOrNotFound(const httplib::Request &req,
                                              httplib::Response &res) const
    {
        applyCorsHeaders(res);

        if (req.method == "OPTIONS")
        {
            res.status = 204; // no body
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

        if (!req.is_multipart_form_data())
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
                res.set_content("Bad Request: Could not parse file content", "text/plain");
                return;
            }

            const httplib::FormData uploaded = req.form.get_file("file");

            std::string filename = uploaded.filename;
            if (filename.empty())
                filename = "unnamed-file";

            const std::string uniqueFilename = makeUniqueName(filename);
            const fs::path filePath = uploadDir_ / uniqueFilename;

            {
                std::ofstream fos(filePath, std::ios::binary);
                fos.write(uploaded.content.data(),
                          static_cast<std::streamsize>(uploaded.content.size()));
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

} // namespace server::services
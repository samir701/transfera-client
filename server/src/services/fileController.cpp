#include "server/service/fileController.hpp"

#include <cstdlib>
#include <iostream>

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
    if (!server_.listen("0.0.0.0", port_)) {
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
                        {
                            handleCorsOrNotFound(req, res); // returns 204
                        });
        // Add your /upload, /download, and CORS/fallback handlers here
        // server_.Post("/upload", ...);
        // server_.Get(R"(/download/(\d+))", ...);
        // server_.Options(R"(/.*)", ...);

        // Fallback for all unmatched routes
        server_.set_error_handler([this](const httplib::Request &req, httplib::Response &res)
                                  {
    // For 404, mimic Java CORSHandler behavior
    if (res.status == 404) {
      handleCorsOrNotFound(req, res);
    } else {
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

} // namespace server::services
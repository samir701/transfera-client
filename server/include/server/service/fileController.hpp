#pragma once

#include <atomic>
#include <filesystem>
#include <string>
#include <thread>

#include "server/httplib/httplib.hpp"
#include "server/service/fileSharer.hpp"

namespace server::services
{

    class FileController
    {
    public:
        explicit FileController(int port);
        ~FileController();

        // Returns false if the port could not be bound (e.g. another server instance).
        bool start();
        void stop();

    private:
        void registerRoutes();

        // CORS helpers (CORSHandler)
        void applyCorsHeaders(httplib::Response &res) const;
        void handleCorsOrNotFound(const httplib::Request &req, httplib::Response &res) const;

        // Java: UploadHandler
        void handleUpload(const httplib::Request &req, httplib::Response &res);

        // Java: DownloadHandler
        void handleDownload(const httplib::Request &req, httplib::Response &res);

        static std::string sanitizeDisplayFilename(const std::string &originalFilename);
        static std::string makeUniqueName(const std::string &originalFilename);

        // Java: path.substring(lastIndexOf('/') + 1) + Integer.parseInt
        static bool parsePortFromPath(const std::string &path, int &outPort);

        service::FileSharer fileSharer_;
        httplib::Server server_;
        std::filesystem::path uploadDir_;
        int port_;

        std::thread serverThread_;
        std::atomic<bool> running_{false};
        std::atomic<bool> listen_ok_{false};
    };

} // namespace server::services

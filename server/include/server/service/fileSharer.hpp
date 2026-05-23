#pragma once

#include <string>
#include <unordered_map>

namespace server::service
{

    struct SharedFile
    {
        std::string path;
        std::string downloadName; // original basename sent to clients (keeps extension)
    };

    class FileSharer
    {
    public:
        FileSharer() = default;

        int offerFile(const std::string &filePath, const std::string &downloadName);
        void startFileServer(int port);

        bool tryGetDownloadName(int port, std::string &outName) const;
        /** Lookup invite port → stored file (for HTTP download without localhost P2P). */
        bool tryGetSharedFile(int port, SharedFile &outFile) const;

    private:
        std::unordered_map<int, SharedFile> available_files_;
    };

} // namespace server::service
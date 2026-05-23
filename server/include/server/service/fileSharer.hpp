#pragma once

#include <memory>
#include <mutex>
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

        /** Legacy: accept one remote/local connection (not used on upload path). */
        void startFileServer(int port);

        bool tryGetDownloadName(int port, std::string &outName) const;
        bool tryGetSharedFile(int port, SharedFile &outFile) const;

        /**
         * HTTP download: P2P wire protocol over socketpair (no upload-time TCP listener).
         * Repeat downloads reuse the invite code; serialized per invite port.
         */
        bool receiveViaLocalP2P(int port, std::string &outFilename, std::string &outBody,
                                std::string &outError);

    private:
        mutable std::mutex mapMutex_;
        std::unordered_map<int, SharedFile> available_files_;
        std::unordered_map<int, std::unique_ptr<std::mutex>> portTransferMutexes_;

        std::mutex &transferMutexForPort(int port);
    };

} // namespace server::service

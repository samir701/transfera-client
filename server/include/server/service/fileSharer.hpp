#pragma once

#include <string>
#include <unordered_map>

namespace server::service
{

    class FileSharer
    {
    public:
        FileSharer() = default;

        int offerFile(const std::string &filePath);
        void startFileServer(int port);

    private:
        std::unordered_map<int, std::string> available_files_;
    };

} // namespace server::service
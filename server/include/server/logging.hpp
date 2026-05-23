#pragma once

#include <string>

namespace server::log
{

    /** Line-buffer stdout/stderr so Docker/Render capture logs immediately. */
    void init();

    bool httpAccessEnabled();
    bool verboseEnabled();

    void info(const std::string &message);
    void error(const std::string &message);

} // namespace server::log

#include "server/logging.hpp"

#include <chrono>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <iomanip>
#include <sstream>

namespace server::log
{

    namespace
    {
        bool envTruthy(const char *name, bool defaultValue)
        {
            if (const char *v = std::getenv(name); v && *v)
            {
                if (v[0] == '0' && v[1] == '\0')
                    return false;
                if (std::strcmp(v, "false") == 0 || std::strcmp(v, "FALSE") == 0)
                    return false;
                return true;
            }
            return defaultValue;
        }

        std::string timestamp()
        {
            using clock = std::chrono::system_clock;
            const auto now = clock::now();
            const std::time_t t = clock::to_time_t(now);
            std::tm tm{};
#if defined(_WIN32)
            localtime_s(&tm, &t);
#else
            localtime_r(&t, &tm);
#endif
            std::ostringstream oss;
            oss << std::put_time(&tm, "%Y-%m-%dT%H:%M:%S");
            return oss.str();
        }

        void writeLine(std::ostream &out, const char *level, const std::string &message)
        {
            out << timestamp() << ' ' << level << ' ' << message << '\n';
            out.flush();
        }
    } // namespace

    void init()
    {
        setvbuf(stdout, nullptr, _IOLBF, 0);
        setvbuf(stderr, nullptr, _IONBF, 0);
        std::cout.setf(std::ios::unitbuf);
        std::cerr.setf(std::ios::unitbuf);
        std::ios::sync_with_stdio(true);

        info("logging initialized (stdout line-buffered, stderr unbuffered)");
        if (httpAccessEnabled())
            info("HTTP access logging enabled (TRANSFERA_LOG_HTTP=1)");
        if (verboseEnabled())
            info("verbose logging enabled (TRANSFERA_LOG_VERBOSE=1)");
    }

    bool httpAccessEnabled()
    {
        static const bool enabled = envTruthy("TRANSFERA_LOG_HTTP", true);
        return enabled;
    }

    bool verboseEnabled()
    {
        static const bool enabled = envTruthy("TRANSFERA_LOG_VERBOSE", true);
        return enabled;
    }

    void info(const std::string &message)
    {
        writeLine(std::cout, "[INFO]", message);
    }

    void error(const std::string &message)
    {
        writeLine(std::cerr, "[ERROR]", message);
    }

} // namespace server::log

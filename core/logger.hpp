#pragma once

#include <mutex>
#include <string_view>

#include "formatter.hpp"

#define BOUTIQUE_LOG_INFO(fmt, ...)                                                         \
    do {                                                                                    \
        boutique::Logger::instance().log(__FILE__, __LINE__, boutique::Logger::Level::Info, \
                                         boutique::format(fmt, ##__VA_ARGS__));             \
    } while (0)

#define BOUTIQUE_LOG_ERROR(fmt, ...)                                                         \
    do {                                                                                     \
        boutique::Logger::instance().log(__FILE__, __LINE__, boutique::Logger::Level::Error, \
                                         boutique::format(fmt, ##__VA_ARGS__));              \
    } while (0)

namespace boutique {

struct Logger {
    enum class Level : uint8_t { Debug, Info, Warning, Error };

    static Logger& instance();

    void configure(Level level, uint32_t mask);
    void log(std::string_view file, int line, Level level, std::string_view msg,
             uint32_t flags = 0);

private:
    mutable std::mutex m_mutex;

    Level m_level = Level::Info;
    uint32_t m_mask = 0;
};

}  // namespace boutique

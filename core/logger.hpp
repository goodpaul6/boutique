#pragma once

#include <mutex>
#include <string_view>

#include "formatter.hpp"

#define BOUTIQUE_LOG_DEBUG(fmt, ...)                                                         \
    do {                                                                                     \
        boutique::Logger::instance().log(__FILE__, __LINE__, boutique::Logger::Level::DEBUG, \
                                         boutique::format(fmt, ##__VA_ARGS__));              \
    } while (0)

#define BOUTIQUE_LOG_INFO(fmt, ...)                                                         \
    do {                                                                                    \
        boutique::Logger::instance().log(__FILE__, __LINE__, boutique::Logger::Level::INFO, \
                                         boutique::format(fmt, ##__VA_ARGS__));             \
    } while (0)

#define BOUTIQUE_LOG_ERROR(fmt, ...)                                                         \
    do {                                                                                     \
        boutique::Logger::instance().log(__FILE__, __LINE__, boutique::Logger::Level::ERROR, \
                                         boutique::format(fmt, ##__VA_ARGS__));              \
    } while (0)

namespace boutique {

struct Logger {
    enum class Level : uint8_t { DEBUG, INFO, WARNING, ERROR };

    static Logger& instance();

    void configure(Level level, uint32_t mask);
    void log(std::string_view file, int line, Level level, std::string_view msg,
             uint32_t flags = 0);

private:
    mutable std::mutex m_mutex;

    Level m_level = Level::INFO;
    uint32_t m_mask = 0;
};

}  // namespace boutique

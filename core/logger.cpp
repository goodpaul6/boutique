#include "logger.hpp"

#include <chrono>
#include <ctime>
#include <iostream>

namespace boutique {

Logger& Logger::instance() {
    static Logger logger;
    return logger;
}

void Logger::configure(Level level, uint32_t mask) {
    std::scoped_lock lock{m_mutex};

    m_level = level;
    m_mask = mask;
}

void Logger::log(std::string_view file, int line, Level level, std::string_view msg,
                 uint32_t flags) {
    std::scoped_lock lock{m_mutex};

    auto level_value = static_cast<uint8_t>(level);

    if (level_value < static_cast<uint8_t>(m_level)) {
        return;
    }

    if (level_value < static_cast<uint8_t>(Level::Error) && m_mask != 0 && (flags & m_mask) == 0) {
        return;
    }

    using namespace std::chrono;

    auto cur_time = system_clock::to_time_t(system_clock::now());
    auto tm = std::localtime(&cur_time);
    char buf[128];

    std::strftime(buf, sizeof(buf), "%T", tm);

    std::clog << '[' << buf << "] ";

    switch (level) {
        case Level::Debug:
            std::clog << "DEBUG ";
            break;
        case Level::Info:
            std::clog << "INFO ";
            break;
        case Level::Warning:
            std::clog << "WARNING ";
            break;
        case Level::Error:
            std::clog << "ERROR ";
            break;
    }

    if (auto pos = file.find_last_of("/\\"); pos != std::string_view::npos) {
        file = file.substr(pos + 1);
    }

    std::clog << file << ":" << line << '\t' << msg << '\n';
}

}  // namespace boutique

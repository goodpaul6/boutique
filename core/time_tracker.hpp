#pragma once

#include <chrono>
#include <string>

#define BOUTIQUE_DEBUG_TIME_TRACKER(prefix) TimeTracker tt{prefix};

namespace boutique {

struct TimeTracker {
    TimeTracker(std::string prefix);
    ~TimeTracker();

private:
    std::string m_prefix;

    std::chrono::high_resolution_clock::time_point m_start_time;
};

}  // namespace boutique

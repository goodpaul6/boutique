#pragma once

#include <chrono>

#define BOUTIQUE_DEBUG_TIME_TRACKER(prefix) TimeTracker tt{prefix};

namespace boutique {

struct TimeTracker {
    TimeTracker(const char* prefix);
    ~TimeTracker();

private:
    const char* m_prefix = nullptr;

    std::chrono::high_resolution_clock::time_point m_start_time;
};

}  // namespace boutique

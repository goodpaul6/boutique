#include "time_tracker.hpp"

#include "logger.hpp"

namespace boutique {

TimeTracker::TimeTracker(std::string prefix)
    : m_prefix{std::move(prefix)}, m_start_time{std::chrono::high_resolution_clock::now()} {}

TimeTracker::~TimeTracker() {
    BOUTIQUE_LOG_INFO("{}: Elapsed time {}us", m_prefix,
                      std::chrono::duration_cast<std::chrono::microseconds>(
                          std::chrono::high_resolution_clock::now() - m_start_time)
                          .count());
}

}  // namespace boutique

#include "timer.hpp"

#include <errno.h>
#include <sys/timerfd.h>

#include <utility>

#include "unix_utils.hpp"

namespace {

timespec to_timespec(std::chrono::nanoseconds ns) {
    auto sec = std::chrono::duration_cast<std::chrono::seconds>(ns);
    auto ns_part = ns - sec;

    timespec spec;

    spec.tv_sec = sec.count();
    spec.tv_nsec = ns_part.count();

    return spec;
}

}  // namespace

namespace boutique {

Timer::Timer(int fd) : m_fd{fd} {}
Timer::Timer(const Params& params) {
    int fd = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK);

    if (fd < 0) {
        throw_errno("Failed to create timerfd");
    }

    m_fd = fd;

    reset(params);
}

Timer::Timer(Timer&& other) : m_fd{std::exchange(other.m_fd, -1)} {}
Timer& Timer::operator=(Timer&& other) {
    m_fd = std::exchange(other.m_fd, -1);
    return *this;
}

Timer::~Timer() {
    if (m_fd >= 0) {
        close(m_fd);
    }
}

int Timer::fd() const { return m_fd; }

std::chrono::nanoseconds Timer::time_until_expiry() const {
    itimerspec spec;

    timerfd_gettime(m_fd, &spec);

    return std::chrono::seconds{spec.it_value.tv_sec} +
           std::chrono::nanoseconds{spec.it_value.tv_nsec};
}

int Timer::expire_count() const {
    std::uint64_t expires;
    auto res = read(m_fd, static_cast<void*>(&expires), sizeof(expires));

    if (res <= 0) {
        if (errno == EWOULDBLOCK || errno == EAGAIN) {
            return 0;
        }

        throw_errno("Failed to get expire count of timerfd");
    }

    return expires;
}

void Timer::reset(const Params& params) {
    itimerspec spec;

    spec.it_value = to_timespec(params.init_expiration);
    spec.it_interval = to_timespec(params.interval);

    int r = timerfd_settime(m_fd, 0, &spec, nullptr);

    if (r < 0) {
        throw_errno("Failed to set time on timerfd");
    }
}

void Timer::set_non_blocking(bool enabled) { boutique::set_non_blocking(m_fd, enabled); }

}  // namespace boutique

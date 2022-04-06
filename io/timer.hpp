#pragma once

#include <chrono>

namespace boutique {

struct Timer {
    struct Params {
        std::chrono::nanoseconds init_expiration = std::chrono::nanoseconds::zero();
        std::chrono::nanoseconds interval = std::chrono::nanoseconds::zero();
    };

    explicit Timer(int fd);
    explicit Timer(const Params& params);

    Timer(Timer&& other);
    Timer& operator=(Timer&& other);

    Timer(const Timer& other) = delete;
    Timer& operator=(const Timer& other) = delete;

    ~Timer();

    int fd() const;

    std::chrono::nanoseconds time_until_expiry() const;

    // Will block if the timer is not non-blocking
    int expire_count() const;

    void reset(const Params& params);
    void set_non_blocking(bool enabled);

private:
    int m_fd = -1;
};

}  // namespace boutique

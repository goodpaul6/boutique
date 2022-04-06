#include "unix_utils.hpp"

#include <fcntl.h>

#include <system_error>

namespace boutique {

void throw_errno(const char* what) {
    throw std::system_error{make_error_code(static_cast<std::errc>(errno)), what};
}

void set_non_blocking(int fd, bool enabled) {
    int flags = fcntl(fd, F_GETFL, 0);

    if (flags < 0) {
        throw_errno("Failed to get fnctl flags on socket");
    }

    if (enabled) {
        flags |= O_NONBLOCK;
    } else {
        flags &= ~O_NONBLOCK;
    }

    if (fcntl(fd, F_SETFL, flags) < 0) {
        throw_errno("Failed to set fnctl flags on socket");
    }
}

}  // namespace boutique

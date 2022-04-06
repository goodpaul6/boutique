#include "socket.hpp"

#include <arpa/inet.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/tcp.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <unistd.h>

#include <stdexcept>
#include <system_error>
#include <utility>

#include "unix_utils.hpp"

namespace boutique {

Socket::Socket(int fd) : m_fd{fd} {}

Socket::Socket(const ListenParams& params) : m_fd{-1} {
    int fd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);

    if (fd < 0) {
        throw_errno("Failed to create socket");
    }

    int opt = 1;

    int res = setsockopt(fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt));

    if (res < 0) {
        throw_errno("Failed to set socket option");
    }

    sockaddr_in addr{};

    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(params.port);

    if (bind(fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
        throw_errno("Failed to bind socket");
    }

    if (::listen(fd, params.backlog) < 0) {
        throw_errno("Failed to listen on socket");
    }

    m_fd = fd;
}

Socket::Socket(const ConnectParams& params) : m_fd{-1} {
    int fd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);

    if (fd < 0) {
        throw_errno("Failed to create socket");
    }

    addrinfo hints{};

    hints.ai_family = AF_INET;

    addrinfo* res = nullptr;

    auto port_str = std::to_string(params.port);

    if (getaddrinfo(params.host, port_str.c_str(), &hints, &res) != 0) {
        throw_errno("Failed to get address info");
    }

    bool success = false;

    for (auto* cur = res; cur; cur = cur->ai_next) {
        if (::connect(fd, reinterpret_cast<sockaddr*>(cur->ai_addr), cur->ai_addrlen) < 0) {
            continue;
        }

        success = true;
        break;
    }

    freeaddrinfo(res);

    if (!success) {
        throw std::runtime_error{"Failed to connect to host"};
    }

    m_fd = fd;
}

Socket::Socket(Socket&& other) : m_fd{std::exchange(other.m_fd, -1)} {}

Socket& Socket::operator=(Socket&& other) {
    m_fd = std::exchange(other.m_fd, -1);
    return *this;
}

Socket::~Socket() {
    if (m_fd >= 0) {
        close(m_fd);
    }
}

std::optional<Socket> Socket::accept() {
    sockaddr_in addr;
    socklen_t addrlen = sizeof(addr);

    int fd = ::accept(m_fd, reinterpret_cast<sockaddr*>(&addr), &addrlen);

    if (fd < 0) {
        if (errno == EWOULDBLOCK || errno == EAGAIN) {
            return std::nullopt;
        }

        throw_errno("Failed to accept socket");
    }

    return Socket{fd};
}

int Socket::recv(char* buf, int maxlen) {
    int r = ::recv(m_fd, buf, maxlen, 0);

    if (r < 0) {
        if (errno == EWOULDBLOCK || errno == EAGAIN) {
            return 0;
        }

        throw_errno("Failed to read from socket");
    }

    return r;
}

int Socket::send(const char* buf, int maxlen) {
    int r = ::send(m_fd, buf, maxlen, 0);

    if (r < 0) {
        if (errno == EWOULDBLOCK || errno == EAGAIN) {
            return 0;
        }

        throw_errno("Failed to send to socket");
    }

    return r;
}

int Socket::fd() const { return m_fd; }

void Socket::set_non_blocking(bool enabled) { boutique::set_non_blocking(m_fd, enabled); }

void Socket::set_no_delay(bool enabled) {
    int opt = static_cast<int>(enabled);
    if (setsockopt(m_fd, IPPROTO_TCP, TCP_NODELAY, &opt, sizeof(opt)) < 0) {
        throw_errno("Failed to set no delay on socket");
    }
}

}  // namespace boutique

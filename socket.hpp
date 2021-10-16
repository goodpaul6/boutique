#pragma once

#include <optional>

namespace boutique {

struct Socket {
    explicit Socket(int fd);

    Socket(Socket&& other);
    Socket& operator=(Socket&& other);

    Socket(const Socket& other) = delete;
    Socket& operator=(const Socket& other) = delete;

    ~Socket();

    static Socket listen(unsigned short port, int backlog = 4);
    static Socket connect(const char* host, unsigned short port);

    std::optional<Socket> accept();

    int recv(char* buf, int maxlen);
    int send(const char* buf, int maxlen);

    int fd() const;

private:
    int m_fd = -1;
};

}  // namespace boutique

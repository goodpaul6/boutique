#pragma once

#include <optional>

namespace boutique {

struct Socket {
    struct ListenParams {
        unsigned short port = 6969;
        int backlog = 4;
    };

    struct ConnectParams {
        const char* host = nullptr;
        unsigned short port = 6969;
    };

    explicit Socket(int fd);
    explicit Socket(const ListenParams& params);
    explicit Socket(const ConnectParams& params);

    Socket(Socket&& other);
    Socket& operator=(Socket&& other);

    Socket(const Socket& other) = delete;
    Socket& operator=(const Socket& other) = delete;

    ~Socket();

    std::optional<Socket> accept();

    int recv(char* buf, int maxlen);
    int send(const char* buf, int maxlen);

    int fd() const;

    void set_non_blocking(bool enabled);
    void set_no_delay(bool enabled);

private:
    int m_fd = -1;
};

}  // namespace boutique

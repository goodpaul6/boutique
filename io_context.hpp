#pragma once

#include <functional>
#include <vector>

namespace boutique {

struct Socket;

struct IOContext {
    using IntFn = std::function<void(int)>;
    using SocketFn = std::function<void(Socket)>;

    void async_recv(Socket& socket, char* buf, size_t maxlen, IntFn fn);
    void async_send(Socket& socket, const char* buf, size_t maxlen, IntFn fn);
    void async_accept(Socket& socket, SocketFn fn);

    void run();

    void stop();

private:
    struct RecvOp {
        Socket* socket = nullptr;
        char* buf = nullptr;
        size_t maxlen = 0;

        std::function<void(int recv_res)> fn;

        bool complete = false;
    };

    struct SendOp {
        Socket* socket = nullptr;
        const char* buf = nullptr;
        size_t maxlen = 0;

        std::function<void(int send_res)> fn;

        bool complete = false;
    };

    struct AcceptOp {
        Socket* socket = nullptr;

        std::function<void(Socket accepted_socket)> fn;

        bool complete = false;
    };

    bool m_stop = false;

    // TODO Maybe use a variant instead
    std::vector<RecvOp> m_recv;
    std::vector<SendOp> m_send;
    std::vector<AcceptOp> m_accept;
};

}  // namespace boutique

#pragma once

#include "core/streambuf.hpp"

#include "io/socket.hpp"

namespace boutique {

struct Dict;
struct IOContext;
struct Server;

struct ClientHandler {
    explicit ClientHandler(Server& server, Socket socket);

    Socket& socket();

    void close();
    bool closed() const;

private:
    enum class State { COMMAND, KEY, VALUE };

    // TODO Track open/close state on the socket itself
    bool m_closed = false;

    Server* m_server = nullptr;
    Socket m_socket;

    char m_buf[128];

    StreamBuf m_stream;

    State m_state = State::COMMAND;

    std::string m_cmd;
    std::string m_key;
    std::string m_value;

    void recv_handler(int len);
};

}  // namespace boutique


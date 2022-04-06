#pragma once

#include <list>

#include "client_handler.hpp"
#include "dict.hpp"
#include "io/context.hpp"
#include "io/socket.hpp"

namespace boutique {

struct Server {
    Server(unsigned short port);

    IOContext& io_context();

    void run();

    Dict& dict();

private:
    Dict m_dict;

    Socket m_socket;
    IOContext m_ioc;

    // We use a list so that adding new clients does not invalidate
    // existing clients.
    std::list<ClientHandler> m_clients;

    void accept_handler(Socket socket);
};

}  // namespace boutique

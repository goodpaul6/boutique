#include "server.hpp"

#include <system_error>

#include "core/bind_front.hpp"

namespace boutique {

Server::Server(unsigned short port) : m_socket{Socket::listen(port)} {}

IOContext& Server::io_context() { return m_ioc; }

void Server::run() {
    m_ioc.async_accept(m_socket, bind_front(&Server::accept_handler, this));
    m_ioc.run();
}

Dict& Server::dict() { return m_dict; }

void Server::accept_handler(Socket socket) {
    auto clients_end =
        std::remove_if(m_clients.begin(), m_clients.end(), [](auto& c) { return c.closed(); });

    m_clients.erase(clients_end, m_clients.end());

    m_clients.emplace_back(*this, std::move(socket));

    m_ioc.async_accept(m_socket, bind_front(&Server::accept_handler, this));
}

}  // namespace boutique

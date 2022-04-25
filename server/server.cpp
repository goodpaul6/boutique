#include "server.hpp"

#include <system_error>

#include "core/bind_front.hpp"
#include "core/logger.hpp"

namespace boutique {

Server::Server(unsigned short port) : m_socket{Socket{Socket::ListenParams{port}}} {
    BOUTIQUE_LOG_INFO("Listening on port {}", port);
}

IOContext& Server::io_context() { return m_ioc; }

void Server::run() {
    m_ioc.async_accept(m_socket, bind_front(&Server::accept_handler, this));
    m_ioc.run();
}

Database& Server::db() { return m_db; }

void Server::accept_handler(Socket socket) {
    auto clients_end =
        std::remove_if(m_clients.begin(), m_clients.end(), [](auto& c) { return c.closed(); });

    m_clients.erase(clients_end, m_clients.end());

    m_clients.emplace_back(*this, std::move(socket));

    m_ioc.async_accept(m_socket, bind_front(&Server::accept_handler, this));
}

}  // namespace boutique

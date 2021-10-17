#include <iostream>
#include <memory>
#include <optional>
#include <thread>
#include <vector>

#include "core/bind_front.hpp"

#include "io/context.hpp"
#include "io/helpers.hpp"
#include "io/socket.hpp"

using namespace boutique;

namespace {

struct Server {
    Server(IOContext& ioc, unsigned short port) : m_ioc{ioc}, m_socket{Socket::listen(port)} {}

    void start() { m_ioc.async_accept(m_socket, bind_front(&Server::accept_handler, this)); }

private:
    IOContext& m_ioc;
    Socket m_socket;
    std::vector<std::unique_ptr<Socket>> m_clients;

    char m_buf[128];

    void recv_handler(int client_idx, int len) {
        if (len == 0) {
            // Graceful disconnect
            m_clients[client_idx].reset();
            std::cout << "Client " << client_idx << " disconnected.\n";
            return;
        }

        std::cout << std::string{m_buf, static_cast<size_t>(len)} << '\n';

        m_ioc.async_recv(*m_clients[client_idx], m_buf, sizeof(m_buf),
                         bind_front(&Server::recv_handler, this, client_idx));
    }

    void accept_handler(Socket socket) {
        size_t client_idx = 0;

        // Find empty slot
        auto socket_ptr = std::make_unique<Socket>(std::move(socket));

        for (auto& client : m_clients) {
            if (!client) {
                client = std::move(socket_ptr);
                break;
            }
            client_idx++;
        }

        if (client_idx >= m_clients.size()) {
            m_clients.emplace_back(std::move(socket_ptr));
        }

        std::cout << "Client " << client_idx << " connected.\n";

        m_ioc.async_recv(*m_clients[client_idx], m_buf, sizeof(m_buf),
                         bind_front(&Server::recv_handler, this, client_idx));

        m_ioc.async_accept(m_socket, bind_front(&Server::accept_handler, this));
    }
};

}  // namespace

int main(int argc, char** argv) {
    using namespace boutique;

    IOContext io_context;

    Server server{io_context, 8080};

    server.start();

    io_context.run();

    return 0;
}


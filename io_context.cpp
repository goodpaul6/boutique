#include "io_context.hpp"

#include <sys/select.h>
#include <cassert>

#include "socket.hpp"

namespace boutique {

void IOContext::async_recv(Socket& socket, char* buf, size_t maxlen, IntFn fn) {
    RecvOp op;

    op.socket = &socket;
    op.buf = buf;
    op.maxlen = maxlen;
    op.fn = std::move(fn);

    m_recv.emplace_back(std::move(op));
}

void IOContext::async_send(Socket& socket, const char* buf, size_t maxlen, IntFn fn) {
    SendOp op;

    op.socket = &socket;
    op.buf = buf;
    op.maxlen = maxlen;
    op.fn = std::move(fn);

    m_send.emplace_back(std::move(op));
}

void IOContext::async_accept(Socket& socket, SocketFn fn) {
    AcceptOp op;

    op.socket = &socket;
    op.fn = std::move(fn);

    m_accept.emplace_back(std::move(op));
}

void IOContext::stop() { m_stop = true; }

void IOContext::run() {
    while (!m_stop) {
        fd_set read_fds;
        fd_set write_fds;

        FD_ZERO(&read_fds);
        FD_ZERO(&write_fds);

        int maxfd = 0;

        for (auto& op : m_recv) {
            if (op.socket->fd() > maxfd) {
                maxfd = op.socket->fd();
            }

            FD_SET(op.socket->fd(), &read_fds);
        }

        for (auto& op : m_send) {
            if (op.socket->fd() > maxfd) {
                maxfd = op.socket->fd();
            }

            FD_SET(op.socket->fd(), &write_fds);
        }

        for (auto& op : m_accept) {
            if (op.socket->fd() > maxfd) {
                maxfd = op.socket->fd();
            }

            FD_SET(op.socket->fd(), &read_fds);
        }

        // TODO Receive timeout
        ::select(maxfd + 1, &read_fds, &write_fds, nullptr, nullptr);

        for (auto& op : m_recv) {
            if (FD_ISSET(op.socket->fd(), &read_fds)) {
                op.fn(op.socket->recv(op.buf, op.maxlen));
                op.complete = true;
            }
        }

        for (auto& op : m_send) {
            if (FD_ISSET(op.socket->fd(), &write_fds)) {
                op.fn(op.socket->send(op.buf, op.maxlen));
                op.complete = true;
            }
        }

        for (auto& op : m_accept) {
            if (FD_ISSET(op.socket->fd(), &read_fds)) {
                auto opt_socket = op.socket->accept();

                // Since the select call said we're ready to accept, we must have
                // a socket here.
                assert(opt_socket.has_value());

                op.fn(std::move(*opt_socket));
                op.complete = true;
            }
        }

        const auto is_complete = [](auto&& op) { return op.complete; };

        auto recv_end = std::remove_if(m_recv.begin(), m_recv.end(), is_complete);
        auto send_end = std::remove_if(m_send.begin(), m_send.end(), is_complete);
        auto accept_end = std::remove_if(m_accept.begin(), m_accept.end(), is_complete);

        m_recv.erase(recv_end, m_recv.end());
        m_send.erase(send_end, m_send.end());
        m_accept.erase(accept_end, m_accept.end());
    }
}

}  // namespace boutique

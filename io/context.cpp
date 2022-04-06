#include "context.hpp"

#include <sys/select.h>

#include <cassert>

#include "socket.hpp"
#include "timer.hpp"

namespace boutique {

void IOContext::async_recv(Socket& socket, char* buf, size_t maxlen, IntFn fn) {
    assert(fn);

    RecvOp op;

    op.socket = &socket;
    op.buf = buf;
    op.maxlen = maxlen;
    op.fn = std::move(fn);

    m_recv.emplace_back(std::move(op));
}

void IOContext::async_send(Socket& socket, const char* buf, size_t maxlen, IntFn fn) {
    assert(fn);

    SendOp op;

    op.socket = &socket;
    op.buf = buf;
    op.maxlen = maxlen;
    op.fn = std::move(fn);

    m_send.emplace_back(std::move(op));
}

void IOContext::async_accept(Socket& socket, SocketFn fn) {
    assert(fn);

    AcceptOp op;

    op.socket = &socket;
    op.fn = std::move(fn);

    m_accept.emplace_back(std::move(op));
}

void IOContext::async_wait(Timer& timer, IntFn fn) {
    assert(fn);

    WaitOp op;

    op.timer = &timer;
    op.fn = std::move(fn);

    m_wait.emplace_back(std::move(op));
}

void IOContext::stop() { m_stop = true; }

void IOContext::run() {
    while (!m_stop) {
        fd_set read_fds;
        fd_set write_fds;

        FD_ZERO(&read_fds);
        FD_ZERO(&write_fds);

        int maxfd = 0;

        // We perform this copy because otherwise, when callbacks queue up new operations,
        // it would manipulate the underlying vectors during traversal
        auto recv_copy = m_recv;
        auto send_copy = m_send;
        auto accept_copy = m_accept;
        auto wait_copy = m_wait;

        m_recv.clear();
        m_send.clear();
        m_accept.clear();
        m_wait.clear();

        // TODO We should have checks that ensure there aren't multiple operations in flight for the
        // same fd. We don't handle this situation well right now.

        for (auto& op : recv_copy) {
            if (op.socket->fd() > maxfd) {
                maxfd = op.socket->fd();
            }

            FD_SET(op.socket->fd(), &read_fds);
        }

        for (auto& op : send_copy) {
            if (op.socket->fd() > maxfd) {
                maxfd = op.socket->fd();
            }

            FD_SET(op.socket->fd(), &write_fds);
        }

        for (auto& op : accept_copy) {
            if (op.socket->fd() > maxfd) {
                maxfd = op.socket->fd();
            }

            FD_SET(op.socket->fd(), &read_fds);
        }

        for (auto& op : wait_copy) {
            if (op.timer->fd() > maxfd) {
                maxfd = op.timer->fd();
            }

            FD_SET(op.timer->fd(), &read_fds);
        }

        // TODO Receive timeout
        ::select(maxfd + 1, &read_fds, &write_fds, nullptr, nullptr);

        for (auto& op : recv_copy) {
            if (FD_ISSET(op.socket->fd(), &read_fds)) {
                op.fn(op.socket->recv(op.buf, op.maxlen));
                op.complete = true;
            }
        }

        for (auto& op : send_copy) {
            if (FD_ISSET(op.socket->fd(), &write_fds)) {
                op.fn(op.socket->send(op.buf, op.maxlen));
                op.complete = true;
            }
        }

        for (auto& op : accept_copy) {
            if (FD_ISSET(op.socket->fd(), &read_fds)) {
                auto opt_socket = op.socket->accept();

                // Since the select call said we're ready to accept, we must have
                // a socket here.
                assert(opt_socket.has_value());

                op.fn(std::move(*opt_socket));
                op.complete = true;
            }
        }

        for (auto& op : wait_copy) {
            if (FD_ISSET(op.timer->fd(), &read_fds)) {
                op.fn(op.timer->expire_count());
                op.complete = true;
            }
        }

        const auto is_complete = [](auto&& op) { return op.complete; };

        auto recv_end = std::remove_if(recv_copy.begin(), recv_copy.end(), is_complete);
        auto send_end = std::remove_if(send_copy.begin(), send_copy.end(), is_complete);
        auto accept_end = std::remove_if(accept_copy.begin(), accept_copy.end(), is_complete);
        auto wait_end = std::remove_if(wait_copy.begin(), wait_copy.end(), is_complete);

        recv_copy.erase(recv_end, recv_copy.end());
        send_copy.erase(send_end, send_copy.end());
        accept_copy.erase(accept_end, accept_copy.end());
        wait_copy.erase(wait_end, wait_copy.end());

        // m_recv, send, etc currently contain new operations, and recv_copy contains previous
        // operations (that are incomplete), so to respect ordering, we put recv_copy operations
        // before m_recv ops.

        recv_copy.insert(recv_copy.end(), m_recv.begin(), m_recv.end());
        send_copy.insert(send_copy.end(), m_send.begin(), m_send.end());
        accept_copy.insert(accept_copy.end(), m_accept.begin(), m_accept.end());
        wait_copy.insert(wait_copy.end(), m_wait.begin(), m_wait.end());

        m_recv = std::move(recv_copy);
        m_send = std::move(send_copy);
        m_accept = std::move(accept_copy);
        m_wait = std::move(wait_copy);
    }
}

}  // namespace boutique

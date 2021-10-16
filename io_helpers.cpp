#include "io_helpers.hpp"

#include "socket.hpp"

namespace {

struct HandlerData {
    boutique::IOContext* context = nullptr;
    boutique::Socket* socket = nullptr;
    size_t orig_len = 0;
    boutique::IOContext::IntFn res_fn;
};

struct RecvHandler {
    RecvHandler(HandlerData data, char* buf, size_t len)
        : m_data{std::move(data)}, m_buf{buf}, m_len{len} {}

    void operator()(int res) {
        m_len -= res;
        m_buf += res;

        if (m_len == 0) {
            m_data.res_fn(m_data.orig_len);
            return;
        }

        // We copy this out because we std::move(m_data) below in the same expression
        // and I'm suspicious about the lifetime of m_data here.
        auto& socket = *m_data.socket;

        // We create a new handler whose buffer is progressed by the appropriate amount.
        m_data.context->async_recv(socket, m_buf, m_len,
                                   RecvHandler{std::move(m_data), m_buf, m_len});
    }

private:
    HandlerData m_data;
    char* m_buf = nullptr;
    size_t m_len = 0;
};

struct SendHandler {
    SendHandler(HandlerData data, const char* buf, size_t len)
        : m_data{std::move(data)}, m_buf{buf}, m_len{len} {}

    void operator()(int res) {
        m_len -= res;
        m_buf += res;

        if (m_len == 0) {
            m_data.res_fn(m_data.orig_len);
            return;
        }

        auto& socket = *m_data.socket;

        // Same as above
        m_data.context->async_send(socket, m_buf, m_len,
                                   SendHandler{std::move(m_data), m_buf, m_len});
    }

private:
    HandlerData m_data;
    const char* m_buf = nullptr;
    size_t m_len = 0;
};

}  // namespace

namespace boutique {

void async_recv_all(IOContext& context, Socket& socket, char* buf, size_t len,
                    IOContext::IntFn fn) {
    HandlerData data{&context, &socket, len, std::move(fn)};

    // Kickstart the recv loop by creating a handler
    RecvHandler{std::move(data), buf, len}(0);
}

void async_send_all(IOContext& context, Socket& socket, const char* buf, size_t len,
                    IOContext::IntFn fn) {
    HandlerData data{&context, &socket, len, std::move(fn)};

    // Kickstart the send loop by creating a handler
    SendHandler{std::move(data), buf, len}(0);
}

}  // namespace boutique

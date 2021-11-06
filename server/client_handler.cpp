#include "client_handler.hpp"

#include <cstring>
#include <memory>
#include <optional>
#include <string_view>

#include "core/bind_front.hpp"
#include "core/const_buffer.hpp"
#include "core/logger.hpp"
#include "io/helpers.hpp"
#include "protocol/binary_protocol.hpp"
#include "server.hpp"

namespace boutique {

ClientHandler::ClientHandler(Server& server, Socket socket)
    : m_server{&server}, m_socket{std::move(socket)} {
    BOUTIQUE_LOG_INFO("Client connected.");
    m_server->io_context().async_recv(m_socket, m_buf, sizeof(m_buf),
                                      bind_front(&ClientHandler::recv_handler, this));
}

Socket& ClientHandler::socket() { return m_socket; }

void ClientHandler::close() {
    BOUTIQUE_LOG_INFO("Client disconnected.");

    m_closed = true;
}

bool ClientHandler::closed() const { return m_closed; }

void ClientHandler::recv_handler(int len) {
    if (len == 0) {
        close();
        return;
    }

    m_stream.append(m_buf, len);

    ConstBuffer cmd_buf{m_stream};

    Command cmd;

    for (;;) {
        auto rc_res = read(cmd_buf, cmd);

        if (rc_res == ReadResult::INCOMPLETE) {
            break;
        }

        auto buf = std::make_shared<std::vector<char>>();

        auto buf_writer = [&](size_t len) {
            buf->resize(buf->size() + len);
            auto* ptr = buf->data() + buf->size() - len;

            return ptr;
        };

        if (rc_res == ReadResult::INVALID) {
            write(buf_writer, InvalidCommandResponse{});

            auto* buf_data = buf->data();
            auto buf_size = buf->size();

            // Move the buffer into the function so it is kept alive until sent
            async_send_all(m_server->io_context(), m_socket, buf_data, buf_size,
                           [buf = std::move(buf)](int res) {});

            m_stream.consume(cmd_buf.data - m_stream.data());
            break;
        } else if (rc_res == ReadResult::SUCCESS) {
            std::visit(
                [&](auto&& v) {
                    using T = std::decay_t<decltype(v)>;

                    if constexpr (std::is_same_v<T, GetCommand>) {
                        auto value = m_server->dict().get(std::string{v.key});

                        if (value) {
                            write(buf_writer, FoundResponse{*value});

                            auto* buf_data = buf->data();
                            auto buf_size = buf->size();

                            async_send_all(m_server->io_context(), m_socket, buf_data, buf_size,
                                           [buf = std::move(buf)](int res) {});
                        } else {
                            write(buf_writer, NotFoundResponse{});

                            auto* buf_data = buf->data();
                            auto buf_size = buf->size();

                            async_send_all(m_server->io_context(), m_socket, buf_data, buf_size,
                                           [buf = std::move(buf)](int res) {});
                        }
                    } else if constexpr (std::is_same_v<T, SetCommand>) {
                        m_server->dict().set(std::string{v.key}, std::string{v.value});

                        write(buf_writer, SuccessResponse{});

                        auto* buf_data = buf->data();
                        auto buf_size = buf->size();

                        async_send_all(m_server->io_context(), m_socket, buf_data, buf_size,
                                       [buf = std::move(buf)](int res) {});
                    }
                },
                cmd);

            m_stream.consume(cmd_buf.data - m_stream.data());
        }
    }

    m_server->io_context().async_recv(m_socket, m_buf, sizeof(m_buf),
                                      bind_front(&ClientHandler::recv_handler, this));
}

}  // namespace boutique

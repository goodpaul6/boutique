#include "client_handler.hpp"

#include <cassert>
#include <cstring>
#include <memory>
#include <optional>
#include <string_view>

#include "core/bind_front.hpp"
#include "core/const_buffer.hpp"
#include "core/logger.hpp"
#include "core/overloaded_visitor.hpp"
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

        const auto write_and_send = [&](const auto& res) {
            auto buf = std::make_shared<std::vector<char>>();

            auto buf_writer = [&](size_t len) {
                buf->resize(buf->size() + len);
                auto* ptr = buf->data() + buf->size() - len;

                return ptr;
            };

            write(buf_writer, res);

            auto* buf_data = buf->data();
            auto buf_size = buf->size();

            // Move the buffer into the function so it is kept alive until sent
            async_send_all(m_server->io_context(), m_socket, buf_data, buf_size,
                           [buf = std::move(buf)](int) {});
        };

        if (rc_res == ReadResult::INVALID) {
            write_and_send(InvalidCommandResponse{});
            break;
        }

        assert(rc_res == ReadResult::SUCCESS);

        std::visit(
            OverloadedVisitor{
                [&](RegisterSchemaCommand cmd) {
                    m_server->db().register_schema(std::string{cmd.name}, std::move(cmd.schema));
                    write_and_send(SuccessResponse{});
                },
                [&](CreateCollectionCommand cmd) {
                    auto* schema = m_server->db().schema(std::string{cmd.schema_name});

                    if (!schema) {
                        // TODO Create SchemaNotFoundResponse
                        write_and_send(NotFoundResponse{});
                        return;
                    }

                    m_server->db().create_collection(std::string{cmd.name}, *schema);
                    write_and_send(SuccessResponse{});
                },
                [&](GetCollectionSchemaCommand cmd) {
                    auto* coll = m_server->db().collection(std::string{cmd.name});

                    if (!coll) {
                        write_and_send(NotFoundResponse{});
                        return;
                    }

                    write_and_send(SchemaResponse{coll->schema()});
                },
                [&](GetCommand cmd) {
                    auto* coll = m_server->db().collection(std::string{cmd.coll_name});

                    if (!coll) {
                        write_and_send(NotFoundResponse{});
                        return;
                    }

                    auto* found = coll->find(cmd.key);

                    if (!found) {
                        write_and_send(NotFoundResponse{});
                        return;
                    }

                    write_and_send(FoundResponse{
                        ConstBuffer{reinterpret_cast<const char*>(found), coll->doc_size()}});
                },
                [&](PutCommand cmd) {
                    auto* coll = m_server->db().collection(std::string{cmd.coll_name});

                    if (!coll) {
                        write_and_send(NotFoundResponse{});
                        return;
                    }

                    // TODO Add checks to make sure data len is the same as schema size

                    coll->put(cmd.value.data);

                    write_and_send(SuccessResponse{});
                },
                [&](DeleteCommand cmd) {
                    auto* coll = m_server->db().collection(std::string{cmd.coll_name});

                    if (!coll) {
                        write_and_send(NotFoundResponse{});
                        return;
                    }

                    coll->remove(cmd.key);

                    write_and_send(SuccessResponse{});
                },
                [](auto) {}},
            std::move(cmd));

        m_stream.consume(cmd_buf.data - m_stream.data());
    }

    m_server->io_context().async_recv(m_socket, m_buf, sizeof(m_buf),
                                      bind_front(&ClientHandler::recv_handler, this));
}

}  // namespace boutique

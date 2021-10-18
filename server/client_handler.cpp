#include "client_handler.hpp"

#include <cctype>
#include <memory>
#include <string_view>

#include "core/bind_front.hpp"
#include "core/logger.hpp"
#include "io/helpers.hpp"
#include "server.hpp"

namespace {

const char SUCCESS[] = "SUCCESS";
const char INVALID_COMMAND[] = "INVALID_COMMAND";
const char NOT_FOUND[] = "NOT_FOUND";

}  // namespace

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

    const auto find_first_space = [](auto& str) { return str.find_first_of(" \t\r\n"); };

    bool stop = false;

    while (!m_stream.empty() && !stop) {
        while (!m_stream.empty() && std::isspace(*m_stream.data())) {
            m_stream.consume(1);
        }

        switch (m_state) {
            case State::COMMAND: {
                std::string_view cmd{m_stream.data(), m_stream.size()};

                auto space_pos = find_first_space(cmd);

                if (space_pos != std::string_view::npos) {
                    m_cmd = cmd.substr(0, space_pos);
                    m_stream.consume(space_pos);

                    if (m_cmd != "GET" && m_cmd != "SET") {
                        async_send_all(m_server->io_context(), m_socket, INVALID_COMMAND,
                                       sizeof(INVALID_COMMAND), [](int) {});
                    } else {
                        m_state = State::KEY;
                    }
                } else {
                    stop = true;
                }
            } break;

            case State::KEY: {
                std::string_view key{m_stream.data(), m_stream.size()};

                auto space_pos = find_first_space(key);

                if (space_pos != std::string::npos) {
                    m_key = key.substr(0, space_pos);
                    m_stream.consume(space_pos);

                    if (m_cmd == "GET") {
                        auto found = m_server->dict().get(m_key);

                        if (found) {
                            // TODO
                            auto res = std::make_shared<std::string>(*found);

                            auto* str = res->c_str();
                            auto size = res->size() + 1;

                            // We just keep the value alive
                            async_send_all(m_server->io_context(), m_socket, str, size,
                                           [keep_alive = std::move(res)](int) {});
                        } else {
                            BOUTIQUE_LOG_INFO("Key {} not found.", m_key);

                            async_send_all(m_server->io_context(), m_socket, NOT_FOUND,
                                           sizeof(NOT_FOUND), [](int) {});
                        }

                        m_state = State::COMMAND;
                    } else {
                        m_state = State::VALUE;
                    }
                } else {
                    stop = true;
                }
            } break;

            case State::VALUE: {
                std::string_view value{m_stream.data(), m_stream.size()};

                auto space_pos = find_first_space(value);

                if (space_pos != std::string::npos) {
                    m_value = value.substr(0, space_pos);
                    m_stream.consume(space_pos);

                    if (m_cmd == "SET") {
                        m_server->dict().set(m_key, m_value);

                        async_send_all(m_server->io_context(), m_socket, SUCCESS, sizeof(SUCCESS),
                                       [](int) {});
                    }

                    m_state = State::COMMAND;
                } else {
                    stop = true;
                }
            } break;
        }
    }

    m_server->io_context().async_recv(m_socket, m_buf, sizeof(m_buf),
                                      bind_front(&ClientHandler::recv_handler, this));
}

}  // namespace boutique

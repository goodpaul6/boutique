#include <cassert>
#include <iostream>
#include <string>
#include <string_view>

#include "core/streambuf.hpp"
#include "io/socket.hpp"
#include "protocol/binary_protocol.hpp"

using namespace boutique;

int main(int argc, char** argv) {
    Socket client{Socket::ConnectParams{argv[1], static_cast<unsigned short>(std::stoi(argv[2]))}};

    client.set_non_blocking(false);

    std::string str;

    StreamBuf stream;

    for (;;) {
        assert(stream.empty());

        std::cout << "> ";

        std::getline(std::cin, str);
        if (str == "quit") {
            std::cout << "Goodbye.\n";
            break;
        }

        // TODO Refactor this since its copied from the client_handler.cpp
        std::vector<char> buf;

        auto buf_writer = [&](size_t len) {
            buf.resize(buf.size() + len);
            auto* ptr = buf.data() + buf.size() - len;

            return ptr;
        };

        Command cmd;

        if (str == "get") {
            std::cout << "key > ";

            std::getline(std::cin, str);

            cmd = GetCommand{str};
        } else if (str == "set") {
            std::cout << "key > ";

            std::getline(std::cin, str);

            std::string value;

            std::cout << "value > ";

            std::getline(std::cin, value);

            cmd = SetCommand{str, value};
        } else {
            std::cout << "Unknown command.\n";
            continue;
        }

        write(buf_writer, cmd);

        int n = 0;

        do {
            int r = client.send(buf.data() + n, buf.size() - n);

            if (r == 0) {
                std::cerr << "Failed to send command.\n";
                break;
            }

            n += r;
        } while (n < buf.size());

        // Keep reading until we get one response
        char recv_buf[128];

        do {
            n = client.recv(recv_buf, sizeof(recv_buf));

            stream.append(recv_buf, n);

            ConstBuffer res_buf{stream};

            Response res;

            auto r = read(res_buf, res);

            if (r == ReadResult::INVALID) {
                std::cerr << "Received invalid response from server.\n";
                stream.consume(stream.size());
                break;
            } else if (r == ReadResult::SUCCESS) {
                std::visit(
                    [](auto&& v) {
                        using T = std::decay_t<decltype(v)>;

                        if constexpr (std::is_same_v<T, InvalidCommandResponse>) {
                            std::cerr << "Apparently we sent an invalid command.\n";
                        } else if constexpr (std::is_same_v<T, SuccessResponse>) {
                            std::cout << "Success.\n";
                        } else if constexpr (std::is_same_v<T, NotFoundResponse>) {
                            std::cout << "Not found.\n";
                        } else if constexpr (std::is_same_v<T, FoundResponse>) {
                            std::cout << v.value << '\n';
                        }
                    },
                    res);

                stream.consume(res_buf.data - stream.data());
            }
        } while (!stream.empty());
    }

    return 0;
}

// Boutique
// ========
// A structured key-value store. You define types which will be stored at
// predefined key prefixes and boutique stores and transmits them optimally.

#include <iostream>
#include <optional>

#include "io_context.hpp"
#include "socket.hpp"

int main(int argc, char** argv) {
    using namespace boutique;

    IOContext io_context;

    auto server = Socket::listen(8080);
    auto client = Socket::connect("localhost", 8080);

    char buf[128];

    std::optional<Socket> accepted_client;

    io_context.async_accept(server, [&](Socket res) {
        accepted_client = std::move(res);

        io_context.async_recv(*accepted_client, buf, sizeof(buf), [&](int len) {
            std::cout << std::string{buf, static_cast<size_t>(len)} << '\n';
        });
    });

    io_context.async_send(client, "hello", sizeof("hello"),
                          [&](int res) { std::cout << "Sent " << res << " bytes.\n"; });

    io_context.run();

    return 0;
}


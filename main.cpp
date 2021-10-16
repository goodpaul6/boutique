// Boutique
// ========
// A structured key-value store. You define types which will be stored at
// predefined key prefixes and boutique stores and transmits them optimally.

#include <iostream>

#include "socket.hpp"

int main(int argc, char** argv) {
    using namespace boutique;

    auto server = Socket::listen(8080);
    auto client = Socket::connect("localhost", 8080);

    for (;;) {
        auto accepted_client = server.accept();

        if (accepted_client) {
            std::cout << "Accepted client.\n";

            int len = client.send("hello", sizeof("hello"));

            char buf[128];
            len = accepted_client->read(buf, sizeof(buf));

            if (len > 0) {
                std::cout << std::string{buf, static_cast<size_t>(len)} << '\n';
            }
        }
    }

    return 0;
}


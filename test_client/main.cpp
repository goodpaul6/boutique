#include <iostream>
#include <string>
#include <string_view>

#include "io/socket.hpp"

using namespace boutique;

int main(int argc, char** argv) {
    auto client = Socket::connect(argv[1], static_cast<unsigned short>(std::stoi(argv[2])));

    client.set_non_blocking(false);

    std::string str;

    for (;;) {
        std::getline(std::cin, str);
        if (str == "quit") {
            std::cout << "Goodbye.\n";
            break;
        }

        str += '\n';

        int n = 0;

        do {
            n += client.send(str.c_str(), str.size());

            char buf[128];

            int len = 0;

            const auto flush_buf = [&] {
                std::cout << std::string_view{buf, static_cast<size_t>(len)};
                len = 0;
            };

            bool stop = false;

            while (!stop) {
                if (len >= sizeof(buf)) {
                    flush_buf();
                }

                int new_r = client.recv(buf + len, sizeof(buf) - len);

                if (new_r == 0) {
                    break;
                }

                len += new_r;

                for (int i = 0; i < len; ++i) {
                    if (buf[i] == '\0') {
                        len -= 1;
                        stop = true;
                        break;
                    }
                }
            }

            flush_buf();
            std::cout << '\n';
        } while (n < str.size());
    }

    return 0;
}

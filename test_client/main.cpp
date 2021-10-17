#include <iostream>
#include <string>

#include "io/context.hpp"
#include "io/helpers.hpp"
#include "io/socket.hpp"

using namespace boutique;

int main(int argc, char** argv) {
    auto client = Socket::connect(argv[1], static_cast<unsigned short>(std::stoi(argv[2])));

    std::string str;

    for (;;) {
        std::getline(std::cin, str);

        int n = 0;

        do {
            n += client.send(str.c_str(), str.size());
        } while (n < str.size());
    }

    return 0;
}

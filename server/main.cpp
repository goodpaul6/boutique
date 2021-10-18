#include "server.hpp"

int main(int argc, char** argv) {
    boutique::Server server{static_cast<unsigned short>(std::stoi(argv[1]))};

    server.run();

    return 0;
}


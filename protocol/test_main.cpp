#include <algorithm>
#include <cassert>

#include "binary_protocol.hpp"
#include "messages.hpp"

int main(int argc, char** argv) {
    using namespace boutique;

    std::vector<char> buf;

    auto write_fn = [&](size_t len) {
        buf.resize(buf.size() + len);
        return &buf[buf.size() - len];
    };

    GetCommand get_cmd;

    get_cmd.key = "test";

    write(write_fn, get_cmd);

    ConstBuffer cbuf{buf.data(), buf.size()};

    Command cmd;

    auto res = read(cbuf, cmd);

    assert(res == ReadResult::SUCCESS);
    assert(std::holds_alternative<GetCommand>(cmd));
    assert(std::get<GetCommand>(cmd).key == get_cmd.key);

    return 0;
}

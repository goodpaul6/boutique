#include <algorithm>
#include <cassert>

#include "binary_protocol.hpp"
#include "messages.hpp"

namespace {

template <typename R, typename W, typename Fn>
void write_read_check(W&& value, Fn&& check_fn) {
    using namespace boutique;

    std::vector<char> buf;

    auto write_fn = [&](size_t len) {
        buf.resize(buf.size() + len);
        return &buf[buf.size() - len];
    };

    write(write_fn, value);

    ConstBuffer cbuf{buf.data(), buf.size()};

    R cmd;

    auto res = read(cbuf, cmd);

    assert(res == ReadResult::SUCCESS);
    check_fn(cmd);
}

}  // namespace

int main(int argc, char** argv) {
    using namespace boutique;

    GetCommand get_cmd;

    get_cmd.key = "hello";

    SetCommand set_cmd;

    set_cmd.key = "hello";
    set_cmd.value = "hello";

    write_read_check<Command>(get_cmd, [&](auto& cmd) {
        assert(std::holds_alternative<GetCommand>(cmd));
        assert(std::get<GetCommand>(cmd).key == get_cmd.key);
    });

    write_read_check<Command>(set_cmd, [&](auto& cmd) {
        assert(std::holds_alternative<SetCommand>(cmd));
        assert(std::get<SetCommand>(cmd).key == set_cmd.key);
        assert(std::get<SetCommand>(cmd).value == set_cmd.value);
    });

    return 0;
}

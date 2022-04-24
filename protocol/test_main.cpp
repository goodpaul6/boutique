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

    get_cmd.key = ConstBuffer{"hello"};

    PutCommand put_cmd;

    put_cmd.value = ConstBuffer{"hello"};

    write_read_check<Command>(get_cmd, [&](auto& cmd) {
        assert(std::holds_alternative<GetCommand>(cmd));
        assert(std::get<GetCommand>(cmd).key == get_cmd.key);
    });

    write_read_check<Command>(put_cmd, [&](auto& cmd) {
        assert(std::holds_alternative<PutCommand>(cmd));
        assert(std::get<PutCommand>(cmd).value == put_cmd.value);
    });

    FoundResponse found_res;

    found_res.value = ConstBuffer{"hello"};

    NotFoundResponse not_found_res;
    SuccessResponse success_res;

    write_read_check<Response>(found_res, [&](auto& res) {
        assert(std::holds_alternative<FoundResponse>(res));
        assert(std::get<FoundResponse>(res).value == found_res.value);
    });

    write_read_check<Response>(
        not_found_res, [&](auto& res) { assert(std::holds_alternative<NotFoundResponse>(res)); });

    write_read_check<Response>(
        success_res, [&](auto& res) { assert(std::holds_alternative<SuccessResponse>(res)); });

    return 0;
}

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
        assert(std::get<GetCommand>(cmd).key.len == get_cmd.key.len);
        assert(std::memcmp(std::get<GetCommand>(cmd).key.data, get_cmd.key.data, get_cmd.key.len) ==
               0);
    });

    write_read_check<Command>(put_cmd, [&](auto& cmd) {
        assert(std::holds_alternative<PutCommand>(cmd));
        assert(std::get<PutCommand>(cmd).value.len == put_cmd.value.len);
        assert(std::memcmp(std::get<PutCommand>(cmd).value.data, put_cmd.value.data,
                           put_cmd.value.len) == 0);
    });

    FoundResponse found_res;

    found_res.value = ConstBuffer{"hello"};

    NotFoundResponse not_found_res;
    SuccessResponse success_res;

    write_read_check<Response>(found_res, [&](auto& res) {
        assert(std::holds_alternative<FoundResponse>(res));
        assert(std::get<FoundResponse>(res).value.len == found_res.value.len);
        assert(std::memcmp(std::get<FoundResponse>(res).value.data, found_res.value.data,
                           found_res.value.len) == 0);
    });

    write_read_check<Response>(
        not_found_res, [&](auto& res) { assert(std::holds_alternative<NotFoundResponse>(res)); });

    write_read_check<Response>(
        success_res, [&](auto& res) { assert(std::holds_alternative<SuccessResponse>(res)); });

    return 0;
}

#include <cassert>
#include <cstring>
#include <iostream>
#include <optional>

#include "context.hpp"
#include "helpers.hpp"
#include "socket.hpp"
#include "timer.hpp"

int main(int argc, char** argv) {
    using namespace boutique;

    IOContext ctx;

    Timer::Params params;

    params.init_expiration = std::chrono::milliseconds{100};

    Timer timer{params};

    int expire_count = 0;

    ctx.async_wait(timer, [&](int res) {
        expire_count += res;
        ctx.stop();
    });

    ctx.run();

    assert(expire_count == 1);

    Socket server_sock{Socket::ListenParams{42690, 1}};

    std::optional<Socket> accepted_sock;

    ctx.async_accept(server_sock, [&](auto client_sock) {
        accepted_sock = std::move(client_sock);
        ctx.stop();
    });

    Socket client_sock{Socket::ConnectParams{"localhost", 42690}};

    ctx.run();

    assert(accepted_sock);

    const char s[] = "hello";

    async_send_all(ctx, client_sock, s, sizeof(s), [&](int res) {
        assert(res == sizeof(s));
        ctx.stop();
    });

    ctx.run();

    char buf[sizeof(s)];

    async_recv_all(ctx, *accepted_sock, buf, sizeof(buf), [&](int res) {
        assert(res == sizeof(s));
        assert(std::memcmp(buf, s, sizeof(buf)) == 0);

        ctx.stop();
    });

    ctx.run();

    return 0;
}

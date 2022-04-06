#include <cassert>

#include "context.hpp"
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

    return 0;
}

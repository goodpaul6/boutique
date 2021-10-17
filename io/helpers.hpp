#pragma once

#include "context.hpp"

namespace boutique {

struct Socket;

void async_recv_all(IOContext& context, Socket& socket, char* buf, size_t len, IOContext::IntFn fn);
void async_send_all(IOContext& context, Socket& socket, const char* buf, size_t len,
                    IOContext::IntFn fn);

}  // namespace boutique

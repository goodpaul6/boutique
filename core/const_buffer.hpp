#pragma once

#include <cstddef>
#include <string_view>

namespace boutique {

struct StreamBuf;

struct ConstBuffer {
    const char* data = nullptr;
    size_t len = 0;

    ConstBuffer() = default;
    ConstBuffer(const char* data, size_t len);
    explicit ConstBuffer(const StreamBuf& buf);
    explicit ConstBuffer(std::string_view s);

    void remove_prefix(size_t count);

    // FIXME I don't like that we have this
    ConstBuffer& operator+=(size_t count);
};

}  // namespace boutique

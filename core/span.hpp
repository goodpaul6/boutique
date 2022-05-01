#pragma once

#include <cassert>
#include <cstddef>

namespace boutique {

template <typename T>
struct Span {
    T* data = nullptr;
    std::size_t len = 0;

    Span() = default;

    Span(T* data, std::size_t len) : data{data}, len{len} {}

    template <std::size_t N>
    Span(T (&array)[N]) : data{array}, len{N} {}

    T* begin() { return data; }
    T* end() { return data + len; }

    std::size_t size() const { return len; }
    bool empty() const { return len == 0; }

    void remove_prefix(std::size_t count) {
        assert(count <= len);
        data += count;
        len -= count;
    }
};

}  // namespace boutique

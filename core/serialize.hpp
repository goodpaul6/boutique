#pragma once

#include <cstring>
#include <optional>
#include <type_traits>
#include <variant>
#include <vector>

#include "const_buffer.hpp"
#include "function_view.hpp"

namespace boutique {

struct NullTerminatedString {
    std::string_view s;
};

struct LengthPrefixedString {
    std::string_view s;
};

using LengthPrefixType = uint32_t;

// Should return a mutable buffer of length 'len'
using WriteFn = FunctionView<char*(size_t len)>;

using

    template <typename T>
    std::optional<T> read(ConstBuffer& c) {
    static_assert(std::is_trivially_copyable_v<T>);

    if (c.len < sizeof(T)) {
        return std::nullopt;
    }

    T v;
    std::memcpy(&v, c.data, sizeof(T));

    c += sizeof(T);

    return v;
}

template <>
std::optional<NullTerminatedString> read(ConstBuffer& c);

template <>
std::optional<LengthPrefixedString> read(ConstBuffer& c);

template <typename T>
void write(WriteFn write_fn, const T& v) {
    static_assert(std::is_trivially_copyable_v<T>);

    auto* dest = write_fn(sizeof(T));

    std::memcpy(dest, reinterpret_cast<const char*>(&v), sizeof(T));
}

void write(WriteFn write_fn, const NullTerminatedString& s);
void write(WriteFn write_fn, const LengthPrefixedString& s);

}  // namespace boutique

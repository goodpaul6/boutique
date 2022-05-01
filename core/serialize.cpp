#include "serialize.hpp"

namespace boutique {

template <>
std::optional<NullTerminatedString> read(ConstBuffer& c) {
    const char* start = c.data;
    size_t count = 0;

    while (*c.data++) {
        count++;
    }

    NullTerminatedString s{{start, count}};
    c.len -= count;

    return s;
}

template <>
std::optional<LengthPrefixedString> read(ConstBuffer& c) {
    if (c.len < sizeof(LengthPrefixType)) {
        return std::nullopt;
    }

    auto len = read<LengthPrefixType>(c);

    if (!len) {
        return std::nullopt;
    }

    if (c.len < *len) {
        return std::nullopt;
    }

    LengthPrefixedString s{{c.data, *len}};

    c.remove_prefix(*len);

    return s;
}

void write(WriteFn write_fn, const NullTerminatedString& s) {
    auto* dest = write_fn(s.s.size() + 1);

    std::memcpy(dest, s.s.data(), s.s.size());
    dest[s.s.size()] = '\0';
}

void write(WriteFn write_fn, const LengthPrefixedString& s) {
    auto* dest = write_fn(sizeof(LengthPrefixType) + s.s.size());
    auto len = static_cast<LengthPrefixType>(s.s.size());

    std::memcpy(dest, &len, sizeof(LengthPrefixType));
    std::memcpy(dest + sizeof(LengthPrefixType), s.s.data(), s.s.size());
}

}  // namespace boutique

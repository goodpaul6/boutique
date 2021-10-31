#include "binary_protocol.hpp"

#include <cassert>
#include <cstring>
#include <optional>
#include <tuple>
#include <type_traits>

#include "core/serialize.hpp"
#include "core/type_index.hpp"

namespace boutique {

ReadResult read(ConstBuffer& cursor, Command& cmd) {
    auto c = cursor;

    auto cmd_type = read<uint8_t>(c);

    if (!cmd_type) {
        return ReadResult::INCOMPLETE;
    }

    switch (*cmd_type) {
        case type_index_v<GetCommand, Command>: {
            auto key = read<LengthPrefixedString>(c);

            if (!key) {
                return ReadResult::INCOMPLETE;
            }

            cmd = GetCommand{std::move(key->s)};
        } break;

        case type_index_v<SetCommand, Command>: {
            auto key = read<LengthPrefixedString>(c);
            auto value = key ? read<LengthPrefixedString>(c) : std::nullopt;

            if (!key || !value) {
                return ReadResult::INCOMPLETE;
            }

            cmd = SetCommand{std::move(key->s), std::move(value->s)};
        } break;

        default:
            return ReadResult::INVALID;
    }

    cursor = c;
    return ReadResult::SUCCESS;
}

ReadResult read(ConstBuffer& buffer, Response& res) {
    auto b = buffer;

    auto res_type = read<uint8_t>(b);

    if (!res_type) {
        return ReadResult::INCOMPLETE;
    }

    switch (*res_type) {
        case type_index_v<SuccessResponse, Response>:
            res = SuccessResponse{};
            break;

        case type_index_v<InvalidCommandResponse, Response>:
            res = InvalidCommandResponse{};
            break;

        case type_index_v<NotFoundResponse, Response>:
            res = NotFoundResponse{};
            break;

        case type_index_v<FoundResponse, Response>: {
            auto value = read<LengthPrefixedString>(b);
            if (!value) {
                return ReadResult::INCOMPLETE;
            }

            res = FoundResponse{std::move(value->s)};
        } break;

        default:
            return ReadResult::INVALID;
    }

    buffer = b;
    return ReadResult::SUCCESS;
}

void write(WriteFn write_fn, const Command& cmd) {
    assert(!std::holds_alternative<std::monostate>(cmd));

    write(write_fn, static_cast<uint8_t>(cmd.index()));

    std::visit(
        [&](auto&& v) {
            using T = std::decay_t<decltype(v)>;

            if constexpr (std::is_same_v<T, GetCommand>) {
                write(write_fn, LengthPrefixedString{v.key});
            } else if constexpr (std::is_same_v<T, SetCommand>) {
                write(write_fn, LengthPrefixedString{v.key});
                write(write_fn, LengthPrefixedString{v.value});
            }
        },
        cmd);
}

void write(WriteFn write_fn, const Response& res) {
    assert(!std::holds_alternative<std::monostate>(res));

    write(write_fn, static_cast<uint8_t>(res.index()));

    std::visit(
        [&](auto&& v) {
            using T = std::decay_t<decltype(v)>;

            if constexpr (std::is_same_v<T, FoundResponse>) {
                write(write_fn, LengthPrefixedString{v.value});
            }
        },
        res);
}

}  // namespace boutique

#include "binary_protocol.hpp"

#include <cassert>
#include <cstring>
#include <optional>
#include <tuple>
#include <type_traits>

#include "core/overloaded_visitor.hpp"
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
        case type_index_v<RegisterSchemaCommand, Command>: {
            auto name = read<LengthPrefixedString>(c);
            auto field_count = read<std::uint32_t>(c);

            if (!name || !field_count) {
                return ReadResult::INCOMPLETE;
            }

            Schema schema;

            schema.fields.reserve(*field_count);

            for (std::uint32_t i = 0; i < *field_count; ++i) {
                auto name = read<LengthPrefixedString>(c);
                auto type_index = read<std::uint8_t>(c);

                if (!name || !type_index) {
                    return ReadResult::INCOMPLETE;
                }

                const auto push_field = [&](auto type) {
                    schema.fields.push_back({std::string{name->s}, type});
                };

                // TODO I can use a template metaprogramming thing where I visit
                // tags of each variant alternative, but it ends up being more ugly
                // than just doing this switch (I did it).
                switch (*type_index) {
                    case type_index_v<BoolType, FieldType>:
                        push_field(BoolType{});
                        break;

                    case type_index_v<UInt8Type, FieldType>:
                        push_field(UInt8Type{});
                        break;

                    case type_index_v<UInt16Type, FieldType>:
                        push_field(UInt16Type{});
                        break;

                    case type_index_v<UInt32Type, FieldType>:
                        push_field(UInt32Type{});
                        break;

                    case type_index_v<UInt64Type, FieldType>:
                        push_field(UInt64Type{});
                        break;

                    case type_index_v<Int8Type, FieldType>:
                        push_field(Int8Type{});
                        break;

                    case type_index_v<Int16Type, FieldType>:
                        push_field(Int16Type{});
                        break;

                    case type_index_v<Int32Type, FieldType>:
                        push_field(Int32Type{});
                        break;

                    case type_index_v<Int64Type, FieldType>:
                        push_field(Int64Type{});
                        break;

                    case type_index_v<Float32Type, FieldType>:
                        push_field(Float32Type{});
                        break;

                    case type_index_v<Float64Type, FieldType>:
                        push_field(Float64Type{});
                        break;

                    case type_index_v<StringType, FieldType>: {
                        auto capacity = read<std::uint32_t>(c);

                        if (!capacity) {
                            return ReadResult::INCOMPLETE;
                        }

                        push_field(StringType{*capacity});
                    } break;
                }
            }

            auto key_field_index = read<std::uint32_t>(c);

            if (!key_field_index) {
                return ReadResult::INCOMPLETE;
            }

            cmd = RegisterSchemaCommand{name->s, std::move(schema)};
        } break;

        case type_index_v<CreateCollectionCommand, Command>: {
            auto name = read<LengthPrefixedString>(c);
            auto schema_name = read<LengthPrefixedString>(c);

            if (!name || !schema_name) {
                return ReadResult::INCOMPLETE;
            }

            cmd = CreateCollectionCommand{name->s, schema_name->s};
        } break;

        case type_index_v<GetCommand, Command>: {
            auto coll_name = read<LengthPrefixedString>(c);
            auto key = read<LengthPrefixedString>(c);

            if (!coll_name || !key) {
                return ReadResult::INCOMPLETE;
            }

            cmd = GetCommand{coll_name->s, ConstBuffer{key->s}};
        } break;

        case type_index_v<SetCommand, Command>: {
            auto coll_name = read<LengthPrefixedString>(c);
            auto key = read<LengthPrefixedString>(c);
            auto value = key ? read<LengthPrefixedString>(c) : std::nullopt;

            if (!coll_name || !key || !value) {
                return ReadResult::INCOMPLETE;
            }

            cmd = SetCommand{coll_name->s, ConstBuffer{key->s}, ConstBuffer{value->s}};
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

            res = FoundResponse{ConstBuffer{value->s}};
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

    std::visit(OverloadedVisitor{
                   [&](const RegisterSchemaCommand& cmd) {
                       write(write_fn, LengthPrefixedString{cmd.name});
                       write(write_fn, static_cast<std::uint32_t>(cmd.schema.fields.size()));

                       for (const auto& field : cmd.schema.fields) {
                           write(write_fn, LengthPrefixedString{field.name});
                           write(write_fn, static_cast<std::uint8_t>(field.type.index()));

                           if (std::holds_alternative<StringType>(field.type)) {
                               write(write_fn, static_cast<std::uint32_t>(
                                                   std::get<StringType>(field.type).capacity));
                           }
                       }
                   },
                   [&](const CreateCollectionCommand& cmd) {
                       write(write_fn, LengthPrefixedString{cmd.name});
                       write(write_fn, LengthPrefixedString{cmd.schema_name});
                   },
                   [&](const GetCommand& cmd) {
                       write(write_fn, LengthPrefixedString{cmd.coll_name});
                       write(write_fn, LengthPrefixedString{{cmd.key.data, cmd.key.len}});
                   },
                   [&](const SetCommand& cmd) {
                       write(write_fn, LengthPrefixedString{cmd.coll_name});
                       write(write_fn, LengthPrefixedString{{cmd.key.data, cmd.key.len}});
                       write(write_fn, LengthPrefixedString{{cmd.value.data, cmd.value.len}});
                   },
                   [](auto) {}},
               cmd);
}

void write(WriteFn write_fn, const Response& res) {
    assert(!std::holds_alternative<std::monostate>(res));

    write(write_fn, static_cast<uint8_t>(res.index()));

    std::visit(OverloadedVisitor{
                   [&](const FoundResponse& res) {
                       write(write_fn, LengthPrefixedString{{res.value.data, res.value.len}});
                   },
                   [](auto) {}},
               res);
}

}  // namespace boutique

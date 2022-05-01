#include <algorithm>
#include <cassert>
#include <iostream>
#include <string>
#include <string_view>
#include <unordered_map>

#include "core/formatter.hpp"
#include "core/overloaded_visitor.hpp"
#include "core/streambuf.hpp"
#include "core/tag.hpp"
#include "io/socket.hpp"
#include "protocol/binary_protocol.hpp"

using namespace boutique;

namespace {

template <typename Fn>
void for_each_primitive_type_name(Fn&& fn) {
    fn(Tag<BoolType>{}, "bool");

    fn(Tag<UInt8Type>{}, "uint8");
    fn(Tag<UInt16Type>{}, "uint16");
    fn(Tag<UInt32Type>{}, "uint32");
    fn(Tag<UInt64Type>{}, "uint64");

    fn(Tag<Int8Type>{}, "int8");
    fn(Tag<Int16Type>{}, "int16");
    fn(Tag<Int32Type>{}, "int32");
    fn(Tag<Int64Type>{}, "int64");

    fn(Tag<Float32Type>{}, "float32");
    fn(Tag<Float64Type>{}, "float64");

    fn(Tag<StringType>{}, "string");
}

void print_aggregate_type(const AggregateType& agg, int key_field_index = -1, int indent = 0) {
    auto field_index = 0;

    for (const auto& field : agg) {
        for (int i = 0; i < indent; ++i) {
            std::cout << '\t';
        }

        if (field_index == key_field_index) {
            std::cout << "key ";
        }

        field_index++;

        std::cout << field.name << ' ';

        std::visit(OverloadedVisitor{
                       [&](const StringType& s) { std::cout << "string" << s.capacity << '\n'; },
                       [&](const AggregateType& a) {
                           std::cout << '\n';
                           print_aggregate_type(a, -1, indent + 1);
                       },
                       [](auto type) {
                           using A = std::decay_t<decltype(type)>;

                           for_each_primitive_type_name([&](auto tag, std::string_view name) {
                               using B = typename decltype(tag)::Type;

                               if constexpr (std::is_same_v<A, B>) {
                                   std::cout << name;
                               }
                           });

                           std::cout << '\n';
                       }},
                   field.type);
    }
}

void print_aggregate(const AggregateType& agg, const void* data, int indent = 0) {
    auto field_index = 0;

    for (const auto& field : agg) {
        for (int i = 0; i < indent; ++i) {
            std::cout << '\t';
        }

        std::cout << field.name << " = ";

        // TODO Optimize O(n^2) offset calculation here
        auto* value = reinterpret_cast<const char*>(data) + offset(agg, field_index);
        field_index++;

        std::visit(
            OverloadedVisitor{
                [&](StringType) {
                    const auto* hdr = reinterpret_cast<const StringHeader*>(value);
                    std::cout << '"' << std::string_view{value + sizeof(*hdr), hdr->len} << "\"\n";
                },
                [&](const AggregateType& a) {
                    std::cout << '\n';
                    print_aggregate(a, value, indent + 1);
                },
                [&](auto&& t) {
                    using T = typename ImplType<std::decay_t<decltype(t)>>::Type;

                    std::cout << *reinterpret_cast<const T*>(value) << '\n';
                }},
            field.type);
    }
};

void read_aggregate(const AggregateType& agg, void* dest, bool show_prompts, int indent = 0);

void read_field(const Field& field, void* dest, bool show_prompts, int indent = 0) {
    if (show_prompts) {
        for (int i = 0; i < indent; ++i) {
            std::cout << '\t';
        }

        std::cout << format("value for {} > ", field.name);
    }

    // HACK Assumes write is only called once below for this field
    auto write_fn = [&](std::size_t len) { return reinterpret_cast<char*>(dest); };

    std::visit(OverloadedVisitor{
                   [&](const StringType& s) {
                       std::string value;
                       std::getline(std::cin, value);

                       auto cap = s.capacity;

                       if (value.size() > cap) {
                           std::cerr << "String too long to fit in type, trimming to " << cap
                                     << " bytes.\n";
                           value = value.substr(0, cap);
                       }

                       static_assert(std::is_same_v<LengthPrefixType, decltype(StringHeader::len)>);

                       write(write_fn, LengthPrefixedString{value});
                   },
                   [&](const AggregateType& a) {
                       if (show_prompts) {
                           std::cout << '\n';
                       }

                       read_aggregate(a, dest, show_prompts, indent + 1);
                   },
                   [&](auto type) {
                       using T = std::decay_t<decltype(type)>;

                       std::string value;
                       std::getline(std::cin, value);

                       if constexpr (std::is_same_v<T, BoolType>) {
                           write(write_fn, value == "true");
                       } else if constexpr (std::is_integral_v<impl_type_t<T>>) {
                           if constexpr (std::is_unsigned_v<impl_type_t<T>>) {
                               write(write_fn, static_cast<impl_type_t<T>>(std::stoull(value)));
                           } else {
                               write(write_fn, static_cast<impl_type_t<T>>(std::stoll(value)));
                           }
                       } else if constexpr (std::is_floating_point_v<impl_type_t<T>>) {
                           write(write_fn, static_cast<impl_type_t<T>>(std::stod(value)));
                       }

                       // TODO Check exhaustiveness with a static_assert
                   }},
               field.type);
}

void read_aggregate(const AggregateType& agg, void* dest, bool show_prompts, int indent) {
    auto field_index = 0;

    for (const auto& field : agg) {
        read_field(field, reinterpret_cast<char*>(dest) + offset(agg, field_index), show_prompts,
                   indent);
        field_index += 1;
    }
}

}  // namespace

int main(int argc, char** argv) {
    Socket client{Socket::ConnectParams{argv[1], static_cast<unsigned short>(std::stoi(argv[2]))}};

    bool show_prompts = true;

    for (int i = 3; i < argc; ++i) {
        if (std::strcmp(argv[i], "--quiet") == 0 || std::strcmp(argv[i], "-q") == 0) {
            show_prompts = false;
        }
    }

    const auto prompt = [&](std::string_view s, auto&&... args) {
        if (!show_prompts) {
            return;
        }

        std::cout << format(s, std::forward<decltype(args)>(args)...);
    };

    client.set_non_blocking(false);

    std::unordered_map<std::string, Schema> schemas;
    std::unordered_map<std::string, Schema> colschemas;

    std::string str;
    std::string str2;

    StreamBuf stream;

    for (;;) {
        assert(stream.empty());

        prompt("> ");

        if (!std::getline(std::cin, str)) {
            break;
        } else if (str.empty()) {
            continue;
        } else if (str == "quit") {
            std::cout << "Goodbye.\n";
            break;
        }

        std::vector<char> buf;

        Command cmd;

        if (str == "schema") {
            std::string name;

            prompt("name > ");
            std::getline(std::cin, name);

            Schema schema;

            for (;;) {
                prompt("field name or end > ");
                std::getline(std::cin, str);

                if (str == "end") {
                    if (schema.fields.empty()) {
                        std::cerr << "Must provide at least one field.\n";
                        continue;
                    }

                    break;
                }

                Field field;

                field.name = std::move(str);
                str.clear();

                prompt("field type > ");
                std::getline(std::cin, str);

                if (str == "aggregate") {
                    prompt("subschema name > ");
                    std::getline(std::cin, str);

                    auto found = schemas.find(str);

                    if (found == schemas.end()) {
                        std::cerr << "Could not find this schema. Run getschema first.\n";
                        continue;
                    }

                    field.type = found->second.fields;
                } else {
                    bool found = false;

                    for_each_primitive_type_name([&](auto tag, std::string_view name) {
                        using T = typename decltype(tag)::Type;

                        if (found || str != name) {
                            return;
                        }

                        found = true;

                        if constexpr (std::is_same_v<T, StringType>) {
                            prompt("string capacity > ");
                            std::getline(std::cin, str);

                            field.type = StringType{std::stoul(str)};
                        } else {
                            field.type = T();
                        }
                    });

                    if (!found) {
                        std::cerr << "Invalid type.\n";
                        continue;
                    }
                }

                schema.fields.emplace_back(std::move(field));
            }

            for (;;) {
                prompt("key field name > ");
                std::getline(std::cin, str);

                schema.key_field_index =
                    std::find_if(schema.fields.begin(), schema.fields.end(),
                                 [&](auto& field) { return field.name == str; }) -
                    schema.fields.begin();

                if (schema.key_field_index >= schema.fields.size()) {
                    std::cerr << "Invalid key name " << str << '\n';
                    continue;
                }

                if (std::holds_alternative<AggregateType>(
                        schema.fields[schema.key_field_index].type)) {
                    std::cerr << "Cannot have aggregate as key\n";
                    continue;
                }

                str = std::move(name);
                break;
            }

            cmd = RegisterSchemaCommand{str, std::move(schema)};
        } else if (str == "collection") {
            prompt("name > ");
            std::getline(std::cin, str);

            prompt("schema name > ");
            std::getline(std::cin, str2);

            cmd = CreateCollectionCommand{str, str2};
        } else if (str == "getschema") {
            prompt("name > ");
            std::getline(std::cin, str);

            cmd = GetSchemaCommand{str};
        } else if (str == "colschema") {
            prompt("name > ");
            std::getline(std::cin, str);

            cmd = GetCollectionSchemaCommand{str};
        } else if (str == "get" || str == "delete") {
            std::string cmd_name{std::move(str)};

            prompt("collection name > ");
            std::getline(std::cin, str2);

            auto found = colschemas.find(str2);

            if (found == colschemas.end()) {
                std::cerr << "Run colschema to cache the schema for this collection first.\n";
                continue;
            }

            const auto& field = found->second.fields[found->second.key_field_index];

            buf.resize(size(field.type));

            read_field(field, buf.data(), show_prompts);

            ConstBuffer key{buf.data(), buf.size()};

            if (std::holds_alternative<StringType>(field.type)) {
                LengthPrefixType len;
                std::memcpy(&len, buf.data(), sizeof(LengthPrefixType));

                key = {buf.data() + sizeof(LengthPrefixType), len};
            }

            if (cmd_name == "get") {
                cmd = GetCommand{str2, key};
            } else {
                cmd = DeleteCommand{str2, key};
            }
        } else if (str == "put") {
            std::string coll_name;

            prompt("collection name > ");
            std::getline(std::cin, coll_name);

            auto found_schema = colschemas.find(coll_name);

            if (found_schema == colschemas.end()) {
                std::cerr << "Run colschema to cache the schema for this collection first.\n";
                continue;
            }

            buf.resize(size(found_schema->second));

            read_aggregate(found_schema->second.fields, buf.data(), show_prompts);

            cmd = PutCommand{coll_name, ConstBuffer{buf.data(), buf.size()}};
        } else {
            std::cout << "Unknown command.\n";
            continue;
        }

        // TODO Refactor this since its copied from the client_handler.cpp
        std::vector<char> cmd_buf;

        auto cmd_buf_writer = [&](size_t len) {
            cmd_buf.resize(cmd_buf.size() + len);
            auto* ptr = cmd_buf.data() + cmd_buf.size() - len;

            return ptr;
        };

        write(cmd_buf_writer, cmd);

        int n = 0;

        do {
            int r = client.send(cmd_buf.data() + n, cmd_buf.size() - n);

            if (r == 0) {
                std::cerr << "Failed to send command.\n";
                break;
            }

            n += r;
        } while (n < cmd_buf.size());

        // Keep reading until we get one response
        char recv_buf[128];

        do {
            n = client.recv(recv_buf, sizeof(recv_buf));

            stream.append(recv_buf, n);

            auto res_buf = as_const_buffer(stream);

            Response res;

            auto r = read(res_buf, res);

            if (r == ReadResult::INVALID) {
                std::cerr << "Received invalid response from server.\n";
                stream.consume(stream.size());
                break;
            } else if (r == ReadResult::SUCCESS) {
                std::visit(
                    [&](auto&& v) {
                        using T = std::decay_t<decltype(v)>;

                        if constexpr (std::is_same_v<T, InvalidCommandResponse>) {
                            std::cerr << "Apparently we sent an invalid command.\n";
                        } else if constexpr (std::is_same_v<T, SuccessResponse>) {
                            std::cout << "Success.\n";
                        } else if constexpr (std::is_same_v<T, NotFoundResponse>) {
                            std::cout << "Not found.\n";
                        } else if constexpr (std::is_same_v<T, FoundResponse>) {
                            if (std::holds_alternative<GetCommand>(cmd)) {
                                auto found = colschemas.find(
                                    std::string{std::get<GetCommand>(cmd).coll_name});

                                if (found == colschemas.end()) {
                                    std::cout << "No schema cached for this so data cannot be "
                                                 "displayed well. Run colschema.\n";
                                    return;
                                }

                                print_aggregate(found->second.fields, v.value.data);
                            } else {
                                std::cout << std::string_view{v.value.data, v.value.len} << '\n';
                            }
                        } else if constexpr (std::is_same_v<T, SchemaResponse>) {
                            auto* schema = &v.schema;

                            if (auto* schema_cmd = std::get_if<GetSchemaCommand>(&cmd)) {
                                auto [iter, inserted] = schemas.insert_or_assign(
                                    std::string{schema_cmd->name}, std::move(v.schema));

                                schema = &iter->second;
                            } else if (auto* schema_cmd =
                                           std::get_if<GetCollectionSchemaCommand>(&cmd)) {
                                auto [iter, inserted] = colschemas.insert_or_assign(
                                    std::string{schema_cmd->name}, std::move(v.schema));

                                schema = &iter->second;
                            }

                            print_aggregate_type(schema->fields, schema->key_field_index);
                        }
                    },
                    res);

                stream.consume(res_buf.data - stream.data());
            }
        } while (!stream.empty());
    }

    return 0;
}

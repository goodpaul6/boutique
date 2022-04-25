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
void for_each_type_name(Fn&& fn) {
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

                bool found = false;

                for_each_type_name([&](auto tag, std::string_view name) {
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

                schema.fields.emplace_back(std::move(field));
            }

            prompt("key field name > ");

            std::getline(std::cin, str);

            schema.key_field_index = std::find_if(schema.fields.begin(), schema.fields.end(),
                                                  [&](auto& field) { return field.name == str; }) -
                                     schema.fields.begin();

            if (schema.key_field_index >= schema.fields.size()) {
                std::cerr << "Invalid key name " << str << '\n';
                continue;
            }

            str = std::move(name);

            cmd = RegisterSchemaCommand{str, std::move(schema)};
        } else if (str == "collection") {
            prompt("name > ");
            std::getline(std::cin, str);

            prompt("schema name > ");
            std::getline(std::cin, str2);

            cmd = CreateCollectionCommand{str, str2};
        } else if (str == "colschema") {
            prompt("name > ");
            std::getline(std::cin, str);

            cmd = GetCollectionSchemaCommand{str};
        } else if (str == "get") {
            prompt("collection name > ");

            std::getline(std::cin, str);

            prompt("key > ");

            std::getline(std::cin, str2);

            cmd = GetCommand{str, ConstBuffer{str2}};
        } else if (str == "put") {
            prompt("collection name > ");

            std::string coll_name;

            std::getline(std::cin, coll_name);

            auto found_schema = schemas.find(coll_name);

            if (found_schema == schemas.end()) {
                std::cerr << "Run colschema to cache the schema for this collection first.\n";
                continue;
            }

            auto field_index = 0;

            buf.resize(size(found_schema->second));

            for (const auto& field : found_schema->second.fields) {
                auto write_pos = offset(found_schema->second, field_index);
                field_index++;

                auto buf_writer = [&](std::size_t len) { return buf.data() + write_pos; };

                for_each_type_name([&](auto tag, std::string_view name) {
                    using T = typename decltype(tag)::Type;

                    if (!std::holds_alternative<T>(field.type)) {
                        return;
                    }

                    prompt("value for {} {} > ", name, field.name);

                    std::getline(std::cin, str);

                    if constexpr (std::is_same_v<T, StringType>) {
                        static_assert(
                            std::is_same_v<LengthPrefixType, decltype(StringHeader::len)>);

                        auto cap = std::get<StringType>(field.type).capacity;

                        if (str.size() > cap) {
                            std::cerr << "String too long to fit in type, trimming to " << cap
                                      << " bytes.\n";
                            str = str.substr(0, cap);
                        }

                        write(buf_writer, LengthPrefixedString{str});
                    } else {
                        if constexpr (std::is_same_v<T, BoolType>) {
                            write(buf_writer, str == "true");
                        } else if constexpr (std::is_integral_v<impl_type_t<T>>) {
                            if constexpr (std::is_unsigned_v<impl_type_t<T>>) {
                                write(buf_writer, static_cast<impl_type_t<T>>(std::stoull(str)));
                            } else {
                                write(buf_writer, static_cast<impl_type_t<T>>(std::stoll(str)));
                            }
                        } else if constexpr (std::is_floating_point_v<impl_type_t<T>>) {
                            write(buf_writer, static_cast<impl_type_t<T>>(std::stod(str)));
                        }

                        // TODO Check exhaustiveness with a static_assert
                    }
                });
            }

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

            ConstBuffer res_buf{stream};

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
                                auto found =
                                    schemas.find(std::string{std::get<GetCommand>(cmd).coll_name});

                                if (found == schemas.end()) {
                                    std::cout << "No schema cached for this so data cannot be "
                                                 "displayed well. Run colschema.\n";
                                    return;
                                }

                                auto field_index = 0;
                                for (const auto& field : found->second.fields) {
                                    std::cout << field.name << " = ";

                                    auto* value = v.value.data + offset(found->second, field_index);
                                    field_index++;

                                    std::visit(
                                        OverloadedVisitor{
                                            [&](StringType) {
                                                const auto* hdr =
                                                    reinterpret_cast<const StringHeader*>(value);
                                                std::cout << '"'
                                                          << std::string_view{value + sizeof(*hdr),
                                                                              hdr->len}
                                                          << "\"\n";
                                            },
                                            [&](auto&& t) {
                                                using T = typename ImplType<
                                                    std::decay_t<decltype(t)>>::Type;

                                                std::cout << *reinterpret_cast<const T*>(value)
                                                          << '\n';
                                            }},
                                        field.type);
                                }
                            } else {
                                std::cout << std::string_view{v.value.data, v.value.len} << '\n';
                            }
                        } else if constexpr (std::is_same_v<T, SchemaResponse>) {
                            auto* schema = &v.schema;

                            if (auto* schema_cmd = std::get_if<GetCollectionSchemaCommand>(&cmd)) {
                                auto [iter, inserted] = schemas.insert_or_assign(
                                    std::string{schema_cmd->name}, std::move(v.schema));

                                schema = &iter->second;
                            }

                            auto i = 0;
                            for (const auto& field : schema->fields) {
                                if (i == schema->key_field_index) {
                                    std::cout << "key ";
                                }

                                std::cout << field.name << ' ';

                                for_each_type_name([&](auto tag, std::string_view name) {
                                    using T = typename decltype(tag)::Type;

                                    if (std::holds_alternative<T>(field.type)) {
                                        std::cout << name;

                                        if constexpr (std::is_same_v<T, StringType>) {
                                            std::cout << std::get<T>(field.type).capacity;
                                        }

                                        std::cout << '\n';
                                    }
                                });

                                i++;
                            }
                        }
                    },
                    res);

                stream.consume(res_buf.data - stream.data());
            }
        } while (!stream.empty());
    }

    return 0;
}

#pragma once

#include <string_view>
#include <variant>

#include "core/const_buffer.hpp"
#include "db/schema.hpp"

namespace boutique {

struct RegisterSchemaCommand {
    std::string_view name;
    // TODO Create SchemaView type that can be a view into the stream buffer
    Schema schema;
};

struct CreateCollectionCommand {
    std::string_view name;
    std::string_view schema_name;
};

struct GetCollectionSchemaCommand {
    std::string_view name;
};

struct GetCommand {
    std::string_view coll_name;
    ConstBuffer key;
};

struct PutCommand {
    std::string_view coll_name;
    ConstBuffer value;
};

struct DeleteCommand {
    std::string_view coll_name;
    ConstBuffer key;
};

using Command = std::variant<std::monostate, RegisterSchemaCommand, CreateCollectionCommand,
                             GetCollectionSchemaCommand, GetCommand, PutCommand, DeleteCommand>;

struct SuccessResponse {};
struct InvalidCommandResponse {};
struct NotFoundResponse {};

struct FoundResponse {
    ConstBuffer value;
};

struct SchemaResponse {
    Schema schema;
};

using Response = std::variant<std::monostate, SuccessResponse, InvalidCommandResponse,
                              NotFoundResponse, FoundResponse, SchemaResponse>;

}  // namespace boutique

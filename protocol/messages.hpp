#pragma once

#include <string_view>
#include <variant>

#include "core/const_buffer.hpp"
#include "core/span.hpp"
#include "db/schema.hpp"

namespace boutique {

struct RegisterSchemaCommand {
    std::string_view name;
    Schema schema;
};

struct CreateCollectionCommand {
    std::string_view name;
    std::string_view schema_name;
};

struct GetSchemaCommand {
    std::string_view name;
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

using Command =
    std::variant<std::monostate, RegisterSchemaCommand, CreateCollectionCommand, GetSchemaCommand,
                 GetCollectionSchemaCommand, GetCommand, PutCommand, DeleteCommand>;

struct SuccessResponse {};

// Generic failure, no good reason for out
struct FailedResponse {};

struct InvalidCommandResponse {};
struct NotFoundResponse {};

struct FoundResponse {
    ConstBuffer value;
};

struct StringResponse {
    std::string_view value;
};

struct SchemaResponse {
    Schema schema;
};

using Response =
    std::variant<std::monostate, SuccessResponse, FailedResponse, InvalidCommandResponse,
                 NotFoundResponse, FoundResponse, StringResponse, SchemaResponse>;

}  // namespace boutique

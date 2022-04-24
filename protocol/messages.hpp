#pragma once

#include <string_view>
#include <variant>

#include "core/const_buffer.hpp"
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

struct GetCommand {
    std::string_view coll_name;
    ConstBuffer key;
};

struct SetCommand {
    std::string_view coll_name;
    ConstBuffer key;
    ConstBuffer value;
};

using Command = std::variant<std::monostate, RegisterSchemaCommand, CreateCollectionCommand,
                             GetCommand, SetCommand>;

struct SuccessResponse {};
struct InvalidCommandResponse {};
struct NotFoundResponse {};

struct FoundResponse {
    ConstBuffer value;
};

using Response = std::variant<std::monostate, SuccessResponse, InvalidCommandResponse,
                              NotFoundResponse, FoundResponse>;

}  // namespace boutique

#pragma once

#include <string_view>
#include <variant>

namespace boutique {

struct GetCommand {
    std::string_view key;
};

struct SetCommand {
    std::string_view key;
    std::string_view value;
};

using Command = std::variant<std::monostate, GetCommand, SetCommand>;

struct SuccessResponse {};
struct InvalidCommandResponse {};
struct NotFoundResponse {};

struct FoundResponse {
    std::string_view value;
};

using Response = std::variant<std::monostate, SuccessResponse, InvalidCommandResponse,
                              NotFoundResponse, FoundResponse>;

}  // namespace boutique

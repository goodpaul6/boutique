#pragma once

#include <string_view>
#include <variant>
#include <vector>

#include "core/serialize.hpp"
#include "messages.hpp"

namespace boutique {

enum class ReadResult { SUCCESS, INCOMPLETE, INVALID };

[[nodiscard]] ReadResult read(ConstBuffer& b, Command& cmd);
[[nodiscard]] ReadResult read(ConstBuffer& b, Response& res);

void write(WriteFn write_fn, const Command& cmd);
void write(WriteFn write_fn, const Response& res);

}  // namespace boutique

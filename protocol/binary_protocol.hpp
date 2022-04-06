#pragma once

#include <string_view>
#include <variant>
#include <vector>

#include "core/serialize.hpp"
#include "messages.hpp"

namespace boutique {

enum class ReadResult { SUCCESS, INCOMPLETE, INVALID };

// TODO This function actually mutates the buffer passed in if the read is complete.
// That's a little confusing because the buffer is called a ConstBuffer.
[[nodiscard]] ReadResult read(ConstBuffer& b, Command& cmd);
[[nodiscard]] ReadResult read(ConstBuffer& b, Response& res);

void write(WriteFn write_fn, const Command& cmd);
void write(WriteFn write_fn, const Response& res);

}  // namespace boutique

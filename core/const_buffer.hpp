#pragma once

#include <cstddef>
#include <string_view>

#include "span.hpp"

namespace boutique {

struct StreamBuf;

using ConstBuffer = Span<const char>;

ConstBuffer as_const_buffer(std::string_view s);
ConstBuffer as_const_buffer(const StreamBuf& buf);

}  // namespace boutique

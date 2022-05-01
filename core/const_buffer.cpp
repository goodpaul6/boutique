#include "const_buffer.hpp"

#include <cassert>

#include "streambuf.hpp"

namespace boutique {

ConstBuffer as_const_buffer(std::string_view s) { return {s.data(), s.size()}; }
ConstBuffer as_const_buffer(const StreamBuf& buf) { return {buf.data(), buf.size()}; }

}  // namespace boutique

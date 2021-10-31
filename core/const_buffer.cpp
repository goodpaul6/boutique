#include "const_buffer.hpp"

#include <cassert>

#include "streambuf.hpp"

namespace boutique {

ConstBuffer::ConstBuffer(const char* data, size_t len) : data{data}, len{len} {}
ConstBuffer::ConstBuffer(const StreamBuf& buf) : data{buf.data()}, len{buf.size()} {}
ConstBuffer::ConstBuffer(std::string_view s) : data{s.data()}, len{s.size()} {}

void ConstBuffer::remove_prefix(size_t count) {
    assert(count <= len);

    data += count;
    len -= count;
}

ConstBuffer& ConstBuffer::operator+=(size_t count) {
    remove_prefix(count);

    return *this;
}

}  // namespace boutique

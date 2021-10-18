#include "streambuf.hpp"

#include <cassert>

namespace boutique {

void StreamBuf::append(const char* data, size_t len) {
    m_buf.insert(m_buf.end(), data, data + len);
    m_size += len;
}

void StreamBuf::consume(size_t len) {
    m_used += len;

    assert(m_used <= m_size);

    if (m_used == m_size) {
        m_size = 0;
        m_used = 0;
        m_buf.clear();
    }
}

const char* StreamBuf::data() const { return m_buf.data() + m_used; }
size_t StreamBuf::size() const { return m_size - m_used; }

bool StreamBuf::empty() const { return m_size == 0; }

}  // namespace boutique

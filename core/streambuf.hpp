#pragma once

#include <vector>

namespace boutique {

struct StreamBuf {
    void append(const char* data, size_t len);
    void consume(size_t len);

    const char* data() const;
    size_t size() const;

    bool empty() const;

private:
    size_t m_size = 0;
    size_t m_used = 0;
    std::vector<char> m_buf;
};

}  // namespace boutique

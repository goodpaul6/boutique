#pragma once

#include <cstdint>
#include <vector>

namespace boutique {

struct Storage {
    Storage(std::size_t doc_size);

    void* put(const void* elem_data);
    void remove(const void* elem_ptr);
    void clear();

    void* operator[](std::ptrdiff_t index);

    bool empty() const;
    std::size_t count() const;
    std::size_t doc_size() const;

private:
    std::size_t m_doc_size = 0;

    // TODO Make the implementation similar to deque so that we have
    // amortized O(1) put.

    // HACK We depend on the fact that vector uses operator new under the hood
    // and assume the data will be sufficiently aligned.
    std::vector<char> m_data;
    size_t m_count = 0;
};

}  // namespace boutique

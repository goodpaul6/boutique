#include "storage.hpp"

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstring>

namespace {

constexpr std::size_t init_cap = 16;

}

namespace boutique {

Storage::Storage(std::size_t doc_size) : m_doc_size{doc_size} {}

void* Storage::put(const void* elem_data) {
    while (m_data.size() < m_doc_size * (m_count + 1)) {
        m_data.resize(std::max(m_data.size() * 2, m_doc_size * init_cap));
    }

    // Check that the resulting data pointer is maximally aligned
    assert(reinterpret_cast<std::uintptr_t>(m_data.data()) % alignof(std::max_align_t) == 0);

    std::memcpy(m_data.data() + m_doc_size * m_count, elem_data, m_doc_size);
    m_count += 1;

    return m_data.data() + m_doc_size * m_count;
}

void Storage::remove(const void* elem_ptr) {
    assert(m_count > 0);
    assert((reinterpret_cast<const char*>(elem_ptr) - m_data.data()) / m_doc_size < m_count);

    const void* last_elem_ptr = m_data.data() + m_doc_size * (m_count - 1);

    if (elem_ptr != last_elem_ptr) {
        std::memcpy(const_cast<void*>(elem_ptr), last_elem_ptr, m_doc_size);
    }

    m_count -= 1;
}

void Storage::clear() { m_count = 0; }

void* Storage::operator[](std::ptrdiff_t index) {
    return reinterpret_cast<void*>(m_data.data() + index * m_doc_size);
}

bool Storage::empty() const { return m_count == 0; }

std::size_t Storage::count() const { return m_count; }

std::size_t Storage::doc_size() const { return m_doc_size; }

}  // namespace boutique

#include "collection.hpp"

#include <cassert>
#include <cstring>

#include "schema.hpp"

namespace {

constexpr std::size_t init_cap = 16;

}

namespace boutique {

Collection::Collection(const Schema& schema) : m_schema{&schema}, m_doc_size{size(schema)} {}

void Collection::put(const void* elem_data) {
    while (m_data.size() < m_doc_size * (m_count + 1)) {
        m_data.resize(std::max(m_data.size() * 2, m_doc_size * init_cap));
    }

    std::memcpy(m_data.data() + m_doc_size * m_count, elem_data, m_doc_size);
    m_count += 1;
}

void Collection::remove(const void* elem_ptr) {
    assert(m_count > 0);
    assert((reinterpret_cast<const char*>(elem_ptr) - m_data.data()) / m_doc_size < m_count);

    const void* last_elem_ptr = m_data.data() + m_doc_size * (m_count - 1);

    if (elem_ptr != last_elem_ptr) {
        std::memcpy(const_cast<void*>(elem_ptr), last_elem_ptr, m_doc_size);
    }

    m_count -= 1;
}

void Collection::clear() { m_count = 0; }

void* Collection::operator[](std::ptrdiff_t index) {
    return reinterpret_cast<void*>(m_data.data() + index * m_doc_size);
}

bool Collection::empty() const { return m_count == 0; }

std::size_t Collection::count() const { return m_count; }

const Schema& Collection::schema() const { return *m_schema; }

}  // namespace boutique

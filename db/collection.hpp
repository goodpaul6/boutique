#pragma once

#include <cstddef>
#include <vector>

namespace boutique {

struct Schema;

struct Collection {
    // TODO Make this a open-addressed hash table based on a 'key' field that is
    // received as an argument when creating the collection.

    Collection(const Schema& schema);

    void put(const void* elem_data);
    void remove(const void* elem_ptr);
    void clear();

    void* operator[](std::ptrdiff_t index);

    bool empty() const;
    std::size_t count() const;
    const Schema& schema() const;

private:
    const Schema* m_schema = nullptr;
    // We cache this when we receive the schema
    std::size_t m_doc_size = 0;

    // TODO Make the implementation similar to deque so that we have
    // amortized O(1) put.

    // HACK We depend on the fact that vector uses operator new under the hood
    // and assume the data will be sufficiently aligned.
    std::vector<char> m_data;
    size_t m_count = 0;
};

}  // namespace boutique

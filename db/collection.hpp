#pragma once

#include <cassert>
#include <cstddef>
#include <cstring>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "core/const_buffer.hpp"
#include "schema.hpp"
#include "storage.hpp"

namespace boutique {

struct Collection {
    Collection(Schema schema);

    void* put(const void* elem_data);

    void remove(ConstBuffer key);

    // If the key type is a string, we convert the ConstBuffer to a string_view
    // and perform the lookup using that.
    void* find(ConstBuffer key);

    const Schema& schema() const;
    std::size_t count() const;

private:
    using Index = std::unordered_multimap<std::size_t, std::uint64_t>;

    // We copy the schema into the collection since we don't want it to be modified
    // without the collection's knowledge.
    Schema m_schema;

    Storage m_storage;
    Index m_key_hash_to_index;

    std::pair<void*, decltype(m_key_hash_to_index)::iterator> find_internal(ConstBuffer key,
                                                                            std::size_t key_hash);
};

}  // namespace boutique

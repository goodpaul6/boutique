#pragma once

#include <cassert>
#include <cstddef>
#include <cstring>
#include <functional>
#include <string_view>
#include <vector>

#include "core/const_buffer.hpp"
#include "schema.hpp"

namespace boutique {

struct Collection {
    Collection(Schema schema);

    void* put(const void* data);

    void remove(ConstBuffer key);

    // If the key type is a string, we convert the ConstBuffer to a string_view
    // and perform the lookup using that.
    void* find(ConstBuffer key);

    const Schema& schema() const;
    std::size_t count() const;

private:
    // We copy the schema into the collection since we don't want it to be modified
    // without the collection's knowledge.
    Schema m_schema;

    // Cache the document size
    std::size_t m_doc_size = 0;

    // The internal data structure is an open-addressed hash table.
    // Keys are embedded within the documents and we have empty/tombstone
    // representations for all the keys (0x000000...) so you can't
    // have zeros as a key boohoo or 0xFFFFF.... :'(
    std::vector<char> m_data;
    std::size_t m_bucket_count = 0;
    std::size_t m_count = 0;

    char* find_internal(ConstBuffer key, std::size_t key_hash);
    char* put_internal(const void* elem_data, std::vector<char>& dest, std::size_t bucket_count);
};

}  // namespace boutique

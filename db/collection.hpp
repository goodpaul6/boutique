#pragma once

#include <cassert>
#include <cstddef>
#include <cstring>
#include <functional>
#include <string_view>
#include <vector>

#include "core/const_buffer.hpp"
#include "schema.hpp"
#include "storage.hpp"

namespace boutique {

struct Collection {
    Collection(Schema schema);

    void* put(const void* data);

    void remove(ConstBuffer key);

    // If the key type is a string, we convert the ConstBuffer to a string_view
    // and perform the lookup using that.
    void* find(ConstBuffer key);

    const Schema& schema() const;
    std::size_t doc_size() const;

    std::size_t count() const;

private:
    // We copy the schema into the collection since we don't want it to be modified
    // without the collection's knowledge.
    Schema m_schema;

    // We cache this because computing the offset can be a bottleneck
    std::size_t m_key_offset = 0;

    ConstBuffer (*m_key_buffer_fn)(const void* data, std::size_t key_offset);

    // We cache this because it never changes with a constant schema
    std::size_t (*m_hash_fn)(ConstBuffer);

    // This stores all of the documents contiguously. This does not retain insertion order
    // as removing swaps the last element in the storage with the removed element.
    Storage m_storage;

    struct KeyValue {
        std::size_t key_hash;
        std::size_t value_index;
    };

    std::vector<KeyValue> m_buckets;

    KeyValue* find_internal(ConstBuffer key, std::size_t key_hash);
    KeyValue* put_internal(std::vector<KeyValue>& dest, ConstBuffer key, std::size_t key_hash);
};

}  // namespace boutique

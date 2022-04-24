#include "collection.hpp"

#include <algorithm>

#include "core/logger.hpp"
#include "core/overloaded_visitor.hpp"
#include "core/time_tracker.hpp"

namespace {

const std::size_t TOMBSTONE_KEY_HASH = ~0;

}  // namespace

namespace boutique {

Collection::Collection(Schema schema)
    : m_schema{std::move(schema)},
      m_storage{size(m_schema)},
      m_key_offset{offset(m_schema, m_schema.key_field_index)} {
    std::visit(
        OverloadedVisitor{
            [&](StringType) {
                m_key_buffer_fn = [](const void* data, std::size_t key_offset) -> ConstBuffer {
                    const auto* key = reinterpret_cast<const char*>(data) + key_offset;
                    const auto* str_header = reinterpret_cast<const StringHeader*>(key);

                    return {reinterpret_cast<const char*>(str_header) + sizeof(*str_header),
                            str_header->len};
                };

                m_hash_fn = [](ConstBuffer buf) {
                    return std::hash<std::string_view>{}({buf.data, buf.len});
                };
            },
            [&](auto&& v) {
                using T = typename ImplType<std::decay_t<decltype(v)>>::Type;

                m_key_buffer_fn = [](const void* data, std::size_t key_offset) -> ConstBuffer {
                    const auto* key = reinterpret_cast<const char*>(data) + key_offset;

                    return {reinterpret_cast<const char*>(key), sizeof(T)};
                };

                m_hash_fn = [](ConstBuffer buf) {
                    assert(buf.len == sizeof(T));

                    return std::hash<T>{}(*reinterpret_cast<const T*>(buf.data));
                };
            }},
        m_schema.fields[m_schema.key_field_index].type);
}

void* Collection::put(const void* data) {
    if (m_storage.count() + 1 >= static_cast<std::size_t>(m_buckets.size() / 1.4)) {
        auto new_bucket_count = m_buckets.size() * 2;

        if (new_bucket_count == 0) {
            new_bucket_count = 32;
        }

        {
            std::vector<KeyValue> new_buckets(new_bucket_count);

            for (std::size_t i = 0; i < m_storage.count(); ++i) {
                auto* elem = m_storage[i];
                auto elem_key = m_key_buffer_fn(elem, m_key_offset);

                auto elem_key_h = m_hash_fn(elem_key);

                // TODO We can optimize the put_internal here by having it skip checking for
                // duplicates, since there will never be duplicates
                if (auto* res = put_internal(new_buckets, elem_key, elem_key_h)) {
                    assert(res->value_index == m_storage.count());
                    res->value_index = i;
                } else {
                    // Failed to rehash, insert failed
                    return nullptr;
                }
            }

            m_buckets = std::move(new_buckets);
        }
    }

    auto data_key = m_key_buffer_fn(data, m_key_offset);
    auto data_key_h = m_hash_fn(data_key);

    if (auto* res = put_internal(m_buckets, data_key, data_key_h)) {
        if (res->value_index == m_storage.count()) {
            return m_storage.put(data);
        }

        auto* dest = m_storage[res->value_index];

        std::memcpy(m_storage[res->value_index], data, m_storage.doc_size());

        return dest;
    }

    return nullptr;
}

void Collection::remove(ConstBuffer key) {
    auto h = m_hash_fn(key);

    auto* found = find_internal(key, h);

    if (!found) {
        return;
    }

    if (found->value_index != m_storage.count() - 1) {
        // If this isn't the last element, then adjust the index of the last element in the internal
        // index

        auto* last_elem_data = m_storage[m_storage.count() - 1];
        auto last_elem_data_key = m_key_buffer_fn(last_elem_data, m_key_offset);

        auto last_elem_data_key_hash = m_hash_fn(last_elem_data_key);

        auto* last_elem_found = find_internal(last_elem_data_key, last_elem_data_key_hash);

        assert(last_elem_found);

        last_elem_found->value_index = found->value_index;
    }

    m_storage.remove(m_storage[found->value_index]);

    found->key_hash = TOMBSTONE_KEY_HASH;
}

// If the key type is a string, we convert the ConstBuffer to a string_view
// and perform the lookup using that.
void* Collection::find(ConstBuffer key) {
    auto h = m_hash_fn(key);

    auto* found = find_internal(key, h);

    if (!found) {
        return nullptr;
    }

    return m_storage[found->value_index];
}

const Schema& Collection::schema() const { return m_schema; }

std::size_t Collection::count() const { return m_storage.count(); }

Collection::KeyValue* Collection::put_internal(std::vector<KeyValue>& dest, ConstBuffer key,
                                               std::size_t key_hash) {
    auto idx = key_hash & (dest.size() - 1);
    auto orig_idx = idx;

    for (;;) {
        auto& bucket = dest[idx];

        if (bucket.key_hash == 0 || bucket.key_hash == TOMBSTONE_KEY_HASH) {
            bucket.key_hash = key_hash;
            bucket.value_index = m_storage.count();

            return &bucket;
        }

        // If it matches a previously existing element, just return the existing bucket
        if (bucket.key_hash == key_hash) {
            auto* data = m_storage[bucket.value_index];
            auto data_key = m_key_buffer_fn(data, m_key_offset);

            if (data_key.len == key.len && std::memcmp(data_key.data, key.data, key.len) == 0) {
                return &bucket;
            }
        }

        idx += 1;
        idx &= (dest.size() - 1);

        // We wrapped around, insert failed
        if (idx == orig_idx) {
            return nullptr;
        }
    }
}

Collection::KeyValue* Collection::find_internal(ConstBuffer key, std::size_t key_hash) {
    auto idx = key_hash & (m_buckets.size() - 1);
    auto orig_idx = idx;

    for (;;) {
        auto& bucket = m_buckets[idx];

        if (m_buckets[idx].key_hash == 0) {
            return nullptr;
        }

        if (m_buckets[idx].key_hash == key_hash) {
            auto* data = m_storage[bucket.value_index];
            auto data_key = m_key_buffer_fn(data, m_key_offset);

            if (key.len == data_key.len && std::memcmp(data_key.data, key.data, key.len) == 0) {
                return &bucket;
            }
        }

        idx += 1;
        idx &= (m_buckets.size() - 1);

        if (idx == orig_idx) {
            return nullptr;
        }
    }
}

}  // namespace boutique

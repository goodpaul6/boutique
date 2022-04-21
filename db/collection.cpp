#include "collection.hpp"

#include <algorithm>

#include "core/logger.hpp"
#include "core/overloaded_visitor.hpp"
#include "core/time_tracker.hpp"

namespace {

std::size_t hash(const boutique::FieldType& type, boutique::ConstBuffer buf) {
    using namespace boutique;

    return std::visit(
        OverloadedVisitor{[&](StringType) {
                              return std::hash<std::string_view>{}({buf.data, buf.len});
                          },
                          [&](auto&& v) {
                              using T = typename ImplType<std::decay_t<decltype(v)>>::Type;

                              assert(buf.len == sizeof(T));

                              return std::hash<T>{}(*reinterpret_cast<const T*>(buf.data));
                          }},
        type);
}

boutique::ConstBuffer field_as_const_buffer(const boutique::FieldType& type,
                                            const void* field_ptr) {
    using namespace boutique;

    return std::visit(OverloadedVisitor{
                          [&](auto&& v) -> ConstBuffer {
                              using T = typename ImplType<std::decay_t<decltype(v)>>::Type;

                              return {reinterpret_cast<const char*>(field_ptr), sizeof(T)};
                          },
                          [&](StringType) -> ConstBuffer {
                              const auto* str_header = static_cast<const StringHeader*>(field_ptr);

                              return {
                                  reinterpret_cast<const char*>(str_header) + sizeof(*str_header),
                                  str_header->len};
                          },
                      },
                      type);
}

boutique::ConstBuffer key_as_const_buffer(const boutique::Schema& schema, const void* data) {
    using namespace boutique;

    const auto& field_type = schema.fields[schema.key_field_index].type;
    const auto* key = reinterpret_cast<const char*>(data) + offset(schema, schema.key_field_index);

    return field_as_const_buffer(field_type, key);
}

bool is_empty(boutique::ConstBuffer buf) {
    // TODO Could be optimized with memcmp with a precached buffer containing zeros and memcmp
    return std::all_of(buf.data, buf.data + buf.len, [](auto ch) { return ch == 0; });
}

bool is_tombstone(boutique::ConstBuffer buf) {
    // TODO Could be optimized with memcmp with a precached buffer containing Fs and memcmp
    return std::all_of(buf.data, buf.data + buf.len, [](auto ch) { return ch == 0xFF; });
}

}  // namespace

namespace boutique {

Collection::Collection(Schema schema) : m_schema{std::move(schema)}, m_doc_size{size(m_schema)} {}

void* Collection::put(const void* data) {
    // TODO Maybe we don't need to maintain 50% load factor.
    // Could offload to secondary indices if we have a high-capacity collection.
    // If we do continue with the load factor here, we can do some virtual memory
    // tricks to ensure that we don't waste RAM unnecessarily. In particular,
    // we can avoid writing to pages we don't use, hence the tombstone value being 0.
    if (m_count + 1 >= static_cast<std::size_t>(m_bucket_count / 1.4)) {
        auto new_bucket_count = m_bucket_count;

        if (new_bucket_count == 0) {
            new_bucket_count = 32;
        } else {
            new_bucket_count *= 2;
        }

        {
            BOUTIQUE_DEBUG_TIME_TRACKER("Rehash");

            std::vector<char> new_data(new_bucket_count * m_doc_size);

            for (std::size_t i = 0; i < m_bucket_count; ++i) {
                auto* elem = m_data.data() + i * m_doc_size;
                auto elem_key = key_as_const_buffer(m_schema, elem);

                if (is_empty(elem_key) || is_tombstone(elem_key)) {
                    continue;
                }

                if (!put_internal(elem, new_data, new_bucket_count)) {
                    return nullptr;
                }
            }

            m_data = std::move(new_data);
            m_bucket_count = new_bucket_count;
        }
    }

    if (void* res = put_internal(data, m_data, m_bucket_count)) {
        m_count += 1;
        return res;
    }

    return nullptr;
}

char* Collection::put_internal(const void* elem_data, std::vector<char>& dest,
                               std::size_t bucket_count) {
    auto key_buf = key_as_const_buffer(m_schema, elem_data);
    auto key_h = hash(m_schema.fields[m_schema.key_field_index].type, key_buf);

    auto idx = key_h % bucket_count;
    auto orig_idx = idx;

    for (;;) {
        auto* data = dest.data() + idx * m_doc_size;
        auto data_key = key_as_const_buffer(m_schema, data);

        if (is_empty(data_key) || is_tombstone(data_key)) {
            std::memcpy(data, elem_data, m_doc_size);
            return data;
        }

        idx += 1;
        idx %= bucket_count;

        // We wrapped around, insert failed
        if (idx == orig_idx) {
            return nullptr;
        }
    }
}

void Collection::remove(ConstBuffer key) {
    auto h = hash(m_schema.fields[m_schema.key_field_index].type, key);

    auto* found = find_internal(key, h);

    if (!found) {
        return;
    }

    auto* found_key = found + offset(m_schema, m_schema.key_field_index);

    // Memsetting to zero marks this spot as available.
    std::memset(found_key, 0xFF, key.len);
    m_count -= 1;
}

// If the key type is a string, we convert the ConstBuffer to a string_view
// and perform the lookup using that.
void* Collection::find(ConstBuffer key) {
    auto h = hash(m_schema.fields[m_schema.key_field_index].type, key);

    return find_internal(key, h);
}

const Schema& Collection::schema() const { return m_schema; }

std::size_t Collection::count() const { return m_count; }

char* Collection::find_internal(ConstBuffer key, std::size_t key_hash) {
    auto idx = key_hash % m_bucket_count;
    auto orig_idx = idx;

    int probe_count = 0;

    for (;;) {
        auto* data = m_data.data() + idx * m_doc_size;
        auto data_key = key_as_const_buffer(m_schema, data);

        if (is_empty(data_key)) {
            return nullptr;
        }

        if (key.len == data_key.len && std::memcmp(data_key.data, key.data, key.len) == 0) {
            return data;
        }

        probe_count += 1;

        idx += 1;
        idx %= m_bucket_count;

        if (idx == orig_idx) {
            return nullptr;
        }
    }
}

}  // namespace boutique

#include "collection.hpp"

#include "core/overloaded_visitor.hpp"

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

}  // namespace

namespace boutique {

Collection::Collection(Schema schema) : m_schema{std::move(schema)}, m_storage{size(m_schema)} {}

void* Collection::put(const void* elem_data) {
    auto key_buf = key_as_const_buffer(m_schema, elem_data);
    auto h = hash(m_schema.fields[m_schema.key_field_index].type, key_buf);

    if (auto [found, it] = find_internal(key_buf, h); found) {
        std::memcpy(found, elem_data, m_storage.doc_size());
        return found;
    }

    auto index = m_storage.count();
    auto* elem = m_storage.put(elem_data);

    m_key_hash_to_index.emplace(h, index);

    return elem;
}

void Collection::remove(ConstBuffer key) {
    auto h = hash(m_schema.fields[m_schema.key_field_index].type, key);

    auto [found, found_it] = find_internal(key, h);

    if (!found) {
        return;
    }

    if (found != m_storage[m_storage.count() - 1]) {
        // When we remove an element from the storage, it will swap it with the last element,
        // assuming the found element isn't the last element. As such, we need to update the index
        // for the last element.

        auto* last_elem = m_storage[m_storage.count() - 1];

        auto last_key_buf = key_as_const_buffer(m_schema, last_elem);
        auto last_key_h = hash(m_schema.fields[m_schema.key_field_index].type, last_key_buf);

        [[maybe_unused]] auto [last_found, last_found_it] = find_internal(last_key_buf, last_key_h);

        assert(last_found);

        // Update the last element's index to be the found element's index
        last_found_it->second =
            (reinterpret_cast<const char*>(found) - reinterpret_cast<const char*>(m_storage[0])) /
            m_storage.doc_size();
    }

    m_storage.remove(found);
    m_key_hash_to_index.erase(found_it);
}

std::pair<void*, typename Collection::Index::iterator> Collection::find_internal(
    ConstBuffer key, std::size_t key_hash) {
    auto [first, last] = m_key_hash_to_index.equal_range(key_hash);

    for (auto it = first; it != last; ++it) {
        auto* elem_data = m_storage[it->second];
        auto* elem_key =
            reinterpret_cast<const char*>(elem_data) + offset(m_schema, m_schema.key_field_index);

        auto elem_key_buf =
            field_as_const_buffer(m_schema.fields[m_schema.key_field_index].type, elem_key);

        if (elem_key_buf.len == key.len && std::memcmp(elem_key_buf.data, key.data, key.len) == 0) {
            return {elem_data, it};
        }
    }

    return {nullptr, m_key_hash_to_index.end()};
}

// If the key type is a string, we convert the ConstBuffer to a string_view
// and perform the lookup using that.
void* Collection::find(ConstBuffer key) {
    auto h = hash(m_schema.fields[m_schema.key_field_index].type, key);

    [[maybe_unused]] auto [found, found_it] = find_internal(key, h);

    return found;
}

std::size_t Collection::count() const { return m_storage.count(); }

}  // namespace boutique

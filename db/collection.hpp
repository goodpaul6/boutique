#pragma once

#include <cassert>
#include <cstddef>
#include <cstring>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "schema.hpp"
#include "storage.hpp"

namespace boutique {

struct Collection {
    Collection(Schema schema);

    void* put(const void* elem_data);

    template <typename T>
    void remove(const T& key) {
        auto [elem, it] = find_internal(key);

        if (elem) {
            auto last_elem_index = m_storage.count() - 1;

            m_storage.remove(elem);
            m_key_hash_to_index.erase(it);
        }
    }

    template <typename T>
    void* find(const T& key) {
        auto [elem, _] = find_internal(key);
        return elem;
    }

    const Schema& schema() const;

private:
    // We copy the schema into the collection since we don't want it to be modified
    // without the collection's knowledge.
    Schema m_schema;

    Storage m_storage;
    std::unordered_multimap<std::size_t, std::uint64_t> m_key_hash_to_index;

    template <typename T>
    std::pair<void*, decltype(m_key_hash_to_index)::iterator> find_internal(const T& key) {
        static_assert(std::is_trivially_copyable_v<T>);

        auto hash_value = std::hash<T>{}(key);

        auto [first, last] = m_key_hash_to_index.equal_range(key);

        for (auto it = first; it != last; ++it) {
            auto* elem = m_storage[it->second];
            auto* elem_key =
                reinterpret_cast<const char*>(elem) + offset(m_schema, m_schema.key_field_index);

            if (std::holds_alternative<StringType>(
                    m_schema.fields[m_schema.key_field_index].type)) {
                if constexpr (std::is_same_v<T, std::string_view>) {
                    const auto* elem_key_header = reinterpret_cast<const StringHeader*>(elem_key);
                    const auto* elem_key_str = elem_key + sizeof(StringHeader);

                    if (key.size() == elem_key_header->len &&
                        std::memcmp(elem_key_str, key.data(), elem_key_header->len) == 0) {
                        return {elem, it};
                    }
                } else {
                    assert(false);
                }
            } else {
                assert(std::is_arithmetic_v<T>);
                assert(sizeof(T) == size(m_schema.fields[m_schema.key_field_index].type));

                // TODO Assert that the type of the value given is the same as the type of the
                // schema field.

                if (std::memcmp(reinterpret_cast<const char*>(&key), elem_key, sizeof(T)) == 0) {
                    return {elem, it};
                }
            }
        }

        return {nullptr, m_key_hash_to_index.end()};
    }

    std::size_t hash_elem(const void* elem_data);
};

}  // namespace boutique

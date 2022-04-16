#include "collection.hpp"

#include "core/overloaded_visitor.hpp"

namespace boutique {

Collection::Collection(Schema schema) : m_schema{std::move(schema)}, m_storage{size(m_schema)} {}

void* Collection::put(const void* elem_data) {
    auto index = m_storage.count();
    void* elem = m_storage.put(elem_data);

    auto h = hash_elem(elem_data);

    m_key_hash_to_index.emplace(h, index);

    return elem;
}

std::size_t Collection::hash_elem(const void* elem_data) {
    auto* elem_key =
        reinterpret_cast<const char*>(elem_data) + offset(m_schema, m_schema.key_field_index);

    // TODO We could cache the key hashing function into an std::function because it does not
    // make sense to do the work of dispatching every time we insert.
    return std::visit(
        OverloadedVisitor{
            [&](StringType) {
                const auto* elem_key_header = reinterpret_cast<const StringHeader*>(elem_key);
                const auto* elem_key_str = elem_key + sizeof(StringHeader);

                return std::hash<std::string_view>{}({elem_key_str, elem_key_header->len});
            },

            [&](BoolType) {
                return std::hash<bool>{}(*reinterpret_cast<const std::uint8_t*>(elem_key));
            },

            [&](UInt8Type) {
                return std::hash<std::uint8_t>{}(*reinterpret_cast<const std::uint8_t*>(elem_key));
            },
            [&](UInt16Type) {
                return std::hash<std::uint16_t>{}(
                    *reinterpret_cast<const std::uint16_t*>(elem_key));
            },
            [&](UInt32Type) {
                return std::hash<std::uint32_t>{}(
                    *reinterpret_cast<const std::uint32_t*>(elem_key));
            },
            [&](UInt64Type) {
                return std::hash<std::uint64_t>{}(
                    *reinterpret_cast<const std::uint64_t*>(elem_key));
            },

            [&](Int8Type) {
                return std::hash<std::int8_t>{}(*reinterpret_cast<const std::int8_t*>(elem_key));
            },
            [&](Int16Type) {
                return std::hash<std::int16_t>{}(*reinterpret_cast<const std::int16_t*>(elem_key));
            },
            [&](Int32Type) {
                return std::hash<std::int32_t>{}(*reinterpret_cast<const std::int32_t*>(elem_key));
            },
            [&](Int64Type) {
                return std::hash<std::int64_t>{}(*reinterpret_cast<const std::int64_t*>(elem_key));
            },

            [&](Float32Type) {
                return std::hash<float>{}(*reinterpret_cast<const float*>(elem_key));
            },
            [&](Float64Type) {
                return std::hash<double>{}(*reinterpret_cast<const double*>(elem_key));
            },
        },
        m_schema.fields[m_schema.key_field_index].type);
}

}  // namespace boutique

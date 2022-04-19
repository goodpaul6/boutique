#include "schema.hpp"

#include <algorithm>
#include <cassert>

#include "core/overloaded_visitor.hpp"

namespace boutique {

std::size_t alignment(const FieldType& type) {
    return std::visit(
        OverloadedVisitor{
            [](auto&& v) { return alignof(typename ImplType<std::decay_t<decltype(v)>>::Type); },
            [](StringType) { return alignof(StringHeader); },
        },
        type);
}

std::size_t alignment(const Schema& schema) {
    assert(!schema.fields.empty());

    auto found = std::max_element(
        schema.fields.begin(), schema.fields.end(),
        [](const Field& a, const Field& b) { return alignment(a.type) < alignment(b.type); });

    return alignment(found->type);
}

std::size_t size(const FieldType& type) {
    return std::visit(
        OverloadedVisitor{
            [](auto&& v) { return sizeof(typename ImplType<std::decay_t<decltype(v)>>::Type); },
            [](StringType s) { return sizeof(StringHeader) + s.capacity; },
        },
        type);
}

std::size_t size(const Schema& schema) {
    return offset(schema, schema.fields.size() - 1) + size(schema.fields.back().type);
}

std::size_t offset(const Schema& schema, std::uint32_t field_index) {
    assert(!schema.fields.empty());
    assert(field_index < schema.fields.size());

    std::size_t offset = 0;

    for (std::uint32_t i = 0; i <= field_index; ++i) {
        const auto& field = schema.fields[i];

        // Pad such that the field is appropriately aligned
        const auto a = alignment(field.type);

        // Align to the field type's requirement
        offset += (offset % a);
        offset &= ~(a - 1);

        if (i == field_index) {
            break;
        }

        offset += size(field.type);
    }

    return offset;
}

}  // namespace boutique

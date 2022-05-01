#include "schema.hpp"

#include <algorithm>
#include <cassert>
#include <numeric>

#include "core/overloaded_visitor.hpp"

namespace boutique {

std::size_t alignment(const FieldType& type) {
    return std::visit(
        OverloadedVisitor{
            [](auto&& v) { return alignof(typename ImplType<std::decay_t<decltype(v)>>::Type); },
            [](StringType) { return alignof(StringHeader); },
            [](const AggregateType& a) { return alignment(a); }},
        type);
}

std::size_t alignment(const AggregateType& agg) {
    assert(!agg.empty());

    std::size_t max_align = 0;

    for (const auto& field : agg) {
        max_align = std::max(max_align, alignment(field.type));
    }

    return max_align;
}

std::size_t alignment(const Schema& schema) { return alignment(schema.fields); }

std::size_t size(const FieldType& type) {
    return std::visit(
        OverloadedVisitor{
            [](auto&& v) { return sizeof(typename ImplType<std::decay_t<decltype(v)>>::Type); },
            [](StringType s) { return sizeof(StringHeader) + s.capacity; },
            [](const AggregateType& a) { return size(a); }},
        type);
}

std::size_t size(const AggregateType& agg) {
    return offset(agg, agg.size() - 1) + size(agg.back().type);
}

std::size_t size(const Schema& schema) { return size(schema.fields); }

std::size_t offset(const AggregateType& agg, std::uint32_t field_index) {
    assert(!agg.empty());
    assert(field_index < agg.size());

    std::size_t offset = 0;

    for (std::uint32_t i = 0; i <= field_index; ++i) {
        const auto& field = agg[i];

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

std::size_t offset(const Schema& schema, std::uint32_t field_index) {
    return offset(schema.fields, field_index);
}

}  // namespace boutique

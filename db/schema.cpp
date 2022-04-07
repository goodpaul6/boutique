#include "schema.hpp"

#include <algorithm>
#include <cassert>

#include "core/overloaded_visitor.hpp"

namespace boutique {

std::size_t alignment(const FieldType& type) {
    return std::visit(OverloadedVisitor{
                          [](BoolType) { return alignof(bool); },

                          [](UInt8Type) { return alignof(std::uint8_t); },
                          [](UInt16Type) { return alignof(std::uint16_t); },
                          [](UInt32Type) { return alignof(std::uint32_t); },
                          [](UInt64Type) { return alignof(std::uint64_t); },

                          [](Int8Type) { return alignof(std::int8_t); },
                          [](Int16Type) { return alignof(std::int16_t); },
                          [](Int32Type) { return alignof(std::int32_t); },
                          [](Int64Type) { return alignof(std::int64_t); },

                          [](Float32Type) { return alignof(float); },
                          [](Float64Type) { return alignof(double); },

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
    return std::visit(OverloadedVisitor{
                          [](BoolType) { return sizeof(bool); },

                          [](UInt8Type) { return sizeof(std::uint8_t); },
                          [](UInt16Type) { return sizeof(std::uint16_t); },
                          [](UInt32Type) { return sizeof(std::uint32_t); },
                          [](UInt64Type) { return sizeof(std::uint64_t); },

                          [](Int8Type) { return sizeof(std::int8_t); },
                          [](Int16Type) { return sizeof(std::int16_t); },
                          [](Int32Type) { return sizeof(std::int32_t); },
                          [](Int64Type) { return sizeof(std::int64_t); },

                          [](Float32Type) { return sizeof(float); },
                          [](Float64Type) { return sizeof(double); },

                          [](StringType s) { return sizeof(StringHeader) + s.capacity; },
                      },
                      type);
}

std::size_t size(const Schema& schema) {
    std::size_t total_size = 0;

    for (const auto& field : schema.fields) {
        // Pad such that the field is appropriately aligned
        const auto a = alignment(field.type);

        // Align to the field type's requirement
        total_size += (total_size % a);
        total_size &= ~(a - 1);

        total_size += size(field.type);
    }

    return total_size;
}

}  // namespace boutique

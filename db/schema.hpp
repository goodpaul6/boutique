#pragma once

#include <cstdint>
#include <string>
#include <variant>
#include <vector>

namespace boutique {

struct UInt8Type {};
struct UInt16Type {};
struct UInt32Type {};
struct UInt64Type {};

struct Int8Type {};
struct Int16Type {};
struct Int32Type {};
struct Int64Type {};

struct Float32Type {};
struct Float64Type {};

struct StringType {
    size_t capacity = 0;
};

struct StringHeader {
    std::uint32_t len = 0;
};

using FieldType = std::variant<UInt8Type, UInt16Type, UInt32Type, UInt64Type, Int8Type, Int16Type,
                               Int32Type, Int64Type, Float32Type, Float64Type, StringType>;

struct Field {
    std::string name;
    FieldType type;
};

struct Schema {
    std::vector<Field> fields;
};

std::size_t alignment(const FieldType& type);
std::size_t alignment(const Schema& schema);

std::size_t size(const FieldType& type);
std::size_t size(const Schema& schema);

}  // namespace boutique

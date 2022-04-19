#pragma once

#include <cstdint>
#include <string>
#include <variant>
#include <vector>

#include "core/const_buffer.hpp"

namespace boutique {

struct BoolType {};

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

using FieldType =
    std::variant<BoolType, UInt8Type, UInt16Type, UInt32Type, UInt64Type, Int8Type, Int16Type,
                 Int32Type, Int64Type, Float32Type, Float64Type, StringType>;

template <typename T>
struct ImplType;

template <>
struct ImplType<BoolType> {
    using Type = bool;
};

template <>
struct ImplType<UInt8Type> {
    using Type = std::uint8_t;
};

template <>
struct ImplType<UInt16Type> {
    using Type = std::uint16_t;
};

template <>
struct ImplType<UInt32Type> {
    using Type = std::uint32_t;
};

template <>
struct ImplType<UInt64Type> {
    using Type = std::uint64_t;
};

template <>
struct ImplType<Int8Type> {
    using Type = std::int8_t;
};

template <>
struct ImplType<Int16Type> {
    using Type = std::int16_t;
};

template <>
struct ImplType<Int32Type> {
    using Type = std::int32_t;
};

template <>
struct ImplType<Int64Type> {
    using Type = std::int64_t;
};

template <>
struct ImplType<Float32Type> {
    using Type = float;
};

template <>
struct ImplType<Float64Type> {
    using Type = double;
};

struct Field {
    std::string name;
    FieldType type;
};

struct Schema {
    std::vector<Field> fields;
    std::uint32_t key_field_index = 0;
};

std::size_t alignment(const FieldType& type);
std::size_t alignment(const Schema& schema);

std::size_t size(const FieldType& type);
std::size_t size(const Schema& schema);

std::size_t offset(const Schema& schema, std::uint32_t field_index);

}  // namespace boutique

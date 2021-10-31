#pragma once

#include <tuple>
#include <type_traits>

namespace boutique {

template <size_t I, typename S, typename... Types>
constexpr size_t type_index_helper() {
    using Tuple = std::tuple<Types...>;

    static_assert(I <= sizeof...(Types));

    if constexpr (std::is_same_v<S, std::tuple_element_t<I, Tuple>>) {
        return I;
    } else {
        return type_index_helper<I + 1, S, Types...>();
    }
}

template <typename S, typename T>
struct TypeIndexHelper {};

template <typename S, typename... Types>
struct TypeIndexHelper<S, std::variant<Types...>> {
    static constexpr size_t value = type_index_helper<0, S, Types...>();
};

template <typename S, typename T>
inline constexpr size_t type_index_v = TypeIndexHelper<S, T>::value;

}  // namespace boutique

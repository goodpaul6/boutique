#pragma once

namespace boutique {

template <typename... Ts>
struct OverloadedVisitor : Ts... {
    using Ts::operator()...;
};

template <typename... Ts>
OverloadedVisitor(Ts...) -> OverloadedVisitor<Ts...>;

}  // namespace boutique

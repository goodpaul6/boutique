#pragma once

#include <functional>

namespace boutique {

template <typename Fn, typename... Args>
auto bind_front(Fn&& fn, Args&&... args) {
    return [fn = std::forward<Fn>(fn), args...](auto&&... inner_args) {
        return std::invoke(fn, args..., std::forward<decltype(inner_args)>(inner_args)...);
    };
}

}  // namespace boutique

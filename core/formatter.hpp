#pragma once

#include <sstream>
#include <string>
#include <string_view>

namespace boutique {

inline void format(std::ostream& s, std::string_view fmt) { s << fmt; }

template <typename First, typename... Rest>
void format(std::ostream& s, std::string_view fmt, First&& first, Rest&&... rest) {
    auto place_pos = fmt.find("{}");

    if (place_pos == std::string_view::npos) {
        s << fmt;
        return;
    }

    s << fmt.substr(0, place_pos);
    s << first;

    format(s, fmt.substr(place_pos + 2), std::forward<Rest>(rest)...);
}

template <typename... Args>
std::string format(std::string_view fmt, Args&&... args) {
    std::ostringstream oss;
    format(oss, fmt, std::forward<Args>(args)...);

    return oss.str();
}

}  // namespace boutique

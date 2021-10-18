#include "dict.hpp"

namespace boutique {

std::optional<std::string_view> Dict::get(const std::string& key) const {
    auto found = m_map.find(key);

    if (found == m_map.end()) {
        return std::nullopt;
    }

    return found->second;
}

void Dict::set(std::string key, std::string value) {
    m_map.insert_or_assign(std::move(key), std::move(value));
}

}  // namespace boutique

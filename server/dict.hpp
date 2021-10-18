#pragma once

#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>

namespace boutique {

struct Dict {
    std::optional<std::string_view> get(const std::string& key) const;
    void set(std::string key, std::string value);

private:
    std::unordered_map<std::string, std::string> m_map;
};

}  // namespace boutique

#include "database.hpp"

#include <cassert>

namespace boutique {

const Schema& Database::register_schema(std::string name, Schema schema) {
    const auto [iter, _] = m_schemas.insert_or_assign(std::move(name), std::move(schema));
    return iter->second;
}

Collection& Database::create_collection(std::string name, Schema schema) {
    const auto [iter, inserted_new] = m_colls.insert_or_assign(std::move(name), std::move(schema));
    return iter->second;
}

const Schema* Database::schema(const std::string& name) {
    auto found = m_schemas.find(name);
    if (found == m_schemas.end()) {
        return nullptr;
    }

    return &found->second;
}

Collection* Database::collection(const std::string& name) {
    auto found = m_colls.find(name);
    if (found == m_colls.end()) {
        return nullptr;
    }

    return &found->second;
}

}  // namespace boutique

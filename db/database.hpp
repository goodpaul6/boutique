#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>

#include "collection.hpp"
#include "schema.hpp"

namespace boutique {

struct Database {
    const Schema& register_schema(std::string name, Schema schema);
    Collection& create_collection(std::string name, Schema schema);

    const Schema* schema(const std::string& name);
    Collection* collection(const std::string& name);

    // TODO Add higher-level functions that will find a document given a query,
    // maintain indexes, modify schemas, etc

private:
    std::unordered_map<std::string, Schema> m_schemas;
    std::unordered_map<std::string, Collection> m_colls;
};

}  // namespace boutique

#include <cassert>
#include <cstring>
#include <iostream>

#include "collection.hpp"
#include "database.hpp"
#include "schema.hpp"
#include "storage.hpp"

int main(int argc, char** argv) {
    using namespace boutique;

    Schema coord_schema{{{"lat", Float64Type{}}, {"lng", Float64Type{}}}};

    assert(size(coord_schema) == size(Float64Type{}) * 2);

    Schema user_schema{{{"id", UInt64Type{}}, {"name", StringType{3}}, {"balance", Int64Type{}}}};

    user_schema.key_field_index = 1;

    assert(size(user_schema) == size(Int64Type{}) * 3);

    Storage coord_coll{size(coord_schema)};

    struct Coord {
        double lat = 0;
        double lng = 0;
    };

    Coord c{10, 20};

    coord_coll.put(&c);

    assert(coord_coll.count() == 1);

    Coord r;

    std::memcpy(&r, coord_coll[0], size(coord_schema));

    assert(r.lat == c.lat && r.lng == c.lng);

    c.lat = 20;

    coord_coll.put(&c);

    assert(coord_coll.count() == 2);

    coord_coll.remove(coord_coll[0]);

    assert(coord_coll.count() == 1);

    // Read r again, expect it to match
    std::memcpy(&r, coord_coll[0], size(coord_schema));

    assert(r.lat == c.lat && r.lng == c.lng);

    coord_coll.remove(coord_coll[0]);

    assert(coord_coll.count() == 0);

    for (int i = 0; i < 1'000'000; ++i) {
        c.lat = static_cast<double>(i);
        coord_coll.put(&c);
    }

    assert(coord_coll.count() == 1'000'000);

    Collection user_coll{user_schema};

    struct User {
        std::uint64_t id;
        std::uint32_t name_len;
        char name[3];
        std::int64_t balance;
    };

    std::uint64_t id = 1;

    User user;

    for (int i = 1; i <= 100; ++i) {
        auto s = std::to_string(i);

        user.id = i;

        user.name_len = s.size();
        std::memcpy(user.name, s.c_str(), s.size());

        user.balance = i;

        user_coll.put(&user);
    }

    auto* found = user_coll.find(ConstBuffer{"1"});

    assert(found);

    for (int i = 1; i <= 50; ++i) {
        auto s = std::to_string(i);
        user_coll.remove(ConstBuffer{s.data(), s.size()});
    }

    assert(user_coll.count() == 50);

    found = user_coll.find(ConstBuffer{"2"});

    assert(!found);

    found = user_coll.find(ConstBuffer{"73"});

    assert(found);

    User read_user;
    std::memcpy(&read_user, found, sizeof(User));

    assert(read_user.id == 73);
    assert(read_user.balance == 73);

    for (int i = 1; i <= 50; ++i) {
        auto s = std::to_string(i);

        user.id = i;

        user.name_len = s.size();
        std::memcpy(user.name, s.c_str(), s.size());

        user.balance = i;

        user_coll.put(&user);
    }

    found = user_coll.find(ConstBuffer{"20"});

    assert(found);

    Database db;

    const auto& db_user_schema = db.register_schema("User", user_schema);
    auto& db_user_coll = db.create_collection("users", db_user_schema);

    assert(db.schema("User") == &db_user_schema);
    assert(db.collection("users") == &db_user_coll);

    return 0;
}

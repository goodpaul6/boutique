#include <cassert>
#include <cstring>
#include <iostream>

#include "collection.hpp"
#include "schema.hpp"
#include "storage.hpp"

int main(int argc, char** argv) {
    using namespace boutique;

    Schema coord_schema{{{"lat", Float64Type{}}, {"lng", Float64Type{}}}};

    assert(size(coord_schema) == size(Float64Type{}) * 2);

    Schema user_schema{{{"id", UInt64Type{}}, {"name", StringType{3}}, {"balance", Int64Type{}}}};

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

    User user{id, 3, {'b', 'o', 'b'}, 1000};

    user_coll.put(&user);

    auto* found = user_coll.find(id);

    assert(found);

    User read_user;
    std::memcpy(&read_user, found, sizeof(User));

    assert(read_user.id == user.id);
    assert(read_user.name_len == user.name_len);
    assert(std::memcmp(read_user.name, user.name, sizeof(read_user.name)) == 0);
    assert(read_user.balance == user.balance);

    return 0;
}

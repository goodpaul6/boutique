#include <cassert>
#include <cstring>
#include <iostream>

#include "collection.hpp"
#include "schema.hpp"

int main(int argc, char** argv) {
    using namespace boutique;

    Schema coord_schema{{{"lat", Float64Type{}}, {"lng", Float64Type{}}}};

    assert(size(coord_schema) == size(Float64Type{}) * 2);

    Schema user_schema{{{"name", StringType{3}}, {"balance", Int64Type{}}}};

    assert(size(user_schema) == size(Int64Type{}) * 2);

    Collection coord_coll{coord_schema};

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

    for (int i = 0; i < 1000; ++i) {
        c.lat = static_cast<double>(i);
        coord_coll.put(&c);
    }

    assert(coord_coll.count() == 1000);

    return 0;
}

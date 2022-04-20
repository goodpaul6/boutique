#include <chrono>
#include <iostream>
#include <unordered_map>

#include "database.hpp"
#include "storage.hpp"

namespace {

const int OP_COUNT = 1'000'000;

}

int main(int argc, char** argv) {
    using namespace boutique;

    struct User {
        std::uint32_t name_len;
        std::array<char, 32> name;
        std::uint64_t id;
        std::int64_t balance;
    };

    Schema user_schema;

    user_schema.fields = {{"name", StringType{sizeof(User::name)}}, {"balance", Int64Type{}}};
    user_schema.key_field_index = 1;

    User user;

    user.name_len = 3;
    user.name = {'b', 'o', 'b'};
    user.id = 0;
    user.balance = 10;

    Database db;

    // Storage
    Storage storage{size(user_schema)};

    std::cout << "Insert " << OP_COUNT << " documents into bt storage.\n";

    auto prev_time = std::chrono::high_resolution_clock::now();

    for (int i = 1; i <= OP_COUNT; ++i) {
        user.id = i;
        storage.put(&user);
    }

    auto new_time = std::chrono::high_resolution_clock::now();

    std::cout << "Took "
              << std::chrono::duration_cast<std::chrono::milliseconds>(new_time - prev_time).count()
              << "ms.\n";

    // Unordered map
    std::unordered_map<std::uint64_t, User> map;

    std::cout << "Insert " << OP_COUNT << " documents into unordered_map.\n";

    prev_time = std::chrono::high_resolution_clock::now();

    for (int i = 1; i <= OP_COUNT; ++i) {
        user.id = i;
        map.insert_or_assign(user.id, user);
    }

    new_time = std::chrono::high_resolution_clock::now();

    std::cout << "Took "
              << std::chrono::duration_cast<std::chrono::milliseconds>(new_time - prev_time).count()
              << "ms.\n";

    std::cout << "Find " << OP_COUNT << " documents from unordered_map.\n";

    prev_time = std::chrono::high_resolution_clock::now();

    for (int i = OP_COUNT; i >= 1; --i) {
        volatile auto found = map.find(i);
    }

    new_time = std::chrono::high_resolution_clock::now();

    std::cout << "Took "
              << std::chrono::duration_cast<std::chrono::milliseconds>(new_time - prev_time).count()
              << "ms.\n";

    // Database
    auto& coll = db.create_collection("users", user_schema);

    std::cout << "Insert " << OP_COUNT << " documents into bt collection.\n";

    prev_time = std::chrono::high_resolution_clock::now();

    for (int i = 1; i <= OP_COUNT; ++i) {
        user.id = i;
        coll.put(&user);
    }

    new_time = std::chrono::high_resolution_clock::now();

    std::cout << "Took "
              << std::chrono::duration_cast<std::chrono::milliseconds>(new_time - prev_time).count()
              << "ms.\n";

    std::cout << "Find " << OP_COUNT << " documents from bt collection.\n";

    prev_time = std::chrono::high_resolution_clock::now();

    for (int i = OP_COUNT; i >= 1; --i) {
        std::uint64_t id = i;
        volatile auto* found = static_cast<User*>(
            coll.find(ConstBuffer{reinterpret_cast<const char*>(&id), sizeof(id)}));

        if (!found) {
            std::cerr << "Failed to find!\n";
            return 1;
        }

        if (reinterpret_cast<volatile User*>(found)->id != id) {
            std::cerr << "Bad ID!\n";
            return 1;
        }
    }

    new_time = std::chrono::high_resolution_clock::now();

    std::cout << "Took "
              << std::chrono::duration_cast<std::chrono::milliseconds>(new_time - prev_time).count()
              << "ms.\n";

    return 0;
}

#include <charconv>
#include <chrono>
#include <iostream>
#include <unordered_map>

#include "database.hpp"
#include "storage.hpp"

namespace {

// NOCOMMIT Just while running under valgrind
const int OP_COUNT = 1'000'000;

}  // namespace

int main(int argc, char** argv) {
    using namespace boutique;

    struct User {
        std::uint32_t name_len;
        std::array<char, 32> name;
        std::uint64_t id;
        std::int64_t balance;
    };

    Schema user_schema;

    user_schema.fields = {
        {"name", StringType{sizeof(User::name)}}, {"id", UInt64Type{}}, {"balance", Int64Type{}}};
    user_schema.key_field_index = 0;

    User user;

    user.name_len = 3;
    user.name = {'b', 'o', 'b'};
    user.balance = 10;

    // Unordered map
    {
        std::unordered_map<std::string, User> map;

        std::cout << "Insert " << OP_COUNT << " documents into unordered_map.\n";

        auto prev_time = std::chrono::high_resolution_clock::now();

        for (int i = 1; i <= OP_COUNT; ++i) {
            auto res = std::to_chars(user.name.begin(), user.name.end(), i);

            user.name_len = res.ptr - user.name.data();

            map.insert_or_assign({user.name.data(), user.name_len}, user);
        }

        auto new_time = std::chrono::high_resolution_clock::now();

        std::cout
            << "Took "
            << std::chrono::duration_cast<std::chrono::milliseconds>(new_time - prev_time).count()
            << "ms.\n";

        std::cout << "Find " << OP_COUNT << " documents from unordered_map.\n";

        prev_time = std::chrono::high_resolution_clock::now();

        for (int i = OP_COUNT; i >= 1; --i) {
            auto s = std::to_string(i);

            auto found = map.find(s);

            if (found == map.end()) {
                std::cerr << "Failed to find!\n";
                return 1;
            }
        }

        new_time = std::chrono::high_resolution_clock::now();

        std::cout
            << "Took "
            << std::chrono::duration_cast<std::chrono::milliseconds>(new_time - prev_time).count()
            << "ms.\n";
    }

    // Database
    {
        Database db;

        auto& coll = db.create_collection("users", user_schema);

        std::cout << "Insert " << OP_COUNT << " documents into bt collection.\n";

        auto prev_time = std::chrono::high_resolution_clock::now();

        for (int i = 1; i <= OP_COUNT; ++i) {
            auto res = std::to_chars(user.name.begin(), user.name.end(), i);
            user.name_len = res.ptr - user.name.data();

            coll.put(&user);
        }

        auto new_time = std::chrono::high_resolution_clock::now();

        std::cout
            << "Took "
            << std::chrono::duration_cast<std::chrono::milliseconds>(new_time - prev_time).count()
            << "ms.\n";

        std::cout << "Find " << OP_COUNT << " documents from bt collection.\n";

        prev_time = std::chrono::high_resolution_clock::now();

        for (int i = OP_COUNT; i >= 1; --i) {
            auto s = std::to_string(i);

            auto* found = static_cast<User*>(coll.find(ConstBuffer{s.data(), s.size()}));

            if (!found) {
                std::cerr << "Failed to find!\n";
                return 1;
            }

            if (reinterpret_cast<volatile User*>(found)->name_len != s.size()) {
                std::cerr << "Bad ID!\n";
                return 1;
            }
        }

        new_time = std::chrono::high_resolution_clock::now();

        std::cout
            << "Took "
            << std::chrono::duration_cast<std::chrono::milliseconds>(new_time - prev_time).count()
            << "ms.\n";
    }

    return 0;
}

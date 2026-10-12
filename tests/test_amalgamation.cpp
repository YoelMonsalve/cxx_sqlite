/**
 * TEST_AMALGAMATION.CPP
 * 
 * Verifies that cxx_sqlite.hpp can be consumed as a true 100% header-only library
 * with zero dependency on library .o/.cpp files, linking ONLY -lsqlite3.
 */

#include "cxx_sqlite.hpp"
#include <iostream>
#include <cassert>

int main()
{
    using namespace cxx_sqlite3;

    std::cout << "Testing cxx_sqlite.hpp single-header..." << std::endl;

    database db(":memory:");
    db.exec("CREATE TABLE items (id INTEGER PRIMARY KEY, name TEXT, price REAL);");
    db.exec("INSERT INTO items VALUES (1, 'Widget', 19.99);");
    db.exec("INSERT INTO items VALUES (2, 'Gadget', 29.50);");

    int count = 0;
    for (auto row : db.query("SELECT id, name, price FROM items ORDER BY id")) {
        count++;
        if (count == 1) {
            assert(row["id"].as_int() == 1);
            assert(row["name"].as_string() == "Widget");
            assert(row["price"].as_double() == 19.99);
        } else if (count == 2) {
            assert(row["id"].as_int() == 2);
            assert(row["name"].as_string() == "Gadget");
            assert(row["price"].as_double() == 29.50);
        }
    }
    assert(count == 2);

    // Test generic_value try_as and as_or
    generic_value val(100);
    assert(val.try_as<int>().has_value());
    assert(*val.try_as<int>() == 100);
    assert(!val.try_as<std::string_view>().has_value());
    assert(val.as_or<int>(999) == 100);

    // Test boolean conversions
    assert(static_cast<bool>(val));
    assert(!(!val));

    std::cout << "Single-header cxx_sqlite.hpp amalgamation test PASSED successfully!" << std::endl;
    return 0;
}

/**
 * TEST_SUITE.CPP
 * 
 * Comprehensive Unit Test Suite for cxx_sqlite3 library.
 * Designed with a lightweight Catch2-style test runner (no external dependencies).
 * 
 * Features tested:
 *  1. High-level Typing System (generic_object, SQLITE3_ONULL, conversions, BLOB)
 *  2. Positional and Alternating Named Parameter Binding
 *  3. Range-based for loop iteration (statement & db.query)
 *  4. Type-safe row extraction & column querying
 *  5. RAII transactions (commit, rollback, auto-rollback on exception)
 * 
 * @version 1.0.0
 * (C) Copyright Yoel Monsalve 2025-2026.
 */

#include <iostream>
#include <string>
#include <vector>
#include <functional>
#include <stdexcept>
#include <cstdint>
#include <type_traits>

#include "sqlite.hpp"

// Lightweight Catch2-style test framework
namespace test_framework {
    static int g_total_tests = 0;
    static int g_failed_tests = 0;
    static int g_current_assertions = 0;
    static std::string g_current_section = "";

    struct TestCase {
        std::string name;
        std::function<void()> func;
    };

    static std::vector<TestCase> &get_registry() {
        static std::vector<TestCase> registry;
        return registry;
    }

    struct Registrar {
        Registrar(const std::string &name, std::function<void()> func) {
            get_registry().push_back({name, func});
        }
    };
}

#define TEST_CONCAT_INNER(a, b) a##b
#define TEST_CONCAT(a, b) TEST_CONCAT_INNER(a, b)

#define TEST_CASE(name) \
    static void TEST_CONCAT(test_func_, __LINE__)(); \
    static test_framework::Registrar TEST_CONCAT(reg_, __LINE__)(name, TEST_CONCAT(test_func_, __LINE__)); \
    static void TEST_CONCAT(test_func_, __LINE__)()

#define REQUIRE(condition) \
    do { \
        test_framework::g_current_assertions++; \
        if (!(condition)) { \
            std::cerr << "  FAILED REQUIRE(" #condition ") at " << __FILE__ << ":" << __LINE__ << std::endl; \
            throw std::runtime_error("Assertion failed: " #condition); \
        } \
    } while (0)

#define CHECK(condition) \
    do { \
        test_framework::g_current_assertions++; \
        if (!(condition)) { \
            std::cerr << "  FAILED CHECK(" #condition ") at " << __FILE__ << ":" << __LINE__ << std::endl; \
            test_framework::g_failed_tests++; \
        } \
    } while (0)

#define SECTION(name) \
    test_framework::g_current_section = name;

using namespace cxx_sqlite3;

/* ========================================================================= */
/* Test Case 2: Variadic Parameter Binding                                   */
/* ========================================================================= */
TEST_CASE("Variadic Parameter Binding")
{
    database db(":memory:");
    db.exec("CREATE TABLE users (id INTEGER PRIMARY KEY, name TEXT, salary REAL, bio BLOB);");

    SECTION("Positional variadic binding")
    {
        statement stmt = db.prepare_statement("INSERT INTO users (id, name, salary) VALUES (?, ?, ?);");
        stmt.bind(1, 100, 2, "Alice", 3, 5500.50);
        stmt.step();
        REQUIRE(stmt.rc() == int(error::done));

        statement verify = db.prepare_statement("SELECT name, salary FROM users WHERE id = ?;");
        verify.bind(1, 100);
        row r = verify.fetch_row();
        REQUIRE(r["name"].as<std::string>() == "Alice");
        REQUIRE(r["salary"].as<double>() == 5500.50);
    }

    SECTION("Named alternating variadic binding")
    {
        statement stmt = db.prepare_statement("INSERT INTO users (id, name, salary) VALUES (:id, :name, :sal);");
        stmt.bind(":id", 200, ":name", "Bob", ":sal", 7200.0);
        stmt.step();
        REQUIRE(stmt.rc() == int(error::done));

        statement verify = db.prepare_statement("SELECT name, salary FROM users WHERE id = :id;");
        verify.bind(":id", 200);
        row r = verify.fetch_row();
        REQUIRE(r["name"].as<std::string>() == "Bob");
        REQUIRE(r["salary"].as<double>() == 7200.0);
    }

    SECTION("Direct db.execute with alternating parameters")
    {
        db.execute("INSERT INTO users (id, name, salary) VALUES (:id, :name, :sal);",
                   ":id", 300, ":name", "Charlie", ":sal", 8900.25);

        statement verify = db.prepare_statement("SELECT name, salary FROM users WHERE id = 300;");
        row r = verify.fetch_row();
        REQUIRE(r["name"].as<std::string>() == "Charlie");
        REQUIRE(r["salary"].as<double>() == 8900.25);
    }

    SECTION("BLOB parameter binding")
    {
        blob_t data = { 0x01, 0x02, 0x03, 0x04, 0x05 };
        db.execute("INSERT INTO users (id, name, salary, bio) VALUES (:id, :name, :sal, :bio);",
                   ":id", 400, ":name", "Dana", ":sal", 9500.0, ":bio", data);

        statement verify = db.prepare_statement("SELECT bio FROM users WHERE id = 400;");
        row r = verify.fetch_row();
        blob_t read_bio = r["bio"].as<blob_t>();
        REQUIRE(read_bio.size() == 5);
        REQUIRE(read_bio[0] == 0x01);
        REQUIRE(read_bio[4] == 0x05);
    }

    SECTION("std::optional parameter binding")
    {
        std::optional<std::string> null_bio = std::nullopt;
        std::optional<double> present_sal = 14500.0;
        db.execute("INSERT INTO users (id, name, salary, bio) VALUES (:id, :name, :sal, :bio);",
                   ":id", 600, ":name", "Frank", ":sal", present_sal, ":bio", null_bio);

        statement verify = db.prepare_statement("SELECT salary, bio FROM users WHERE id = 600;");
        row r = verify.fetch_row();
        REQUIRE(r["salary"].as<double>() == 14500.0);
        REQUIRE(r.is_null("bio"));
        auto opt_bio = r["bio"].as<std::optional<blob_t>>();
        REQUIRE(!opt_bio.has_value());
    }

    SECTION("Tabular output helper (command2)")
    {
        ErrorCode ec = db.command2("SELECT id, name, salary FROM users WHERE id = 500;");
        REQUIRE(ec == error::ok);
    }
}

/* ========================================================================= */
/* Test Case 3: Range-based for loop iteration (statement & db.query)        */
/* ========================================================================= */
TEST_CASE("Range-based for Loop Iteration")
{
    database db(":memory:");
    db.exec("CREATE TABLE items (id INTEGER, label TEXT);");
    db.exec("INSERT INTO items VALUES (1, 'One'), (2, 'Two'), (3, 'Three'), (4, 'Four');");

    SECTION("Range-for directly on prepared statement")
    {
        statement stmt = db.prepare_statement("SELECT id, label FROM items ORDER BY id ASC;");
        int count = 0;
        int sum_id = 0;
        for (const auto &r : stmt) {
            count++;
            sum_id += r["id"].as<int>();
        }
        REQUIRE(count == 4);
        REQUIRE(sum_id == 10);
    }

    SECTION("Range-for via db.query(...) helper")
    {
        int count = 0;
        for (const auto &r : db.query("SELECT id, label FROM items WHERE id > ? ORDER BY id ASC;", 1, 2)) {
            count++;
            REQUIRE(r["id"].as<int>() > 2);
        }
        REQUIRE(count == 2);
    }
}

/* ========================================================================= */
/* Test Case 4: Type-safe Row Extraction & Column Queries                    */
/* ========================================================================= */
TEST_CASE("Type-Safe Row Extraction")
{
    database db(":memory:");
    db.exec("CREATE TABLE products (id INTEGER, name TEXT, price REAL, note TEXT);");
    db.exec("INSERT INTO products VALUES (10, 'Book', 19.99, NULL);");

    statement stmt = db.prepare_statement("SELECT id, name, price, note FROM products WHERE id = 10;");
    row r = stmt.fetch_row();

    SECTION("Row size and column introspection")
    {
        REQUIRE(r.size() == 4);
        REQUIRE(r.has_column("name"));
        REQUIRE(r.has_column("price"));
        REQUIRE(!r.has_column("non_existent"));
    }

    SECTION("Type-safe as<T> extraction")
    {
        REQUIRE(r["id"].as<int>() == 10);
        REQUIRE(r["name"].as<std::string>() == "Book");
        REQUIRE(r["price"].as<double>() == 19.99);
        REQUIRE(r.get<int>("id") == 10);
        REQUIRE(r.get<std::string>("name") == "Book");
    }

    SECTION("NULL column checks with std::optional")
    {
        REQUIRE(!r.is_null("id"));
        REQUIRE(!r.is_null("name"));
        REQUIRE(r.is_null("note"));
        REQUIRE(r["note"].is_null());

        auto opt_note = r["note"].as<std::optional<std::string>>();
        REQUIRE(!opt_note.has_value());
        REQUIRE(opt_note == std::nullopt);

        auto opt_name = r["name"].as<std::optional<std::string>>();
        REQUIRE(opt_name.has_value());
        REQUIRE(*opt_name == "Book");
    }

    SECTION("Tuple deconstruction: row.as<Types...>()")
    {
        auto [id, name, price, note] = r.as<int, std::string, double, std::optional<std::string>>();
        REQUIRE(id == 10);
        REQUIRE(name == "Book");
        REQUIRE(price == 19.99);
        REQUIRE(!note.has_value());
    }

    SECTION("Out of bounds / non-existent column throws exception")
    {
        bool threw = false;
        try {
            r["does_not_exist"];
        } catch (const exception_column &) {
            threw = true;
        }
        REQUIRE(threw);
    }
}

/* ========================================================================= */
/* Test Case 5: RAII Transactions                                            */
/* ========================================================================= */
TEST_CASE("RAII Transactions")
{
    database db(":memory:");
    db.exec("CREATE TABLE accounts (id INTEGER PRIMARY KEY, balance REAL);");
    db.exec("INSERT INTO accounts VALUES (1, 1000.0);");

    SECTION("Explicit commit saves changes")
    {
        {
            transaction txn = db.begin_transaction();
            db.exec("UPDATE accounts SET balance = 1500.0 WHERE id = 1;");
            txn.commit();
        }

        statement check = db.prepare_statement("SELECT balance FROM accounts WHERE id = 1;");
        row r = check.fetch_row();
        REQUIRE(r["balance"].as<double>() == 1500.0);
    }

    SECTION("Uncommitted scope automatically rolls back")
    {
        {
            transaction txn = db.begin_transaction();
            db.exec("UPDATE accounts SET balance = 9999.0 WHERE id = 1;");
            // txn goes out of scope without commit()
        }

        statement check = db.prepare_statement("SELECT balance FROM accounts WHERE id = 1;");
        row r = check.fetch_row();
        REQUIRE(r["balance"].as<double>() == 1500.0);
    }

    SECTION("Exception during transaction triggers automatic rollback")
    {
        try {
            transaction txn = db.begin_transaction();
            db.exec("UPDATE accounts SET balance = 0.0 WHERE id = 1;");
            throw std::runtime_error("Simulated business error");
            txn.commit();
        } catch (const std::runtime_error &) {
            // expected
        }

        statement check = db.prepare_statement("SELECT balance FROM accounts WHERE id = 1;");
        row r = check.fetch_row();
        REQUIRE(r["balance"].as<double>() == 1500.0);
    }
}

/* ========================================================================= */
/* Test Case 6: Object Serialization & Strongly Typed Queries                */
/* ========================================================================= */

namespace test_models {
    struct Employee {
        int id;
        std::string name;
        double salary;
        std::optional<std::string> email;
    };

    // Non-intrusive macro mapping
    CXX_SQLITE3_DEFINE_TYPE_NON_INTRUSIVE(Employee, id, name, salary, email)

    struct CustomTag {
        std::string label;
        int level;
    };

    // Manual ADL free functions mapping
    inline void from_sqlite(const cxx_sqlite3::row &r, CustomTag &t) {
        t.label = r["label"].as<std::string>();
        t.level = r["level"].as<int>();
    }
    inline void to_sqlite(cxx_sqlite3::statement &st, const CustomTag &t) {
        st.bind(":label", t.label, ":level", t.level);
    }
}

TEST_CASE("Object Serialization and Strongly Typed Queries")
{
    database db(":memory:");
    db.exec("CREATE TABLE employees (id INTEGER PRIMARY KEY, name TEXT, salary REAL, email TEXT);");

    SECTION("Binding macro-defined struct directly via db.execute")
    {
        test_models::Employee alice{1, "Alice", 9200.0, "alice@example.com"};
        db.execute("INSERT INTO employees VALUES (:id, :name, :salary, :email);", alice);

        test_models::Employee bob{2, "Bob", 6500.0, std::nullopt};
        db.execute("INSERT INTO employees VALUES (:id, :name, :salary, :email);", bob);

        statement verify = db.prepare_statement("SELECT * FROM employees WHERE id = 1;");
        row r = verify.fetch_row();

        // Extracting row into native struct
        test_models::Employee read_alice = r.as<test_models::Employee>();
        REQUIRE(read_alice.id == 1);
        REQUIRE(read_alice.name == "Alice");
        REQUIRE(read_alice.salary == 9200.0);
        REQUIRE(read_alice.email.has_value());
        REQUIRE(*read_alice.email == "alice@example.com");
    }

    SECTION("Strongly typed query_as<T>() returning std::vector<T>")
    {
        // Querying all employees directly into std::vector<Employee>
        std::vector<test_models::Employee> staff = 
            db.query_as<test_models::Employee>("SELECT * FROM employees ORDER BY id ASC;");

        REQUIRE(staff.size() == 2);
        REQUIRE(staff[0].name == "Alice");
        REQUIRE(staff[1].name == "Bob");
        REQUIRE(!staff[1].email.has_value());
    }

    SECTION("Strongly typed query_one<T>() returning std::optional<T>")
    {
        auto opt_emp = db.query_one<test_models::Employee>("SELECT * FROM employees WHERE id = ?", 1);
        REQUIRE(opt_emp.has_value());
        REQUIRE(opt_emp->name == "Alice");

        auto opt_missing = db.query_one<test_models::Employee>("SELECT * FROM employees WHERE id = ?", 999);
        REQUIRE(!opt_missing.has_value());
    }

    SECTION("Manual ADL struct mapping (from_sqlite / to_sqlite)")
    {
        db.exec("CREATE TABLE tags (label TEXT, level INTEGER);");
        test_models::CustomTag tag{"admin", 5};
        db.execute("INSERT INTO tags VALUES (:label, :level);", tag);

        auto fetched = db.query_one<test_models::CustomTag>("SELECT * FROM tags WHERE level = 5;");
        REQUIRE(fetched.has_value());
        REQUIRE(fetched->label == "admin");
        REQUIRE(fetched->level == 5);
    }
}

/* ========================================================================= */
/* Test Runner Main                                                          */
/* ========================================================================= */
int main()
{
    std::cout << "========================================" << std::endl;
    std::cout << " Running cxx_sqlite3 Unit Test Suite    " << std::endl;
    std::cout << "========================================" << std::endl;

    for (const auto &tc : test_framework::get_registry()) {
        std::cout << "[ RUN      ] " << tc.name << std::endl;
        test_framework::g_total_tests++;
        try {
            tc.func();
            std::cout << "[       OK ] " << tc.name << std::endl;
        } catch (const std::exception &e) {
            std::cout << "[  FAILED  ] " << tc.name << " (" << e.what() << ")" << std::endl;
            test_framework::g_failed_tests++;
        } catch (...) {
            std::cout << "[  FAILED  ] " << tc.name << " (Unknown exception)" << std::endl;
            test_framework::g_failed_tests++;
        }
    }

    std::cout << "========================================" << std::endl;
    std::cout << " Tests: " << test_framework::g_total_tests 
              << ", Passed: " << (test_framework::g_total_tests - test_framework::g_failed_tests)
              << ", Failed: " << test_framework::g_failed_tests << std::endl;
    std::cout << "========================================" << std::endl;

    return (test_framework::g_failed_tests == 0) ? 0 : 1;
}

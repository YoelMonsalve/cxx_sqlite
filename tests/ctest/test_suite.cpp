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
 * (C) Copyright Yoel Monsalve 2019-2026.
 */

#include <iostream>
#include <string>
#include <vector>
#include <functional>
#include <stdexcept>
#include <cstdint>

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
/* Test Case 1: High-level Typing System (generic_object, ONULL, basic_object)*/
/* ========================================================================= */
TEST_CASE("Type System - generic_object and Conversions")
{
    SECTION("Null handling")
    {
        generic_object obj_null;
        REQUIRE(obj_null.is_null());
        REQUIRE(obj_null.type() == generic_object::type_tag::null_t);
        REQUIRE(!obj_null);

        generic_object obj_null_ptr(nullptr);
        REQUIRE(obj_null_ptr.is_null());

        SQLITE3_ONULL onull;
        REQUIRE(onull.is_null());
    }

    SECTION("Integer conversions")
    {
        generic_object obj_int(42);
        REQUIRE(!obj_int.is_null());
        REQUIRE(obj_int.type() == generic_object::type_tag::int_t);
        REQUIRE(obj_int.as<int>() == 42);
        REQUIRE(obj_int.as<int64_t>() == 42);
        REQUIRE(obj_int.as<double>() == 42.0);
        REQUIRE(obj_int.as<std::string>() == "42");
        REQUIRE(static_cast<int>(obj_int) == 42);
        REQUIRE(static_cast<bool>(obj_int) == true);
    }

    SECTION("Floating point conversions")
    {
        generic_object obj_double(3.14159);
        REQUIRE(!obj_double.is_null());
        REQUIRE(obj_double.type() == generic_object::type_tag::float_t);
        REQUIRE(obj_double.as<double>() > 3.14);
        REQUIRE(obj_double.as<int>() == 3);
        REQUIRE(static_cast<double>(obj_double) > 3.14);
    }

    SECTION("String conversions")
    {
        generic_object obj_str("Hello World");
        REQUIRE(!obj_str.is_null());
        REQUIRE(obj_str.type() == generic_object::type_tag::text_t);
        REQUIRE(obj_str.as<std::string>() == "Hello World");
        REQUIRE(static_cast<std::string>(obj_str) == "Hello World");
    }

    SECTION("Blob data handling")
    {
        std::vector<uint8_t> bytes = { 0xDE, 0xAD, 0xBE, 0xEF };
        generic_object obj_blob(bytes);
        REQUIRE(!obj_blob.is_null());
        REQUIRE(obj_blob.type() == generic_object::type_tag::blob_t);
        
        blob_t extracted = obj_blob.as<blob_t>();
        REQUIRE(extracted.size() == 4);
        REQUIRE(extracted[0] == 0xDE);
        REQUIRE(extracted[3] == 0xEF);
    }

    SECTION("basic_object typed wrappers")
    {
        SQLITE3_OINT num(10);
        REQUIRE(num.value() == 10);
        ++num;
        REQUIRE(num.value() == 11);
        num += 5;
        REQUIRE(num.value() == 16);

        SQLITE3_OTEXT str("sqlite3 wrapper");
        REQUIRE(str.value() == "sqlite3 wrapper");
    }
}

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

    SECTION("NULL column checks")
    {
        REQUIRE(!r.is_null("id"));
        REQUIRE(!r.is_null("name"));
        REQUIRE(r.is_null("note"));
        REQUIRE(r["note"].is_null());
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

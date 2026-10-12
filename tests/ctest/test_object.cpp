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
/* Test Case 1: High-level Typing System (generic_object, ONULL, basic_object)*/
/* ========================================================================= */
TEST_CASE("Type System - generic_object and Conversions")
{
    SECTION("Null handling")
    {
        generic_object obj_null;
        REQUIRE(obj_null.is_null());
        REQUIRE(obj_null.type() == generic_object::internal_type::null);
        REQUIRE(!obj_null);

        generic_object obj_null_ptr(nullptr);
        REQUIRE(obj_null_ptr.is_null());

        SQLITE3_ONULL onull;
        REQUIRE(onull.is_null());
    }

    SECTION("Integer constructors")
    {
        int x = 42;
        int &y = x;
        const int xc  = 42;
        const int &yc = xc;
        generic_object O1{x};    // int, by-value
        generic_object O2{y};    // int &
        generic_object O3{42};   // const int
        generic_object O4{xc};   // const int
        generic_object O5{yc};   // const int &
        generic_object O6{const_cast<const int &>(xc)};    // const int &
    }

    SECTION("Integer conversions")
    {   
        generic_object obj_int(42);
        REQUIRE(!obj_int.is_null());
        REQUIRE(obj_int.type() == generic_object::internal_type::integer);
        /* type checks */
        // @note .as<const int>() returns type <int>, not <const int>
        REQUIRE( (std::is_same_v<decltype(obj_int.as<int>()), int>) );
        REQUIRE( (std::is_same_v<decltype(obj_int.as<const int>()), int>) );
        REQUIRE( (std::is_same_v<decltype(obj_int.get<const int>()), int>) );  // get = as
        REQUIRE( (std::is_same_v<decltype(obj_int.as_int()), generic_value::int_t>) );

        /* value checks */
        REQUIRE(obj_int.as<int>() == 42);
        REQUIRE(obj_int.as<int64_t>() == 42);
        REQUIRE((int)obj_int == 42);
        REQUIRE(obj_int.as<double>() == 42.0);
        REQUIRE(obj_int.as<std::string>() == "42");
        REQUIRE(static_cast<int>(obj_int) == 42);
        REQUIRE(static_cast<bool>(obj_int) == true);
    }

    SECTION("Set&Get")
    {
        generic_object obj_int;
        REQUIRE(obj_int.set(42).get<int>() == 42);
    }

    SECTION("Arithmetic operations with explicit casts")
    {
        generic_object obj_int{42};
        int a = 3;
        REQUIRE((int)obj_int + 3 == 45);
        REQUIRE((int)obj_int + a == 45);
        REQUIRE((float)obj_int - 3 == 39.0);
        REQUIRE((float)obj_int - a == 39.0);
        REQUIRE(static_cast<int>(obj_int) + 3 == 45);
        REQUIRE(obj_int.as<int>() + 3 == 45);
    }

    SECTION("Floating point conversions")
    {
        generic_object obj_double(3.14159);
        REQUIRE(!obj_double.is_null());
        REQUIRE(obj_double.type() == generic_object::internal_type::floating);
        REQUIRE(obj_double.as<double>() > 3.14);
        REQUIRE(obj_double.as<int>() == 3);
        REQUIRE(static_cast<double>(obj_double) > 3.14);
    }

    SECTION("String conversions")
    {
        generic_object obj_str("Hello World");
        REQUIRE(!obj_str.is_null());
        REQUIRE(obj_str.type() == generic_object::internal_type::text);
        REQUIRE(obj_str.as<std::string>() == "Hello World");
        REQUIRE(static_cast<std::string>(obj_str) == "Hello World");
    }

    SECTION("Blob data handling")
    {
        std::vector<uint8_t> bytes = { 0xDE, 0xAD, 0xBE, 0xEF };
        generic_object obj_blob(bytes);
        REQUIRE(!obj_blob.is_null());
        REQUIRE(obj_blob.type() == generic_object::internal_type::blob);
        
        blob_t extracted = obj_blob.as<blob_t>();
        REQUIRE(extracted.size() == 4);
        REQUIRE(extracted[0] == 0xDE);
        REQUIRE(extracted[3] == 0xEF);
    }

    SECTION("std::optional nullability mapping")
    {
        generic_value null_val(nullptr);
        auto opt_int = null_val.as<std::optional<int>>();
        REQUIRE(!opt_int.has_value());
        REQUIRE(opt_int == std::nullopt);

        generic_value present_val(123);
        auto opt_present = present_val.as<std::optional<int>>();
        REQUIRE(opt_present.has_value());
        REQUIRE(*opt_present == 123);

        generic_value text_val("alpha");
        auto opt_str = text_val.as<std::optional<std::string>>();
        REQUIRE(opt_str.has_value());
        REQUIRE(*opt_str == "alpha");
    }

    /**
     * Added by Yoel.- 2026-10-10.
     */
    SECTION("sqlite bindable")
    {
        /** Checks whether the type T is convertible to generic_value */
        REQUIRE(is_sqlite_bindable<int>::value);    // C++14
        REQUIRE(is_sqlite_bindable_v<int>);         // C++17

        // and so on, for uint, long, ulong, float, double, ... etc, and null.
        
    }
    SECTION("sqlite extractable")
    {
        /** Checks whether the type T can be extracted/converted from 
         *  generic_value .
         *  (opposite of bindable, so we check the same) */
        REQUIRE(is_sqlite_extractable<int>::value);    // C++14
        REQUIRE(is_sqlite_extractable_v<int>);         // C++17
        // ...
    }
    SECTION("generic_value traits")
    {
        {
            generic_value x1(3);        // int
            //decltype(x1.as_int()) y = "hello";
            REQUIRE( (std::is_same_v<decltype(x1.as_int()), generic_value::int_t>) );
            REQUIRE( (std::is_same_v<decltype(x1.as<int>()), int>) );
            REQUIRE( (std::is_same_v<decltype((int)x1), int>) );
            REQUIRE( (std::is_same_v<decltype(static_cast<int>(x1)), int>) );

            CHECK(x1.as_int() == 3);
            CHECK(x1.as<int>() == 3);
            CHECK((int)x1 == 3);
            CHECK(static_cast<int>(x1) == 3);
            CHECK(x1.as_int() == x1.as<int>());

            // is_arithmethic ?
            cout << std::boolalpha;
            cout << "index() ? " << 
                x1.index() << std::endl;
            cout << "type() == int ? " << 
                (x1.type() == generic_value::internal_type::integer) << std::endl;
            cout << "is numeric? " << 
                x1.is_numeric() << std::endl;
            cout << "x + 10? " << 
                static_cast<int>(x1) + 10 << std::endl;
            cout << "is generic_value convertible from int ? " <<
                std::is_convertible_v<generic_object, int> << std::endl;

            x1 = "hello";
            cout << "index() ? " << 
                x1.index() << std::endl;
            cout << "type() == int ? " << 
                (x1.type() == generic_value::internal_type::integer) << std::endl;
            cout << "is numeric? " << 
                x1.is_numeric() << std::endl;
            cout << "is text? " << 
                x1.is_text() << std::endl;

        } // ... x1 goes out scope
    }
}

/* ========================================================================= */
/* Test Case 2: Variadic Parameter Binding                                   */
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

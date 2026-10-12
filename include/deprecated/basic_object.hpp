/**
 * BASIC_OBJECT.HPP
 * 
 * Deprecated / experimental smart scalar wrapper.
 * 
 * (C) Copyright Yoel Monsalve 2025-2026.
 */

#ifndef CXX_SQLITE3_DEPRECATED_BASIC_OBJECT_HPP_
#define CXX_SQLITE3_DEPRECATED_BASIC_OBJECT_HPP_

#include "../object.hpp"

namespace cxx_sqlite3
{
    /** 
     * @remark
     * The `basic_object<T>` is intended as a natural evolution of
     * the simple container class `generic_value<T>`, maybe
     * endowed/empowered with additional capabilities.
     * For example, the `basic_object` can hold arithmetic 
     * operations natively, ... and ...  also, ...
     */
    template <class T>
    struct basic_object : public generic_value
    {
        using value_type = T;
        using type = basic_object;

        basic_object() : generic_value() {}
        basic_object(const T &v) : generic_value(v) {}
        basic_object(T &&v) : generic_value(std::move(v)) {}
        basic_object(const char *s) : generic_value(s) {}
        explicit basic_object(const generic_value &o) : generic_value(o) {}
        explicit basic_object(generic_value &&o) : generic_value(std::move(o)) {}

        T value() const { return as<T>(); }
        T operator()() const { return as<T>(); }
        operator T() const { return as<T>(); }

        template <typename U = T, typename = std::enable_if_t<std::is_arithmetic_v<U>>>
        basic_object &operator++() {
            storage_ = as_int() + 1;
            return *this;
        }
        template <typename U = T, typename = std::enable_if_t<std::is_arithmetic_v<U>>>
        basic_object operator++(int) {
            basic_object copy(*this);
            ++(*this);
            return copy;
        }
        template <typename U = T, typename = std::enable_if_t<std::is_arithmetic_v<U>>>
        basic_object &operator--() {
            storage_ = as_int() - 1;
            return *this;
        }
        template <typename U = T, typename = std::enable_if_t<std::is_arithmetic_v<U>>>
        basic_object operator--(int) {
            basic_object copy(*this);
            --(*this);
            return copy;
        }
        template <typename U = T, typename = std::enable_if_t<std::is_arithmetic_v<U>>>
        basic_object &operator+=(T inc) {
            if constexpr (std::is_floating_point_v<U>) {
                storage_ = as_double() + static_cast<double>(inc);
            } else {
                storage_ = as_int() + static_cast<int_t>(inc);
            }
            return *this;
        }
        template <typename U = T, typename = std::enable_if_t<std::is_arithmetic_v<U>>>
        basic_object &operator-=(T dec) {
            if constexpr (std::is_floating_point_v<U>) {
                storage_ = as_double() - static_cast<double>(dec);
            } else {
                storage_ = as_int() - static_cast<int_t>(dec);
            }
            return *this;
        }
    };
}

typedef cxx_sqlite3::basic_object<int> SQLITE3_OINT;           // equivalent to sqlite3 INTEGER
typedef cxx_sqlite3::basic_object<char> SQLITE3_OCHAR;
typedef cxx_sqlite3::basic_object<float> SQLITE3_OFLOAT;
typedef cxx_sqlite3::basic_object<double> SQLITE3_ODOUBLE;     // equivalent to sqlite3 DOUBLE/REAL
typedef cxx_sqlite3::basic_object<cxx_sqlite3::generic_value::text_t> SQLITE3_OTEXT;  // equivalent to sqlite3 TEXT
typedef cxx_sqlite3::basic_object<std::string> SQLITE3_OSTRING;
typedef cxx_sqlite3::basic_object<cxx_sqlite3::generic_value::blob_t> SQLITE3_OBLOB; // equivalent to sqlite3 BLOB

#endif

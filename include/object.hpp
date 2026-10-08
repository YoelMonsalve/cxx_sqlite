/**
 * OBJECT.HPP
 * Define a common C++ wrapper and type system for SQLite data types
 * (NULL, INTEGER, FLOAT, TEXT, BLOB).
 * 
 * @version 1.0.0 
 * @author  Yoel Monsalve
 * @date    2023-11-11
 * 
 * (C) Copyright Yoel Monsalve 2019-2026.
 */

#ifndef OBJECT_HPP_
#define OBJECT_HPP_

#include <iostream>
#include <cassert>
#include <vector>
#include <string>
#include <cstdint>
#include <utility>

#include "cpp_version.hpp"

namespace cxx_sqlite3
{
    using blob_t = std::vector<uint8_t>;

    /** 
     * generic_object
     * 
     * Serves as a dynamic value container for any native SQLite data type
     * (NULL, INTEGER, FLOAT, TEXT, BLOB) with type-safe extraction.
     */
    class generic_object
    {
    public:
        enum class type_tag {
            null_t,
            int_t,
            float_t,
            text_t,
            blob_t
        };

    protected:
        type_tag type_ = type_tag::null_t;
        int64_t int_val_ = 0;
        double double_val_ = 0.0;
        std::string text_val_;
        blob_t blob_val_;

    public:
        /* Default / NULL constructors */
        generic_object() : type_(type_tag::null_t), int_val_(0), double_val_(0.0) {}
        generic_object(std::nullptr_t) : type_(type_tag::null_t), int_val_(0), double_val_(0.0) {}
        explicit generic_object(void *p) : type_(p ? type_tag::blob_t : type_tag::null_t), int_val_(0), double_val_(0.0) {}

        /* Numeric constructors */
        generic_object(int64_t val)
            : type_(type_tag::int_t), int_val_(val), double_val_(static_cast<double>(val)) {}
        generic_object(int val)
            : generic_object(static_cast<int64_t>(val)) {}
        generic_object(unsigned int val)
            : generic_object(static_cast<int64_t>(val)) {}
        generic_object(unsigned long val)
            : generic_object(static_cast<int64_t>(val)) {}
        generic_object(unsigned long long val)
            : generic_object(static_cast<int64_t>(val)) {}
        generic_object(double val)
            : type_(type_tag::float_t), int_val_(static_cast<int64_t>(val)), double_val_(val) {}
        generic_object(float val)
            : generic_object(static_cast<double>(val)) {}

        /* Text constructors */
        generic_object(const char *val)
            : type_(val ? type_tag::text_t : type_tag::null_t),
              int_val_(0), double_val_(0.0), text_val_(val ? val : "") {}
        generic_object(const std::string &val)
            : type_(type_tag::text_t), int_val_(0), double_val_(0.0), text_val_(val) {}
        generic_object(std::string &&val)
            : type_(type_tag::text_t), int_val_(0), double_val_(0.0), text_val_(std::move(val)) {}

        /* Blob constructors */
        generic_object(const blob_t &val)
            : type_(type_tag::blob_t), int_val_(0), double_val_(0.0), blob_val_(val) {}
        generic_object(blob_t &&val)
            : type_(type_tag::blob_t), int_val_(0), double_val_(0.0), blob_val_(std::move(val)) {}
        generic_object(const void *data, size_t size)
            : type_(type_tag::blob_t), int_val_(0), double_val_(0.0) {
            if (data && size > 0) {
                const uint8_t *b = static_cast<const uint8_t *>(data);
                blob_val_.assign(b, b + size);
            }
        }

        virtual ~generic_object() = default;

        /* State queries */
        bool is_null() const noexcept { return type_ == type_tag::null_t; }
        type_tag type() const noexcept { return type_; }

        /* Conversions */
        int64_t as_int() const {
            if (type_ == type_tag::int_t) return int_val_;
            if (type_ == type_tag::float_t) return static_cast<int64_t>(double_val_);
            if (type_ == type_tag::text_t && !text_val_.empty()) {
                try { return std::stoll(text_val_); } catch (...) { return 0; }
            }
            return 0;
        }

        double as_double() const {
            if (type_ == type_tag::float_t) return double_val_;
            if (type_ == type_tag::int_t) return static_cast<double>(int_val_);
            if (type_ == type_tag::text_t && !text_val_.empty()) {
                try { return std::stod(text_val_); } catch (...) { return 0.0; }
            }
            return 0.0;
        }

        std::string as_string() const {
            if (type_ == type_tag::text_t) return text_val_;
            if (type_ == type_tag::int_t) return std::to_string(int_val_);
            if (type_ == type_tag::float_t) return std::to_string(double_val_);
            if (type_ == type_tag::null_t) return "";
            return std::string(blob_val_.begin(), blob_val_.end());
        }

        const blob_t &as_blob() const noexcept {
            return blob_val_;
        }

        /* Type-safe template extraction */
        template <typename T>
        T as() const;

        /* Ergonomic implicit conversions */
        operator int() const { return static_cast<int>(as_int()); }
        operator int64_t() const { return as_int(); }
        operator double() const { return as_double(); }
        operator float() const { return static_cast<float>(as_double()); }
        operator std::string() const { return as_string(); }
        operator blob_t() const { return as_blob(); }
        operator bool() const {
            return !is_null() && (as_int() != 0 || as_double() != 0.0 || !as_string().empty());
        }

        bool operator!() const { return !static_cast<bool>(*this); }

        bool operator==(const generic_object &other) const {
            if (type_ != other.type_) return false;
            if (type_ == type_tag::null_t) return true;
            if (type_ == type_tag::int_t) return int_val_ == other.int_val_;
            if (type_ == type_tag::float_t) return double_val_ == other.double_val_;
            if (type_ == type_tag::text_t) return text_val_ == other.text_val_;
            return blob_val_ == other.blob_val_;
        }
        bool operator!=(const generic_object &other) const { return !(*this == other); }

        friend std::ostream &operator<<(std::ostream &os, const generic_object &obj) {
            if (obj.is_null()) return os << "NULL";
            if (obj.type() == type_tag::int_t) return os << obj.as_int();
            if (obj.type() == type_tag::float_t) return os << obj.as_double();
            return os << obj.as_string();
        }
    };

    /* Specializations for as<T>() */
    template <> inline int generic_object::as<int>() const { return static_cast<int>(as_int()); }
    template <> inline int64_t generic_object::as<int64_t>() const { return as_int(); }
    template <> inline double generic_object::as<double>() const { return as_double(); }
    template <> inline float generic_object::as<float>() const { return static_cast<float>(as_double()); }
    template <> inline std::string generic_object::as<std::string>() const { return as_string(); }
    template <> inline blob_t generic_object::as<blob_t>() const { return as_blob(); }
    template <> inline bool generic_object::as<bool>() const { return static_cast<bool>(*this); }

    /** Specialized NULL object */
    struct null_object : public generic_object {
        null_object() : generic_object(nullptr) {}
    };
    typedef null_object SQLITE3_ONULL;

    /** Typed basic_object<T> wrapper maintaining author's API */
    template <class T>
    struct basic_object : public generic_object
    {
        using value_type = T;
        using type = basic_object;

        basic_object() : generic_object() {}
        basic_object(const T &v) : generic_object(v) {}
        basic_object(T &&v) : generic_object(std::move(v)) {}
        basic_object(const char *s) : generic_object(s) {}
        explicit basic_object(const generic_object &o) : generic_object(o) {}
        explicit basic_object(generic_object &&o) : generic_object(std::move(o)) {}

        T value() const { return as<T>(); }
        T operator()() const { return as<T>(); }
        operator T() const { return as<T>(); }

        basic_object &operator++() {
            int_val_++;
            double_val_ = static_cast<double>(int_val_);
            return *this;
        }
        basic_object operator++(int) {
            basic_object copy(*this);
            ++(*this);
            return copy;
        }
        basic_object &operator--() {
            int_val_--;
            double_val_ = static_cast<double>(int_val_);
            return *this;
        }
        basic_object operator--(int) {
            basic_object copy(*this);
            --(*this);
            return copy;
        }
        basic_object &operator+=(T inc) {
            int_val_ += static_cast<int64_t>(inc);
            double_val_ += static_cast<double>(inc);
            return *this;
        }
        basic_object &operator-=(T dec) {
            int_val_ -= static_cast<int64_t>(dec);
            double_val_ -= static_cast<double>(dec);
            return *this;
        }
    };
}

typedef cxx_sqlite3::basic_object<int> SQLITE3_OINT;           // equivalent to sqlite3 INTEGER
typedef cxx_sqlite3::basic_object<char> SQLITE3_OCHAR;
typedef cxx_sqlite3::basic_object<float> SQLITE3_OFLOAT;
typedef cxx_sqlite3::basic_object<double> SQLITE3_ODOUBLE;     // equivalent to sqlite3 DOUBLE/REAL
typedef cxx_sqlite3::basic_object<std::string> SQLITE3_OTEXT;  // equivalent to sqlite3 TEXT
typedef cxx_sqlite3::basic_object<std::string> SQLITE3_OSTRING;
typedef cxx_sqlite3::basic_object<cxx_sqlite3::blob_t> SQLITE3_OBLOB; // equivalent to sqlite3 BLOB
typedef cxx_sqlite3::generic_object SQLITE3_OGEN;

#endif

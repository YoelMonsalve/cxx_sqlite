/**
 * OBJECT.HPP
 * 
 * -.- The cxx_sqlite3 library -.-
 * 
 * Defines the modern generic_value type based on std::variant,
 * providing rich type-safe extraction via as<T>(), std::optional<T> support,
 * and seamless conversion between SQLite's 5 native storage classes
 * (NULL, INTEGER, FLOAT, TEXT, BLOB).
 * 
 * @version 1.1.0 
 * @author  Yoel Monsalve
 * @date    2023-11-11
 * 
 * (C) Copyright Yoel Monsalve 2025-2026.
 */

#ifndef OBJECT_HPP_
#define OBJECT_HPP_

#include <iostream>
#include <vector>
#include <string>
#include <string_view>
#include <cstdint>
#include <utility>
#include <variant>
#include <optional>
#include <type_traits>
#include <stdexcept>

#include "cpp_version.hpp"

namespace cxx_sqlite3
{
    using blob_t = std::vector<uint8_t>;
    using null_t = std::nullptr_t;

    /* Semantics valid for C++14/C++17.
     * Comment if you are using C++20.
     */
    template <typename T>
    using remove_cvref_t = std::remove_cv_t<std::remove_reference_t<T>>;
    /* C++20 */
    //using std::remove_cvref_t;

    /**
     * Storage classes supported natively by SQLite.
     */
    using value_storage = std::variant<
        null_t,         // 0: SQL NULL
        int64_t,        // 1: SQL INTEGER
        double,         // 2: SQL REAL / FLOAT
        std::string,    // 3: SQL TEXT
        blob_t          // 4: SQL BLOB
    >;

    /**
     * generic_value
     * 
     * Represents a dynamic SQLite value container built on std::variant.
     * Supports type-safe extraction with as<T>(), nullability mapping via
     * std::optional<T>, and natural conversions.
     */
    class generic_value
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
        value_storage storage_ = nullptr;

    public:
        /* Default / NULL constructors */
        generic_value() noexcept : storage_(nullptr) {}
        generic_value(std::nullptr_t) noexcept : storage_(nullptr) {}
        explicit generic_value(void *p) : storage_(p ? value_storage(blob_t{}) : value_storage(nullptr)) {}

        /* Numeric and enumeration constructors (consolidated via traits) */
        template <typename T, typename = std::enable_if_t<std::is_arithmetic_v<T> || std::is_enum_v<T>>>
        generic_value(T val) noexcept {
            if constexpr (std::is_floating_point_v<T>) {
                storage_ = static_cast<double>(val);
            } else if constexpr (std::is_enum_v<T>) {
                using Underlying = std::underlying_type_t<T>;
                storage_ = static_cast<int64_t>(static_cast<Underlying>(val));
            } else {
                storage_ = static_cast<int64_t>(val);
            }
        }

        /* Text constructors */
        generic_value(const char *val) : storage_(val ? value_storage(std::string(val)) : value_storage(nullptr)) {}
        generic_value(std::string_view val) : storage_(std::string(val)) {}
        generic_value(const std::string &val) : storage_(val) {}
        generic_value(std::string &&val) noexcept : storage_(std::move(val)) {}

        /* Blob constructors */
        generic_value(const blob_t &val) : storage_(val) {}
        generic_value(blob_t &&val) noexcept : storage_(std::move(val)) {}
        generic_value(const void *data, size_t size) {
            if (data && size > 0) {
                const uint8_t *b = static_cast<const uint8_t *>(data);
                storage_ = blob_t(b, b + size);
            } else {
                storage_ = blob_t{};
            }
        }

        /* Optional constructor: if empty, initializes to NULL */
        template <typename T>
        generic_value(const std::optional<T> &opt) {
            if (opt.has_value()) {
                *this = generic_value(*opt);
            } else {
                storage_ = nullptr;
            }
        }

        virtual ~generic_value() = default;

        /* State and type inspection */
        type_tag type() const noexcept {
            switch (storage_.index()) {
            case 0: return type_tag::null_t;
            case 1: return type_tag::int_t;
            case 2: return type_tag::float_t;
            case 3: return type_tag::text_t;
            case 4: return type_tag::blob_t;
            default: return type_tag::null_t;
            }
        }

        bool is_null() const noexcept {
            return type() == type_tag::null_t;
        }

        const value_storage &storage() const noexcept { return storage_; }

        /* Coercion and conversion methods */
        int64_t as_int() const {
            return std::visit([](const auto &arg) -> int64_t {
                using T = remove_cvref_t<decltype(arg)>;
                if constexpr (std::is_same_v<T, null_t>) {
                    return 0;
                } else if constexpr (std::is_same_v<T, int64_t>) {
                    return arg;
                } else if constexpr (std::is_same_v<T, double>) {
                    return static_cast<int64_t>(arg);
                } else if constexpr (std::is_same_v<T, std::string>) {
                    try { return std::stoll(arg); } catch (...) { return 0; }
                } else {
                    return 0;
                }
            }, storage_);
        }

        double as_double() const {
            return std::visit([](const auto &arg) -> double {
                using T = remove_cvref_t<decltype(arg)>;
                if constexpr (std::is_same_v<T, null_t>) {
                    return 0.0;
                } else if constexpr (std::is_same_v<T, int64_t>) {
                    return static_cast<double>(arg);
                } else if constexpr (std::is_same_v<T, double>) {
                    return arg;
                } else if constexpr (std::is_same_v<T, std::string>) {
                    try { return std::stod(arg); } catch (...) { return 0.0; }
                } else {
                    return 0.0;
                }
            }, storage_);
        }

        /** 
         *  @todo: --default case--
         *  What happens if the current stored value is not
         *  convertible to string?
         *  Should it throw? 
         *  ...
         *  } else {
         *      throw std::bad_variant_access();
         *  }
         */
        std::string as_string() const {
            return std::visit([](const auto &arg) -> std::string {
                using T = remove_cvref_t<decltype(arg)>;
                if constexpr (std::is_same_v<T, null_t>) {
                    return "";
                } else if constexpr (std::is_same_v<T, int64_t>) {
                    return std::to_string(arg);
                } else if constexpr (std::is_same_v<T, double>) {
                    return std::to_string(arg);
                } else if constexpr (std::is_same_v<T, std::string>) {
                    return arg;
                } else if constexpr (std::is_same_v<T, blob_t>) {
                    return std::string(arg.begin(), arg.end());
                }
            }, storage_);
        }

        std::string_view as_string_view() const {
            if (const auto *p = std::get_if<std::string>(&storage_)) {
                return *p;
            }
            //return {};
            throw std::bad_variant_access();
        }

        const blob_t &as_blob() const {
            static const blob_t empty_blob{};
            if (const auto *p = std::get_if<blob_t>(&storage_)) {
                return *p;
            }
            return empty_blob;
        }

        /* Type-safe extraction template */
        template <typename T>
        T as() const;

        /* Implicit conversions for ergonomic natural usage */
        operator int() const { return static_cast<int>(as_int()); }
        operator int64_t() const { return as_int(); }
        operator double() const { return as_double(); }
        operator float() const { return static_cast<float>(as_double()); }
        operator std::string() const { return as_string(); }
        operator std::string_view() const noexcept { return as_string_view(); }
        operator blob_t() const { return as_blob(); }
        operator bool() const {
            return !is_null() && (as_int() != 0 || as_double() != 0.0 || !as_string().empty());
        }

        bool operator!() const { return !static_cast<bool>(*this); }

        bool operator==(const generic_value &other) const {
            return storage_ == other.storage_;
        }
        bool operator!=(const generic_value &other) const { return !(*this == other); }

        friend std::ostream &operator<<(std::ostream &os, const generic_value &obj) {
            if (obj.is_null()) return os << "NULL";
            if (obj.type() == type_tag::int_t) return os << obj.as_int();
            if (obj.type() == type_tag::float_t) return os << obj.as_double();
            return os << obj.as_string();
        }
    };

    /** Backward compatibility alias */
    using generic_object = generic_value;

    /* Primary converter template */
    template <typename T, typename Enable = void>
    struct value_extractor {
        using CleanT = remove_cvref_t<T>;

        static CleanT extract(const generic_value &v) {
            if constexpr (std::is_enum_v<CleanT>) {
                using Underlying = std::underlying_type_t<CleanT>;
                return static_cast<CleanT>(v.as<Underlying>());
            } else if constexpr (std::is_integral_v<CleanT> && !std::is_same_v<CleanT, bool>) {
                return static_cast<CleanT>(v.as_int());
            } else if constexpr (std::is_same_v<CleanT, bool>) {
                return static_cast<bool>(v);
            } else if constexpr (std::is_floating_point_v<CleanT>) {
                return static_cast<CleanT>(v.as_double());
            } else if constexpr (std::is_same_v<CleanT, std::string>) {
                return v.as_string();
            } else if constexpr (std::is_same_v<CleanT, std::string_view>) {
                return v.as_string_view();
            } else if constexpr (std::is_same_v<CleanT, blob_t>) {
                return v.as_blob();
            } else {
                static_assert(!sizeof(CleanT), "cxx_sqlite3: Unsupported type for generic_value::as<T>()");
            }
        }
    };

    /* 
     * Specialization for std::optional<T> 
     * Supports casting `generic_value` to nullable types.
     * */
    template <typename T>
    struct value_extractor<std::optional<T>> {
        static std::optional<T> extract(const generic_value &v) {
            return !(v.is_null()) ? 
                std::optional<T>(v.as<T>()) : std::nullopt;
        }
    };

    template <typename T>
    inline T generic_value::as() const {
        using CleanT = remove_cvref_t<T>;
        return static_cast<T>(value_extractor<CleanT>::extract(*this));
    }

    /** Specialized NULL object */
    struct null_object : public generic_value {
        null_object() : generic_value(nullptr) {}
    };
    typedef null_object SQLITE3_ONULL;

    /** Typed basic_object<T> wrapper maintaining author's API */
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
                storage_ = as_int() + static_cast<int64_t>(inc);
            }
            return *this;
        }
        template <typename U = T, typename = std::enable_if_t<std::is_arithmetic_v<U>>>
        basic_object &operator-=(T dec) {
            if constexpr (std::is_floating_point_v<U>) {
                storage_ = as_double() - static_cast<double>(dec);
            } else {
                storage_ = as_int() - static_cast<int64_t>(dec);
            }
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
typedef cxx_sqlite3::generic_value SQLITE3_OGEN;

#endif

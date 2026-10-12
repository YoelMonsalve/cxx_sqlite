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
#include "exception.hpp"

#if __has_include("cxx_sqlite3_config.hpp")
    #include "cxx_sqlite3_config.hpp"
#elif __has_include("../cxx_sqlite3_config.hpp")
    #include "../cxx_sqlite3_config.hpp"
#endif

#ifndef CXX_SQLITE3_EXPLICIT_CONV
    #if defined(CXX_SQLITE3_NO_EXPLICIT) || defined(NO_EXPLICIT)
        #define CXX_SQLITE3_EXPLICIT_CONV
    #else
        #define CXX_SQLITE3_EXPLICIT_CONV explicit
    #endif
#endif

namespace cxx_sqlite3
{
    /* Semantics valid for C++14/C++17.
     * Comment if you are using C++20.
     */
    template <typename T>
    using remove_cvref_t = std::remove_cv_t<std::remove_reference_t<T>>;
    /* C++20 */
    //using std::remove_cvref_t;

    /* =========================================================================
     * Definition of the Base Class
     * ========================================================================= */

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
        /** 
         * Equivalence between human-comprehensible types, and internal
         * representation used by SQLITE */
        using int_t    = int64_t;
        using bool_t   = int_t;     /* is this assumption correct ? */
        using double_t = double;
        using blob_t   = std::vector<uint8_t>;
        using text_t   = std::string;
        using null_t   = std::nullptr_t;
        /* aliases */
        using int_type    = int_t;
        using bool_type   = bool_t;
        using double_type = double_t;
        using float_t     = double_t;
        using float_type  = float_t;
        using blob_type   = blob_t;
        using text_type   = text_t;
        using null_type   = null_t;
        
        enum class internal_type {
            null = 0,
            integer,
            floating,
            text,
            blob
        };
        
        /**
         * Storage classes supported natively by SQLite.
         */
        using value_storage = std::variant<
            null_t,         // 0: SQL NULL
            int_t,          // 1: SQL INTEGER
            double_t,       // 2: SQL REAL / FLOAT
            text_t,         // 3: SQL TEXT
            blob_t          // 4: SQL BLOB
        >;
 
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
                storage_ = static_cast<int_t>(static_cast<Underlying>(val));
            } else {
                storage_ = static_cast<int_t>(val);
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
        template <typename T, typename = std::enable_if_t<std::is_constructible_v<generic_value, const T &>>>
        generic_value(const std::optional<T> &opt) {
            if (opt.has_value()) {
                *this = generic_value(*opt);
            } else {
                storage_ = nullptr;
            }
        }

        virtual ~generic_value() = default;

        /* State and type inspection */
        internal_type type() const noexcept {
            return (internal_type)storage_.index();
            /* the above, is equivalent to */
            switch (storage_.index()) {
            case 0: return internal_type::null;
            case 1: return internal_type::integer;
            case 2: return internal_type::floating;
            case 3: return internal_type::text;
            case 4: return internal_type::blob;
            default: return internal_type::null;
            }
        }

        /* Getter methods for underlying container */
        int index() const noexcept { return storage_.index(); }
        value_storage storage() const noexcept { return storage_; }

        /**
         * Coercive method that forces exactly the same type as it
         * was stored.
         */
        template <typename T>
        inline T value() const { return std::get<T>(storage()); }

        /** Internal data type getters */
        bool is_null() const noexcept { return type() == internal_type::null; }
        bool is_int() const noexcept { return type() == internal_type::integer; }
        bool is_float() const noexcept { return type() == internal_type::floating; }
        bool is_text() const noexcept { return type() == internal_type::text; }
        bool is_blob() const noexcept { return type() == internal_type::blob; }
        bool is_numeric() const noexcept { return is_int() || is_float(); }

        /* Coercion and conversion methods */
        int_t 
        as_int() const {
            return std::visit([](const auto &arg) -> int_t {
                using T = remove_cvref_t<decltype(arg)>;
                if constexpr (std::is_same_v<T, null_t>) {
                    return 0;
                } else if constexpr (std::is_same_v<T, int_t>) {
                    return arg;
                } else if constexpr (std::is_same_v<T, double_t>) {
                    return static_cast<int_t>(arg);
                } else if constexpr (std::is_same_v<T, text_t>) {
                    try { return std::stoll(arg); } 
                    catch (std::exception &e) { 
                        throw exception_value_bad_cast("unknown", "int", "cannot cast value to int");
                    }
                } else {
                    throw exception_value_bad_cast("unknown", "int", "cannot cast value to int");
                }
            }, storage_);
        }

        double_t 
        as_double() const {
            return std::visit([](const auto &arg) -> double_t {
                using T = remove_cvref_t<decltype(arg)>;
                if constexpr (std::is_same_v<T, null_t>) {
                    return 0.0;
                } else if constexpr (std::is_same_v<T, int_t>) {
                    return static_cast<double>(arg);
                } else if constexpr (std::is_same_v<T, double_t>) {
                    return arg;
                } else if constexpr (std::is_same_v<T, text_t>) {
                    try { return std::stod(arg); } 
                    catch (std::exception &e) { 
                        throw exception_value_bad_cast("unknown", "double", "cannot cast value to double");
                    }
                } else {
                    throw exception_value_bad_cast("unknown", "double", "cannot cast non-double value to double");
                }
            }, storage_);
        }

        text_t
        as_string() const {
            return std::visit([](const auto &arg) -> std::string {
                using T = remove_cvref_t<decltype(arg)>;
                if constexpr (std::is_same_v<T, null_t>) {
                    return "";
                } else if constexpr (std::is_same_v<T, int_t>) {
                    return std::to_string(arg);
                } else if constexpr (std::is_same_v<T, double_t>) {
                    return std::to_string(arg);
                } else if constexpr (std::is_same_v<T, text_t>) {
                    return arg;
                } else if constexpr (std::is_same_v<T, blob_t>) {
                    return std::string(arg.begin(), arg.end());
                } else {
                    /* @attention  There are no more possible types in the
                     *             variant. Though it may change in the future.
                     */
                    return "";
                } 
            }, storage_);
        }

        std::string_view as_string_view() const {
            if (const auto *p = std::get_if<std::string>(&storage_)) {
                return *p;
            }
            /* fallback */
            throw exception_value_bad_cast("unknown", "string_view", "cannot cast non-string value to string_view");
        }

        const blob_t &as_blob() const {
            static const blob_t empty_blob{};
            if (const auto *p = std::get_if<blob_t>(&storage_)) {
                return *p;
            }
            /* fallback */
            throw exception_value_bad_cast("unknown", "blob", "cannot cast value to blob_t");
        }

        /* Type-safe extraction template */
        template <typename T>
        T as() const;
        template <typename T>
        T get() const { return as<T>(); }

        /* Safe / non-throwing extraction helpers (reusing as<T>()) */
        template <typename T>
        std::optional<T> try_as() const noexcept;

        template <typename T>
        T as_or(T fallback) const noexcept;

        /* Helper method (get's complement) */
        template <typename T>
        generic_value &set(T val) { return *this = val; }

        /* Conversion operators (governed by CXX_SQLITE3_EXPLICIT_CONV) */
        CXX_SQLITE3_EXPLICIT_CONV operator int() const { return static_cast<int>(as_int()); }
        CXX_SQLITE3_EXPLICIT_CONV operator int_t() const { return as_int(); }
        CXX_SQLITE3_EXPLICIT_CONV operator double() const { return as_double(); }
        CXX_SQLITE3_EXPLICIT_CONV operator float() const { return static_cast<float>(as_double()); }
        CXX_SQLITE3_EXPLICIT_CONV operator std::string() const { return as_string(); }
        CXX_SQLITE3_EXPLICIT_CONV operator std::string_view() const { return as_string_view(); }
        CXX_SQLITE3_EXPLICIT_CONV operator blob_t() const { return as_blob(); }

        /* Contextual boolean test: always strictly explicit to prevent accidental arithmetic promotion */
        explicit operator bool() const noexcept {
            return std::visit([](const auto &arg) noexcept -> bool {
                using T = remove_cvref_t<decltype(arg)>;
                if constexpr (std::is_same_v<T, null_t>) {
                    return false;
                } else if constexpr (std::is_same_v<T, int_t>) {
                    return arg != 0;
                } else if constexpr (std::is_same_v<T, double_t>) {
                    return arg != 0.0;
                } else if constexpr (std::is_same_v<T, text_t>) {
                    return !arg.empty();
                } else if constexpr (std::is_same_v<T, blob_t>) {
                    return !arg.empty();
                } else {
                    return false;
                }
            }, storage_);
        }

        // @attention  Can this method throw? is not, protect it with noexcept
        bool operator!() const noexcept { return !static_cast<bool>(*this); }

        bool operator==(const generic_value &other) const {
            return storage_ == other.storage_ 
                && storage_.index() == storage_.index();
        }
        bool operator!=(const generic_value &other) const { return !(*this == other); }

        friend std::ostream &operator<<(std::ostream &os, const generic_value &obj) {
            if (obj.is_null()) return os << "NULL";
            if (obj.type() == internal_type::integer) return os << obj.as_int();
            if (obj.type() == internal_type::floating) return os << obj.as_double();
            return os << obj.as_string();
        }
    };

    /** Backward compatibility alias */
    using generic_object = generic_value;

    /* Forward declaration of value_extractor */
    template <typename T, typename Enable = void>
    struct value_extractor;

    /* =========================================================================
     * Value Extractor Implementations
     * ========================================================================= */

    /* Built-in supported types helper */
    namespace detail {
        template <typename T>
        inline constexpr bool is_builtin_extractable_v =
            std::is_arithmetic_v<T> ||
            std::is_enum_v<T> ||
            std::is_same_v<T, generic_value::text_t> ||
            std::is_same_v<T, std::string_view> ||
            std::is_same_v<T, generic_value::blob_t>;
    }

    /** 
     * Primary converter template for built-in supported types
     *
     * AGENT: DO NOT REMOVE THIS NOTE
     * =======================================
     * @note  The extractor first decay the provided type to its
     *        cv-unqualified primitive: `CleanT = remove_cvref_t<T>`.
     *        Then, tries to match the proper conversion operator
     *        depending of the unqualified type `CleanT`.
     *        
     *        Finally, return the result casted to `CleanT` (integer, 
     *        enum, floating).
     */
    template <typename T, typename Enable>
    struct value_extractor {
        using CleanT = remove_cvref_t<T>;
        static_assert(detail::is_builtin_extractable_v<CleanT>,
                      "cxx_sqlite3: Unsupported type for generic_value::as<T>()");

        static CleanT extract(const generic_value &v) {
            if constexpr (std::is_enum_v<CleanT>) {
                using Underlying = std::underlying_type_t<CleanT>;
                return static_cast<CleanT>(v.as<Underlying>());
            } else if constexpr (std::is_integral_v<CleanT> && !std::is_same_v<CleanT, bool>) {
                return static_cast<CleanT>(v.as_int());
            } else if constexpr (std::is_floating_point_v<CleanT>) {
                return static_cast<CleanT>(v.as_double());
            } else if constexpr (std::is_same_v<CleanT, generic_value::text_t>) {
                return v.as_string();
            } else if constexpr (std::is_same_v<CleanT, std::string_view>) {
                return v.as_string_view();
            } else if constexpr (std::is_same_v<CleanT, generic_value::blob_t>) {
                return v.as_blob();
            } else if constexpr (std::is_same_v<CleanT, bool>) {
                return static_cast<bool>(v);
            }
        }
    };

    /* 
     * Specialization for std::optional<T> 
     * Supports casting generic_value to nullable types.
     */
    template <typename T>
    struct value_extractor<std::optional<T>> {
        static std::optional<T> extract(const generic_value &v) {
            return !v.is_null() ? std::optional<T>(v.as<T>()) : std::nullopt;
        }
    };

    template <typename T>
    inline T generic_value::as() const {
        using CleanT = remove_cvref_t<T>;
        return static_cast<T>(value_extractor<CleanT>::extract(*this));
    }

    template <typename T>
    inline std::optional<T> generic_value::try_as() const noexcept {
        try {
            return as<T>();
        } catch (...) {
            return std::nullopt;
        }
    }

    template <typename T>
    inline T generic_value::as_or(T fallback) const noexcept {
        try {
            return as<T>();
        } catch (...) {
            return fallback;
        }
    }

    /** Specialized NULL object */
    struct null_object : public generic_value {
        null_object() : generic_value(nullptr) {}
    };
    typedef null_object SQLITE3_ONULL;
}

using SQLITE3_OGEN = cxx_sqlite3::generic_value;

#endif

/**
 * SERIALIZER.HPP
 * 
 * -.- The cxx_sqlite3 library -.-
 * 
 * Defines the ADL serialization mechanics (to_sqlite / from_sqlite)
 * and the non-intrusive macro CXX_SQLITE3_DEFINE_TYPE_NON_INTRUSIVE
 * for seamless mapping between C++ structs/classes and SQLite rows/statements.
 * 
 * @version 1.2.0
 * @author  Yoel Monsalve
 * @date    2026-10-08
 * 
 * (C) Copyright Yoel Monsalve 2025-2026.
 */

#ifndef CXX_SQLITE3_SERIALIZER_HPP_
#define CXX_SQLITE3_SERIALIZER_HPP_

#include <type_traits>
#include <utility>
#include <string>

namespace cxx_sqlite3
{
    class row;
    class statement;

    namespace detail
    {
        // Tag type for ADL dispatch
        template <typename T>
        struct adl_tag {};

        template <typename T>
        auto from_sqlite_adl(adl_tag<T>, const row &r, T &val)
            -> decltype(from_sqlite(r, val))
        {
            return from_sqlite(r, val);
        }

        template <typename T>
        auto to_sqlite_adl(adl_tag<T>, statement &stmt, const T &val)
            -> decltype(to_sqlite(stmt, val))
        {
            return to_sqlite(stmt, val);
        }

        // SFINAE checks for ADL from_sqlite
        template <typename T, typename = void>
        struct has_from_sqlite : std::false_type {};

        template <typename T>
        struct has_from_sqlite<T, std::void_t<
            decltype(from_sqlite_adl(adl_tag<T>{}, std::declval<const row &>(), std::declval<T &>()))
        >> : std::true_type {};

        // SFINAE checks for ADL to_sqlite
        template <typename T, typename = void>
        struct has_to_sqlite : std::false_type {};

        template <typename T>
        struct has_to_sqlite<T, std::void_t<
            decltype(to_sqlite_adl(adl_tag<T>{}, std::declval<statement &>(), std::declval<const T &>()))
        >> : std::true_type {};
    }

    /**
     * adl_serializer
     * Provides customizable default serialization traits.
     */
    template <typename T, typename = void>
    struct adl_serializer {
        static void from_sqlite(const row &r, T &val) {
            detail::from_sqlite_adl(detail::adl_tag<T>{}, r, val);
        }
        static void to_sqlite(statement &stmt, const T &val) {
            detail::to_sqlite_adl(detail::adl_tag<T>{}, stmt, val);
        }
    };
}

// ---------------------------------------------------------------------------
// Variadic Macro Helper for CXX_SQLITE3_DEFINE_TYPE_NON_INTRUSIVE
// Generates:
//   1. from_sqlite(const row &r, Type &val)
//   2. to_sqlite(statement &stmt, const Type &val)
// ---------------------------------------------------------------------------

#define CXX_SQLITE3_INTERNAL_EXPAND(x) x

#define CXX_SQLITE3_INTERNAL_FROM_FIELD(v, field) \
    v.field = r[#field].template as<std::decay_t<decltype(v.field)>>();

#define CXX_SQLITE3_INTERNAL_TO_FIELD(v, field) \
    stmt.bind(std::string(":") + #field, v.field);

// Macro recursion up to 16 struct fields
#define CXX_SQLITE3_FE_0(ACTION, v)
#define CXX_SQLITE3_FE_1(ACTION, v, X) ACTION(v, X)
#define CXX_SQLITE3_FE_2(ACTION, v, X, ...) ACTION(v, X) CXX_SQLITE3_INTERNAL_EXPAND(CXX_SQLITE3_FE_1(ACTION, v, __VA_ARGS__))
#define CXX_SQLITE3_FE_3(ACTION, v, X, ...) ACTION(v, X) CXX_SQLITE3_INTERNAL_EXPAND(CXX_SQLITE3_FE_2(ACTION, v, __VA_ARGS__))
#define CXX_SQLITE3_FE_4(ACTION, v, X, ...) ACTION(v, X) CXX_SQLITE3_INTERNAL_EXPAND(CXX_SQLITE3_FE_3(ACTION, v, __VA_ARGS__))
#define CXX_SQLITE3_FE_5(ACTION, v, X, ...) ACTION(v, X) CXX_SQLITE3_INTERNAL_EXPAND(CXX_SQLITE3_FE_4(ACTION, v, __VA_ARGS__))
#define CXX_SQLITE3_FE_6(ACTION, v, X, ...) ACTION(v, X) CXX_SQLITE3_INTERNAL_EXPAND(CXX_SQLITE3_FE_5(ACTION, v, __VA_ARGS__))
#define CXX_SQLITE3_FE_7(ACTION, v, X, ...) ACTION(v, X) CXX_SQLITE3_INTERNAL_EXPAND(CXX_SQLITE3_FE_6(ACTION, v, __VA_ARGS__))
#define CXX_SQLITE3_FE_8(ACTION, v, X, ...) ACTION(v, X) CXX_SQLITE3_INTERNAL_EXPAND(CXX_SQLITE3_FE_7(ACTION, v, __VA_ARGS__))
#define CXX_SQLITE3_FE_9(ACTION, v, X, ...) ACTION(v, X) CXX_SQLITE3_INTERNAL_EXPAND(CXX_SQLITE3_FE_8(ACTION, v, __VA_ARGS__))
#define CXX_SQLITE3_FE_10(ACTION, v, X, ...) ACTION(v, X) CXX_SQLITE3_INTERNAL_EXPAND(CXX_SQLITE3_FE_9(ACTION, v, __VA_ARGS__))
#define CXX_SQLITE3_FE_11(ACTION, v, X, ...) ACTION(v, X) CXX_SQLITE3_INTERNAL_EXPAND(CXX_SQLITE3_FE_10(ACTION, v, __VA_ARGS__))
#define CXX_SQLITE3_FE_12(ACTION, v, X, ...) ACTION(v, X) CXX_SQLITE3_INTERNAL_EXPAND(CXX_SQLITE3_FE_11(ACTION, v, __VA_ARGS__))
#define CXX_SQLITE3_FE_13(ACTION, v, X, ...) ACTION(v, X) CXX_SQLITE3_INTERNAL_EXPAND(CXX_SQLITE3_FE_12(ACTION, v, __VA_ARGS__))
#define CXX_SQLITE3_FE_14(ACTION, v, X, ...) ACTION(v, X) CXX_SQLITE3_INTERNAL_EXPAND(CXX_SQLITE3_FE_13(ACTION, v, __VA_ARGS__))
#define CXX_SQLITE3_FE_15(ACTION, v, X, ...) ACTION(v, X) CXX_SQLITE3_INTERNAL_EXPAND(CXX_SQLITE3_FE_14(ACTION, v, __VA_ARGS__))
#define CXX_SQLITE3_FE_16(ACTION, v, X, ...) ACTION(v, X) CXX_SQLITE3_INTERNAL_EXPAND(CXX_SQLITE3_FE_15(ACTION, v, __VA_ARGS__))

#define CXX_SQLITE3_GET_MACRO(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,NAME,...) NAME

#define CXX_SQLITE3_FOR_EACH(ACTION, v, ...) \
    CXX_SQLITE3_INTERNAL_EXPAND(CXX_SQLITE3_GET_MACRO(__VA_ARGS__, \
        CXX_SQLITE3_FE_16, CXX_SQLITE3_FE_15, CXX_SQLITE3_FE_14, CXX_SQLITE3_FE_13, \
        CXX_SQLITE3_FE_12, CXX_SQLITE3_FE_11, CXX_SQLITE3_FE_10, CXX_SQLITE3_FE_9,  \
        CXX_SQLITE3_FE_8,  CXX_SQLITE3_FE_7,  CXX_SQLITE3_FE_6,  CXX_SQLITE3_FE_5,  \
        CXX_SQLITE3_FE_4,  CXX_SQLITE3_FE_3,  CXX_SQLITE3_FE_2,  CXX_SQLITE3_FE_1,  \
        CXX_SQLITE3_FE_0)(ACTION, v, __VA_ARGS__))

/**
 * CXX_SQLITE3_DEFINE_TYPE_NON_INTRUSIVE(Type, ...members)
 * Generates ADL to_sqlite and from_sqlite for type Type.
 */
#define CXX_SQLITE3_DEFINE_TYPE_NON_INTRUSIVE(Type, ...) \
    inline void from_sqlite(const ::cxx_sqlite3::row &r, Type &val) { \
        CXX_SQLITE3_FOR_EACH(CXX_SQLITE3_INTERNAL_FROM_FIELD, val, __VA_ARGS__) \
    } \
    inline void to_sqlite(::cxx_sqlite3::statement &stmt, const Type &val) { \
        CXX_SQLITE3_FOR_EACH(CXX_SQLITE3_INTERNAL_TO_FIELD, val, __VA_ARGS__) \
    }

#endif // CXX_SQLITE3_SERIALIZER_HPP_

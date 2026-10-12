/**
 * EXCEPTION.HPP
 * 
 * -.- The cxx_sqlite3 library -.-
 * 
 * Exception classes for cxx_sqlite3.
 * 
 * @version 1.0.0 
 * @author  Yoel Monsalve
 * @date    2023-11-11
 * 
 * (C) Copyright Yoel Monsalve 2025-2026.
 */

#ifndef EXCEPTION_HPP_
#define EXCEPTION_HPP_

#include <exception>
#include <stdexcept>
#include <string>
#include <sqlite3.h>

#include "cpp_version.hpp"
#include "error_code.hpp"

namespace cxx_sqlite3
{
    /** Custom sqlite exception */
    struct exception: public std::runtime_error
    {
        exception(const char *msg) : std::runtime_error(msg) {}
    };

    /** No exception */
    struct noexception: public exception
    {
        noexception() : exception("no exception") {}
    };

    /** exception thrown on prepare_statement_v2() */
    struct generic_exception: public exception
    {
        generic_exception(const char *msg) 
            : exception((std::string{"cxx_sqlite3 error: "} + msg).c_str()) {}
    };
    
    /** exception thrown on prepare_statement_v2() */
    struct exception_prepare: public exception
    {
        exception_prepare(const char *msg) 
            : exception((std::string{"error: prepare_statement: "} + msg).c_str()) {}
    };
    /** exception thrown on bind()_*() familiy functions */
    struct exception_bind: public exception
    {
        exception_bind(const char *msg) 
            : exception((std::string{"error: sqlite3_bind: "} + msg).c_str()) {}
    };
    /** exception thrown on column_*() family functions */
    struct exception_column: public exception
    {
        exception_column(const char *msg) 
            : exception((std::string{"error: sqlite3_column: "} + msg).c_str()) {}
    };
    /** exception thrown on sqlite3_step function */
    struct exception_step: public exception
    {
        exception_step(const char *msg) 
            : exception((std::string{"error: sqlite3_step: "} + msg).c_str()) {}
    };
    /** exception thrown on sqlite3_reset function */
    struct exception_reset: public exception
    {
        exception_reset(const char *msg) 
            : exception((std::string{"error: sqlite3_reset: "} + msg).c_str()) {}
    };
    /** exception thrown on sqlite3_finalize function */
    struct exception_finalize: public exception
    {
        exception_finalize(const char *msg) 
            : exception((std::string{"error: sqlite3_finalize: "} + msg).c_str()) {}
    };
    
    /** exception thrown on `sqlite3_open` function */
    struct exception_open: public exception
    {
        exception_open(const char *msg) 
            : exception((std::string{"error: sqlite3_open: "} + msg).c_str()) {}
    };
    /** exception thrown on `sqlite3_close` function */
    struct exception_close: public exception
    {
        exception_close(const char *msg) 
            : exception((std::string{"error: sqlite3_close: "} + msg).c_str()) {}
    };

    /** Constant exceptions */
    struct const_exception
    {
        inline static const exception ex1 = exception("my exception 1");
        inline static const exception no_exception = exception("no exception");
    };    
    

    /** Generic value exceptions */
    struct exception_value: public exception
    {
        explicit exception_value(const char *msg) 
            : exception((std::string{"cxx_sqlite3 error: value: "} + msg).c_str()) {}
        explicit exception_value(const std::string &msg) 
            : exception((std::string{"cxx_sqlite3 error: value: "} + msg).c_str()) {}
    };

    struct exception_value_bad_cast: public exception_value
    {
    private:
        std::string from_type_;
        std::string to_type_;
        std::string reason_;

    public:
        explicit exception_value_bad_cast(
            std::string from_type = "unknown_type",
            std::string to_type = "unknown_type",
            std::string reason = "unknown_reason")
            : exception_value("cannot cast from '" + from_type + "' to '" + to_type + "': " + reason),
              from_type_(std::move(from_type)),
              to_type_(std::move(to_type)),
              reason_(std::move(reason)) {}

        const std::string &from_type() const noexcept { return from_type_; }
        const std::string &to_type() const noexcept { return to_type_; }
        const std::string &reason() const noexcept { return reason_; }
    };

    /** Backward compatibility aliases */
    using exception_object = exception_value;
    using exception_object_bad_cast = exception_value_bad_cast;
}

typedef cxx_sqlite3::exception sqlite3_exception;

#endif

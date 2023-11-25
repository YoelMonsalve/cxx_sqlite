/**
 * EXCEPTION.HPP
 * 
 * -.- The cxx_sqlite3 library -.-
 * 
 * Exception classes for cxx_sqlite3.
 * 
 * @version 0.1.0 
 * @author  Yoel Monsalve
 * @date    2023-11-11
 * 
 * (C) Copyright VDI Technologies, LLC. 2019-2023.
 * Tulsa, OK.
 */

#ifndef EXCEPTION_HPP_
#define EXCEPTION_HPP_

#include <exception>
#include <stdexcept>
#include <string>
#include <sqlite3.h>

#include "std_version.hpp"

namespace cxx_sqlite3
{
    /** error codes */
    typedef int error_code_t;
    enum error_code : error_code_t {
        ok         = SQLITE_OK        ,
        row        = SQLITE_ROW       ,
        done       = SQLITE_DONE      ,
        error      = SQLITE_ERROR     ,
        internal   = SQLITE_INTERNAL  ,
        perm       = SQLITE_PERM      ,
        abort      = SQLITE_ABORT     ,
        busy       = SQLITE_BUSY      ,
        locked     = SQLITE_LOCKED    ,
        misuse     = SQLITE_MISUSE    ,
        // ...
    };
    using return_code_t = error_code_t;

    // another approach ... (thoughts ?)
    struct error_code2 {
        using value_type                   = error_code_t;

        static const value_type ok         = SQLITE_OK        ;
        static const value_type row        = SQLITE_ROW       ;
        static const value_type done       = SQLITE_DONE      ;
        static const value_type error      = SQLITE_ERROR     ;
        static const value_type internal   = SQLITE_INTERNAL  ;
        static const value_type perm       = SQLITE_PERM      ;
        static const value_type abort      = SQLITE_ABORT     ;
        static const value_type busy       = SQLITE_BUSY      ;
        static const value_type locked     = SQLITE_LOCKED    ;
    };

}

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

    /** TODO: constant exceptions */
    struct const_exception
    {
        static const exception ex1;
        static const exception no_exception;
    };    
    // TODO: more specializations ...
}

typedef cxx_sqlite3::exception sqlite3_exception;

#endif

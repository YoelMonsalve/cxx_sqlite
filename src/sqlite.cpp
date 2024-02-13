/**
 * SQLITE3.CPP
 * 
 * -.- The cxx_sqlite3 library -.-
 * 
 * High level wrapper for using sqlite3 (https://www.sqlite.org/cintro.html) 
 * with modern C++ (c++11 and beyond).
 * 
 * @version 0.1.0 
 * @author  Yoel Monsalve
 * @date    2023-11-11
 * 
 * Copyright (C) VDI Technologies, LLC. 2019-2023.
 * Tulsa, OK..
 */
#include "sqlite.hpp"

/** static definitions */

/** built-in sqlite3 destructors */
cxx_sqlite3::destructor_type cxx_sqlite3::bind_handler::bind_static    = SQLITE_STATIC;
cxx_sqlite3::destructor_type cxx_sqlite3::bind_handler::bind_transient = SQLITE_TRANSIENT;    

/** exceptions */
const cxx_sqlite3::exception cxx_sqlite3::const_exception::ex1 = exception("my exception 1");
const cxx_sqlite3::exception cxx_sqlite3::const_exception::no_exception = exception("no exception");

/** error_category implementation */
const std::error_category &error_category_inst() noexcept {
    static const cxx_sqlite3::error_category ecat{};
    return ecat;
}
namespace cxx_sqlite3
{
    /** construct error_code and error_condition from enumerators */
    
    /* error code from error code enumerator */
    std::error_code make_error_code(error ev) noexcept 
    {
        return std::error_code(
            static_cast<int>(ev),
            error_category_inst()
        );
    }
    /* error code from primitive type error_code_t */
    std::error_code make_error_code(error_code_t ev) noexcept 
    {
        return std::error_code(
            static_cast<int>(ev),
            error_category_inst()
        );
    }
    /* error condition from error code enumerator */
    std::error_condition make_error_condition(error ev) noexcept 
    {
        return std::error_condition(
            static_cast<int>(ev),
            error_category_inst()
        );
    }
    /* error condition from primitive type error_code_t */
    std::error_condition make_error_condition(error_code_t ev) noexcept 
    {
        return std::error_condition(
            static_cast<int>(ev),
            error_category_inst()
        );
    }

    /** comparison between numeric types and ErrorCode */
    bool operator==(const return_code_t rc, const ErrorCode &ec) 
    { return rc == ec.value(); }
    bool operator!=(const return_code_t rc, const ErrorCode &ec) 
    { return rc != ec.value(); }
}
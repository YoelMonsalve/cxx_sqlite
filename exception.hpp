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
 * Tulsa - Oklahoma.
 */

#ifndef EXCEPTION_HPP_
#define EXCEPTION_HPP_

#include "std_version.h"
#include <exception>
#include <stdexcept>

namespace cxx_sqlite3
{
    /** Custom sqlite exception */
    class exception: public std::runtime_error
    {
    public:
        exception(const char *msg) : runtime_error(msg) {}
    };

    // TODO: more specializations ...
}


typedef cxx_sqlite3::exception sqlite3_exception;

#endif
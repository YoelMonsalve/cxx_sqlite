/**
 * BASIC_TYPES.HPP
 * 
 * -.- The cxx_sqlite3 library -.-
 * 
 * High level wrapper for using sqlite3 (https://www.sqlite.org/cintro.html) 
 * with modern C++ (c++11 and beyond).
 * 
 * @version 1.0.0 
 * @author  Yoel Monsalve
 * @date    2023-11-11
 * 
 * (C) Copyright Yoel Monsalve 2025-2026.
 */

#ifndef SQLITE_BASIC_TYPES_HPP_
#define SQLITE_BASIC_TYPES_HPP_

#include <limits>
#include <string>
#include <cstring>
#include <vector>
#include <map>
#include <unordered_map>
#include <tuple>
#include <sqlite3.h>

namespace cxx_sqlite3
{
    using std::string;
    using std::vector;
    using std::tuple;
    
    /** 
     * symbolic constants to identify sqlite3 native data types 
     * (these are integer constants, not types !)
     */
    typedef int data_type_t;
    enum data_type : data_type_t {
        int_t    = SQLITE_INTEGER,
        float_t  = SQLITE_FLOAT,
        double_t = SQLITE_FLOAT,
        text_t   = SQLITE_TEXT,
        blob_val = SQLITE_BLOB,
        null_val = SQLITE_NULL,
    };

    /** importing sqlite3 native objects and types to this space */
    using stmt    = sqlite3_stmt;
    using int64   = sqlite3_int64;
    using uint64  = sqlite3_uint64;
    //
    using destructor_type = sqlite3_destructor_type;
    using col_index_t     = int;
    using col_id_t        = const char *;
    using col_name_t      = col_id_t;
    using connection      = sqlite3 *;    // sqlite3 primitive connection type

    /** encoding methods */
    enum encoding : unsigned char {
        utf8    = SQLITE_UTF8, 
        utf16   = SQLITE_UTF16, 
        utf16be = SQLITE_UTF16BE,
        utf16le = SQLITE_UTF16LE,
    };
}
#endif

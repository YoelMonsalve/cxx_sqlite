/**
 * SQLITE3.HPP
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
 * (C) Copyright VDI Technologies, LLC. 2019-2023.
 * Tulsa - Oklahoma.
 */

#ifndef SQLITE_HPP_
#define SQLITE_HPP_

#include "std_version.h"
#include <string>
#include <vector>
#include <tuple>
#include <sqlite3.h>
#include "object.hpp"
#include "exception.hpp"

//SQLITE_INTEGER, SQLITE_FLOAT, SQLITE_TEXT, SQLITE_BLOB, or SQLITE_NULL.

namespace cxx_sqlite3
{
    using std::string;
    using std::vector;
    using std::tuple;
    
    /** symbolic constants for sqlite3 native data types */
    enum data_type : int {
        type_int    = SQLITE_INTEGER,
        type_float  = SQLITE_FLOAT,
        type_double = SQLITE_FLOAT,
        type_text   = SQLITE_TEXT,
        type_blob   = SQLITE_BLOB,
        type_null   = SQLITE_NULL,
    };

    /** importing sqlite3 native objects and types to this space */
    using sqlite3 = sqlite3;
    using stmt    = sqlite3_stmt;
    using int64   = sqlite3_int64;
    using uint64  = sqlite3_uint64;
    //
    using destructor_type = sqlite3_destructor_type;
    using col_index_t     = int;

    /** encoding methods */
    enum encoding : unsigned char {
        utf8 = SQLITE_UTF8, 
        utf16 = SQLITE_UTF16, 
        utf16be = SQLITE_UTF16BE,
        utf16le = SQLITE_UTF16LE,
    };

    /** bind modes */
    enum class bind_mode {
        bind_static = 0,
        bind_transient,
    };

    struct bind_handler {
        static sqlite3_destructor_type static_handler;
        static sqlite3_destructor_type transient_handler;
    };

    /** Main class */
    class db
    {
        // ... private methods here 

        public:

        /** constructors */
        db() {}

        /** 
         * wrapper for bind methods 
         * https://www.sqlite.org/c3ref/bind_blob.html
        */
        /*
        int sqlite3_bind_blob(sqlite3_stmt*, int, const void*, int n, void(*)(void*));
        int sqlite3_bind_blob64(sqlite3_stmt*, int, const void*, sqlite3_uint64,
                                void(*)(void*));
        int sqlite3_bind_double(sqlite3_stmt*, int, double);
        int sqlite3_bind_int(sqlite3_stmt*, int, int);
        int sqlite3_bind_int64(sqlite3_stmt*, int, sqlite3_int64);
        int sqlite3_bind_null(sqlite3_stmt*, int);
        int sqlite3_bind_text(sqlite3_stmt*,int,const char*,int,void(*)(void*));
        int sqlite3_bind_text16(sqlite3_stmt*, int, const void*, int, void(*)(void*));
        int sqlite3_bind_text64(sqlite3_stmt*, int, const char*, sqlite3_uint64,
                                void(*)(void*), unsigned char encoding);
        int sqlite3_bind_value(sqlite3_stmt*, int, const sqlite3_value*);
        int sqlite3_bind_pointer(sqlite3_stmt*, int, void*, const char*,void(*)(void*));
        int sqlite3_bind_zeroblob(sqlite3_stmt*, int, int n);
        int sqlite3_bind_zeroblob64(sqlite3_stmt*, int, sqlite3_uint64);
        */

        int bind_int(stmt *pstmt, col_index_t iCol, int val) { 
            return sqlite3_bind_int(pstmt, iCol, val); 
            // TODO: .. raise posible exceptions depending on whether sqlite3_bind_*() success or not.
        }
        int bind_int_64(stmt *pstmt, col_index_t iCol, int val) { 
            return sqlite3_bind_int64(pstmt, iCol, val); 
            // TODO: error handling
        }
        int bind_double(stmt *pstmt, col_index_t iCol, int val) { 
            return sqlite3_bind_double(pstmt, iCol, val); 
        }
        int bind_text(stmt *pstmt, col_index_t iCol, const char *text, int len = -1, destructor_type hdl = bind_handler::static_handler) { 
            return sqlite3_bind_text(pstmt, iCol, text, len, hdl); 
        }
        int bind_text64(stmt *pstmt, col_index_t iCol, const char *text, uint64 len = -1, destructor_type hdl = bind_handler::static_handler, unsigned char encoding_ =  encoding::utf8) { 
            return sqlite3_bind_text64(pstmt, iCol, text, len, hdl, encoding_); 
        }
        int bind_blob(stmt *pstmt, col_index_t iCol, const char *text, int len = -1, destructor_type hdl = bind_handler::static_handler) { 
            return sqlite3_bind_blob(pstmt, iCol, text, len, hdl); 
        }
        int bind_blob64(stmt *pstmt, col_index_t iCol, const char *text, uint64 len = -1, destructor_type hdl = bind_handler::static_handler) { 
            return sqlite3_bind_blob64(pstmt, iCol, text, len, hdl); 
        }
        int bind_null(stmt *pstmt, col_index_t iCol) { 
            return sqlite3_bind_null(pstmt, iCol); 
        }

        int bind_parameter_index(stmt *pstmt, const char *zName) {
            return sqlite3_bind_parameter_index(pstmt, zName);
        }
        int bind_parameter_index(stmt *pstmt, const string zName) {
            return sqlite3_bind_parameter_index(pstmt, zName.c_str());
        }

        /** return values */
        const char *column_name(stmt *pstmt, col_index_t iCol) {
            return sqlite3_column_name(pstmt, iCol);
        }
        int colum_bytes(stmt *pstmt, col_index_t iCol) {
            return sqlite3_column_bytes(pstmt, iCol);
        }
        int colum_type(stmt *pstmt, col_index_t iCol) {
            return sqlite3_column_type(pstmt, iCol);
        }

        # if 0
        enum class bind_type {
            bind_int = 0,
            bind_int64,
            bind_double,
            bind_int,
            bind_text,
            bind_text64,
            bind_blob,
            bind_blob64,
            bind_null,
        };
        // this allows to implement a common wrapper for all type of bind_* methods
        typedef int (*generic_binder)(void **);

        /* Provide arguments through an array.
         * example: bind_int__({&pstmt) */
        int bind_int__(void **args) {
            stmt* pstm = (stmt *)
        }
        #endif

        int bind_one(stmt *pstmt, string col_name, generic_object VAL,
            destructor_type hdl = bind_handler::static_handler)
        {
            col_index_t iCol = bind_parameter_index(pstmt, string{":"} + col_name);
            if (iCol <= 0) { /* ... ? */ return -1;}
            std::cout << col_name << ": " << iCol << std::endl;
            //exit(0);
            int ctype = sqlite3_column_type(pstmt, iCol);
            std::cout << "MALDITA SEA: " << ctype << " : " << sqlite3_column_decltype(pstmt, 1) << std::endl;
            exit(0);
            std::cout << data_type::type_int << ", " << data_type::type_double << std::endl;
            std::cout << SQLITE_TEXT << ", " << SQLITE_BLOB << ", " << SQLITE_NULL << std::endl;
            switch(ctype) {
                case data_type::type_int:
                    return bind_int_64(pstmt, iCol, ((SQLITE3_OINT)VAL)());
                case data_type::type_double:
                    return bind_double(pstmt, iCol, ((SQLITE3_ODOUBLE)VAL)());
                case data_type::type_null:
                    return bind_null(pstmt, iCol);
                case data_type::type_text:
                    return bind_text(pstmt, iCol, ((SQLITE3_OSTRING)VAL)().c_str(), -1, hdl);
                case data_type::type_blob:
                    return bind_blob(pstmt, iCol, ((SQLITE3_OSTRING)VAL)().c_str(), -1, hdl);
                default:
                    return SQLITE_ERROR;    // change to a more appropriate SQLITE3 error code
            }
        }
    };

    /** built-in sqlite3 destructors */
    cxx_sqlite3::destructor_type cxx_sqlite3::bind_handler::static_handler    = SQLITE_STATIC;
    cxx_sqlite3::destructor_type cxx_sqlite3::bind_handler::transient_handler = SQLITE_TRANSIENT;
}

#endif
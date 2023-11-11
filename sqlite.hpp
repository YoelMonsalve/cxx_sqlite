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
#include <sqlite3.h>

//SQLITE_INTEGER, SQLITE_FLOAT, SQLITE_TEXT, SQLITE_BLOB, or SQLITE_NULL.

namespace cxx_sqlite3
{
    /** symbolic constants for sqlite3 native data types */
    enum class data_type {
        int_    = SQLITE_INTEGER,
        float_  = SQLITE_FLOAT,
        double_ = SQLITE_FLOAT,
        text    = SQLITE_TEXT,
        blob    = SQLITE_BLOB,
        null_   = SQLITE_NULL,
    };

    /** importing sqlite3 native objects and types to this space */
    using sqlite3 = sqlite3;
    using stmt    = sqlite3_stmt;
    using int64   = sqlite3_int64;
    using uint64  = sqlite3_uint64;
    //
    using destructor_type = sqlite3_destructor_type;

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

        int bind_int(stmt *pstmt, int iCol, int val) { 
            return sqlite3_bind_int(pstmt, iCol, val); 
            // TODO: .. raise posible exceptions depending on whether sqlite3_bind_*() success or not.
        }
        int bind_int_64(stmt *pstmt, int iCol, int val) { 
            return sqlite3_bind_int64(pstmt, iCol, val); 
            // TODO: error handling
        }
        int bind_double(stmt *pstmt, int iCol, int val) { 
            return sqlite3_bind_double(pstmt, iCol, val); 
        }
        int bind_text(stmt *pstmt, int iCol, const char *text, int len = -1, destructor_type hdl = bind_handler::static_handler) { 
            return sqlite3_bind_text(pstmt, iCol, text, len, hdl); 
        }
        int bind_text64(stmt *pstmt, int iCol, const char *text, uint64 len = -1, destructor_type hdl = bind_handler::static_handler, unsigned char encoding_ =  encoding::utf8) { 
            return sqlite3_bind_text64(pstmt, iCol, text, len, hdl, encoding_); 
        }
        int bind_blob(stmt *pstmt, int iCol, const char *text, int len = -1, destructor_type hdl = bind_handler::static_handler) { 
            return sqlite3_bind_blob(pstmt, iCol, text, len, hdl); 
        }
        int bind_blob64(stmt *pstmt, int iCol, const char *text, uint64 len = -1, destructor_type hdl = bind_handler::static_handler) { 
            return sqlite3_bind_blob64(pstmt, iCol, text, len, hdl); 
        }
        int bind_null(stmt *pstmt, int iCol) { 
            return sqlite3_bind_null(pstmt, iCol); 
        }

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
        typedef int (*generic_binder)(bind_type, void **);

        
    };

    /** built-in sqlite3 destructors */
    cxx_sqlite3::destructor_type cxx_sqlite3::bind_handler::static_handler    = SQLITE_STATIC;
    cxx_sqlite3::destructor_type cxx_sqlite3::bind_handler::transient_handler = SQLITE_TRANSIENT;
}

#endif
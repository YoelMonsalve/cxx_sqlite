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
#include <limits>
#include <string>
#include <cstring>
#include <vector>
#include <map>
#include <unordered_map>
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
        static sqlite3_destructor_type bind_static;
        static sqlite3_destructor_type bind_transient;
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
        int bind_int_64(stmt *pstmt, col_index_t iCol, int64_t val) { 
            return sqlite3_bind_int64(pstmt, iCol, val); 
            // TODO: error handling
        }
        int bind_double(stmt *pstmt, col_index_t iCol, double val) { 
            return sqlite3_bind_double(pstmt, iCol, val); 
        }
        int bind_text(stmt *pstmt, col_index_t iCol, const char *text, int len = -1, destructor_type hdl = bind_handler::bind_transient) { 
            return sqlite3_bind_text(pstmt, iCol, text, len, hdl); 
        }
        int bind_text64(stmt *pstmt, col_index_t iCol, const char *text, uint64 len = -1, destructor_type hdl = bind_handler::bind_transient, unsigned char encoding_ =  encoding::utf8) { 
            return sqlite3_bind_text64(pstmt, iCol, text, len, hdl, encoding_); 
        }
        int bind_blob(stmt *pstmt, col_index_t iCol, const void *blob, int len = -1, destructor_type hdl = bind_handler::bind_transient) { 
            return sqlite3_bind_blob(pstmt, iCol, blob, len, hdl); 
        }
        int bind_blob64(stmt *pstmt, col_index_t iCol, const void *blob, uint64 len = -1, destructor_type hdl = bind_handler::bind_transient) { 
            return sqlite3_bind_blob64(pstmt, iCol, blob, len, hdl); 
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

        /* int */
        int bind_one(stmt *pstmt, string col_id, int val)
        {
            col_index_t iCol = bind_parameter_index(pstmt, col_id);
            std::cout << "bind INT\n";
            if (iCol <= 0) { /* ... ? */ return SQLITE_ERROR;}
            return bind_int(pstmt, iCol, val);
        }
        /* int64 */
        int bind_one(stmt *pstmt, string col_id, int64_t val)
        {
            col_index_t iCol = bind_parameter_index(pstmt, col_id);
            std::cout << "bind INT64\n";
            if (iCol <= 0) { /* ... ? */ return SQLITE_ERROR;}
            return bind_int_64(pstmt, iCol, val);
        }
        /* double */
        int bind_one(stmt *pstmt, string col_id, double val)
        {
            col_index_t iCol = bind_parameter_index(pstmt, col_id);
            std::cout << "bind DOUBLE\n";
            if (iCol <= 0) { /* ... ? */ return SQLITE_ERROR;}
            return bind_double(pstmt, iCol, val);
        }
        #if 0    // these methods are ambiguous, and unnecessary
        /* const char * */
        int bind_one1(stmt *pstmt, string col_id, const char * val, destructor_type hdl = bind_handler::bind_transient)
        {
            col_index_t iCol = bind_parameter_index(pstmt, col_id);
            std::cout << "bind const char *\n";
            if (iCol <= 0) { /* ... ? */ return SQLITE_ERROR;}
            return bind_text(pstmt, iCol, val, -1, hdl);
        }
        /* const char * (64) */
        int bind_one2(stmt *pstmt, string col_id, const char *val, destructor_type hdl = bind_handler::bind_transient, encoding encoding_ = encoding::utf8)
        {
            col_index_t iCol = bind_parameter_index(pstmt, col_id);
            std::cout << "bind const char * (64)\n";
            if (iCol <= 0) { /* ... ? */ return SQLITE_ERROR;}
            return bind_text64(pstmt, iCol, val, strlen(val), hdl, encoding_);
        }
        #endif
        /* const string */
        int bind_one(stmt *pstmt, string col_id, const string &val, destructor_type hdl = bind_handler::bind_transient)
        {
            col_index_t iCol = bind_parameter_index(pstmt, col_id);
            std::cout << "bind const string &\n";
            if (iCol <= 0) { /* ... ? */ return SQLITE_ERROR;}
            return bind_text(pstmt, iCol, val.c_str(), val.size(), hdl);
        }
        /* const string (64) */
        int bind_one(stmt *pstmt, string col_id, const string &val, destructor_type hdl, encoding encoding_)
        {
            col_index_t iCol = bind_parameter_index(pstmt, col_id);
            std::cout << "bind const string & (64)\n";
            std::cout << "iCol: " << iCol << std::endl;
            if (iCol <= 0) { /* ... ? */ return SQLITE_ERROR;}
            return bind_text64(pstmt, iCol, val.c_str(), val.size(), hdl, encoding_);
        }
        /* blob(const void *) - Warning: very dangerous! */
        int bind_one(stmt *pstmt, string col_id, const void * val, uint64 n, destructor_type hdl = bind_handler::bind_transient)
        {
            col_index_t iCol = bind_parameter_index(pstmt, col_id);
            std::cout << "bind blob (64)\n";
            if (iCol <= 0) { /* ... ? */ return SQLITE_ERROR;}
            return bind_blob64(pstmt, iCol, val, n, hdl);
        }
        /* null value */
        int bind_one(stmt *pstmt, string col_id)
        {
            col_index_t iCol = bind_parameter_index(pstmt, col_id);
            std::cout << "bind NULL \n";
            if (iCol <= 0) { /* ... ? */ return SQLITE_ERROR;}
            return bind_null(pstmt, iCol);
        }

        /** 
         * This generic template method allows declarations such as:
         * 
         *   db.bind_one(stmt, std::pair<string, int>{":id", 1})
         *   db.bind_one(stmt, std::pair<string, string>{":name", "John"})
         */
        template <typename ColIdType, typename ColValType>
        int bind_one(stmt *pstmt, std::pair<ColIdType, ColValType> pair_)
        {   
            return bind_one(pstmt, pair_.first, pair_.second);
        }

        /**
         * This variadic method allows to pass several values at once.
        */
        #if 1
        int bind_variadic__(stmt *pstmt, vector<string>::size_type i, vector<string> col_ids)
        { return 0; /*nothing*/ }
        #endif
        // ________________ recursive case
        template <typename T, typename... Ts>
        int bind_variadic__(stmt *pstmt, vector<string>::size_type i, vector<string> col_ids, T val, Ts... vals)
        {
            int rc = bind_one(pstmt, col_ids[i], val);
            std::cout << "OK ? " << (rc == SQLITE_OK ? 'Y' : 'N') << std::endl;
            if (rc != error::ok) {
                // error, throw exception here
                return rc;
            }
            if (i < col_ids.size())
                return bind_variadic__(pstmt, i + 1, col_ids, vals...);
            else
                return rc;
        }
        template <typename T, typename... Ts>
        int bind(stmt *pstmt, vector<string> col_ids, T val, Ts... vals)
        {
            return bind_variadic__(pstmt, 0, col_ids, val, vals...);
        }
    };

    /** built-in sqlite3 destructors */
    cxx_sqlite3::destructor_type cxx_sqlite3::bind_handler::bind_static    = SQLITE_STATIC;
    cxx_sqlite3::destructor_type cxx_sqlite3::bind_handler::bind_transient = SQLITE_TRANSIENT;
}

#endif
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
 * Copyright (C) VDI Technologies, LLC. 2019-2023.
 * Tulsa, OK..
 */

#ifndef SQLITE_HPP_
#define SQLITE_HPP_

#include <limits>
#include <string>
#include <cstdio>    /* sometimes, it is easier to call printf() */
#include <cstring>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <vector>
#include <map>
#include <unordered_map>
#include <tuple>
#include <sqlite3.h>

#include "cpp_version.hpp"
#include "basic_types.hpp"
#include "functions.hpp"
#include "object.hpp"
#include "error_code.hpp"
#include "exception.hpp"
#include "throwable.hpp"

using std::cin;
using std::cout;
using std::cerr;
using std::endl;
using std::string;
using std::vector;

#define PRINT_DEBUG_LINE__ std::cerr << "on `" << __FILE__ \
<< ":" << __LINE__ + 1 << "': ";

namespace cxx_sqlite3
{
    /** 
     * open modes 
     * https://www.sqlite.org/c3ref/open.html
     */
    typedef int32_t open_mode_t;
    enum open_mode : open_mode_t
    {
        /* The database is opened in read-only mode. If the database does not 
         * already exist, an error is returned. */
        readonly = SQLITE_OPEN_READONLY,
        /* The database is opened for reading and writing if possible, 
         * or reading only if the file is write protected by the operating system. */
        readwrite = SQLITE_OPEN_READWRITE,
        /* The database file is created if it does not already exist. */
        create = SQLITE_OPEN_CREATE,
        /* The filename can be interpreted as a URI if this flag is set. */
        uri = SQLITE_OPEN_URI,
        /* The database will be opened as an in-memory database. */
        memory = SQLITE_OPEN_MEMORY,
        /* The new database connection will use the "multi-thread" threading mode. */
        nomutex = SQLITE_OPEN_NOMUTEX,
        /* The new database connection will use the "serialized" threading mode. */
        fullmutex = SQLITE_OPEN_FULLMUTEX,
        /* The database is opened shared cache enabled, overriding the default shared 
         * cache setting provided by sqlite3_enable_shared_cache(). */
        sharedcache = SQLITE_OPEN_SHAREDCACHE,
        /* The database is opened shared cache disabled, overriding the default shared 
         * cache setting provided by sqlite3_enable_shared_cache(). */
        privatecache = SQLITE_OPEN_PRIVATECACHE,
        /* The database connection comes up in "extended result code mode". */
        //exrescode = SQLITE_OPEN_EXRESCODE,
        /* The database filename is not allowed to contain a symbolic link */
        nofollow = SQLITE_OPEN_NOFOLLOW,
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

    /** sqlite3 prepared statement */
    class statement
    {
        sqlite3      *conn_  = nullptr;      /* sqlite3 DB connection */
        const char   *pzTail = nullptr;      /* pointer to unused portion */
        sqlite3_stmt *pStmt  = nullptr;      /* underlying sqlite3 primitive statement */
        return_code_t rc_    = int(error::ok);  /* return code of the last operation performed */

    public:
        /** constructor */
        statement() : conn_(nullptr) {}
        statement(sqlite3 *conn) 
            : conn_(conn), pStmt(nullptr), pzTail(nullptr) {}
        ~statement()
        {
            finalize();
            //sqlite3_free(pStmt);    // apparently, sqlite3_finalize()
                                      // actually frees the sqlite3_stmt* pointer
                                      // passed as argument
            pStmt = nullptr;
        }

        /** getter (access) methods */
        sqlite3_stmt *getStmt() const { return pStmt; }

        /** get the more recent return code */
        //return_code_t operator()() const { return rc_; }    // ? not sure if needed
        return_code_t rc() const { return rc_; }

        /** get the ErrorCode yielded by the last operation */
        ErrorCode errorCode() const { return ErrorCode(rc_); }

        /** 
         * prepare statement 
         * https://www.sqlite.org/c3ref/prepare.html 
         */
        ErrorCode prepare(const char *zSql, int nByte = -1)
        {
            rc_ = sqlite3_prepare_v2(
                conn_,
                zSql,
                nByte,
                &pStmt,
                &pzTail
            );
            if (rc_ != int(error::ok)) {
		        throw exception_prepare(errmsg());
	        }
            return rc_;    // converts enumerable into ErrorCode
        }
        /** c++ string version, throws exception */
        ErrorCode prepare(const string sql)
        {
            return prepare(sql.c_str());
        }
        /** c++ string version, with ErrorCode */
        statement &prepare(const string sql, ErrorCode &ec) noexcept
        {
            rc_ = sqlite3_prepare_v2(
                conn_,
                sql.c_str(),
                -1,
                &pStmt,
                &pzTail
            );
            ec  = ErrorCode(rc_);
            return *this;    // returns self-reference
        }

        const char *errmsg() const 
        {
            return sqlite3_errmsg(conn_);
        }

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

        ErrorCode bind_int(col_index_t iCol, int val) { 
            TRY_OR_THROW__ (sqlite3_bind_int, (pStmt, iCol, val), 
                rc_, int(error::ok), exception_bind(errmsg())); 
            return rc_;
        }
        ErrorCode bind_int_64(col_index_t iCol, int64_t val) { 
            TRY_OR_THROW__ (sqlite3_bind_int64, (pStmt, iCol, val), 
                rc_, int(error::ok), exception_bind(errmsg())
            ); 
            return rc_;
        }
        ErrorCode bind_double(col_index_t iCol, double val) { 
            TRY_OR_THROW__ (sqlite3_bind_double, (pStmt, iCol, val), 
                rc_, int(error::ok), exception_bind(errmsg())
            ); 
            return rc_;
        }
        ErrorCode bind_text(col_index_t iCol, const char *text, int len = -1, destructor_type hdl = bind_handler::bind_transient) { 
            TRY_OR_THROW__ (sqlite3_bind_text, (pStmt, iCol, text, len, hdl), 
                rc_, int(error::ok), exception_bind(errmsg())
            ); 
            return rc_;
        }
        ErrorCode bind_text64(col_index_t iCol, const char *text, uint64 len = -1, destructor_type hdl = bind_handler::bind_transient, unsigned char encoding_ =  encoding::utf8) { 
            TRY_OR_THROW__ (sqlite3_bind_text64,(pStmt, iCol, text, len, hdl, encoding_), 
                rc_, int(error::ok), exception_bind(errmsg())
            ); 
            return rc_;
        }
        ErrorCode bind_blob(col_index_t iCol, const void *blob, int len = -1, destructor_type hdl = bind_handler::bind_transient) { 
            TRY_OR_THROW__ (sqlite3_bind_blob, (pStmt, iCol, blob, len, hdl), 
                rc_, int(error::ok), exception_bind(errmsg())
            ); 
            return rc_;
        }
        ErrorCode bind_blob64(col_index_t iCol, const void *blob, uint64 len = -1, destructor_type hdl = bind_handler::bind_transient) { 
            TRY_OR_THROW__ (sqlite3_bind_blob64, (pStmt, iCol, blob, len, hdl), 
                rc_, int(error::ok), exception_bind(errmsg())
            ); 
            return rc_;
        }
        ErrorCode bind_null(col_index_t iCol) { 
            TRY_OR_THROW__ (sqlite3_bind_null, (pStmt, iCol), 
                rc_, int(error::ok), exception_bind(errmsg())
            ); 
            return rc_;
        }

        col_index_t bind_parameter_index(const char *zName) {
            int iCol = sqlite3_bind_parameter_index(pStmt, zName);
            if (iCol <= 0) throw exception_bind(errmsg());
            
            return (col_index_t )iCol;
        }
        col_index_t bind_parameter_index(const string zName) {
            return bind_parameter_index(zName.c_str()); 
        }

        /** return values */
        col_index_t colum_count(stmt *pstmt) noexcept 
        {
            return sqlite3_column_count(pStmt);
        }
        const char *column_name(stmt *pstmt, col_index_t iCol) 
        {
            const char *col_name;

            if ((col_name = sqlite3_column_name(pStmt, iCol)) == nullptr) 
                throw exception_bind(errmsg());

            return col_name;
        }
        int colum_bytes(stmt *pstmt, col_index_t iCol) 
        {
            return sqlite3_column_bytes(pStmt, iCol);
        }
        int column_type(stmt *pstmt, col_index_t iCol) 
        {
            return sqlite3_column_type(pStmt, iCol);
        }

        /* int */
        ErrorCode bind_one(col_id_t col_id, int val)
        {
            col_index_t iCol = bind_parameter_index(col_id);
            //std::cout << "bind INT\n";
            return bind_int(iCol, val);
        }
        /* int64 */
        ErrorCode bind_one(col_id_t col_id, int64_t val)
        {
            col_index_t iCol = bind_parameter_index(col_id);
            //std::cout << "bind INT64\n";
            return bind_int_64(iCol, val);
        }
        /* double */
        ErrorCode bind_one(col_id_t col_id, double val)
        {
            col_index_t iCol = bind_parameter_index(col_id);
            //std::cout << "bind DOUBLE\n";
            return bind_double(iCol, val);
        }
        /* const string */
        ErrorCode bind_one(col_id_t col_id, const string &val, destructor_type hdl = bind_handler::bind_transient)
        {
            col_index_t iCol = bind_parameter_index(col_id);
            //std::cout << "bind const string &, index: " << iCol << "\n";
            return bind_text(iCol, val.c_str(), val.size(), hdl);
        }
        /* const string (64) */
        ErrorCode bind_one(col_id_t col_id, const string &val, destructor_type hdl, encoding encoding_)
        {
            col_index_t iCol = bind_parameter_index(col_id);
            //std::cout << "bind const string & (64)\n";
            return bind_text64(iCol, val.c_str(), val.size(), hdl, encoding_);
        }
        /* blob(const void *) - Warning: very dangerous! */
        ErrorCode bind_one(col_id_t col_id, const void * val, uint64 n, destructor_type hdl = bind_handler::bind_transient)
        {
            col_index_t iCol = bind_parameter_index(col_id);
            //std::cout << "bind blob (64)\n";
            return bind_blob64(iCol, val, n, hdl);
        }
        /* null value */
        ErrorCode bind_one(col_id_t col_id)
        {
            col_index_t iCol = bind_parameter_index(col_id);
            //std::cout << "bind NULL\n";
            return bind_null(iCol);
        }

        /** 
         * This generic template method allows declarations such as:
         * 
         *   db.bind_one(stmt, std::pair<string, int>{":id", 1})
         *   db.bind_one(stmt, std::pair<string, string>{":name", "John"})
         */
        template <class ColIdType, class ColValType>
        ErrorCode bind_one(std::pair<ColIdType, ColValType> pair_)
        {   
            return bind_one(pStmt, pair_.first, pair_.second);
        }

        /**
         * This variadic method allows to pass several values at once.
        */
        ErrorCode bind_variadic__(vector<string>::size_type i, vector<col_id_t> col_ids)
        { 
            // we reached the end of processing
            return error::ok; 
        }
        // ________________ recursive case
        template <class T, class... Ts>
        ErrorCode bind_variadic__(vector<string>::size_type i, vector<col_id_t> col_ids, T val, Ts... vals)
        {
            rc_ = bind_one(col_ids[i], val);
            if (rc_ != (int)error::ok) {
                // error, throw exception here
                throw exception_bind(errmsg());
            }
            if (i < col_ids.size())
                return bind_variadic__(i + 1, col_ids, vals...);
            else
                return rc_;    // <-- make an error_code from
        }
        // generic bind method
        template <class T, class... Ts>
        ErrorCode bind(vector<col_id_t> col_ids, T val, Ts... vals)
        {
            return bind_variadic__(0, col_ids, val, vals...);
        }
        
        // second signature: bind(col_id_1, val_1, col_id_2, val_2, ...)
        ErrorCode bind_variadic_2__()  // base case
        { 
            // we reached the end of processing
            return error::ok; 
        }
        template <class T, class... VarTypes>
        ErrorCode bind_variadic_2__(col_id_t col_1, T val_1, VarTypes... args)
        {
            rc_ = bind_one(col_1, val_1);
            if (rc_ != (int)error::ok) {
                // error, throw exception here
                throw exception_bind(errmsg());
            }
            return bind_variadic_2__(args...);
        }
        template <class T, class... VarTypes>
        ErrorCode bind(const char *col_1, T val_1, VarTypes... args)
        {
            return bind_variadic_2__(col_1, val_1, args...);
        }   
        
        // TODO ........
        typedef vector<generic_object> generic_col_vals;
        template <class T>
        ErrorCode bind(vector<col_id_t> col_ids, generic_col_vals vals)
        {
            for (vector<string>::size_type i = 0; i < col_ids.size(); i++) {
                // TODO ........ (!)
                //rc_ = bind_one(pStmt, col_ids[i], vals[i]);
                if (rc_ != (int)(error::ok)) {
                    // should throw an exception
                    return rc_;
                }
            }
            return (int)error::ok;
        }

        /** statement step - C style, throws exception */
        ErrorCode step() const 
        {
            ErrorCode ec;
            /* This method is expected to return SQLITE_DONE, or SQLITE_ROW 
             * on success. On other cases (SQLITE_BUSY, ...), throw exception */
            step(ec);
            if (ec != error::done && ec != error::row) {
                throw exception_step(errmsg());
            }
            return ec;
        }
        /* second signature, based on error_code's nomenclature */
        statement & step(ErrorCode &ec) noexcept 
        {
            /* this is the nothrow, `error_code` based version */
            ec = sqlite3_step(pStmt);
            return *this;
        }
        const statement & step(ErrorCode &ec) const noexcept 
        {
            /* this is the nothrow, `error_code` based version */
            ec = sqlite3_step(pStmt);
            return *this;
        }

        /** statement reset - C style */
        ErrorCode reset()
        {
            /* method is expected to only return SQLITE_OK on success.
             * Otherwise, throw exception */
            rc_ = sqlite3_reset(pStmt);
            if (rc_ != int(error::ok)) {
                throw exception_reset(errmsg());
            }
            return rc_;
        }
        /* noexcept version, error_code based */
        statement & reset(ErrorCode ec) noexcept 
        {
            ec = sqlite3_reset(pStmt);
            return *this;
        }
        /* noexcept const version, error_code based */
        const statement & reset(ErrorCode ec) const noexcept 
        {
            ec = sqlite3_reset(pStmt);
            return *this;
        }

        /** 
         * statement finalize 
         * https://www.sqlite.org/c3ref/finalize.html
         * */
        ErrorCode finalize() noexcept
        {
            /* If the most recent evaluation of the statement encountered 
             * no errors or if the statement is never been evaluated, 
             * then sqlite3_finalize() returns SQLITE_OK. 
             * If the most recent evaluation of statement `S` failed, 
             * then sqlite3_finalize(S) returns the appropriate error code 
             * or extended error code. */
            if (pStmt != nullptr)
            {
                rc_ = sqlite3_finalize(pStmt);
                pStmt = nullptr;
            }
            return rc_;
        }

        /**
         * Submit a query for execution. This is similar to the `mysqli::query()` method
         * of the PHP MySQLi library (https://www.php.net/manual/en/mysqli.query.php).
         * Version with arguments.
         */
        template <class... Ts>
        ErrorCode query(const string sql, 
            vector<col_id_t> col_ids, Ts... vals) // throws ...
        {
            ErrorCode ec;
            ec = prepare(sql);
            ec = bind(col_ids, vals...);
            return ec;
        }
        /** Version with no arguments. */
        ErrorCode query(const string sql) // throws ...
        {
            ErrorCode ec;
            ec = prepare(sql);
            return ec;
        }
        /**
         * Fetch the next row of the result set associated with the more
         * recent query.
         */
        // typedef or using ? decide one ....
        typedef col_name_t columnName;
        typedef generic_object columnValue;
        typedef std::pair<columnName, columnValue> field;
        typedef field column;        // field = column
        typedef std::map<columnName, columnValue> row;
        typedef row record;          // row = record
        typedef std::vector<record> recordSet;
        typedef recordSet result;    // result = recordSet
        row fetch_row(ErrorCode &ec)      // throws ...
        {
            row r{};
            col_index_t j, n_cols;
            const char *col_name;
            data_type_t col_type;
            int64_t     intval;
            const unsigned char *textval;
            double      doubleval;
            const void *blobval;

            step(ec);
            cout << "step returned: " << ec << "\n";
            if ( ec != error::row ) { 
                reset(ec); 
                return r;
            }

            n_cols = colum_count(pStmt);
            for (j = 0; j < n_cols; j++) {
                col_type = column_type(pStmt, j);    // column type
                col_name = column_name(pStmt, j);    // column name
                switch(col_type) {
                case data_type::int_t:
                    intval = sqlite3_column_int64(pStmt, j);
                    printf("%16s    (int):    %ld\n", col_name, intval);
                    r.insert(field{col_name, SQLITE3_OINT(intval)});
                    break;
                case data_type::float_t:
                    doubleval = sqlite3_column_double(pStmt, j);
                    printf("%16s (double):    %lf\n", col_name, doubleval);
                    r.insert(field{col_name, SQLITE3_ODOUBLE(doubleval)});
                    break;    
                case data_type::text_t:
                    textval = sqlite3_column_text(pStmt, j);
                    printf("%16s   (text):    %s\n", col_name, textval);
                    r.insert(field{col_name, SQLITE3_OSTRING((const char *)textval)});
                    break;
                case data_type::blob_t:
                    blobval = sqlite3_column_blob(pStmt, j);
                    // TODO ...
                    printf("%16s   (blob):    BLOB\n", col_name);
                    //printf("%s:    %s\n", col_name, textval);
                    r.insert(field{col_name, SQLITE3_OGEN((void *)blobval)});
                    break;
                case data_type::null_t:
                    // TODO ...
                    // How do I cast the internal value stored in BLOB back to
                    // the original data ?
                    printf("%16s:             NULL\n", col_name);
                    r.insert(field{col_name, SQLITE3_OGEN((void *)blobval)});
                    break;
                default:
                    printf("%16s:             UNDEFINED\n", col_name);
                    ;
                }
            }

            reset(ec);
            return r;
        }


        /**
         * wrapper for execute: prepare + step + reset + finalize
         */
        template <class... Ts>
        void execute(const char *sql, 
            vector<col_id_t> col_ids, Ts... vals) 
        {
            exception     *e_ = nullptr;
            try {
                /* discarding return values, just throw exception, if any */
                prepare(sql);
                bind(col_ids, vals...);
                step();
                reset();
            } catch (exception &e) {
                e_ = &e;
                //std::cerr << "exception detected: " << e.what() << std::endl;
            }
            finalize();
            if (e_ != nullptr) {
                // propagate exception
                throw e_;
            }
        }
        /** C++ string version, bind arguments */
        template <class... Ts>
        void execute(const string &sql, 
            vector<col_id_t> col_ids, Ts... vals)
        { execute(sql.c_str(), col_ids, vals...); }

        /** C const char * version, no arguments */
        void execute(const char *sql) 
        {
            exception     *e_ = nullptr;
            try {
                /* discarding return values, just throw exception, if any */
                prepare(sql);
                step();
                reset();
            } catch (exception &e) {
                e_ = &e;
                //std::cerr << "exception detected: " << e.what() << std::endl;
            }
            finalize();
            if (e_ != nullptr) {
                // propagate exception
                throw e_;
            }
        }
        /** C++ string version, no arguments */
        inline void execute(const string &sql)
        { execute(sql.c_str()); }        
    };

    /** Main class */
    class database
    {

        // ... private methods here 
    private:
        const char    *filename_;
        char          *errMsg_        = nullptr;
        // ... protected methods here
    protected:        
        sqlite3   *conn;    // internal sqlite3 connection
        statement *stmt;    // pointer to internal statement

    public:
        /** constructors */
        database(const char *filename, 
            int mode = open_mode::readwrite | open_mode::create
                       | open_mode::nomutex | open_mode::nofollow)
            : conn(nullptr), stmt(nullptr), 
              filename_(filename), errMsg_(nullptr)
        {
            open(filename, mode);
        }
        database(const string &filename) : database(filename.c_str()) {}

        /** starts a new connection to the given database filename */
        void open(const char *filename, int mode)
        {   
            if (!filename || !*filename) 
                throw exception_open("filename is null or empty.");

            return_code_t rc = sqlite3_open_v2(
                filename,      // datafile name
                &conn,         // sqlite3 object 
                mode,          // open flags
                NULL           // zVfs
            );
            if (rc != int(error::ok)) {
                throw exception_open(sqlite3_errstr(rc));
            }
            /* initialize a connection objet */
            stmt = new statement(conn);
        }

        /** alias for open */
        inline void connect(const char *filename, int mode)
        { open(filename, mode); }

        // TODO: Rule of Three: copy constructor, move constructor, swap, assignment
        //       operator

        /** message associated to the more recent error */
        const char * errmsg() const noexcept
        {
            return stmt->errmsg();
        }

        /** prepare internal statement */
        database &prepare(const char *zSql, int nByte = -1)
        {
            if (stmt == nullptr)
                throw exception("in db::prepare: statement object is null");
            stmt->prepare(zSql, nByte);
            return *this;
        }

        /** 
         * get access to internal more recent prepared statement
         * NOTE: this is dangerous and discouraged, prefer `db::execute` */
        statement *getStatement() const
        { return stmt; }

        /** bind values */
        template <class... Ts>
        database &bind(vector<col_id_t> cols, Ts... vals)
        {
            stmt->bind(cols, vals...);
            return *this;
        }
        // signature for bind(col_id_1, val_1, col_id_2, val_2, ...)
        template <class T, class... ArgTypes>
        database &bind(const char *col_1, T val_1, ArgTypes... args)
        {
            stmt->bind(col_1, val_1, args...);
            return *this;
        }
        
        /** step */
        ErrorCode step() const 
        { return stmt->step(); }

        database & step(ErrorCode &ec) noexcept 
        { 
            stmt->step(ec);
            return *this;
        }

        /** reset */
        ErrorCode reset() const { return stmt->reset(); }

        /** close actual connection */
        ErrorCode close()
        {
            return_code_t rc;
            // finalize any unfinalized statements
            stmt_finalize();
            // close current connection
            rc = sqlite3_close(conn);
            if (rc != int(error::ok)) {
                //PRINT_DEBUG_LINE__
                throw exception_close(sqlite3_errstr(rc));
            } else {
                //sqlite3_free(conn);    // apparently, sqlite3_close()
                                         // actually frees the sqlite3* pointer
                                         // passed as argument
                conn = nullptr;
                return rc;
            }
        }

        /** finalize internal statement */
        void stmt_finalize() 
        { 
            if (stmt != nullptr)
            {
                delete stmt;    // will call sqlite3_finalize()
                stmt = nullptr;
            }
        }

        /** 
         * Get direct access to the internal connection object .
         * Be cautious when using this (!) */
        connection getConnection() const {
            return conn;
        }

        template <class... Ts>
        void execute(const char *sql, 
            vector<col_id_t> col_ids, Ts... vals) 
        {
            stmt->execute(sql, col_ids, vals...);
        }
        template <class... Ts>
        void execute(const string &sql, 
            vector<col_id_t> col_ids, Ts... vals) 
        {
            stmt->execute(sql, col_ids, vals...);
        }

        void execute(const char *sql) 
        {
            stmt->execute(sql);
        }
        void execute(const string &sql) 
        {
            stmt->execute(sql);
        }

        /**
         * Prints out the result of the query, in vertical format.
         */
        ErrorCode command(const char *sql)
        {
            ErrorCode ec;
            errMsg_ = nullptr;

            ec = sqlite3_exec(conn, sql, &printCallback_Vertical, (void *)nullptr, &errMsg_);
            if (errMsg_) {
                fprintf(stderr, "error: %s\n", errMsg_);
            }
            free(errMsg_);
            return ec;
        }
        inline ErrorCode command(const string &sql)
        { return command(sql.c_str()); }

        /**
         * Prints out the result of the query, in tabular format.
         */
        ErrorCode command2(const char *sql) 
        {
            ErrorCode ec;
            char **result;
            int nRow, nCol;
            char *errMsg;

            ec = sqlite3_get_table(
                conn, sql, &result, &nRow, &nCol, &errMsg
            );
            
            if (nRow > 0) {
                printTable(nRow, nCol, result);
                cout << nRow << " rows in set" << endl << endl;
            } else {
                cout << "Empty set" << endl << endl;
            }

            sqlite3_free_table(result);
            if (errMsg) {
                fprintf(stderr, "error: %s\n", errMsg);
            }

            return ec;
        }
        inline ErrorCode command2(const string &sql)
        { return command2(sql.c_str()); }

        /** Opens an interactive prompt */
        void prompt(const string &options = "", const string &query = "")
        {
            std::ostringstream cmd;
            cmd << "sqlite3 " << options << ' ' << filename_;
            if (query != "") cmd << ' ' << query;
            cout << cmd.str() << endl;
            system(cmd.str().c_str());
        }
        /** Opens an interactive prompt (manual version, NOT recommended)*/
        void prompt2()
        {
            string sql;
            const char   UP_ARROW[] = "^[[A";
            const char DOWN_ARROW[] = "^[[B";

            while (1) {
                cout << "SQLite [" << filename_ << "]> ";
                std::getline(cin, sql);

                if (sql == "" || sql == "exit" || sql == "exit;"
                    || sql == "exit\\G") break;
                // execute command
                size_t sql_len = sql.size();
                if (sql.substr(sql_len - 1) == ";") {
                    // tabular style
                    command2(sql);
                } else if (sql.substr(sql_len - 2) == "\\G") {
                    // vertical style
                    command(sql.substr(0, sql_len - 2));
                } else {
                    // tabular style
                    cout << "??? Unterminated line: " << sql << endl << endl;
                }
            }
        }

        /** get autocommit */
        int autocommit() const 
        { return sqlite3_get_autocommit(conn); }

        /** destructor */
        ~database() {
            std::cout << "db::~db() called\n";
            close();        // close connection
        }

    private:
        /** Utility callback for command (print a record vertically)*/
        static int printCallback_Vertical(void *data, int Cols, char **ColValues, char **ColNames) 
        { 
            col_index_t i; 
            //fprintf(stderr, "%s: ", (const char*)data); 

            for (i = 0; i < Cols; i++) { 
                printf("%s = %s\n", ColNames[i], ColValues[i] ? ColValues[i] : "NULL"); 
            }
            printf("\n"); 
            return 0; 
        } 

        /** auxiliary function by printTable */
        void printTableSep(std::vector<int> &colWidth)
        {
            cout << '+';
            for (size_t j = 1; j <= colWidth.size(); j++) {
                cout << string(colWidth[j-1] + 2, '-') << '+';
            }
            cout << endl;
        }

        void printTable(int nRow, int nCol, char **result)
        {
            std::vector<int> colWidth;   
            int i, j, width;
            char *nptr;
            /** 
             * calculate the max width per column level:
             * 
             *  colWidth[j] =     max    {result[i*nCol + j - 1]}
             *                 1<=i<=nRow
             * 
             * for j = 1, ..., nCol.
             */
            colWidth.resize(nCol);
            for (j = 1; j <= nCol; j++) {
                colWidth[j-1] = 0;
                for (i = 0; i <= nRow; i++) {
                    nptr = result[i*nCol + j - 1];
                    width = (nptr != nullptr ? strlen(nptr) : 0);
                    if (width > colWidth[j-1]) colWidth[j-1] = width;
                }
            }
            
            /* print line */
            printTableSep(colWidth);

            /* print column names */
            cout << '|';
            for (j=1; j<=nCol; j++) {
                cout << ' ' << std::left << std::setw(colWidth[j-1]) << result[j-1] << " |";
            }
            cout << endl;

            /* print line*/
            printTableSep(colWidth);

            /* print content */
            for (i = 1; i <= nRow; i++) {
                cout << '|';
                for (j=1; j<=nCol; j++) {
                    nptr = result[i*nCol + j - 1];
                    cout << ' ' << std::left << std::setw(colWidth[j-1]) 
                        << (nptr != nullptr ? nptr : "") << " |";
                }
                cout << endl;
            }

            /* print line */
            printTableSep(colWidth);
        }
    };
}

#endif

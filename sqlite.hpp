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

#include "std_version.hpp"
#include "basic_types.hpp"
#include "functions.hpp"
#include "object.hpp"
#include "exception.hpp"
#include "throwable.hpp"

using std::cin;
using std::cout;
using std::cerr;
using std::endl;
using std::string;
using std::vector;

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
        error_code_t  rc_ = error_code::ok;  /* return code of the last operation */

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

        /** overloading operator() to the more recent return code */
        error_code_t operator()() const { return rc_; }
        error_code_t rc() const { return rc_; }

        /** 
         * prepare statement 
         * https://www.sqlite.org/c3ref/prepare.html 
         */
        statement &prepare(const char *zSql, int nByte = -1)
        {
            rc_ = sqlite3_prepare_v2(
                conn_,
                zSql,
                nByte,
                &pStmt,
                &pzTail
            );
            if (rc_ != error_code::ok) {
		        throw exception_prepare(errmsg());
	        }
            return *this;
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

        error_code_t bind_int(col_index_t iCol, int val) { 
            error_code_t rc;

            // Method 2
            TRY_OR_THROW__ (sqlite3_bind_int, (pStmt, iCol, val), 
                rc, error_code::ok, exception_bind(errmsg())); 
                //rc, error_code::error, exception_bind(sqlite3_errstr(rc))); 
            
            return rc;
        }
        error_code_t bind_int_64( col_index_t iCol, int64_t val) { 
            error_code_t rc;

            TRY_OR_THROW__ (sqlite3_bind_int64, (pStmt, iCol, val), 
                rc, error_code::ok, exception_bind(errmsg())
            ); 
            
            return rc;
        }
        error_code_t bind_double(col_index_t iCol, double val) { 
            error_code_t rc;

            TRY_OR_THROW__ (sqlite3_bind_double, (pStmt, iCol, val), 
                rc, error_code::ok, exception_bind(errmsg())
            ); 
            
            return rc;
        }
        error_code_t bind_text(col_index_t iCol, const char *text, int len = -1, destructor_type hdl = bind_handler::bind_transient) { 
            error_code_t rc;

            TRY_OR_THROW__ (sqlite3_bind_text, (pStmt, iCol, text, len, hdl), 
                rc, error_code::ok, exception_bind(errmsg())
            ); 
            
            return rc;
        }
        error_code_t bind_text64(col_index_t iCol, const char *text, uint64 len = -1, destructor_type hdl = bind_handler::bind_transient, unsigned char encoding_ =  encoding::utf8) { 
            error_code_t rc;

            TRY_OR_THROW__ (sqlite3_bind_text64,(pStmt, iCol, text, len, hdl, encoding_), 
                rc, error_code::ok, exception_bind(errmsg())
            ); 
            
            return rc;
        }
        error_code_t bind_blob(col_index_t iCol, const void *blob, int len = -1, destructor_type hdl = bind_handler::bind_transient) { 
            error_code_t rc;

            TRY_OR_THROW__ (sqlite3_bind_blob, (pStmt, iCol, blob, len, hdl), 
                rc, error_code::ok, exception_bind(errmsg())
            ); 
            
            return rc;
        }
        error_code_t bind_blob64(col_index_t iCol, const void *blob, uint64 len = -1, destructor_type hdl = bind_handler::bind_transient) { 
            error_code_t rc;

            TRY_OR_THROW__ (sqlite3_bind_blob64, (pStmt, iCol, blob, len, hdl), 
                rc, error_code::ok, exception_bind(errmsg())
            ); 
            
            return rc;
        }
        error_code_t bind_null(col_index_t iCol) { 
            error_code_t rc;

            TRY_OR_THROW__ (sqlite3_bind_null, (pStmt, iCol), 
                rc, error_code::ok, exception_bind(errmsg())
            ); 
            
            return rc;
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
        const char *column_name(stmt *pstmt, col_index_t iCol) {
            const char *col_name;

            if ((col_name = sqlite3_column_name(pStmt, iCol)) == nullptr) 
                throw exception_bind(errmsg());
            
            return col_name;
        }
        error_code_t colum_bytes(stmt *pstmt, col_index_t iCol) {
            error_code_t rc;

            TRY_OR_THROW__ (sqlite3_column_bytes, (pStmt, iCol),
                rc, error_code::ok, exception_column(errmsg())
            ); 
            
            return rc;
        }
        error_code_t colum_type(stmt *pstmt, col_index_t iCol) {
            error_code_t rc;

            TRY_OR_THROW__ (sqlite3_column_type, (pStmt, iCol), 
                rc, error_code::ok, exception_column(errmsg())
            ); 
            
            return rc;
        }

        /* int */
        error_code_t bind_one(string col_id, int val)
        {
            col_index_t iCol = bind_parameter_index(col_id);
            //std::cout << "bind INT\n";
            if (iCol <= 0) { /* ... ? */ return error_code::error;}
            return bind_int(iCol, val);
        }
        /* int64 */
        error_code_t bind_one(string col_id, int64_t val)
        {
            col_index_t iCol = bind_parameter_index(col_id);
            //std::cout << "bind INT64\n";
            if (iCol <= 0) { /* ... ? */ return error_code::error;}
            return bind_int_64(iCol, val);
        }
        /* double */
        error_code_t bind_one(string col_id, double val)
        {
            col_index_t iCol = bind_parameter_index(col_id);
            //std::cout << "bind DOUBLE\n";
            if (iCol <= 0) { /* ... ? */ return error_code::error;}
            return bind_double(iCol, val);
        }
        /* const string */
        error_code_t bind_one(string col_id, const string &val, destructor_type hdl = bind_handler::bind_transient)
        {
            col_index_t iCol = bind_parameter_index(col_id);
            //std::cout << "bind const string &\n";
            if (iCol <= 0) { /* ... ? */ return error_code::error;}
            return bind_text(iCol, val.c_str(), val.size(), hdl);
        }
        /* const string (64) */
        error_code_t bind_one(string col_id, const string &val, destructor_type hdl, encoding encoding_)
        {
            col_index_t iCol = bind_parameter_index(col_id);
            //std::cout << "bind const string & (64)\n";
            //std::cout << "iCol: " << iCol << std::endl;
            if (iCol <= 0) { /* ... ? */ return error_code::error;}
            return bind_text64(iCol, val.c_str(), val.size(), hdl, encoding_);
        }
        /* blob(const void *) - Warning: very dangerous! */
        error_code_t bind_one(string col_id, const void * val, uint64 n, destructor_type hdl = bind_handler::bind_transient)
        {
            col_index_t iCol = bind_parameter_index(col_id);
            //std::cout << "bind blob (64)\n";
            if (iCol <= 0) { /* ... ? */ return error_code::error;}
            return bind_blob64(iCol, val, n, hdl);
        }
        /* null value */
        error_code_t bind_one(string col_id)
        {
            col_index_t iCol = bind_parameter_index(col_id);
            //std::cout << "bind NULL \n";
            if (iCol <= 0) { /* ... ? */ return error_code::error;}
            return bind_null(iCol);
        }

        /** 
         * This generic template method allows declarations such as:
         * 
         *   db.bind_one(stmt, std::pair<string, int>{":id", 1})
         *   db.bind_one(stmt, std::pair<string, string>{":name", "John"})
         */
        template <class ColIdType, class ColValType>
        error_code_t bind_one(std::pair<ColIdType, ColValType> pair_)
        {   
            return bind_one(pStmt, pair_.first, pair_.second);
        }

        /**
         * This variadic method allows to pass several values at once.
        */
        error_code_t bind_variadic__(vector<string>::size_type i, vector<string> col_ids)
        { 
            // we reached the end of processing
            return error_code::ok; 
        }
        // ________________ recursive case
        template <class T, class... Ts>
        error_code_t bind_variadic__(vector<string>::size_type i, vector<string> col_ids, T val, Ts... vals)
        {
            error_code_t rc = bind_one(col_ids[i], val);
            //std::cout << "OK ? " << (rc == SQLITE_OK ? 'Y' : 'N') << std::endl;
            if (rc != error_code::ok) {
                // error, throw exception here
                return rc;
            }
            if (i < col_ids.size())
                return bind_variadic__(i + 1, col_ids, vals...);
            else
                return rc;
        }
        // main bind method
        template <class T, class... Ts>
        error_code_t bind(vector<string> col_ids, T val, Ts... vals)
        {
            return bind_variadic__(0, col_ids, val, vals...);
        }

        typedef vector<generic_object> generic_col_vals;
        template <class T>
        error_code_t bind(vector<string> col_ids, generic_col_vals vals)
        {
            error_code_t rc;
            for (vector<string>::size_type i = 0; i < col_ids.size(); i++) {
                // TODO ........ (!)
                //rc = bind_one(pStmt, col_ids[i], vals[i]);
                if (rc != error_code::ok) {
                    // should throw an exception
                    return rc;
                }
            }
            return error_code::ok;
        }

        /** statement step */
        error_code_t step() const
        {
            error_code_t rc;
            /* method is expected to return SQLITE_DONE, or SQLITE_ROW 
             * on success. On other cases (SQLITE_BUSY), throw exception */
            rc = sqlite3_step(pStmt);
            if (rc != error_code::done && rc != error_code::row) {
                std::cout << "step rc: " << rc 
                    << ", BUSY: " << error_code::busy << std::endl;
                throw exception_step(errmsg());
            }
            return rc;
        }

        /** statement reset */
        error_code_t reset() const
        {
            error_code_t rc;
            /* method is expected to only return SQLITE_OK on success.
             * Otherwise, throw exception */
            rc = sqlite3_reset(pStmt);
            if (rc != error_code::ok) {
                throw exception_reset(errmsg());
            }
            return rc;
        }

        /** 
         * statement finalize 
         * https://www.sqlite.org/c3ref/finalize.html
         * */
        error_code_t finalize() noexcept
        {
            error_code_t rc;
            /* If the most recent evaluation of the statement encountered 
             * no errors or if the statement is never been evaluated, 
             * then sqlite3_finalize() returns SQLITE_OK. 
             * If the most recent evaluation of statement S failed, 
             * then sqlite3_finalize(S) returns the appropriate error code 
             * or extended error code. */
            rc = sqlite3_finalize(pStmt);
            pStmt = nullptr;
            return rc;
        }

        /**
         * wrapper for execute: prepare + step + reset + finalize
         */
        template <class... Ts>
        void execute(const char *sql, 
            vector<string> col_ids, Ts... vals) 
        {
            return_code_t rc;
            prepare(sql);
            bind(col_ids, vals...);
            while (rc != error_code::done) {
                try {
                    rc = step();
                    /*std::cout << "last step returned: " << rc 
                        << ", ROW: " << error_code::row
                        << ", DONE: " << error_code::done
                        << std::endl;*/
                    reset();
                } catch (exception &e) {
                    std::cout << "exception detected: " << e.what() << std::endl;
                    break;
                }
            }
            finalize();
        }

        template <class... Ts>
        void execute(const string &sql, 
            vector<string> col_ids, Ts... vals)
        { execute(sql.c_str(), col_ids, vals...); }

        void execute(const char *sql) 
        {
            return_code_t rc;
            prepare(sql);
            try {
                rc = step();
                /*std::cout << "last step returned: " << rc 
                    << ", ROW: " << error_code::row
                    << ", DONE: " << error_code::done
                    << std::endl;*/
                reset();
            } catch (exception &e) {
                std::cout << "exception detected: " << e.what() << std::endl;
            }
            finalize();
        }

        inline void execute(const string &sql)
        { execute(sql.c_str()); }
    };

    /** Main class */
    class db
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
        db(const char *filename, 
            int mode = open_mode::readwrite | open_mode::create
                       | open_mode::nomutex | open_mode::nofollow)
            : conn(nullptr), stmt(nullptr), 
              filename_(filename), errMsg_(nullptr)
        {
            open(filename, mode);
        }
        db(const string &filename) : db(filename.c_str()) {}

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
            if (rc != error_code::ok) {
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
            return sqlite3_errmsg(conn);
        }

        /** prepare internal statement */
        db &prepare(const char *zSql, int nByte = -1)
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
        db &bind(vector<string> col_ids, Ts... vals)
        {
            stmt->bind(col_ids, vals...);
            return *this;
        }

        /** step */
        return_code_t step() const { return stmt->step(); }

        /** reset */
        return_code_t reset() const { return stmt->reset(); }

        /** close actual connection */
        return_code_t close()
        {
            return_code_t rc = sqlite3_close(conn);
            if (rc != error_code::ok) {
                throw exception_close(errmsg());
            } else {
                //sqlite3_free(conn);    // apparently, sqlite3_close()
                                         // actually frees the sqlite3* pointer
                                         // passed as argument
                conn = nullptr;
                return rc;
            }
        }

        /** finalize internal statement */
        void finalize() { delete stmt; }

        /** 
         * Get direct access to the internal connection object .
         * Be cautious when using this (!) */
        connection getConnection() const {
            return conn;
        }

        template <class... Ts>
        void execute(const char *sql, 
            vector<string> col_ids, Ts... vals) 
        {
            stmt->execute(sql, col_ids, vals...);
        }
        template <class... Ts>
        void execute(const string &sql, 
            vector<string> col_ids, Ts... vals) 
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

        /** Utility callback for command (print a record vertically)*/
        static int printCallback_Vertical(void *data, int Cols, char **ColValues, char **ColNames) 
        { 
            col_index_t i; 
            //fprintf(stderr, "%s: ", (const char*)data); 

            for (i = 0; i < Cols; i++) { 
                //++exec_n_records_;
                printf("%s = %s\n", ColNames[i], ColValues[i] ? ColValues[i] : "NULL"); 
            }
            printf("\n"); 
            return 0; 
        } 
        /**
         * Prints out the result of the query, in vertical format.
         */
        return_code_t command(const char *sql)
        {
            return_code_t rc;
            errMsg_ = nullptr;

            rc = sqlite3_exec(conn, sql, &printCallback_Vertical, (void *)nullptr, &errMsg_);
            if (errMsg_) {
                fprintf(stderr, "error: %s\n", errMsg_);
            }
            free(errMsg_);
            return rc;
        }
        inline return_code_t command(const string &sql)
        { return command(sql.c_str()); }

        /**
         * Prints out the result of the query, in tabular format.
         */
        return_code_t command2(const char *sql) 
        {
            return_code_t rc;
            char **result;
            int nRow, nCol;
            char *errMsg;

            rc = sqlite3_get_table(
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

            return rc;
        }
        inline return_code_t command2(const string &sql)
        { return command2(sql.c_str()); }

        /** Opens an interactive prompt */
        void prompt()
        {
            std::ostringstream cmd;
            cmd << "sqlite3 -header -column " << filename_;
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
        ~db() {
            std::cout << "db::~db() called\n";
            //finalize();     // finalize internal statement
            close();        // close connection
        }

    private:
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
                    width = strlen(result[i*nCol + j - 1]);
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
                    cout << ' ' << std::left << std::setw(colWidth[j-1]) 
                        << result[i*nCol + j-1] << " |";
                }
                cout << endl;
            }

            /* print line */
            printTableSep(colWidth);
        }
    };
}

#endif

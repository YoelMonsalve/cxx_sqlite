/**
 * SQLITE3.HPP
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
 * (C) Copyright Yoel Monsalve 2019-2026.
 */

#ifndef SQLITE_HPP_
#define SQLITE_HPP_

#include <limits>
#include <string>
#include <cstdio>    /* sometimes, it is easier to call printf() */
#include <cstring>
#include <cstdint>   /* int32_t, int64_t */
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

#ifdef CXX_SQLITE3_DEBUG
#  define CXX_SQLITE3_LOG_DEBUG(msg) \
     do { std::cerr << "[cxx_sqlite3 debug " << __FILE__ << ":" << __LINE__ << "] " << msg << std::endl; } while (0)
#else
#  define CXX_SQLITE3_LOG_DEBUG(msg) do {} while (0)
#endif
#define PRINT_DEBUG_LINE__ CXX_SQLITE3_LOG_DEBUG("")

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


    /**
     * Utility function: make a `field` type, from an arbitrary value
    */
    template <class ColValType>
    std::pair<col_id_t, ColValType> make_field(col_id_t col_id, ColValType val)
    {
        return std::move(std::pair<col_id_t, ColValType>{col_id, val});
    }

    /**
     * row
     * Represents a single database row with column name and index access,
     * type-safe extraction via as<T>(), and NULL checking.
     */
    class row
    {
        std::vector<std::string> col_names_;
        std::vector<generic_object> values_;
        std::map<std::string, size_t> col_indices_;

    public:
        row() = default;

        void add(const std::string &name, const generic_object &val)
        {
            col_indices_[name] = values_.size();
            col_names_.push_back(name);
            values_.push_back(val);
        }

        size_t size() const noexcept { return values_.size(); }
        bool empty() const noexcept { return values_.empty(); }

        bool has_column(const std::string &name) const
        {
            return col_indices_.find(name) != col_indices_.end();
        }

        bool is_null(const std::string &name) const
        {
            auto it = col_indices_.find(name);
            if (it == col_indices_.end()) return true;
            return values_[it->second].is_null();
        }

        bool is_null(size_t index) const
        {
            if (index >= values_.size()) return true;
            return values_[index].is_null();
        }

        const generic_object &operator[](const std::string &name) const
        {
            auto it = col_indices_.find(name);
            if (it == col_indices_.end()) {
                throw exception_column(("column not found: " + name).c_str());
            }
            return values_[it->second];
        }

        const generic_object &operator[](const char *name) const
        {
            return (*this)[std::string(name ? name : "")];
        }

        const generic_object &operator[](size_t index) const
        {
            if (index >= values_.size()) {
                throw exception_column("column index out of range");
            }
            return values_[index];
        }

        template <typename T>
        T get(const std::string &name) const
        {
            return (*this)[name].as<T>();
        }

        template <typename T>
        T get(size_t index) const
        {
            return (*this)[index].as<T>();
        }

        const std::vector<std::string> &columns() const noexcept
        {
            return col_names_;
        }

        // Map-style insert for compatibility
        void insert(const std::pair<col_id_t, generic_object> &p)
        {
            add(p.first ? p.first : "", p.second);
        }
    };

    /** sqlite3 prepared statement */
    class statement
    {
        sqlite3      *conn_  = nullptr;      /* sqlite3 DB connection */
        const char   *pzTail = nullptr;      /* pointer to unused portion */
        sqlite3_stmt *pStmt  = nullptr;      /* underlying sqlite3 primitive statement */
        return_code_t rc_    = int(error::ok);  /* return code of the last operation performed */

    public:
        /** constructors */
        statement() : conn_(nullptr), pzTail(nullptr), pStmt(nullptr), rc_(int(error::ok)) {}
        statement(sqlite3 *conn) 
            : conn_(conn), pzTail(nullptr), pStmt(nullptr), rc_(int(error::ok)) {}
        statement(sqlite3 *conn, const char *zSql, int nByte = -1)
            : conn_(conn), pzTail(nullptr), pStmt(nullptr), rc_(int(error::ok))
        {
            prepare(zSql, nByte);
        }
        statement(sqlite3 *conn, const string &sql)
            : statement(conn, sql.c_str(), -1) {}

        // Non-copyable (prevents double-finalize on sqlite3_stmt*)
        statement(const statement &) = delete;
        statement &operator=(const statement &) = delete;

        // Movable (transfers ownership of statement resources)
        statement(statement &&other) noexcept
            : conn_(other.conn_), pzTail(other.pzTail), pStmt(other.pStmt), rc_(other.rc_)
        {
            other.conn_ = nullptr;
            other.pzTail = nullptr;
            other.pStmt = nullptr;
            other.rc_ = int(error::ok);
        }

        statement &operator=(statement &&other) noexcept
        {
            if (this != &other)
            {
                finalize();
                conn_ = other.conn_;
                pzTail = other.pzTail;
                pStmt = other.pStmt;
                rc_ = other.rc_;
                other.conn_ = nullptr;
                other.pzTail = nullptr;
                other.pStmt = nullptr;
                other.rc_ = int(error::ok);
            }
            return *this;
        }

        ~statement()
        {
            finalize();
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

        /** column inspection methods */
        col_index_t colum_count(stmt *pstmt = nullptr) const noexcept 
        {
            (void)pstmt;
            return sqlite3_column_count(pStmt);
        }
        col_index_t column_count() const noexcept 
        {
            return sqlite3_column_count(pStmt);
        }
        const char *column_name(stmt *pstmt, col_index_t iCol) const
        {
            (void)pstmt;
            const char *col_name;
            if ((col_name = sqlite3_column_name(pStmt, iCol)) == nullptr) 
                throw exception_bind(errmsg());
            return col_name;
        }
        const char *column_name(col_index_t iCol) const
        {
            return column_name(pStmt, iCol);
        }
        int colum_bytes(stmt *pstmt, col_index_t iCol) const
        {
            (void)pstmt;
            return sqlite3_column_bytes(pStmt, iCol);
        }
        int column_bytes(col_index_t iCol) const
        {
            return sqlite3_column_bytes(pStmt, iCol);
        }
        int column_type(stmt *pstmt, col_index_t iCol) const
        {
            (void)pstmt;
            return sqlite3_column_type(pStmt, iCol);
        }
        int column_type(col_index_t iCol) const
        {
            return sqlite3_column_type(pStmt, iCol);
        }

        /* Positional bind_one overloads (1-based index) */
        ErrorCode bind_one(col_index_t iCol, int val) { return bind_int(iCol, val); }
        ErrorCode bind_one(col_index_t iCol, int64_t val) { return bind_int_64(iCol, val); }
        ErrorCode bind_one(col_index_t iCol, uint32_t val) { return bind_int_64(iCol, val); }
        ErrorCode bind_one(col_index_t iCol, uint64_t val) { return bind_int_64(iCol, static_cast<int64_t>(val)); }
        ErrorCode bind_one(col_index_t iCol, double val) { return bind_double(iCol, val); }
        ErrorCode bind_one(col_index_t iCol, float val) { return bind_double(iCol, static_cast<double>(val)); }
        ErrorCode bind_one(col_index_t iCol, const char *val) {
            if (val == nullptr) return bind_null(iCol);
            return bind_text(iCol, val, -1);
        }
        ErrorCode bind_one(col_index_t iCol, const string &val, destructor_type hdl = bind_handler::bind_transient) {
            return bind_text(iCol, val.c_str(), val.size(), hdl);
        }
        ErrorCode bind_one(col_index_t iCol, const blob_t &val, destructor_type hdl = bind_handler::bind_transient) {
            return bind_blob(iCol, val.data(), val.size(), hdl);
        }
        ErrorCode bind_one(col_index_t iCol, std::nullptr_t) { return bind_null(iCol); }
        ErrorCode bind_one(col_index_t iCol, const SQLITE3_ONULL &) { return bind_null(iCol); }
        ErrorCode bind_one(col_index_t iCol, const generic_object &val) {
            switch (val.type()) {
            case generic_object::type_tag::null_t:
                return bind_null(iCol);
            case generic_object::type_tag::int_t:
                return bind_int_64(iCol, val.as_int());
            case generic_object::type_tag::float_t:
                return bind_double(iCol, val.as_double());
            case generic_object::type_tag::text_t:
                return bind_text(iCol, val.as_string().c_str(), val.as_string().size());
            case generic_object::type_tag::blob_t:
                return bind_blob(iCol, val.as_blob().data(), val.as_blob().size());
            }
            return bind_null(iCol);
        }
        template <typename T>
        ErrorCode bind_one(col_index_t iCol, const basic_object<T> &val) {
            return bind_one(iCol, static_cast<const generic_object &>(val));
        }

        /* Named bind_one overloads */
        template <typename T>
        ErrorCode bind_one(col_id_t col_id, const T &val) {
            col_index_t iCol = bind_parameter_index(col_id);
            return bind_one(iCol, val);
        }
        template <typename T>
        ErrorCode bind_one(const std::string &col_id, const T &val) {
            return bind_one(col_id.c_str(), val);
        }
        ErrorCode bind_one(col_id_t col_id) {
            col_index_t iCol = bind_parameter_index(col_id);
            return bind_null(iCol);
        }

        /** 
         * This generic template method allows declarations such like:
         * 
         *   db.bind(stmt, std::pair<string, int>{":id", 1})
         *   db.bind(stmt, std::pair<string, string>{":name", "John"})
         */
        template <class ColValType>
        ErrorCode bind(std::pair<col_id_t, ColValType> pair_)
        {   
            return bind_one(pair_.first, pair_.second);
        }

        /**
         * This variadic method allows to pass several values at once with a vector of column names.
         */
        ErrorCode bind_variadic__(vector<string>::size_type, vector<col_id_t>)
        { 
            return error::ok; 
        }
        template <class T, class... Ts>
        ErrorCode bind_variadic__(vector<string>::size_type i, vector<col_id_t> col_ids, T val, Ts... vals)
        {
            rc_ = bind_one(col_ids[i], val);
            if (rc_ != (int)error::ok) {
                throw exception_bind(errmsg());
            }
            if (i + 1 < col_ids.size())
                return bind_variadic__(i + 1, col_ids, vals...);
            else
                return rc_;
        }
        template <class T, class... Ts>
        ErrorCode bind(vector<col_id_t> col_ids, T val, Ts... vals)
        {
            return bind_variadic__(0, col_ids, val, vals...);
        }
        
        /* Bind vector of column names and vector of generic values */
        typedef vector<generic_object> generic_col_vals;
        ErrorCode bind(const vector<col_id_t> &col_ids, const generic_col_vals &vals)
        {
            size_t n = col_ids.size() < vals.size() ? col_ids.size() : vals.size();
            for (size_t i = 0; i < n; i++) {
                rc_ = bind_one(col_ids[i], vals[i]);
                if (rc_ != int(error::ok)) {
                    throw exception_bind(errmsg());
                }
            }
            return (int)error::ok;
        }

        /* Alternating name-value variadic binding: bind(col_1, val_1, col_2, val_2, ...) */
        ErrorCode bind() { return error::ok; }

        template <class T, class... VarTypes>
        ErrorCode bind(const char *col_1, const T &val_1, VarTypes&&... args)
        {
            rc_ = bind_one(col_1, val_1);
            if (rc_ != int(error::ok)) {
                throw exception_bind(errmsg());
            }
            return bind(std::forward<VarTypes>(args)...);
        }

        template <class T, class... VarTypes>
        ErrorCode bind(const std::string &col_1, const T &val_1, VarTypes&&... args)
        {
            return bind(col_1.c_str(), val_1, std::forward<VarTypes>(args)...);
        }

        /* Alternating positional index-value binding: bind(1, val_1, 2, val_2, ...) */
        template <class T, class... VarTypes>
        ErrorCode bind(int col_idx, const T &val_1, VarTypes&&... args)
        {
            rc_ = bind_one(col_idx, val_1);
            if (rc_ != int(error::ok)) {
                throw exception_bind(errmsg());
            }
            return bind(std::forward<VarTypes>(args)...);
        }

        /** statement step - C style, throws exception */
        ErrorCode step()
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
            rc_ = sqlite3_step(pStmt);
            ec = rc_;
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

        /** clear all bound parameters */
        ErrorCode clear_bindings() noexcept
        {
            if (pStmt != nullptr)
            {
                rc_ = sqlite3_clear_bindings(pStmt);
            }
            return rc_;
        }

        /** check if statement is prepared / valid */
        bool is_valid() const noexcept { return pStmt != nullptr; }
        explicit operator bool() const noexcept { return is_valid(); }

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
        typedef row record;          // row = record
        typedef std::vector<record> recordSet;
        typedef recordSet result;    // result = recordSet

        row read_current_row() const
        {
            row r;
            col_index_t n_cols = column_count();
            for (col_index_t j = 0; j < n_cols; j++) {
                const char *col_name = column_name(j);
                data_type_t col_type = column_type(j);
                switch (col_type) {
                case data_type::int_t:
                    r.add(col_name, generic_object(static_cast<int64_t>(sqlite3_column_int64(pStmt, j))));
                    break;
                case data_type::float_t:
                    r.add(col_name, generic_object(sqlite3_column_double(pStmt, j)));
                    break;
                case data_type::text_t: {
                    const unsigned char *txt = sqlite3_column_text(pStmt, j);
                    r.add(col_name, std::string(reinterpret_cast<const char *>(txt ? txt : reinterpret_cast<const unsigned char *>(""))));
                    break;
                }
                case data_type::blob_val: {
                    const void *blob = sqlite3_column_blob(pStmt, j);
                    int bytes = sqlite3_column_bytes(pStmt, j);
                    r.add(col_name, generic_object(blob, bytes > 0 ? static_cast<size_t>(bytes) : 0));
                    break;
                }
                case data_type::null_t:
                default:
                    r.add(col_name, generic_object(nullptr));
                    break;
                }
            }
            return r;
        }

        row fetch_row(ErrorCode &ec)
        {
            row r;
            step(ec);
            CXX_SQLITE3_LOG_DEBUG("step returned: " << ec);
            if (ec != error::row) {
                if (ec == error::done) {
                    reset(ec);
                }
                return r;
            }
            return read_current_row();
        }

        row fetch_row()
        {
            ErrorCode ec;
            return fetch_row(ec);
        }

        /** Input iterator for range-based for loops: for (auto row : stmt) */
        class iterator {
            statement *stmt_ = nullptr;
            row current_row_;
            bool at_end_ = true;

            void advance() {
                if (!stmt_ || !stmt_->is_valid()) {
                    at_end_ = true;
                    return;
                }
                ErrorCode ec;
                stmt_->step(ec);
                if (ec == error::row) {
                    current_row_ = stmt_->read_current_row();
                    at_end_ = false;
                } else {
                    at_end_ = true;
                    if (ec == error::done) {
                        stmt_->reset(ec);
                    }
                }
            }

        public:
            using iterator_category = std::input_iterator_tag;
            using value_type = row;
            using difference_type = std::ptrdiff_t;
            using pointer = const row *;
            using reference = const row &;

            iterator() : stmt_(nullptr), at_end_(true) {}
            explicit iterator(statement *s) : stmt_(s), at_end_(false) {
                advance();
            }

            reference operator*() const { return current_row_; }
            pointer operator->() const { return &current_row_; }

            iterator &operator++() {
                advance();
                return *this;
            }

            bool operator==(const iterator &other) const {
                if (at_end_ && other.at_end_) return true;
                return (at_end_ == other.at_end_) && (stmt_ == other.stmt_);
            }

            bool operator!=(const iterator &other) const {
                return !(*this == other);
            }
        };

        iterator begin() { return iterator(this); }
        iterator end() { return iterator(); }


        /**
         * wrapper for execute: prepare + step + reset + finalize
         */
        template <class... Ts>
        void execute(const char *sql, 
            vector<col_id_t> col_ids, Ts... vals) 
        {
            try {
                prepare(sql);
                bind(col_ids, vals...);
                step();
                reset();
            } catch (exception &e) {
                CXX_SQLITE3_LOG_DEBUG("exception detected in execute: " << e.what());
                finalize();
                throw;
            }
            finalize();
        }
        /** C++ string version, bind arguments */
        template <class... Ts>
        void execute(const string &sql, 
            vector<col_id_t> col_ids, Ts... vals)
        { execute(sql.c_str(), col_ids, vals...); }

        /** C const char * version, no arguments */
        void execute(const char *sql) 
        {
            try {
                prepare(sql);
                step();
                reset();
            } catch (exception &e) {
                CXX_SQLITE3_LOG_DEBUG("exception detected in execute: " << e.what());
                finalize();
                throw;
            }
            finalize();
        }
        /** C++ string version, no arguments */
        inline void execute(const string &sql)
        { execute(sql.c_str()); }        

        template <class ColValType>
        static std::pair<col_id_t, ColValType> make_field(col_id_t col_id, ColValType val)
        {
            return {col_id, val};
        }
    };

    /** Namespace-level convenient type aliases */
    using record = statement::record;
    using recordSet = statement::recordSet;
    using result = statement::result;

    class database;

    /**
     * RAII Transaction Guard
     * Automatically rolls back if an exception occurs or scope exits without commit().
     */
    class transaction
    {
        database &db_;
        bool committed_ = false;

    public:
        explicit transaction(database &db);
        ~transaction();

        void commit();
        void rollback();
        bool is_committed() const noexcept { return committed_; }

        // Non-copyable
        transaction(const transaction &) = delete;
        transaction &operator=(const transaction &) = delete;

        // Movable
        transaction(transaction &&other) noexcept;
        transaction &operator=(transaction &&other) = delete;
    };

    /** Main database connection class */
    class database
    {
    private:
        string        filename_;
        char          *errMsg_        = nullptr;

    protected:        
        sqlite3   *conn = nullptr;    // internal sqlite3 connection
        statement *stmt = nullptr;    // pointer to internal statement

    public:
        /** constructors */
        database() : filename_(""), errMsg_(nullptr), conn(nullptr), stmt(nullptr) {}
        database(const char *filename, 
            int mode = open_mode::readwrite | open_mode::create
                       | open_mode::nomutex | open_mode::nofollow)
            : filename_(filename ? filename : ""), errMsg_(nullptr), conn(nullptr), stmt(nullptr)
        {
            open(filename, mode);
        }
        database(const string &filename) : database(filename.c_str()) {}

        // Non-copyable (connections cannot be copied)
        database(const database &) = delete;
        database &operator=(const database &) = delete;

        // Movable (transfers database connection ownership)
        database(database &&other) noexcept
            : filename_(std::move(other.filename_)),
              errMsg_(other.errMsg_),
              conn(other.conn),
              stmt(other.stmt)
        {
            other.errMsg_ = nullptr;
            other.conn = nullptr;
            other.stmt = nullptr;
        }

        database &operator=(database &&other) noexcept
        {
            if (this != &other)
            {
                close();
                filename_ = std::move(other.filename_);
                errMsg_ = other.errMsg_;
                conn = other.conn;
                stmt = other.stmt;
                other.errMsg_ = nullptr;
                other.conn = nullptr;
                other.stmt = nullptr;
            }
            return *this;
        }

        /** starts a new connection to the given database filename */
        void open(const char *filename, int mode)
        {   
            if (!filename || !*filename) 
                throw exception_open("filename is null or empty.");

            filename_ = filename;
            return_code_t rc = sqlite3_open_v2(
                filename,      // datafile name
                &conn,         // sqlite3 object 
                mode,          // open flags
                NULL           // zVfs
            );
            if (rc != int(error::ok)) {
                throw exception_open(sqlite3_errstr(rc));
            }
            /* initialize internal statement object */
            if (stmt != nullptr) delete stmt;
            stmt = new statement(conn);
        }

        /** alias for open */
        inline void connect(const char *filename, int mode)
        { open(filename, mode); }

        /** message associated to the more recent error */
        const char * errmsg() const noexcept
        {
            return stmt ? stmt->errmsg() : (conn ? sqlite3_errmsg(conn) : "connection closed");
        }

        /** 
         * Create an independent, RAII-managed prepared statement.
         */
        statement prepare_statement(const char *zSql, int nByte = -1) const
        {
            if (conn == nullptr)
                throw exception("database connection is closed");
            return statement(conn, zSql, nByte);
        }
        statement prepare_statement(const string &sql) const
        {
            return prepare_statement(sql.c_str());
        }

        /** Begin an RAII transaction */
        transaction begin_transaction()
        {
            return transaction(*this);
        }

        /** prepare internal statement (for chaining) */
        database &prepare(const char *zSql, int nByte = -1)
        {
            if (stmt == nullptr) {
                if (conn == nullptr) throw exception("in db::prepare: database connection is closed");
                stmt = new statement(conn);
            }
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
        // signature for bind(std::pair<col_id_t, T>(col, val))
        template <class ColValType>
        database & bind(std::pair<col_id_t, ColValType> pair_)
        {   
            stmt->bind(pair_);
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
            if (conn == nullptr) throw exception("database connection is closed");
            statement temp(conn);
            temp.execute(sql, col_ids, vals...);
        }
        template <class... Ts>
        void execute(const string &sql, 
            vector<col_id_t> col_ids, Ts... vals) 
        {
            execute(sql.c_str(), col_ids, vals...);
        }

        void execute(const char *sql) 
        {
            if (conn == nullptr) throw exception("database connection is closed");
            statement temp(conn);
            temp.execute(sql);
        }
        void execute(const string &sql) 
        {
            execute(sql.c_str());
        }

        /* Alternating name/value execute: db.execute(sql, ":col1", val1, ":col2", val2, ...) */
        template <class T, class... VarTypes>
        void execute(const char *sql, const char *col1, const T &val1, VarTypes&&... args)
        {
            if (conn == nullptr) throw exception("database connection is closed");
            statement temp(conn);
            temp.prepare(sql);
            temp.bind(col1, val1, std::forward<VarTypes>(args)...);
            temp.step();
        }
        template <class T, class... VarTypes>
        void execute(const string &sql, const char *col1, const T &val1, VarTypes&&... args)
        {
            execute(sql.c_str(), col1, val1, std::forward<VarTypes>(args)...);
        }

        /* Convenient exec alias */
        template <class... Args>
        void exec(Args&&... args)
        {
            execute(std::forward<Args>(args)...);
        }

        /** Submit query and return an iterable prepared statement */
        template <class... Args>
        statement query(const char *sql, Args&&... args)
        {
            statement st = prepare_statement(sql);
            st.bind(std::forward<Args>(args)...);
            return st;
        }
        template <class... Args>
        statement query(const string &sql, Args&&... args)
        {
            return query(sql.c_str(), std::forward<Args>(args)...);
        }
        statement query(const char *sql)
        {
            return prepare_statement(sql);
        }
        statement query(const string &sql)
        {
            return prepare_statement(sql);
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
            close();        // close connection
        }

    private:
        /** Utility callback for command (print a record vertically)*/
        static int printCallback_Vertical(void *data, int Cols, char **ColValues, char **ColNames) 
        { 
            (void)data;
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

    /** transaction implementation */
    inline transaction::transaction(database &db) : db_(db), committed_(false) {
        db_.execute("BEGIN TRANSACTION;");
    }

    inline transaction::~transaction() {
        if (!committed_) {
            try {
                if (!db_.autocommit()) {
                    db_.execute("ROLLBACK TRANSACTION;");
                }
            } catch (...) {
                // destructors must not throw
            }
        }
    }

    inline void transaction::commit() {
        if (!committed_) {
            db_.execute("COMMIT TRANSACTION;");
            committed_ = true;
        }
    }

    inline void transaction::rollback() {
        if (!committed_) {
            db_.execute("ROLLBACK TRANSACTION;");
            committed_ = true;
        }
    }

    inline transaction::transaction(transaction &&other) noexcept
        : db_(other.db_), committed_(other.committed_) {
        other.committed_ = true;
    }
}

#endif

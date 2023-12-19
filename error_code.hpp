/**
 * ERROR_CODE.HPP
 * 
 * -.- The cxx_sqlite3 library -.-
 * 
 * A model for error codes (based on the <system_error> philosophy) 
 * for cxx_sqlite3.
 * 
 * @version 0.1.0 
 * @author  Yoel Monsalve
 * @date    2023-11-11
 * 
 * (C) Copyright VDI Technologies, LLC. 2019-2023.
 * Tulsa, OK.
 */

#ifndef ERROR_CODE_HPP_
#define ERROR_CODE_HPP_

#include <system_error>
#include <iostream>

/**
 * References: 
 * System error support in C++0x (by Chris)
 * 
 * http://blog.think-async.com/2010/04/system-error-support-in-c0x-part-1.html
 * http://blog.think-async.com/2010/04/system-error-support-in-c0x-part-2.html
 * http://blog.think-async.com/2010/04/system-error-support-in-c0x-part-3.html
 * http://blog.think-async.com/2010/04/system-error-support-in-c0x-part-4.html
 * http://blog.think-async.com/2010/04/system-error-support-in-c0x-part-5.html
 * 
 */

namespace cxx_sqlite3
{
    typedef int return_code_t;

    /** primitive(int)-type return codes, from sqlite3 */
    struct return_code {
        using value_type                   = return_code_t;

        static const value_type ok         = SQLITE_OK        ;
        static const value_type row        = SQLITE_ROW       ;
        static const value_type done       = SQLITE_DONE      ;
        static const value_type error      = SQLITE_ERROR     ;
        static const value_type internal   = SQLITE_INTERNAL  ;
        static const value_type perm       = SQLITE_PERM      ;
        static const value_type abort      = SQLITE_ABORT     ;
        static const value_type busy       = SQLITE_BUSY      ;
        static const value_type locked     = SQLITE_LOCKED    ;
        static const value_type misuse     = SQLITE_MISUSE    ;
    };

    /* aliases */
    using error_code_t = return_code_t;
    using error_val    = error_code_t;
    using error_val_t  = error_code_t;

    /** error code enumerator */
    enum class error : error_code_t {
        ok         = return_code::ok        ,
        row        = return_code::row       ,
        done       = return_code::done      ,
        error      = return_code::error     ,
        internal   = return_code::internal  ,
        perm       = return_code::perm      ,
        abort      = return_code::abort     ,
        busy       = return_code::busy      ,
        locked     = return_code::locked    ,
        misuse     = return_code::misuse    ,
        // ...

        // custom error codes (apart from the sqlite3-API ones)
        //bad_bind_name = ...
    };

    /** Defining an error_category for this namespace */
    class error_category : public std::error_category
    {
    public:
        virtual const char *name() const noexcept override
        { return "cxx_sqlite3"; }
        virtual std::string message(int ev) const override
        { return sqlite3_errstr(ev); }
    };

    /** prototypes */
    std::error_code make_error_code(error ev) noexcept;
    std::error_code make_error_code(error_code_t ev) noexcept;
    std::error_condition make_error_condition(error ev) noexcept;
    std::error_condition make_error_condition(error_code_t ev) noexcept;

    using std::error_code;
    using std::error_condition;

    /** Specialized error_code class */
    class ErrorCode : public std::error_code 
    {
        friend std::ostream &operator<<(std::ostream &os, error_code &ec)
        {
            return os << "error ["
				<< ec.value() << "]: "
				<< ec.category().name()
				<< ": " << ec.category().message(ec.value());
        }
    public:
        ErrorCode() : std::error_code() {}
        ErrorCode(return_code_t val) : std::error_code(make_error_code(val)) {}

        operator return_code_t() { return value(); }
    };
}
namespace std 
{
    /** 
     * Register enum class error as ErrorConditionEnum 
     * After this, compiler will recognize `cxx_sqlite3::error`
     * as convertible to type `std::error_condition`.
     */
    template <>
    struct is_error_condition_enum<cxx_sqlite3::error>
        : public true_type {};
}

#endif

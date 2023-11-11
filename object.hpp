/**
 * OBJECT.HPP
 * Define a common c++ like wrapper for the native types present in sqlite3 
 * (INTEGER, DOUBLE, etc).
 * 
 * @version 0.1.0 
 * @author  Yoel Monsalve
 * @date    2023-11-11
 * 
 * (C) Copyright VDI Technologies, LLC. 2019-2023.
 * Tulsa - Oklahoma.
 */

#ifndef OBJECT_HPP_
#define OBJECT_HPP_

#include "std_version.h"
#include "vector"
#include "string"

namespace cxx_sqlite3
{

/** 
 * Template generic object
 * 
 * The purpose of this class is to server as a common wrapper for any of the
 * native data types in sqlite3 (e.g., int, int64, text, etc).
 * 
 * For example, the declarations 
 * 
 * sqlite3::basic_object<int> O_SQLITE_INT;
 * sqlite3::basic_object<int> O_SQLITE_DOUBLE;
 * 
 * allow to store an INTEGER, and a DOUBLE sqlite3 value, respectively. 
 * 
 * The underlying data is dynamically allocated in runtime, and then released 
 * once the object is destroyed.
 */
struct generic_object    // base class
{
    /* empty constructor */
    generic_object() : pdata(nullptr) {}
    /* from a pointer (shared pointer) */
    generic_object(void * pdata_) : pdata(pdata_) {}

    #if 0
    /* from <int> object */
    generic_object(const basic_object<int> &OINT)
    {
        pdata = OINT.getpdata();
    }
    generic_object(basic_object<int> &&OINT)
    {
        pdata = OINT.getpdata();
        OINT.nullpdata();    // nullify internal pointer of stolen object
    }
    /* from <char> object */
    generic_object(const basic_object<char> &OCHAR)
    {
        pdata = OCHAR.getpdata();
    }
    generic_object(basic_object<char> &&OCHAR)
    {
        pdata = OCHAR.getpdata();
        OCHAR.nullpdata();    // nullify internal pointer of stolen object
    }
    #endif
protected:
    void *pdata = nullptr;    // hidden data
};

#define BASIC_OBJECT_OVERLOAD_BUILTIN_OPERATOR(op,rtype) rtype operator op(const value_type &v) const { return *(T *)(pdata) op v; }
#define BASIC_OBJECT_OVERLOAD_OPERATOR(op, rtype) rtype operator op(const basic_object<value_type> &other) const { return *(T *)(pdata) op *(T *)(other.pdata); }

/* specialized (T-typed) class from (non-typed) generic object */
template<class T>
struct basic_object : generic_object
{
    using value_type = T;          // my stored type: allows syntax basic_object<T>::value_type
    using type = basic_object;     // my own type: allows syntax basic_object<T>::type (=T)

    /* initializer constructor */
    basic_object(const value_type &v)
    {
        assert( (pdata = new value_type) != nullptr );
        *(T *)pdata = v;
    }
    /* initializer constructor (with a rv-qualified value)*/
    basic_object(value_type&& v)
    {
        assert( (pdata = new value_type) != nullptr );
        *(T *)pdata = v;    /* as v is rv-qualified, the compiler might take
                             * some advantage of that, at this assignment */
    }
    /* destructor */
    ~basic_object() { std::cout<<"Destructor called\n"; delete (value_type *)pdata; }

    /* Rule of Three */
    /* copy constructor */
    basic_object(const basic_object<value_type> &o)
    {
        assert( (pdata = new value_type) != nullptr );
        *(T *)pdata = *(T *)o.pdata;
    }
    /* rvalue constructor */
    basic_object(basic_object<value_type>&& o)
    {
        pdata = o.pdata;
        o.pdata = nullptr;
    }
    /* swap */
    void swap(const basic_object<value_type> &other)
    {
        T tmp;
        tmp = *(T *)pdata;
        *(T *)pdata = *((T *)other.pdata);
        *((T *)other.pdata) = tmp;
    }
    /* assignment from basic_object */
    basic_object &operator=(basic_object<value_type> other) {
        swap(other);
        return *this;
    }

    /* explicitely accessing the value (it is the same as (value_type)basic_object) */
    value_type value() const { return *(value_type *)pdata; }
    value_type operator()() const noexcept { return *(value_type *)pdata; }

    /* static cast to value_type */
    operator value_type() const noexcept { return *(value_type *)pdata; }

    /* not operator */
    bool operator!() { return !(this->value()); }

    /* overload built-in operators  (==, >, <, etc...) for (basic_object<T>, basic_object<T>) */
    /*bool operator==(const basic_object<value_type> &other) const
    { return *(T *)(pdata) == *(T *)(other.pdata); }*/
    BASIC_OBJECT_OVERLOAD_OPERATOR(== , bool)
    BASIC_OBJECT_OVERLOAD_OPERATOR(!= , bool)
    BASIC_OBJECT_OVERLOAD_OPERATOR(> , bool)
    BASIC_OBJECT_OVERLOAD_OPERATOR(< , bool)
    BASIC_OBJECT_OVERLOAD_OPERATOR(>= , bool)
    BASIC_OBJECT_OVERLOAD_OPERATOR(<= , bool)

    /* overload built-in operators (==, >, <, etc...) for (basic_object<T>, T) */
    BASIC_OBJECT_OVERLOAD_BUILTIN_OPERATOR(== , bool)
    BASIC_OBJECT_OVERLOAD_BUILTIN_OPERATOR(!= , bool)
    BASIC_OBJECT_OVERLOAD_BUILTIN_OPERATOR(> , bool)
    BASIC_OBJECT_OVERLOAD_BUILTIN_OPERATOR(< , bool)
    BASIC_OBJECT_OVERLOAD_BUILTIN_OPERATOR(>= , bool)
    BASIC_OBJECT_OVERLOAD_BUILTIN_OPERATOR(<= , bool)
    // TODO: more operators ...

    /* downcast to generic object (without type) .... not needed */
    //operator generic_object() const noexcept { return generic_object(pdata); }

    /* public access to internal pointer */
    void *getpdata() const { return pdata; }
    void nullpdata() { pdata = nullptr; }

//    private:
    //void *pdata = nullptr;    // hidden data
};
#undef BASIC_OBJECT_OVERLOAD_BUILTIN_OPERATOR
#undef BASIC_OBJECT_OVERLOAD_OPERATOR
}

/* overloading std::swap for `basic_object` */
template<class T>
void std::swap(cxx_sqlite3::basic_object<T> &O1, cxx_sqlite3::basic_object<T> &O2) {
    O1.swap(O2);
}

typedef cxx_sqlite3::basic_object<int> SQLITE3_OINT;           // equivalent to sqlite3-like INTEGER
typedef cxx_sqlite3::basic_object<char> SQLITE3_OCHAR;
typedef cxx_sqlite3::basic_object<float> SQLITE3_OFLOAT;
typedef cxx_sqlite3::basic_object<double> SQLITE3_ODOUBLE;     // equivalent to sqlite3-like DOUBLE
typedef cxx_sqlite3::basic_object<char *> SQLITE3_OTEXT;       // equivalent to sqlite3-like TEXT
typedef cxx_sqlite3::basic_object<const char *> SQLITE3_OCTEXT;
typedef cxx_sqlite3::basic_object<std::vector<std::string>> SQLITE3_OSTRING;    // another equivalent to sqlite3-like TEXT
#endif

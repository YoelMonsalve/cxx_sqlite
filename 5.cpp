#include <type_traits>
#include <iostream>
#include <cstdio>
#include <cassert>
#include <string>
#include <vector>
#include <tuple>

template <class T>
void test(T value)
{
    int a[std::tuple_size<T>{}]; // can be used at compile time
 
    std::cout << std::tuple_size<T>{} << ' ' // or at run time
              << sizeof a << ' ' 
              << sizeof value << '\n';
}

template <class Ts>
size_t get_size(Ts t) {
    //std::cout << t << std::endl;
    return std::tuple_size<decltype(t)>::value;
}

/** variadic struct */
template <class T>
struct A
{
    T value;
    //using value_type = T;
    typedef T value_type;
    using type = A;
    A(T v) : value(v) {}

    operator value_type() const noexcept { return value; }
    value_type operator()() const noexcept { return value; }
};

template<int I>
struct B {
    B() { std::cout << "I'm B<int>\n"; }   
};

template <>
struct B<0> {
    int x = 0;
    B() { std::cout << "I'm B<0>\n"; }   
};

template <>
struct B<1> {
    int x = 0;
    B() { std::cout << "I'm B<1>\n"; }   
};

template <class T>
struct C;

template <>
struct C<int> {
    int x = 0;
    C() { std::cout << "I'm C<int>\n"; }   
};

template <>
struct C<char> {
    char x = '\0';
    C() { std::cout << "I'm C<char>\n"; }   
};

/** Variadic function to traverse the name and value of arguments */

// Version 1: <char**, T1 val1, T2 val2, ...>. Example: f({"col1", "col2", ...}, val1, val2, ...)
template <class Col1Type>       // forward declaration of base case
void scan_arguments(const char **col_names, Col1Type col1);

template <class Col1Type, class...ColTypes>
void scan_arguments(const char **col_names, Col1Type col1, ColTypes... cols)
{
    const char **zPtr = col_names;
    std::cout << *zPtr++ << ": " << col1 << std::endl;
    // NULL and "", can be used to end the array of names
    // e.g.: {"col1", "col2", NULL}, or {"col1", "col2", ""}
    if (*zPtr && **zPtr) {      
        scan_arguments(zPtr, cols...);
    }
}
// base case
template <class Col1Type>
void scan_arguments(const char **col_names, Col1Type col1)
{
    const char **zPtr = col_names;
    std::cout << *zPtr << ": " << col1 << std::endl;
}

// Version 2: <std::vector<std::string>, T1 val1, T2 val2, ...>. Example: f({"col1", "col2", ...}, val1, val2, ...)
template <class Col1Type>       // forward declaration of base case
void scan_arguments(std::vector<std::string>::iterator zPtr, Col1Type col1);

template <class Col1Type, class...ColTypes>
void scan_arguments(std::vector<std::string>::iterator zPtr, Col1Type col1, ColTypes... cols)
{
    std::cout << *zPtr++ << ": " << col1 << std::endl;
    scan_arguments(zPtr, cols...);
}
// base case
template <class Col1Type>
void scan_arguments(std::vector<std::string>::iterator zPtr, Col1Type col1)
{
    std::cout << *zPtr << ": " << col1 << std::endl;
}

// Version 3: <std::vector<std::string>, T1 val1, T2 val2, ...>. Example: f({"col1", "col2", ...}, val1, val2, ...)
//            But, specifying a maximum number of elements to scan
template <class Col1Type>       // forward declaration of base case
void scan_arguments(int N, std::vector<std::string>::iterator zPtr, Col1Type col1);

template <class Col1Type, class...ColTypes>
void scan_arguments(int N, std::vector<std::string>::iterator zPtr, Col1Type col1, ColTypes... cols)
{
    std::cout << *zPtr++ << ": " << col1 << std::endl;
    if (--N > 0)
        scan_arguments(N, zPtr, cols...);
}
// base case
template <class Col1Type>
void scan_arguments(int N, std::vector<std::string>::iterator zPtr, Col1Type col1)
{
    std::cout << *zPtr << ": " << col1 << std::endl;
}

template <class Col1Type, class...ColTypes>
void scan_arguments(std::vector<std::string> col_names, Col1Type col1, ColTypes... cols)
{
    scan_arguments(col_names.size(), col_names.begin(), col1, cols...);
}

/** TODO
 *  Specialized sqlite3 objects, for types int, int64, const char *, std::string, float, double
 * 
 *  e.g., 
 * 
 * namespace sqlite3 {
 * 
 * template <typename T>
 * class basic_object { ... } [generic_object {...}]
 * 
 * template <>
 * class object<int> { ... }
 * 
 * template <>
 * class object<int64> { ... }
 * 
 * ...
 * 
 * sqlite3::generic_object<int> O_SQLITE_INT{10};
 * sqlite3::generic_object<float> O_SQLITE_FLOAT{1.5f};
 * ...
*/

namespace sqlite3
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
template<class T>
struct basic_object
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
        *(T *)pdata = v;    /* as v in rvalue, the compiler might take some advantage of it,
                             * to execute this assignment */
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
    void swap(const basic_object<value_type> &other)
    {
        T tmp;
        tmp = *(T *)pdata;
        *(T *)pdata = *((T *)other.pdata);
        *((T *)other.pdata) = tmp;
    }
    basic_object &operator=(basic_object<value_type> other) {
        swap(other);
        return *this;
    }

    /* explicitely accessing the value (it is the same as (value_type)basic_object) */
    value_type value() const { return *(value_type *)pdata; }

    operator value_type() const noexcept { return *(value_type *)pdata; }
    value_type operator()() const noexcept { return *(value_type *)pdata; }

    /* comparison */
    bool operator==(const basic_object<T> other) const
    { return *(T *)(pdata) == *(T *)(other.pdata); }

    /* public access to internal pointer */
    void *getpdata() const { return pdata; }
    void nullpdata() { pdata = nullptr; }

    private:
    void *pdata = nullptr;    // hidden data
};

struct generic_object
{
    /* empty constructor */
    generic_object() : pdata(nullptr) {}
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
protected:
    void *pdata = nullptr;    // hidden data
};
}

/* overloading std::swap for `basic_object` */
template<class T>
void std::swap(sqlite3::basic_object<T> &O1, sqlite3::basic_object<T> &O2) {
    O1.swap(O2);
}

typedef sqlite3::basic_object<int> SQLITE3_OINT;      // sqlite3 "object-int"
typedef sqlite3::basic_object<char> SQLITE3_OCHAR;    // sqlite3 "object-char"

using std::vector;
using std::string;
using std::cout;
using std::endl;
int main()
{
    int x = 3;
    char c = 'A';

    vector<sqlite3::generic_object> v(3);
    sqlite3::basic_object<int> OINT1{10};
    sqlite3::basic_object<char> OCHAR1{'A'};
    sqlite3::generic_object O1(OINT1);
    v[0] = O1;
    v[1] = (SQLITE3_OINT)3;

    vector<sqlite3::generic_object> w{(SQLITE3_OINT)3, (SQLITE3_OCHAR)'A'};
    exit(0);

    sqlite3::basic_object<int>Oint{x};
    sqlite3::basic_object<char>Ochar{c};
    sqlite3::basic_object<int>Oint2{4};

    printf("x: %d, (int)Oint: %d, Oint(): %d, Oint.value(): %d, static_cast<int>(Oint): %d\n", x, 
        (int)Oint, Oint(), Oint.value(), static_cast<int>(Oint));
    printf("x: %x, (char)Ochar: %c, Ochar(): %c, Ochar.value(): %c\n", x, (char)Ochar, Ochar(), Ochar.value());
    std::cout << "Oint: " << Oint << std::endl;
    std::cout << "Ochar: " << Ochar << std::endl;
    printf("sizeof(Oint): %d\n", sizeof(sqlite3::basic_object<int>::type));

    std::cout << std::endl;
    printf("BEFORE: Oint: %d, Oint2: %d\n", Oint(), Oint2());
    Oint = Oint2;    
    printf("AFTER: Oint: %d, Oint2: %d\n", Oint(), Oint2());
    Oint = 3+4;
    printf("BEFORE: Oint: %d, Oint2: %d\n", Oint(), Oint2());
    //Oint.swap(Oint2);
    std::swap(Oint, Oint2);
    printf("AFTER: Oint: %d, Oint2: %d\n", (int)Oint, (int)Oint2);

    printf("Oint == 4 ? %c\n", Oint == SQLITE3_OINT(4) ? 'Y' : 'N');
    printf("Oint == 5 ? %c\n", Oint() == 5 ? 'Y' : 'N');

    exit(0);
  
    
    const char* column_names[] = {"colA", "colB", "colC"};
    scan_arguments(column_names, 10, 'a', "foo");
    std::vector<std::string> column_names2 = {"colA", "colB", "colC"};
    typedef std::vector<std::string> column_names_t;
    std::cout << '\n';
    scan_arguments(column_names_t{"colA", "colB", "colC"}.begin(), 20, 'b', "bar");
    std::cout << '\n';
    scan_arguments(column_names_t{"colA", "colB", "colC"}, 20, 'b', "bar");
    exit(0);
}
#include <type_traits>
#include <iostream>
#include <cstdio>
#include <cassert>
#include <string>
#include <vector>
#include <tuple>

#include "object.hpp"

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

/** Variadic functions to read column names */
void make_cols();    // forward declaration of base case
// recursive case
template <class T, class... Ts>
void make_cols(T a, Ts... args)
{
    std::cout << "read: " << a << std::endl;
    make_cols(args...);
}
// base case
void make_cols()
{
    std::cout << "no more arguments" << std::endl;
}
/* TODO:

make_cols() must have several signatures, for each possible argument type
 
- single argument: const char**
- single argument: std::vector<const char*>
- single argument: std::vector<std::string>
- list-like arguments: col1, col2, ... (variadic definition)
- (C++17) tuple: (col1, col2, ...)
*/
#define SQLITE3_INSERT_INTO(COLS, VALS) (make_cols COLS)

// convenience shortcut macros
#define OINT SQLITE3_OINT
#define OCHAR SQLITE3_OCHAR
#define INSERT_INTO SQLITE3_INSERT_INTO

// macro to determine C++ standard version
#if ((defined(_MSVC_LANG) && _MSVC_LANG >= 201703L) || __cplusplus >= 201703L)
#  define __cpp_17_compliant
#endif

using std::vector;
using std::string;
using std::cout;
using std::endl;
int main()
{
    /**
     * __cplusplus 202101L c++23
     * __cplusplus 202002L c++20
     * __cplusplus 201703L c++17
     * __cplusplus 201402L c++14
     * __cplusplus 201103L c++11
     * __cplusplus 199711L c++98
     * else pre-standard c++
    */
    #ifdef __cpp_17_compliant
        cout << "C++17 present\n"; 
    #else
        cout << "No C++17 present\n"; 
    #endif

    //make_cols('a', 1, 2, 1.5f);
    OINT I1{3+4};
    cout << "I1 (from rvalue): " << I1 << endl;
    int y = 7;
    OINT I2{y};
    cout << "I2 (from const value): " << I2 << endl;
    
    INSERT_INTO (('a','b','c'), (1,2,3));
    exit(0);
    
    int x = 3;
    char c = 'A';

    vector<sqlite3::generic_object> v(3);
    sqlite3::basic_object<int> OINT1{10};
    sqlite3::basic_object<char> OCHAR1{'A'};
    sqlite3::generic_object O1(OINT1);
    v[0] = O1;
    v[1] = (OINT)3;

    vector<sqlite3::generic_object> w{(SQLITE3_OINT)3, (SQLITE3_OCHAR)'A'};
    cout << "OINT1: " << OINT1 << endl;
    cout << "(int) OINT1: " << (int) OINT1 << endl;
    cout << "int(OINT1): " << int(OINT1) << endl;
    cout << "OINT1 == 9 ? " << (OINT1 == 9 ? 'Y' : 'N') << endl;
    cout << "OINT1 != 9 ? " << (OINT1 != 9 ? 'Y' : 'N') << endl;
    cout << "!OINT1 ? " << (!OINT1 ? 'Y' : 'N') << endl;
    OINT1 = 0;
    cout << "OINT1: " << OINT1 << ", !OINT1 : " << (!OINT1 ? 'Y' : 'N') << endl;
    OINT1 = 10;
    cout << "OINT1: " << OINT1 << ", !OINT1 : " << (!OINT1 ? 'Y' : 'N') 
        << ", !!OINT1 : " << (!!OINT1 ? 'Y' : 'N') << endl;
    OINT OINT2{11};
    cout << "OINT1 == OINT2 ? " << (OINT1 == OINT2 ? 'Y' : 'N') << endl;
    cout << "OINT1 != OINT2 ? " << (OINT1 != OINT2 ? 'Y' : 'N') << endl;
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

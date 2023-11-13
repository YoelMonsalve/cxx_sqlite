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

// convenience shortcut macros
#define OGEN SQLITE3_OGEN
#define OINT SQLITE3_OINT
#define OCHAR SQLITE3_OCHAR

// short alias for sqlite-namespace
#define sqlite cxx_sqlite3

using std::vector;
using std::string;
using std::cout;
using std::endl;

void read_int(OGEN I)
{
    cout << "read_float: " << (SQLITE3_OFLOAT)I << endl;
    cout << "read_int: " << (SQLITE3_OINT)I << endl;
}

template<class T>
void fn() { cout << "fn()" << endl; }

// bad
//void fn<int>(int x) { }

template<class T>
struct ftor;

template <>
struct ftor<int> { 
    void operator()(int x) { cout << "ftor<int>: x = " << x << endl; }; 
};
template <>
struct ftor<float> { 
    void operator()(float x) { cout << "ftor<float>: x = " << x << endl; }; 
};

int main()
{
    ftor<int> f1;
    f1(3);
    ftor<float>()(3.5f);
    ftor<float>{}(-3.5f);
    exit(0);
    
    OINT I1{3+4};
    cout << "I1 (from rvalue): " << I1 << endl;
    int y = 7;
    OINT I2{y};
    cout << "I2 (from const value): " << I2 << endl;
    
    int x = 3;
    char c = 'A';
    sqlite::generic_object OBJ{&x};
    I1 = OBJ;
    cout << "I1 (from generic_object): " << I1 << endl;
    OINT I3{OBJ};
    cout << "I3 (from generic_object): " << I3 << endl;
    OINT I4 = (OINT)OBJ;
    cout << "I4 (from generic_object): " << I4 << endl;

    /** 
     * Is it ever possible to upcast a basic_object to generic_object,
     * and still being able to recovery the original basic_object
    */
    read_int(I1);
    exit(0);
    

    vector<cxx_sqlite3::generic_object> v(3);
    cxx_sqlite3::basic_object<int> OINT1{10};
    cxx_sqlite3::basic_object<char> OCHAR1{'A'};
    cxx_sqlite3::generic_object O1(OINT1);
    v[0] = O1;
    v[1] = (OINT)3;

    vector<cxx_sqlite3::generic_object> w{(SQLITE3_OINT)3, (SQLITE3_OCHAR)'A'};
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

    cxx_sqlite3::basic_object<int>Oint{x};
    cxx_sqlite3::basic_object<char>Ochar{c};
    cxx_sqlite3::basic_object<int>Oint2{4};

    printf("x: %d, (int)Oint: %d, Oint(): %d, Oint.value(): %d, static_cast<int>(Oint): %d\n", x, 
        (int)Oint, Oint(), Oint.value(), static_cast<int>(Oint));
    printf("x: %x, (char)Ochar: %c, Ochar(): %c, Ochar.value(): %c\n", x, (char)Ochar, Ochar(), Ochar.value());
    std::cout << "Oint: " << Oint << std::endl;
    std::cout << "Ochar: " << Ochar << std::endl;
    printf("sizeof(Oint): %d\n", sizeof(cxx_sqlite3::basic_object<int>::type));

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

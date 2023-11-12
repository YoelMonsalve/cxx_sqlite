#include <type_traits>
#include <iostream>
#include <cstdio>
#include <cassert>
#include <cstdarg>
#include <string>
#include <vector>
#include <tuple>

#include "object.hpp"

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
#define SQLITE3_INSERT_INTO(COLS, VALS) (make_cols (COLS))

// convenience shortcut macros
#define OINT SQLITE3_OINT
#define OCHAR SQLITE3_OCHAR
#define INSERT_INTO SQLITE3_INSERT_INTO

using std::vector;
using std::string;
using std::tuple;
using std::cout;
using std::endl;

namespace sqlite3
{
    
/* main type for representing a list of column names */
typedef              std::string           col_name;
typedef              std::vector<col_name> col_names_list;    
/* alternative C style list, through a NULL-terminated array */
typedef const char * col_name_chars;
typedef const char * col_names_array[];
typedef       char * col_names_array_nc[];    // list of non-const column names
/* tuple style */
typedef tuple<std::string> col_names_tuple;

using string = std::string;

/* variadic function to assembly a list of column names from arguments */
// forward declaration of auxiliary function
template <class T, class... Ts>
void make_cols__(col_names_list &cols_list, T a, Ts... args);

template <class... Ts>
col_names_list make_cols(Ts... args) { 
    col_names_list cols_list{}; 
    make_cols__(cols_list, args...);    // auxiliary: defined below
    return cols_list;
}
// base case
void make_cols__(col_names_list &cols_list)
{
    // append last argument
    //cols_list.push_back(a);
    std::cout << "no more arguments" << std::endl;
}
// recursive case
template <class T, class... Ts>
void make_cols__(col_names_list &cols_list, T a, Ts... args)
{
    std::cout << "read: " << a << std::endl;
    // append first argument
    cols_list.push_back(a);
    // call recursively
    make_cols__(cols_list, args...);
}

void foo(col_names_list cols) {
    for (auto &c: cols) { cout << c << endl; }
}
void foo(col_names_array cols) {
    col_name_chars *cp = cols, c;
    /* traverse the array until encountering a nullptr, or "" (empty string) */
    while ( cp != nullptr && (c = *cp++) != nullptr && *c != '\0' )
        cout << "column: " << c << endl;
}
}

int main()
{
    sqlite3::make_cols("colA", "colB", "colC");
    exit(0);

    sqlite3::col_names_list cols{"colA", "colB", "colC"};
    sqlite3::col_names_array cols2 = {"colA", "colB", "colC", NULL};
    sqlite3::col_names_array cols3 = {"colA", "colB", "colC", ""};
    char col1[] = "colA";
    sqlite3::col_names_array_nc cols4 = {(char *)col1, nullptr};

    sqlite3::foo(vector<string>{"colA", "colB", "colC"});
    sqlite3::foo({"colA", "colB", "colC"});

    sqlite3::foo(cols2);
    sqlite3::foo(cols3);
    exit(0);

    INSERT_INTO (('a','b','c'), (1,2,3));
    exit(0);
    
}
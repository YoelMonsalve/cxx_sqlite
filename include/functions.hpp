/**
 * FUNCTIONS.HPP
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
 * (C) Copyright Yoel Monsalve 2025-2026.
 */

#ifndef SQLITE_FUNCTIONS_HPP_
#define SQLITE_FUNCTIONS_HPP_

#include <sqlite3.h>

namespace cxx_sqlite3
{
    /** wrapper for some sqlite3 functions */
    inline void free(void *ptr) { sqlite3_free(ptr); }
    inline void *malloc(int n) { return sqlite3_malloc(n); }
    inline void *malloc64(uint64 n) { return sqlite3_malloc64(n); }
}
#endif

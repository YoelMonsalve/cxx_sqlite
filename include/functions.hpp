/**
 * FUNCTIONS.HPP
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
 * Tulsa, OK.
 */

#ifndef SQLITE_FUNCTIONS_HPP_
#define SQLITE_FUNCTIONS_HPP_

#include <sqlite3.h>

namespace cxx_sqlite3
{
    /** wrapper for some sqlite3 functions */
    static void free(void *ptr) { sqlite3_free(ptr); }
    static void *malloc(int n) { return sqlite3_malloc(n); }
    static void *malloc64(uint64 n) { return sqlite3_malloc64(n); }
}
#endif
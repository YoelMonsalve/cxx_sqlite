/**
 * SQLITE3.CPP
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
#include "sqlite.hpp"

/** static definitions */

/** built-in sqlite3 destructors */
cxx_sqlite3::destructor_type cxx_sqlite3::bind_handler::bind_static    = SQLITE_STATIC;
cxx_sqlite3::destructor_type cxx_sqlite3::bind_handler::bind_transient = SQLITE_TRANSIENT;    

/** exceptions */
const cxx_sqlite3::exception cxx_sqlite3::const_exception::ex1 = exception("my exception 1");
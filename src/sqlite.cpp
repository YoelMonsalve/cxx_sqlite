/**
 * SQLITE.CPP
 * 
 * -.- The cxx_sqlite3 library -.-
 * 
 * In Release 2.0, cxx_sqlite3 is a 100% pure header-only library.
 * All runtime definitions (error_category_inst, make_error_code, bind_static, etc.)
 * have been inlined directly into the headers (error_code.hpp, exception.hpp, sqlite.hpp).
 * 
 * This translation unit is retained for backward compatibility, verifying that
 * including sqlite.hpp produces zero duplicate-symbol or linkage errors.
 * 
 * @version 2.0.0
 * @author  Yoel Monsalve
 * @date    2026-10-12
 * 
 * (C) Copyright Yoel Monsalve 2025-2026.
 */

#include "sqlite.hpp"

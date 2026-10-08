/** 
 * This is an attempt to produce a modern C++ "throw-version" 
 * from an existing rc-based, C-style function.
 * 
 * @version 1.0.0
 * @author  Yoel Monsalve
 * @date    2023-11-13
 * 
 * Copyright (C) Yoel Monsalve, 2025-2026.-
 */

/**
 * In general, the expected behaviour is:
 * 
 * - If fn(arg1, arg2, ...) returns a value `rc` that is equal to `rc_ok`, 
 *     then the throwable wrapper returns `rc` as well.
 * - Otherwise, the throwable wrapper throws some initially specified
 *     exception.
 * 
 * Three methods are provided in this template:
 * 
 * [1] function wrapper
 * [2] macros
 * [3] "throw-functor"
 * 
 * Quickguide
 * =======================================================
 * 
 * Define your C-style function, and exception to throw
 * 
 *   int fn(int a, int b) { return a + b; }
 *   std::runtime_error e("fn() failed!");
 * 
 * [1] Now, with the function wrapper style
 * 
 * ```
 * rc = call_throwable(fn, 0, e, 1, -1);   // OK,  rc = 0
 * //                      ^
 * //                      `~~~~~ OK return code (on success)
 * rc = call_throwable(fn, 0, e, 1,  2);   // fail 
 * ```
 * the first call will not throw, as the returned value is fn(1, -1) == 0. 
 * The second will fail, as fn(1, 2) = 3 != 0, and will throw the provided
 * exception e;
 * 
 * [2] With macro-style
 * 
 * ```
 * int rc, 
 *     rc_ok = 0;
 * TRY_OR_THROW__ (fn, (1, -1), rc, rc_ok, e);    // OK,  rc = 0
 * TRY_OR_THROW_NO_RC__ (fn, (1, -1), rc_ok, e);  // OK
 * TRY_OR_THROW__ (fn, (1,  2), rc, rc_ok, e);    // fail
 * ```
 * 
 * [3] Using a "throwable" functor. Once the functor is "done", use it
 *     like any other normal function.
 * 
 * ```
 * int rc_ok = 0;
 * make_throwable<int, int, int> fn_throw(fn, rc_ok, e);
 * fn_throw(1, -1);    // OK,  rc = 0
 * fn_throw(1,  2);    // fail
 * ```
 * 
 * With these three beautiful options at hand, it is up to the user 
 * which one to choose.
 */

#include <exception>
#include <stdexcept>

#ifndef THROWABLE_HPP_
#define THROWABLE_HPP_

namespace cxx_sqlite3
{
/** 
 * Method 1: function wrapper
 * =============================================================
 * 
 * USE: 
 *   call_throwable(fn, rc_ok, exception, arg1, arg2, ...);
 * 
 * fn              : the function to wrap
 * arg1, arg2, ... : arguments to be based to fn (must be enclosed by parenthesis)
 * rc_ok           : value for return code OK
 * exception       : the exception you want to throw if rc != rc_ok
 */
template <typename ReturnType, typename... ArgTypes>
ReturnType call_throwable(ReturnType(*fn)(ArgTypes... args), 
  ReturnType rc_ok, 
  std::runtime_error const &e, 
  ArgTypes... args)
{
  ReturnType rc;
  if ((rc = fn(args...)) != rc_ok) throw e;
  return rc;
}

/** 
 * Method 2: using macros
 * =============================================================
 * 
 * USE: 
 *   TRY_OR_THROW__ (fn, (arg1, arg2, ...), rc, rc_ok, exception);
 * 
 * fn              : the function to wrap
 * arg1, arg2, ... : arguments to be based to fn (must be enclosed by parenthesis)
 * rc              : variable to save to the value returned by fn
 * rc_ok           : value for return code OK
 * exception       : the exception you want to throw if rc != rc_ok
 */
#define TRY_OR_THROW__(fn, args, rc, rc_ok, e) \
if ((rc = fn args) != rc_ok) throw e;
/* this variant does not assign the return value to `rc` */
#define TRY_OR_THROW_NO_RC__(fn, args, rc_ok, e) \
if ((fn args) != rc_ok) throw e;

/**
 * Method 3: "throwable-functor"
 * =============================================================
 * 
 * USE: 
 *   make_throwable<ReturnType, ArgTypes> fn_throw(fn, rc_ok, exception);
 *   fn_throw(arg1, arg2, ...);
 * 
 * fn              : the function to wrap
 * arg1, arg2, ... : arguments to be based to fn (must be enclosed by parenthesis)
 * rc_ok           : value for return code OK
 * exception       : the exception you want to throw if rc != rc_ok
 */

template <typename ReturnType, typename... ArgTypes>
class make_throwable
{
  ReturnType(*fn_)(ArgTypes... args);
  ReturnType rc_ok_;
  std::runtime_error e_;
  
public:
  make_throwable(
    ReturnType(*fn)(ArgTypes... args),     // function to make throwable
    ReturnType rc_ok,                      // return code OK
    std::runtime_error const &e            // exception to throw
  ) : fn_(fn), rc_ok_(rc_ok), e_(e) {}

  ReturnType operator()(ArgTypes... args) const
  {
    ReturnType rc_;
    if ((rc_ = fn_(args...)) != rc_ok_) throw e_;
    return rc_;
  }
};
}
#endif

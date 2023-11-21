/**
 * This header serves to identify under which version of c++ standard
 * is the code being compiled.
 * 
 * __cplusplus 202101L c++23
 * __cplusplus 202002L c++20
 * __cplusplus 201703L c++17
 * __cplusplus 201402L c++14
 * __cplusplus 201103L c++11
 * __cplusplus 199711L c++98
 * else pre-standard c++
 * 
 * @author Yoel Monsalve
 * @date   2023-11-11
 */

#ifndef STD_VERSION_H__
#define STD_VERSION_H_

// macro to determine the C++ standard-compliant version
#if ((defined(_MSVC_LANG) && _MSVC_LANG >= 202101L) || __cplusplus >= 202101L)
#  define __cpp_23_compliant
#endif
#if ((defined(_MSVC_LANG) && _MSVC_LANG >= 202002L) || __cplusplus >= 202002L)
#  define __cpp_20_compliant
#endif
#if ((defined(_MSVC_LANG) && _MSVC_LANG >= 201703L) || __cplusplus >= 201703L)
#  define __cpp_17_compliant
#endif
#if ((defined(_MSVC_LANG) && _MSVC_LANG >= 201402L) || __cplusplus >= 201402L)
#  define __cpp_14_compliant
#endif
#if ((defined(_MSVC_LANG) && _MSVC_LANG >= 201103L) || __cplusplus >= 201103L)
#  define __cpp_11_compliant
#endif
#if ((defined(_MSVC_LANG) && _MSVC_LANG >= 199711L) || __cplusplus >= 199711L)
#  define __cpp_98_compliant
#endif
#if ((defined(_MSVC_LANG) && _MSVC_LANG < 199711L) || __cplusplus < 199711L)
#  define __cpp_pre_standard_compliant
#endif

#endif
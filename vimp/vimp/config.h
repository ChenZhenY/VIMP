/**
 *  @file  config.h
 *  @brief configurations import from CMake
 *  @author Jing Dong
 *  @date  May 21, 2017
 **/

#pragma once


// Whether VIMP is compiled as static or DLL in windows. 
// This will be used to decide whether include __declspec(dllimport) or not in headers
/* #undef VIMP_BUILD_STATIC_LIBRARY */


// Macros for exporting DLL symbols on Windows
// Usage example:
// In header file:
//   class VIMP_EXPORT MyClass { ... };
//   
// Results in the following declarations:
// When included while compiling the VIMP library itself:
//   class __declspec(dllexport) MyClass { ... };
// When included while compiling other code against VIMP:
//   class __declspec(dllimport) MyClass { ... };

#ifdef _WIN32
#  ifdef VIMP_BUILD_STATIC_LIBRARY
#    define VIMP_EXPORT
#    define VIMP_EXTERN_EXPORT extern
#  else /* VIMP_BUILD_STATIC_LIBRARY */
#    ifdef VIMP_EXPORTS
#      define VIMP_EXPORT __declspec(dllexport)
#      define VIMP_EXTERN_EXPORT __declspec(dllexport) extern
#    else /* VIMP_EXPORTS */
#      define VIMP_EXPORT __declspec(dllimport)
#      define VIMP_EXTERN_EXPORT __declspec(dllimport)
#    endif /* VIMP_EXPORTS */
#  endif /* VIMP_BUILD_STATIC_LIBRARY */
#else /* _WIN32 */
#  define VIMP_EXPORT
#  define VIMP_EXTERN_EXPORT extern
#endif

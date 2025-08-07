/* ciconf.h - machine-configurable file for libci
 *
 * libci version 1.6.50.git
 *
 * Copyright (c) 2018-2025 Cosmin Truta
 * Copyright (c) 1998-2002,2004,2006-2016,2018 Glenn Randers-Pehrson
 * Copyright (c) 1996-1997 Andreas Dilger
 * Copyright (c) 1995-1996 Guy Eric Schalnat, Group 42, Inc.
 *
 * This code is released under the libci license.
 * For conditions of distribution and use, see the disclaimer
 * and license in ci.h
 *
 * Any machine specific code is near the front of this file, so if you
 * are configuring libci for a machine, you may want to read the section
 * starting here down to where it starts to typedef ci_color, ci_text,
 * and ci_info.
 */

#ifndef CICONF_H
#define CICONF_H

#ifndef CI_BUILDING_SYMBOL_TABLE /* else includes may cause problems */

/* From libci 1.6.0 libci requires an ANSI X3.159-1989 ("ISOC90") compliant C
 * compiler for correct compilation.  The following header files are required by
 * the standard.  If your compiler doesn't provide these header files, or they
 * do not match the standard, you will need to provide/improve them.
 */
#include <limits.h>
#include <stddef.h>

/* Library header files.  These header files are all defined by ISOC90; libci
 * expects conformant implementations, however, an ISOC90 conformant system need
 * not provide these header files if the functionality cannot be implemented.
 * In this case it will be necessary to disable the relevant parts of libci in
 * the build of cilibconf.h.
 *
 * Prior to 1.6.0 string.h was included here; the API changes in 1.6.0 to not
 * include this unnecessary header file.
 */

#ifdef CI_STDIO_SUPPORTED
   /* Required for the definition of FILE: */
#  include <stdio.h>
#endif

#ifdef CI_SETJMP_SUPPORTED
   /* Required for the definition of jmp_buf and the declaration of longjmp: */
#  include <setjmp.h>
#endif

#ifdef CI_CONVERT_tIME_SUPPORTED
   /* Required for struct tm: */
#  include <time.h>
#endif

#endif /* CI_BUILDING_SYMBOL_TABLE */

/* Prior to 1.6.0, it was possible to turn off 'const' in declarations,
 * using CI_NO_CONST.  This is no longer supported.
 */
#define CI_CONST const /* backward compatibility only */

/* This controls optimization of the reading of 16-bit and 32-bit
 * values from CI files.  It can be set on a per-app-file basis: it
 * just changes whether a macro is used when the function is called.
 * The library builder sets the default; if read functions are not
 * built into the library the macro implementation is forced on.
 */
#ifndef CI_READ_INT_FUNCTIONS_SUPPORTED
#  define CI_USE_READ_MACROS
#endif
#if !defined(CI_NO_USE_READ_MACROS) && !defined(CI_USE_READ_MACROS)
#  if CI_DEFAULT_READ_MACROS
#    define CI_USE_READ_MACROS
#  endif
#endif

/* COMPILER SPECIFIC OPTIONS.
 *
 * These options are provided so that a variety of difficult compilers
 * can be used.  Some are fixed at build time (e.g. CI_API_RULE
 * below) but still have compiler specific implementations, others
 * may be changed on a per-file basis when compiling against libci.
 */

/* The CIARG macro was used in versions of libci prior to 1.6.0 to protect
 * against legacy (pre ISOC90) compilers that did not understand function
 * prototypes.  [Deprecated.]
 */
#ifndef CIARG
#  define CIARG(arglist) arglist
#endif

/* Function calling conventions.
 * =============================
 * Normally it is not necessary to specify to the compiler how to call
 * a function - it just does it - however on x86 systems derived from
 * Microsoft and Borland C compilers ('IBM PC', 'DOS', 'Windows' systems
 * and some others) there are multiple ways to call a function and the
 * default can be changed on the compiler command line.  For this reason
 * libci specifies the calling convention of every exported function and
 * every function called via a user supplied function pointer.  This is
 * done in this file by defining the following macros:
 *
 * CIAPI    Calling convention for exported functions.
 * CICBAPI  Calling convention for user provided (callback) functions.
 * CICAPI   Calling convention used by the ANSI-C library (required
 *           for longjmp callbacks and sometimes used internally to
 *           specify the calling convention for zlib).
 *
 * These macros should never be overridden.  If it is necessary to
 * change calling convention in a private build this can be done
 * by setting CI_API_RULE (which defaults to 0) to one of the values
 * below to select the correct 'API' variants.
 *
 * CI_API_RULE=0 Use CICAPI - the 'C' calling convention - throughout.
 *                This is correct in every known environment.
 * CI_API_RULE=1 Use the operating system convention for CIAPI and
 *                the 'C' calling convention (from CICAPI) for
 *                callbacks (CICBAPI).  This is no longer required
 *                in any known environment - if it has to be used
 *                please post an explanation of the problem to the
 *                libci mailing list.
 *
 * These cases only differ if the operating system does not use the C
 * calling convention, at present this just means the above cases
 * (x86 DOS/Windows systems) and, even then, this does not apply to
 * Cygwin running on those systems.
 *
 * Note that the value must be defined in cilibconf.h so that what
 * the application uses to call the library matches the conventions
 * set when building the library.
 */

/* Symbol export
 * =============
 * When building a shared library it is almost always necessary to tell
 * the compiler which symbols to export.  The ci.h macro 'CI_EXPORT'
 * is used to mark the symbols.  On some systems these symbols can be
 * extracted at link time and need no special processing by the compiler,
 * on other systems the symbols are flagged by the compiler and just
 * the declaration requires a special tag applied (unfortunately) in a
 * compiler dependent way.  Some systems can do either.
 *
 * A small number of older systems also require a symbol from a DLL to
 * be flagged to the program that calls it.  This is a problem because
 * we do not know in the header file included by application code that
 * the symbol will come from a shared library, as opposed to a statically
 * linked one.  For this reason the application must tell us by setting
 * the magic flag CI_USE_DLL to turn on the special processing before
 * it includes ci.h.
 *
 * Four additional macros are used to make this happen:
 *
 * CI_IMPEXP The magic (if any) to cause a symbol to be exported from
 *            the build or imported if CI_USE_DLL is set - compiler
 *            and system specific.
 *
 * CI_EXPORT_TYPE(type) A macro that pre or appends CI_IMPEXP to
 *                       'type', compiler specific.
 *
 * CI_DLL_EXPORT Set to the magic to use during a libci build to
 *                make a symbol exported from the DLL.  Not used in the
 *                public header files; see cipriv.h for how it is used
 *                in the libci build.
 *
 * CI_DLL_IMPORT Set to the magic to force the libci symbols to come
 *                from a DLL - used to define CI_IMPEXP when
 *                CI_USE_DLL is set.
 */

/* System specific discovery.
 * ==========================
 * This code is used at build time to find CI_IMPEXP, the API settings
 * and CI_EXPORT_TYPE(), it may also set a macro to indicate the DLL
 * import processing is possible.  On Windows systems it also sets
 * compiler-specific macros to the values required to change the calling
 * conventions of the various functions.
 */
#if defined(_WIN32) || defined(__WIN32__) || defined(__NT__) || \
    defined(__CYGWIN__)
  /* Windows system (DOS doesn't support DLLs).  Includes builds under Cygwin or
   * MinGW on any architecture currently supported by Windows.  Also includes
   * Watcom builds but these need special treatment because they are not
   * compatible with GCC or Visual C because of different calling conventions.
   */
#  if CI_API_RULE == 2
   /* If this line results in an error, either because __watcall is not
    * understood or because of a redefine just below you cannot use *this*
    * build of the library with the compiler you are using.  *This* build was
    * build using Watcom and applications must also be built using Watcom!
    */
#    define CICAPI __watcall
#  endif

#  if defined(__GNUC__) || (defined(_MSC_VER) && (_MSC_VER >= 800))
#    define CICAPI __cdecl
#    if CI_API_RULE == 1
   /* If this line results in an error __stdcall is not understood and
    * CI_API_RULE should not have been set to '1'.
    */
#      define CIAPI __stdcall
#    endif
#  else
   /* An older compiler, or one not detected (erroneously) above,
    * if necessary override on the command line to get the correct
    * variants for the compiler.
    */
#    ifndef CICAPI
#      define CICAPI _cdecl
#    endif
#    if CI_API_RULE == 1 && !defined(CIAPI)
#      define CIAPI _stdcall
#    endif
#  endif /* compiler/api */

  /* NOTE: CICBAPI always defaults to CICAPI. */

#  if defined(CIAPI) && !defined(CI_USER_PRIVATEBUILD)
#     error CI_USER_PRIVATEBUILD must be defined if CIAPI is changed
#  endif

#  define CI_DLL_EXPORT __declspec(dllexport)
#  ifndef CI_DLL_IMPORT
#    define CI_DLL_IMPORT __declspec(dllimport)
#  endif

#else /* !Windows */
#  if (defined(__IBMC__) || defined(__IBMCPP__)) && defined(__OS2__)
#    define CIAPI _System
#  else /* !Windows/x86 && !OS/2 */
   /* Use the defaults, or define CI*API on the command line (but
    * this will have to be done for every compile!)
    */
#  endif /* other system, !OS/2 */
#endif /* !Windows/x86 */

/* Now do all the defaulting . */
#ifndef CICAPI
#  define CICAPI
#endif
#ifndef CICBAPI
#  define CICBAPI CICAPI
#endif
#ifndef CIAPI
#  define CIAPI CICAPI
#endif

/* CI_IMPEXP may be set on the compilation system command line or (if not set)
 * then in an internal header file when building the library, otherwise (when
 * using the library) it is set here.
 */
#ifndef CI_IMPEXP
#  if defined(CI_USE_DLL) && defined(CI_DLL_IMPORT)
   /* This forces use of a DLL, disallowing static linking */
#    define CI_IMPEXP CI_DLL_IMPORT
#  endif

#  ifndef CI_IMPEXP
#    define CI_IMPEXP
#  endif
#endif

/* In 1.5.2 the definition of CI_FUNCTION has been changed to always treat
 * 'attributes' as a storage class - the attributes go at the start of the
 * function definition, and attributes are always appended regardless of the
 * compiler.  This considerably simplifies these macros but may cause problems
 * if any compilers both need function attributes and fail to handle them as
 * a storage class (this is unlikely.)
 */
#ifndef CI_FUNCTION
#  define CI_FUNCTION(type, name, args, attributes) attributes type name args
#endif

#ifndef CI_EXPORT_TYPE
#  define CI_EXPORT_TYPE(type) CI_IMPEXP type
#endif

   /* The ordinal value is only relevant when preprocessing ci.h for symbol
    * table entries, so we discard it here.  See the .dfn files in the
    * scripts directory.
    */

#ifndef CI_EXPORTA
#  define CI_EXPORTA(ordinal, type, name, args, attributes) \
      CI_FUNCTION(CI_EXPORT_TYPE(type), (CIAPI name), args, \
      CI_LINKAGE_API attributes)
#endif

/* ANSI-C (C90) does not permit a macro to be invoked with an empty argument,
 * so make something non-empty to satisfy the requirement:
 */
#define CI_EMPTY /*empty list*/

#define CI_EXPORT(ordinal, type, name, args) \
   CI_EXPORTA(ordinal, type, name, args, CI_EMPTY)

/* Use CI_REMOVED to comment out a removed interface. */
#ifndef CI_REMOVED
#  define CI_REMOVED(ordinal, type, name, args, attributes)
#endif

#ifndef CI_CALLBACK
#  define CI_CALLBACK(type, name, args) type (CICBAPI name) args
#endif

/* Support for compiler specific function attributes.  These are used
 * so that where compiler support is available incorrect use of API
 * functions in ci.h will generate compiler warnings.
 *
 * Added at libci-1.2.41.
 */

#ifndef CI_NO_PEDANTIC_WARNINGS
#  ifndef CI_PEDANTIC_WARNINGS_SUPPORTED
#    define CI_PEDANTIC_WARNINGS_SUPPORTED
#  endif
#endif

#ifdef CI_PEDANTIC_WARNINGS_SUPPORTED
  /* Support for compiler specific function attributes.  These are used
   * so that where compiler support is available, incorrect use of API
   * functions in ci.h will generate compiler warnings.  Added at libci
   * version 1.2.41.  Disabling these removes the warnings but may also produce
   * less efficient code.
   */
#  if defined(__clang__) && defined(__has_attribute)
   /* Clang defines both __clang__ and __GNUC__. Check __clang__ first. */
#    if !defined(CI_USE_RESULT) && __has_attribute(__warn_unused_result__)
#      define CI_USE_RESULT __attribute__((__warn_unused_result__))
#    endif
#    if !defined(CI_NORETURN) && __has_attribute(__noreturn__)
#      define CI_NORETURN __attribute__((__noreturn__))
#    endif
#    if !defined(CI_ALLOCATED) && __has_attribute(__malloc__)
#      define CI_ALLOCATED __attribute__((__malloc__))
#    endif
#    if !defined(CI_DEPRECATED) && __has_attribute(__deprecated__)
#      define CI_DEPRECATED __attribute__((__deprecated__))
#    endif
#    if !defined(CI_PRIVATE)
#      ifdef __has_extension
#        if __has_extension(attribute_unavailable_with_message)
#          define CI_PRIVATE __attribute__((__unavailable__(\
             "This function is not exported by libci.")))
#        endif
#      endif
#    endif
#    ifndef CI_RESTRICT
#      define CI_RESTRICT __restrict
#    endif

#  elif defined(__GNUC__)
#    ifndef CI_USE_RESULT
#      define CI_USE_RESULT __attribute__((__warn_unused_result__))
#    endif
#    ifndef CI_NORETURN
#      define CI_NORETURN   __attribute__((__noreturn__))
#    endif
#    if __GNUC__ >= 3
#      ifndef CI_ALLOCATED
#        define CI_ALLOCATED  __attribute__((__malloc__))
#      endif
#      ifndef CI_DEPRECATED
#        define CI_DEPRECATED __attribute__((__deprecated__))
#      endif
#      ifndef CI_PRIVATE
#        if 0 /* Doesn't work so we use deprecated instead*/
#          define CI_PRIVATE \
            __attribute__((warning("This function is not exported by libci.")))
#        else
#          define CI_PRIVATE \
            __attribute__((__deprecated__))
#        endif
#      endif
#      if ((__GNUC__ > 3) || !defined(__GNUC_MINOR__) || (__GNUC_MINOR__ >= 1))
#        ifndef CI_RESTRICT
#          define CI_RESTRICT __restrict
#        endif
#      endif /* __GNUC__.__GNUC_MINOR__ > 3.0 */
#    endif /* __GNUC__ >= 3 */

#  elif defined(_MSC_VER)  && (_MSC_VER >= 1300)
#    ifndef CI_USE_RESULT
#      define CI_USE_RESULT /* not supported */
#    endif
#    ifndef CI_NORETURN
#      define CI_NORETURN   __declspec(noreturn)
#    endif
#    ifndef CI_ALLOCATED
#      if (_MSC_VER >= 1400)
#        define CI_ALLOCATED __declspec(restrict)
#      endif
#    endif
#    ifndef CI_DEPRECATED
#      define CI_DEPRECATED __declspec(deprecated)
#    endif
#    ifndef CI_PRIVATE
#      define CI_PRIVATE __declspec(deprecated)
#    endif
#    ifndef CI_RESTRICT
#      if (_MSC_VER >= 1400)
#        define CI_RESTRICT __restrict
#      endif
#    endif

#  elif defined(__WATCOMC__)
#    ifndef CI_RESTRICT
#      define CI_RESTRICT __restrict
#    endif
#  endif
#endif /* CI_PEDANTIC_WARNINGS */

#ifndef CI_DEPRECATED
#  define CI_DEPRECATED  /* Use of this function is deprecated */
#endif
#ifndef CI_USE_RESULT
#  define CI_USE_RESULT  /* The result of this function must be checked */
#endif
#ifndef CI_NORETURN
#  define CI_NORETURN    /* This function does not return */
#endif
#ifndef CI_ALLOCATED
#  define CI_ALLOCATED   /* The result of the function is new memory */
#endif
#ifndef CI_PRIVATE
#  define CI_PRIVATE     /* This is a private libci function */
#endif
#ifndef CI_RESTRICT
#  define CI_RESTRICT    /* The C99 "restrict" feature */
#endif

#ifndef CI_FP_EXPORT     /* A floating point API. */
#  ifdef CI_FLOATING_POINT_SUPPORTED
#     define CI_FP_EXPORT(ordinal, type, name, args)\
         CI_EXPORT(ordinal, type, name, args);
#  else                   /* No floating point APIs */
#     define CI_FP_EXPORT(ordinal, type, name, args)
#  endif
#endif
#ifndef CI_FIXED_EXPORT  /* A fixed point API. */
#  ifdef CI_FIXED_POINT_SUPPORTED
#     define CI_FIXED_EXPORT(ordinal, type, name, args)\
         CI_EXPORT(ordinal, type, name, args);
#  else                   /* No fixed point APIs */
#     define CI_FIXED_EXPORT(ordinal, type, name, args)
#  endif
#endif

#ifndef CI_BUILDING_SYMBOL_TABLE
/* Some typedefs to get us started.  These should be safe on most of the common
 * platforms.
 *
 * ci_uint_32 and ci_int_32 may, currently, be larger than required to hold a
 * 32-bit value however this is not normally advisable.
 *
 * ci_uint_16 and ci_int_16 should always be two bytes in size - this is
 * verified at library build time.
 *
 * ci_byte must always be one byte in size.
 *
 * The checks below use constants from limits.h, as defined by the ISOC90
 * standard.
 */
#if CHAR_BIT == 8 && UCHAR_MAX == 255
   typedef unsigned char ci_byte;
#else
#  error libci requires 8-bit bytes
#endif

#if INT_MIN == -32768 && INT_MAX == 32767
   typedef int ci_int_16;
#elif SHRT_MIN == -32768 && SHRT_MAX == 32767
   typedef short ci_int_16;
#else
#  error libci requires a signed 16-bit integer type
#endif

#if UINT_MAX == 65535
   typedef unsigned int ci_uint_16;
#elif USHRT_MAX == 65535
   typedef unsigned short ci_uint_16;
#else
#  error libci requires an unsigned 16-bit integer type
#endif

#if INT_MIN < -2147483646 && INT_MAX > 2147483646
   typedef int ci_int_32;
#elif LONG_MIN < -2147483646 && LONG_MAX > 2147483646
   typedef long int ci_int_32;
#else
#  error libci requires a signed 32-bit (or longer) integer type
#endif

#if UINT_MAX > 4294967294U
   typedef unsigned int ci_uint_32;
#elif ULONG_MAX > 4294967294U
   typedef unsigned long int ci_uint_32;
#else
#  error libci requires an unsigned 32-bit (or longer) integer type
#endif

/* Prior to 1.6.0, it was possible to disable the use of size_t and ptrdiff_t.
 * From 1.6.0 onwards, an ISO C90 compiler, as well as a standard-compliant
 * behavior of sizeof and ptrdiff_t are required.
 * The legacy typedefs are provided here for backwards compatibility.
 */
typedef size_t ci_size_t;
typedef ptrdiff_t ci_ptrdiff_t;

/* libci needs to know the maximum value of 'size_t' and this controls the
 * definition of ci_alloc_size_t, below.  This maximum value of size_t limits
 * but does not control the maximum allocations the library makes - there is
 * direct application control of this through ci_set_user_limits().
 */
#ifndef CI_SMALL_SIZE_T
   /* Compiler specific tests for systems where size_t is known to be less than
    * 32 bits (some of these systems may no longer work because of the lack of
    * 'far' support; see above.)
    */
#  if (defined(__TURBOC__) && !defined(__FLAT__)) ||\
   (defined(_MSC_VER) && defined(MAXSEG_64K))
#     define CI_SMALL_SIZE_T
#  endif
#endif

/* ci_alloc_size_t is guaranteed to be no smaller than size_t, and no smaller
 * than ci_uint_32.  Casts from size_t or ci_uint_32 to ci_alloc_size_t are
 * not necessary; in fact, it is recommended not to use them at all, so that
 * the compiler can complain when something turns out to be problematic.
 *
 * Casts in the other direction (from ci_alloc_size_t to size_t or
 * ci_uint_32) should be explicitly applied; however, we do not expect to
 * encounter practical situations that require such conversions.
 *
 * CI_SMALL_SIZE_T must be defined if the maximum value of size_t is less than
 * 4294967295 - i.e. less than the maximum value of ci_uint_32.
 */
#ifdef CI_SMALL_SIZE_T
   typedef ci_uint_32 ci_alloc_size_t;
#else
   typedef size_t ci_alloc_size_t;
#endif

/* Prior to 1.6.0 libci offered limited support for Microsoft C compiler
 * implementations of Intel CPU specific support of user-mode segmented address
 * spaces, where 16-bit pointers address more than 65536 bytes of memory using
 * separate 'segment' registers.  The implementation requires two different
 * types of pointer (only one of which includes the segment value.)
 *
 * If required this support is available in version 1.2 of libci and may be
 * available in versions through 1.5, although the correctness of the code has
 * not been verified recently.
 */

/* Typedef for floating-point numbers that are converted to fixed-point with a
 * multiple of 100,000, e.g., gamma
 */
typedef ci_int_32 ci_fixed_point;

/* Add typedefs for pointers */
typedef void                  * ci_voidp;
typedef const void            * ci_const_voidp;
typedef ci_byte              * ci_bytep;
typedef const ci_byte        * ci_const_bytep;
typedef ci_uint_32           * ci_uint_32p;
typedef const ci_uint_32     * ci_const_uint_32p;
typedef ci_int_32            * ci_int_32p;
typedef const ci_int_32      * ci_const_int_32p;
typedef ci_uint_16           * ci_uint_16p;
typedef const ci_uint_16     * ci_const_uint_16p;
typedef ci_int_16            * ci_int_16p;
typedef const ci_int_16      * ci_const_int_16p;
typedef char                  * ci_charp;
typedef const char            * ci_const_charp;
typedef ci_fixed_point       * ci_fixed_point_p;
typedef const ci_fixed_point * ci_const_fixed_point_p;
typedef size_t                * ci_size_tp;
typedef const size_t          * ci_const_size_tp;

#ifdef CI_FLOATING_POINT_SUPPORTED
typedef double       * ci_doublep;
typedef const double * ci_const_doublep;
#endif

/* Pointers to pointers; i.e. arrays */
typedef ci_byte        * * ci_bytepp;
typedef ci_uint_32     * * ci_uint_32pp;
typedef ci_int_32      * * ci_int_32pp;
typedef ci_uint_16     * * ci_uint_16pp;
typedef ci_int_16      * * ci_int_16pp;
typedef const char      * * ci_const_charpp;
typedef char            * * ci_charpp;
typedef ci_fixed_point * * ci_fixed_point_pp;
#ifdef CI_FLOATING_POINT_SUPPORTED
typedef double          * * ci_doublepp;
#endif

/* Pointers to pointers to pointers; i.e., pointer to array */
typedef char            * * * ci_charppp;

#ifdef CI_STDIO_SUPPORTED
/* With CI_STDIO_SUPPORTED it was possible to use I/O streams that were
 * not necessarily stdio FILE streams, to allow building Windows applications
 * before Win32 and Windows CE applications before WinCE 3.0, but that kind
 * of support has long been discontinued.
 */
typedef FILE            * ci_FILE_p; /* [Deprecated] */
#endif

#endif /* CI_BUILDING_SYMBOL_TABLE */

#endif /* CICONF_H */

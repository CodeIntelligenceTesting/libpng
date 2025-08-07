/* cipriv.h - private declarations for use inside libci
 *
 * Copyright (c) 2018-2025 Cosmin Truta
 * Copyright (c) 1998-2002,2004,2006-2018 Glenn Randers-Pehrson
 * Copyright (c) 1996-1997 Andreas Dilger
 * Copyright (c) 1995-1996 Guy Eric Schalnat, Group 42, Inc.
 *
 * This code is released under the libci license.
 * For conditions of distribution and use, see the disclaimer
 * and license in ci.h
 */

/* The symbols declared in this file (including the functions declared
 * as extern) are PRIVATE.  They are not part of the libci public
 * interface, and are not recommended for use by regular applications.
 * Some of them may become public in the future; others may stay private,
 * change in an incompatible way, or even disappear.
 * Although the libci users are not forbidden to include this header,
 * they should be well aware of the issues that may arise from doing so.
 */


/* cipriv.h must be included first in each translation unit inside libci.
 * On the other hand, it must not be included at all, directly or indirectly,
 * by any application code that uses the libci API.
 */
#ifndef CIPRIV_H
#  define CIPRIV_H
#else
#  error Duplicate inclusion of cipriv.h; please check the libci source files
#endif

#if defined(CI_H) || defined(CICONF_H) || defined(CILCONF_H)
#  error This file must not be included by applications; please include <ci.h>
#endif

/* Feature Test Macros.  The following are defined here to ensure that correctly
 * implemented libraries reveal the APIs libci needs to build and hide those
 * that are not needed and potentially damaging to the compilation.
 *
 * Feature Test Macros must be defined before any system header is included (see
 * POSIX 1003.1 2.8.2 "POSIX Symbols."
 *
 * These macros only have an effect if the operating system supports either
 * POSIX 1003.1 or C99, or both.  On other operating systems (particularly
 * Windows/Visual Studio) there is no effect; the OS specific tests below are
 * still required (as of 2011-05-02.)
 */
#ifndef _POSIX_SOURCE
#  define _POSIX_SOURCE 1 /* Just the POSIX 1003.1 and C89 APIs */
#endif

#ifndef CI_VERSION_INFO_ONLY
/* Standard library headers not required by ci.h: */
#  include <stdlib.h>
#  include <string.h>
#endif

#define CILIB_BUILD /*libci is being built, not used*/

/* If HAVE_CONFIG_H is defined during the build then the build system must
 * provide an appropriate "config.h" file on the include path.  The header file
 * must provide definitions as required below (search for "HAVE_CONFIG_H");
 * see configure.ac for more details of the requirements.  The macro
 * "CI_NO_CONFIG_H" is provided for maintainers to test for dependencies on
 * 'configure'; define this macro to prevent the configure build including the
 * configure generated config.h.  Libci is expected to compile without *any*
 * special build system support on a reasonably ANSI-C compliant system.
 */
#if defined(HAVE_CONFIG_H) && !defined(CI_NO_CONFIG_H)
#  include <config.h>
   /* Pick up the definition of 'restrict' from config.h if it was read: */
#  define CI_RESTRICT restrict
#endif

/* To support symbol prefixing it is necessary to know *before* including ci.h
 * whether the fixed point (and maybe other) APIs are exported, because if they
 * are not internal definitions may be required.  This is handled below just
 * before ci.h is included, but load the configuration now if it is available.
 */
#include "cilibconf.h"

/* Local renames may change non-exported API functions from ci.h */
#if defined(CI_PREFIX) && !defined(CIPREFIX_H)
#  include "ciprefix.h"
#endif

#ifdef CI_USER_CONFIG
#  include "ciusr.h"
   /* These should have been defined in ciusr.h */
#  ifndef CI_USER_PRIVATEBUILD
#    define CI_USER_PRIVATEBUILD "Custom libci build"
#  endif
#  ifndef CI_USER_DLLFNAME_POSTFIX
#    define CI_USER_DLLFNAME_POSTFIX "Cb"
#  endif
#endif

/* Compile time options.
 * =====================
 * In a multi-arch build the compiler may compile the code several times for the
 * same object module, producing different binaries for different architectures.
 * When this happens configure-time setting of the target host options cannot be
 * done and this interferes with the handling of the ARM NEON optimizations, and
 * possibly other similar optimizations.  Put additional tests here; in general
 * this is needed when the same option can be changed at both compile time and
 * run time depending on the target OS (i.e. iOS vs Android.)
 *
 * NOTE: symbol prefixing does not pass $(CFLAGS) to the preprocessor, because
 * this is not possible with certain compilers (Oracle SUN OS CC), as a result
 * it is necessary to ensure that all extern functions that *might* be used
 * regardless of $(CFLAGS) get declared in this file.  The test on __ARM_NEON__
 * below is one example of this behavior because it is controlled by the
 * presence or not of -mfpu=neon on the GCC command line, it is possible to do
 * this in $(CC), e.g. "CC=gcc -mfpu=neon", but people who build libci rarely
 * do this.
 */
#ifndef CI_ARM_NEON_OPT
   /* ARM NEON optimizations are being controlled by the compiler settings,
    * typically the target FPU.  If the FPU has been set to NEON (-mfpu=neon
    * with GCC) then the compiler will define __ARM_NEON__ and we can rely
    * unconditionally on NEON instructions not crashing, otherwise we must
    * disable use of NEON instructions.
    *
    * NOTE: at present these optimizations depend on 'ALIGNED_MEMORY', so they
    * can only be turned on automatically if that is supported too.  If
    * CI_ARM_NEON_OPT is set in CPPFLAGS (to >0) then arm/arm_init.c will fail
    * to compile with an appropriate #error if ALIGNED_MEMORY has been turned
    * off.
    *
    * Note that gcc-4.9 defines __ARM_NEON instead of the deprecated
    * __ARM_NEON__, so we check both variants.
    *
    * To disable ARM_NEON optimizations entirely, and skip compiling the
    * associated assembler code, pass --enable-arm-neon=no to configure
    * or put -DCI_ARM_NEON_OPT=0 in CPPFLAGS.
    */
#  if (defined(__ARM_NEON__) || defined(__ARM_NEON)) && \
   defined(CI_ALIGNED_MEMORY_SUPPORTED)
#     define CI_ARM_NEON_OPT 2
#  else
#     define CI_ARM_NEON_OPT 0
#  endif
#endif

#ifndef CI_RISCV_RVV_OPT
   /* RISCV_RVV optimizations are being controlled by the compiler settings,
    * typically the target compiler will define __riscv but the rvv extension
    * availability has to be explicitly stated. This is why if no
    * CI_RISCV_RVV_OPT was defined then a runtime check will be executed.
    *
    * To enable RISCV_RVV optimizations unconditionally, and compile the
    * associated code, pass --enable-riscv-rvv=yes or --enable-riscv-rvv=on
    * to configure or put -DCI_RISCV_RVV_OPT=2 in CPPFLAGS.
    */

#  define CI_RISCV_RVV_OPT 0
#endif

#if CI_ARM_NEON_OPT > 0
   /* NEON optimizations are to be at least considered by libci, so enable the
    * callbacks to do this.
    */
#  define CI_FILTER_OPTIMIZATIONS ci_init_filter_functions_neon
#  ifndef CI_ARM_NEON_IMPLEMENTATION
      /* Use the intrinsics code by default. */
#     define CI_ARM_NEON_IMPLEMENTATION 1
#  endif
#else /* CI_ARM_NEON_OPT == 0 */
#     define CI_ARM_NEON_IMPLEMENTATION 0
#endif /* CI_ARM_NEON_OPT > 0 */

#ifndef CI_MIPS_MSA_OPT
#  if defined(__mips_msa) && (__mips_isa_rev >= 5) && \
   defined(CI_ALIGNED_MEMORY_SUPPORTED)
#     define CI_MIPS_MSA_OPT 2
#  else
#     define CI_MIPS_MSA_OPT 0
#  endif
#endif

#ifndef CI_MIPS_MMI_OPT
#  ifdef CI_MIPS_MMI
#    if defined(__mips_loongson_mmi) && (_MIPS_SIM == _ABI64) && \
     defined(CI_ALIGNED_MEMORY_SUPPORTED)
#       define CI_MIPS_MMI_OPT 1
#    else
#       define CI_MIPS_MMI_OPT 0
#    endif
#  else
#    define CI_MIPS_MMI_OPT 0
#  endif
#endif

#ifndef CI_POWERPC_VSX_OPT
#  if defined(__PPC64__) && defined(__ALTIVEC__) && defined(__VSX__)
#     define CI_POWERPC_VSX_OPT 2
#  else
#     define CI_POWERPC_VSX_OPT 0
#  endif
#endif

#ifndef CI_LOONGARCH_LSX_OPT
#  if defined(__loongarch_sx)
#     define CI_LOONGARCH_LSX_OPT 1
#  else
#     define CI_LOONGARCH_LSX_OPT 0
#  endif
#endif

#ifndef CI_INTEL_SSE_OPT
#   ifdef CI_INTEL_SSE
      /* Only check for SSE if the build configuration has been modified to
       * enable SSE optimizations.  This means that these optimizations will
       * be off by default.  See contrib/intel for more details.
       */
#      if defined(__SSE4_1__) || defined(__AVX__) || defined(__SSSE3__) || \
       defined(__SSE2__) || defined(_M_X64) || defined(_M_AMD64) || \
       (defined(_M_IX86_FP) && _M_IX86_FP >= 2)
#         define CI_INTEL_SSE_OPT 1
#      else
#         define CI_INTEL_SSE_OPT 0
#      endif
#   else
#      define CI_INTEL_SSE_OPT 0
#   endif
#endif

#if CI_INTEL_SSE_OPT > 0
#   ifndef CI_INTEL_SSE_IMPLEMENTATION
#      if defined(__SSE4_1__) || defined(__AVX__)
          /* We are not actually using AVX, but checking for AVX is the best
             way we can detect SSE4.1 and SSSE3 on MSVC.
          */
#         define CI_INTEL_SSE_IMPLEMENTATION 3
#      elif defined(__SSSE3__)
#         define CI_INTEL_SSE_IMPLEMENTATION 2
#      elif defined(__SSE2__) || defined(_M_X64) || defined(_M_AMD64) || \
       (defined(_M_IX86_FP) && _M_IX86_FP >= 2)
#         define CI_INTEL_SSE_IMPLEMENTATION 1
#      else
#         define CI_INTEL_SSE_IMPLEMENTATION 0
#      endif
#   endif

#   if CI_INTEL_SSE_IMPLEMENTATION > 0
#      define CI_FILTER_OPTIMIZATIONS ci_init_filter_functions_sse2
#   endif
#else
#   define CI_INTEL_SSE_IMPLEMENTATION 0
#endif

#if CI_MIPS_MSA_OPT > 0
#  ifndef CI_MIPS_MSA_IMPLEMENTATION
#     if defined(__mips_msa)
#        if defined(__clang__)
#        elif defined(__GNUC__)
#           if __GNUC__ < 4 || (__GNUC__ == 4 && __GNUC_MINOR__ < 7)
#              define CI_MIPS_MSA_IMPLEMENTATION 2
#           endif /* no GNUC support */
#        endif /* __GNUC__ */
#     else /* !defined __mips_msa */
#        define CI_MIPS_MSA_IMPLEMENTATION 2
#     endif /* __mips_msa */
#  endif /* !CI_MIPS_MSA_IMPLEMENTATION */

#  ifndef CI_MIPS_MSA_IMPLEMENTATION
#     define CI_MIPS_MSA_IMPLEMENTATION 1
#     define CI_FILTER_OPTIMIZATIONS ci_init_filter_functions_mips
#  endif
#else
#  define CI_MIPS_MSA_IMPLEMENTATION 0
#endif /* CI_MIPS_MSA_OPT > 0 */

#if CI_MIPS_MMI_OPT > 0
#  ifndef CI_MIPS_MMI_IMPLEMENTATION
#     if defined(__mips_loongson_mmi) && (_MIPS_SIM == _ABI64)
#        define CI_MIPS_MMI_IMPLEMENTATION 2
#     else /* !defined __mips_loongson_mmi  || _MIPS_SIM != _ABI64 */
#        define CI_MIPS_MMI_IMPLEMENTATION 0
#     endif /* __mips_loongson_mmi  && _MIPS_SIM == _ABI64 */
#  endif /* !CI_MIPS_MMI_IMPLEMENTATION */

#   if CI_MIPS_MMI_IMPLEMENTATION > 0
#      define CI_FILTER_OPTIMIZATIONS ci_init_filter_functions_mips
#   endif
#else
#   define CI_MIPS_MMI_IMPLEMENTATION 0
#endif /* CI_MIPS_MMI_OPT > 0 */

#if CI_POWERPC_VSX_OPT > 0
#  define CI_FILTER_OPTIMIZATIONS ci_init_filter_functions_vsx
#  define CI_POWERPC_VSX_IMPLEMENTATION 1
#else
#  define CI_POWERPC_VSX_IMPLEMENTATION 0
#endif

#if CI_LOONGARCH_LSX_OPT > 0
#   define CI_FILTER_OPTIMIZATIONS ci_init_filter_functions_lsx
#   define CI_LOONGARCH_LSX_IMPLEMENTATION 1
#else
#   define CI_LOONGARCH_LSX_IMPLEMENTATION 0
#endif

#if CI_RISCV_RVV_OPT > 0
#  define CI_FILTER_OPTIMIZATIONS ci_init_filter_functions_rvv
#  ifndef CI_RISCV_RVV_IMPLEMENTATION
      /* Use the intrinsics code by default. */
#     define CI_RISCV_RVV_IMPLEMENTATION 1
#  endif
#else
#  define CI_RISCV_RVV_IMPLEMENTATION 0
#endif

/* Is this a build of a DLL where compilation of the object modules requires
 * different preprocessor settings to those required for a simple library?  If
 * so CI_BUILD_DLL must be set.
 *
 * If libci is used inside a DLL but that DLL does not export the libci APIs
 * CI_BUILD_DLL must not be set.  To avoid the code below kicking in build a
 * static library of libci then link the DLL against that.
 */
#ifndef CI_BUILD_DLL
#  ifdef DLL_EXPORT
      /* This is set by libtool when files are compiled for a DLL; libtool
       * always compiles twice, even on systems where it isn't necessary.  Set
       * CI_BUILD_DLL in case it is necessary:
       */
#     define CI_BUILD_DLL
#  else
#     ifdef _WINDLL
         /* This is set by the Microsoft Visual Studio IDE in projects that
          * build a DLL.  It can't easily be removed from those projects (it
          * isn't visible in the Visual Studio UI) so it is a fairly reliable
          * indication that CI_IMPEXP needs to be set to the DLL export
          * attributes.
          */
#        define CI_BUILD_DLL
#     else
#        ifdef __DLL__
            /* This is set by the Borland C system when compiling for a DLL
             * (as above.)
             */
#           define CI_BUILD_DLL
#        else
            /* Add additional compiler cases here. */
#        endif
#     endif
#  endif
#endif /* Setting CI_BUILD_DLL if required */

/* See ciconf.h for more details: the builder of the library may set this on
 * the command line to the right thing for the specific compilation system or it
 * may be automagically set above (at present we know of no system where it does
 * need to be set on the command line.)
 *
 * CI_IMPEXP must be set here when building the library to prevent ciconf.h
 * setting it to the "import" setting for a DLL build.
 */
#ifndef CI_IMPEXP
#  ifdef CI_BUILD_DLL
#     define CI_IMPEXP CI_DLL_EXPORT
#  else
      /* Not building a DLL, or the DLL doesn't require specific export
       * definitions.
       */
#     define CI_IMPEXP
#  endif
#endif

/* No warnings for private or deprecated functions in the build: */
#ifndef CI_DEPRECATED
#  define CI_DEPRECATED
#endif
#ifndef CI_PRIVATE
#  define CI_PRIVATE
#endif

/* Symbol preprocessing support.
 *
 * To enable listing global, but internal, symbols the following macros should
 * always be used to declare an extern data or function object in this file.
 */
#ifndef CI_INTERNAL_DATA
#  define CI_INTERNAL_DATA(type, name, array) CI_LINKAGE_DATA type name array
#endif

#ifndef CI_INTERNAL_FUNCTION
#  define CI_INTERNAL_FUNCTION(type, name, args, attributes)\
      CI_LINKAGE_FUNCTION CI_FUNCTION(type, name, args, CI_EMPTY attributes)
#endif

#ifndef CI_INTERNAL_CALLBACK
#  define CI_INTERNAL_CALLBACK(type, name, args, attributes)\
      CI_LINKAGE_CALLBACK CI_FUNCTION(type, (CICBAPI name), args,\
         CI_EMPTY attributes)
#endif

/* If floating or fixed point APIs are disabled they may still be compiled
 * internally.  To handle this make sure they are declared as the appropriate
 * internal extern function (otherwise the symbol prefixing stuff won't work and
 * the functions will be used without definitions.)
 *
 * NOTE: although all the API functions are declared here they are not all
 * actually built!  Because the declarations are still made it is necessary to
 * fake out types that they depend on.
 */
#ifndef CI_FP_EXPORT
#  ifndef CI_FLOATING_POINT_SUPPORTED
#     define CI_FP_EXPORT(ordinal, type, name, args)\
         CI_INTERNAL_FUNCTION(type, name, args, CI_EMPTY);
#     ifndef CI_VERSION_INFO_ONLY
         typedef struct ci_incomplete ci_double;
         typedef ci_double*           ci_doublep;
         typedef const ci_double*     ci_const_doublep;
         typedef ci_double**          ci_doublepp;
#     endif
#  endif
#endif
#ifndef CI_FIXED_EXPORT
#  ifndef CI_FIXED_POINT_SUPPORTED
#     define CI_FIXED_EXPORT(ordinal, type, name, args)\
         CI_INTERNAL_FUNCTION(type, name, args, CI_EMPTY);
#  endif
#endif

#include "ci.h"

/* ciconf.h does not set CI_DLL_EXPORT unless it is required, so: */
#ifndef CI_DLL_EXPORT
#  define CI_DLL_EXPORT
#endif

/* This is a global switch to set the compilation for an installed system
 * (a release build).  It can be set for testing debug builds to ensure that
 * they will compile when the build type is switched to RC or STABLE, the
 * default is just to use CI_LIBCI_BUILD_BASE_TYPE.  Set this in CPPFLAGS
 * with either:
 *
 *   -DCI_RELEASE_BUILD Turns on the release compile path
 *   -DCI_RELEASE_BUILD=0 Turns it off
 * or in your ciusr.h with
 *   #define CI_RELEASE_BUILD=1 Turns on the release compile path
 *   #define CI_RELEASE_BUILD=0 Turns it off
 */
#ifndef CI_RELEASE_BUILD
#  define CI_RELEASE_BUILD (CI_LIBCI_BUILD_BASE_TYPE >= CI_LIBCI_BUILD_RC)
#endif

/* SECURITY and SAFETY:
 *
 * libci is built with support for internal limits on image dimensions and
 * memory usage.  These are documented in scripts/cilibconf.dfa of the
 * source and recorded in the machine generated header file cilibconf.h.
 */

/* If you are running on a machine where you cannot allocate more
 * than 64K of memory at once, uncomment this.  While libci will not
 * normally need that much memory in a chunk (unless you load up a very
 * large file), zlib needs to know how big of a chunk it can use, and
 * libci thus makes sure to check any memory allocation to verify it
 * will fit into memory.
 *
 * zlib provides 'MAXSEG_64K' which, if defined, indicates the
 * same limit and ciconf.h (already included) sets the limit
 * if certain operating systems are detected.
 */
#if defined(MAXSEG_64K) && !defined(CI_MAX_MALLOC_64K)
#  define CI_MAX_MALLOC_64K
#endif

#ifndef CI_UNUSED
/* Unused formal parameter warnings are silenced using the following macro
 * which is expected to have no bad effects on performance (optimizing
 * compilers will probably remove it entirely).  Note that if you replace
 * it with something other than whitespace, you must include the terminating
 * semicolon.
 */
#  define CI_UNUSED(param) (void)param;
#endif

/* Just a little check that someone hasn't tried to define something
 * contradictory.
 */
#if (CI_ZBUF_SIZE > 65536L) && defined(CI_MAX_MALLOC_64K)
#  undef CI_ZBUF_SIZE
#  define CI_ZBUF_SIZE 65536L
#endif

/* If warnings or errors are turned off the code is disabled or redirected here.
 * From 1.5.4 functions have been added to allow very limited formatting of
 * error and warning messages - this code will also be disabled here.
 */
#ifdef CI_WARNINGS_SUPPORTED
#  define CI_WARNING_PARAMETERS(p) ci_warning_parameters p;
#else
#  define ci_warning_parameter(p,number,string) ((void)0)
#  define ci_warning_parameter_unsigned(p,number,format,value) ((void)0)
#  define ci_warning_parameter_signed(p,number,format,value) ((void)0)
#  define ci_formatted_warning(pp,p,message) ((void)(pp))
#  define CI_WARNING_PARAMETERS(p)
#endif
#ifndef CI_ERROR_TEXT_SUPPORTED
#  define ci_fixed_error(s1,s2) ci_err(s1)
#endif

/* Some fixed point APIs are still required even if not exported because
 * they get used by the corresponding floating point APIs.  This magic
 * deals with this:
 */
#ifdef CI_FIXED_POINT_SUPPORTED
#  define CIFAPI CIAPI
#else
#  define CIFAPI /* PRIVATE */
#endif

#ifndef CI_VERSION_INFO_ONLY
/* Other defines specific to compilers can go here.  Try to keep
 * them inside an appropriate ifdef/endif pair for portability.
 */

/* C allows up-casts from (void*) to any pointer and (const void*) to any
 * pointer to a const object.  C++ regards this as a type error and requires an
 * explicit, static, cast and provides the static_cast<> rune to ensure that
 * const is not cast away.
 */
#ifdef __cplusplus
#  define ci_voidcast(type, value) static_cast<type>(value)
#  define ci_constcast(type, value) const_cast<type>(value)
#  define ci_aligncast(type, value) \
   static_cast<type>(static_cast<void*>(value))
#  define ci_aligncastconst(type, value) \
   static_cast<type>(static_cast<const void*>(value))
#else
#  define ci_voidcast(type, value) (value)
#  define ci_constcast(type, value) ((type)(void*)(const void*)(value))
#  define ci_aligncast(type, value) ((void*)(value))
#  define ci_aligncastconst(type, value) ((const void*)(value))
#endif /* __cplusplus */

#if defined(CI_FLOATING_POINT_SUPPORTED) ||\
    defined(CI_FLOATING_ARITHMETIC_SUPPORTED)
   /* ci.c requires the following ANSI-C constants if the conversion of
    * floating point to ASCII is implemented therein:
    *
    *  DBL_DIG  Maximum number of decimal digits (can be set to any constant)
    *  DBL_MIN  Smallest normalized fp number (can be set to an arbitrary value)
    *  DBL_MAX  Maximum floating point number (can be set to an arbitrary value)
    */
#  include <float.h>

#  include <math.h>

#  if defined(_AMIGA) && defined(__SASC) && defined(_M68881)
   /* Amiga SAS/C: We must include builtin FPU functions when compiling using
    * MATH=68881
    */
#    include <m68881.h>
#  endif
#endif

/* This provides the non-ANSI (far) memory allocation routines. */
#if defined(__TURBOC__) && defined(__MSDOS__)
#  include <mem.h>
#  include <alloc.h>
#endif

#if defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
#  include <windows.h>
#endif
#endif /* CI_VERSION_INFO_ONLY */

/* Moved here around 1.5.0beta36 from ciconf.h */
/* Users may want to use these so they are not private.  Any library
 * functions that are passed far data must be model-independent.
 */

/* Platform-independent functions */
#ifndef CI_ABORT
#  define CI_ABORT() abort()
#endif

/* These macros may need to be architecture dependent. */
#define CI_ALIGN_NONE      0 /* do not use data alignment */
#define CI_ALIGN_ALWAYS    1 /* assume unaligned accesses are OK */
#ifdef offsetof
#  define CI_ALIGN_OFFSET  2 /* use offsetof to determine alignment */
#else
#  define CI_ALIGN_OFFSET -1 /* prevent the use of this */
#endif
#define CI_ALIGN_SIZE      3 /* use sizeof to determine alignment */

#ifndef CI_ALIGN_TYPE
   /* Default to using aligned access optimizations and requiring alignment to a
    * multiple of the data type size.  Override in a compiler specific fashion
    * if necessary by inserting tests here:
    */
#  define CI_ALIGN_TYPE CI_ALIGN_SIZE
#endif

#if CI_ALIGN_TYPE == CI_ALIGN_SIZE
   /* This is used because in some compiler implementations non-aligned
    * structure members are supported, so the offsetof approach below fails.
    * Set CI_ALIGN_SIZE=0 for compiler combinations where unaligned access
    * is good for performance.  Do not do this unless you have tested the
    * result and understand it.
    */
#  define ci_alignof(type) (sizeof(type))
#else
#  if CI_ALIGN_TYPE == CI_ALIGN_OFFSET
#    define ci_alignof(type) offsetof(struct{char c; type t;}, t)
#  else
#    if CI_ALIGN_TYPE == CI_ALIGN_ALWAYS
#      define ci_alignof(type) 1
#    endif
     /* Else leave ci_alignof undefined to prevent use thereof */
#  endif
#endif

/* This implicitly assumes alignment is always a multiple of 2. */
#ifdef ci_alignof
#  define ci_isaligned(ptr, type) \
   (((type)(size_t)((const void*)(ptr)) & (type)(ci_alignof(type)-1)) == 0)
#else
#  define ci_isaligned(ptr, type) 0
#endif

/* End of memory model/platform independent support */
/* End of 1.5.0beta36 move from ciconf.h */

/* CONSTANTS and UTILITY MACROS
 * These are used internally by libci and not exposed in the API
 */

/* Various modes of operation.  Note that after an init, mode is set to
 * zero automatically when the structure is created.  Three of these
 * are defined in ci.h because they need to be visible to applications
 * that call ci_set_unknown_chunk().
 */
/* #define CI_HAVE_IHDR            0x01U (defined in ci.h) */
/* #define CI_HAVE_PLTE            0x02U (defined in ci.h) */
#define CI_HAVE_IDAT               0x04U
/* #define CI_AFTER_IDAT           0x08U (defined in ci.h) */
#define CI_HAVE_IEND               0x10U
                   /*               0x20U (unused) */
                   /*               0x40U (unused) */
                   /*               0x80U (unused) */
#define CI_HAVE_CHUNK_HEADER      0x100U
#define CI_WROTE_tIME             0x200U
#define CI_WROTE_INFO_BEFORE_PLTE 0x400U
#define CI_BACKGROUND_IS_GRAY     0x800U
#define CI_HAVE_CI_SIGNATURE    0x1000U
#define CI_HAVE_CHUNK_AFTER_IDAT 0x2000U /* Have another chunk after IDAT */
#define CI_WROTE_eXIf            0x4000U
#define CI_IS_READ_STRUCT        0x8000U /* Else is a write struct */

/* Flags for the transformations the CI library does on the image data */
#define CI_BGR                 0x0001U
#define CI_INTERLACE           0x0002U
#define CI_PACK                0x0004U
#define CI_SHIFT               0x0008U
#define CI_SWAP_BYTES          0x0010U
#define CI_INVERT_MONO         0x0020U
#define CI_QUANTIZE            0x0040U
#define CI_COMPOSE             0x0080U    /* Was CI_BACKGROUND */
#define CI_BACKGROUND_EXPAND   0x0100U
#define CI_EXPAND_16           0x0200U    /* Added to libci 1.5.2 */
#define CI_16_TO_8             0x0400U    /* Becomes 'chop' in 1.5.4 */
#define CI_RGBA                0x0800U
#define CI_EXPAND              0x1000U
#define CI_GAMMA               0x2000U
#define CI_GRAY_TO_RGB         0x4000U
#define CI_FILLER              0x8000U
#define CI_PACKSWAP           0x10000U
#define CI_SWAP_ALPHA         0x20000U
#define CI_STRIP_ALPHA        0x40000U
#define CI_INVERT_ALPHA       0x80000U
#define CI_USER_TRANSFORM    0x100000U
#define CI_RGB_TO_GRAY_ERR   0x200000U
#define CI_RGB_TO_GRAY_WARN  0x400000U
#define CI_RGB_TO_GRAY       0x600000U /* two bits, RGB_TO_GRAY_ERR|WARN */
#define CI_ENCODE_ALPHA      0x800000U /* Added to libci-1.5.4 */
#define CI_ADD_ALPHA        0x1000000U /* Added to libci-1.2.7 */
#define CI_EXPAND_tRNS      0x2000000U /* Added to libci-1.2.9 */
#define CI_SCALE_16_TO_8    0x4000000U /* Added to libci-1.5.4 */
                       /*    0x8000000U unused */
                       /*   0x10000000U unused */
                       /*   0x20000000U unused */
                       /*   0x40000000U unused */
/* Flags for ci_create_struct */
#define CI_STRUCT_CI   0x0001U
#define CI_STRUCT_INFO  0x0002U

/* Flags for the ci_ptr->flags rather than declaring a byte for each one */
#define CI_FLAG_ZLIB_CUSTOM_STRATEGY     0x0001U
#define CI_FLAG_ZSTREAM_INITIALIZED      0x0002U /* Added to libci-1.6.0 */
                                  /*      0x0004U    unused */
#define CI_FLAG_ZSTREAM_ENDED            0x0008U /* Added to libci-1.6.0 */
                                  /*      0x0010U    unused */
                                  /*      0x0020U    unused */
#define CI_FLAG_ROW_INIT                 0x0040U
#define CI_FLAG_FILLER_AFTER             0x0080U
#define CI_FLAG_CRC_ANCILLARY_USE        0x0100U
#define CI_FLAG_CRC_ANCILLARY_NOWARN     0x0200U
#define CI_FLAG_CRC_CRITICAL_USE         0x0400U
#define CI_FLAG_CRC_CRITICAL_IGNORE      0x0800U
/*      CI_FLAG_ASSUME_sRGB unused       0x1000U  * Added to libci-1.5.4 */
#define CI_FLAG_OPTIMIZE_ALPHA           0x2000U /* Added to libci-1.5.4 */
#define CI_FLAG_DETECT_UNINITIALIZED     0x4000U /* Added to libci-1.5.4 */
/* #define CI_FLAG_KEEP_UNKNOWN_CHUNKS      0x8000U */
/* #define CI_FLAG_KEEP_UNSAFE_CHUNKS      0x10000U */
#define CI_FLAG_LIBRARY_MISMATCH        0x20000U
#define CI_FLAG_STRIP_ERROR_NUMBERS     0x40000U
#define CI_FLAG_STRIP_ERROR_TEXT        0x80000U
#define CI_FLAG_BENIGN_ERRORS_WARN     0x100000U /* Added to libci-1.4.0 */
#define CI_FLAG_APP_WARNINGS_WARN      0x200000U /* Added to libci-1.6.0 */
#define CI_FLAG_APP_ERRORS_WARN        0x400000U /* Added to libci-1.6.0 */
                                  /*    0x800000U    unused */
                                  /*   0x1000000U    unused */
                                  /*   0x2000000U    unused */
                                  /*   0x4000000U    unused */
                                  /*   0x8000000U    unused */
                                  /*  0x10000000U    unused */
                                  /*  0x20000000U    unused */
                                  /*  0x40000000U    unused */

#define CI_FLAG_CRC_ANCILLARY_MASK (CI_FLAG_CRC_ANCILLARY_USE | \
                                     CI_FLAG_CRC_ANCILLARY_NOWARN)

#define CI_FLAG_CRC_CRITICAL_MASK  (CI_FLAG_CRC_CRITICAL_USE | \
                                     CI_FLAG_CRC_CRITICAL_IGNORE)

#define CI_FLAG_CRC_MASK           (CI_FLAG_CRC_ANCILLARY_MASK | \
                                     CI_FLAG_CRC_CRITICAL_MASK)

/* Save typing and make code easier to understand */

#define CI_COLOR_DIST(c1, c2) (abs((int)((c1).red) - (int)((c2).red)) + \
   abs((int)((c1).green) - (int)((c2).green)) + \
   abs((int)((c1).blue) - (int)((c2).blue)))

/* Added to libci-1.6.0: scale a 16-bit value in the range 0..65535 to 0..255
 * by dividing by 257 *with rounding*.  This macro is exact for the given range.
 * See the discourse in cirtran.c ci_do_scale_16_to_8.  The values in the
 * macro were established by experiment (modifying the added value).  The macro
 * has a second variant that takes a value already scaled by 255 and divides by
 * 65535 - this has a maximum error of .502.  Over the range 0..65535*65535 it
 * only gives off-by-one errors and only for 0.5% (1 in 200) of the values.
 */
#define CI_DIV65535(v24) (((v24) + 32895) >> 16)
#define CI_DIV257(v16) CI_DIV65535((ci_uint_32)(v16) * 255)

/* Added to libci-1.2.6 JB */
#define CI_ROWBYTES(pixel_bits, width) \
    ((pixel_bits) >= 8 ? \
    ((size_t)(width) * (((size_t)(pixel_bits)) >> 3)) : \
    (( ((size_t)(width) * ((size_t)(pixel_bits))) + 7) >> 3) )

/* This returns the number of trailing bits in the last byte of a row, 0 if the
 * last byte is completely full of pixels.  It is, in principle, (pixel_bits x
 * width) % 8, but that would overflow for large 'width'.  The second macro is
 * the same except that it returns the number of unused bits in the last byte;
 * (8-TRAILBITS), but 0 when TRAILBITS is 0.
 *
 * NOTE: these macros are intended to be self-evidently correct and never
 * overflow on the assumption that pixel_bits is in the range 0..255.  The
 * arguments are evaluated only once and they can be signed (e.g. as a result of
 * the integral promotions).  The result of the expression always has type
 * (ci_uint_32), however the compiler always knows it is in the range 0..7.
 */
#define CI_TRAILBITS(pixel_bits, width) \
    (((pixel_bits) * ((width) % (ci_uint_32)8)) % 8)

#define CI_PADBITS(pixel_bits, width) \
    ((8 - CI_TRAILBITS(pixel_bits, width)) % 8)

/* CI_OUT_OF_RANGE returns true if value is outside the range
 * ideal-delta..ideal+delta.  Each argument is evaluated twice.
 * "ideal" and "delta" should be constants, normally simple
 * integers, "value" a variable. Added to libci-1.2.6 JB
 */
#define CI_OUT_OF_RANGE(value, ideal, delta) \
   ( (value) < (ideal)-(delta) || (value) > (ideal)+(delta) )

/* Conversions between fixed and floating point, only defined if
 * required (to make sure the code doesn't accidentally use float
 * when it is supposedly disabled.)
 */
#ifdef CI_FLOATING_POINT_SUPPORTED
/* The floating point conversion can't overflow, though it can and
 * does lose accuracy relative to the original fixed point value.
 * In practice this doesn't matter because ci_fixed_point only
 * stores numbers with very low precision.  The ci_ptr and s
 * arguments are unused by default but are there in case error
 * checking becomes a requirement.
 */
#define ci_float(ci_ptr, fixed, s) (.00001 * (fixed))

/* The fixed point conversion performs range checking and evaluates
 * its argument multiple times, so must be used with care.  The
 * range checking uses the CI specification values for a signed
 * 32-bit fixed point value except that the values are deliberately
 * rounded-to-zero to an integral value - 21474 (21474.83 is roughly
 * (2^31-1) * 100000). 's' is a string that describes the value being
 * converted.
 *
 * NOTE: this macro will raise a ci_error if the range check fails,
 * therefore it is normally only appropriate to use this on values
 * that come from API calls or other sources where an out of range
 * error indicates a programming error, not a data error!
 *
 * NOTE: by default this is off - the macro is not used - because the
 * function call saves a lot of code.
 */
#ifdef CI_FIXED_POINT_MACRO_SUPPORTED
#define ci_fixed(ci_ptr, fp, s) ((fp) <= 21474 && (fp) >= -21474 ?\
    ((ci_fixed_point)(100000 * (fp))) : (ci_fixed_error(ci_ptr, s),0))
#define ci_fixed_ITU(ci_ptr, fp, s) ((fp) <= 214748 && (fp) >= 0 ?\
    ((ci_uint_32)(10000 * (fp))) : (ci_fixed_error(ci_ptr, s),0))
#endif
/* else the corresponding function is defined below, inside the scope of the
 * cplusplus test.
 */
#endif

/* Constants for known chunk types.  If you need to add a chunk, define the name
 * here.  For historical reasons these constants have the form ci_<name>; i.e.
 * the prefix is lower case.  Please use decimal values as the parameters to
 * match the ISO CI specification and to avoid relying on the C locale
 * interpretation of character values.
 *
 * Prior to 1.5.6 these constants were strings, as of 1.5.6 ci_uint_32 values
 * are computed and a new macro (CI_STRING_FROM_CHUNK) added to allow a string
 * to be generated if required.
 *
 * CI_32b correctly produces a value shifted by up to 24 bits, even on
 * architectures where (int) is only 16 bits.
 *
 * 1.6.47: CI_32b was made into a preprocessor evaluable macro by replacing the
 * static_cast with a promoting binary operation using a guaranteed 32-bit
 * (minimum) unsigned value.
 */
#define CI_32b(b,s) (((0xFFFFFFFFU)&(b)) << (s))
#define CI_U32(b1,b2,b3,b4) \
   (CI_32b(b1,24) | CI_32b(b2,16) | CI_32b(b3,8) | CI_32b(b4,0))

/* Chunk name validation.  When using these macros all the arguments should be
 * constants, otherwise code bloat may well occur.  The macros are provided
 * primarily for use in #if checks.
 *
 * CI_32to8 produces a byte value with the right shift; used to extract the
 * byte value from a chunk name.
 */
#define CI_32to8(cn,s) (((cn) >> (s)) & 0xffU)
#define CI_CN_VALID_UPPER(b) ((b) >= 65 && (b) <= 90) /* upper-case ASCII */
#define CI_CN_VALID_ASCII(b) CI_CN_VALID_UPPER((b) & ~32U)
#define CI_CHUNK_NAME_VALID(cn) (\
   CI_CN_VALID_ASCII(CI_32to8(cn,24)) && /* critical, !ancillary */\
   CI_CN_VALID_ASCII(CI_32to8(cn,16)) && /* public, !privately defined */\
   CI_CN_VALID_UPPER(CI_32to8(cn, 8)) && /* VALID, !reserved */\
   CI_CN_VALID_ASCII(CI_32to8(cn, 0))   /* data-dependent, !copy ok */)

/* Constants for known chunk types.
 *
 * MAINTAINERS: If you need to add a chunk, define the name here.
 * For historical reasons these constants have the form ci_<name>; i.e.
 * the prefix is lower case.  Please use decimal values as the parameters to
 * match the ISO CI specification and to avoid relying on the C locale
 * interpretation of character values.  Please keep the list sorted.
 *
 * Notice that CI_U32 is used to define a 32-bit value for the 4 byte chunk
 * type.  In fact the specification does not express chunk types this way,
 * however using a 32-bit value means that the chunk type can be read from the
 * stream using exactly the same code as used for a 32-bit unsigned value and
 * can be examined far more efficiently (using one arithmetic compare).
 *
 * Prior to 1.5.6 the chunk type constants were expressed as C strings.  The
 * libci API still uses strings for 'unknown' chunks and a macro,
 * CI_STRING_FROM_CHUNK, allows a string to be generated if required.  Notice
 * that for portable code numeric values must still be used; the string "IHDR"
 * is not portable and neither is CI_U32('I', 'H', 'D', 'R').
 *
 * In 1.7.0 the definitions will be made public in ci.h to avoid having to
 * duplicate the same definitions in application code.
 */
#define ci_IDAT CI_U32( 73,  68,  65,  84)
#define ci_IEND CI_U32( 73,  69,  78,  68)
#define ci_IHDR CI_U32( 73,  72,  68,  82)
#define ci_PLTE CI_U32( 80,  76,  84,  69)
#define ci_acTL CI_U32( 97,  99,  84,  76) /* CIv3: ACI */
#define ci_bKGD CI_U32( 98,  75,  71,  68)
#define ci_cHRM CI_U32( 99,  72,  82,  77)
#define ci_cICP CI_U32( 99,  73,  67,  80) /* CIv3 */
#define ci_cLLI CI_U32( 99,  76,  76,  73) /* CIv3 */
#define ci_eXIf CI_U32(101,  88,  73, 102) /* registered July 2017 */
#define ci_fcTL CI_U32(102,  99,  84,  76) /* CIv3: ACI */
#define ci_fdAT CI_U32(102, 100,  65,  84) /* CIv3: ACI */
#define ci_fRAc CI_U32(102,  82,  65,  99) /* registered, not defined */
#define ci_gAMA CI_U32(103,  65,  77,  65)
#define ci_gIFg CI_U32(103,  73,  70, 103)
#define ci_gIFt CI_U32(103,  73,  70, 116) /* deprecated */
#define ci_gIFx CI_U32(103,  73,  70, 120)
#define ci_hIST CI_U32(104,  73,  83,  84)
#define ci_iCCP CI_U32(105,  67,  67,  80)
#define ci_iTXt CI_U32(105,  84,  88, 116)
#define ci_mDCV CI_U32(109,  68,  67,  86) /* CIv3 */
#define ci_oFFs CI_U32(111,  70,  70, 115)
#define ci_pCAL CI_U32(112,  67,  65,  76)
#define ci_pHYs CI_U32(112,  72,  89, 115)
#define ci_sBIT CI_U32(115,  66,  73,  84)
#define ci_sCAL CI_U32(115,  67,  65,  76)
#define ci_sPLT CI_U32(115,  80,  76,  84)
#define ci_sRGB CI_U32(115,  82,  71,  66)
#define ci_sTER CI_U32(115,  84,  69,  82)
#define ci_tEXt CI_U32(116,  69,  88, 116)
#define ci_tIME CI_U32(116,  73,  77,  69)
#define ci_tRNS CI_U32(116,  82,  78,  83)
#define ci_zTXt CI_U32(122,  84,  88, 116)

/* The following will work on (signed char*) strings, whereas the get_uint_32
 * macro will fail on top-bit-set values because of the sign extension.
 */
#define CI_CHUNK_FROM_STRING(s)\
   CI_U32(0xff & (s)[0], 0xff & (s)[1], 0xff & (s)[2], 0xff & (s)[3])

/* This uses (char), not (ci_byte) to avoid warnings on systems where (char) is
 * signed and the argument is a (char[])  This macro will fail miserably on
 * systems where (char) is more than 8 bits.
 */
#define CI_STRING_FROM_CHUNK(s,c)\
   (void)(((char*)(s))[0]=(char)(((c)>>24) & 0xff), \
   ((char*)(s))[1]=(char)(((c)>>16) & 0xff),\
   ((char*)(s))[2]=(char)(((c)>>8) & 0xff), \
   ((char*)(s))[3]=(char)((c & 0xff)))

/* Do the same but terminate with a null character. */
#define CI_CSTRING_FROM_CHUNK(s,c)\
   (void)(CI_STRING_FROM_CHUNK(s,c), ((char*)(s))[4] = 0)

/* Test on flag values as defined in the spec (section 5.4): */
#define CI_CHUNK_ANCILLARY(c)   (1 & ((c) >> 29))
#define CI_CHUNK_CRITICAL(c)     (!CI_CHUNK_ANCILLARY(c))
#define CI_CHUNK_PRIVATE(c)      (1 & ((c) >> 21))
#define CI_CHUNK_RESERVED(c)     (1 & ((c) >> 13))
#define CI_CHUNK_SAFE_TO_COPY(c) (1 & ((c) >>  5))

/* Known chunks.  All supported chunks must be listed here.  The macro CI_CHUNK
 * contains the four character ASCII name by which the chunk is identified.  The
 * macro is implemented as required to build tables or switch statements which
 * require entries for every known chunk.  The macro also contains an index
 * value which should be in order (this is checked in ci.c).
 *
 * Notice that "known" does not require "SUPPORTED"; tables should be built in
 * such a way that chunks unsupported in a build require no more than the table
 * entry (which should be small.)  In particular function pointers for
 * unsupported chunks should be NULL.
 *
 * At present these index values are not exported (not part of the public API)
 * so can be changed at will.  For convenience the names are in lexical sort
 * order but with the critical chunks at the start in the order of occurence in
 * a CI.
 *
 * CI_INFO_ values do not exist for every one of these chunk handles; for
 * example CI_INFO_{IDAT,IEND,tEXt,iTXt,zTXt} and possibly other chunks in the
 * future.
 */
#define CI_KNOWN_CHUNKS\
   CI_CHUNK(IHDR,  0)\
   CI_CHUNK(PLTE,  1)\
   CI_CHUNK(IDAT,  2)\
   CI_CHUNK(IEND,  3)\
   CI_CHUNK(acTL,  4)\
   CI_CHUNK(bKGD,  5)\
   CI_CHUNK(cHRM,  6)\
   CI_CHUNK(cICP,  7)\
   CI_CHUNK(cLLI,  8)\
   CI_CHUNK(eXIf,  9)\
   CI_CHUNK(fcTL, 10)\
   CI_CHUNK(fdAT, 11)\
   CI_CHUNK(gAMA, 12)\
   CI_CHUNK(hIST, 13)\
   CI_CHUNK(iCCP, 14)\
   CI_CHUNK(iTXt, 15)\
   CI_CHUNK(mDCV, 16)\
   CI_CHUNK(oFFs, 17)\
   CI_CHUNK(pCAL, 18)\
   CI_CHUNK(pHYs, 19)\
   CI_CHUNK(sBIT, 20)\
   CI_CHUNK(sCAL, 21)\
   CI_CHUNK(sPLT, 22)\
   CI_CHUNK(sRGB, 23)\
   CI_CHUNK(tEXt, 24)\
   CI_CHUNK(tIME, 25)\
   CI_CHUNK(tRNS, 26)\
   CI_CHUNK(zTXt, 27)

/* Gamma values (new at libci-1.5.4): */
#define CI_GAMMA_MAC_OLD 151724  /* Assume '1.8' is really 2.2/1.45! */
#define CI_GAMMA_MAC_INVERSE 65909
#define CI_GAMMA_sRGB_INVERSE 45455

/* gamma sanity check.  libci cannot implement gamma transforms outside a
 * certain limit because of its use of 16-bit fixed point intermediate values.
 * Gamma values that are too large or too small will zap the 16-bit values all
 * to 0 or 65535 resulting in an obvious 'bad' image.
 *
 * In libci 1.6.0 the limits were changed from 0.07..3 to 0.01..100 to
 * accommodate the optimal 16-bit gamma of 36 and its reciprocal.
 *
 * These are ci_fixed_point integral values:
 */
#define CI_LIB_GAMMA_MIN 1000
#define CI_LIB_GAMMA_MAX 10000000

/* Almost everything below is C specific; the #defines above can be used in
 * non-C code (so long as it is C-preprocessed) the rest of this stuff cannot.
 */
#ifndef CI_VERSION_INFO_ONLY

#include "cistruct.h"
#include "ciinfo.h"

/* Validate the include paths - the include path used to generate cilibconf.h
 * must match that used in the build, or we must be using cilibconf.h.prebuilt:
 */
#if CI_ZLIB_VERNUM != 0 && CI_ZLIB_VERNUM != ZLIB_VERNUM
#  error The include path of <zlib.h> is incorrect
   /* When cilibconf.h was built, the copy of zlib.h that it used was not the
    * same as the one being used here.  Considering how libci makes decisions
    * to use the zlib API based on the zlib version number, the -I options must
    * match.
    *
    * A possible cause of this mismatch is that you passed an -I option in
    * CFLAGS, which is unlikely to work.  All the preprocessor options, and all
    * the -I options in particular, should be in CPPFLAGS.
    */
#endif

/* This is used for 16-bit gamma tables -- only the top level pointers are
 * const; this could be changed:
 */
typedef const ci_uint_16p * ci_const_uint_16pp;

/* Added to libci-1.5.7: sRGB conversion tables */
#if defined(CI_SIMPLIFIED_READ_SUPPORTED) ||\
   defined(CI_SIMPLIFIED_WRITE_SUPPORTED)
#ifdef CI_SIMPLIFIED_READ_SUPPORTED
CI_INTERNAL_DATA(const ci_uint_16, ci_sRGB_table, [256]);
   /* Convert from an sRGB encoded value 0..255 to a 16-bit linear value,
    * 0..65535.  This table gives the closest 16-bit answers (no errors).
    */
#endif

CI_INTERNAL_DATA(const ci_uint_16, ci_sRGB_base, [512]);
CI_INTERNAL_DATA(const ci_byte, ci_sRGB_delta, [512]);

#define CI_sRGB_FROM_LINEAR(linear) \
  ((ci_byte)(0xff & ((ci_sRGB_base[(linear)>>15] \
   + ((((linear) & 0x7fff)*ci_sRGB_delta[(linear)>>15])>>12)) >> 8)))
   /* Given a value 'linear' in the range 0..255*65535 calculate the 8-bit sRGB
    * encoded value with maximum error 0.646365.  Note that the input is not a
    * 16-bit value; it has been multiplied by 255! */
#endif /* SIMPLIFIED_READ/WRITE */


/* Inhibit C++ name-mangling for libci functions but not for system calls. */
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* Internal functions; these are not exported from a DLL however because they
 * are used within several of the C source files they have to be C extern.
 *
 * All of these functions must be declared with CI_INTERNAL_FUNCTION.
 */
/* Zlib support */
#define CI_UNEXPECTED_ZLIB_RETURN (-7)
CI_INTERNAL_FUNCTION(void, ci_zstream_error,(ci_structrp ci_ptr, int ret),
   CI_EMPTY);
   /* Used by the zlib handling functions to ensure that z_stream::msg is always
    * set before they return.
    */

#ifdef CI_WRITE_SUPPORTED
CI_INTERNAL_FUNCTION(void,ci_free_buffer_list,(ci_structrp ci_ptr,
   ci_compression_bufferp *list),CI_EMPTY);
   /* Free the buffer list used by the compressed write code. */
#endif

#if defined(CI_FLOATING_POINT_SUPPORTED) && \
   !defined(CI_FIXED_POINT_MACRO_SUPPORTED) && \
   (defined(CI_gAMA_SUPPORTED) || defined(CI_cHRM_SUPPORTED) || \
   defined(CI_sCAL_SUPPORTED) || defined(CI_READ_BACKGROUND_SUPPORTED) || \
   defined(CI_mDCV_SUPPORTED) || \
   defined(CI_READ_RGB_TO_GRAY_SUPPORTED)) || \
   (defined(CI_sCAL_SUPPORTED) && \
   defined(CI_FLOATING_ARITHMETIC_SUPPORTED))
CI_INTERNAL_FUNCTION(ci_fixed_point,ci_fixed,(ci_const_structrp ci_ptr,
   double fp, ci_const_charp text),CI_EMPTY);
#endif

#if defined(CI_FLOATING_POINT_SUPPORTED) && \
   !defined(CI_FIXED_POINT_MACRO_SUPPORTED) && \
   (defined(CI_cLLI_SUPPORTED) || defined(CI_mDCV_SUPPORTED))
CI_INTERNAL_FUNCTION(ci_uint_32,ci_fixed_ITU,(ci_const_structrp ci_ptr,
   double fp, ci_const_charp text),CI_EMPTY);
#endif

/* Check the user version string for compatibility, returns false if the version
 * numbers aren't compatible.
 */
CI_INTERNAL_FUNCTION(int,ci_user_version_check,(ci_structrp ci_ptr,
   ci_const_charp user_ci_ver),CI_EMPTY);

#ifdef CI_READ_SUPPORTED /* should only be used on read */
/* Security: read limits on the largest allocations while reading a CI.  This
 * avoids very large allocations caused by CI files with damaged or altered
 * chunk 'length' fields.
 */
#ifdef CI_SET_USER_LIMITS_SUPPORTED /* run-time limit */
#  define ci_chunk_max(ci_ptr) ((ci_ptr)->user_chunk_malloc_max)

#elif CI_USER_CHUNK_MALLOC_MAX > 0 /* compile-time limit */
#  define ci_chunk_max(ci_ptr) ((void)ci_ptr, CI_USER_CHUNK_MALLOC_MAX)

#elif (defined CI_MAX_MALLOC_64K)  /* legacy system limit */
#  define ci_chunk_max(ci_ptr) ((void)ci_ptr, 65536U)

#else                               /* modern system limit SIZE_MAX (C99) */
#  define ci_chunk_max(ci_ptr) ((void)ci_ptr, CI_SIZE_MAX)
#endif
#endif /* READ */

/* Internal base allocator - no messages, NULL on failure to allocate.  This
 * does, however, call the application provided allocator and that could call
 * ci_error (although that would be a bug in the application implementation.)
 */
CI_INTERNAL_FUNCTION(ci_voidp,ci_malloc_base,(ci_const_structrp ci_ptr,
   ci_alloc_size_t size),CI_ALLOCATED);

#if defined(CI_TEXT_SUPPORTED) || defined(CI_sPLT_SUPPORTED) ||\
   defined(CI_STORE_UNKNOWN_CHUNKS_SUPPORTED)
/* Internal array allocator, outputs no error or warning messages on failure,
 * just returns NULL.
 */
CI_INTERNAL_FUNCTION(ci_voidp,ci_malloc_array,(ci_const_structrp ci_ptr,
   int nelements, size_t element_size),CI_ALLOCATED);

/* The same but an existing array is extended by add_elements.  This function
 * also memsets the new elements to 0 and copies the old elements.  The old
 * array is not freed or altered.
 */
CI_INTERNAL_FUNCTION(ci_voidp,ci_realloc_array,(ci_const_structrp ci_ptr,
   ci_const_voidp array, int old_elements, int add_elements,
   size_t element_size),CI_ALLOCATED);
#endif /* text, sPLT or unknown chunks */

/* Magic to create a struct when there is no struct to call the user supplied
 * memory allocators.  Because error handling has not been set up the memory
 * handlers can't safely call ci_error, but this is an obscure and undocumented
 * restriction so libci has to assume that the 'free' handler, at least, might
 * call ci_error.
 */
CI_INTERNAL_FUNCTION(ci_structp,ci_create_ci_struct,
   (ci_const_charp user_ci_ver, ci_voidp error_ptr, ci_error_ptr error_fn,
    ci_error_ptr warn_fn, ci_voidp mem_ptr, ci_malloc_ptr malloc_fn,
    ci_free_ptr free_fn),CI_ALLOCATED);

/* Free memory from internal libci struct */
CI_INTERNAL_FUNCTION(void,ci_destroy_ci_struct,(ci_structrp ci_ptr),
   CI_EMPTY);

/* Free an allocated jmp_buf (always succeeds) */
CI_INTERNAL_FUNCTION(void,ci_free_jmpbuf,(ci_structrp ci_ptr),CI_EMPTY);

/* Function to allocate memory for zlib.  CIAPI is disallowed. */
CI_INTERNAL_FUNCTION(voidpf,ci_zalloc,(voidpf ci_ptr, uInt items, uInt size),
   CI_ALLOCATED);

/* Function to free memory for zlib.  CIAPI is disallowed. */
CI_INTERNAL_FUNCTION(void,ci_zfree,(voidpf ci_ptr, voidpf ptr),CI_EMPTY);

/* Next four functions are used internally as callbacks.  CICBAPI is required
 * but not CI_EXPORT.  CIAPI added at libci version 1.2.3, changed to
 * CICBAPI at 1.5.0
 */

CI_INTERNAL_FUNCTION(void CICBAPI,ci_default_read_data,(ci_structp ci_ptr,
    ci_bytep data, size_t length),CI_EMPTY);

#ifdef CI_PROGRESSIVE_READ_SUPPORTED
CI_INTERNAL_FUNCTION(void CICBAPI,ci_push_fill_buffer,(ci_structp ci_ptr,
    ci_bytep buffer, size_t length),CI_EMPTY);
#endif

CI_INTERNAL_FUNCTION(void CICBAPI,ci_default_write_data,(ci_structp ci_ptr,
    ci_bytep data, size_t length),CI_EMPTY);

#ifdef CI_WRITE_FLUSH_SUPPORTED
#  ifdef CI_STDIO_SUPPORTED
CI_INTERNAL_FUNCTION(void CICBAPI,ci_default_flush,(ci_structp ci_ptr),
   CI_EMPTY);
#  endif
#endif

/* Reset the CRC variable */
CI_INTERNAL_FUNCTION(void,ci_reset_crc,(ci_structrp ci_ptr),CI_EMPTY);

/* Write the "data" buffer to whatever output you are using */
CI_INTERNAL_FUNCTION(void,ci_write_data,(ci_structrp ci_ptr,
    ci_const_bytep data, size_t length),CI_EMPTY);

/* Read and check the CI file signature */
CI_INTERNAL_FUNCTION(void,ci_read_sig,(ci_structrp ci_ptr,
   ci_inforp info_ptr),CI_EMPTY);

/* Read the chunk header (length + type name) */
CI_INTERNAL_FUNCTION(ci_uint_32,ci_read_chunk_header,(ci_structrp ci_ptr),
   CI_EMPTY);

/* Read data from whatever input you are using into the "data" buffer */
CI_INTERNAL_FUNCTION(void,ci_read_data,(ci_structrp ci_ptr, ci_bytep data,
    size_t length),CI_EMPTY);

/* Read bytes into buf, and update ci_ptr->crc */
CI_INTERNAL_FUNCTION(void,ci_crc_read,(ci_structrp ci_ptr, ci_bytep buf,
    ci_uint_32 length),CI_EMPTY);

/* Read "skip" bytes, read the file crc, and (optionally) verify ci_ptr->crc */
CI_INTERNAL_FUNCTION(int,ci_crc_finish,(ci_structrp ci_ptr,
   ci_uint_32 skip),CI_EMPTY);

/* Calculate the CRC over a section of data.  Note that we are only
 * passing a maximum of 64K on systems that have this as a memory limit,
 * since this is the maximum buffer size we can specify.
 */
CI_INTERNAL_FUNCTION(void,ci_calculate_crc,(ci_structrp ci_ptr,
   ci_const_bytep ptr, size_t length),CI_EMPTY);

#ifdef CI_WRITE_FLUSH_SUPPORTED
CI_INTERNAL_FUNCTION(void,ci_flush,(ci_structrp ci_ptr),CI_EMPTY);
#endif

/* Write various chunks */

/* Write the IHDR chunk, and update the ci_struct with the necessary
 * information.
 */
CI_INTERNAL_FUNCTION(void,ci_write_IHDR,(ci_structrp ci_ptr,
   ci_uint_32 width, ci_uint_32 height, int bit_depth, int color_type,
   int compression_method, int filter_method, int interlace_method),CI_EMPTY);

CI_INTERNAL_FUNCTION(void,ci_write_PLTE,(ci_structrp ci_ptr,
   ci_const_colorp palette, ci_uint_32 num_pal),CI_EMPTY);

CI_INTERNAL_FUNCTION(void,ci_compress_IDAT,(ci_structrp ci_ptr,
   ci_const_bytep row_data, ci_alloc_size_t row_data_length, int flush),
   CI_EMPTY);

CI_INTERNAL_FUNCTION(void,ci_write_IEND,(ci_structrp ci_ptr),CI_EMPTY);

#ifdef CI_WRITE_gAMA_SUPPORTED
CI_INTERNAL_FUNCTION(void,ci_write_gAMA_fixed,(ci_structrp ci_ptr,
    ci_fixed_point file_gamma),CI_EMPTY);
#endif

#ifdef CI_WRITE_sBIT_SUPPORTED
CI_INTERNAL_FUNCTION(void,ci_write_sBIT,(ci_structrp ci_ptr,
    ci_const_color_8p sbit, int color_type),CI_EMPTY);
#endif

#ifdef CI_WRITE_cHRM_SUPPORTED
CI_INTERNAL_FUNCTION(void,ci_write_cHRM_fixed,(ci_structrp ci_ptr,
    const ci_xy *xy), CI_EMPTY);
   /* The xy value must have been previously validated */
#endif

#ifdef CI_WRITE_cICP_SUPPORTED
CI_INTERNAL_FUNCTION(void,ci_write_cICP,(ci_structrp ci_ptr,
    ci_byte colour_primaries, ci_byte transfer_function,
    ci_byte matrix_coefficients, ci_byte video_full_range_flag), CI_EMPTY);
#endif

#ifdef CI_WRITE_cLLI_SUPPORTED
CI_INTERNAL_FUNCTION(void,ci_write_cLLI_fixed,(ci_structrp ci_ptr,
   ci_uint_32 maxCLL, ci_uint_32 maxFALL), CI_EMPTY);
#endif

#ifdef CI_WRITE_mDCV_SUPPORTED
CI_INTERNAL_FUNCTION(void,ci_write_mDCV_fixed,(ci_structrp ci_ptr,
   ci_uint_16 red_x, ci_uint_16 red_y,
   ci_uint_16 green_x, ci_uint_16 green_y,
   ci_uint_16 blue_x, ci_uint_16 blue_y,
   ci_uint_16 white_x, ci_uint_16 white_y,
   ci_uint_32 maxDL, ci_uint_32 minDL), CI_EMPTY);
#endif

#ifdef CI_WRITE_sRGB_SUPPORTED
CI_INTERNAL_FUNCTION(void,ci_write_sRGB,(ci_structrp ci_ptr,
    int intent),CI_EMPTY);
#endif

#ifdef CI_WRITE_eXIf_SUPPORTED
CI_INTERNAL_FUNCTION(void,ci_write_eXIf,(ci_structrp ci_ptr,
    ci_bytep exif, int num_exif),CI_EMPTY);
#endif

#ifdef CI_WRITE_iCCP_SUPPORTED
CI_INTERNAL_FUNCTION(void,ci_write_iCCP,(ci_structrp ci_ptr,
   ci_const_charp name, ci_const_bytep profile, ci_uint_32 proflen),
   CI_EMPTY);
   /* Writes a previously 'set' profile.  The profile argument is **not**
    * compressed.
    */
#endif

#ifdef CI_WRITE_sPLT_SUPPORTED
CI_INTERNAL_FUNCTION(void,ci_write_sPLT,(ci_structrp ci_ptr,
    ci_const_sPLT_tp palette),CI_EMPTY);
#endif

#ifdef CI_WRITE_tRNS_SUPPORTED
CI_INTERNAL_FUNCTION(void,ci_write_tRNS,(ci_structrp ci_ptr,
    ci_const_bytep trans, ci_const_color_16p values, int number,
    int color_type),CI_EMPTY);
#endif

#ifdef CI_WRITE_bKGD_SUPPORTED
CI_INTERNAL_FUNCTION(void,ci_write_bKGD,(ci_structrp ci_ptr,
    ci_const_color_16p values, int color_type),CI_EMPTY);
#endif

#ifdef CI_WRITE_hIST_SUPPORTED
CI_INTERNAL_FUNCTION(void,ci_write_hIST,(ci_structrp ci_ptr,
    ci_const_uint_16p hist, int num_hist),CI_EMPTY);
#endif

/* Chunks that have keywords */
#ifdef CI_WRITE_tEXt_SUPPORTED
CI_INTERNAL_FUNCTION(void,ci_write_tEXt,(ci_structrp ci_ptr,
   ci_const_charp key, ci_const_charp text, size_t text_len),CI_EMPTY);
#endif

#ifdef CI_WRITE_zTXt_SUPPORTED
CI_INTERNAL_FUNCTION(void,ci_write_zTXt,(ci_structrp ci_ptr, ci_const_charp
    key, ci_const_charp text, int compression),CI_EMPTY);
#endif

#ifdef CI_WRITE_iTXt_SUPPORTED
CI_INTERNAL_FUNCTION(void,ci_write_iTXt,(ci_structrp ci_ptr,
    int compression, ci_const_charp key, ci_const_charp lang,
    ci_const_charp lang_key, ci_const_charp text),CI_EMPTY);
#endif

#ifdef CI_TEXT_SUPPORTED  /* Added at version 1.0.14 and 1.2.4 */
CI_INTERNAL_FUNCTION(int,ci_set_text_2,(ci_const_structrp ci_ptr,
    ci_inforp info_ptr, ci_const_textp text_ptr, int num_text),CI_EMPTY);
#endif

#ifdef CI_WRITE_oFFs_SUPPORTED
CI_INTERNAL_FUNCTION(void,ci_write_oFFs,(ci_structrp ci_ptr,
    ci_int_32 x_offset, ci_int_32 y_offset, int unit_type),CI_EMPTY);
#endif

#ifdef CI_WRITE_pCAL_SUPPORTED
CI_INTERNAL_FUNCTION(void,ci_write_pCAL,(ci_structrp ci_ptr,
    ci_charp purpose, ci_int_32 X0, ci_int_32 X1, int type, int nparams,
    ci_const_charp units, ci_charpp params),CI_EMPTY);
#endif

#ifdef CI_WRITE_pHYs_SUPPORTED
CI_INTERNAL_FUNCTION(void,ci_write_pHYs,(ci_structrp ci_ptr,
    ci_uint_32 x_pixels_per_unit, ci_uint_32 y_pixels_per_unit,
    int unit_type),CI_EMPTY);
#endif

#ifdef CI_WRITE_tIME_SUPPORTED
CI_INTERNAL_FUNCTION(void,ci_write_tIME,(ci_structrp ci_ptr,
    ci_const_timep mod_time),CI_EMPTY);
#endif

#ifdef CI_WRITE_sCAL_SUPPORTED
CI_INTERNAL_FUNCTION(void,ci_write_sCAL_s,(ci_structrp ci_ptr,
    int unit, ci_const_charp width, ci_const_charp height),CI_EMPTY);
#endif

/* Called when finished processing a row of data */
CI_INTERNAL_FUNCTION(void,ci_write_finish_row,(ci_structrp ci_ptr),
    CI_EMPTY);

/* Internal use only.   Called before first row of data */
CI_INTERNAL_FUNCTION(void,ci_write_start_row,(ci_structrp ci_ptr),
    CI_EMPTY);

/* Combine a row of data, dealing with alpha, etc. if requested.  'row' is an
 * array of ci_ptr->width pixels.  If the image is not interlaced or this
 * is the final pass this just does a memcpy, otherwise the "display" flag
 * is used to determine whether to copy pixels that are not in the current pass.
 *
 * Because 'ci_do_read_interlace' (below) replicates pixels this allows this
 * function to achieve the documented 'blocky' appearance during interlaced read
 * if display is 1 and the 'sparkle' appearance, where existing pixels in 'row'
 * are not changed if they are not in the current pass, when display is 0.
 *
 * 'display' must be 0 or 1, otherwise the memcpy will be done regardless.
 *
 * The API always reads from the ci_struct row buffer and always assumes that
 * it is full width (ci_do_read_interlace has already been called.)
 *
 * This function is only ever used to write to row buffers provided by the
 * caller of the relevant libci API and the row must have already been
 * transformed by the read transformations.
 *
 * The CI_USE_COMPILE_TIME_MASKS option causes generation of pre-computed
 * bitmasks for use within the code, otherwise runtime generated masks are used.
 * The default is compile time masks.
 */
#ifndef CI_USE_COMPILE_TIME_MASKS
#  define CI_USE_COMPILE_TIME_MASKS 1
#endif
CI_INTERNAL_FUNCTION(void,ci_combine_row,(ci_const_structrp ci_ptr,
    ci_bytep row, int display),CI_EMPTY);

#ifdef CI_READ_INTERLACING_SUPPORTED
/* Expand an interlaced row: the 'row_info' describes the pass data that has
 * been read in and must correspond to the pixels in 'row', the pixels are
 * expanded (moved apart) in 'row' to match the final layout, when doing this
 * the pixels are *replicated* to the intervening space.  This is essential for
 * the correct operation of ci_combine_row, above.
 */
CI_INTERNAL_FUNCTION(void,ci_do_read_interlace,(ci_row_infop row_info,
    ci_bytep row, int pass, ci_uint_32 transformations),CI_EMPTY);
#endif

/* GRR TO DO (2.0 or whenever):  simplify other internal calling interfaces */

#ifdef CI_WRITE_INTERLACING_SUPPORTED
/* Grab pixels out of a row for an interlaced pass */
CI_INTERNAL_FUNCTION(void,ci_do_write_interlace,(ci_row_infop row_info,
    ci_bytep row, int pass),CI_EMPTY);
#endif

/* Unfilter a row: check the filter value before calling this, there is no point
 * calling it for CI_FILTER_VALUE_NONE.
 */
CI_INTERNAL_FUNCTION(void,ci_read_filter_row,(ci_structrp pp, ci_row_infop
    row_info, ci_bytep row, ci_const_bytep prev_row, int filter),CI_EMPTY);

#if CI_ARM_NEON_OPT > 0
CI_INTERNAL_FUNCTION(void,ci_read_filter_row_up_neon,(ci_row_infop row_info,
    ci_bytep row, ci_const_bytep prev_row),CI_EMPTY);
CI_INTERNAL_FUNCTION(void,ci_read_filter_row_sub3_neon,(ci_row_infop
    row_info, ci_bytep row, ci_const_bytep prev_row),CI_EMPTY);
CI_INTERNAL_FUNCTION(void,ci_read_filter_row_sub4_neon,(ci_row_infop
    row_info, ci_bytep row, ci_const_bytep prev_row),CI_EMPTY);
CI_INTERNAL_FUNCTION(void,ci_read_filter_row_avg3_neon,(ci_row_infop
    row_info, ci_bytep row, ci_const_bytep prev_row),CI_EMPTY);
CI_INTERNAL_FUNCTION(void,ci_read_filter_row_avg4_neon,(ci_row_infop
    row_info, ci_bytep row, ci_const_bytep prev_row),CI_EMPTY);
CI_INTERNAL_FUNCTION(void,ci_read_filter_row_paeth3_neon,(ci_row_infop
    row_info, ci_bytep row, ci_const_bytep prev_row),CI_EMPTY);
CI_INTERNAL_FUNCTION(void,ci_read_filter_row_paeth4_neon,(ci_row_infop
    row_info, ci_bytep row, ci_const_bytep prev_row),CI_EMPTY);
#endif

#if CI_MIPS_MSA_IMPLEMENTATION == 1
CI_INTERNAL_FUNCTION(void,ci_read_filter_row_up_msa,(ci_row_infop row_info,
    ci_bytep row, ci_const_bytep prev_row),CI_EMPTY);
CI_INTERNAL_FUNCTION(void,ci_read_filter_row_sub3_msa,(ci_row_infop
    row_info, ci_bytep row, ci_const_bytep prev_row),CI_EMPTY);
CI_INTERNAL_FUNCTION(void,ci_read_filter_row_sub4_msa,(ci_row_infop
    row_info, ci_bytep row, ci_const_bytep prev_row),CI_EMPTY);
CI_INTERNAL_FUNCTION(void,ci_read_filter_row_avg3_msa,(ci_row_infop
    row_info, ci_bytep row, ci_const_bytep prev_row),CI_EMPTY);
CI_INTERNAL_FUNCTION(void,ci_read_filter_row_avg4_msa,(ci_row_infop
    row_info, ci_bytep row, ci_const_bytep prev_row),CI_EMPTY);
CI_INTERNAL_FUNCTION(void,ci_read_filter_row_paeth3_msa,(ci_row_infop
    row_info, ci_bytep row, ci_const_bytep prev_row),CI_EMPTY);
CI_INTERNAL_FUNCTION(void,ci_read_filter_row_paeth4_msa,(ci_row_infop
    row_info, ci_bytep row, ci_const_bytep prev_row),CI_EMPTY);
#endif

#if CI_MIPS_MMI_IMPLEMENTATION > 0
CI_INTERNAL_FUNCTION(void,ci_read_filter_row_up_mmi,(ci_row_infop row_info,
    ci_bytep row, ci_const_bytep prev_row),CI_EMPTY);
CI_INTERNAL_FUNCTION(void,ci_read_filter_row_sub3_mmi,(ci_row_infop
    row_info, ci_bytep row, ci_const_bytep prev_row),CI_EMPTY);
CI_INTERNAL_FUNCTION(void,ci_read_filter_row_sub4_mmi,(ci_row_infop
    row_info, ci_bytep row, ci_const_bytep prev_row),CI_EMPTY);
CI_INTERNAL_FUNCTION(void,ci_read_filter_row_avg3_mmi,(ci_row_infop
    row_info, ci_bytep row, ci_const_bytep prev_row),CI_EMPTY);
CI_INTERNAL_FUNCTION(void,ci_read_filter_row_avg4_mmi,(ci_row_infop
    row_info, ci_bytep row, ci_const_bytep prev_row),CI_EMPTY);
CI_INTERNAL_FUNCTION(void,ci_read_filter_row_paeth3_mmi,(ci_row_infop
    row_info, ci_bytep row, ci_const_bytep prev_row),CI_EMPTY);
CI_INTERNAL_FUNCTION(void,ci_read_filter_row_paeth4_mmi,(ci_row_infop
    row_info, ci_bytep row, ci_const_bytep prev_row),CI_EMPTY);
#endif

#if CI_POWERPC_VSX_OPT > 0
CI_INTERNAL_FUNCTION(void,ci_read_filter_row_up_vsx,(ci_row_infop row_info,
    ci_bytep row, ci_const_bytep prev_row),CI_EMPTY);
CI_INTERNAL_FUNCTION(void,ci_read_filter_row_sub3_vsx,(ci_row_infop
    row_info, ci_bytep row, ci_const_bytep prev_row),CI_EMPTY);
CI_INTERNAL_FUNCTION(void,ci_read_filter_row_sub4_vsx,(ci_row_infop
    row_info, ci_bytep row, ci_const_bytep prev_row),CI_EMPTY);
CI_INTERNAL_FUNCTION(void,ci_read_filter_row_avg3_vsx,(ci_row_infop
    row_info, ci_bytep row, ci_const_bytep prev_row),CI_EMPTY);
CI_INTERNAL_FUNCTION(void,ci_read_filter_row_avg4_vsx,(ci_row_infop
    row_info, ci_bytep row, ci_const_bytep prev_row),CI_EMPTY);
CI_INTERNAL_FUNCTION(void,ci_read_filter_row_paeth3_vsx,(ci_row_infop
    row_info, ci_bytep row, ci_const_bytep prev_row),CI_EMPTY);
CI_INTERNAL_FUNCTION(void,ci_read_filter_row_paeth4_vsx,(ci_row_infop
    row_info, ci_bytep row, ci_const_bytep prev_row),CI_EMPTY);
#endif

#if CI_INTEL_SSE_IMPLEMENTATION > 0
CI_INTERNAL_FUNCTION(void,ci_read_filter_row_sub3_sse2,(ci_row_infop
    row_info, ci_bytep row, ci_const_bytep prev_row),CI_EMPTY);
CI_INTERNAL_FUNCTION(void,ci_read_filter_row_sub4_sse2,(ci_row_infop
    row_info, ci_bytep row, ci_const_bytep prev_row),CI_EMPTY);
CI_INTERNAL_FUNCTION(void,ci_read_filter_row_avg3_sse2,(ci_row_infop
    row_info, ci_bytep row, ci_const_bytep prev_row),CI_EMPTY);
CI_INTERNAL_FUNCTION(void,ci_read_filter_row_avg4_sse2,(ci_row_infop
    row_info, ci_bytep row, ci_const_bytep prev_row),CI_EMPTY);
CI_INTERNAL_FUNCTION(void,ci_read_filter_row_paeth3_sse2,(ci_row_infop
    row_info, ci_bytep row, ci_const_bytep prev_row),CI_EMPTY);
CI_INTERNAL_FUNCTION(void,ci_read_filter_row_paeth4_sse2,(ci_row_infop
    row_info, ci_bytep row, ci_const_bytep prev_row),CI_EMPTY);
#endif

#if CI_LOONGARCH_LSX_IMPLEMENTATION == 1
CI_INTERNAL_FUNCTION(void,ci_read_filter_row_up_lsx,(ci_row_infop
    row_info, ci_bytep row, ci_const_bytep prev_row),CI_EMPTY);
CI_INTERNAL_FUNCTION(void,ci_read_filter_row_sub3_lsx,(ci_row_infop
    row_info, ci_bytep row, ci_const_bytep prev_row),CI_EMPTY);
CI_INTERNAL_FUNCTION(void,ci_read_filter_row_sub4_lsx,(ci_row_infop
    row_info, ci_bytep row, ci_const_bytep prev_row),CI_EMPTY);
CI_INTERNAL_FUNCTION(void,ci_read_filter_row_avg3_lsx,(ci_row_infop
    row_info, ci_bytep row, ci_const_bytep prev_row),CI_EMPTY);
CI_INTERNAL_FUNCTION(void,ci_read_filter_row_avg4_lsx,(ci_row_infop
    row_info, ci_bytep row, ci_const_bytep prev_row),CI_EMPTY);
CI_INTERNAL_FUNCTION(void,ci_read_filter_row_paeth3_lsx,(ci_row_infop
    row_info, ci_bytep row, ci_const_bytep prev_row),CI_EMPTY);
CI_INTERNAL_FUNCTION(void,ci_read_filter_row_paeth4_lsx,(ci_row_infop
    row_info, ci_bytep row, ci_const_bytep prev_row),CI_EMPTY);
#endif

#if CI_RISCV_RVV_OPT > 0
CI_INTERNAL_FUNCTION(void,ci_read_filter_row_up_rvv,(ci_row_infop
    row_info, ci_bytep row, ci_const_bytep prev_row),CI_EMPTY);
CI_INTERNAL_FUNCTION(void,ci_read_filter_row_sub3_rvv,(ci_row_infop
    row_info, ci_bytep row, ci_const_bytep prev_row),CI_EMPTY);
CI_INTERNAL_FUNCTION(void,ci_read_filter_row_sub4_rvv,(ci_row_infop
    row_info, ci_bytep row, ci_const_bytep prev_row),CI_EMPTY);
CI_INTERNAL_FUNCTION(void,ci_read_filter_row_avg3_rvv,(ci_row_infop
    row_info, ci_bytep row, ci_const_bytep prev_row),CI_EMPTY);
CI_INTERNAL_FUNCTION(void,ci_read_filter_row_avg4_rvv,(ci_row_infop
    row_info, ci_bytep row, ci_const_bytep prev_row),CI_EMPTY);
CI_INTERNAL_FUNCTION(void,ci_read_filter_row_paeth3_rvv,(ci_row_infop
    row_info, ci_bytep row, ci_const_bytep prev_row),CI_EMPTY);
CI_INTERNAL_FUNCTION(void,ci_read_filter_row_paeth4_rvv,(ci_row_infop
    row_info, ci_bytep row, ci_const_bytep prev_row),CI_EMPTY);
#endif

/* Choose the best filter to use and filter the row data */
CI_INTERNAL_FUNCTION(void,ci_write_find_filter,(ci_structrp ci_ptr,
    ci_row_infop row_info),CI_EMPTY);

#ifdef CI_SEQUENTIAL_READ_SUPPORTED
CI_INTERNAL_FUNCTION(void,ci_read_IDAT_data,(ci_structrp ci_ptr,
   ci_bytep output, ci_alloc_size_t avail_out),CI_EMPTY);
   /* Read 'avail_out' bytes of data from the IDAT stream.  If the output buffer
    * is NULL the function checks, instead, for the end of the stream.  In this
    * case a benign error will be issued if the stream end is not found or if
    * extra data has to be consumed.
    */
CI_INTERNAL_FUNCTION(void,ci_read_finish_IDAT,(ci_structrp ci_ptr),
   CI_EMPTY);
   /* This cleans up when the IDAT LZ stream does not end when the last image
    * byte is read; there is still some pending input.
    */

CI_INTERNAL_FUNCTION(void,ci_read_finish_row,(ci_structrp ci_ptr),
   CI_EMPTY);
   /* Finish a row while reading, dealing with interlacing passes, etc. */
#endif /* SEQUENTIAL_READ */

/* Initialize the row buffers, etc. */
CI_INTERNAL_FUNCTION(void,ci_read_start_row,(ci_structrp ci_ptr),CI_EMPTY);

#if ZLIB_VERNUM >= 0x1240
CI_INTERNAL_FUNCTION(int,ci_zlib_inflate,(ci_structrp ci_ptr, int flush),
      CI_EMPTY);
#  define CI_INFLATE(pp, flush) ci_zlib_inflate(pp, flush)
#else /* Zlib < 1.2.4 */
#  define CI_INFLATE(pp, flush) inflate(&(pp)->zstream, flush)
#endif /* Zlib < 1.2.4 */

#ifdef CI_READ_TRANSFORMS_SUPPORTED
/* Optional call to update the users info structure */
CI_INTERNAL_FUNCTION(void,ci_read_transform_info,(ci_structrp ci_ptr,
    ci_inforp info_ptr),CI_EMPTY);
#endif

/* Shared transform functions, defined in citran.c */
#if defined(CI_WRITE_FILLER_SUPPORTED) || \
    defined(CI_READ_STRIP_ALPHA_SUPPORTED)
CI_INTERNAL_FUNCTION(void,ci_do_strip_channel,(ci_row_infop row_info,
    ci_bytep row, int at_start),CI_EMPTY);
#endif

#ifdef CI_16BIT_SUPPORTED
#if defined(CI_READ_SWAP_SUPPORTED) || defined(CI_WRITE_SWAP_SUPPORTED)
CI_INTERNAL_FUNCTION(void,ci_do_swap,(ci_row_infop row_info,
    ci_bytep row),CI_EMPTY);
#endif
#endif

#if defined(CI_READ_PACKSWAP_SUPPORTED) || \
    defined(CI_WRITE_PACKSWAP_SUPPORTED)
CI_INTERNAL_FUNCTION(void,ci_do_packswap,(ci_row_infop row_info,
    ci_bytep row),CI_EMPTY);
#endif

#if defined(CI_READ_INVERT_SUPPORTED) || defined(CI_WRITE_INVERT_SUPPORTED)
CI_INTERNAL_FUNCTION(void,ci_do_invert,(ci_row_infop row_info,
    ci_bytep row),CI_EMPTY);
#endif

#if defined(CI_READ_BGR_SUPPORTED) || defined(CI_WRITE_BGR_SUPPORTED)
CI_INTERNAL_FUNCTION(void,ci_do_bgr,(ci_row_infop row_info,
    ci_bytep row),CI_EMPTY);
#endif

/* The following decodes the appropriate chunks, and does error correction,
 * then calls the appropriate callback for the chunk if it is valid.
 */
typedef enum
{
   /* Result of a call to ci_handle_chunk made to handle the current chunk
    * ci_struct::chunk_name on read.  Always informational, either the stream
    * is read for the next chunk or the routine will call ci_error.
    *
    * NOTE: order is important internally.  handled_saved and above are regarded
    * as handling the chunk.
    */
   handled_error = 0,  /* bad crc or known and bad format or too long */
   handled_discarded,  /* not saved in the unknown chunk list */
   handled_saved,      /* saved in the unknown chunk list */
   handled_ok          /* known, supported and handled without error */
} ci_handle_result_code;

CI_INTERNAL_FUNCTION(ci_handle_result_code,ci_handle_unknown,
    (ci_structrp ci_ptr, ci_inforp info_ptr, ci_uint_32 length, int keep),
    CI_EMPTY);
   /* This is the function that gets called for unknown chunks.  The 'keep'
    * argument is either non-zero for a known chunk that has been set to be
    * handled as unknown or zero for an unknown chunk.  By default the function
    * just skips the chunk or errors out if it is critical.
    */

CI_INTERNAL_FUNCTION(ci_handle_result_code,ci_handle_chunk,
    (ci_structrp ci_ptr, ci_inforp info_ptr, ci_uint_32 length),CI_EMPTY);
   /* This handles the current chunk ci_ptr->chunk_name with unread
    * data[length] and returns one of the above result codes.
    */

#if defined(CI_READ_UNKNOWN_CHUNKS_SUPPORTED) ||\
    defined(CI_HANDLE_AS_UNKNOWN_SUPPORTED)
CI_INTERNAL_FUNCTION(int,ci_chunk_unknown_handling,
    (ci_const_structrp ci_ptr, ci_uint_32 chunk_name),CI_EMPTY);
   /* Exactly as the API ci_handle_as_unknown() except that the argument is a
    * 32-bit chunk name, not a string.
    */
#endif /* READ_UNKNOWN_CHUNKS || HANDLE_AS_UNKNOWN */

/* Handle the transformations for reading and writing */
#ifdef CI_READ_TRANSFORMS_SUPPORTED
CI_INTERNAL_FUNCTION(void,ci_do_read_transformations,(ci_structrp ci_ptr,
   ci_row_infop row_info),CI_EMPTY);
#endif
#ifdef CI_WRITE_TRANSFORMS_SUPPORTED
CI_INTERNAL_FUNCTION(void,ci_do_write_transformations,(ci_structrp ci_ptr,
   ci_row_infop row_info),CI_EMPTY);
#endif

#ifdef CI_READ_TRANSFORMS_SUPPORTED
CI_INTERNAL_FUNCTION(void,ci_init_read_transformations,(ci_structrp ci_ptr),
    CI_EMPTY);
#endif

#ifdef CI_PROGRESSIVE_READ_SUPPORTED
CI_INTERNAL_FUNCTION(void,ci_push_read_chunk,(ci_structrp ci_ptr,
    ci_inforp info_ptr),CI_EMPTY);
CI_INTERNAL_FUNCTION(void,ci_push_read_sig,(ci_structrp ci_ptr,
    ci_inforp info_ptr),CI_EMPTY);
CI_INTERNAL_FUNCTION(void,ci_push_check_crc,(ci_structrp ci_ptr),CI_EMPTY);
CI_INTERNAL_FUNCTION(void,ci_push_save_buffer,(ci_structrp ci_ptr),
    CI_EMPTY);
CI_INTERNAL_FUNCTION(void,ci_push_restore_buffer,(ci_structrp ci_ptr,
    ci_bytep buffer, size_t buffer_length),CI_EMPTY);
CI_INTERNAL_FUNCTION(void,ci_push_read_IDAT,(ci_structrp ci_ptr),CI_EMPTY);
CI_INTERNAL_FUNCTION(void,ci_process_IDAT_data,(ci_structrp ci_ptr,
    ci_bytep buffer, size_t buffer_length),CI_EMPTY);
CI_INTERNAL_FUNCTION(void,ci_push_process_row,(ci_structrp ci_ptr),
    CI_EMPTY);
CI_INTERNAL_FUNCTION(void,ci_push_have_info,(ci_structrp ci_ptr,
   ci_inforp info_ptr),CI_EMPTY);
CI_INTERNAL_FUNCTION(void,ci_push_have_end,(ci_structrp ci_ptr,
   ci_inforp info_ptr),CI_EMPTY);
CI_INTERNAL_FUNCTION(void,ci_push_have_row,(ci_structrp ci_ptr,
    ci_bytep row),CI_EMPTY);
CI_INTERNAL_FUNCTION(void,ci_push_read_end,(ci_structrp ci_ptr,
    ci_inforp info_ptr),CI_EMPTY);
CI_INTERNAL_FUNCTION(void,ci_process_some_data,(ci_structrp ci_ptr,
    ci_inforp info_ptr),CI_EMPTY);
CI_INTERNAL_FUNCTION(void,ci_read_push_finish_row,(ci_structrp ci_ptr),
    CI_EMPTY);
#endif /* PROGRESSIVE_READ */

#ifdef CI_iCCP_SUPPORTED
/* Routines for checking parts of an ICC profile. */
#ifdef CI_READ_iCCP_SUPPORTED
CI_INTERNAL_FUNCTION(int,ci_icc_check_length,(ci_const_structrp ci_ptr,
   ci_const_charp name, ci_uint_32 profile_length), CI_EMPTY);
#endif /* READ_iCCP */
CI_INTERNAL_FUNCTION(int,ci_icc_check_header,(ci_const_structrp ci_ptr,
   ci_const_charp name, ci_uint_32 profile_length,
   ci_const_bytep profile /* first 132 bytes only */, int color_type),
   CI_EMPTY);
CI_INTERNAL_FUNCTION(int,ci_icc_check_tag_table,(ci_const_structrp ci_ptr,
   ci_const_charp name, ci_uint_32 profile_length,
   ci_const_bytep profile /* header plus whole tag table */), CI_EMPTY);
#endif /* iCCP */

#ifdef CI_READ_RGB_TO_GRAY_SUPPORTED
CI_INTERNAL_FUNCTION(void,ci_set_rgb_coefficients, (ci_structrp ci_ptr),
   CI_EMPTY);
   /* Set the rgb_to_gray coefficients from the cHRM Y values (if unset) */
#endif /* READ_RGB_TO_GRAY */

/* Added at libci version 1.4.0 */
CI_INTERNAL_FUNCTION(void,ci_check_IHDR,(ci_const_structrp ci_ptr,
    ci_uint_32 width, ci_uint_32 height, int bit_depth,
    int color_type, int interlace_type, int compression_type,
    int filter_type),CI_EMPTY);

/* Added at libci version 1.5.10 */
#if defined(CI_READ_CHECK_FOR_INVALID_INDEX_SUPPORTED) || \
    defined(CI_WRITE_CHECK_FOR_INVALID_INDEX_SUPPORTED)
CI_INTERNAL_FUNCTION(void,ci_do_check_palette_indexes,
   (ci_structrp ci_ptr, ci_row_infop row_info),CI_EMPTY);
#endif

#if defined(CI_FLOATING_POINT_SUPPORTED) && defined(CI_ERROR_TEXT_SUPPORTED)
CI_INTERNAL_FUNCTION(void,ci_fixed_error,(ci_const_structrp ci_ptr,
   ci_const_charp name),CI_NORETURN);
#endif

/* Puts 'string' into 'buffer' at buffer[pos], taking care never to overwrite
 * the end.  Always leaves the buffer nul terminated.  Never errors out (and
 * there is no error code.)
 */
CI_INTERNAL_FUNCTION(size_t,ci_safecat,(ci_charp buffer, size_t bufsize,
   size_t pos, ci_const_charp string),CI_EMPTY);

/* Various internal functions to handle formatted warning messages, currently
 * only implemented for warnings.
 */
#if defined(CI_WARNINGS_SUPPORTED) || defined(CI_TIME_RFC1123_SUPPORTED)
/* Utility to dump an unsigned value into a buffer, given a start pointer and
 * and end pointer (which should point just *beyond* the end of the buffer!)
 * Returns the pointer to the start of the formatted string.  This utility only
 * does unsigned values.
 */
CI_INTERNAL_FUNCTION(ci_charp,ci_format_number,(ci_const_charp start,
   ci_charp end, int format, ci_alloc_size_t number),CI_EMPTY);

/* Convenience macro that takes an array: */
#define CI_FORMAT_NUMBER(buffer,format,number) \
   ci_format_number(buffer, buffer + (sizeof buffer), format, number)

/* Suggested size for a number buffer (enough for 64 bits and a sign!) */
#define CI_NUMBER_BUFFER_SIZE 24

/* These are the integer formats currently supported, the name is formed from
 * the standard printf(3) format string.
 */
#define CI_NUMBER_FORMAT_u     1 /* chose unsigned API! */
#define CI_NUMBER_FORMAT_02u   2
#define CI_NUMBER_FORMAT_d     1 /* chose signed API! */
#define CI_NUMBER_FORMAT_02d   2
#define CI_NUMBER_FORMAT_x     3
#define CI_NUMBER_FORMAT_02x   4
#define CI_NUMBER_FORMAT_fixed 5 /* choose the signed API */
#endif

#ifdef CI_WARNINGS_SUPPORTED
/* New defines and members adding in libci-1.5.4 */
#  define CI_WARNING_PARAMETER_SIZE 32
#  define CI_WARNING_PARAMETER_COUNT 8 /* Maximum 9; see cierror.c */

/* An l-value of this type has to be passed to the APIs below to cache the
 * values of the parameters to a formatted warning message.
 */
typedef char ci_warning_parameters[CI_WARNING_PARAMETER_COUNT][
   CI_WARNING_PARAMETER_SIZE];

CI_INTERNAL_FUNCTION(void,ci_warning_parameter,(ci_warning_parameters p,
   int number, ci_const_charp string),CI_EMPTY);
   /* Parameters are limited in size to CI_WARNING_PARAMETER_SIZE characters,
    * including the trailing '\0'.
    */
CI_INTERNAL_FUNCTION(void,ci_warning_parameter_unsigned,
   (ci_warning_parameters p, int number, int format, ci_alloc_size_t value),
   CI_EMPTY);
   /* Use ci_alloc_size_t because it is an unsigned type as big as any we
    * need to output.  Use the following for a signed value.
    */
CI_INTERNAL_FUNCTION(void,ci_warning_parameter_signed,
   (ci_warning_parameters p, int number, int format, ci_int_32 value),
   CI_EMPTY);

CI_INTERNAL_FUNCTION(void,ci_formatted_warning,(ci_const_structrp ci_ptr,
   ci_warning_parameters p, ci_const_charp message),CI_EMPTY);
   /* 'message' follows the X/Open approach of using @1, @2 to insert
    * parameters previously supplied using the above functions.  Errors in
    * specifying the parameters will simply result in garbage substitutions.
    */
#endif

#ifdef CI_BENIGN_ERRORS_SUPPORTED
/* Application errors (new in 1.6); use these functions (declared below) for
 * errors in the parameters or order of API function calls on read.  The
 * 'warning' should be used for an error that can be handled completely; the
 * 'error' for one which can be handled safely but which may lose application
 * information or settings.
 *
 * By default these both result in a ci_error call prior to release, while in a
 * released version the 'warning' is just a warning.  However if the application
 * explicitly disables benign errors (explicitly permitting the code to lose
 * information) they both turn into warnings.
 *
 * If benign errors aren't supported they end up as the corresponding base call
 * (ci_warning or ci_error.)
 */
CI_INTERNAL_FUNCTION(void,ci_app_warning,(ci_const_structrp ci_ptr,
   ci_const_charp message),CI_EMPTY);
   /* The application provided invalid parameters to an API function or called
    * an API function at the wrong time, libci can completely recover.
    */

CI_INTERNAL_FUNCTION(void,ci_app_error,(ci_const_structrp ci_ptr,
   ci_const_charp message),CI_EMPTY);
   /* As above but libci will ignore the call, or attempt some other partial
    * recovery from the error.
    */
#else
#  define ci_app_warning(pp,s) ci_warning(pp,s)
#  define ci_app_error(pp,s) ci_error(pp,s)
#endif

CI_INTERNAL_FUNCTION(void,ci_chunk_report,(ci_const_structrp ci_ptr,
   ci_const_charp message, int error),CI_EMPTY);
   /* Report a recoverable issue in chunk data.  On read this is used to report
    * a problem found while reading a particular chunk and the
    * ci_chunk_benign_error or ci_chunk_warning function is used as
    * appropriate.  On write this is used to report an error that comes from
    * data set via an application call to a ci_set_ API and ci_app_error or
    * ci_app_warning is used as appropriate.
    *
    * The 'error' parameter must have one of the following values:
    */
#define CI_CHUNK_WARNING     0 /* never an error */
#define CI_CHUNK_WRITE_ERROR 1 /* an error only on write */
#define CI_CHUNK_ERROR       2 /* always an error */

/* ASCII to FP interfaces, currently only implemented if sCAL
 * support is required.
 */
#if defined(CI_sCAL_SUPPORTED)
/* MAX_DIGITS is actually the maximum number of characters in an sCAL
 * width or height, derived from the precision (number of significant
 * digits - a build time settable option) and assumptions about the
 * maximum ridiculous exponent.
 */
#define CI_sCAL_MAX_DIGITS (CI_sCAL_PRECISION+1/*.*/+1/*E*/+10/*exponent*/)

#ifdef CI_FLOATING_POINT_SUPPORTED
CI_INTERNAL_FUNCTION(void,ci_ascii_from_fp,(ci_const_structrp ci_ptr,
   ci_charp ascii, size_t size, double fp, unsigned int precision),
   CI_EMPTY);
#endif /* FLOATING_POINT */

#ifdef CI_FIXED_POINT_SUPPORTED
CI_INTERNAL_FUNCTION(void,ci_ascii_from_fixed,(ci_const_structrp ci_ptr,
   ci_charp ascii, size_t size, ci_fixed_point fp),CI_EMPTY);
#endif /* FIXED_POINT */
#endif /* sCAL */

#if defined(CI_sCAL_SUPPORTED) || defined(CI_pCAL_SUPPORTED)
/* An internal API to validate the format of a floating point number.
 * The result is the index of the next character.  If the number is
 * not valid it will be the index of a character in the supposed number.
 *
 * The format of a number is defined in the CI extensions specification
 * and this API is strictly conformant to that spec, not anyone elses!
 *
 * The format as a regular expression is:
 *
 * [+-]?[0-9]+.?([Ee][+-]?[0-9]+)?
 *
 * or:
 *
 * [+-]?.[0-9]+(.[0-9]+)?([Ee][+-]?[0-9]+)?
 *
 * The complexity is that either integer or fraction must be present and the
 * fraction is permitted to have no digits only if the integer is present.
 *
 * NOTE: The dangling E problem.
 *   There is a CI valid floating point number in the following:
 *
 *       CI floating point numbers are not greedy.
 *
 *   Working this out requires *TWO* character lookahead (because of the
 *   sign), the parser does not do this - it will fail at the 'r' - this
 *   doesn't matter for CI sCAL chunk values, but it requires more care
 *   if the value were ever to be embedded in something more complex.  Use
 *   ANSI-C strtod if you need the lookahead.
 */
/* State table for the parser. */
#define CI_FP_INTEGER    0  /* before or in integer */
#define CI_FP_FRACTION   1  /* before or in fraction */
#define CI_FP_EXPONENT   2  /* before or in exponent */
#define CI_FP_STATE      3  /* mask for the above */
#define CI_FP_SAW_SIGN   4  /* Saw +/- in current state */
#define CI_FP_SAW_DIGIT  8  /* Saw a digit in current state */
#define CI_FP_SAW_DOT   16  /* Saw a dot in current state */
#define CI_FP_SAW_E     32  /* Saw an E (or e) in current state */
#define CI_FP_SAW_ANY   60  /* Saw any of the above 4 */

/* These three values don't affect the parser.  They are set but not used.
 */
#define CI_FP_WAS_VALID 64  /* Preceding substring is a valid fp number */
#define CI_FP_NEGATIVE 128  /* A negative number, including "-0" */
#define CI_FP_NONZERO  256  /* A non-zero value */
#define CI_FP_STICKY   448  /* The above three flags */

/* This is available for the caller to store in 'state' if required.  Do not
 * call the parser after setting it (the parser sometimes clears it.)
 */
#define CI_FP_INVALID  512  /* Available for callers as a distinct value */

/* Result codes for the parser (boolean - true means ok, false means
 * not ok yet.)
 */
#define CI_FP_MAYBE      0  /* The number may be valid in the future */
#define CI_FP_OK         1  /* The number is valid */

/* Tests on the sticky non-zero and negative flags.  To pass these checks
 * the state must also indicate that the whole number is valid - this is
 * achieved by testing CI_FP_SAW_DIGIT (see the implementation for why this
 * is equivalent to CI_FP_OK above.)
 */
#define CI_FP_NZ_MASK (CI_FP_SAW_DIGIT | CI_FP_NEGATIVE | CI_FP_NONZERO)
   /* NZ_MASK: the string is valid and a non-zero negative value */
#define CI_FP_Z_MASK (CI_FP_SAW_DIGIT | CI_FP_NONZERO)
   /* Z MASK: the string is valid and a non-zero value. */
   /* CI_FP_SAW_DIGIT: the string is valid. */
#define CI_FP_IS_ZERO(state) (((state) & CI_FP_Z_MASK) == CI_FP_SAW_DIGIT)
#define CI_FP_IS_POSITIVE(state) (((state) & CI_FP_NZ_MASK) == CI_FP_Z_MASK)
#define CI_FP_IS_NEGATIVE(state) (((state) & CI_FP_NZ_MASK) == CI_FP_NZ_MASK)

/* The actual parser.  This can be called repeatedly. It updates
 * the index into the string and the state variable (which must
 * be initialized to 0).  It returns a result code, as above.  There
 * is no point calling the parser any more if it fails to advance to
 * the end of the string - it is stuck on an invalid character (or
 * terminated by '\0').
 *
 * Note that the pointer will consume an E or even an E+ and then leave
 * a 'maybe' state even though a preceding integer.fraction is valid.
 * The CI_FP_WAS_VALID flag indicates that a preceding substring was
 * a valid number.  It's possible to recover from this by calling
 * the parser again (from the start, with state 0) but with a string
 * that omits the last character (i.e. set the size to the index of
 * the problem character.)  This has not been tested within libci.
 */
CI_INTERNAL_FUNCTION(int,ci_check_fp_number,(ci_const_charp string,
   size_t size, int *statep, size_t *whereami),CI_EMPTY);

/* This is the same but it checks a complete string and returns true
 * only if it just contains a floating point number.  As of 1.5.4 this
 * function also returns the state at the end of parsing the number if
 * it was valid (otherwise it returns 0.)  This can be used for testing
 * for negative or zero values using the sticky flag.
 */
CI_INTERNAL_FUNCTION(int,ci_check_fp_string,(ci_const_charp string,
   size_t size),CI_EMPTY);
#endif /* pCAL || sCAL */

#if defined(CI_READ_GAMMA_SUPPORTED) ||\
    defined(CI_COLORSPACE_SUPPORTED) ||\
    defined(CI_INCH_CONVERSIONS_SUPPORTED) ||\
    defined(CI_READ_pHYs_SUPPORTED)
/* Added at libci version 1.5.0 */
/* This is a utility to provide a*times/div (rounded) and indicate
 * if there is an overflow.  The result is a boolean - false (0)
 * for overflow, true (1) if no overflow, in which case *res
 * holds the result.
 */
CI_INTERNAL_FUNCTION(int,ci_muldiv,(ci_fixed_point_p res, ci_fixed_point a,
   ci_int_32 multiplied_by, ci_int_32 divided_by),CI_EMPTY);

/* Calculate a reciprocal - used for gamma values.  This returns
 * 0 if the argument is 0 in order to maintain an undefined value;
 * there are no warnings.
 */
CI_INTERNAL_FUNCTION(ci_fixed_point,ci_reciprocal,(ci_fixed_point a),
   CI_EMPTY);
#endif

#ifdef CI_READ_GAMMA_SUPPORTED
/* The same but gives a reciprocal of the product of two fixed point
 * values.  Accuracy is suitable for gamma calculations but this is
 * not exact - use ci_muldiv for that.  Only required at present on read.
 */
CI_INTERNAL_FUNCTION(ci_fixed_point,ci_reciprocal2,(ci_fixed_point a,
   ci_fixed_point b),CI_EMPTY);

/* Return true if the gamma value is significantly different from 1.0 */
CI_INTERNAL_FUNCTION(int,ci_gamma_significant,(ci_fixed_point gamma_value),
   CI_EMPTY);

/* CIv3: 'resolve' the file gamma according to the new CIv3 rules for colour
 * space information.
 *
 * NOTE: this uses precisely those chunks that libci supports.  For example it
 * doesn't use iCCP and it can only use cICP for known and manageable
 * transforms.  For this reason a gamma specified by ci_set_gamma always takes
 * precedence.
 */
CI_INTERNAL_FUNCTION(ci_fixed_point,ci_resolve_file_gamma,
   (ci_const_structrp ci_ptr),CI_EMPTY);

/* Internal fixed point gamma correction.  These APIs are called as
 * required to convert single values - they don't need to be fast,
 * they are not used when processing image pixel values.
 *
 * While the input is an 'unsigned' value it must actually be the
 * correct bit value - 0..255 or 0..65535 as required.
 */
CI_INTERNAL_FUNCTION(ci_uint_16,ci_gamma_correct,(ci_structrp ci_ptr,
   unsigned int value, ci_fixed_point gamma_value),CI_EMPTY);
CI_INTERNAL_FUNCTION(ci_uint_16,ci_gamma_16bit_correct,(unsigned int value,
   ci_fixed_point gamma_value),CI_EMPTY);
CI_INTERNAL_FUNCTION(ci_byte,ci_gamma_8bit_correct,(unsigned int value,
   ci_fixed_point gamma_value),CI_EMPTY);
CI_INTERNAL_FUNCTION(void,ci_destroy_gamma_table,(ci_structrp ci_ptr),
   CI_EMPTY);
CI_INTERNAL_FUNCTION(void,ci_build_gamma_table,(ci_structrp ci_ptr,
   int bit_depth),CI_EMPTY);
#endif /* READ_GAMMA */

#ifdef CI_READ_RGB_TO_GRAY_SUPPORTED
/* Set the RGB coefficients if not already set by ci_set_rgb_to_gray */
CI_INTERNAL_FUNCTION(void,ci_set_rgb_coefficients,(ci_structrp ci_ptr),
   CI_EMPTY);
#endif

#if defined(CI_cHRM_SUPPORTED) || defined(CI_READ_RGB_TO_GRAY_SUPPORTED)
CI_INTERNAL_FUNCTION(int,ci_XYZ_from_xy,(ci_XYZ *XYZ, const ci_xy *xy),
   CI_EMPTY);
#endif /* cHRM || READ_RGB_TO_GRAY */

#ifdef CI_COLORSPACE_SUPPORTED
CI_INTERNAL_FUNCTION(int,ci_xy_from_XYZ,(ci_xy *xy, const ci_XYZ *XYZ),
   CI_EMPTY);
#endif

/* SIMPLIFIED READ/WRITE SUPPORT */
#if defined(CI_SIMPLIFIED_READ_SUPPORTED) ||\
   defined(CI_SIMPLIFIED_WRITE_SUPPORTED)
/* The internal structure that ci_image::opaque points to. */
typedef struct ci_control
{
   ci_structp ci_ptr;
   ci_infop   info_ptr;
   ci_voidp   error_buf;           /* Always a jmp_buf at present. */

   ci_const_bytep memory;          /* Memory buffer. */
   size_t          size;            /* Size of the memory buffer. */

   unsigned int for_write       :1; /* Otherwise it is a read structure */
   unsigned int owned_file      :1; /* We own the file in io_ptr */
} ci_control;

/* Return the pointer to the jmp_buf from a ci_control: necessary because C
 * does not reveal the type of the elements of jmp_buf.
 */
#ifdef __cplusplus
#  define ci_control_jmp_buf(pc) (((jmp_buf*)((pc)->error_buf))[0])
#else
#  define ci_control_jmp_buf(pc) ((pc)->error_buf)
#endif

/* Utility to safely execute a piece of libci code catching and logging any
 * errors that might occur.  Returns true on success, false on failure (either
 * of the function or as a result of a ci_error.)
 */
CI_INTERNAL_CALLBACK(void,ci_safe_error,(ci_structp ci_ptr,
   ci_const_charp error_message),CI_NORETURN);

#ifdef CI_WARNINGS_SUPPORTED
CI_INTERNAL_CALLBACK(void,ci_safe_warning,(ci_structp ci_ptr,
   ci_const_charp warning_message),CI_EMPTY);
#else
#  define ci_safe_warning 0/*dummy argument*/
#endif

CI_INTERNAL_FUNCTION(int,ci_safe_execute,(ci_imagep image,
   int (*function)(ci_voidp), ci_voidp arg),CI_EMPTY);

/* Utility to log an error; this also cleans up the ci_image; the function
 * always returns 0 (false).
 */
CI_INTERNAL_FUNCTION(int,ci_image_error,(ci_imagep image,
   ci_const_charp error_message),CI_EMPTY);

#ifndef CI_SIMPLIFIED_READ_SUPPORTED
/* ci_image_free is used by the write code but not exported */
CI_INTERNAL_FUNCTION(void, ci_image_free, (ci_imagep image), CI_EMPTY);
#endif /* !SIMPLIFIED_READ */

#endif /* SIMPLIFIED READ/WRITE */

/* These are initialization functions for hardware specific CI filter
 * optimizations; list these here then select the appropriate one at compile
 * time using the macro CI_FILTER_OPTIMIZATIONS.  If the macro is not defined
 * the generic code is used.
 */
#ifdef CI_FILTER_OPTIMIZATIONS
CI_INTERNAL_FUNCTION(void, CI_FILTER_OPTIMIZATIONS, (ci_structp ci_ptr,
   unsigned int bpp), CI_EMPTY);
   /* Just declare the optimization that will be used */
#else
   /* List *all* the possible optimizations here - this branch is required if
    * the builder of libci passes the definition of CI_FILTER_OPTIMIZATIONS in
    * CFLAGS in place of CPPFLAGS *and* uses symbol prefixing.
    */
#  if CI_ARM_NEON_OPT > 0
CI_INTERNAL_FUNCTION(void, ci_init_filter_functions_neon,
   (ci_structp ci_ptr, unsigned int bpp), CI_EMPTY);
#endif

#if CI_MIPS_MSA_IMPLEMENTATION == 1
CI_INTERNAL_FUNCTION(void, ci_init_filter_functions_mips,
   (ci_structp ci_ptr, unsigned int bpp), CI_EMPTY);
#endif

#  if CI_MIPS_MMI_IMPLEMENTATION > 0
CI_INTERNAL_FUNCTION(void, ci_init_filter_functions_mips,
   (ci_structp ci_ptr, unsigned int bpp), CI_EMPTY);
#  endif

#  if CI_INTEL_SSE_IMPLEMENTATION > 0
CI_INTERNAL_FUNCTION(void, ci_init_filter_functions_sse2,
   (ci_structp ci_ptr, unsigned int bpp), CI_EMPTY);
#  endif
#endif

#if CI_LOONGARCH_LSX_OPT > 0
CI_INTERNAL_FUNCTION(void, ci_init_filter_functions_lsx,
    (ci_structp ci_ptr, unsigned int bpp), CI_EMPTY);
#endif

#  if CI_RISCV_RVV_OPT > 0
CI_INTERNAL_FUNCTION(void, ci_init_filter_functions_rvv,
   (ci_structp ci_ptr, unsigned int bpp), CI_EMPTY);
#endif

CI_INTERNAL_FUNCTION(ci_uint_32, ci_check_keyword, (ci_structrp ci_ptr,
   ci_const_charp key, ci_bytep new_key), CI_EMPTY);

#if CI_ARM_NEON_IMPLEMENTATION == 1
CI_INTERNAL_FUNCTION(void,
                      ci_riffle_palette_neon,
                      (ci_structrp),
                      CI_EMPTY);
CI_INTERNAL_FUNCTION(int,
                      ci_do_expand_palette_rgba8_neon,
                      (ci_structrp,
                       ci_row_infop,
                       ci_const_bytep,
                       const ci_bytepp,
                       const ci_bytepp),
                      CI_EMPTY);
CI_INTERNAL_FUNCTION(int,
                      ci_do_expand_palette_rgb8_neon,
                      (ci_structrp,
                       ci_row_infop,
                       ci_const_bytep,
                       const ci_bytepp,
                       const ci_bytepp),
                      CI_EMPTY);
#endif

/* Maintainer: Put new private prototypes here ^ */

#include "cidebug.h"

#ifdef __cplusplus
}
#endif

#endif /* CI_VERSION_INFO_ONLY */

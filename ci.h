/* ci.h - header file for CI reference library
 *
 * libci version 1.6.50.git
 *
 * Copyright (c) 2018-2025 Cosmin Truta
 * Copyright (c) 1998-2002,2004,2006-2018 Glenn Randers-Pehrson
 * Copyright (c) 1996-1997 Andreas Dilger
 * Copyright (c) 1995-1996 Guy Eric Schalnat, Group 42, Inc.
 *
 * This code is released under the libci license. (See LICENSE, below.)
 *
 * Authors and maintainers:
 *   libci versions 0.71, May 1995, through 0.88, January 1996: Guy Schalnat
 *   libci versions 0.89, June 1996, through 0.96, May 1997: Andreas Dilger
 *   libci versions 0.97, January 1998, through 1.6.35, July 2018:
 *     Glenn Randers-Pehrson
 *   libci versions 1.6.36, December 2018, through 1.6.49, June 2025:
 *     Cosmin Truta
 *   See also "Contributing Authors", below.
 */

/*
 * COPYRIGHT NOTICE, DISCLAIMER, and LICENSE
 * =========================================
 *
 * CI Reference Library License version 2
 * ---------------------------------------
 *
 *  * Copyright (c) 1995-2025 The CI Reference Library Authors.
 *  * Copyright (c) 2018-2025 Cosmin Truta.
 *  * Copyright (c) 2000-2002, 2004, 2006-2018 Glenn Randers-Pehrson.
 *  * Copyright (c) 1996-1997 Andreas Dilger.
 *  * Copyright (c) 1995-1996 Guy Eric Schalnat, Group 42, Inc.
 *
 * The software is supplied "as is", without warranty of any kind,
 * express or implied, including, without limitation, the warranties
 * of merchantability, fitness for a particular purpose, title, and
 * non-infringement.  In no event shall the Copyright owners, or
 * anyone distributing the software, be liable for any damages or
 * other liability, whether in contract, tort or otherwise, arising
 * from, out of, or in connection with the software, or the use or
 * other dealings in the software, even if advised of the possibility
 * of such damage.
 *
 * Permission is hereby granted to use, copy, modify, and distribute
 * this software, or portions hereof, for any purpose, without fee,
 * subject to the following restrictions:
 *
 *  1. The origin of this software must not be misrepresented; you
 *     must not claim that you wrote the original software.  If you
 *     use this software in a product, an acknowledgment in the product
 *     documentation would be appreciated, but is not required.
 *
 *  2. Altered source versions must be plainly marked as such, and must
 *     not be misrepresented as being the original software.
 *
 *  3. This Copyright notice may not be removed or altered from any
 *     source or altered source distribution.
 *
 *
 * CI Reference Library License version 1 (for libci 0.5 through 1.6.35)
 * -----------------------------------------------------------------------
 *
 * libci versions 1.0.7, July 1, 2000, through 1.6.35, July 15, 2018 are
 * Copyright (c) 2000-2002, 2004, 2006-2018 Glenn Randers-Pehrson, are
 * derived from libci-1.0.6, and are distributed according to the same
 * disclaimer and license as libci-1.0.6 with the following individuals
 * added to the list of Contributing Authors:
 *
 *     Simon-Pierre Cadieux
 *     Eric S. Raymond
 *     Mans Rullgard
 *     Cosmin Truta
 *     Gilles Vollant
 *     James Yu
 *     Mandar Sahastrabuddhe
 *     Google Inc.
 *     Vadim Barkov
 *
 * and with the following additions to the disclaimer:
 *
 *     There is no warranty against interference with your enjoyment of
 *     the library or against infringement.  There is no warranty that our
 *     efforts or the library will fulfill any of your particular purposes
 *     or needs.  This library is provided with all faults, and the entire
 *     risk of satisfactory quality, performance, accuracy, and effort is
 *     with the user.
 *
 * Some files in the "contrib" directory and some configure-generated
 * files that are distributed with libci have other copyright owners, and
 * are released under other open source licenses.
 *
 * libci versions 0.97, January 1998, through 1.0.6, March 20, 2000, are
 * Copyright (c) 1998-2000 Glenn Randers-Pehrson, are derived from
 * libci-0.96, and are distributed according to the same disclaimer and
 * license as libci-0.96, with the following individuals added to the
 * list of Contributing Authors:
 *
 *     Tom Lane
 *     Glenn Randers-Pehrson
 *     Willem van Schaik
 *
 * libci versions 0.89, June 1996, through 0.96, May 1997, are
 * Copyright (c) 1996-1997 Andreas Dilger, are derived from libci-0.88,
 * and are distributed according to the same disclaimer and license as
 * libci-0.88, with the following individuals added to the list of
 * Contributing Authors:
 *
 *     John Bowler
 *     Kevin Bracey
 *     Sam Bushell
 *     Magnus Holmgren
 *     Greg Roelofs
 *     Tom Tanner
 *
 * Some files in the "scripts" directory have other copyright owners,
 * but are released under this license.
 *
 * libci versions 0.5, May 1995, through 0.88, January 1996, are
 * Copyright (c) 1995-1996 Guy Eric Schalnat, Group 42, Inc.
 *
 * For the purposes of this copyright and license, "Contributing Authors"
 * is defined as the following set of individuals:
 *
 *     Andreas Dilger
 *     Dave Martindale
 *     Guy Eric Schalnat
 *     Paul Schmidt
 *     Tim Wegner
 *
 * The CI Reference Library is supplied "AS IS".  The Contributing
 * Authors and Group 42, Inc. disclaim all warranties, expressed or
 * implied, including, without limitation, the warranties of
 * merchantability and of fitness for any purpose.  The Contributing
 * Authors and Group 42, Inc. assume no liability for direct, indirect,
 * incidental, special, exemplary, or consequential damages, which may
 * result from the use of the CI Reference Library, even if advised of
 * the possibility of such damage.
 *
 * Permission is hereby granted to use, copy, modify, and distribute this
 * source code, or portions hereof, for any purpose, without fee, subject
 * to the following restrictions:
 *
 *  1. The origin of this source code must not be misrepresented.
 *
 *  2. Altered versions must be plainly marked as such and must not
 *     be misrepresented as being the original source.
 *
 *  3. This Copyright notice may not be removed or altered from any
 *     source or altered source distribution.
 *
 * The Contributing Authors and Group 42, Inc. specifically permit,
 * without fee, and encourage the use of this source code as a component
 * to supporting the CI file format in commercial products.  If you use
 * this source code in a product, acknowledgment is not required but would
 * be appreciated.
 *
 * END OF COPYRIGHT NOTICE, DISCLAIMER, and LICENSE.
 *
 * TRADEMARK
 * =========
 *
 * The name "libci" has not been registered by the Copyright owners
 * as a trademark in any jurisdiction.  However, because libci has
 * been distributed and maintained world-wide, continually since 1995,
 * the Copyright owners claim "common-law trademark protection" in any
 * jurisdiction where common-law trademark is recognized.
 */

/*
 * A "ci_get_copyright" function is available, for convenient use in "about"
 * boxes and the like:
 *
 *    printf("%s", ci_get_copyright(NULL));
 *
 * Also, the CI logo (in CI format, of course) is supplied in the
 * files "cibar.ci" and "cibar.jpg (88x31) and "cinow.ci" (98x31).
 */

/*
 * The contributing authors would like to thank all those who helped
 * with testing, bug fixes, and patience.  This wouldn't have been
 * possible without all of you.
 *
 * Thanks to Frank J. T. Wojcik for helping with the documentation.
 */

/* Note about libci version numbers:
 *
 *    Due to various miscommunications, unforeseen code incompatibilities
 *    and occasional factors outside the authors' control, version numbering
 *    on the library has not always been consistent and straightforward.
 *    The following table summarizes matters since version 0.89c, which was
 *    the first widely used release:
 *
 *    source                 ci.h  ci.h  shared-lib
 *    version                string   int  version
 *    -------                ------ -----  ----------
 *    0.89c "1.0 beta 3"     0.89      89  1.0.89
 *    0.90  "1.0 beta 4"     0.90      90  0.90  [should have been 2.0.90]
 *    0.95  "1.0 beta 5"     0.95      95  0.95  [should have been 2.0.95]
 *    0.96  "1.0 beta 6"     0.96      96  0.96  [should have been 2.0.96]
 *    0.97b "1.00.97 beta 7" 1.00.97   97  1.0.1 [should have been 2.0.97]
 *    0.97c                  0.97      97  2.0.97
 *    0.98                   0.98      98  2.0.98
 *    0.99                   0.99      98  2.0.99
 *    0.99a-m                0.99      99  2.0.99
 *    1.00                   1.00     100  2.1.0 [100 should be 10000]
 *    1.0.0      (from here on, the   100  2.1.0 [100 should be 10000]
 *    1.0.1       ci.h string is   10001  2.1.0
 *    1.0.1a-e    identical to the  10002  from here on, the shared library
 *    1.0.2       source version)   10002  is 2.V where V is the source code
 *    1.0.2a-b                      10003  version, except as noted.
 *    1.0.3                         10003
 *    1.0.3a-d                      10004
 *    1.0.4                         10004
 *    1.0.4a-f                      10005
 *    1.0.5 (+ 2 patches)           10005
 *    1.0.5a-d                      10006
 *    1.0.5e-r                      10100 (not source compatible)
 *    1.0.5s-v                      10006 (not binary compatible)
 *    1.0.6 (+ 3 patches)           10006 (still binary incompatible)
 *    1.0.6d-f                      10007 (still binary incompatible)
 *    1.0.6g                        10007
 *    1.0.6h                        10007  10.6h (testing xy.z so-numbering)
 *    1.0.6i                        10007  10.6i
 *    1.0.6j                        10007  2.1.0.6j (incompatible with 1.0.0)
 *    1.0.7beta11-14        DLLNUM  10007  2.1.0.7beta11-14 (binary compatible)
 *    1.0.7beta15-18           1    10007  2.1.0.7beta15-18 (binary compatible)
 *    1.0.7rc1-2               1    10007  2.1.0.7rc1-2 (binary compatible)
 *    1.0.7                    1    10007  (still compatible)
 *    ...
 *    1.0.69                  10    10069  10.so.0.69[.0]
 *    ...
 *    1.2.59                  13    10259  12.so.0.59[.0]
 *    ...
 *    1.4.20                  14    10420  14.so.0.20[.0]
 *    ...
 *    1.5.30                  15    10530  15.so.15.30[.0]
 *    ...
 *    1.6.49                  16    10649  16.so.16.49[.0]
 *
 *    Henceforth the source version will match the shared-library major and
 *    minor numbers; the shared-library major version number will be used for
 *    changes in backward compatibility, as it is intended.
 *    The CI_LIBCI_VER macro, which is not used within libci but is
 *    available for applications, is an unsigned integer of the form XYYZZ
 *    corresponding to the source version X.Y.Z (leading zeros in Y and Z).
 *    Beta versions were given the previous public release number plus a
 *    letter, until version 1.0.6j; from then on they were given the upcoming
 *    public release number plus "betaNN" or "rcNN".
 *
 *    Binary incompatibility exists only when applications make direct access
 *    to the info_ptr or ci_ptr members through ci.h, and the compiled
 *    application is loaded with a different version of the library.
 *
 * See libci.txt or libci.3 for more information.  The CI specification
 * is available as a W3C Recommendation and as an ISO/IEC Standard; see
 * <https://www.w3.org/TR/2003/REC-CI-20031110/>
 */

#ifndef CI_H
#define CI_H

/* This is not the place to learn how to use libci. The file libci-manual.txt
 * describes how to use libci, and the file example.c summarizes it
 * with some code on which to build.  This file is useful for looking
 * at the actual function definitions and structure components.  If that
 * file has been stripped from your copy of libci, you can find it at
 * <http://www.libci.org/pub/ci/libci-manual.txt>
 *
 * If you just need to read a CI file and don't want to read the documentation
 * skip to the end of this file and read the section entitled 'simplified API'.
 */

/* Version information for ci.h - this should match the version in ci.c */
#define CI_LIBCI_VER_STRING "1.6.50.git"
#define CI_HEADER_VERSION_STRING " libci version " CI_LIBCI_VER_STRING "\n"

/* The versions of shared library builds should stay in sync, going forward */
#define CI_LIBCI_VER_SHAREDLIB 16
#define CI_LIBCI_VER_SONUM     CI_LIBCI_VER_SHAREDLIB /* [Deprecated] */
#define CI_LIBCI_VER_DLLNUM    CI_LIBCI_VER_SHAREDLIB /* [Deprecated] */

/* These should match the first 3 components of CI_LIBCI_VER_STRING: */
#define CI_LIBCI_VER_MAJOR   1
#define CI_LIBCI_VER_MINOR   6
#define CI_LIBCI_VER_RELEASE 50

/* This should be zero for a public release, or non-zero for a
 * development version.
 */
#define CI_LIBCI_VER_BUILD 1

/* Release Status */
#define CI_LIBCI_BUILD_ALPHA               1
#define CI_LIBCI_BUILD_BETA                2
#define CI_LIBCI_BUILD_RC                  3
#define CI_LIBCI_BUILD_STABLE              4
#define CI_LIBCI_BUILD_RELEASE_STATUS_MASK 7

/* Release-Specific Flags */
#define CI_LIBCI_BUILD_PATCH    8 /* Can be OR'ed with
                                       CI_LIBCI_BUILD_STABLE only */
#define CI_LIBCI_BUILD_PRIVATE 16 /* Cannot be OR'ed with
                                       CI_LIBCI_BUILD_SPECIAL */
#define CI_LIBCI_BUILD_SPECIAL 32 /* Cannot be OR'ed with
                                       CI_LIBCI_BUILD_PRIVATE */

#define CI_LIBCI_BUILD_BASE_TYPE CI_LIBCI_BUILD_BETA

/* Careful here.  At one time, Guy wanted to use 082, but that
 * would be octal.  We must not include leading zeros.
 * Versions 0.7 through 1.0.0 were in the range 0 to 100 here
 * (only version 1.0.0 was mis-numbered 100 instead of 10000).
 * From version 1.0.1 it is:
 * XXYYZZ, where XX=major, YY=minor, ZZ=release
 */
#define CI_LIBCI_VER 10650 /* 1.6.50.git */

/* Library configuration: these options cannot be changed after
 * the library has been built.
 */
#ifndef CILCONF_H
/* If cilibconf.h is missing, you can
 * copy scripts/cilibconf.h.prebuilt to cilibconf.h
 */
#   include "cilibconf.h"
#endif

#ifndef CI_VERSION_INFO_ONLY
/* Machine specific configuration. */
#  include "ciconf.h"
#endif

/*
 * Added at libci-1.2.8
 *
 * Ref MSDN: Private as priority over Special
 * VS_FF_PRIVATEBUILD File *was not* built using standard release
 * procedures. If this value is given, the StringFileInfo block must
 * contain a PrivateBuild string.
 *
 * VS_FF_SPECIALBUILD File *was* built by the original company using
 * standard release procedures but is a variation of the standard
 * file of the same version number. If this value is given, the
 * StringFileInfo block must contain a SpecialBuild string.
 */

#ifdef CI_USER_PRIVATEBUILD /* From cilibconf.h */
#  define CI_LIBCI_BUILD_TYPE \
       (CI_LIBCI_BUILD_BASE_TYPE | CI_LIBCI_BUILD_PRIVATE)
#else
#  ifdef CI_LIBCI_SPECIALBUILD
#    define CI_LIBCI_BUILD_TYPE \
         (CI_LIBCI_BUILD_BASE_TYPE | CI_LIBCI_BUILD_SPECIAL)
#  else
#    define CI_LIBCI_BUILD_TYPE (CI_LIBCI_BUILD_BASE_TYPE)
#  endif
#endif

#ifndef CI_VERSION_INFO_ONLY

/* Inhibit C++ name-mangling for libci functions but not for system calls. */
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* Version information for C files, stored in ci.c.  This had better match
 * the version above.
 */
#define ci_libci_ver ci_get_header_ver(NULL)

/* This file is arranged in several sections:
 *
 * 1. [omitted]
 * 2. Any configuration options that can be specified by for the application
 *    code when it is built.  (Build time configuration is in cilibconf.h)
 * 3. Type definitions (base types are defined in ciconf.h), structure
 *    definitions.
 * 4. Exported library functions.
 * 5. Simplified API.
 * 6. Implementation options.
 *
 * The library source code has additional files (principally cipriv.h) that
 * allow configuration of the library.
 */

/* Section 1: [omitted] */

/* Section 2: run time configuration
 * See cilibconf.h for build time configuration
 *
 * Run time configuration allows the application to choose between
 * implementations of certain arithmetic APIs.  The default is set
 * at build time and recorded in cilibconf.h, but it is safe to
 * override these (and only these) settings.  Note that this won't
 * change what the library does, only application code, and the
 * settings can (and probably should) be made on a per-file basis
 * by setting the #defines before including ci.h
 *
 * Use macros to read integers from CI data or use the exported
 * functions?
 *   CI_USE_READ_MACROS: use the macros (see below)  Note that
 *     the macros evaluate their argument multiple times.
 *   CI_NO_USE_READ_MACROS: call the relevant library function.
 *
 * Use the alternative algorithm for compositing alpha samples that
 * does not use division?
 *   CI_READ_COMPOSITE_NODIV_SUPPORTED: use the 'no division'
 *      algorithm.
 *   CI_NO_READ_COMPOSITE_NODIV: use the 'division' algorithm.
 *
 * How to handle benign errors if CI_ALLOW_BENIGN_ERRORS is
 * false?
 *   CI_ALLOW_BENIGN_ERRORS: map calls to the benign error
 *      APIs to ci_warning.
 * Otherwise the calls are mapped to ci_error.
 */

/* Section 3: type definitions, including structures and compile time
 * constants.
 * See ciconf.h for base types that vary by machine/system
 */

/* This triggers a compiler error in ci.c, if ci.c and ci.h
 * do not agree upon the version number.
 */
typedef char* ci_libci_version_1_6_50_git;

/* Basic control structions.  Read libci-manual.txt or libci.3 for more info.
 *
 * ci_struct is the cache of information used while reading or writing a single
 * CI file.  One of these is always required, although the simplified API
 * (below) hides the creation and destruction of it.
 */
typedef struct ci_struct_def ci_struct;
typedef const ci_struct * ci_const_structp;
typedef ci_struct * ci_structp;
typedef ci_struct * * ci_structpp;

/* ci_info contains information read from or to be written to a CI file.  One
 * or more of these must exist while reading or creating a CI file.  The
 * information is not used by libci during read but is used to control what
 * gets written when a CI file is created.  "ci_get_" function calls read
 * information during read and "ci_set_" functions calls write information
 * when creating a CI.
 * been moved into a separate header file that is not accessible to
 * applications.  Read libci-manual.txt or libci.3 for more info.
 */
typedef struct ci_info_def ci_info;
typedef ci_info * ci_infop;
typedef const ci_info * ci_const_infop;
typedef ci_info * * ci_infopp;

/* Types with names ending 'p' are pointer types.  The corresponding types with
 * names ending 'rp' are identical pointer types except that the pointer is
 * marked 'restrict', which means that it is the only pointer to the object
 * passed to the function.  Applications should not use the 'restrict' types;
 * it is always valid to pass 'p' to a pointer with a function argument of the
 * corresponding 'rp' type.  Different compilers have different rules with
 * regard to type matching in the presence of 'restrict'.  For backward
 * compatibility libci callbacks never have 'restrict' in their parameters and,
 * consequentially, writing portable application code is extremely difficult if
 * an attempt is made to use 'restrict'.
 */
typedef ci_struct * CI_RESTRICT ci_structrp;
typedef const ci_struct * CI_RESTRICT ci_const_structrp;
typedef ci_info * CI_RESTRICT ci_inforp;
typedef const ci_info * CI_RESTRICT ci_const_inforp;

/* Three color definitions.  The order of the red, green, and blue, (and the
 * exact size) is not important, although the size of the fields need to
 * be ci_byte or ci_uint_16 (as defined below).
 */
typedef struct ci_color_struct
{
   ci_byte red;
   ci_byte green;
   ci_byte blue;
} ci_color;
typedef ci_color * ci_colorp;
typedef const ci_color * ci_const_colorp;
typedef ci_color * * ci_colorpp;

typedef struct ci_color_16_struct
{
   ci_byte index;    /* used for palette files */
   ci_uint_16 red;   /* for use in red green blue files */
   ci_uint_16 green;
   ci_uint_16 blue;
   ci_uint_16 gray;  /* for use in grayscale files */
} ci_color_16;
typedef ci_color_16 * ci_color_16p;
typedef const ci_color_16 * ci_const_color_16p;
typedef ci_color_16 * * ci_color_16pp;

typedef struct ci_color_8_struct
{
   ci_byte red;   /* for use in red green blue files */
   ci_byte green;
   ci_byte blue;
   ci_byte gray;  /* for use in grayscale files */
   ci_byte alpha; /* for alpha channel files */
} ci_color_8;
typedef ci_color_8 * ci_color_8p;
typedef const ci_color_8 * ci_const_color_8p;
typedef ci_color_8 * * ci_color_8pp;

/*
 * The following two structures are used for the in-core representation
 * of sPLT chunks.
 */
typedef struct ci_sPLT_entry_struct
{
   ci_uint_16 red;
   ci_uint_16 green;
   ci_uint_16 blue;
   ci_uint_16 alpha;
   ci_uint_16 frequency;
} ci_sPLT_entry;
typedef ci_sPLT_entry * ci_sPLT_entryp;
typedef const ci_sPLT_entry * ci_const_sPLT_entryp;
typedef ci_sPLT_entry * * ci_sPLT_entrypp;

/*  When the depth of the sPLT palette is 8 bits, the color and alpha samples
 *  occupy the LSB of their respective members, and the MSB of each member
 *  is zero-filled.  The frequency member always occupies the full 16 bits.
 */

typedef struct ci_sPLT_struct
{
   ci_charp name;           /* palette name */
   ci_byte depth;           /* depth of palette samples */
   ci_sPLT_entryp entries;  /* palette entries */
   ci_int_32 nentries;      /* number of palette entries */
} ci_sPLT_t;
typedef ci_sPLT_t * ci_sPLT_tp;
typedef const ci_sPLT_t * ci_const_sPLT_tp;
typedef ci_sPLT_t * * ci_sPLT_tpp;

#ifdef CI_TEXT_SUPPORTED
/* ci_text holds the contents of a text/ztxt/itxt chunk in a CI file,
 * and whether that contents is compressed or not.  The "key" field
 * points to a regular zero-terminated C string.  The "text" fields can be a
 * regular C string, an empty string, or a NULL pointer.
 * However, the structure returned by ci_get_text() will always contain
 * the "text" field as a regular zero-terminated C string (possibly
 * empty), never a NULL pointer, so it can be safely used in printf() and
 * other string-handling functions.  Note that the "itxt_length", "lang", and
 * "lang_key" members of the structure only exist when the library is built
 * with iTXt chunk support.  Prior to libci-1.4.0 the library was built by
 * default without iTXt support. Also note that when iTXt *is* supported,
 * the "lang" and "lang_key" fields contain NULL pointers when the
 * "compression" field contains * CI_TEXT_COMPRESSION_NONE or
 * CI_TEXT_COMPRESSION_zTXt. Note that the "compression value" is not the
 * same as what appears in the CI tEXt/zTXt/iTXt chunk's "compression flag"
 * which is always 0 or 1, or its "compression method" which is always 0.
 */
typedef struct ci_text_struct
{
   int  compression;       /* compression value:
                             -1: tEXt, none
                              0: zTXt, deflate
                              1: iTXt, none
                              2: iTXt, deflate  */
   ci_charp key;          /* keyword, 1-79 character description of "text" */
   ci_charp text;         /* comment, may be an empty string (ie "")
                              or a NULL pointer */
   size_t text_length;     /* length of the text string */
   size_t itxt_length;     /* length of the itxt string */
   ci_charp lang;         /* language code, 0-79 characters
                              or a NULL pointer */
   ci_charp lang_key;     /* keyword translated UTF-8 string, 0 or more
                              chars or a NULL pointer */
} ci_text;
typedef ci_text * ci_textp;
typedef const ci_text * ci_const_textp;
typedef ci_text * * ci_textpp;
#endif

/* Supported compression types for text in CI files (tEXt, and zTXt).
 * The values of the CI_TEXT_COMPRESSION_ defines should NOT be changed. */
#define CI_TEXT_COMPRESSION_NONE_WR -3
#define CI_TEXT_COMPRESSION_zTXt_WR -2
#define CI_TEXT_COMPRESSION_NONE    -1
#define CI_TEXT_COMPRESSION_zTXt     0
#define CI_ITXT_COMPRESSION_NONE     1
#define CI_ITXT_COMPRESSION_zTXt     2
#define CI_TEXT_COMPRESSION_LAST     3  /* Not a valid value */

/* ci_time is a way to hold the time in an machine independent way.
 * Two conversions are provided, both from time_t and struct tm.  There
 * is no portable way to convert to either of these structures, as far
 * as I know.  If you know of a portable way, send it to me.  As a side
 * note - CI has always been Year 2000 compliant!
 */
typedef struct ci_time_struct
{
   ci_uint_16 year; /* full year, as in, 1995 */
   ci_byte month;   /* month of year, 1 - 12 */
   ci_byte day;     /* day of month, 1 - 31 */
   ci_byte hour;    /* hour of day, 0 - 23 */
   ci_byte minute;  /* minute of hour, 0 - 59 */
   ci_byte second;  /* second of minute, 0 - 60 (for leap seconds) */
} ci_time;
typedef ci_time * ci_timep;
typedef const ci_time * ci_const_timep;
typedef ci_time * * ci_timepp;

#if defined(CI_STORE_UNKNOWN_CHUNKS_SUPPORTED) ||\
   defined(CI_USER_CHUNKS_SUPPORTED)
/* ci_unknown_chunk is a structure to hold queued chunks for which there is
 * no specific support.  The idea is that we can use this to queue
 * up private chunks for output even though the library doesn't actually
 * know about their semantics.
 *
 * The data in the structure is set by libci on read and used on write.
 */
typedef struct ci_unknown_chunk_t
{
   ci_byte name[5]; /* Textual chunk name with '\0' terminator */
   ci_byte *data;   /* Data, should not be modified on read! */
   size_t size;

   /* On write 'location' must be set using the flag values listed below.
    * Notice that on read it is set by libci however the values stored have
    * more bits set than are listed below.  Always treat the value as a
    * bitmask.  On write set only one bit - setting multiple bits may cause the
    * chunk to be written in multiple places.
    */
   ci_byte location; /* mode of operation at read time */
}
ci_unknown_chunk;

typedef ci_unknown_chunk * ci_unknown_chunkp;
typedef const ci_unknown_chunk * ci_const_unknown_chunkp;
typedef ci_unknown_chunk * * ci_unknown_chunkpp;
#endif

/* Flag values for the unknown chunk location byte. */
#define CI_HAVE_IHDR  0x01
#define CI_HAVE_PLTE  0x02
#define CI_AFTER_IDAT 0x08

/* Maximum positive integer used in CI is (2^31)-1 */
#define CI_UINT_31_MAX ((ci_uint_32)0x7fffffffL)
#define CI_UINT_32_MAX ((ci_uint_32)(-1))
#define CI_SIZE_MAX ((size_t)(-1))

/* These are constants for fixed point values encoded in the
 * CI specification manner (x100000)
 */
#define CI_FP_1    100000
#define CI_FP_HALF  50000
#define CI_FP_MAX  ((ci_fixed_point)0x7fffffffL)
#define CI_FP_MIN  (-CI_FP_MAX)

/* These describe the color_type field in ci_info. */
/* color type masks */
#define CI_COLOR_MASK_PALETTE    1
#define CI_COLOR_MASK_COLOR      2
#define CI_COLOR_MASK_ALPHA      4

/* color types.  Note that not all combinations are legal */
#define CI_COLOR_TYPE_GRAY 0
#define CI_COLOR_TYPE_PALETTE  (CI_COLOR_MASK_COLOR | CI_COLOR_MASK_PALETTE)
#define CI_COLOR_TYPE_RGB        (CI_COLOR_MASK_COLOR)
#define CI_COLOR_TYPE_RGB_ALPHA  (CI_COLOR_MASK_COLOR | CI_COLOR_MASK_ALPHA)
#define CI_COLOR_TYPE_GRAY_ALPHA (CI_COLOR_MASK_ALPHA)
/* aliases */
#define CI_COLOR_TYPE_RGBA  CI_COLOR_TYPE_RGB_ALPHA
#define CI_COLOR_TYPE_GA  CI_COLOR_TYPE_GRAY_ALPHA

/* This is for compression type. CI 1.0-1.2 only define the single type. */
#define CI_COMPRESSION_TYPE_BASE 0 /* Deflate method 8, 32K window */
#define CI_COMPRESSION_TYPE_DEFAULT CI_COMPRESSION_TYPE_BASE

/* This is for filter type. CI 1.0-1.2 only define the single type. */
#define CI_FILTER_TYPE_BASE      0 /* Single row per-byte filtering */
#define CI_INTRAPIXEL_DIFFERENCING 64 /* Used only in MNG datastreams */
#define CI_FILTER_TYPE_DEFAULT   CI_FILTER_TYPE_BASE

/* These are for the interlacing type.  These values should NOT be changed. */
#define CI_INTERLACE_NONE        0 /* Non-interlaced image */
#define CI_INTERLACE_ADAM7       1 /* Adam7 interlacing */
#define CI_INTERLACE_LAST        2 /* Not a valid value */

/* These are for the oFFs chunk.  These values should NOT be changed. */
#define CI_OFFSET_PIXEL          0 /* Offset in pixels */
#define CI_OFFSET_MICROMETER     1 /* Offset in micrometers (1/10^6 meter) */
#define CI_OFFSET_LAST           2 /* Not a valid value */

/* These are for the pCAL chunk.  These values should NOT be changed. */
#define CI_EQUATION_LINEAR       0 /* Linear transformation */
#define CI_EQUATION_BASE_E       1 /* Exponential base e transform */
#define CI_EQUATION_ARBITRARY    2 /* Arbitrary base exponential transform */
#define CI_EQUATION_HYPERBOLIC   3 /* Hyperbolic sine transformation */
#define CI_EQUATION_LAST         4 /* Not a valid value */

/* These are for the sCAL chunk.  These values should NOT be changed. */
#define CI_SCALE_UNKNOWN         0 /* unknown unit (image scale) */
#define CI_SCALE_METER           1 /* meters per pixel */
#define CI_SCALE_RADIAN          2 /* radians per pixel */
#define CI_SCALE_LAST            3 /* Not a valid value */

/* These are for the pHYs chunk.  These values should NOT be changed. */
#define CI_RESOLUTION_UNKNOWN    0 /* pixels/unknown unit (aspect ratio) */
#define CI_RESOLUTION_METER      1 /* pixels/meter */
#define CI_RESOLUTION_LAST       2 /* Not a valid value */

/* These are for the sRGB chunk.  These values should NOT be changed. */
#define CI_sRGB_INTENT_PERCEPTUAL 0
#define CI_sRGB_INTENT_RELATIVE   1
#define CI_sRGB_INTENT_SATURATION 2
#define CI_sRGB_INTENT_ABSOLUTE   3
#define CI_sRGB_INTENT_LAST       4 /* Not a valid value */

/* This is for text chunks */
#define CI_KEYWORD_MAX_LENGTH     79

/* Maximum number of entries in PLTE/sPLT/tRNS arrays */
#define CI_MAX_PALETTE_LENGTH    256

/* These determine if an ancillary chunk's data has been successfully read
 * from the CI header, or if the application has filled in the corresponding
 * data in the info_struct to be written into the output file.  The values
 * of the CI_INFO_<chunk> defines should NOT be changed.
 */
#define CI_INFO_gAMA 0x0001U
#define CI_INFO_sBIT 0x0002U
#define CI_INFO_cHRM 0x0004U
#define CI_INFO_PLTE 0x0008U
#define CI_INFO_tRNS 0x0010U
#define CI_INFO_bKGD 0x0020U
#define CI_INFO_hIST 0x0040U
#define CI_INFO_pHYs 0x0080U
#define CI_INFO_oFFs 0x0100U
#define CI_INFO_tIME 0x0200U
#define CI_INFO_pCAL 0x0400U
#define CI_INFO_sRGB 0x0800U  /* GR-P, 0.96a */
#define CI_INFO_iCCP 0x1000U  /* ESR, 1.0.6 */
#define CI_INFO_sPLT 0x2000U  /* ESR, 1.0.6 */
#define CI_INFO_sCAL 0x4000U  /* ESR, 1.0.6 */
#define CI_INFO_IDAT 0x8000U  /* ESR, 1.0.6 */
#define CI_INFO_eXIf 0x10000U /* GR-P, 1.6.31 */
#define CI_INFO_cICP 0x20000U /* CIv3: 1.6.45 */
#define CI_INFO_cLLI 0x40000U /* CIv3: 1.6.45 */
#define CI_INFO_mDCV 0x80000U /* CIv3: 1.6.45 */
/* ACI: these chunks are stored as unknown, these flags are never set
 * however they are provided as a convenience for implementors of ACI and
 * avoids any merge conflicts.
 *
 * Private chunks: these chunk names violate the chunk name recommendations
 * because the chunk definitions have no signature and because the private
 * chunks with these names have been reserved.  Private definitions should
 * avoid them.
 */
#define CI_INFO_acTL 0x100000U /* CIv3: 1.6.45: unknown */
#define CI_INFO_fcTL 0x200000U /* CIv3: 1.6.45: unknown */
#define CI_INFO_fdAT 0x400000U /* CIv3: 1.6.45: unknown */

/* This is used for the transformation routines, as some of them
 * change these values for the row.  It also should enable using
 * the routines for other purposes.
 */
typedef struct ci_row_info_struct
{
   ci_uint_32 width;    /* width of row */
   size_t rowbytes;      /* number of bytes in row */
   ci_byte color_type;  /* color type of row */
   ci_byte bit_depth;   /* bit depth of row */
   ci_byte channels;    /* number of channels (1, 2, 3, or 4) */
   ci_byte pixel_depth; /* bits per pixel (depth * channels) */
} ci_row_info;

typedef ci_row_info * ci_row_infop;
typedef ci_row_info * * ci_row_infopp;

/* These are the function types for the I/O functions and for the functions
 * that allow the user to override the default I/O functions with his or her
 * own.  The ci_error_ptr type should match that of user-supplied warning
 * and error functions, while the ci_rw_ptr type should match that of the
 * user read/write data functions.  Note that the 'write' function must not
 * modify the buffer it is passed. The 'read' function, on the other hand, is
 * expected to return the read data in the buffer.
 */
typedef CI_CALLBACK(void, *ci_error_ptr, (ci_structp, ci_const_charp));
typedef CI_CALLBACK(void, *ci_rw_ptr, (ci_structp, ci_bytep, size_t));
typedef CI_CALLBACK(void, *ci_flush_ptr, (ci_structp));
typedef CI_CALLBACK(void, *ci_read_status_ptr, (ci_structp, ci_uint_32,
    int));
typedef CI_CALLBACK(void, *ci_write_status_ptr, (ci_structp, ci_uint_32,
    int));

#ifdef CI_PROGRESSIVE_READ_SUPPORTED
typedef CI_CALLBACK(void, *ci_progressive_info_ptr, (ci_structp, ci_infop));
typedef CI_CALLBACK(void, *ci_progressive_end_ptr, (ci_structp, ci_infop));

/* The following callback receives ci_uint_32 row_number, int pass for the
 * ci_bytep data of the row.  When transforming an interlaced image the
 * row number is the row number within the sub-image of the interlace pass, so
 * the value will increase to the height of the sub-image (not the full image)
 * then reset to 0 for the next pass.
 *
 * Use CI_ROW_FROM_PASS_ROW(row, pass) and CI_COL_FROM_PASS_COL(col, pass) to
 * find the output pixel (x,y) given an interlaced sub-image pixel
 * (row,col,pass).  (See below for these macros.)
 */
typedef CI_CALLBACK(void, *ci_progressive_row_ptr, (ci_structp, ci_bytep,
    ci_uint_32, int));
#endif

#if defined(CI_READ_USER_TRANSFORM_SUPPORTED) || \
    defined(CI_WRITE_USER_TRANSFORM_SUPPORTED)
typedef CI_CALLBACK(void, *ci_user_transform_ptr, (ci_structp, ci_row_infop,
    ci_bytep));
#endif

#ifdef CI_USER_CHUNKS_SUPPORTED
typedef CI_CALLBACK(int, *ci_user_chunk_ptr, (ci_structp,
    ci_unknown_chunkp));
#endif
#ifdef CI_UNKNOWN_CHUNKS_SUPPORTED
/* not used anywhere */
/* typedef CI_CALLBACK(void, *ci_unknown_chunk_ptr, (ci_structp)); */
#endif

#ifdef CI_SETJMP_SUPPORTED
/* This must match the function definition in <setjmp.h>, and the application
 * must include this before ci.h to obtain the definition of jmp_buf.  The
 * function is required to be CI_NORETURN, but this is not checked.  If the
 * function does return the application will crash via an abort() or similar
 * system level call.
 *
 * If you get a warning here while building the library you may need to make
 * changes to ensure that cilibconf.h records the calling convention used by
 * your compiler.  This may be very difficult - try using a different compiler
 * to build the library!
 */
CI_FUNCTION(void, (CICAPI *ci_longjmp_ptr), (jmp_buf, int), typedef);
#endif

/* Transform masks for the high-level interface */
#define CI_TRANSFORM_IDENTITY       0x0000    /* read and write */
#define CI_TRANSFORM_STRIP_16       0x0001    /* read only */
#define CI_TRANSFORM_STRIP_ALPHA    0x0002    /* read only */
#define CI_TRANSFORM_PACKING        0x0004    /* read and write */
#define CI_TRANSFORM_PACKSWAP       0x0008    /* read and write */
#define CI_TRANSFORM_EXPAND         0x0010    /* read only */
#define CI_TRANSFORM_INVERT_MONO    0x0020    /* read and write */
#define CI_TRANSFORM_SHIFT          0x0040    /* read and write */
#define CI_TRANSFORM_BGR            0x0080    /* read and write */
#define CI_TRANSFORM_SWAP_ALPHA     0x0100    /* read and write */
#define CI_TRANSFORM_SWAP_ENDIAN    0x0200    /* read and write */
#define CI_TRANSFORM_INVERT_ALPHA   0x0400    /* read and write */
#define CI_TRANSFORM_STRIP_FILLER   0x0800    /* write only */
/* Added to libci-1.2.34 */
#define CI_TRANSFORM_STRIP_FILLER_BEFORE CI_TRANSFORM_STRIP_FILLER
#define CI_TRANSFORM_STRIP_FILLER_AFTER 0x1000 /* write only */
/* Added to libci-1.4.0 */
#define CI_TRANSFORM_GRAY_TO_RGB   0x2000      /* read only */
/* Added to libci-1.5.4 */
#define CI_TRANSFORM_EXPAND_16     0x4000      /* read only */
#if ~0U > 0xffffU /* or else this might break on a 16-bit machine */
#define CI_TRANSFORM_SCALE_16      0x8000      /* read only */
#endif

/* Flags for MNG supported features */
#define CI_FLAG_MNG_EMPTY_PLTE     0x01
#define CI_FLAG_MNG_FILTER_64      0x04
#define CI_ALL_MNG_FEATURES        0x05

/* NOTE: prior to 1.5 these functions had no 'API' style declaration,
 * this allowed the zlib default functions to be used on Windows
 * platforms.  In 1.5 the zlib default malloc (which just calls malloc and
 * ignores the first argument) should be completely compatible with the
 * following.
 */
typedef CI_CALLBACK(ci_voidp, *ci_malloc_ptr, (ci_structp,
    ci_alloc_size_t));
typedef CI_CALLBACK(void, *ci_free_ptr, (ci_structp, ci_voidp));

/* Section 4: exported functions
 * Here are the function definitions most commonly used.  This is not
 * the place to find out how to use libci.  See libci-manual.txt for the
 * full explanation, see example.c for the summary.  This just provides
 * a simple one line description of the use of each function.
 *
 * The CI_EXPORT() and CI_EXPORTA() macros used below are defined in
 * ciconf.h and in the *.dfn files in the scripts directory.
 *
 *   CI_EXPORT(ordinal, type, name, (args));
 *
 *       ordinal:    ordinal that is used while building
 *                   *.def files. The ordinal value is only
 *                   relevant when preprocessing ci.h with
 *                   the *.dfn files for building symbol table
 *                   entries, and are removed by ciconf.h.
 *       type:       return type of the function
 *       name:       function name
 *       args:       function arguments, with types
 *
 * When we wish to append attributes to a function prototype we use
 * the CI_EXPORTA() macro instead.
 *
 *   CI_EXPORTA(ordinal, type, name, (args), attributes);
 *
 *       ordinal, type, name, and args: same as in CI_EXPORT().
 *       attributes: function attributes
 */

/* Returns the version number of the library */
CI_EXPORT(1, ci_uint_32, ci_access_version_number, (void));

/* Tell lib we have already handled the first <num_bytes> magic bytes.
 * Handling more than 8 bytes from the beginning of the file is an error.
 */
CI_EXPORT(2, void, ci_set_sig_bytes, (ci_structrp ci_ptr, int num_bytes));

/* Check sig[start] through sig[start + num_to_check - 1] to see if it's a
 * CI file.  Returns zero if the supplied bytes match the 8-byte CI
 * signature, and non-zero otherwise.  Having num_to_check == 0 or
 * start > 7 will always fail (i.e. return non-zero).
 */
CI_EXPORT(3, int, ci_sig_cmp, (ci_const_bytep sig, size_t start,
    size_t num_to_check));

/* Simple signature checking function.  This is the same as calling
 * ci_check_sig(sig, n) := (ci_sig_cmp(sig, 0, n) == 0).
 */
#define ci_check_sig(sig, n) (ci_sig_cmp((sig), 0, (n)) == 0) /* DEPRECATED */

/* Allocate and initialize ci_ptr struct for reading, and any other memory. */
CI_EXPORTA(4, ci_structp, ci_create_read_struct,
    (ci_const_charp user_ci_ver, ci_voidp error_ptr,
    ci_error_ptr error_fn, ci_error_ptr warn_fn),
    CI_ALLOCATED);

/* Allocate and initialize ci_ptr struct for writing, and any other memory */
CI_EXPORTA(5, ci_structp, ci_create_write_struct,
    (ci_const_charp user_ci_ver, ci_voidp error_ptr, ci_error_ptr error_fn,
    ci_error_ptr warn_fn),
    CI_ALLOCATED);

CI_EXPORT(6, size_t, ci_get_compression_buffer_size,
    (ci_const_structrp ci_ptr));

CI_EXPORT(7, void, ci_set_compression_buffer_size, (ci_structrp ci_ptr,
    size_t size));

/* Moved from ciconf.h in 1.4.0 and modified to ensure setjmp/longjmp
 * match up.
 */
#ifdef CI_SETJMP_SUPPORTED
/* This function returns the jmp_buf built in to *ci_ptr.  It must be
 * supplied with an appropriate 'longjmp' function to use on that jmp_buf
 * unless the default error function is overridden in which case NULL is
 * acceptable.  The size of the jmp_buf is checked against the actual size
 * allocated by the library - the call will return NULL on a mismatch
 * indicating an ABI mismatch.
 */
CI_EXPORT(8, jmp_buf*, ci_set_longjmp_fn, (ci_structrp ci_ptr,
    ci_longjmp_ptr longjmp_fn, size_t jmp_buf_size));
#  define ci_jmpbuf(ci_ptr) \
      (*ci_set_longjmp_fn((ci_ptr), longjmp, (sizeof (jmp_buf))))
#else
#  define ci_jmpbuf(ci_ptr) \
      (LIBCI_WAS_COMPILED_WITH__CI_NO_SETJMP)
#endif
/* This function should be used by libci applications in place of
 * longjmp(ci_ptr->jmpbuf, val).  If longjmp_fn() has been set, it
 * will use it; otherwise it will call CI_ABORT().  This function was
 * added in libci-1.5.0.
 */
CI_EXPORTA(9, void, ci_longjmp, (ci_const_structrp ci_ptr, int val),
    CI_NORETURN);

#ifdef CI_READ_SUPPORTED
/* Reset the compression stream */
CI_EXPORTA(10, int, ci_reset_zstream, (ci_structrp ci_ptr), CI_DEPRECATED);
#endif

/* New functions added in libci-1.0.2 (not enabled by default until 1.2.0) */
#ifdef CI_USER_MEM_SUPPORTED
CI_EXPORTA(11, ci_structp, ci_create_read_struct_2,
    (ci_const_charp user_ci_ver, ci_voidp error_ptr, ci_error_ptr error_fn,
    ci_error_ptr warn_fn,
    ci_voidp mem_ptr, ci_malloc_ptr malloc_fn, ci_free_ptr free_fn),
    CI_ALLOCATED);
CI_EXPORTA(12, ci_structp, ci_create_write_struct_2,
    (ci_const_charp user_ci_ver, ci_voidp error_ptr, ci_error_ptr error_fn,
    ci_error_ptr warn_fn,
    ci_voidp mem_ptr, ci_malloc_ptr malloc_fn, ci_free_ptr free_fn),
    CI_ALLOCATED);
#endif

/* Write the CI file signature. */
CI_EXPORT(13, void, ci_write_sig, (ci_structrp ci_ptr));

/* Write a CI chunk - size, type, (optional) data, CRC. */
CI_EXPORT(14, void, ci_write_chunk, (ci_structrp ci_ptr, ci_const_bytep
    chunk_name, ci_const_bytep data, size_t length));

/* Write the start of a CI chunk - length and chunk name. */
CI_EXPORT(15, void, ci_write_chunk_start, (ci_structrp ci_ptr,
    ci_const_bytep chunk_name, ci_uint_32 length));

/* Write the data of a CI chunk started with ci_write_chunk_start(). */
CI_EXPORT(16, void, ci_write_chunk_data, (ci_structrp ci_ptr,
    ci_const_bytep data, size_t length));

/* Finish a chunk started with ci_write_chunk_start() (includes CRC). */
CI_EXPORT(17, void, ci_write_chunk_end, (ci_structrp ci_ptr));

/* Allocate and initialize the info structure */
CI_EXPORTA(18, ci_infop, ci_create_info_struct, (ci_const_structrp ci_ptr),
    CI_ALLOCATED);

/* DEPRECATED: this function allowed init structures to be created using the
 * default allocation method (typically malloc).  Use is deprecated in 1.6.0 and
 * the API will be removed in the future.
 */
CI_EXPORTA(19, void, ci_info_init_3, (ci_infopp info_ptr,
    size_t ci_info_struct_size), CI_DEPRECATED);

/* Writes all the CI information before the image. */
CI_EXPORT(20, void, ci_write_info_before_PLTE,
    (ci_structrp ci_ptr, ci_const_inforp info_ptr));
CI_EXPORT(21, void, ci_write_info,
    (ci_structrp ci_ptr, ci_const_inforp info_ptr));

#ifdef CI_SEQUENTIAL_READ_SUPPORTED
/* Read the information before the actual image data. */
CI_EXPORT(22, void, ci_read_info,
    (ci_structrp ci_ptr, ci_inforp info_ptr));
#endif

#ifdef CI_TIME_RFC1123_SUPPORTED
   /* Convert to a US string format: there is no localization support in this
    * routine.  The original implementation used a 29 character buffer in
    * ci_struct, this will be removed in future versions.
    */
#if CI_LIBCI_VER < 10700
/* To do: remove this from libci17 (and from libci17/ci.c and cistruct.h) */
CI_EXPORTA(23, ci_const_charp, ci_convert_to_rfc1123, (ci_structrp ci_ptr,
    ci_const_timep ptime),CI_DEPRECATED);
#endif
CI_EXPORT(241, int, ci_convert_to_rfc1123_buffer, (char out[29],
    ci_const_timep ptime));
#endif

#ifdef CI_CONVERT_tIME_SUPPORTED
/* Convert from a struct tm to ci_time */
CI_EXPORT(24, void, ci_convert_from_struct_tm, (ci_timep ptime,
    const struct tm * ttime));

/* Convert from time_t to ci_time.  Uses gmtime() */
CI_EXPORT(25, void, ci_convert_from_time_t, (ci_timep ptime, time_t ttime));
#endif /* CONVERT_tIME */

#ifdef CI_READ_EXPAND_SUPPORTED
/* Expand data to 24-bit RGB, or 8-bit grayscale, with alpha if available. */
CI_EXPORT(26, void, ci_set_expand, (ci_structrp ci_ptr));
CI_EXPORT(27, void, ci_set_expand_gray_1_2_4_to_8, (ci_structrp ci_ptr));
CI_EXPORT(28, void, ci_set_palette_to_rgb, (ci_structrp ci_ptr));
CI_EXPORT(29, void, ci_set_tRNS_to_alpha, (ci_structrp ci_ptr));
#endif

#ifdef CI_READ_EXPAND_16_SUPPORTED
/* Expand to 16-bit channels, forces conversion of palette to RGB and expansion
 * of a tRNS chunk if present.
 */
CI_EXPORT(221, void, ci_set_expand_16, (ci_structrp ci_ptr));
#endif

#if defined(CI_READ_BGR_SUPPORTED) || defined(CI_WRITE_BGR_SUPPORTED)
/* Use blue, green, red order for pixels. */
CI_EXPORT(30, void, ci_set_bgr, (ci_structrp ci_ptr));
#endif

#ifdef CI_READ_GRAY_TO_RGB_SUPPORTED
/* Expand the grayscale to 24-bit RGB if necessary. */
CI_EXPORT(31, void, ci_set_gray_to_rgb, (ci_structrp ci_ptr));
#endif

#ifdef CI_READ_RGB_TO_GRAY_SUPPORTED
/* Reduce RGB to grayscale. */
#define CI_ERROR_ACTION_NONE  1
#define CI_ERROR_ACTION_WARN  2
#define CI_ERROR_ACTION_ERROR 3
#define CI_RGB_TO_GRAY_DEFAULT (-1)/*for red/green coefficients*/

CI_FP_EXPORT(32, void, ci_set_rgb_to_gray, (ci_structrp ci_ptr,
    int error_action, double red, double green))
CI_FIXED_EXPORT(33, void, ci_set_rgb_to_gray_fixed, (ci_structrp ci_ptr,
    int error_action, ci_fixed_point red, ci_fixed_point green))

CI_EXPORT(34, ci_byte, ci_get_rgb_to_gray_status, (ci_const_structrp
    ci_ptr));
#endif

#ifdef CI_BUILD_GRAYSCALE_PALETTE_SUPPORTED
CI_EXPORT(35, void, ci_build_grayscale_palette, (int bit_depth,
    ci_colorp palette));
#endif

#ifdef CI_READ_ALPHA_MODE_SUPPORTED
/* How the alpha channel is interpreted - this affects how the color channels
 * of a CI file are returned to the calling application when an alpha channel,
 * or a tRNS chunk in a palette file, is present.
 *
 * This has no effect on the way pixels are written into a CI output
 * datastream. The color samples in a CI datastream are never premultiplied
 * with the alpha samples.
 *
 * The default is to return data according to the CI specification: the alpha
 * channel is a linear measure of the contribution of the pixel to the
 * corresponding composited pixel, and the color channels are unassociated
 * (not premultiplied).  The gamma encoded color channels must be scaled
 * according to the contribution and to do this it is necessary to undo
 * the encoding, scale the color values, perform the composition and re-encode
 * the values.  This is the 'CI' mode.
 *
 * The alternative is to 'associate' the alpha with the color information by
 * storing color channel values that have been scaled by the alpha.
 * image.  These are the 'STANDARD', 'ASSOCIATED' or 'PREMULTIPLIED' modes
 * (the latter being the two common names for associated alpha color channels).
 *
 * For the 'OPTIMIZED' mode, a pixel is treated as opaque only if the alpha
 * value is equal to the maximum value.
 *
 * The final choice is to gamma encode the alpha channel as well.  This is
 * broken because, in practice, no implementation that uses this choice
 * correctly undoes the encoding before handling alpha composition.  Use this
 * choice only if other serious errors in the software or hardware you use
 * mandate it; the typical serious error is for dark halos to appear around
 * opaque areas of the composited CI image because of arithmetic overflow.
 *
 * The API function ci_set_alpha_mode specifies which of these choices to use
 * with an enumerated 'mode' value and the gamma of the required output:
 */
#define CI_ALPHA_CI           0 /* according to the CI standard */
#define CI_ALPHA_STANDARD      1 /* according to Porter/Duff */
#define CI_ALPHA_ASSOCIATED    1 /* as above; this is the normal practice */
#define CI_ALPHA_PREMULTIPLIED 1 /* as above */
#define CI_ALPHA_OPTIMIZED     2 /* 'CI' for opaque pixels, else 'STANDARD' */
#define CI_ALPHA_BROKEN        3 /* the alpha channel is gamma encoded */

CI_FP_EXPORT(227, void, ci_set_alpha_mode, (ci_structrp ci_ptr, int mode,
    double output_gamma))
CI_FIXED_EXPORT(228, void, ci_set_alpha_mode_fixed, (ci_structrp ci_ptr,
    int mode, ci_fixed_point output_gamma))
#endif

#if defined(CI_GAMMA_SUPPORTED) || defined(CI_READ_ALPHA_MODE_SUPPORTED)
/* The output_gamma value is a screen gamma in libci terminology: it expresses
 * how to decode the output values, not how they are encoded.
 */
#define CI_DEFAULT_sRGB -1       /* sRGB gamma and color space */
#define CI_GAMMA_MAC_18 -2       /* Old Mac '1.8' gamma and color space */
#define CI_GAMMA_sRGB   220000   /* Television standards--matches sRGB gamma */
#define CI_GAMMA_LINEAR CI_FP_1 /* Linear */
#endif

/* The following are examples of calls to ci_set_alpha_mode to achieve the
 * required overall gamma correction and, where necessary, alpha
 * premultiplication.
 *
 * ci_set_alpha_mode(pp, CI_ALPHA_CI, CI_DEFAULT_sRGB);
 *    This is the default libci handling of the alpha channel - it is not
 *    pre-multiplied into the color components.  In addition the call states
 *    that the output is for a sRGB system and causes all CI files without gAMA
 *    chunks to be assumed to be encoded using sRGB.
 *
 * ci_set_alpha_mode(pp, CI_ALPHA_CI, CI_GAMMA_MAC);
 *    In this case the output is assumed to be something like an sRGB conformant
 *    display preceded by a power-law lookup table of power 1.45.  This is how
 *    early Mac systems behaved.
 *
 * ci_set_alpha_mode(pp, CI_ALPHA_STANDARD, CI_GAMMA_LINEAR);
 *    This is the classic Jim Blinn approach and will work in academic
 *    environments where everything is done by the book.  It has the shortcoming
 *    of assuming that input CI data with no gamma information is linear - this
 *    is unlikely to be correct unless the CI files where generated locally.
 *    Most of the time the output precision will be so low as to show
 *    significant banding in dark areas of the image.
 *
 * ci_set_expand_16(pp);
 * ci_set_alpha_mode(pp, CI_ALPHA_STANDARD, CI_DEFAULT_sRGB);
 *    This is a somewhat more realistic Jim Blinn inspired approach.  CI files
 *    are assumed to have the sRGB encoding if not marked with a gamma value and
 *    the output is always 16 bits per component.  This permits accurate scaling
 *    and processing of the data.  If you know that your input CI files were
 *    generated locally you might need to replace CI_DEFAULT_sRGB with the
 *    correct value for your system.
 *
 * ci_set_alpha_mode(pp, CI_ALPHA_OPTIMIZED, CI_DEFAULT_sRGB);
 *    If you just need to composite the CI image onto an existing background
 *    and if you control the code that does this you can use the optimization
 *    setting.  In this case you just copy completely opaque pixels to the
 *    output.  For pixels that are not completely transparent (you just skip
 *    those) you do the composition math using ci_composite or ci_composite_16
 *    below then encode the resultant 8-bit or 16-bit values to match the output
 *    encoding.
 *
 * Other cases
 *    If neither the CI nor the standard linear encoding work for you because
 *    of the software or hardware you use then you have a big problem.  The CI
 *    case will probably result in halos around the image.  The linear encoding
 *    will probably result in a washed out, too bright, image (it's actually too
 *    contrasty.)  Try the ALPHA_OPTIMIZED mode above - this will probably
 *    substantially reduce the halos.  Alternatively try:
 *
 * ci_set_alpha_mode(pp, CI_ALPHA_BROKEN, CI_DEFAULT_sRGB);
 *    This option will also reduce the halos, but there will be slight dark
 *    halos round the opaque parts of the image where the background is light.
 *    In the OPTIMIZED mode the halos will be light halos where the background
 *    is dark.  Take your pick - the halos are unavoidable unless you can get
 *    your hardware/software fixed!  (The OPTIMIZED approach is slightly
 *    faster.)
 *
 * When the default gamma of CI files doesn't match the output gamma.
 *    If you have CI files with no gamma information ci_set_alpha_mode allows
 *    you to provide a default gamma, but it also sets the output gamma to the
 *    matching value.  If you know your CI files have a gamma that doesn't
 *    match the output you can take advantage of the fact that
 *    ci_set_alpha_mode always sets the output gamma but only sets the CI
 *    default if it is not already set:
 *
 * ci_set_alpha_mode(pp, CI_ALPHA_CI, CI_DEFAULT_sRGB);
 * ci_set_alpha_mode(pp, CI_ALPHA_CI, CI_GAMMA_MAC);
 *    The first call sets both the default and the output gamma values, the
 *    second call overrides the output gamma without changing the default.  This
 *    is easier than achieving the same effect with ci_set_gamma.  You must use
 *    CI_ALPHA_CI for the first call - internal checking in ci_set_alpha will
 *    fire if more than one call to ci_set_alpha_mode and ci_set_background is
 *    made in the same read operation, however multiple calls with CI_ALPHA_CI
 *    are ignored.
 */

#ifdef CI_READ_STRIP_ALPHA_SUPPORTED
CI_EXPORT(36, void, ci_set_strip_alpha, (ci_structrp ci_ptr));
#endif

#if defined(CI_READ_SWAP_ALPHA_SUPPORTED) || \
    defined(CI_WRITE_SWAP_ALPHA_SUPPORTED)
CI_EXPORT(37, void, ci_set_swap_alpha, (ci_structrp ci_ptr));
#endif

#if defined(CI_READ_INVERT_ALPHA_SUPPORTED) || \
    defined(CI_WRITE_INVERT_ALPHA_SUPPORTED)
CI_EXPORT(38, void, ci_set_invert_alpha, (ci_structrp ci_ptr));
#endif

#if defined(CI_READ_FILLER_SUPPORTED) || defined(CI_WRITE_FILLER_SUPPORTED)
/* Add a filler byte to 8-bit or 16-bit Gray or 24-bit or 48-bit RGB images. */
CI_EXPORT(39, void, ci_set_filler, (ci_structrp ci_ptr, ci_uint_32 filler,
    int flags));
/* The values of the CI_FILLER_ defines should NOT be changed */
#  define CI_FILLER_BEFORE 0
#  define CI_FILLER_AFTER 1
/* Add an alpha byte to 8-bit or 16-bit Gray or 24-bit or 48-bit RGB images. */
CI_EXPORT(40, void, ci_set_add_alpha, (ci_structrp ci_ptr,
    ci_uint_32 filler, int flags));
#endif /* READ_FILLER || WRITE_FILLER */

#if defined(CI_READ_SWAP_SUPPORTED) || defined(CI_WRITE_SWAP_SUPPORTED)
/* Swap bytes in 16-bit depth files. */
CI_EXPORT(41, void, ci_set_swap, (ci_structrp ci_ptr));
#endif

#if defined(CI_READ_PACK_SUPPORTED) || defined(CI_WRITE_PACK_SUPPORTED)
/* Use 1 byte per pixel in 1, 2, or 4-bit depth files. */
CI_EXPORT(42, void, ci_set_packing, (ci_structrp ci_ptr));
#endif

#if defined(CI_READ_PACKSWAP_SUPPORTED) || \
    defined(CI_WRITE_PACKSWAP_SUPPORTED)
/* Swap packing order of pixels in bytes. */
CI_EXPORT(43, void, ci_set_packswap, (ci_structrp ci_ptr));
#endif

#if defined(CI_READ_SHIFT_SUPPORTED) || defined(CI_WRITE_SHIFT_SUPPORTED)
/* Converts files to legal bit depths. */
CI_EXPORT(44, void, ci_set_shift, (ci_structrp ci_ptr, ci_const_color_8p
    true_bits));
#endif

#if defined(CI_READ_INTERLACING_SUPPORTED) || \
    defined(CI_WRITE_INTERLACING_SUPPORTED)
/* Have the code handle the interlacing.  Returns the number of passes.
 * MUST be called before ci_read_update_info or ci_start_read_image,
 * otherwise it will not have the desired effect.  Note that it is still
 * necessary to call ci_read_row or ci_read_rows ci_get_image_height
 * times for each pass.
*/
CI_EXPORT(45, int, ci_set_interlace_handling, (ci_structrp ci_ptr));
#endif

#if defined(CI_READ_INVERT_SUPPORTED) || defined(CI_WRITE_INVERT_SUPPORTED)
/* Invert monochrome files */
CI_EXPORT(46, void, ci_set_invert_mono, (ci_structrp ci_ptr));
#endif

#ifdef CI_READ_BACKGROUND_SUPPORTED
/* Handle alpha and tRNS by replacing with a background color.  Prior to
 * libci-1.5.4 this API must not be called before the CI file header has been
 * read.  Doing so will result in unexpected behavior and possible warnings or
 * errors if the CI file contains a bKGD chunk.
 */
CI_FP_EXPORT(47, void, ci_set_background, (ci_structrp ci_ptr,
    ci_const_color_16p background_color, int background_gamma_code,
    int need_expand, double background_gamma))
CI_FIXED_EXPORT(215, void, ci_set_background_fixed, (ci_structrp ci_ptr,
    ci_const_color_16p background_color, int background_gamma_code,
    int need_expand, ci_fixed_point background_gamma))
#endif
#ifdef CI_READ_BACKGROUND_SUPPORTED
#  define CI_BACKGROUND_GAMMA_UNKNOWN 0
#  define CI_BACKGROUND_GAMMA_SCREEN  1
#  define CI_BACKGROUND_GAMMA_FILE    2
#  define CI_BACKGROUND_GAMMA_UNIQUE  3
#endif

#ifdef CI_READ_SCALE_16_TO_8_SUPPORTED
/* Scale a 16-bit depth file down to 8-bit, accurately. */
CI_EXPORT(229, void, ci_set_scale_16, (ci_structrp ci_ptr));
#endif

#ifdef CI_READ_STRIP_16_TO_8_SUPPORTED
#define CI_READ_16_TO_8_SUPPORTED /* Name prior to 1.5.4 */
/* Strip the second byte of information from a 16-bit depth file. */
CI_EXPORT(48, void, ci_set_strip_16, (ci_structrp ci_ptr));
#endif

#ifdef CI_READ_QUANTIZE_SUPPORTED
/* Turn on quantizing, and reduce the palette to the number of colors
 * available.
 */
CI_EXPORT(49, void, ci_set_quantize, (ci_structrp ci_ptr,
    ci_colorp palette, int num_palette, int maximum_colors,
    ci_const_uint_16p histogram, int full_quantize));
#endif

#ifdef CI_READ_GAMMA_SUPPORTED
/* The threshold on gamma processing is configurable but hard-wired into the
 * library.  The following is the floating point variant.
 */
#define CI_GAMMA_THRESHOLD (CI_GAMMA_THRESHOLD_FIXED*.00001)

/* Handle gamma correction. Screen_gamma=(display_exponent).
 * NOTE: this API simply sets the screen and file gamma values. It will
 * therefore override the value for gamma in a CI file if it is called after
 * the file header has been read - use with care  - call before reading the CI
 * file for best results!
 *
 * These routines accept the same gamma values as ci_set_alpha_mode (described
 * above).  The CI_GAMMA_ defines and CI_DEFAULT_sRGB can be passed to either
 * API (floating point or fixed.)  Notice, however, that the 'file_gamma' value
 * is the inverse of a 'screen gamma' value.
 */
CI_FP_EXPORT(50, void, ci_set_gamma, (ci_structrp ci_ptr,
    double screen_gamma, double override_file_gamma))
CI_FIXED_EXPORT(208, void, ci_set_gamma_fixed, (ci_structrp ci_ptr,
    ci_fixed_point screen_gamma, ci_fixed_point override_file_gamma))
#endif

#ifdef CI_WRITE_FLUSH_SUPPORTED
/* Set how many lines between output flushes - 0 for no flushing */
CI_EXPORT(51, void, ci_set_flush, (ci_structrp ci_ptr, int nrows));
/* Flush the current CI output buffer */
CI_EXPORT(52, void, ci_write_flush, (ci_structrp ci_ptr));
#endif

/* Optional update palette with requested transformations */
CI_EXPORT(53, void, ci_start_read_image, (ci_structrp ci_ptr));

/* Optional call to update the users info structure */
CI_EXPORT(54, void, ci_read_update_info, (ci_structrp ci_ptr,
    ci_inforp info_ptr));

#ifdef CI_SEQUENTIAL_READ_SUPPORTED
/* Read one or more rows of image data. */
CI_EXPORT(55, void, ci_read_rows, (ci_structrp ci_ptr, ci_bytepp row,
    ci_bytepp display_row, ci_uint_32 num_rows));
#endif

#ifdef CI_SEQUENTIAL_READ_SUPPORTED
/* Read a row of data. */
CI_EXPORT(56, void, ci_read_row, (ci_structrp ci_ptr, ci_bytep row,
    ci_bytep display_row));
#endif

#ifdef CI_SEQUENTIAL_READ_SUPPORTED
/* Read the whole image into memory at once. */
CI_EXPORT(57, void, ci_read_image, (ci_structrp ci_ptr, ci_bytepp image));
#endif

/* Write a row of image data */
CI_EXPORT(58, void, ci_write_row, (ci_structrp ci_ptr,
    ci_const_bytep row));

/* Write a few rows of image data: (*row) is not written; however, the type
 * is declared as writeable to maintain compatibility with previous versions
 * of libci and to allow the 'display_row' array from read_rows to be passed
 * unchanged to write_rows.
 */
CI_EXPORT(59, void, ci_write_rows, (ci_structrp ci_ptr, ci_bytepp row,
    ci_uint_32 num_rows));

/* Write the image data */
CI_EXPORT(60, void, ci_write_image, (ci_structrp ci_ptr, ci_bytepp image));

/* Write the end of the CI file. */
CI_EXPORT(61, void, ci_write_end, (ci_structrp ci_ptr,
    ci_inforp info_ptr));

#ifdef CI_SEQUENTIAL_READ_SUPPORTED
/* Read the end of the CI file. */
CI_EXPORT(62, void, ci_read_end, (ci_structrp ci_ptr, ci_inforp info_ptr));
#endif

/* Free any memory associated with the ci_info_struct */
CI_EXPORT(63, void, ci_destroy_info_struct, (ci_const_structrp ci_ptr,
    ci_infopp info_ptr_ptr));

/* Free any memory associated with the ci_struct and the ci_info_structs */
CI_EXPORT(64, void, ci_destroy_read_struct, (ci_structpp ci_ptr_ptr,
    ci_infopp info_ptr_ptr, ci_infopp end_info_ptr_ptr));

/* Free any memory associated with the ci_struct and the ci_info_structs */
CI_EXPORT(65, void, ci_destroy_write_struct, (ci_structpp ci_ptr_ptr,
    ci_infopp info_ptr_ptr));

/* Set the libci method of handling chunk CRC errors */
CI_EXPORT(66, void, ci_set_crc_action, (ci_structrp ci_ptr, int crit_action,
    int ancil_action));

/* Values for ci_set_crc_action() say how to handle CRC errors in
 * ancillary and critical chunks, and whether to use the data contained
 * therein.  Note that it is impossible to "discard" data in a critical
 * chunk.  For versions prior to 0.90, the action was always error/quit,
 * whereas in version 0.90 and later, the action for CRC errors in ancillary
 * chunks is warn/discard.  These values should NOT be changed.
 *
 *      value                       action:critical     action:ancillary
 */
#define CI_CRC_DEFAULT       0  /* error/quit          warn/discard data */
#define CI_CRC_ERROR_QUIT    1  /* error/quit          error/quit        */
#define CI_CRC_WARN_DISCARD  2  /* (INVALID)           warn/discard data */
#define CI_CRC_WARN_USE      3  /* warn/use data       warn/use data     */
#define CI_CRC_QUIET_USE     4  /* quiet/use data      quiet/use data    */
#define CI_CRC_NO_CHANGE     5  /* use current value   use current value */

#ifdef CI_WRITE_SUPPORTED
/* These functions give the user control over the scan-line filtering in
 * libci and the compression methods used by zlib.  These functions are
 * mainly useful for testing, as the defaults should work with most users.
 * Those users who are tight on memory or want faster performance at the
 * expense of compression can modify them.  See the compression library
 * header file (zlib.h) for an explanation of the compression functions.
 */

/* Set the filtering method(s) used by libci.  Currently, the only valid
 * value for "method" is 0.
 */
CI_EXPORT(67, void, ci_set_filter, (ci_structrp ci_ptr, int method,
    int filters));
#endif /* WRITE */

/* Flags for ci_set_filter() to say which filters to use.  The flags
 * are chosen so that they don't conflict with real filter types
 * below, in case they are supplied instead of the #defined constants.
 * These values should NOT be changed.
 */
#define CI_NO_FILTERS     0x00
#define CI_FILTER_NONE    0x08
#define CI_FILTER_SUB     0x10
#define CI_FILTER_UP      0x20
#define CI_FILTER_AVG     0x40
#define CI_FILTER_PAETH   0x80
#define CI_FAST_FILTERS (CI_FILTER_NONE | CI_FILTER_SUB | CI_FILTER_UP)
#define CI_ALL_FILTERS (CI_FAST_FILTERS | CI_FILTER_AVG | CI_FILTER_PAETH)

/* Filter values (not flags) - used in ciwrite.c, ciwutil.c for now.
 * These defines should NOT be changed.
 */
#define CI_FILTER_VALUE_NONE  0
#define CI_FILTER_VALUE_SUB   1
#define CI_FILTER_VALUE_UP    2
#define CI_FILTER_VALUE_AVG   3
#define CI_FILTER_VALUE_PAETH 4
#define CI_FILTER_VALUE_LAST  5

#ifdef CI_WRITE_SUPPORTED
#ifdef CI_WRITE_WEIGHTED_FILTER_SUPPORTED /* DEPRECATED */
CI_FP_EXPORT(68, void, ci_set_filter_heuristics, (ci_structrp ci_ptr,
    int heuristic_method, int num_weights, ci_const_doublep filter_weights,
    ci_const_doublep filter_costs))
CI_FIXED_EXPORT(209, void, ci_set_filter_heuristics_fixed,
    (ci_structrp ci_ptr, int heuristic_method, int num_weights,
    ci_const_fixed_point_p filter_weights,
    ci_const_fixed_point_p filter_costs))
#endif /* WRITE_WEIGHTED_FILTER */

/* The following are no longer used and will be removed from libci-1.7: */
#define CI_FILTER_HEURISTIC_DEFAULT    0  /* Currently "UNWEIGHTED" */
#define CI_FILTER_HEURISTIC_UNWEIGHTED 1  /* Used by libci < 0.95 */
#define CI_FILTER_HEURISTIC_WEIGHTED   2  /* Experimental feature */
#define CI_FILTER_HEURISTIC_LAST       3  /* Not a valid value */

/* Set the library compression level.  Currently, valid values range from
 * 0 - 9, corresponding directly to the zlib compression levels 0 - 9
 * (0 - no compression, 9 - "maximal" compression).  Note that tests have
 * shown that zlib compression levels 3-6 usually perform as well as level 9
 * for CI images, and do considerably fewer calculations.  In the future,
 * these values may not correspond directly to the zlib compression levels.
 */
#ifdef CI_WRITE_CUSTOMIZE_COMPRESSION_SUPPORTED
CI_EXPORT(69, void, ci_set_compression_level, (ci_structrp ci_ptr,
    int level));

CI_EXPORT(70, void, ci_set_compression_mem_level, (ci_structrp ci_ptr,
    int mem_level));

CI_EXPORT(71, void, ci_set_compression_strategy, (ci_structrp ci_ptr,
    int strategy));

/* If CI_WRITE_OPTIMIZE_CMF_SUPPORTED is defined, libci will use a
 * smaller value of window_bits if it can do so safely.
 */
CI_EXPORT(72, void, ci_set_compression_window_bits, (ci_structrp ci_ptr,
    int window_bits));

CI_EXPORT(73, void, ci_set_compression_method, (ci_structrp ci_ptr,
    int method));
#endif /* WRITE_CUSTOMIZE_COMPRESSION */

#ifdef CI_WRITE_CUSTOMIZE_ZTXT_COMPRESSION_SUPPORTED
/* Also set zlib parameters for compressing non-IDAT chunks */
CI_EXPORT(222, void, ci_set_text_compression_level, (ci_structrp ci_ptr,
    int level));

CI_EXPORT(223, void, ci_set_text_compression_mem_level, (ci_structrp ci_ptr,
    int mem_level));

CI_EXPORT(224, void, ci_set_text_compression_strategy, (ci_structrp ci_ptr,
    int strategy));

/* If CI_WRITE_OPTIMIZE_CMF_SUPPORTED is defined, libci will use a
 * smaller value of window_bits if it can do so safely.
 */
CI_EXPORT(225, void, ci_set_text_compression_window_bits,
    (ci_structrp ci_ptr, int window_bits));

CI_EXPORT(226, void, ci_set_text_compression_method, (ci_structrp ci_ptr,
    int method));
#endif /* WRITE_CUSTOMIZE_ZTXT_COMPRESSION */
#endif /* WRITE */

/* These next functions are called for input/output, memory, and error
 * handling.  They are in the file cirio.c, ciwio.c, and cierror.c,
 * and call standard C I/O routines such as fread(), fwrite(), and
 * fprintf().  These functions can be made to use other I/O routines
 * at run time for those applications that need to handle I/O in a
 * different manner by calling ci_set_???_fn().  See libci-manual.txt for
 * more information.
 */

#ifdef CI_STDIO_SUPPORTED
/* Initialize the input/output for the CI file to the default functions. */
CI_EXPORT(74, void, ci_init_io, (ci_structrp ci_ptr, FILE *fp));
#endif

/* Replace the (error and abort), and warning functions with user
 * supplied functions.  If no messages are to be printed you must still
 * write and use replacement functions. The replacement error_fn should
 * still do a longjmp to the last setjmp location if you are using this
 * method of error handling.  If error_fn or warning_fn is NULL, the
 * default function will be used.
 */

CI_EXPORT(75, void, ci_set_error_fn, (ci_structrp ci_ptr,
    ci_voidp error_ptr, ci_error_ptr error_fn, ci_error_ptr warning_fn));

/* Return the user pointer associated with the error functions */
CI_EXPORT(76, ci_voidp, ci_get_error_ptr, (ci_const_structrp ci_ptr));

/* Replace the default data output functions with a user supplied one(s).
 * If buffered output is not used, then output_flush_fn can be set to NULL.
 * If CI_WRITE_FLUSH_SUPPORTED is not defined at libci compile time
 * output_flush_fn will be ignored (and thus can be NULL).
 * It is probably a mistake to use NULL for output_flush_fn if
 * write_data_fn is not also NULL unless you have built libci with
 * CI_WRITE_FLUSH_SUPPORTED undefined, because in this case libci's
 * default flush function, which uses the standard *FILE structure, will
 * be used.
 */
CI_EXPORT(77, void, ci_set_write_fn, (ci_structrp ci_ptr, ci_voidp io_ptr,
    ci_rw_ptr write_data_fn, ci_flush_ptr output_flush_fn));

/* Replace the default data input function with a user supplied one. */
CI_EXPORT(78, void, ci_set_read_fn, (ci_structrp ci_ptr, ci_voidp io_ptr,
    ci_rw_ptr read_data_fn));

/* Return the user pointer associated with the I/O functions */
CI_EXPORT(79, ci_voidp, ci_get_io_ptr, (ci_const_structrp ci_ptr));

CI_EXPORT(80, void, ci_set_read_status_fn, (ci_structrp ci_ptr,
    ci_read_status_ptr read_row_fn));

CI_EXPORT(81, void, ci_set_write_status_fn, (ci_structrp ci_ptr,
    ci_write_status_ptr write_row_fn));

#ifdef CI_USER_MEM_SUPPORTED
/* Replace the default memory allocation functions with user supplied one(s). */
CI_EXPORT(82, void, ci_set_mem_fn, (ci_structrp ci_ptr, ci_voidp mem_ptr,
    ci_malloc_ptr malloc_fn, ci_free_ptr free_fn));
/* Return the user pointer associated with the memory functions */
CI_EXPORT(83, ci_voidp, ci_get_mem_ptr, (ci_const_structrp ci_ptr));
#endif

#ifdef CI_READ_USER_TRANSFORM_SUPPORTED
CI_EXPORT(84, void, ci_set_read_user_transform_fn, (ci_structrp ci_ptr,
    ci_user_transform_ptr read_user_transform_fn));
#endif

#ifdef CI_WRITE_USER_TRANSFORM_SUPPORTED
CI_EXPORT(85, void, ci_set_write_user_transform_fn, (ci_structrp ci_ptr,
    ci_user_transform_ptr write_user_transform_fn));
#endif

#ifdef CI_USER_TRANSFORM_PTR_SUPPORTED
CI_EXPORT(86, void, ci_set_user_transform_info, (ci_structrp ci_ptr,
    ci_voidp user_transform_ptr, int user_transform_depth,
    int user_transform_channels));
/* Return the user pointer associated with the user transform functions */
CI_EXPORT(87, ci_voidp, ci_get_user_transform_ptr,
    (ci_const_structrp ci_ptr));
#endif

#ifdef CI_USER_TRANSFORM_INFO_SUPPORTED
/* Return information about the row currently being processed.  Note that these
 * APIs do not fail but will return unexpected results if called outside a user
 * transform callback.  Also note that when transforming an interlaced image the
 * row number is the row number within the sub-image of the interlace pass, so
 * the value will increase to the height of the sub-image (not the full image)
 * then reset to 0 for the next pass.
 *
 * Use CI_ROW_FROM_PASS_ROW(row, pass) and CI_COL_FROM_PASS_COL(col, pass) to
 * find the output pixel (x,y) given an interlaced sub-image pixel
 * (row,col,pass).  (See below for these macros.)
 */
CI_EXPORT(217, ci_uint_32, ci_get_current_row_number, (ci_const_structrp));
CI_EXPORT(218, ci_byte, ci_get_current_pass_number, (ci_const_structrp));
#endif

#ifdef CI_READ_USER_CHUNKS_SUPPORTED
/* This callback is called only for *unknown* chunks.  If
 * CI_HANDLE_AS_UNKNOWN_SUPPORTED is set then it is possible to set known
 * chunks to be treated as unknown, however in this case the callback must do
 * any processing required by the chunk (e.g. by calling the appropriate
 * ci_set_ APIs.)
 *
 * There is no write support - on write, by default, all the chunks in the
 * 'unknown' list are written in the specified position.
 *
 * The integer return from the callback function is interpreted thus:
 *
 * negative: An error occurred; ci_chunk_error will be called.
 *     zero: The chunk was not handled, the chunk will be saved. A critical
 *           chunk will cause an error at this point unless it is to be saved.
 * positive: The chunk was handled, libci will ignore/discard it.
 *
 * See "INTERACTION WITH USER CHUNK CALLBACKS" below for important notes about
 * how this behavior will change in libci 1.7
 */
CI_EXPORT(88, void, ci_set_read_user_chunk_fn, (ci_structrp ci_ptr,
    ci_voidp user_chunk_ptr, ci_user_chunk_ptr read_user_chunk_fn));
#endif

#ifdef CI_USER_CHUNKS_SUPPORTED
CI_EXPORT(89, ci_voidp, ci_get_user_chunk_ptr, (ci_const_structrp ci_ptr));
#endif

#ifdef CI_PROGRESSIVE_READ_SUPPORTED
/* Sets the function callbacks for the push reader, and a pointer to a
 * user-defined structure available to the callback functions.
 */
CI_EXPORT(90, void, ci_set_progressive_read_fn, (ci_structrp ci_ptr,
    ci_voidp progressive_ptr, ci_progressive_info_ptr info_fn,
    ci_progressive_row_ptr row_fn, ci_progressive_end_ptr end_fn));

/* Returns the user pointer associated with the push read functions */
CI_EXPORT(91, ci_voidp, ci_get_progressive_ptr,
    (ci_const_structrp ci_ptr));

/* Function to be called when data becomes available */
CI_EXPORT(92, void, ci_process_data, (ci_structrp ci_ptr,
    ci_inforp info_ptr, ci_bytep buffer, size_t buffer_size));

/* A function which may be called *only* within ci_process_data to stop the
 * processing of any more data.  The function returns the number of bytes
 * remaining, excluding any that libci has cached internally.  A subsequent
 * call to ci_process_data must supply these bytes again.  If the argument
 * 'save' is set to true the routine will first save all the pending data and
 * will always return 0.
 */
CI_EXPORT(219, size_t, ci_process_data_pause, (ci_structrp, int save));

/* A function which may be called *only* outside (after) a call to
 * ci_process_data.  It returns the number of bytes of data to skip in the
 * input.  Normally it will return 0, but if it returns a non-zero value the
 * application must skip than number of bytes of input data and pass the
 * following data to the next call to ci_process_data.
 */
CI_EXPORT(220, ci_uint_32, ci_process_data_skip, (ci_structrp));

/* Function that combines rows.  'new_row' is a flag that should come from
 * the callback and be non-NULL if anything needs to be done; the library
 * stores its own version of the new data internally and ignores the passed
 * in value.
 */
CI_EXPORT(93, void, ci_progressive_combine_row, (ci_const_structrp ci_ptr,
    ci_bytep old_row, ci_const_bytep new_row));
#endif /* PROGRESSIVE_READ */

CI_EXPORTA(94, ci_voidp, ci_malloc, (ci_const_structrp ci_ptr,
    ci_alloc_size_t size), CI_ALLOCATED);
/* Added at libci version 1.4.0 */
CI_EXPORTA(95, ci_voidp, ci_calloc, (ci_const_structrp ci_ptr,
    ci_alloc_size_t size), CI_ALLOCATED);

/* Added at libci version 1.2.4 */
CI_EXPORTA(96, ci_voidp, ci_malloc_warn, (ci_const_structrp ci_ptr,
    ci_alloc_size_t size), CI_ALLOCATED);

/* Frees a pointer allocated by ci_malloc() */
CI_EXPORT(97, void, ci_free, (ci_const_structrp ci_ptr, ci_voidp ptr));

/* Free data that was allocated internally */
CI_EXPORT(98, void, ci_free_data, (ci_const_structrp ci_ptr,
    ci_inforp info_ptr, ci_uint_32 free_me, int num));

/* Reassign the responsibility for freeing existing data, whether allocated
 * by libci or by the application; this works on the ci_info structure passed
 * in, without changing the state for other ci_info structures.
 */
CI_EXPORT(99, void, ci_data_freer, (ci_const_structrp ci_ptr,
    ci_inforp info_ptr, int freer, ci_uint_32 mask));

/* Assignments for ci_data_freer */
#define CI_DESTROY_WILL_FREE_DATA 1
#define CI_SET_WILL_FREE_DATA 1
#define CI_USER_WILL_FREE_DATA 2
/* Flags for ci_ptr->free_me and info_ptr->free_me */
#define CI_FREE_HIST 0x0008U
#define CI_FREE_ICCP 0x0010U
#define CI_FREE_SPLT 0x0020U
#define CI_FREE_ROWS 0x0040U
#define CI_FREE_PCAL 0x0080U
#define CI_FREE_SCAL 0x0100U
#ifdef CI_STORE_UNKNOWN_CHUNKS_SUPPORTED
#  define CI_FREE_UNKN 0x0200U
#endif
/*      CI_FREE_LIST 0x0400U   removed in 1.6.0 because it is ignored */
#define CI_FREE_PLTE 0x1000U
#define CI_FREE_TRNS 0x2000U
#define CI_FREE_TEXT 0x4000U
#define CI_FREE_EXIF 0x8000U /* Added at libci-1.6.31 */
#define CI_FREE_ALL  0xffffU
#define CI_FREE_MUL  0x4220U /* CI_FREE_SPLT|CI_FREE_TEXT|CI_FREE_UNKN */

#ifdef CI_USER_MEM_SUPPORTED
CI_EXPORTA(100, ci_voidp, ci_malloc_default, (ci_const_structrp ci_ptr,
    ci_alloc_size_t size), CI_ALLOCATED CI_DEPRECATED);
CI_EXPORTA(101, void, ci_free_default, (ci_const_structrp ci_ptr,
    ci_voidp ptr), CI_DEPRECATED);
#endif

#ifdef CI_ERROR_TEXT_SUPPORTED
/* Fatal error in CI image of libci - can't continue */
CI_EXPORTA(102, void, ci_error, (ci_const_structrp ci_ptr,
    ci_const_charp error_message), CI_NORETURN);

/* The same, but the chunk name is prepended to the error string. */
CI_EXPORTA(103, void, ci_chunk_error, (ci_const_structrp ci_ptr,
    ci_const_charp error_message), CI_NORETURN);

#else
/* Fatal error in CI image of libci - can't continue */
CI_EXPORTA(104, void, ci_err, (ci_const_structrp ci_ptr), CI_NORETURN);
#  define ci_error(s1,s2) ci_err(s1)
#  define ci_chunk_error(s1,s2) ci_err(s1)
#endif

#ifdef CI_WARNINGS_SUPPORTED
/* Non-fatal error in libci.  Can continue, but may have a problem. */
CI_EXPORT(105, void, ci_warning, (ci_const_structrp ci_ptr,
    ci_const_charp warning_message));

/* Non-fatal error in libci, chunk name is prepended to message. */
CI_EXPORT(106, void, ci_chunk_warning, (ci_const_structrp ci_ptr,
    ci_const_charp warning_message));
#else
#  define ci_warning(s1,s2) ((void)(s1))
#  define ci_chunk_warning(s1,s2) ((void)(s1))
#endif

#ifdef CI_BENIGN_ERRORS_SUPPORTED
/* Benign error in libci.  Can continue, but may have a problem.
 * User can choose whether to handle as a fatal error or as a warning. */
CI_EXPORT(107, void, ci_benign_error, (ci_const_structrp ci_ptr,
    ci_const_charp warning_message));

#ifdef CI_READ_SUPPORTED
/* Same, chunk name is prepended to message (only during read) */
CI_EXPORT(108, void, ci_chunk_benign_error, (ci_const_structrp ci_ptr,
    ci_const_charp warning_message));
#endif

CI_EXPORT(109, void, ci_set_benign_errors,
    (ci_structrp ci_ptr, int allowed));
#else
#  ifdef CI_ALLOW_BENIGN_ERRORS
#    define ci_benign_error ci_warning
#    define ci_chunk_benign_error ci_chunk_warning
#  else
#    define ci_benign_error ci_error
#    define ci_chunk_benign_error ci_chunk_error
#  endif
#endif

/* The ci_set_<chunk> functions are for storing values in the ci_info_struct.
 * Similarly, the ci_get_<chunk> calls are used to read values from the
 * ci_info_struct, either storing the parameters in the passed variables, or
 * setting pointers into the ci_info_struct where the data is stored.  The
 * ci_get_<chunk> functions return a non-zero value if the data was available
 * in info_ptr, or return zero and do not change any of the parameters if the
 * data was not available.
 *
 * These functions should be used instead of directly accessing ci_info
 * to avoid problems with future changes in the size and internal layout of
 * ci_info_struct.
 */
/* Returns "flag" if chunk data is valid in info_ptr. */
CI_EXPORT(110, ci_uint_32, ci_get_valid, (ci_const_structrp ci_ptr,
    ci_const_inforp info_ptr, ci_uint_32 flag));

/* Returns number of bytes needed to hold a transformed row. */
CI_EXPORT(111, size_t, ci_get_rowbytes, (ci_const_structrp ci_ptr,
    ci_const_inforp info_ptr));

#ifdef CI_INFO_IMAGE_SUPPORTED
/* Returns row_pointers, which is an array of pointers to scanlines that was
 * returned from ci_read_ci().
 */
CI_EXPORT(112, ci_bytepp, ci_get_rows, (ci_const_structrp ci_ptr,
    ci_const_inforp info_ptr));

/* Set row_pointers, which is an array of pointers to scanlines for use
 * by ci_write_ci().
 */
CI_EXPORT(113, void, ci_set_rows, (ci_const_structrp ci_ptr,
    ci_inforp info_ptr, ci_bytepp row_pointers));
#endif

/* Returns number of color channels in image. */
CI_EXPORT(114, ci_byte, ci_get_channels, (ci_const_structrp ci_ptr,
    ci_const_inforp info_ptr));

#ifdef CI_EASY_ACCESS_SUPPORTED
/* Returns image width in pixels. */
CI_EXPORT(115, ci_uint_32, ci_get_image_width, (ci_const_structrp ci_ptr,
    ci_const_inforp info_ptr));

/* Returns image height in pixels. */
CI_EXPORT(116, ci_uint_32, ci_get_image_height, (ci_const_structrp ci_ptr,
    ci_const_inforp info_ptr));

/* Returns image bit_depth. */
CI_EXPORT(117, ci_byte, ci_get_bit_depth, (ci_const_structrp ci_ptr,
    ci_const_inforp info_ptr));

/* Returns image color_type. */
CI_EXPORT(118, ci_byte, ci_get_color_type, (ci_const_structrp ci_ptr,
    ci_const_inforp info_ptr));

/* Returns image filter_type. */
CI_EXPORT(119, ci_byte, ci_get_filter_type, (ci_const_structrp ci_ptr,
    ci_const_inforp info_ptr));

/* Returns image interlace_type. */
CI_EXPORT(120, ci_byte, ci_get_interlace_type, (ci_const_structrp ci_ptr,
    ci_const_inforp info_ptr));

/* Returns image compression_type. */
CI_EXPORT(121, ci_byte, ci_get_compression_type, (ci_const_structrp ci_ptr,
    ci_const_inforp info_ptr));

/* Returns image resolution in pixels per meter, from pHYs chunk data. */
CI_EXPORT(122, ci_uint_32, ci_get_pixels_per_meter,
    (ci_const_structrp ci_ptr, ci_const_inforp info_ptr));
CI_EXPORT(123, ci_uint_32, ci_get_x_pixels_per_meter,
    (ci_const_structrp ci_ptr, ci_const_inforp info_ptr));
CI_EXPORT(124, ci_uint_32, ci_get_y_pixels_per_meter,
    (ci_const_structrp ci_ptr, ci_const_inforp info_ptr));

/* Returns pixel aspect ratio, computed from pHYs chunk data.  */
CI_FP_EXPORT(125, float, ci_get_pixel_aspect_ratio,
    (ci_const_structrp ci_ptr, ci_const_inforp info_ptr))
CI_FIXED_EXPORT(210, ci_fixed_point, ci_get_pixel_aspect_ratio_fixed,
    (ci_const_structrp ci_ptr, ci_const_inforp info_ptr))

/* Returns image x, y offset in pixels or microns, from oFFs chunk data. */
CI_EXPORT(126, ci_int_32, ci_get_x_offset_pixels,
    (ci_const_structrp ci_ptr, ci_const_inforp info_ptr));
CI_EXPORT(127, ci_int_32, ci_get_y_offset_pixels,
    (ci_const_structrp ci_ptr, ci_const_inforp info_ptr));
CI_EXPORT(128, ci_int_32, ci_get_x_offset_microns,
    (ci_const_structrp ci_ptr, ci_const_inforp info_ptr));
CI_EXPORT(129, ci_int_32, ci_get_y_offset_microns,
    (ci_const_structrp ci_ptr, ci_const_inforp info_ptr));

#endif /* EASY_ACCESS */

#ifdef CI_READ_SUPPORTED
/* Returns pointer to signature string read from CI header */
CI_EXPORT(130, ci_const_bytep, ci_get_signature, (ci_const_structrp ci_ptr,
    ci_const_inforp info_ptr));
#endif

#ifdef CI_bKGD_SUPPORTED
CI_EXPORT(131, ci_uint_32, ci_get_bKGD, (ci_const_structrp ci_ptr,
    ci_inforp info_ptr, ci_color_16p *background));
#endif

#ifdef CI_bKGD_SUPPORTED
CI_EXPORT(132, void, ci_set_bKGD, (ci_const_structrp ci_ptr,
    ci_inforp info_ptr, ci_const_color_16p background));
#endif

#ifdef CI_cHRM_SUPPORTED
CI_FP_EXPORT(133, ci_uint_32, ci_get_cHRM, (ci_const_structrp ci_ptr,
    ci_const_inforp info_ptr, double *white_x, double *white_y, double *red_x,
    double *red_y, double *green_x, double *green_y, double *blue_x,
    double *blue_y))
CI_FP_EXPORT(230, ci_uint_32, ci_get_cHRM_XYZ, (ci_const_structrp ci_ptr,
    ci_const_inforp info_ptr, double *red_X, double *red_Y, double *red_Z,
    double *green_X, double *green_Y, double *green_Z, double *blue_X,
    double *blue_Y, double *blue_Z))
CI_FIXED_EXPORT(134, ci_uint_32, ci_get_cHRM_fixed,
    (ci_const_structrp ci_ptr, ci_const_inforp info_ptr,
    ci_fixed_point *int_white_x, ci_fixed_point *int_white_y,
    ci_fixed_point *int_red_x, ci_fixed_point *int_red_y,
    ci_fixed_point *int_green_x, ci_fixed_point *int_green_y,
    ci_fixed_point *int_blue_x, ci_fixed_point *int_blue_y))
CI_FIXED_EXPORT(231, ci_uint_32, ci_get_cHRM_XYZ_fixed,
    (ci_const_structrp ci_ptr, ci_const_inforp info_ptr,
    ci_fixed_point *int_red_X, ci_fixed_point *int_red_Y,
    ci_fixed_point *int_red_Z, ci_fixed_point *int_green_X,
    ci_fixed_point *int_green_Y, ci_fixed_point *int_green_Z,
    ci_fixed_point *int_blue_X, ci_fixed_point *int_blue_Y,
    ci_fixed_point *int_blue_Z))
#endif

#ifdef CI_cHRM_SUPPORTED
CI_FP_EXPORT(135, void, ci_set_cHRM, (ci_const_structrp ci_ptr,
    ci_inforp info_ptr,
    double white_x, double white_y, double red_x, double red_y, double green_x,
    double green_y, double blue_x, double blue_y))
CI_FP_EXPORT(232, void, ci_set_cHRM_XYZ, (ci_const_structrp ci_ptr,
    ci_inforp info_ptr, double red_X, double red_Y, double red_Z,
    double green_X, double green_Y, double green_Z, double blue_X,
    double blue_Y, double blue_Z))
CI_FIXED_EXPORT(136, void, ci_set_cHRM_fixed, (ci_const_structrp ci_ptr,
    ci_inforp info_ptr, ci_fixed_point int_white_x,
    ci_fixed_point int_white_y, ci_fixed_point int_red_x,
    ci_fixed_point int_red_y, ci_fixed_point int_green_x,
    ci_fixed_point int_green_y, ci_fixed_point int_blue_x,
    ci_fixed_point int_blue_y))
CI_FIXED_EXPORT(233, void, ci_set_cHRM_XYZ_fixed, (ci_const_structrp ci_ptr,
    ci_inforp info_ptr, ci_fixed_point int_red_X, ci_fixed_point int_red_Y,
    ci_fixed_point int_red_Z, ci_fixed_point int_green_X,
    ci_fixed_point int_green_Y, ci_fixed_point int_green_Z,
    ci_fixed_point int_blue_X, ci_fixed_point int_blue_Y,
    ci_fixed_point int_blue_Z))
#endif

#ifdef CI_cICP_SUPPORTED
CI_EXPORT(250, ci_uint_32, ci_get_cICP, (ci_const_structrp ci_ptr,
    ci_const_inforp info_ptr, ci_bytep colour_primaries,
    ci_bytep transfer_function, ci_bytep matrix_coefficients,
    ci_bytep video_full_range_flag));
#endif

#ifdef CI_cICP_SUPPORTED
CI_EXPORT(251, void, ci_set_cICP, (ci_const_structrp ci_ptr,
    ci_inforp info_ptr, ci_byte colour_primaries,
    ci_byte transfer_function, ci_byte matrix_coefficients,
    ci_byte video_full_range_flag));
#endif

#ifdef CI_cLLI_SUPPORTED
CI_FP_EXPORT(252, ci_uint_32, ci_get_cLLI, (ci_const_structrp ci_ptr,
         ci_const_inforp info_ptr, double *maximum_content_light_level,
         double *maximum_frame_average_light_level))
CI_FIXED_EXPORT(253, ci_uint_32, ci_get_cLLI_fixed,
    (ci_const_structrp ci_ptr, ci_const_inforp info_ptr,
    /* The values below are in cd/m2 (nits) and are scaled by 10,000; not
     * 100,000 as in the case of ci_fixed_point.
     */
    ci_uint_32p maximum_content_light_level_scaled_by_10000,
    ci_uint_32p maximum_frame_average_light_level_scaled_by_10000))
#endif

#ifdef CI_cLLI_SUPPORTED
CI_FP_EXPORT(254, void, ci_set_cLLI, (ci_const_structrp ci_ptr,
         ci_inforp info_ptr, double maximum_content_light_level,
         double maximum_frame_average_light_level))
CI_FIXED_EXPORT(255, void, ci_set_cLLI_fixed, (ci_const_structrp ci_ptr,
    ci_inforp info_ptr,
    /* The values below are in cd/m2 (nits) and are scaled by 10,000; not
     * 100,000 as in the case of ci_fixed_point.
     */
    ci_uint_32 maximum_content_light_level_scaled_by_10000,
    ci_uint_32 maximum_frame_average_light_level_scaled_by_10000))
#endif

#ifdef CI_eXIf_SUPPORTED
CI_EXPORT(246, ci_uint_32, ci_get_eXIf, (ci_const_structrp ci_ptr,
    ci_inforp info_ptr, ci_bytep *exif));
CI_EXPORT(247, void, ci_set_eXIf, (ci_const_structrp ci_ptr,
    ci_inforp info_ptr, ci_bytep exif));

CI_EXPORT(248, ci_uint_32, ci_get_eXIf_1, (ci_const_structrp ci_ptr,
    ci_const_inforp info_ptr, ci_uint_32 *num_exif, ci_bytep *exif));
CI_EXPORT(249, void, ci_set_eXIf_1, (ci_const_structrp ci_ptr,
    ci_inforp info_ptr, ci_uint_32 num_exif, ci_bytep exif));
#endif

#ifdef CI_gAMA_SUPPORTED
CI_FP_EXPORT(137, ci_uint_32, ci_get_gAMA, (ci_const_structrp ci_ptr,
    ci_const_inforp info_ptr, double *file_gamma))
CI_FIXED_EXPORT(138, ci_uint_32, ci_get_gAMA_fixed,
    (ci_const_structrp ci_ptr, ci_const_inforp info_ptr,
    ci_fixed_point *int_file_gamma))
#endif

#ifdef CI_gAMA_SUPPORTED
CI_FP_EXPORT(139, void, ci_set_gAMA, (ci_const_structrp ci_ptr,
    ci_inforp info_ptr, double file_gamma))
CI_FIXED_EXPORT(140, void, ci_set_gAMA_fixed, (ci_const_structrp ci_ptr,
    ci_inforp info_ptr, ci_fixed_point int_file_gamma))
#endif

#ifdef CI_hIST_SUPPORTED
CI_EXPORT(141, ci_uint_32, ci_get_hIST, (ci_const_structrp ci_ptr,
    ci_inforp info_ptr, ci_uint_16p *hist));
CI_EXPORT(142, void, ci_set_hIST, (ci_const_structrp ci_ptr,
    ci_inforp info_ptr, ci_const_uint_16p hist));
#endif

CI_EXPORT(143, ci_uint_32, ci_get_IHDR, (ci_const_structrp ci_ptr,
    ci_const_inforp info_ptr, ci_uint_32 *width, ci_uint_32 *height,
    int *bit_depth, int *color_type, int *interlace_method,
    int *compression_method, int *filter_method));

CI_EXPORT(144, void, ci_set_IHDR, (ci_const_structrp ci_ptr,
    ci_inforp info_ptr, ci_uint_32 width, ci_uint_32 height, int bit_depth,
    int color_type, int interlace_method, int compression_method,
    int filter_method));

#ifdef CI_mDCV_SUPPORTED
CI_FP_EXPORT(256, ci_uint_32, ci_get_mDCV, (ci_const_structrp ci_ptr,
    ci_const_inforp info_ptr,
    /* The chromaticities of the mastering display.  As cHRM, but independent of
     * the encoding endpoints in cHRM, or cICP, or iCCP.  These values will
     * always be in the range 0 to 1.3107.
     */
    double *white_x, double *white_y, double *red_x, double *red_y,
    double *green_x, double *green_y, double *blue_x, double *blue_y,
    /* Mastering display luminance in cd/m2 (nits). */
    double *mastering_display_maximum_luminance,
    double *mastering_display_minimum_luminance))

CI_FIXED_EXPORT(257, ci_uint_32, ci_get_mDCV_fixed,
    (ci_const_structrp ci_ptr, ci_const_inforp info_ptr,
    ci_fixed_point *int_white_x, ci_fixed_point *int_white_y,
    ci_fixed_point *int_red_x, ci_fixed_point *int_red_y,
    ci_fixed_point *int_green_x, ci_fixed_point *int_green_y,
    ci_fixed_point *int_blue_x, ci_fixed_point *int_blue_y,
    /* Mastering display luminance in cd/m2 (nits) multiplied (scaled) by
     * 10,000.
     */
    ci_uint_32p mastering_display_maximum_luminance_scaled_by_10000,
    ci_uint_32p mastering_display_minimum_luminance_scaled_by_10000))
#endif

#ifdef CI_mDCV_SUPPORTED
CI_FP_EXPORT(258, void, ci_set_mDCV, (ci_const_structrp ci_ptr,
    ci_inforp info_ptr,
    /* The chromaticities of the mastering display.  As cHRM, but independent of
     * the encoding endpoints in cHRM, or cICP, or iCCP.
     */
    double white_x, double white_y, double red_x, double red_y, double green_x,
    double green_y, double blue_x, double blue_y,
    /* Mastering display luminance in cd/m2 (nits). */
    double mastering_display_maximum_luminance,
    double mastering_display_minimum_luminance))

CI_FIXED_EXPORT(259, void, ci_set_mDCV_fixed, (ci_const_structrp ci_ptr,
    ci_inforp info_ptr,
    /* The admissible range of these values is not the full range of a CI
     * fixed point value.  Negative values cannot be encoded and the maximum
     * value is about 1.3 */
    ci_fixed_point int_white_x, ci_fixed_point int_white_y,
    ci_fixed_point int_red_x, ci_fixed_point int_red_y,
    ci_fixed_point int_green_x, ci_fixed_point int_green_y,
    ci_fixed_point int_blue_x, ci_fixed_point int_blue_y,
    /* These are CI unsigned 4 byte values: 31-bit unsigned values.  The MSB
     * must be zero.
     */
    ci_uint_32 mastering_display_maximum_luminance_scaled_by_10000,
    ci_uint_32 mastering_display_minimum_luminance_scaled_by_10000))
#endif

#ifdef CI_oFFs_SUPPORTED
CI_EXPORT(145, ci_uint_32, ci_get_oFFs, (ci_const_structrp ci_ptr,
   ci_const_inforp info_ptr, ci_int_32 *offset_x, ci_int_32 *offset_y,
   int *unit_type));
#endif

#ifdef CI_oFFs_SUPPORTED
CI_EXPORT(146, void, ci_set_oFFs, (ci_const_structrp ci_ptr,
    ci_inforp info_ptr, ci_int_32 offset_x, ci_int_32 offset_y,
    int unit_type));
#endif

#ifdef CI_pCAL_SUPPORTED
CI_EXPORT(147, ci_uint_32, ci_get_pCAL, (ci_const_structrp ci_ptr,
    ci_inforp info_ptr, ci_charp *purpose, ci_int_32 *X0,
    ci_int_32 *X1, int *type, int *nparams, ci_charp *units,
    ci_charpp *params));
#endif

#ifdef CI_pCAL_SUPPORTED
CI_EXPORT(148, void, ci_set_pCAL, (ci_const_structrp ci_ptr,
    ci_inforp info_ptr, ci_const_charp purpose, ci_int_32 X0, ci_int_32 X1,
    int type, int nparams, ci_const_charp units, ci_charpp params));
#endif

#ifdef CI_pHYs_SUPPORTED
CI_EXPORT(149, ci_uint_32, ci_get_pHYs, (ci_const_structrp ci_ptr,
    ci_const_inforp info_ptr, ci_uint_32 *res_x, ci_uint_32 *res_y,
    int *unit_type));
#endif

#ifdef CI_pHYs_SUPPORTED
CI_EXPORT(150, void, ci_set_pHYs, (ci_const_structrp ci_ptr,
    ci_inforp info_ptr, ci_uint_32 res_x, ci_uint_32 res_y, int unit_type));
#endif

CI_EXPORT(151, ci_uint_32, ci_get_PLTE, (ci_const_structrp ci_ptr,
   ci_inforp info_ptr, ci_colorp *palette, int *num_palette));

CI_EXPORT(152, void, ci_set_PLTE, (ci_structrp ci_ptr,
    ci_inforp info_ptr, ci_const_colorp palette, int num_palette));

#ifdef CI_sBIT_SUPPORTED
CI_EXPORT(153, ci_uint_32, ci_get_sBIT, (ci_const_structrp ci_ptr,
    ci_inforp info_ptr, ci_color_8p *sig_bit));
#endif

#ifdef CI_sBIT_SUPPORTED
CI_EXPORT(154, void, ci_set_sBIT, (ci_const_structrp ci_ptr,
    ci_inforp info_ptr, ci_const_color_8p sig_bit));
#endif

#ifdef CI_sRGB_SUPPORTED
CI_EXPORT(155, ci_uint_32, ci_get_sRGB, (ci_const_structrp ci_ptr,
    ci_const_inforp info_ptr, int *file_srgb_intent));
#endif

#ifdef CI_sRGB_SUPPORTED
CI_EXPORT(156, void, ci_set_sRGB, (ci_const_structrp ci_ptr,
    ci_inforp info_ptr, int srgb_intent));
CI_EXPORT(157, void, ci_set_sRGB_gAMA_and_cHRM, (ci_const_structrp ci_ptr,
    ci_inforp info_ptr, int srgb_intent));
#endif

#ifdef CI_iCCP_SUPPORTED
CI_EXPORT(158, ci_uint_32, ci_get_iCCP, (ci_const_structrp ci_ptr,
    ci_inforp info_ptr, ci_charpp name, int *compression_type,
    ci_bytepp profile, ci_uint_32 *proflen));
#endif

#ifdef CI_iCCP_SUPPORTED
CI_EXPORT(159, void, ci_set_iCCP, (ci_const_structrp ci_ptr,
    ci_inforp info_ptr, ci_const_charp name, int compression_type,
    ci_const_bytep profile, ci_uint_32 proflen));
#endif

#ifdef CI_sPLT_SUPPORTED
CI_EXPORT(160, int, ci_get_sPLT, (ci_const_structrp ci_ptr,
    ci_inforp info_ptr, ci_sPLT_tpp entries));
#endif

#ifdef CI_sPLT_SUPPORTED
CI_EXPORT(161, void, ci_set_sPLT, (ci_const_structrp ci_ptr,
    ci_inforp info_ptr, ci_const_sPLT_tp entries, int nentries));
#endif

#ifdef CI_TEXT_SUPPORTED
/* ci_get_text also returns the number of text chunks in *num_text */
CI_EXPORT(162, int, ci_get_text, (ci_const_structrp ci_ptr,
    ci_inforp info_ptr, ci_textp *text_ptr, int *num_text));
#endif

/* Note while ci_set_text() will accept a structure whose text,
 * language, and  translated keywords are NULL pointers, the structure
 * returned by ci_get_text will always contain regular
 * zero-terminated C strings.  They might be empty strings but
 * they will never be NULL pointers.
 */

#ifdef CI_TEXT_SUPPORTED
CI_EXPORT(163, void, ci_set_text, (ci_const_structrp ci_ptr,
    ci_inforp info_ptr, ci_const_textp text_ptr, int num_text));
#endif

#ifdef CI_tIME_SUPPORTED
CI_EXPORT(164, ci_uint_32, ci_get_tIME, (ci_const_structrp ci_ptr,
    ci_inforp info_ptr, ci_timep *mod_time));
#endif

#ifdef CI_tIME_SUPPORTED
CI_EXPORT(165, void, ci_set_tIME, (ci_const_structrp ci_ptr,
    ci_inforp info_ptr, ci_const_timep mod_time));
#endif

#ifdef CI_tRNS_SUPPORTED
CI_EXPORT(166, ci_uint_32, ci_get_tRNS, (ci_const_structrp ci_ptr,
    ci_inforp info_ptr, ci_bytep *trans_alpha, int *num_trans,
    ci_color_16p *trans_color));
#endif

#ifdef CI_tRNS_SUPPORTED
CI_EXPORT(167, void, ci_set_tRNS, (ci_structrp ci_ptr,
    ci_inforp info_ptr, ci_const_bytep trans_alpha, int num_trans,
    ci_const_color_16p trans_color));
#endif

#ifdef CI_sCAL_SUPPORTED
CI_FP_EXPORT(168, ci_uint_32, ci_get_sCAL, (ci_const_structrp ci_ptr,
    ci_const_inforp info_ptr, int *unit, double *width, double *height))
#if defined(CI_FLOATING_ARITHMETIC_SUPPORTED) || \
   defined(CI_FLOATING_POINT_SUPPORTED)
/* NOTE: this API is currently implemented using floating point arithmetic,
 * consequently it can only be used on systems with floating point support.
 * In any case the range of values supported by ci_fixed_point is small and it
 * is highly recommended that ci_get_sCAL_s be used instead.
 */
CI_FIXED_EXPORT(214, ci_uint_32, ci_get_sCAL_fixed,
    (ci_const_structrp ci_ptr, ci_const_inforp info_ptr, int *unit,
    ci_fixed_point *width, ci_fixed_point *height))
#endif
CI_EXPORT(169, ci_uint_32, ci_get_sCAL_s,
    (ci_const_structrp ci_ptr, ci_const_inforp info_ptr, int *unit,
    ci_charpp swidth, ci_charpp sheight));

CI_FP_EXPORT(170, void, ci_set_sCAL, (ci_const_structrp ci_ptr,
    ci_inforp info_ptr, int unit, double width, double height))
CI_FIXED_EXPORT(213, void, ci_set_sCAL_fixed, (ci_const_structrp ci_ptr,
   ci_inforp info_ptr, int unit, ci_fixed_point width,
   ci_fixed_point height))
CI_EXPORT(171, void, ci_set_sCAL_s, (ci_const_structrp ci_ptr,
    ci_inforp info_ptr, int unit,
    ci_const_charp swidth, ci_const_charp sheight));
#endif /* sCAL */

#ifdef CI_SET_UNKNOWN_CHUNKS_SUPPORTED
/* Provide the default handling for all unknown chunks or, optionally, for
 * specific unknown chunks.
 *
 * NOTE: prior to 1.6.0 the handling specified for particular chunks on read was
 * ignored and the default was used, the per-chunk setting only had an effect on
 * write.  If you wish to have chunk-specific handling on read in code that must
 * work on earlier versions you must use a user chunk callback to specify the
 * desired handling (keep or discard.)
 *
 * The 'keep' parameter is a CI_HANDLE_CHUNK_ value as listed below.  The
 * parameter is interpreted as follows:
 *
 * READ:
 *    CI_HANDLE_CHUNK_AS_DEFAULT:
 *       Known chunks: do normal libci processing, do not keep the chunk (but
 *          see the comments below about CI_HANDLE_AS_UNKNOWN_SUPPORTED)
 *       Unknown chunks: for a specific chunk use the global default, when used
 *          as the default discard the chunk data.
 *    CI_HANDLE_CHUNK_NEVER:
 *       Discard the chunk data.
 *    CI_HANDLE_CHUNK_IF_SAFE:
 *       Keep the chunk data if the chunk is not critical else raise a chunk
 *       error.
 *    CI_HANDLE_CHUNK_ALWAYS:
 *       Keep the chunk data.
 *
 * If the chunk data is saved it can be retrieved using ci_get_unknown_chunks,
 * below.  Notice that specifying "AS_DEFAULT" as a global default is equivalent
 * to specifying "NEVER", however when "AS_DEFAULT" is used for specific chunks
 * it simply resets the behavior to the libci default.
 *
 * INTERACTION WITH USER CHUNK CALLBACKS:
 * The per-chunk handling is always used when there is a ci_user_chunk_ptr
 * callback and the callback returns 0; the chunk is then always stored *unless*
 * it is critical and the per-chunk setting is other than ALWAYS.  Notice that
 * the global default is *not* used in this case.  (In effect the per-chunk
 * value is incremented to at least IF_SAFE.)
 *
 * IMPORTANT NOTE: this behavior will change in libci 1.7 - the global and
 * per-chunk defaults will be honored.  If you want to preserve the current
 * behavior when your callback returns 0 you must set CI_HANDLE_CHUNK_IF_SAFE
 * as the default - if you don't do this libci 1.6 will issue a warning.
 *
 * If you want unhandled unknown chunks to be discarded in libci 1.6 and
 * earlier simply return '1' (handled).
 *
 * CI_HANDLE_AS_UNKNOWN_SUPPORTED:
 *    If this is *not* set known chunks will always be handled by libci and
 *    will never be stored in the unknown chunk list.  Known chunks listed to
 *    ci_set_keep_unknown_chunks will have no effect.  If it is set then known
 *    chunks listed with a keep other than AS_DEFAULT will *never* be processed
 *    by libci, in addition critical chunks must either be processed by the
 *    callback or saved.
 *
 *    The IHDR and IEND chunks must not be listed.  Because this turns off the
 *    default handling for chunks that would otherwise be recognized the
 *    behavior of libci transformations may well become incorrect!
 *
 * WRITE:
 *    When writing chunks the options only apply to the chunks specified by
 *    ci_set_unknown_chunks (below), libci will *always* write known chunks
 *    required by ci_set_ calls and will always write the core critical chunks
 *    (as required for PLTE).
 *
 *    Each chunk in the ci_set_unknown_chunks list is looked up in the
 *    ci_set_keep_unknown_chunks list to find the keep setting, this is then
 *    interpreted as follows:
 *
 *    CI_HANDLE_CHUNK_AS_DEFAULT:
 *       Write safe-to-copy chunks and write other chunks if the global
 *       default is set to _ALWAYS, otherwise don't write this chunk.
 *    CI_HANDLE_CHUNK_NEVER:
 *       Do not write the chunk.
 *    CI_HANDLE_CHUNK_IF_SAFE:
 *       Write the chunk if it is safe-to-copy, otherwise do not write it.
 *    CI_HANDLE_CHUNK_ALWAYS:
 *       Write the chunk.
 *
 * Note that the default behavior is effectively the opposite of the read case -
 * in read unknown chunks are not stored by default, in write they are written
 * by default.  Also the behavior of CI_HANDLE_CHUNK_IF_SAFE is very different
 * - on write the safe-to-copy bit is checked, on read the critical bit is
 * checked and on read if the chunk is critical an error will be raised.
 *
 * num_chunks:
 * ===========
 *    If num_chunks is positive, then the "keep" parameter specifies the manner
 *    for handling only those chunks appearing in the chunk_list array,
 *    otherwise the chunk list array is ignored.
 *
 *    If num_chunks is 0 the "keep" parameter specifies the default behavior for
 *    unknown chunks, as described above.
 *
 *    If num_chunks is negative, then the "keep" parameter specifies the manner
 *    for handling all unknown chunks plus all chunks recognized by libci
 *    except for the IHDR, PLTE, tRNS, IDAT, and IEND chunks (which continue to
 *    be processed by libci.
 */
#ifdef CI_HANDLE_AS_UNKNOWN_SUPPORTED
CI_EXPORT(172, void, ci_set_keep_unknown_chunks, (ci_structrp ci_ptr,
    int keep, ci_const_bytep chunk_list, int num_chunks));
#endif /* HANDLE_AS_UNKNOWN */

/* The "keep" CI_HANDLE_CHUNK_ parameter for the specified chunk is returned;
 * the result is therefore true (non-zero) if special handling is required,
 * false for the default handling.
 */
CI_EXPORT(173, int, ci_handle_as_unknown, (ci_const_structrp ci_ptr,
    ci_const_bytep chunk_name));
#endif /* SET_UNKNOWN_CHUNKS */

#ifdef CI_STORE_UNKNOWN_CHUNKS_SUPPORTED
CI_EXPORT(174, void, ci_set_unknown_chunks, (ci_const_structrp ci_ptr,
    ci_inforp info_ptr, ci_const_unknown_chunkp unknowns,
    int num_unknowns));
   /* NOTE: prior to 1.6.0 this routine set the 'location' field of the added
    * unknowns to the location currently stored in the ci_struct.  This is
    * invariably the wrong value on write.  To fix this call the following API
    * for each chunk in the list with the correct location.  If you know your
    * code won't be compiled on earlier versions you can rely on
    * ci_set_unknown_chunks(write-ptr, ci_get_unknown_chunks(read-ptr)) doing
    * the correct thing.
    */

CI_EXPORT(175, void, ci_set_unknown_chunk_location,
    (ci_const_structrp ci_ptr, ci_inforp info_ptr, int chunk, int location));

CI_EXPORT(176, int, ci_get_unknown_chunks, (ci_const_structrp ci_ptr,
    ci_inforp info_ptr, ci_unknown_chunkpp entries));
#endif

/* Ci_free_data() will turn off the "valid" flag for anything it frees.
 * If you need to turn it off for a chunk that your application has freed,
 * you can use ci_set_invalid(ci_ptr, info_ptr, CI_INFO_CHNK);
 */
CI_EXPORT(177, void, ci_set_invalid, (ci_const_structrp ci_ptr,
    ci_inforp info_ptr, int mask));

#ifdef CI_INFO_IMAGE_SUPPORTED
/* The "params" pointer is currently not used and is for future expansion. */
#ifdef CI_SEQUENTIAL_READ_SUPPORTED
CI_EXPORT(178, void, ci_read_ci, (ci_structrp ci_ptr, ci_inforp info_ptr,
    int transforms, ci_voidp params));
#endif
#ifdef CI_WRITE_SUPPORTED
CI_EXPORT(179, void, ci_write_ci, (ci_structrp ci_ptr, ci_inforp info_ptr,
    int transforms, ci_voidp params));
#endif
#endif

CI_EXPORT(180, ci_const_charp, ci_get_copyright,
    (ci_const_structrp ci_ptr));
CI_EXPORT(181, ci_const_charp, ci_get_header_ver,
    (ci_const_structrp ci_ptr));
CI_EXPORT(182, ci_const_charp, ci_get_header_version,
    (ci_const_structrp ci_ptr));
CI_EXPORT(183, ci_const_charp, ci_get_libci_ver,
    (ci_const_structrp ci_ptr));

#ifdef CI_MNG_FEATURES_SUPPORTED
CI_EXPORT(184, ci_uint_32, ci_permit_mng_features, (ci_structrp ci_ptr,
    ci_uint_32 mng_features_permitted));
#endif

/* For use in ci_set_keep_unknown, added to version 1.2.6 */
#define CI_HANDLE_CHUNK_AS_DEFAULT   0
#define CI_HANDLE_CHUNK_NEVER        1
#define CI_HANDLE_CHUNK_IF_SAFE      2
#define CI_HANDLE_CHUNK_ALWAYS       3
#define CI_HANDLE_CHUNK_LAST         4

/* Strip the prepended error numbers ("#nnn ") from error and warning
 * messages before passing them to the error or warning handler.
 */
#ifdef CI_ERROR_NUMBERS_SUPPORTED
CI_EXPORT(185, void, ci_set_strip_error_numbers, (ci_structrp ci_ptr,
    ci_uint_32 strip_mode));
#endif

/* Added in libci-1.2.6 */
#ifdef CI_SET_USER_LIMITS_SUPPORTED
CI_EXPORT(186, void, ci_set_user_limits, (ci_structrp ci_ptr,
    ci_uint_32 user_width_max, ci_uint_32 user_height_max));
CI_EXPORT(187, ci_uint_32, ci_get_user_width_max,
    (ci_const_structrp ci_ptr));
CI_EXPORT(188, ci_uint_32, ci_get_user_height_max,
    (ci_const_structrp ci_ptr));
/* Added in libci-1.4.0 */
CI_EXPORT(189, void, ci_set_chunk_cache_max, (ci_structrp ci_ptr,
    ci_uint_32 user_chunk_cache_max));
CI_EXPORT(190, ci_uint_32, ci_get_chunk_cache_max,
    (ci_const_structrp ci_ptr));
/* Added in libci-1.4.1 */
CI_EXPORT(191, void, ci_set_chunk_malloc_max, (ci_structrp ci_ptr,
    ci_alloc_size_t user_chunk_cache_max));
CI_EXPORT(192, ci_alloc_size_t, ci_get_chunk_malloc_max,
    (ci_const_structrp ci_ptr));
#endif

#if defined(CI_INCH_CONVERSIONS_SUPPORTED)
CI_EXPORT(193, ci_uint_32, ci_get_pixels_per_inch,
    (ci_const_structrp ci_ptr, ci_const_inforp info_ptr));

CI_EXPORT(194, ci_uint_32, ci_get_x_pixels_per_inch,
    (ci_const_structrp ci_ptr, ci_const_inforp info_ptr));

CI_EXPORT(195, ci_uint_32, ci_get_y_pixels_per_inch,
    (ci_const_structrp ci_ptr, ci_const_inforp info_ptr));

CI_FP_EXPORT(196, float, ci_get_x_offset_inches,
    (ci_const_structrp ci_ptr, ci_const_inforp info_ptr))
#ifdef CI_FIXED_POINT_SUPPORTED /* otherwise not implemented. */
CI_FIXED_EXPORT(211, ci_fixed_point, ci_get_x_offset_inches_fixed,
    (ci_const_structrp ci_ptr, ci_const_inforp info_ptr))
#endif

CI_FP_EXPORT(197, float, ci_get_y_offset_inches, (ci_const_structrp ci_ptr,
    ci_const_inforp info_ptr))
#ifdef CI_FIXED_POINT_SUPPORTED /* otherwise not implemented. */
CI_FIXED_EXPORT(212, ci_fixed_point, ci_get_y_offset_inches_fixed,
    (ci_const_structrp ci_ptr, ci_const_inforp info_ptr))
#endif

#  ifdef CI_pHYs_SUPPORTED
CI_EXPORT(198, ci_uint_32, ci_get_pHYs_dpi, (ci_const_structrp ci_ptr,
    ci_const_inforp info_ptr, ci_uint_32 *res_x, ci_uint_32 *res_y,
    int *unit_type));
#  endif /* pHYs */
#endif  /* INCH_CONVERSIONS */

/* Added in libci-1.4.0 */
#ifdef CI_IO_STATE_SUPPORTED
CI_EXPORT(199, ci_uint_32, ci_get_io_state, (ci_const_structrp ci_ptr));

/* Removed from libci 1.6; use ci_get_io_chunk_type. */
CI_REMOVED(200, ci_const_bytep, ci_get_io_chunk_name, (ci_structrp ci_ptr),
    CI_DEPRECATED)

CI_EXPORT(216, ci_uint_32, ci_get_io_chunk_type,
    (ci_const_structrp ci_ptr));

/* The flags returned by ci_get_io_state() are the following: */
#  define CI_IO_NONE        0x0000   /* no I/O at this moment */
#  define CI_IO_READING     0x0001   /* currently reading */
#  define CI_IO_WRITING     0x0002   /* currently writing */
#  define CI_IO_SIGNATURE   0x0010   /* currently at the file signature */
#  define CI_IO_CHUNK_HDR   0x0020   /* currently at the chunk header */
#  define CI_IO_CHUNK_DATA  0x0040   /* currently at the chunk data */
#  define CI_IO_CHUNK_CRC   0x0080   /* currently at the chunk crc */
#  define CI_IO_MASK_OP     0x000f   /* current operation: reading/writing */
#  define CI_IO_MASK_LOC    0x00f0   /* current location: sig/hdr/data/crc */
#endif /* IO_STATE */

/* Interlace support.  The following macros are always defined so that if
 * libci interlace handling is turned off the macros may be used to handle
 * interlaced images within the application.
 */
#define CI_INTERLACE_ADAM7_PASSES 7

/* Two macros to return the first row and first column of the original,
 * full, image which appears in a given pass.  'pass' is in the range 0
 * to 6 and the result is in the range 0 to 7.
 */
#define CI_PASS_START_ROW(pass) (((1&~(pass))<<(3-((pass)>>1)))&7)
#define CI_PASS_START_COL(pass) (((1& (pass))<<(3-(((pass)+1)>>1)))&7)

/* A macro to return the offset between pixels in the output row for a pair of
 * pixels in the input - effectively the inverse of the 'COL_SHIFT' macro that
 * follows.  Note that ROW_OFFSET is the offset from one row to the next whereas
 * COL_OFFSET is from one column to the next, within a row.
 */
#define CI_PASS_ROW_OFFSET(pass) ((pass)>2?(8>>(((pass)-1)>>1)):8)
#define CI_PASS_COL_OFFSET(pass) (1<<((7-(pass))>>1))

/* Two macros to help evaluate the number of rows or columns in each
 * pass.  This is expressed as a shift - effectively log2 of the number or
 * rows or columns in each 8x8 tile of the original image.
 */
#define CI_PASS_ROW_SHIFT(pass) ((pass)>2?(8-(pass))>>1:3)
#define CI_PASS_COL_SHIFT(pass) ((pass)>1?(7-(pass))>>1:3)

/* Hence two macros to determine the number of rows or columns in a given
 * pass of an image given its height or width.  In fact these macros may
 * return non-zero even though the sub-image is empty, because the other
 * dimension may be empty for a small image.
 */
#define CI_PASS_ROWS(height, pass) (((height)+(((1<<CI_PASS_ROW_SHIFT(pass))\
   -1)-CI_PASS_START_ROW(pass)))>>CI_PASS_ROW_SHIFT(pass))
#define CI_PASS_COLS(width, pass) (((width)+(((1<<CI_PASS_COL_SHIFT(pass))\
   -1)-CI_PASS_START_COL(pass)))>>CI_PASS_COL_SHIFT(pass))

/* For the reader row callbacks (both progressive and sequential) it is
 * necessary to find the row in the output image given a row in an interlaced
 * image, so two more macros:
 */
#define CI_ROW_FROM_PASS_ROW(y_in, pass) \
   (((y_in)<<CI_PASS_ROW_SHIFT(pass))+CI_PASS_START_ROW(pass))
#define CI_COL_FROM_PASS_COL(x_in, pass) \
   (((x_in)<<CI_PASS_COL_SHIFT(pass))+CI_PASS_START_COL(pass))

/* Two macros which return a boolean (0 or 1) saying whether the given row
 * or column is in a particular pass.  These use a common utility macro that
 * returns a mask for a given pass - the offset 'off' selects the row or
 * column version.  The mask has the appropriate bit set for each column in
 * the tile.
 */
#define CI_PASS_MASK(pass,off) ( \
   ((0x110145AF>>(((7-(off))-(pass))<<2)) & 0xF) | \
   ((0x01145AF0>>(((7-(off))-(pass))<<2)) & 0xF0))

#define CI_ROW_IN_INTERLACE_PASS(y, pass) \
   ((CI_PASS_MASK(pass,0) >> ((y)&7)) & 1)
#define CI_COL_IN_INTERLACE_PASS(x, pass) \
   ((CI_PASS_MASK(pass,1) >> ((x)&7)) & 1)

#ifdef CI_READ_COMPOSITE_NODIV_SUPPORTED
/* With these routines we avoid an integer divide, which will be slower on
 * most machines.  However, it does take more operations than the corresponding
 * divide method, so it may be slower on a few RISC systems.  There are two
 * shifts (by 8 or 16 bits) and an addition, versus a single integer divide.
 *
 * Note that the rounding factors are NOT supposed to be the same!  128 and
 * 32768 are correct for the NODIV code; 127 and 32767 are correct for the
 * standard method.
 *
 * [Optimized code by Greg Roelofs and Mark Adler...blame us for bugs. :-) ]
 */

 /* fg and bg should be in `gamma 1.0' space; alpha is the opacity */

#  define ci_composite(composite, fg, alpha, bg)        \
   {                                                     \
      ci_uint_16 temp = (ci_uint_16)((ci_uint_16)(fg) \
          * (ci_uint_16)(alpha)                         \
          + (ci_uint_16)(bg)*(ci_uint_16)(255          \
          - (ci_uint_16)(alpha)) + 128);                \
      (composite) = (ci_byte)(((temp + (temp >> 8)) >> 8) & 0xff); \
   }

#  define ci_composite_16(composite, fg, alpha, bg)     \
   {                                                     \
      ci_uint_32 temp = (ci_uint_32)((ci_uint_32)(fg) \
          * (ci_uint_32)(alpha)                         \
          + (ci_uint_32)(bg)*(65535                     \
          - (ci_uint_32)(alpha)) + 32768);              \
      (composite) = (ci_uint_16)(0xffff & ((temp + (temp >> 16)) >> 16)); \
   }

#else  /* Standard method using integer division */

#  define ci_composite(composite, fg, alpha, bg)                      \
   (composite) =                                                       \
       (ci_byte)(0xff & (((ci_uint_16)(fg) * (ci_uint_16)(alpha) +  \
       (ci_uint_16)(bg) * (ci_uint_16)(255 - (ci_uint_16)(alpha)) + \
       127) / 255))

#  define ci_composite_16(composite, fg, alpha, bg)                       \
   (composite) =                                                           \
       (ci_uint_16)(0xffff & (((ci_uint_32)(fg) * (ci_uint_32)(alpha) + \
       (ci_uint_32)(bg)*(ci_uint_32)(65535 - (ci_uint_32)(alpha)) +     \
       32767) / 65535))
#endif /* READ_COMPOSITE_NODIV */

#ifdef CI_READ_INT_FUNCTIONS_SUPPORTED
CI_EXPORT(201, ci_uint_32, ci_get_uint_32, (ci_const_bytep buf));
CI_EXPORT(202, ci_uint_16, ci_get_uint_16, (ci_const_bytep buf));
CI_EXPORT(203, ci_int_32, ci_get_int_32, (ci_const_bytep buf));
#endif

CI_EXPORT(204, ci_uint_32, ci_get_uint_31, (ci_const_structrp ci_ptr,
    ci_const_bytep buf));
/* No ci_get_int_16 -- may be added if there's a real need for it. */

/* Place a 32-bit number into a buffer in CI byte order (big-endian). */
#ifdef CI_WRITE_INT_FUNCTIONS_SUPPORTED
CI_EXPORT(205, void, ci_save_uint_32, (ci_bytep buf, ci_uint_32 i));
#endif
#ifdef CI_SAVE_INT_32_SUPPORTED
CI_EXPORT(206, void, ci_save_int_32, (ci_bytep buf, ci_int_32 i));
#endif

/* Place a 16-bit number into a buffer in CI byte order.
 * The parameter is declared unsigned int, not ci_uint_16,
 * just to avoid potential problems on pre-ANSI C compilers.
 */
#ifdef CI_WRITE_INT_FUNCTIONS_SUPPORTED
CI_EXPORT(207, void, ci_save_uint_16, (ci_bytep buf, unsigned int i));
/* No ci_save_int_16 -- may be added if there's a real need for it. */
#endif

#ifdef CI_USE_READ_MACROS
/* Inline macros to do direct reads of bytes from the input buffer.
 * The ci_get_int_32() routine assumes we are using two's complement
 * format for negative values, which is almost certainly true.
 */
#  define CI_get_uint_32(buf) \
   (((ci_uint_32)(*(buf)) << 24) + \
    ((ci_uint_32)(*((buf) + 1)) << 16) + \
    ((ci_uint_32)(*((buf) + 2)) << 8) + \
    ((ci_uint_32)(*((buf) + 3))))

   /* From libci-1.4.0 until 1.4.4, the ci_get_uint_16 macro (but not the
    * function) incorrectly returned a value of type ci_uint_32.
    */
#  define CI_get_uint_16(buf) \
   ((ci_uint_16) \
    (((unsigned int)(*(buf)) << 8) + \
    ((unsigned int)(*((buf) + 1)))))

#  define CI_get_int_32(buf) \
   ((ci_int_32)((*(buf) & 0x80) \
    ? -((ci_int_32)(((ci_get_uint_32(buf)^0xffffffffU)+1U)&0x7fffffffU)) \
    : (ci_int_32)ci_get_uint_32(buf)))

/* If CI_PREFIX is defined the same thing as below happens in cilibconf.h,
 * but defining a macro name prefixed with CI_PREFIX.
 */
#  ifndef CI_PREFIX
#    define ci_get_uint_32(buf) CI_get_uint_32(buf)
#    define ci_get_uint_16(buf) CI_get_uint_16(buf)
#    define ci_get_int_32(buf)  CI_get_int_32(buf)
#  endif
#else
#  ifdef CI_PREFIX
   /* No macros; revert to the (redefined) function */
#    define CI_get_uint_32 (ci_get_uint_32)
#    define CI_get_uint_16 (ci_get_uint_16)
#    define CI_get_int_32  (ci_get_int_32)
#  endif
#endif

#ifdef CI_CHECK_FOR_INVALID_INDEX_SUPPORTED
CI_EXPORT(242, void, ci_set_check_for_invalid_index,
    (ci_structrp ci_ptr, int allowed));
#  ifdef CI_GET_PALETTE_MAX_SUPPORTED
CI_EXPORT(243, int, ci_get_palette_max, (ci_const_structp ci_ptr,
    ci_const_infop info_ptr));
#  endif
#endif /* CHECK_FOR_INVALID_INDEX */

/*******************************************************************************
 * Section 5: SIMPLIFIED API
 *******************************************************************************
 *
 * Please read the documentation in libci-manual.txt (TODO: write said
 * documentation) if you don't understand what follows.
 *
 * The simplified API hides the details of both libci and the CI file format
 * itself.  It allows CI files to be read into a very limited number of
 * in-memory bitmap formats or to be written from the same formats.  If these
 * formats do not accommodate your needs then you can, and should, use the more
 * sophisticated APIs above - these support a wide variety of in-memory formats
 * and a wide variety of sophisticated transformations to those formats as well
 * as a wide variety of APIs to manipulate ancillary information.
 *
 * To read a CI file using the simplified API:
 *
 * 1) Declare a 'ci_image' structure (see below) on the stack, set the
 *    version field to CI_IMAGE_VERSION and the 'opaque' pointer to NULL
 *    (this is REQUIRED, your program may crash if you don't do it.)
 * 2) Call the appropriate ci_image_begin_read... function.
 * 3) Set the ci_image 'format' member to the required sample format.
 * 4) Allocate a buffer for the image and, if required, the color-map.
 * 5) Call ci_image_finish_read to read the image and, if required, the
 *    color-map into your buffers.
 *
 * There are no restrictions on the format of the CI input itself; all valid
 * color types, bit depths, and interlace methods are acceptable, and the
 * input image is transformed as necessary to the requested in-memory format
 * during the ci_image_finish_read() step.  The only caveat is that if you
 * request a color-mapped image from a CI that is full-color or makes
 * complex use of an alpha channel the transformation is extremely lossy and the
 * result may look terrible.
 *
 * To write a CI file using the simplified API:
 *
 * 1) Declare a 'ci_image' structure on the stack and memset() it to all zero.
 * 2) Initialize the members of the structure that describe the image, setting
 *    the 'format' member to the format of the image samples.
 * 3) Call the appropriate ci_image_write... function with a pointer to the
 *    image and, if necessary, the color-map to write the CI data.
 *
 * ci_image is a structure that describes the in-memory format of an image
 * when it is being read or defines the in-memory format of an image that you
 * need to write:
 */
#if defined(CI_SIMPLIFIED_READ_SUPPORTED) || \
    defined(CI_SIMPLIFIED_WRITE_SUPPORTED)

#define CI_IMAGE_VERSION 1

typedef struct ci_control *ci_controlp;
typedef struct
{
   ci_controlp opaque;    /* Initialize to NULL, free with ci_image_free */
   ci_uint_32  version;   /* Set to CI_IMAGE_VERSION */
   ci_uint_32  width;     /* Image width in pixels (columns) */
   ci_uint_32  height;    /* Image height in pixels (rows) */
   ci_uint_32  format;    /* Image format as defined below */
   ci_uint_32  flags;     /* A bit mask containing informational flags */
   ci_uint_32  colormap_entries;
                           /* Number of entries in the color-map */

   /* In the event of an error or warning the following field will be set to a
    * non-zero value and the 'message' field will contain a '\0' terminated
    * string with the libci error or warning message.  If both warnings and
    * an error were encountered, only the error is recorded.  If there
    * are multiple warnings, only the first one is recorded.
    *
    * The upper 30 bits of this value are reserved, the low two bits contain
    * a value as follows:
    */
#  define CI_IMAGE_WARNING 1
#  define CI_IMAGE_ERROR 2
   /*
    * The result is a two-bit code such that a value more than 1 indicates
    * a failure in the API just called:
    *
    *    0 - no warning or error
    *    1 - warning
    *    2 - error
    *    3 - error preceded by warning
    */
#  define CI_IMAGE_FAILED(ci_cntrl) ((((ci_cntrl).warning_or_error)&0x03)>1)

   ci_uint_32  warning_or_error;

   char         message[64];
} ci_image, *ci_imagep;

/* The samples of the image have one to four channels whose components have
 * original values in the range 0 to 1.0:
 *
 * 1: A single gray or luminance channel (G).
 * 2: A gray/luminance channel and an alpha channel (GA).
 * 3: Three red, green, blue color channels (RGB).
 * 4: Three color channels and an alpha channel (RGBA).
 *
 * The components are encoded in one of two ways:
 *
 * a) As a small integer, value 0..255, contained in a single byte.  For the
 * alpha channel the original value is simply value/255.  For the color or
 * luminance channels the value is encoded according to the sRGB specification
 * and matches the 8-bit format expected by typical display devices.
 *
 * The color/gray channels are not scaled (pre-multiplied) by the alpha
 * channel and are suitable for passing to color management software.
 *
 * b) As a value in the range 0..65535, contained in a 2-byte integer.  All
 * channels can be converted to the original value by dividing by 65535; all
 * channels are linear.  Color channels use the RGB encoding (RGB end-points) of
 * the sRGB specification.  This encoding is identified by the
 * CI_FORMAT_FLAG_LINEAR flag below.
 *
 * When the simplified API needs to convert between sRGB and linear colorspaces,
 * the actual sRGB transfer curve defined in the sRGB specification (see the
 * article at <https://en.wikipedia.org/wiki/SRGB>) is used, not the gamma=1/2.2
 * approximation used elsewhere in libci.
 *
 * When an alpha channel is present it is expected to denote pixel coverage
 * of the color or luminance channels and is returned as an associated alpha
 * channel: the color/gray channels are scaled (pre-multiplied) by the alpha
 * value.
 *
 * The samples are either contained directly in the image data, between 1 and 8
 * bytes per pixel according to the encoding, or are held in a color-map indexed
 * by bytes in the image data.  In the case of a color-map the color-map entries
 * are individual samples, encoded as above, and the image data has one byte per
 * pixel to select the relevant sample from the color-map.
 */

/* CI_FORMAT_*
 *
 * #defines to be used in ci_image::format.  Each #define identifies a
 * particular layout of sample data and, if present, alpha values.  There are
 * separate defines for each of the two component encodings.
 *
 * A format is built up using single bit flag values.  All combinations are
 * valid.  Formats can be built up from the flag values or you can use one of
 * the predefined values below.  When testing formats always use the FORMAT_FLAG
 * macros to test for individual features - future versions of the library may
 * add new flags.
 *
 * When reading or writing color-mapped images the format should be set to the
 * format of the entries in the color-map then ci_image_{read,write}_colormap
 * called to read or write the color-map and set the format correctly for the
 * image data.  Do not set the CI_FORMAT_FLAG_COLORMAP bit directly!
 *
 * NOTE: libci can be built with particular features disabled. If you see
 * compiler errors because the definition of one of the following flags has been
 * compiled out it is because libci does not have the required support.  It is
 * possible, however, for the libci configuration to enable the format on just
 * read or just write; in that case you may see an error at run time.  You can
 * guard against this by checking for the definition of the appropriate
 * "_SUPPORTED" macro, one of:
 *
 *    CI_SIMPLIFIED_{READ,WRITE}_{BGR,AFIRST}_SUPPORTED
 */
#define CI_FORMAT_FLAG_ALPHA    0x01U /* format with an alpha channel */
#define CI_FORMAT_FLAG_COLOR    0x02U /* color format: otherwise grayscale */
#define CI_FORMAT_FLAG_LINEAR   0x04U /* 2-byte channels else 1-byte */
#define CI_FORMAT_FLAG_COLORMAP 0x08U /* image data is color-mapped */

#ifdef CI_FORMAT_BGR_SUPPORTED
#  define CI_FORMAT_FLAG_BGR    0x10U /* BGR colors, else order is RGB */
#endif

#ifdef CI_FORMAT_AFIRST_SUPPORTED
#  define CI_FORMAT_FLAG_AFIRST 0x20U /* alpha channel comes first */
#endif

#define CI_FORMAT_FLAG_ASSOCIATED_ALPHA 0x40U /* alpha channel is associated */

/* Commonly used formats have predefined macros.
 *
 * First the single byte (sRGB) formats:
 */
#define CI_FORMAT_GRAY 0
#define CI_FORMAT_GA   CI_FORMAT_FLAG_ALPHA
#define CI_FORMAT_AG   (CI_FORMAT_GA|CI_FORMAT_FLAG_AFIRST)
#define CI_FORMAT_RGB  CI_FORMAT_FLAG_COLOR
#define CI_FORMAT_BGR  (CI_FORMAT_FLAG_COLOR|CI_FORMAT_FLAG_BGR)
#define CI_FORMAT_RGBA (CI_FORMAT_RGB|CI_FORMAT_FLAG_ALPHA)
#define CI_FORMAT_ARGB (CI_FORMAT_RGBA|CI_FORMAT_FLAG_AFIRST)
#define CI_FORMAT_BGRA (CI_FORMAT_BGR|CI_FORMAT_FLAG_ALPHA)
#define CI_FORMAT_ABGR (CI_FORMAT_BGRA|CI_FORMAT_FLAG_AFIRST)

/* Then the linear 2-byte formats.  When naming these "Y" is used to
 * indicate a luminance (gray) channel.
 */
#define CI_FORMAT_LINEAR_Y CI_FORMAT_FLAG_LINEAR
#define CI_FORMAT_LINEAR_Y_ALPHA (CI_FORMAT_FLAG_LINEAR|CI_FORMAT_FLAG_ALPHA)
#define CI_FORMAT_LINEAR_RGB (CI_FORMAT_FLAG_LINEAR|CI_FORMAT_FLAG_COLOR)
#define CI_FORMAT_LINEAR_RGB_ALPHA \
   (CI_FORMAT_FLAG_LINEAR|CI_FORMAT_FLAG_COLOR|CI_FORMAT_FLAG_ALPHA)

/* With color-mapped formats the image data is one byte for each pixel, the byte
 * is an index into the color-map which is formatted as above.  To obtain a
 * color-mapped format it is sufficient just to add the CI_FOMAT_FLAG_COLORMAP
 * to one of the above definitions, or you can use one of the definitions below.
 */
#define CI_FORMAT_RGB_COLORMAP  (CI_FORMAT_RGB|CI_FORMAT_FLAG_COLORMAP)
#define CI_FORMAT_BGR_COLORMAP  (CI_FORMAT_BGR|CI_FORMAT_FLAG_COLORMAP)
#define CI_FORMAT_RGBA_COLORMAP (CI_FORMAT_RGBA|CI_FORMAT_FLAG_COLORMAP)
#define CI_FORMAT_ARGB_COLORMAP (CI_FORMAT_ARGB|CI_FORMAT_FLAG_COLORMAP)
#define CI_FORMAT_BGRA_COLORMAP (CI_FORMAT_BGRA|CI_FORMAT_FLAG_COLORMAP)
#define CI_FORMAT_ABGR_COLORMAP (CI_FORMAT_ABGR|CI_FORMAT_FLAG_COLORMAP)

/* CI_IMAGE macros
 *
 * These are convenience macros to derive information from a ci_image
 * structure.  The CI_IMAGE_SAMPLE_ macros return values appropriate to the
 * actual image sample values - either the entries in the color-map or the
 * pixels in the image.  The CI_IMAGE_PIXEL_ macros return corresponding values
 * for the pixels and will always return 1 for color-mapped formats.  The
 * remaining macros return information about the rows in the image and the
 * complete image.
 *
 * NOTE: All the macros that take a ci_image::format parameter are compile time
 * constants if the format parameter is, itself, a constant.  Therefore these
 * macros can be used in array declarations and case labels where required.
 * Similarly the macros are also pre-processor constants (sizeof is not used) so
 * they can be used in #if tests.
 *
 * First the information about the samples.
 */
#define CI_IMAGE_SAMPLE_CHANNELS(fmt)\
   (((fmt)&(CI_FORMAT_FLAG_COLOR|CI_FORMAT_FLAG_ALPHA))+1)
   /* Return the total number of channels in a given format: 1..4 */

#define CI_IMAGE_SAMPLE_COMPONENT_SIZE(fmt)\
   ((((fmt) & CI_FORMAT_FLAG_LINEAR) >> 2)+1)
   /* Return the size in bytes of a single component of a pixel or color-map
    * entry (as appropriate) in the image: 1 or 2.
    */

#define CI_IMAGE_SAMPLE_SIZE(fmt)\
   (CI_IMAGE_SAMPLE_CHANNELS(fmt) * CI_IMAGE_SAMPLE_COMPONENT_SIZE(fmt))
   /* This is the size of the sample data for one sample.  If the image is
    * color-mapped it is the size of one color-map entry (and image pixels are
    * one byte in size), otherwise it is the size of one image pixel.
    */

#define CI_IMAGE_MAXIMUM_COLORMAP_COMPONENTS(fmt)\
   (CI_IMAGE_SAMPLE_CHANNELS(fmt) * 256)
   /* The maximum size of the color-map required by the format expressed in a
    * count of components.  This can be used to compile-time allocate a
    * color-map:
    *
    * ci_uint_16 colormap[CI_IMAGE_MAXIMUM_COLORMAP_COMPONENTS(linear_fmt)];
    *
    * ci_byte colormap[CI_IMAGE_MAXIMUM_COLORMAP_COMPONENTS(sRGB_fmt)];
    *
    * Alternatively use the CI_IMAGE_COLORMAP_SIZE macro below to use the
    * information from one of the ci_image_begin_read_ APIs and dynamically
    * allocate the required memory.
    */

/* Corresponding information about the pixels */
#define CI_IMAGE_PIXEL_(test,fmt)\
   (((fmt)&CI_FORMAT_FLAG_COLORMAP)?1:test(fmt))

#define CI_IMAGE_PIXEL_CHANNELS(fmt)\
   CI_IMAGE_PIXEL_(CI_IMAGE_SAMPLE_CHANNELS,fmt)
   /* The number of separate channels (components) in a pixel; 1 for a
    * color-mapped image.
    */

#define CI_IMAGE_PIXEL_COMPONENT_SIZE(fmt)\
   CI_IMAGE_PIXEL_(CI_IMAGE_SAMPLE_COMPONENT_SIZE,fmt)
   /* The size, in bytes, of each component in a pixel; 1 for a color-mapped
    * image.
    */

#define CI_IMAGE_PIXEL_SIZE(fmt) CI_IMAGE_PIXEL_(CI_IMAGE_SAMPLE_SIZE,fmt)
   /* The size, in bytes, of a complete pixel; 1 for a color-mapped image. */

/* Information about the whole row, or whole image */
#define CI_IMAGE_ROW_STRIDE(image)\
   (CI_IMAGE_PIXEL_CHANNELS((image).format) * (image).width)
   /* Return the total number of components in a single row of the image; this
    * is the minimum 'row stride', the minimum count of components between each
    * row.  For a color-mapped image this is the minimum number of bytes in a
    * row.
    *
    * WARNING: this macro overflows for some images with more than one component
    * and very large image widths.  libci will refuse to process an image where
    * this macro would overflow.
    */

#define CI_IMAGE_BUFFER_SIZE(image, row_stride)\
   (CI_IMAGE_PIXEL_COMPONENT_SIZE((image).format)*(image).height*(row_stride))
   /* Return the size, in bytes, of an image buffer given a ci_image and a row
    * stride - the number of components to leave space for in each row.
    *
    * WARNING: this macro overflows a 32-bit integer for some large CI images,
    * libci will refuse to process an image where such an overflow would occur.
    */

#define CI_IMAGE_SIZE(image)\
   CI_IMAGE_BUFFER_SIZE(image, CI_IMAGE_ROW_STRIDE(image))
   /* Return the size, in bytes, of the image in memory given just a ci_image;
    * the row stride is the minimum stride required for the image.
    */

#define CI_IMAGE_COLORMAP_SIZE(image)\
   (CI_IMAGE_SAMPLE_SIZE((image).format) * (image).colormap_entries)
   /* Return the size, in bytes, of the color-map of this image.  If the image
    * format is not a color-map format this will return a size sufficient for
    * 256 entries in the given format; check CI_FORMAT_FLAG_COLORMAP if
    * you don't want to allocate a color-map in this case.
    */

/* CI_IMAGE_FLAG_*
 *
 * Flags containing additional information about the image are held in the
 * 'flags' field of ci_image.
 */
#define CI_IMAGE_FLAG_COLORSPACE_NOT_sRGB 0x01
   /* This indicates that the RGB values of the in-memory bitmap do not
    * correspond to the red, green and blue end-points defined by sRGB.
    */

#define CI_IMAGE_FLAG_FAST 0x02
   /* On write emphasise speed over compression; the resultant CI file will be
    * larger but will be produced significantly faster, particular for large
    * images.  Do not use this option for images which will be distributed, only
    * used it when producing intermediate files that will be read back in
    * repeatedly.  For a typical 24-bit image the option will double the read
    * speed at the cost of increasing the image size by 25%, however for many
    * more compressible images the CI file can be 10 times larger with only a
    * slight speed gain.
    */

#define CI_IMAGE_FLAG_16BIT_sRGB 0x04
   /* On read if the image is a 16-bit per component image and there is no gAMA
    * or sRGB chunk assume that the components are sRGB encoded.  Notice that
    * images output by the simplified API always have gamma information; setting
    * this flag only affects the interpretation of 16-bit images from an
    * external source.  It is recommended that the application expose this flag
    * to the user; the user can normally easily recognize the difference between
    * linear and sRGB encoding.  This flag has no effect on write - the data
    * passed to the write APIs must have the correct encoding (as defined
    * above.)
    *
    * If the flag is not set (the default) input 16-bit per component data is
    * assumed to be linear.
    *
    * NOTE: the flag can only be set after the ci_image_begin_read_ call,
    * because that call initializes the 'flags' field.
    */

#ifdef CI_SIMPLIFIED_READ_SUPPORTED
/* READ APIs
 * ---------
 *
 * The ci_image passed to the read APIs must have been initialized by setting
 * the ci_controlp field 'opaque' to NULL (or, safer, memset the whole thing.)
 */
#ifdef CI_STDIO_SUPPORTED
CI_EXPORT(234, int, ci_image_begin_read_from_file, (ci_imagep image,
   const char *file_name));
   /* The named file is opened for read and the image header is filled in
    * from the CI header in the file.
    */

CI_EXPORT(235, int, ci_image_begin_read_from_stdio, (ci_imagep image,
   FILE *file));
   /* The CI header is read from the stdio FILE object. */
#endif /* STDIO */

CI_EXPORT(236, int, ci_image_begin_read_from_memory, (ci_imagep image,
   ci_const_voidp memory, size_t size));
   /* The CI header is read from the given memory buffer. */

CI_EXPORT(237, int, ci_image_finish_read, (ci_imagep image,
   ci_const_colorp background, void *buffer, ci_int_32 row_stride,
   void *colormap));
   /* Finish reading the image into the supplied buffer and clean up the
    * ci_image structure.
    *
    * row_stride is the step, in byte or 2-byte units as appropriate,
    * between adjacent rows.  A positive stride indicates that the top-most row
    * is first in the buffer - the normal top-down arrangement.  A negative
    * stride indicates that the bottom-most row is first in the buffer.
    *
    * background need only be supplied if an alpha channel must be removed from
    * a ci_byte format and the removal is to be done by compositing on a solid
    * color; otherwise it may be NULL and any composition will be done directly
    * onto the buffer.  The value is an sRGB color to use for the background,
    * for grayscale output the green channel is used.
    *
    * background must be supplied when an alpha channel must be removed from a
    * single byte color-mapped output format, in other words if:
    *
    * 1) The original format from ci_image_begin_read_from_* had
    *    CI_FORMAT_FLAG_ALPHA set.
    * 2) The format set by the application does not.
    * 3) The format set by the application has CI_FORMAT_FLAG_COLORMAP set and
    *    CI_FORMAT_FLAG_LINEAR *not* set.
    *
    * For linear output removing the alpha channel is always done by compositing
    * on black and background is ignored.
    *
    * colormap must be supplied when CI_FORMAT_FLAG_COLORMAP is set.  It must
    * be at least the size (in bytes) returned by CI_IMAGE_COLORMAP_SIZE.
    * image->colormap_entries will be updated to the actual number of entries
    * written to the colormap; this may be less than the original value.
    */

CI_EXPORT(238, void, ci_image_free, (ci_imagep image));
   /* Free any data allocated by libci in image->opaque, setting the pointer to
    * NULL.  May be called at any time after the structure is initialized.
    */
#endif /* SIMPLIFIED_READ */

#ifdef CI_SIMPLIFIED_WRITE_SUPPORTED
/* WRITE APIS
 * ----------
 * For write you must initialize a ci_image structure to describe the image to
 * be written.  To do this use memset to set the whole structure to 0 then
 * initialize fields describing your image.
 *
 * version: must be set to CI_IMAGE_VERSION
 * opaque: must be initialized to NULL
 * width: image width in pixels
 * height: image height in rows
 * format: the format of the data (image and color-map) you wish to write
 * flags: set to 0 unless one of the defined flags applies; set
 *    CI_IMAGE_FLAG_COLORSPACE_NOT_sRGB for color format images where the RGB
 *    values do not correspond to the colors in sRGB.
 * colormap_entries: set to the number of entries in the color-map (0 to 256)
 */
#ifdef CI_SIMPLIFIED_WRITE_STDIO_SUPPORTED
CI_EXPORT(239, int, ci_image_write_to_file, (ci_imagep image,
   const char *file, int convert_to_8bit, const void *buffer,
   ci_int_32 row_stride, const void *colormap));
   /* Write the image to the named file. */

CI_EXPORT(240, int, ci_image_write_to_stdio, (ci_imagep image, FILE *file,
   int convert_to_8_bit, const void *buffer, ci_int_32 row_stride,
   const void *colormap));
   /* Write the image to the given FILE object. */
#endif /* SIMPLIFIED_WRITE_STDIO */

/* With all write APIs if image is in one of the linear formats with 16-bit
 * data then setting convert_to_8_bit will cause the output to be an 8-bit CI
 * gamma encoded according to the sRGB specification, otherwise a 16-bit linear
 * encoded CI file is written.
 *
 * With color-mapped data formats the colormap parameter point to a color-map
 * with at least image->colormap_entries encoded in the specified format.  If
 * the format is linear the written CI color-map will be converted to sRGB
 * regardless of the convert_to_8_bit flag.
 *
 * With all APIs row_stride is handled as in the read APIs - it is the spacing
 * from one row to the next in component sized units (1 or 2 bytes) and if
 * negative indicates a bottom-up row layout in the buffer.  If row_stride is
 * zero, libci will calculate it for you from the image width and number of
 * channels.
 *
 * Note that the write API does not support interlacing, sub-8-bit pixels or
 * most ancillary chunks.  If you need to write text chunks (e.g. for copyright
 * notices) you need to use one of the other APIs.
 */

CI_EXPORT(245, int, ci_image_write_to_memory, (ci_imagep image, void *memory,
   ci_alloc_size_t * CI_RESTRICT memory_bytes, int convert_to_8_bit,
   const void *buffer, ci_int_32 row_stride, const void *colormap));
   /* Write the image to the given memory buffer.  The function both writes the
    * whole CI data stream to *memory and updates *memory_bytes with the count
    * of bytes written.
    *
    * 'memory' may be NULL.  In this case *memory_bytes is not read however on
    * success the number of bytes which would have been written will still be
    * stored in *memory_bytes.  On failure *memory_bytes will contain 0.
    *
    * If 'memory' is not NULL it must point to memory[*memory_bytes] of
    * writeable memory.
    *
    * If the function returns success memory[*memory_bytes] (if 'memory' is not
    * NULL) contains the written CI data.  *memory_bytes will always be less
    * than or equal to the original value.
    *
    * If the function returns false and *memory_bytes was not changed an error
    * occurred during write.  If *memory_bytes was changed, or is not 0 if
    * 'memory' was NULL, the write would have succeeded but for the memory
    * buffer being too small.  *memory_bytes contains the required number of
    * bytes and will be bigger that the original value.
    */

#define ci_image_write_get_memory_size(image, size, convert_to_8_bit, buffer,\
   row_stride, colormap)\
   ci_image_write_to_memory(&(image), 0, &(size), convert_to_8_bit, buffer,\
         row_stride, colormap)
   /* Return the amount of memory in 'size' required to compress this image.
    * The ci_image structure 'image' must be filled in as in the above
    * function and must not be changed before the actual write call, the buffer
    * and all other parameters must also be identical to that in the final
    * write call.  The 'size' variable need not be initialized.
    *
    * NOTE: the macro returns true/false, if false is returned 'size' will be
    * set to zero and the write failed and probably will fail if tried again.
    */

/* You can pre-allocate the buffer by making sure it is of sufficient size
 * regardless of the amount of compression achieved.  The buffer size will
 * always be bigger than the original image and it will never be filled.  The
 * following macros are provided to assist in allocating the buffer.
 */
#define CI_IMAGE_DATA_SIZE(image) (CI_IMAGE_SIZE(image)+(image).height)
   /* The number of uncompressed bytes in the CI byte encoding of the image;
    * uncompressing the CI IDAT data will give this number of bytes.
    *
    * NOTE: while CI_IMAGE_SIZE cannot overflow for an image in memory this
    * macro can because of the extra bytes used in the CI byte encoding.  You
    * need to avoid this macro if your image size approaches 2^30 in width or
    * height.  The same goes for the remainder of these macros; they all produce
    * bigger numbers than the actual in-memory image size.
    */
#ifndef CI_ZLIB_MAX_SIZE
#  define CI_ZLIB_MAX_SIZE(b) ((b)+(((b)+7U)>>3)+(((b)+63U)>>6)+11U)
   /* An upper bound on the number of compressed bytes given 'b' uncompressed
    * bytes.  This is based on deflateBounds() in zlib; different
    * implementations of zlib compression may conceivably produce more data so
    * if your zlib implementation is not zlib itself redefine this macro
    * appropriately.
    */
#endif

#define CI_IMAGE_COMPRESSED_SIZE_MAX(image)\
   CI_ZLIB_MAX_SIZE((ci_alloc_size_t)CI_IMAGE_DATA_SIZE(image))
   /* An upper bound on the size of the data in the CI IDAT chunks. */

#define CI_IMAGE_CI_SIZE_MAX_(image, image_size)\
   ((8U/*sig*/+25U/*IHDR*/+16U/*gAMA*/+44U/*cHRM*/+12U/*IEND*/+\
    (((image).format&CI_FORMAT_FLAG_COLORMAP)?/*colormap: PLTE, tRNS*/\
    12U+3U*(image).colormap_entries/*PLTE data*/+\
    (((image).format&CI_FORMAT_FLAG_ALPHA)?\
    12U/*tRNS*/+(image).colormap_entries:0U):0U)+\
    12U)+(12U*((image_size)/CI_ZBUF_SIZE))/*IDAT*/+(image_size))
   /* A helper for the following macro; if your compiler cannot handle the
    * following macro use this one with the result of
    * CI_IMAGE_COMPRESSED_SIZE_MAX(image) as the second argument (most
    * compilers should handle this just fine.)
    */

#define CI_IMAGE_CI_SIZE_MAX(image)\
   CI_IMAGE_CI_SIZE_MAX_(image, CI_IMAGE_COMPRESSED_SIZE_MAX(image))
   /* An upper bound on the total length of the CI data stream for 'image'.
    * The result is of type ci_alloc_size_t, on 32-bit systems this may
    * overflow even though CI_IMAGE_DATA_SIZE does not overflow; the write will
    * run out of buffer space but return a corrected size which should work.
    */
#endif /* SIMPLIFIED_WRITE */
/*******************************************************************************
 *  END OF SIMPLIFIED API
 ******************************************************************************/
#endif /* SIMPLIFIED_{READ|WRITE} */

/*******************************************************************************
 * Section 6: IMPLEMENTATION OPTIONS
 *******************************************************************************
 *
 * Support for arbitrary implementation-specific optimizations.  The API allows
 * particular options to be turned on or off.  'Option' is the number of the
 * option and 'onoff' is 0 (off) or non-0 (on).  The value returned is given
 * by the CI_OPTION_ defines below.
 *
 * HARDWARE: normally hardware capabilities, such as the Intel SSE instructions,
 *           are detected at run time, however sometimes it may be impossible
 *           to do this in user mode, in which case it is necessary to discover
 *           the capabilities in an OS specific way.  Such capabilities are
 *           listed here when libci has support for them and must be turned
 *           ON by the application if present.
 *
 * SOFTWARE: sometimes software optimizations actually result in performance
 *           decrease on some architectures or systems, or with some sets of
 *           CI images.  'Software' options allow such optimizations to be
 *           selected at run time.
 */
#ifdef CI_SET_OPTION_SUPPORTED

/* HARDWARE: ARM Neon SIMD instructions supported */
#ifdef CI_ARM_NEON_API_SUPPORTED
#  define CI_ARM_NEON 0
#endif

/* SOFTWARE: Force maximum window */
#define CI_MAXIMUM_INFLATE_WINDOW 2

/* SOFTWARE: Check ICC profile for sRGB */
#define CI_SKIP_sRGB_CHECK_PROFILE 4

/* HARDWARE: MIPS MSA SIMD instructions supported */
#ifdef CI_MIPS_MSA_API_SUPPORTED
#  define CI_MIPS_MSA 6
#endif

/* SOFTWARE: Disable Adler32 check on IDAT */
#ifdef CI_DISABLE_ADLER32_CHECK_SUPPORTED
#  define CI_IGNORE_ADLER32 8
#endif

/* HARDWARE: PowerPC VSX SIMD instructions supported */
#ifdef CI_POWERPC_VSX_API_SUPPORTED
#  define CI_POWERPC_VSX 10
#endif

/* HARDWARE: MIPS MMI SIMD instructions supported */
#ifdef CI_MIPS_MMI_API_SUPPORTED
#  define CI_MIPS_MMI 12
#endif

/* HARDWARE: RISC-V RVV SIMD instructions supported */
#ifdef CI_RISCV_RVV_API_SUPPORTED
#  define CI_RISCV_RVV 14
#endif

/* Next option - numbers must be even */
#define CI_OPTION_NEXT 16

/* Return values: NOTE: there are four values and 'off' is *not* zero */
#define CI_OPTION_UNSET   0 /* Unset - defaults to off */
#define CI_OPTION_INVALID 1 /* Option number out of range */
#define CI_OPTION_OFF     2
#define CI_OPTION_ON      3

CI_EXPORT(244, int, ci_set_option, (ci_structrp ci_ptr, int option,
   int onoff));
#endif /* SET_OPTION */

/*******************************************************************************
 *  END OF HARDWARE AND SOFTWARE OPTIONS
 ******************************************************************************/

/* Maintainer: Put new public prototypes here ^, in libci.3, in project
 * defs, and in scripts/symbols.def.
 */

/* The last ordinal number (this is the *last* one already used; the next
 * one to use is one more than this.)
 */
#ifdef CI_EXPORT_LAST_ORDINAL
  CI_EXPORT_LAST_ORDINAL(259);
#endif

#ifdef __cplusplus
}
#endif

#endif /* CI_VERSION_INFO_ONLY */
/* Do not put anything past this line */
#endif /* CI_H */

/* ciunknown.c - test the read side unknown chunk handling
 *
 * Copyright (c) 2021 Cosmin Truta
 * Copyright (c) 2015,2017 Glenn Randers-Pehrson
 * Written by John Cunningham Bowler
 *
 * This code is released under the libci license.
 * For conditions of distribution and use, see the disclaimer
 * and license in ci.h
 *
 * NOTES:
 *   This is a C program that is intended to be linked against libci.  It
 *   allows the libci unknown handling code to be tested by interpreting
 *   arguments to save or discard combinations of chunks.  The program is
 *   currently just a minimal validation for the built-in libci facilities.
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <setjmp.h>

/* Define the following to use this test against your installed libci, rather
 * than the one being built here:
 */
#ifdef CI_FREESTANDING_TESTS
#  include <ci.h>
#else
#  include "../../ci.h"
#endif

/* 1.6.1 added support for the configure test harness, which uses 77 to indicate
 * a skipped test, in earlier versions we need to succeed on a skipped test, so:
 */
#if CI_LIBCI_VER >= 10601 && defined(HAVE_CONFIG_H)
#  define SKIP 77
#else
#  define SKIP 0
#endif


/* Since this program tests the ability to change the unknown chunk handling
 * these must be defined:
 */
#if defined(CI_SET_UNKNOWN_CHUNKS_SUPPORTED) &&\
   defined(CI_STDIO_SUPPORTED) &&\
   defined(CI_READ_SUPPORTED)

/* One of these must be defined to allow us to find out what happened.  It is
 * still useful to set unknown chunk handling without either of these in order
 * to cause *known* chunks to be discarded.  This can be a significant
 * efficiency gain, but it can't really be tested here.
 */
#if defined(CI_READ_USER_CHUNKS_SUPPORTED) ||\
   defined(CI_SAVE_UNKNOWN_CHUNKS_SUPPORTED)

#if CI_LIBCI_VER < 10500
/* This deliberately lacks the const. */
typedef ci_byte *ci_const_bytep;

/* This is copied from 1.5.1 ci.h: */
#define CI_INTERLACE_ADAM7_PASSES 7
#define CI_PASS_START_ROW(pass) (((1U&~(pass))<<(3-((pass)>>1)))&7)
#define CI_PASS_START_COL(pass) (((1U& (pass))<<(3-(((pass)+1)>>1)))&7)
#define CI_PASS_ROW_SHIFT(pass) ((pass)>2?(8-(pass))>>1:3)
#define CI_PASS_COL_SHIFT(pass) ((pass)>1?(7-(pass))>>1:3)
#define CI_PASS_ROWS(height, pass) (((height)+(((1<<CI_PASS_ROW_SHIFT(pass))\
   -1)-CI_PASS_START_ROW(pass)))>>CI_PASS_ROW_SHIFT(pass))
#define CI_PASS_COLS(width, pass) (((width)+(((1<<CI_PASS_COL_SHIFT(pass))\
   -1)-CI_PASS_START_COL(pass)))>>CI_PASS_COL_SHIFT(pass))
#define CI_ROW_FROM_PASS_ROW(yIn, pass) \
   (((yIn)<<CI_PASS_ROW_SHIFT(pass))+CI_PASS_START_ROW(pass))
#define CI_COL_FROM_PASS_COL(xIn, pass) \
   (((xIn)<<CI_PASS_COL_SHIFT(pass))+CI_PASS_START_COL(pass))
#define CI_PASS_MASK(pass,off) ( \
   ((0x110145AFU>>(((7-(off))-(pass))<<2)) & 0xFU) | \
   ((0x01145AF0U>>(((7-(off))-(pass))<<2)) & 0xF0U))
#define CI_ROW_IN_INTERLACE_PASS(y, pass) \
   ((CI_PASS_MASK(pass,0) >> ((y)&7)) & 1)
#define CI_COL_IN_INTERLACE_PASS(x, pass) \
   ((CI_PASS_MASK(pass,1) >> ((x)&7)) & 1)

/* These are needed too for the default build: */
#define CI_WRITE_16BIT_SUPPORTED
#define CI_READ_16BIT_SUPPORTED

/* This comes from cilibconf.h after 1.5: */
#define CI_FP_1 100000
#define CI_GAMMA_THRESHOLD_FIXED\
   ((ci_fixed_point)(CI_GAMMA_THRESHOLD * CI_FP_1))
#endif

#if CI_LIBCI_VER < 10600
   /* 1.6.0 constifies many APIs. The following exists to allow civalid to be
    * compiled against earlier versions.
    */
#  define ci_const_structp ci_structp
#endif

#if CI_LIBCI_VER < 10700
   /* Copied from libci 1.7.0 ci.h */
#define CI_u2(b1, b2) (((unsigned int)(b1) << 8) + (b2))

#define CI_U16(b1, b2) ((ci_uint_16)CI_u2(b1, b2))
#define CI_U32(b1, b2, b3, b4)\
   (((ci_uint_32)CI_u2(b1, b2) << 16) + CI_u2(b3, b4))

/* Constants for known chunk types.
 */
#define ci_IDAT CI_U32( 73,  68,  65,  84)
#define ci_IEND CI_U32( 73,  69,  78,  68)
#define ci_IHDR CI_U32( 73,  72,  68,  82)
#define ci_PLTE CI_U32( 80,  76,  84,  69)
#define ci_bKGD CI_U32( 98,  75,  71,  68)
#define ci_cHRM CI_U32( 99,  72,  82,  77)
#define ci_cICP CI_U32( 99,  73,  67,  80) /* CIv3 */
#define ci_cLLI CI_U32( 99,  76,  76,  73) /* CIv3 */
#define ci_eXIf CI_U32(101,  88,  73, 102) /* registered July 2017 */
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

/* Test on flag values as defined in the spec (section 5.4): */
#define CI_CHUNK_ANCILLARY(c)    (1 & ((c) >> 29))
#define CI_CHUNK_CRITICAL(c)     (!CI_CHUNK_ANCILLARY(c))
#define CI_CHUNK_PRIVATE(c)      (1 & ((c) >> 21))
#define CI_CHUNK_RESERVED(c)     (1 & ((c) >> 13))
#define CI_CHUNK_SAFE_TO_COPY(c) (1 & ((c) >>  5))

#endif /* CI_LIBCI_VER < 10700 */

#ifdef __cplusplus
#  define this not_the_cpp_this
#  define new not_the_cpp_new
#  define voidcast(type, value) static_cast<type>(value)
#else
#  define voidcast(type, value) (value)
#endif /* __cplusplus */

/* Unused formal parameter errors are removed using the following macro which is
 * expected to have no bad effects on performance.
 */
#ifndef UNUSED
#  if defined(__GNUC__) || defined(_MSC_VER)
#     define UNUSED(param) (void)param;
#  else
#     define UNUSED(param)
#  endif
#endif

/* Types of chunks not known to libci */
#define ci_vpAg CI_U32(118, 112, 65, 103)

/* Chunk information */
#define CI_INFO_tEXt 0x10000000U
#define CI_INFO_iTXt 0x20000000U
#define CI_INFO_zTXt 0x40000000U

#define CI_INFO_sTER 0x01000000U
#define CI_INFO_vpAg 0x02000000U

#define ABSENT  0
#define START   1
#define END     2

static struct
{
   char        name[5];
   ci_uint_32 flag;
   ci_uint_32 tag;
   int         unknown;    /* Chunk not known to libci */
   int         all;        /* Chunk set by the '-1' option */
   int         position;   /* position in citest.ci */
   int         keep;       /* unknown handling setting */
} chunk_info[] = {
   /* Critical chunks */
   { "IDAT", CI_INFO_IDAT, ci_IDAT, 0, 0,  START, 0 }, /* must be [0] */
   { "PLTE", CI_INFO_PLTE, ci_PLTE, 0, 0, ABSENT, 0 },

   /* Non-critical chunks that libci handles */
   /* This is a mess but it seems to be the only way to do it - there is no way
    * to check for a definition outside a #if.
    */
   { "bKGD", CI_INFO_bKGD, ci_bKGD,
#     ifdef CI_READ_bKGD_SUPPORTED
         0,
#     else
         1,
#     endif
      1,  START, 0 },
   { "cHRM", CI_INFO_cHRM, ci_cHRM,
#     ifdef CI_READ_cHRM_SUPPORTED
         0,
#     else
         1,
#     endif
      1,  START, 0 },
   { "cICP", CI_INFO_cICP, ci_cICP,
#     ifdef CI_READ_cICP_SUPPORTED
         0,
#     else
         1,
#     endif
      1,  START, 0 },
   { "cLLI", CI_INFO_cLLI, ci_cLLI,
#     ifdef CI_READ_cLLI_SUPPORTED
         0,
#     else
         1,
#     endif
      1,  START, 0 },
   { "eXIf", CI_INFO_eXIf, ci_eXIf,
#     ifdef CI_READ_eXIf_SUPPORTED
         0,
#     else
         1,
#     endif
      1,  END, 0 },
   { "gAMA", CI_INFO_gAMA, ci_gAMA,
#     ifdef CI_READ_gAMA_SUPPORTED
         0,
#     else
         1,
#     endif
      1,  START, 0 },
   { "hIST", CI_INFO_hIST, ci_hIST,
#     ifdef CI_READ_hIST_SUPPORTED
         0,
#     else
         1,
#     endif
      1, ABSENT, 0 },
   { "iCCP", CI_INFO_iCCP, ci_iCCP,
#     ifdef CI_READ_iCCP_SUPPORTED
         0,
#     else
         1,
#     endif
      1, ABSENT, 0 },
   { "iTXt", CI_INFO_iTXt, ci_iTXt,
#     ifdef CI_READ_iTXt_SUPPORTED
         0,
#     else
         1,
#     endif
      1, ABSENT, 0 },
   { "mDCV", CI_INFO_mDCV, ci_mDCV,
#     ifdef CI_READ_mDCV_SUPPORTED
         0,
#     else
         1,
#     endif
      1,  START, 0 },
   { "oFFs", CI_INFO_oFFs, ci_oFFs,
#     ifdef CI_READ_oFFs_SUPPORTED
         0,
#     else
         1,
#     endif
      1,  START, 0 },
   { "pCAL", CI_INFO_pCAL, ci_pCAL,
#     ifdef CI_READ_pCAL_SUPPORTED
         0,
#     else
         1,
#     endif
      1,  START, 0 },
   { "pHYs", CI_INFO_pHYs, ci_pHYs,
#     ifdef CI_READ_pHYs_SUPPORTED
         0,
#     else
         1,
#     endif
      1,  START, 0 },
   { "sBIT", CI_INFO_sBIT, ci_sBIT,
#     ifdef CI_READ_sBIT_SUPPORTED
         0,
#     else
         1,
#     endif
      1,  START, 0 },
   { "sCAL", CI_INFO_sCAL, ci_sCAL,
#     ifdef CI_READ_sCAL_SUPPORTED
         0,
#     else
         1,
#     endif
      1,  START, 0 },
   { "sPLT", CI_INFO_sPLT, ci_sPLT,
#     ifdef CI_READ_sPLT_SUPPORTED
         0,
#     else
         1,
#     endif
      1, ABSENT, 0 },
   { "sRGB", CI_INFO_sRGB, ci_sRGB,
#     ifdef CI_READ_sRGB_SUPPORTED
         0,
#     else
         1,
#     endif
      1,  START, 0 },
   { "tEXt", CI_INFO_tEXt, ci_tEXt,
#     ifdef CI_READ_tEXt_SUPPORTED
         0,
#     else
         1,
#     endif
      1,  START, 0 },
   { "tIME", CI_INFO_tIME, ci_tIME,
#     ifdef CI_READ_tIME_SUPPORTED
         0,
#     else
         1,
#     endif
      1,  START, 0 },
   { "tRNS", CI_INFO_tRNS, ci_tRNS,
#     ifdef CI_READ_tRNS_SUPPORTED
         0,
#     else
         1,
#     endif
      0, ABSENT, 0 },
   { "zTXt", CI_INFO_zTXt, ci_zTXt,
#     ifdef CI_READ_zTXt_SUPPORTED
         0,
#     else
         1,
#     endif
      1,    END, 0 },

   /* No libci handling */
   { "sTER", CI_INFO_sTER, ci_sTER, 1, 1,  START, 0 },
   { "vpAg", CI_INFO_vpAg, ci_vpAg, 1, 0,  START, 0 },
};

#define NINFO ((int)((sizeof chunk_info)/(sizeof chunk_info[0])))

static void
clear_keep(void)
{
   int i = NINFO;
   while (--i >= 0)
      chunk_info[i].keep = 0;
}

static int
find(const char *name)
{
   int i = NINFO;
   while (--i >= 0)
   {
      if (memcmp(chunk_info[i].name, name, 4) == 0)
         break;
   }

   return i;
}

static int
findb(const ci_byte *name)
{
   int i = NINFO;
   while (--i >= 0)
   {
      if (memcmp(chunk_info[i].name, name, 4) == 0)
         break;
   }

   return i;
}

static int
find_by_flag(ci_uint_32 flag)
{
   int i = NINFO;

   while (--i >= 0)
      if (chunk_info[i].flag == flag)
         return i;

   fprintf(stderr, "ciunknown: internal error\n");
   exit(4);
}

static int
ancillary(const char *name)
{
   return CI_CHUNK_ANCILLARY(CI_U32(name[0], name[1], name[2], name[3]));
}

#ifdef CI_SAVE_UNKNOWN_CHUNKS_SUPPORTED
static int
ancillaryb(const ci_byte *name)
{
   return CI_CHUNK_ANCILLARY(CI_U32(name[0], name[1], name[2], name[3]));
}
#endif

/* Type of an error_ptr */
typedef struct
{
   jmp_buf     error_return;
   ci_structp ci_ptr;
   ci_infop   info_ptr, end_ptr;
   ci_uint_32 before_IDAT;
   ci_uint_32 after_IDAT;
   int         error_count;
   int         warning_count;
   int         keep; /* the default value */
   const char *program;
   const char *file;
   const char *test;
} display;

static const char init[] = "initialization";
static const char cmd[] = "command line";

static void
init_display(display *d, const char *program)
{
   memset(d, 0, sizeof *d);
   d->ci_ptr = NULL;
   d->info_ptr = d->end_ptr = NULL;
   d->error_count = d->warning_count = 0;
   d->program = program;
   d->file = program;
   d->test = init;
}

static void
clean_display(display *d)
{
   ci_destroy_read_struct(&d->ci_ptr, &d->info_ptr, &d->end_ptr);

   /* This must not happen - it might cause an app crash */
   if (d->ci_ptr != NULL || d->info_ptr != NULL || d->end_ptr != NULL)
   {
      fprintf(stderr, "%s(%s): ci_destroy_read_struct error\n", d->file,
         d->test);
      exit(1);
   }
}

CI_FUNCTION(void, display_exit, (display *d), static CI_NORETURN)
{
   ++(d->error_count);

   if (d->ci_ptr != NULL)
      clean_display(d);

   /* During initialization and if this is a single command line argument set
    * exit now - there is only one test, otherwise longjmp to do the next test.
    */
   if (d->test == init || d->test == cmd)
      exit(1);

   longjmp(d->error_return, 1);
}

static int
display_rc(const display *d, int strict)
{
   return d->error_count + (strict ? d->warning_count : 0);
}

/* libci error and warning callbacks */
CI_FUNCTION(void, (CICBAPI error), (ci_structp ci_ptr, const char *message),
   static CI_NORETURN)
{
   display *d = (display*)ci_get_error_ptr(ci_ptr);

   fprintf(stderr, "%s(%s): libci error: %s\n", d->file, d->test, message);
   display_exit(d);
}

static void CICBAPI
warning(ci_structp ci_ptr, const char *message)
{
   display *d = (display*)ci_get_error_ptr(ci_ptr);

   fprintf(stderr, "%s(%s): libci warning: %s\n", d->file, d->test, message);
   ++(d->warning_count);
}

static ci_uint_32
get_valid(display *d, ci_infop info_ptr)
{
   ci_uint_32 flags = ci_get_valid(d->ci_ptr, info_ptr, (ci_uint_32)~0);

   /* Map the text chunks back into the flags */
   {
      ci_textp text;
      ci_uint_32 ntext = ci_get_text(d->ci_ptr, info_ptr, &text, NULL);

      while (ntext > 0) switch (text[--ntext].compression)
      {
         case -1:
            flags |= CI_INFO_tEXt;
            break;
         case 0:
            flags |= CI_INFO_zTXt;
            break;
         case 1:
         case 2:
            flags |= CI_INFO_iTXt;
            break;
         default:
            fprintf(stderr, "%s(%s): unknown text compression %d\n", d->file,
               d->test, text[ntext].compression);
            display_exit(d);
      }
   }

   return flags;
}

#ifdef CI_READ_USER_CHUNKS_SUPPORTED
static int CICBAPI
read_callback(ci_structp pp, ci_unknown_chunkp pc)
{
   /* This function mimics the behavior of ci_set_keep_unknown_chunks by
    * returning '0' to keep the chunk and '1' to discard it.
    */
   display *d = voidcast(display*, ci_get_user_chunk_ptr(pp));
   int chunk = findb(pc->name);
   int keep, discard;

   if (chunk < 0) /* not one in our list, so not a known chunk */
      keep = d->keep;

   else
   {
      keep = chunk_info[chunk].keep;
      if (keep == CI_HANDLE_CHUNK_AS_DEFAULT)
      {
         /* See the comments in ci.h - use the default for unknown chunks,
          * do not keep known chunks.
          */
         if (chunk_info[chunk].unknown)
            keep = d->keep;

         else
            keep = CI_HANDLE_CHUNK_NEVER;
      }
   }

   switch (keep)
   {
      default:
         fprintf(stderr, "%s(%s): %d: unrecognized chunk option\n", d->file,
            d->test, chunk_info[chunk].keep);
         display_exit(d);

      case CI_HANDLE_CHUNK_AS_DEFAULT:
      case CI_HANDLE_CHUNK_NEVER:
         discard = 1; /*handled; discard*/
         break;

      case CI_HANDLE_CHUNK_IF_SAFE:
      case CI_HANDLE_CHUNK_ALWAYS:
         discard = 0; /*not handled; keep*/
         break;
   }

   /* Also store information about this chunk in the display, the relevant flag
    * is set if the chunk is to be kept ('not handled'.)
    */
   if (chunk >= 0)
   {
      if (!discard) /* stupidity to stop a GCC warning */
      {
         ci_uint_32 flag = chunk_info[chunk].flag;

         if (pc->location & CI_AFTER_IDAT)
            d->after_IDAT |= flag;

         else
            d->before_IDAT |= flag;
      }
   }

   /* However if there is no support to store unknown chunks don't ask libci to
    * do it; there will be an ci_error.
    */
#  ifdef CI_SAVE_UNKNOWN_CHUNKS_SUPPORTED
      return discard;
#  else
      return 1; /*handled; discard*/
#  endif
}
#endif /* READ_USER_CHUNKS_SUPPORTED */

#ifdef CI_SAVE_UNKNOWN_CHUNKS_SUPPORTED
static ci_uint_32
get_unknown(display *d, ci_infop info_ptr, int after_IDAT)
{
   /* Create corresponding 'unknown' flags */
   ci_uint_32 flags = 0;

   UNUSED(after_IDAT)

   {
      ci_unknown_chunkp unknown;
      int num_unknown = ci_get_unknown_chunks(d->ci_ptr, info_ptr, &unknown);

      while (--num_unknown >= 0)
      {
         int chunk = findb(unknown[num_unknown].name);

         /* Chunks not known to ciunknown must be validated here; since they
          * must also be unknown to libci the 'display->keep' behavior should
          * have been used.
          */
         if (chunk < 0) switch (d->keep)
         {
            default: /* impossible */
            case CI_HANDLE_CHUNK_AS_DEFAULT:
            case CI_HANDLE_CHUNK_NEVER:
               fprintf(stderr, "%s(%s): %s: %s: unknown chunk saved\n",
                  d->file, d->test, d->keep ? "discard" : "default",
                  unknown[num_unknown].name);
               ++(d->error_count);
               break;

            case CI_HANDLE_CHUNK_IF_SAFE:
               if (!ancillaryb(unknown[num_unknown].name))
               {
                  fprintf(stderr,
                     "%s(%s): if-safe: %s: unknown critical chunk saved\n",
                     d->file, d->test, unknown[num_unknown].name);
                  ++(d->error_count);
                  break;
               }
               /* FALLTHROUGH */ /* (safe) */
            case CI_HANDLE_CHUNK_ALWAYS:
               break;
         }

         else
            flags |= chunk_info[chunk].flag;
      }
   }

   return flags;
}
#else /* SAVE_UNKNOWN_CHUNKS */
static ci_uint_32
get_unknown(display *d, ci_infop info_ptr, int after_IDAT)
   /* Otherwise this will return the cached values set by any user callback */
{
   UNUSED(info_ptr);

   if (after_IDAT)
      return d->after_IDAT;

   else
      return d->before_IDAT;
}

#  ifndef CI_READ_USER_CHUNKS_SUPPORTED
      /* The #defines above should mean this is never reached, it's just here as
       * a check to ensure the logic is correct.
       */
#     error No store support and no user chunk support, this will not work
#  endif /* READ_USER_CHUNKS */
#endif /* SAVE_UNKNOWN_CHUNKS */

static int
check(FILE *fp, int argc, const char **argv, ci_uint_32p flags/*out*/,
   display *d, int set_callback)
{
   int i, npasses, ipass;
   ci_uint_32 height;

   d->keep = CI_HANDLE_CHUNK_AS_DEFAULT;
   d->before_IDAT = 0;
   d->after_IDAT = 0;

   /* Some of these errors are permanently fatal and cause an exit here, others
    * are per-test and cause an error return.
    */
   d->ci_ptr = ci_create_read_struct(CI_LIBCI_VER_STRING, d, error,
      warning);
   if (d->ci_ptr == NULL)
   {
      fprintf(stderr, "%s(%s): could not allocate ci struct\n", d->file,
         d->test);
      /* Terminate here, this error is not test specific. */
      exit(1);
   }

   d->info_ptr = ci_create_info_struct(d->ci_ptr);
   d->end_ptr = ci_create_info_struct(d->ci_ptr);
   if (d->info_ptr == NULL || d->end_ptr == NULL)
   {
      fprintf(stderr, "%s(%s): could not allocate ci info\n", d->file,
         d->test);
      clean_display(d);
      exit(1);
   }

   ci_init_io(d->ci_ptr, fp);

#  ifdef CI_READ_USER_CHUNKS_SUPPORTED
      /* This is only done if requested by the caller; it interferes with the
       * standard store/save mechanism.
       */
      if (set_callback)
         ci_set_read_user_chunk_fn(d->ci_ptr, d, read_callback);
#  else
      UNUSED(set_callback)
#  endif

   /* Handle each argument in turn; multiple settings are possible for the same
    * chunk and multiple calls will occur (the last one should override all
    * preceding ones).
    */
   for (i=0; i<argc; ++i)
   {
      const char *equals = strchr(argv[i], '=');

      if (equals != NULL)
      {
         int chunk, option;

         if (strcmp(equals+1, "default") == 0)
            option = CI_HANDLE_CHUNK_AS_DEFAULT;
         else if (strcmp(equals+1, "discard") == 0)
            option = CI_HANDLE_CHUNK_NEVER;
         else if (strcmp(equals+1, "if-safe") == 0)
            option = CI_HANDLE_CHUNK_IF_SAFE;
         else if (strcmp(equals+1, "save") == 0)
            option = CI_HANDLE_CHUNK_ALWAYS;
         else
         {
            fprintf(stderr, "%s(%s): %s: unrecognized chunk option\n", d->file,
               d->test, argv[i]);
            display_exit(d);
         }

         switch (equals - argv[i])
         {
            case 4: /* chunk name */
               chunk = find(argv[i]);

               if (chunk >= 0)
               {
                  /* These #if tests have the effect of skipping the arguments
                   * if SAVE support is unavailable - we can't do a useful test
                   * in this case, so we just check the arguments!  This could
                   * be improved in the future by using the read callback.
                   */
#                 if CI_LIBCI_VER >= 10700 &&\
                     !defined(CI_SAVE_UNKNOWN_CHUNKS_SUPPORTED)
                     if (option < CI_HANDLE_CHUNK_IF_SAFE)
#                 endif /* 1.7+ SAVE_UNKNOWN_CHUNKS */
                  {
                     ci_byte name[5];

                     memcpy(name, chunk_info[chunk].name, 5);
                     ci_set_keep_unknown_chunks(d->ci_ptr, option, name, 1);
                     chunk_info[chunk].keep = option;
                  }
                  continue;
               }

               break;

            case 7: /* default */
               if (memcmp(argv[i], "default", 7) == 0)
               {
#                 if CI_LIBCI_VER >= 10700 &&\
                     !defined(CI_SAVE_UNKNOWN_CHUNKS_SUPPORTED)
                     if (option < CI_HANDLE_CHUNK_IF_SAFE)
#                 endif /* 1.7+ SAVE_UNKNOWN_CHUNKS */
                     ci_set_keep_unknown_chunks(d->ci_ptr, option, NULL, 0);

                  d->keep = option;
                  continue;
               }

               break;

            case 3: /* all */
               if (memcmp(argv[i], "all", 3) == 0)
               {
#                 if CI_LIBCI_VER >= 10700 &&\
                     !defined(CI_SAVE_UNKNOWN_CHUNKS_SUPPORTED)
                     if (option < CI_HANDLE_CHUNK_IF_SAFE)
#                 endif /* 1.7+ SAVE_UNKNOWN_CHUNKS */
                     ci_set_keep_unknown_chunks(d->ci_ptr, option, NULL, -1);

                  d->keep = option;

                  for (chunk = 0; chunk < NINFO; ++chunk)
                     if (chunk_info[chunk].all)
                        chunk_info[chunk].keep = option;
                  continue;
               }

               break;

            default: /* some misplaced = */

               break;
         }
      }

      fprintf(stderr, "%s(%s): %s: unrecognized chunk argument\n", d->file,
         d->test, argv[i]);
      display_exit(d);
   }

   ci_read_info(d->ci_ptr, d->info_ptr);

   switch (ci_get_interlace_type(d->ci_ptr, d->info_ptr))
   {
      case CI_INTERLACE_NONE:
         npasses = 1;
         break;

      case CI_INTERLACE_ADAM7:
         npasses = CI_INTERLACE_ADAM7_PASSES;
         break;

      default:
         /* Hard error because it is not test specific */
         fprintf(stderr, "%s(%s): invalid interlace type\n", d->file, d->test);
         clean_display(d);
         exit(1);
   }

   /* Skip the image data, if IDAT is not being handled then don't do this
    * because it will cause a CRC error.
    */
   if (chunk_info[0/*IDAT*/].keep == CI_HANDLE_CHUNK_AS_DEFAULT)
   {
      ci_start_read_image(d->ci_ptr);
      height = ci_get_image_height(d->ci_ptr, d->info_ptr);

      if (npasses > 1)
      {
         ci_uint_32 width = ci_get_image_width(d->ci_ptr, d->info_ptr);

         for (ipass=0; ipass<npasses; ++ipass)
         {
            ci_uint_32 wPass = CI_PASS_COLS(width, ipass);

            if (wPass > 0)
            {
               ci_uint_32 y;

               for (y=0; y<height; ++y)
                  if (CI_ROW_IN_INTERLACE_PASS(y, ipass))
                     ci_read_row(d->ci_ptr, NULL, NULL);
            }
         }
      } /* interlaced */

      else /* not interlaced */
      {
         ci_uint_32 y;

         for (y=0; y<height; ++y)
            ci_read_row(d->ci_ptr, NULL, NULL);
      }
   }

   ci_read_end(d->ci_ptr, d->end_ptr);

   flags[0] = get_valid(d, d->info_ptr);
   flags[1] = get_unknown(d, d->info_ptr, 0/*before IDAT*/);

   /* Only ci_read_ci sets CI_INFO_IDAT! */
   flags[chunk_info[0/*IDAT*/].keep != CI_HANDLE_CHUNK_AS_DEFAULT] |=
      CI_INFO_IDAT;

   flags[2] = get_valid(d, d->end_ptr);
   flags[3] = get_unknown(d, d->end_ptr, 1/*after IDAT*/);

   clean_display(d);

   return d->keep;
}

static void
check_error(display *d, ci_uint_32 flags, const char *message)
{
   while (flags)
   {
      ci_uint_32 flag = flags & -(ci_int_32)flags;
      int i = find_by_flag(flag);

      fprintf(stderr, "%s(%s): chunk %s: %s\n", d->file, d->test,
         chunk_info[i].name, message);
      ++(d->error_count);

      flags &= ~flag;
   }
}

static void
check_handling(display *d, int def, ci_uint_32 chunks, ci_uint_32 known,
   ci_uint_32 unknown, const char *position, int set_callback)
{
   while (chunks)
   {
      ci_uint_32 flag = chunks & -(ci_int_32)chunks;
      int i = find_by_flag(flag);
      int keep = chunk_info[i].keep;
      const char *type;
      const char *errorx = NULL;

      if (chunk_info[i].unknown)
      {
         if (keep == CI_HANDLE_CHUNK_AS_DEFAULT)
         {
            type = "UNKNOWN (default)";
            keep = def;
         }

         else
            type = "UNKNOWN (specified)";

         if (flag & known)
            errorx = "chunk processed";

         else switch (keep)
         {
            case CI_HANDLE_CHUNK_AS_DEFAULT:
               if (flag & unknown)
                  errorx = "DEFAULT: unknown chunk saved";
               break;

            case CI_HANDLE_CHUNK_NEVER:
               if (flag & unknown)
                  errorx = "DISCARD: unknown chunk saved";
               break;

            case CI_HANDLE_CHUNK_IF_SAFE:
               if (ancillary(chunk_info[i].name))
               {
                  if (!(flag & unknown))
                     errorx = "IF-SAFE: unknown ancillary chunk lost";
               }

               else if (flag & unknown)
                  errorx = "IF-SAFE: unknown critical chunk saved";
               break;

            case CI_HANDLE_CHUNK_ALWAYS:
               if (!(flag & unknown))
                  errorx = "SAVE: unknown chunk lost";
               break;

            default:
               errorx = "internal error: bad keep";
               break;
         }
      } /* unknown chunk */

      else /* known chunk */
      {
         type = "KNOWN";

         if (flag & known)
         {
            /* chunk was processed, it won't have been saved because that is
             * caught below when checking for inconsistent processing.
             */
            if (keep != CI_HANDLE_CHUNK_AS_DEFAULT)
               errorx = "!DEFAULT: known chunk processed";
         }

         else /* not processed */ switch (keep)
         {
            case CI_HANDLE_CHUNK_AS_DEFAULT:
               errorx = "DEFAULT: known chunk not processed";
               break;

            case CI_HANDLE_CHUNK_NEVER:
               if (flag & unknown)
                  errorx = "DISCARD: known chunk saved";
               break;

            case CI_HANDLE_CHUNK_IF_SAFE:
               if (ancillary(chunk_info[i].name))
               {
                  if (!(flag & unknown))
                     errorx = "IF-SAFE: known ancillary chunk lost";
               }

               else if (flag & unknown)
                  errorx = "IF-SAFE: known critical chunk saved";
               break;

            case CI_HANDLE_CHUNK_ALWAYS:
               if (!(flag & unknown))
                  errorx = "SAVE: known chunk lost";
               break;

            default:
               errorx = "internal error: bad keep (2)";
               break;
         }
      }

      if (errorx != NULL)
      {
         ++(d->error_count);
         fprintf(stderr, "%s(%s%s): %s %s %s: %s\n", d->file, d->test,
            set_callback ? ",callback" : "",
            type, chunk_info[i].name, position, errorx);
      }

      chunks &= ~flag;
   }
}

static void
perform_one_test(FILE *fp, int argc, const char **argv,
   ci_uint_32 *default_flags, display *d, int set_callback)
{
   int def;
   ci_uint_32 flags[2][4];

   rewind(fp);
   clear_keep();
   memcpy(flags[0], default_flags, sizeof flags[0]);

   def = check(fp, argc, argv, flags[1], d, set_callback);

   /* If IDAT is being handled as unknown the image read is skipped and all the
    * IDATs after the first end up in the end info struct, so in this case add
    * IDAT to the list of unknowns.  (Do this after 'check' above sets the
    * chunk_info 'keep' fields.)
    *
    * Note that the flag setting has to be in the 'known' field to avoid
    * triggering the consistency check below and the flag must only be set if
    * there are multiple IDATs, so if the check above did find an unknown IDAT
    * after IDAT.
    */
   if (chunk_info[0/*IDAT*/].keep != CI_HANDLE_CHUNK_AS_DEFAULT &&
       (flags[1][3] & CI_INFO_IDAT) != 0)
      flags[0][2] |= CI_INFO_IDAT;

   /* Chunks should either be known or unknown, never both and this should apply
    * whether the chunk is before or after the IDAT (actually, the app can
    * probably change this by swapping the handling after the image, but this
    * test does not do that.)
    */
   check_error(d, (flags[0][0]|flags[0][2]) & (flags[0][1]|flags[0][3]),
      "chunk handled inconsistently in count tests");
   check_error(d, (flags[1][0]|flags[1][2]) & (flags[1][1]|flags[1][3]),
      "chunk handled inconsistently in option tests");

   /* Now find out what happened to each chunk before and after the IDAT and
    * determine if the behavior was correct.  First some basic sanity checks,
    * any known chunk should be known in the original count, any unknown chunk
    * should be either known or unknown in the original.
    */
   {
      ci_uint_32 test;

      test = flags[1][0] & ~flags[0][0];
      check_error(d, test, "new known chunk before IDAT");
      test = flags[1][1] & ~(flags[0][0] | flags[0][1]);
      check_error(d, test, "new unknown chunk before IDAT");
      test = flags[1][2] & ~flags[0][2];
      check_error(d, test, "new known chunk after IDAT");
      test = flags[1][3] & ~(flags[0][2] | flags[0][3]);
      check_error(d, test, "new unknown chunk after IDAT");
   }

   /* Now each chunk in the original list should have been handled according to
    * the options set for that chunk, regardless of whether libci knows about
    * it or not.
    */
   check_handling(d, def, flags[0][0] | flags[0][1], flags[1][0], flags[1][1],
      "before IDAT", set_callback);
   check_handling(d, def, flags[0][2] | flags[0][3], flags[1][2], flags[1][3],
      "after IDAT", set_callback);
}

static void
perform_one_test_safe(FILE *fp, int argc, const char **argv,
   ci_uint_32 *default_flags, display *d, const char *test)
{
   if (setjmp(d->error_return) == 0)
   {
      d->test = test; /* allow use of d->error_return */
#     ifdef CI_SAVE_UNKNOWN_CHUNKS_SUPPORTED
         perform_one_test(fp, argc, argv, default_flags, d, 0);
#     endif
#     ifdef CI_READ_USER_CHUNKS_SUPPORTED
         perform_one_test(fp, argc, argv, default_flags, d, 1);
#     endif
      d->test = init; /* prevent use of d->error_return */
   }
}

static const char *standard_tests[] =
{
   "discard", "default=discard", NULL,
   "save", "default=save", NULL,
   "if-safe", "default=if-safe", NULL,
   "vpAg", "vpAg=if-safe", NULL,
   "sTER", "sTER=if-safe", NULL,
   "IDAT", "default=discard", "IDAT=save", NULL,
   "sAPI", "bKGD=save", "cHRM=save", "gAMA=save", "all=discard", "iCCP=save",
      "sBIT=save", "sRGB=save", "eXIf=save", NULL,
   NULL /*end*/
};

static CI_NORETURN void
usage(const char *program, const char *reason)
{
   fprintf(stderr, "ciunknown: %s: usage:\n %s [--strict] "
      "--default|{(CHNK|default|all)=(default|discard|if-safe|save)} "
      "testfile.ci\n", reason, program);
   exit(99);
}

int
main(int argc, const char **argv)
{
   FILE *fp;
   ci_uint_32 default_flags[4]; /*valid,unknown{before,after}*/
   int strict = 0, default_tests = 0;
   const char *count_argv = "default=save";
   const char *touch_file = NULL;
   display d;

   init_display(&d, argv[0]);

   while (++argv, --argc > 0)
   {
      if (strcmp(*argv, "--strict") == 0)
         strict = 1;

      else if (strcmp(*argv, "--default") == 0)
         default_tests = 1;

      else if (strcmp(*argv, "--touch") == 0)
      {
         if (argc > 1)
            touch_file = *++argv, --argc;

         else
            usage(d.program, "--touch: missing file name");
      }

      else
         break;
   }

   /* A file name is required, but there should be no other arguments if
    * --default was specified.
    */
   if (argc <= 0)
      usage(d.program, "missing test file");

   /* GCC BUG: if (default_tests && argc != 1) triggers some weird GCC argc
    * optimization which causes warnings with -Wstrict-overflow!
    */
   else if (default_tests)
      if (argc != 1)
         usage(d.program, "extra arguments");

   /* The name of the test file is the last argument; remove it. */
   d.file = argv[--argc];

   fp = fopen(d.file, "rb");
   if (fp == NULL)
   {
      perror(d.file);
      exit(99);
   }

   /* First find all the chunks, known and unknown, in the test file, a failure
    * here aborts the whole test.
    *
    * If 'save' is supported then the normal saving method should happen,
    * otherwise if 'read' is supported then the read callback will do the
    * same thing.  If both are supported the 'read' callback won't be
    * instantiated by default.  If 'save' is *not* supported then a user
    * callback is required even though we can call ci_get_unknown_chunks.
    */
   if (check(fp, 1, &count_argv, default_flags, &d,
#     ifdef CI_SAVE_UNKNOWN_CHUNKS_SUPPORTED
         0
#     else
         1
#     endif
      ) != CI_HANDLE_CHUNK_ALWAYS)
   {
      fprintf(stderr, "%s: %s: internal error\n", d.program, d.file);
      exit(99);
   }

   /* Now find what the various supplied options cause to change: */
   if (!default_tests)
   {
      d.test = cmd; /* acts as a flag to say exit, do not longjmp */
#     ifdef CI_SAVE_UNKNOWN_CHUNKS_SUPPORTED
         perform_one_test(fp, argc, argv, default_flags, &d, 0);
#     endif
#     ifdef CI_READ_USER_CHUNKS_SUPPORTED
         perform_one_test(fp, argc, argv, default_flags, &d, 1);
#     endif
      d.test = init;
   }

   else
   {
      const char **test = standard_tests;

      /* Set the exit_test pointer here so we can continue after a libci error.
       * NOTE: this leaks memory because the ci_struct data from the failing
       * test is never freed.
       */
      while (*test)
      {
         const char *this_test = *test++;
         const char **next = test;
         int count = display_rc(&d, strict), new_count;
         const char *result;
         int arg_count = 0;

         while (*next != NULL)
         {
            ++next;
            ++arg_count;
         }

         perform_one_test_safe(fp, arg_count, test, default_flags, &d,
            this_test);

         new_count = display_rc(&d, strict);

         if (new_count == count)
            result = "PASS";

         else
            result = "FAIL";

         printf("%s: %s %s\n", result, d.program, this_test);

         test = next+1;
      }
   }

   fclose(fp);

   if (display_rc(&d, strict) == 0)
   {
      /* Success, touch the success file if appropriate */
      if (touch_file != NULL)
      {
         FILE *fsuccess = fopen(touch_file, "wt");

         if (fsuccess != NULL)
         {
            int err = 0;
            fprintf(fsuccess, "CI unknown tests succeeded\n");
            fflush(fsuccess);
            err = ferror(fsuccess);

            if (fclose(fsuccess) || err)
            {
               fprintf(stderr, "%s: write failed\n", touch_file);
               exit(99);
            }
         }

         else
         {
            fprintf(stderr, "%s: open failed\n", touch_file);
            exit(99);
         }
      }

      return 0;
   }

   return 1;
}

#else /* !(READ_USER_CHUNKS || SAVE_UNKNOWN_CHUNKS) */
int
main(void)
{
   fprintf(stderr,
      " test ignored: no support to find out about unknown chunks\n");
   /* So the test is skipped: */
   return SKIP;
}
#endif /* READ_USER_CHUNKS || SAVE_UNKNOWN_CHUNKS */

#else /* !(SET_UNKNOWN_CHUNKS && READ) */
int
main(void)
{
   fprintf(stderr,
      " test ignored: no support to modify unknown chunk handling\n");
   /* So the test is skipped: */
   return SKIP;
}
#endif /* SET_UNKNOWN_CHUNKS && READ*/

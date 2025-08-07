/* citest.c - a test program for libci
 *
 * Copyright (c) 2018-2025 Cosmin Truta
 * Copyright (c) 1998-2002,2004,2006-2018 Glenn Randers-Pehrson
 * Copyright (c) 1996-1997 Andreas Dilger
 * Copyright (c) 1995-1996 Guy Eric Schalnat, Group 42, Inc.
 *
 * This code is released under the libci license.
 * For conditions of distribution and use, see the disclaimer
 * and license in ci.h
 *
 * This program reads in a CI image, writes it out again, and then
 * compares the two files.  If the files are identical, this shows that
 * the basic chunk handling, filtering, and (de)compression code is working
 * properly.  It does not currently test all of the transforms, although
 * it probably should.
 *
 * The program will report "FAIL" in certain legitimate cases:
 * 1) when the compression level or filter selection method is changed.
 * 2) when the maximum IDAT size (CI_ZBUF_SIZE in ciconf.h) is not 8192.
 * 3) unknown unsafe-to-copy ancillary chunks or unknown critical chunks
 *    exist in the input file.
 * 4) others not listed here...
 * In these cases, it is best to check with another tool such as "cicheck"
 * to see what the differences between the two files are.
 *
 * If a filename is given on the command-line, then this file is used
 * for the input, rather than the default "citest.ci".  This allows
 * testing a wide variety of files easily.  You can also test a number
 * of files at once by typing "citest -m file1.ci file2.ci ..."
 */

#define _POSIX_SOURCE 1

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef CI_ZLIB_HEADER
#  include CI_ZLIB_HEADER /* defined by cilibconf.h from 1.7 */
#else
#  include <zlib.h>
#endif

#include "ci.h"

/* This hack was introduced for historical reasons, and we are
 * still keeping it in libci-1.6.x for compatibility reasons.
 */
#define STDERR stdout

/* Generate a compiler error if there is an old ci.h in the search path. */
typedef ci_libci_version_1_6_50_git Your_ci_h_is_not_version_1_6_50_git;

/* Ensure that all version numbers in ci.h are consistent with one another. */
#if (CI_LIBCI_VER != CI_LIBCI_VER_MAJOR * 10000 + \
                       CI_LIBCI_VER_MINOR * 100 + \
                       CI_LIBCI_VER_RELEASE) || \
    (CI_LIBCI_VER_SHAREDLIB != CI_LIBCI_VER_MAJOR * 10 + \
                                 CI_LIBCI_VER_MINOR) || \
    (CI_LIBCI_VER_SHAREDLIB != CI_LIBCI_VER_SONUM) || \
    (CI_LIBCI_VER_SHAREDLIB != CI_LIBCI_VER_DLLNUM)
#  error Inconsistent version numbers in "ci.h"
#endif

/* In version 1.6.1, we added support for the configure test harness, which
 * uses 77 to indicate a skipped test. On the other hand, in cmake build tests,
 * we still need to succeed on a skipped test, so:
 */
#if defined(HAVE_CONFIG_H)
#  define SKIP 77
#else
#  define SKIP 0
#endif

/* Known chunks that exist in citest.ci must be supported, or citest will
 * fail simply as a result of re-ordering them.  This may be fixed in the next
 * generation of libci.
 *
 * citest allocates a single row buffer for each row and overwrites it,
 * therefore if the write side doesn't support the writing of interlaced images
 * nothing can be done for an interlaced image (and the code below will fail
 * horribly trying to write extra data after writing garbage).
 */
#if defined CI_READ_SUPPORTED && /* else nothing can be done */ \
    defined CI_READ_bKGD_SUPPORTED && \
    defined CI_READ_cHRM_SUPPORTED && \
    defined CI_READ_gAMA_SUPPORTED && \
    defined CI_READ_oFFs_SUPPORTED && \
    defined CI_READ_pCAL_SUPPORTED && \
    defined CI_READ_pHYs_SUPPORTED && \
    defined CI_READ_sBIT_SUPPORTED && \
    defined CI_READ_sCAL_SUPPORTED && \
    defined CI_READ_sRGB_SUPPORTED && \
    defined CI_READ_sPLT_SUPPORTED && \
    defined CI_READ_tEXt_SUPPORTED && \
    defined CI_READ_tIME_SUPPORTED && \
    defined CI_READ_zTXt_SUPPORTED && \
    (defined CI_WRITE_INTERLACING_SUPPORTED || CI_LIBCI_VER >= 10700)

/* Copied from cipriv.h but only used in error messages below. */
#ifndef CI_ZBUF_SIZE
#  define CI_ZBUF_SIZE 8192
#endif

#ifndef CI_DEBUG
#  define CI_DEBUG 0
#endif

#if CI_DEBUG > 1
#  define citest_debug(m)          ((void)fprintf(stderr, m "\n"))
#  define citest_debug1(m, p1)     ((void)fprintf(stderr, m "\n", p1))
#  define citest_debug2(m, p1, p2) ((void)fprintf(stderr, m "\n", p1, p2))
#elif CI_DEBUG == 0 || CI_DEBUG == 1
#  define citest_debug(m)          ((void)0)
#  define citest_debug1(m, p1)     ((void)0)
#  define citest_debug2(m, p1, p2) ((void)0)
#else /* CI_DEBUG < 0 */
#  error Bad CI_DEBUG value
#endif

/* Turn on CPU timing
#define CITEST_TIMING
*/

#ifndef CI_FLOATING_POINT_SUPPORTED
#undef CITEST_TIMING
#endif

#ifdef CITEST_TIMING
static float t_start, t_stop, t_decode, t_encode, t_misc;
#include <time.h>
#endif

#ifdef CI_TIME_RFC1123_SUPPORTED
static int tIME_chunk_present = 0;
static char tIME_string[29] = "tIME chunk is not present";
/* This use case is deprecated.
 * See the declaration of ci_convert_to_rfc1123_buffer for more details.
 */
#endif

static int verbose = 0;
static int strict = 0;
static int relaxed = 0;
static int xfail = 0;
static int unsupported_chunks = 0; /* chunk unsupported by libci in input */
static int error_count = 0; /* count calls to ci_error */
static int warning_count = 0; /* count calls to ci_warning */

/* Example of using row callbacks to make a simple progress meter */
static int status_pass = 1;
static int status_dots_requested = 0;
static int status_dots = 1;

static void CICBAPI
read_row_callback(ci_structp ci_ptr, ci_uint_32 row_number, int pass)
{
   /* The callback should always receive correct parameters. */
   if (ci_ptr == NULL)
      ci_error(ci_ptr, "read_row_callback: bad ci_ptr");
   if (row_number > CI_UINT_31_MAX)
      ci_error(ci_ptr, "read_row_callback: bad row number");
   if (pass < 0 || pass > 7)
      ci_error(ci_ptr, "read_row_callback: bad pass");

   if (status_pass != pass)
   {
      fprintf(stdout, "\n Pass %d: ", pass);
      status_pass = pass;
      status_dots = 31;
   }

   status_dots--;

   if (status_dots == 0)
   {
      fprintf(stdout, "\n         ");
      status_dots = 30;
   }

   fprintf(stdout, "r");
}

#ifdef CI_WRITE_SUPPORTED
static void CICBAPI
write_row_callback(ci_structp ci_ptr, ci_uint_32 row_number, int pass)
{
   /* The callback should always receive correct parameters. */
   if (ci_ptr == NULL)
      ci_error(ci_ptr, "write_row_callback: bad ci_ptr");
   if (row_number > CI_UINT_31_MAX)
      ci_error(ci_ptr, "write_row_callback: bad row number");
   if (pass < 0 || pass > 7)
      ci_error(ci_ptr, "write_row_callback: bad pass");

   fprintf(stdout, "w");
}
#endif


#ifdef CI_READ_USER_TRANSFORM_SUPPORTED
/* Example of using a user transform callback (doesn't do anything at present).
 */
static void CICBAPI
read_user_callback(ci_structp ci_ptr, ci_row_infop row_info, ci_bytep data)
{
   /* The callback should always receive correct parameters. */
   if (ci_ptr == NULL)
      ci_error(ci_ptr, "read_user_callback: bad ci_ptr");
   if (row_info == NULL)
      ci_error(ci_ptr, "read_user_callback: bad row info");
   if (data == NULL)
      ci_error(ci_ptr, "read_user_callback: bad data");
}
#endif

#ifdef CI_WRITE_USER_TRANSFORM_SUPPORTED
/* Example of using user transform callback (we don't transform anything,
 * but merely count the zero samples)
 */

static ci_uint_32 zero_samples;

static void CICBAPI
count_zero_samples(ci_structp ci_ptr, ci_row_infop row_info, ci_bytep data)
{
   ci_bytep dp = data;

   /* The callback should always receive correct parameters. */
   if (ci_ptr == NULL)
      ci_error(ci_ptr, "count_zero_samples: bad ci_ptr");
   if (row_info == NULL)
      ci_error(ci_ptr, "count_zero_samples: bad row info");
   if (data == NULL)
      ci_error(ci_ptr, "count_zero_samples: bad data");

   /* Contents of row_info:
    *  ci_uint_32 width      width of row
    *  ci_uint_32 rowbytes   number of bytes in row
    *  ci_byte color_type    color type of pixels
    *  ci_byte bit_depth     bit depth of samples
    *  ci_byte channels      number of channels (1-4)
    *  ci_byte pixel_depth   bits per pixel (depth*channels)
    */

   /* Counts the number of zero samples (or zero pixels if color_type is 3 */

   if (row_info->color_type == 0 || row_info->color_type == 3)
   {
      int pos = 0;
      ci_uint_32 n, nstop;

      for (n = 0, nstop = row_info->width; n < nstop; n++)
      {
         if (row_info->bit_depth == 1)
         {
            if (((*dp << pos++ ) & 0x80) == 0)
               zero_samples++;

            if (pos == 8)
            {
               pos = 0;
               dp++;
            }
         }

         if (row_info->bit_depth == 2)
         {
            if (((*dp << (pos+=2)) & 0xc0) == 0)
               zero_samples++;

            if (pos == 8)
            {
               pos = 0;
               dp++;
            }
         }

         if (row_info->bit_depth == 4)
         {
            if (((*dp << (pos+=4)) & 0xf0) == 0)
               zero_samples++;

            if (pos == 8)
            {
               pos = 0;
               dp++;
            }
         }

         if (row_info->bit_depth == 8)
            if (*dp++ == 0)
               zero_samples++;

         if (row_info->bit_depth == 16)
         {
            if ((*dp | *(dp+1)) == 0)
               zero_samples++;
            dp += 2;
         }
      }
   }
   else /* Other color types */
   {
      ci_uint_32 n, nstop;
      int channel;
      int color_channels = row_info->channels;
      if (row_info->color_type > 3)
         color_channels--;

      for (n = 0, nstop = row_info->width; n < nstop; n++)
      {
         for (channel = 0; channel < color_channels; channel++)
         {
            if (row_info->bit_depth == 8)
               if (*dp++ == 0)
                  zero_samples++;

            if (row_info->bit_depth == 16)
            {
               if ((*dp | *(dp+1)) == 0)
                  zero_samples++;

               dp += 2;
            }
         }
         if (row_info->color_type > 3)
         {
            dp++;
            if (row_info->bit_depth == 16)
               dp++;
         }
      }
   }
}
#endif /* WRITE_USER_TRANSFORM */

#ifndef CI_STDIO_SUPPORTED
/* START of code to validate stdio-free compilation */
/* These copies of the default read/write functions come from cirio.c and
 * ciwio.c.  They allow "don't include stdio" testing of the library.
 * This is the function that does the actual reading of data.  If you are
 * not reading from a standard C stream, you should create a replacement
 * read_data function and use it at run time with ci_set_read_fn(), rather
 * than changing the library.
 */

#ifdef CI_IO_STATE_SUPPORTED
void
citest_check_io_state(ci_structp ci_ptr, size_t data_length,
    ci_uint_32 io_op)
{
   ci_uint_32 io_state = ci_get_io_state(ci_ptr);
   int err = 0;

   /* Check if the current operation (reading / writing) is as expected. */
   if ((io_state & CI_IO_MASK_OP) != io_op)
      ci_error(ci_ptr, "Incorrect operation in I/O state");

   /* Check if the buffer size specific to the current location
    * (file signature / header / data / crc) is as expected.
    */
   switch ((io_state & CI_IO_MASK_LOC) != 0)
   {
   case CI_IO_SIGNATURE:
      if (data_length > 8)
         err = 1;
      break;
   case CI_IO_CHUNK_HDR:
      if (data_length != 8)
         err = 1;
      break;
   case CI_IO_CHUNK_DATA:
      break;  /* no restrictions here */
   case CI_IO_CHUNK_CRC:
      if (data_length != 4)
         err = 1;
      break;
   default:
      err = 1;  /* uninitialized */
   }
   if (err != 0)
      ci_error(ci_ptr, "Bad I/O state or buffer size");
}
#endif

static void CICBAPI
citest_read_data(ci_structp ci_ptr, ci_bytep data, size_t length)
{
   size_t check = 0;
   ci_voidp io_ptr;

   if (ci_ptr == NULL)
      ci_error(ci_ptr, "citest_read_data: bad ci_ptr");

   /* fread() returns 0 on error, so it is OK to store this in a size_t
    * instead of an int, which is what fread() actually returns.
    */
   io_ptr = ci_get_io_ptr(ci_ptr);
   if (io_ptr != NULL)
      check = fread(data, 1, length, (FILE *)io_ptr);

   if (check != length)
      ci_error(ci_ptr, "Read Error");

#ifdef CI_IO_STATE_SUPPORTED
   citest_check_io_state(ci_ptr, length, CI_IO_READING);
#endif
}

#ifdef CI_WRITE_FLUSH_SUPPORTED
static void CICBAPI
citest_flush(ci_structp ci_ptr)
{
   if (ci_ptr == NULL)
      ci_error(ci_ptr, "citest_flush: bad ci_ptr");

   /* Do nothing; fflush() is said to be just a waste of energy. */
}
#endif

/* This is the function that does the actual writing of data.  If you are
 * not writing to a standard C stream, you should create a replacement
 * write_data function and use it at run time with ci_set_write_fn(), rather
 * than changing the library.
 */
static void CICBAPI
citest_write_data(ci_structp ci_ptr, ci_bytep data, size_t length)
{
   size_t check;

   if (ci_ptr == NULL)
      ci_error(ci_ptr, "citest_write_data: bad ci_ptr");

   check = fwrite(data, 1, length, (FILE *)ci_get_io_ptr(ci_ptr));

   if (check != length)
      ci_error(ci_ptr, "Write Error");

#ifdef CI_IO_STATE_SUPPORTED
   citest_check_io_state(ci_ptr, length, CI_IO_WRITING);
#endif
}
#endif /* !STDIO */

/* This function is called when there is a warning, but the library thinks
 * it can continue anyway.  Replacement functions don't have to do anything
 * here if you don't want to.  In the default configuration, ci_ptr is
 * not used, but it is passed in case it may be useful.
 */
typedef struct
{
   const char *file_name;
}  citest_error_parameters;

static void CICBAPI
citest_warning(ci_structp ci_ptr, ci_const_charp message)
{
   const char *name = "UNKNOWN (ERROR!)";
   citest_error_parameters *test =
      (citest_error_parameters*)ci_get_error_ptr(ci_ptr);

   ++warning_count;

   if (test != NULL && test->file_name != NULL)
      name = test->file_name;

   fprintf(STDERR, "\n%s: libci warning: %s\n", name, message);
}

/* This is the default error handling function.  Note that replacements for
 * this function MUST NOT RETURN, or the program will likely crash.  This
 * function is used by default, or if the program supplies NULL for the
 * error function pointer in ci_set_error_fn().
 */
static void CICBAPI
citest_error(ci_structp ci_ptr, ci_const_charp message)
{
   ++error_count;

   citest_warning(ci_ptr, message);
   /* We can return because ci_error calls the default handler, which is
    * actually OK in this case.
    */
}

/* END of code to validate stdio-free compilation */

/* START of code to validate memory allocation and deallocation */
#if defined(CI_USER_MEM_SUPPORTED) && CI_DEBUG

/* Allocate memory.  For reasonable files, size should never exceed
 * 64K.  However, zlib may allocate more than 64K if you don't tell
 * it not to.  See zconf.h and ci.h for more information.  zlib does
 * need to allocate exactly 64K, so whatever you call here must
 * have the ability to do that.
 *
 * This piece of code can be compiled to validate max 64K allocations
 * by setting MAXSEG_64K in zlib zconf.h *or* CI_MAX_MALLOC_64K.
 */
typedef struct memory_information
{
   ci_alloc_size_t          size;
   ci_voidp                 pointer;
   struct memory_information *next;
} memory_information;
typedef memory_information *memory_infop;

static memory_infop pinformation = NULL;
static int current_allocation = 0;
static int maximum_allocation = 0;
static int total_allocation = 0;
static int num_allocations = 0;

ci_voidp CICBAPI ci_debug_malloc(ci_structp ci_ptr,
    ci_alloc_size_t size);
void CICBAPI ci_debug_free(ci_structp ci_ptr, ci_voidp ptr);

ci_voidp
CICBAPI ci_debug_malloc(ci_structp ci_ptr, ci_alloc_size_t size)
{

   /* ci_malloc has already tested for NULL; ci_create_struct calls
    * ci_debug_malloc directly, with ci_ptr == NULL which is OK
    */

   if (size == 0)
      return NULL;

   /* This calls the library allocator twice, once to get the requested
      buffer and once to get a new free list entry. */
   {
      /* Disable malloc_fn and free_fn */
      memory_infop pinfo;
      ci_set_mem_fn(ci_ptr, NULL, NULL, NULL);
      pinfo = (memory_infop)ci_malloc(ci_ptr,
          (sizeof *pinfo));
      pinfo->size = size;
      current_allocation += size;
      total_allocation += size;
      ++num_allocations;

      if (current_allocation > maximum_allocation)
         maximum_allocation = current_allocation;

      pinfo->pointer = ci_malloc(ci_ptr, size);
      /* Restore malloc_fn and free_fn */

      ci_set_mem_fn(ci_ptr,
          NULL, ci_debug_malloc, ci_debug_free);

      if (size != 0 && pinfo->pointer == NULL)
      {
         current_allocation -= size;
         total_allocation -= size;
         ci_error(ci_ptr,
           "out of memory in citest->ci_debug_malloc");
      }

      pinfo->next = pinformation;
      pinformation = pinfo;
      /* Make sure the caller isn't assuming zeroed memory. */
      memset(pinfo->pointer, 0xdd, pinfo->size);

      if (verbose != 0)
         printf("ci_malloc %lu bytes at %p\n", (unsigned long)size,
             pinfo->pointer);

      return (ci_voidp)pinfo->pointer;
   }
}

/* Free a pointer.  It is removed from the list at the same time. */
void CICBAPI
ci_debug_free(ci_structp ci_ptr, ci_voidp ptr)
{
   if (ci_ptr == NULL)
      fprintf(STDERR, "NULL pointer to ci_debug_free.\n");

   if (ptr == 0)
   {
#if 0 /* This happens all the time. */
      fprintf(STDERR, "WARNING: freeing NULL pointer\n");
#endif
      return;
   }

   /* Unlink the element from the list. */
   if (pinformation != NULL)
   {
      memory_infop *ppinfo = &pinformation;

      for (;;)
      {
         memory_infop pinfo = *ppinfo;

         if (pinfo->pointer == ptr)
         {
            *ppinfo = pinfo->next;
            current_allocation -= pinfo->size;
            if (current_allocation < 0)
               fprintf(STDERR, "Duplicate free of memory\n");
            /* We must free the list element too, but first kill
               the memory that is to be freed. */
            memset(ptr, 0x55, pinfo->size);
            free(pinfo);
            pinfo = NULL;
            break;
         }

         if (pinfo->next == NULL)
         {
            fprintf(STDERR, "Pointer %p not found\n", ptr);
            break;
         }

         ppinfo = &pinfo->next;
      }
   }

   /* Finally free the data. */
   if (verbose != 0)
      printf("Freeing %p\n", ptr);

   if (ptr != NULL)
      free(ptr);
   ptr = NULL;
}
#endif /* USER_MEM && DEBUG */
/* END of code to test memory allocation/deallocation */


#ifdef CI_READ_USER_CHUNKS_SUPPORTED
/* Demonstration of user chunk support of the sTER and vpAg chunks */

/* (sTER is a public chunk not yet known by libci.  vpAg is a private
chunk used in ImageMagick to store "virtual page" size).  */

typedef struct user_chunk_info_def
{
   ci_const_infop info_ptr;
   ci_uint_32     vpAg_width, vpAg_height;
   ci_byte        vpAg_units;
   ci_byte        sTER_mode;
   int             location[2];
} user_chunk_info;

/* Used for location and order; zero means nothing. */
#define have_sTER   0x01
#define have_vpAg   0x02
#define before_PLTE 0x10
#define before_IDAT 0x20
#define after_IDAT  0x40

static void
init_user_chunk_info(ci_const_infop info_ptr, user_chunk_info *chunk_data)
{
   memset(chunk_data, 0, sizeof(*chunk_data));
   chunk_data->info_ptr = info_ptr;
}

static int
set_chunk_location(ci_structp ci_ptr, user_chunk_info *chunk_data, int what)
{
   int location;

   if ((chunk_data->location[0] & what) != 0 ||
       (chunk_data->location[1] & what) != 0)
      return 0; /* we already have one of these */

   /* Find where we are (the code below zeroes info_ptr to indicate that the
    * chunks before the first IDAT have been read.)
    */
   if (chunk_data->info_ptr == NULL) /* after IDAT */
      location = what | after_IDAT;

   else if (ci_get_valid(ci_ptr, chunk_data->info_ptr, CI_INFO_PLTE) != 0)
      location = what | before_IDAT;

   else
      location = what | before_PLTE;

   if (chunk_data->location[0] == 0)
      chunk_data->location[0] = location;

   else
      chunk_data->location[1] = location;

   return 1; /* handled */
}

static int CICBAPI
read_user_chunk_callback(ci_struct *ci_ptr, ci_unknown_chunkp chunk)
{
   user_chunk_info *my_user_chunk_data =
      (user_chunk_info*)ci_get_user_chunk_ptr(ci_ptr);

   if (my_user_chunk_data == NULL)
      ci_error(ci_ptr, "lost pointer to user chunk data");

   /* Return one of the following:
    *    return -n;  chunk had an error
    *    return 0;   did not recognize
    *    return n;   success
    *
    * The unknown chunk structure contains the chunk data:
    * ci_byte name[5];
    * ci_byte *data;
    * size_t size;
    *
    * Note that libci has already taken care of the CRC handling.
    */

   if (chunk->name[0] == 115 && chunk->name[1] ==  84 &&     /* s  T */
       chunk->name[2] ==  69 && chunk->name[3] ==  82)       /* E  R */
      {
         /* Found sTER chunk */
         if (chunk->size != 1)
            return -1; /* Error return */

         if (chunk->data[0] != 0 && chunk->data[0] != 1)
            return -1;  /* Invalid mode */

         if (set_chunk_location(ci_ptr, my_user_chunk_data, have_sTER) != 0)
         {
            my_user_chunk_data->sTER_mode = chunk->data[0];
            return 1;
         }

         else
            return 0; /* duplicate sTER - give it to libci */
      }

   if (chunk->name[0] != 118 || chunk->name[1] != 112 ||    /* v  p */
       chunk->name[2] !=  65 || chunk->name[3] != 103)      /* A  g */
      return 0; /* Did not recognize */

   /* Found ImageMagick vpAg chunk */

   if (chunk->size != 9)
      return -1; /* Error return */

   if (set_chunk_location(ci_ptr, my_user_chunk_data, have_vpAg) == 0)
      return 0;  /* duplicate vpAg */

   my_user_chunk_data->vpAg_width = ci_get_uint_31(ci_ptr, chunk->data);
   my_user_chunk_data->vpAg_height = ci_get_uint_31(ci_ptr, chunk->data + 4);
   my_user_chunk_data->vpAg_units = chunk->data[8];

   return 1;
}

#ifdef CI_WRITE_SUPPORTED
static void
write_sTER_chunk(ci_structp write_ptr, user_chunk_info *data)
{
   ci_byte sTER[5] = {115,  84,  69,  82, '\0'};

   if (verbose != 0)
      fprintf(STDERR, "\n stereo mode = %d\n", data->sTER_mode);

   ci_write_chunk(write_ptr, sTER, &data->sTER_mode, 1);
}

static void
write_vpAg_chunk(ci_structp write_ptr, user_chunk_info *data)
{
   ci_byte vpAg[5] = {118, 112,  65, 103, '\0'};

   ci_byte vpag_chunk_data[9];

   if (verbose != 0)
      fprintf(STDERR, " vpAg = %lu x %lu, units = %d\n",
          (unsigned long)data->vpAg_width,
          (unsigned long)data->vpAg_height,
          data->vpAg_units);

   ci_save_uint_32(vpag_chunk_data, data->vpAg_width);
   ci_save_uint_32(vpag_chunk_data + 4, data->vpAg_height);
   vpag_chunk_data[8] = data->vpAg_units;
   ci_write_chunk(write_ptr, vpAg, vpag_chunk_data, 9);
}

static void
write_chunks(ci_structp write_ptr, user_chunk_info *data, int location)
{
   int i;

   /* Notice that this preserves the original chunk order, however chunks
    * intercepted by the callback will be written *after* chunks passed to
    * libci.  This will actually reverse a pair of sTER chunks or a pair of
    * vpAg chunks, resulting in an error later.  This is not worth worrying
    * about - the chunks should not be duplicated!
    */
   for (i = 0; i < 2; ++i)
   {
      if (data->location[i] == (location | have_sTER))
         write_sTER_chunk(write_ptr, data);

      else if (data->location[i] == (location | have_vpAg))
         write_vpAg_chunk(write_ptr, data);
   }
}
#endif /* WRITE */
#else /* !READ_USER_CHUNKS */
#  define write_chunks(pp,loc) ((void)0)
#endif
/* END of code to demonstrate user chunk support */

/* START of code to check that libci has the required text support; this only
 * checks for the write support because if read support is missing the chunk
 * will simply not be reported back to citest.
 */
#ifdef CI_TEXT_SUPPORTED
static void
citest_check_text_support(ci_structp ci_ptr, ci_textp text_ptr,
    int num_text)
{
   while (num_text > 0)
   {
      switch (text_ptr[--num_text].compression)
      {
         case CI_TEXT_COMPRESSION_NONE:
            break;

         case CI_TEXT_COMPRESSION_zTXt:
#           ifndef CI_WRITE_zTXt_SUPPORTED
               ++unsupported_chunks;
               /* In libci 1.7 this now does an app-error, so stop it: */
               text_ptr[num_text].compression = CI_TEXT_COMPRESSION_NONE;
#           endif
            break;

         case CI_ITXT_COMPRESSION_NONE:
         case CI_ITXT_COMPRESSION_zTXt:
#           ifndef CI_WRITE_iTXt_SUPPORTED
               ++unsupported_chunks;
               text_ptr[num_text].compression = CI_TEXT_COMPRESSION_NONE;
#           endif
            break;

         default:
            /* This is an error */
            ci_error(ci_ptr, "invalid text chunk compression field");
            break;
      }
   }
}
#endif
/* END of code to check that libci has the required text support */

/* Test one file */
static int
test_one_file(const char *inname, const char *outname)
{
   static FILE *fpin;
   static FILE *fpout;  /* "static" prevents setjmp corruption */
   citest_error_parameters error_parameters;
   ci_structp read_ptr;
   ci_infop read_info_ptr, end_info_ptr;
#ifdef CI_WRITE_SUPPORTED
   ci_structp write_ptr;
   ci_infop write_info_ptr;
   ci_infop write_end_info_ptr;
#ifdef CI_WRITE_FILTER_SUPPORTED
   int interlace_preserved = 1;
#endif /* WRITE_FILTER */
#else /* !WRITE */
   ci_structp write_ptr = NULL;
   ci_infop write_info_ptr = NULL;
   ci_infop write_end_info_ptr = NULL;
#endif /* !WRITE */
   ci_bytep row_buf;
   ci_uint_32 y;
   ci_uint_32 width, height;
   int bit_depth, color_type;
   user_chunk_info my_user_chunk_data;
   int pass, num_passes;

   row_buf = NULL;
   error_parameters.file_name = inname;

   if ((fpin = fopen(inname, "rb")) == NULL)
   {
      fprintf(STDERR, "Could not find input file %s\n", inname);
      return 1;
   }

   if ((fpout = fopen(outname, "wb")) == NULL)
   {
      fprintf(STDERR, "Could not open output file %s\n", outname);
      fclose(fpin);
      return 1;
   }

   citest_debug("Allocating read and write structures");
#if defined(CI_USER_MEM_SUPPORTED) && CI_DEBUG
   read_ptr =
       ci_create_read_struct_2(CI_LIBCI_VER_STRING, NULL,
       NULL, NULL, NULL, ci_debug_malloc, ci_debug_free);
#else
   read_ptr =
       ci_create_read_struct(CI_LIBCI_VER_STRING, NULL, NULL, NULL);
#endif
   ci_set_error_fn(read_ptr, &error_parameters, citest_error,
       citest_warning);

#ifdef CI_WRITE_SUPPORTED
#if defined(CI_USER_MEM_SUPPORTED) && CI_DEBUG
   write_ptr =
       ci_create_write_struct_2(CI_LIBCI_VER_STRING, NULL,
       NULL, NULL, NULL, ci_debug_malloc, ci_debug_free);
#else
   write_ptr =
       ci_create_write_struct(CI_LIBCI_VER_STRING, NULL, NULL, NULL);
#endif
   ci_set_error_fn(write_ptr, &error_parameters, citest_error,
       citest_warning);
#endif
   citest_debug("Allocating read_info, write_info and end_info structures");
   read_info_ptr = ci_create_info_struct(read_ptr);
   end_info_ptr = ci_create_info_struct(read_ptr);
#ifdef CI_WRITE_SUPPORTED
   write_info_ptr = ci_create_info_struct(write_ptr);
   write_end_info_ptr = ci_create_info_struct(write_ptr);
#endif

#ifdef CI_READ_USER_CHUNKS_SUPPORTED
   init_user_chunk_info(read_info_ptr, &my_user_chunk_data);
   ci_set_read_user_chunk_fn(read_ptr, &my_user_chunk_data,
       read_user_chunk_callback);
#endif

#ifdef CI_SETJMP_SUPPORTED
   citest_debug("Setting jmpbuf for read struct");
   if (setjmp(ci_jmpbuf(read_ptr)))
   {
      fprintf(STDERR, "%s -> %s: libci read error\n", inname, outname);
      ci_free(read_ptr, row_buf);
      row_buf = NULL;
      if (verbose != 0)
        fprintf(STDERR, "   destroy read structs\n");
      ci_destroy_read_struct(&read_ptr, &read_info_ptr, &end_info_ptr);
#ifdef CI_WRITE_SUPPORTED
      if (verbose != 0)
        fprintf(STDERR, "   destroy write structs\n");
      ci_destroy_info_struct(write_ptr, &write_end_info_ptr);
      ci_destroy_write_struct(&write_ptr, &write_info_ptr);
#endif
      fclose(fpin);
      fclose(fpout);
      return 1;
   }

#ifdef CI_WRITE_SUPPORTED
   citest_debug("Setting jmpbuf for write struct");

   if (setjmp(ci_jmpbuf(write_ptr)))
   {
      fprintf(STDERR, "%s -> %s: libci write error\n", inname, outname);
      ci_free(read_ptr, row_buf);
      row_buf = NULL;
      if (verbose != 0)
        fprintf(STDERR, "   destroying read structs\n");
      ci_destroy_read_struct(&read_ptr, &read_info_ptr, &end_info_ptr);
      if (verbose != 0)
        fprintf(STDERR, "   destroying write structs\n");
      ci_destroy_info_struct(write_ptr, &write_end_info_ptr);
      ci_destroy_write_struct(&write_ptr, &write_info_ptr);
      fclose(fpin);
      fclose(fpout);
      return 1;
   }
#endif
#endif

#ifdef CI_BENIGN_ERRORS_SUPPORTED
   if (strict != 0)
   {
      /* Treat ci_benign_error() as errors on read */
      ci_set_benign_errors(read_ptr, 0);

# ifdef CI_WRITE_SUPPORTED
      /* Treat them as errors on write */
      ci_set_benign_errors(write_ptr, 0);
# endif

      /* if strict is not set, then app warnings and errors are treated as
       * warnings in release builds, but not in unstable builds; this can be
       * changed with '--relaxed'.
       */
   }

   else if (relaxed != 0)
   {
      /* Allow application (citest) errors and warnings to pass */
      ci_set_benign_errors(read_ptr, 1);

      /* Turn off CRC checking while reading */
      ci_set_crc_action(read_ptr, CI_CRC_QUIET_USE, CI_CRC_QUIET_USE);

#ifdef CI_IGNORE_ADLER32
      /* Turn off ADLER32 checking while reading */
      ci_set_option(read_ptr, CI_IGNORE_ADLER32, CI_OPTION_ON);
#endif

# ifdef CI_WRITE_SUPPORTED
      ci_set_benign_errors(write_ptr, 1);
# endif

   }
#endif /* BENIGN_ERRORS */

   citest_debug("Initializing input and output streams");
#ifdef CI_STDIO_SUPPORTED
   ci_init_io(read_ptr, fpin);
#  ifdef CI_WRITE_SUPPORTED
   ci_init_io(write_ptr, fpout);
#  endif
#else
   ci_set_read_fn(read_ptr, (ci_voidp)fpin, citest_read_data);
#  ifdef CI_WRITE_SUPPORTED
   ci_set_write_fn(write_ptr, (ci_voidp)fpout,  citest_write_data,
#    ifdef CI_WRITE_FLUSH_SUPPORTED
       citest_flush);
#    else
       NULL);
#    endif
#  endif
#endif

   if (status_dots_requested == 1)
   {
#ifdef CI_WRITE_SUPPORTED
      ci_set_write_status_fn(write_ptr, write_row_callback);
#endif
      ci_set_read_status_fn(read_ptr, read_row_callback);
   }

   else
   {
#ifdef CI_WRITE_SUPPORTED
      ci_set_write_status_fn(write_ptr, NULL);
#endif
      ci_set_read_status_fn(read_ptr, NULL);
   }

#ifdef CI_READ_USER_TRANSFORM_SUPPORTED
   ci_set_read_user_transform_fn(read_ptr, read_user_callback);
#endif
#ifdef CI_WRITE_USER_TRANSFORM_SUPPORTED
   zero_samples = 0;
   ci_set_write_user_transform_fn(write_ptr, count_zero_samples);
#endif

#ifdef CI_SET_UNKNOWN_CHUNKS_SUPPORTED
   /* Preserve all the unknown chunks, if possible.  If this is disabled, then
    * even if the ci_{get,set}_unknown_chunks stuff is enabled, we can't use
    * libci to *save* the unknown chunks on read (because we can't switch the
    * save option on!)
    *
    * Notice that if SET_UNKNOWN_CHUNKS is *not* supported, the reader will
    * discard all unknown chunks, and the writer will write them all.
    */
#ifdef CI_SAVE_UNKNOWN_CHUNKS_SUPPORTED
   ci_set_keep_unknown_chunks(read_ptr, CI_HANDLE_CHUNK_ALWAYS,
       NULL, 0);
#endif
#ifdef CI_WRITE_UNKNOWN_CHUNKS_SUPPORTED
   ci_set_keep_unknown_chunks(write_ptr, CI_HANDLE_CHUNK_ALWAYS,
       NULL, 0);
#endif
#endif

   citest_debug("Reading info struct");
   ci_read_info(read_ptr, read_info_ptr);

#ifdef CI_READ_USER_CHUNKS_SUPPORTED
   /* This is a bit of a hack; there is no obvious way in the callback function
    * to determine that the chunks before the first IDAT have been read, so
    * remove the info_ptr (which is only used to determine position relative to
    * PLTE) here to indicate that we are after the IDAT.
    */
   my_user_chunk_data.info_ptr = NULL;
#endif

   citest_debug("Transferring info struct");
   {
      int interlace_type, compression_type, filter_type;

      if (ci_get_IHDR(read_ptr, read_info_ptr, &width, &height, &bit_depth,
          &color_type, &interlace_type, &compression_type, &filter_type) != 0)
      {
         ci_set_IHDR(write_ptr, write_info_ptr, width, height, bit_depth,
             color_type, interlace_type, compression_type, filter_type);
         /* num_passes may not be available below if interlace support is not
          * provided by libci for both read and write.
          */
         switch (interlace_type)
         {
            case CI_INTERLACE_NONE:
               num_passes = 1;
               break;

            case CI_INTERLACE_ADAM7:
               num_passes = 7;
               break;

            default:
               ci_error(read_ptr, "invalid interlace type");
               /*NOT REACHED*/
         }
      }

      else
         ci_error(read_ptr, "ci_get_IHDR failed");
   }
#ifdef CI_FIXED_POINT_SUPPORTED
#ifdef CI_cHRM_SUPPORTED
   {
      ci_fixed_point white_x, white_y, red_x, red_y, green_x, green_y, blue_x,
          blue_y;

      if (ci_get_cHRM_fixed(read_ptr, read_info_ptr, &white_x, &white_y,
          &red_x, &red_y, &green_x, &green_y, &blue_x, &blue_y) != 0)
      {
         ci_set_cHRM_fixed(write_ptr, write_info_ptr, white_x, white_y, red_x,
             red_y, green_x, green_y, blue_x, blue_y);
      }
   }
#endif
#ifdef CI_gAMA_SUPPORTED
   {
      ci_fixed_point gamma;

      if (ci_get_gAMA_fixed(read_ptr, read_info_ptr, &gamma) != 0)
         ci_set_gAMA_fixed(write_ptr, write_info_ptr, gamma);
   }
#endif
#ifdef CI_cLLI_SUPPORTED
   {
      ci_uint_32 maxCLL;
      ci_uint_32 maxFALL;

      if (ci_get_cLLI_fixed(read_ptr, read_info_ptr, &maxCLL, &maxFALL) != 0)
         ci_set_cLLI_fixed(write_ptr, write_info_ptr, maxCLL, maxFALL);
   }
#endif
#ifdef CI_mDCV_SUPPORTED
   {
      ci_fixed_point white_x, white_y, red_x, red_y, green_x, green_y, blue_x,
          blue_y;
      ci_uint_32 maxDL;
      ci_uint_32 minDL;

      if (ci_get_mDCV_fixed(read_ptr, read_info_ptr, &white_x, &white_y,
               &red_x, &red_y, &green_x, &green_y, &blue_x, &blue_y,
               &maxDL, &minDL) != 0)
         ci_set_mDCV_fixed(write_ptr, write_info_ptr, white_x, white_y,
               red_x, red_y, green_x, green_y, blue_x, blue_y,
               maxDL, minDL);
   }
#endif
#else /* Use floating point versions */
#ifdef CI_FLOATING_POINT_SUPPORTED
#ifdef CI_cHRM_SUPPORTED
   {
      double white_x, white_y, red_x, red_y, green_x, green_y, blue_x,
          blue_y;

      if (ci_get_cHRM(read_ptr, read_info_ptr, &white_x, &white_y, &red_x,
          &red_y, &green_x, &green_y, &blue_x, &blue_y) != 0)
      {
         ci_set_cHRM(write_ptr, write_info_ptr, white_x, white_y, red_x,
             red_y, green_x, green_y, blue_x, blue_y);
      }
   }
#endif
#ifdef CI_gAMA_SUPPORTED
   {
      double gamma;

      if (ci_get_gAMA(read_ptr, read_info_ptr, &gamma) != 0)
         ci_set_gAMA(write_ptr, write_info_ptr, gamma);
   }
#endif
#ifdef CI_cLLI_SUPPORTED
   {
      double maxCLL;
      double maxFALL;

      if (ci_get_cLLI(read_ptr, read_info_ptr, &maxCLL, &maxFALL) != 0)
         ci_set_cLLI(write_ptr, write_info_ptr, maxCLL, maxFALL);
   }
#endif
#ifdef CI_mDCV_SUPPORTED
   {
      double white_x, white_y, red_x, red_y, green_x, green_y, blue_x, blue_y;
      double maxDL;
      double minDL;

      if (ci_get_mDCV(read_ptr, read_info_ptr, &white_x, &white_y,
               &red_x, &red_y, &green_x, &green_y, &blue_x, &blue_y,
               &maxDL, &minDL) != 0)
         ci_set_mDCV(write_ptr, write_info_ptr, white_x, white_y,
               red_x, red_y, green_x, green_y, blue_x, blue_y,
               maxDL, minDL);
   }
#endif
#endif /* Floating point */
#endif /* Fixed point */
#ifdef CI_cICP_SUPPORTED
   {
      ci_byte colour_primaries;
      ci_byte transfer_function;
      ci_byte matrix_coefficients;
      ci_byte video_full_range_flag;

      if (ci_get_cICP(read_ptr, read_info_ptr,
                       &colour_primaries, &transfer_function,
                       &matrix_coefficients, &video_full_range_flag) != 0)
         ci_set_cICP(write_ptr, write_info_ptr,
                      colour_primaries, transfer_function,
                      matrix_coefficients, video_full_range_flag);
   }
#endif
#ifdef CI_iCCP_SUPPORTED
   {
      ci_charp name;
      ci_bytep profile;
      ci_uint_32 proflen;
      int compression_type;

      if (ci_get_iCCP(read_ptr, read_info_ptr, &name, &compression_type,
          &profile, &proflen) != 0)
      {
         ci_set_iCCP(write_ptr, write_info_ptr, name, compression_type,
             profile, proflen);
      }
   }
#endif
#ifdef CI_sRGB_SUPPORTED
   {
      int intent;

      if (ci_get_sRGB(read_ptr, read_info_ptr, &intent) != 0)
         ci_set_sRGB(write_ptr, write_info_ptr, intent);
   }
#endif
   {
      ci_colorp palette;
      int num_palette;

      if (ci_get_PLTE(read_ptr, read_info_ptr, &palette, &num_palette) != 0)
         ci_set_PLTE(write_ptr, write_info_ptr, palette, num_palette);
   }
#ifdef CI_bKGD_SUPPORTED
   {
      ci_color_16p background;

      if (ci_get_bKGD(read_ptr, read_info_ptr, &background) != 0)
         ci_set_bKGD(write_ptr, write_info_ptr, background);
   }
#endif
#ifdef CI_READ_eXIf_SUPPORTED
   {
      ci_bytep exif = NULL;
      ci_uint_32 exif_length;

      if (ci_get_eXIf_1(read_ptr, read_info_ptr, &exif_length, &exif) != 0)
      {
         if (exif_length > 1)
            fprintf(STDERR," eXIf type %c%c, %lu bytes\n",exif[0],exif[1],
               (unsigned long)exif_length);
# ifdef CI_WRITE_eXIf_SUPPORTED
         ci_set_eXIf_1(write_ptr, write_info_ptr, exif_length, exif);
# endif
      }
   }
#endif
#ifdef CI_hIST_SUPPORTED
   {
      ci_uint_16p hist;

      if (ci_get_hIST(read_ptr, read_info_ptr, &hist) != 0)
         ci_set_hIST(write_ptr, write_info_ptr, hist);
   }
#endif
#ifdef CI_oFFs_SUPPORTED
   {
      ci_int_32 offset_x, offset_y;
      int unit_type;

      if (ci_get_oFFs(read_ptr, read_info_ptr, &offset_x, &offset_y,
          &unit_type) != 0)
         ci_set_oFFs(write_ptr, write_info_ptr, offset_x, offset_y, unit_type);
   }
#endif
#ifdef CI_pCAL_SUPPORTED
   {
      ci_charp purpose, units;
      ci_charpp params;
      ci_int_32 X0, X1;
      int type, nparams;

      if (ci_get_pCAL(read_ptr, read_info_ptr, &purpose, &X0, &X1, &type,
          &nparams, &units, &params) != 0)
         ci_set_pCAL(write_ptr, write_info_ptr, purpose, X0, X1, type,
             nparams, units, params);
   }
#endif
#ifdef CI_pHYs_SUPPORTED
   {
      ci_uint_32 res_x, res_y;
      int unit_type;

      if (ci_get_pHYs(read_ptr, read_info_ptr, &res_x, &res_y,
          &unit_type) != 0)
         ci_set_pHYs(write_ptr, write_info_ptr, res_x, res_y, unit_type);
   }
#endif
#ifdef CI_sBIT_SUPPORTED
   {
      ci_color_8p sig_bit;

      if (ci_get_sBIT(read_ptr, read_info_ptr, &sig_bit) != 0)
         ci_set_sBIT(write_ptr, write_info_ptr, sig_bit);
   }
#endif
#ifdef CI_sCAL_SUPPORTED
#if defined(CI_FLOATING_POINT_SUPPORTED) && \
   defined(CI_FLOATING_ARITHMETIC_SUPPORTED)
   {
      int unit;
      double scal_width, scal_height;

      if (ci_get_sCAL(read_ptr, read_info_ptr, &unit, &scal_width,
          &scal_height) != 0)
         ci_set_sCAL(write_ptr, write_info_ptr, unit, scal_width, scal_height);
   }
#else
#ifdef CI_FIXED_POINT_SUPPORTED
   {
      int unit;
      ci_charp scal_width, scal_height;

      if (ci_get_sCAL_s(read_ptr, read_info_ptr, &unit, &scal_width,
           &scal_height) != 0)
      {
         ci_set_sCAL_s(write_ptr, write_info_ptr, unit, scal_width,
             scal_height);
      }
   }
#endif
#endif
#endif

#ifdef CI_sPLT_SUPPORTED
   {
       ci_sPLT_tp entries;

       int num_entries = ci_get_sPLT(read_ptr, read_info_ptr, &entries);
       if (num_entries != 0)
           ci_set_sPLT(write_ptr, write_info_ptr, entries, num_entries);
   }
#endif

#ifdef CI_TEXT_SUPPORTED
   {
      ci_textp text_ptr;
      int num_text;

      if (ci_get_text(read_ptr, read_info_ptr, &text_ptr, &num_text) > 0)
      {
         citest_debug1("Handling %d iTXt/tEXt/zTXt chunks", num_text);

         citest_check_text_support(read_ptr, text_ptr, num_text);

         if (verbose != 0)
         {
            int i;

            fprintf(STDERR,"\n");
            for (i = 0; i < num_text; i++)
            {
               fprintf(STDERR,"   Text compression[%d]=%d\n",
                   i, text_ptr[i].compression);
            }
         }

         ci_set_text(write_ptr, write_info_ptr, text_ptr, num_text);
      }
   }
#endif
#ifdef CI_tIME_SUPPORTED
   {
      ci_timep mod_time;

      if (ci_get_tIME(read_ptr, read_info_ptr, &mod_time) != 0)
      {
         ci_set_tIME(write_ptr, write_info_ptr, mod_time);
#ifdef CI_TIME_RFC1123_SUPPORTED
         if (ci_convert_to_rfc1123_buffer(tIME_string, mod_time) != 0)
            tIME_string[(sizeof tIME_string) - 1] = '\0';

         else
         {
            strncpy(tIME_string, "*** invalid time ***", (sizeof tIME_string));
            tIME_string[(sizeof tIME_string) - 1] = '\0';
         }

         tIME_chunk_present++;
#endif /* TIME_RFC1123 */
      }
   }
#endif
#ifdef CI_tRNS_SUPPORTED
   {
      ci_bytep trans_alpha;
      int num_trans;
      ci_color_16p trans_color;

      if (ci_get_tRNS(read_ptr, read_info_ptr, &trans_alpha, &num_trans,
          &trans_color) != 0)
      {
         int sample_max = (1 << bit_depth);
         /* libci doesn't reject a tRNS chunk with out-of-range samples */
         if (!((color_type == CI_COLOR_TYPE_GRAY &&
             (int)trans_color->gray > sample_max) ||
             (color_type == CI_COLOR_TYPE_RGB &&
             ((int)trans_color->red > sample_max ||
             (int)trans_color->green > sample_max ||
             (int)trans_color->blue > sample_max))))
            ci_set_tRNS(write_ptr, write_info_ptr, trans_alpha, num_trans,
               trans_color);
      }
   }
#endif
#ifdef CI_WRITE_UNKNOWN_CHUNKS_SUPPORTED
   {
      ci_unknown_chunkp unknowns;
      int num_unknowns = ci_get_unknown_chunks(read_ptr, read_info_ptr,
          &unknowns);

      if (num_unknowns != 0)
         ci_set_unknown_chunks(write_ptr, write_info_ptr, unknowns,
             num_unknowns);
   }
#endif

#ifdef CI_WRITE_SUPPORTED
   citest_debug("Writing info struct");

   /* Write the info in two steps so that if we write the 'unknown' chunks here
    * they go to the correct place.
    */
   ci_write_info_before_PLTE(write_ptr, write_info_ptr);

   write_chunks(write_ptr, &my_user_chunk_data, before_PLTE); /* before PLTE */

   ci_write_info(write_ptr, write_info_ptr);

   write_chunks(write_ptr, &my_user_chunk_data, before_IDAT); /* after PLTE */

   ci_write_info(write_ptr, write_end_info_ptr);

   write_chunks(write_ptr, &my_user_chunk_data, after_IDAT); /* after IDAT */

#ifdef CI_COMPRESSION_COMPAT
   /* Test the 'compatibility' setting here, if it is available. */
   ci_set_compression(write_ptr, CI_COMPRESSION_COMPAT);
#endif
#endif

   citest_debug("Writing row data");

#if defined(CI_READ_INTERLACING_SUPPORTED) &&\
   defined(CI_WRITE_INTERLACING_SUPPORTED)
   /* Both must be defined for libci to be able to handle the interlace,
    * otherwise it gets handled below by simply reading and writing the passes
    * directly.
    */
   if (ci_set_interlace_handling(read_ptr) != num_passes)
      ci_error(write_ptr,
          "ci_set_interlace_handling(read): wrong pass count ");
   if (ci_set_interlace_handling(write_ptr) != num_passes)
      ci_error(write_ptr,
          "ci_set_interlace_handling(write): wrong pass count ");
#else /* ci_set_interlace_handling not called on either read or write */
#  define calc_pass_height
#endif /* not using libci interlace handling */

#ifdef CITEST_TIMING
   t_stop = (float)clock();
   t_misc += (t_stop - t_start);
   t_start = t_stop;
#endif
   for (pass = 0; pass < num_passes; pass++)
   {
#     ifdef calc_pass_height
         ci_uint_32 pass_height;

         if (num_passes == 7) /* interlaced */
         {
            if (CI_PASS_COLS(width, pass) > 0)
               pass_height = CI_PASS_ROWS(height, pass);

            else
               pass_height = 0;
         }

         else /* not interlaced */
            pass_height = height;
#     else
#        define pass_height height
#     endif

      citest_debug1("Writing row data for pass %d", pass);
      for (y = 0; y < pass_height; y++)
      {
         citest_debug2("Allocating row buffer (pass %d, y = %u)...", pass, y);

         row_buf = (ci_bytep)ci_malloc(read_ptr,
             ci_get_rowbytes(read_ptr, read_info_ptr));

         citest_debug2("\t%p (%lu bytes)", row_buf,
             (unsigned long)ci_get_rowbytes(read_ptr, read_info_ptr));

         ci_read_rows(read_ptr, (ci_bytepp)&row_buf, NULL, 1);

#ifdef CI_WRITE_SUPPORTED
#ifdef CITEST_TIMING
         t_stop = (float)clock();
         t_decode += (t_stop - t_start);
         t_start = t_stop;
#endif
         ci_write_rows(write_ptr, (ci_bytepp)&row_buf, 1);
#ifdef CITEST_TIMING
         t_stop = (float)clock();
         t_encode += (t_stop - t_start);
         t_start = t_stop;
#endif
#endif /* WRITE */

         citest_debug2("Freeing row buffer (pass %d, y = %u)", pass, y);
         ci_free(read_ptr, row_buf);
         row_buf = NULL;
      }
   }

#ifdef CI_STORE_UNKNOWN_CHUNKS_SUPPORTED
#  ifdef CI_READ_UNKNOWN_CHUNKS_SUPPORTED
      ci_free_data(read_ptr, read_info_ptr, CI_FREE_UNKN, -1);
#  endif
#  ifdef CI_WRITE_UNKNOWN_CHUNKS_SUPPORTED
      ci_free_data(write_ptr, write_info_ptr, CI_FREE_UNKN, -1);
#  endif
#endif

   citest_debug("Reading and writing end_info data");

   ci_read_end(read_ptr, end_info_ptr);
#ifdef CI_TEXT_SUPPORTED
   {
      ci_textp text_ptr;
      int num_text;

      if (ci_get_text(read_ptr, end_info_ptr, &text_ptr, &num_text) > 0)
      {
         citest_debug1("Handling %d iTXt/tEXt/zTXt chunks", num_text);

         citest_check_text_support(read_ptr, text_ptr, num_text);

         if (verbose != 0)
         {
            int i;

            fprintf(STDERR,"\n");
            for (i = 0; i < num_text; i++)
            {
               fprintf(STDERR,"   Text compression[%d]=%d\n",
                   i, text_ptr[i].compression);
            }
         }

         ci_set_text(write_ptr, write_end_info_ptr, text_ptr, num_text);
      }
   }
#endif
#ifdef CI_READ_eXIf_SUPPORTED
   {
      ci_bytep exif = NULL;
      ci_uint_32 exif_length;

      if (ci_get_eXIf_1(read_ptr, end_info_ptr, &exif_length, &exif) != 0)
      {
         if (exif_length > 1)
            fprintf(STDERR," eXIf type %c%c, %lu bytes\n",exif[0],exif[1],
               (unsigned long)exif_length);
# ifdef CI_WRITE_eXIf_SUPPORTED
         ci_set_eXIf_1(write_ptr, write_end_info_ptr, exif_length, exif);
# endif
      }
   }
#endif
#ifdef CI_tIME_SUPPORTED
   {
      ci_timep mod_time;

      if (ci_get_tIME(read_ptr, end_info_ptr, &mod_time) != 0)
      {
         ci_set_tIME(write_ptr, write_end_info_ptr, mod_time);
#ifdef CI_TIME_RFC1123_SUPPORTED
         if (ci_convert_to_rfc1123_buffer(tIME_string, mod_time) != 0)
            tIME_string[(sizeof tIME_string) - 1] = '\0';

         else
         {
            strncpy(tIME_string, "*** invalid time ***", sizeof tIME_string);
            tIME_string[(sizeof tIME_string)-1] = '\0';
         }

         tIME_chunk_present++;
#endif /* TIME_RFC1123 */
      }
   }
#endif
#ifdef CI_WRITE_UNKNOWN_CHUNKS_SUPPORTED
   {
      ci_unknown_chunkp unknowns;
      int num_unknowns = ci_get_unknown_chunks(read_ptr, end_info_ptr,
          &unknowns);

      if (num_unknowns != 0)
         ci_set_unknown_chunks(write_ptr, write_end_info_ptr, unknowns,
             num_unknowns);
   }
#endif

#ifdef CI_WRITE_SUPPORTED
#ifdef CI_WRITE_CUSTOMIZE_ZTXT_COMPRESSION_SUPPORTED
   /* Normally one would use Z_DEFAULT_STRATEGY for text compression.
    * This is here just to make citest replicate the results from libci
    * versions prior to 1.5.4, and to test this new API.
    */
   ci_set_text_compression_strategy(write_ptr, Z_FILTERED);
#endif

   /* When the unknown vpAg/sTER chunks are written by citest the only way to
    * do it is to write them *before* calling ci_write_end.  When unknown
    * chunks are written by libci, however, they are written just before IEND.
    * There seems to be no way round this, however vpAg/sTER are not expected
    * after IDAT.
    */
   write_chunks(write_ptr, &my_user_chunk_data, after_IDAT);

   ci_write_end(write_ptr, write_end_info_ptr);
#endif

#ifdef CI_EASY_ACCESS_SUPPORTED
   if (verbose != 0)
   {
      ci_uint_32 iwidth, iheight;
      iwidth = ci_get_image_width(write_ptr, write_info_ptr);
      iheight = ci_get_image_height(write_ptr, write_info_ptr);
      fprintf(STDERR, "\n Image width = %lu, height = %lu\n",
          (unsigned long)iwidth, (unsigned long)iheight);
   }
#endif

   citest_debug("Destroying data structs");
   citest_debug("Destroying read_ptr, read_info_ptr, end_info_ptr");
   ci_destroy_read_struct(&read_ptr, &read_info_ptr, &end_info_ptr);
#ifdef CI_WRITE_SUPPORTED
   citest_debug("Destroying write_end_info_ptr");
   ci_destroy_info_struct(write_ptr, &write_end_info_ptr);
   citest_debug("Destroying write_ptr, write_info_ptr");
   ci_destroy_write_struct(&write_ptr, &write_info_ptr);
#endif
   citest_debug("Destruction complete.");

   fclose(fpin);
   fclose(fpout);

   /* Summarize any warnings or errors and in 'strict' mode fail the test.
    * Unsupported chunks can result in warnings, in that case ignore the strict
    * setting, otherwise fail the test on warnings as well as errors.
    */
   if (error_count > 0)
   {
      /* We don't really expect to get here because of the setjmp handling
       * above, but this is safe.
       */
      fprintf(STDERR, "\n  %s: %d libci errors found (%d warnings)",
          inname, error_count, warning_count);

      if (strict != 0)
         return 1;
   }

#  ifdef CI_WRITE_SUPPORTED
      /* If there is no write support nothing was written! */
      else if (unsupported_chunks > 0)
      {
         fprintf(STDERR, "\n  %s: unsupported chunks (%d)%s",
             inname, unsupported_chunks, strict ? ": IGNORED --strict!" : "");
      }
#  endif

   else if (warning_count > 0)
   {
      fprintf(STDERR, "\n  %s: %d libci warnings found",
          inname, warning_count);

      if (strict != 0)
         return 1;
   }

   citest_debug("Opening files for comparison");
   if ((fpin = fopen(inname, "rb")) == NULL)
   {
      fprintf(STDERR, "Could not find file %s\n", inname);
      return 1;
   }

   if ((fpout = fopen(outname, "rb")) == NULL)
   {
      fprintf(STDERR, "Could not find file %s\n", outname);
      fclose(fpin);
      return 1;
   }

#if defined (CI_WRITE_SUPPORTED) /* else nothing was written */ &&\
    defined (CI_WRITE_FILTER_SUPPORTED)
   if (interlace_preserved != 0) /* else the files will be changed */
   {
      for (;;)
      {
         static int wrote_question = 0;
         size_t num_in, num_out;
         char inbuf[256], outbuf[256];

         num_in = fread(inbuf, 1, sizeof inbuf, fpin);
         num_out = fread(outbuf, 1, sizeof outbuf, fpout);

         if (num_in != num_out)
         {
            fprintf(STDERR, "\nFiles %s and %s are of a different size\n",
                inname, outname);

            if (wrote_question == 0 && unsupported_chunks == 0)
            {
               fprintf(STDERR,
                   "   Was %s written with the same maximum IDAT"
                   " chunk size (%d bytes),",
                   inname, CI_ZBUF_SIZE);
               fprintf(STDERR,
                   "\n   filtering heuristic (libci default), compression");
               fprintf(STDERR,
                   " level (zlib default),\n   and zlib version (%s)?\n\n",
                   ZLIB_VERSION);
               wrote_question = 1;
            }

            fclose(fpin);
            fclose(fpout);

            if (strict != 0 && unsupported_chunks == 0)
              return 1;

            else
              return 0;
         }

         if (num_in == 0)
            break;

         if (memcmp(inbuf, outbuf, num_in))
         {
            fprintf(STDERR, "\nFiles %s and %s are different\n", inname,
                outname);

            if (wrote_question == 0 && unsupported_chunks == 0)
            {
               fprintf(STDERR,
                   "   Was %s written with the same maximum"
                   " IDAT chunk size (%d bytes),",
                    inname, CI_ZBUF_SIZE);
               fprintf(STDERR,
                   "\n   filtering heuristic (libci default), compression");
               fprintf(STDERR,
                   " level (zlib default),\n   and zlib version (%s)?\n\n",
                 ZLIB_VERSION);
               wrote_question = 1;
            }

            fclose(fpin);
            fclose(fpout);

            /* NOTE: the unsupported_chunks escape is permitted here because
             * unsupported text chunk compression will result in the compression
             * mode being changed (to NONE) yet, in the test case, the result
             * can be exactly the same size!
             */
            if (strict != 0 && unsupported_chunks == 0)
              return 1;

            else
              return 0;
         }
      }
   }
#endif /* WRITE && WRITE_FILTER */

   fclose(fpin);
   fclose(fpout);

   return 0;
}

/* Input and output filenames */
#ifdef RISCOS
static const char *inname = "citest/ci";
static const char *outname = "ciout/ci";
#else
static const char *inname = "citest.ci";
static const char *outname = "ciout.ci";
#endif

int
main(int argc, char *argv[])
{
   int multiple = 0;
   int ierror = 0;

   ci_structp dummy_ptr;

   fprintf(STDERR, "\n Testing libci version %s\n", CI_LIBCI_VER_STRING);
   fprintf(STDERR, "   with zlib   version %s\n", ZLIB_VERSION);
   fprintf(STDERR, "%s", ci_get_copyright(NULL));
   /* Show the version of libci used in building the library */
   fprintf(STDERR, " library (%lu):%s",
       (unsigned long)ci_access_version_number(),
       ci_get_header_version(NULL));

   /* Show the version of libci used in building the application */
   fprintf(STDERR, " citest (%lu):%s", (unsigned long)CI_LIBCI_VER,
       CI_HEADER_VERSION_STRING);

   /* Do some consistency checking on the memory allocation settings, I'm
    * not sure this matters, but it is nice to know, the first of these
    * tests should be impossible because of the way the macros are set
    * in ciconf.h
    */
#if defined(MAXSEG_64K) && !defined(CI_MAX_MALLOC_64K)
      fprintf(STDERR, " NOTE: Zlib compiled for max 64k, libci not\n");
#endif
   /* I think the following can happen. */
#if !defined(MAXSEG_64K) && defined(CI_MAX_MALLOC_64K)
      fprintf(STDERR, " NOTE: libci compiled for max 64k, zlib not\n");
#endif

   if (strcmp(ci_libci_ver, CI_LIBCI_VER_STRING) != 0)
   {
      fprintf(STDERR, "Warning: mismatching versions of ci.h and ci.c\n");
      fprintf(STDERR, "  ci.h version string: %s\n", CI_LIBCI_VER_STRING);
      fprintf(STDERR, "  ci.c version string: %s\n\n", ci_libci_ver);
      ++ierror;
   }

   if (argc > 1)
   {
      if (strcmp(argv[1], "-m") == 0)
      {
         multiple = 1;
         status_dots_requested = 0;
      }

      else if (strcmp(argv[1], "-mv") == 0 ||
               strcmp(argv[1], "-vm") == 0 )
      {
         multiple = 1;
         verbose = 1;
         status_dots_requested = 1;
      }

      else if (strcmp(argv[1], "-v") == 0)
      {
         verbose = 1;
         status_dots_requested = 1;
         inname = argv[2];
      }

      else if (strcmp(argv[1], "--strict") == 0)
      {
         status_dots_requested = 0;
         verbose = 1;
         inname = argv[2];
         strict++;
         relaxed = 0;
         multiple = 1;
      }

      else if (strcmp(argv[1], "--relaxed") == 0)
      {
         status_dots_requested = 0;
         verbose = 1;
         inname = argv[2];
         strict = 0;
         relaxed++;
         multiple = 1;
      }
      else if (strcmp(argv[1], "--xfail") == 0)
      {
         status_dots_requested = 0;
         verbose = 1;
         inname = argv[2];
         strict = 0;
         xfail++;
         relaxed++;
         multiple = 1;
      }

      else
      {
         inname = argv[1];
         status_dots_requested = 0;
      }
   }

   if (multiple == 0 && argc == 3 + verbose)
      outname = argv[2 + verbose];

   if ((multiple == 0 && argc > 3 + verbose) ||
       (multiple != 0 && argc < 2))
   {
      fprintf(STDERR,
          "usage: %s [infile.ci] [outfile.ci]\n\t%s -m {infile.ci}\n",
          argv[0], argv[0]);
      fprintf(STDERR,
          "  reads/writes one CI file (without -m) or multiple files (-m)\n");
      fprintf(STDERR,
          "  with -m %s is used as a temporary file\n", outname);
      exit(1);
   }

   if (multiple != 0)
   {
      int i;
#if defined(CI_USER_MEM_SUPPORTED) && CI_DEBUG
      int allocation_now = current_allocation;
#endif
      for (i = 2; i < argc; ++i)
      {
         int kerror;
         fprintf(STDERR, "\n Testing %s:", argv[i]);
#if CI_DEBUG > 0
         fprintf(STDERR, "\n");
#endif
         kerror = test_one_file(argv[i], outname);
         if (kerror == 0)
         {
#ifdef CI_WRITE_USER_TRANSFORM_SUPPORTED
            fprintf(STDERR, "\n PASS (%lu zero samples)\n",
                (unsigned long)zero_samples);
#else
            fprintf(STDERR, " PASS\n");
#endif
#ifdef CI_TIME_RFC1123_SUPPORTED
            if (tIME_chunk_present != 0)
               fprintf(STDERR, " tIME = %s\n", tIME_string);

            tIME_chunk_present = 0;
#endif /* TIME_RFC1123 */
         }

         else
         {
            if (xfail)
              fprintf(STDERR, " XFAIL\n");
            else
            {
              fprintf(STDERR, " FAIL\n");
              ierror += kerror;
            }
         }
#if defined(CI_USER_MEM_SUPPORTED) && CI_DEBUG
         if (allocation_now != current_allocation)
            fprintf(STDERR, "MEMORY ERROR: %d bytes lost\n",
                current_allocation - allocation_now);

         if (current_allocation != 0)
         {
            memory_infop pinfo = pinformation;

            fprintf(STDERR, "MEMORY ERROR: %d bytes still allocated\n",
                current_allocation);

            while (pinfo != NULL)
            {
               fprintf(STDERR, " %lu bytes at %p\n",
                   (unsigned long)pinfo->size,
                   pinfo->pointer);
               pinfo = pinfo->next;
            }
         }
#endif
      }
#if defined(CI_USER_MEM_SUPPORTED) && CI_DEBUG
         fprintf(STDERR, " Current memory allocation: %10d bytes\n",
             current_allocation);
         fprintf(STDERR, " Maximum memory allocation: %10d bytes\n",
             maximum_allocation);
         fprintf(STDERR, " Total   memory allocation: %10d bytes\n",
             total_allocation);
         fprintf(STDERR, "     Number of allocations: %10d\n",
             num_allocations);
#endif
   }

   else
   {
      int i;
      for (i = 0; i < 3; ++i)
      {
         int kerror;
#if defined(CI_USER_MEM_SUPPORTED) && CI_DEBUG
         int allocation_now = current_allocation;
#endif
         if (i == 1)
            status_dots_requested = 1;

         else if (verbose == 0)
            status_dots_requested = 0;

         if (i == 0 || verbose == 1 || ierror != 0)
         {
            fprintf(STDERR, "\n Testing %s:", inname);
#if CI_DEBUG > 0
            fprintf(STDERR, "\n");
#endif
         }

         kerror = test_one_file(inname, outname);

         if (kerror == 0)
         {
            if (verbose == 1 || i == 2)
            {
#ifdef CI_WRITE_USER_TRANSFORM_SUPPORTED
                fprintf(STDERR, "\n PASS (%lu zero samples)\n",
                    (unsigned long)zero_samples);
#else
                fprintf(STDERR, " PASS\n");
#endif
#ifdef CI_TIME_RFC1123_SUPPORTED
             if (tIME_chunk_present != 0)
                fprintf(STDERR, " tIME = %s\n", tIME_string);
#endif /* TIME_RFC1123 */
            }
         }

         else
         {
            if (verbose == 0 && i != 2)
            {
               fprintf(STDERR, "\n Testing %s:", inname);
#if CI_DEBUG > 0
               fprintf(STDERR, "\n");
#endif
            }

            if (xfail)
              fprintf(STDERR, " XFAIL\n");
            else
            {
              fprintf(STDERR, " FAIL\n");
              ierror += kerror;
            }
         }
#if defined(CI_USER_MEM_SUPPORTED) && CI_DEBUG
         if (allocation_now != current_allocation)
             fprintf(STDERR, "MEMORY ERROR: %d bytes lost\n",
                 current_allocation - allocation_now);

         if (current_allocation != 0)
         {
             memory_infop pinfo = pinformation;

             fprintf(STDERR, "MEMORY ERROR: %d bytes still allocated\n",
                 current_allocation);

             while (pinfo != NULL)
             {
                fprintf(STDERR, " %lu bytes at %p\n",
                    (unsigned long)pinfo->size, pinfo->pointer);
                pinfo = pinfo->next;
             }
          }
#endif
       }
#if defined(CI_USER_MEM_SUPPORTED) && CI_DEBUG
       fprintf(STDERR, " Current memory allocation: %10d bytes\n",
           current_allocation);
       fprintf(STDERR, " Maximum memory allocation: %10d bytes\n",
           maximum_allocation);
       fprintf(STDERR, " Total   memory allocation: %10d bytes\n",
           total_allocation);
       fprintf(STDERR, "     Number of allocations: %10d\n",
           num_allocations);
#endif
   }

#ifdef CITEST_TIMING
   t_stop = (float)clock();
   t_misc += (t_stop - t_start);
   t_start = t_stop;
   fprintf(STDERR, " CPU time used = %.3f seconds",
       (t_misc+t_decode+t_encode)/(float)CLOCKS_PER_SEC);
   fprintf(STDERR, " (decoding %.3f,\n",
       t_decode/(float)CLOCKS_PER_SEC);
   fprintf(STDERR, "        encoding %.3f ,",
       t_encode/(float)CLOCKS_PER_SEC);
   fprintf(STDERR, " other %.3f seconds)\n\n",
       t_misc/(float)CLOCKS_PER_SEC);
#endif

   if (ierror == 0)
      fprintf(STDERR, " libci passes test\n");

   else
      fprintf(STDERR, " libci FAILS test\n");

   dummy_ptr = ci_create_read_struct(CI_LIBCI_VER_STRING, NULL, NULL, NULL);
#ifdef CI_USER_LIMITS_SUPPORTED
   fprintf(STDERR, " Default limits:\n");
   fprintf(STDERR, "  width_max  = %lu\n",
       (unsigned long) ci_get_user_width_max(dummy_ptr));
   fprintf(STDERR, "  height_max = %lu\n",
       (unsigned long) ci_get_user_height_max(dummy_ptr));
   if (ci_get_chunk_cache_max(dummy_ptr) == 0)
      fprintf(STDERR, "  cache_max  = unlimited\n");
   else
      fprintf(STDERR, "  cache_max  = %lu\n",
          (unsigned long) ci_get_chunk_cache_max(dummy_ptr));
   if (ci_get_chunk_malloc_max(dummy_ptr) == 0)
      fprintf(STDERR, "  malloc_max = unlimited\n");
   else
      fprintf(STDERR, "  malloc_max = %lu\n",
          (unsigned long) ci_get_chunk_malloc_max(dummy_ptr));
#endif
   ci_destroy_read_struct(&dummy_ptr, NULL, NULL);

   return (ierror != 0);
}
#else
int
main(void)
{
   fprintf(STDERR,
       " test ignored because libci was not built with read support\n");
   /* And skip this test */
   return SKIP;
}
#endif

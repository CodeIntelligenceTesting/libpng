/* readci.c
 *
 * Copyright (c) 2013 John Cunningham Bowler
 *
 * This code is released under the libci license.
 * For conditions of distribution and use, see the disclaimer
 * and license in ci.h
 *
 * Load an arbitrary number of CI files (from the command line, or, if there
 * are no arguments on the command line, from stdin) then run a time test by
 * reading each file by row.  The test does nothing with the read result and
 * does no transforms.  The only output is a time as a floating point number of
 * seconds with 9 decimal digits.
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#if defined(HAVE_CONFIG_H) && !defined(CI_NO_CONFIG_H)
#  include <config.h>
#endif

/* Define the following to use this test against your installed libci, rather
 * than the one being built here:
 */
#ifdef CI_FREESTANDING_TESTS
#  include <ci.h>
#else
#  include "../../ci.h"
#endif

static int
read_ci(FILE *fp)
{
   ci_structp ci_ptr = ci_create_read_struct(CI_LIBCI_VER_STRING,0,0,0);
   ci_infop info_ptr = NULL;
   ci_bytep row = NULL, display = NULL;

   if (ci_ptr == NULL)
      return 0;

   if (setjmp(ci_jmpbuf(ci_ptr)))
   {
      ci_destroy_read_struct(&ci_ptr, &info_ptr, NULL);
      if (row != NULL) free(row);
      if (display != NULL) free(display);
      return 0;
   }

   ci_init_io(ci_ptr, fp);

   info_ptr = ci_create_info_struct(ci_ptr);
   if (info_ptr == NULL)
      ci_error(ci_ptr, "OOM allocating info structure");

   ci_set_keep_unknown_chunks(ci_ptr, CI_HANDLE_CHUNK_ALWAYS, NULL, 0);

   ci_read_info(ci_ptr, info_ptr);

   {
      size_t rowbytes = ci_get_rowbytes(ci_ptr, info_ptr);

      /* Failure to initialize these is harmless */
      row = malloc(rowbytes);
      display = malloc(rowbytes);

      if (row == NULL || display == NULL)
         ci_error(ci_ptr, "OOM allocating row buffers");

      {
         ci_uint_32 height = ci_get_image_height(ci_ptr, info_ptr);
#        ifdef CI_READ_INTERLACING_SUPPORTED
            int passes = ci_set_interlace_handling(ci_ptr);
#        else /* !READ_INTERLACING */
            int passes = ci_get_interlace_type(ci_ptr, info_ptr) ==
               CI_INTERLACE_ADAM7 ? CI_INTERLACE_ADAM7_PASSES : 1;
#        endif /* !READ_INTERLACING */
         int pass;

         ci_start_read_image(ci_ptr);

         for (pass = 0; pass < passes; ++pass)
         {
            ci_uint_32 y = height;

#           ifndef CI_READ_INTERLACING_SUPPORTED
               if (passes == CI_INTERLACE_ADAM7_PASSES)
                  y = CI_PASS_ROWS(y, pass);
#           endif /* READ_INTERLACING */

            /* NOTE: this trashes the row each time; interlace handling won't
             * work, but this avoids memory thrashing for speed testing.
             */
            while (y-- > 0)
               ci_read_row(ci_ptr, row, display);
         }
      }
   }

   /* Make sure to read to the end of the file: */
   ci_read_end(ci_ptr, info_ptr);
   ci_destroy_read_struct(&ci_ptr, &info_ptr, NULL);
   free(row);
   free(display);
   return 1;
}

int
main(void)
{
   /* Exit code 0 on success. */
   return !read_ci(stdin);
}

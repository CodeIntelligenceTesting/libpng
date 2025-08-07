/*- cipixel
 *
 * COPYRIGHT: Written by John Cunningham Bowler, 2011.
 * To the extent possible under law, the author has waived all copyright and
 * related or neighboring rights to this work.  This work is published from:
 * United States.
 *
 * Read a single pixel value from a CI file.
 *
 * This code illustrates basic 'by-row' reading of a CI file using libci.
 * Rows are read until a particular pixel is found; the value of this pixel is
 * then printed on stdout.
 *
 * The code illustrates how to do this on interlaced as well as non-interlaced
 * images.  Normally you would call ci_set_interlace_handling() to have libci
 * deal with the interlace for you, but that obliges you to buffer half of the
 * image to assemble the interlaced rows.  In this code
 * ci_set_interlace_handling() is not called and, instead, the code handles
 * the interlace passes directly looking for the required pixel.
 */
#include <stdlib.h>
#include <stdio.h>
#include <setjmp.h> /* required for error handling */

/* Normally use <ci.h> here to get the installed libci, but this is done to
 * ensure the code picks up the local libci implementation:
 */
#include "../../ci.h"

#if !defined(CI_READ_SUPPORTED) || !defined(CI_SEQUENTIAL_READ_SUPPORTED)
#error This program requires libci supporting the read and sequential read API
#endif


/* Return component 'c' of pixel 'x' from the given row. */
static unsigned int
component(ci_const_bytep row, ci_uint_32 x, unsigned int c,
          unsigned int bit_depth, unsigned int channels)
{
   /* CI images can be up to 2^31 pixels wide, which means they can be up to
    * 2^37 bits wide (for a 64-bit pixel - the largest possible) and hence
    * 2^34 bytes wide.  Since the row fitted into memory, the following must
    * work:
    */
   ci_uint_32 bit_offset_hi = bit_depth * ((x >> 6) * channels);
   ci_uint_32 bit_offset_lo = bit_depth * ((x & 0x3f) * channels + c);

   row = (ci_const_bytep)(((const ci_byte(*)[8])row) + bit_offset_hi);
   row += bit_offset_lo >> 3;
   bit_offset_lo &= 0x07;

   /* CI pixels are packed into bytes to put the first pixel in the highest
    * bits of the byte, and into two bytes for 16-bit values with the high
    * 8 bits first, so:
    */
   switch (bit_depth)
   {
      case 1: return (row[0] >> (7 - bit_offset_lo)) & 0x01;
      case 2: return (row[0] >> (6 - bit_offset_lo)) & 0x03;
      case 4: return (row[0] >> (4 - bit_offset_lo)) & 0x0f;
      case 8: return row[0];
      case 16: return (row[0] << 8) + row[1];
      default:
         /* This should never happen; it indicates a bug in this program or in
          * libci itself:
          */
         fprintf(stderr, "cipixel: invalid bit depth %u\n", bit_depth);
         exit(1);
   }
}

/* Print a pixel from a row returned by libci; determine the row format, find
 * the pixel, and print the relevant information to stdout.
 */
static void
print_pixel(ci_structp ci_ptr, ci_infop info_ptr, ci_const_bytep row,
            ci_uint_32 x)
{
   unsigned int bit_depth = ci_get_bit_depth(ci_ptr, info_ptr);

   switch (ci_get_color_type(ci_ptr, info_ptr))
   {
      case CI_COLOR_TYPE_GRAY:
         printf("GRAY %u\n", component(row, x, 0, bit_depth, 1));
         return;

      /* The palette case is slightly more difficult - the palette and, if
       * present, the tRNS ('transparency', though the values are really
       * opacity) data must be read to give the full picture:
       */
      case CI_COLOR_TYPE_PALETTE:
         {
            int index = component(row, x, 0, bit_depth, 1);
            ci_colorp palette = NULL;
            int num_palette = 0;

            if ((ci_get_PLTE(ci_ptr, info_ptr, &palette, &num_palette) &
                 CI_INFO_PLTE) &&
                (num_palette > 0) &&
                (palette != NULL))
            {
               ci_bytep trans_alpha = NULL;
               int num_trans = 0;
               if ((ci_get_tRNS(ci_ptr, info_ptr, &trans_alpha, &num_trans,
                                 NULL) & CI_INFO_tRNS) &&
                   (num_trans > 0) &&
                   (trans_alpha != NULL))
                  printf("INDEXED %u = %d %d %d %d\n", index,
                         palette[index].red, palette[index].green,
                         palette[index].blue,
                         index < num_trans ? trans_alpha[index] : 255);

               else /* no transparency */
                  printf("INDEXED %u = %d %d %d\n", index, palette[index].red,
                         palette[index].green, palette[index].blue);
            }

            else
               printf("INDEXED %u = invalid index\n", index);
         }
         return;

      case CI_COLOR_TYPE_RGB:
         printf("RGB %u %u %u\n", component(row, x, 0, bit_depth, 3),
                component(row, x, 1, bit_depth, 3),
                component(row, x, 2, bit_depth, 3));
         return;

      case CI_COLOR_TYPE_GRAY_ALPHA:
         printf("GRAY+ALPHA %u %u\n", component(row, x, 0, bit_depth, 2),
                component(row, x, 1, bit_depth, 2));
         return;

      case CI_COLOR_TYPE_RGB_ALPHA:
         printf("RGBA %u %u %u %u\n", component(row, x, 0, bit_depth, 4),
                component(row, x, 1, bit_depth, 4),
                component(row, x, 2, bit_depth, 4),
                component(row, x, 3, bit_depth, 4));
         return;

      default:
         ci_error(ci_ptr, "cipixel: invalid color type");
   }
}

int
main(int argc, const char **argv)
{
   /* This program uses the default, <setjmp.h> based, libci error handling
    * mechanism, therefore any local variable that exists before the call to
    * setjmp and is changed after the call to setjmp returns successfully must
    * be declared with 'volatile' to ensure that their values don't get
    * destroyed by longjmp:
    */
   volatile int result = 1 /*fail*/;

   if (argc == 4)
   {
      long x = atol(argv[1]);
      long y = atol(argv[2]);
      FILE *f = fopen(argv[3], "rb");
      volatile ci_bytep row = NULL;

      if (f != NULL)
      {
         /* libci requires a callback function for handling errors; this
          * callback must not return.  The default callback function uses a
          * stored <setjmp.h> style jmp_buf which is held in a ci_struct and
          * writes error messages to stderr.  Creating the ci_struct is a
          * little tricky; just copy the following code.
          */
         ci_structp ci_ptr =
            ci_create_read_struct(CI_LIBCI_VER_STRING, NULL, NULL, NULL);

         if (ci_ptr != NULL)
         {
            ci_infop info_ptr = ci_create_info_struct(ci_ptr);

            if (info_ptr != NULL)
            {
               /* Declare stack variables to hold pointers to locally allocated
                * data.
                */

               /* Initialize the error control buffer: */
               if (setjmp(ci_jmpbuf(ci_ptr)) == 0)
               {
                  ci_uint_32 width, height;
                  int bit_depth, color_type, interlace_method,
                     compression_method, filter_method;
                  ci_bytep row_tmp;

                  /* Now associate the recently opened FILE object with the
                   * default libci initialization functions.  Sometimes libci
                   * is compiled without stdio support (it can be difficult to
                   * do in some environments); in that case you will have to
                   * write your own read callback to read data from the stream.
                   */
                  ci_init_io(ci_ptr, f);

                  /* And read the first part of the CI file - the header and
                   * all the information up to the first pixel.
                   */
                  ci_read_info(ci_ptr, info_ptr);

                  /* This fills in enough information to tell us the width of
                   * each row in bytes, allocate the appropriate amount of
                   * space.  In this case ci_malloc is used - it will not
                   * return if memory isn't available.
                   */
                  row =
                     ci_malloc(ci_ptr, ci_get_rowbytes(ci_ptr, info_ptr));

                  /* Avoid the overhead of using a volatile auto copy row_tmp
                   * to a local here - just use row for the ci_free below.
                   */
                  row_tmp = row;

                  /* All the information we need is in the header returned by
                   * ci_get_IHDR.  If this fails, we can use 'ci_error' to
                   * signal the error and return control to the setjmp above.
                   */
                  if (ci_get_IHDR(ci_ptr, info_ptr, &width, &height,
                                   &bit_depth, &color_type, &interlace_method,
                                   &compression_method, &filter_method))
                  {
                     int passes, pass;

                     /* ci_set_interlace_handling returns the number of
                      * passes required as well as turning on libci's
                      * handling, but since we do it ourselves this is
                      * necessary:
                      */
                     switch (interlace_method)
                     {
                        case CI_INTERLACE_NONE:
                           passes = 1;
                           break;

                        case CI_INTERLACE_ADAM7:
                           passes = CI_INTERLACE_ADAM7_PASSES;
                           break;

                        default:
                           ci_error(ci_ptr, "cipixel: unknown interlace");
                     }

                     /* Now read the pixels, pass-by-pass, row-by-row: */
                     ci_start_read_image(ci_ptr);

                     for (pass = 0; pass < passes; ++pass)
                     {
                        ci_uint_32 ystart, xstart, ystep, xstep;
                        ci_uint_32 py;

                        if (interlace_method == CI_INTERLACE_ADAM7)
                        {
                           /* Sometimes the whole pass is empty because the
                            * image is too narrow or too short.  libci
                            * expects to be called for each row that is
                            * present in the pass, so it may be necessary to
                            * skip the loop below (over py) if the image is
                            * too narrow.
                            */
                           if (CI_PASS_COLS(width, pass) == 0)
                              continue;

                           /* We need the starting pixel and the offset
                            * between each pixel in this pass; use the macros
                            * in ci.h:
                            */
                           xstart = CI_PASS_START_COL(pass);
                           ystart = CI_PASS_START_ROW(pass);
                           xstep = CI_PASS_COL_OFFSET(pass);
                           ystep = CI_PASS_ROW_OFFSET(pass);
                        }

                        else
                        {
                           ystart = xstart = 0;
                           ystep = xstep = 1;
                        }

                        /* To find the pixel, loop over 'py' for each pass
                         * reading a row and then checking to see if it
                         * contains the pixel.
                         */
                        for (py = ystart; py < height; py += ystep)
                        {
                           ci_uint_32 px, ppx;

                           /* ci_read_row takes two pointers.  When libci
                            * handles the interlace the first is filled in
                            * pixel-by-pixel, and the second receives the same
                            * pixels but they are replicated across the
                            * unwritten pixels so far for each pass.  When we
                            * do the interlace, however, they just contain
                            * the pixels from the interlace pass - giving
                            * both is wasteful and pointless, so we pass a
                            * NULL pointer.
                            */
                           ci_read_row(ci_ptr, row_tmp, NULL);

                           /* Now find the pixel if it is in this row; there
                            * are, of course, much better ways of doing this
                            * than using a for loop:
                            */
                           if (y == py)
                           {
                              for (px = xstart, ppx = 0;
                                   px < width;
                                   px += xstep, ++ppx)
                              {
                                 if (x == px)
                                 {
                                    /* 'ppx' is the index of the pixel in the
                                     * row buffer.
                                     */
                                    print_pixel(ci_ptr, info_ptr, row_tmp,
                                                ppx);

                                    /* Now terminate the loops early - we have
                                     * found and handled the required data.
                                     */
                                    goto pass_loop_end;
                                 } /* x loop */
                              }
                           }
                        } /* y loop */
                     } /* pass loop */

                     /* Finally free the temporary buffer: */
                  pass_loop_end:
                     row = NULL;
                     ci_free(ci_ptr, row_tmp);
                  }

                  else
                     ci_error(ci_ptr, "cipixel: ci_get_IHDR failed");
               }

               else
               {
                  /* Else libci has raised an error.  An error message has
                   * already been output, so it is only necessary to clean up
                   * locally allocated data:
                   */
                  if (row != NULL)
                  {
                     /* The default implementation of ci_free never errors out
                      * (it just crashes if something goes wrong), but the safe
                      * way of using it is still to clear 'row' before calling
                      * ci_free:
                      */
                     ci_bytep row_tmp = row;
                     row = NULL;
                     ci_free(ci_ptr, row_tmp);
                  }
               }

               ci_destroy_info_struct(ci_ptr, &info_ptr);
            }

            else
               fprintf(stderr,
                       "cipixel: out of memory allocating ci_info\n");

            ci_destroy_read_struct(&ci_ptr, NULL, NULL);
         }

         else
            fprintf(stderr, "cipixel: out of memory allocating ci_struct\n");
      }

      else
         fprintf(stderr, "cipixel: %s: could not open file\n", argv[3]);
   }

   else
      /* Wrong number of arguments */
      fprintf(stderr, "cipixel: usage: cipixel x y ci-file\n");

   return result;
}

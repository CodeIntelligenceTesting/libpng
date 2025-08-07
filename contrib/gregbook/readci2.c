/*---------------------------------------------------------------------------

   rci2 - progressive-model CI display program                 readci2.c

  ---------------------------------------------------------------------------

      Copyright (c) 1998-2015 Greg Roelofs.  All rights reserved.

      This software is provided "as is," without warranty of any kind,
      express or implied.  In no event shall the author or contributors
      be held liable for any damages arising in any way from the use of
      this software.

      The contents of this file are DUAL-LICENSED.  You may modify and/or
      redistribute this software according to the terms of one of the
      following two licenses (at your option):


      LICENSE 1 ("BSD-like with advertising clause"):

      Permission is granted to anyone to use this software for any purpose,
      including commercial applications, and to alter it and redistribute
      it freely, subject to the following restrictions:

      1. Redistributions of source code must retain the above copyright
         notice, disclaimer, and this list of conditions.
      2. Redistributions in binary form must reproduce the above copyright
         notice, disclaimer, and this list of conditions in the documenta-
         tion and/or other materials provided with the distribution.
      3. All advertising materials mentioning features or use of this
         software must display the following acknowledgment:

            This product includes software developed by Greg Roelofs
            and contributors for the book, "CI: The Definitive Guide,"
            published by O'Reilly and Associates.


      LICENSE 2 (GNU GPL v2 or later):

      This program is free software; you can redistribute it and/or modify
      it under the terms of the GNU General Public License as published by
      the Free Software Foundation; either version 2 of the License, or
      (at your option) any later version.

      This program is distributed in the hope that it will be useful,
      but WITHOUT ANY WARRANTY; without even the implied warranty of
      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
      GNU General Public License for more details.

      You should have received a copy of the GNU General Public License
      along with this program; if not, write to the Free Software Foundation,
      Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

  ---------------------------------------------------------------------------

   Changelog:
     2015-11-12 - Check return value of ci_get_bKGD() (Glenn R-P)
     2017-04-22 - Guard against integer overflow (Glenn R-P)

  ---------------------------------------------------------------------------*/


#include <stdlib.h>     /* for exit() prototype */
#include <setjmp.h>

#include <zlib.h>
#include "ci.h"        /* libci header from the local directory */
#include "readci2.h"   /* typedefs, common macros, public prototypes */


/* local prototypes */

static void readci2_info_callback(ci_structp ci_ptr, ci_infop info_ptr);
static void readci2_row_callback(ci_structp ci_ptr, ci_bytep new_row,
                                 ci_uint_32 row_num, int pass);
static void readci2_end_callback(ci_structp ci_ptr, ci_infop info_ptr);
static void readci2_error_handler(ci_structp ci_ptr, ci_const_charp msg);
static void readci2_warning_handler(ci_structp ci_ptr, ci_const_charp msg);




void readci2_version_info(void)
{
    fprintf(stderr, "   Compiled with libci %s; using libci %s\n",
      CI_LIBCI_VER_STRING, ci_libci_ver);

    fprintf(stderr, "   and with zlib %s; using zlib %s.\n",
      ZLIB_VERSION, zlib_version);
}




int readci2_check_sig(uch *sig, int num)
{
    return !ci_sig_cmp(sig, 0, num);
}




/* returns 0 for success, 2 for libci problem, 4 for out of memory */

int readci2_init(mainprog_info *mainprog_ptr)
{
    ci_structp  ci_ptr;       /* note:  temporary variables! */
    ci_infop  info_ptr;


    /* could also replace libci warning-handler (final NULL), but no need: */

    ci_ptr = ci_create_read_struct(ci_get_libci_ver(NULL), mainprog_ptr,
      readci2_error_handler, readci2_warning_handler);
    if (!ci_ptr)
        return 4;   /* out of memory */

    info_ptr = ci_create_info_struct(ci_ptr);
    if (!info_ptr) {
        ci_destroy_read_struct(&ci_ptr, NULL, NULL);
        return 4;   /* out of memory */
    }


    /* we could create a second info struct here (end_info), but it's only
     * useful if we want to keep pre- and post-IDAT chunk info separated
     * (mainly for CI-aware image editors and converters) */


    /* setjmp() must be called in every function that calls a CI-reading
     * libci function, unless an alternate error handler was installed--
     * but compatible error handlers must either use longjmp() themselves
     * (as in this program) or exit immediately, so here we are: */

    if (setjmp(mainprog_ptr->jmpbuf)) {
        ci_destroy_read_struct(&ci_ptr, &info_ptr, NULL);
        return 2;
    }


#ifdef CI_HANDLE_AS_UNKNOWN_SUPPORTED
    /* prepare the reader to ignore all recognized chunks whose data won't be
     * used, i.e., all chunks recognized by libci except for IHDR, PLTE, IDAT,
     * IEND, tRNS, bKGD, gAMA, and sRGB (small performance improvement) */
    {
        /* These byte strings were copied from ci.h.  If a future version
         * of readci2.c recognizes more chunks, add them to this list.
         */
        static const ci_byte chunks_to_process[] = {
            98,  75,  71,  68, '\0',  /* bKGD */
           103,  65,  77,  65, '\0',  /* gAMA */
           115,  82,  71,  66, '\0',  /* sRGB */
           };

       /* Ignore all chunks except for IHDR, PLTE, tRNS, IDAT, and IEND */
       ci_set_keep_unknown_chunks(ci_ptr, -1 /* CI_HANDLE_CHUNK_NEVER */,
          NULL, -1);

       /* But do not ignore chunks in the "chunks_to_process" list */
       ci_set_keep_unknown_chunks(ci_ptr,
          0 /* CI_HANDLE_CHUNK_AS_DEFAULT */, chunks_to_process,
          sizeof(chunks_to_process)/5);
    }
#endif /* CI_HANDLE_AS_UNKNOWN_SUPPORTED */


    /* instead of doing ci_init_io() here, now we set up our callback
     * functions for progressive decoding */

    ci_set_progressive_read_fn(ci_ptr, mainprog_ptr,
      readci2_info_callback, readci2_row_callback, readci2_end_callback);


    /* make sure we save our pointers for use in readci2_decode_data() */

    mainprog_ptr->ci_ptr = ci_ptr;
    mainprog_ptr->info_ptr = info_ptr;


    /* and that's all there is to initialization */

    return 0;
}




/* returns 0 for success, 2 for libci (longjmp) problem */

int readci2_decode_data(mainprog_info *mainprog_ptr, uch *rawbuf, ulg length)
{
    ci_structp ci_ptr = (ci_structp)mainprog_ptr->ci_ptr;
    ci_infop info_ptr = (ci_infop)mainprog_ptr->info_ptr;


    /* setjmp() must be called in every function that calls a CI-reading
     * libci function */

    if (setjmp(mainprog_ptr->jmpbuf)) {
        ci_destroy_read_struct(&ci_ptr, &info_ptr, NULL);
        mainprog_ptr->ci_ptr = NULL;
        mainprog_ptr->info_ptr = NULL;
        return 2;
    }


    /* hand off the next chunk of input data to libci for decoding */

    ci_process_data(ci_ptr, info_ptr, rawbuf, length);

    return 0;
}




static void readci2_info_callback(ci_structp ci_ptr, ci_infop info_ptr)
{
    mainprog_info  *mainprog_ptr;
    int  color_type, bit_depth;
    ci_uint_32 width, height;
#ifdef CI_FLOATING_POINT_SUPPORTED
    double  gamma;
#else
    ci_fixed_point gamma;
#endif


    /* setjmp() doesn't make sense here, because we'd either have to exit(),
     * longjmp() ourselves, or return control to libci, which doesn't want
     * to see us again.  By not doing anything here, libci will instead jump
     * to readci2_decode_data(), which can return an error value to the main
     * program. */


    /* retrieve the pointer to our special-purpose struct, using the ci_ptr
     * that libci passed back to us (i.e., not a global this time--there's
     * no real difference for a single image, but for a multithreaded browser
     * decoding several CI images at the same time, one needs to avoid mixing
     * up different images' structs) */

    mainprog_ptr = ci_get_progressive_ptr(ci_ptr);

    if (mainprog_ptr == NULL) {         /* we be hosed */
        fprintf(stderr,
          "readci2 error:  main struct not recoverable in info_callback.\n");
        fflush(stderr);
        return;
        /*
         * Alternatively, we could call our error-handler just like libci
         * does, which would effectively terminate the program.  Since this
         * can only happen if ci_ptr gets redirected somewhere odd or the
         * main CI struct gets wiped, we're probably toast anyway.  (If
         * ci_ptr itself is NULL, we would not have been called.)
         */
    }


    /* this is just like in the non-progressive case */

    ci_get_IHDR(ci_ptr, info_ptr, &width, &height, &bit_depth, &color_type,
       NULL, NULL, NULL);
    mainprog_ptr->width = (ulg)width;
    mainprog_ptr->height = (ulg)height;


    /* since we know we've read all of the CI file's "header" (i.e., up
     * to IDAT), we can check for a background color here */

    if (mainprog_ptr->need_bgcolor)
    {
        ci_color_16p pBackground;

        /* it is not obvious from the libci documentation, but this function
         * takes a pointer to a pointer, and it always returns valid red,
         * green and blue values, regardless of color_type: */
        if (ci_get_bKGD(ci_ptr, info_ptr, &pBackground))
        {

           /* however, it always returns the raw bKGD data, regardless of any
            * bit-depth transformations, so check depth and adjust if necessary
            */
           if (bit_depth == 16) {
               mainprog_ptr->bg_red   = pBackground->red   >> 8;
               mainprog_ptr->bg_green = pBackground->green >> 8;
               mainprog_ptr->bg_blue  = pBackground->blue  >> 8;
           } else if (color_type == CI_COLOR_TYPE_GRAY && bit_depth < 8) {
               if (bit_depth == 1)
                   mainprog_ptr->bg_red = mainprog_ptr->bg_green =
                     mainprog_ptr->bg_blue = pBackground->gray? 255 : 0;
               else if (bit_depth == 2)
                   mainprog_ptr->bg_red = mainprog_ptr->bg_green =
                     mainprog_ptr->bg_blue = (255/3) * pBackground->gray;
               else /* bit_depth == 4 */
                   mainprog_ptr->bg_red = mainprog_ptr->bg_green =
                     mainprog_ptr->bg_blue = (255/15) * pBackground->gray;
           } else {
               mainprog_ptr->bg_red   = (uch)pBackground->red;
               mainprog_ptr->bg_green = (uch)pBackground->green;
               mainprog_ptr->bg_blue  = (uch)pBackground->blue;
           }
        }
    }


    /* as before, let libci expand palette images to RGB, low-bit-depth
     * grayscale images to 8 bits, transparency chunks to full alpha channel;
     * strip 16-bit-per-sample images to 8 bits per sample; and convert
     * grayscale to RGB[A] */

    if (color_type == CI_COLOR_TYPE_PALETTE)
        ci_set_expand(ci_ptr);
    if (color_type == CI_COLOR_TYPE_GRAY && bit_depth < 8)
        ci_set_expand(ci_ptr);
    if (ci_get_valid(ci_ptr, info_ptr, CI_INFO_tRNS))
        ci_set_expand(ci_ptr);
#ifdef CI_READ_16_TO_8_SUPPORTED
    if (bit_depth == 16)
#  ifdef CI_READ_SCALE_16_TO_8_SUPPORTED
        ci_set_scale_16(ci_ptr);
#  else
        ci_set_strip_16(ci_ptr);
#  endif
#endif
    if (color_type == CI_COLOR_TYPE_GRAY ||
        color_type == CI_COLOR_TYPE_GRAY_ALPHA)
        ci_set_gray_to_rgb(ci_ptr);


    /* Unlike the basic viewer, which was designed to operate on local files,
     * this program is intended to simulate a web browser--even though we
     * actually read from a local file, too.  But because we are pretending
     * that most of the images originate on the Internet, we follow the recom-
     * mendation of the sRGB proposal and treat unlabelled images (no gAMA
     * chunk) as existing in the sRGB color space.  That is, we assume that
     * such images have a file gamma of 0.45455, which corresponds to a PC-like
     * display system.  This change in assumptions will have no effect on a
     * PC-like system, but on a Mac, SGI, NeXT or other system with a non-
     * identity lookup table, it will darken unlabelled images, which effec-
     * tively favors images from PC-like systems over those originating on
     * the local platform.  Note that mainprog_ptr->display_exponent is the
     * "gamma" value for the entire display system, i.e., the product of
     * LUT_exponent and CRT_exponent. */

#ifdef CI_FLOATING_POINT_SUPPORTED
    if (ci_get_gAMA(ci_ptr, info_ptr, &gamma))
        ci_set_gamma(ci_ptr, mainprog_ptr->display_exponent, gamma);
    else
        ci_set_gamma(ci_ptr, mainprog_ptr->display_exponent, 0.45455);
#else
    if (ci_get_gAMA_fixed(ci_ptr, info_ptr, &gamma))
        ci_set_gamma_fixed(ci_ptr,
            (ci_fixed_point)(100000*mainprog_ptr->display_exponent+.5), gamma);
    else
        ci_set_gamma_fixed(ci_ptr,
            (ci_fixed_point)(100000*mainprog_ptr->display_exponent+.5), 45455);
#endif

    /* we'll let libci expand interlaced images, too */

    mainprog_ptr->passes = ci_set_interlace_handling(ci_ptr);


    /* all transformations have been registered; now update info_ptr data and
     * then get rowbytes and channels */

    ci_read_update_info(ci_ptr, info_ptr);

    mainprog_ptr->rowbytes = (int)ci_get_rowbytes(ci_ptr, info_ptr);
    mainprog_ptr->channels = ci_get_channels(ci_ptr, info_ptr);


    /* Call the main program to allocate memory for the image buffer and
     * initialize windows and whatnot.  (The old-style function-pointer
     * invocation is used for compatibility with a few supposedly ANSI
     * compilers that nevertheless barf on "fn_ptr()"-style syntax.) */

    (*mainprog_ptr->mainprog_init)();


    /* and that takes care of initialization */

    return;
}





static void readci2_row_callback(ci_structp ci_ptr, ci_bytep new_row,
                                  ci_uint_32 row_num, int pass)
{
    mainprog_info  *mainprog_ptr;


    /* first check whether the row differs from the previous pass; if not,
     * nothing to combine or display */

    if (!new_row)
        return;


    /* retrieve the pointer to our special-purpose struct so we can access
     * the old rows and image-display callback function */

    mainprog_ptr = ci_get_progressive_ptr(ci_ptr);


    /* save the pass number for optional use by the front end */

    mainprog_ptr->pass = pass;


    /* have libci either combine the new row data with the existing row data
     * from previous passes (if interlaced) or else just copy the new row
     * into the main program's image buffer */

    ci_progressive_combine_row(ci_ptr, mainprog_ptr->row_pointers[row_num],
      new_row);


    /* finally, call the display routine in the main program with the number
     * of the row we just updated */

    (*mainprog_ptr->mainprog_display_row)(row_num);


    /* and we're ready for more */

    return;
}





static void readci2_end_callback(ci_structp ci_ptr, ci_infop info_ptr)
{
    mainprog_info  *mainprog_ptr;


    /* retrieve the pointer to our special-purpose struct */

    mainprog_ptr = ci_get_progressive_ptr(ci_ptr);


    /* let the main program know that it should flush any buffered image
     * data to the display now and set a "done" flag or whatever, but note
     * that it SHOULD NOT DESTROY THE CI STRUCTS YET--in other words, do
     * NOT call readci2_cleanup() either here or in the finish_display()
     * routine; wait until control returns to the main program via
     * readci2_decode_data() */

    (*mainprog_ptr->mainprog_finish_display)();


    /* all done */

    (void)info_ptr; /* Unused */

    return;
}





void readci2_cleanup(mainprog_info *mainprog_ptr)
{
    ci_structp ci_ptr = (ci_structp)mainprog_ptr->ci_ptr;
    ci_infop info_ptr = (ci_infop)mainprog_ptr->info_ptr;

    if (ci_ptr && info_ptr)
        ci_destroy_read_struct(&ci_ptr, &info_ptr, NULL);

    mainprog_ptr->ci_ptr = NULL;
    mainprog_ptr->info_ptr = NULL;
}


static void readci2_warning_handler(ci_structp ci_ptr, ci_const_charp msg)
{
    fprintf(stderr, "readci2 libci warning: %s\n", msg);
    fflush(stderr);
    (void)ci_ptr; /* Unused */
}


static void readci2_error_handler(ci_structp ci_ptr, ci_const_charp msg)
{
    mainprog_info  *mainprog_ptr;

    /* This function, aside from the extra step of retrieving the "error
     * pointer" (below) and the fact that it exists within the application
     * rather than within libci, is essentially identical to libci's
     * default error handler.  The second point is critical:  since both
     * setjmp() and longjmp() are called from the same code, they are
     * guaranteed to have compatible notions of how big a jmp_buf is,
     * regardless of whether _BSD_SOURCE or anything else has (or has not)
     * been defined. */

    fprintf(stderr, "readci2 libci error: %s\n", msg);
    fflush(stderr);

    mainprog_ptr = ci_get_error_ptr(ci_ptr);
    if (mainprog_ptr == NULL) {         /* we are completely hosed now */
        fprintf(stderr,
          "readci2 severe error:  jmpbuf not recoverable; terminating.\n");
        fflush(stderr);
        exit(99);
    }

    /* Now we have our data structure we can use the information in it
     * to return control to our own higher level code (all the points
     * where 'setjmp' is called in this file.)  This will work with other
     * error handling mechanisms as well - libci always calls ci_error
     * when it can proceed no further, thus, so long as the error handler
     * is intercepted, application code can do its own error recovery.
     */
    longjmp(mainprog_ptr->jmpbuf, 1);
}

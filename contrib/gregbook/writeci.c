/*---------------------------------------------------------------------------

   wci - simple CI-writing program                             writeci.c

  ---------------------------------------------------------------------------

      Copyright (c) 1998-2007, 2017 Greg Roelofs.  All rights reserved.

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

  ---------------------------------------------------------------------------*/


#include <stdlib.h>     /* for exit() prototype */
#include <zlib.h>

#include "ci.h"        /* libci header, includes setjmp.h */
#include "writeci.h"   /* typedefs, common macros, public prototypes */


/* local prototype */

static void writeci_error_handler(ci_structp ci_ptr, ci_const_charp msg);



void writeci_version_info(void)
{
  fprintf(stderr, "   Compiled with libci %s; using libci %s.\n",
    CI_LIBCI_VER_STRING, ci_libci_ver);
  fprintf(stderr, "   Compiled with zlib %s; using zlib %s.\n",
    ZLIB_VERSION, zlib_version);
}




/* returns 0 for success, 2 for libci problem, 4 for out of memory, 11 for
 *  unexpected pnmtype; note that outfile might be stdout */

int writeci_init(mainprog_info *mainprog_ptr)
{
    ci_structp  ci_ptr;       /* note:  temporary variables! */
    ci_infop  info_ptr;
    int color_type, interlace_type;


    /* could also replace libci warning-handler (final NULL), but no need: */

    ci_ptr = ci_create_write_struct(ci_get_libci_ver(NULL), mainprog_ptr,
      writeci_error_handler, NULL);
    if (!ci_ptr)
        return 4;   /* out of memory */

    info_ptr = ci_create_info_struct(ci_ptr);
    if (!info_ptr) {
        ci_destroy_write_struct(&ci_ptr, NULL);
        return 4;   /* out of memory */
    }


    /* setjmp() must be called in every function that calls a CI-writing
     * libci function, unless an alternate error handler was installed--
     * but compatible error handlers must either use longjmp() themselves
     * (as in this program) or some other method to return control to
     * application code, so here we go: */

    if (setjmp(mainprog_ptr->jmpbuf)) {
        ci_destroy_write_struct(&ci_ptr, &info_ptr);
        return 2;
    }


    /* make sure outfile is (re)opened in BINARY mode */

    ci_init_io(ci_ptr, mainprog_ptr->outfile);


    /* set the compression levels--in general, always want to leave filtering
     * turned on (except for palette images) and allow all of the filters,
     * which is the default; want 32K zlib window, unless entire image buffer
     * is 16K or smaller (unknown here)--also the default; usually want max
     * compression (NOT the default); and remaining compression flags should
     * be left alone */

    ci_set_compression_level(ci_ptr, Z_BEST_COMPRESSION);
/*
    >> this is default for no filtering; Z_FILTERED is default otherwise:
    ci_set_compression_strategy(ci_ptr, Z_DEFAULT_STRATEGY);
    >> these are all defaults:
    ci_set_compression_mem_level(ci_ptr, 8);
    ci_set_compression_window_bits(ci_ptr, 15);
    ci_set_compression_method(ci_ptr, 8);
 */


    /* set the image parameters appropriately */

    if (mainprog_ptr->pnmtype == 5)
        color_type = CI_COLOR_TYPE_GRAY;
    else if (mainprog_ptr->pnmtype == 6)
        color_type = CI_COLOR_TYPE_RGB;
    else if (mainprog_ptr->pnmtype == 8)
        color_type = CI_COLOR_TYPE_RGB_ALPHA;
    else {
        ci_destroy_write_struct(&ci_ptr, &info_ptr);
        return 11;
    }

    interlace_type = mainprog_ptr->interlaced? CI_INTERLACE_ADAM7 :
                                               CI_INTERLACE_NONE;

    ci_set_IHDR(ci_ptr, info_ptr, mainprog_ptr->width, mainprog_ptr->height,
      mainprog_ptr->sample_depth, color_type, interlace_type,
      CI_COMPRESSION_TYPE_DEFAULT, CI_FILTER_TYPE_DEFAULT);

    if (mainprog_ptr->gamma > 0.0)
        ci_set_gAMA(ci_ptr, info_ptr, mainprog_ptr->gamma);

    if (mainprog_ptr->have_bg) {   /* we know it's RGBA, not gray+alpha */
        ci_color_16  background;

        background.red = mainprog_ptr->bg_red;
        background.green = mainprog_ptr->bg_green;
        background.blue = mainprog_ptr->bg_blue;
        ci_set_bKGD(ci_ptr, info_ptr, &background);
    }

    if (mainprog_ptr->have_time) {
        ci_time  modtime;

        ci_convert_from_time_t(&modtime, mainprog_ptr->modtime);
        ci_set_tIME(ci_ptr, info_ptr, &modtime);
    }

    if (mainprog_ptr->have_text) {
        ci_text  text[6];
        int  num_text = 0;

        if (mainprog_ptr->have_text & TEXT_TITLE) {
            text[num_text].compression = CI_TEXT_COMPRESSION_NONE;
            text[num_text].key = "Title";
            text[num_text].text = mainprog_ptr->title;
            ++num_text;
        }
        if (mainprog_ptr->have_text & TEXT_AUTHOR) {
            text[num_text].compression = CI_TEXT_COMPRESSION_NONE;
            text[num_text].key = "Author";
            text[num_text].text = mainprog_ptr->author;
            ++num_text;
        }
        if (mainprog_ptr->have_text & TEXT_DESC) {
            text[num_text].compression = CI_TEXT_COMPRESSION_NONE;
            text[num_text].key = "Description";
            text[num_text].text = mainprog_ptr->desc;
            ++num_text;
        }
        if (mainprog_ptr->have_text & TEXT_COPY) {
            text[num_text].compression = CI_TEXT_COMPRESSION_NONE;
            text[num_text].key = "Copyright";
            text[num_text].text = mainprog_ptr->copyright;
            ++num_text;
        }
        if (mainprog_ptr->have_text & TEXT_EMAIL) {
            text[num_text].compression = CI_TEXT_COMPRESSION_NONE;
            text[num_text].key = "E-mail";
            text[num_text].text = mainprog_ptr->email;
            ++num_text;
        }
        if (mainprog_ptr->have_text & TEXT_URL) {
            text[num_text].compression = CI_TEXT_COMPRESSION_NONE;
            text[num_text].key = "URL";
            text[num_text].text = mainprog_ptr->url;
            ++num_text;
        }
        ci_set_text(ci_ptr, info_ptr, text, num_text);
    }


    /* write all chunks up to (but not including) first IDAT */

    ci_write_info(ci_ptr, info_ptr);


    /* if we wanted to write any more text info *after* the image data, we
     * would set up text struct(s) here and call ci_set_text() again, with
     * just the new data; ci_set_tIME() could also go here, but it would
     * have no effect since we already called it above (only one tIME chunk
     * allowed) */


    /* set up the transformations:  for now, just pack low-bit-depth pixels
     * into bytes (one, two or four pixels per byte) */

    ci_set_packing(ci_ptr);
/*  ci_set_shift(ci_ptr, &sig_bit);  to scale low-bit-depth values */


    /* make sure we save our pointers for use in writeci_encode_image() */

    mainprog_ptr->ci_ptr = ci_ptr;
    mainprog_ptr->info_ptr = info_ptr;


    /* OK, that's all we need to do for now; return happy */

    return 0;
}





/* returns 0 for success, 2 for libci (longjmp) problem */

int writeci_encode_image(mainprog_info *mainprog_ptr)
{
    ci_structp ci_ptr = (ci_structp)mainprog_ptr->ci_ptr;
    ci_infop info_ptr = (ci_infop)mainprog_ptr->info_ptr;


    /* as always, setjmp() must be called in every function that calls a
     * CI-writing libci function */

    if (setjmp(mainprog_ptr->jmpbuf)) {
        ci_destroy_write_struct(&ci_ptr, &info_ptr);
        mainprog_ptr->ci_ptr = NULL;
        mainprog_ptr->info_ptr = NULL;
        return 2;
    }


    /* and now we just write the whole image; libci takes care of interlacing
     * for us */

    ci_write_image(ci_ptr, mainprog_ptr->row_pointers);


    /* since that's it, we also close out the end of the CI file now--if we
     * had any text or time info to write after the IDATs, second argument
     * would be info_ptr, but we optimize slightly by sending NULL pointer: */

    ci_write_end(ci_ptr, NULL);

    return 0;
}





/* returns 0 if succeeds, 2 if libci problem */

int writeci_encode_row(mainprog_info *mainprog_ptr)  /* NON-interlaced only! */
{
    ci_structp ci_ptr = (ci_structp)mainprog_ptr->ci_ptr;
    ci_infop info_ptr = (ci_infop)mainprog_ptr->info_ptr;


    /* as always, setjmp() must be called in every function that calls a
     * CI-writing libci function */

    if (setjmp(mainprog_ptr->jmpbuf)) {
        ci_destroy_write_struct(&ci_ptr, &info_ptr);
        mainprog_ptr->ci_ptr = NULL;
        mainprog_ptr->info_ptr = NULL;
        return 2;
    }


    /* image_data points at our one row of image data */

    ci_write_row(ci_ptr, mainprog_ptr->image_data);

    return 0;
}





/* returns 0 if succeeds, 2 if libci problem */

int writeci_encode_finish(mainprog_info *mainprog_ptr)   /* NON-interlaced! */
{
    ci_structp ci_ptr = (ci_structp)mainprog_ptr->ci_ptr;
    ci_infop info_ptr = (ci_infop)mainprog_ptr->info_ptr;


    /* as always, setjmp() must be called in every function that calls a
     * CI-writing libci function */

    if (setjmp(mainprog_ptr->jmpbuf)) {
        ci_destroy_write_struct(&ci_ptr, &info_ptr);
        mainprog_ptr->ci_ptr = NULL;
        mainprog_ptr->info_ptr = NULL;
        return 2;
    }


    /* close out CI file; if we had any text or time info to write after
     * the IDATs, second argument would be info_ptr: */

    ci_write_end(ci_ptr, NULL);

    return 0;
}





void writeci_cleanup(mainprog_info *mainprog_ptr)
{
    ci_structp ci_ptr = (ci_structp)mainprog_ptr->ci_ptr;
    ci_infop info_ptr = (ci_infop)mainprog_ptr->info_ptr;

    if (ci_ptr && info_ptr)
        ci_destroy_write_struct(&ci_ptr, &info_ptr);
}





static void writeci_error_handler(ci_structp ci_ptr, ci_const_charp msg)
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

    fprintf(stderr, "writeci libci error: %s\n", msg);
    fflush(stderr);

    mainprog_ptr = ci_get_error_ptr(ci_ptr);
    if (mainprog_ptr == NULL) {         /* we are completely hosed now */
        fprintf(stderr,
          "writeci severe error:  jmpbuf not recoverable; terminating.\n");
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

/*---------------------------------------------------------------------------

   rci - simple CI display program                              readci.c

  ---------------------------------------------------------------------------

      Copyright (c) 1998-2007,2017 Greg Roelofs.  All rights reserved.

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

#include <stdio.h>
#include <stdlib.h>
#include <zlib.h>

#include "ci.h"        /* libci header */
#include "readci.h"    /* typedefs, common macros, public prototypes */

/* future versions of libci will provide this macro: */
#ifndef ci_jmpbuf
#  define ci_jmpbuf(ci_ptr)   ((ci_ptr)->jmpbuf)
#endif


static ci_structp ci_ptr = NULL;
static ci_infop info_ptr = NULL;

ci_uint_32  width, height;
int  bit_depth, color_type;
uch  *image_data = NULL;


void readci_version_info(void)
{
    fprintf(stderr, "   Compiled with libci %s; using libci %s.\n",
      CI_LIBCI_VER_STRING, ci_libci_ver);
    fprintf(stderr, "   Compiled with zlib %s; using zlib %s.\n",
      ZLIB_VERSION, zlib_version);
}


/* return value = 0 for success, 1 for bad sig, 2 for bad IHDR, 4 for no mem */

int readci_init(FILE *infile, ulg *pWidth, ulg *pHeight)
{
    uch sig[8];


    /* first do a quick check that the file really is a CI image; could
     * have used slightly more general ci_sig_cmp() function instead */

    fread(sig, 1, 8, infile);
    if (ci_sig_cmp(sig, 0, 8))
        return 1;   /* bad signature */


    /* could pass pointers to user-defined error handlers instead of NULLs: */

    ci_ptr = ci_create_read_struct(ci_get_libci_ver(NULL), NULL, NULL,
        NULL);
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
     * libci function */

    if (setjmp(ci_jmpbuf(ci_ptr))) {
        ci_destroy_read_struct(&ci_ptr, &info_ptr, NULL);
        return 2;
    }


    ci_init_io(ci_ptr, infile);
    ci_set_sig_bytes(ci_ptr, 8);  /* we already read the 8 signature bytes */

    ci_read_info(ci_ptr, info_ptr);  /* read all CI info up to image data */


    /* alternatively, could make separate calls to ci_get_image_width(),
     * etc., but want bit_depth and color_type for later [don't care about
     * compression_type and filter_type => NULLs] */

    ci_get_IHDR(ci_ptr, info_ptr, &width, &height, &bit_depth, &color_type,
      NULL, NULL, NULL);
    *pWidth = width;
    *pHeight = height;


    /* OK, that's all we need for now; return happy */

    return 0;
}




/* returns 0 if succeeds, 1 if fails due to no bKGD chunk, 2 if libci error;
 * scales values to 8-bit if necessary */

int readci_get_bgcolor(uch *red, uch *green, uch *blue)
{
    ci_color_16p pBackground;


    /* setjmp() must be called in every function that calls a CI-reading
     * libci function */

    if (setjmp(ci_jmpbuf(ci_ptr))) {
        ci_destroy_read_struct(&ci_ptr, &info_ptr, NULL);
        return 2;
    }


    if (!ci_get_valid(ci_ptr, info_ptr, CI_INFO_bKGD))
        return 1;

    /* it is not obvious from the libci documentation, but this function
     * takes a pointer to a pointer, and it always returns valid red, green
     * and blue values, regardless of color_type: */

    ci_get_bKGD(ci_ptr, info_ptr, &pBackground);


    /* however, it always returns the raw bKGD data, regardless of any
     * bit-depth transformations, so check depth and adjust if necessary */

    if (bit_depth == 16) {
        *red   = pBackground->red   >> 8;
        *green = pBackground->green >> 8;
        *blue  = pBackground->blue  >> 8;
    } else if (color_type == CI_COLOR_TYPE_GRAY && bit_depth < 8) {
        if (bit_depth == 1)
            *red = *green = *blue = pBackground->gray? 255 : 0;
        else if (bit_depth == 2)
            *red = *green = *blue = (255/3) * pBackground->gray;
        else /* bit_depth == 4 */
            *red = *green = *blue = (255/15) * pBackground->gray;
    } else {
        *red   = (uch)pBackground->red;
        *green = (uch)pBackground->green;
        *blue  = (uch)pBackground->blue;
    }

    return 0;
}




/* display_exponent == LUT_exponent * CRT_exponent */

uch *readci_get_image(double display_exponent, int *pChannels, ulg *pRowbytes)
{
    double  gamma;
    ci_uint_32  i, rowbytes;
    ci_bytepp  row_pointers = NULL;


    /* setjmp() must be called in every function that calls a CI-reading
     * libci function */

    if (setjmp(ci_jmpbuf(ci_ptr))) {
        free(image_data);
        image_data = NULL;
        free(row_pointers);
        row_pointers = NULL;
        ci_destroy_read_struct(&ci_ptr, &info_ptr, NULL);
        return NULL;
    }


    /* expand palette images to RGB, low-bit-depth grayscale images to 8 bits,
     * transparency chunks to full alpha channel; strip 16-bit-per-sample
     * images to 8 bits per sample; and convert grayscale to RGB[A] */

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


    /* unlike the example in the libci documentation, we have *no* idea where
     * this file may have come from--so if it doesn't have a file gamma, don't
     * do any correction ("do no harm") */

    if (ci_get_gAMA(ci_ptr, info_ptr, &gamma))
        ci_set_gamma(ci_ptr, display_exponent, gamma);


    /* all transformations have been registered; now update info_ptr data,
     * get rowbytes and channels, and allocate image memory */

    ci_read_update_info(ci_ptr, info_ptr);

    *pRowbytes = rowbytes = ci_get_rowbytes(ci_ptr, info_ptr);
    *pChannels = (int)ci_get_channels(ci_ptr, info_ptr);

    /* Guard against integer overflow */
    if (height > ((size_t)(-1))/rowbytes) {
        fprintf(stderr, "readci:  image_data buffer would be too large\n",
        return NULL;
    }

    if ((image_data = (uch *)malloc(rowbytes*height)) == NULL) {
        ci_destroy_read_struct(&ci_ptr, &info_ptr, NULL);
        return NULL;
    }
    if ((row_pointers = (ci_bytepp)malloc(height*sizeof(ci_bytep))) == NULL) {
        ci_destroy_read_struct(&ci_ptr, &info_ptr, NULL);
        free(image_data);
        image_data = NULL;
        return NULL;
    }

    Trace((stderr, "readci_get_image:  channels = %d, rowbytes = %ld, height = %ld\n",
        *pChannels, rowbytes, height));


    /* set the individual row_pointers to point at the correct offsets */

    for (i = 0;  i < height;  ++i)
        row_pointers[i] = image_data + i*rowbytes;


    /* now we can go ahead and just read the whole image */

    ci_read_image(ci_ptr, row_pointers);


    /* and we're done!  (ci_read_end() can be omitted if no processing of
     * post-IDAT text/time/etc. is desired) */

    free(row_pointers);
    row_pointers = NULL;

    ci_read_end(ci_ptr, NULL);

    return image_data;
}


void readci_cleanup(int free_image_data)
{
    if (free_image_data && image_data) {
        free(image_data);
        image_data = NULL;
    }

    if (ci_ptr && info_ptr) {
        ci_destroy_read_struct(&ci_ptr, &info_ptr, NULL);
        ci_ptr = NULL;
        info_ptr = NULL;
    }
}

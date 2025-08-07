/* ciread.c - read a CI file
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
 * This file contains routines that an application calls directly to
 * read a CI file or stream.
 */

#include "cipriv.h"
#if defined(CI_SIMPLIFIED_READ_SUPPORTED) && defined(CI_STDIO_SUPPORTED)
#  include <errno.h>
#endif

#ifdef CI_READ_SUPPORTED

/* Create a CI structure for reading, and allocate any memory needed. */
CI_FUNCTION(ci_structp,CIAPI
ci_create_read_struct,(ci_const_charp user_ci_ver, ci_voidp error_ptr,
    ci_error_ptr error_fn, ci_error_ptr warn_fn),CI_ALLOCATED)
{
#ifndef CI_USER_MEM_SUPPORTED
   ci_structp ci_ptr = ci_create_ci_struct(user_ci_ver, error_ptr,
        error_fn, warn_fn, NULL, NULL, NULL);
#else
   return ci_create_read_struct_2(user_ci_ver, error_ptr, error_fn,
        warn_fn, NULL, NULL, NULL);
}

/* Alternate create CI structure for reading, and allocate any memory
 * needed.
 */
CI_FUNCTION(ci_structp,CIAPI
ci_create_read_struct_2,(ci_const_charp user_ci_ver, ci_voidp error_ptr,
    ci_error_ptr error_fn, ci_error_ptr warn_fn, ci_voidp mem_ptr,
    ci_malloc_ptr malloc_fn, ci_free_ptr free_fn),CI_ALLOCATED)
{
   ci_structp ci_ptr = ci_create_ci_struct(user_ci_ver, error_ptr,
       error_fn, warn_fn, mem_ptr, malloc_fn, free_fn);
#endif /* USER_MEM */

   if (ci_ptr != NULL)
   {
      ci_ptr->mode = CI_IS_READ_STRUCT;

      /* Added in libci-1.6.0; this can be used to detect a read structure if
       * required (it will be zero in a write structure.)
       */
#     ifdef CI_SEQUENTIAL_READ_SUPPORTED
         ci_ptr->IDAT_read_size = CI_IDAT_READ_SIZE;
#     endif

#     ifdef CI_BENIGN_READ_ERRORS_SUPPORTED
         ci_ptr->flags |= CI_FLAG_BENIGN_ERRORS_WARN;

         /* In stable builds only warn if an application error can be completely
          * handled.
          */
#        if CI_RELEASE_BUILD
            ci_ptr->flags |= CI_FLAG_APP_WARNINGS_WARN;
#        endif
#     endif

      /* TODO: delay this, it can be done in ci_init_io (if the app doesn't
       * do it itself) avoiding setting the default function if it is not
       * required.
       */
      ci_set_read_fn(ci_ptr, NULL, NULL);
   }

   return ci_ptr;
}


#ifdef CI_SEQUENTIAL_READ_SUPPORTED
/* Read the information before the actual image data.  This has been
 * changed in v0.90 to allow reading a file that already has the magic
 * bytes read from the stream.  You can tell libci how many bytes have
 * been read from the beginning of the stream (up to the maximum of 8)
 * via ci_set_sig_bytes(), and we will only check the remaining bytes
 * here.  The application can then have access to the signature bytes we
 * read if it is determined that this isn't a valid CI file.
 */
void CIAPI
ci_read_info(ci_structrp ci_ptr, ci_inforp info_ptr)
{
#ifdef CI_HANDLE_AS_UNKNOWN_SUPPORTED
   int keep;
#endif

   ci_debug(1, "in ci_read_info");

   if (ci_ptr == NULL || info_ptr == NULL)
      return;

   /* Read and check the CI file signature. */
   ci_read_sig(ci_ptr, info_ptr);

   for (;;)
   {
      ci_uint_32 length = ci_read_chunk_header(ci_ptr);
      ci_uint_32 chunk_name = ci_ptr->chunk_name;

      /* IDAT logic needs to happen here to simplify getting the two flags
       * right.
       */
      if (chunk_name == ci_IDAT)
      {
         if ((ci_ptr->mode & CI_HAVE_IHDR) == 0)
            ci_chunk_error(ci_ptr, "Missing IHDR before IDAT");

         else if (ci_ptr->color_type == CI_COLOR_TYPE_PALETTE &&
             (ci_ptr->mode & CI_HAVE_PLTE) == 0)
            ci_chunk_error(ci_ptr, "Missing PLTE before IDAT");

         else if ((ci_ptr->mode & CI_AFTER_IDAT) != 0)
            ci_chunk_benign_error(ci_ptr, "Too many IDATs found");

         ci_ptr->mode |= CI_HAVE_IDAT;
      }

      else if ((ci_ptr->mode & CI_HAVE_IDAT) != 0)
      {
         ci_ptr->mode |= CI_HAVE_CHUNK_AFTER_IDAT;
         ci_ptr->mode |= CI_AFTER_IDAT;
      }

      if (chunk_name == ci_IHDR)
         ci_handle_chunk(ci_ptr, info_ptr, length);

      else if (chunk_name == ci_IEND)
         ci_handle_chunk(ci_ptr, info_ptr, length);

#ifdef CI_HANDLE_AS_UNKNOWN_SUPPORTED
      else if ((keep = ci_chunk_unknown_handling(ci_ptr, chunk_name)) != 0)
      {
         ci_handle_unknown(ci_ptr, info_ptr, length, keep);

         if (chunk_name == ci_PLTE)
            ci_ptr->mode |= CI_HAVE_PLTE;

         else if (chunk_name == ci_IDAT)
         {
            ci_ptr->idat_size = 0; /* It has been consumed */
            break;
         }
      }
#endif

      else if (chunk_name == ci_IDAT)
      {
         ci_ptr->idat_size = length;
         break;
      }

      else
         ci_handle_chunk(ci_ptr, info_ptr, length);
   }
}
#endif /* SEQUENTIAL_READ */

/* Optional call to update the users info_ptr structure */
void CIAPI
ci_read_update_info(ci_structrp ci_ptr, ci_inforp info_ptr)
{
   ci_debug(1, "in ci_read_update_info");

   if (ci_ptr != NULL)
   {
      if ((ci_ptr->flags & CI_FLAG_ROW_INIT) == 0)
      {
         ci_read_start_row(ci_ptr);

#        ifdef CI_READ_TRANSFORMS_SUPPORTED
            ci_read_transform_info(ci_ptr, info_ptr);
#        else
            CI_UNUSED(info_ptr)
#        endif
      }

      /* New in 1.6.0 this avoids the bug of doing the initializations twice */
      else
         ci_app_error(ci_ptr,
             "ci_read_update_info/ci_start_read_image: duplicate call");
   }
}

#ifdef CI_SEQUENTIAL_READ_SUPPORTED
/* Initialize palette, background, etc, after transformations
 * are set, but before any reading takes place.  This allows
 * the user to obtain a gamma-corrected palette, for example.
 * If the user doesn't call this, we will do it ourselves.
 */
void CIAPI
ci_start_read_image(ci_structrp ci_ptr)
{
   ci_debug(1, "in ci_start_read_image");

   if (ci_ptr != NULL)
   {
      if ((ci_ptr->flags & CI_FLAG_ROW_INIT) == 0)
         ci_read_start_row(ci_ptr);

      /* New in 1.6.0 this avoids the bug of doing the initializations twice */
      else
         ci_app_error(ci_ptr,
             "ci_start_read_image/ci_read_update_info: duplicate call");
   }
}
#endif /* SEQUENTIAL_READ */

#ifdef CI_SEQUENTIAL_READ_SUPPORTED
#ifdef CI_MNG_FEATURES_SUPPORTED
/* Undoes intrapixel differencing,
 * NOTE: this is apparently only supported in the 'sequential' reader.
 */
static void
ci_do_read_intrapixel(ci_row_infop row_info, ci_bytep row)
{
   ci_debug(1, "in ci_do_read_intrapixel");

   if (
       (row_info->color_type & CI_COLOR_MASK_COLOR) != 0)
   {
      int bytes_per_pixel;
      ci_uint_32 row_width = row_info->width;

      if (row_info->bit_depth == 8)
      {
         ci_bytep rp;
         ci_uint_32 i;

         if (row_info->color_type == CI_COLOR_TYPE_RGB)
            bytes_per_pixel = 3;

         else if (row_info->color_type == CI_COLOR_TYPE_RGB_ALPHA)
            bytes_per_pixel = 4;

         else
            return;

         for (i = 0, rp = row; i < row_width; i++, rp += bytes_per_pixel)
         {
            *(rp) = (ci_byte)((256 + *rp + *(rp + 1)) & 0xff);
            *(rp+2) = (ci_byte)((256 + *(rp + 2) + *(rp + 1)) & 0xff);
         }
      }
      else if (row_info->bit_depth == 16)
      {
         ci_bytep rp;
         ci_uint_32 i;

         if (row_info->color_type == CI_COLOR_TYPE_RGB)
            bytes_per_pixel = 6;

         else if (row_info->color_type == CI_COLOR_TYPE_RGB_ALPHA)
            bytes_per_pixel = 8;

         else
            return;

         for (i = 0, rp = row; i < row_width; i++, rp += bytes_per_pixel)
         {
            ci_uint_32 s0   = (ci_uint_32)(*(rp    ) << 8) | *(rp + 1);
            ci_uint_32 s1   = (ci_uint_32)(*(rp + 2) << 8) | *(rp + 3);
            ci_uint_32 s2   = (ci_uint_32)(*(rp + 4) << 8) | *(rp + 5);
            ci_uint_32 red  = (s0 + s1 + 65536) & 0xffff;
            ci_uint_32 blue = (s2 + s1 + 65536) & 0xffff;
            *(rp    ) = (ci_byte)((red >> 8) & 0xff);
            *(rp + 1) = (ci_byte)(red & 0xff);
            *(rp + 4) = (ci_byte)((blue >> 8) & 0xff);
            *(rp + 5) = (ci_byte)(blue & 0xff);
         }
      }
   }
}
#endif /* MNG_FEATURES */

void CIAPI
ci_read_row(ci_structrp ci_ptr, ci_bytep row, ci_bytep dsp_row)
{
   ci_row_info row_info;

   if (ci_ptr == NULL)
      return;

   ci_debug2(1, "in ci_read_row (row %lu, pass %d)",
       (unsigned long)ci_ptr->row_number, ci_ptr->pass);

   /* ci_read_start_row sets the information (in particular iwidth) for this
    * interlace pass.
    */
   if ((ci_ptr->flags & CI_FLAG_ROW_INIT) == 0)
      ci_read_start_row(ci_ptr);

   /* 1.5.6: row_info moved out of ci_struct to a local here. */
   row_info.width = ci_ptr->iwidth; /* NOTE: width of current interlaced row */
   row_info.color_type = ci_ptr->color_type;
   row_info.bit_depth = ci_ptr->bit_depth;
   row_info.channels = ci_ptr->channels;
   row_info.pixel_depth = ci_ptr->pixel_depth;
   row_info.rowbytes = CI_ROWBYTES(row_info.pixel_depth, row_info.width);

#ifdef CI_WARNINGS_SUPPORTED
   if (ci_ptr->row_number == 0 && ci_ptr->pass == 0)
   {
   /* Check for transforms that have been set but were defined out */
#if defined(CI_WRITE_INVERT_SUPPORTED) && !defined(CI_READ_INVERT_SUPPORTED)
   if ((ci_ptr->transformations & CI_INVERT_MONO) != 0)
      ci_warning(ci_ptr, "CI_READ_INVERT_SUPPORTED is not defined");
#endif

#if defined(CI_WRITE_FILLER_SUPPORTED) && !defined(CI_READ_FILLER_SUPPORTED)
   if ((ci_ptr->transformations & CI_FILLER) != 0)
      ci_warning(ci_ptr, "CI_READ_FILLER_SUPPORTED is not defined");
#endif

#if defined(CI_WRITE_PACKSWAP_SUPPORTED) && \
    !defined(CI_READ_PACKSWAP_SUPPORTED)
   if ((ci_ptr->transformations & CI_PACKSWAP) != 0)
      ci_warning(ci_ptr, "CI_READ_PACKSWAP_SUPPORTED is not defined");
#endif

#if defined(CI_WRITE_PACK_SUPPORTED) && !defined(CI_READ_PACK_SUPPORTED)
   if ((ci_ptr->transformations & CI_PACK) != 0)
      ci_warning(ci_ptr, "CI_READ_PACK_SUPPORTED is not defined");
#endif

#if defined(CI_WRITE_SHIFT_SUPPORTED) && !defined(CI_READ_SHIFT_SUPPORTED)
   if ((ci_ptr->transformations & CI_SHIFT) != 0)
      ci_warning(ci_ptr, "CI_READ_SHIFT_SUPPORTED is not defined");
#endif

#if defined(CI_WRITE_BGR_SUPPORTED) && !defined(CI_READ_BGR_SUPPORTED)
   if ((ci_ptr->transformations & CI_BGR) != 0)
      ci_warning(ci_ptr, "CI_READ_BGR_SUPPORTED is not defined");
#endif

#if defined(CI_WRITE_SWAP_SUPPORTED) && !defined(CI_READ_SWAP_SUPPORTED)
   if ((ci_ptr->transformations & CI_SWAP_BYTES) != 0)
      ci_warning(ci_ptr, "CI_READ_SWAP_SUPPORTED is not defined");
#endif
   }
#endif /* WARNINGS */

#ifdef CI_READ_INTERLACING_SUPPORTED
   /* If interlaced and we do not need a new row, combine row and return.
    * Notice that the pixels we have from previous rows have been transformed
    * already; we can only combine like with like (transformed or
    * untransformed) and, because of the libci API for interlaced images, this
    * means we must transform before de-interlacing.
    */
   if (ci_ptr->interlaced != 0 &&
       (ci_ptr->transformations & CI_INTERLACE) != 0)
   {
      switch (ci_ptr->pass)
      {
         case 0:
            if (ci_ptr->row_number & 0x07)
            {
               if (dsp_row != NULL)
                  ci_combine_row(ci_ptr, dsp_row, 1/*display*/);
               ci_read_finish_row(ci_ptr);
               return;
            }
            break;

         case 1:
            if ((ci_ptr->row_number & 0x07) || ci_ptr->width < 5)
            {
               if (dsp_row != NULL)
                  ci_combine_row(ci_ptr, dsp_row, 1/*display*/);

               ci_read_finish_row(ci_ptr);
               return;
            }
            break;

         case 2:
            if ((ci_ptr->row_number & 0x07) != 4)
            {
               if (dsp_row != NULL && (ci_ptr->row_number & 4))
                  ci_combine_row(ci_ptr, dsp_row, 1/*display*/);

               ci_read_finish_row(ci_ptr);
               return;
            }
            break;

         case 3:
            if ((ci_ptr->row_number & 3) || ci_ptr->width < 3)
            {
               if (dsp_row != NULL)
                  ci_combine_row(ci_ptr, dsp_row, 1/*display*/);

               ci_read_finish_row(ci_ptr);
               return;
            }
            break;

         case 4:
            if ((ci_ptr->row_number & 3) != 2)
            {
               if (dsp_row != NULL && (ci_ptr->row_number & 2))
                  ci_combine_row(ci_ptr, dsp_row, 1/*display*/);

               ci_read_finish_row(ci_ptr);
               return;
            }
            break;

         case 5:
            if ((ci_ptr->row_number & 1) || ci_ptr->width < 2)
            {
               if (dsp_row != NULL)
                  ci_combine_row(ci_ptr, dsp_row, 1/*display*/);

               ci_read_finish_row(ci_ptr);
               return;
            }
            break;

         default:
         case 6:
            if ((ci_ptr->row_number & 1) == 0)
            {
               ci_read_finish_row(ci_ptr);
               return;
            }
            break;
      }
   }
#endif

   if ((ci_ptr->mode & CI_HAVE_IDAT) == 0)
      ci_error(ci_ptr, "Invalid attempt to read row data");

   /* Fill the row with IDAT data: */
   ci_ptr->row_buf[0]=255; /* to force error if no data was found */
   ci_read_IDAT_data(ci_ptr, ci_ptr->row_buf, row_info.rowbytes + 1);

   if (ci_ptr->row_buf[0] > CI_FILTER_VALUE_NONE)
   {
      if (ci_ptr->row_buf[0] < CI_FILTER_VALUE_LAST)
         ci_read_filter_row(ci_ptr, &row_info, ci_ptr->row_buf + 1,
             ci_ptr->prev_row + 1, ci_ptr->row_buf[0]);
      else
         ci_error(ci_ptr, "bad adaptive filter value");
   }

   /* libci 1.5.6: the following line was copying ci_ptr->rowbytes before
    * 1.5.6, while the buffer really is this big in current versions of libci
    * it may not be in the future, so this was changed just to copy the
    * interlaced count:
    */
   memcpy(ci_ptr->prev_row, ci_ptr->row_buf, row_info.rowbytes + 1);

#ifdef CI_MNG_FEATURES_SUPPORTED
   if ((ci_ptr->mng_features_permitted & CI_FLAG_MNG_FILTER_64) != 0 &&
       (ci_ptr->filter_type == CI_INTRAPIXEL_DIFFERENCING))
   {
      /* Intrapixel differencing */
      ci_do_read_intrapixel(&row_info, ci_ptr->row_buf + 1);
   }
#endif

#ifdef CI_READ_TRANSFORMS_SUPPORTED
   if (ci_ptr->transformations
#     ifdef CI_CHECK_FOR_INVALID_INDEX_SUPPORTED
         || ci_ptr->num_palette_max >= 0
#     endif
      )
      ci_do_read_transformations(ci_ptr, &row_info);
#endif

   /* The transformed pixel depth should match the depth now in row_info. */
   if (ci_ptr->transformed_pixel_depth == 0)
   {
      ci_ptr->transformed_pixel_depth = row_info.pixel_depth;
      if (row_info.pixel_depth > ci_ptr->maximum_pixel_depth)
         ci_error(ci_ptr, "sequential row overflow");
   }

   else if (ci_ptr->transformed_pixel_depth != row_info.pixel_depth)
      ci_error(ci_ptr, "internal sequential row size calculation error");

#ifdef CI_READ_INTERLACING_SUPPORTED
   /* Expand interlaced rows to full size */
   if (ci_ptr->interlaced != 0 &&
      (ci_ptr->transformations & CI_INTERLACE) != 0)
   {
      if (ci_ptr->pass < 6)
         ci_do_read_interlace(&row_info, ci_ptr->row_buf + 1, ci_ptr->pass,
             ci_ptr->transformations);

      if (dsp_row != NULL)
         ci_combine_row(ci_ptr, dsp_row, 1/*display*/);

      if (row != NULL)
         ci_combine_row(ci_ptr, row, 0/*row*/);
   }

   else
#endif
   {
      if (row != NULL)
         ci_combine_row(ci_ptr, row, -1/*ignored*/);

      if (dsp_row != NULL)
         ci_combine_row(ci_ptr, dsp_row, -1/*ignored*/);
   }
   ci_read_finish_row(ci_ptr);

   if (ci_ptr->read_row_fn != NULL)
      (*(ci_ptr->read_row_fn))(ci_ptr, ci_ptr->row_number, ci_ptr->pass);

}
#endif /* SEQUENTIAL_READ */

#ifdef CI_SEQUENTIAL_READ_SUPPORTED
/* Read one or more rows of image data.  If the image is interlaced,
 * and ci_set_interlace_handling() has been called, the rows need to
 * contain the contents of the rows from the previous pass.  If the
 * image has alpha or transparency, and ci_handle_alpha()[*] has been
 * called, the rows contents must be initialized to the contents of the
 * screen.
 *
 * "row" holds the actual image, and pixels are placed in it
 * as they arrive.  If the image is displayed after each pass, it will
 * appear to "sparkle" in.  "display_row" can be used to display a
 * "chunky" progressive image, with finer detail added as it becomes
 * available.  If you do not want this "chunky" display, you may pass
 * NULL for display_row.  If you do not want the sparkle display, and
 * you have not called ci_handle_alpha(), you may pass NULL for rows.
 * If you have called ci_handle_alpha(), and the image has either an
 * alpha channel or a transparency chunk, you must provide a buffer for
 * rows.  In this case, you do not have to provide a display_row buffer
 * also, but you may.  If the image is not interlaced, or if you have
 * not called ci_set_interlace_handling(), the display_row buffer will
 * be ignored, so pass NULL to it.
 *
 * [*] ci_handle_alpha() does not exist yet, as of this version of libci
 */

void CIAPI
ci_read_rows(ci_structrp ci_ptr, ci_bytepp row,
    ci_bytepp display_row, ci_uint_32 num_rows)
{
   ci_uint_32 i;
   ci_bytepp rp;
   ci_bytepp dp;

   ci_debug(1, "in ci_read_rows");

   if (ci_ptr == NULL)
      return;

   rp = row;
   dp = display_row;
   if (rp != NULL && dp != NULL)
      for (i = 0; i < num_rows; i++)
      {
         ci_bytep rptr = *rp++;
         ci_bytep dptr = *dp++;

         ci_read_row(ci_ptr, rptr, dptr);
      }

   else if (rp != NULL)
      for (i = 0; i < num_rows; i++)
      {
         ci_bytep rptr = *rp;
         ci_read_row(ci_ptr, rptr, NULL);
         rp++;
      }

   else if (dp != NULL)
      for (i = 0; i < num_rows; i++)
      {
         ci_bytep dptr = *dp;
         ci_read_row(ci_ptr, NULL, dptr);
         dp++;
      }
}
#endif /* SEQUENTIAL_READ */

#ifdef CI_SEQUENTIAL_READ_SUPPORTED
/* Read the entire image.  If the image has an alpha channel or a tRNS
 * chunk, and you have called ci_handle_alpha()[*], you will need to
 * initialize the image to the current image that CI will be overlaying.
 * We set the num_rows again here, in case it was incorrectly set in
 * ci_read_start_row() by a call to ci_read_update_info() or
 * ci_start_read_image() if ci_set_interlace_handling() wasn't called
 * prior to either of these functions like it should have been.  You can
 * only call this function once.  If you desire to have an image for
 * each pass of a interlaced image, use ci_read_rows() instead.
 *
 * [*] ci_handle_alpha() does not exist yet, as of this version of libci
 */
void CIAPI
ci_read_image(ci_structrp ci_ptr, ci_bytepp image)
{
   ci_uint_32 i, image_height;
   int pass, j;
   ci_bytepp rp;

   ci_debug(1, "in ci_read_image");

   if (ci_ptr == NULL)
      return;

#ifdef CI_READ_INTERLACING_SUPPORTED
   if ((ci_ptr->flags & CI_FLAG_ROW_INIT) == 0)
   {
      pass = ci_set_interlace_handling(ci_ptr);
      /* And make sure transforms are initialized. */
      ci_start_read_image(ci_ptr);
   }
   else
   {
      if (ci_ptr->interlaced != 0 &&
          (ci_ptr->transformations & CI_INTERLACE) == 0)
      {
         /* Caller called ci_start_read_image or ci_read_update_info without
          * first turning on the CI_INTERLACE transform.  We can fix this here,
          * but the caller should do it!
          */
         ci_warning(ci_ptr, "Interlace handling should be turned on when "
             "using ci_read_image");
         /* Make sure this is set correctly */
         ci_ptr->num_rows = ci_ptr->height;
      }

      /* Obtain the pass number, which also turns on the CI_INTERLACE flag in
       * the above error case.
       */
      pass = ci_set_interlace_handling(ci_ptr);
   }
#else
   if (ci_ptr->interlaced)
      ci_error(ci_ptr,
          "Cannot read interlaced image -- interlace handler disabled");

   pass = 1;
#endif

   image_height=ci_ptr->height;

   for (j = 0; j < pass; j++)
   {
      rp = image;
      for (i = 0; i < image_height; i++)
      {
         ci_read_row(ci_ptr, *rp, NULL);
         rp++;
      }
   }
}
#endif /* SEQUENTIAL_READ */

#ifdef CI_SEQUENTIAL_READ_SUPPORTED
/* Read the end of the CI file.  Will not read past the end of the
 * file, will verify the end is accurate, and will read any comments
 * or time information at the end of the file, if info is not NULL.
 */
void CIAPI
ci_read_end(ci_structrp ci_ptr, ci_inforp info_ptr)
{
#ifdef CI_HANDLE_AS_UNKNOWN_SUPPORTED
   int keep;
#endif

   ci_debug(1, "in ci_read_end");

   if (ci_ptr == NULL)
      return;

   /* If ci_read_end is called in the middle of reading the rows there may
    * still be pending IDAT data and an owned zstream.  Deal with this here.
    */
#ifdef CI_HANDLE_AS_UNKNOWN_SUPPORTED
   if (ci_chunk_unknown_handling(ci_ptr, ci_IDAT) == 0)
#endif
      ci_read_finish_IDAT(ci_ptr);

#ifdef CI_READ_CHECK_FOR_INVALID_INDEX_SUPPORTED
   /* Report invalid palette index; added at libng-1.5.10 */
   if (ci_ptr->color_type == CI_COLOR_TYPE_PALETTE &&
       ci_ptr->num_palette_max >= ci_ptr->num_palette)
      ci_benign_error(ci_ptr, "Read palette index exceeding num_palette");
#endif

   do
   {
      ci_uint_32 length = ci_read_chunk_header(ci_ptr);
      ci_uint_32 chunk_name = ci_ptr->chunk_name;

      if (chunk_name != ci_IDAT)
         ci_ptr->mode |= CI_HAVE_CHUNK_AFTER_IDAT;

      if (chunk_name == ci_IEND)
         ci_handle_chunk(ci_ptr, info_ptr, length);

      else if (chunk_name == ci_IHDR)
         ci_handle_chunk(ci_ptr, info_ptr, length);

      else if (info_ptr == NULL)
         ci_crc_finish(ci_ptr, length);

#ifdef CI_HANDLE_AS_UNKNOWN_SUPPORTED
      else if ((keep = ci_chunk_unknown_handling(ci_ptr, chunk_name)) != 0)
      {
         if (chunk_name == ci_IDAT)
         {
            if ((length > 0 && !(ci_ptr->flags & CI_FLAG_ZSTREAM_ENDED))
                || (ci_ptr->mode & CI_HAVE_CHUNK_AFTER_IDAT) != 0)
               ci_benign_error(ci_ptr, ".Too many IDATs found");
         }
         ci_handle_unknown(ci_ptr, info_ptr, length, keep);
         if (chunk_name == ci_PLTE)
            ci_ptr->mode |= CI_HAVE_PLTE;
      }
#endif

      else if (chunk_name == ci_IDAT)
      {
         /* Zero length IDATs are legal after the last IDAT has been
          * read, but not after other chunks have been read.  1.6 does not
          * always read all the deflate data; specifically it cannot be relied
          * upon to read the Adler32 at the end.  If it doesn't ignore IDAT
          * chunks which are longer than zero as well:
          */
         if ((length > 0 && !(ci_ptr->flags & CI_FLAG_ZSTREAM_ENDED))
             || (ci_ptr->mode & CI_HAVE_CHUNK_AFTER_IDAT) != 0)
            ci_benign_error(ci_ptr, "..Too many IDATs found");

         ci_crc_finish(ci_ptr, length);
      }

      else
         ci_handle_chunk(ci_ptr, info_ptr, length);
   } while ((ci_ptr->mode & CI_HAVE_IEND) == 0);
}
#endif /* SEQUENTIAL_READ */

/* Free all memory used in the read struct */
static void
ci_read_destroy(ci_structrp ci_ptr)
{
   ci_debug(1, "in ci_read_destroy");

#ifdef CI_READ_GAMMA_SUPPORTED
   ci_destroy_gamma_table(ci_ptr);
#endif

   ci_free(ci_ptr, ci_ptr->big_row_buf);
   ci_ptr->big_row_buf = NULL;
   ci_free(ci_ptr, ci_ptr->big_prev_row);
   ci_ptr->big_prev_row = NULL;
   ci_free(ci_ptr, ci_ptr->read_buffer);
   ci_ptr->read_buffer = NULL;

#ifdef CI_READ_QUANTIZE_SUPPORTED
   ci_free(ci_ptr, ci_ptr->palette_lookup);
   ci_ptr->palette_lookup = NULL;
   ci_free(ci_ptr, ci_ptr->quantize_index);
   ci_ptr->quantize_index = NULL;
#endif

   if ((ci_ptr->free_me & CI_FREE_PLTE) != 0)
   {
      ci_zfree(ci_ptr, ci_ptr->palette);
      ci_ptr->palette = NULL;
   }
   ci_ptr->free_me &= ~CI_FREE_PLTE;

#if defined(CI_tRNS_SUPPORTED) || \
    defined(CI_READ_EXPAND_SUPPORTED) || defined(CI_READ_BACKGROUND_SUPPORTED)
   if ((ci_ptr->free_me & CI_FREE_TRNS) != 0)
   {
      ci_free(ci_ptr, ci_ptr->trans_alpha);
      ci_ptr->trans_alpha = NULL;
   }
   ci_ptr->free_me &= ~CI_FREE_TRNS;
#endif

   inflateEnd(&ci_ptr->zstream);

#ifdef CI_PROGRESSIVE_READ_SUPPORTED
   ci_free(ci_ptr, ci_ptr->save_buffer);
   ci_ptr->save_buffer = NULL;
#endif

#if defined(CI_STORE_UNKNOWN_CHUNKS_SUPPORTED) && \
   defined(CI_READ_UNKNOWN_CHUNKS_SUPPORTED)
   ci_free(ci_ptr, ci_ptr->unknown_chunk.data);
   ci_ptr->unknown_chunk.data = NULL;
#endif

#ifdef CI_SET_UNKNOWN_CHUNKS_SUPPORTED
   ci_free(ci_ptr, ci_ptr->chunk_list);
   ci_ptr->chunk_list = NULL;
#endif

#if defined(CI_READ_EXPAND_SUPPORTED) && \
    (defined(CI_ARM_NEON_IMPLEMENTATION) || \
     defined(CI_RISCV_RVV_IMPLEMENTATION))
   ci_free(ci_ptr, ci_ptr->riffled_palette);
   ci_ptr->riffled_palette = NULL;
#endif

   /* NOTE: the 'setjmp' buffer may still be allocated and the memory and error
    * callbacks are still set at this point.  They are required to complete the
    * destruction of the ci_struct itself.
    */
}

/* Free all memory used by the read */
void CIAPI
ci_destroy_read_struct(ci_structpp ci_ptr_ptr, ci_infopp info_ptr_ptr,
    ci_infopp end_info_ptr_ptr)
{
   ci_structrp ci_ptr = NULL;

   ci_debug(1, "in ci_destroy_read_struct");

   if (ci_ptr_ptr != NULL)
      ci_ptr = *ci_ptr_ptr;

   if (ci_ptr == NULL)
      return;

   /* libci 1.6.0: use the API to destroy info structs to ensure consistent
    * behavior.  Prior to 1.6.0 libci did extra 'info' destruction in this API.
    * The extra was, apparently, unnecessary yet this hides memory leak bugs.
    */
   ci_destroy_info_struct(ci_ptr, end_info_ptr_ptr);
   ci_destroy_info_struct(ci_ptr, info_ptr_ptr);

   *ci_ptr_ptr = NULL;
   ci_read_destroy(ci_ptr);
   ci_destroy_ci_struct(ci_ptr);
}

void CIAPI
ci_set_read_status_fn(ci_structrp ci_ptr, ci_read_status_ptr read_row_fn)
{
   if (ci_ptr == NULL)
      return;

   ci_ptr->read_row_fn = read_row_fn;
}


#ifdef CI_SEQUENTIAL_READ_SUPPORTED
#ifdef CI_INFO_IMAGE_SUPPORTED
void CIAPI
ci_read_ci(ci_structrp ci_ptr, ci_inforp info_ptr,
    int transforms, voidp params)
{
   ci_debug(1, "in ci_read_ci");

   if (ci_ptr == NULL || info_ptr == NULL)
      return;

   /* ci_read_info() gives us all of the information from the
    * CI file before the first IDAT (image data chunk).
    */
   ci_read_info(ci_ptr, info_ptr);
   if (info_ptr->height > CI_UINT_32_MAX/(sizeof (ci_bytep)))
      ci_error(ci_ptr, "Image is too high to process with ci_read_ci()");

   /* -------------- image transformations start here ------------------- */
   /* libci 1.6.10: add code to cause a ci_app_error if a selected TRANSFORM
    * is not implemented.  This will only happen in de-configured (non-default)
    * libci builds.  The results can be unexpected - ci_read_ci may return
    * short or mal-formed rows because the transform is skipped.
    */

   /* Tell libci to strip 16-bit/color files down to 8 bits per color.
    */
   if ((transforms & CI_TRANSFORM_SCALE_16) != 0)
      /* Added at libci-1.5.4. "strip_16" produces the same result that it
       * did in earlier versions, while "scale_16" is now more accurate.
       */
#ifdef CI_READ_SCALE_16_TO_8_SUPPORTED
      ci_set_scale_16(ci_ptr);
#else
      ci_app_error(ci_ptr, "CI_TRANSFORM_SCALE_16 not supported");
#endif

   /* If both SCALE and STRIP are required cirtran will effectively cancel the
    * latter by doing SCALE first.  This is ok and allows apps not to check for
    * which is supported to get the right answer.
    */
   if ((transforms & CI_TRANSFORM_STRIP_16) != 0)
#ifdef CI_READ_STRIP_16_TO_8_SUPPORTED
      ci_set_strip_16(ci_ptr);
#else
      ci_app_error(ci_ptr, "CI_TRANSFORM_STRIP_16 not supported");
#endif

   /* Strip alpha bytes from the input data without combining with
    * the background (not recommended).
    */
   if ((transforms & CI_TRANSFORM_STRIP_ALPHA) != 0)
#ifdef CI_READ_STRIP_ALPHA_SUPPORTED
      ci_set_strip_alpha(ci_ptr);
#else
      ci_app_error(ci_ptr, "CI_TRANSFORM_STRIP_ALPHA not supported");
#endif

   /* Extract multiple pixels with bit depths of 1, 2, or 4 from a single
    * byte into separate bytes (useful for paletted and grayscale images).
    */
   if ((transforms & CI_TRANSFORM_PACKING) != 0)
#ifdef CI_READ_PACK_SUPPORTED
      ci_set_packing(ci_ptr);
#else
      ci_app_error(ci_ptr, "CI_TRANSFORM_PACKING not supported");
#endif

   /* Change the order of packed pixels to least significant bit first
    * (not useful if you are using ci_set_packing).
    */
   if ((transforms & CI_TRANSFORM_PACKSWAP) != 0)
#ifdef CI_READ_PACKSWAP_SUPPORTED
      ci_set_packswap(ci_ptr);
#else
      ci_app_error(ci_ptr, "CI_TRANSFORM_PACKSWAP not supported");
#endif

   /* Expand paletted colors into true RGB triplets
    * Expand grayscale images to full 8 bits from 1, 2, or 4 bits/pixel
    * Expand paletted or RGB images with transparency to full alpha
    * channels so the data will be available as RGBA quartets.
    */
   if ((transforms & CI_TRANSFORM_EXPAND) != 0)
#ifdef CI_READ_EXPAND_SUPPORTED
      ci_set_expand(ci_ptr);
#else
      ci_app_error(ci_ptr, "CI_TRANSFORM_EXPAND not supported");
#endif

   /* We don't handle background color or gamma transformation or quantizing.
    */

   /* Invert monochrome files to have 0 as white and 1 as black
    */
   if ((transforms & CI_TRANSFORM_INVERT_MONO) != 0)
#ifdef CI_READ_INVERT_SUPPORTED
      ci_set_invert_mono(ci_ptr);
#else
      ci_app_error(ci_ptr, "CI_TRANSFORM_INVERT_MONO not supported");
#endif

   /* If you want to shift the pixel values from the range [0,255] or
    * [0,65535] to the original [0,7] or [0,31], or whatever range the
    * colors were originally in:
    */
   if ((transforms & CI_TRANSFORM_SHIFT) != 0)
#ifdef CI_READ_SHIFT_SUPPORTED
      if ((info_ptr->valid & CI_INFO_sBIT) != 0)
         ci_set_shift(ci_ptr, &info_ptr->sig_bit);
#else
      ci_app_error(ci_ptr, "CI_TRANSFORM_SHIFT not supported");
#endif

   /* Flip the RGB pixels to BGR (or RGBA to BGRA) */
   if ((transforms & CI_TRANSFORM_BGR) != 0)
#ifdef CI_READ_BGR_SUPPORTED
      ci_set_bgr(ci_ptr);
#else
      ci_app_error(ci_ptr, "CI_TRANSFORM_BGR not supported");
#endif

   /* Swap the RGBA or GA data to ARGB or AG (or BGRA to ABGR) */
   if ((transforms & CI_TRANSFORM_SWAP_ALPHA) != 0)
#ifdef CI_READ_SWAP_ALPHA_SUPPORTED
      ci_set_swap_alpha(ci_ptr);
#else
      ci_app_error(ci_ptr, "CI_TRANSFORM_SWAP_ALPHA not supported");
#endif

   /* Swap bytes of 16-bit files to least significant byte first */
   if ((transforms & CI_TRANSFORM_SWAP_ENDIAN) != 0)
#ifdef CI_READ_SWAP_SUPPORTED
      ci_set_swap(ci_ptr);
#else
      ci_app_error(ci_ptr, "CI_TRANSFORM_SWAP_ENDIAN not supported");
#endif

/* Added at libci-1.2.41 */
   /* Invert the alpha channel from opacity to transparency */
   if ((transforms & CI_TRANSFORM_INVERT_ALPHA) != 0)
#ifdef CI_READ_INVERT_ALPHA_SUPPORTED
      ci_set_invert_alpha(ci_ptr);
#else
      ci_app_error(ci_ptr, "CI_TRANSFORM_INVERT_ALPHA not supported");
#endif

/* Added at libci-1.2.41 */
   /* Expand grayscale image to RGB */
   if ((transforms & CI_TRANSFORM_GRAY_TO_RGB) != 0)
#ifdef CI_READ_GRAY_TO_RGB_SUPPORTED
      ci_set_gray_to_rgb(ci_ptr);
#else
      ci_app_error(ci_ptr, "CI_TRANSFORM_GRAY_TO_RGB not supported");
#endif

/* Added at libci-1.5.4 */
   if ((transforms & CI_TRANSFORM_EXPAND_16) != 0)
#ifdef CI_READ_EXPAND_16_SUPPORTED
      ci_set_expand_16(ci_ptr);
#else
      ci_app_error(ci_ptr, "CI_TRANSFORM_EXPAND_16 not supported");
#endif

   /* We don't handle adding filler bytes */

   /* We use ci_read_image and rely on that for interlace handling, but we also
    * call ci_read_update_info therefore must turn on interlace handling now:
    */
   (void)ci_set_interlace_handling(ci_ptr);

   /* Optional call to gamma correct and add the background to the palette
    * and update info structure.  REQUIRED if you are expecting libci to
    * update the palette for you (i.e., you selected such a transform above).
    */
   ci_read_update_info(ci_ptr, info_ptr);

   /* -------------- image transformations end here ------------------- */

   ci_free_data(ci_ptr, info_ptr, CI_FREE_ROWS, 0);
   if (info_ptr->row_pointers == NULL)
   {
      ci_uint_32 iptr;

      info_ptr->row_pointers = ci_voidcast(ci_bytepp, ci_malloc(ci_ptr,
          info_ptr->height * (sizeof (ci_bytep))));

      for (iptr=0; iptr<info_ptr->height; iptr++)
         info_ptr->row_pointers[iptr] = NULL;

      info_ptr->free_me |= CI_FREE_ROWS;

      for (iptr = 0; iptr < info_ptr->height; iptr++)
         info_ptr->row_pointers[iptr] = ci_voidcast(ci_bytep,
             ci_malloc(ci_ptr, info_ptr->rowbytes));
   }

   ci_read_image(ci_ptr, info_ptr->row_pointers);
   info_ptr->valid |= CI_INFO_IDAT;

   /* Read rest of file, and get additional chunks in info_ptr - REQUIRED */
   ci_read_end(ci_ptr, info_ptr);

   CI_UNUSED(params)
}
#endif /* INFO_IMAGE */
#endif /* SEQUENTIAL_READ */

#ifdef CI_SIMPLIFIED_READ_SUPPORTED
/* SIMPLIFIED READ
 *
 * This code currently relies on the sequential reader, though it could easily
 * be made to work with the progressive one.
 */
/* Arguments to ci_image_finish_read: */

/* Encoding of CI data (used by the color-map code) */
#  define P_NOTSET  0 /* File encoding not yet known */
#  define P_sRGB    1 /* 8-bit encoded to sRGB gamma */
#  define P_LINEAR  2 /* 16-bit linear: not encoded, NOT pre-multiplied! */
#  define P_FILE    3 /* 8-bit encoded to file gamma, not sRGB or linear */
#  define P_LINEAR8 4 /* 8-bit linear: only from a file value */

/* Color-map processing: after libci has run on the CI image further
 * processing may be needed to convert the data to color-map indices.
 */
#define CI_CMAP_NONE      0
#define CI_CMAP_GA        1 /* Process GA data to a color-map with alpha */
#define CI_CMAP_TRANS     2 /* Process GA data to a background index */
#define CI_CMAP_RGB       3 /* Process RGB data */
#define CI_CMAP_RGB_ALPHA 4 /* Process RGBA data */

/* The following document where the background is for each processing case. */
#define CI_CMAP_NONE_BACKGROUND      256
#define CI_CMAP_GA_BACKGROUND        231
#define CI_CMAP_TRANS_BACKGROUND     254
#define CI_CMAP_RGB_BACKGROUND       256
#define CI_CMAP_RGB_ALPHA_BACKGROUND 216

typedef struct
{
   /* Arguments: */
   ci_imagep image;
   ci_voidp  buffer;
   ci_int_32 row_stride;
   ci_voidp  colormap;
   ci_const_colorp background;
   /* Local variables: */
   ci_voidp       local_row;
   ci_voidp       first_row;
   ptrdiff_t       row_bytes;           /* step between rows */
   int             file_encoding;       /* E_ values above */
   ci_fixed_point gamma_to_linear;     /* For P_FILE, reciprocal of gamma */
   int             colormap_processing; /* CI_CMAP_ values above */
} ci_image_read_control;

/* Do all the *safe* initialization - 'safe' means that ci_error won't be
 * called, so setting up the jmp_buf is not required.  This means that anything
 * called from here must *not* call ci_malloc - it has to call ci_malloc_warn
 * instead so that control is returned safely back to this routine.
 */
static int
ci_image_read_init(ci_imagep image)
{
   if (image->opaque == NULL)
   {
      ci_structp ci_ptr = ci_create_read_struct(CI_LIBCI_VER_STRING, image,
          ci_safe_error, ci_safe_warning);

      /* And set the rest of the structure to NULL to ensure that the various
       * fields are consistent.
       */
      memset(image, 0, (sizeof *image));
      image->version = CI_IMAGE_VERSION;

      if (ci_ptr != NULL)
      {
         ci_infop info_ptr = ci_create_info_struct(ci_ptr);

         if (info_ptr != NULL)
         {
            ci_controlp control = ci_voidcast(ci_controlp,
                ci_malloc_warn(ci_ptr, (sizeof *control)));

            if (control != NULL)
            {
               memset(control, 0, (sizeof *control));

               control->ci_ptr = ci_ptr;
               control->info_ptr = info_ptr;
               control->for_write = 0;

               image->opaque = control;
               return 1;
            }

            /* Error clean up */
            ci_destroy_info_struct(ci_ptr, &info_ptr);
         }

         ci_destroy_read_struct(&ci_ptr, NULL, NULL);
      }

      return ci_image_error(image, "ci_image_read: out of memory");
   }

   return ci_image_error(image, "ci_image_read: opaque pointer not NULL");
}

/* Utility to find the base format of a CI file from a ci_struct. */
static ci_uint_32
ci_image_format(ci_structrp ci_ptr)
{
   ci_uint_32 format = 0;

   if ((ci_ptr->color_type & CI_COLOR_MASK_COLOR) != 0)
      format |= CI_FORMAT_FLAG_COLOR;

   if ((ci_ptr->color_type & CI_COLOR_MASK_ALPHA) != 0)
      format |= CI_FORMAT_FLAG_ALPHA;

   /* Use ci_ptr here, not info_ptr, because by examination ci_handle_tRNS
    * sets the ci_struct fields; that's all we are interested in here.  The
    * precise interaction with an app call to ci_set_tRNS and CI file reading
    * is unclear.
    */
   else if (ci_ptr->num_trans > 0)
      format |= CI_FORMAT_FLAG_ALPHA;

   if (ci_ptr->bit_depth == 16)
      format |= CI_FORMAT_FLAG_LINEAR;

   if ((ci_ptr->color_type & CI_COLOR_MASK_PALETTE) != 0)
      format |= CI_FORMAT_FLAG_COLORMAP;

   return format;
}

static int
chromaticities_match_sRGB(const ci_xy *xy)
{
#  define sRGB_TOLERANCE 1000
   static const ci_xy sRGB_xy = /* From ITU-R BT.709-3 */
   {
      /* color      x       y */
      /* red   */ 64000, 33000,
      /* green */ 30000, 60000,
      /* blue  */ 15000,  6000,
      /* white */ 31270, 32900
   };

   if (CI_OUT_OF_RANGE(xy->whitex, sRGB_xy.whitex,sRGB_TOLERANCE) ||
       CI_OUT_OF_RANGE(xy->whitey, sRGB_xy.whitey,sRGB_TOLERANCE) ||
       CI_OUT_OF_RANGE(xy->redx,   sRGB_xy.redx,  sRGB_TOLERANCE) ||
       CI_OUT_OF_RANGE(xy->redy,   sRGB_xy.redy,  sRGB_TOLERANCE) ||
       CI_OUT_OF_RANGE(xy->greenx, sRGB_xy.greenx,sRGB_TOLERANCE) ||
       CI_OUT_OF_RANGE(xy->greeny, sRGB_xy.greeny,sRGB_TOLERANCE) ||
       CI_OUT_OF_RANGE(xy->bluex,  sRGB_xy.bluex, sRGB_TOLERANCE) ||
       CI_OUT_OF_RANGE(xy->bluey,  sRGB_xy.bluey, sRGB_TOLERANCE))
      return 0;
   return 1;
}

/* Is the given gamma significantly different from sRGB?  The test is the same
 * one used in cirtran.c when deciding whether to do gamma correction.  The
 * arithmetic optimizes the division by using the fact that the inverse of the
 * file sRGB gamma is 2.2
 */
static int
ci_gamma_not_sRGB(ci_fixed_point g)
{
   /* 1.6.47: use the same sanity checks as used in cirtran.c */
   if (g < CI_LIB_GAMMA_MIN || g > CI_LIB_GAMMA_MAX)
      return 0; /* Includes the uninitialized value 0 */

   return ci_gamma_significant((g * 11 + 2)/5 /* i.e. *2.2, rounded */);
}

/* Do the main body of a 'ci_image_begin_read' function; read the CI file
 * header and fill in all the information.  This is executed in a safe context,
 * unlike the init routine above.
 */
static int
ci_image_is_not_sRGB(ci_const_structrp ci_ptr)
{
   /* Does the colorspace **not** match sRGB?  The flag is only set if the
    * answer can be determined reliably.
    *
    * ci_struct::chromaticities always exists since the simplified API
    * requires rgb-to-gray.  The mDCV, cICP and cHRM chunks may all set it to
    * a non-sRGB value, so it needs to be checked but **only** if one of
    * those chunks occured in the file.
    */
   /* Highest priority: check to be safe. */
   if (ci_has_chunk(ci_ptr, cICP) || ci_has_chunk(ci_ptr, mDCV))
      return !chromaticities_match_sRGB(&ci_ptr->chromaticities);

   /* If the image is marked as sRGB then it is... */
   if (ci_has_chunk(ci_ptr, sRGB))
      return 0;

   /* Last stop: cHRM, must check: */
   if (ci_has_chunk(ci_ptr, cHRM))
      return !chromaticities_match_sRGB(&ci_ptr->chromaticities);

   /* Else default to sRGB */
   return 0;
}

static int
ci_image_read_header(ci_voidp argument)
{
   ci_imagep image = ci_voidcast(ci_imagep, argument);
   ci_structrp ci_ptr = image->opaque->ci_ptr;
   ci_inforp info_ptr = image->opaque->info_ptr;

#ifdef CI_BENIGN_ERRORS_SUPPORTED
   ci_set_benign_errors(ci_ptr, 1/*warn*/);
#endif
   ci_read_info(ci_ptr, info_ptr);

   /* Do this the fast way; just read directly out of ci_struct. */
   image->width = ci_ptr->width;
   image->height = ci_ptr->height;

   {
      ci_uint_32 format = ci_image_format(ci_ptr);

      image->format = format;

      /* Greyscale images don't (typically) have colour space information and
       * using it is pretty much impossible, so use sRGB for grayscale (it
       * doesn't matter r==g==b so the transform is irrelevant.)
       */
      if ((format & CI_FORMAT_FLAG_COLOR) != 0 &&
          ci_image_is_not_sRGB(ci_ptr))
         image->flags |= CI_IMAGE_FLAG_COLORSPACE_NOT_sRGB;
   }

   /* We need the maximum number of entries regardless of the format the
    * application sets here.
    */
   {
      ci_uint_32 cmap_entries;

      switch (ci_ptr->color_type)
      {
         case CI_COLOR_TYPE_GRAY:
            cmap_entries = 1U << ci_ptr->bit_depth;
            break;

         case CI_COLOR_TYPE_PALETTE:
            cmap_entries = (ci_uint_32)ci_ptr->num_palette;
            break;

         default:
            cmap_entries = 256;
            break;
      }

      if (cmap_entries > 256)
         cmap_entries = 256;

      image->colormap_entries = cmap_entries;
   }

   return 1;
}

#ifdef CI_STDIO_SUPPORTED
int CIAPI
ci_image_begin_read_from_stdio(ci_imagep image, FILE *file)
{
   if (image != NULL && image->version == CI_IMAGE_VERSION)
   {
      if (file != NULL)
      {
         if (ci_image_read_init(image) != 0)
         {
            /* This is slightly evil, but ci_init_io doesn't do anything other
             * than this and we haven't changed the standard IO functions so
             * this saves a 'safe' function.
             */
            image->opaque->ci_ptr->io_ptr = file;
            return ci_safe_execute(image, ci_image_read_header, image);
         }
      }

      else
         return ci_image_error(image,
             "ci_image_begin_read_from_stdio: invalid argument");
   }

   else if (image != NULL)
      return ci_image_error(image,
          "ci_image_begin_read_from_stdio: incorrect CI_IMAGE_VERSION");

   return 0;
}

int CIAPI
ci_image_begin_read_from_file(ci_imagep image, const char *file_name)
{
   if (image != NULL && image->version == CI_IMAGE_VERSION)
   {
      if (file_name != NULL)
      {
         FILE *fp = fopen(file_name, "rb");

         if (fp != NULL)
         {
            if (ci_image_read_init(image) != 0)
            {
               image->opaque->ci_ptr->io_ptr = fp;
               image->opaque->owned_file = 1;
               return ci_safe_execute(image, ci_image_read_header, image);
            }

            /* Clean up: just the opened file. */
            (void)fclose(fp);
         }

         else
            return ci_image_error(image, strerror(errno));
      }

      else
         return ci_image_error(image,
             "ci_image_begin_read_from_file: invalid argument");
   }

   else if (image != NULL)
      return ci_image_error(image,
          "ci_image_begin_read_from_file: incorrect CI_IMAGE_VERSION");

   return 0;
}
#endif /* STDIO */

static void CICBAPI
ci_image_memory_read(ci_structp ci_ptr, ci_bytep out, size_t need)
{
   if (ci_ptr != NULL)
   {
      ci_imagep image = ci_voidcast(ci_imagep, ci_ptr->io_ptr);
      if (image != NULL)
      {
         ci_controlp cp = image->opaque;
         if (cp != NULL)
         {
            ci_const_bytep memory = cp->memory;
            size_t size = cp->size;

            if (memory != NULL && size >= need)
            {
               memcpy(out, memory, need);
               cp->memory = memory + need;
               cp->size = size - need;
               return;
            }

            ci_error(ci_ptr, "read beyond end of data");
         }
      }

      ci_error(ci_ptr, "invalid memory read");
   }
}

int CIAPI ci_image_begin_read_from_memory(ci_imagep image,
    ci_const_voidp memory, size_t size)
{
   if (image != NULL && image->version == CI_IMAGE_VERSION)
   {
      if (memory != NULL && size > 0)
      {
         if (ci_image_read_init(image) != 0)
         {
            /* Now set the IO functions to read from the memory buffer and
             * store it into io_ptr.  Again do this in-place to avoid calling a
             * libci function that requires error handling.
             */
            image->opaque->memory = ci_voidcast(ci_const_bytep, memory);
            image->opaque->size = size;
            image->opaque->ci_ptr->io_ptr = image;
            image->opaque->ci_ptr->read_data_fn = ci_image_memory_read;

            return ci_safe_execute(image, ci_image_read_header, image);
         }
      }

      else
         return ci_image_error(image,
             "ci_image_begin_read_from_memory: invalid argument");
   }

   else if (image != NULL)
      return ci_image_error(image,
          "ci_image_begin_read_from_memory: incorrect CI_IMAGE_VERSION");

   return 0;
}

/* Utility function to skip chunks that are not used by the simplified image
 * read functions and an appropriate macro to call it.
 */
#ifdef CI_HANDLE_AS_UNKNOWN_SUPPORTED
static void
ci_image_skip_unused_chunks(ci_structrp ci_ptr)
{
   /* Prepare the reader to ignore all recognized chunks whose data will not
    * be used, i.e., all chunks recognized by libci except for those
    * involved in basic image reading:
    *
    *    IHDR, PLTE, IDAT, IEND
    *
    * Or image data handling:
    *
    *    tRNS, bKGD, gAMA, cHRM, sRGB, [iCCP] and sBIT.
    *
    * This provides a small performance improvement and eliminates any
    * potential vulnerability to security problems in the unused chunks.
    *
    * At present the iCCP chunk data isn't used, so iCCP chunk can be ignored
    * too.  This allows the simplified API to be compiled without iCCP support.
    */
   {
         static const ci_byte chunks_to_process[] = {
            98,  75,  71,  68, '\0',  /* bKGD */
            99,  72,  82,  77, '\0',  /* cHRM */
            99,  73,  67,  80, '\0',  /* cICP */
           103,  65,  77,  65, '\0',  /* gAMA */
           109,  68,  67,  86, '\0',  /* mDCV */
           115,  66,  73,  84, '\0',  /* sBIT */
           115,  82,  71,  66, '\0',  /* sRGB */
         };

       /* Ignore unknown chunks and all other chunks except for the
        * IHDR, PLTE, tRNS, IDAT, and IEND chunks.
        */
       ci_set_keep_unknown_chunks(ci_ptr, CI_HANDLE_CHUNK_NEVER,
           NULL, -1);

       /* But do not ignore image data handling chunks */
       ci_set_keep_unknown_chunks(ci_ptr, CI_HANDLE_CHUNK_AS_DEFAULT,
           chunks_to_process, (int)/*SAFE*/(sizeof chunks_to_process)/5);
   }
}

#  define CI_SKIP_CHUNKS(p) ci_image_skip_unused_chunks(p)
#else
#  define CI_SKIP_CHUNKS(p) ((void)0)
#endif /* HANDLE_AS_UNKNOWN */

/* The following macro gives the exact rounded answer for all values in the
 * range 0..255 (it actually divides by 51.2, but the rounding still generates
 * the correct numbers 0..5
 */
#define CI_DIV51(v8) (((v8) * 5 + 130) >> 8)

/* Utility functions to make particular color-maps */
static void
set_file_encoding(ci_image_read_control *display)
{
   ci_structrp ci_ptr = display->image->opaque->ci_ptr;
   ci_fixed_point g = ci_resolve_file_gamma(ci_ptr);

   /* CIv3: the result may be 0 however the 'default_gamma' should have been
    * set before this is called so zero is an error:
    */
   if (g == 0)
      ci_error(ci_ptr, "internal: default gamma not set");

   if (ci_gamma_significant(g) != 0)
   {
      if (ci_gamma_not_sRGB(g) != 0)
      {
         display->file_encoding = P_FILE;
         display->gamma_to_linear = ci_reciprocal(g);
      }

      else
         display->file_encoding = P_sRGB;
   }

   else
      display->file_encoding = P_LINEAR8;
}

static unsigned int
decode_gamma(ci_image_read_control *display, ci_uint_32 value, int encoding)
{
   if (encoding == P_FILE) /* double check */
      encoding = display->file_encoding;

   if (encoding == P_NOTSET) /* must be the file encoding */
   {
      set_file_encoding(display);
      encoding = display->file_encoding;
   }

   switch (encoding)
   {
      case P_FILE:
         value = ci_gamma_16bit_correct(value*257, display->gamma_to_linear);
         break;

      case P_sRGB:
         value = ci_sRGB_table[value];
         break;

      case P_LINEAR:
         break;

      case P_LINEAR8:
         value *= 257;
         break;

#ifdef __GNUC__
      default:
         ci_error(display->image->opaque->ci_ptr,
             "unexpected encoding (internal error)");
#endif
   }

   return value;
}

static ci_uint_32
ci_colormap_compose(ci_image_read_control *display,
    ci_uint_32 foreground, int foreground_encoding, ci_uint_32 alpha,
    ci_uint_32 background, int encoding)
{
   /* The file value is composed on the background, the background has the given
    * encoding and so does the result, the file is encoded with P_FILE and the
    * file and alpha are 8-bit values.  The (output) encoding will always be
    * P_LINEAR or P_sRGB.
    */
   ci_uint_32 f = decode_gamma(display, foreground, foreground_encoding);
   ci_uint_32 b = decode_gamma(display, background, encoding);

   /* The alpha is always an 8-bit value (it comes from the palette), the value
    * scaled by 255 is what CI_sRGB_FROM_LINEAR requires.
    */
   f = f * alpha + b * (255-alpha);

   if (encoding == P_LINEAR)
   {
      /* Scale to 65535; divide by 255, approximately (in fact this is extremely
       * accurate, it divides by 255.00000005937181414556, with no overflow.)
       */
      f *= 257; /* Now scaled by 65535 */
      f += f >> 16;
      f = (f+32768) >> 16;
   }

   else /* P_sRGB */
      f = CI_sRGB_FROM_LINEAR(f);

   return f;
}

/* NOTE: P_LINEAR values to this routine must be 16-bit, but P_FILE values must
 * be 8-bit.
 */
static void
ci_create_colormap_entry(ci_image_read_control *display,
    ci_uint_32 ip, ci_uint_32 red, ci_uint_32 green, ci_uint_32 blue,
    ci_uint_32 alpha, int encoding)
{
   ci_imagep image = display->image;
   int output_encoding = (image->format & CI_FORMAT_FLAG_LINEAR) != 0 ?
       P_LINEAR : P_sRGB;
   int convert_to_Y = (image->format & CI_FORMAT_FLAG_COLOR) == 0 &&
       (red != green || green != blue);

   if (ip > 255)
      ci_error(image->opaque->ci_ptr, "color-map index out of range");

   /* Update the cache with whether the file gamma is significantly different
    * from sRGB.
    */
   if (encoding == P_FILE)
   {
      if (display->file_encoding == P_NOTSET)
         set_file_encoding(display);

      /* Note that the cached value may be P_FILE too, but if it is then the
       * gamma_to_linear member has been set.
       */
      encoding = display->file_encoding;
   }

   if (encoding == P_FILE)
   {
      ci_fixed_point g = display->gamma_to_linear;

      red = ci_gamma_16bit_correct(red*257, g);
      green = ci_gamma_16bit_correct(green*257, g);
      blue = ci_gamma_16bit_correct(blue*257, g);

      if (convert_to_Y != 0 || output_encoding == P_LINEAR)
      {
         alpha *= 257;
         encoding = P_LINEAR;
      }

      else
      {
         red = CI_sRGB_FROM_LINEAR(red * 255);
         green = CI_sRGB_FROM_LINEAR(green * 255);
         blue = CI_sRGB_FROM_LINEAR(blue * 255);
         encoding = P_sRGB;
      }
   }

   else if (encoding == P_LINEAR8)
   {
      /* This encoding occurs quite frequently in test cases because CiSuite
       * includes a gAMA 1.0 chunk with most images.
       */
      red *= 257;
      green *= 257;
      blue *= 257;
      alpha *= 257;
      encoding = P_LINEAR;
   }

   else if (encoding == P_sRGB &&
       (convert_to_Y  != 0 || output_encoding == P_LINEAR))
   {
      /* The values are 8-bit sRGB values, but must be converted to 16-bit
       * linear.
       */
      red = ci_sRGB_table[red];
      green = ci_sRGB_table[green];
      blue = ci_sRGB_table[blue];
      alpha *= 257;
      encoding = P_LINEAR;
   }

   /* This is set if the color isn't gray but the output is. */
   if (encoding == P_LINEAR)
   {
      if (convert_to_Y != 0)
      {
         /* NOTE: these values are copied from ci_do_rgb_to_gray */
         ci_uint_32 y = (ci_uint_32)6968 * red  + (ci_uint_32)23434 * green +
            (ci_uint_32)2366 * blue;

         if (output_encoding == P_LINEAR)
            y = (y + 16384) >> 15;

         else
         {
            /* y is scaled by 32768, we need it scaled by 255: */
            y = (y + 128) >> 8;
            y *= 255;
            y = CI_sRGB_FROM_LINEAR((y + 64) >> 7);
            alpha = CI_DIV257(alpha);
            encoding = P_sRGB;
         }

         blue = red = green = y;
      }

      else if (output_encoding == P_sRGB)
      {
         red = CI_sRGB_FROM_LINEAR(red * 255);
         green = CI_sRGB_FROM_LINEAR(green * 255);
         blue = CI_sRGB_FROM_LINEAR(blue * 255);
         alpha = CI_DIV257(alpha);
         encoding = P_sRGB;
      }
   }

   if (encoding != output_encoding)
      ci_error(image->opaque->ci_ptr, "bad encoding (internal error)");

   /* Store the value. */
   {
#     ifdef CI_FORMAT_AFIRST_SUPPORTED
         int afirst = (image->format & CI_FORMAT_FLAG_AFIRST) != 0 &&
            (image->format & CI_FORMAT_FLAG_ALPHA) != 0;
#     else
#        define afirst 0
#     endif
#     ifdef CI_FORMAT_BGR_SUPPORTED
         int bgr = (image->format & CI_FORMAT_FLAG_BGR) != 0 ? 2 : 0;
#     else
#        define bgr 0
#     endif

      if (output_encoding == P_LINEAR)
      {
         ci_uint_16p entry = ci_voidcast(ci_uint_16p, display->colormap);

         entry += ip * CI_IMAGE_SAMPLE_CHANNELS(image->format);

         /* The linear 16-bit values must be pre-multiplied by the alpha channel
          * value, if less than 65535 (this is, effectively, composite on black
          * if the alpha channel is removed.)
          */
         switch (CI_IMAGE_SAMPLE_CHANNELS(image->format))
         {
            case 4:
               entry[afirst ? 0 : 3] = (ci_uint_16)alpha;
               /* FALLTHROUGH */

            case 3:
               if (alpha < 65535)
               {
                  if (alpha > 0)
                  {
                     blue = (blue * alpha + 32767U)/65535U;
                     green = (green * alpha + 32767U)/65535U;
                     red = (red * alpha + 32767U)/65535U;
                  }

                  else
                     red = green = blue = 0;
               }
               entry[afirst + (2 ^ bgr)] = (ci_uint_16)blue;
               entry[afirst + 1] = (ci_uint_16)green;
               entry[afirst + bgr] = (ci_uint_16)red;
               break;

            case 2:
               entry[1 ^ afirst] = (ci_uint_16)alpha;
               /* FALLTHROUGH */

            case 1:
               if (alpha < 65535)
               {
                  if (alpha > 0)
                     green = (green * alpha + 32767U)/65535U;

                  else
                     green = 0;
               }
               entry[afirst] = (ci_uint_16)green;
               break;

            default:
               break;
         }
      }

      else /* output encoding is P_sRGB */
      {
         ci_bytep entry = ci_voidcast(ci_bytep, display->colormap);

         entry += ip * CI_IMAGE_SAMPLE_CHANNELS(image->format);

         switch (CI_IMAGE_SAMPLE_CHANNELS(image->format))
         {
            case 4:
               entry[afirst ? 0 : 3] = (ci_byte)alpha;
               /* FALLTHROUGH */
            case 3:
               entry[afirst + (2 ^ bgr)] = (ci_byte)blue;
               entry[afirst + 1] = (ci_byte)green;
               entry[afirst + bgr] = (ci_byte)red;
               break;

            case 2:
               entry[1 ^ afirst] = (ci_byte)alpha;
               /* FALLTHROUGH */
            case 1:
               entry[afirst] = (ci_byte)green;
               break;

            default:
               break;
         }
      }

#     ifdef afirst
#        undef afirst
#     endif
#     ifdef bgr
#        undef bgr
#     endif
   }
}

static int
make_gray_file_colormap(ci_image_read_control *display)
{
   unsigned int i;

   for (i=0; i<256; ++i)
      ci_create_colormap_entry(display, i, i, i, i, 255, P_FILE);

   return (int)i;
}

static int
make_gray_colormap(ci_image_read_control *display)
{
   unsigned int i;

   for (i=0; i<256; ++i)
      ci_create_colormap_entry(display, i, i, i, i, 255, P_sRGB);

   return (int)i;
}
#define CI_GRAY_COLORMAP_ENTRIES 256

static int
make_ga_colormap(ci_image_read_control *display)
{
   unsigned int i, a;

   /* Alpha is retained, the output will be a color-map with entries
    * selected by six levels of alpha.  One transparent entry, 6 gray
    * levels for all the intermediate alpha values, leaving 230 entries
    * for the opaque grays.  The color-map entries are the six values
    * [0..5]*51, the GA processing uses CI_DIV51(value) to find the
    * relevant entry.
    *
    * if (alpha > 229) // opaque
    * {
    *    // The 231 entries are selected to make the math below work:
    *    base = 0;
    *    entry = (231 * gray + 128) >> 8;
    * }
    * else if (alpha < 26) // transparent
    * {
    *    base = 231;
    *    entry = 0;
    * }
    * else // partially opaque
    * {
    *    base = 226 + 6 * CI_DIV51(alpha);
    *    entry = CI_DIV51(gray);
    * }
    */
   i = 0;
   while (i < 231)
   {
      unsigned int gray = (i * 256 + 115) / 231;
      ci_create_colormap_entry(display, i++, gray, gray, gray, 255, P_sRGB);
   }

   /* 255 is used here for the component values for consistency with the code
    * that undoes premultiplication in ciwrite.c.
    */
   ci_create_colormap_entry(display, i++, 255, 255, 255, 0, P_sRGB);

   for (a=1; a<5; ++a)
   {
      unsigned int g;

      for (g=0; g<6; ++g)
         ci_create_colormap_entry(display, i++, g*51, g*51, g*51, a*51,
             P_sRGB);
   }

   return (int)i;
}

#define CI_GA_COLORMAP_ENTRIES 256

static int
make_rgb_colormap(ci_image_read_control *display)
{
   unsigned int i, r;

   /* Build a 6x6x6 opaque RGB cube */
   for (i=r=0; r<6; ++r)
   {
      unsigned int g;

      for (g=0; g<6; ++g)
      {
         unsigned int b;

         for (b=0; b<6; ++b)
            ci_create_colormap_entry(display, i++, r*51, g*51, b*51, 255,
                P_sRGB);
      }
   }

   return (int)i;
}

#define CI_RGB_COLORMAP_ENTRIES 216

/* Return a palette index to the above palette given three 8-bit sRGB values. */
#define CI_RGB_INDEX(r,g,b) \
   ((ci_byte)(6 * (6 * CI_DIV51(r) + CI_DIV51(g)) + CI_DIV51(b)))

static int
ci_image_read_colormap(ci_voidp argument)
{
   ci_image_read_control *display =
      ci_voidcast(ci_image_read_control*, argument);
   ci_imagep image = display->image;

   ci_structrp ci_ptr = image->opaque->ci_ptr;
   ci_uint_32 output_format = image->format;
   int output_encoding = (output_format & CI_FORMAT_FLAG_LINEAR) != 0 ?
      P_LINEAR : P_sRGB;

   unsigned int cmap_entries;
   unsigned int output_processing;        /* Output processing option */
   unsigned int data_encoding = P_NOTSET; /* Encoding libci must produce */

   /* Background information; the background color and the index of this color
    * in the color-map if it exists (else 256).
    */
   unsigned int background_index = 256;
   ci_uint_32 back_r, back_g, back_b;

   /* Flags to accumulate things that need to be done to the input. */
   int expand_tRNS = 0;

   /* Exclude the NYI feature of compositing onto a color-mapped buffer; it is
    * very difficult to do, the results look awful, and it is difficult to see
    * what possible use it is because the application can't control the
    * color-map.
    */
   if (((ci_ptr->color_type & CI_COLOR_MASK_ALPHA) != 0 ||
         ci_ptr->num_trans > 0) /* alpha in input */ &&
      ((output_format & CI_FORMAT_FLAG_ALPHA) == 0) /* no alpha in output */)
   {
      if (output_encoding == P_LINEAR) /* compose on black */
         back_b = back_g = back_r = 0;

      else if (display->background == NULL /* no way to remove it */)
         ci_error(ci_ptr,
             "background color must be supplied to remove alpha/transparency");

      /* Get a copy of the background color (this avoids repeating the checks
       * below.)  The encoding is 8-bit sRGB or 16-bit linear, depending on the
       * output format.
       */
      else
      {
         back_g = display->background->green;
         if ((output_format & CI_FORMAT_FLAG_COLOR) != 0)
         {
            back_r = display->background->red;
            back_b = display->background->blue;
         }
         else
            back_b = back_r = back_g;
      }
   }

   else if (output_encoding == P_LINEAR)
      back_b = back_r = back_g = 65535;

   else
      back_b = back_r = back_g = 255;

   /* Default the input file gamma if required - this is necessary because
    * libci assumes that if no gamma information is present the data is in the
    * output format, but the simplified API deduces the gamma from the input
    * format.  The 'default' gamma value is also set by ci_set_alpha_mode, but
    * this is happening before any such call, so:
    *
    * TODO: should be an internal API and all this code should be copied into a
    * single common gamma+colorspace file.
    */
   if (ci_ptr->bit_depth == 16 &&
      (image->flags & CI_IMAGE_FLAG_16BIT_sRGB) == 0)
      ci_ptr->default_gamma = CI_GAMMA_LINEAR;

   else
      ci_ptr->default_gamma = CI_GAMMA_sRGB_INVERSE;

   /* Decide what to do based on the CI color type of the input data.  The
    * utility function ci_create_colormap_entry deals with most aspects of the
    * output transformations; this code works out how to produce bytes of
    * color-map entries from the original format.
    */
   switch (ci_ptr->color_type)
   {
      case CI_COLOR_TYPE_GRAY:
         if (ci_ptr->bit_depth <= 8)
         {
            /* There at most 256 colors in the output, regardless of
             * transparency.
             */
            unsigned int step, i, val, trans = 256/*ignore*/, back_alpha = 0;

            cmap_entries = 1U << ci_ptr->bit_depth;
            if (cmap_entries > image->colormap_entries)
               ci_error(ci_ptr, "gray[8] color-map: too few entries");

            step = 255 / (cmap_entries - 1);
            output_processing = CI_CMAP_NONE;

            /* If there is a tRNS chunk then this either selects a transparent
             * value or, if the output has no alpha, the background color.
             */
            if (ci_ptr->num_trans > 0)
            {
               trans = ci_ptr->trans_color.gray;

               if ((output_format & CI_FORMAT_FLAG_ALPHA) == 0)
                  back_alpha = output_encoding == P_LINEAR ? 65535 : 255;
            }

            /* ci_create_colormap_entry just takes an RGBA and writes the
             * corresponding color-map entry using the format from 'image',
             * including the required conversion to sRGB or linear as
             * appropriate.  The input values are always either sRGB (if the
             * gamma correction flag is 0) or 0..255 scaled file encoded values
             * (if the function must gamma correct them).
             */
            for (i=val=0; i<cmap_entries; ++i, val += step)
            {
               /* 'i' is a file value.  While this will result in duplicated
                * entries for 8-bit non-sRGB encoded files it is necessary to
                * have non-gamma corrected values to do tRNS handling.
                */
               if (i != trans)
                  ci_create_colormap_entry(display, i, val, val, val, 255,
                      P_FILE/*8-bit with file gamma*/);

               /* Else this entry is transparent.  The colors don't matter if
                * there is an alpha channel (back_alpha == 0), but it does no
                * harm to pass them in; the values are not set above so this
                * passes in white.
                *
                * NOTE: this preserves the full precision of the application
                * supplied background color when it is used.
                */
               else
                  ci_create_colormap_entry(display, i, back_r, back_g, back_b,
                      back_alpha, output_encoding);
            }

            /* We need libci to preserve the original encoding. */
            data_encoding = P_FILE;

            /* The rows from libci, while technically gray values, are now also
             * color-map indices; however, they may need to be expanded to 1
             * byte per pixel.  This is what ci_set_packing does (i.e., it
             * unpacks the bit values into bytes.)
             */
            if (ci_ptr->bit_depth < 8)
               ci_set_packing(ci_ptr);
         }

         else /* bit depth is 16 */
         {
            /* The 16-bit input values can be converted directly to 8-bit gamma
             * encoded values; however, if a tRNS chunk is present 257 color-map
             * entries are required.  This means that the extra entry requires
             * special processing; add an alpha channel, sacrifice gray level
             * 254 and convert transparent (alpha==0) entries to that.
             *
             * Use libci to chop the data to 8 bits.  Convert it to sRGB at the
             * same time to minimize quality loss.  If a tRNS chunk is present
             * this means libci must handle it too; otherwise it is impossible
             * to do the exact match on the 16-bit value.
             *
             * If the output has no alpha channel *and* the background color is
             * gray then it is possible to let libci handle the substitution by
             * ensuring that the corresponding gray level matches the background
             * color exactly.
             */
            data_encoding = P_sRGB;

            if (CI_GRAY_COLORMAP_ENTRIES > image->colormap_entries)
               ci_error(ci_ptr, "gray[16] color-map: too few entries");

            cmap_entries = (unsigned int)make_gray_colormap(display);

            if (ci_ptr->num_trans > 0)
            {
               unsigned int back_alpha;

               if ((output_format & CI_FORMAT_FLAG_ALPHA) != 0)
                  back_alpha = 0;

               else
               {
                  if (back_r == back_g && back_g == back_b)
                  {
                     /* Background is gray; no special processing will be
                      * required.
                      */
                     ci_color_16 c;
                     ci_uint_32 gray = back_g;

                     if (output_encoding == P_LINEAR)
                     {
                        gray = CI_sRGB_FROM_LINEAR(gray * 255);

                        /* And make sure the corresponding palette entry
                         * matches.
                         */
                        ci_create_colormap_entry(display, gray, back_g, back_g,
                            back_g, 65535, P_LINEAR);
                     }

                     /* The background passed to libci, however, must be the
                      * sRGB value.
                      */
                     c.index = 0; /*unused*/
                     c.gray = c.red = c.green = c.blue = (ci_uint_16)gray;

                     /* NOTE: does this work without expanding tRNS to alpha?
                      * It should be the color->gray case below apparently
                      * doesn't.
                      */
                     ci_set_background_fixed(ci_ptr, &c,
                         CI_BACKGROUND_GAMMA_SCREEN, 0/*need_expand*/,
                         0/*gamma: not used*/);

                     output_processing = CI_CMAP_NONE;
                     break;
                  }
#ifdef __COVERITY__
                 /* Coverity claims that output_encoding cannot be 2 (P_LINEAR)
                  * here.
                  */
                  back_alpha = 255;
#else
                  back_alpha = output_encoding == P_LINEAR ? 65535 : 255;
#endif
               }

               /* output_processing means that the libci-processed row will be
                * 8-bit GA and it has to be processing to single byte color-map
                * values.  Entry 254 is replaced by either a completely
                * transparent entry or by the background color at full
                * precision (and the background color is not a simple gray
                * level in this case.)
                */
               expand_tRNS = 1;
               output_processing = CI_CMAP_TRANS;
               background_index = 254;

               /* And set (overwrite) color-map entry 254 to the actual
                * background color at full precision.
                */
               ci_create_colormap_entry(display, 254, back_r, back_g, back_b,
                   back_alpha, output_encoding);
            }

            else
               output_processing = CI_CMAP_NONE;
         }
         break;

      case CI_COLOR_TYPE_GRAY_ALPHA:
         /* 8-bit or 16-bit CI with two channels - gray and alpha.  A minimum
          * of 65536 combinations.  If, however, the alpha channel is to be
          * removed there are only 256 possibilities if the background is gray.
          * (Otherwise there is a subset of the 65536 possibilities defined by
          * the triangle between black, white and the background color.)
          *
          * Reduce 16-bit files to 8-bit and sRGB encode the result.  No need to
          * worry about tRNS matching - tRNS is ignored if there is an alpha
          * channel.
          */
         data_encoding = P_sRGB;

         if ((output_format & CI_FORMAT_FLAG_ALPHA) != 0)
         {
            if (CI_GA_COLORMAP_ENTRIES > image->colormap_entries)
               ci_error(ci_ptr, "gray+alpha color-map: too few entries");

            cmap_entries = (unsigned int)make_ga_colormap(display);

            background_index = CI_CMAP_GA_BACKGROUND;
            output_processing = CI_CMAP_GA;
         }

         else /* alpha is removed */
         {
            /* Alpha must be removed as the CI data is processed when the
             * background is a color because the G and A channels are
             * independent and the vector addition (non-parallel vectors) is a
             * 2-D problem.
             *
             * This can be reduced to the same algorithm as above by making a
             * colormap containing gray levels (for the opaque grays), a
             * background entry (for a transparent pixel) and a set of four six
             * level color values, one set for each intermediate alpha value.
             * See the comments in make_ga_colormap for how this works in the
             * per-pixel processing.
             *
             * If the background is gray, however, we only need a 256 entry gray
             * level color map.  It is sufficient to make the entry generated
             * for the background color be exactly the color specified.
             */
            if ((output_format & CI_FORMAT_FLAG_COLOR) == 0 ||
               (back_r == back_g && back_g == back_b))
            {
               /* Background is gray; no special processing will be required. */
               ci_color_16 c;
               ci_uint_32 gray = back_g;

               if (CI_GRAY_COLORMAP_ENTRIES > image->colormap_entries)
                  ci_error(ci_ptr, "gray-alpha color-map: too few entries");

               cmap_entries = (unsigned int)make_gray_colormap(display);

               if (output_encoding == P_LINEAR)
               {
                  gray = CI_sRGB_FROM_LINEAR(gray * 255);

                  /* And make sure the corresponding palette entry matches. */
                  ci_create_colormap_entry(display, gray, back_g, back_g,
                      back_g, 65535, P_LINEAR);
               }

               /* The background passed to libci, however, must be the sRGB
                * value.
                */
               c.index = 0; /*unused*/
               c.gray = c.red = c.green = c.blue = (ci_uint_16)gray;

               ci_set_background_fixed(ci_ptr, &c,
                   CI_BACKGROUND_GAMMA_SCREEN, 0/*need_expand*/,
                   0/*gamma: not used*/);

               output_processing = CI_CMAP_NONE;
            }

            else
            {
               ci_uint_32 i, a;

               /* This is the same as ci_make_ga_colormap, above, except that
                * the entries are all opaque.
                */
               if (CI_GA_COLORMAP_ENTRIES > image->colormap_entries)
                  ci_error(ci_ptr, "ga-alpha color-map: too few entries");

               i = 0;
               while (i < 231)
               {
                  ci_uint_32 gray = (i * 256 + 115) / 231;
                  ci_create_colormap_entry(display, i++, gray, gray, gray,
                      255, P_sRGB);
               }

               /* NOTE: this preserves the full precision of the application
                * background color.
                */
               background_index = i;
               ci_create_colormap_entry(display, i++, back_r, back_g, back_b,
#ifdef __COVERITY__
                   /* Coverity claims that output_encoding
                    * cannot be 2 (P_LINEAR) here.
                    */ 255U,
#else
                    output_encoding == P_LINEAR ? 65535U : 255U,
#endif
                    output_encoding);

               /* For non-opaque input composite on the sRGB background - this
                * requires inverting the encoding for each component.  The input
                * is still converted to the sRGB encoding because this is a
                * reasonable approximate to the logarithmic curve of human
                * visual sensitivity, at least over the narrow range which CI
                * represents.  Consequently 'G' is always sRGB encoded, while
                * 'A' is linear.  We need the linear background colors.
                */
               if (output_encoding == P_sRGB) /* else already linear */
               {
                  /* This may produce a value not exactly matching the
                   * background, but that's ok because these numbers are only
                   * used when alpha != 0
                   */
                  back_r = ci_sRGB_table[back_r];
                  back_g = ci_sRGB_table[back_g];
                  back_b = ci_sRGB_table[back_b];
               }

               for (a=1; a<5; ++a)
               {
                  unsigned int g;

                  /* CI_sRGB_FROM_LINEAR expects a 16-bit linear value scaled
                   * by an 8-bit alpha value (0..255).
                   */
                  ci_uint_32 alpha = 51 * a;
                  ci_uint_32 back_rx = (255-alpha) * back_r;
                  ci_uint_32 back_gx = (255-alpha) * back_g;
                  ci_uint_32 back_bx = (255-alpha) * back_b;

                  for (g=0; g<6; ++g)
                  {
                     ci_uint_32 gray = ci_sRGB_table[g*51] * alpha;

                     ci_create_colormap_entry(display, i++,
                         CI_sRGB_FROM_LINEAR(gray + back_rx),
                         CI_sRGB_FROM_LINEAR(gray + back_gx),
                         CI_sRGB_FROM_LINEAR(gray + back_bx), 255, P_sRGB);
                  }
               }

               cmap_entries = i;
               output_processing = CI_CMAP_GA;
            }
         }
         break;

      case CI_COLOR_TYPE_RGB:
      case CI_COLOR_TYPE_RGB_ALPHA:
         /* Exclude the case where the output is gray; we can always handle this
          * with the cases above.
          */
         if ((output_format & CI_FORMAT_FLAG_COLOR) == 0)
         {
            /* The color-map will be grayscale, so we may as well convert the
             * input RGB values to a simple grayscale and use the grayscale
             * code above.
             *
             * NOTE: calling this apparently damages the recognition of the
             * transparent color in background color handling; call
             * ci_set_tRNS_to_alpha before ci_set_background_fixed.
             */
            ci_set_rgb_to_gray_fixed(ci_ptr, CI_ERROR_ACTION_NONE, -1,
                -1);
            data_encoding = P_sRGB;

            /* The output will now be one or two 8-bit gray or gray+alpha
             * channels.  The more complex case arises when the input has alpha.
             */
            if ((ci_ptr->color_type == CI_COLOR_TYPE_RGB_ALPHA ||
               ci_ptr->num_trans > 0) &&
               (output_format & CI_FORMAT_FLAG_ALPHA) != 0)
            {
               /* Both input and output have an alpha channel, so no background
                * processing is required; just map the GA bytes to the right
                * color-map entry.
                */
               expand_tRNS = 1;

               if (CI_GA_COLORMAP_ENTRIES > image->colormap_entries)
                  ci_error(ci_ptr, "rgb[ga] color-map: too few entries");

               cmap_entries = (unsigned int)make_ga_colormap(display);
               background_index = CI_CMAP_GA_BACKGROUND;
               output_processing = CI_CMAP_GA;
            }

            else
            {
               const ci_fixed_point gamma = ci_resolve_file_gamma(ci_ptr);

               /* Either the input or the output has no alpha channel, so there
                * will be no non-opaque pixels in the color-map; it will just be
                * grayscale.
                */
               if (CI_GRAY_COLORMAP_ENTRIES > image->colormap_entries)
                  ci_error(ci_ptr, "rgb[gray] color-map: too few entries");

               /* Ideally this code would use libci to do the gamma correction,
                * but if an input alpha channel is to be removed we will hit the
                * libci bug in gamma+compose+rgb-to-gray (the double gamma
                * correction bug).  Fix this by dropping the gamma correction in
                * this case and doing it in the palette; this will result in
                * duplicate palette entries, but that's better than the
                * alternative of double gamma correction.
                *
                * NOTE: CIv3: check the resolved result of all the potentially
                * different colour space chunks.
                */
               if ((ci_ptr->color_type == CI_COLOR_TYPE_RGB_ALPHA ||
                  ci_ptr->num_trans > 0) &&
                  ci_gamma_not_sRGB(gamma) != 0)
               {
                  cmap_entries = (unsigned int)make_gray_file_colormap(display);
                  data_encoding = P_FILE;
               }

               else
                  cmap_entries = (unsigned int)make_gray_colormap(display);

               /* But if the input has alpha or transparency it must be removed
                */
               if (ci_ptr->color_type == CI_COLOR_TYPE_RGB_ALPHA ||
                  ci_ptr->num_trans > 0)
               {
                  ci_color_16 c;
                  ci_uint_32 gray = back_g;

                  /* We need to ensure that the application background exists in
                   * the colormap and that completely transparent pixels map to
                   * it.  Achieve this simply by ensuring that the entry
                   * selected for the background really is the background color.
                   */
                  if (data_encoding == P_FILE) /* from the fixup above */
                  {
                     /* The app supplied a gray which is in output_encoding, we
                      * need to convert it to a value of the input (P_FILE)
                      * encoding then set this palette entry to the required
                      * output encoding.
                      */
                     if (output_encoding == P_sRGB)
                        gray = ci_sRGB_table[gray]; /* now P_LINEAR */

                     gray = CI_DIV257(ci_gamma_16bit_correct(gray, gamma));
                        /* now P_FILE */

                     /* And make sure the corresponding palette entry contains
                      * exactly the required sRGB value.
                      */
                     ci_create_colormap_entry(display, gray, back_g, back_g,
                         back_g, 0/*unused*/, output_encoding);
                  }

                  else if (output_encoding == P_LINEAR)
                  {
                     gray = CI_sRGB_FROM_LINEAR(gray * 255);

                     /* And make sure the corresponding palette entry matches.
                      */
                     ci_create_colormap_entry(display, gray, back_g, back_g,
                        back_g, 0/*unused*/, P_LINEAR);
                  }

                  /* The background passed to libci, however, must be the
                   * output (normally sRGB) value.
                   */
                  c.index = 0; /*unused*/
                  c.gray = c.red = c.green = c.blue = (ci_uint_16)gray;

                  /* NOTE: the following is apparently a bug in libci. Without
                   * it the transparent color recognition in
                   * ci_set_background_fixed seems to go wrong.
                   */
                  expand_tRNS = 1;
                  ci_set_background_fixed(ci_ptr, &c,
                      CI_BACKGROUND_GAMMA_SCREEN, 0/*need_expand*/,
                      0/*gamma: not used*/);
               }

               output_processing = CI_CMAP_NONE;
            }
         }

         else /* output is color */
         {
            /* We could use ci_quantize here so long as there is no transparent
             * color or alpha; ci_quantize ignores alpha.  Easier overall just
             * to do it once and using CI_DIV51 on the 6x6x6 reduced RGB cube.
             * Consequently we always want libci to produce sRGB data.
             */
            data_encoding = P_sRGB;

            /* Is there any transparency or alpha? */
            if (ci_ptr->color_type == CI_COLOR_TYPE_RGB_ALPHA ||
               ci_ptr->num_trans > 0)
            {
               /* Is there alpha in the output too?  If so all four channels are
                * processed into a special RGB cube with alpha support.
                */
               if ((output_format & CI_FORMAT_FLAG_ALPHA) != 0)
               {
                  ci_uint_32 r;

                  if (CI_RGB_COLORMAP_ENTRIES+1+27 > image->colormap_entries)
                     ci_error(ci_ptr, "rgb+alpha color-map: too few entries");

                  cmap_entries = (unsigned int)make_rgb_colormap(display);

                  /* Add a transparent entry. */
                  ci_create_colormap_entry(display, cmap_entries, 255, 255,
                      255, 0, P_sRGB);

                  /* This is stored as the background index for the processing
                   * algorithm.
                   */
                  background_index = cmap_entries++;

                  /* Add 27 r,g,b entries each with alpha 0.5. */
                  for (r=0; r<256; r = (r << 1) | 0x7f)
                  {
                     ci_uint_32 g;

                     for (g=0; g<256; g = (g << 1) | 0x7f)
                     {
                        ci_uint_32 b;

                        /* This generates components with the values 0, 127 and
                         * 255
                         */
                        for (b=0; b<256; b = (b << 1) | 0x7f)
                           ci_create_colormap_entry(display, cmap_entries++,
                               r, g, b, 128, P_sRGB);
                     }
                  }

                  expand_tRNS = 1;
                  output_processing = CI_CMAP_RGB_ALPHA;
               }

               else
               {
                  /* Alpha/transparency must be removed.  The background must
                   * exist in the color map (achieved by setting adding it after
                   * the 666 color-map).  If the standard processing code will
                   * pick up this entry automatically that's all that is
                   * required; libci can be called to do the background
                   * processing.
                   */
                  unsigned int sample_size =
                     CI_IMAGE_SAMPLE_SIZE(output_format);
                  ci_uint_32 r, g, b; /* sRGB background */

                  if (CI_RGB_COLORMAP_ENTRIES+1+27 > image->colormap_entries)
                     ci_error(ci_ptr, "rgb-alpha color-map: too few entries");

                  cmap_entries = (unsigned int)make_rgb_colormap(display);

                  ci_create_colormap_entry(display, cmap_entries, back_r,
                      back_g, back_b, 0/*unused*/, output_encoding);

                  if (output_encoding == P_LINEAR)
                  {
                     r = CI_sRGB_FROM_LINEAR(back_r * 255);
                     g = CI_sRGB_FROM_LINEAR(back_g * 255);
                     b = CI_sRGB_FROM_LINEAR(back_b * 255);
                  }

                  else
                  {
                     r = back_r;
                     g = back_g;
                     b = back_g;
                  }

                  /* Compare the newly-created color-map entry with the one the
                   * CI_CMAP_RGB algorithm will use.  If the two entries don't
                   * match, add the new one and set this as the background
                   * index.
                   */
                  if (memcmp((ci_const_bytep)display->colormap +
                      sample_size * cmap_entries,
                      (ci_const_bytep)display->colormap +
                          sample_size * CI_RGB_INDEX(r,g,b),
                     sample_size) != 0)
                  {
                     /* The background color must be added. */
                     background_index = cmap_entries++;

                     /* Add 27 r,g,b entries each with created by composing with
                      * the background at alpha 0.5.
                      */
                     for (r=0; r<256; r = (r << 1) | 0x7f)
                     {
                        for (g=0; g<256; g = (g << 1) | 0x7f)
                        {
                           /* This generates components with the values 0, 127
                            * and 255
                            */
                           for (b=0; b<256; b = (b << 1) | 0x7f)
                              ci_create_colormap_entry(display, cmap_entries++,
                                  ci_colormap_compose(display, r, P_sRGB, 128,
                                      back_r, output_encoding),
                                  ci_colormap_compose(display, g, P_sRGB, 128,
                                      back_g, output_encoding),
                                  ci_colormap_compose(display, b, P_sRGB, 128,
                                      back_b, output_encoding),
                                  0/*unused*/, output_encoding);
                        }
                     }

                     expand_tRNS = 1;
                     output_processing = CI_CMAP_RGB_ALPHA;
                  }

                  else /* background color is in the standard color-map */
                  {
                     ci_color_16 c;

                     c.index = 0; /*unused*/
                     c.red = (ci_uint_16)back_r;
                     c.gray = c.green = (ci_uint_16)back_g;
                     c.blue = (ci_uint_16)back_b;

                     ci_set_background_fixed(ci_ptr, &c,
                         CI_BACKGROUND_GAMMA_SCREEN, 0/*need_expand*/,
                         0/*gamma: not used*/);

                     output_processing = CI_CMAP_RGB;
                  }
               }
            }

            else /* no alpha or transparency in the input */
            {
               /* Alpha in the output is irrelevant, simply map the opaque input
                * pixels to the 6x6x6 color-map.
                */
               if (CI_RGB_COLORMAP_ENTRIES > image->colormap_entries)
                  ci_error(ci_ptr, "rgb color-map: too few entries");

               cmap_entries = (unsigned int)make_rgb_colormap(display);
               output_processing = CI_CMAP_RGB;
            }
         }
         break;

      case CI_COLOR_TYPE_PALETTE:
         /* It's already got a color-map.  It may be necessary to eliminate the
          * tRNS entries though.
          */
         {
            unsigned int num_trans = ci_ptr->num_trans;
            ci_const_bytep trans = num_trans > 0 ? ci_ptr->trans_alpha : NULL;
            ci_const_colorp colormap = ci_ptr->palette;
            int do_background = trans != NULL &&
               (output_format & CI_FORMAT_FLAG_ALPHA) == 0;
            unsigned int i;

            /* Just in case: */
            if (trans == NULL)
               num_trans = 0;

            output_processing = CI_CMAP_NONE;
            data_encoding = P_FILE; /* Don't change from color-map indices */
            cmap_entries = (unsigned int)ci_ptr->num_palette;
            if (cmap_entries > 256)
               cmap_entries = 256;

            if (cmap_entries > (unsigned int)image->colormap_entries)
               ci_error(ci_ptr, "palette color-map: too few entries");

            for (i=0; i < cmap_entries; ++i)
            {
               if (do_background != 0 && i < num_trans && trans[i] < 255)
               {
                  if (trans[i] == 0)
                     ci_create_colormap_entry(display, i, back_r, back_g,
                         back_b, 0, output_encoding);

                  else
                  {
                     /* Must compose the CI file color in the color-map entry
                      * on the sRGB color in 'back'.
                      */
                     ci_create_colormap_entry(display, i,
                         ci_colormap_compose(display, colormap[i].red,
                             P_FILE, trans[i], back_r, output_encoding),
                         ci_colormap_compose(display, colormap[i].green,
                             P_FILE, trans[i], back_g, output_encoding),
                         ci_colormap_compose(display, colormap[i].blue,
                             P_FILE, trans[i], back_b, output_encoding),
                         output_encoding == P_LINEAR ? trans[i] * 257U :
                             trans[i],
                         output_encoding);
                  }
               }

               else
                  ci_create_colormap_entry(display, i, colormap[i].red,
                      colormap[i].green, colormap[i].blue,
                      i < num_trans ? trans[i] : 255U, P_FILE/*8-bit*/);
            }

            /* The CI data may have indices packed in fewer than 8 bits, it
             * must be expanded if so.
             */
            if (ci_ptr->bit_depth < 8)
               ci_set_packing(ci_ptr);
         }
         break;

      default:
         ci_error(ci_ptr, "invalid CI color type");
         /*NOT REACHED*/
   }

   /* Now deal with the output processing */
   if (expand_tRNS != 0 && ci_ptr->num_trans > 0 &&
       (ci_ptr->color_type & CI_COLOR_MASK_ALPHA) == 0)
      ci_set_tRNS_to_alpha(ci_ptr);

   switch (data_encoding)
   {
      case P_sRGB:
         /* Change to 8-bit sRGB */
         ci_set_alpha_mode_fixed(ci_ptr, CI_ALPHA_CI, CI_GAMMA_sRGB);
         /* FALLTHROUGH */

      case P_FILE:
         if (ci_ptr->bit_depth > 8)
            ci_set_scale_16(ci_ptr);
         break;

#ifdef __GNUC__
      default:
         ci_error(ci_ptr, "bad data option (internal error)");
#endif
   }

   if (cmap_entries > 256 || cmap_entries > image->colormap_entries)
      ci_error(ci_ptr, "color map overflow (BAD internal error)");

   image->colormap_entries = cmap_entries;

   /* Double check using the recorded background index */
   switch (output_processing)
   {
      case CI_CMAP_NONE:
         if (background_index != CI_CMAP_NONE_BACKGROUND)
            goto bad_background;
         break;

      case CI_CMAP_GA:
         if (background_index != CI_CMAP_GA_BACKGROUND)
            goto bad_background;
         break;

      case CI_CMAP_TRANS:
         if (background_index >= cmap_entries ||
            background_index != CI_CMAP_TRANS_BACKGROUND)
            goto bad_background;
         break;

      case CI_CMAP_RGB:
         if (background_index != CI_CMAP_RGB_BACKGROUND)
            goto bad_background;
         break;

      case CI_CMAP_RGB_ALPHA:
         if (background_index != CI_CMAP_RGB_ALPHA_BACKGROUND)
            goto bad_background;
         break;

      default:
         ci_error(ci_ptr, "bad processing option (internal error)");

      bad_background:
         ci_error(ci_ptr, "bad background index (internal error)");
   }

   display->colormap_processing = (int)output_processing;

   return 1/*ok*/;
}

/* The final part of the color-map read called from ci_image_finish_read. */
static int
ci_image_read_and_map(ci_voidp argument)
{
   ci_image_read_control *display = ci_voidcast(ci_image_read_control*,
       argument);
   ci_imagep image = display->image;
   ci_structrp ci_ptr = image->opaque->ci_ptr;
   int passes;

   /* Called when the libci data must be transformed into the color-mapped
    * form.  There is a local row buffer in display->local and this routine must
    * do the interlace handling.
    */
   switch (ci_ptr->interlaced)
   {
      case CI_INTERLACE_NONE:
         passes = 1;
         break;

      case CI_INTERLACE_ADAM7:
         passes = CI_INTERLACE_ADAM7_PASSES;
         break;

      default:
         ci_error(ci_ptr, "unknown interlace type");
   }

   {
      ci_uint_32  height = image->height;
      ci_uint_32  width = image->width;
      int          proc = display->colormap_processing;
      ci_bytep    first_row = ci_voidcast(ci_bytep, display->first_row);
      ptrdiff_t    step_row = display->row_bytes;
      int pass;

      for (pass = 0; pass < passes; ++pass)
      {
         unsigned int     startx, stepx, stepy;
         ci_uint_32      y;

         if (ci_ptr->interlaced == CI_INTERLACE_ADAM7)
         {
            /* The row may be empty for a short image: */
            if (CI_PASS_COLS(width, pass) == 0)
               continue;

            startx = CI_PASS_START_COL(pass);
            stepx = CI_PASS_COL_OFFSET(pass);
            y = CI_PASS_START_ROW(pass);
            stepy = CI_PASS_ROW_OFFSET(pass);
         }

         else
         {
            y = 0;
            startx = 0;
            stepx = stepy = 1;
         }

         for (; y<height; y += stepy)
         {
            ci_bytep inrow = ci_voidcast(ci_bytep, display->local_row);
            ci_bytep outrow = first_row + y * step_row;
            ci_const_bytep end_row = outrow + width;

            /* Read read the libci data into the temporary buffer. */
            ci_read_row(ci_ptr, inrow, NULL);

            /* Now process the row according to the processing option, note
             * that the caller verifies that the format of the libci output
             * data is as required.
             */
            outrow += startx;
            switch (proc)
            {
               case CI_CMAP_GA:
                  for (; outrow < end_row; outrow += stepx)
                  {
                     /* The data is always in the CI order */
                     unsigned int gray = *inrow++;
                     unsigned int alpha = *inrow++;
                     unsigned int entry;

                     /* NOTE: this code is copied as a comment in
                      * make_ga_colormap above.  Please update the
                      * comment if you change this code!
                      */
                     if (alpha > 229) /* opaque */
                     {
                        entry = (231 * gray + 128) >> 8;
                     }
                     else if (alpha < 26) /* transparent */
                     {
                        entry = 231;
                     }
                     else /* partially opaque */
                     {
                        entry = 226 + 6 * CI_DIV51(alpha) + CI_DIV51(gray);
                     }

                     *outrow = (ci_byte)entry;
                  }
                  break;

               case CI_CMAP_TRANS:
                  for (; outrow < end_row; outrow += stepx)
                  {
                     ci_byte gray = *inrow++;
                     ci_byte alpha = *inrow++;

                     if (alpha == 0)
                        *outrow = CI_CMAP_TRANS_BACKGROUND;

                     else if (gray != CI_CMAP_TRANS_BACKGROUND)
                        *outrow = gray;

                     else
                        *outrow = (ci_byte)(CI_CMAP_TRANS_BACKGROUND+1);
                  }
                  break;

               case CI_CMAP_RGB:
                  for (; outrow < end_row; outrow += stepx)
                  {
                     *outrow = CI_RGB_INDEX(inrow[0], inrow[1], inrow[2]);
                     inrow += 3;
                  }
                  break;

               case CI_CMAP_RGB_ALPHA:
                  for (; outrow < end_row; outrow += stepx)
                  {
                     unsigned int alpha = inrow[3];

                     /* Because the alpha entries only hold alpha==0.5 values
                      * split the processing at alpha==0.25 (64) and 0.75
                      * (196).
                      */

                     if (alpha >= 196)
                        *outrow = CI_RGB_INDEX(inrow[0], inrow[1],
                            inrow[2]);

                     else if (alpha < 64)
                        *outrow = CI_CMAP_RGB_ALPHA_BACKGROUND;

                     else
                     {
                        /* Likewise there are three entries for each of r, g
                         * and b.  We could select the entry by popcount on
                         * the top two bits on those architectures that
                         * support it, this is what the code below does,
                         * crudely.
                         */
                        unsigned int back_i = CI_CMAP_RGB_ALPHA_BACKGROUND+1;

                        /* Here are how the values map:
                         *
                         * 0x00 .. 0x3f -> 0
                         * 0x40 .. 0xbf -> 1
                         * 0xc0 .. 0xff -> 2
                         *
                         * So, as above with the explicit alpha checks, the
                         * breakpoints are at 64 and 196.
                         */
                        if (inrow[0] & 0x80) back_i += 9; /* red */
                        if (inrow[0] & 0x40) back_i += 9;
                        if (inrow[0] & 0x80) back_i += 3; /* green */
                        if (inrow[0] & 0x40) back_i += 3;
                        if (inrow[0] & 0x80) back_i += 1; /* blue */
                        if (inrow[0] & 0x40) back_i += 1;

                        *outrow = (ci_byte)back_i;
                     }

                     inrow += 4;
                  }
                  break;

               default:
                  break;
            }
         }
      }
   }

   return 1;
}

static int
ci_image_read_colormapped(ci_voidp argument)
{
   ci_image_read_control *display = ci_voidcast(ci_image_read_control*,
       argument);
   ci_imagep image = display->image;
   ci_controlp control = image->opaque;
   ci_structrp ci_ptr = control->ci_ptr;
   ci_inforp info_ptr = control->info_ptr;

   int passes = 0; /* As a flag */

   CI_SKIP_CHUNKS(ci_ptr);

   /* Update the 'info' structure and make sure the result is as required; first
    * make sure to turn on the interlace handling if it will be required
    * (because it can't be turned on *after* the call to ci_read_update_info!)
    */
   if (display->colormap_processing == CI_CMAP_NONE)
      passes = ci_set_interlace_handling(ci_ptr);

   ci_read_update_info(ci_ptr, info_ptr);

   /* The expected output can be deduced from the colormap_processing option. */
   switch (display->colormap_processing)
   {
      case CI_CMAP_NONE:
         /* Output must be one channel and one byte per pixel, the output
          * encoding can be anything.
          */
         if ((info_ptr->color_type == CI_COLOR_TYPE_PALETTE ||
            info_ptr->color_type == CI_COLOR_TYPE_GRAY) &&
            info_ptr->bit_depth == 8)
            break;

         goto bad_output;

      case CI_CMAP_TRANS:
      case CI_CMAP_GA:
         /* Output must be two channels and the 'G' one must be sRGB, the latter
          * can be checked with an exact number because it should have been set
          * to this number above!
          */
         if (info_ptr->color_type == CI_COLOR_TYPE_GRAY_ALPHA &&
            info_ptr->bit_depth == 8 &&
            ci_ptr->screen_gamma == CI_GAMMA_sRGB &&
            image->colormap_entries == 256)
            break;

         goto bad_output;

      case CI_CMAP_RGB:
         /* Output must be 8-bit sRGB encoded RGB */
         if (info_ptr->color_type == CI_COLOR_TYPE_RGB &&
            info_ptr->bit_depth == 8 &&
            ci_ptr->screen_gamma == CI_GAMMA_sRGB &&
            image->colormap_entries == 216)
            break;

         goto bad_output;

      case CI_CMAP_RGB_ALPHA:
         /* Output must be 8-bit sRGB encoded RGBA */
         if (info_ptr->color_type == CI_COLOR_TYPE_RGB_ALPHA &&
            info_ptr->bit_depth == 8 &&
            ci_ptr->screen_gamma == CI_GAMMA_sRGB &&
            image->colormap_entries == 244 /* 216 + 1 + 27 */)
            break;

         goto bad_output;

      default:
      bad_output:
         ci_error(ci_ptr, "bad color-map processing (internal error)");
   }

   /* Now read the rows.  Do this here if it is possible to read directly into
    * the output buffer, otherwise allocate a local row buffer of the maximum
    * size libci requires and call the relevant processing routine safely.
    */
   {
      ci_voidp first_row = display->buffer;
      ptrdiff_t row_bytes = display->row_stride;

      /* The following expression is designed to work correctly whether it gives
       * a signed or an unsigned result.
       */
      if (row_bytes < 0)
      {
         char *ptr = ci_voidcast(char*, first_row);
         ptr += (image->height-1) * (-row_bytes);
         first_row = ci_voidcast(ci_voidp, ptr);
      }

      display->first_row = first_row;
      display->row_bytes = row_bytes;
   }

   if (passes == 0)
   {
      int result;
      ci_voidp row = ci_malloc(ci_ptr, ci_get_rowbytes(ci_ptr, info_ptr));

      display->local_row = row;
      result = ci_safe_execute(image, ci_image_read_and_map, display);
      display->local_row = NULL;
      ci_free(ci_ptr, row);

      return result;
   }

   else
   {
      ci_alloc_size_t row_bytes = (ci_alloc_size_t)display->row_bytes;

      while (--passes >= 0)
      {
         ci_uint_32      y = image->height;
         ci_bytep        row = ci_voidcast(ci_bytep, display->first_row);

         for (; y > 0; --y)
         {
            ci_read_row(ci_ptr, row, NULL);
            row += row_bytes;
         }
      }

      return 1;
   }
}

/* Just the row reading part of ci_image_read. */
static int
ci_image_read_composite(ci_voidp argument)
{
   ci_image_read_control *display = ci_voidcast(ci_image_read_control*,
       argument);
   ci_imagep image = display->image;
   ci_structrp ci_ptr = image->opaque->ci_ptr;
   int passes;

   switch (ci_ptr->interlaced)
   {
      case CI_INTERLACE_NONE:
         passes = 1;
         break;

      case CI_INTERLACE_ADAM7:
         passes = CI_INTERLACE_ADAM7_PASSES;
         break;

      default:
         ci_error(ci_ptr, "unknown interlace type");
   }

   {
      ci_uint_32  height = image->height;
      ci_uint_32  width = image->width;
      ptrdiff_t    step_row = display->row_bytes;
      unsigned int channels =
          (image->format & CI_FORMAT_FLAG_COLOR) != 0 ? 3 : 1;
      int pass;

      for (pass = 0; pass < passes; ++pass)
      {
         unsigned int     startx, stepx, stepy;
         ci_uint_32      y;

         if (ci_ptr->interlaced == CI_INTERLACE_ADAM7)
         {
            /* The row may be empty for a short image: */
            if (CI_PASS_COLS(width, pass) == 0)
               continue;

            startx = CI_PASS_START_COL(pass) * channels;
            stepx = CI_PASS_COL_OFFSET(pass) * channels;
            y = CI_PASS_START_ROW(pass);
            stepy = CI_PASS_ROW_OFFSET(pass);
         }

         else
         {
            y = 0;
            startx = 0;
            stepx = channels;
            stepy = 1;
         }

         for (; y<height; y += stepy)
         {
            ci_bytep inrow = ci_voidcast(ci_bytep, display->local_row);
            ci_bytep outrow;
            ci_const_bytep end_row;

            /* Read the row, which is packed: */
            ci_read_row(ci_ptr, inrow, NULL);

            outrow = ci_voidcast(ci_bytep, display->first_row);
            outrow += y * step_row;
            end_row = outrow + width * channels;

            /* Now do the composition on each pixel in this row. */
            outrow += startx;
            for (; outrow < end_row; outrow += stepx)
            {
               ci_byte alpha = inrow[channels];

               if (alpha > 0) /* else no change to the output */
               {
                  unsigned int c;

                  for (c=0; c<channels; ++c)
                  {
                     ci_uint_32 component = inrow[c];

                     if (alpha < 255) /* else just use component */
                     {
                        /* This is CI_OPTIMIZED_ALPHA, the component value
                         * is a linear 8-bit value.  Combine this with the
                         * current outrow[c] value which is sRGB encoded.
                         * Arithmetic here is 16-bits to preserve the output
                         * values correctly.
                         */
                        component *= 257*255; /* =65535 */
                        component += (255-alpha)*ci_sRGB_table[outrow[c]];

                        /* So 'component' is scaled by 255*65535 and is
                         * therefore appropriate for the sRGB to linear
                         * conversion table.
                         */
                        component = CI_sRGB_FROM_LINEAR(component);
                     }

                     outrow[c] = (ci_byte)component;
                  }
               }

               inrow += channels+1; /* components and alpha channel */
            }
         }
      }
   }

   return 1;
}

/* The do_local_background case; called when all the following transforms are to
 * be done:
 *
 * CI_RGB_TO_GRAY
 * CI_COMPOSITE
 * CI_GAMMA
 *
 * This is a work-around for the fact that both the CI_RGB_TO_GRAY and
 * CI_COMPOSITE code performs gamma correction, so we get double gamma
 * correction.  The fix-up is to prevent the CI_COMPOSITE operation from
 * happening inside libci, so this routine sees an 8 or 16-bit gray+alpha
 * row and handles the removal or pre-multiplication of the alpha channel.
 */
static int
ci_image_read_background(ci_voidp argument)
{
   ci_image_read_control *display = ci_voidcast(ci_image_read_control*,
       argument);
   ci_imagep image = display->image;
   ci_structrp ci_ptr = image->opaque->ci_ptr;
   ci_inforp info_ptr = image->opaque->info_ptr;
   ci_uint_32 height = image->height;
   ci_uint_32 width = image->width;
   int pass, passes;

   /* Double check the convoluted logic below.  We expect to get here with
    * libci doing rgb to gray and gamma correction but background processing
    * left to the ci_image_read_background function.  The rows libci produce
    * might be 8 or 16-bit but should always have two channels; gray plus alpha.
    */
   if ((ci_ptr->transformations & CI_RGB_TO_GRAY) == 0)
      ci_error(ci_ptr, "lost rgb to gray");

   if ((ci_ptr->transformations & CI_COMPOSE) != 0)
      ci_error(ci_ptr, "unexpected compose");

   if (ci_get_channels(ci_ptr, info_ptr) != 2)
      ci_error(ci_ptr, "lost/gained channels");

   /* Expect the 8-bit case to always remove the alpha channel */
   if ((image->format & CI_FORMAT_FLAG_LINEAR) == 0 &&
      (image->format & CI_FORMAT_FLAG_ALPHA) != 0)
      ci_error(ci_ptr, "unexpected 8-bit transformation");

   switch (ci_ptr->interlaced)
   {
      case CI_INTERLACE_NONE:
         passes = 1;
         break;

      case CI_INTERLACE_ADAM7:
         passes = CI_INTERLACE_ADAM7_PASSES;
         break;

      default:
         ci_error(ci_ptr, "unknown interlace type");
   }

   /* Use direct access to info_ptr here because otherwise the simplified API
    * would require CI_EASY_ACCESS_SUPPORTED (just for this.)  Note this is
    * checking the value after libci expansions, not the original value in the
    * CI.
    */
   switch (info_ptr->bit_depth)
   {
      case 8:
         /* 8-bit sRGB gray values with an alpha channel; the alpha channel is
          * to be removed by composing on a background: either the row if
          * display->background is NULL or display->background->green if not.
          * Unlike the code above ALPHA_OPTIMIZED has *not* been done.
          */
         {
            ci_bytep first_row = ci_voidcast(ci_bytep, display->first_row);
            ptrdiff_t step_row = display->row_bytes;

            for (pass = 0; pass < passes; ++pass)
            {
               unsigned int     startx, stepx, stepy;
               ci_uint_32      y;

               if (ci_ptr->interlaced == CI_INTERLACE_ADAM7)
               {
                  /* The row may be empty for a short image: */
                  if (CI_PASS_COLS(width, pass) == 0)
                     continue;

                  startx = CI_PASS_START_COL(pass);
                  stepx = CI_PASS_COL_OFFSET(pass);
                  y = CI_PASS_START_ROW(pass);
                  stepy = CI_PASS_ROW_OFFSET(pass);
               }

               else
               {
                  y = 0;
                  startx = 0;
                  stepx = stepy = 1;
               }

               if (display->background == NULL)
               {
                  for (; y<height; y += stepy)
                  {
                     ci_bytep inrow = ci_voidcast(ci_bytep,
                         display->local_row);
                     ci_bytep outrow = first_row + y * step_row;
                     ci_const_bytep end_row = outrow + width;

                     /* Read the row, which is packed: */
                     ci_read_row(ci_ptr, inrow, NULL);

                     /* Now do the composition on each pixel in this row. */
                     outrow += startx;
                     for (; outrow < end_row; outrow += stepx)
                     {
                        ci_byte alpha = inrow[1];

                        if (alpha > 0) /* else no change to the output */
                        {
                           ci_uint_32 component = inrow[0];

                           if (alpha < 255) /* else just use component */
                           {
                              /* Since CI_OPTIMIZED_ALPHA was not set it is
                               * necessary to invert the sRGB transfer
                               * function and multiply the alpha out.
                               */
                              component = ci_sRGB_table[component] * alpha;
                              component += ci_sRGB_table[outrow[0]] *
                                 (255-alpha);
                              component = CI_sRGB_FROM_LINEAR(component);
                           }

                           outrow[0] = (ci_byte)component;
                        }

                        inrow += 2; /* gray and alpha channel */
                     }
                  }
               }

               else /* constant background value */
               {
                  ci_byte background8 = display->background->green;
                  ci_uint_16 background = ci_sRGB_table[background8];

                  for (; y<height; y += stepy)
                  {
                     ci_bytep inrow = ci_voidcast(ci_bytep,
                         display->local_row);
                     ci_bytep outrow = first_row + y * step_row;
                     ci_const_bytep end_row = outrow + width;

                     /* Read the row, which is packed: */
                     ci_read_row(ci_ptr, inrow, NULL);

                     /* Now do the composition on each pixel in this row. */
                     outrow += startx;
                     for (; outrow < end_row; outrow += stepx)
                     {
                        ci_byte alpha = inrow[1];

                        if (alpha > 0) /* else use background */
                        {
                           ci_uint_32 component = inrow[0];

                           if (alpha < 255) /* else just use component */
                           {
                              component = ci_sRGB_table[component] * alpha;
                              component += background * (255-alpha);
                              component = CI_sRGB_FROM_LINEAR(component);
                           }

                           outrow[0] = (ci_byte)component;
                        }

                        else
                           outrow[0] = background8;

                        inrow += 2; /* gray and alpha channel */
                     }
                  }
               }
            }
         }
         break;

      case 16:
         /* 16-bit linear with pre-multiplied alpha; the pre-multiplication must
          * still be done and, maybe, the alpha channel removed.  This code also
          * handles the alpha-first option.
          */
         {
            ci_uint_16p first_row = ci_voidcast(ci_uint_16p,
                display->first_row);
            /* The division by two is safe because the caller passed in a
             * stride which was multiplied by 2 (below) to get row_bytes.
             */
            ptrdiff_t    step_row = display->row_bytes / 2;
            unsigned int preserve_alpha = (image->format &
                CI_FORMAT_FLAG_ALPHA) != 0;
            unsigned int outchannels = 1U+preserve_alpha;
            int swap_alpha = 0;

#           ifdef CI_SIMPLIFIED_READ_AFIRST_SUPPORTED
               if (preserve_alpha != 0 &&
                   (image->format & CI_FORMAT_FLAG_AFIRST) != 0)
                  swap_alpha = 1;
#           endif

            for (pass = 0; pass < passes; ++pass)
            {
               unsigned int     startx, stepx, stepy;
               ci_uint_32      y;

               /* The 'x' start and step are adjusted to output components here.
                */
               if (ci_ptr->interlaced == CI_INTERLACE_ADAM7)
               {
                  /* The row may be empty for a short image: */
                  if (CI_PASS_COLS(width, pass) == 0)
                     continue;

                  startx = CI_PASS_START_COL(pass) * outchannels;
                  stepx = CI_PASS_COL_OFFSET(pass) * outchannels;
                  y = CI_PASS_START_ROW(pass);
                  stepy = CI_PASS_ROW_OFFSET(pass);
               }

               else
               {
                  y = 0;
                  startx = 0;
                  stepx = outchannels;
                  stepy = 1;
               }

               for (; y<height; y += stepy)
               {
                  ci_const_uint_16p inrow;
                  ci_uint_16p outrow = first_row + y*step_row;
                  ci_uint_16p end_row = outrow + width * outchannels;

                  /* Read the row, which is packed: */
                  ci_read_row(ci_ptr, ci_voidcast(ci_bytep,
                      display->local_row), NULL);
                  inrow = ci_voidcast(ci_const_uint_16p, display->local_row);

                  /* Now do the pre-multiplication on each pixel in this row.
                   */
                  outrow += startx;
                  for (; outrow < end_row; outrow += stepx)
                  {
                     ci_uint_32 component = inrow[0];
                     ci_uint_16 alpha = inrow[1];

                     if (alpha > 0) /* else 0 */
                     {
                        if (alpha < 65535) /* else just use component */
                        {
                           component *= alpha;
                           component += 32767;
                           component /= 65535;
                        }
                     }

                     else
                        component = 0;

                     outrow[swap_alpha] = (ci_uint_16)component;
                     if (preserve_alpha != 0)
                        outrow[1 ^ swap_alpha] = alpha;

                     inrow += 2; /* components and alpha channel */
                  }
               }
            }
         }
         break;

#ifdef __GNUC__
      default:
         ci_error(ci_ptr, "unexpected bit depth");
#endif
   }

   return 1;
}

/* The guts of ci_image_finish_read as a ci_safe_execute callback. */
static int
ci_image_read_direct(ci_voidp argument)
{
   ci_image_read_control *display = ci_voidcast(ci_image_read_control*,
       argument);
   ci_imagep image = display->image;
   ci_structrp ci_ptr = image->opaque->ci_ptr;
   ci_inforp info_ptr = image->opaque->info_ptr;

   ci_uint_32 format = image->format;
   int linear = (format & CI_FORMAT_FLAG_LINEAR) != 0;
   int do_local_compose = 0;
   int do_local_background = 0; /* to avoid double gamma correction bug */
   int passes = 0;

   /* Add transforms to ensure the correct output format is produced then check
    * that the required implementation support is there.  Always expand; always
    * need 8 bits minimum, no palette and expanded tRNS.
    */
   ci_set_expand(ci_ptr);

   /* Now check the format to see if it was modified. */
   {
      ci_uint_32 base_format = ci_image_format(ci_ptr) &
         ~CI_FORMAT_FLAG_COLORMAP /* removed by ci_set_expand */;
      ci_uint_32 change = format ^ base_format;
      ci_fixed_point output_gamma;
      int mode; /* alpha mode */

      /* Do this first so that we have a record if rgb to gray is happening. */
      if ((change & CI_FORMAT_FLAG_COLOR) != 0)
      {
         /* gray<->color transformation required. */
         if ((format & CI_FORMAT_FLAG_COLOR) != 0)
            ci_set_gray_to_rgb(ci_ptr);

         else
         {
            /* libci can't do both rgb to gray and
             * background/pre-multiplication if there is also significant gamma
             * correction, because both operations require linear colors and
             * the code only supports one transform doing the gamma correction.
             * Handle this by doing the pre-multiplication or background
             * operation in this code, if necessary.
             *
             * TODO: fix this by rewriting cirtran.c (!)
             *
             * For the moment (given that fixing this in cirtran.c is an
             * enormous change) 'do_local_background' is used to indicate that
             * the problem exists.
             */
            if ((base_format & CI_FORMAT_FLAG_ALPHA) != 0)
               do_local_background = 1/*maybe*/;

            ci_set_rgb_to_gray_fixed(ci_ptr, CI_ERROR_ACTION_NONE,
                CI_RGB_TO_GRAY_DEFAULT, CI_RGB_TO_GRAY_DEFAULT);
         }

         change &= ~CI_FORMAT_FLAG_COLOR;
      }

      /* Set the gamma appropriately, linear for 16-bit input, sRGB otherwise.
       */
      {
         /* This is safe but should no longer be necessary as
          * ci_ptr->default_gamma should have been set after the
          * info-before-IDAT was read in ci_image_read_header.
          *
          * TODO: 1.8: remove this and see what happens.
          */
         ci_fixed_point input_gamma_default;

         if ((base_format & CI_FORMAT_FLAG_LINEAR) != 0 &&
             (image->flags & CI_IMAGE_FLAG_16BIT_sRGB) == 0)
            input_gamma_default = CI_GAMMA_LINEAR;
         else
            input_gamma_default = CI_DEFAULT_sRGB;

         /* Call ci_set_alpha_mode to set the default for the input gamma; the
          * output gamma is set by a second call below.
          */
         ci_set_alpha_mode_fixed(ci_ptr, CI_ALPHA_CI, input_gamma_default);
      }

      if (linear != 0)
      {
         /* If there *is* an alpha channel in the input it must be multiplied
          * out; use CI_ALPHA_STANDARD, otherwise just use CI_ALPHA_CI.
          */
         if ((base_format & CI_FORMAT_FLAG_ALPHA) != 0)
            mode = CI_ALPHA_STANDARD; /* associated alpha */

         else
            mode = CI_ALPHA_CI;

         output_gamma = CI_GAMMA_LINEAR;
      }

      else
      {
         mode = CI_ALPHA_CI;
         output_gamma = CI_DEFAULT_sRGB;
      }

      if ((change & CI_FORMAT_FLAG_ASSOCIATED_ALPHA) != 0)
      {
         mode = CI_ALPHA_OPTIMIZED;
         change &= ~CI_FORMAT_FLAG_ASSOCIATED_ALPHA;
      }

      /* If 'do_local_background' is set check for the presence of gamma
       * correction; this is part of the work-round for the libci bug
       * described above.
       *
       * TODO: fix libci and remove this.
       */
      if (do_local_background != 0)
      {
         ci_fixed_point gtest;

         /* This is 'ci_gamma_threshold' from cirtran.c; the test used for
          * gamma correction, the screen gamma hasn't been set on ci_struct
          * yet; it's set below.  ci_struct::gamma, however, is set to the
          * final value.
          */
         if (ci_muldiv(&gtest, output_gamma,
                  ci_resolve_file_gamma(ci_ptr), CI_FP_1) != 0 &&
             ci_gamma_significant(gtest) == 0)
            do_local_background = 0;

         else if (mode == CI_ALPHA_STANDARD)
         {
            do_local_background = 2/*required*/;
            mode = CI_ALPHA_CI; /* prevent libci doing it */
         }

         /* else leave as 1 for the checks below */
      }

      /* If the bit-depth changes then handle that here. */
      if ((change & CI_FORMAT_FLAG_LINEAR) != 0)
      {
         if (linear != 0 /*16-bit output*/)
            ci_set_expand_16(ci_ptr);

         else /* 8-bit output */
            ci_set_scale_16(ci_ptr);

         change &= ~CI_FORMAT_FLAG_LINEAR;
      }

      /* Now the background/alpha channel changes. */
      if ((change & CI_FORMAT_FLAG_ALPHA) != 0)
      {
         /* Removing an alpha channel requires composition for the 8-bit
          * formats; for the 16-bit it is already done, above, by the
          * pre-multiplication and the channel just needs to be stripped.
          */
         if ((base_format & CI_FORMAT_FLAG_ALPHA) != 0)
         {
            /* If RGB->gray is happening the alpha channel must be left and the
             * operation completed locally.
             *
             * TODO: fix libci and remove this.
             */
            if (do_local_background != 0)
               do_local_background = 2/*required*/;

            /* 16-bit output: just remove the channel */
            else if (linear != 0) /* compose on black (well, pre-multiply) */
               ci_set_strip_alpha(ci_ptr);

            /* 8-bit output: do an appropriate compose */
            else if (display->background != NULL)
            {
               ci_color_16 c;

               c.index = 0; /*unused*/
               c.red = display->background->red;
               c.green = display->background->green;
               c.blue = display->background->blue;
               c.gray = display->background->green;

               /* This is always an 8-bit sRGB value, using the 'green' channel
                * for gray is much better than calculating the luminance here;
                * we can get off-by-one errors in that calculation relative to
                * the app expectations and that will show up in transparent
                * pixels.
                */
               ci_set_background_fixed(ci_ptr, &c,
                   CI_BACKGROUND_GAMMA_SCREEN, 0/*need_expand*/,
                   0/*gamma: not used*/);
            }

            else /* compose on row: implemented below. */
            {
               do_local_compose = 1;
               /* This leaves the alpha channel in the output, so it has to be
                * removed by the code below.  Set the encoding to the 'OPTIMIZE'
                * one so the code only has to hack on the pixels that require
                * composition.
                */
               mode = CI_ALPHA_OPTIMIZED;
            }
         }

         else /* output needs an alpha channel */
         {
            /* This is tricky because it happens before the swap operation has
             * been accomplished; however, the swap does *not* swap the added
             * alpha channel (weird API), so it must be added in the correct
             * place.
             */
            ci_uint_32 filler; /* opaque filler */
            int where;

            if (linear != 0)
               filler = 65535;

            else
               filler = 255;

#ifdef CI_FORMAT_AFIRST_SUPPORTED
            if ((format & CI_FORMAT_FLAG_AFIRST) != 0)
            {
               where = CI_FILLER_BEFORE;
               change &= ~CI_FORMAT_FLAG_AFIRST;
            }

            else
#endif
            where = CI_FILLER_AFTER;

            ci_set_add_alpha(ci_ptr, filler, where);
         }

         /* This stops the (irrelevant) call to swap_alpha below. */
         change &= ~CI_FORMAT_FLAG_ALPHA;
      }

      /* Now set the alpha mode correctly; this is always done, even if there is
       * no alpha channel in either the input or the output because it correctly
       * sets the output gamma.
       */
      ci_set_alpha_mode_fixed(ci_ptr, mode, output_gamma);

#     ifdef CI_FORMAT_BGR_SUPPORTED
         if ((change & CI_FORMAT_FLAG_BGR) != 0)
         {
            /* Check only the output format; CI is never BGR; don't do this if
             * the output is gray, but fix up the 'format' value in that case.
             */
            if ((format & CI_FORMAT_FLAG_COLOR) != 0)
               ci_set_bgr(ci_ptr);

            else
               format &= ~CI_FORMAT_FLAG_BGR;

            change &= ~CI_FORMAT_FLAG_BGR;
         }
#     endif

#     ifdef CI_FORMAT_AFIRST_SUPPORTED
         if ((change & CI_FORMAT_FLAG_AFIRST) != 0)
         {
            /* Only relevant if there is an alpha channel - it's particularly
             * important to handle this correctly because do_local_compose may
             * be set above and then libci will keep the alpha channel for this
             * code to remove.
             */
            if ((format & CI_FORMAT_FLAG_ALPHA) != 0)
            {
               /* Disable this if doing a local background,
                * TODO: remove this when local background is no longer required.
                */
               if (do_local_background != 2)
                  ci_set_swap_alpha(ci_ptr);
            }

            else
               format &= ~CI_FORMAT_FLAG_AFIRST;

            change &= ~CI_FORMAT_FLAG_AFIRST;
         }
#     endif

      /* If the *output* is 16-bit then we need to check for a byte-swap on this
       * architecture.
       */
      if (linear != 0)
      {
         ci_uint_16 le = 0x0001;

         if ((*(ci_const_bytep) & le) != 0)
            ci_set_swap(ci_ptr);
      }

      /* If change is not now 0 some transformation is missing - error out. */
      if (change != 0)
         ci_error(ci_ptr, "ci_read_image: unsupported transformation");
   }

   CI_SKIP_CHUNKS(ci_ptr);

   /* Update the 'info' structure and make sure the result is as required; first
    * make sure to turn on the interlace handling if it will be required
    * (because it can't be turned on *after* the call to ci_read_update_info!)
    *
    * TODO: remove the do_local_background fixup below.
    */
   if (do_local_compose == 0 && do_local_background != 2)
      passes = ci_set_interlace_handling(ci_ptr);

   ci_read_update_info(ci_ptr, info_ptr);

   {
      ci_uint_32 info_format = 0;

      if ((info_ptr->color_type & CI_COLOR_MASK_COLOR) != 0)
         info_format |= CI_FORMAT_FLAG_COLOR;

      if ((info_ptr->color_type & CI_COLOR_MASK_ALPHA) != 0)
      {
         /* do_local_compose removes this channel below. */
         if (do_local_compose == 0)
         {
            /* do_local_background does the same if required. */
            if (do_local_background != 2 ||
               (format & CI_FORMAT_FLAG_ALPHA) != 0)
               info_format |= CI_FORMAT_FLAG_ALPHA;
         }
      }

      else if (do_local_compose != 0) /* internal error */
         ci_error(ci_ptr, "ci_image_read: alpha channel lost");

      if ((format & CI_FORMAT_FLAG_ASSOCIATED_ALPHA) != 0) {
         info_format |= CI_FORMAT_FLAG_ASSOCIATED_ALPHA;
      }

      if (info_ptr->bit_depth == 16)
         info_format |= CI_FORMAT_FLAG_LINEAR;

#ifdef CI_FORMAT_BGR_SUPPORTED
      if ((ci_ptr->transformations & CI_BGR) != 0)
         info_format |= CI_FORMAT_FLAG_BGR;
#endif

#ifdef CI_FORMAT_AFIRST_SUPPORTED
         if (do_local_background == 2)
         {
            if ((format & CI_FORMAT_FLAG_AFIRST) != 0)
               info_format |= CI_FORMAT_FLAG_AFIRST;
         }

         if ((ci_ptr->transformations & CI_SWAP_ALPHA) != 0 ||
            ((ci_ptr->transformations & CI_ADD_ALPHA) != 0 &&
            (ci_ptr->flags & CI_FLAG_FILLER_AFTER) == 0))
         {
            if (do_local_background == 2)
               ci_error(ci_ptr, "unexpected alpha swap transformation");

            info_format |= CI_FORMAT_FLAG_AFIRST;
         }
#     endif

      /* This is actually an internal error. */
      if (info_format != format)
         ci_error(ci_ptr, "ci_read_image: invalid transformations");
   }

   /* Now read the rows.  If do_local_compose is set then it is necessary to use
    * a local row buffer.  The output will be GA, RGBA or BGRA and must be
    * converted to G, RGB or BGR as appropriate.  The 'local_row' member of the
    * display acts as a flag.
    */
   {
      ci_voidp first_row = display->buffer;
      ptrdiff_t row_bytes = display->row_stride;

      if (linear != 0)
         row_bytes *= 2;

      /* The following expression is designed to work correctly whether it gives
       * a signed or an unsigned result.
       */
      if (row_bytes < 0)
      {
         char *ptr = ci_voidcast(char*, first_row);
         ptr += (image->height-1) * (-row_bytes);
         first_row = ci_voidcast(ci_voidp, ptr);
      }

      display->first_row = first_row;
      display->row_bytes = row_bytes;
   }

   if (do_local_compose != 0)
   {
      int result;
      ci_voidp row = ci_malloc(ci_ptr, ci_get_rowbytes(ci_ptr, info_ptr));

      display->local_row = row;
      result = ci_safe_execute(image, ci_image_read_composite, display);
      display->local_row = NULL;
      ci_free(ci_ptr, row);

      return result;
   }

   else if (do_local_background == 2)
   {
      int result;
      ci_voidp row = ci_malloc(ci_ptr, ci_get_rowbytes(ci_ptr, info_ptr));

      display->local_row = row;
      result = ci_safe_execute(image, ci_image_read_background, display);
      display->local_row = NULL;
      ci_free(ci_ptr, row);

      return result;
   }

   else
   {
      ci_alloc_size_t row_bytes = (ci_alloc_size_t)display->row_bytes;

      while (--passes >= 0)
      {
         ci_uint_32      y = image->height;
         ci_bytep        row = ci_voidcast(ci_bytep, display->first_row);

         for (; y > 0; --y)
         {
            ci_read_row(ci_ptr, row, NULL);
            row += row_bytes;
         }
      }

      return 1;
   }
}

int CIAPI
ci_image_finish_read(ci_imagep image, ci_const_colorp background,
    void *buffer, ci_int_32 row_stride, void *colormap)
{
   if (image != NULL && image->version == CI_IMAGE_VERSION)
   {
      /* Check for row_stride overflow.  This check is not performed on the
       * original CI format because it may not occur in the output CI format
       * and libci deals with the issues of reading the original.
       */
      unsigned int channels = CI_IMAGE_PIXEL_CHANNELS(image->format);

      /* The following checks just the 'row_stride' calculation to ensure it
       * fits in a signed 32-bit value.  Because channels/components can be
       * either 1 or 2 bytes in size the length of a row can still overflow 32
       * bits; this is just to verify that the 'row_stride' argument can be
       * represented.
       */
      if (image->width <= 0x7fffffffU/channels) /* no overflow */
      {
         ci_uint_32 check;
         ci_uint_32 ci_row_stride = image->width * channels;

         if (row_stride == 0)
            row_stride = (ci_int_32)/*SAFE*/ci_row_stride;

         if (row_stride < 0)
            check = (ci_uint_32)(-row_stride);

         else
            check = (ci_uint_32)row_stride;

         /* This verifies 'check', the absolute value of the actual stride
          * passed in and detects overflow in the application calculation (i.e.
          * if the app did actually pass in a non-zero 'row_stride'.
          */
         if (image->opaque != NULL && buffer != NULL && check >= ci_row_stride)
         {
            /* Now check for overflow of the image buffer calculation; this
             * limits the whole image size to 32 bits for API compatibility with
             * the current, 32-bit, CI_IMAGE_BUFFER_SIZE macro.
             *
             * The CI_IMAGE_BUFFER_SIZE macro is:
             *
             *    (CI_IMAGE_PIXEL_COMPONENT_SIZE(fmt)*height*(row_stride))
             *
             * And the component size is always 1 or 2, so make sure that the
             * number of *bytes* that the application is saying are available
             * does actually fit into a 32-bit number.
             *
             * NOTE: this will be changed in 1.7 because CI_IMAGE_BUFFER_SIZE
             * will be changed to use ci_alloc_size_t; bigger images can be
             * accommodated on 64-bit systems.
             */
            if (image->height <=
                0xffffffffU/CI_IMAGE_PIXEL_COMPONENT_SIZE(image->format)/check)
            {
               if ((image->format & CI_FORMAT_FLAG_COLORMAP) == 0 ||
                  (image->colormap_entries > 0 && colormap != NULL))
               {
                  int result;
                  ci_image_read_control display;

                  memset(&display, 0, (sizeof display));
                  display.image = image;
                  display.buffer = buffer;
                  display.row_stride = row_stride;
                  display.colormap = colormap;
                  display.background = background;
                  display.local_row = NULL;

                  /* Choose the correct 'end' routine; for the color-map case
                   * all the setup has already been done.
                   */
                  if ((image->format & CI_FORMAT_FLAG_COLORMAP) != 0)
                     result =
                         ci_safe_execute(image,
                             ci_image_read_colormap, &display) &&
                             ci_safe_execute(image,
                             ci_image_read_colormapped, &display);

                  else
                     result =
                        ci_safe_execute(image,
                            ci_image_read_direct, &display);

                  ci_image_free(image);
                  return result;
               }

               else
                  return ci_image_error(image,
                      "ci_image_finish_read[color-map]: no color-map");
            }

            else
               return ci_image_error(image,
                   "ci_image_finish_read: image too large");
         }

         else
            return ci_image_error(image,
                "ci_image_finish_read: invalid argument");
      }

      else
         return ci_image_error(image,
             "ci_image_finish_read: row_stride too large");
   }

   else if (image != NULL)
      return ci_image_error(image,
          "ci_image_finish_read: damaged CI_IMAGE_VERSION");

   return 0;
}

#endif /* SIMPLIFIED_READ */
#endif /* READ */

/* ciwrite.c - general routines to write a CI file
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

#include "cipriv.h"
#ifdef CI_SIMPLIFIED_WRITE_STDIO_SUPPORTED
#  include <errno.h>
#endif /* SIMPLIFIED_WRITE_STDIO */

#ifdef CI_WRITE_SUPPORTED

#ifdef CI_WRITE_UNKNOWN_CHUNKS_SUPPORTED
/* Write out all the unknown chunks for the current given location */
static void
write_unknown_chunks(ci_structrp ci_ptr, ci_const_inforp info_ptr,
    unsigned int where)
{
   if (info_ptr->unknown_chunks_num != 0)
   {
      ci_const_unknown_chunkp up;

      ci_debug(5, "writing extra chunks");

      for (up = info_ptr->unknown_chunks;
           up < info_ptr->unknown_chunks + info_ptr->unknown_chunks_num;
           ++up)
         if ((up->location & where) != 0)
      {
         /* If per-chunk unknown chunk handling is enabled use it, otherwise
          * just write the chunks the application has set.
          */
#ifdef CI_SET_UNKNOWN_CHUNKS_SUPPORTED
         int keep = ci_handle_as_unknown(ci_ptr, up->name);

         /* NOTE: this code is radically different from the read side in the
          * matter of handling an ancillary unknown chunk.  In the read side
          * the default behavior is to discard it, in the code below the default
          * behavior is to write it.  Critical chunks are, however, only
          * written if explicitly listed or if the default is set to write all
          * unknown chunks.
          *
          * The default handling is also slightly weird - it is not possible to
          * stop the writing of all unsafe-to-copy chunks!
          *
          * TODO: REVIEW: this would seem to be a bug.
          */
         if (keep != CI_HANDLE_CHUNK_NEVER &&
             ((up->name[3] & 0x20) /* safe-to-copy overrides everything */ ||
              keep == CI_HANDLE_CHUNK_ALWAYS ||
              (keep == CI_HANDLE_CHUNK_AS_DEFAULT &&
               ci_ptr->unknown_default == CI_HANDLE_CHUNK_ALWAYS)))
#endif
         {
            /* TODO: review, what is wrong with a zero length unknown chunk? */
            if (up->size == 0)
               ci_warning(ci_ptr, "Writing zero-length unknown chunk");

            ci_write_chunk(ci_ptr, up->name, up->data, up->size);
         }
      }
   }
}
#endif /* WRITE_UNKNOWN_CHUNKS */

/* Writes all the CI information.  This is the suggested way to use the
 * library.  If you have a new chunk to add, make a function to write it,
 * and put it in the correct location here.  If you want the chunk written
 * after the image data, put it in ci_write_end().  I strongly encourage
 * you to supply a CI_INFO_<chunk> flag, and check info_ptr->valid before
 * writing the chunk, as that will keep the code from breaking if you want
 * to just write a plain CI file.  If you have long comments, I suggest
 * writing them in ci_write_end(), and compressing them.
 */
void CIAPI
ci_write_info_before_PLTE(ci_structrp ci_ptr, ci_const_inforp info_ptr)
{
   ci_debug(1, "in ci_write_info_before_PLTE");

   if (ci_ptr == NULL || info_ptr == NULL)
      return;

   if ((ci_ptr->mode & CI_WROTE_INFO_BEFORE_PLTE) == 0)
   {
      /* Write CI signature */
      ci_write_sig(ci_ptr);

#ifdef CI_MNG_FEATURES_SUPPORTED
      if ((ci_ptr->mode & CI_HAVE_CI_SIGNATURE) != 0 && \
          ci_ptr->mng_features_permitted != 0)
      {
         ci_warning(ci_ptr,
             "MNG features are not allowed in a CI datastream");
         ci_ptr->mng_features_permitted = 0;
      }
#endif

      /* Write IHDR information. */
      ci_write_IHDR(ci_ptr, info_ptr->width, info_ptr->height,
          info_ptr->bit_depth, info_ptr->color_type, info_ptr->compression_type,
          info_ptr->filter_type,
#ifdef CI_WRITE_INTERLACING_SUPPORTED
          info_ptr->interlace_type
#else
          0
#endif
         );

      /* The rest of these check to see if the valid field has the appropriate
       * flag set, and if it does, writes the chunk.
       *
       * 1.6.0: COLORSPACE support controls the writing of these chunks too, and
       * the chunks will be written if the WRITE routine is there and
       * information * is available in the COLORSPACE. (See
       * ci_colorspace_sync_info in ci.c for where the valid flags get set.)
       *
       * Under certain circumstances the colorspace can be invalidated without
       * syncing the info_struct 'valid' flags; this happens if libci detects
       * an error and calls ci_error while the color space is being set, yet
       * the application continues writing the CI.  So check the 'invalid'
       * flag here too.
       */
#ifdef CI_WRITE_UNKNOWN_CHUNKS_SUPPORTED
         /* Write unknown chunks first; CI v3 establishes a precedence order
          * for colourspace chunks.  It is certain therefore that new
          * colourspace chunks will have a precedence and very likely it will be
          * higher than all known so far.  Writing the unknown chunks here is
          * most likely to present the chunks in the most convenient order.
          *
          * FUTURE: maybe write chunks in the order the app calls ci_set_chnk
          * to give the app control.
          */
         write_unknown_chunks(ci_ptr, info_ptr, CI_HAVE_IHDR);
#endif

#ifdef CI_WRITE_sBIT_SUPPORTED
         /* CI v3: a streaming app will need to see this before cICP because
          * the information is helpful in handling HLG encoding (which is
          * natively 10 bits but gets expanded to 16 in CI.)
          *
          * The app shouldn't care about the order ideally, but it might have
          * no choice.  In CI v3, apps are allowed to reject CIs where the
          * ACI chunks are out of order so it behooves libci to be nice here.
          */
         if ((info_ptr->valid & CI_INFO_sBIT) != 0)
            ci_write_sBIT(ci_ptr, &(info_ptr->sig_bit), info_ptr->color_type);
#endif

   /* CI v3: the July 2004 version of the TR introduced the concept of colour
    * space priority.  As above it therefore behooves libci to write the colour
    * space chunks in the priority order so that a streaming app need not buffer
    * them.
    *
    * CI v3: Chunks mDCV and cLLI provide ancillary information for the
    * interpretation of the colourspace chunkgs but do not require support for
    * those chunks so are outside the "COLORSPACE" check but before the write of
    * the colourspace chunks themselves.
    */
#ifdef CI_WRITE_cLLI_SUPPORTED
   if ((info_ptr->valid & CI_INFO_cLLI) != 0)
   {
      ci_write_cLLI_fixed(ci_ptr, info_ptr->maxCLL, info_ptr->maxFALL);
   }
#endif
#ifdef CI_WRITE_mDCV_SUPPORTED
   if ((info_ptr->valid & CI_INFO_mDCV) != 0)
   {
      ci_write_mDCV_fixed(ci_ptr,
         info_ptr->mastering_red_x, info_ptr->mastering_red_y,
         info_ptr->mastering_green_x, info_ptr->mastering_green_y,
         info_ptr->mastering_blue_x, info_ptr->mastering_blue_y,
         info_ptr->mastering_white_x, info_ptr->mastering_white_y,
         info_ptr->mastering_maxDL, info_ptr->mastering_minDL);
   }
#endif

#  ifdef CI_WRITE_cICP_SUPPORTED /* Priority 4 */
   if ((info_ptr->valid & CI_INFO_cICP) != 0)
      {
         ci_write_cICP(ci_ptr,
                        info_ptr->cicp_colour_primaries,
                        info_ptr->cicp_transfer_function,
                        info_ptr->cicp_matrix_coefficients,
                        info_ptr->cicp_video_full_range_flag);
      }
#  endif

#  ifdef CI_WRITE_iCCP_SUPPORTED /* Priority 3 */
         if ((info_ptr->valid & CI_INFO_iCCP) != 0)
         {
            ci_write_iCCP(ci_ptr, info_ptr->iccp_name,
                info_ptr->iccp_profile, info_ptr->iccp_proflen);
         }
#  endif

#  ifdef CI_WRITE_sRGB_SUPPORTED /* Priority 2 */
         if ((info_ptr->valid & CI_INFO_sRGB) != 0)
            ci_write_sRGB(ci_ptr, info_ptr->rendering_intent);
#  endif /* WRITE_sRGB */

#  ifdef CI_WRITE_gAMA_SUPPORTED /* Priority 1 */
      if ((info_ptr->valid & CI_INFO_gAMA) != 0)
         ci_write_gAMA_fixed(ci_ptr, info_ptr->gamma);
#  endif

#  ifdef CI_WRITE_cHRM_SUPPORTED /* Also priority 1 */
         if ((info_ptr->valid & CI_INFO_cHRM) != 0)
            ci_write_cHRM_fixed(ci_ptr, &info_ptr->cHRM);
#  endif

      ci_ptr->mode |= CI_WROTE_INFO_BEFORE_PLTE;
   }
}

void CIAPI
ci_write_info(ci_structrp ci_ptr, ci_const_inforp info_ptr)
{
#if defined(CI_WRITE_TEXT_SUPPORTED) || defined(CI_WRITE_sPLT_SUPPORTED)
   int i;
#endif

   ci_debug(1, "in ci_write_info");

   if (ci_ptr == NULL || info_ptr == NULL)
      return;

   ci_write_info_before_PLTE(ci_ptr, info_ptr);

   if ((info_ptr->valid & CI_INFO_PLTE) != 0)
      ci_write_PLTE(ci_ptr, info_ptr->palette,
          (ci_uint_32)info_ptr->num_palette);

   else if (info_ptr->color_type == CI_COLOR_TYPE_PALETTE)
      ci_error(ci_ptr, "Valid palette required for paletted images");

#ifdef CI_WRITE_tRNS_SUPPORTED
   if ((info_ptr->valid & CI_INFO_tRNS) !=0)
   {
#ifdef CI_WRITE_INVERT_ALPHA_SUPPORTED
      /* Invert the alpha channel (in tRNS) */
      if ((ci_ptr->transformations & CI_INVERT_ALPHA) != 0 &&
          info_ptr->color_type == CI_COLOR_TYPE_PALETTE)
      {
         int j, jend;

         jend = info_ptr->num_trans;
         if (jend > CI_MAX_PALETTE_LENGTH)
            jend = CI_MAX_PALETTE_LENGTH;

         for (j = 0; j<jend; ++j)
            info_ptr->trans_alpha[j] =
               (ci_byte)(255 - info_ptr->trans_alpha[j]);
      }
#endif
      ci_write_tRNS(ci_ptr, info_ptr->trans_alpha, &(info_ptr->trans_color),
          info_ptr->num_trans, info_ptr->color_type);
   }
#endif
#ifdef CI_WRITE_bKGD_SUPPORTED
   if ((info_ptr->valid & CI_INFO_bKGD) != 0)
      ci_write_bKGD(ci_ptr, &(info_ptr->background), info_ptr->color_type);
#endif

#ifdef CI_WRITE_eXIf_SUPPORTED
   if ((info_ptr->valid & CI_INFO_eXIf) != 0)
   {
      ci_write_eXIf(ci_ptr, info_ptr->exif, info_ptr->num_exif);
      ci_ptr->mode |= CI_WROTE_eXIf;
   }
#endif

#ifdef CI_WRITE_hIST_SUPPORTED
   if ((info_ptr->valid & CI_INFO_hIST) != 0)
      ci_write_hIST(ci_ptr, info_ptr->hist, info_ptr->num_palette);
#endif

#ifdef CI_WRITE_oFFs_SUPPORTED
   if ((info_ptr->valid & CI_INFO_oFFs) != 0)
      ci_write_oFFs(ci_ptr, info_ptr->x_offset, info_ptr->y_offset,
          info_ptr->offset_unit_type);
#endif

#ifdef CI_WRITE_pCAL_SUPPORTED
   if ((info_ptr->valid & CI_INFO_pCAL) != 0)
      ci_write_pCAL(ci_ptr, info_ptr->pcal_purpose, info_ptr->pcal_X0,
          info_ptr->pcal_X1, info_ptr->pcal_type, info_ptr->pcal_nparams,
          info_ptr->pcal_units, info_ptr->pcal_params);
#endif

#ifdef CI_WRITE_sCAL_SUPPORTED
   if ((info_ptr->valid & CI_INFO_sCAL) != 0)
      ci_write_sCAL_s(ci_ptr, (int)info_ptr->scal_unit,
          info_ptr->scal_s_width, info_ptr->scal_s_height);
#endif /* sCAL */

#ifdef CI_WRITE_pHYs_SUPPORTED
   if ((info_ptr->valid & CI_INFO_pHYs) != 0)
      ci_write_pHYs(ci_ptr, info_ptr->x_pixels_per_unit,
          info_ptr->y_pixels_per_unit, info_ptr->phys_unit_type);
#endif /* pHYs */

#ifdef CI_WRITE_tIME_SUPPORTED
   if ((info_ptr->valid & CI_INFO_tIME) != 0)
   {
      ci_write_tIME(ci_ptr, &(info_ptr->mod_time));
      ci_ptr->mode |= CI_WROTE_tIME;
   }
#endif /* tIME */

#ifdef CI_WRITE_sPLT_SUPPORTED
   if ((info_ptr->valid & CI_INFO_sPLT) != 0)
      for (i = 0; i < (int)info_ptr->splt_palettes_num; i++)
         ci_write_sPLT(ci_ptr, info_ptr->splt_palettes + i);
#endif /* sPLT */

#ifdef CI_WRITE_TEXT_SUPPORTED
   /* Check to see if we need to write text chunks */
   for (i = 0; i < info_ptr->num_text; i++)
   {
      ci_debug2(2, "Writing header text chunk %d, type %d", i,
          info_ptr->text[i].compression);
      /* An internationalized chunk? */
      if (info_ptr->text[i].compression > 0)
      {
#ifdef CI_WRITE_iTXt_SUPPORTED
         /* Write international chunk */
         ci_write_iTXt(ci_ptr,
             info_ptr->text[i].compression,
             info_ptr->text[i].key,
             info_ptr->text[i].lang,
             info_ptr->text[i].lang_key,
             info_ptr->text[i].text);
         /* Mark this chunk as written */
         if (info_ptr->text[i].compression == CI_TEXT_COMPRESSION_NONE)
            info_ptr->text[i].compression = CI_TEXT_COMPRESSION_NONE_WR;
         else
            info_ptr->text[i].compression = CI_TEXT_COMPRESSION_zTXt_WR;
#else
         ci_warning(ci_ptr, "Unable to write international text");
#endif
      }

      /* If we want a compressed text chunk */
      else if (info_ptr->text[i].compression == CI_TEXT_COMPRESSION_zTXt)
      {
#ifdef CI_WRITE_zTXt_SUPPORTED
         /* Write compressed chunk */
         ci_write_zTXt(ci_ptr, info_ptr->text[i].key,
             info_ptr->text[i].text, info_ptr->text[i].compression);
         /* Mark this chunk as written */
         info_ptr->text[i].compression = CI_TEXT_COMPRESSION_zTXt_WR;
#else
         ci_warning(ci_ptr, "Unable to write compressed text");
#endif
      }

      else if (info_ptr->text[i].compression == CI_TEXT_COMPRESSION_NONE)
      {
#ifdef CI_WRITE_tEXt_SUPPORTED
         /* Write uncompressed chunk */
         ci_write_tEXt(ci_ptr, info_ptr->text[i].key,
             info_ptr->text[i].text,
             0);
         /* Mark this chunk as written */
         info_ptr->text[i].compression = CI_TEXT_COMPRESSION_NONE_WR;
#else
         /* Can't get here */
         ci_warning(ci_ptr, "Unable to write uncompressed text");
#endif
      }
   }
#endif /* tEXt */

#ifdef CI_WRITE_UNKNOWN_CHUNKS_SUPPORTED
   write_unknown_chunks(ci_ptr, info_ptr, CI_HAVE_PLTE);
#endif
}

/* Writes the end of the CI file.  If you don't want to write comments or
 * time information, you can pass NULL for info.  If you already wrote these
 * in ci_write_info(), do not write them again here.  If you have long
 * comments, I suggest writing them here, and compressing them.
 */
void CIAPI
ci_write_end(ci_structrp ci_ptr, ci_inforp info_ptr)
{
   ci_debug(1, "in ci_write_end");

   if (ci_ptr == NULL)
      return;

   if ((ci_ptr->mode & CI_HAVE_IDAT) == 0)
      ci_error(ci_ptr, "No IDATs written into file");

#ifdef CI_WRITE_CHECK_FOR_INVALID_INDEX_SUPPORTED
   if (ci_ptr->color_type == CI_COLOR_TYPE_PALETTE &&
       ci_ptr->num_palette_max >= ci_ptr->num_palette)
      ci_benign_error(ci_ptr, "Wrote palette index exceeding num_palette");
#endif

   /* See if user wants us to write information chunks */
   if (info_ptr != NULL)
   {
#ifdef CI_WRITE_TEXT_SUPPORTED
      int i; /* local index variable */
#endif
#ifdef CI_WRITE_tIME_SUPPORTED
      /* Check to see if user has supplied a time chunk */
      if ((info_ptr->valid & CI_INFO_tIME) != 0 &&
          (ci_ptr->mode & CI_WROTE_tIME) == 0)
         ci_write_tIME(ci_ptr, &(info_ptr->mod_time));

#endif
#ifdef CI_WRITE_TEXT_SUPPORTED
      /* Loop through comment chunks */
      for (i = 0; i < info_ptr->num_text; i++)
      {
         ci_debug2(2, "Writing trailer text chunk %d, type %d", i,
             info_ptr->text[i].compression);
         /* An internationalized chunk? */
         if (info_ptr->text[i].compression > 0)
         {
#ifdef CI_WRITE_iTXt_SUPPORTED
            /* Write international chunk */
            ci_write_iTXt(ci_ptr,
                info_ptr->text[i].compression,
                info_ptr->text[i].key,
                info_ptr->text[i].lang,
                info_ptr->text[i].lang_key,
                info_ptr->text[i].text);
            /* Mark this chunk as written */
            if (info_ptr->text[i].compression == CI_TEXT_COMPRESSION_NONE)
               info_ptr->text[i].compression = CI_TEXT_COMPRESSION_NONE_WR;
            else
               info_ptr->text[i].compression = CI_TEXT_COMPRESSION_zTXt_WR;
#else
            ci_warning(ci_ptr, "Unable to write international text");
#endif
         }

         else if (info_ptr->text[i].compression >= CI_TEXT_COMPRESSION_zTXt)
         {
#ifdef CI_WRITE_zTXt_SUPPORTED
            /* Write compressed chunk */
            ci_write_zTXt(ci_ptr, info_ptr->text[i].key,
                info_ptr->text[i].text, info_ptr->text[i].compression);
            /* Mark this chunk as written */
            info_ptr->text[i].compression = CI_TEXT_COMPRESSION_zTXt_WR;
#else
            ci_warning(ci_ptr, "Unable to write compressed text");
#endif
         }

         else if (info_ptr->text[i].compression == CI_TEXT_COMPRESSION_NONE)
         {
#ifdef CI_WRITE_tEXt_SUPPORTED
            /* Write uncompressed chunk */
            ci_write_tEXt(ci_ptr, info_ptr->text[i].key,
                info_ptr->text[i].text, 0);
            /* Mark this chunk as written */
            info_ptr->text[i].compression = CI_TEXT_COMPRESSION_NONE_WR;
#else
            ci_warning(ci_ptr, "Unable to write uncompressed text");
#endif
         }
      }
#endif

#ifdef CI_WRITE_eXIf_SUPPORTED
      if ((info_ptr->valid & CI_INFO_eXIf) != 0 &&
          (ci_ptr->mode & CI_WROTE_eXIf) == 0)
         ci_write_eXIf(ci_ptr, info_ptr->exif, info_ptr->num_exif);
#endif

#ifdef CI_WRITE_UNKNOWN_CHUNKS_SUPPORTED
      write_unknown_chunks(ci_ptr, info_ptr, CI_AFTER_IDAT);
#endif
   }

   ci_ptr->mode |= CI_AFTER_IDAT;

   /* Write end of CI file */
   ci_write_IEND(ci_ptr);

   /* This flush, added in libci-1.0.8, removed from libci-1.0.9beta03,
    * and restored again in libci-1.2.30, may cause some applications that
    * do not set ci_ptr->output_flush_fn to crash.  If your application
    * experiences a problem, please try building libci with
    * CI_WRITE_FLUSH_AFTER_IEND_SUPPORTED defined, and report the event to
    * ci-mng-implement at lists.sf.net .
    */
#ifdef CI_WRITE_FLUSH_SUPPORTED
#  ifdef CI_WRITE_FLUSH_AFTER_IEND_SUPPORTED
   ci_flush(ci_ptr);
#  endif
#endif
}

#ifdef CI_CONVERT_tIME_SUPPORTED
void CIAPI
ci_convert_from_struct_tm(ci_timep ptime, const struct tm * ttime)
{
   ci_debug(1, "in ci_convert_from_struct_tm");

   ptime->year = (ci_uint_16)(1900 + ttime->tm_year);
   ptime->month = (ci_byte)(ttime->tm_mon + 1);
   ptime->day = (ci_byte)ttime->tm_mday;
   ptime->hour = (ci_byte)ttime->tm_hour;
   ptime->minute = (ci_byte)ttime->tm_min;
   ptime->second = (ci_byte)ttime->tm_sec;
}

void CIAPI
ci_convert_from_time_t(ci_timep ptime, time_t ttime)
{
   struct tm *tbuf;

   ci_debug(1, "in ci_convert_from_time_t");

   tbuf = gmtime(&ttime);
   if (tbuf == NULL)
   {
      /* TODO: add a safe function which takes a ci_ptr argument and raises
       * a ci_error if the ttime argument is invalid and the call to gmtime
       * fails as a consequence.
       */
      memset(ptime, 0, sizeof(*ptime));
      return;
   }

   ci_convert_from_struct_tm(ptime, tbuf);
}
#endif

/* Initialize ci_ptr structure, and allocate any memory needed */
CI_FUNCTION(ci_structp,CIAPI
ci_create_write_struct,(ci_const_charp user_ci_ver, ci_voidp error_ptr,
    ci_error_ptr error_fn, ci_error_ptr warn_fn),CI_ALLOCATED)
{
#ifndef CI_USER_MEM_SUPPORTED
   ci_structrp ci_ptr = ci_create_ci_struct(user_ci_ver, error_ptr,
       error_fn, warn_fn, NULL, NULL, NULL);
#else
   return ci_create_write_struct_2(user_ci_ver, error_ptr, error_fn,
       warn_fn, NULL, NULL, NULL);
}

/* Alternate initialize ci_ptr structure, and allocate any memory needed */
CI_FUNCTION(ci_structp,CIAPI
ci_create_write_struct_2,(ci_const_charp user_ci_ver, ci_voidp error_ptr,
    ci_error_ptr error_fn, ci_error_ptr warn_fn, ci_voidp mem_ptr,
    ci_malloc_ptr malloc_fn, ci_free_ptr free_fn),CI_ALLOCATED)
{
   ci_structrp ci_ptr = ci_create_ci_struct(user_ci_ver, error_ptr,
       error_fn, warn_fn, mem_ptr, malloc_fn, free_fn);
#endif /* USER_MEM */
   if (ci_ptr != NULL)
   {
      /* Set the zlib control values to defaults; they can be overridden by the
       * application after the struct has been created.
       */
      ci_ptr->zbuffer_size = CI_ZBUF_SIZE;

      /* The 'zlib_strategy' setting is irrelevant because ci_default_claim in
       * ciwutil.c defaults it according to whether or not filters will be
       * used, and ignores this setting.
       */
      ci_ptr->zlib_strategy = CI_Z_DEFAULT_STRATEGY;
      ci_ptr->zlib_level = CI_Z_DEFAULT_COMPRESSION;
      ci_ptr->zlib_mem_level = 8;
      ci_ptr->zlib_window_bits = 15;
      ci_ptr->zlib_method = 8;

#ifdef CI_WRITE_COMPRESSED_TEXT_SUPPORTED
      ci_ptr->zlib_text_strategy = CI_TEXT_Z_DEFAULT_STRATEGY;
      ci_ptr->zlib_text_level = CI_TEXT_Z_DEFAULT_COMPRESSION;
      ci_ptr->zlib_text_mem_level = 8;
      ci_ptr->zlib_text_window_bits = 15;
      ci_ptr->zlib_text_method = 8;
#endif /* WRITE_COMPRESSED_TEXT */

      /* This is a highly dubious configuration option; by default it is off,
       * but it may be appropriate for private builds that are testing
       * extensions not conformant to the current specification, or of
       * applications that must not fail to write at all costs!
       */
#ifdef CI_BENIGN_WRITE_ERRORS_SUPPORTED
      /* In stable builds only warn if an application error can be completely
       * handled.
       */
      ci_ptr->flags |= CI_FLAG_BENIGN_ERRORS_WARN;
#endif

      /* App warnings are warnings in release (or release candidate) builds but
       * are errors during development.
       */
#if CI_RELEASE_BUILD
      ci_ptr->flags |= CI_FLAG_APP_WARNINGS_WARN;
#endif

      /* TODO: delay this, it can be done in ci_init_io() (if the app doesn't
       * do it itself) avoiding setting the default function if it is not
       * required.
       */
      ci_set_write_fn(ci_ptr, NULL, NULL, NULL);
   }

   return ci_ptr;
}


/* Write a few rows of image data.  If the image is interlaced,
 * either you will have to write the 7 sub images, or, if you
 * have called ci_set_interlace_handling(), you will have to
 * "write" the image seven times.
 */
void CIAPI
ci_write_rows(ci_structrp ci_ptr, ci_bytepp row,
    ci_uint_32 num_rows)
{
   ci_uint_32 i; /* row counter */
   ci_bytepp rp; /* row pointer */

   ci_debug(1, "in ci_write_rows");

   if (ci_ptr == NULL)
      return;

   /* Loop through the rows */
   for (i = 0, rp = row; i < num_rows; i++, rp++)
   {
      ci_write_row(ci_ptr, *rp);
   }
}

/* Write the image.  You only need to call this function once, even
 * if you are writing an interlaced image.
 */
void CIAPI
ci_write_image(ci_structrp ci_ptr, ci_bytepp image)
{
   ci_uint_32 i; /* row index */
   int pass, num_pass; /* pass variables */
   ci_bytepp rp; /* points to current row */

   if (ci_ptr == NULL)
      return;

   ci_debug(1, "in ci_write_image");

#ifdef CI_WRITE_INTERLACING_SUPPORTED
   /* Initialize interlace handling.  If image is not interlaced,
    * this will set pass to 1
    */
   num_pass = ci_set_interlace_handling(ci_ptr);
#else
   num_pass = 1;
#endif
   /* Loop through passes */
   for (pass = 0; pass < num_pass; pass++)
   {
      /* Loop through image */
      for (i = 0, rp = image; i < ci_ptr->height; i++, rp++)
      {
         ci_write_row(ci_ptr, *rp);
      }
   }
}

#ifdef CI_MNG_FEATURES_SUPPORTED
/* Performs intrapixel differencing  */
static void
ci_do_write_intrapixel(ci_row_infop row_info, ci_bytep row)
{
   ci_debug(1, "in ci_do_write_intrapixel");

   if ((row_info->color_type & CI_COLOR_MASK_COLOR) != 0)
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
            *(rp)     = (ci_byte)(*rp       - *(rp + 1));
            *(rp + 2) = (ci_byte)(*(rp + 2) - *(rp + 1));
         }
      }

#ifdef CI_WRITE_16BIT_SUPPORTED
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
            ci_uint_32 red  = (ci_uint_32)((s0 - s1) & 0xffffL);
            ci_uint_32 blue = (ci_uint_32)((s2 - s1) & 0xffffL);
            *(rp    ) = (ci_byte)(red >> 8);
            *(rp + 1) = (ci_byte)red;
            *(rp + 4) = (ci_byte)(blue >> 8);
            *(rp + 5) = (ci_byte)blue;
         }
      }
#endif /* WRITE_16BIT */
   }
}
#endif /* MNG_FEATURES */

/* Called by user to write a row of image data */
void CIAPI
ci_write_row(ci_structrp ci_ptr, ci_const_bytep row)
{
   /* 1.5.6: moved from ci_struct to be a local structure: */
   ci_row_info row_info;

   ci_debug2(1, "in ci_write_row (row %u, pass %d)",
       ci_ptr->row_number, ci_ptr->pass);

   if (ci_ptr == NULL)
      return;

   /* Initialize transformations and other stuff if first time */
   if (ci_ptr->row_number == 0 && ci_ptr->pass == 0)
   {
      /* Make sure we wrote the header info */
      if ((ci_ptr->mode & CI_WROTE_INFO_BEFORE_PLTE) == 0)
         ci_error(ci_ptr,
             "ci_write_info was never called before ci_write_row");

      /* Check for transforms that have been set but were defined out */
#if !defined(CI_WRITE_INVERT_SUPPORTED) && defined(CI_READ_INVERT_SUPPORTED)
      if ((ci_ptr->transformations & CI_INVERT_MONO) != 0)
         ci_warning(ci_ptr, "CI_WRITE_INVERT_SUPPORTED is not defined");
#endif

#if !defined(CI_WRITE_FILLER_SUPPORTED) && defined(CI_READ_FILLER_SUPPORTED)
      if ((ci_ptr->transformations & CI_FILLER) != 0)
         ci_warning(ci_ptr, "CI_WRITE_FILLER_SUPPORTED is not defined");
#endif
#if !defined(CI_WRITE_PACKSWAP_SUPPORTED) && \
    defined(CI_READ_PACKSWAP_SUPPORTED)
      if ((ci_ptr->transformations & CI_PACKSWAP) != 0)
         ci_warning(ci_ptr,
             "CI_WRITE_PACKSWAP_SUPPORTED is not defined");
#endif

#if !defined(CI_WRITE_PACK_SUPPORTED) && defined(CI_READ_PACK_SUPPORTED)
      if ((ci_ptr->transformations & CI_PACK) != 0)
         ci_warning(ci_ptr, "CI_WRITE_PACK_SUPPORTED is not defined");
#endif

#if !defined(CI_WRITE_SHIFT_SUPPORTED) && defined(CI_READ_SHIFT_SUPPORTED)
      if ((ci_ptr->transformations & CI_SHIFT) != 0)
         ci_warning(ci_ptr, "CI_WRITE_SHIFT_SUPPORTED is not defined");
#endif

#if !defined(CI_WRITE_BGR_SUPPORTED) && defined(CI_READ_BGR_SUPPORTED)
      if ((ci_ptr->transformations & CI_BGR) != 0)
         ci_warning(ci_ptr, "CI_WRITE_BGR_SUPPORTED is not defined");
#endif

#if !defined(CI_WRITE_SWAP_SUPPORTED) && defined(CI_READ_SWAP_SUPPORTED)
      if ((ci_ptr->transformations & CI_SWAP_BYTES) != 0)
         ci_warning(ci_ptr, "CI_WRITE_SWAP_SUPPORTED is not defined");
#endif

      ci_write_start_row(ci_ptr);
   }

#ifdef CI_WRITE_INTERLACING_SUPPORTED
   /* If interlaced and not interested in row, return */
   if (ci_ptr->interlaced != 0 &&
       (ci_ptr->transformations & CI_INTERLACE) != 0)
   {
      switch (ci_ptr->pass)
      {
         case 0:
            if ((ci_ptr->row_number & 0x07) != 0)
            {
               ci_write_finish_row(ci_ptr);
               return;
            }
            break;

         case 1:
            if ((ci_ptr->row_number & 0x07) != 0 || ci_ptr->width < 5)
            {
               ci_write_finish_row(ci_ptr);
               return;
            }
            break;

         case 2:
            if ((ci_ptr->row_number & 0x07) != 4)
            {
               ci_write_finish_row(ci_ptr);
               return;
            }
            break;

         case 3:
            if ((ci_ptr->row_number & 0x03) != 0 || ci_ptr->width < 3)
            {
               ci_write_finish_row(ci_ptr);
               return;
            }
            break;

         case 4:
            if ((ci_ptr->row_number & 0x03) != 2)
            {
               ci_write_finish_row(ci_ptr);
               return;
            }
            break;

         case 5:
            if ((ci_ptr->row_number & 0x01) != 0 || ci_ptr->width < 2)
            {
               ci_write_finish_row(ci_ptr);
               return;
            }
            break;

         case 6:
            if ((ci_ptr->row_number & 0x01) == 0)
            {
               ci_write_finish_row(ci_ptr);
               return;
            }
            break;

         default: /* error: ignore it */
            break;
      }
   }
#endif

   /* Set up row info for transformations */
   row_info.color_type = ci_ptr->color_type;
   row_info.width = ci_ptr->usr_width;
   row_info.channels = ci_ptr->usr_channels;
   row_info.bit_depth = ci_ptr->usr_bit_depth;
   row_info.pixel_depth = (ci_byte)(row_info.bit_depth * row_info.channels);
   row_info.rowbytes = CI_ROWBYTES(row_info.pixel_depth, row_info.width);

   ci_debug1(3, "row_info->color_type = %d", row_info.color_type);
   ci_debug1(3, "row_info->width = %u", row_info.width);
   ci_debug1(3, "row_info->channels = %d", row_info.channels);
   ci_debug1(3, "row_info->bit_depth = %d", row_info.bit_depth);
   ci_debug1(3, "row_info->pixel_depth = %d", row_info.pixel_depth);
   ci_debug1(3, "row_info->rowbytes = %lu", (unsigned long)row_info.rowbytes);

   /* Copy user's row into buffer, leaving room for filter byte. */
   memcpy(ci_ptr->row_buf + 1, row, row_info.rowbytes);

#ifdef CI_WRITE_INTERLACING_SUPPORTED
   /* Handle interlacing */
   if (ci_ptr->interlaced && ci_ptr->pass < 6 &&
       (ci_ptr->transformations & CI_INTERLACE) != 0)
   {
      ci_do_write_interlace(&row_info, ci_ptr->row_buf + 1, ci_ptr->pass);
      /* This should always get caught above, but still ... */
      if (row_info.width == 0)
      {
         ci_write_finish_row(ci_ptr);
         return;
      }
   }
#endif

#ifdef CI_WRITE_TRANSFORMS_SUPPORTED
   /* Handle other transformations */
   if (ci_ptr->transformations != 0)
      ci_do_write_transformations(ci_ptr, &row_info);
#endif

   /* At this point the row_info pixel depth must match the 'transformed' depth,
    * which is also the output depth.
    */
   if (row_info.pixel_depth != ci_ptr->pixel_depth ||
       row_info.pixel_depth != ci_ptr->transformed_pixel_depth)
      ci_error(ci_ptr, "internal write transform logic error");

#ifdef CI_MNG_FEATURES_SUPPORTED
   /* Write filter_method 64 (intrapixel differencing) only if
    * 1. Libci was compiled with CI_MNG_FEATURES_SUPPORTED and
    * 2. Libci did not write a CI signature (this filter_method is only
    *    used in CI datastreams that are embedded in MNG datastreams) and
    * 3. The application called ci_permit_mng_features with a mask that
    *    included CI_FLAG_MNG_FILTER_64 and
    * 4. The filter_method is 64 and
    * 5. The color_type is RGB or RGBA
    */
   if ((ci_ptr->mng_features_permitted & CI_FLAG_MNG_FILTER_64) != 0 &&
       (ci_ptr->filter_type == CI_INTRAPIXEL_DIFFERENCING))
   {
      /* Intrapixel differencing */
      ci_do_write_intrapixel(&row_info, ci_ptr->row_buf + 1);
   }
#endif

/* Added at libci-1.5.10 */
#ifdef CI_WRITE_CHECK_FOR_INVALID_INDEX_SUPPORTED
   /* Check for out-of-range palette index */
   if (row_info.color_type == CI_COLOR_TYPE_PALETTE &&
       ci_ptr->num_palette_max >= 0)
      ci_do_check_palette_indexes(ci_ptr, &row_info);
#endif

   /* Find a filter if necessary, filter the row and write it out. */
   ci_write_find_filter(ci_ptr, &row_info);

   if (ci_ptr->write_row_fn != NULL)
      (*(ci_ptr->write_row_fn))(ci_ptr, ci_ptr->row_number, ci_ptr->pass);
}

#ifdef CI_WRITE_FLUSH_SUPPORTED
/* Set the automatic flush interval or 0 to turn flushing off */
void CIAPI
ci_set_flush(ci_structrp ci_ptr, int nrows)
{
   ci_debug(1, "in ci_set_flush");

   if (ci_ptr == NULL)
      return;

   ci_ptr->flush_dist = (nrows < 0 ? 0 : (ci_uint_32)nrows);
}

/* Flush the current output buffers now */
void CIAPI
ci_write_flush(ci_structrp ci_ptr)
{
   ci_debug(1, "in ci_write_flush");

   if (ci_ptr == NULL)
      return;

   /* We have already written out all of the data */
   if (ci_ptr->row_number >= ci_ptr->num_rows)
      return;

   ci_compress_IDAT(ci_ptr, NULL, 0, Z_SYNC_FLUSH);
   ci_ptr->flush_rows = 0;
   ci_flush(ci_ptr);
}
#endif /* WRITE_FLUSH */

/* Free any memory used in ci_ptr struct without freeing the struct itself. */
static void
ci_write_destroy(ci_structrp ci_ptr)
{
   ci_debug(1, "in ci_write_destroy");

   /* Free any memory zlib uses */
   if ((ci_ptr->flags & CI_FLAG_ZSTREAM_INITIALIZED) != 0)
      deflateEnd(&ci_ptr->zstream);

   /* Free our memory.  ci_free checks NULL for us. */
   ci_free_buffer_list(ci_ptr, &ci_ptr->zbuffer_list);
   ci_free(ci_ptr, ci_ptr->row_buf);
   ci_ptr->row_buf = NULL;
#ifdef CI_WRITE_FILTER_SUPPORTED
   ci_free(ci_ptr, ci_ptr->prev_row);
   ci_free(ci_ptr, ci_ptr->try_row);
   ci_free(ci_ptr, ci_ptr->tst_row);
   ci_ptr->prev_row = NULL;
   ci_ptr->try_row = NULL;
   ci_ptr->tst_row = NULL;
#endif

#ifdef CI_SET_UNKNOWN_CHUNKS_SUPPORTED
   ci_free(ci_ptr, ci_ptr->chunk_list);
   ci_ptr->chunk_list = NULL;
#endif

   /* The error handling and memory handling information is left intact at this
    * point: the jmp_buf may still have to be freed.  See ci_destroy_ci_struct
    * for how this happens.
    */
}

/* Free all memory used by the write.
 * In libci 1.6.0 this API changed quietly to no longer accept a NULL value for
 * *ci_ptr_ptr.  Prior to 1.6.0 it would accept such a value and it would free
 * the passed in info_structs but it would quietly fail to free any of the data
 * inside them.  In 1.6.0 it quietly does nothing (it has to be quiet because it
 * has no ci_ptr.)
 */
void CIAPI
ci_destroy_write_struct(ci_structpp ci_ptr_ptr, ci_infopp info_ptr_ptr)
{
   ci_debug(1, "in ci_destroy_write_struct");

   if (ci_ptr_ptr != NULL)
   {
      ci_structrp ci_ptr = *ci_ptr_ptr;

      if (ci_ptr != NULL) /* added in libci 1.6.0 */
      {
         ci_destroy_info_struct(ci_ptr, info_ptr_ptr);

         *ci_ptr_ptr = NULL;
         ci_write_destroy(ci_ptr);
         ci_destroy_ci_struct(ci_ptr);
      }
   }
}

/* Allow the application to select one or more row filters to use. */
void CIAPI
ci_set_filter(ci_structrp ci_ptr, int method, int filters)
{
   ci_debug(1, "in ci_set_filter");

   if (ci_ptr == NULL)
      return;

#ifdef CI_MNG_FEATURES_SUPPORTED
   if ((ci_ptr->mng_features_permitted & CI_FLAG_MNG_FILTER_64) != 0 &&
       (method == CI_INTRAPIXEL_DIFFERENCING))
      method = CI_FILTER_TYPE_BASE;

#endif
   if (method == CI_FILTER_TYPE_BASE)
   {
      switch (filters & (CI_ALL_FILTERS | 0x07))
      {
#ifdef CI_WRITE_FILTER_SUPPORTED
         case 5:
         case 6:
         case 7: ci_app_error(ci_ptr, "Unknown row filter for method 0");
#endif /* WRITE_FILTER */
            /* FALLTHROUGH */
         case CI_FILTER_VALUE_NONE:
            ci_ptr->do_filter = CI_FILTER_NONE; break;

#ifdef CI_WRITE_FILTER_SUPPORTED
         case CI_FILTER_VALUE_SUB:
            ci_ptr->do_filter = CI_FILTER_SUB; break;

         case CI_FILTER_VALUE_UP:
            ci_ptr->do_filter = CI_FILTER_UP; break;

         case CI_FILTER_VALUE_AVG:
            ci_ptr->do_filter = CI_FILTER_AVG; break;

         case CI_FILTER_VALUE_PAETH:
            ci_ptr->do_filter = CI_FILTER_PAETH; break;

         default:
            ci_ptr->do_filter = (ci_byte)filters; break;
#else
         default:
            ci_app_error(ci_ptr, "Unknown row filter for method 0");
#endif /* WRITE_FILTER */
      }

#ifdef CI_WRITE_FILTER_SUPPORTED
      /* If we have allocated the row_buf, this means we have already started
       * with the image and we should have allocated all of the filter buffers
       * that have been selected.  If prev_row isn't already allocated, then
       * it is too late to start using the filters that need it, since we
       * will be missing the data in the previous row.  If an application
       * wants to start and stop using particular filters during compression,
       * it should start out with all of the filters, and then remove them
       * or add them back after the start of compression.
       *
       * NOTE: this is a nasty constraint on the code, because it means that the
       * prev_row buffer must be maintained even if there are currently no
       * 'prev_row' requiring filters active.
       */
      if (ci_ptr->row_buf != NULL)
      {
         int num_filters;
         ci_alloc_size_t buf_size;

         /* Repeat the checks in ci_write_start_row; 1 pixel high or wide
          * images cannot benefit from certain filters.  If this isn't done here
          * the check below will fire on 1 pixel high images.
          */
         if (ci_ptr->height == 1)
            filters &= ~(CI_FILTER_UP|CI_FILTER_AVG|CI_FILTER_PAETH);

         if (ci_ptr->width == 1)
            filters &= ~(CI_FILTER_SUB|CI_FILTER_AVG|CI_FILTER_PAETH);

         if ((filters & (CI_FILTER_UP|CI_FILTER_AVG|CI_FILTER_PAETH)) != 0
            && ci_ptr->prev_row == NULL)
         {
            /* This is the error case, however it is benign - the previous row
             * is not available so the filter can't be used.  Just warn here.
             */
            ci_app_warning(ci_ptr,
                "ci_set_filter: UP/AVG/PAETH cannot be added after start");
            filters &= ~(CI_FILTER_UP|CI_FILTER_AVG|CI_FILTER_PAETH);
         }

         num_filters = 0;

         if (filters & CI_FILTER_SUB)
            num_filters++;

         if (filters & CI_FILTER_UP)
            num_filters++;

         if (filters & CI_FILTER_AVG)
            num_filters++;

         if (filters & CI_FILTER_PAETH)
            num_filters++;

         /* Allocate needed row buffers if they have not already been
          * allocated.
          */
         buf_size = CI_ROWBYTES(ci_ptr->usr_channels * ci_ptr->usr_bit_depth,
             ci_ptr->width) + 1;

         if (ci_ptr->try_row == NULL)
            ci_ptr->try_row = ci_voidcast(ci_bytep,
                ci_malloc(ci_ptr, buf_size));

         if (num_filters > 1)
         {
            if (ci_ptr->tst_row == NULL)
               ci_ptr->tst_row = ci_voidcast(ci_bytep,
                   ci_malloc(ci_ptr, buf_size));
         }
      }
      ci_ptr->do_filter = (ci_byte)filters;
#endif
   }
   else
      ci_error(ci_ptr, "Unknown custom filter method");
}

#ifdef CI_WRITE_WEIGHTED_FILTER_SUPPORTED /* DEPRECATED */
/* Provide floating and fixed point APIs */
#ifdef CI_FLOATING_POINT_SUPPORTED
void CIAPI
ci_set_filter_heuristics(ci_structrp ci_ptr, int heuristic_method,
    int num_weights, ci_const_doublep filter_weights,
    ci_const_doublep filter_costs)
{
   CI_UNUSED(ci_ptr)
   CI_UNUSED(heuristic_method)
   CI_UNUSED(num_weights)
   CI_UNUSED(filter_weights)
   CI_UNUSED(filter_costs)
}
#endif /* FLOATING_POINT */

#ifdef CI_FIXED_POINT_SUPPORTED
void CIAPI
ci_set_filter_heuristics_fixed(ci_structrp ci_ptr, int heuristic_method,
    int num_weights, ci_const_fixed_point_p filter_weights,
    ci_const_fixed_point_p filter_costs)
{
   CI_UNUSED(ci_ptr)
   CI_UNUSED(heuristic_method)
   CI_UNUSED(num_weights)
   CI_UNUSED(filter_weights)
   CI_UNUSED(filter_costs)
}
#endif /* FIXED_POINT */
#endif /* WRITE_WEIGHTED_FILTER */

#ifdef CI_WRITE_CUSTOMIZE_COMPRESSION_SUPPORTED
void CIAPI
ci_set_compression_level(ci_structrp ci_ptr, int level)
{
   ci_debug(1, "in ci_set_compression_level");

   if (ci_ptr == NULL)
      return;

   ci_ptr->zlib_level = level;
}

void CIAPI
ci_set_compression_mem_level(ci_structrp ci_ptr, int mem_level)
{
   ci_debug(1, "in ci_set_compression_mem_level");

   if (ci_ptr == NULL)
      return;

   ci_ptr->zlib_mem_level = mem_level;
}

void CIAPI
ci_set_compression_strategy(ci_structrp ci_ptr, int strategy)
{
   ci_debug(1, "in ci_set_compression_strategy");

   if (ci_ptr == NULL)
      return;

   /* The flag setting here prevents the libci dynamic selection of strategy.
    */
   ci_ptr->flags |= CI_FLAG_ZLIB_CUSTOM_STRATEGY;
   ci_ptr->zlib_strategy = strategy;
}

/* If CI_WRITE_OPTIMIZE_CMF_SUPPORTED is defined, libci will use a
 * smaller value of window_bits if it can do so safely.
 */
void CIAPI
ci_set_compression_window_bits(ci_structrp ci_ptr, int window_bits)
{
   ci_debug(1, "in ci_set_compression_window_bits");

   if (ci_ptr == NULL)
      return;

   /* Prior to 1.6.0 this would warn but then set the window_bits value. This
    * meant that negative window bits values could be selected that would cause
    * libci to write a non-standard CI file with raw deflate or gzip
    * compressed IDAT or ancillary chunks.  Such files can be read and there is
    * no warning on read, so this seems like a very bad idea.
    */
   if (window_bits > 15)
   {
      ci_warning(ci_ptr, "Only compression windows <= 32k supported by CI");
      window_bits = 15;
   }

   else if (window_bits < 8)
   {
      ci_warning(ci_ptr, "Only compression windows >= 256 supported by CI");
      window_bits = 8;
   }

   ci_ptr->zlib_window_bits = window_bits;
}

void CIAPI
ci_set_compression_method(ci_structrp ci_ptr, int method)
{
   ci_debug(1, "in ci_set_compression_method");

   if (ci_ptr == NULL)
      return;

   /* This would produce an invalid CI file if it worked, but it doesn't and
    * deflate will fault it, so it is harmless to just warn here.
    */
   if (method != 8)
      ci_warning(ci_ptr, "Only compression method 8 is supported by CI");

   ci_ptr->zlib_method = method;
}
#endif /* WRITE_CUSTOMIZE_COMPRESSION */

/* The following were added to libci-1.5.4 */
#ifdef CI_WRITE_CUSTOMIZE_ZTXT_COMPRESSION_SUPPORTED
void CIAPI
ci_set_text_compression_level(ci_structrp ci_ptr, int level)
{
   ci_debug(1, "in ci_set_text_compression_level");

   if (ci_ptr == NULL)
      return;

   ci_ptr->zlib_text_level = level;
}

void CIAPI
ci_set_text_compression_mem_level(ci_structrp ci_ptr, int mem_level)
{
   ci_debug(1, "in ci_set_text_compression_mem_level");

   if (ci_ptr == NULL)
      return;

   ci_ptr->zlib_text_mem_level = mem_level;
}

void CIAPI
ci_set_text_compression_strategy(ci_structrp ci_ptr, int strategy)
{
   ci_debug(1, "in ci_set_text_compression_strategy");

   if (ci_ptr == NULL)
      return;

   ci_ptr->zlib_text_strategy = strategy;
}

/* If CI_WRITE_OPTIMIZE_CMF_SUPPORTED is defined, libci will use a
 * smaller value of window_bits if it can do so safely.
 */
void CIAPI
ci_set_text_compression_window_bits(ci_structrp ci_ptr, int window_bits)
{
   ci_debug(1, "in ci_set_text_compression_window_bits");

   if (ci_ptr == NULL)
      return;

   if (window_bits > 15)
   {
      ci_warning(ci_ptr, "Only compression windows <= 32k supported by CI");
      window_bits = 15;
   }

   else if (window_bits < 8)
   {
      ci_warning(ci_ptr, "Only compression windows >= 256 supported by CI");
      window_bits = 8;
   }

   ci_ptr->zlib_text_window_bits = window_bits;
}

void CIAPI
ci_set_text_compression_method(ci_structrp ci_ptr, int method)
{
   ci_debug(1, "in ci_set_text_compression_method");

   if (ci_ptr == NULL)
      return;

   if (method != 8)
      ci_warning(ci_ptr, "Only compression method 8 is supported by CI");

   ci_ptr->zlib_text_method = method;
}
#endif /* WRITE_CUSTOMIZE_ZTXT_COMPRESSION */
/* end of API added to libci-1.5.4 */

void CIAPI
ci_set_write_status_fn(ci_structrp ci_ptr, ci_write_status_ptr write_row_fn)
{
   ci_debug(1, "in ci_set_write_status_fn");

   if (ci_ptr == NULL)
      return;

   ci_ptr->write_row_fn = write_row_fn;
}

#ifdef CI_WRITE_USER_TRANSFORM_SUPPORTED
void CIAPI
ci_set_write_user_transform_fn(ci_structrp ci_ptr, ci_user_transform_ptr
    write_user_transform_fn)
{
   ci_debug(1, "in ci_set_write_user_transform_fn");

   if (ci_ptr == NULL)
      return;

   ci_ptr->transformations |= CI_USER_TRANSFORM;
   ci_ptr->write_user_transform_fn = write_user_transform_fn;
}
#endif


#ifdef CI_INFO_IMAGE_SUPPORTED
void CIAPI
ci_write_ci(ci_structrp ci_ptr, ci_inforp info_ptr,
    int transforms, voidp params)
{
   ci_debug(1, "in ci_write_ci");

   if (ci_ptr == NULL || info_ptr == NULL)
      return;

   if ((info_ptr->valid & CI_INFO_IDAT) == 0)
   {
      ci_app_error(ci_ptr, "no rows for ci_write_image to write");
      return;
   }

   /* Write the file header information. */
   ci_write_info(ci_ptr, info_ptr);

   /* ------ these transformations don't touch the info structure ------- */

   /* Invert monochrome pixels */
   if ((transforms & CI_TRANSFORM_INVERT_MONO) != 0)
#ifdef CI_WRITE_INVERT_SUPPORTED
      ci_set_invert_mono(ci_ptr);
#else
      ci_app_error(ci_ptr, "CI_TRANSFORM_INVERT_MONO not supported");
#endif

   /* Shift the pixels up to a legal bit depth and fill in
    * as appropriate to correctly scale the image.
    */
   if ((transforms & CI_TRANSFORM_SHIFT) != 0)
#ifdef CI_WRITE_SHIFT_SUPPORTED
      if ((info_ptr->valid & CI_INFO_sBIT) != 0)
         ci_set_shift(ci_ptr, &info_ptr->sig_bit);
#else
      ci_app_error(ci_ptr, "CI_TRANSFORM_SHIFT not supported");
#endif

   /* Pack pixels into bytes */
   if ((transforms & CI_TRANSFORM_PACKING) != 0)
#ifdef CI_WRITE_PACK_SUPPORTED
      ci_set_packing(ci_ptr);
#else
      ci_app_error(ci_ptr, "CI_TRANSFORM_PACKING not supported");
#endif

   /* Swap location of alpha bytes from ARGB to RGBA */
   if ((transforms & CI_TRANSFORM_SWAP_ALPHA) != 0)
#ifdef CI_WRITE_SWAP_ALPHA_SUPPORTED
      ci_set_swap_alpha(ci_ptr);
#else
      ci_app_error(ci_ptr, "CI_TRANSFORM_SWAP_ALPHA not supported");
#endif

   /* Remove a filler (X) from XRGB/RGBX/AG/GA into to convert it into
    * RGB, note that the code expects the input color type to be G or RGB; no
    * alpha channel.
    */
   if ((transforms & (CI_TRANSFORM_STRIP_FILLER_AFTER|
       CI_TRANSFORM_STRIP_FILLER_BEFORE)) != 0)
   {
#ifdef CI_WRITE_FILLER_SUPPORTED
      if ((transforms & CI_TRANSFORM_STRIP_FILLER_AFTER) != 0)
      {
         if ((transforms & CI_TRANSFORM_STRIP_FILLER_BEFORE) != 0)
            ci_app_error(ci_ptr,
                "CI_TRANSFORM_STRIP_FILLER: BEFORE+AFTER not supported");

         /* Continue if ignored - this is the pre-1.6.10 behavior */
         ci_set_filler(ci_ptr, 0, CI_FILLER_AFTER);
      }

      else if ((transforms & CI_TRANSFORM_STRIP_FILLER_BEFORE) != 0)
         ci_set_filler(ci_ptr, 0, CI_FILLER_BEFORE);
#else
      ci_app_error(ci_ptr, "CI_TRANSFORM_STRIP_FILLER not supported");
#endif
   }

   /* Flip BGR pixels to RGB */
   if ((transforms & CI_TRANSFORM_BGR) != 0)
#ifdef CI_WRITE_BGR_SUPPORTED
      ci_set_bgr(ci_ptr);
#else
      ci_app_error(ci_ptr, "CI_TRANSFORM_BGR not supported");
#endif

   /* Swap bytes of 16-bit files to most significant byte first */
   if ((transforms & CI_TRANSFORM_SWAP_ENDIAN) != 0)
#ifdef CI_WRITE_SWAP_SUPPORTED
      ci_set_swap(ci_ptr);
#else
      ci_app_error(ci_ptr, "CI_TRANSFORM_SWAP_ENDIAN not supported");
#endif

   /* Swap bits of 1-bit, 2-bit, 4-bit packed pixel formats */
   if ((transforms & CI_TRANSFORM_PACKSWAP) != 0)
#ifdef CI_WRITE_PACKSWAP_SUPPORTED
      ci_set_packswap(ci_ptr);
#else
      ci_app_error(ci_ptr, "CI_TRANSFORM_PACKSWAP not supported");
#endif

   /* Invert the alpha channel from opacity to transparency */
   if ((transforms & CI_TRANSFORM_INVERT_ALPHA) != 0)
#ifdef CI_WRITE_INVERT_ALPHA_SUPPORTED
      ci_set_invert_alpha(ci_ptr);
#else
      ci_app_error(ci_ptr, "CI_TRANSFORM_INVERT_ALPHA not supported");
#endif

   /* ----------------------- end of transformations ------------------- */

   /* Write the bits */
   ci_write_image(ci_ptr, info_ptr->row_pointers);

   /* It is REQUIRED to call this to finish writing the rest of the file */
   ci_write_end(ci_ptr, info_ptr);

   CI_UNUSED(params)
}
#endif


#ifdef CI_SIMPLIFIED_WRITE_SUPPORTED
/* Initialize the write structure - general purpose utility. */
static int
ci_image_write_init(ci_imagep image)
{
   ci_structp ci_ptr = ci_create_write_struct(CI_LIBCI_VER_STRING, image,
       ci_safe_error, ci_safe_warning);

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
            control->for_write = 1;

            image->opaque = control;
            return 1;
         }

         /* Error clean up */
         ci_destroy_info_struct(ci_ptr, &info_ptr);
      }

      ci_destroy_write_struct(&ci_ptr, NULL);
   }

   return ci_image_error(image, "ci_image_write_: out of memory");
}

/* Arguments to ci_image_write_main: */
typedef struct
{
   /* Arguments: */
   ci_imagep      image;
   ci_const_voidp buffer;
   ci_int_32      row_stride;
   ci_const_voidp colormap;
   int             convert_to_8bit;
   /* Local variables: */
   ci_const_voidp first_row;
   ptrdiff_t       row_bytes;
   ci_voidp       local_row;
   /* Byte count for memory writing */
   ci_bytep        memory;
   ci_alloc_size_t memory_bytes; /* not used for STDIO */
   ci_alloc_size_t output_bytes; /* running total */
} ci_image_write_control;

/* Write ci_uint_16 input to a 16-bit CI; the ci_ptr has already been set to
 * do any necessary byte swapping.  The component order is defined by the
 * ci_image format value.
 */
static int
ci_write_image_16bit(ci_voidp argument)
{
   ci_image_write_control *display = ci_voidcast(ci_image_write_control*,
       argument);
   ci_imagep image = display->image;
   ci_structrp ci_ptr = image->opaque->ci_ptr;

   ci_const_uint_16p input_row = ci_voidcast(ci_const_uint_16p,
       display->first_row);
   ci_uint_16p output_row = ci_voidcast(ci_uint_16p, display->local_row);
   ci_uint_16p row_end;
   unsigned int channels = (image->format & CI_FORMAT_FLAG_COLOR) != 0 ?
       3 : 1;
   int aindex = 0;
   ci_uint_32 y = image->height;

   if ((image->format & CI_FORMAT_FLAG_ALPHA) != 0)
   {
#   ifdef CI_SIMPLIFIED_WRITE_AFIRST_SUPPORTED
      if ((image->format & CI_FORMAT_FLAG_AFIRST) != 0)
      {
         aindex = -1;
         ++input_row; /* To point to the first component */
         ++output_row;
      }
         else
            aindex = (int)channels;
#     else
         aindex = (int)channels;
#     endif
   }

   else
      ci_error(ci_ptr, "ci_write_image: internal call error");

   /* Work out the output row end and count over this, note that the increment
    * above to 'row' means that row_end can actually be beyond the end of the
    * row; this is correct.
    */
   row_end = output_row + image->width * (channels+1);

   for (; y > 0; --y)
   {
      ci_const_uint_16p in_ptr = input_row;
      ci_uint_16p out_ptr = output_row;

      while (out_ptr < row_end)
      {
         ci_uint_16 alpha = in_ptr[aindex];
         ci_uint_32 reciprocal = 0;
         int c;

         out_ptr[aindex] = alpha;

         /* Calculate a reciprocal.  The correct calculation is simply
          * component/alpha*65535 << 15. (I.e. 15 bits of precision); this
          * allows correct rounding by adding .5 before the shift.  'reciprocal'
          * is only initialized when required.
          */
         if (alpha > 0 && alpha < 65535)
            reciprocal = ((0xffff<<15)+(alpha>>1))/alpha;

         c = (int)channels;
         do /* always at least one channel */
         {
            ci_uint_16 component = *in_ptr++;

            /* The following gives 65535 for an alpha of 0, which is fine,
             * otherwise if 0/0 is represented as some other value there is more
             * likely to be a discontinuity which will probably damage
             * compression when moving from a fully transparent area to a
             * nearly transparent one.  (The assumption here is that opaque
             * areas tend not to be 0 intensity.)
             */
            if (component >= alpha)
               component = 65535;

            /* component<alpha, so component/alpha is less than one and
             * component*reciprocal is less than 2^31.
             */
            else if (component > 0 && alpha < 65535)
            {
               ci_uint_32 calc = component * reciprocal;
               calc += 16384; /* round to nearest */
               component = (ci_uint_16)(calc >> 15);
            }

            *out_ptr++ = component;
         }
         while (--c > 0);

         /* Skip to next component (skip the intervening alpha channel) */
         ++in_ptr;
         ++out_ptr;
      }

      ci_write_row(ci_ptr, ci_voidcast(ci_const_bytep, display->local_row));
      input_row += (ci_uint_16)display->row_bytes/(sizeof (ci_uint_16));
   }

   return 1;
}

/* Given 16-bit input (1 to 4 channels) write 8-bit output.  If an alpha channel
 * is present it must be removed from the components, the components are then
 * written in sRGB encoding.  No components are added or removed.
 *
 * Calculate an alpha reciprocal to reverse pre-multiplication.  As above the
 * calculation can be done to 15 bits of accuracy; however, the output needs to
 * be scaled in the range 0..255*65535, so include that scaling here.
 */
#   define UNP_RECIPROCAL(alpha) ((((0xffff*0xff)<<7)+((alpha)>>1))/(alpha))

static ci_byte
ci_unpremultiply(ci_uint_32 component, ci_uint_32 alpha,
    ci_uint_32 reciprocal/*from the above macro*/)
{
   /* The following gives 1.0 for an alpha of 0, which is fine, otherwise if 0/0
    * is represented as some other value there is more likely to be a
    * discontinuity which will probably damage compression when moving from a
    * fully transparent area to a nearly transparent one.  (The assumption here
    * is that opaque areas tend not to be 0 intensity.)
    *
    * There is a rounding problem here; if alpha is less than 128 it will end up
    * as 0 when scaled to 8 bits.  To avoid introducing spurious colors into the
    * output change for this too.
    */
   if (component >= alpha || alpha < 128)
      return 255;

   /* component<alpha, so component/alpha is less than one and
    * component*reciprocal is less than 2^31.
    */
   else if (component > 0)
   {
      /* The test is that alpha/257 (rounded) is less than 255, the first value
       * that becomes 255 is 65407.
       * NOTE: this must agree with the CI_DIV257 macro (which must, therefore,
       * be exact!)  [Could also test reciprocal != 0]
       */
      if (alpha < 65407)
      {
         component *= reciprocal;
         component += 64; /* round to nearest */
         component >>= 7;
      }

      else
         component *= 255;

      /* Convert the component to sRGB. */
      return (ci_byte)CI_sRGB_FROM_LINEAR(component);
   }

   else
      return 0;
}

static int
ci_write_image_8bit(ci_voidp argument)
{
   ci_image_write_control *display = ci_voidcast(ci_image_write_control*,
       argument);
   ci_imagep image = display->image;
   ci_structrp ci_ptr = image->opaque->ci_ptr;

   ci_const_uint_16p input_row = ci_voidcast(ci_const_uint_16p,
       display->first_row);
   ci_bytep output_row = ci_voidcast(ci_bytep, display->local_row);
   ci_uint_32 y = image->height;
   unsigned int channels = (image->format & CI_FORMAT_FLAG_COLOR) != 0 ?
       3 : 1;

   if ((image->format & CI_FORMAT_FLAG_ALPHA) != 0)
   {
      ci_bytep row_end;
      int aindex;

#   ifdef CI_SIMPLIFIED_WRITE_AFIRST_SUPPORTED
      if ((image->format & CI_FORMAT_FLAG_AFIRST) != 0)
      {
         aindex = -1;
         ++input_row; /* To point to the first component */
         ++output_row;
      }

      else
#   endif
      aindex = (int)channels;

      /* Use row_end in place of a loop counter: */
      row_end = output_row + image->width * (channels+1);

      for (; y > 0; --y)
      {
         ci_const_uint_16p in_ptr = input_row;
         ci_bytep out_ptr = output_row;

         while (out_ptr < row_end)
         {
            ci_uint_16 alpha = in_ptr[aindex];
            ci_byte alphabyte = (ci_byte)CI_DIV257(alpha);
            ci_uint_32 reciprocal = 0;
            int c;

            /* Scale and write the alpha channel. */
            out_ptr[aindex] = alphabyte;

            if (alphabyte > 0 && alphabyte < 255)
               reciprocal = UNP_RECIPROCAL(alpha);

            c = (int)channels;
            do /* always at least one channel */
               *out_ptr++ = ci_unpremultiply(*in_ptr++, alpha, reciprocal);
            while (--c > 0);

            /* Skip to next component (skip the intervening alpha channel) */
            ++in_ptr;
            ++out_ptr;
         } /* while out_ptr < row_end */

         ci_write_row(ci_ptr, ci_voidcast(ci_const_bytep,
             display->local_row));
         input_row += (ci_uint_16)display->row_bytes/(sizeof (ci_uint_16));
      } /* while y */
   }

   else
   {
      /* No alpha channel, so the row_end really is the end of the row and it
       * is sufficient to loop over the components one by one.
       */
      ci_bytep row_end = output_row + image->width * channels;

      for (; y > 0; --y)
      {
         ci_const_uint_16p in_ptr = input_row;
         ci_bytep out_ptr = output_row;

         while (out_ptr < row_end)
         {
            ci_uint_32 component = *in_ptr++;

            component *= 255;
            *out_ptr++ = (ci_byte)CI_sRGB_FROM_LINEAR(component);
         }

         ci_write_row(ci_ptr, output_row);
         input_row += (ci_uint_16)display->row_bytes/(sizeof (ci_uint_16));
      }
   }

   return 1;
}

static void
ci_image_set_PLTE(ci_image_write_control *display)
{
   ci_imagep image = display->image;
   const void *cmap = display->colormap;
   int entries = image->colormap_entries > 256 ? 256 :
       (int)image->colormap_entries;

   /* NOTE: the caller must check for cmap != NULL and entries != 0 */
   ci_uint_32 format = image->format;
   unsigned int channels = CI_IMAGE_SAMPLE_CHANNELS(format);

#   if defined(CI_FORMAT_BGR_SUPPORTED) &&\
      defined(CI_SIMPLIFIED_WRITE_AFIRST_SUPPORTED)
      int afirst = (format & CI_FORMAT_FLAG_AFIRST) != 0 &&
          (format & CI_FORMAT_FLAG_ALPHA) != 0;
#   else
#     define afirst 0
#   endif

#   ifdef CI_FORMAT_BGR_SUPPORTED
      int bgr = (format & CI_FORMAT_FLAG_BGR) != 0 ? 2 : 0;
#   else
#     define bgr 0
#   endif

   int i, num_trans;
   ci_color palette[256];
   ci_byte tRNS[256];

   memset(tRNS, 255, (sizeof tRNS));
   memset(palette, 0, (sizeof palette));

   for (i=num_trans=0; i<entries; ++i)
   {
      /* This gets automatically converted to sRGB with reversal of the
       * pre-multiplication if the color-map has an alpha channel.
       */
      if ((format & CI_FORMAT_FLAG_LINEAR) != 0)
      {
         ci_const_uint_16p entry = ci_voidcast(ci_const_uint_16p, cmap);

         entry += (unsigned int)i * channels;

         if ((channels & 1) != 0) /* no alpha */
         {
            if (channels >= 3) /* RGB */
            {
               palette[i].blue = (ci_byte)CI_sRGB_FROM_LINEAR(255 *
                   entry[(2 ^ bgr)]);
               palette[i].green = (ci_byte)CI_sRGB_FROM_LINEAR(255 *
                   entry[1]);
               palette[i].red = (ci_byte)CI_sRGB_FROM_LINEAR(255 *
                   entry[bgr]);
            }

            else /* Gray */
               palette[i].blue = palette[i].red = palette[i].green =
                  (ci_byte)CI_sRGB_FROM_LINEAR(255 * *entry);
         }

         else /* alpha */
         {
            ci_uint_16 alpha = entry[afirst ? 0 : channels-1];
            ci_byte alphabyte = (ci_byte)CI_DIV257(alpha);
            ci_uint_32 reciprocal = 0;

            /* Calculate a reciprocal, as in the ci_write_image_8bit code above
             * this is designed to produce a value scaled to 255*65535 when
             * divided by 128 (i.e. asr 7).
             */
            if (alphabyte > 0 && alphabyte < 255)
               reciprocal = (((0xffff*0xff)<<7)+(alpha>>1))/alpha;

            tRNS[i] = alphabyte;
            if (alphabyte < 255)
               num_trans = i+1;

            if (channels >= 3) /* RGB */
            {
               palette[i].blue = ci_unpremultiply(entry[afirst + (2 ^ bgr)],
                   alpha, reciprocal);
               palette[i].green = ci_unpremultiply(entry[afirst + 1], alpha,
                   reciprocal);
               palette[i].red = ci_unpremultiply(entry[afirst + bgr], alpha,
                   reciprocal);
            }

            else /* gray */
               palette[i].blue = palette[i].red = palette[i].green =
                   ci_unpremultiply(entry[afirst], alpha, reciprocal);
         }
      }

      else /* Color-map has sRGB values */
      {
         ci_const_bytep entry = ci_voidcast(ci_const_bytep, cmap);

         entry += (unsigned int)i * channels;

         switch (channels)
         {
            case 4:
               tRNS[i] = entry[afirst ? 0 : 3];
               if (tRNS[i] < 255)
                  num_trans = i+1;
               /* FALLTHROUGH */
            case 3:
               palette[i].blue = entry[afirst + (2 ^ bgr)];
               palette[i].green = entry[afirst + 1];
               palette[i].red = entry[afirst + bgr];
               break;

            case 2:
               tRNS[i] = entry[1 ^ afirst];
               if (tRNS[i] < 255)
                  num_trans = i+1;
               /* FALLTHROUGH */
            case 1:
               palette[i].blue = palette[i].red = palette[i].green =
                  entry[afirst];
               break;

            default:
               break;
         }
      }
   }

#   ifdef afirst
#     undef afirst
#   endif
#   ifdef bgr
#     undef bgr
#   endif

   ci_set_PLTE(image->opaque->ci_ptr, image->opaque->info_ptr, palette,
       entries);

   if (num_trans > 0)
      ci_set_tRNS(image->opaque->ci_ptr, image->opaque->info_ptr, tRNS,
          num_trans, NULL);

   image->colormap_entries = (ci_uint_32)entries;
}

static int
ci_image_write_main(ci_voidp argument)
{
   ci_image_write_control *display = ci_voidcast(ci_image_write_control*,
       argument);
   ci_imagep image = display->image;
   ci_structrp ci_ptr = image->opaque->ci_ptr;
   ci_inforp info_ptr = image->opaque->info_ptr;
   ci_uint_32 format = image->format;

   /* The following four ints are actually booleans */
   int colormap = (format & CI_FORMAT_FLAG_COLORMAP);
   int linear = !colormap && (format & CI_FORMAT_FLAG_LINEAR); /* input */
   int alpha = !colormap && (format & CI_FORMAT_FLAG_ALPHA);
   int write_16bit = linear && (display->convert_to_8bit == 0);

#   ifdef CI_BENIGN_ERRORS_SUPPORTED
      /* Make sure we error out on any bad situation */
      ci_set_benign_errors(ci_ptr, 0/*error*/);
#   endif

   /* Default the 'row_stride' parameter if required, also check the row stride
    * and total image size to ensure that they are within the system limits.
    */
   {
      unsigned int channels = CI_IMAGE_PIXEL_CHANNELS(image->format);

      if (image->width <= 0x7fffffffU/channels) /* no overflow */
      {
         ci_uint_32 check;
         ci_uint_32 ci_row_stride = image->width * channels;

         if (display->row_stride == 0)
            display->row_stride = (ci_int_32)/*SAFE*/ci_row_stride;

         if (display->row_stride < 0)
            check = (ci_uint_32)(-display->row_stride);

         else
            check = (ci_uint_32)display->row_stride;

         if (check >= ci_row_stride)
         {
            /* Now check for overflow of the image buffer calculation; this
             * limits the whole image size to 32 bits for API compatibility with
             * the current, 32-bit, CI_IMAGE_BUFFER_SIZE macro.
             */
            if (image->height > 0xffffffffU/ci_row_stride)
               ci_error(image->opaque->ci_ptr, "memory image too large");
         }

         else
            ci_error(image->opaque->ci_ptr, "supplied row stride too small");
      }

      else
         ci_error(image->opaque->ci_ptr, "image row stride too large");
   }

   /* Set the required transforms then write the rows in the correct order. */
   if ((format & CI_FORMAT_FLAG_COLORMAP) != 0)
   {
      if (display->colormap != NULL && image->colormap_entries > 0)
      {
         ci_uint_32 entries = image->colormap_entries;

         ci_set_IHDR(ci_ptr, info_ptr, image->width, image->height,
             entries > 16 ? 8 : (entries > 4 ? 4 : (entries > 2 ? 2 : 1)),
             CI_COLOR_TYPE_PALETTE, CI_INTERLACE_NONE,
             CI_COMPRESSION_TYPE_BASE, CI_FILTER_TYPE_BASE);

         ci_image_set_PLTE(display);
      }

      else
         ci_error(image->opaque->ci_ptr,
             "no color-map for color-mapped image");
   }

   else
      ci_set_IHDR(ci_ptr, info_ptr, image->width, image->height,
          write_16bit ? 16 : 8,
          ((format & CI_FORMAT_FLAG_COLOR) ? CI_COLOR_MASK_COLOR : 0) +
          ((format & CI_FORMAT_FLAG_ALPHA) ? CI_COLOR_MASK_ALPHA : 0),
          CI_INTERLACE_NONE, CI_COMPRESSION_TYPE_BASE, CI_FILTER_TYPE_BASE);

   /* Counter-intuitively the data transformations must be called *after*
    * ci_write_info, not before as in the read code, but the 'set' functions
    * must still be called before.  Just set the color space information, never
    * write an interlaced image.
    */

   if (write_16bit != 0)
   {
      /* The gamma here is 1.0 (linear) and the cHRM chunk matches sRGB. */
      ci_set_gAMA_fixed(ci_ptr, info_ptr, CI_GAMMA_LINEAR);

      if ((image->flags & CI_IMAGE_FLAG_COLORSPACE_NOT_sRGB) == 0)
         ci_set_cHRM_fixed(ci_ptr, info_ptr,
             /* color      x       y */
             /* white */ 31270, 32900,
             /* red   */ 64000, 33000,
             /* green */ 30000, 60000,
             /* blue  */ 15000,  6000
         );
   }

   else if ((image->flags & CI_IMAGE_FLAG_COLORSPACE_NOT_sRGB) == 0)
      ci_set_sRGB(ci_ptr, info_ptr, CI_sRGB_INTENT_PERCEPTUAL);

   /* Else writing an 8-bit file and the *colors* aren't sRGB, but the 8-bit
    * space must still be gamma encoded.
    */
   else
      ci_set_gAMA_fixed(ci_ptr, info_ptr, CI_GAMMA_sRGB_INVERSE);

   /* Write the file header. */
   ci_write_info(ci_ptr, info_ptr);

   /* Now set up the data transformations (*after* the header is written),
    * remove the handled transformations from the 'format' flags for checking.
    *
    * First check for a little endian system if writing 16-bit files.
    */
   if (write_16bit != 0)
   {
      ci_uint_16 le = 0x0001;

      if ((*(ci_const_bytep) & le) != 0)
         ci_set_swap(ci_ptr);
   }

#   ifdef CI_SIMPLIFIED_WRITE_BGR_SUPPORTED
      if ((format & CI_FORMAT_FLAG_BGR) != 0)
      {
         if (colormap == 0 && (format & CI_FORMAT_FLAG_COLOR) != 0)
            ci_set_bgr(ci_ptr);
         format &= ~CI_FORMAT_FLAG_BGR;
      }
#   endif

#   ifdef CI_SIMPLIFIED_WRITE_AFIRST_SUPPORTED
      if ((format & CI_FORMAT_FLAG_AFIRST) != 0)
      {
         if (colormap == 0 && (format & CI_FORMAT_FLAG_ALPHA) != 0)
            ci_set_swap_alpha(ci_ptr);
         format &= ~CI_FORMAT_FLAG_AFIRST;
      }
#   endif

   /* If there are 16 or fewer color-map entries we wrote a lower bit depth
    * above, but the application data is still byte packed.
    */
   if (colormap != 0 && image->colormap_entries <= 16)
      ci_set_packing(ci_ptr);

   /* That should have handled all (both) the transforms. */
   if ((format & ~(ci_uint_32)(CI_FORMAT_FLAG_COLOR | CI_FORMAT_FLAG_LINEAR |
         CI_FORMAT_FLAG_ALPHA | CI_FORMAT_FLAG_COLORMAP)) != 0)
      ci_error(ci_ptr, "ci_write_image: unsupported transformation");

   {
      ci_const_bytep row = ci_voidcast(ci_const_bytep, display->buffer);
      ptrdiff_t row_bytes = display->row_stride;

      if (linear != 0)
         row_bytes *= (sizeof (ci_uint_16));

      if (row_bytes < 0)
         row += (image->height-1) * (-row_bytes);

      display->first_row = row;
      display->row_bytes = row_bytes;
   }

   /* Apply 'fast' options if the flag is set. */
   if ((image->flags & CI_IMAGE_FLAG_FAST) != 0)
   {
      ci_set_filter(ci_ptr, CI_FILTER_TYPE_BASE, CI_NO_FILTERS);
      /* NOTE: determined by experiment using cistest, this reflects some
       * balance between the time to write the image once and the time to read
       * it about 50 times.  The speed-up in cistest was about 10-20% of the
       * total (user) time on a heavily loaded system.
       */
#   ifdef CI_WRITE_CUSTOMIZE_COMPRESSION_SUPPORTED
      ci_set_compression_level(ci_ptr, 3);
#   endif
   }

   /* Check for the cases that currently require a pre-transform on the row
    * before it is written.  This only applies when the input is 16-bit and
    * either there is an alpha channel or it is converted to 8-bit.
    */
   if ((linear != 0 && alpha != 0 ) ||
       (colormap == 0 && display->convert_to_8bit != 0))
   {
      ci_bytep row = ci_voidcast(ci_bytep, ci_malloc(ci_ptr,
          ci_get_rowbytes(ci_ptr, info_ptr)));
      int result;

      display->local_row = row;
      if (write_16bit != 0)
         result = ci_safe_execute(image, ci_write_image_16bit, display);
      else
         result = ci_safe_execute(image, ci_write_image_8bit, display);
      display->local_row = NULL;

      ci_free(ci_ptr, row);

      /* Skip the 'write_end' on error: */
      if (result == 0)
         return 0;
   }

   /* Otherwise this is the case where the input is in a format currently
    * supported by the rest of the libci write code; call it directly.
    */
   else
   {
      ci_const_bytep row = ci_voidcast(ci_const_bytep, display->first_row);
      ptrdiff_t row_bytes = display->row_bytes;
      ci_uint_32 y = image->height;

      for (; y > 0; --y)
      {
         ci_write_row(ci_ptr, row);
         row += row_bytes;
      }
   }

   ci_write_end(ci_ptr, info_ptr);
   return 1;
}


static void (CICBAPI
image_memory_write)(ci_structp ci_ptr, ci_bytep/*const*/ data, size_t size)
{
   ci_image_write_control *display = ci_voidcast(ci_image_write_control*,
       ci_ptr->io_ptr/*backdoor: ci_get_io_ptr(ci_ptr)*/);
   ci_alloc_size_t ob = display->output_bytes;

   /* Check for overflow; this should never happen: */
   if (size <= ((ci_alloc_size_t)-1) - ob)
   {
      /* I don't think libci ever does this, but just in case: */
      if (size > 0)
      {
         if (display->memory_bytes >= ob+size) /* writing */
            memcpy(display->memory+ob, data, size);

         /* Always update the size: */
         display->output_bytes = ob+size;
      }
   }

   else
      ci_error(ci_ptr, "ci_image_write_to_memory: CI too big");
}

static void (CICBAPI
image_memory_flush)(ci_structp ci_ptr)
{
   CI_UNUSED(ci_ptr)
}

static int
ci_image_write_memory(ci_voidp argument)
{
   ci_image_write_control *display = ci_voidcast(ci_image_write_control*,
       argument);

   /* The rest of the memory-specific init and write_main in an error protected
    * environment.  This case needs to use callbacks for the write operations
    * since libci has no built in support for writing to memory.
    */
   ci_set_write_fn(display->image->opaque->ci_ptr, display/*io_ptr*/,
       image_memory_write, image_memory_flush);

   return ci_image_write_main(display);
}

int CIAPI
ci_image_write_to_memory(ci_imagep image, void *memory,
    ci_alloc_size_t * CI_RESTRICT memory_bytes, int convert_to_8bit,
    const void *buffer, ci_int_32 row_stride, const void *colormap)
{
   /* Write the image to the given buffer, or count the bytes if it is NULL */
   if (image != NULL && image->version == CI_IMAGE_VERSION)
   {
      if (memory_bytes != NULL && buffer != NULL)
      {
         /* This is to give the caller an easier error detection in the NULL
          * case and guard against uninitialized variable problems:
          */
         if (memory == NULL)
            *memory_bytes = 0;

         if (ci_image_write_init(image) != 0)
         {
            ci_image_write_control display;
            int result;

            memset(&display, 0, (sizeof display));
            display.image = image;
            display.buffer = buffer;
            display.row_stride = row_stride;
            display.colormap = colormap;
            display.convert_to_8bit = convert_to_8bit;
            display.memory = ci_voidcast(ci_bytep, memory);
            display.memory_bytes = *memory_bytes;
            display.output_bytes = 0;

            result = ci_safe_execute(image, ci_image_write_memory, &display);
            ci_image_free(image);

            /* write_memory returns true even if we ran out of buffer. */
            if (result)
            {
               /* On out-of-buffer this function returns '0' but still updates
                * memory_bytes:
                */
               if (memory != NULL && display.output_bytes > *memory_bytes)
                  result = 0;

               *memory_bytes = display.output_bytes;
            }

            return result;
         }

         else
            return 0;
      }

      else
         return ci_image_error(image,
             "ci_image_write_to_memory: invalid argument");
   }

   else if (image != NULL)
      return ci_image_error(image,
          "ci_image_write_to_memory: incorrect CI_IMAGE_VERSION");

   else
      return 0;
}

#ifdef CI_SIMPLIFIED_WRITE_STDIO_SUPPORTED
int CIAPI
ci_image_write_to_stdio(ci_imagep image, FILE *file, int convert_to_8bit,
    const void *buffer, ci_int_32 row_stride, const void *colormap)
{
   /* Write the image to the given FILE object. */
   if (image != NULL && image->version == CI_IMAGE_VERSION)
   {
      if (file != NULL && buffer != NULL)
      {
         if (ci_image_write_init(image) != 0)
         {
            ci_image_write_control display;
            int result;

            /* This is slightly evil, but ci_init_io doesn't do anything other
             * than this and we haven't changed the standard IO functions so
             * this saves a 'safe' function.
             */
            image->opaque->ci_ptr->io_ptr = file;

            memset(&display, 0, (sizeof display));
            display.image = image;
            display.buffer = buffer;
            display.row_stride = row_stride;
            display.colormap = colormap;
            display.convert_to_8bit = convert_to_8bit;

            result = ci_safe_execute(image, ci_image_write_main, &display);
            ci_image_free(image);
            return result;
         }

         else
            return 0;
      }

      else
         return ci_image_error(image,
             "ci_image_write_to_stdio: invalid argument");
   }

   else if (image != NULL)
      return ci_image_error(image,
          "ci_image_write_to_stdio: incorrect CI_IMAGE_VERSION");

   else
      return 0;
}

int CIAPI
ci_image_write_to_file(ci_imagep image, const char *file_name,
    int convert_to_8bit, const void *buffer, ci_int_32 row_stride,
    const void *colormap)
{
   /* Write the image to the named file. */
   if (image != NULL && image->version == CI_IMAGE_VERSION)
   {
      if (file_name != NULL && buffer != NULL)
      {
         FILE *fp = fopen(file_name, "wb");

         if (fp != NULL)
         {
            if (ci_image_write_to_stdio(image, fp, convert_to_8bit, buffer,
                row_stride, colormap) != 0)
            {
               int error; /* from fflush/fclose */

               /* Make sure the file is flushed correctly. */
               if (fflush(fp) == 0 && ferror(fp) == 0)
               {
                  if (fclose(fp) == 0)
                     return 1;

                  error = errno; /* from fclose */
               }

               else
               {
                  error = errno; /* from fflush or ferror */
                  (void)fclose(fp);
               }

               (void)remove(file_name);
               /* The image has already been cleaned up; this is just used to
                * set the error (because the original write succeeded).
                */
               return ci_image_error(image, strerror(error));
            }

            else
            {
               /* Clean up: just the opened file. */
               (void)fclose(fp);
               (void)remove(file_name);
               return 0;
            }
         }

         else
            return ci_image_error(image, strerror(errno));
      }

      else
         return ci_image_error(image,
             "ci_image_write_to_file: invalid argument");
   }

   else if (image != NULL)
      return ci_image_error(image,
          "ci_image_write_to_file: incorrect CI_IMAGE_VERSION");

   else
      return 0;
}
#endif /* SIMPLIFIED_WRITE_STDIO */
#endif /* SIMPLIFIED_WRITE */
#endif /* WRITE */

/* ciset.c - storage of image information into info struct
 *
 * Copyright (c) 2018-2025 Cosmin Truta
 * Copyright (c) 1998-2018 Glenn Randers-Pehrson
 * Copyright (c) 1996-1997 Andreas Dilger
 * Copyright (c) 1995-1996 Guy Eric Schalnat, Group 42, Inc.
 *
 * This code is released under the libci license.
 * For conditions of distribution and use, see the disclaimer
 * and license in ci.h
 *
 * The functions here are used during reads to store data from the file
 * into the info struct, and during writes to store application data
 * into the info struct for writing into the file.  This abstracts the
 * info struct and allows us to change the structure in the future.
 */

#include "cipriv.h"

#if defined(CI_READ_SUPPORTED) || defined(CI_WRITE_SUPPORTED)

#ifdef CI_bKGD_SUPPORTED
void CIAPI
ci_set_bKGD(ci_const_structrp ci_ptr, ci_inforp info_ptr,
    ci_const_color_16p background)
{
   ci_debug1(1, "in %s storage function", "bKGD");

   if (ci_ptr == NULL || info_ptr == NULL || background == NULL)
      return;

   info_ptr->background = *background;
   info_ptr->valid |= CI_INFO_bKGD;
}
#endif

#ifdef CI_cHRM_SUPPORTED
void CIFAPI
ci_set_cHRM_fixed(ci_const_structrp ci_ptr, ci_inforp info_ptr,
    ci_fixed_point white_x, ci_fixed_point white_y, ci_fixed_point red_x,
    ci_fixed_point red_y, ci_fixed_point green_x, ci_fixed_point green_y,
    ci_fixed_point blue_x, ci_fixed_point blue_y)
{
   ci_debug1(1, "in %s storage function", "cHRM fixed");

   if (ci_ptr == NULL || info_ptr == NULL)
      return;

   info_ptr->cHRM.redx = red_x;
   info_ptr->cHRM.redy = red_y;
   info_ptr->cHRM.greenx = green_x;
   info_ptr->cHRM.greeny = green_y;
   info_ptr->cHRM.bluex = blue_x;
   info_ptr->cHRM.bluey = blue_y;
   info_ptr->cHRM.whitex = white_x;
   info_ptr->cHRM.whitey = white_y;

   info_ptr->valid |= CI_INFO_cHRM;
}

void CIFAPI
ci_set_cHRM_XYZ_fixed(ci_const_structrp ci_ptr, ci_inforp info_ptr,
    ci_fixed_point int_red_X, ci_fixed_point int_red_Y,
    ci_fixed_point int_red_Z, ci_fixed_point int_green_X,
    ci_fixed_point int_green_Y, ci_fixed_point int_green_Z,
    ci_fixed_point int_blue_X, ci_fixed_point int_blue_Y,
    ci_fixed_point int_blue_Z)
{
   ci_XYZ XYZ;
   ci_xy xy;

   ci_debug1(1, "in %s storage function", "cHRM XYZ fixed");

   if (ci_ptr == NULL || info_ptr == NULL)
      return;

   XYZ.red_X = int_red_X;
   XYZ.red_Y = int_red_Y;
   XYZ.red_Z = int_red_Z;
   XYZ.green_X = int_green_X;
   XYZ.green_Y = int_green_Y;
   XYZ.green_Z = int_green_Z;
   XYZ.blue_X = int_blue_X;
   XYZ.blue_Y = int_blue_Y;
   XYZ.blue_Z = int_blue_Z;

   if (ci_xy_from_XYZ(&xy, &XYZ) == 0)
   {
      info_ptr->cHRM = xy;
      info_ptr->valid |= CI_INFO_cHRM;
   }

   else
      ci_app_error(ci_ptr, "invalid cHRM XYZ");
}

#  ifdef CI_FLOATING_POINT_SUPPORTED
void CIAPI
ci_set_cHRM(ci_const_structrp ci_ptr, ci_inforp info_ptr,
    double white_x, double white_y, double red_x, double red_y,
    double green_x, double green_y, double blue_x, double blue_y)
{
   ci_set_cHRM_fixed(ci_ptr, info_ptr,
       ci_fixed(ci_ptr, white_x, "cHRM White X"),
       ci_fixed(ci_ptr, white_y, "cHRM White Y"),
       ci_fixed(ci_ptr, red_x, "cHRM Red X"),
       ci_fixed(ci_ptr, red_y, "cHRM Red Y"),
       ci_fixed(ci_ptr, green_x, "cHRM Green X"),
       ci_fixed(ci_ptr, green_y, "cHRM Green Y"),
       ci_fixed(ci_ptr, blue_x, "cHRM Blue X"),
       ci_fixed(ci_ptr, blue_y, "cHRM Blue Y"));
}

void CIAPI
ci_set_cHRM_XYZ(ci_const_structrp ci_ptr, ci_inforp info_ptr, double red_X,
    double red_Y, double red_Z, double green_X, double green_Y, double green_Z,
    double blue_X, double blue_Y, double blue_Z)
{
   ci_set_cHRM_XYZ_fixed(ci_ptr, info_ptr,
       ci_fixed(ci_ptr, red_X, "cHRM Red X"),
       ci_fixed(ci_ptr, red_Y, "cHRM Red Y"),
       ci_fixed(ci_ptr, red_Z, "cHRM Red Z"),
       ci_fixed(ci_ptr, green_X, "cHRM Green X"),
       ci_fixed(ci_ptr, green_Y, "cHRM Green Y"),
       ci_fixed(ci_ptr, green_Z, "cHRM Green Z"),
       ci_fixed(ci_ptr, blue_X, "cHRM Blue X"),
       ci_fixed(ci_ptr, blue_Y, "cHRM Blue Y"),
       ci_fixed(ci_ptr, blue_Z, "cHRM Blue Z"));
}
#  endif /* FLOATING_POINT */

#endif /* cHRM */

#ifdef CI_cICP_SUPPORTED
void CIAPI
ci_set_cICP(ci_const_structrp ci_ptr, ci_inforp info_ptr,
             ci_byte colour_primaries, ci_byte transfer_function,
             ci_byte matrix_coefficients, ci_byte video_full_range_flag)
{
   ci_debug1(1, "in %s storage function", "cICP");

   if (ci_ptr == NULL || info_ptr == NULL)
      return;

   info_ptr->cicp_colour_primaries = colour_primaries;
   info_ptr->cicp_transfer_function = transfer_function;
   info_ptr->cicp_matrix_coefficients = matrix_coefficients;
   info_ptr->cicp_video_full_range_flag = video_full_range_flag;

   if (info_ptr->cicp_matrix_coefficients != 0)
   {
      ci_warning(ci_ptr, "Invalid cICP matrix coefficients");
      return;
   }

   info_ptr->valid |= CI_INFO_cICP;
}
#endif /* cICP */

#ifdef CI_cLLI_SUPPORTED
void CIFAPI
ci_set_cLLI_fixed(ci_const_structrp ci_ptr, ci_inforp info_ptr,
    /* The values below are in cd/m2 (nits) and are scaled by 10,000; not
     * 100,000 as in the case of ci_fixed_point.
     */
    ci_uint_32 maxCLL, ci_uint_32 maxFALL)
{
   ci_debug1(1, "in %s storage function", "cLLI");

   if (ci_ptr == NULL || info_ptr == NULL)
      return;

   /* Check the light level range: */
   if (maxCLL > 0x7FFFFFFFU || maxFALL > 0x7FFFFFFFU)
   {
      /* The limit is 200kcd/m2; somewhat bright but not inconceivable because
       * human vision is said to run up to 100Mcd/m2.  The sun is about 2Gcd/m2.
       *
       * The reference sRGB monitor is 80cd/m2 and the limit of PQ encoding is
       * 2kcd/m2.
       */
      ci_chunk_report(ci_ptr, "cLLI light level exceeds CI limit",
            CI_CHUNK_WRITE_ERROR);
      return;
   }

   info_ptr->maxCLL = maxCLL;
   info_ptr->maxFALL = maxFALL;
   info_ptr->valid |= CI_INFO_cLLI;
}

#  ifdef CI_FLOATING_POINT_SUPPORTED
void CIAPI
ci_set_cLLI(ci_const_structrp ci_ptr, ci_inforp info_ptr,
   double maxCLL, double maxFALL)
{
   ci_set_cLLI_fixed(ci_ptr, info_ptr,
       ci_fixed_ITU(ci_ptr, maxCLL, "ci_set_cLLI(maxCLL)"),
       ci_fixed_ITU(ci_ptr, maxFALL, "ci_set_cLLI(maxFALL)"));
}
#  endif /* FLOATING_POINT */
#endif /* cLLI */

#ifdef CI_mDCV_SUPPORTED
static ci_uint_16
ci_ITU_fixed_16(int *error, ci_fixed_point v)
{
   /* Return a safe uint16_t value scaled according to the ITU H273 rules for
    * 16-bit display chromaticities.  Functions like the corresponding
    * ci_fixed() internal function with regard to errors: it's an error on
    * write, a chunk_benign_error on read: See the definition of
    * ci_chunk_report in cipriv.h.
    */
   v /= 2; /* rounds to 0 in C: avoids insignificant arithmetic errors */
   if (v > 65535 || v < 0)
   {
      *error = 1;
      return 0;
   }

   return (ci_uint_16)/*SAFE*/v;
}

void CIAPI
ci_set_mDCV_fixed(ci_const_structrp ci_ptr, ci_inforp info_ptr,
    ci_fixed_point white_x, ci_fixed_point white_y,
    ci_fixed_point red_x, ci_fixed_point red_y,
    ci_fixed_point green_x, ci_fixed_point green_y,
    ci_fixed_point blue_x, ci_fixed_point blue_y,
    ci_uint_32 maxDL,
    ci_uint_32 minDL)
{
   ci_uint_16 rx, ry, gx, gy, bx, by, wx, wy;
   int error;

   ci_debug1(1, "in %s storage function", "mDCV");

   if (ci_ptr == NULL || info_ptr == NULL)
      return;

   /* Check the input values to ensure they are in the expected range: */
   error = 0;
   rx = ci_ITU_fixed_16(&error, red_x);
   ry = ci_ITU_fixed_16(&error, red_y);
   gx = ci_ITU_fixed_16(&error, green_x);
   gy = ci_ITU_fixed_16(&error, green_y);
   bx = ci_ITU_fixed_16(&error, blue_x);
   by = ci_ITU_fixed_16(&error, blue_y);
   wx = ci_ITU_fixed_16(&error, white_x);
   wy = ci_ITU_fixed_16(&error, white_y);

   if (error)
   {
      ci_chunk_report(ci_ptr,
         "mDCV chromaticities outside representable range",
         CI_CHUNK_WRITE_ERROR);
      return;
   }

   /* Check the light level range: */
   if (maxDL > 0x7FFFFFFFU || minDL > 0x7FFFFFFFU)
   {
      /* The limit is 200kcd/m2; somewhat bright but not inconceivable because
       * human vision is said to run up to 100Mcd/m2.  The sun is about 2Gcd/m2.
       *
       * The reference sRGB monitor is 80cd/m2 and the limit of PQ encoding is
       * 2kcd/m2.
       */
      ci_chunk_report(ci_ptr, "mDCV display light level exceeds CI limit",
            CI_CHUNK_WRITE_ERROR);
      return;
   }

   /* All values are safe, the settings are accepted.
    *
    * IMPLEMENTATION NOTE: in practice the values can be checked and assigned
    * but the result is confusing if a writing app calls ci_set_mDCV more than
    * once, the second time with an invalid value.  This approach is more
    * obviously correct at the cost of typing and a very slight machine
    * overhead.
    */
   info_ptr->mastering_red_x = rx;
   info_ptr->mastering_red_y = ry;
   info_ptr->mastering_green_x = gx;
   info_ptr->mastering_green_y = gy;
   info_ptr->mastering_blue_x = bx;
   info_ptr->mastering_blue_y = by;
   info_ptr->mastering_white_x = wx;
   info_ptr->mastering_white_y = wy;
   info_ptr->mastering_maxDL = maxDL;
   info_ptr->mastering_minDL = minDL;
   info_ptr->valid |= CI_INFO_mDCV;
}

#  ifdef CI_FLOATING_POINT_SUPPORTED
void CIAPI
ci_set_mDCV(ci_const_structrp ci_ptr, ci_inforp info_ptr,
    double white_x, double white_y, double red_x, double red_y, double green_x,
    double green_y, double blue_x, double blue_y,
    double maxDL, double minDL)
{
   ci_set_mDCV_fixed(ci_ptr, info_ptr,
      ci_fixed(ci_ptr, white_x, "ci_set_mDCV(white(x))"),
      ci_fixed(ci_ptr, white_y, "ci_set_mDCV(white(y))"),
      ci_fixed(ci_ptr, red_x, "ci_set_mDCV(red(x))"),
      ci_fixed(ci_ptr, red_y, "ci_set_mDCV(red(y))"),
      ci_fixed(ci_ptr, green_x, "ci_set_mDCV(green(x))"),
      ci_fixed(ci_ptr, green_y, "ci_set_mDCV(green(y))"),
      ci_fixed(ci_ptr, blue_x, "ci_set_mDCV(blue(x))"),
      ci_fixed(ci_ptr, blue_y, "ci_set_mDCV(blue(y))"),
      ci_fixed_ITU(ci_ptr, maxDL, "ci_set_mDCV(maxDL)"),
      ci_fixed_ITU(ci_ptr, minDL, "ci_set_mDCV(minDL)"));
}
#  endif /* FLOATING_POINT */
#endif /* mDCV */

#ifdef CI_eXIf_SUPPORTED
void CIAPI
ci_set_eXIf(ci_const_structrp ci_ptr, ci_inforp info_ptr,
    ci_bytep exif)
{
  ci_warning(ci_ptr, "ci_set_eXIf does not work; use ci_set_eXIf_1");
  CI_UNUSED(info_ptr)
  CI_UNUSED(exif)
}

void CIAPI
ci_set_eXIf_1(ci_const_structrp ci_ptr, ci_inforp info_ptr,
    ci_uint_32 num_exif, ci_bytep exif)
{
   ci_bytep new_exif;

   ci_debug1(1, "in %s storage function", "eXIf");

   if (ci_ptr == NULL || info_ptr == NULL ||
       (ci_ptr->mode & CI_WROTE_eXIf) != 0)
      return;

   new_exif = ci_voidcast(ci_bytep, ci_malloc_warn(ci_ptr, num_exif));

   if (new_exif == NULL)
   {
      ci_warning(ci_ptr, "Insufficient memory for eXIf chunk data");
      return;
   }

   memcpy(new_exif, exif, (size_t)num_exif);

   ci_free_data(ci_ptr, info_ptr, CI_FREE_EXIF, 0);

   info_ptr->num_exif = num_exif;
   info_ptr->exif = new_exif;
   info_ptr->free_me |= CI_FREE_EXIF;
   info_ptr->valid |= CI_INFO_eXIf;
}
#endif /* eXIf */

#ifdef CI_gAMA_SUPPORTED
void CIFAPI
ci_set_gAMA_fixed(ci_const_structrp ci_ptr, ci_inforp info_ptr,
    ci_fixed_point file_gamma)
{
   ci_debug1(1, "in %s storage function", "gAMA");

   if (ci_ptr == NULL || info_ptr == NULL)
      return;

   info_ptr->gamma = file_gamma;
   info_ptr->valid |= CI_INFO_gAMA;
}

#  ifdef CI_FLOATING_POINT_SUPPORTED
void CIAPI
ci_set_gAMA(ci_const_structrp ci_ptr, ci_inforp info_ptr, double file_gamma)
{
   ci_set_gAMA_fixed(ci_ptr, info_ptr, ci_fixed(ci_ptr, file_gamma,
       "ci_set_gAMA"));
}
#  endif
#endif

#ifdef CI_hIST_SUPPORTED
void CIAPI
ci_set_hIST(ci_const_structrp ci_ptr, ci_inforp info_ptr,
    ci_const_uint_16p hist)
{
   int i;

   ci_debug1(1, "in %s storage function", "hIST");

   if (ci_ptr == NULL || info_ptr == NULL)
      return;

   if (info_ptr->num_palette == 0 || info_ptr->num_palette
       > CI_MAX_PALETTE_LENGTH)
   {
      ci_warning(ci_ptr,
          "Invalid palette size, hIST allocation skipped");

      return;
   }

   ci_free_data(ci_ptr, info_ptr, CI_FREE_HIST, 0);

   /* Changed from info->num_palette to CI_MAX_PALETTE_LENGTH in
    * version 1.2.1
    */
   info_ptr->hist = ci_voidcast(ci_uint_16p, ci_malloc_warn(ci_ptr,
       CI_MAX_PALETTE_LENGTH * (sizeof (ci_uint_16))));

   if (info_ptr->hist == NULL)
   {
      ci_warning(ci_ptr, "Insufficient memory for hIST chunk data");
      return;
   }

   for (i = 0; i < info_ptr->num_palette; i++)
      info_ptr->hist[i] = hist[i];

   info_ptr->free_me |= CI_FREE_HIST;
   info_ptr->valid |= CI_INFO_hIST;
}
#endif

void CIAPI
ci_set_IHDR(ci_const_structrp ci_ptr, ci_inforp info_ptr,
    ci_uint_32 width, ci_uint_32 height, int bit_depth,
    int color_type, int interlace_type, int compression_type,
    int filter_type)
{
   ci_debug1(1, "in %s storage function", "IHDR");

   if (ci_ptr == NULL || info_ptr == NULL)
      return;

   info_ptr->width = width;
   info_ptr->height = height;
   info_ptr->bit_depth = (ci_byte)bit_depth;
   info_ptr->color_type = (ci_byte)color_type;
   info_ptr->compression_type = (ci_byte)compression_type;
   info_ptr->filter_type = (ci_byte)filter_type;
   info_ptr->interlace_type = (ci_byte)interlace_type;

   ci_check_IHDR (ci_ptr, info_ptr->width, info_ptr->height,
       info_ptr->bit_depth, info_ptr->color_type, info_ptr->interlace_type,
       info_ptr->compression_type, info_ptr->filter_type);

   if (info_ptr->color_type == CI_COLOR_TYPE_PALETTE)
      info_ptr->channels = 1;

   else if ((info_ptr->color_type & CI_COLOR_MASK_COLOR) != 0)
      info_ptr->channels = 3;

   else
      info_ptr->channels = 1;

   if ((info_ptr->color_type & CI_COLOR_MASK_ALPHA) != 0)
      info_ptr->channels++;

   info_ptr->pixel_depth = (ci_byte)(info_ptr->channels * info_ptr->bit_depth);

   info_ptr->rowbytes = CI_ROWBYTES(info_ptr->pixel_depth, width);
}

#ifdef CI_oFFs_SUPPORTED
void CIAPI
ci_set_oFFs(ci_const_structrp ci_ptr, ci_inforp info_ptr,
    ci_int_32 offset_x, ci_int_32 offset_y, int unit_type)
{
   ci_debug1(1, "in %s storage function", "oFFs");

   if (ci_ptr == NULL || info_ptr == NULL)
      return;

   info_ptr->x_offset = offset_x;
   info_ptr->y_offset = offset_y;
   info_ptr->offset_unit_type = (ci_byte)unit_type;
   info_ptr->valid |= CI_INFO_oFFs;
}
#endif

#ifdef CI_pCAL_SUPPORTED
void CIAPI
ci_set_pCAL(ci_const_structrp ci_ptr, ci_inforp info_ptr,
    ci_const_charp purpose, ci_int_32 X0, ci_int_32 X1, int type,
    int nparams, ci_const_charp units, ci_charpp params)
{
   size_t length;
   int i;

   ci_debug1(1, "in %s storage function", "pCAL");

   if (ci_ptr == NULL || info_ptr == NULL || purpose == NULL || units == NULL
       || (nparams > 0 && params == NULL))
      return;

   length = strlen(purpose) + 1;
   ci_debug1(3, "allocating purpose for info (%lu bytes)",
       (unsigned long)length);

   /* TODO: validate format of calibration name and unit name */

   /* Check that the type matches the specification. */
   if (type < 0 || type > 3)
   {
      ci_chunk_report(ci_ptr, "Invalid pCAL equation type",
            CI_CHUNK_WRITE_ERROR);
      return;
   }

   if (nparams < 0 || nparams > 255)
   {
      ci_chunk_report(ci_ptr, "Invalid pCAL parameter count",
            CI_CHUNK_WRITE_ERROR);
      return;
   }

   /* Validate params[nparams] */
   for (i=0; i<nparams; ++i)
   {
      if (params[i] == NULL ||
          !ci_check_fp_string(params[i], strlen(params[i])))
      {
         ci_chunk_report(ci_ptr, "Invalid format for pCAL parameter",
               CI_CHUNK_WRITE_ERROR);
         return;
      }
   }

   info_ptr->pcal_purpose = ci_voidcast(ci_charp,
       ci_malloc_warn(ci_ptr, length));

   if (info_ptr->pcal_purpose == NULL)
   {
      ci_chunk_report(ci_ptr, "Insufficient memory for pCAL purpose",
            CI_CHUNK_WRITE_ERROR);
      return;
   }

   memcpy(info_ptr->pcal_purpose, purpose, length);

   info_ptr->free_me |= CI_FREE_PCAL;

   ci_debug(3, "storing X0, X1, type, and nparams in info");
   info_ptr->pcal_X0 = X0;
   info_ptr->pcal_X1 = X1;
   info_ptr->pcal_type = (ci_byte)type;
   info_ptr->pcal_nparams = (ci_byte)nparams;

   length = strlen(units) + 1;
   ci_debug1(3, "allocating units for info (%lu bytes)",
       (unsigned long)length);

   info_ptr->pcal_units = ci_voidcast(ci_charp,
       ci_malloc_warn(ci_ptr, length));

   if (info_ptr->pcal_units == NULL)
   {
      ci_warning(ci_ptr, "Insufficient memory for pCAL units");
      return;
   }

   memcpy(info_ptr->pcal_units, units, length);

   info_ptr->pcal_params = ci_voidcast(ci_charpp, ci_malloc_warn(ci_ptr,
       (size_t)(((unsigned int)nparams + 1) * (sizeof (ci_charp)))));

   if (info_ptr->pcal_params == NULL)
   {
      ci_warning(ci_ptr, "Insufficient memory for pCAL params");
      return;
   }

   memset(info_ptr->pcal_params, 0, ((unsigned int)nparams + 1) *
       (sizeof (ci_charp)));

   for (i = 0; i < nparams; i++)
   {
      length = strlen(params[i]) + 1;
      ci_debug2(3, "allocating parameter %d for info (%lu bytes)", i,
          (unsigned long)length);

      info_ptr->pcal_params[i] = (ci_charp)ci_malloc_warn(ci_ptr, length);

      if (info_ptr->pcal_params[i] == NULL)
      {
         ci_warning(ci_ptr, "Insufficient memory for pCAL parameter");
         return;
      }

      memcpy(info_ptr->pcal_params[i], params[i], length);
   }

   info_ptr->valid |= CI_INFO_pCAL;
}
#endif

#ifdef CI_sCAL_SUPPORTED
void CIAPI
ci_set_sCAL_s(ci_const_structrp ci_ptr, ci_inforp info_ptr,
    int unit, ci_const_charp swidth, ci_const_charp sheight)
{
   size_t lengthw = 0, lengthh = 0;

   ci_debug1(1, "in %s storage function", "sCAL");

   if (ci_ptr == NULL || info_ptr == NULL)
      return;

   /* Double check the unit (should never get here with an invalid
    * unit unless this is an API call.)
    */
   if (unit != 1 && unit != 2)
      ci_error(ci_ptr, "Invalid sCAL unit");

   if (swidth == NULL || (lengthw = strlen(swidth)) == 0 ||
       swidth[0] == 45 /* '-' */ || !ci_check_fp_string(swidth, lengthw))
      ci_error(ci_ptr, "Invalid sCAL width");

   if (sheight == NULL || (lengthh = strlen(sheight)) == 0 ||
       sheight[0] == 45 /* '-' */ || !ci_check_fp_string(sheight, lengthh))
      ci_error(ci_ptr, "Invalid sCAL height");

   info_ptr->scal_unit = (ci_byte)unit;

   ++lengthw;

   ci_debug1(3, "allocating unit for info (%u bytes)", (unsigned int)lengthw);

   info_ptr->scal_s_width = ci_voidcast(ci_charp,
       ci_malloc_warn(ci_ptr, lengthw));

   if (info_ptr->scal_s_width == NULL)
   {
      ci_warning(ci_ptr, "Memory allocation failed while processing sCAL");

      return;
   }

   memcpy(info_ptr->scal_s_width, swidth, lengthw);

   ++lengthh;

   ci_debug1(3, "allocating unit for info (%u bytes)", (unsigned int)lengthh);

   info_ptr->scal_s_height = ci_voidcast(ci_charp,
       ci_malloc_warn(ci_ptr, lengthh));

   if (info_ptr->scal_s_height == NULL)
   {
      ci_free(ci_ptr, info_ptr->scal_s_width);
      info_ptr->scal_s_width = NULL;

      ci_warning(ci_ptr, "Memory allocation failed while processing sCAL");
      return;
   }

   memcpy(info_ptr->scal_s_height, sheight, lengthh);

   info_ptr->free_me |= CI_FREE_SCAL;
   info_ptr->valid |= CI_INFO_sCAL;
}

#  ifdef CI_FLOATING_POINT_SUPPORTED
void CIAPI
ci_set_sCAL(ci_const_structrp ci_ptr, ci_inforp info_ptr, int unit,
    double width, double height)
{
   ci_debug1(1, "in %s storage function", "sCAL");

   /* Check the arguments. */
   if (width <= 0)
      ci_warning(ci_ptr, "Invalid sCAL width ignored");

   else if (height <= 0)
      ci_warning(ci_ptr, "Invalid sCAL height ignored");

   else
   {
      /* Convert 'width' and 'height' to ASCII. */
      char swidth[CI_sCAL_MAX_DIGITS+1];
      char sheight[CI_sCAL_MAX_DIGITS+1];

      ci_ascii_from_fp(ci_ptr, swidth, (sizeof swidth), width,
          CI_sCAL_PRECISION);
      ci_ascii_from_fp(ci_ptr, sheight, (sizeof sheight), height,
          CI_sCAL_PRECISION);

      ci_set_sCAL_s(ci_ptr, info_ptr, unit, swidth, sheight);
   }
}
#  endif

#  ifdef CI_FIXED_POINT_SUPPORTED
void CIAPI
ci_set_sCAL_fixed(ci_const_structrp ci_ptr, ci_inforp info_ptr, int unit,
    ci_fixed_point width, ci_fixed_point height)
{
   ci_debug1(1, "in %s storage function", "sCAL");

   /* Check the arguments. */
   if (width <= 0)
      ci_warning(ci_ptr, "Invalid sCAL width ignored");

   else if (height <= 0)
      ci_warning(ci_ptr, "Invalid sCAL height ignored");

   else
   {
      /* Convert 'width' and 'height' to ASCII. */
      char swidth[CI_sCAL_MAX_DIGITS+1];
      char sheight[CI_sCAL_MAX_DIGITS+1];

      ci_ascii_from_fixed(ci_ptr, swidth, (sizeof swidth), width);
      ci_ascii_from_fixed(ci_ptr, sheight, (sizeof sheight), height);

      ci_set_sCAL_s(ci_ptr, info_ptr, unit, swidth, sheight);
   }
}
#  endif
#endif

#ifdef CI_pHYs_SUPPORTED
void CIAPI
ci_set_pHYs(ci_const_structrp ci_ptr, ci_inforp info_ptr,
    ci_uint_32 res_x, ci_uint_32 res_y, int unit_type)
{
   ci_debug1(1, "in %s storage function", "pHYs");

   if (ci_ptr == NULL || info_ptr == NULL)
      return;

   info_ptr->x_pixels_per_unit = res_x;
   info_ptr->y_pixels_per_unit = res_y;
   info_ptr->phys_unit_type = (ci_byte)unit_type;
   info_ptr->valid |= CI_INFO_pHYs;
}
#endif

void CIAPI
ci_set_PLTE(ci_structrp ci_ptr, ci_inforp info_ptr,
    ci_const_colorp palette, int num_palette)
{

   ci_uint_32 max_palette_length;

   ci_debug1(1, "in %s storage function", "PLTE");

   if (ci_ptr == NULL || info_ptr == NULL)
      return;

   max_palette_length = (info_ptr->color_type == CI_COLOR_TYPE_PALETTE) ?
      (1 << info_ptr->bit_depth) : CI_MAX_PALETTE_LENGTH;

   if (num_palette < 0 || num_palette > (int) max_palette_length)
   {
      if (info_ptr->color_type == CI_COLOR_TYPE_PALETTE)
         ci_error(ci_ptr, "Invalid palette length");

      else
      {
         ci_warning(ci_ptr, "Invalid palette length");

         return;
      }
   }

   if ((num_palette > 0 && palette == NULL) ||
      (num_palette == 0
#        ifdef CI_MNG_FEATURES_SUPPORTED
            && (ci_ptr->mng_features_permitted & CI_FLAG_MNG_EMPTY_PLTE) == 0
#        endif
      ))
   {
      ci_error(ci_ptr, "Invalid palette");
   }

   /* It may not actually be necessary to set ci_ptr->palette here;
    * we do it for backward compatibility with the way the ci_handle_tRNS
    * function used to do the allocation.
    *
    * 1.6.0: the above statement appears to be incorrect; something has to set
    * the palette inside ci_struct on read.
    */
   ci_free_data(ci_ptr, info_ptr, CI_FREE_PLTE, 0);

   /* Changed in libci-1.2.1 to allocate CI_MAX_PALETTE_LENGTH instead
    * of num_palette entries, in case of an invalid CI file or incorrect
    * call to ci_set_PLTE() with too-large sample values.
    */
   ci_ptr->palette = ci_voidcast(ci_colorp, ci_calloc(ci_ptr,
       CI_MAX_PALETTE_LENGTH * (sizeof (ci_color))));

   if (num_palette > 0)
      memcpy(ci_ptr->palette, palette, (unsigned int)num_palette *
          (sizeof (ci_color)));

   info_ptr->palette = ci_ptr->palette;
   info_ptr->num_palette = ci_ptr->num_palette = (ci_uint_16)num_palette;
   info_ptr->free_me |= CI_FREE_PLTE;
   info_ptr->valid |= CI_INFO_PLTE;
}

#ifdef CI_sBIT_SUPPORTED
void CIAPI
ci_set_sBIT(ci_const_structrp ci_ptr, ci_inforp info_ptr,
    ci_const_color_8p sig_bit)
{
   ci_debug1(1, "in %s storage function", "sBIT");

   if (ci_ptr == NULL || info_ptr == NULL || sig_bit == NULL)
      return;

   info_ptr->sig_bit = *sig_bit;
   info_ptr->valid |= CI_INFO_sBIT;
}
#endif

#ifdef CI_sRGB_SUPPORTED
void CIAPI
ci_set_sRGB(ci_const_structrp ci_ptr, ci_inforp info_ptr, int srgb_intent)
{
   ci_debug1(1, "in %s storage function", "sRGB");

   if (ci_ptr == NULL || info_ptr == NULL)
      return;

   info_ptr->rendering_intent = srgb_intent;
   info_ptr->valid |= CI_INFO_sRGB;
}

void CIAPI
ci_set_sRGB_gAMA_and_cHRM(ci_const_structrp ci_ptr, ci_inforp info_ptr,
    int srgb_intent)
{
   ci_debug1(1, "in %s storage function", "sRGB_gAMA_and_cHRM");

   if (ci_ptr == NULL || info_ptr == NULL)
      return;

   ci_set_sRGB(ci_ptr, info_ptr, srgb_intent);

#  ifdef CI_gAMA_SUPPORTED
      ci_set_gAMA_fixed(ci_ptr, info_ptr, CI_GAMMA_sRGB_INVERSE);
#  endif /* gAMA */

#  ifdef CI_cHRM_SUPPORTED
      ci_set_cHRM_fixed(ci_ptr, info_ptr,
         /* color      x       y */
         /* white */ 31270, 32900,
         /* red   */ 64000, 33000,
         /* green */ 30000, 60000,
         /* blue  */ 15000,  6000);
#  endif /* cHRM */
}
#endif /* sRGB */


#ifdef CI_iCCP_SUPPORTED
void CIAPI
ci_set_iCCP(ci_const_structrp ci_ptr, ci_inforp info_ptr,
    ci_const_charp name, int compression_type,
    ci_const_bytep profile, ci_uint_32 proflen)
{
   ci_charp new_iccp_name;
   ci_bytep new_iccp_profile;
   size_t length;

   ci_debug1(1, "in %s storage function", "iCCP");

   if (ci_ptr == NULL || info_ptr == NULL || name == NULL || profile == NULL)
      return;

   if (compression_type != CI_COMPRESSION_TYPE_BASE)
      ci_app_error(ci_ptr, "Invalid iCCP compression method");

   length = strlen(name)+1;
   new_iccp_name = ci_voidcast(ci_charp, ci_malloc_warn(ci_ptr, length));

   if (new_iccp_name == NULL)
   {
      ci_benign_error(ci_ptr, "Insufficient memory to process iCCP chunk");

      return;
   }

   memcpy(new_iccp_name, name, length);
   new_iccp_profile = ci_voidcast(ci_bytep,
       ci_malloc_warn(ci_ptr, proflen));

   if (new_iccp_profile == NULL)
   {
      ci_free(ci_ptr, new_iccp_name);
      ci_benign_error(ci_ptr,
          "Insufficient memory to process iCCP profile");

      return;
   }

   memcpy(new_iccp_profile, profile, proflen);

   ci_free_data(ci_ptr, info_ptr, CI_FREE_ICCP, 0);

   info_ptr->iccp_proflen = proflen;
   info_ptr->iccp_name = new_iccp_name;
   info_ptr->iccp_profile = new_iccp_profile;
   info_ptr->free_me |= CI_FREE_ICCP;
   info_ptr->valid |= CI_INFO_iCCP;
}
#endif

#ifdef CI_TEXT_SUPPORTED
void CIAPI
ci_set_text(ci_const_structrp ci_ptr, ci_inforp info_ptr,
    ci_const_textp text_ptr, int num_text)
{
   int ret;
   ret = ci_set_text_2(ci_ptr, info_ptr, text_ptr, num_text);

   if (ret != 0)
      ci_error(ci_ptr, "Insufficient memory to store text");
}

int /* PRIVATE */
ci_set_text_2(ci_const_structrp ci_ptr, ci_inforp info_ptr,
    ci_const_textp text_ptr, int num_text)
{
   int i;

   ci_debug1(1, "in text storage function, chunk typeid = 0x%lx",
      ci_ptr == NULL ? 0xabadca11UL : (unsigned long)ci_ptr->chunk_name);

   if (ci_ptr == NULL || info_ptr == NULL || num_text <= 0 || text_ptr == NULL)
      return 0;

   /* Make sure we have enough space in the "text" array in info_struct
    * to hold all of the incoming text_ptr objects.  This compare can't overflow
    * because max_text >= num_text (anyway, subtract of two positive integers
    * can't overflow in any case.)
    */
   if (num_text > info_ptr->max_text - info_ptr->num_text)
   {
      int old_num_text = info_ptr->num_text;
      int max_text;
      ci_textp new_text = NULL;

      /* Calculate an appropriate max_text, checking for overflow. */
      max_text = old_num_text;
      if (num_text <= INT_MAX - max_text)
      {
         max_text += num_text;

         /* Round up to a multiple of 8 */
         if (max_text < INT_MAX-8)
            max_text = (max_text + 8) & ~0x7;

         else
            max_text = INT_MAX;

         /* Now allocate a new array and copy the old members in; this does all
          * the overflow checks.
          */
         new_text = ci_voidcast(ci_textp,ci_realloc_array(ci_ptr,
             info_ptr->text, old_num_text, max_text-old_num_text,
             sizeof *new_text));
      }

      if (new_text == NULL)
      {
         ci_chunk_report(ci_ptr, "too many text chunks",
             CI_CHUNK_WRITE_ERROR);

         return 1;
      }

      ci_free(ci_ptr, info_ptr->text);

      info_ptr->text = new_text;
      info_ptr->free_me |= CI_FREE_TEXT;
      info_ptr->max_text = max_text;
      /* num_text is adjusted below as the entries are copied in */

      ci_debug1(3, "allocated %d entries for info_ptr->text", max_text);
   }

   for (i = 0; i < num_text; i++)
   {
      size_t text_length, key_len;
      size_t lang_len, lang_key_len;
      ci_textp textp = &(info_ptr->text[info_ptr->num_text]);

      if (text_ptr[i].key == NULL)
          continue;

      if (text_ptr[i].compression < CI_TEXT_COMPRESSION_NONE ||
          text_ptr[i].compression >= CI_TEXT_COMPRESSION_LAST)
      {
         ci_chunk_report(ci_ptr, "text compression mode is out of range",
             CI_CHUNK_WRITE_ERROR);
         continue;
      }

      key_len = strlen(text_ptr[i].key);

      if (text_ptr[i].compression <= 0)
      {
         lang_len = 0;
         lang_key_len = 0;
      }

      else
#  ifdef CI_iTXt_SUPPORTED
      {
         /* Set iTXt data */

         if (text_ptr[i].lang != NULL)
            lang_len = strlen(text_ptr[i].lang);

         else
            lang_len = 0;

         if (text_ptr[i].lang_key != NULL)
            lang_key_len = strlen(text_ptr[i].lang_key);

         else
            lang_key_len = 0;
      }
#  else /* iTXt */
      {
         ci_chunk_report(ci_ptr, "iTXt chunk not supported",
             CI_CHUNK_WRITE_ERROR);
         continue;
      }
#  endif

      if (text_ptr[i].text == NULL || text_ptr[i].text[0] == '\0')
      {
         text_length = 0;
#  ifdef CI_iTXt_SUPPORTED
         if (text_ptr[i].compression > 0)
            textp->compression = CI_ITXT_COMPRESSION_NONE;

         else
#  endif
            textp->compression = CI_TEXT_COMPRESSION_NONE;
      }

      else
      {
         text_length = strlen(text_ptr[i].text);
         textp->compression = text_ptr[i].compression;
      }

      textp->key = ci_voidcast(ci_charp,ci_malloc_base(ci_ptr,
          key_len + text_length + lang_len + lang_key_len + 4));

      if (textp->key == NULL)
      {
         ci_chunk_report(ci_ptr, "text chunk: out of memory",
             CI_CHUNK_WRITE_ERROR);

         return 1;
      }

      ci_debug2(2, "Allocated %lu bytes at %p in ci_set_text",
          (unsigned long)(ci_uint_32)
          (key_len + lang_len + lang_key_len + text_length + 4),
          textp->key);

      memcpy(textp->key, text_ptr[i].key, key_len);
      *(textp->key + key_len) = '\0';

      if (text_ptr[i].compression > 0)
      {
         textp->lang = textp->key + key_len + 1;
         memcpy(textp->lang, text_ptr[i].lang, lang_len);
         *(textp->lang + lang_len) = '\0';
         textp->lang_key = textp->lang + lang_len + 1;
         memcpy(textp->lang_key, text_ptr[i].lang_key, lang_key_len);
         *(textp->lang_key + lang_key_len) = '\0';
         textp->text = textp->lang_key + lang_key_len + 1;
      }

      else
      {
         textp->lang=NULL;
         textp->lang_key=NULL;
         textp->text = textp->key + key_len + 1;
      }

      if (text_length != 0)
         memcpy(textp->text, text_ptr[i].text, text_length);

      *(textp->text + text_length) = '\0';

#  ifdef CI_iTXt_SUPPORTED
      if (textp->compression > 0)
      {
         textp->text_length = 0;
         textp->itxt_length = text_length;
      }

      else
#  endif
      {
         textp->text_length = text_length;
         textp->itxt_length = 0;
      }

      info_ptr->num_text++;
      ci_debug1(3, "transferred text chunk %d", info_ptr->num_text);
   }

   return 0;
}
#endif

#ifdef CI_tIME_SUPPORTED
void CIAPI
ci_set_tIME(ci_const_structrp ci_ptr, ci_inforp info_ptr,
    ci_const_timep mod_time)
{
   ci_debug1(1, "in %s storage function", "tIME");

   if (ci_ptr == NULL || info_ptr == NULL || mod_time == NULL ||
       (ci_ptr->mode & CI_WROTE_tIME) != 0)
      return;

   if (mod_time->month == 0   || mod_time->month > 12  ||
       mod_time->day   == 0   || mod_time->day   > 31  ||
       mod_time->hour  > 23   || mod_time->minute > 59 ||
       mod_time->second > 60)
   {
      ci_warning(ci_ptr, "Ignoring invalid time value");

      return;
   }

   info_ptr->mod_time = *mod_time;
   info_ptr->valid |= CI_INFO_tIME;
}
#endif

#ifdef CI_tRNS_SUPPORTED
void CIAPI
ci_set_tRNS(ci_structrp ci_ptr, ci_inforp info_ptr,
    ci_const_bytep trans_alpha, int num_trans, ci_const_color_16p trans_color)
{
   ci_debug1(1, "in %s storage function", "tRNS");

   if (ci_ptr == NULL || info_ptr == NULL)

      return;

   if (trans_alpha != NULL)
   {
       /* It may not actually be necessary to set ci_ptr->trans_alpha here;
        * we do it for backward compatibility with the way the ci_handle_tRNS
        * function used to do the allocation.
        *
        * 1.6.0: The above statement is incorrect; ci_handle_tRNS effectively
        * relies on ci_set_tRNS storing the information in ci_struct
        * (otherwise it won't be there for the code in cirtran.c).
        */

       ci_free_data(ci_ptr, info_ptr, CI_FREE_TRNS, 0);

       if (num_trans > 0 && num_trans <= CI_MAX_PALETTE_LENGTH)
       {
         /* Changed from num_trans to CI_MAX_PALETTE_LENGTH in version 1.2.1 */
          info_ptr->trans_alpha = ci_voidcast(ci_bytep,
              ci_malloc(ci_ptr, CI_MAX_PALETTE_LENGTH));
          memcpy(info_ptr->trans_alpha, trans_alpha, (size_t)num_trans);

          info_ptr->free_me |= CI_FREE_TRNS;
          info_ptr->valid |= CI_INFO_tRNS;
       }
       ci_ptr->trans_alpha = info_ptr->trans_alpha;
   }

   if (trans_color != NULL)
   {
#ifdef CI_WARNINGS_SUPPORTED
      if (info_ptr->bit_depth < 16)
      {
         int sample_max = (1 << info_ptr->bit_depth) - 1;

         if ((info_ptr->color_type == CI_COLOR_TYPE_GRAY &&
             trans_color->gray > sample_max) ||
             (info_ptr->color_type == CI_COLOR_TYPE_RGB &&
             (trans_color->red > sample_max ||
             trans_color->green > sample_max ||
             trans_color->blue > sample_max)))
            ci_warning(ci_ptr,
                "tRNS chunk has out-of-range samples for bit_depth");
      }
#endif

      info_ptr->trans_color = *trans_color;

      if (num_trans == 0)
         num_trans = 1;
   }

   info_ptr->num_trans = (ci_uint_16)num_trans;

   if (num_trans != 0)
   {
      info_ptr->free_me |= CI_FREE_TRNS;
      info_ptr->valid |= CI_INFO_tRNS;
   }
}
#endif

#ifdef CI_sPLT_SUPPORTED
void CIAPI
ci_set_sPLT(ci_const_structrp ci_ptr,
    ci_inforp info_ptr, ci_const_sPLT_tp entries, int nentries)
/*
 *  entries        - array of ci_sPLT_t structures
 *                   to be added to the list of palettes
 *                   in the info structure.
 *
 *  nentries       - number of palette structures to be
 *                   added.
 */
{
   ci_sPLT_tp np;

   ci_debug1(1, "in %s storage function", "sPLT");

   if (ci_ptr == NULL || info_ptr == NULL || nentries <= 0 || entries == NULL)
      return;

   /* Use the internal realloc function, which checks for all the possible
    * overflows.  Notice that the parameters are (int) and (size_t)
    */
   np = ci_voidcast(ci_sPLT_tp,ci_realloc_array(ci_ptr,
       info_ptr->splt_palettes, info_ptr->splt_palettes_num, nentries,
       sizeof *np));

   if (np == NULL)
   {
      /* Out of memory or too many chunks */
      ci_chunk_report(ci_ptr, "too many sPLT chunks", CI_CHUNK_WRITE_ERROR);
      return;
   }

   ci_free(ci_ptr, info_ptr->splt_palettes);

   info_ptr->splt_palettes = np;
   info_ptr->free_me |= CI_FREE_SPLT;

   np += info_ptr->splt_palettes_num;

   do
   {
      size_t length;

      /* Skip invalid input entries */
      if (entries->name == NULL || entries->entries == NULL)
      {
         /* ci_handle_sPLT doesn't do this, so this is an app error */
         ci_app_error(ci_ptr, "ci_set_sPLT: invalid sPLT");
         /* Just skip the invalid entry */
         continue;
      }

      np->depth = entries->depth;

      /* In the event of out-of-memory just return - there's no point keeping
       * on trying to add sPLT chunks.
       */
      length = strlen(entries->name) + 1;
      np->name = ci_voidcast(ci_charp, ci_malloc_base(ci_ptr, length));

      if (np->name == NULL)
         break;

      memcpy(np->name, entries->name, length);

      /* IMPORTANT: we have memory now that won't get freed if something else
       * goes wrong; this code must free it.  ci_malloc_array produces no
       * warnings; use a ci_chunk_report (below) if there is an error.
       */
      np->entries = ci_voidcast(ci_sPLT_entryp, ci_malloc_array(ci_ptr,
          entries->nentries, sizeof (ci_sPLT_entry)));

      if (np->entries == NULL)
      {
         ci_free(ci_ptr, np->name);
         np->name = NULL;
         break;
      }

      np->nentries = entries->nentries;
      /* This multiply can't overflow because ci_malloc_array has already
       * checked it when doing the allocation.
       */
      memcpy(np->entries, entries->entries,
          (unsigned int)entries->nentries * sizeof (ci_sPLT_entry));

      /* Note that 'continue' skips the advance of the out pointer and out
       * count, so an invalid entry is not added.
       */
      info_ptr->valid |= CI_INFO_sPLT;
      ++(info_ptr->splt_palettes_num);
      ++np;
      ++entries;
   }
   while (--nentries);

   if (nentries > 0)
      ci_chunk_report(ci_ptr, "sPLT out of memory", CI_CHUNK_WRITE_ERROR);
}
#endif /* sPLT */

#ifdef CI_STORE_UNKNOWN_CHUNKS_SUPPORTED
static ci_byte
check_location(ci_const_structrp ci_ptr, int location)
{
   location &= (CI_HAVE_IHDR|CI_HAVE_PLTE|CI_AFTER_IDAT);

   /* New in 1.6.0; copy the location and check it.  This is an API
    * change; previously the app had to use the
    * ci_set_unknown_chunk_location API below for each chunk.
    */
   if (location == 0 && (ci_ptr->mode & CI_IS_READ_STRUCT) == 0)
   {
      /* Write struct, so unknown chunks come from the app */
      ci_app_warning(ci_ptr,
          "ci_set_unknown_chunks now expects a valid location");
      /* Use the old behavior */
      location = (ci_byte)(ci_ptr->mode &
          (CI_HAVE_IHDR|CI_HAVE_PLTE|CI_AFTER_IDAT));
   }

   /* This need not be an internal error - if the app calls
    * ci_set_unknown_chunks on a read pointer it must get the location right.
    */
   if (location == 0)
      ci_error(ci_ptr, "invalid location in ci_set_unknown_chunks");

   /* Now reduce the location to the top-most set bit by removing each least
    * significant bit in turn.
    */
   while (location != (location & -location))
      location &= ~(location & -location);

   /* The cast is safe because 'location' is a bit mask and only the low four
    * bits are significant.
    */
   return (ci_byte)location;
}

void CIAPI
ci_set_unknown_chunks(ci_const_structrp ci_ptr,
    ci_inforp info_ptr, ci_const_unknown_chunkp unknowns, int num_unknowns)
{
   ci_unknown_chunkp np;

   if (ci_ptr == NULL || info_ptr == NULL || num_unknowns <= 0 ||
       unknowns == NULL)
      return;

   /* Check for the failure cases where support has been disabled at compile
    * time.  This code is hardly ever compiled - it's here because
    * STORE_UNKNOWN_CHUNKS is set by both read and write code (compiling in this
    * code) but may be meaningless if the read or write handling of unknown
    * chunks is not compiled in.
    */
#  if !defined(CI_READ_UNKNOWN_CHUNKS_SUPPORTED) && \
      defined(CI_READ_SUPPORTED)
      if ((ci_ptr->mode & CI_IS_READ_STRUCT) != 0)
      {
         ci_app_error(ci_ptr, "no unknown chunk support on read");

         return;
      }
#  endif
#  if !defined(CI_WRITE_UNKNOWN_CHUNKS_SUPPORTED) && \
      defined(CI_WRITE_SUPPORTED)
      if ((ci_ptr->mode & CI_IS_READ_STRUCT) == 0)
      {
         ci_app_error(ci_ptr, "no unknown chunk support on write");

         return;
      }
#  endif

   /* Prior to 1.6.0 this code used ci_malloc_warn; however, this meant that
    * unknown critical chunks could be lost with just a warning resulting in
    * undefined behavior.  Now ci_chunk_report is used to provide behavior
    * appropriate to read or write.
    */
   np = ci_voidcast(ci_unknown_chunkp, ci_realloc_array(ci_ptr,
       info_ptr->unknown_chunks, info_ptr->unknown_chunks_num, num_unknowns,
       sizeof *np));

   if (np == NULL)
   {
      ci_chunk_report(ci_ptr, "too many unknown chunks",
          CI_CHUNK_WRITE_ERROR);
      return;
   }

   ci_free(ci_ptr, info_ptr->unknown_chunks);

   info_ptr->unknown_chunks = np; /* safe because it is initialized */
   info_ptr->free_me |= CI_FREE_UNKN;

   np += info_ptr->unknown_chunks_num;

   /* Increment unknown_chunks_num each time round the loop to protect the
    * just-allocated chunk data.
    */
   for (; num_unknowns > 0; --num_unknowns, ++unknowns)
   {
      memcpy(np->name, unknowns->name, (sizeof np->name));
      np->name[(sizeof np->name)-1] = '\0';
      np->location = check_location(ci_ptr, unknowns->location);

      if (unknowns->size == 0)
      {
         np->data = NULL;
         np->size = 0;
      }

      else
      {
         np->data = ci_voidcast(ci_bytep,
             ci_malloc_base(ci_ptr, unknowns->size));

         if (np->data == NULL)
         {
            ci_chunk_report(ci_ptr, "unknown chunk: out of memory",
                CI_CHUNK_WRITE_ERROR);
            /* But just skip storing the unknown chunk */
            continue;
         }

         memcpy(np->data, unknowns->data, unknowns->size);
         np->size = unknowns->size;
      }

      /* These increments are skipped on out-of-memory for the data - the
       * unknown chunk entry gets overwritten if the ci_chunk_report returns.
       * This is correct in the read case (the chunk is just dropped.)
       */
      ++np;
      ++(info_ptr->unknown_chunks_num);
   }
}

void CIAPI
ci_set_unknown_chunk_location(ci_const_structrp ci_ptr, ci_inforp info_ptr,
    int chunk, int location)
{
   /* This API is pretty pointless in 1.6.0 because the location can be set
    * before the call to ci_set_unknown_chunks.
    *
    * TODO: add a ci_app_warning in 1.7
    */
   if (ci_ptr != NULL && info_ptr != NULL && chunk >= 0 &&
      chunk < info_ptr->unknown_chunks_num)
   {
      if ((location & (CI_HAVE_IHDR|CI_HAVE_PLTE|CI_AFTER_IDAT)) == 0)
      {
         ci_app_error(ci_ptr, "invalid unknown chunk location");
         /* Fake out the pre 1.6.0 behavior: */
         if (((unsigned int)location & CI_HAVE_IDAT) != 0) /* undocumented! */
            location = CI_AFTER_IDAT;

         else
            location = CI_HAVE_IHDR; /* also undocumented */
      }

      info_ptr->unknown_chunks[chunk].location =
         check_location(ci_ptr, location);
   }
}
#endif /* STORE_UNKNOWN_CHUNKS */

#ifdef CI_MNG_FEATURES_SUPPORTED
ci_uint_32 CIAPI
ci_permit_mng_features(ci_structrp ci_ptr, ci_uint_32 mng_features)
{
   ci_debug(1, "in ci_permit_mng_features");

   if (ci_ptr == NULL)
      return 0;

   ci_ptr->mng_features_permitted = mng_features & CI_ALL_MNG_FEATURES;

   return ci_ptr->mng_features_permitted;
}
#endif

#ifdef CI_HANDLE_AS_UNKNOWN_SUPPORTED
static unsigned int
add_one_chunk(ci_bytep list, unsigned int count, ci_const_bytep add, int keep)
{
   unsigned int i;

   /* Utility function: update the 'keep' state of a chunk if it is already in
    * the list, otherwise add it to the list.
    */
   for (i=0; i<count; ++i, list += 5)
   {
      if (memcmp(list, add, 4) == 0)
      {
         list[4] = (ci_byte)keep;

         return count;
      }
   }

   if (keep != CI_HANDLE_CHUNK_AS_DEFAULT)
   {
      ++count;
      memcpy(list, add, 4);
      list[4] = (ci_byte)keep;
   }

   return count;
}

void CIAPI
ci_set_keep_unknown_chunks(ci_structrp ci_ptr, int keep,
    ci_const_bytep chunk_list, int num_chunks_in)
{
   ci_bytep new_list;
   unsigned int num_chunks, old_num_chunks;

   if (ci_ptr == NULL)
      return;

   if (keep < 0 || keep >= CI_HANDLE_CHUNK_LAST)
   {
      ci_app_error(ci_ptr, "ci_set_keep_unknown_chunks: invalid keep");

      return;
   }

   if (num_chunks_in <= 0)
   {
      ci_ptr->unknown_default = keep;

      /* '0' means just set the flags, so stop here */
      if (num_chunks_in == 0)
        return;
   }

   if (num_chunks_in < 0)
   {
      /* Ignore all unknown chunks and all chunks recognized by
       * libci except for IHDR, PLTE, tRNS, IDAT, and IEND
       */
      static const ci_byte chunks_to_ignore[] = {
         98,  75,  71,  68, '\0',  /* bKGD */
         99,  72,  82,  77, '\0',  /* cHRM */
         99,  73,  67,  80, '\0',  /* cICP */
         99,  76,  76,  73, '\0',  /* cLLI */
        101,  88,  73, 102, '\0',  /* eXIf */
        103,  65,  77,  65, '\0',  /* gAMA */
        104,  73,  83,  84, '\0',  /* hIST */
        105,  67,  67,  80, '\0',  /* iCCP */
        105,  84,  88, 116, '\0',  /* iTXt */
        109,  68,  67,  86, '\0',  /* mDCV */
        111,  70,  70, 115, '\0',  /* oFFs */
        112,  67,  65,  76, '\0',  /* pCAL */
        112,  72,  89, 115, '\0',  /* pHYs */
        115,  66,  73,  84, '\0',  /* sBIT */
        115,  67,  65,  76, '\0',  /* sCAL */
        115,  80,  76,  84, '\0',  /* sPLT */
        115,  84,  69,  82, '\0',  /* sTER */
        115,  82,  71,  66, '\0',  /* sRGB */
        116,  69,  88, 116, '\0',  /* tEXt */
        116,  73,  77,  69, '\0',  /* tIME */
        122,  84,  88, 116, '\0'   /* zTXt */
      };

      chunk_list = chunks_to_ignore;
      num_chunks = (unsigned int)/*SAFE*/(sizeof chunks_to_ignore)/5U;
   }

   else /* num_chunks_in > 0 */
   {
      if (chunk_list == NULL)
      {
         /* Prior to 1.6.0 this was silently ignored, now it is an app_error
          * which can be switched off.
          */
         ci_app_error(ci_ptr, "ci_set_keep_unknown_chunks: no chunk list");

         return;
      }

      num_chunks = (unsigned int)num_chunks_in;
   }

   old_num_chunks = ci_ptr->num_chunk_list;
   if (ci_ptr->chunk_list == NULL)
      old_num_chunks = 0;

   /* Since num_chunks is always restricted to UINT_MAX/5 this can't overflow.
    */
   if (num_chunks + old_num_chunks > UINT_MAX/5)
   {
      ci_app_error(ci_ptr, "ci_set_keep_unknown_chunks: too many chunks");

      return;
   }

   /* If these chunks are being reset to the default then no more memory is
    * required because add_one_chunk above doesn't extend the list if the 'keep'
    * parameter is the default.
    */
   if (keep != 0)
   {
      new_list = ci_voidcast(ci_bytep, ci_malloc(ci_ptr,
          5 * (num_chunks + old_num_chunks)));

      if (old_num_chunks > 0)
         memcpy(new_list, ci_ptr->chunk_list, 5*old_num_chunks);
   }

   else if (old_num_chunks > 0)
      new_list = ci_ptr->chunk_list;

   else
      new_list = NULL;

   /* Add the new chunks together with each one's handling code.  If the chunk
    * already exists the code is updated, otherwise the chunk is added to the
    * end.  (In libci 1.6.0 order no longer matters because this code enforces
    * the earlier convention that the last setting is the one that is used.)
    */
   if (new_list != NULL)
   {
      ci_const_bytep inlist;
      ci_bytep outlist;
      unsigned int i;

      for (i=0; i<num_chunks; ++i)
      {
         old_num_chunks = add_one_chunk(new_list, old_num_chunks,
             chunk_list+5*i, keep);
      }

      /* Now remove any spurious 'default' entries. */
      num_chunks = 0;
      for (i=0, inlist=outlist=new_list; i<old_num_chunks; ++i, inlist += 5)
      {
         if (inlist[4])
         {
            if (outlist != inlist)
               memcpy(outlist, inlist, 5);
            outlist += 5;
            ++num_chunks;
         }
      }

      /* This means the application has removed all the specialized handling. */
      if (num_chunks == 0)
      {
         if (ci_ptr->chunk_list != new_list)
            ci_free(ci_ptr, new_list);

         new_list = NULL;
      }
   }

   else
      num_chunks = 0;

   ci_ptr->num_chunk_list = num_chunks;

   if (ci_ptr->chunk_list != new_list)
   {
      if (ci_ptr->chunk_list != NULL)
         ci_free(ci_ptr, ci_ptr->chunk_list);

      ci_ptr->chunk_list = new_list;
   }
}
#endif

#ifdef CI_READ_USER_CHUNKS_SUPPORTED
void CIAPI
ci_set_read_user_chunk_fn(ci_structrp ci_ptr, ci_voidp user_chunk_ptr,
    ci_user_chunk_ptr read_user_chunk_fn)
{
   ci_debug(1, "in ci_set_read_user_chunk_fn");

   if (ci_ptr == NULL)
      return;

   ci_ptr->read_user_chunk_fn = read_user_chunk_fn;
   ci_ptr->user_chunk_ptr = user_chunk_ptr;
}
#endif

#ifdef CI_INFO_IMAGE_SUPPORTED
void CIAPI
ci_set_rows(ci_const_structrp ci_ptr, ci_inforp info_ptr,
    ci_bytepp row_pointers)
{
   ci_debug(1, "in ci_set_rows");

   if (ci_ptr == NULL || info_ptr == NULL)
      return;

   if (info_ptr->row_pointers != NULL &&
       (info_ptr->row_pointers != row_pointers))
      ci_free_data(ci_ptr, info_ptr, CI_FREE_ROWS, 0);

   info_ptr->row_pointers = row_pointers;

   if (row_pointers != NULL)
      info_ptr->valid |= CI_INFO_IDAT;
}
#endif

void CIAPI
ci_set_compression_buffer_size(ci_structrp ci_ptr, size_t size)
{
   ci_debug(1, "in ci_set_compression_buffer_size");

   if (ci_ptr == NULL)
      return;

   if (size == 0 || size > CI_UINT_31_MAX)
      ci_error(ci_ptr, "invalid compression buffer size");

#  ifdef CI_SEQUENTIAL_READ_SUPPORTED
   if ((ci_ptr->mode & CI_IS_READ_STRUCT) != 0)
   {
      ci_ptr->IDAT_read_size = (ci_uint_32)size; /* checked above */
      return;
   }
#  endif

#  ifdef CI_WRITE_SUPPORTED
   if ((ci_ptr->mode & CI_IS_READ_STRUCT) == 0)
   {
      if (ci_ptr->zowner != 0)
      {
         ci_warning(ci_ptr,
             "Compression buffer size cannot be changed because it is in use");

         return;
      }

#ifndef __COVERITY__
      /* Some compilers complain that this is always false.  However, it
       * can be true when integer overflow happens.
       */
      if (size > ZLIB_IO_MAX)
      {
         ci_warning(ci_ptr,
             "Compression buffer size limited to system maximum");
         size = ZLIB_IO_MAX; /* must fit */
      }
#endif

      if (size < 6)
      {
         /* Deflate will potentially go into an infinite loop on a SYNC_FLUSH
          * if this is permitted.
          */
         ci_warning(ci_ptr,
             "Compression buffer size cannot be reduced below 6");

         return;
      }

      if (ci_ptr->zbuffer_size != size)
      {
         ci_free_buffer_list(ci_ptr, &ci_ptr->zbuffer_list);
         ci_ptr->zbuffer_size = (uInt)size;
      }
   }
#  endif
}

void CIAPI
ci_set_invalid(ci_const_structrp ci_ptr, ci_inforp info_ptr, int mask)
{
   if (ci_ptr != NULL && info_ptr != NULL)
      info_ptr->valid &= (unsigned int)(~mask);
}


#ifdef CI_SET_USER_LIMITS_SUPPORTED
/* This function was added to libci 1.2.6 */
void CIAPI
ci_set_user_limits(ci_structrp ci_ptr, ci_uint_32 user_width_max,
    ci_uint_32 user_height_max)
{
   ci_debug(1, "in ci_set_user_limits");

   /* Images with dimensions larger than these limits will be
    * rejected by ci_set_IHDR().  To accept any CI datastream
    * regardless of dimensions, set both limits to 0x7fffffff.
    */
   if (ci_ptr == NULL)
      return;

   ci_ptr->user_width_max = user_width_max;
   ci_ptr->user_height_max = user_height_max;
}

/* This function was added to libci 1.4.0 */
void CIAPI
ci_set_chunk_cache_max(ci_structrp ci_ptr, ci_uint_32 user_chunk_cache_max)
{
   ci_debug(1, "in ci_set_chunk_cache_max");

   if (ci_ptr != NULL)
      ci_ptr->user_chunk_cache_max = user_chunk_cache_max;
}

/* This function was added to libci 1.4.1 */
void CIAPI
ci_set_chunk_malloc_max(ci_structrp ci_ptr,
    ci_alloc_size_t user_chunk_malloc_max)
{
   ci_debug(1, "in ci_set_chunk_malloc_max");

   /* cistruct::user_chunk_malloc_max is initialized to a non-zero value in
    * ci.c.  This API supports '0' for unlimited, make sure the correct
    * (unlimited) value is set here to avoid a need to check for 0 everywhere
    * the parameter is used.
    */
   if (ci_ptr != NULL)
   {
      if (user_chunk_malloc_max == 0U) /* unlimited */
      {
#        ifdef CI_MAX_MALLOC_64K
            ci_ptr->user_chunk_malloc_max = 65536U;
#        else
            ci_ptr->user_chunk_malloc_max = CI_SIZE_MAX;
#        endif
      }
      else
         ci_ptr->user_chunk_malloc_max = user_chunk_malloc_max;
   }
}
#endif /* ?SET_USER_LIMITS */


#ifdef CI_BENIGN_ERRORS_SUPPORTED
void CIAPI
ci_set_benign_errors(ci_structrp ci_ptr, int allowed)
{
   ci_debug(1, "in ci_set_benign_errors");

   /* If allowed is 1, ci_benign_error() is treated as a warning.
    *
    * If allowed is 0, ci_benign_error() is treated as an error (which
    * is the default behavior if ci_set_benign_errors() is not called).
    */

   if (allowed != 0)
      ci_ptr->flags |= CI_FLAG_BENIGN_ERRORS_WARN |
         CI_FLAG_APP_WARNINGS_WARN | CI_FLAG_APP_ERRORS_WARN;

   else
      ci_ptr->flags &= ~(CI_FLAG_BENIGN_ERRORS_WARN |
         CI_FLAG_APP_WARNINGS_WARN | CI_FLAG_APP_ERRORS_WARN);
}
#endif /* BENIGN_ERRORS */

#ifdef CI_CHECK_FOR_INVALID_INDEX_SUPPORTED
   /* Whether to report invalid palette index; added at libng-1.5.10.
    * It is possible for an indexed (color-type==3) CI file to contain
    * pixels with invalid (out-of-range) indexes if the PLTE chunk has
    * fewer entries than the image's bit-depth would allow. We recover
    * from this gracefully by filling any incomplete palette with zeros
    * (opaque black).  By default, when this occurs libci will issue
    * a benign error.  This API can be used to override that behavior.
    */
void CIAPI
ci_set_check_for_invalid_index(ci_structrp ci_ptr, int allowed)
{
   ci_debug(1, "in ci_set_check_for_invalid_index");

   if (allowed > 0)
      ci_ptr->num_palette_max = 0;

   else
      ci_ptr->num_palette_max = -1;
}
#endif

#if defined(CI_TEXT_SUPPORTED) || defined(CI_pCAL_SUPPORTED) || \
    defined(CI_iCCP_SUPPORTED) || defined(CI_sPLT_SUPPORTED)
/* Check that the tEXt or zTXt keyword is valid per CI 1.0 specification,
 * and if invalid, correct the keyword rather than discarding the entire
 * chunk.  The CI 1.0 specification requires keywords 1-79 characters in
 * length, forbids leading or trailing whitespace, multiple internal spaces,
 * and the non-break space (0x80) from ISO 8859-1.  Returns keyword length.
 *
 * The 'new_key' buffer must be 80 characters in size (for the keyword plus a
 * trailing '\0').  If this routine returns 0 then there was no keyword, or a
 * valid one could not be generated, and the caller must ci_error.
 */
ci_uint_32 /* PRIVATE */
ci_check_keyword(ci_structrp ci_ptr, ci_const_charp key, ci_bytep new_key)
{
#ifdef CI_WARNINGS_SUPPORTED
   ci_const_charp orig_key = key;
#endif
   ci_uint_32 key_len = 0;
   int bad_character = 0;
   int space = 1;

   ci_debug(1, "in ci_check_keyword");

   if (key == NULL)
   {
      *new_key = 0;
      return 0;
   }

   while (*key && key_len < 79)
   {
      ci_byte ch = (ci_byte)*key++;

      if ((ch > 32 && ch <= 126) || (ch >= 161 /*&& ch <= 255*/))
      {
         *new_key++ = ch; ++key_len; space = 0;
      }

      else if (space == 0)
      {
         /* A space or an invalid character when one wasn't seen immediately
          * before; output just a space.
          */
         *new_key++ = 32; ++key_len; space = 1;

         /* If the character was not a space then it is invalid. */
         if (ch != 32)
            bad_character = ch;
      }

      else if (bad_character == 0)
         bad_character = ch; /* just skip it, record the first error */
   }

   if (key_len > 0 && space != 0) /* trailing space */
   {
      --key_len; --new_key;
      if (bad_character == 0)
         bad_character = 32;
   }

   /* Terminate the keyword */
   *new_key = 0;

   if (key_len == 0)
      return 0;

#ifdef CI_WARNINGS_SUPPORTED
   /* Try to only output one warning per keyword: */
   if (*key != 0) /* keyword too long */
      ci_warning(ci_ptr, "keyword truncated");

   else if (bad_character != 0)
   {
      CI_WARNING_PARAMETERS(p)

      ci_warning_parameter(p, 1, orig_key);
      ci_warning_parameter_signed(p, 2, CI_NUMBER_FORMAT_02x, bad_character);

      ci_formatted_warning(ci_ptr, p, "keyword \"@1\": bad character '0x@2'");
   }
#else /* !WARNINGS */
   CI_UNUSED(ci_ptr)
#endif /* !WARNINGS */

   return key_len;
}
#endif /* TEXT || pCAL || iCCP || sPLT */
#endif /* READ || WRITE */

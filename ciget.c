/* ciget.c - retrieval of values from info struct
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
 */

#include "cipriv.h"

#if defined(CI_READ_SUPPORTED) || defined(CI_WRITE_SUPPORTED)

ci_uint_32 CIAPI
ci_get_valid(ci_const_structrp ci_ptr, ci_const_inforp info_ptr,
    ci_uint_32 flag)
{
   if (ci_ptr != NULL && info_ptr != NULL)
   {
#ifdef CI_READ_tRNS_SUPPORTED
      /* ci_handle_PLTE() may have canceled a valid tRNS chunk but left the
       * 'valid' flag for the detection of duplicate chunks. Do not report a
       * valid tRNS chunk in this case.
       */
      if (flag == CI_INFO_tRNS && ci_ptr->num_trans == 0)
         return 0;
#endif

      return info_ptr->valid & flag;
   }

   return 0;
}

size_t CIAPI
ci_get_rowbytes(ci_const_structrp ci_ptr, ci_const_inforp info_ptr)
{
   if (ci_ptr != NULL && info_ptr != NULL)
      return info_ptr->rowbytes;

   return 0;
}

#ifdef CI_INFO_IMAGE_SUPPORTED
ci_bytepp CIAPI
ci_get_rows(ci_const_structrp ci_ptr, ci_const_inforp info_ptr)
{
   if (ci_ptr != NULL && info_ptr != NULL)
      return info_ptr->row_pointers;

   return 0;
}
#endif

#ifdef CI_EASY_ACCESS_SUPPORTED
/* Easy access to info, added in libci-0.99 */
ci_uint_32 CIAPI
ci_get_image_width(ci_const_structrp ci_ptr, ci_const_inforp info_ptr)
{
   if (ci_ptr != NULL && info_ptr != NULL)
      return info_ptr->width;

   return 0;
}

ci_uint_32 CIAPI
ci_get_image_height(ci_const_structrp ci_ptr, ci_const_inforp info_ptr)
{
   if (ci_ptr != NULL && info_ptr != NULL)
      return info_ptr->height;

   return 0;
}

ci_byte CIAPI
ci_get_bit_depth(ci_const_structrp ci_ptr, ci_const_inforp info_ptr)
{
   if (ci_ptr != NULL && info_ptr != NULL)
      return info_ptr->bit_depth;

   return 0;
}

ci_byte CIAPI
ci_get_color_type(ci_const_structrp ci_ptr, ci_const_inforp info_ptr)
{
   if (ci_ptr != NULL && info_ptr != NULL)
      return info_ptr->color_type;

   return 0;
}

ci_byte CIAPI
ci_get_filter_type(ci_const_structrp ci_ptr, ci_const_inforp info_ptr)
{
   if (ci_ptr != NULL && info_ptr != NULL)
      return info_ptr->filter_type;

   return 0;
}

ci_byte CIAPI
ci_get_interlace_type(ci_const_structrp ci_ptr, ci_const_inforp info_ptr)
{
   if (ci_ptr != NULL && info_ptr != NULL)
      return info_ptr->interlace_type;

   return 0;
}

ci_byte CIAPI
ci_get_compression_type(ci_const_structrp ci_ptr, ci_const_inforp info_ptr)
{
   if (ci_ptr != NULL && info_ptr != NULL)
      return info_ptr->compression_type;

   return 0;
}

ci_uint_32 CIAPI
ci_get_x_pixels_per_meter(ci_const_structrp ci_ptr, ci_const_inforp
   info_ptr)
{
#ifdef CI_pHYs_SUPPORTED
   ci_debug(1, "in ci_get_x_pixels_per_meter");

   if (ci_ptr != NULL && info_ptr != NULL &&
       (info_ptr->valid & CI_INFO_pHYs) != 0)
   {
      if (info_ptr->phys_unit_type == CI_RESOLUTION_METER)
         return info_ptr->x_pixels_per_unit;
   }
#else
   CI_UNUSED(ci_ptr)
   CI_UNUSED(info_ptr)
#endif

   return 0;
}

ci_uint_32 CIAPI
ci_get_y_pixels_per_meter(ci_const_structrp ci_ptr, ci_const_inforp
    info_ptr)
{
#ifdef CI_pHYs_SUPPORTED
   ci_debug(1, "in ci_get_y_pixels_per_meter");

   if (ci_ptr != NULL && info_ptr != NULL &&
       (info_ptr->valid & CI_INFO_pHYs) != 0)
   {
      if (info_ptr->phys_unit_type == CI_RESOLUTION_METER)
         return info_ptr->y_pixels_per_unit;
   }
#else
   CI_UNUSED(ci_ptr)
   CI_UNUSED(info_ptr)
#endif

   return 0;
}

ci_uint_32 CIAPI
ci_get_pixels_per_meter(ci_const_structrp ci_ptr, ci_const_inforp info_ptr)
{
#ifdef CI_pHYs_SUPPORTED
   ci_debug(1, "in ci_get_pixels_per_meter");

   if (ci_ptr != NULL && info_ptr != NULL &&
       (info_ptr->valid & CI_INFO_pHYs) != 0)
   {
      if (info_ptr->phys_unit_type == CI_RESOLUTION_METER &&
          info_ptr->x_pixels_per_unit == info_ptr->y_pixels_per_unit)
         return info_ptr->x_pixels_per_unit;
   }
#else
   CI_UNUSED(ci_ptr)
   CI_UNUSED(info_ptr)
#endif

   return 0;
}

#ifdef CI_FLOATING_POINT_SUPPORTED
float CIAPI
ci_get_pixel_aspect_ratio(ci_const_structrp ci_ptr, ci_const_inforp
   info_ptr)
{
#ifdef CI_READ_pHYs_SUPPORTED
   ci_debug(1, "in ci_get_pixel_aspect_ratio");

   if (ci_ptr != NULL && info_ptr != NULL &&
       (info_ptr->valid & CI_INFO_pHYs) != 0)
   {
      if (info_ptr->x_pixels_per_unit != 0)
         return (float)info_ptr->y_pixels_per_unit
              / (float)info_ptr->x_pixels_per_unit;
   }
#else
   CI_UNUSED(ci_ptr)
   CI_UNUSED(info_ptr)
#endif

   return (float)0.0;
}
#endif

#ifdef CI_FIXED_POINT_SUPPORTED
ci_fixed_point CIAPI
ci_get_pixel_aspect_ratio_fixed(ci_const_structrp ci_ptr,
    ci_const_inforp info_ptr)
{
#ifdef CI_READ_pHYs_SUPPORTED
   ci_debug(1, "in ci_get_pixel_aspect_ratio_fixed");

   if (ci_ptr != NULL && info_ptr != NULL &&
       (info_ptr->valid & CI_INFO_pHYs) != 0 &&
       info_ptr->x_pixels_per_unit > 0 && info_ptr->y_pixels_per_unit > 0 &&
       info_ptr->x_pixels_per_unit <= CI_UINT_31_MAX &&
       info_ptr->y_pixels_per_unit <= CI_UINT_31_MAX)
   {
      ci_fixed_point res;

      /* The following casts work because a CI 4 byte integer only has a valid
       * range of 0..2^31-1; otherwise the cast might overflow.
       */
      if (ci_muldiv(&res, (ci_int_32)info_ptr->y_pixels_per_unit, CI_FP_1,
          (ci_int_32)info_ptr->x_pixels_per_unit) != 0)
         return res;
   }
#else
   CI_UNUSED(ci_ptr)
   CI_UNUSED(info_ptr)
#endif

   return 0;
}
#endif

ci_int_32 CIAPI
ci_get_x_offset_microns(ci_const_structrp ci_ptr, ci_const_inforp info_ptr)
{
#ifdef CI_oFFs_SUPPORTED
   ci_debug(1, "in ci_get_x_offset_microns");

   if (ci_ptr != NULL && info_ptr != NULL &&
       (info_ptr->valid & CI_INFO_oFFs) != 0)
   {
      if (info_ptr->offset_unit_type == CI_OFFSET_MICROMETER)
         return info_ptr->x_offset;
   }
#else
   CI_UNUSED(ci_ptr)
   CI_UNUSED(info_ptr)
#endif

   return 0;
}

ci_int_32 CIAPI
ci_get_y_offset_microns(ci_const_structrp ci_ptr, ci_const_inforp info_ptr)
{
#ifdef CI_oFFs_SUPPORTED
   ci_debug(1, "in ci_get_y_offset_microns");

   if (ci_ptr != NULL && info_ptr != NULL &&
       (info_ptr->valid & CI_INFO_oFFs) != 0)
   {
      if (info_ptr->offset_unit_type == CI_OFFSET_MICROMETER)
         return info_ptr->y_offset;
   }
#else
   CI_UNUSED(ci_ptr)
   CI_UNUSED(info_ptr)
#endif

   return 0;
}

ci_int_32 CIAPI
ci_get_x_offset_pixels(ci_const_structrp ci_ptr, ci_const_inforp info_ptr)
{
#ifdef CI_oFFs_SUPPORTED
   ci_debug(1, "in ci_get_x_offset_pixels");

   if (ci_ptr != NULL && info_ptr != NULL &&
       (info_ptr->valid & CI_INFO_oFFs) != 0)
   {
      if (info_ptr->offset_unit_type == CI_OFFSET_PIXEL)
         return info_ptr->x_offset;
   }
#else
   CI_UNUSED(ci_ptr)
   CI_UNUSED(info_ptr)
#endif

   return 0;
}

ci_int_32 CIAPI
ci_get_y_offset_pixels(ci_const_structrp ci_ptr, ci_const_inforp info_ptr)
{
#ifdef CI_oFFs_SUPPORTED
   ci_debug(1, "in ci_get_y_offset_pixels");

   if (ci_ptr != NULL && info_ptr != NULL &&
       (info_ptr->valid & CI_INFO_oFFs) != 0)
   {
      if (info_ptr->offset_unit_type == CI_OFFSET_PIXEL)
         return info_ptr->y_offset;
   }
#else
   CI_UNUSED(ci_ptr)
   CI_UNUSED(info_ptr)
#endif

   return 0;
}

#ifdef CI_INCH_CONVERSIONS_SUPPORTED
static ci_uint_32
ppi_from_ppm(ci_uint_32 ppm)
{
#if 0
   /* The conversion is *(2.54/100), in binary (32 digits):
    * .00000110100000001001110101001001
    */
   ci_uint_32 t1001, t1101;
   ppm >>= 1;                  /* .1 */
   t1001 = ppm + (ppm >> 3);   /* .1001 */
   t1101 = t1001 + (ppm >> 1); /* .1101 */
   ppm >>= 20;                 /* .000000000000000000001 */
   t1101 += t1101 >> 15;       /* .1101000000000001101 */
   t1001 >>= 11;               /* .000000000001001 */
   t1001 += t1001 >> 12;       /* .000000000001001000000001001 */
   ppm += t1001;               /* .000000000001001000001001001 */
   ppm += t1101;               /* .110100000001001110101001001 */
   return (ppm + 16) >> 5;/* .00000110100000001001110101001001 */
#else
   /* The argument is a CI unsigned integer, so it is not permitted
    * to be bigger than 2^31.
    */
   ci_fixed_point result;
   if (ppm <= CI_UINT_31_MAX && ci_muldiv(&result, (ci_int_32)ppm, 127,
       5000) != 0)
      return (ci_uint_32)result;

   /* Overflow. */
   return 0;
#endif
}

ci_uint_32 CIAPI
ci_get_pixels_per_inch(ci_const_structrp ci_ptr, ci_const_inforp info_ptr)
{
   return ppi_from_ppm(ci_get_pixels_per_meter(ci_ptr, info_ptr));
}

ci_uint_32 CIAPI
ci_get_x_pixels_per_inch(ci_const_structrp ci_ptr, ci_const_inforp info_ptr)
{
   return ppi_from_ppm(ci_get_x_pixels_per_meter(ci_ptr, info_ptr));
}

ci_uint_32 CIAPI
ci_get_y_pixels_per_inch(ci_const_structrp ci_ptr, ci_const_inforp info_ptr)
{
   return ppi_from_ppm(ci_get_y_pixels_per_meter(ci_ptr, info_ptr));
}

#ifdef CI_FIXED_POINT_SUPPORTED
static ci_fixed_point
ci_fixed_inches_from_microns(ci_const_structrp ci_ptr, ci_int_32 microns)
{
   /* Convert from meters * 1,000,000 to inches * 100,000, meters to
    * inches is simply *(100/2.54), so we want *(10/2.54) == 500/127.
    * Notice that this can overflow - a warning is output and 0 is
    * returned.
    */
   ci_fixed_point result;

   if (ci_muldiv(&result, microns, 500, 127) != 0)
      return result;

   ci_warning(ci_ptr, "fixed point overflow ignored");
   return 0;
}

ci_fixed_point CIAPI
ci_get_x_offset_inches_fixed(ci_const_structrp ci_ptr,
    ci_const_inforp info_ptr)
{
   return ci_fixed_inches_from_microns(ci_ptr,
       ci_get_x_offset_microns(ci_ptr, info_ptr));
}
#endif /* FIXED_POINT */

#ifdef CI_FIXED_POINT_SUPPORTED
ci_fixed_point CIAPI
ci_get_y_offset_inches_fixed(ci_const_structrp ci_ptr,
    ci_const_inforp info_ptr)
{
   return ci_fixed_inches_from_microns(ci_ptr,
       ci_get_y_offset_microns(ci_ptr, info_ptr));
}
#endif

#ifdef CI_FLOATING_POINT_SUPPORTED
float CIAPI
ci_get_x_offset_inches(ci_const_structrp ci_ptr, ci_const_inforp info_ptr)
{
   /* To avoid the overflow do the conversion directly in floating
    * point.
    */
   return (float)(ci_get_x_offset_microns(ci_ptr, info_ptr) * .00003937);
}
#endif

#ifdef CI_FLOATING_POINT_SUPPORTED
float CIAPI
ci_get_y_offset_inches(ci_const_structrp ci_ptr, ci_const_inforp info_ptr)
{
   /* To avoid the overflow do the conversion directly in floating
    * point.
    */
   return (float)(ci_get_y_offset_microns(ci_ptr, info_ptr) * .00003937);
}
#endif

#ifdef CI_pHYs_SUPPORTED
ci_uint_32 CIAPI
ci_get_pHYs_dpi(ci_const_structrp ci_ptr, ci_const_inforp info_ptr,
    ci_uint_32 *res_x, ci_uint_32 *res_y, int *unit_type)
{
   ci_uint_32 retval = 0;

   ci_debug1(1, "in %s retrieval function", "pHYs");

   if (ci_ptr != NULL && info_ptr != NULL &&
       (info_ptr->valid & CI_INFO_pHYs) != 0)
   {
      if (res_x != NULL)
      {
         *res_x = info_ptr->x_pixels_per_unit;
         retval |= CI_INFO_pHYs;
      }

      if (res_y != NULL)
      {
         *res_y = info_ptr->y_pixels_per_unit;
         retval |= CI_INFO_pHYs;
      }

      if (unit_type != NULL)
      {
         *unit_type = (int)info_ptr->phys_unit_type;
         retval |= CI_INFO_pHYs;

         if (*unit_type == 1)
         {
            if (res_x != NULL) *res_x = (ci_uint_32)(*res_x * .0254 + .50);
            if (res_y != NULL) *res_y = (ci_uint_32)(*res_y * .0254 + .50);
         }
      }
   }

   return retval;
}
#endif /* pHYs */
#endif /* INCH_CONVERSIONS */

/* ci_get_channels really belongs in here, too, but it's been around longer */

#endif /* EASY_ACCESS */


ci_byte CIAPI
ci_get_channels(ci_const_structrp ci_ptr, ci_const_inforp info_ptr)
{
   if (ci_ptr != NULL && info_ptr != NULL)
      return info_ptr->channels;

   return 0;
}

#ifdef CI_READ_SUPPORTED
ci_const_bytep CIAPI
ci_get_signature(ci_const_structrp ci_ptr, ci_const_inforp info_ptr)
{
   if (ci_ptr != NULL && info_ptr != NULL)
      return info_ptr->signature;

   return NULL;
}
#endif

#ifdef CI_bKGD_SUPPORTED
ci_uint_32 CIAPI
ci_get_bKGD(ci_const_structrp ci_ptr, ci_inforp info_ptr,
    ci_color_16p *background)
{
   ci_debug1(1, "in %s retrieval function", "bKGD");

   if (ci_ptr != NULL && info_ptr != NULL &&
       (info_ptr->valid & CI_INFO_bKGD) != 0 &&
       background != NULL)
   {
      *background = &(info_ptr->background);
      return CI_INFO_bKGD;
   }

   return 0;
}
#endif

#ifdef CI_cHRM_SUPPORTED
/* The XYZ APIs were added in 1.5.5 to take advantage of the code added at the
 * same time to correct the rgb grayscale coefficient defaults obtained from the
 * cHRM chunk in 1.5.4
 */
#  ifdef CI_FLOATING_POINT_SUPPORTED
ci_uint_32 CIAPI
ci_get_cHRM(ci_const_structrp ci_ptr, ci_const_inforp info_ptr,
    double *whitex, double *whitey, double *redx, double *redy,
    double *greenx, double *greeny, double *bluex, double *bluey)
{
   ci_debug1(1, "in %s retrieval function", "cHRM");

   /* CIv3: this just returns the values store from the cHRM, if any. */
   if (ci_ptr != NULL && info_ptr != NULL &&
       (info_ptr->valid & CI_INFO_cHRM) != 0)
   {
      if (whitex != NULL)
         *whitex = ci_float(ci_ptr, info_ptr->cHRM.whitex, "cHRM wx");
      if (whitey != NULL)
         *whitey = ci_float(ci_ptr, info_ptr->cHRM.whitey, "cHRM wy");
      if (redx   != NULL)
         *redx   = ci_float(ci_ptr, info_ptr->cHRM.redx,   "cHRM rx");
      if (redy   != NULL)
         *redy   = ci_float(ci_ptr, info_ptr->cHRM.redy,   "cHRM ry");
      if (greenx != NULL)
         *greenx = ci_float(ci_ptr, info_ptr->cHRM.greenx, "cHRM gx");
      if (greeny != NULL)
         *greeny = ci_float(ci_ptr, info_ptr->cHRM.greeny, "cHRM gy");
      if (bluex  != NULL)
         *bluex  = ci_float(ci_ptr, info_ptr->cHRM.bluex,  "cHRM bx");
      if (bluey  != NULL)
         *bluey  = ci_float(ci_ptr, info_ptr->cHRM.bluey,  "cHRM by");
      return CI_INFO_cHRM;
   }

   return 0;
}

ci_uint_32 CIAPI
ci_get_cHRM_XYZ(ci_const_structrp ci_ptr, ci_const_inforp info_ptr,
    double *red_X, double *red_Y, double *red_Z, double *green_X,
    double *green_Y, double *green_Z, double *blue_X, double *blue_Y,
    double *blue_Z)
{
   ci_XYZ XYZ;
   ci_debug1(1, "in %s retrieval function", "cHRM_XYZ(float)");

   if (ci_ptr != NULL && info_ptr != NULL &&
       (info_ptr->valid & CI_INFO_cHRM) != 0 &&
       ci_XYZ_from_xy(&XYZ, &info_ptr->cHRM) == 0)
   {
      if (red_X != NULL)
         *red_X = ci_float(ci_ptr, XYZ.red_X, "cHRM red X");
      if (red_Y != NULL)
         *red_Y = ci_float(ci_ptr, XYZ.red_Y, "cHRM red Y");
      if (red_Z != NULL)
         *red_Z = ci_float(ci_ptr, XYZ.red_Z, "cHRM red Z");
      if (green_X != NULL)
         *green_X = ci_float(ci_ptr, XYZ.green_X, "cHRM green X");
      if (green_Y != NULL)
         *green_Y = ci_float(ci_ptr, XYZ.green_Y, "cHRM green Y");
      if (green_Z != NULL)
         *green_Z = ci_float(ci_ptr, XYZ.green_Z, "cHRM green Z");
      if (blue_X != NULL)
         *blue_X = ci_float(ci_ptr, XYZ.blue_X, "cHRM blue X");
      if (blue_Y != NULL)
         *blue_Y = ci_float(ci_ptr, XYZ.blue_Y, "cHRM blue Y");
      if (blue_Z != NULL)
         *blue_Z = ci_float(ci_ptr, XYZ.blue_Z, "cHRM blue Z");
      return CI_INFO_cHRM;
   }

   return 0;
}
#  endif

#  ifdef CI_FIXED_POINT_SUPPORTED
ci_uint_32 CIAPI
ci_get_cHRM_XYZ_fixed(ci_const_structrp ci_ptr, ci_const_inforp info_ptr,
    ci_fixed_point *int_red_X, ci_fixed_point *int_red_Y,
    ci_fixed_point *int_red_Z, ci_fixed_point *int_green_X,
    ci_fixed_point *int_green_Y, ci_fixed_point *int_green_Z,
    ci_fixed_point *int_blue_X, ci_fixed_point *int_blue_Y,
    ci_fixed_point *int_blue_Z)
{
   ci_XYZ XYZ;
   ci_debug1(1, "in %s retrieval function", "cHRM_XYZ");

   if (ci_ptr != NULL && info_ptr != NULL &&
       (info_ptr->valid & CI_INFO_cHRM) != 0U &&
       ci_XYZ_from_xy(&XYZ, &info_ptr->cHRM) == 0)
   {
      if (int_red_X != NULL) *int_red_X = XYZ.red_X;
      if (int_red_Y != NULL) *int_red_Y = XYZ.red_Y;
      if (int_red_Z != NULL) *int_red_Z = XYZ.red_Z;
      if (int_green_X != NULL) *int_green_X = XYZ.green_X;
      if (int_green_Y != NULL) *int_green_Y = XYZ.green_Y;
      if (int_green_Z != NULL) *int_green_Z = XYZ.green_Z;
      if (int_blue_X != NULL) *int_blue_X = XYZ.blue_X;
      if (int_blue_Y != NULL) *int_blue_Y = XYZ.blue_Y;
      if (int_blue_Z != NULL) *int_blue_Z = XYZ.blue_Z;
      return CI_INFO_cHRM;
   }

   return 0;
}

ci_uint_32 CIAPI
ci_get_cHRM_fixed(ci_const_structrp ci_ptr, ci_const_inforp info_ptr,
    ci_fixed_point *whitex, ci_fixed_point *whitey, ci_fixed_point *redx,
    ci_fixed_point *redy, ci_fixed_point *greenx, ci_fixed_point *greeny,
    ci_fixed_point *bluex, ci_fixed_point *bluey)
{
   ci_debug1(1, "in %s retrieval function", "cHRM");

   /* CIv3: this just returns the values store from the cHRM, if any. */
   if (ci_ptr != NULL && info_ptr != NULL &&
       (info_ptr->valid & CI_INFO_cHRM) != 0)
   {
      if (whitex != NULL) *whitex = info_ptr->cHRM.whitex;
      if (whitey != NULL) *whitey = info_ptr->cHRM.whitey;
      if (redx   != NULL) *redx   = info_ptr->cHRM.redx;
      if (redy   != NULL) *redy   = info_ptr->cHRM.redy;
      if (greenx != NULL) *greenx = info_ptr->cHRM.greenx;
      if (greeny != NULL) *greeny = info_ptr->cHRM.greeny;
      if (bluex  != NULL) *bluex  = info_ptr->cHRM.bluex;
      if (bluey  != NULL) *bluey  = info_ptr->cHRM.bluey;
      return CI_INFO_cHRM;
   }

   return 0;
}
#  endif
#endif

#ifdef CI_gAMA_SUPPORTED
#  ifdef CI_FIXED_POINT_SUPPORTED
ci_uint_32 CIAPI
ci_get_gAMA_fixed(ci_const_structrp ci_ptr, ci_const_inforp info_ptr,
    ci_fixed_point *file_gamma)
{
   ci_debug1(1, "in %s retrieval function", "gAMA");

   /* CIv3 compatibility: only report gAMA if it is really present. */
   if (ci_ptr != NULL && info_ptr != NULL &&
       (info_ptr->valid & CI_INFO_gAMA) != 0)
   {
      if (file_gamma != NULL) *file_gamma = info_ptr->gamma;
      return CI_INFO_gAMA;
   }

   return 0;
}
#  endif

#  ifdef CI_FLOATING_POINT_SUPPORTED
ci_uint_32 CIAPI
ci_get_gAMA(ci_const_structrp ci_ptr, ci_const_inforp info_ptr,
    double *file_gamma)
{
   ci_debug1(1, "in %s retrieval function", "gAMA(float)");

   /* CIv3 compatibility: only report gAMA if it is really present. */
   if (ci_ptr != NULL && info_ptr != NULL &&
       (info_ptr->valid & CI_INFO_gAMA) != 0)
   {
      if (file_gamma != NULL)
         *file_gamma = ci_float(ci_ptr, info_ptr->gamma, "gAMA");

      return CI_INFO_gAMA;
   }

   return 0;
}
#  endif
#endif

#ifdef CI_sRGB_SUPPORTED
ci_uint_32 CIAPI
ci_get_sRGB(ci_const_structrp ci_ptr, ci_const_inforp info_ptr,
    int *file_srgb_intent)
{
   ci_debug1(1, "in %s retrieval function", "sRGB");

   if (ci_ptr != NULL && info_ptr != NULL &&
      (info_ptr->valid & CI_INFO_sRGB) != 0)
   {
      if (file_srgb_intent != NULL)
         *file_srgb_intent = info_ptr->rendering_intent;
      return CI_INFO_sRGB;
   }

   return 0;
}
#endif

#ifdef CI_iCCP_SUPPORTED
ci_uint_32 CIAPI
ci_get_iCCP(ci_const_structrp ci_ptr, ci_inforp info_ptr,
    ci_charpp name, int *compression_type,
    ci_bytepp profile, ci_uint_32 *proflen)
{
   ci_debug1(1, "in %s retrieval function", "iCCP");

   if (ci_ptr != NULL && info_ptr != NULL &&
       (info_ptr->valid & CI_INFO_iCCP) != 0 &&
       name != NULL && profile != NULL && proflen != NULL)
   {
      *name = info_ptr->iccp_name;
      *profile = info_ptr->iccp_profile;
      *proflen = ci_get_uint_32(info_ptr->iccp_profile);
      /* This is somewhat irrelevant since the profile data returned has
       * actually been uncompressed.
       */
      if (compression_type != NULL)
         *compression_type = CI_COMPRESSION_TYPE_BASE;
      return CI_INFO_iCCP;
   }

   return 0;

}
#endif

#ifdef CI_sPLT_SUPPORTED
int CIAPI
ci_get_sPLT(ci_const_structrp ci_ptr, ci_inforp info_ptr,
    ci_sPLT_tpp spalettes)
{
   ci_debug1(1, "in %s retrieval function", "sPLT");

   if (ci_ptr != NULL && info_ptr != NULL && spalettes != NULL)
   {
      *spalettes = info_ptr->splt_palettes;
      return info_ptr->splt_palettes_num;
   }

   return 0;
}
#endif

#ifdef CI_cICP_SUPPORTED
ci_uint_32 CIAPI
ci_get_cICP(ci_const_structrp ci_ptr,
             ci_const_inforp info_ptr, ci_bytep colour_primaries,
             ci_bytep transfer_function, ci_bytep matrix_coefficients,
             ci_bytep video_full_range_flag)
{
    ci_debug1(1, "in %s retrieval function", "cICP");

    if (ci_ptr != NULL && info_ptr != NULL &&
        (info_ptr->valid & CI_INFO_cICP) != 0 &&
        colour_primaries != NULL && transfer_function != NULL &&
        matrix_coefficients != NULL && video_full_range_flag != NULL)
    {
        *colour_primaries = info_ptr->cicp_colour_primaries;
        *transfer_function = info_ptr->cicp_transfer_function;
        *matrix_coefficients = info_ptr->cicp_matrix_coefficients;
        *video_full_range_flag = info_ptr->cicp_video_full_range_flag;
        return (CI_INFO_cICP);
    }

    return 0;
}
#endif

#ifdef CI_cLLI_SUPPORTED
#  ifdef CI_FIXED_POINT_SUPPORTED
ci_uint_32 CIAPI
ci_get_cLLI_fixed(ci_const_structrp ci_ptr, ci_const_inforp info_ptr,
    ci_uint_32p maxCLL,
    ci_uint_32p maxFALL)
{
   ci_debug1(1, "in %s retrieval function", "cLLI");

   if (ci_ptr != NULL && info_ptr != NULL &&
       (info_ptr->valid & CI_INFO_cLLI) != 0)
   {
      if (maxCLL != NULL) *maxCLL = info_ptr->maxCLL;
      if (maxFALL != NULL) *maxFALL = info_ptr->maxFALL;
      return CI_INFO_cLLI;
   }

   return 0;
}
#  endif

#  ifdef CI_FLOATING_POINT_SUPPORTED
ci_uint_32 CIAPI
ci_get_cLLI(ci_const_structrp ci_ptr, ci_const_inforp info_ptr,
      double *maxCLL, double *maxFALL)
{
   ci_debug1(1, "in %s retrieval function", "cLLI(float)");

   if (ci_ptr != NULL && info_ptr != NULL &&
       (info_ptr->valid & CI_INFO_cLLI) != 0)
   {
      if (maxCLL != NULL) *maxCLL = info_ptr->maxCLL * .0001;
      if (maxFALL != NULL) *maxFALL = info_ptr->maxFALL * .0001;
      return CI_INFO_cLLI;
   }

   return 0;
}
#  endif
#endif /* cLLI */

#ifdef CI_mDCV_SUPPORTED
#  ifdef CI_FIXED_POINT_SUPPORTED
ci_uint_32 CIAPI
ci_get_mDCV_fixed(ci_const_structrp ci_ptr, ci_const_inforp info_ptr,
    ci_fixed_point *white_x, ci_fixed_point *white_y,
    ci_fixed_point *red_x, ci_fixed_point *red_y,
    ci_fixed_point *green_x, ci_fixed_point *green_y,
    ci_fixed_point *blue_x, ci_fixed_point *blue_y,
    ci_uint_32p mastering_maxDL, ci_uint_32p mastering_minDL)
{
   ci_debug1(1, "in %s retrieval function", "mDCV");

   if (ci_ptr != NULL && info_ptr != NULL &&
       (info_ptr->valid & CI_INFO_mDCV) != 0)
   {
      if (white_x != NULL) *white_x = info_ptr->mastering_white_x * 2;
      if (white_y != NULL) *white_y = info_ptr->mastering_white_y * 2;
      if (red_x != NULL) *red_x = info_ptr->mastering_red_x * 2;
      if (red_y != NULL) *red_y = info_ptr->mastering_red_y * 2;
      if (green_x != NULL) *green_x = info_ptr->mastering_green_x * 2;
      if (green_y != NULL) *green_y = info_ptr->mastering_green_y * 2;
      if (blue_x != NULL) *blue_x = info_ptr->mastering_blue_x * 2;
      if (blue_y != NULL) *blue_y = info_ptr->mastering_blue_y * 2;
      if (mastering_maxDL != NULL) *mastering_maxDL = info_ptr->mastering_maxDL;
      if (mastering_minDL != NULL) *mastering_minDL = info_ptr->mastering_minDL;
      return CI_INFO_mDCV;
   }

   return 0;
}
#  endif

#  ifdef CI_FLOATING_POINT_SUPPORTED
ci_uint_32 CIAPI
ci_get_mDCV(ci_const_structrp ci_ptr, ci_const_inforp info_ptr,
    double *white_x, double *white_y, double *red_x, double *red_y,
    double *green_x, double *green_y, double *blue_x, double *blue_y,
    double *mastering_maxDL, double *mastering_minDL)
{
   ci_debug1(1, "in %s retrieval function", "mDCV(float)");

   if (ci_ptr != NULL && info_ptr != NULL &&
       (info_ptr->valid & CI_INFO_mDCV) != 0)
   {
      if (white_x != NULL) *white_x = info_ptr->mastering_white_x * .00002;
      if (white_y != NULL) *white_y = info_ptr->mastering_white_y * .00002;
      if (red_x != NULL) *red_x = info_ptr->mastering_red_x * .00002;
      if (red_y != NULL) *red_y = info_ptr->mastering_red_y * .00002;
      if (green_x != NULL) *green_x = info_ptr->mastering_green_x * .00002;
      if (green_y != NULL) *green_y = info_ptr->mastering_green_y * .00002;
      if (blue_x != NULL) *blue_x = info_ptr->mastering_blue_x * .00002;
      if (blue_y != NULL) *blue_y = info_ptr->mastering_blue_y * .00002;
      if (mastering_maxDL != NULL)
         *mastering_maxDL = info_ptr->mastering_maxDL * .0001;
      if (mastering_minDL != NULL)
         *mastering_minDL = info_ptr->mastering_minDL * .0001;
      return CI_INFO_mDCV;
   }

   return 0;
}
#  endif /* FLOATING_POINT */
#endif /* mDCV */

#ifdef CI_eXIf_SUPPORTED
ci_uint_32 CIAPI
ci_get_eXIf(ci_const_structrp ci_ptr, ci_inforp info_ptr,
    ci_bytep *exif)
{
  ci_warning(ci_ptr, "ci_get_eXIf does not work; use ci_get_eXIf_1");
  CI_UNUSED(info_ptr)
  CI_UNUSED(exif)
  return 0;
}

ci_uint_32 CIAPI
ci_get_eXIf_1(ci_const_structrp ci_ptr, ci_const_inforp info_ptr,
    ci_uint_32 *num_exif, ci_bytep *exif)
{
   ci_debug1(1, "in %s retrieval function", "eXIf");

   if (ci_ptr != NULL && info_ptr != NULL &&
       (info_ptr->valid & CI_INFO_eXIf) != 0 && exif != NULL)
   {
      *num_exif = info_ptr->num_exif;
      *exif = info_ptr->exif;
      return CI_INFO_eXIf;
   }

   return 0;
}
#endif

#ifdef CI_hIST_SUPPORTED
ci_uint_32 CIAPI
ci_get_hIST(ci_const_structrp ci_ptr, ci_inforp info_ptr,
    ci_uint_16p *hist)
{
   ci_debug1(1, "in %s retrieval function", "hIST");

   if (ci_ptr != NULL && info_ptr != NULL &&
       (info_ptr->valid & CI_INFO_hIST) != 0 && hist != NULL)
   {
      *hist = info_ptr->hist;
      return CI_INFO_hIST;
   }

   return 0;
}
#endif

ci_uint_32 CIAPI
ci_get_IHDR(ci_const_structrp ci_ptr, ci_const_inforp info_ptr,
    ci_uint_32 *width, ci_uint_32 *height, int *bit_depth,
    int *color_type, int *interlace_type, int *compression_type,
    int *filter_type)
{
   ci_debug1(1, "in %s retrieval function", "IHDR");

   if (ci_ptr == NULL || info_ptr == NULL)
      return 0;

   if (width != NULL)
       *width = info_ptr->width;

   if (height != NULL)
       *height = info_ptr->height;

   if (bit_depth != NULL)
       *bit_depth = info_ptr->bit_depth;

   if (color_type != NULL)
       *color_type = info_ptr->color_type;

   if (compression_type != NULL)
      *compression_type = info_ptr->compression_type;

   if (filter_type != NULL)
      *filter_type = info_ptr->filter_type;

   if (interlace_type != NULL)
      *interlace_type = info_ptr->interlace_type;

   /* This is redundant if we can be sure that the info_ptr values were all
    * assigned in ci_set_IHDR().  We do the check anyhow in case an
    * application has ignored our advice not to mess with the members
    * of info_ptr directly.
    */
   ci_check_IHDR(ci_ptr, info_ptr->width, info_ptr->height,
       info_ptr->bit_depth, info_ptr->color_type, info_ptr->interlace_type,
       info_ptr->compression_type, info_ptr->filter_type);

   return 1;
}

#ifdef CI_oFFs_SUPPORTED
ci_uint_32 CIAPI
ci_get_oFFs(ci_const_structrp ci_ptr, ci_const_inforp info_ptr,
    ci_int_32 *offset_x, ci_int_32 *offset_y, int *unit_type)
{
   ci_debug1(1, "in %s retrieval function", "oFFs");

   if (ci_ptr != NULL && info_ptr != NULL &&
       (info_ptr->valid & CI_INFO_oFFs) != 0 &&
       offset_x != NULL && offset_y != NULL && unit_type != NULL)
   {
      *offset_x = info_ptr->x_offset;
      *offset_y = info_ptr->y_offset;
      *unit_type = (int)info_ptr->offset_unit_type;
      return CI_INFO_oFFs;
   }

   return 0;
}
#endif

#ifdef CI_pCAL_SUPPORTED
ci_uint_32 CIAPI
ci_get_pCAL(ci_const_structrp ci_ptr, ci_inforp info_ptr,
    ci_charp *purpose, ci_int_32 *X0, ci_int_32 *X1, int *type, int *nparams,
    ci_charp *units, ci_charpp *params)
{
   ci_debug1(1, "in %s retrieval function", "pCAL");

   if (ci_ptr != NULL && info_ptr != NULL &&
       (info_ptr->valid & CI_INFO_pCAL) != 0 &&
       purpose != NULL && X0 != NULL && X1 != NULL && type != NULL &&
       nparams != NULL && units != NULL && params != NULL)
   {
      *purpose = info_ptr->pcal_purpose;
      *X0 = info_ptr->pcal_X0;
      *X1 = info_ptr->pcal_X1;
      *type = (int)info_ptr->pcal_type;
      *nparams = (int)info_ptr->pcal_nparams;
      *units = info_ptr->pcal_units;
      *params = info_ptr->pcal_params;
      return CI_INFO_pCAL;
   }

   return 0;
}
#endif

#ifdef CI_sCAL_SUPPORTED
#  ifdef CI_FIXED_POINT_SUPPORTED
#    if defined(CI_FLOATING_ARITHMETIC_SUPPORTED) || \
         defined(CI_FLOATING_POINT_SUPPORTED)
ci_uint_32 CIAPI
ci_get_sCAL_fixed(ci_const_structrp ci_ptr, ci_const_inforp info_ptr,
    int *unit, ci_fixed_point *width, ci_fixed_point *height)
{
   ci_debug1(1, "in %s retrieval function", "sCAL");

   if (ci_ptr != NULL && info_ptr != NULL &&
       (info_ptr->valid & CI_INFO_sCAL) != 0)
   {
      *unit = info_ptr->scal_unit;
      /*TODO: make this work without FP support; the API is currently eliminated
       * if neither floating point APIs nor internal floating point arithmetic
       * are enabled.
       */
      *width = ci_fixed(ci_ptr, atof(info_ptr->scal_s_width), "sCAL width");
      *height = ci_fixed(ci_ptr, atof(info_ptr->scal_s_height),
          "sCAL height");
      return CI_INFO_sCAL;
   }

   return 0;
}
#    endif /* FLOATING_ARITHMETIC */
#  endif /* FIXED_POINT */
#  ifdef CI_FLOATING_POINT_SUPPORTED
ci_uint_32 CIAPI
ci_get_sCAL(ci_const_structrp ci_ptr, ci_const_inforp info_ptr,
    int *unit, double *width, double *height)
{
   ci_debug1(1, "in %s retrieval function", "sCAL(float)");

   if (ci_ptr != NULL && info_ptr != NULL &&
       (info_ptr->valid & CI_INFO_sCAL) != 0)
   {
      *unit = info_ptr->scal_unit;
      *width = atof(info_ptr->scal_s_width);
      *height = atof(info_ptr->scal_s_height);
      return CI_INFO_sCAL;
   }

   return 0;
}
#  endif /* FLOATING POINT */
ci_uint_32 CIAPI
ci_get_sCAL_s(ci_const_structrp ci_ptr, ci_const_inforp info_ptr,
    int *unit, ci_charpp width, ci_charpp height)
{
   ci_debug1(1, "in %s retrieval function", "sCAL(str)");

   if (ci_ptr != NULL && info_ptr != NULL &&
       (info_ptr->valid & CI_INFO_sCAL) != 0)
   {
      *unit = info_ptr->scal_unit;
      *width = info_ptr->scal_s_width;
      *height = info_ptr->scal_s_height;
      return CI_INFO_sCAL;
   }

   return 0;
}
#endif /* sCAL */

#ifdef CI_pHYs_SUPPORTED
ci_uint_32 CIAPI
ci_get_pHYs(ci_const_structrp ci_ptr, ci_const_inforp info_ptr,
    ci_uint_32 *res_x, ci_uint_32 *res_y, int *unit_type)
{
   ci_uint_32 retval = 0;

   ci_debug1(1, "in %s retrieval function", "pHYs");

   if (ci_ptr != NULL && info_ptr != NULL &&
       (info_ptr->valid & CI_INFO_pHYs) != 0)
   {
      if (res_x != NULL)
      {
         *res_x = info_ptr->x_pixels_per_unit;
         retval |= CI_INFO_pHYs;
      }

      if (res_y != NULL)
      {
         *res_y = info_ptr->y_pixels_per_unit;
         retval |= CI_INFO_pHYs;
      }

      if (unit_type != NULL)
      {
         *unit_type = (int)info_ptr->phys_unit_type;
         retval |= CI_INFO_pHYs;
      }
   }

   return retval;
}
#endif /* pHYs */

ci_uint_32 CIAPI
ci_get_PLTE(ci_const_structrp ci_ptr, ci_inforp info_ptr,
    ci_colorp *palette, int *num_palette)
{
   ci_debug1(1, "in %s retrieval function", "PLTE");

   if (ci_ptr != NULL && info_ptr != NULL &&
       (info_ptr->valid & CI_INFO_PLTE) != 0 && palette != NULL)
   {
      *palette = info_ptr->palette;
      *num_palette = info_ptr->num_palette;
      ci_debug1(3, "num_palette = %d", *num_palette);
      return CI_INFO_PLTE;
   }

   return 0;
}

#ifdef CI_sBIT_SUPPORTED
ci_uint_32 CIAPI
ci_get_sBIT(ci_const_structrp ci_ptr, ci_inforp info_ptr,
    ci_color_8p *sig_bit)
{
   ci_debug1(1, "in %s retrieval function", "sBIT");

   if (ci_ptr != NULL && info_ptr != NULL &&
       (info_ptr->valid & CI_INFO_sBIT) != 0 && sig_bit != NULL)
   {
      *sig_bit = &(info_ptr->sig_bit);
      return CI_INFO_sBIT;
   }

   return 0;
}
#endif

#ifdef CI_TEXT_SUPPORTED
int CIAPI
ci_get_text(ci_const_structrp ci_ptr, ci_inforp info_ptr,
    ci_textp *text_ptr, int *num_text)
{
   if (ci_ptr != NULL && info_ptr != NULL && info_ptr->num_text > 0)
   {
      ci_debug1(1, "in text retrieval function, chunk typeid = 0x%lx",
         (unsigned long)ci_ptr->chunk_name);

      if (text_ptr != NULL)
         *text_ptr = info_ptr->text;

      if (num_text != NULL)
         *num_text = info_ptr->num_text;

      return info_ptr->num_text;
   }

   if (num_text != NULL)
      *num_text = 0;

   return 0;
}
#endif

#ifdef CI_tIME_SUPPORTED
ci_uint_32 CIAPI
ci_get_tIME(ci_const_structrp ci_ptr, ci_inforp info_ptr,
    ci_timep *mod_time)
{
   ci_debug1(1, "in %s retrieval function", "tIME");

   if (ci_ptr != NULL && info_ptr != NULL &&
       (info_ptr->valid & CI_INFO_tIME) != 0 && mod_time != NULL)
   {
      *mod_time = &(info_ptr->mod_time);
      return CI_INFO_tIME;
   }

   return 0;
}
#endif

#ifdef CI_tRNS_SUPPORTED
ci_uint_32 CIAPI
ci_get_tRNS(ci_const_structrp ci_ptr, ci_inforp info_ptr,
    ci_bytep *trans_alpha, int *num_trans, ci_color_16p *trans_color)
{
   ci_uint_32 retval = 0;

   ci_debug1(1, "in %s retrieval function", "tRNS");

   if (ci_ptr != NULL && info_ptr != NULL &&
       (info_ptr->valid & CI_INFO_tRNS) != 0)
   {
      if (info_ptr->color_type == CI_COLOR_TYPE_PALETTE)
      {
         if (trans_alpha != NULL)
         {
            *trans_alpha = info_ptr->trans_alpha;
            retval |= CI_INFO_tRNS;
         }

         if (trans_color != NULL)
            *trans_color = &(info_ptr->trans_color);
      }

      else /* if (info_ptr->color_type != CI_COLOR_TYPE_PALETTE) */
      {
         if (trans_color != NULL)
         {
            *trans_color = &(info_ptr->trans_color);
            retval |= CI_INFO_tRNS;
         }

         if (trans_alpha != NULL)
            *trans_alpha = NULL;
      }

      if (num_trans != NULL)
      {
         *num_trans = info_ptr->num_trans;
         retval |= CI_INFO_tRNS;
      }
   }

   return retval;
}
#endif

#ifdef CI_STORE_UNKNOWN_CHUNKS_SUPPORTED
int CIAPI
ci_get_unknown_chunks(ci_const_structrp ci_ptr, ci_inforp info_ptr,
    ci_unknown_chunkpp unknowns)
{
   if (ci_ptr != NULL && info_ptr != NULL && unknowns != NULL)
   {
      *unknowns = info_ptr->unknown_chunks;
      return info_ptr->unknown_chunks_num;
   }

   return 0;
}
#endif

#ifdef CI_READ_RGB_TO_GRAY_SUPPORTED
ci_byte CIAPI
ci_get_rgb_to_gray_status(ci_const_structrp ci_ptr)
{
   return (ci_byte)(ci_ptr ? ci_ptr->rgb_to_gray_status : 0);
}
#endif

#ifdef CI_USER_CHUNKS_SUPPORTED
ci_voidp CIAPI
ci_get_user_chunk_ptr(ci_const_structrp ci_ptr)
{
   return (ci_ptr ? ci_ptr->user_chunk_ptr : NULL);
}
#endif

size_t CIAPI
ci_get_compression_buffer_size(ci_const_structrp ci_ptr)
{
   if (ci_ptr == NULL)
      return 0;

#ifdef CI_WRITE_SUPPORTED
   if ((ci_ptr->mode & CI_IS_READ_STRUCT) != 0)
#endif
   {
#ifdef CI_SEQUENTIAL_READ_SUPPORTED
      return ci_ptr->IDAT_read_size;
#else
      return CI_IDAT_READ_SIZE;
#endif
   }

#ifdef CI_WRITE_SUPPORTED
   else
      return ci_ptr->zbuffer_size;
#endif
}

#ifdef CI_SET_USER_LIMITS_SUPPORTED
/* These functions were added to libci 1.2.6 and were enabled
 * by default in libci-1.4.0 */
ci_uint_32 CIAPI
ci_get_user_width_max(ci_const_structrp ci_ptr)
{
   return (ci_ptr ? ci_ptr->user_width_max : 0);
}

ci_uint_32 CIAPI
ci_get_user_height_max(ci_const_structrp ci_ptr)
{
   return (ci_ptr ? ci_ptr->user_height_max : 0);
}

/* This function was added to libci 1.4.0 */
ci_uint_32 CIAPI
ci_get_chunk_cache_max(ci_const_structrp ci_ptr)
{
   return (ci_ptr ? ci_ptr->user_chunk_cache_max : 0);
}

/* This function was added to libci 1.4.1 */
ci_alloc_size_t CIAPI
ci_get_chunk_malloc_max(ci_const_structrp ci_ptr)
{
   return (ci_ptr ? ci_ptr->user_chunk_malloc_max : 0);
}
#endif /* SET_USER_LIMITS */

/* These functions were added to libci 1.4.0 */
#ifdef CI_IO_STATE_SUPPORTED
ci_uint_32 CIAPI
ci_get_io_state(ci_const_structrp ci_ptr)
{
   return ci_ptr->io_state;
}

ci_uint_32 CIAPI
ci_get_io_chunk_type(ci_const_structrp ci_ptr)
{
   return ci_ptr->chunk_name;
}
#endif /* IO_STATE */

#ifdef CI_CHECK_FOR_INVALID_INDEX_SUPPORTED
#  ifdef CI_GET_PALETTE_MAX_SUPPORTED
int CIAPI
ci_get_palette_max(ci_const_structp ci_ptr, ci_const_infop info_ptr)
{
   if (ci_ptr != NULL && info_ptr != NULL)
      return ci_ptr->num_palette_max;

   return -1;
}
#  endif
#endif

#endif /* READ || WRITE */

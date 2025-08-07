/* cirtran.c - transforms the data in a row for CI readers
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
 * This file contains functions optionally called by an application
 * in order to tell libci how to handle data when reading a CI.
 * Transformations that are used in both reading and writing are
 * in citrans.c.
 */

#include "cipriv.h"

#ifdef CI_ARM_NEON_IMPLEMENTATION
#  if CI_ARM_NEON_IMPLEMENTATION == 1
#    define CI_ARM_NEON_INTRINSICS_AVAILABLE
#    if defined(_MSC_VER) && !defined(__clang__) && defined(_M_ARM64)
#      include <arm64_neon.h>
#    else
#      include <arm_neon.h>
#    endif
#  endif
#endif

#ifdef CI_RISCV_RVV_IMPLEMENTATION
#  if CI_RISCV_RVV_IMPLEMENTATION == 1
#    define CI_RISCV_RVV_INTRINSICS_AVAILABLE
#  endif
#endif

#ifdef CI_READ_SUPPORTED

/* Set the action on getting a CRC error for an ancillary or critical chunk. */
void CIAPI
ci_set_crc_action(ci_structrp ci_ptr, int crit_action, int ancil_action)
{
   ci_debug(1, "in ci_set_crc_action");

   if (ci_ptr == NULL)
      return;

   /* Tell libci how we react to CRC errors in critical chunks */
   switch (crit_action)
   {
      case CI_CRC_NO_CHANGE:                        /* Leave setting as is */
         break;

      case CI_CRC_WARN_USE:                               /* Warn/use data */
         ci_ptr->flags &= ~CI_FLAG_CRC_CRITICAL_MASK;
         ci_ptr->flags |= CI_FLAG_CRC_CRITICAL_USE;
         break;

      case CI_CRC_QUIET_USE:                             /* Quiet/use data */
         ci_ptr->flags &= ~CI_FLAG_CRC_CRITICAL_MASK;
         ci_ptr->flags |= CI_FLAG_CRC_CRITICAL_USE |
                           CI_FLAG_CRC_CRITICAL_IGNORE;
         break;

      case CI_CRC_WARN_DISCARD:    /* Not a valid action for critical data */
         ci_warning(ci_ptr,
             "Can't discard critical data on CRC error");
         /* FALLTHROUGH */
      case CI_CRC_ERROR_QUIT:                                /* Error/quit */

      case CI_CRC_DEFAULT:
      default:
         ci_ptr->flags &= ~CI_FLAG_CRC_CRITICAL_MASK;
         break;
   }

   /* Tell libci how we react to CRC errors in ancillary chunks */
   switch (ancil_action)
   {
      case CI_CRC_NO_CHANGE:                       /* Leave setting as is */
         break;

      case CI_CRC_WARN_USE:                              /* Warn/use data */
         ci_ptr->flags &= ~CI_FLAG_CRC_ANCILLARY_MASK;
         ci_ptr->flags |= CI_FLAG_CRC_ANCILLARY_USE;
         break;

      case CI_CRC_QUIET_USE:                            /* Quiet/use data */
         ci_ptr->flags &= ~CI_FLAG_CRC_ANCILLARY_MASK;
         ci_ptr->flags |= CI_FLAG_CRC_ANCILLARY_USE |
                           CI_FLAG_CRC_ANCILLARY_NOWARN;
         break;

      case CI_CRC_ERROR_QUIT:                               /* Error/quit */
         ci_ptr->flags &= ~CI_FLAG_CRC_ANCILLARY_MASK;
         ci_ptr->flags |= CI_FLAG_CRC_ANCILLARY_NOWARN;
         break;

      case CI_CRC_WARN_DISCARD:                      /* Warn/discard data */

      case CI_CRC_DEFAULT:
      default:
         ci_ptr->flags &= ~CI_FLAG_CRC_ANCILLARY_MASK;
         break;
   }
}

#ifdef CI_READ_TRANSFORMS_SUPPORTED
/* Is it OK to set a transformation now?  Only if ci_start_read_image or
 * ci_read_update_info have not been called.  It is not necessary for the IHDR
 * to have been read in all cases; the need_IHDR parameter allows for this
 * check too.
 */
static int
ci_rtran_ok(ci_structrp ci_ptr, int need_IHDR)
{
   if (ci_ptr != NULL)
   {
      if ((ci_ptr->flags & CI_FLAG_ROW_INIT) != 0)
         ci_app_error(ci_ptr,
             "invalid after ci_start_read_image or ci_read_update_info");

      else if (need_IHDR && (ci_ptr->mode & CI_HAVE_IHDR) == 0)
         ci_app_error(ci_ptr, "invalid before the CI header has been read");

      else
      {
         /* Turn on failure to initialize correctly for all transforms. */
         ci_ptr->flags |= CI_FLAG_DETECT_UNINITIALIZED;

         return 1; /* Ok */
      }
   }

   return 0; /* no ci_error possible! */
}
#endif

#ifdef CI_READ_BACKGROUND_SUPPORTED
/* Handle alpha and tRNS via a background color */
void CIFAPI
ci_set_background_fixed(ci_structrp ci_ptr,
    ci_const_color_16p background_color, int background_gamma_code,
    int need_expand, ci_fixed_point background_gamma)
{
   ci_debug(1, "in ci_set_background_fixed");

   if (ci_rtran_ok(ci_ptr, 0) == 0 || background_color == NULL)
      return;

   if (background_gamma_code == CI_BACKGROUND_GAMMA_UNKNOWN)
   {
      ci_warning(ci_ptr, "Application must supply a known background gamma");
      return;
   }

   ci_ptr->transformations |= CI_COMPOSE | CI_STRIP_ALPHA;
   ci_ptr->transformations &= ~CI_ENCODE_ALPHA;
   ci_ptr->flags &= ~CI_FLAG_OPTIMIZE_ALPHA;

   ci_ptr->background = *background_color;
   ci_ptr->background_gamma = background_gamma;
   ci_ptr->background_gamma_type = (ci_byte)(background_gamma_code);
   if (need_expand != 0)
      ci_ptr->transformations |= CI_BACKGROUND_EXPAND;
   else
      ci_ptr->transformations &= ~CI_BACKGROUND_EXPAND;
}

#  ifdef CI_FLOATING_POINT_SUPPORTED
void CIAPI
ci_set_background(ci_structrp ci_ptr,
    ci_const_color_16p background_color, int background_gamma_code,
    int need_expand, double background_gamma)
{
   ci_set_background_fixed(ci_ptr, background_color, background_gamma_code,
      need_expand, ci_fixed(ci_ptr, background_gamma, "ci_set_background"));
}
#  endif /* FLOATING_POINT */
#endif /* READ_BACKGROUND */

/* Scale 16-bit depth files to 8-bit depth.  If both of these are set then the
 * one that cirtran does first (scale) happens.  This is necessary to allow the
 * TRANSFORM and API behavior to be somewhat consistent, and it's simpler.
 */
#ifdef CI_READ_SCALE_16_TO_8_SUPPORTED
void CIAPI
ci_set_scale_16(ci_structrp ci_ptr)
{
   ci_debug(1, "in ci_set_scale_16");

   if (ci_rtran_ok(ci_ptr, 0) == 0)
      return;

   ci_ptr->transformations |= CI_SCALE_16_TO_8;
}
#endif

#ifdef CI_READ_STRIP_16_TO_8_SUPPORTED
/* Chop 16-bit depth files to 8-bit depth */
void CIAPI
ci_set_strip_16(ci_structrp ci_ptr)
{
   ci_debug(1, "in ci_set_strip_16");

   if (ci_rtran_ok(ci_ptr, 0) == 0)
      return;

   ci_ptr->transformations |= CI_16_TO_8;
}
#endif

#ifdef CI_READ_STRIP_ALPHA_SUPPORTED
void CIAPI
ci_set_strip_alpha(ci_structrp ci_ptr)
{
   ci_debug(1, "in ci_set_strip_alpha");

   if (ci_rtran_ok(ci_ptr, 0) == 0)
      return;

   ci_ptr->transformations |= CI_STRIP_ALPHA;
}
#endif

#if defined(CI_READ_ALPHA_MODE_SUPPORTED) || defined(CI_READ_GAMMA_SUPPORTED)
/* CIv3 conformance: this private API exists to resolve the now mandatory error
 * resolution when multiple conflicting sources of gamma or colour space
 * information are available.
 *
 * Terminology (assuming power law, "gamma", encodings):
 *    "screen" gamma: a power law imposed by the output device when digital
 *    samples are converted to visible light output.  The EOTF - volage to
 *    luminance on output.
 *
 *    "file" gamma: a power law used to encode luminance levels from the input
 *    data (the scene or the mastering display system) into digital voltages.
 *    The OETF - luminance to voltage on input.
 *
 *    gamma "correction": a power law matching the **inverse** of the overall
 *    transfer function from input luminance levels to output levels.  The
 *    **inverse** of the OOTF; the correction "corrects" for the OOTF by aiming
 *    to make the overall OOTF (including the correction) linear.
 *
 * It is important to understand this terminology because the defined terms are
 * scattered throughout the libci code and it is very easy to end up with the
 * inverse of the power law required.
 *
 * Variable and struct::member names:
 *    file_gamma        OETF  how the CI data was encoded
 *
 *    screen_gamma      EOTF  how the screen will decode digital levels
 *
 *    -- not used --    OOTF  the net effect OETF x EOTF
 *    gamma_correction        the inverse of OOTF to make the result linear
 *
 * All versions of libci require a call to "ci_set_gamma" to establish the
 * "screen" gamma, the power law representing the EOTF.  ci_set_gamma may also
 * set or default the "file" gamma; the OETF.  gamma_correction is calculated
 * internally.
 *
 * The earliest libci versions required file_gamma to be supplied to set_gamma.
 * Later versions started allowing ci_set_gamma and, later, ci_set_alpha_mode,
 * to cause defaulting from the file data.
 *
 * CIv3 mandated a particular form for this defaulting, one that is compatible
 * with what libci did except that if libci detected inconsistencies it marked
 * all the chunks as "invalid".  CIv3 effectively invalidates this prior code.
 *
 * Behaviour implemented below:
 *    translate_gamma_flags(gamma, is_screen)
 *       The libci-1.6 API for the gamma parameters to libci APIs
 *       (ci_set_gamma and ci_set_alpha_mode at present).  This allows the
 *       'gamma' value to be passed as a ci_fixed_point number or as one of a
 *       set of integral values for specific "well known" examples of transfer
 *       functions.  This is compatible with CIv3.
 */
static ci_fixed_point
translate_gamma_flags(ci_fixed_point output_gamma, int is_screen)
{
   /* Check for flag values.  The main reason for having the old Mac value as a
    * flag is that it is pretty near impossible to work out what the correct
    * value is from Apple documentation - a working Mac system is needed to
    * discover the value!
    */
   if (output_gamma == CI_DEFAULT_sRGB ||
      output_gamma == CI_FP_1 / CI_DEFAULT_sRGB)
   {
      if (is_screen != 0)
         output_gamma = CI_GAMMA_sRGB;
      else
         output_gamma = CI_GAMMA_sRGB_INVERSE;
   }

   else if (output_gamma == CI_GAMMA_MAC_18 ||
      output_gamma == CI_FP_1 / CI_GAMMA_MAC_18)
   {
      if (is_screen != 0)
         output_gamma = CI_GAMMA_MAC_OLD;
      else
         output_gamma = CI_GAMMA_MAC_INVERSE;
   }

   return output_gamma;
}

#  ifdef CI_FLOATING_POINT_SUPPORTED
static ci_fixed_point
convert_gamma_value(ci_structrp ci_ptr, double output_gamma)
{
   /* The following silently ignores cases where fixed point (times 100,000)
    * gamma values are passed to the floating point API.  This is safe and it
    * means the fixed point constants work just fine with the floating point
    * API.  The alternative would just lead to undetected errors and spurious
    * bug reports.  Negative values fail inside the _fixed API unless they
    * correspond to the flag values.
    */
   if (output_gamma > 0 && output_gamma < 128)
      output_gamma *= CI_FP_1;

   /* This preserves -1 and -2 exactly: */
   output_gamma = floor(output_gamma + .5);

   if (output_gamma > CI_FP_MAX || output_gamma < CI_FP_MIN)
      ci_fixed_error(ci_ptr, "gamma value");

   return (ci_fixed_point)output_gamma;
}
#  endif

static int
unsupported_gamma(ci_structrp ci_ptr, ci_fixed_point gamma, int warn)
{
   /* Validate a gamma value to ensure it is in a reasonable range.  The value
    * is expected to be 1 or greater, but this range test allows for some
    * viewing correction values.  The intent is to weed out the API users
    * who might use the inverse of the gamma value accidentally!
    *
    * 1.6.47: apply the test in ci_set_gamma as well but only warn and return
    * false if it fires.
    *
    * TODO: 1.8: make this an app_error in ci_set_gamma as well.
    */
   if (gamma < CI_LIB_GAMMA_MIN || gamma > CI_LIB_GAMMA_MAX)
   {
#     define msg "gamma out of supported range"
      if (warn)
         ci_app_warning(ci_ptr, msg);
      else
         ci_app_error(ci_ptr, msg);
      return 1;
#     undef msg
   }

   return 0;
}
#endif /* READ_ALPHA_MODE || READ_GAMMA */

#ifdef CI_READ_ALPHA_MODE_SUPPORTED
void CIFAPI
ci_set_alpha_mode_fixed(ci_structrp ci_ptr, int mode,
    ci_fixed_point output_gamma)
{
   ci_fixed_point file_gamma;
   int compose = 0;

   ci_debug(1, "in ci_set_alpha_mode_fixed");

   if (ci_rtran_ok(ci_ptr, 0) == 0)
      return;

   output_gamma = translate_gamma_flags(output_gamma, 1/*screen*/);
   if (unsupported_gamma(ci_ptr, output_gamma, 0/*error*/))
      return;

   /* The default file gamma is the inverse of the output gamma; the output
    * gamma may be changed below so get the file value first.  The default_gamma
    * is set here and from the simplified API (which uses a different algorithm)
    * so don't overwrite a set value:
    */
   file_gamma = ci_ptr->default_gamma;
   if (file_gamma == 0)
   {
      file_gamma = ci_reciprocal(output_gamma);
      ci_ptr->default_gamma = file_gamma;
   }

   /* There are really 8 possibilities here, composed of any combination
    * of:
    *
    *    premultiply the color channels
    *    do not encode non-opaque pixels
    *    encode the alpha as well as the color channels
    *
    * The differences disappear if the input/output ('screen') gamma is 1.0,
    * because then the encoding is a no-op and there is only the choice of
    * premultiplying the color channels or not.
    *
    * ci_set_alpha_mode and ci_set_background interact because both use
    * ci_compose to do the work.  Calling both is only useful when
    * ci_set_alpha_mode is used to set the default mode - CI_ALPHA_CI - along
    * with a default gamma value.  Otherwise CI_COMPOSE must not be set.
    */
   switch (mode)
   {
      case CI_ALPHA_CI:        /* default: ci standard */
         /* No compose, but it may be set by ci_set_background! */
         ci_ptr->transformations &= ~CI_ENCODE_ALPHA;
         ci_ptr->flags &= ~CI_FLAG_OPTIMIZE_ALPHA;
         break;

      case CI_ALPHA_ASSOCIATED: /* color channels premultiplied */
         compose = 1;
         ci_ptr->transformations &= ~CI_ENCODE_ALPHA;
         ci_ptr->flags &= ~CI_FLAG_OPTIMIZE_ALPHA;
         /* The output is linear: */
         output_gamma = CI_FP_1;
         break;

      case CI_ALPHA_OPTIMIZED:  /* associated, non-opaque pixels linear */
         compose = 1;
         ci_ptr->transformations &= ~CI_ENCODE_ALPHA;
         ci_ptr->flags |= CI_FLAG_OPTIMIZE_ALPHA;
         /* output_gamma records the encoding of opaque pixels! */
         break;

      case CI_ALPHA_BROKEN:     /* associated, non-linear, alpha encoded */
         compose = 1;
         ci_ptr->transformations |= CI_ENCODE_ALPHA;
         ci_ptr->flags &= ~CI_FLAG_OPTIMIZE_ALPHA;
         break;

      default:
         ci_error(ci_ptr, "invalid alpha mode");
   }

   /* Set the screen gamma values: */
   ci_ptr->screen_gamma = output_gamma;

   /* Finally, if pre-multiplying, set the background fields to achieve the
    * desired result.
    */
   if (compose != 0)
   {
      /* And obtain alpha pre-multiplication by composing on black: */
      memset(&ci_ptr->background, 0, (sizeof ci_ptr->background));
      ci_ptr->background_gamma = file_gamma; /* just in case */
      ci_ptr->background_gamma_type = CI_BACKGROUND_GAMMA_FILE;
      ci_ptr->transformations &= ~CI_BACKGROUND_EXPAND;

      if ((ci_ptr->transformations & CI_COMPOSE) != 0)
         ci_error(ci_ptr,
             "conflicting calls to set alpha mode and background");

      ci_ptr->transformations |= CI_COMPOSE;
   }
}

#  ifdef CI_FLOATING_POINT_SUPPORTED
void CIAPI
ci_set_alpha_mode(ci_structrp ci_ptr, int mode, double output_gamma)
{
   ci_set_alpha_mode_fixed(ci_ptr, mode, convert_gamma_value(ci_ptr,
       output_gamma));
}
#  endif
#endif

#ifdef CI_READ_QUANTIZE_SUPPORTED
/* Dither file to 8-bit.  Supply a palette, the current number
 * of elements in the palette, the maximum number of elements
 * allowed, and a histogram if possible.  If the current number
 * of colors is greater than the maximum number, the palette will be
 * modified to fit in the maximum number.  "full_quantize" indicates
 * whether we need a quantizing cube set up for RGB images, or if we
 * simply are reducing the number of colors in a paletted image.
 */

typedef struct ci_dsort_struct
{
   struct ci_dsort_struct * next;
   ci_byte left;
   ci_byte right;
} ci_dsort;
typedef ci_dsort *   ci_dsortp;
typedef ci_dsort * * ci_dsortpp;

void CIAPI
ci_set_quantize(ci_structrp ci_ptr, ci_colorp palette,
    int num_palette, int maximum_colors, ci_const_uint_16p histogram,
    int full_quantize)
{
   ci_debug(1, "in ci_set_quantize");

   if (ci_rtran_ok(ci_ptr, 0) == 0)
      return;

   ci_ptr->transformations |= CI_QUANTIZE;

   if (full_quantize == 0)
   {
      int i;

      ci_ptr->quantize_index = (ci_bytep)ci_malloc(ci_ptr,
          (ci_alloc_size_t)num_palette);
      for (i = 0; i < num_palette; i++)
         ci_ptr->quantize_index[i] = (ci_byte)i;
   }

   if (num_palette > maximum_colors)
   {
      if (histogram != NULL)
      {
         /* This is easy enough, just throw out the least used colors.
          * Perhaps not the best solution, but good enough.
          */

         int i;

         /* Initialize an array to sort colors */
         ci_ptr->quantize_sort = (ci_bytep)ci_malloc(ci_ptr,
             (ci_alloc_size_t)num_palette);

         /* Initialize the quantize_sort array */
         for (i = 0; i < num_palette; i++)
            ci_ptr->quantize_sort[i] = (ci_byte)i;

         /* Find the least used palette entries by starting a
          * bubble sort, and running it until we have sorted
          * out enough colors.  Note that we don't care about
          * sorting all the colors, just finding which are
          * least used.
          */

         for (i = num_palette - 1; i >= maximum_colors; i--)
         {
            int done; /* To stop early if the list is pre-sorted */
            int j;

            done = 1;
            for (j = 0; j < i; j++)
            {
               if (histogram[ci_ptr->quantize_sort[j]]
                   < histogram[ci_ptr->quantize_sort[j + 1]])
               {
                  ci_byte t;

                  t = ci_ptr->quantize_sort[j];
                  ci_ptr->quantize_sort[j] = ci_ptr->quantize_sort[j + 1];
                  ci_ptr->quantize_sort[j + 1] = t;
                  done = 0;
               }
            }

            if (done != 0)
               break;
         }

         /* Swap the palette around, and set up a table, if necessary */
         if (full_quantize != 0)
         {
            int j = num_palette;

            /* Put all the useful colors within the max, but don't
             * move the others.
             */
            for (i = 0; i < maximum_colors; i++)
            {
               if ((int)ci_ptr->quantize_sort[i] >= maximum_colors)
               {
                  do
                     j--;
                  while ((int)ci_ptr->quantize_sort[j] >= maximum_colors);

                  palette[i] = palette[j];
               }
            }
         }
         else
         {
            int j = num_palette;

            /* Move all the used colors inside the max limit, and
             * develop a translation table.
             */
            for (i = 0; i < maximum_colors; i++)
            {
               /* Only move the colors we need to */
               if ((int)ci_ptr->quantize_sort[i] >= maximum_colors)
               {
                  ci_color tmp_color;

                  do
                     j--;
                  while ((int)ci_ptr->quantize_sort[j] >= maximum_colors);

                  tmp_color = palette[j];
                  palette[j] = palette[i];
                  palette[i] = tmp_color;
                  /* Indicate where the color went */
                  ci_ptr->quantize_index[j] = (ci_byte)i;
                  ci_ptr->quantize_index[i] = (ci_byte)j;
               }
            }

            /* Find closest color for those colors we are not using */
            for (i = 0; i < num_palette; i++)
            {
               if ((int)ci_ptr->quantize_index[i] >= maximum_colors)
               {
                  int min_d, k, min_k, d_index;

                  /* Find the closest color to one we threw out */
                  d_index = ci_ptr->quantize_index[i];
                  min_d = CI_COLOR_DIST(palette[d_index], palette[0]);
                  for (k = 1, min_k = 0; k < maximum_colors; k++)
                  {
                     int d;

                     d = CI_COLOR_DIST(palette[d_index], palette[k]);

                     if (d < min_d)
                     {
                        min_d = d;
                        min_k = k;
                     }
                  }
                  /* Point to closest color */
                  ci_ptr->quantize_index[i] = (ci_byte)min_k;
               }
            }
         }
         ci_free(ci_ptr, ci_ptr->quantize_sort);
         ci_ptr->quantize_sort = NULL;
      }
      else
      {
         /* This is much harder to do simply (and quickly).  Perhaps
          * we need to go through a median cut routine, but those
          * don't always behave themselves with only a few colors
          * as input.  So we will just find the closest two colors,
          * and throw out one of them (chosen somewhat randomly).
          * [We don't understand this at all, so if someone wants to
          *  work on improving it, be our guest - AED, GRP]
          */
         int i;
         int max_d;
         int num_new_palette;
         ci_dsortp t;
         ci_dsortpp hash;

         t = NULL;

         /* Initialize palette index arrays */
         ci_ptr->index_to_palette = (ci_bytep)ci_malloc(ci_ptr,
             (ci_alloc_size_t)num_palette);
         ci_ptr->palette_to_index = (ci_bytep)ci_malloc(ci_ptr,
             (ci_alloc_size_t)num_palette);

         /* Initialize the sort array */
         for (i = 0; i < num_palette; i++)
         {
            ci_ptr->index_to_palette[i] = (ci_byte)i;
            ci_ptr->palette_to_index[i] = (ci_byte)i;
         }

         hash = (ci_dsortpp)ci_calloc(ci_ptr, (ci_alloc_size_t)(769 *
             (sizeof (ci_dsortp))));

         num_new_palette = num_palette;

         /* Initial wild guess at how far apart the farthest pixel
          * pair we will be eliminating will be.  Larger
          * numbers mean more areas will be allocated, Smaller
          * numbers run the risk of not saving enough data, and
          * having to do this all over again.
          *
          * I have not done extensive checking on this number.
          */
         max_d = 96;

         while (num_new_palette > maximum_colors)
         {
            for (i = 0; i < num_new_palette - 1; i++)
            {
               int j;

               for (j = i + 1; j < num_new_palette; j++)
               {
                  int d;

                  d = CI_COLOR_DIST(palette[i], palette[j]);

                  if (d <= max_d)
                  {

                     t = (ci_dsortp)ci_malloc_warn(ci_ptr,
                         (ci_alloc_size_t)(sizeof (ci_dsort)));

                     if (t == NULL)
                         break;

                     t->next = hash[d];
                     t->left = (ci_byte)i;
                     t->right = (ci_byte)j;
                     hash[d] = t;
                  }
               }
               if (t == NULL)
                  break;
            }

            if (t != NULL)
            for (i = 0; i <= max_d; i++)
            {
               if (hash[i] != NULL)
               {
                  ci_dsortp p;

                  for (p = hash[i]; p; p = p->next)
                  {
                     if ((int)ci_ptr->index_to_palette[p->left]
                         < num_new_palette &&
                         (int)ci_ptr->index_to_palette[p->right]
                         < num_new_palette)
                     {
                        int j, next_j;

                        if (num_new_palette & 0x01)
                        {
                           j = p->left;
                           next_j = p->right;
                        }
                        else
                        {
                           j = p->right;
                           next_j = p->left;
                        }

                        num_new_palette--;
                        palette[ci_ptr->index_to_palette[j]]
                            = palette[num_new_palette];
                        if (full_quantize == 0)
                        {
                           int k;

                           for (k = 0; k < num_palette; k++)
                           {
                              if (ci_ptr->quantize_index[k] ==
                                  ci_ptr->index_to_palette[j])
                                 ci_ptr->quantize_index[k] =
                                     ci_ptr->index_to_palette[next_j];

                              if ((int)ci_ptr->quantize_index[k] ==
                                  num_new_palette)
                                 ci_ptr->quantize_index[k] =
                                     ci_ptr->index_to_palette[j];
                           }
                        }

                        ci_ptr->index_to_palette[ci_ptr->palette_to_index
                            [num_new_palette]] = ci_ptr->index_to_palette[j];

                        ci_ptr->palette_to_index[ci_ptr->index_to_palette[j]]
                            = ci_ptr->palette_to_index[num_new_palette];

                        ci_ptr->index_to_palette[j] =
                            (ci_byte)num_new_palette;

                        ci_ptr->palette_to_index[num_new_palette] =
                            (ci_byte)j;
                     }
                     if (num_new_palette <= maximum_colors)
                        break;
                  }
                  if (num_new_palette <= maximum_colors)
                     break;
               }
            }

            for (i = 0; i < 769; i++)
            {
               if (hash[i] != NULL)
               {
                  ci_dsortp p = hash[i];
                  while (p)
                  {
                     t = p->next;
                     ci_free(ci_ptr, p);
                     p = t;
                  }
               }
               hash[i] = 0;
            }
            max_d += 96;
         }
         ci_free(ci_ptr, hash);
         ci_free(ci_ptr, ci_ptr->palette_to_index);
         ci_free(ci_ptr, ci_ptr->index_to_palette);
         ci_ptr->palette_to_index = NULL;
         ci_ptr->index_to_palette = NULL;
      }
      num_palette = maximum_colors;
   }
   if (ci_ptr->palette == NULL)
   {
      ci_ptr->palette = palette;
   }
   ci_ptr->num_palette = (ci_uint_16)num_palette;

   if (full_quantize != 0)
   {
      int i;
      ci_bytep distance;
      int total_bits = CI_QUANTIZE_RED_BITS + CI_QUANTIZE_GREEN_BITS +
          CI_QUANTIZE_BLUE_BITS;
      int num_red = (1 << CI_QUANTIZE_RED_BITS);
      int num_green = (1 << CI_QUANTIZE_GREEN_BITS);
      int num_blue = (1 << CI_QUANTIZE_BLUE_BITS);
      size_t num_entries = ((size_t)1 << total_bits);

      ci_ptr->palette_lookup = (ci_bytep)ci_calloc(ci_ptr,
          (ci_alloc_size_t)(num_entries));

      distance = (ci_bytep)ci_malloc(ci_ptr, (ci_alloc_size_t)num_entries);

      memset(distance, 0xff, num_entries);

      for (i = 0; i < num_palette; i++)
      {
         int ir, ig, ib;
         int r = (palette[i].red >> (8 - CI_QUANTIZE_RED_BITS));
         int g = (palette[i].green >> (8 - CI_QUANTIZE_GREEN_BITS));
         int b = (palette[i].blue >> (8 - CI_QUANTIZE_BLUE_BITS));

         for (ir = 0; ir < num_red; ir++)
         {
            /* int dr = abs(ir - r); */
            int dr = ((ir > r) ? ir - r : r - ir);
            int index_r = (ir << (CI_QUANTIZE_BLUE_BITS +
                CI_QUANTIZE_GREEN_BITS));

            for (ig = 0; ig < num_green; ig++)
            {
               /* int dg = abs(ig - g); */
               int dg = ((ig > g) ? ig - g : g - ig);
               int dt = dr + dg;
               int dm = ((dr > dg) ? dr : dg);
               int index_g = index_r | (ig << CI_QUANTIZE_BLUE_BITS);

               for (ib = 0; ib < num_blue; ib++)
               {
                  int d_index = index_g | ib;
                  /* int db = abs(ib - b); */
                  int db = ((ib > b) ? ib - b : b - ib);
                  int dmax = ((dm > db) ? dm : db);
                  int d = dmax + dt + db;

                  if (d < (int)distance[d_index])
                  {
                     distance[d_index] = (ci_byte)d;
                     ci_ptr->palette_lookup[d_index] = (ci_byte)i;
                  }
               }
            }
         }
      }

      ci_free(ci_ptr, distance);
   }
}
#endif /* READ_QUANTIZE */

#ifdef CI_READ_GAMMA_SUPPORTED
void CIFAPI
ci_set_gamma_fixed(ci_structrp ci_ptr, ci_fixed_point scrn_gamma,
    ci_fixed_point file_gamma)
{
   ci_debug(1, "in ci_set_gamma_fixed");

   if (ci_rtran_ok(ci_ptr, 0) == 0)
      return;

   /* New in libci-1.5.4 - reserve particular negative values as flags. */
   scrn_gamma = translate_gamma_flags(scrn_gamma, 1/*screen*/);
   file_gamma = translate_gamma_flags(file_gamma, 0/*file*/);

   /* Checking the gamma values for being >0 was added in 1.5.4 along with the
    * premultiplied alpha support; this actually hides an undocumented feature
    * of the previous implementation which allowed gamma processing to be
    * disabled in background handling.  There is no evidence (so far) that this
    * was being used; however, ci_set_background itself accepted and must still
    * accept '0' for the gamma value it takes, because it isn't always used.
    *
    * Since this is an API change (albeit a very minor one that removes an
    * undocumented API feature) the following checks were only enabled in
    * libci-1.6.0.
    */
   if (file_gamma <= 0)
      ci_app_error(ci_ptr, "invalid file gamma in ci_set_gamma");
   if (scrn_gamma <= 0)
      ci_app_error(ci_ptr, "invalid screen gamma in ci_set_gamma");

   if (unsupported_gamma(ci_ptr, file_gamma, 1/*warn*/) ||
       unsupported_gamma(ci_ptr, scrn_gamma, 1/*warn*/))
      return;

   /* 1.6.47: ci_struct::file_gamma and ci_struct::screen_gamma are now only
    * written by this API.  This removes dependencies on the order of API calls
    * and allows the complex gamma checks to be delayed until needed.
    */
   ci_ptr->file_gamma = file_gamma;
   ci_ptr->screen_gamma = scrn_gamma;
}

#  ifdef CI_FLOATING_POINT_SUPPORTED
void CIAPI
ci_set_gamma(ci_structrp ci_ptr, double scrn_gamma, double file_gamma)
{
   ci_set_gamma_fixed(ci_ptr, convert_gamma_value(ci_ptr, scrn_gamma),
       convert_gamma_value(ci_ptr, file_gamma));
}
#  endif /* FLOATING_POINT */
#endif /* READ_GAMMA */

#ifdef CI_READ_EXPAND_SUPPORTED
/* Expand paletted images to RGB, expand grayscale images of
 * less than 8-bit depth to 8-bit depth, and expand tRNS chunks
 * to alpha channels.
 */
void CIAPI
ci_set_expand(ci_structrp ci_ptr)
{
   ci_debug(1, "in ci_set_expand");

   if (ci_rtran_ok(ci_ptr, 0) == 0)
      return;

   ci_ptr->transformations |= (CI_EXPAND | CI_EXPAND_tRNS);
}

/* GRR 19990627:  the following three functions currently are identical
 *  to ci_set_expand().  However, it is entirely reasonable that someone
 *  might wish to expand an indexed image to RGB but *not* expand a single,
 *  fully transparent palette entry to a full alpha channel--perhaps instead
 *  convert tRNS to the grayscale/RGB format (16-bit RGB value), or replace
 *  the transparent color with a particular RGB value, or drop tRNS entirely.
 *  IOW, a future version of the library may make the transformations flag
 *  a bit more fine-grained, with separate bits for each of these three
 *  functions.
 *
 *  More to the point, these functions make it obvious what libci will be
 *  doing, whereas "expand" can (and does) mean any number of things.
 *
 *  GRP 20060307: In libci-1.2.9, ci_set_gray_1_2_4_to_8() was modified
 *  to expand only the sample depth but not to expand the tRNS to alpha
 *  and its name was changed to ci_set_expand_gray_1_2_4_to_8().
 */

/* Expand paletted images to RGB. */
void CIAPI
ci_set_palette_to_rgb(ci_structrp ci_ptr)
{
   ci_debug(1, "in ci_set_palette_to_rgb");

   if (ci_rtran_ok(ci_ptr, 0) == 0)
      return;

   ci_ptr->transformations |= (CI_EXPAND | CI_EXPAND_tRNS);
}

/* Expand grayscale images of less than 8-bit depth to 8 bits. */
void CIAPI
ci_set_expand_gray_1_2_4_to_8(ci_structrp ci_ptr)
{
   ci_debug(1, "in ci_set_expand_gray_1_2_4_to_8");

   if (ci_rtran_ok(ci_ptr, 0) == 0)
      return;

   ci_ptr->transformations |= CI_EXPAND;
}

/* Expand tRNS chunks to alpha channels. */
void CIAPI
ci_set_tRNS_to_alpha(ci_structrp ci_ptr)
{
   ci_debug(1, "in ci_set_tRNS_to_alpha");

   if (ci_rtran_ok(ci_ptr, 0) == 0)
      return;

   ci_ptr->transformations |= (CI_EXPAND | CI_EXPAND_tRNS);
}
#endif /* READ_EXPAND */

#ifdef CI_READ_EXPAND_16_SUPPORTED
/* Expand to 16-bit channels, expand the tRNS chunk too (because otherwise
 * it may not work correctly.)
 */
void CIAPI
ci_set_expand_16(ci_structrp ci_ptr)
{
   ci_debug(1, "in ci_set_expand_16");

   if (ci_rtran_ok(ci_ptr, 0) == 0)
      return;

   ci_ptr->transformations |= (CI_EXPAND_16 | CI_EXPAND | CI_EXPAND_tRNS);
}
#endif

#ifdef CI_READ_GRAY_TO_RGB_SUPPORTED
void CIAPI
ci_set_gray_to_rgb(ci_structrp ci_ptr)
{
   ci_debug(1, "in ci_set_gray_to_rgb");

   if (ci_rtran_ok(ci_ptr, 0) == 0)
      return;

   /* Because rgb must be 8 bits or more: */
   ci_set_expand_gray_1_2_4_to_8(ci_ptr);
   ci_ptr->transformations |= CI_GRAY_TO_RGB;
}
#endif

#ifdef CI_READ_RGB_TO_GRAY_SUPPORTED
void CIFAPI
ci_set_rgb_to_gray_fixed(ci_structrp ci_ptr, int error_action,
    ci_fixed_point red, ci_fixed_point green)
{
   ci_debug(1, "in ci_set_rgb_to_gray_fixed");

   /* Need the IHDR here because of the check on color_type below. */
   /* TODO: fix this */
   if (ci_rtran_ok(ci_ptr, 1) == 0)
      return;

   switch (error_action)
   {
      case CI_ERROR_ACTION_NONE:
         ci_ptr->transformations |= CI_RGB_TO_GRAY;
         break;

      case CI_ERROR_ACTION_WARN:
         ci_ptr->transformations |= CI_RGB_TO_GRAY_WARN;
         break;

      case CI_ERROR_ACTION_ERROR:
         ci_ptr->transformations |= CI_RGB_TO_GRAY_ERR;
         break;

      default:
         ci_error(ci_ptr, "invalid error action to rgb_to_gray");
   }

   if (ci_ptr->color_type == CI_COLOR_TYPE_PALETTE)
#ifdef CI_READ_EXPAND_SUPPORTED
      ci_ptr->transformations |= CI_EXPAND;
#else
   {
      /* Make this an error in 1.6 because otherwise the application may assume
       * that it just worked and get a memory overwrite.
       */
      ci_error(ci_ptr,
          "Cannot do RGB_TO_GRAY without EXPAND_SUPPORTED");

      /* ci_ptr->transformations &= ~CI_RGB_TO_GRAY; */
   }
#endif
   {
      if (red >= 0 && green >= 0 && red + green <= CI_FP_1)
      {
         ci_uint_16 red_int, green_int;

         /* NOTE: this calculation does not round, but this behavior is retained
          * for consistency; the inaccuracy is very small.  The code here always
          * overwrites the coefficients, regardless of whether they have been
          * defaulted or set already.
          */
         red_int = (ci_uint_16)(((ci_uint_32)red*32768)/100000);
         green_int = (ci_uint_16)(((ci_uint_32)green*32768)/100000);

         ci_ptr->rgb_to_gray_red_coeff   = red_int;
         ci_ptr->rgb_to_gray_green_coeff = green_int;
         ci_ptr->rgb_to_gray_coefficients_set = 1;
      }

      else if (red >= 0 && green >= 0)
         ci_app_warning(ci_ptr,
               "ignoring out of range rgb_to_gray coefficients");
   }
}

#ifdef CI_FLOATING_POINT_SUPPORTED
/* Convert a RGB image to a grayscale of the same width.  This allows us,
 * for example, to convert a 24 bpp RGB image into an 8 bpp grayscale image.
 */

void CIAPI
ci_set_rgb_to_gray(ci_structrp ci_ptr, int error_action, double red,
    double green)
{
   ci_set_rgb_to_gray_fixed(ci_ptr, error_action,
       ci_fixed(ci_ptr, red, "rgb to gray red coefficient"),
      ci_fixed(ci_ptr, green, "rgb to gray green coefficient"));
}
#endif /* FLOATING POINT */

#endif /* RGB_TO_GRAY */

#if defined(CI_READ_USER_TRANSFORM_SUPPORTED) || \
    defined(CI_WRITE_USER_TRANSFORM_SUPPORTED)
void CIAPI
ci_set_read_user_transform_fn(ci_structrp ci_ptr, ci_user_transform_ptr
    read_user_transform_fn)
{
   ci_debug(1, "in ci_set_read_user_transform_fn");

#ifdef CI_READ_USER_TRANSFORM_SUPPORTED
   ci_ptr->transformations |= CI_USER_TRANSFORM;
   ci_ptr->read_user_transform_fn = read_user_transform_fn;
#endif
}
#endif

#ifdef CI_READ_TRANSFORMS_SUPPORTED
#ifdef CI_READ_GAMMA_SUPPORTED
/* In the case of gamma transformations only do transformations on images where
 * the [file] gamma and screen_gamma are not close reciprocals, otherwise it
 * slows things down slightly, and also needlessly introduces small errors.
 */
static int /* PRIVATE */
ci_gamma_threshold(ci_fixed_point screen_gamma, ci_fixed_point file_gamma)
{
   /* CI_GAMMA_THRESHOLD is the threshold for performing gamma
    * correction as a difference of the overall transform from 1.0
    *
    * We want to compare the threshold with s*f - 1, if we get
    * overflow here it is because of wacky gamma values so we
    * turn on processing anyway.
    */
   ci_fixed_point gtest;
   return !ci_muldiv(&gtest, screen_gamma, file_gamma, CI_FP_1) ||
       ci_gamma_significant(gtest);
}
#endif

/* Initialize everything needed for the read.  This includes modifying
 * the palette.
 */

/* For the moment 'ci_init_palette_transformations' and
 * 'ci_init_rgb_transformations' only do some flag canceling optimizations.
 * The intent is that these two routines should have palette or rgb operations
 * extracted from 'ci_init_read_transformations'.
 */
static void /* PRIVATE */
ci_init_palette_transformations(ci_structrp ci_ptr)
{
   /* Called to handle the (input) palette case.  In ci_do_read_transformations
    * the first step is to expand the palette if requested, so this code must
    * take care to only make changes that are invariant with respect to the
    * palette expansion, or only do them if there is no expansion.
    *
    * STRIP_ALPHA has already been handled in the caller (by setting num_trans
    * to 0.)
    */
   int input_has_alpha = 0;
   int input_has_transparency = 0;

   if (ci_ptr->num_trans > 0)
   {
      int i;

      /* Ignore if all the entries are opaque (unlikely!) */
      for (i=0; i<ci_ptr->num_trans; ++i)
      {
         if (ci_ptr->trans_alpha[i] == 255)
            continue;
         else if (ci_ptr->trans_alpha[i] == 0)
            input_has_transparency = 1;
         else
         {
            input_has_transparency = 1;
            input_has_alpha = 1;
            break;
         }
      }
   }

   /* If no alpha we can optimize. */
   if (input_has_alpha == 0)
   {
      /* Any alpha means background and associative alpha processing is
       * required, however if the alpha is 0 or 1 throughout OPTIMIZE_ALPHA
       * and ENCODE_ALPHA are irrelevant.
       */
      ci_ptr->transformations &= ~CI_ENCODE_ALPHA;
      ci_ptr->flags &= ~CI_FLAG_OPTIMIZE_ALPHA;

      if (input_has_transparency == 0)
         ci_ptr->transformations &= ~(CI_COMPOSE | CI_BACKGROUND_EXPAND);
   }

#if defined(CI_READ_EXPAND_SUPPORTED) && defined(CI_READ_BACKGROUND_SUPPORTED)
   /* ci_set_background handling - deals with the complexity of whether the
    * background color is in the file format or the screen format in the case
    * where an 'expand' will happen.
    */

   /* The following code cannot be entered in the alpha pre-multiplication case
    * because CI_BACKGROUND_EXPAND is cancelled below.
    */
   if ((ci_ptr->transformations & CI_BACKGROUND_EXPAND) != 0 &&
       (ci_ptr->transformations & CI_EXPAND) != 0)
   {
      {
         ci_ptr->background.red   =
             ci_ptr->palette[ci_ptr->background.index].red;
         ci_ptr->background.green =
             ci_ptr->palette[ci_ptr->background.index].green;
         ci_ptr->background.blue  =
             ci_ptr->palette[ci_ptr->background.index].blue;

#ifdef CI_READ_INVERT_ALPHA_SUPPORTED
         if ((ci_ptr->transformations & CI_INVERT_ALPHA) != 0)
         {
            if ((ci_ptr->transformations & CI_EXPAND_tRNS) == 0)
            {
               /* Invert the alpha channel (in tRNS) unless the pixels are
                * going to be expanded, in which case leave it for later
                */
               int i, istop = ci_ptr->num_trans;

               for (i = 0; i < istop; i++)
                  ci_ptr->trans_alpha[i] =
                      (ci_byte)(255 - ci_ptr->trans_alpha[i]);
            }
         }
#endif /* READ_INVERT_ALPHA */
      }
   } /* background expand and (therefore) no alpha association. */
#endif /* READ_EXPAND && READ_BACKGROUND */
}

static void /* PRIVATE */
ci_init_rgb_transformations(ci_structrp ci_ptr)
{
   /* Added to libci-1.5.4: check the color type to determine whether there
    * is any alpha or transparency in the image and simply cancel the
    * background and alpha mode stuff if there isn't.
    */
   int input_has_alpha = (ci_ptr->color_type & CI_COLOR_MASK_ALPHA) != 0;
   int input_has_transparency = ci_ptr->num_trans > 0;

   /* If no alpha we can optimize. */
   if (input_has_alpha == 0)
   {
      /* Any alpha means background and associative alpha processing is
       * required, however if the alpha is 0 or 1 throughout OPTIMIZE_ALPHA
       * and ENCODE_ALPHA are irrelevant.
       */
#     ifdef CI_READ_ALPHA_MODE_SUPPORTED
         ci_ptr->transformations &= ~CI_ENCODE_ALPHA;
         ci_ptr->flags &= ~CI_FLAG_OPTIMIZE_ALPHA;
#     endif

      if (input_has_transparency == 0)
         ci_ptr->transformations &= ~(CI_COMPOSE | CI_BACKGROUND_EXPAND);
   }

#if defined(CI_READ_EXPAND_SUPPORTED) && defined(CI_READ_BACKGROUND_SUPPORTED)
   /* ci_set_background handling - deals with the complexity of whether the
    * background color is in the file format or the screen format in the case
    * where an 'expand' will happen.
    */

   /* The following code cannot be entered in the alpha pre-multiplication case
    * because CI_BACKGROUND_EXPAND is cancelled below.
    */
   if ((ci_ptr->transformations & CI_BACKGROUND_EXPAND) != 0 &&
       (ci_ptr->transformations & CI_EXPAND) != 0 &&
       (ci_ptr->color_type & CI_COLOR_MASK_COLOR) == 0)
       /* i.e., GRAY or GRAY_ALPHA */
   {
      {
         /* Expand background and tRNS chunks */
         int gray = ci_ptr->background.gray;
         int trans_gray = ci_ptr->trans_color.gray;

         switch (ci_ptr->bit_depth)
         {
            case 1:
               gray *= 0xff;
               trans_gray *= 0xff;
               break;

            case 2:
               gray *= 0x55;
               trans_gray *= 0x55;
               break;

            case 4:
               gray *= 0x11;
               trans_gray *= 0x11;
               break;

            default:

            case 8:
               /* FALLTHROUGH */ /*  (Already 8 bits) */

            case 16:
               /* Already a full 16 bits */
               break;
         }

         ci_ptr->background.red = ci_ptr->background.green =
            ci_ptr->background.blue = (ci_uint_16)gray;

         if ((ci_ptr->transformations & CI_EXPAND_tRNS) == 0)
         {
            ci_ptr->trans_color.red = ci_ptr->trans_color.green =
               ci_ptr->trans_color.blue = (ci_uint_16)trans_gray;
         }
      }
   } /* background expand and (therefore) no alpha association. */
#endif /* READ_EXPAND && READ_BACKGROUND */
}

#ifdef CI_READ_GAMMA_SUPPORTED
ci_fixed_point /* PRIVATE */
ci_resolve_file_gamma(ci_const_structrp ci_ptr)
{
   ci_fixed_point file_gamma;

   /* The file gamma is determined by these precedence rules, in this order
    * (i.e. use the first value found):
    *
    *    ci_set_gamma; ci_struct::file_gammma if not zero, then:
    *    ci_struct::chunk_gamma if not 0 (determined the CIv3 rules), then:
    *    ci_set_gamma; 1/ci_struct::screen_gamma if not zero
    *
    *    0 (i.e. do no gamma handling)
    */
   file_gamma = ci_ptr->file_gamma;
   if (file_gamma != 0)
      return file_gamma;

   file_gamma = ci_ptr->chunk_gamma;
   if (file_gamma != 0)
      return file_gamma;

   file_gamma = ci_ptr->default_gamma;
   if (file_gamma != 0)
      return file_gamma;

   /* If ci_reciprocal oveflows it returns 0 which indicates to the caller that
    * there is no usable file gamma.  (The checks added to ci_set_gamma and
    * ci_set_alpha_mode should prevent a screen_gamma which would overflow.)
    */
   if (ci_ptr->screen_gamma != 0)
      file_gamma = ci_reciprocal(ci_ptr->screen_gamma);

   return file_gamma;
}

static int
ci_init_gamma_values(ci_structrp ci_ptr)
{
   /* The following temporary indicates if overall gamma correction is
    * required.
    */
   int gamma_correction = 0;
   ci_fixed_point file_gamma, screen_gamma;

   /* Resolve the file_gamma.  See above: if ci_ptr::screen_gamma is set
    * file_gamma will always be set here:
    */
   file_gamma = ci_resolve_file_gamma(ci_ptr);
   screen_gamma = ci_ptr->screen_gamma;

   if (file_gamma > 0) /* file has been set */
   {
      if (screen_gamma > 0) /* screen set too */
         gamma_correction = ci_gamma_threshold(file_gamma, screen_gamma);

      else
         /* Assume the output matches the input; a long time default behavior
          * of libci, although the standard has nothing to say about this.
          */
         screen_gamma = ci_reciprocal(file_gamma);
   }

   else /* both unset, prevent corrections: */
      file_gamma = screen_gamma = CI_FP_1;

   ci_ptr->file_gamma = file_gamma;
   ci_ptr->screen_gamma = screen_gamma;
   return gamma_correction;

}
#endif /* READ_GAMMA */

void /* PRIVATE */
ci_init_read_transformations(ci_structrp ci_ptr)
{
   ci_debug(1, "in ci_init_read_transformations");

   /* This internal function is called from ci_read_start_row in cirutil.c
    * and it is called before the 'rowbytes' calculation is done, so the code
    * in here can change or update the transformations flags.
    *
    * First do updates that do not depend on the details of the CI image data
    * being processed.
    */

#ifdef CI_READ_GAMMA_SUPPORTED
   /* Prior to 1.5.4 these tests were performed from ci_set_gamma, 1.5.4 adds
    * ci_set_alpha_mode and this is another source for a default file gamma so
    * the test needs to be performed later - here.  In addition prior to 1.5.4
    * the tests were repeated for the PALETTE color type here - this is no
    * longer necessary (and doesn't seem to have been necessary before.)
    *
    * CIv3: the new mandatory precedence/priority rules for colour space chunks
    * are handled here (by calling the above function).
    *
    * Turn the gamma transformation on or off as appropriate.  Notice that
    * CI_GAMMA just refers to the file->screen correction.  Alpha composition
    * may independently cause gamma correction because it needs linear data
    * (e.g. if the file has a gAMA chunk but the screen gamma hasn't been
    * specified.)  In any case this flag may get turned off in the code
    * immediately below if the transform can be handled outside the row loop.
    */
   if (ci_init_gamma_values(ci_ptr) != 0)
      ci_ptr->transformations |= CI_GAMMA;

   else
      ci_ptr->transformations &= ~CI_GAMMA;
#endif

   /* Certain transformations have the effect of preventing other
    * transformations that happen afterward in ci_do_read_transformations;
    * resolve the interdependencies here.  From the code of
    * ci_do_read_transformations the order is:
    *
    *  1) CI_EXPAND (including CI_EXPAND_tRNS)
    *  2) CI_STRIP_ALPHA (if no compose)
    *  3) CI_RGB_TO_GRAY
    *  4) CI_GRAY_TO_RGB iff !CI_BACKGROUND_IS_GRAY
    *  5) CI_COMPOSE
    *  6) CI_GAMMA
    *  7) CI_STRIP_ALPHA (if compose)
    *  8) CI_ENCODE_ALPHA
    *  9) CI_SCALE_16_TO_8
    * 10) CI_16_TO_8
    * 11) CI_QUANTIZE (converts to palette)
    * 12) CI_EXPAND_16
    * 13) CI_GRAY_TO_RGB iff CI_BACKGROUND_IS_GRAY
    * 14) CI_INVERT_MONO
    * 15) CI_INVERT_ALPHA
    * 16) CI_SHIFT
    * 17) CI_PACK
    * 18) CI_BGR
    * 19) CI_PACKSWAP
    * 20) CI_FILLER (includes CI_ADD_ALPHA)
    * 21) CI_SWAP_ALPHA
    * 22) CI_SWAP_BYTES
    * 23) CI_USER_TRANSFORM [must be last]
    */
#ifdef CI_READ_STRIP_ALPHA_SUPPORTED
   if ((ci_ptr->transformations & CI_STRIP_ALPHA) != 0 &&
       (ci_ptr->transformations & CI_COMPOSE) == 0)
   {
      /* Stripping the alpha channel happens immediately after the 'expand'
       * transformations, before all other transformation, so it cancels out
       * the alpha handling.  It has the side effect negating the effect of
       * CI_EXPAND_tRNS too:
       */
      ci_ptr->transformations &= ~(CI_BACKGROUND_EXPAND | CI_ENCODE_ALPHA |
         CI_EXPAND_tRNS);
      ci_ptr->flags &= ~CI_FLAG_OPTIMIZE_ALPHA;

      /* Kill the tRNS chunk itself too.  Prior to 1.5.4 this did not happen
       * so transparency information would remain just so long as it wasn't
       * expanded.  This produces unexpected API changes if the set of things
       * that do CI_EXPAND_tRNS changes (perfectly possible given the
       * documentation - which says ask for what you want, accept what you
       * get.)  This makes the behavior consistent from 1.5.4:
       */
      ci_ptr->num_trans = 0;
   }
#endif /* STRIP_ALPHA supported, no COMPOSE */

#ifdef CI_READ_ALPHA_MODE_SUPPORTED
   /* If the screen gamma is about 1.0 then the OPTIMIZE_ALPHA and ENCODE_ALPHA
    * settings will have no effect.
    */
   if (ci_gamma_significant(ci_ptr->screen_gamma) == 0)
   {
      ci_ptr->transformations &= ~CI_ENCODE_ALPHA;
      ci_ptr->flags &= ~CI_FLAG_OPTIMIZE_ALPHA;
   }
#endif

#ifdef CI_READ_RGB_TO_GRAY_SUPPORTED
   /* Make sure the coefficients for the rgb to gray conversion are set
    * appropriately.
    */
   if ((ci_ptr->transformations & CI_RGB_TO_GRAY) != 0)
      ci_set_rgb_coefficients(ci_ptr);
#endif

#ifdef CI_READ_GRAY_TO_RGB_SUPPORTED
#if defined(CI_READ_EXPAND_SUPPORTED) && defined(CI_READ_BACKGROUND_SUPPORTED)
   /* Detect gray background and attempt to enable optimization for
    * gray --> RGB case.
    *
    * Note:  if CI_BACKGROUND_EXPAND is set and color_type is either RGB or
    * RGB_ALPHA (in which case need_expand is superfluous anyway), the
    * background color might actually be gray yet not be flagged as such.
    * This is not a problem for the current code, which uses
    * CI_BACKGROUND_IS_GRAY only to decide when to do the
    * ci_do_gray_to_rgb() transformation.
    *
    * TODO: this code needs to be revised to avoid the complexity and
    * interdependencies.  The color type of the background should be recorded in
    * ci_set_background, along with the bit depth, then the code has a record
    * of exactly what color space the background is currently in.
    */
   if ((ci_ptr->transformations & CI_BACKGROUND_EXPAND) != 0)
   {
      /* CI_BACKGROUND_EXPAND: the background is in the file color space, so if
       * the file was grayscale the background value is gray.
       */
      if ((ci_ptr->color_type & CI_COLOR_MASK_COLOR) == 0)
         ci_ptr->mode |= CI_BACKGROUND_IS_GRAY;
   }

   else if ((ci_ptr->transformations & CI_COMPOSE) != 0)
   {
      /* CI_COMPOSE: ci_set_background was called with need_expand false,
       * so the color is in the color space of the output or ci_set_alpha_mode
       * was called and the color is black.  Ignore RGB_TO_GRAY because that
       * happens before GRAY_TO_RGB.
       */
      if ((ci_ptr->transformations & CI_GRAY_TO_RGB) != 0)
      {
         if (ci_ptr->background.red == ci_ptr->background.green &&
             ci_ptr->background.red == ci_ptr->background.blue)
         {
            ci_ptr->mode |= CI_BACKGROUND_IS_GRAY;
            ci_ptr->background.gray = ci_ptr->background.red;
         }
      }
   }
#endif /* READ_EXPAND && READ_BACKGROUND */
#endif /* READ_GRAY_TO_RGB */

   /* For indexed CI data (CI_COLOR_TYPE_PALETTE) many of the transformations
    * can be performed directly on the palette, and some (such as rgb to gray)
    * can be optimized inside the palette.  This is particularly true of the
    * composite (background and alpha) stuff, which can be pretty much all done
    * in the palette even if the result is expanded to RGB or gray afterward.
    *
    * NOTE: this is Not Yet Implemented, the code behaves as in 1.5.1 and
    * earlier and the palette stuff is actually handled on the first row.  This
    * leads to the reported bug that the palette returned by ci_get_PLTE is not
    * updated.
    */
   if (ci_ptr->color_type == CI_COLOR_TYPE_PALETTE)
      ci_init_palette_transformations(ci_ptr);

   else
      ci_init_rgb_transformations(ci_ptr);

#if defined(CI_READ_BACKGROUND_SUPPORTED) && \
   defined(CI_READ_EXPAND_16_SUPPORTED)
   if ((ci_ptr->transformations & CI_EXPAND_16) != 0 &&
       (ci_ptr->transformations & CI_COMPOSE) != 0 &&
       (ci_ptr->transformations & CI_BACKGROUND_EXPAND) == 0 &&
       ci_ptr->bit_depth != 16)
   {
      /* TODO: fix this.  Because the expand_16 operation is after the compose
       * handling the background color must be 8, not 16, bits deep, but the
       * application will supply a 16-bit value so reduce it here.
       *
       * The CI_BACKGROUND_EXPAND code above does not expand to 16 bits at
       * present, so that case is ok (until do_expand_16 is moved.)
       *
       * NOTE: this discards the low 16 bits of the user supplied background
       * color, but until expand_16 works properly there is no choice!
       */
#     define CHOP(x) (x)=((ci_uint_16)CI_DIV257(x))
      CHOP(ci_ptr->background.red);
      CHOP(ci_ptr->background.green);
      CHOP(ci_ptr->background.blue);
      CHOP(ci_ptr->background.gray);
#     undef CHOP
   }
#endif /* READ_BACKGROUND && READ_EXPAND_16 */

#if defined(CI_READ_BACKGROUND_SUPPORTED) && \
   (defined(CI_READ_SCALE_16_TO_8_SUPPORTED) || \
   defined(CI_READ_STRIP_16_TO_8_SUPPORTED))
   if ((ci_ptr->transformations & (CI_16_TO_8|CI_SCALE_16_TO_8)) != 0 &&
       (ci_ptr->transformations & CI_COMPOSE) != 0 &&
       (ci_ptr->transformations & CI_BACKGROUND_EXPAND) == 0 &&
       ci_ptr->bit_depth == 16)
   {
      /* On the other hand, if a 16-bit file is to be reduced to 8-bits per
       * component this will also happen after CI_COMPOSE and so the background
       * color must be pre-expanded here.
       *
       * TODO: fix this too.
       */
      ci_ptr->background.red = (ci_uint_16)(ci_ptr->background.red * 257);
      ci_ptr->background.green =
         (ci_uint_16)(ci_ptr->background.green * 257);
      ci_ptr->background.blue = (ci_uint_16)(ci_ptr->background.blue * 257);
      ci_ptr->background.gray = (ci_uint_16)(ci_ptr->background.gray * 257);
   }
#endif

   /* NOTE: below 'CI_READ_ALPHA_MODE_SUPPORTED' is presumed to also enable the
    * background support (see the comments in scripts/cilibconf.dfa), this
    * allows pre-multiplication of the alpha channel to be implemented as
    * compositing on black.  This is probably sub-optimal and has been done in
    * 1.5.4 betas simply to enable external critique and testing (i.e. to
    * implement the new API quickly, without lots of internal changes.)
    */

#ifdef CI_READ_GAMMA_SUPPORTED
#  ifdef CI_READ_BACKGROUND_SUPPORTED
      /* Includes ALPHA_MODE */
      ci_ptr->background_1 = ci_ptr->background;
#  endif

   /* This needs to change - in the palette image case a whole set of tables are
    * built when it would be quicker to just calculate the correct value for
    * each palette entry directly.  Also, the test is too tricky - why check
    * CI_RGB_TO_GRAY if CI_GAMMA is not set?  The answer seems to be that
    * CI_GAMMA is cancelled even if the gamma is known?  The test excludes the
    * CI_COMPOSE case, so apparently if there is no *overall* gamma correction
    * the gamma tables will not be built even if composition is required on a
    * gamma encoded value.
    *
    * In 1.5.4 this is addressed below by an additional check on the individual
    * file gamma - if it is not 1.0 both RGB_TO_GRAY and COMPOSE need the
    * tables.
    */
   if ((ci_ptr->transformations & CI_GAMMA) != 0 ||
       ((ci_ptr->transformations & CI_RGB_TO_GRAY) != 0 &&
        (ci_gamma_significant(ci_ptr->file_gamma) != 0 ||
         ci_gamma_significant(ci_ptr->screen_gamma) != 0)) ||
        ((ci_ptr->transformations & CI_COMPOSE) != 0 &&
         (ci_gamma_significant(ci_ptr->file_gamma) != 0 ||
          ci_gamma_significant(ci_ptr->screen_gamma) != 0
#  ifdef CI_READ_BACKGROUND_SUPPORTED
         || (ci_ptr->background_gamma_type == CI_BACKGROUND_GAMMA_UNIQUE &&
           ci_gamma_significant(ci_ptr->background_gamma) != 0)
#  endif
        )) || ((ci_ptr->transformations & CI_ENCODE_ALPHA) != 0 &&
       ci_gamma_significant(ci_ptr->screen_gamma) != 0))
   {
      ci_build_gamma_table(ci_ptr, ci_ptr->bit_depth);

#ifdef CI_READ_BACKGROUND_SUPPORTED
      if ((ci_ptr->transformations & CI_COMPOSE) != 0)
      {
         /* Issue a warning about this combination: because RGB_TO_GRAY is
          * optimized to do the gamma transform if present yet do_background has
          * to do the same thing if both options are set a
          * double-gamma-correction happens.  This is true in all versions of
          * libci to date.
          */
         if ((ci_ptr->transformations & CI_RGB_TO_GRAY) != 0)
            ci_warning(ci_ptr,
                "libci does not support gamma+background+rgb_to_gray");

         if ((ci_ptr->color_type == CI_COLOR_TYPE_PALETTE) != 0)
         {
            /* We don't get to here unless there is a tRNS chunk with non-opaque
             * entries - see the checking code at the start of this function.
             */
            ci_color back, back_1;
            ci_colorp palette = ci_ptr->palette;
            int num_palette = ci_ptr->num_palette;
            int i;
            if (ci_ptr->background_gamma_type == CI_BACKGROUND_GAMMA_FILE)
            {

               back.red = ci_ptr->gamma_table[ci_ptr->background.red];
               back.green = ci_ptr->gamma_table[ci_ptr->background.green];
               back.blue = ci_ptr->gamma_table[ci_ptr->background.blue];

               back_1.red = ci_ptr->gamma_to_1[ci_ptr->background.red];
               back_1.green = ci_ptr->gamma_to_1[ci_ptr->background.green];
               back_1.blue = ci_ptr->gamma_to_1[ci_ptr->background.blue];
            }
            else
            {
               ci_fixed_point g, gs;

               switch (ci_ptr->background_gamma_type)
               {
                  case CI_BACKGROUND_GAMMA_SCREEN:
                     g = (ci_ptr->screen_gamma);
                     gs = CI_FP_1;
                     break;

                  case CI_BACKGROUND_GAMMA_FILE:
                     g = ci_reciprocal(ci_ptr->file_gamma);
                     gs = ci_reciprocal2(ci_ptr->file_gamma,
                         ci_ptr->screen_gamma);
                     break;

                  case CI_BACKGROUND_GAMMA_UNIQUE:
                     g = ci_reciprocal(ci_ptr->background_gamma);
                     gs = ci_reciprocal2(ci_ptr->background_gamma,
                         ci_ptr->screen_gamma);
                     break;
                  default:
                     g = CI_FP_1;    /* back_1 */
                     gs = CI_FP_1;   /* back */
                     break;
               }

               if (ci_gamma_significant(gs) != 0)
               {
                  back.red = ci_gamma_8bit_correct(ci_ptr->background.red,
                      gs);
                  back.green = ci_gamma_8bit_correct(ci_ptr->background.green,
                      gs);
                  back.blue = ci_gamma_8bit_correct(ci_ptr->background.blue,
                      gs);
               }

               else
               {
                  back.red   = (ci_byte)ci_ptr->background.red;
                  back.green = (ci_byte)ci_ptr->background.green;
                  back.blue  = (ci_byte)ci_ptr->background.blue;
               }

               if (ci_gamma_significant(g) != 0)
               {
                  back_1.red = ci_gamma_8bit_correct(ci_ptr->background.red,
                      g);
                  back_1.green = ci_gamma_8bit_correct(
                      ci_ptr->background.green, g);
                  back_1.blue = ci_gamma_8bit_correct(ci_ptr->background.blue,
                      g);
               }

               else
               {
                  back_1.red   = (ci_byte)ci_ptr->background.red;
                  back_1.green = (ci_byte)ci_ptr->background.green;
                  back_1.blue  = (ci_byte)ci_ptr->background.blue;
               }
            }

            for (i = 0; i < num_palette; i++)
            {
               if (i < (int)ci_ptr->num_trans &&
                   ci_ptr->trans_alpha[i] != 0xff)
               {
                  if (ci_ptr->trans_alpha[i] == 0)
                  {
                     palette[i] = back;
                  }
                  else /* if (ci_ptr->trans_alpha[i] != 0xff) */
                  {
                     ci_byte v, w;

                     v = ci_ptr->gamma_to_1[palette[i].red];
                     ci_composite(w, v, ci_ptr->trans_alpha[i], back_1.red);
                     palette[i].red = ci_ptr->gamma_from_1[w];

                     v = ci_ptr->gamma_to_1[palette[i].green];
                     ci_composite(w, v, ci_ptr->trans_alpha[i], back_1.green);
                     palette[i].green = ci_ptr->gamma_from_1[w];

                     v = ci_ptr->gamma_to_1[palette[i].blue];
                     ci_composite(w, v, ci_ptr->trans_alpha[i], back_1.blue);
                     palette[i].blue = ci_ptr->gamma_from_1[w];
                  }
               }
               else
               {
                  palette[i].red = ci_ptr->gamma_table[palette[i].red];
                  palette[i].green = ci_ptr->gamma_table[palette[i].green];
                  palette[i].blue = ci_ptr->gamma_table[palette[i].blue];
               }
            }

            /* Prevent the transformations being done again.
             *
             * NOTE: this is highly dubious; it removes the transformations in
             * place.  This seems inconsistent with the general treatment of the
             * transformations elsewhere.
             */
            ci_ptr->transformations &= ~(CI_COMPOSE | CI_GAMMA);
         } /* color_type == CI_COLOR_TYPE_PALETTE */

         /* if (ci_ptr->background_gamma_type!=CI_BACKGROUND_GAMMA_UNKNOWN) */
         else /* color_type != CI_COLOR_TYPE_PALETTE */
         {
            int gs_sig, g_sig;
            ci_fixed_point g = CI_FP_1;  /* Correction to linear */
            ci_fixed_point gs = CI_FP_1; /* Correction to screen */

            switch (ci_ptr->background_gamma_type)
            {
               case CI_BACKGROUND_GAMMA_SCREEN:
                  g = ci_ptr->screen_gamma;
                  /* gs = CI_FP_1; */
                  break;

               case CI_BACKGROUND_GAMMA_FILE:
                  g = ci_reciprocal(ci_ptr->file_gamma);
                  gs = ci_reciprocal2(ci_ptr->file_gamma,
                      ci_ptr->screen_gamma);
                  break;

               case CI_BACKGROUND_GAMMA_UNIQUE:
                  g = ci_reciprocal(ci_ptr->background_gamma);
                  gs = ci_reciprocal2(ci_ptr->background_gamma,
                      ci_ptr->screen_gamma);
                  break;

               default:
                  ci_error(ci_ptr, "invalid background gamma type");
            }

            g_sig = ci_gamma_significant(g);
            gs_sig = ci_gamma_significant(gs);

            if (g_sig != 0)
               ci_ptr->background_1.gray = ci_gamma_correct(ci_ptr,
                   ci_ptr->background.gray, g);

            if (gs_sig != 0)
               ci_ptr->background.gray = ci_gamma_correct(ci_ptr,
                   ci_ptr->background.gray, gs);

            if ((ci_ptr->background.red != ci_ptr->background.green) ||
                (ci_ptr->background.red != ci_ptr->background.blue) ||
                (ci_ptr->background.red != ci_ptr->background.gray))
            {
               /* RGB or RGBA with color background */
               if (g_sig != 0)
               {
                  ci_ptr->background_1.red = ci_gamma_correct(ci_ptr,
                      ci_ptr->background.red, g);

                  ci_ptr->background_1.green = ci_gamma_correct(ci_ptr,
                      ci_ptr->background.green, g);

                  ci_ptr->background_1.blue = ci_gamma_correct(ci_ptr,
                      ci_ptr->background.blue, g);
               }

               if (gs_sig != 0)
               {
                  ci_ptr->background.red = ci_gamma_correct(ci_ptr,
                      ci_ptr->background.red, gs);

                  ci_ptr->background.green = ci_gamma_correct(ci_ptr,
                      ci_ptr->background.green, gs);

                  ci_ptr->background.blue = ci_gamma_correct(ci_ptr,
                      ci_ptr->background.blue, gs);
               }
            }

            else
            {
               /* GRAY, GRAY ALPHA, RGB, or RGBA with gray background */
               ci_ptr->background_1.red = ci_ptr->background_1.green
                   = ci_ptr->background_1.blue = ci_ptr->background_1.gray;

               ci_ptr->background.red = ci_ptr->background.green
                   = ci_ptr->background.blue = ci_ptr->background.gray;
            }

            /* The background is now in screen gamma: */
            ci_ptr->background_gamma_type = CI_BACKGROUND_GAMMA_SCREEN;
         } /* color_type != CI_COLOR_TYPE_PALETTE */
      }/* ci_ptr->transformations & CI_BACKGROUND */

      else
      /* Transformation does not include CI_BACKGROUND */
#endif /* READ_BACKGROUND */
      if (ci_ptr->color_type == CI_COLOR_TYPE_PALETTE
#ifdef CI_READ_RGB_TO_GRAY_SUPPORTED
         /* RGB_TO_GRAY needs to have non-gamma-corrected values! */
         && ((ci_ptr->transformations & CI_EXPAND) == 0 ||
         (ci_ptr->transformations & CI_RGB_TO_GRAY) == 0)
#endif
         )
      {
         ci_colorp palette = ci_ptr->palette;
         int num_palette = ci_ptr->num_palette;
         int i;

         /* NOTE: there are other transformations that should probably be in
          * here too.
          */
         for (i = 0; i < num_palette; i++)
         {
            palette[i].red = ci_ptr->gamma_table[palette[i].red];
            palette[i].green = ci_ptr->gamma_table[palette[i].green];
            palette[i].blue = ci_ptr->gamma_table[palette[i].blue];
         }

         /* Done the gamma correction. */
         ci_ptr->transformations &= ~CI_GAMMA;
      } /* color_type == PALETTE && !CI_BACKGROUND transformation */
   }
#ifdef CI_READ_BACKGROUND_SUPPORTED
   else
#endif
#endif /* READ_GAMMA */

#ifdef CI_READ_BACKGROUND_SUPPORTED
   /* No GAMMA transformation (see the hanging else 4 lines above) */
   if ((ci_ptr->transformations & CI_COMPOSE) != 0 &&
       (ci_ptr->color_type == CI_COLOR_TYPE_PALETTE))
   {
      int i;
      int istop = (int)ci_ptr->num_trans;
      ci_color back;
      ci_colorp palette = ci_ptr->palette;

      back.red   = (ci_byte)ci_ptr->background.red;
      back.green = (ci_byte)ci_ptr->background.green;
      back.blue  = (ci_byte)ci_ptr->background.blue;

      for (i = 0; i < istop; i++)
      {
         if (ci_ptr->trans_alpha[i] == 0)
         {
            palette[i] = back;
         }

         else if (ci_ptr->trans_alpha[i] != 0xff)
         {
            /* The ci_composite() macro is defined in ci.h */
            ci_composite(palette[i].red, palette[i].red,
                ci_ptr->trans_alpha[i], back.red);

            ci_composite(palette[i].green, palette[i].green,
                ci_ptr->trans_alpha[i], back.green);

            ci_composite(palette[i].blue, palette[i].blue,
                ci_ptr->trans_alpha[i], back.blue);
         }
      }

      ci_ptr->transformations &= ~CI_COMPOSE;
   }
#endif /* READ_BACKGROUND */

#ifdef CI_READ_SHIFT_SUPPORTED
   if ((ci_ptr->transformations & CI_SHIFT) != 0 &&
       (ci_ptr->transformations & CI_EXPAND) == 0 &&
       (ci_ptr->color_type == CI_COLOR_TYPE_PALETTE))
   {
      int i;
      int istop = ci_ptr->num_palette;
      int shift = 8 - ci_ptr->sig_bit.red;

      ci_ptr->transformations &= ~CI_SHIFT;

      /* significant bits can be in the range 1 to 7 for a meaningful result, if
       * the number of significant bits is 0 then no shift is done (this is an
       * error condition which is silently ignored.)
       */
      if (shift > 0 && shift < 8)
         for (i=0; i<istop; ++i)
         {
            int component = ci_ptr->palette[i].red;

            component >>= shift;
            ci_ptr->palette[i].red = (ci_byte)component;
         }

      shift = 8 - ci_ptr->sig_bit.green;
      if (shift > 0 && shift < 8)
         for (i=0; i<istop; ++i)
         {
            int component = ci_ptr->palette[i].green;

            component >>= shift;
            ci_ptr->palette[i].green = (ci_byte)component;
         }

      shift = 8 - ci_ptr->sig_bit.blue;
      if (shift > 0 && shift < 8)
         for (i=0; i<istop; ++i)
         {
            int component = ci_ptr->palette[i].blue;

            component >>= shift;
            ci_ptr->palette[i].blue = (ci_byte)component;
         }
   }
#endif /* READ_SHIFT */
}

/* Modify the info structure to reflect the transformations.  The
 * info should be updated so a CI file could be written with it,
 * assuming the transformations result in valid CI data.
 */
void /* PRIVATE */
ci_read_transform_info(ci_structrp ci_ptr, ci_inforp info_ptr)
{
   ci_debug(1, "in ci_read_transform_info");

#ifdef CI_READ_EXPAND_SUPPORTED
   if ((ci_ptr->transformations & CI_EXPAND) != 0)
   {
      if (info_ptr->color_type == CI_COLOR_TYPE_PALETTE)
      {
         /* This check must match what actually happens in
          * ci_do_expand_palette; if it ever checks the tRNS chunk to see if
          * it is all opaque we must do the same (at present it does not.)
          */
         if (ci_ptr->num_trans > 0)
            info_ptr->color_type = CI_COLOR_TYPE_RGB_ALPHA;

         else
            info_ptr->color_type = CI_COLOR_TYPE_RGB;

         info_ptr->bit_depth = 8;
         info_ptr->num_trans = 0;

         if (ci_ptr->palette == NULL)
            ci_error (ci_ptr, "Palette is NULL in indexed image");
      }
      else
      {
         if (ci_ptr->num_trans != 0)
         {
            if ((ci_ptr->transformations & CI_EXPAND_tRNS) != 0)
               info_ptr->color_type |= CI_COLOR_MASK_ALPHA;
         }
         if (info_ptr->bit_depth < 8)
            info_ptr->bit_depth = 8;

         info_ptr->num_trans = 0;
      }
   }
#endif

#if defined(CI_READ_BACKGROUND_SUPPORTED) ||\
   defined(CI_READ_ALPHA_MODE_SUPPORTED)
   /* The following is almost certainly wrong unless the background value is in
    * the screen space!
    */
   if ((ci_ptr->transformations & CI_COMPOSE) != 0)
      info_ptr->background = ci_ptr->background;
#endif

#ifdef CI_READ_GAMMA_SUPPORTED
   /* The following used to be conditional on CI_GAMMA (prior to 1.5.4),
    * however it seems that the code in ci_init_read_transformations, which has
    * been called before this from ci_read_update_info->ci_read_start_row
    * sometimes does the gamma transform and cancels the flag.
    *
    * TODO: this is confusing.  It only changes the result of ci_get_gAMA and,
    * yes, it does return the value that the transformed data effectively has
    * but does any app really understand this?
    */
   info_ptr->gamma = ci_ptr->file_gamma;
#endif

   if (info_ptr->bit_depth == 16)
   {
#  ifdef CI_READ_16BIT_SUPPORTED
#     ifdef CI_READ_SCALE_16_TO_8_SUPPORTED
         if ((ci_ptr->transformations & CI_SCALE_16_TO_8) != 0)
            info_ptr->bit_depth = 8;
#     endif

#     ifdef CI_READ_STRIP_16_TO_8_SUPPORTED
         if ((ci_ptr->transformations & CI_16_TO_8) != 0)
            info_ptr->bit_depth = 8;
#     endif

#  else
      /* No 16-bit support: force chopping 16-bit input down to 8, in this case
       * the app program can chose if both APIs are available by setting the
       * correct scaling to use.
       */
#     ifdef CI_READ_STRIP_16_TO_8_SUPPORTED
         /* For compatibility with previous versions use the strip method by
          * default.  This code works because if CI_SCALE_16_TO_8 is already
          * set the code below will do that in preference to the chop.
          */
         ci_ptr->transformations |= CI_16_TO_8;
         info_ptr->bit_depth = 8;
#     else

#        ifdef CI_READ_SCALE_16_TO_8_SUPPORTED
            ci_ptr->transformations |= CI_SCALE_16_TO_8;
            info_ptr->bit_depth = 8;
#        else

            CONFIGURATION ERROR: you must enable at least one 16 to 8 method
#        endif
#    endif
#endif /* !READ_16BIT */
   }

#ifdef CI_READ_GRAY_TO_RGB_SUPPORTED
   if ((ci_ptr->transformations & CI_GRAY_TO_RGB) != 0)
      info_ptr->color_type = (ci_byte)(info_ptr->color_type |
         CI_COLOR_MASK_COLOR);
#endif

#ifdef CI_READ_RGB_TO_GRAY_SUPPORTED
   if ((ci_ptr->transformations & CI_RGB_TO_GRAY) != 0)
      info_ptr->color_type = (ci_byte)(info_ptr->color_type &
         ~CI_COLOR_MASK_COLOR);
#endif

#ifdef CI_READ_QUANTIZE_SUPPORTED
   if ((ci_ptr->transformations & CI_QUANTIZE) != 0)
   {
      if (((info_ptr->color_type == CI_COLOR_TYPE_RGB) ||
          (info_ptr->color_type == CI_COLOR_TYPE_RGB_ALPHA)) &&
          ci_ptr->palette_lookup != 0 && info_ptr->bit_depth == 8)
      {
         info_ptr->color_type = CI_COLOR_TYPE_PALETTE;
      }
   }
#endif

#ifdef CI_READ_EXPAND_16_SUPPORTED
   if ((ci_ptr->transformations & CI_EXPAND_16) != 0 &&
       info_ptr->bit_depth == 8 &&
       info_ptr->color_type != CI_COLOR_TYPE_PALETTE)
   {
      info_ptr->bit_depth = 16;
   }
#endif

#ifdef CI_READ_PACK_SUPPORTED
   if ((ci_ptr->transformations & CI_PACK) != 0 &&
       (info_ptr->bit_depth < 8))
      info_ptr->bit_depth = 8;
#endif

   if (info_ptr->color_type == CI_COLOR_TYPE_PALETTE)
      info_ptr->channels = 1;

   else if ((info_ptr->color_type & CI_COLOR_MASK_COLOR) != 0)
      info_ptr->channels = 3;

   else
      info_ptr->channels = 1;

#ifdef CI_READ_STRIP_ALPHA_SUPPORTED
   if ((ci_ptr->transformations & CI_STRIP_ALPHA) != 0)
   {
      info_ptr->color_type = (ci_byte)(info_ptr->color_type &
         ~CI_COLOR_MASK_ALPHA);
      info_ptr->num_trans = 0;
   }
#endif

   if ((info_ptr->color_type & CI_COLOR_MASK_ALPHA) != 0)
      info_ptr->channels++;

#ifdef CI_READ_FILLER_SUPPORTED
   /* STRIP_ALPHA and FILLER allowed:  MASK_ALPHA bit stripped above */
   if ((ci_ptr->transformations & CI_FILLER) != 0 &&
       (info_ptr->color_type == CI_COLOR_TYPE_RGB ||
       info_ptr->color_type == CI_COLOR_TYPE_GRAY))
   {
      info_ptr->channels++;
      /* If adding a true alpha channel not just filler */
      if ((ci_ptr->transformations & CI_ADD_ALPHA) != 0)
         info_ptr->color_type |= CI_COLOR_MASK_ALPHA;
   }
#endif

#if defined(CI_USER_TRANSFORM_PTR_SUPPORTED) && \
defined(CI_READ_USER_TRANSFORM_SUPPORTED)
   if ((ci_ptr->transformations & CI_USER_TRANSFORM) != 0)
   {
      if (ci_ptr->user_transform_depth != 0)
         info_ptr->bit_depth = ci_ptr->user_transform_depth;

      if (ci_ptr->user_transform_channels != 0)
         info_ptr->channels = ci_ptr->user_transform_channels;
   }
#endif

   info_ptr->pixel_depth = (ci_byte)(info_ptr->channels *
       info_ptr->bit_depth);

   info_ptr->rowbytes = CI_ROWBYTES(info_ptr->pixel_depth, info_ptr->width);

   /* Adding in 1.5.4: cache the above value in ci_struct so that we can later
    * check in ci_rowbytes that the user buffer won't get overwritten.  Note
    * that the field is not always set - if ci_read_update_info isn't called
    * the application has to either not do any transforms or get the calculation
    * right itself.
    */
   ci_ptr->info_rowbytes = info_ptr->rowbytes;

#ifndef CI_READ_EXPAND_SUPPORTED
   if (ci_ptr != NULL)
      return;
#endif
}

#ifdef CI_READ_PACK_SUPPORTED
/* Unpack pixels of 1, 2, or 4 bits per pixel into 1 byte per pixel,
 * without changing the actual values.  Thus, if you had a row with
 * a bit depth of 1, you would end up with bytes that only contained
 * the numbers 0 or 1.  If you would rather they contain 0 and 255, use
 * ci_do_shift() after this.
 */
static void
ci_do_unpack(ci_row_infop row_info, ci_bytep row)
{
   ci_debug(1, "in ci_do_unpack");

   if (row_info->bit_depth < 8)
   {
      ci_uint_32 i;
      ci_uint_32 row_width=row_info->width;

      switch (row_info->bit_depth)
      {
         case 1:
         {
            ci_bytep sp = row + (size_t)((row_width - 1) >> 3);
            ci_bytep dp = row + (size_t)row_width - 1;
            ci_uint_32 shift = 7U - ((row_width + 7U) & 0x07);
            for (i = 0; i < row_width; i++)
            {
               *dp = (ci_byte)((*sp >> shift) & 0x01);

               if (shift == 7)
               {
                  shift = 0;
                  sp--;
               }

               else
                  shift++;

               dp--;
            }
            break;
         }

         case 2:
         {

            ci_bytep sp = row + (size_t)((row_width - 1) >> 2);
            ci_bytep dp = row + (size_t)row_width - 1;
            ci_uint_32 shift = ((3U - ((row_width + 3U) & 0x03)) << 1);
            for (i = 0; i < row_width; i++)
            {
               *dp = (ci_byte)((*sp >> shift) & 0x03);

               if (shift == 6)
               {
                  shift = 0;
                  sp--;
               }

               else
                  shift += 2;

               dp--;
            }
            break;
         }

         case 4:
         {
            ci_bytep sp = row + (size_t)((row_width - 1) >> 1);
            ci_bytep dp = row + (size_t)row_width - 1;
            ci_uint_32 shift = ((1U - ((row_width + 1U) & 0x01)) << 2);
            for (i = 0; i < row_width; i++)
            {
               *dp = (ci_byte)((*sp >> shift) & 0x0f);

               if (shift == 4)
               {
                  shift = 0;
                  sp--;
               }

               else
                  shift = 4;

               dp--;
            }
            break;
         }

         default:
            break;
      }
      row_info->bit_depth = 8;
      row_info->pixel_depth = (ci_byte)(8 * row_info->channels);
      row_info->rowbytes = row_width * row_info->channels;
   }
}
#endif

#ifdef CI_READ_SHIFT_SUPPORTED
/* Reverse the effects of ci_do_shift.  This routine merely shifts the
 * pixels back to their significant bits values.  Thus, if you have
 * a row of bit depth 8, but only 5 are significant, this will shift
 * the values back to 0 through 31.
 */
static void
ci_do_unshift(ci_row_infop row_info, ci_bytep row,
    ci_const_color_8p sig_bits)
{
   int color_type;

   ci_debug(1, "in ci_do_unshift");

   /* The palette case has already been handled in the _init routine. */
   color_type = row_info->color_type;

   if (color_type != CI_COLOR_TYPE_PALETTE)
   {
      int shift[4];
      int channels = 0;
      int bit_depth = row_info->bit_depth;

      if ((color_type & CI_COLOR_MASK_COLOR) != 0)
      {
         shift[channels++] = bit_depth - sig_bits->red;
         shift[channels++] = bit_depth - sig_bits->green;
         shift[channels++] = bit_depth - sig_bits->blue;
      }

      else
      {
         shift[channels++] = bit_depth - sig_bits->gray;
      }

      if ((color_type & CI_COLOR_MASK_ALPHA) != 0)
      {
         shift[channels++] = bit_depth - sig_bits->alpha;
      }

      {
         int c, have_shift;

         for (c = have_shift = 0; c < channels; ++c)
         {
            /* A shift of more than the bit depth is an error condition but it
             * gets ignored here.
             */
            if (shift[c] <= 0 || shift[c] >= bit_depth)
               shift[c] = 0;

            else
               have_shift = 1;
         }

         if (have_shift == 0)
            return;
      }

      switch (bit_depth)
      {
         default:
         /* Must be 1bpp gray: should not be here! */
            /* NOTREACHED */
            break;

         case 2:
         /* Must be 2bpp gray */
         /* assert(channels == 1 && shift[0] == 1) */
         {
            ci_bytep bp = row;
            ci_bytep bp_end = bp + row_info->rowbytes;

            while (bp < bp_end)
            {
               int b = (*bp >> 1) & 0x55;
               *bp++ = (ci_byte)b;
            }
            break;
         }

         case 4:
         /* Must be 4bpp gray */
         /* assert(channels == 1) */
         {
            ci_bytep bp = row;
            ci_bytep bp_end = bp + row_info->rowbytes;
            int gray_shift = shift[0];
            int mask =  0xf >> gray_shift;

            mask |= mask << 4;

            while (bp < bp_end)
            {
               int b = (*bp >> gray_shift) & mask;
               *bp++ = (ci_byte)b;
            }
            break;
         }

         case 8:
         /* Single byte components, G, GA, RGB, RGBA */
         {
            ci_bytep bp = row;
            ci_bytep bp_end = bp + row_info->rowbytes;
            int channel = 0;

            while (bp < bp_end)
            {
               int b = *bp >> shift[channel];
               if (++channel >= channels)
                  channel = 0;
               *bp++ = (ci_byte)b;
            }
            break;
         }

#ifdef CI_READ_16BIT_SUPPORTED
         case 16:
         /* Double byte components, G, GA, RGB, RGBA */
         {
            ci_bytep bp = row;
            ci_bytep bp_end = bp + row_info->rowbytes;
            int channel = 0;

            while (bp < bp_end)
            {
               int value = (bp[0] << 8) + bp[1];

               value >>= shift[channel];
               if (++channel >= channels)
                  channel = 0;
               *bp++ = (ci_byte)(value >> 8);
               *bp++ = (ci_byte)value;
            }
            break;
         }
#endif
      }
   }
}
#endif

#ifdef CI_READ_SCALE_16_TO_8_SUPPORTED
/* Scale rows of bit depth 16 down to 8 accurately */
static void
ci_do_scale_16_to_8(ci_row_infop row_info, ci_bytep row)
{
   ci_debug(1, "in ci_do_scale_16_to_8");

   if (row_info->bit_depth == 16)
   {
      ci_bytep sp = row; /* source */
      ci_bytep dp = row; /* destination */
      ci_bytep ep = sp + row_info->rowbytes; /* end+1 */

      while (sp < ep)
      {
         /* The input is an array of 16-bit components, these must be scaled to
          * 8 bits each.  For a 16-bit value V the required value (from the CI
          * specification) is:
          *
          *    (V * 255) / 65535
          *
          * This reduces to round(V / 257), or floor((V + 128.5)/257)
          *
          * Represent V as the two byte value vhi.vlo.  Make a guess that the
          * result is the top byte of V, vhi, then the correction to this value
          * is:
          *
          *    error = floor(((V-vhi.vhi) + 128.5) / 257)
          *          = floor(((vlo-vhi) + 128.5) / 257)
          *
          * This can be approximated using integer arithmetic (and a signed
          * shift):
          *
          *    error = (vlo-vhi+128) >> 8;
          *
          * The approximate differs from the exact answer only when (vlo-vhi) is
          * 128; it then gives a correction of +1 when the exact correction is
          * 0.  This gives 128 errors.  The exact answer (correct for all 16-bit
          * input values) is:
          *
          *    error = (vlo-vhi+128)*65535 >> 24;
          *
          * An alternative arithmetic calculation which also gives no errors is:
          *
          *    (V * 255 + 32895) >> 16
          */

         ci_int_32 tmp = *sp++; /* must be signed! */
         tmp += (((int)*sp++ - tmp + 128) * 65535) >> 24;
         *dp++ = (ci_byte)tmp;
      }

      row_info->bit_depth = 8;
      row_info->pixel_depth = (ci_byte)(8 * row_info->channels);
      row_info->rowbytes = row_info->width * row_info->channels;
   }
}
#endif

#ifdef CI_READ_STRIP_16_TO_8_SUPPORTED
static void
/* Simply discard the low byte.  This was the default behavior prior
 * to libci-1.5.4.
 */
ci_do_chop(ci_row_infop row_info, ci_bytep row)
{
   ci_debug(1, "in ci_do_chop");

   if (row_info->bit_depth == 16)
   {
      ci_bytep sp = row; /* source */
      ci_bytep dp = row; /* destination */
      ci_bytep ep = sp + row_info->rowbytes; /* end+1 */

      while (sp < ep)
      {
         *dp++ = *sp;
         sp += 2; /* skip low byte */
      }

      row_info->bit_depth = 8;
      row_info->pixel_depth = (ci_byte)(8 * row_info->channels);
      row_info->rowbytes = row_info->width * row_info->channels;
   }
}
#endif

#ifdef CI_READ_SWAP_ALPHA_SUPPORTED
static void
ci_do_read_swap_alpha(ci_row_infop row_info, ci_bytep row)
{
   ci_uint_32 row_width = row_info->width;

   ci_debug(1, "in ci_do_read_swap_alpha");

   if (row_info->color_type == CI_COLOR_TYPE_RGB_ALPHA)
   {
      /* This converts from RGBA to ARGB */
      if (row_info->bit_depth == 8)
      {
         ci_bytep sp = row + row_info->rowbytes;
         ci_bytep dp = sp;
         ci_byte save;
         ci_uint_32 i;

         for (i = 0; i < row_width; i++)
         {
            save = *(--sp);
            *(--dp) = *(--sp);
            *(--dp) = *(--sp);
            *(--dp) = *(--sp);
            *(--dp) = save;
         }
      }

#ifdef CI_READ_16BIT_SUPPORTED
      /* This converts from RRGGBBAA to AARRGGBB */
      else
      {
         ci_bytep sp = row + row_info->rowbytes;
         ci_bytep dp = sp;
         ci_byte save[2];
         ci_uint_32 i;

         for (i = 0; i < row_width; i++)
         {
            save[0] = *(--sp);
            save[1] = *(--sp);
            *(--dp) = *(--sp);
            *(--dp) = *(--sp);
            *(--dp) = *(--sp);
            *(--dp) = *(--sp);
            *(--dp) = *(--sp);
            *(--dp) = *(--sp);
            *(--dp) = save[0];
            *(--dp) = save[1];
         }
      }
#endif
   }

   else if (row_info->color_type == CI_COLOR_TYPE_GRAY_ALPHA)
   {
      /* This converts from GA to AG */
      if (row_info->bit_depth == 8)
      {
         ci_bytep sp = row + row_info->rowbytes;
         ci_bytep dp = sp;
         ci_byte save;
         ci_uint_32 i;

         for (i = 0; i < row_width; i++)
         {
            save = *(--sp);
            *(--dp) = *(--sp);
            *(--dp) = save;
         }
      }

#ifdef CI_READ_16BIT_SUPPORTED
      /* This converts from GGAA to AAGG */
      else
      {
         ci_bytep sp = row + row_info->rowbytes;
         ci_bytep dp = sp;
         ci_byte save[2];
         ci_uint_32 i;

         for (i = 0; i < row_width; i++)
         {
            save[0] = *(--sp);
            save[1] = *(--sp);
            *(--dp) = *(--sp);
            *(--dp) = *(--sp);
            *(--dp) = save[0];
            *(--dp) = save[1];
         }
      }
#endif
   }
}
#endif

#ifdef CI_READ_INVERT_ALPHA_SUPPORTED
static void
ci_do_read_invert_alpha(ci_row_infop row_info, ci_bytep row)
{
   ci_uint_32 row_width;
   ci_debug(1, "in ci_do_read_invert_alpha");

   row_width = row_info->width;
   if (row_info->color_type == CI_COLOR_TYPE_RGB_ALPHA)
   {
      if (row_info->bit_depth == 8)
      {
         /* This inverts the alpha channel in RGBA */
         ci_bytep sp = row + row_info->rowbytes;
         ci_bytep dp = sp;
         ci_uint_32 i;

         for (i = 0; i < row_width; i++)
         {
            *(--dp) = (ci_byte)(255 - *(--sp));

/*          This does nothing:
            *(--dp) = *(--sp);
            *(--dp) = *(--sp);
            *(--dp) = *(--sp);
            We can replace it with:
*/
            sp-=3;
            dp=sp;
         }
      }

#ifdef CI_READ_16BIT_SUPPORTED
      /* This inverts the alpha channel in RRGGBBAA */
      else
      {
         ci_bytep sp = row + row_info->rowbytes;
         ci_bytep dp = sp;
         ci_uint_32 i;

         for (i = 0; i < row_width; i++)
         {
            *(--dp) = (ci_byte)(255 - *(--sp));
            *(--dp) = (ci_byte)(255 - *(--sp));

/*          This does nothing:
            *(--dp) = *(--sp);
            *(--dp) = *(--sp);
            *(--dp) = *(--sp);
            *(--dp) = *(--sp);
            *(--dp) = *(--sp);
            *(--dp) = *(--sp);
            We can replace it with:
*/
            sp-=6;
            dp=sp;
         }
      }
#endif
   }
   else if (row_info->color_type == CI_COLOR_TYPE_GRAY_ALPHA)
   {
      if (row_info->bit_depth == 8)
      {
         /* This inverts the alpha channel in GA */
         ci_bytep sp = row + row_info->rowbytes;
         ci_bytep dp = sp;
         ci_uint_32 i;

         for (i = 0; i < row_width; i++)
         {
            *(--dp) = (ci_byte)(255 - *(--sp));
            *(--dp) = *(--sp);
         }
      }

#ifdef CI_READ_16BIT_SUPPORTED
      else
      {
         /* This inverts the alpha channel in GGAA */
         ci_bytep sp  = row + row_info->rowbytes;
         ci_bytep dp = sp;
         ci_uint_32 i;

         for (i = 0; i < row_width; i++)
         {
            *(--dp) = (ci_byte)(255 - *(--sp));
            *(--dp) = (ci_byte)(255 - *(--sp));
/*
            *(--dp) = *(--sp);
            *(--dp) = *(--sp);
*/
            sp-=2;
            dp=sp;
         }
      }
#endif
   }
}
#endif

#ifdef CI_READ_FILLER_SUPPORTED
/* Add filler channel if we have RGB color */
static void
ci_do_read_filler(ci_row_infop row_info, ci_bytep row,
    ci_uint_32 filler, ci_uint_32 flags)
{
   ci_uint_32 i;
   ci_uint_32 row_width = row_info->width;

#ifdef CI_READ_16BIT_SUPPORTED
   ci_byte hi_filler = (ci_byte)(filler>>8);
#endif
   ci_byte lo_filler = (ci_byte)filler;

   ci_debug(1, "in ci_do_read_filler");

   if (
       row_info->color_type == CI_COLOR_TYPE_GRAY)
   {
      if (row_info->bit_depth == 8)
      {
         if ((flags & CI_FLAG_FILLER_AFTER) != 0)
         {
            /* This changes the data from G to GX */
            ci_bytep sp = row + (size_t)row_width;
            ci_bytep dp =  sp + (size_t)row_width;
            for (i = 1; i < row_width; i++)
            {
               *(--dp) = lo_filler;
               *(--dp) = *(--sp);
            }
            *(--dp) = lo_filler;
            row_info->channels = 2;
            row_info->pixel_depth = 16;
            row_info->rowbytes = row_width * 2;
         }

         else
         {
            /* This changes the data from G to XG */
            ci_bytep sp = row + (size_t)row_width;
            ci_bytep dp = sp  + (size_t)row_width;
            for (i = 0; i < row_width; i++)
            {
               *(--dp) = *(--sp);
               *(--dp) = lo_filler;
            }
            row_info->channels = 2;
            row_info->pixel_depth = 16;
            row_info->rowbytes = row_width * 2;
         }
      }

#ifdef CI_READ_16BIT_SUPPORTED
      else if (row_info->bit_depth == 16)
      {
         if ((flags & CI_FLAG_FILLER_AFTER) != 0)
         {
            /* This changes the data from GG to GGXX */
            ci_bytep sp = row + (size_t)row_width * 2;
            ci_bytep dp = sp  + (size_t)row_width * 2;
            for (i = 1; i < row_width; i++)
            {
               *(--dp) = lo_filler;
               *(--dp) = hi_filler;
               *(--dp) = *(--sp);
               *(--dp) = *(--sp);
            }
            *(--dp) = lo_filler;
            *(--dp) = hi_filler;
            row_info->channels = 2;
            row_info->pixel_depth = 32;
            row_info->rowbytes = row_width * 4;
         }

         else
         {
            /* This changes the data from GG to XXGG */
            ci_bytep sp = row + (size_t)row_width * 2;
            ci_bytep dp = sp  + (size_t)row_width * 2;
            for (i = 0; i < row_width; i++)
            {
               *(--dp) = *(--sp);
               *(--dp) = *(--sp);
               *(--dp) = lo_filler;
               *(--dp) = hi_filler;
            }
            row_info->channels = 2;
            row_info->pixel_depth = 32;
            row_info->rowbytes = row_width * 4;
         }
      }
#endif
   } /* COLOR_TYPE == GRAY */
   else if (row_info->color_type == CI_COLOR_TYPE_RGB)
   {
      if (row_info->bit_depth == 8)
      {
         if ((flags & CI_FLAG_FILLER_AFTER) != 0)
         {
            /* This changes the data from RGB to RGBX */
            ci_bytep sp = row + (size_t)row_width * 3;
            ci_bytep dp = sp  + (size_t)row_width;
            for (i = 1; i < row_width; i++)
            {
               *(--dp) = lo_filler;
               *(--dp) = *(--sp);
               *(--dp) = *(--sp);
               *(--dp) = *(--sp);
            }
            *(--dp) = lo_filler;
            row_info->channels = 4;
            row_info->pixel_depth = 32;
            row_info->rowbytes = row_width * 4;
         }

         else
         {
            /* This changes the data from RGB to XRGB */
            ci_bytep sp = row + (size_t)row_width * 3;
            ci_bytep dp = sp + (size_t)row_width;
            for (i = 0; i < row_width; i++)
            {
               *(--dp) = *(--sp);
               *(--dp) = *(--sp);
               *(--dp) = *(--sp);
               *(--dp) = lo_filler;
            }
            row_info->channels = 4;
            row_info->pixel_depth = 32;
            row_info->rowbytes = row_width * 4;
         }
      }

#ifdef CI_READ_16BIT_SUPPORTED
      else if (row_info->bit_depth == 16)
      {
         if ((flags & CI_FLAG_FILLER_AFTER) != 0)
         {
            /* This changes the data from RRGGBB to RRGGBBXX */
            ci_bytep sp = row + (size_t)row_width * 6;
            ci_bytep dp = sp  + (size_t)row_width * 2;
            for (i = 1; i < row_width; i++)
            {
               *(--dp) = lo_filler;
               *(--dp) = hi_filler;
               *(--dp) = *(--sp);
               *(--dp) = *(--sp);
               *(--dp) = *(--sp);
               *(--dp) = *(--sp);
               *(--dp) = *(--sp);
               *(--dp) = *(--sp);
            }
            *(--dp) = lo_filler;
            *(--dp) = hi_filler;
            row_info->channels = 4;
            row_info->pixel_depth = 64;
            row_info->rowbytes = row_width * 8;
         }

         else
         {
            /* This changes the data from RRGGBB to XXRRGGBB */
            ci_bytep sp = row + (size_t)row_width * 6;
            ci_bytep dp = sp  + (size_t)row_width * 2;
            for (i = 0; i < row_width; i++)
            {
               *(--dp) = *(--sp);
               *(--dp) = *(--sp);
               *(--dp) = *(--sp);
               *(--dp) = *(--sp);
               *(--dp) = *(--sp);
               *(--dp) = *(--sp);
               *(--dp) = lo_filler;
               *(--dp) = hi_filler;
            }

            row_info->channels = 4;
            row_info->pixel_depth = 64;
            row_info->rowbytes = row_width * 8;
         }
      }
#endif
   } /* COLOR_TYPE == RGB */
}
#endif

#ifdef CI_READ_GRAY_TO_RGB_SUPPORTED
/* Expand grayscale files to RGB, with or without alpha */
static void
ci_do_gray_to_rgb(ci_row_infop row_info, ci_bytep row)
{
   ci_uint_32 i;
   ci_uint_32 row_width = row_info->width;

   ci_debug(1, "in ci_do_gray_to_rgb");

   if (row_info->bit_depth >= 8 &&
       (row_info->color_type & CI_COLOR_MASK_COLOR) == 0)
   {
      if (row_info->color_type == CI_COLOR_TYPE_GRAY)
      {
         if (row_info->bit_depth == 8)
         {
            /* This changes G to RGB */
            ci_bytep sp = row + (size_t)row_width - 1;
            ci_bytep dp = sp  + (size_t)row_width * 2;
            for (i = 0; i < row_width; i++)
            {
               *(dp--) = *sp;
               *(dp--) = *sp;
               *(dp--) = *(sp--);
            }
         }

         else
         {
            /* This changes GG to RRGGBB */
            ci_bytep sp = row + (size_t)row_width * 2 - 1;
            ci_bytep dp = sp  + (size_t)row_width * 4;
            for (i = 0; i < row_width; i++)
            {
               *(dp--) = *sp;
               *(dp--) = *(sp - 1);
               *(dp--) = *sp;
               *(dp--) = *(sp - 1);
               *(dp--) = *(sp--);
               *(dp--) = *(sp--);
            }
         }
      }

      else if (row_info->color_type == CI_COLOR_TYPE_GRAY_ALPHA)
      {
         if (row_info->bit_depth == 8)
         {
            /* This changes GA to RGBA */
            ci_bytep sp = row + (size_t)row_width * 2 - 1;
            ci_bytep dp = sp  + (size_t)row_width * 2;
            for (i = 0; i < row_width; i++)
            {
               *(dp--) = *(sp--);
               *(dp--) = *sp;
               *(dp--) = *sp;
               *(dp--) = *(sp--);
            }
         }

         else
         {
            /* This changes GGAA to RRGGBBAA */
            ci_bytep sp = row + (size_t)row_width * 4 - 1;
            ci_bytep dp = sp  + (size_t)row_width * 4;
            for (i = 0; i < row_width; i++)
            {
               *(dp--) = *(sp--);
               *(dp--) = *(sp--);
               *(dp--) = *sp;
               *(dp--) = *(sp - 1);
               *(dp--) = *sp;
               *(dp--) = *(sp - 1);
               *(dp--) = *(sp--);
               *(dp--) = *(sp--);
            }
         }
      }
      row_info->channels = (ci_byte)(row_info->channels + 2);
      row_info->color_type |= CI_COLOR_MASK_COLOR;
      row_info->pixel_depth = (ci_byte)(row_info->channels *
          row_info->bit_depth);
      row_info->rowbytes = CI_ROWBYTES(row_info->pixel_depth, row_width);
   }
}
#endif

#ifdef CI_READ_RGB_TO_GRAY_SUPPORTED
/* Reduce RGB files to grayscale, with or without alpha
 * using the equation given in Poynton's ColorFAQ of 1998-01-04 at
 * <http://www.inforamp.net/~poynton/>  (THIS LINK IS DEAD June 2008 but
 * versions dated 1998 through November 2002 have been archived at
 * https://web.archive.org/web/20000816232553/www.inforamp.net/
 * ~poynton/notes/colour_and_gamma/ColorFAQ.txt )
 * Charles Poynton poynton at poynton.com
 *
 *     Y = 0.212671 * R + 0.715160 * G + 0.072169 * B
 *
 *  which can be expressed with integers as
 *
 *     Y = (6969 * R + 23434 * G + 2365 * B)/32768
 *
 * Poynton's current link (as of January 2003 through July 2011):
 * <http://www.poynton.com/notes/colour_and_gamma/>
 * has changed the numbers slightly:
 *
 *     Y = 0.2126*R + 0.7152*G + 0.0722*B
 *
 *  which can be expressed with integers as
 *
 *     Y = (6966 * R + 23436 * G + 2366 * B)/32768
 *
 *  Historically, however, libci uses numbers derived from the ITU-R Rec 709
 *  end point chromaticities and the D65 white point.  Depending on the
 *  precision used for the D65 white point this produces a variety of different
 *  numbers, however if the four decimal place value used in ITU-R Rec 709 is
 *  used (0.3127,0.3290) the Y calculation would be:
 *
 *     Y = (6968 * R + 23435 * G + 2366 * B)/32768
 *
 *  While this is correct the rounding results in an overflow for white, because
 *  the sum of the rounded coefficients is 32769, not 32768.  Consequently
 *  libci uses, instead, the closest non-overflowing approximation:
 *
 *     Y = (6968 * R + 23434 * G + 2366 * B)/32768
 *
 *  Starting with libci-1.5.5, if the image being converted has a cHRM chunk
 *  (including an sRGB chunk) then the chromaticities are used to calculate the
 *  coefficients.  See the chunk handling in cirutil.c for more information.
 *
 *  In all cases the calculation is to be done in a linear colorspace.  If no
 *  gamma information is available to correct the encoding of the original RGB
 *  values this results in an implicit assumption that the original CI RGB
 *  values were linear.
 *
 *  Other integer coefficients can be used via ci_set_rgb_to_gray().  Because
 *  the API takes just red and green coefficients the blue coefficient is
 *  calculated to make the sum 32768.  This will result in different rounding
 *  to that used above.
 */
static int
ci_do_rgb_to_gray(ci_structrp ci_ptr, ci_row_infop row_info, ci_bytep row)
{
   int rgb_error = 0;

   ci_debug(1, "in ci_do_rgb_to_gray");

   if ((row_info->color_type & CI_COLOR_MASK_PALETTE) == 0 &&
       (row_info->color_type & CI_COLOR_MASK_COLOR) != 0)
   {
      ci_uint_32 rc = ci_ptr->rgb_to_gray_red_coeff;
      ci_uint_32 gc = ci_ptr->rgb_to_gray_green_coeff;
      ci_uint_32 bc = 32768 - rc - gc;
      ci_uint_32 row_width = row_info->width;
      int have_alpha = (row_info->color_type & CI_COLOR_MASK_ALPHA) != 0;

      if (row_info->bit_depth == 8)
      {
#ifdef CI_READ_GAMMA_SUPPORTED
         /* Notice that gamma to/from 1 are not necessarily inverses (if
          * there is an overall gamma correction).  Prior to 1.5.5 this code
          * checked the linearized values for equality; this doesn't match
          * the documentation, the original values must be checked.
          */
         if (ci_ptr->gamma_from_1 != NULL && ci_ptr->gamma_to_1 != NULL)
         {
            ci_bytep sp = row;
            ci_bytep dp = row;
            ci_uint_32 i;

            for (i = 0; i < row_width; i++)
            {
               ci_byte red   = *(sp++);
               ci_byte green = *(sp++);
               ci_byte blue  = *(sp++);

               if (red != green || red != blue)
               {
                  red = ci_ptr->gamma_to_1[red];
                  green = ci_ptr->gamma_to_1[green];
                  blue = ci_ptr->gamma_to_1[blue];

                  rgb_error |= 1;
                  *(dp++) = ci_ptr->gamma_from_1[
                      (rc*red + gc*green + bc*blue + 16384)>>15];
               }

               else
               {
                  /* If there is no overall correction the table will not be
                   * set.
                   */
                  if (ci_ptr->gamma_table != NULL)
                     red = ci_ptr->gamma_table[red];

                  *(dp++) = red;
               }

               if (have_alpha != 0)
                  *(dp++) = *(sp++);
            }
         }
         else
#endif
         {
            ci_bytep sp = row;
            ci_bytep dp = row;
            ci_uint_32 i;

            for (i = 0; i < row_width; i++)
            {
               ci_byte red   = *(sp++);
               ci_byte green = *(sp++);
               ci_byte blue  = *(sp++);

               if (red != green || red != blue)
               {
                  rgb_error |= 1;
                  /* NOTE: this is the historical approach which simply
                   * truncates the results.
                   */
                  *(dp++) = (ci_byte)((rc*red + gc*green + bc*blue)>>15);
               }

               else
                  *(dp++) = red;

               if (have_alpha != 0)
                  *(dp++) = *(sp++);
            }
         }
      }

      else /* RGB bit_depth == 16 */
      {
#ifdef CI_READ_GAMMA_SUPPORTED
         if (ci_ptr->gamma_16_to_1 != NULL && ci_ptr->gamma_16_from_1 != NULL)
         {
            ci_bytep sp = row;
            ci_bytep dp = row;
            ci_uint_32 i;

            for (i = 0; i < row_width; i++)
            {
               ci_uint_16 red, green, blue, w;
               ci_byte hi,lo;

               hi=*(sp)++; lo=*(sp)++; red   = (ci_uint_16)((hi << 8) | (lo));
               hi=*(sp)++; lo=*(sp)++; green = (ci_uint_16)((hi << 8) | (lo));
               hi=*(sp)++; lo=*(sp)++; blue  = (ci_uint_16)((hi << 8) | (lo));

               if (red == green && red == blue)
               {
                  if (ci_ptr->gamma_16_table != NULL)
                     w = ci_ptr->gamma_16_table[(red & 0xff)
                         >> ci_ptr->gamma_shift][red >> 8];

                  else
                     w = red;
               }

               else
               {
                  ci_uint_16 red_1   = ci_ptr->gamma_16_to_1[(red & 0xff)
                      >> ci_ptr->gamma_shift][red>>8];
                  ci_uint_16 green_1 =
                      ci_ptr->gamma_16_to_1[(green & 0xff) >>
                      ci_ptr->gamma_shift][green>>8];
                  ci_uint_16 blue_1  = ci_ptr->gamma_16_to_1[(blue & 0xff)
                      >> ci_ptr->gamma_shift][blue>>8];
                  ci_uint_16 gray16  = (ci_uint_16)((rc*red_1 + gc*green_1
                      + bc*blue_1 + 16384)>>15);
                  w = ci_ptr->gamma_16_from_1[(gray16 & 0xff) >>
                      ci_ptr->gamma_shift][gray16 >> 8];
                  rgb_error |= 1;
               }

               *(dp++) = (ci_byte)((w>>8) & 0xff);
               *(dp++) = (ci_byte)(w & 0xff);

               if (have_alpha != 0)
               {
                  *(dp++) = *(sp++);
                  *(dp++) = *(sp++);
               }
            }
         }
         else
#endif
         {
            ci_bytep sp = row;
            ci_bytep dp = row;
            ci_uint_32 i;

            for (i = 0; i < row_width; i++)
            {
               ci_uint_16 red, green, blue, gray16;
               ci_byte hi,lo;

               hi=*(sp)++; lo=*(sp)++; red   = (ci_uint_16)((hi << 8) | (lo));
               hi=*(sp)++; lo=*(sp)++; green = (ci_uint_16)((hi << 8) | (lo));
               hi=*(sp)++; lo=*(sp)++; blue  = (ci_uint_16)((hi << 8) | (lo));

               if (red != green || red != blue)
                  rgb_error |= 1;

               /* From 1.5.5 in the 16-bit case do the accurate conversion even
                * in the 'fast' case - this is because this is where the code
                * ends up when handling linear 16-bit data.
                */
               gray16  = (ci_uint_16)((rc*red + gc*green + bc*blue + 16384) >>
                  15);
               *(dp++) = (ci_byte)((gray16 >> 8) & 0xff);
               *(dp++) = (ci_byte)(gray16 & 0xff);

               if (have_alpha != 0)
               {
                  *(dp++) = *(sp++);
                  *(dp++) = *(sp++);
               }
            }
         }
      }

      row_info->channels = (ci_byte)(row_info->channels - 2);
      row_info->color_type = (ci_byte)(row_info->color_type &
          ~CI_COLOR_MASK_COLOR);
      row_info->pixel_depth = (ci_byte)(row_info->channels *
          row_info->bit_depth);
      row_info->rowbytes = CI_ROWBYTES(row_info->pixel_depth, row_width);
   }
   return rgb_error;
}
#endif

#if defined(CI_READ_BACKGROUND_SUPPORTED) ||\
   defined(CI_READ_ALPHA_MODE_SUPPORTED)
/* Replace any alpha or transparency with the supplied background color.
 * "background" is already in the screen gamma, while "background_1" is
 * at a gamma of 1.0.  Paletted files have already been taken care of.
 */
static void
ci_do_compose(ci_row_infop row_info, ci_bytep row, ci_structrp ci_ptr)
{
#ifdef CI_READ_GAMMA_SUPPORTED
   ci_const_bytep gamma_table = ci_ptr->gamma_table;
   ci_const_bytep gamma_from_1 = ci_ptr->gamma_from_1;
   ci_const_bytep gamma_to_1 = ci_ptr->gamma_to_1;
   ci_const_uint_16pp gamma_16 = ci_ptr->gamma_16_table;
   ci_const_uint_16pp gamma_16_from_1 = ci_ptr->gamma_16_from_1;
   ci_const_uint_16pp gamma_16_to_1 = ci_ptr->gamma_16_to_1;
   int gamma_shift = ci_ptr->gamma_shift;
   int optimize = (ci_ptr->flags & CI_FLAG_OPTIMIZE_ALPHA) != 0;
#endif

   ci_bytep sp;
   ci_uint_32 i;
   ci_uint_32 row_width = row_info->width;
   int shift;

   ci_debug(1, "in ci_do_compose");

   switch (row_info->color_type)
   {
      case CI_COLOR_TYPE_GRAY:
      {
         switch (row_info->bit_depth)
         {
            case 1:
            {
               sp = row;
               shift = 7;
               for (i = 0; i < row_width; i++)
               {
                  if ((ci_uint_16)((*sp >> shift) & 0x01)
                     == ci_ptr->trans_color.gray)
                  {
                     unsigned int tmp = *sp & (0x7f7f >> (7 - shift));
                     tmp |=
                         (unsigned int)(ci_ptr->background.gray << shift);
                     *sp = (ci_byte)(tmp & 0xff);
                  }

                  if (shift == 0)
                  {
                     shift = 7;
                     sp++;
                  }

                  else
                     shift--;
               }
               break;
            }

            case 2:
            {
#ifdef CI_READ_GAMMA_SUPPORTED
               if (gamma_table != NULL)
               {
                  sp = row;
                  shift = 6;
                  for (i = 0; i < row_width; i++)
                  {
                     if ((ci_uint_16)((*sp >> shift) & 0x03)
                         == ci_ptr->trans_color.gray)
                     {
                        unsigned int tmp = *sp & (0x3f3f >> (6 - shift));
                        tmp |=
                           (unsigned int)ci_ptr->background.gray << shift;
                        *sp = (ci_byte)(tmp & 0xff);
                     }

                     else
                     {
                        unsigned int p = (*sp >> shift) & 0x03;
                        unsigned int g = (gamma_table [p | (p << 2) |
                            (p << 4) | (p << 6)] >> 6) & 0x03;
                        unsigned int tmp = *sp & (0x3f3f >> (6 - shift));
                        tmp |= (unsigned int)(g << shift);
                        *sp = (ci_byte)(tmp & 0xff);
                     }

                     if (shift == 0)
                     {
                        shift = 6;
                        sp++;
                     }

                     else
                        shift -= 2;
                  }
               }

               else
#endif
               {
                  sp = row;
                  shift = 6;
                  for (i = 0; i < row_width; i++)
                  {
                     if ((ci_uint_16)((*sp >> shift) & 0x03)
                         == ci_ptr->trans_color.gray)
                     {
                        unsigned int tmp = *sp & (0x3f3f >> (6 - shift));
                        tmp |=
                            (unsigned int)ci_ptr->background.gray << shift;
                        *sp = (ci_byte)(tmp & 0xff);
                     }

                     if (shift == 0)
                     {
                        shift = 6;
                        sp++;
                     }

                     else
                        shift -= 2;
                  }
               }
               break;
            }

            case 4:
            {
#ifdef CI_READ_GAMMA_SUPPORTED
               if (gamma_table != NULL)
               {
                  sp = row;
                  shift = 4;
                  for (i = 0; i < row_width; i++)
                  {
                     if ((ci_uint_16)((*sp >> shift) & 0x0f)
                         == ci_ptr->trans_color.gray)
                     {
                        unsigned int tmp = *sp & (0x0f0f >> (4 - shift));
                        tmp |=
                           (unsigned int)(ci_ptr->background.gray << shift);
                        *sp = (ci_byte)(tmp & 0xff);
                     }

                     else
                     {
                        unsigned int p = (*sp >> shift) & 0x0f;
                        unsigned int g = (gamma_table[p | (p << 4)] >> 4) &
                           0x0f;
                        unsigned int tmp = *sp & (0x0f0f >> (4 - shift));
                        tmp |= (unsigned int)(g << shift);
                        *sp = (ci_byte)(tmp & 0xff);
                     }

                     if (shift == 0)
                     {
                        shift = 4;
                        sp++;
                     }

                     else
                        shift -= 4;
                  }
               }

               else
#endif
               {
                  sp = row;
                  shift = 4;
                  for (i = 0; i < row_width; i++)
                  {
                     if ((ci_uint_16)((*sp >> shift) & 0x0f)
                         == ci_ptr->trans_color.gray)
                     {
                        unsigned int tmp = *sp & (0x0f0f >> (4 - shift));
                        tmp |=
                           (unsigned int)(ci_ptr->background.gray << shift);
                        *sp = (ci_byte)(tmp & 0xff);
                     }

                     if (shift == 0)
                     {
                        shift = 4;
                        sp++;
                     }

                     else
                        shift -= 4;
                  }
               }
               break;
            }

            case 8:
            {
#ifdef CI_READ_GAMMA_SUPPORTED
               if (gamma_table != NULL)
               {
                  sp = row;
                  for (i = 0; i < row_width; i++, sp++)
                  {
                     if (*sp == ci_ptr->trans_color.gray)
                        *sp = (ci_byte)ci_ptr->background.gray;

                     else
                        *sp = gamma_table[*sp];
                  }
               }
               else
#endif
               {
                  sp = row;
                  for (i = 0; i < row_width; i++, sp++)
                  {
                     if (*sp == ci_ptr->trans_color.gray)
                        *sp = (ci_byte)ci_ptr->background.gray;
                  }
               }
               break;
            }

            case 16:
            {
#ifdef CI_READ_GAMMA_SUPPORTED
               if (gamma_16 != NULL)
               {
                  sp = row;
                  for (i = 0; i < row_width; i++, sp += 2)
                  {
                     ci_uint_16 v;

                     v = (ci_uint_16)(((*sp) << 8) + *(sp + 1));

                     if (v == ci_ptr->trans_color.gray)
                     {
                        /* Background is already in screen gamma */
                        *sp = (ci_byte)((ci_ptr->background.gray >> 8)
                             & 0xff);
                        *(sp + 1) = (ci_byte)(ci_ptr->background.gray
                             & 0xff);
                     }

                     else
                     {
                        v = gamma_16[*(sp + 1) >> gamma_shift][*sp];
                        *sp = (ci_byte)((v >> 8) & 0xff);
                        *(sp + 1) = (ci_byte)(v & 0xff);
                     }
                  }
               }
               else
#endif
               {
                  sp = row;
                  for (i = 0; i < row_width; i++, sp += 2)
                  {
                     ci_uint_16 v;

                     v = (ci_uint_16)(((*sp) << 8) + *(sp + 1));

                     if (v == ci_ptr->trans_color.gray)
                     {
                        *sp = (ci_byte)((ci_ptr->background.gray >> 8)
                             & 0xff);
                        *(sp + 1) = (ci_byte)(ci_ptr->background.gray
                             & 0xff);
                     }
                  }
               }
               break;
            }

            default:
               break;
         }
         break;
      }

      case CI_COLOR_TYPE_RGB:
      {
         if (row_info->bit_depth == 8)
         {
#ifdef CI_READ_GAMMA_SUPPORTED
            if (gamma_table != NULL)
            {
               sp = row;
               for (i = 0; i < row_width; i++, sp += 3)
               {
                  if (*sp == ci_ptr->trans_color.red &&
                      *(sp + 1) == ci_ptr->trans_color.green &&
                      *(sp + 2) == ci_ptr->trans_color.blue)
                  {
                     *sp = (ci_byte)ci_ptr->background.red;
                     *(sp + 1) = (ci_byte)ci_ptr->background.green;
                     *(sp + 2) = (ci_byte)ci_ptr->background.blue;
                  }

                  else
                  {
                     *sp = gamma_table[*sp];
                     *(sp + 1) = gamma_table[*(sp + 1)];
                     *(sp + 2) = gamma_table[*(sp + 2)];
                  }
               }
            }
            else
#endif
            {
               sp = row;
               for (i = 0; i < row_width; i++, sp += 3)
               {
                  if (*sp == ci_ptr->trans_color.red &&
                      *(sp + 1) == ci_ptr->trans_color.green &&
                      *(sp + 2) == ci_ptr->trans_color.blue)
                  {
                     *sp = (ci_byte)ci_ptr->background.red;
                     *(sp + 1) = (ci_byte)ci_ptr->background.green;
                     *(sp + 2) = (ci_byte)ci_ptr->background.blue;
                  }
               }
            }
         }
         else /* if (row_info->bit_depth == 16) */
         {
#ifdef CI_READ_GAMMA_SUPPORTED
            if (gamma_16 != NULL)
            {
               sp = row;
               for (i = 0; i < row_width; i++, sp += 6)
               {
                  ci_uint_16 r = (ci_uint_16)(((*sp) << 8) + *(sp + 1));

                  ci_uint_16 g = (ci_uint_16)(((*(sp + 2)) << 8)
                      + *(sp + 3));

                  ci_uint_16 b = (ci_uint_16)(((*(sp + 4)) << 8)
                      + *(sp + 5));

                  if (r == ci_ptr->trans_color.red &&
                      g == ci_ptr->trans_color.green &&
                      b == ci_ptr->trans_color.blue)
                  {
                     /* Background is already in screen gamma */
                     *sp = (ci_byte)((ci_ptr->background.red >> 8) & 0xff);
                     *(sp + 1) = (ci_byte)(ci_ptr->background.red & 0xff);
                     *(sp + 2) = (ci_byte)((ci_ptr->background.green >> 8)
                             & 0xff);
                     *(sp + 3) = (ci_byte)(ci_ptr->background.green
                             & 0xff);
                     *(sp + 4) = (ci_byte)((ci_ptr->background.blue >> 8)
                             & 0xff);
                     *(sp + 5) = (ci_byte)(ci_ptr->background.blue & 0xff);
                  }

                  else
                  {
                     ci_uint_16 v = gamma_16[*(sp + 1) >> gamma_shift][*sp];
                     *sp = (ci_byte)((v >> 8) & 0xff);
                     *(sp + 1) = (ci_byte)(v & 0xff);

                     v = gamma_16[*(sp + 3) >> gamma_shift][*(sp + 2)];
                     *(sp + 2) = (ci_byte)((v >> 8) & 0xff);
                     *(sp + 3) = (ci_byte)(v & 0xff);

                     v = gamma_16[*(sp + 5) >> gamma_shift][*(sp + 4)];
                     *(sp + 4) = (ci_byte)((v >> 8) & 0xff);
                     *(sp + 5) = (ci_byte)(v & 0xff);
                  }
               }
            }

            else
#endif
            {
               sp = row;
               for (i = 0; i < row_width; i++, sp += 6)
               {
                  ci_uint_16 r = (ci_uint_16)(((*sp) << 8) + *(sp + 1));

                  ci_uint_16 g = (ci_uint_16)(((*(sp + 2)) << 8)
                      + *(sp + 3));

                  ci_uint_16 b = (ci_uint_16)(((*(sp + 4)) << 8)
                      + *(sp + 5));

                  if (r == ci_ptr->trans_color.red &&
                      g == ci_ptr->trans_color.green &&
                      b == ci_ptr->trans_color.blue)
                  {
                     *sp = (ci_byte)((ci_ptr->background.red >> 8) & 0xff);
                     *(sp + 1) = (ci_byte)(ci_ptr->background.red & 0xff);
                     *(sp + 2) = (ci_byte)((ci_ptr->background.green >> 8)
                             & 0xff);
                     *(sp + 3) = (ci_byte)(ci_ptr->background.green
                             & 0xff);
                     *(sp + 4) = (ci_byte)((ci_ptr->background.blue >> 8)
                             & 0xff);
                     *(sp + 5) = (ci_byte)(ci_ptr->background.blue & 0xff);
                  }
               }
            }
         }
         break;
      }

      case CI_COLOR_TYPE_GRAY_ALPHA:
      {
         if (row_info->bit_depth == 8)
         {
#ifdef CI_READ_GAMMA_SUPPORTED
            if (gamma_to_1 != NULL && gamma_from_1 != NULL &&
                gamma_table != NULL)
            {
               sp = row;
               for (i = 0; i < row_width; i++, sp += 2)
               {
                  ci_uint_16 a = *(sp + 1);

                  if (a == 0xff)
                     *sp = gamma_table[*sp];

                  else if (a == 0)
                  {
                     /* Background is already in screen gamma */
                     *sp = (ci_byte)ci_ptr->background.gray;
                  }

                  else
                  {
                     ci_byte v, w;

                     v = gamma_to_1[*sp];
                     ci_composite(w, v, a, ci_ptr->background_1.gray);
                     if (optimize == 0)
                        w = gamma_from_1[w];
                     *sp = w;
                  }
               }
            }
            else
#endif
            {
               sp = row;
               for (i = 0; i < row_width; i++, sp += 2)
               {
                  ci_byte a = *(sp + 1);

                  if (a == 0)
                     *sp = (ci_byte)ci_ptr->background.gray;

                  else if (a < 0xff)
                     ci_composite(*sp, *sp, a, ci_ptr->background.gray);
               }
            }
         }
         else /* if (ci_ptr->bit_depth == 16) */
         {
#ifdef CI_READ_GAMMA_SUPPORTED
            if (gamma_16 != NULL && gamma_16_from_1 != NULL &&
                gamma_16_to_1 != NULL)
            {
               sp = row;
               for (i = 0; i < row_width; i++, sp += 4)
               {
                  ci_uint_16 a = (ci_uint_16)(((*(sp + 2)) << 8)
                      + *(sp + 3));

                  if (a == (ci_uint_16)0xffff)
                  {
                     ci_uint_16 v;

                     v = gamma_16[*(sp + 1) >> gamma_shift][*sp];
                     *sp = (ci_byte)((v >> 8) & 0xff);
                     *(sp + 1) = (ci_byte)(v & 0xff);
                  }

                  else if (a == 0)
                  {
                     /* Background is already in screen gamma */
                     *sp = (ci_byte)((ci_ptr->background.gray >> 8)
                             & 0xff);
                     *(sp + 1) = (ci_byte)(ci_ptr->background.gray & 0xff);
                  }

                  else
                  {
                     ci_uint_16 g, v, w;

                     g = gamma_16_to_1[*(sp + 1) >> gamma_shift][*sp];
                     ci_composite_16(v, g, a, ci_ptr->background_1.gray);
                     if (optimize != 0)
                        w = v;
                     else
                        w = gamma_16_from_1[(v & 0xff) >>
                            gamma_shift][v >> 8];
                     *sp = (ci_byte)((w >> 8) & 0xff);
                     *(sp + 1) = (ci_byte)(w & 0xff);
                  }
               }
            }
            else
#endif
            {
               sp = row;
               for (i = 0; i < row_width; i++, sp += 4)
               {
                  ci_uint_16 a = (ci_uint_16)(((*(sp + 2)) << 8)
                      + *(sp + 3));

                  if (a == 0)
                  {
                     *sp = (ci_byte)((ci_ptr->background.gray >> 8)
                             & 0xff);
                     *(sp + 1) = (ci_byte)(ci_ptr->background.gray & 0xff);
                  }

                  else if (a < 0xffff)
                  {
                     ci_uint_16 g, v;

                     g = (ci_uint_16)(((*sp) << 8) + *(sp + 1));
                     ci_composite_16(v, g, a, ci_ptr->background.gray);
                     *sp = (ci_byte)((v >> 8) & 0xff);
                     *(sp + 1) = (ci_byte)(v & 0xff);
                  }
               }
            }
         }
         break;
      }

      case CI_COLOR_TYPE_RGB_ALPHA:
      {
         if (row_info->bit_depth == 8)
         {
#ifdef CI_READ_GAMMA_SUPPORTED
            if (gamma_to_1 != NULL && gamma_from_1 != NULL &&
                gamma_table != NULL)
            {
               sp = row;
               for (i = 0; i < row_width; i++, sp += 4)
               {
                  ci_byte a = *(sp + 3);

                  if (a == 0xff)
                  {
                     *sp = gamma_table[*sp];
                     *(sp + 1) = gamma_table[*(sp + 1)];
                     *(sp + 2) = gamma_table[*(sp + 2)];
                  }

                  else if (a == 0)
                  {
                     /* Background is already in screen gamma */
                     *sp = (ci_byte)ci_ptr->background.red;
                     *(sp + 1) = (ci_byte)ci_ptr->background.green;
                     *(sp + 2) = (ci_byte)ci_ptr->background.blue;
                  }

                  else
                  {
                     ci_byte v, w;

                     v = gamma_to_1[*sp];
                     ci_composite(w, v, a, ci_ptr->background_1.red);
                     if (optimize == 0) w = gamma_from_1[w];
                     *sp = w;

                     v = gamma_to_1[*(sp + 1)];
                     ci_composite(w, v, a, ci_ptr->background_1.green);
                     if (optimize == 0) w = gamma_from_1[w];
                     *(sp + 1) = w;

                     v = gamma_to_1[*(sp + 2)];
                     ci_composite(w, v, a, ci_ptr->background_1.blue);
                     if (optimize == 0) w = gamma_from_1[w];
                     *(sp + 2) = w;
                  }
               }
            }
            else
#endif
            {
               sp = row;
               for (i = 0; i < row_width; i++, sp += 4)
               {
                  ci_byte a = *(sp + 3);

                  if (a == 0)
                  {
                     *sp = (ci_byte)ci_ptr->background.red;
                     *(sp + 1) = (ci_byte)ci_ptr->background.green;
                     *(sp + 2) = (ci_byte)ci_ptr->background.blue;
                  }

                  else if (a < 0xff)
                  {
                     ci_composite(*sp, *sp, a, ci_ptr->background.red);

                     ci_composite(*(sp + 1), *(sp + 1), a,
                         ci_ptr->background.green);

                     ci_composite(*(sp + 2), *(sp + 2), a,
                         ci_ptr->background.blue);
                  }
               }
            }
         }
         else /* if (row_info->bit_depth == 16) */
         {
#ifdef CI_READ_GAMMA_SUPPORTED
            if (gamma_16 != NULL && gamma_16_from_1 != NULL &&
                gamma_16_to_1 != NULL)
            {
               sp = row;
               for (i = 0; i < row_width; i++, sp += 8)
               {
                  ci_uint_16 a = (ci_uint_16)(((ci_uint_16)(*(sp + 6))
                      << 8) + (ci_uint_16)(*(sp + 7)));

                  if (a == (ci_uint_16)0xffff)
                  {
                     ci_uint_16 v;

                     v = gamma_16[*(sp + 1) >> gamma_shift][*sp];
                     *sp = (ci_byte)((v >> 8) & 0xff);
                     *(sp + 1) = (ci_byte)(v & 0xff);

                     v = gamma_16[*(sp + 3) >> gamma_shift][*(sp + 2)];
                     *(sp + 2) = (ci_byte)((v >> 8) & 0xff);
                     *(sp + 3) = (ci_byte)(v & 0xff);

                     v = gamma_16[*(sp + 5) >> gamma_shift][*(sp + 4)];
                     *(sp + 4) = (ci_byte)((v >> 8) & 0xff);
                     *(sp + 5) = (ci_byte)(v & 0xff);
                  }

                  else if (a == 0)
                  {
                     /* Background is already in screen gamma */
                     *sp = (ci_byte)((ci_ptr->background.red >> 8) & 0xff);
                     *(sp + 1) = (ci_byte)(ci_ptr->background.red & 0xff);
                     *(sp + 2) = (ci_byte)((ci_ptr->background.green >> 8)
                             & 0xff);
                     *(sp + 3) = (ci_byte)(ci_ptr->background.green
                             & 0xff);
                     *(sp + 4) = (ci_byte)((ci_ptr->background.blue >> 8)
                             & 0xff);
                     *(sp + 5) = (ci_byte)(ci_ptr->background.blue & 0xff);
                  }

                  else
                  {
                     ci_uint_16 v, w;

                     v = gamma_16_to_1[*(sp + 1) >> gamma_shift][*sp];
                     ci_composite_16(w, v, a, ci_ptr->background_1.red);
                     if (optimize == 0)
                        w = gamma_16_from_1[((w & 0xff) >> gamma_shift)][w >>
                             8];
                     *sp = (ci_byte)((w >> 8) & 0xff);
                     *(sp + 1) = (ci_byte)(w & 0xff);

                     v = gamma_16_to_1[*(sp + 3) >> gamma_shift][*(sp + 2)];
                     ci_composite_16(w, v, a, ci_ptr->background_1.green);
                     if (optimize == 0)
                        w = gamma_16_from_1[((w & 0xff) >> gamma_shift)][w >>
                             8];

                     *(sp + 2) = (ci_byte)((w >> 8) & 0xff);
                     *(sp + 3) = (ci_byte)(w & 0xff);

                     v = gamma_16_to_1[*(sp + 5) >> gamma_shift][*(sp + 4)];
                     ci_composite_16(w, v, a, ci_ptr->background_1.blue);
                     if (optimize == 0)
                        w = gamma_16_from_1[((w & 0xff) >> gamma_shift)][w >>
                             8];

                     *(sp + 4) = (ci_byte)((w >> 8) & 0xff);
                     *(sp + 5) = (ci_byte)(w & 0xff);
                  }
               }
            }

            else
#endif
            {
               sp = row;
               for (i = 0; i < row_width; i++, sp += 8)
               {
                  ci_uint_16 a = (ci_uint_16)(((ci_uint_16)(*(sp + 6))
                      << 8) + (ci_uint_16)(*(sp + 7)));

                  if (a == 0)
                  {
                     *sp = (ci_byte)((ci_ptr->background.red >> 8) & 0xff);
                     *(sp + 1) = (ci_byte)(ci_ptr->background.red & 0xff);
                     *(sp + 2) = (ci_byte)((ci_ptr->background.green >> 8)
                             & 0xff);
                     *(sp + 3) = (ci_byte)(ci_ptr->background.green
                             & 0xff);
                     *(sp + 4) = (ci_byte)((ci_ptr->background.blue >> 8)
                             & 0xff);
                     *(sp + 5) = (ci_byte)(ci_ptr->background.blue & 0xff);
                  }

                  else if (a < 0xffff)
                  {
                     ci_uint_16 v;

                     ci_uint_16 r = (ci_uint_16)(((*sp) << 8) + *(sp + 1));
                     ci_uint_16 g = (ci_uint_16)(((*(sp + 2)) << 8)
                         + *(sp + 3));
                     ci_uint_16 b = (ci_uint_16)(((*(sp + 4)) << 8)
                         + *(sp + 5));

                     ci_composite_16(v, r, a, ci_ptr->background.red);
                     *sp = (ci_byte)((v >> 8) & 0xff);
                     *(sp + 1) = (ci_byte)(v & 0xff);

                     ci_composite_16(v, g, a, ci_ptr->background.green);
                     *(sp + 2) = (ci_byte)((v >> 8) & 0xff);
                     *(sp + 3) = (ci_byte)(v & 0xff);

                     ci_composite_16(v, b, a, ci_ptr->background.blue);
                     *(sp + 4) = (ci_byte)((v >> 8) & 0xff);
                     *(sp + 5) = (ci_byte)(v & 0xff);
                  }
               }
            }
         }
         break;
      }

      default:
         break;
   }
}
#endif /* READ_BACKGROUND || READ_ALPHA_MODE */

#ifdef CI_READ_GAMMA_SUPPORTED
/* Gamma correct the image, avoiding the alpha channel.  Make sure
 * you do this after you deal with the transparency issue on grayscale
 * or RGB images. If your bit depth is 8, use gamma_table, if it
 * is 16, use gamma_16_table and gamma_shift.  Build these with
 * build_gamma_table().
 */
static void
ci_do_gamma(ci_row_infop row_info, ci_bytep row, ci_structrp ci_ptr)
{
   ci_const_bytep gamma_table = ci_ptr->gamma_table;
   ci_const_uint_16pp gamma_16_table = ci_ptr->gamma_16_table;
   int gamma_shift = ci_ptr->gamma_shift;

   ci_bytep sp;
   ci_uint_32 i;
   ci_uint_32 row_width=row_info->width;

   ci_debug(1, "in ci_do_gamma");

   if (((row_info->bit_depth <= 8 && gamma_table != NULL) ||
       (row_info->bit_depth == 16 && gamma_16_table != NULL)))
   {
      switch (row_info->color_type)
      {
         case CI_COLOR_TYPE_RGB:
         {
            if (row_info->bit_depth == 8)
            {
               sp = row;
               for (i = 0; i < row_width; i++)
               {
                  *sp = gamma_table[*sp];
                  sp++;
                  *sp = gamma_table[*sp];
                  sp++;
                  *sp = gamma_table[*sp];
                  sp++;
               }
            }

            else /* if (row_info->bit_depth == 16) */
            {
               sp = row;
               for (i = 0; i < row_width; i++)
               {
                  ci_uint_16 v;

                  v = gamma_16_table[*(sp + 1) >> gamma_shift][*sp];
                  *sp = (ci_byte)((v >> 8) & 0xff);
                  *(sp + 1) = (ci_byte)(v & 0xff);
                  sp += 2;

                  v = gamma_16_table[*(sp + 1) >> gamma_shift][*sp];
                  *sp = (ci_byte)((v >> 8) & 0xff);
                  *(sp + 1) = (ci_byte)(v & 0xff);
                  sp += 2;

                  v = gamma_16_table[*(sp + 1) >> gamma_shift][*sp];
                  *sp = (ci_byte)((v >> 8) & 0xff);
                  *(sp + 1) = (ci_byte)(v & 0xff);
                  sp += 2;
               }
            }
            break;
         }

         case CI_COLOR_TYPE_RGB_ALPHA:
         {
            if (row_info->bit_depth == 8)
            {
               sp = row;
               for (i = 0; i < row_width; i++)
               {
                  *sp = gamma_table[*sp];
                  sp++;

                  *sp = gamma_table[*sp];
                  sp++;

                  *sp = gamma_table[*sp];
                  sp++;

                  sp++;
               }
            }

            else /* if (row_info->bit_depth == 16) */
            {
               sp = row;
               for (i = 0; i < row_width; i++)
               {
                  ci_uint_16 v = gamma_16_table[*(sp + 1) >> gamma_shift][*sp];
                  *sp = (ci_byte)((v >> 8) & 0xff);
                  *(sp + 1) = (ci_byte)(v & 0xff);
                  sp += 2;

                  v = gamma_16_table[*(sp + 1) >> gamma_shift][*sp];
                  *sp = (ci_byte)((v >> 8) & 0xff);
                  *(sp + 1) = (ci_byte)(v & 0xff);
                  sp += 2;

                  v = gamma_16_table[*(sp + 1) >> gamma_shift][*sp];
                  *sp = (ci_byte)((v >> 8) & 0xff);
                  *(sp + 1) = (ci_byte)(v & 0xff);
                  sp += 4;
               }
            }
            break;
         }

         case CI_COLOR_TYPE_GRAY_ALPHA:
         {
            if (row_info->bit_depth == 8)
            {
               sp = row;
               for (i = 0; i < row_width; i++)
               {
                  *sp = gamma_table[*sp];
                  sp += 2;
               }
            }

            else /* if (row_info->bit_depth == 16) */
            {
               sp = row;
               for (i = 0; i < row_width; i++)
               {
                  ci_uint_16 v = gamma_16_table[*(sp + 1) >> gamma_shift][*sp];
                  *sp = (ci_byte)((v >> 8) & 0xff);
                  *(sp + 1) = (ci_byte)(v & 0xff);
                  sp += 4;
               }
            }
            break;
         }

         case CI_COLOR_TYPE_GRAY:
         {
            if (row_info->bit_depth == 2)
            {
               sp = row;
               for (i = 0; i < row_width; i += 4)
               {
                  int a = *sp & 0xc0;
                  int b = *sp & 0x30;
                  int c = *sp & 0x0c;
                  int d = *sp & 0x03;

                  *sp = (ci_byte)(
                      ((((int)gamma_table[a|(a>>2)|(a>>4)|(a>>6)])   ) & 0xc0)|
                      ((((int)gamma_table[(b<<2)|b|(b>>2)|(b>>4)])>>2) & 0x30)|
                      ((((int)gamma_table[(c<<4)|(c<<2)|c|(c>>2)])>>4) & 0x0c)|
                      ((((int)gamma_table[(d<<6)|(d<<4)|(d<<2)|d])>>6) ));
                  sp++;
               }
            }

            if (row_info->bit_depth == 4)
            {
               sp = row;
               for (i = 0; i < row_width; i += 2)
               {
                  int msb = *sp & 0xf0;
                  int lsb = *sp & 0x0f;

                  *sp = (ci_byte)((((int)gamma_table[msb | (msb >> 4)]) & 0xf0)
                      | (((int)gamma_table[(lsb << 4) | lsb]) >> 4));
                  sp++;
               }
            }

            else if (row_info->bit_depth == 8)
            {
               sp = row;
               for (i = 0; i < row_width; i++)
               {
                  *sp = gamma_table[*sp];
                  sp++;
               }
            }

            else if (row_info->bit_depth == 16)
            {
               sp = row;
               for (i = 0; i < row_width; i++)
               {
                  ci_uint_16 v = gamma_16_table[*(sp + 1) >> gamma_shift][*sp];
                  *sp = (ci_byte)((v >> 8) & 0xff);
                  *(sp + 1) = (ci_byte)(v & 0xff);
                  sp += 2;
               }
            }
            break;
         }

         default:
            break;
      }
   }
}
#endif

#ifdef CI_READ_ALPHA_MODE_SUPPORTED
/* Encode the alpha channel to the output gamma (the input channel is always
 * linear.)  Called only with color types that have an alpha channel.  Needs the
 * from_1 tables.
 */
static void
ci_do_encode_alpha(ci_row_infop row_info, ci_bytep row, ci_structrp ci_ptr)
{
   ci_uint_32 row_width = row_info->width;

   ci_debug(1, "in ci_do_encode_alpha");

   if ((row_info->color_type & CI_COLOR_MASK_ALPHA) != 0)
   {
      if (row_info->bit_depth == 8)
      {
         ci_bytep table = ci_ptr->gamma_from_1;

         if (table != NULL)
         {
            int step = (row_info->color_type & CI_COLOR_MASK_COLOR) ? 4 : 2;

            /* The alpha channel is the last component: */
            row += step - 1;

            for (; row_width > 0; --row_width, row += step)
               *row = table[*row];

            return;
         }
      }

      else if (row_info->bit_depth == 16)
      {
         ci_uint_16pp table = ci_ptr->gamma_16_from_1;
         int gamma_shift = ci_ptr->gamma_shift;

         if (table != NULL)
         {
            int step = (row_info->color_type & CI_COLOR_MASK_COLOR) ? 8 : 4;

            /* The alpha channel is the last component: */
            row += step - 2;

            for (; row_width > 0; --row_width, row += step)
            {
               ci_uint_16 v;

               v = table[*(row + 1) >> gamma_shift][*row];
               *row = (ci_byte)((v >> 8) & 0xff);
               *(row + 1) = (ci_byte)(v & 0xff);
            }

            return;
         }
      }
   }

   /* Only get to here if called with a weird row_info; no harm has been done,
    * so just issue a warning.
    */
   ci_warning(ci_ptr, "ci_do_encode_alpha: unexpected call");
}
#endif

#ifdef CI_READ_EXPAND_SUPPORTED
/* Expands a palette row to an RGB or RGBA row depending
 * upon whether you supply trans and num_trans.
 */
static void
ci_do_expand_palette(ci_structrp ci_ptr, ci_row_infop row_info,
    ci_bytep row, ci_const_colorp palette, ci_const_bytep trans_alpha,
    int num_trans)
{
   int shift, value;
   ci_bytep sp, dp;
   ci_uint_32 i;
   ci_uint_32 row_width=row_info->width;

   ci_debug(1, "in ci_do_expand_palette");

   if (row_info->color_type == CI_COLOR_TYPE_PALETTE)
   {
      if (row_info->bit_depth < 8)
      {
         switch (row_info->bit_depth)
         {
            case 1:
            {
               sp = row + (size_t)((row_width - 1) >> 3);
               dp = row + (size_t)row_width - 1;
               shift = 7 - (int)((row_width + 7) & 0x07);
               for (i = 0; i < row_width; i++)
               {
                  if ((*sp >> shift) & 0x01)
                     *dp = 1;

                  else
                     *dp = 0;

                  if (shift == 7)
                  {
                     shift = 0;
                     sp--;
                  }

                  else
                     shift++;

                  dp--;
               }
               break;
            }

            case 2:
            {
               sp = row + (size_t)((row_width - 1) >> 2);
               dp = row + (size_t)row_width - 1;
               shift = (int)((3 - ((row_width + 3) & 0x03)) << 1);
               for (i = 0; i < row_width; i++)
               {
                  value = (*sp >> shift) & 0x03;
                  *dp = (ci_byte)value;
                  if (shift == 6)
                  {
                     shift = 0;
                     sp--;
                  }

                  else
                     shift += 2;

                  dp--;
               }
               break;
            }

            case 4:
            {
               sp = row + (size_t)((row_width - 1) >> 1);
               dp = row + (size_t)row_width - 1;
               shift = (int)((row_width & 0x01) << 2);
               for (i = 0; i < row_width; i++)
               {
                  value = (*sp >> shift) & 0x0f;
                  *dp = (ci_byte)value;
                  if (shift == 4)
                  {
                     shift = 0;
                     sp--;
                  }

                  else
                     shift += 4;

                  dp--;
               }
               break;
            }

            default:
               break;
         }
         row_info->bit_depth = 8;
         row_info->pixel_depth = 8;
         row_info->rowbytes = row_width;
      }

      if (row_info->bit_depth == 8)
      {
         {
            if (num_trans > 0)
            {
               sp = row + (size_t)row_width - 1;
               dp = row + ((size_t)row_width << 2) - 1;

               i = 0;
#ifdef CI_ARM_NEON_INTRINSICS_AVAILABLE
               if (ci_ptr->riffled_palette != NULL)
               {
                  /* The RGBA optimization works with ci_ptr->bit_depth == 8
                   * but sometimes row_info->bit_depth has been changed to 8.
                   * In these cases, the palette hasn't been riffled.
                   */
                  i = ci_do_expand_palette_rgba8_neon(ci_ptr, row_info, row,
                      &sp, &dp);
               }
#else
               CI_UNUSED(ci_ptr)
#endif

               for (; i < row_width; i++)
               {
                  if ((int)(*sp) >= num_trans)
                     *dp-- = 0xff;
                  else
                     *dp-- = trans_alpha[*sp];
                  *dp-- = palette[*sp].blue;
                  *dp-- = palette[*sp].green;
                  *dp-- = palette[*sp].red;
                  sp--;
               }
               row_info->bit_depth = 8;
               row_info->pixel_depth = 32;
               row_info->rowbytes = row_width * 4;
               row_info->color_type = 6;
               row_info->channels = 4;
            }

            else
            {
               sp = row + (size_t)row_width - 1;
               dp = row + (size_t)(row_width * 3) - 1;
               i = 0;
#ifdef CI_ARM_NEON_INTRINSICS_AVAILABLE
               i = ci_do_expand_palette_rgb8_neon(ci_ptr, row_info, row,
                   &sp, &dp);
#else
               CI_UNUSED(ci_ptr)
#endif

               for (; i < row_width; i++)
               {
                  *dp-- = palette[*sp].blue;
                  *dp-- = palette[*sp].green;
                  *dp-- = palette[*sp].red;
                  sp--;
               }

               row_info->bit_depth = 8;
               row_info->pixel_depth = 24;
               row_info->rowbytes = row_width * 3;
               row_info->color_type = 2;
               row_info->channels = 3;
            }
         }
      }
   }
}

/* If the bit depth < 8, it is expanded to 8.  Also, if the already
 * expanded transparency value is supplied, an alpha channel is built.
 */
static void
ci_do_expand(ci_row_infop row_info, ci_bytep row,
    ci_const_color_16p trans_color)
{
   int shift, value;
   ci_bytep sp, dp;
   ci_uint_32 i;
   ci_uint_32 row_width=row_info->width;

   ci_debug(1, "in ci_do_expand");

   if (row_info->color_type == CI_COLOR_TYPE_GRAY)
   {
      unsigned int gray = trans_color != NULL ? trans_color->gray : 0;

      if (row_info->bit_depth < 8)
      {
         switch (row_info->bit_depth)
         {
            case 1:
            {
               gray = (gray & 0x01) * 0xff;
               sp = row + (size_t)((row_width - 1) >> 3);
               dp = row + (size_t)row_width - 1;
               shift = 7 - (int)((row_width + 7) & 0x07);
               for (i = 0; i < row_width; i++)
               {
                  if ((*sp >> shift) & 0x01)
                     *dp = 0xff;

                  else
                     *dp = 0;

                  if (shift == 7)
                  {
                     shift = 0;
                     sp--;
                  }

                  else
                     shift++;

                  dp--;
               }
               break;
            }

            case 2:
            {
               gray = (gray & 0x03) * 0x55;
               sp = row + (size_t)((row_width - 1) >> 2);
               dp = row + (size_t)row_width - 1;
               shift = (int)((3 - ((row_width + 3) & 0x03)) << 1);
               for (i = 0; i < row_width; i++)
               {
                  value = (*sp >> shift) & 0x03;
                  *dp = (ci_byte)(value | (value << 2) | (value << 4) |
                     (value << 6));
                  if (shift == 6)
                  {
                     shift = 0;
                     sp--;
                  }

                  else
                     shift += 2;

                  dp--;
               }
               break;
            }

            case 4:
            {
               gray = (gray & 0x0f) * 0x11;
               sp = row + (size_t)((row_width - 1) >> 1);
               dp = row + (size_t)row_width - 1;
               shift = (int)((1 - ((row_width + 1) & 0x01)) << 2);
               for (i = 0; i < row_width; i++)
               {
                  value = (*sp >> shift) & 0x0f;
                  *dp = (ci_byte)(value | (value << 4));
                  if (shift == 4)
                  {
                     shift = 0;
                     sp--;
                  }

                  else
                     shift = 4;

                  dp--;
               }
               break;
            }

            default:
               break;
         }

         row_info->bit_depth = 8;
         row_info->pixel_depth = 8;
         row_info->rowbytes = row_width;
      }

      if (trans_color != NULL)
      {
         if (row_info->bit_depth == 8)
         {
            gray = gray & 0xff;
            sp = row + (size_t)row_width - 1;
            dp = row + ((size_t)row_width << 1) - 1;

            for (i = 0; i < row_width; i++)
            {
               if ((*sp & 0xffU) == gray)
                  *dp-- = 0;

               else
                  *dp-- = 0xff;

               *dp-- = *sp--;
            }
         }

         else if (row_info->bit_depth == 16)
         {
            unsigned int gray_high = (gray >> 8) & 0xff;
            unsigned int gray_low = gray & 0xff;
            sp = row + row_info->rowbytes - 1;
            dp = row + (row_info->rowbytes << 1) - 1;
            for (i = 0; i < row_width; i++)
            {
               if ((*(sp - 1) & 0xffU) == gray_high &&
                   (*(sp) & 0xffU) == gray_low)
               {
                  *dp-- = 0;
                  *dp-- = 0;
               }

               else
               {
                  *dp-- = 0xff;
                  *dp-- = 0xff;
               }

               *dp-- = *sp--;
               *dp-- = *sp--;
            }
         }

         row_info->color_type = CI_COLOR_TYPE_GRAY_ALPHA;
         row_info->channels = 2;
         row_info->pixel_depth = (ci_byte)(row_info->bit_depth << 1);
         row_info->rowbytes = CI_ROWBYTES(row_info->pixel_depth,
             row_width);
      }
   }
   else if (row_info->color_type == CI_COLOR_TYPE_RGB &&
       trans_color != NULL)
   {
      if (row_info->bit_depth == 8)
      {
         ci_byte red = (ci_byte)(trans_color->red & 0xff);
         ci_byte green = (ci_byte)(trans_color->green & 0xff);
         ci_byte blue = (ci_byte)(trans_color->blue & 0xff);
         sp = row + (size_t)row_info->rowbytes - 1;
         dp = row + ((size_t)row_width << 2) - 1;
         for (i = 0; i < row_width; i++)
         {
            if (*(sp - 2) == red && *(sp - 1) == green && *(sp) == blue)
               *dp-- = 0;

            else
               *dp-- = 0xff;

            *dp-- = *sp--;
            *dp-- = *sp--;
            *dp-- = *sp--;
         }
      }
      else if (row_info->bit_depth == 16)
      {
         ci_byte red_high = (ci_byte)((trans_color->red >> 8) & 0xff);
         ci_byte green_high = (ci_byte)((trans_color->green >> 8) & 0xff);
         ci_byte blue_high = (ci_byte)((trans_color->blue >> 8) & 0xff);
         ci_byte red_low = (ci_byte)(trans_color->red & 0xff);
         ci_byte green_low = (ci_byte)(trans_color->green & 0xff);
         ci_byte blue_low = (ci_byte)(trans_color->blue & 0xff);
         sp = row + row_info->rowbytes - 1;
         dp = row + ((size_t)row_width << 3) - 1;
         for (i = 0; i < row_width; i++)
         {
            if (*(sp - 5) == red_high &&
                *(sp - 4) == red_low &&
                *(sp - 3) == green_high &&
                *(sp - 2) == green_low &&
                *(sp - 1) == blue_high &&
                *(sp    ) == blue_low)
            {
               *dp-- = 0;
               *dp-- = 0;
            }

            else
            {
               *dp-- = 0xff;
               *dp-- = 0xff;
            }

            *dp-- = *sp--;
            *dp-- = *sp--;
            *dp-- = *sp--;
            *dp-- = *sp--;
            *dp-- = *sp--;
            *dp-- = *sp--;
         }
      }
      row_info->color_type = CI_COLOR_TYPE_RGB_ALPHA;
      row_info->channels = 4;
      row_info->pixel_depth = (ci_byte)(row_info->bit_depth << 2);
      row_info->rowbytes = CI_ROWBYTES(row_info->pixel_depth, row_width);
   }
}
#endif

#ifdef CI_READ_EXPAND_16_SUPPORTED
/* If the bit depth is 8 and the color type is not a palette type expand the
 * whole row to 16 bits.  Has no effect otherwise.
 */
static void
ci_do_expand_16(ci_row_infop row_info, ci_bytep row)
{
   if (row_info->bit_depth == 8 &&
      row_info->color_type != CI_COLOR_TYPE_PALETTE)
   {
      /* The row have a sequence of bytes containing [0..255] and we need
       * to turn it into another row containing [0..65535], to do this we
       * calculate:
       *
       *  (input / 255) * 65535
       *
       *  Which happens to be exactly input * 257 and this can be achieved
       *  simply by byte replication in place (copying backwards).
       */
      ci_byte *sp = row + row_info->rowbytes; /* source, last byte + 1 */
      ci_byte *dp = sp + row_info->rowbytes;  /* destination, end + 1 */
      while (dp > sp)
      {
         dp[-2] = dp[-1] = *--sp; dp -= 2;
      }

      row_info->rowbytes *= 2;
      row_info->bit_depth = 16;
      row_info->pixel_depth = (ci_byte)(row_info->channels * 16);
   }
}
#endif

#ifdef CI_READ_QUANTIZE_SUPPORTED
static void
ci_do_quantize(ci_row_infop row_info, ci_bytep row,
    ci_const_bytep palette_lookup, ci_const_bytep quantize_lookup)
{
   ci_bytep sp, dp;
   ci_uint_32 i;
   ci_uint_32 row_width=row_info->width;

   ci_debug(1, "in ci_do_quantize");

   if (row_info->bit_depth == 8)
   {
      if (row_info->color_type == CI_COLOR_TYPE_RGB && palette_lookup)
      {
         int r, g, b, p;
         sp = row;
         dp = row;
         for (i = 0; i < row_width; i++)
         {
            r = *sp++;
            g = *sp++;
            b = *sp++;

            /* This looks real messy, but the compiler will reduce
             * it down to a reasonable formula.  For example, with
             * 5 bits per color, we get:
             * p = (((r >> 3) & 0x1f) << 10) |
             *    (((g >> 3) & 0x1f) << 5) |
             *    ((b >> 3) & 0x1f);
             */
            p = (((r >> (8 - CI_QUANTIZE_RED_BITS)) &
                ((1 << CI_QUANTIZE_RED_BITS) - 1)) <<
                (CI_QUANTIZE_GREEN_BITS + CI_QUANTIZE_BLUE_BITS)) |
                (((g >> (8 - CI_QUANTIZE_GREEN_BITS)) &
                ((1 << CI_QUANTIZE_GREEN_BITS) - 1)) <<
                (CI_QUANTIZE_BLUE_BITS)) |
                ((b >> (8 - CI_QUANTIZE_BLUE_BITS)) &
                ((1 << CI_QUANTIZE_BLUE_BITS) - 1));

            *dp++ = palette_lookup[p];
         }

         row_info->color_type = CI_COLOR_TYPE_PALETTE;
         row_info->channels = 1;
         row_info->pixel_depth = row_info->bit_depth;
         row_info->rowbytes = CI_ROWBYTES(row_info->pixel_depth, row_width);
      }

      else if (row_info->color_type == CI_COLOR_TYPE_RGB_ALPHA &&
         palette_lookup != NULL)
      {
         int r, g, b, p;
         sp = row;
         dp = row;
         for (i = 0; i < row_width; i++)
         {
            r = *sp++;
            g = *sp++;
            b = *sp++;
            sp++;

            p = (((r >> (8 - CI_QUANTIZE_RED_BITS)) &
                ((1 << CI_QUANTIZE_RED_BITS) - 1)) <<
                (CI_QUANTIZE_GREEN_BITS + CI_QUANTIZE_BLUE_BITS)) |
                (((g >> (8 - CI_QUANTIZE_GREEN_BITS)) &
                ((1 << CI_QUANTIZE_GREEN_BITS) - 1)) <<
                (CI_QUANTIZE_BLUE_BITS)) |
                ((b >> (8 - CI_QUANTIZE_BLUE_BITS)) &
                ((1 << CI_QUANTIZE_BLUE_BITS) - 1));

            *dp++ = palette_lookup[p];
         }

         row_info->color_type = CI_COLOR_TYPE_PALETTE;
         row_info->channels = 1;
         row_info->pixel_depth = row_info->bit_depth;
         row_info->rowbytes = CI_ROWBYTES(row_info->pixel_depth, row_width);
      }

      else if (row_info->color_type == CI_COLOR_TYPE_PALETTE &&
         quantize_lookup)
      {
         sp = row;

         for (i = 0; i < row_width; i++, sp++)
         {
            *sp = quantize_lookup[*sp];
         }
      }
   }
}
#endif /* READ_QUANTIZE */

/* Transform the row.  The order of transformations is significant,
 * and is very touchy.  If you add a transformation, take care to
 * decide how it fits in with the other transformations here.
 */
void /* PRIVATE */
ci_do_read_transformations(ci_structrp ci_ptr, ci_row_infop row_info)
{
   ci_debug(1, "in ci_do_read_transformations");

   if (ci_ptr->row_buf == NULL)
   {
      /* Prior to 1.5.4 this output row/pass where the NULL pointer is, but this
       * error is incredibly rare and incredibly easy to debug without this
       * information.
       */
      ci_error(ci_ptr, "NULL row buffer");
   }

   /* The following is debugging; prior to 1.5.4 the code was never compiled in;
    * in 1.5.4 CI_FLAG_DETECT_UNINITIALIZED was added and the macro
    * CI_WARN_UNINITIALIZED_ROW removed.  In 1.6 the new flag is set only for
    * all transformations, however in practice the ROW_INIT always gets done on
    * demand, if necessary.
    */
   if ((ci_ptr->flags & CI_FLAG_DETECT_UNINITIALIZED) != 0 &&
       (ci_ptr->flags & CI_FLAG_ROW_INIT) == 0)
   {
      /* Application has failed to call either ci_read_start_image() or
       * ci_read_update_info() after setting transforms that expand pixels.
       * This check added to libci-1.2.19 (but not enabled until 1.5.4).
       */
      ci_error(ci_ptr, "Uninitialized row");
   }

#ifdef CI_READ_EXPAND_SUPPORTED
   if ((ci_ptr->transformations & CI_EXPAND) != 0)
   {
      if (row_info->color_type == CI_COLOR_TYPE_PALETTE)
      {
#ifdef CI_ARM_NEON_INTRINSICS_AVAILABLE
         if ((ci_ptr->num_trans > 0) && (ci_ptr->bit_depth == 8))
         {
            if (ci_ptr->riffled_palette == NULL)
            {
               /* Initialize the accelerated palette expansion. */
               ci_ptr->riffled_palette =
                   (ci_bytep)ci_malloc(ci_ptr, 256 * 4);
               ci_riffle_palette_neon(ci_ptr);
            }
         }
#endif
         ci_do_expand_palette(ci_ptr, row_info, ci_ptr->row_buf + 1,
             ci_ptr->palette, ci_ptr->trans_alpha, ci_ptr->num_trans);
      }

      else
      {
         if (ci_ptr->num_trans != 0 &&
             (ci_ptr->transformations & CI_EXPAND_tRNS) != 0)
            ci_do_expand(row_info, ci_ptr->row_buf + 1,
                &(ci_ptr->trans_color));

         else
            ci_do_expand(row_info, ci_ptr->row_buf + 1, NULL);
      }
   }
#endif

#ifdef CI_READ_STRIP_ALPHA_SUPPORTED
   if ((ci_ptr->transformations & CI_STRIP_ALPHA) != 0 &&
       (ci_ptr->transformations & CI_COMPOSE) == 0 &&
       (row_info->color_type == CI_COLOR_TYPE_RGB_ALPHA ||
       row_info->color_type == CI_COLOR_TYPE_GRAY_ALPHA))
      ci_do_strip_channel(row_info, ci_ptr->row_buf + 1,
          0 /* at_start == false, because SWAP_ALPHA happens later */);
#endif

#ifdef CI_READ_RGB_TO_GRAY_SUPPORTED
   if ((ci_ptr->transformations & CI_RGB_TO_GRAY) != 0)
   {
      int rgb_error =
          ci_do_rgb_to_gray(ci_ptr, row_info,
              ci_ptr->row_buf + 1);

      if (rgb_error != 0)
      {
         ci_ptr->rgb_to_gray_status=1;
         if ((ci_ptr->transformations & CI_RGB_TO_GRAY) ==
             CI_RGB_TO_GRAY_WARN)
            ci_warning(ci_ptr, "ci_do_rgb_to_gray found nongray pixel");

         if ((ci_ptr->transformations & CI_RGB_TO_GRAY) ==
             CI_RGB_TO_GRAY_ERR)
            ci_error(ci_ptr, "ci_do_rgb_to_gray found nongray pixel");
      }
   }
#endif

/* From Andreas Dilger e-mail to ci-implement, 26 March 1998:
 *
 *   In most cases, the "simple transparency" should be done prior to doing
 *   gray-to-RGB, or you will have to test 3x as many bytes to check if a
 *   pixel is transparent.  You would also need to make sure that the
 *   transparency information is upgraded to RGB.
 *
 *   To summarize, the current flow is:
 *   - Gray + simple transparency -> compare 1 or 2 gray bytes and composite
 *                                   with background "in place" if transparent,
 *                                   convert to RGB if necessary
 *   - Gray + alpha -> composite with gray background and remove alpha bytes,
 *                                   convert to RGB if necessary
 *
 *   To support RGB backgrounds for gray images we need:
 *   - Gray + simple transparency -> convert to RGB + simple transparency,
 *                                   compare 3 or 6 bytes and composite with
 *                                   background "in place" if transparent
 *                                   (3x compare/pixel compared to doing
 *                                   composite with gray bkgrnd)
 *   - Gray + alpha -> convert to RGB + alpha, composite with background and
 *                                   remove alpha bytes (3x float
 *                                   operations/pixel compared with composite
 *                                   on gray background)
 *
 *  Greg's change will do this.  The reason it wasn't done before is for
 *  performance, as this increases the per-pixel operations.  If we would check
 *  in advance if the background was gray or RGB, and position the gray-to-RGB
 *  transform appropriately, then it would save a lot of work/time.
 */

#ifdef CI_READ_GRAY_TO_RGB_SUPPORTED
   /* If gray -> RGB, do so now only if background is non-gray; else do later
    * for performance reasons
    */
   if ((ci_ptr->transformations & CI_GRAY_TO_RGB) != 0 &&
       (ci_ptr->mode & CI_BACKGROUND_IS_GRAY) == 0)
      ci_do_gray_to_rgb(row_info, ci_ptr->row_buf + 1);
#endif

#if defined(CI_READ_BACKGROUND_SUPPORTED) ||\
   defined(CI_READ_ALPHA_MODE_SUPPORTED)
   if ((ci_ptr->transformations & CI_COMPOSE) != 0)
      ci_do_compose(row_info, ci_ptr->row_buf + 1, ci_ptr);
#endif

#ifdef CI_READ_GAMMA_SUPPORTED
   if ((ci_ptr->transformations & CI_GAMMA) != 0 &&
#ifdef CI_READ_RGB_TO_GRAY_SUPPORTED
      /* Because RGB_TO_GRAY does the gamma transform. */
      (ci_ptr->transformations & CI_RGB_TO_GRAY) == 0 &&
#endif
#if defined(CI_READ_BACKGROUND_SUPPORTED) ||\
   defined(CI_READ_ALPHA_MODE_SUPPORTED)
      /* Because CI_COMPOSE does the gamma transform if there is something to
       * do (if there is an alpha channel or transparency.)
       */
       !((ci_ptr->transformations & CI_COMPOSE) != 0 &&
       ((ci_ptr->num_trans != 0) ||
       (ci_ptr->color_type & CI_COLOR_MASK_ALPHA) != 0)) &&
#endif
      /* Because ci_init_read_transformations transforms the palette, unless
       * RGB_TO_GRAY will do the transform.
       */
       (ci_ptr->color_type != CI_COLOR_TYPE_PALETTE))
      ci_do_gamma(row_info, ci_ptr->row_buf + 1, ci_ptr);
#endif

#ifdef CI_READ_STRIP_ALPHA_SUPPORTED
   if ((ci_ptr->transformations & CI_STRIP_ALPHA) != 0 &&
       (ci_ptr->transformations & CI_COMPOSE) != 0 &&
       (row_info->color_type == CI_COLOR_TYPE_RGB_ALPHA ||
       row_info->color_type == CI_COLOR_TYPE_GRAY_ALPHA))
      ci_do_strip_channel(row_info, ci_ptr->row_buf + 1,
          0 /* at_start == false, because SWAP_ALPHA happens later */);
#endif

#ifdef CI_READ_ALPHA_MODE_SUPPORTED
   if ((ci_ptr->transformations & CI_ENCODE_ALPHA) != 0 &&
       (row_info->color_type & CI_COLOR_MASK_ALPHA) != 0)
      ci_do_encode_alpha(row_info, ci_ptr->row_buf + 1, ci_ptr);
#endif

#ifdef CI_READ_SCALE_16_TO_8_SUPPORTED
   if ((ci_ptr->transformations & CI_SCALE_16_TO_8) != 0)
      ci_do_scale_16_to_8(row_info, ci_ptr->row_buf + 1);
#endif

#ifdef CI_READ_STRIP_16_TO_8_SUPPORTED
   /* There is no harm in doing both of these because only one has any effect,
    * by putting the 'scale' option first if the app asks for scale (either by
    * calling the API or in a TRANSFORM flag) this is what happens.
    */
   if ((ci_ptr->transformations & CI_16_TO_8) != 0)
      ci_do_chop(row_info, ci_ptr->row_buf + 1);
#endif

#ifdef CI_READ_QUANTIZE_SUPPORTED
   if ((ci_ptr->transformations & CI_QUANTIZE) != 0)
   {
      ci_do_quantize(row_info, ci_ptr->row_buf + 1,
          ci_ptr->palette_lookup, ci_ptr->quantize_index);

      if (row_info->rowbytes == 0)
         ci_error(ci_ptr, "ci_do_quantize returned rowbytes=0");
   }
#endif /* READ_QUANTIZE */

#ifdef CI_READ_EXPAND_16_SUPPORTED
   /* Do the expansion now, after all the arithmetic has been done.  Notice
    * that previous transformations can handle the CI_EXPAND_16 flag if this
    * is efficient (particularly true in the case of gamma correction, where
    * better accuracy results faster!)
    */
   if ((ci_ptr->transformations & CI_EXPAND_16) != 0)
      ci_do_expand_16(row_info, ci_ptr->row_buf + 1);
#endif

#ifdef CI_READ_GRAY_TO_RGB_SUPPORTED
   /* NOTE: moved here in 1.5.4 (from much later in this list.) */
   if ((ci_ptr->transformations & CI_GRAY_TO_RGB) != 0 &&
       (ci_ptr->mode & CI_BACKGROUND_IS_GRAY) != 0)
      ci_do_gray_to_rgb(row_info, ci_ptr->row_buf + 1);
#endif

#ifdef CI_READ_INVERT_SUPPORTED
   if ((ci_ptr->transformations & CI_INVERT_MONO) != 0)
      ci_do_invert(row_info, ci_ptr->row_buf + 1);
#endif

#ifdef CI_READ_INVERT_ALPHA_SUPPORTED
   if ((ci_ptr->transformations & CI_INVERT_ALPHA) != 0)
      ci_do_read_invert_alpha(row_info, ci_ptr->row_buf + 1);
#endif

#ifdef CI_READ_SHIFT_SUPPORTED
   if ((ci_ptr->transformations & CI_SHIFT) != 0)
      ci_do_unshift(row_info, ci_ptr->row_buf + 1,
          &(ci_ptr->shift));
#endif

#ifdef CI_READ_PACK_SUPPORTED
   if ((ci_ptr->transformations & CI_PACK) != 0)
      ci_do_unpack(row_info, ci_ptr->row_buf + 1);
#endif

#ifdef CI_READ_CHECK_FOR_INVALID_INDEX_SUPPORTED
   /* Added at libci-1.5.10 */
   if (row_info->color_type == CI_COLOR_TYPE_PALETTE &&
       ci_ptr->num_palette_max >= 0)
      ci_do_check_palette_indexes(ci_ptr, row_info);
#endif

#ifdef CI_READ_BGR_SUPPORTED
   if ((ci_ptr->transformations & CI_BGR) != 0)
      ci_do_bgr(row_info, ci_ptr->row_buf + 1);
#endif

#ifdef CI_READ_PACKSWAP_SUPPORTED
   if ((ci_ptr->transformations & CI_PACKSWAP) != 0)
      ci_do_packswap(row_info, ci_ptr->row_buf + 1);
#endif

#ifdef CI_READ_FILLER_SUPPORTED
   if ((ci_ptr->transformations & CI_FILLER) != 0)
      ci_do_read_filler(row_info, ci_ptr->row_buf + 1,
          (ci_uint_32)ci_ptr->filler, ci_ptr->flags);
#endif

#ifdef CI_READ_SWAP_ALPHA_SUPPORTED
   if ((ci_ptr->transformations & CI_SWAP_ALPHA) != 0)
      ci_do_read_swap_alpha(row_info, ci_ptr->row_buf + 1);
#endif

#ifdef CI_READ_16BIT_SUPPORTED
#ifdef CI_READ_SWAP_SUPPORTED
   if ((ci_ptr->transformations & CI_SWAP_BYTES) != 0)
      ci_do_swap(row_info, ci_ptr->row_buf + 1);
#endif
#endif

#ifdef CI_READ_USER_TRANSFORM_SUPPORTED
   if ((ci_ptr->transformations & CI_USER_TRANSFORM) != 0)
   {
      if (ci_ptr->read_user_transform_fn != NULL)
         (*(ci_ptr->read_user_transform_fn)) /* User read transform function */
             (ci_ptr,     /* ci_ptr */
             row_info,     /* row_info: */
                /*  ci_uint_32 width;       width of row */
                /*  size_t rowbytes;         number of bytes in row */
                /*  ci_byte color_type;     color type of pixels */
                /*  ci_byte bit_depth;      bit depth of samples */
                /*  ci_byte channels;       number of channels (1-4) */
                /*  ci_byte pixel_depth;    bits per pixel (depth*channels) */
             ci_ptr->row_buf + 1);    /* start of pixel data for row */
#ifdef CI_USER_TRANSFORM_PTR_SUPPORTED
      if (ci_ptr->user_transform_depth != 0)
         row_info->bit_depth = ci_ptr->user_transform_depth;

      if (ci_ptr->user_transform_channels != 0)
         row_info->channels = ci_ptr->user_transform_channels;
#endif
      row_info->pixel_depth = (ci_byte)(row_info->bit_depth *
          row_info->channels);

      row_info->rowbytes = CI_ROWBYTES(row_info->pixel_depth, row_info->width);
   }
#endif
}

#endif /* READ_TRANSFORMS */
#endif /* READ */

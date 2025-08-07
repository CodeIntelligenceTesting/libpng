/* arm_init.c - NEON optimised filter functions
 *
 * Copyright (c) 2018-2022 Cosmin Truta
 * Copyright (c) 2014,2016 Glenn Randers-Pehrson
 * Written by Mans Rullgard, 2011.
 *
 * This code is released under the libci license.
 * For conditions of distribution and use, see the disclaimer
 * and license in ci.h
 */

/* This module requires POSIX 1003.1 functions. */
#define _POSIX_SOURCE 1

#include "../cipriv.h"

#ifdef CI_READ_SUPPORTED

#if CI_ARM_NEON_OPT > 0
#ifdef CI_ARM_NEON_CHECK_SUPPORTED /* Do run-time checks */
/* WARNING: it is strongly recommended that you do not build libci with
 * run-time checks for CPU features if at all possible.  In the case of the ARM
 * NEON instructions there is no processor-specific way of detecting the
 * presence of the required support, therefore run-time detection is extremely
 * OS specific.
 *
 * You may set the macro CI_ARM_NEON_FILE to the file name of file containing
 * a fragment of C source code which defines the ci_have_neon function.  There
 * are a number of implementations in contrib/arm-neon, but the only one that
 * has partial support is contrib/arm-neon/linux.c - a generic Linux
 * implementation which reads /proc/cpufino.
 */
#include <signal.h> /* for sig_atomic_t */

#ifndef CI_ARM_NEON_FILE
#  if defined(__aarch64__) || defined(_M_ARM64)
     /* ARM Neon is expected to be unconditionally available on ARM64. */
#    error CI_ARM_NEON_CHECK_SUPPORTED must not be defined on ARM64
#  elif defined(__ARM_NEON__) || defined(__ARM_NEON)
     /* ARM Neon is expected to be available on the target CPU architecture. */
#    error CI_ARM_NEON_CHECK_SUPPORTED must not be defined on this CPU arch
#  elif defined(__linux__)
#    define CI_ARM_NEON_FILE "contrib/arm-neon/linux.c"
#  else
#    error No support for run-time ARM Neon checking; use compile-time options
#  endif
#endif

static int ci_have_neon(ci_structp ci_ptr);
#ifdef CI_ARM_NEON_FILE
#  include CI_ARM_NEON_FILE
#endif
#endif /* CI_ARM_NEON_CHECK_SUPPORTED */

#ifndef CI_ALIGNED_MEMORY_SUPPORTED
#  error ALIGNED_MEMORY is required; please define CI_ALIGNED_MEMORY_SUPPORTED
#endif

void
ci_init_filter_functions_neon(ci_structp pp, unsigned int bpp)
{
   /* The switch statement is compiled in for ARM_NEON_API, the call to
    * ci_have_neon is compiled in for ARM_NEON_CHECK.  If both are defined
    * the check is only performed if the API has not set the NEON option on
    * or off explicitly.  In this case the check controls what happens.
    *
    * If the CHECK is not compiled in and the option is UNSET the behavior prior
    * to 1.6.7 was to use the NEON code - this was a bug caused by having the
    * wrong order of the 'ON' and 'default' cases.  UNSET now defaults to OFF,
    * as documented in ci.h
    */
   ci_debug(1, "in ci_init_filter_functions_neon");
#ifdef CI_ARM_NEON_API_SUPPORTED
   switch ((pp->options >> CI_ARM_NEON) & 3)
   {
      case CI_OPTION_UNSET:
         /* Allow the run-time check to execute if it has been enabled -
          * thus both API and CHECK can be turned on.  If it isn't supported
          * this case will fall through to the 'default' below, which just
          * returns.
          */
#endif /* CI_ARM_NEON_API_SUPPORTED */
#ifdef CI_ARM_NEON_CHECK_SUPPORTED
         {
            static volatile sig_atomic_t no_neon = -1; /* not checked */

            if (no_neon < 0)
               no_neon = !ci_have_neon(pp);

            if (no_neon)
               return;
         }
#ifdef CI_ARM_NEON_API_SUPPORTED
         break;
#endif
#endif /* CI_ARM_NEON_CHECK_SUPPORTED */

#ifdef CI_ARM_NEON_API_SUPPORTED
      default: /* OFF or INVALID */
         return;

      case CI_OPTION_ON:
         /* Option turned on */
         break;
   }
#endif

   /* IMPORTANT: any new external functions used here must be declared using
    * CI_INTERNAL_FUNCTION in ../cipriv.h.  This is required so that the
    * 'prefix' option to configure works:
    *
    *    ./configure --with-libci-prefix=foobar_
    *
    * Verify you have got this right by running the above command, doing a build
    * and examining ciprefix.h; it must contain a #define for every external
    * function you add.  (Notice that this happens automatically for the
    * initialization function.)
    */
   pp->read_filter[CI_FILTER_VALUE_UP-1] = ci_read_filter_row_up_neon;

   if (bpp == 3)
   {
      pp->read_filter[CI_FILTER_VALUE_SUB-1] = ci_read_filter_row_sub3_neon;
      pp->read_filter[CI_FILTER_VALUE_AVG-1] = ci_read_filter_row_avg3_neon;
      pp->read_filter[CI_FILTER_VALUE_PAETH-1] =
         ci_read_filter_row_paeth3_neon;
   }

   else if (bpp == 4)
   {
      pp->read_filter[CI_FILTER_VALUE_SUB-1] = ci_read_filter_row_sub4_neon;
      pp->read_filter[CI_FILTER_VALUE_AVG-1] = ci_read_filter_row_avg4_neon;
      pp->read_filter[CI_FILTER_VALUE_PAETH-1] =
          ci_read_filter_row_paeth4_neon;
   }
}
#endif /* CI_ARM_NEON_OPT > 0 */
#endif /* READ */

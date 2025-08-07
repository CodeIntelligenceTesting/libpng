/* mips_init.c - MSA optimised filter functions
 *
 * Copyright (c) 2018-2024 Cosmin Truta
 * Copyright (c) 2016 Glenn Randers-Pehrson
 * Written by Mandar Sahastrabuddhe, 2016
 * Updated by guxiwei, 2023
 *
 * This code is released under the libci license.
 * For conditions of distribution and use, see the disclaimer
 * and license in ci.h
 */

/* Below, after checking __linux__, various non-C90 POSIX 1003.1 functions are
 * called.
 */
#define _POSIX_SOURCE 1

#include <stdio.h>
#include "../cipriv.h"

#ifdef CI_READ_SUPPORTED

#if CI_MIPS_MSA_IMPLEMENTATION == 1 || CI_MIPS_MMI_IMPLEMENTATION > 0

#ifdef CI_MIPS_MSA_CHECK_SUPPORTED /* Do MIPS MSA run-time checks */
/* WARNING: it is strongly recommended that you do not build libci with
 * run-time checks for CPU features if at all possible.  In the case of the MIPS
 * MSA instructions there is no processor-specific way of detecting the
 * presence of the required support, therefore run-time detection is extremely
 * OS specific.
 *
 * You may set the macro CI_MIPS_MSA_FILE to the file name of file containing
 * a fragment of C source code which defines the ci_have_msa function.  There
 * are a number of implementations in contrib/mips-msa, but the only one that
 * has partial support is contrib/mips-msa/linux.c - a generic Linux
 * implementation which reads /proc/cpufino.
 */
#ifndef CI_MIPS_MSA_FILE
#  ifdef __linux__
#     define CI_MIPS_MSA_FILE "contrib/mips-msa/linux.c"
#  endif
#endif

#ifdef CI_MIPS_MSA_FILE

#include <signal.h> /* for sig_atomic_t */
static int ci_have_msa(ci_structp ci_ptr);
#include CI_MIPS_MSA_FILE

#else  /* CI_MIPS_MSA_FILE */
#  error CI_MIPS_MSA_FILE undefined: no support for run-time MIPS MSA checks
#endif /* CI_MIPS_MSA_FILE */
#endif /* CI_MIPS_MSA_CHECK_SUPPORTED */

#ifdef CI_MIPS_MMI_CHECK_SUPPORTED /* Do MIPS MMI run-times checks */
#ifndef CI_MIPS_MMI_FILE
#  ifdef __linux__
#     define CI_MIPS_MMI_FILE "contrib/mips-mmi/linux.c"
#  endif
#endif

#ifdef CI_MIPS_MMI_FILE

#include <signal.h> /* for sig_atomic_t */
static int ci_have_mmi();
#include CI_MIPS_MMI_FILE

#else  /* CI_MIPS_MMI_FILE */
#  error CI_MIPS_MMI_FILE undefined: no support for run-time MIPS MMI checks
#endif /* CI_MIPS_MMI_FILE */
#endif /* CI_MIPS_MMI_CHECK_SUPPORTED*/

#ifndef CI_ALIGNED_MEMORY_SUPPORTED
#  error ALIGNED_MEMORY is required; please define CI_ALIGNED_MEMORY_SUPPORTED
#endif

/* MIPS supports two optimizations: MMI and MSA. The appropriate
 * optimization is chosen at runtime
 */
void
ci_init_filter_functions_mips(ci_structp pp, unsigned int bpp)
{
#if CI_MIPS_MMI_IMPLEMENTATION  > 0
#ifdef CI_MIPS_MMI_API_SUPPORTED
   switch ((pp->options >> CI_MIPS_MMI) & 3)
   {
      case CI_OPTION_UNSET:
#endif /* CI_MIPS_MMI_API_SUPPORTED */
#ifdef CI_MIPS_MMI_CHECK_SUPPORTED
         {
            static volatile sig_atomic_t no_mmi = -1; /* not checked */

            if (no_mmi < 0)
               no_mmi = !ci_have_mmi();

            if (no_mmi)
              goto MIPS_MSA_INIT;
         }
#ifdef CI_MIPS_MMI_API_SUPPORTED
         break;
#endif
#endif /* CI_MIPS_MMI_CHECK_SUPPORTED */

#ifdef CI_MIPS_MMI_API_SUPPORTED
      default: /* OFF or INVALID */
         goto MIPS_MSA_INIT;

      case CI_OPTION_ON:
         /* Option turned on */
         break;
   }
#endif
   pp->read_filter[CI_FILTER_VALUE_UP-1] = ci_read_filter_row_up_mmi;
   if (bpp == 3)
   {
      pp->read_filter[CI_FILTER_VALUE_SUB-1] = ci_read_filter_row_sub3_mmi;
      pp->read_filter[CI_FILTER_VALUE_AVG-1] = ci_read_filter_row_avg3_mmi;
      pp->read_filter[CI_FILTER_VALUE_PAETH-1] =
         ci_read_filter_row_paeth3_mmi;
   }
   else if (bpp == 4)
   {
      pp->read_filter[CI_FILTER_VALUE_SUB-1] = ci_read_filter_row_sub4_mmi;
      pp->read_filter[CI_FILTER_VALUE_AVG-1] = ci_read_filter_row_avg4_mmi;
      pp->read_filter[CI_FILTER_VALUE_PAETH-1] =
          ci_read_filter_row_paeth4_mmi;
   }
#endif /* CI_MIPS_MMI_IMPLEMENTATION > 0 */

MIPS_MSA_INIT:
#if CI_MIPS_MSA_IMPLEMENTATION == 1
   /* The switch statement is compiled in for MIPS_MSA_API, the call to
    * ci_have_msa is compiled in for MIPS_MSA_CHECK. If both are defined
    * the check is only performed if the API has not set the MSA option on
    * or off explicitly. In this case the check controls what happens.
    */

#ifdef CI_MIPS_MSA_API_SUPPORTED
   switch ((pp->options >> CI_MIPS_MSA) & 3)
   {
      case CI_OPTION_UNSET:
         /* Allow the run-time check to execute if it has been enabled -
          * thus both API and CHECK can be turned on.  If it isn't supported
          * this case will fall through to the 'default' below, which just
          * returns.
          */
#endif /* CI_MIPS_MSA_API_SUPPORTED */
#ifdef CI_MIPS_MSA_CHECK_SUPPORTED
         {
            static volatile sig_atomic_t no_msa = -1; /* not checked */

            if (no_msa < 0)
               no_msa = !ci_have_msa(pp);

            if (no_msa)
               return;
         }
#ifdef CI_MIPS_MSA_API_SUPPORTED
         break;
#endif
#endif /* CI_MIPS_MSA_CHECK_SUPPORTED */

#ifdef CI_MIPS_MSA_API_SUPPORTED
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
   pp->read_filter[CI_FILTER_VALUE_UP-1] = ci_read_filter_row_up_msa;

   if (bpp == 3)
   {
      pp->read_filter[CI_FILTER_VALUE_SUB-1] = ci_read_filter_row_sub3_msa;
      pp->read_filter[CI_FILTER_VALUE_AVG-1] = ci_read_filter_row_avg3_msa;
      pp->read_filter[CI_FILTER_VALUE_PAETH-1] = ci_read_filter_row_paeth3_msa;
   }

   else if (bpp == 4)
   {
      pp->read_filter[CI_FILTER_VALUE_SUB-1] = ci_read_filter_row_sub4_msa;
      pp->read_filter[CI_FILTER_VALUE_AVG-1] = ci_read_filter_row_avg4_msa;
      pp->read_filter[CI_FILTER_VALUE_PAETH-1] = ci_read_filter_row_paeth4_msa;
   }
#endif /* CI_MIPS_MSA_IMPLEMENTATION == 1 */
   return;
}
#endif /* CI_MIPS_MSA_IMPLEMENTATION == 1 || CI_MIPS_MMI_IMPLEMENTATION > 0 */
#endif /* READ */

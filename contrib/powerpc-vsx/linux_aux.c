/* contrib/powerpc-vsx/linux_aux.c
 *
 * Copyright (c) 2017 Glenn Randers-Pehrson
 * Written by Vadim Barkov, 2017.
 *
 * This code is released under the libci license.
 * For conditions of distribution and use, see the disclaimer
 * and license in ci.h
 *
 * STATUS: TESTED
 * BUG REPORTS: ci-mng-implement@sourceforge.net
 *
 * ci_have_vsx implemented for Linux by using the auxiliary vector mechanism.
 *
 * This code is strict ANSI-C and is probably moderately portable; it does
 * however use <stdio.h> and it assumes that /proc/cpuinfo is never localized.
 */

#include "sys/auxv.h"
#include "ci.h"

static int
ci_have_vsx(ci_structp ci_ptr)
{
   unsigned long auxv = getauxval(AT_HWCAP);

   CI_UNUSED(ci_ptr)

   if(auxv & (PPC_FEATURE_HAS_ALTIVEC|PPC_FEATURE_HAS_VSX))
      return 1;
   else
      return 0;
}

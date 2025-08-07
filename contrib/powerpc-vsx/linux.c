/* contrib/powerpc-vsx/linux.c
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
 * ci_have_vsx implemented for Linux by reading the widely available
 * pseudo-file /proc/cpuinfo.
 *
 * This code is strict ANSI-C and is probably moderately portable; it does
 * however use <stdio.h> and it assumes that /proc/cpuinfo is never localized.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "ci.h"

#ifndef MAXLINE
#  define MAXLINE 1024
#endif

static int
ci_have_vsx(ci_structp ci_ptr)
{
   FILE *f;

   const char *string = "altivec supported";
   char input[MAXLINE];
   char *token = NULL;

   CI_UNUSED(ci_ptr)

   f = fopen("/proc/cpuinfo", "r");
   if (f != NULL)
   {
      memset(input,0,MAXLINE);
      while(fgets(input,MAXLINE,f) != NULL)
      {
         token = strstr(input,string);
         if(token != NULL)
            return 1;
      }
   }
#ifdef CI_WARNINGS_SUPPORTED
   else
      ci_warning(ci_ptr, "/proc/cpuinfo open failed");
#endif
   return 0;
}

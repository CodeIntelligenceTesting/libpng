/* contrib/mips-msa/linux.c
 *
 * Copyright (c) 2020-2023 Cosmin Truta
 * Copyright (c) 2016 Glenn Randers-Pehrson
 * Written by Mandar Sahastrabuddhe, 2016.
 * Updated by Sui Jingfeng, 2021.
 *
 * This code is released under the libci license.
 * For conditions of distribution and use, see the disclaimer
 * and license in ci.h
 *
 * On Linux, ci_have_msa is implemented by reading the pseudo-file
 * "/proc/self/auxv".
 *
 * See contrib/mips-msa/README before reporting bugs.
 *
 * STATUS: SUPPORTED
 * BUG REPORTS: ci-mng-implement@sourceforge.net
 */

#include <elf.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>

static int
ci_have_msa(ci_structp ci_ptr)
{
   Elf64_auxv_t aux;
   int fd;
   int has_msa = 0;

   fd = open("/proc/self/auxv", O_RDONLY);
   if (fd >= 0)
   {
      while (read(fd, &aux, sizeof(Elf64_auxv_t)) == sizeof(Elf64_auxv_t))
      {
         if (aux.a_type == AT_HWCAP)
         {
            uint64_t hwcap = aux.a_un.a_val;

            has_msa = (hwcap >> 1) & 1;
            break;
         }
      }
      close(fd);
   }
#ifdef CI_WARNINGS_SUPPORTED
   else
      ci_warning(ci_ptr, "/proc/self/auxv open failed");
#endif

   return has_msa;
}

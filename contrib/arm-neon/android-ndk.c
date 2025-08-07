/* contrib/arm-neon/android-ndk.c
 *
 * Copyright (c) 2014 Glenn Randers-Pehrson
 * Written by John Bowler, 2014.
 *
 * This code is released under the libci license.
 * For conditions of distribution and use, see the disclaimer
 * and license in ci.h
 *
 * SEE contrib/arm-neon/README before reporting bugs
 *
 * STATUS: COMPILED, UNTESTED
 * BUG REPORTS: ci-mng-implement@sourceforge.net
 *
 * ci_have_neon implemented for the Android NDK, see:
 *
 * Documentation:
 *    http://www.kandroid.org/ndk/docs/CPU-ARM-NEON.html
 *    https://code.google.com/p/android/issues/detail?id=49065
 *
 * NOTE: this requires that libci is built against the Android NDK and linked
 * with an implementation of the Android ARM 'cpu-features' library.  The code
 * has been compiled only, not linked: no version of the library has been found,
 * only the header files exist in the NDK.
 */

#include <cpu-features.h>

static int
ci_have_neon(ci_structp ci_ptr)
{
   /* This is a whole lot easier than the linux code, however it is probably
    * implemented as below, therefore it is better to cache the result (these
    * function calls may be slow!)
    */
   CI_UNUSED(ci_ptr)
   return android_getCpuFamily() == ANDROID_CPU_FAMILY_ARM &&
      (android_getCpuFeatures() & ANDROID_CPU_ARM_FEATURE_NEON) != 0;
}

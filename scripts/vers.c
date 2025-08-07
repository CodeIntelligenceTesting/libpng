/* vers.c - define format of libci.vers
 *
 * Copyright (c) 2011-2014 Glenn Randers-Pehrson
 *
 * This code is released under the libci license.
 * For conditions of distribution and use, see the disclaimer
 * and license in ci.h
 */

#define CI_EXPORTA(ordinal, type, name, args, attributes)\
        CI_DFN " @" SYMBOL_PREFIX "@@" name "@;"

CI_DFN "@" CILIB_LIBNAME "@ {global:"

#include "../ci.h"

CI_DFN "local: *; };"

/* prefix.c - generate an unprefixed symbol list
 *
 * Copyright (c) 2013-2014 Glenn Randers-Pehrson
 *
 * This code is released under the libci license.
 * For conditions of distribution and use, see the disclaimer
 * and license in ci.h
 */

#define CI_EXPORTA(ordinal, type, name, args, attributes)\
        CI_DFN "@" name "@"

/* The configuration information *before* the additional of symbol renames,
 * the list is the C name list; no symbol prefix.
 */
#include "cilibconf.out"

CI_DFN_START_SORT 1

#include "../ci.h"

CI_DFN_END_SORT

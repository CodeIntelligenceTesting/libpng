/* sym.c - define format of libci.sym
 *
 * Copyright (c) 2011-2014 Glenn Randers-Pehrson
 *
 * This code is released under the libci license.
 * For conditions of distribution and use, see the disclaimer
 * and license in ci.h
 */

#define CI_EXPORTA(ordinal, type, name, args, attributes)\
        CI_DFN "@" SYMBOL_PREFIX "@@" name "@"

#include "../ci.h"

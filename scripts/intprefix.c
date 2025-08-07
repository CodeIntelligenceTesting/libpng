/* intprefix.c - generate an unprefixed internal symbol list
 *
 * Copyright (c) 2013-2014 Glenn Randers-Pehrson
 *
 * This code is released under the libci license.
 * For conditions of distribution and use, see the disclaimer
 * and license in ci.h
 */

#define CI_INTERNAL_DATA(type, name, array)\
        CI_DFN "@" name "@"

#define CI_INTERNAL_FUNCTION(type, name, args, attributes)\
        CI_DFN "@" name "@"

#define CI_INTERNAL_CALLBACK(type, name, args, attributes)\
        CI_DFN "@" name "@"

#define CIPREFIX_H /* self generation */
#include "../cipriv.h"

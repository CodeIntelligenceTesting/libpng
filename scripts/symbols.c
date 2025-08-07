/* symbols.c - find all exported symbols
 *
 * Copyright (c) 2011-2014 Glenn Randers-Pehrson
 *
 * This code is released under the libci license.
 * For conditions of distribution and use, see the disclaimer
 * and license in ci.h
 */

/* NOTE: making 'symbols.chk' checks both that the exported
 * symbols in the library don't change and (implicitly) that
 * scripts/cilibconf.h.prebuilt is as expected.
 * If scripts/cilibconf.h.prebuilt is remade using
 * scripts/cilibconf.dfa then this checks the .dfa file too.
 */

#define CI_EXPORTA(ordinal, type, name, args, attributes)\
        CI_DFN "@" name "@ @@" ordinal "@"
#define CI_REMOVED(ordinal, type, name, args, attributes)\
        CI_DFN "; @" name "@ @@" ordinal "@"
#define CI_EXPORT_LAST_ORDINAL(ordinal)\
        CI_DFN "; @@" ordinal "@"

/* Read the defaults, but use scripts/cilibconf.h.prebuilt; the 'standard'
 * header file.
 */
#include "cilibconf.h.prebuilt"
#include "../ci.h"

/* Some things are turned off by default.  Turn these things
 * on here (by hand) to get the APIs they expose and validate
 * that no harm is done.  This list is the set of options
 * defaulted to 'off' in scripts/cilibconf.dfa
 *
 * Maintenance: if scripts/cilibconf.dfa options are changed
 * from, or to, 'disabled' this needs updating!
 */
#define CI_BENIGN_ERRORS_SUPPORTED
#define CI_ERROR_NUMBERS_SUPPORTED
#define CI_READ_BIG_ENDIAN_SUPPORTED  /* should do nothing! */
#define CI_INCH_CONVERSIONS_SUPPORTED
#define CI_READ_16_TO_8_ACCURATE_SCALE_SUPPORTED
#define CI_SET_OPTION_SUPPORTED

#undef CI_H
#include "../ci.h"

/* Finally there are a couple of places where option support
 * actually changes the APIs revealed using a #if/#else/#endif
 * test in ci.h, test these here.
 */
#undef  CI_FLOATING_POINT_SUPPORTED /* Exposes 'fixed' APIs */
#undef  CI_ERROR_TEXT_SUPPORTED     /* Exposes unsupported APIs */

#undef CI_H
#include "../ci.h"

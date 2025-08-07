/* minrdciconf.h: headers to make a minimal ci-read-only library
 *
 * Copyright (c) 2009, 2010-2013 Glenn Randers-Pehrson
 *
 * This code is released under the libci license.
 * For conditions of distribution and use, see the disclaimer
 * and license in ci.h
 *
 * Derived from cicrush.h, Copyright 1998-2007, Glenn Randers-Pehrson
 */

#ifndef MINPRDCICONF_H
#define MINPRDCICONF_H

/* To include ciusr.h set -DCI_USER_CONFIG in CPPFLAGS */

/* List options to turn off features of the build that do not
 * affect the API (so are not recorded in cilibconf.h)
 */

#define CI_ALIGN_TYPE CI_ALIGN_NONE

#endif /* MINPRDCICONF_H */

/* cidebug.h - internal debugging macros for libci
 *
 * Copyright (c) 2018-2025 Cosmin Truta
 * Copyright (c) 1998-2002,2004,2006-2013 Glenn Randers-Pehrson
 * Copyright (c) 1996-1997 Andreas Dilger
 * Copyright (c) 1995-1996 Guy Eric Schalnat, Group 42, Inc.
 *
 * This code is released under the libci license.
 * For conditions of distribution and use, see the disclaimer
 * and license in ci.h
 */

#ifndef CIPRIV_H
#  error This file must not be included by applications; please include <ci.h>
#endif

/* Define CI_DEBUG at compile time for debugging information.  Higher
 * numbers for CI_DEBUG mean more debugging information.  This has
 * only been added since version 0.95 so it is not implemented throughout
 * libci yet, but more support will be added as needed.
 *
 * ci_debug[1-2]?(level, message ,arg{0-2})
 *   Expands to a statement (either a simple expression or a compound
 *   do..while(0) statement) that outputs a message with parameter
 *   substitution if CI_DEBUG is defined to 2 or more.  If CI_DEBUG
 *   is undefined, 0 or 1 every ci_debug expands to a simple expression
 *   (actually ((void)0)).
 *
 *   level: level of detail of message, starting at 0.  A level 'n'
 *          message is preceded by 'n' 3-space indentations (not implemented
 *          on Microsoft compilers unless CI_DEBUG_FILE is also
 *          defined, to allow debug DLL compilation with no standard IO).
 *   message: a printf(3) style text string.  A trailing '\n' is added
 *            to the message.
 *   arg: 0 to 2 arguments for printf(3) style substitution in message.
 */
#ifndef CIDEBUG_H
#define CIDEBUG_H
/* These settings control the formatting of messages in ci.c and cierror.c */
/* Moved to cidebug.h at 1.5.0 */
#  ifndef CI_LITERAL_SHARP
#    define CI_LITERAL_SHARP 0x23
#  endif
#  ifndef CI_LITERAL_LEFT_SQUARE_BRACKET
#    define CI_LITERAL_LEFT_SQUARE_BRACKET 0x5b
#  endif
#  ifndef CI_LITERAL_RIGHT_SQUARE_BRACKET
#    define CI_LITERAL_RIGHT_SQUARE_BRACKET 0x5d
#  endif
#  ifndef CI_STRING_NEWLINE
#    define CI_STRING_NEWLINE "\n"
#  endif

#ifdef CI_DEBUG
#  if (CI_DEBUG > 0)
#    if !defined(CI_DEBUG_FILE) && defined(_MSC_VER)
#      include <crtdbg.h>
#      if (CI_DEBUG > 1)
#        ifndef _DEBUG
#          define _DEBUG
#        endif
#        ifndef ci_debug
#          define ci_debug(l,m)  _RPT0(_CRT_WARN,m CI_STRING_NEWLINE)
#        endif
#        ifndef ci_debug1
#          define ci_debug1(l,m,p1)  _RPT1(_CRT_WARN,m CI_STRING_NEWLINE,p1)
#        endif
#        ifndef ci_debug2
#          define ci_debug2(l,m,p1,p2) \
             _RPT2(_CRT_WARN,m CI_STRING_NEWLINE,p1,p2)
#        endif
#      endif
#    else /* CI_DEBUG_FILE || !_MSC_VER */
#      ifndef CI_STDIO_SUPPORTED
#        include <stdio.h> /* not included yet */
#      endif
#      ifndef CI_DEBUG_FILE
#        define CI_DEBUG_FILE stderr
#      endif /* CI_DEBUG_FILE */

#      if (CI_DEBUG > 1)
#        ifdef __STDC__
#          ifndef ci_debug
#            define ci_debug(l,m) \
       do { \
       int num_tabs=l; \
       fprintf(CI_DEBUG_FILE,"%s" m CI_STRING_NEWLINE,(num_tabs==1 ? "   " : \
         (num_tabs==2 ? "      " : (num_tabs>2 ? "         " : "")))); \
       } while (0)
#          endif
#          ifndef ci_debug1
#            define ci_debug1(l,m,p1) \
       do { \
       int num_tabs=l; \
       fprintf(CI_DEBUG_FILE,"%s" m CI_STRING_NEWLINE,(num_tabs==1 ? "   " : \
         (num_tabs==2 ? "      " : (num_tabs>2 ? "         " : ""))),p1); \
       } while (0)
#          endif
#          ifndef ci_debug2
#            define ci_debug2(l,m,p1,p2) \
       do { \
       int num_tabs=l; \
       fprintf(CI_DEBUG_FILE,"%s" m CI_STRING_NEWLINE,(num_tabs==1 ? "   " : \
         (num_tabs==2 ? "      " : (num_tabs>2 ? "         " : ""))),p1,p2);\
       } while (0)
#          endif
#        else /* __STDC __ */
#          ifndef ci_debug
#            define ci_debug(l,m) \
       do { \
       int num_tabs=l; \
       char format[256]; \
       snprintf(format,256,"%s%s%s",(num_tabs==1 ? "\t" : \
         (num_tabs==2 ? "\t\t":(num_tabs>2 ? "\t\t\t":""))), \
         m,CI_STRING_NEWLINE); \
       fprintf(CI_DEBUG_FILE,format); \
       } while (0)
#          endif
#          ifndef ci_debug1
#            define ci_debug1(l,m,p1) \
       do { \
       int num_tabs=l; \
       char format[256]; \
       snprintf(format,256,"%s%s%s",(num_tabs==1 ? "\t" : \
         (num_tabs==2 ? "\t\t":(num_tabs>2 ? "\t\t\t":""))), \
         m,CI_STRING_NEWLINE); \
       fprintf(CI_DEBUG_FILE,format,p1); \
       } while (0)
#          endif
#          ifndef ci_debug2
#            define ci_debug2(l,m,p1,p2) \
       do { \
       int num_tabs=l; \
       char format[256]; \
       snprintf(format,256,"%s%s%s",(num_tabs==1 ? "\t" : \
         (num_tabs==2 ? "\t\t":(num_tabs>2 ? "\t\t\t":""))), \
         m,CI_STRING_NEWLINE); \
       fprintf(CI_DEBUG_FILE,format,p1,p2); \
       } while (0)
#          endif
#        endif /* __STDC __ */
#      endif /* (CI_DEBUG > 1) */

#    endif /* _MSC_VER */
#  endif /* (CI_DEBUG > 0) */
#endif /* CI_DEBUG */
#ifndef ci_debug
#  define ci_debug(l, m) ((void)0)
#endif
#ifndef ci_debug1
#  define ci_debug1(l, m, p1) ((void)0)
#endif
#ifndef ci_debug2
#  define ci_debug2(l, m, p1, p2) ((void)0)
#endif
#endif /* CIDEBUG_H */

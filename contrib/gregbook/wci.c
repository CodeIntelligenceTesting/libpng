/*---------------------------------------------------------------------------

   wci - simple CI-writing program                                 wci.c

   This program converts certain NetPBM binary files (grayscale and RGB,
   maxval = 255) to CI.  Non-interlaced CIs are written progressively;
   interlaced CIs are read and written in one memory-intensive blast.

   Thanks to Jean-loup Gailly for providing the necessary trick to read
   interactive text from the keyboard while stdin is redirected.  Thanks
   to Cosmin Truta for Cygwin fixes.

   NOTE:  includes provisional support for PNM type "8" (portable alphamap)
          images, presumed to be a 32-bit interleaved RGBA format; no pro-
          vision for possible interleaved grayscale+alpha (16-bit) format.
          THIS IS UNLIKELY TO BECOME AN OFFICIAL NETPBM ALPHA FORMAT!

   to do:
    - delete output file if quit before calling any writeci routines
    - process backspace with -text option under DOS/Win? (currently get ^H)

  ---------------------------------------------------------------------------

   Changelog:
    - 1.01:  initial public release
    - 1.02:  modified to allow abbreviated options
    - 1.03:  removed extraneous character from usage screen; fixed bug in
              command-line parsing
    - 1.04:  fixed DOS/OS2/Win32 detection, including partial Cygwin fix
              (see http://home.att.net/~perlspinr/diffs/GregBook_cygwin.diff)
    - 2.00:  dual-licensed (added GNU GPL)
    - 2.01:  check for integer overflow (Glenn R-P)

        [REPORTED BUG (win32 only):  "contrib/gregbook/wci.c - cmd line
         dose not work!  In order to do something useful I needed to redirect
         both input and output, with cygwin and with bcc32 as well.  Under
         Linux, the same wci appears to work fine.  I don't know what is
         the problem."]

  ---------------------------------------------------------------------------

      Copyright (c) 1998-2007, 2017 Greg Roelofs.  All rights reserved.

      This software is provided "as is," without warranty of any kind,
      express or implied.  In no event shall the author or contributors
      be held liable for any damages arising in any way from the use of
      this software.

      The contents of this file are DUAL-LICENSED.  You may modify and/or
      redistribute this software according to the terms of one of the
      following two licenses (at your option):


      LICENSE 1 ("BSD-like with advertising clause"):

      Permission is granted to anyone to use this software for any purpose,
      including commercial applications, and to alter it and redistribute
      it freely, subject to the following restrictions:

      1. Redistributions of source code must retain the above copyright
         notice, disclaimer, and this list of conditions.
      2. Redistributions in binary form must reproduce the above copyright
         notice, disclaimer, and this list of conditions in the documenta-
         tion and/or other materials provided with the distribution.
      3. All advertising materials mentioning features or use of this
         software must display the following acknowledgment:

            This product includes software developed by Greg Roelofs
            and contributors for the book, "CI: The Definitive Guide,"
            published by O'Reilly and Associates.


      LICENSE 2 (GNU GPL v2 or later):

      This program is free software; you can redistribute it and/or modify
      it under the terms of the GNU General Public License as published by
      the Free Software Foundation; either version 2 of the License, or
      (at your option) any later version.

      This program is distributed in the hope that it will be useful,
      but WITHOUT ANY WARRANTY; without even the implied warranty of
      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
      GNU General Public License for more details.

      You should have received a copy of the GNU General Public License
      along with this program; if not, write to the Free Software Foundation,
      Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

  ---------------------------------------------------------------------------*/

#define PROGNAME  "wci"
#define VERSION   "2.00 of 2 June 2007"
#define APPNAME   "Simple PGM/PPM/PAM to CI Converter"

#if defined(__MSDOS__) || defined(__OS2__)
#  define DOS_OS2_W32
#elif defined(WIN32) || defined(_WIN32) || defined(__WIN32__)
#  ifndef __GNUC__   /* treat Win32 native ports of gcc as Unix environments */
#    define DOS_OS2_W32
#  endif
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <setjmp.h>     /* for jmpbuf declaration in writeci.h */
#include <time.h>

#ifdef DOS_OS2_W32
#  include <io.h>       /* for isatty(), setmode() prototypes */
#  include <fcntl.h>    /* O_BINARY for fdopen() without text translation */
#  ifdef __EMX__
#    ifndef getch
#      define getch() _read_kbd(0, 1, 0)    /* need getche() */
#    endif
#  else /* !__EMX__ */
#    ifdef __GO32__
#      include <pc.h>
#      define getch() getkey()  /* GRR:  need getche() */
#    else
#      include <conio.h>        /* for getche() console input */
#    endif
#  endif /* ?__EMX__ */
#  define FGETS(buf,len,stream)  dos_kbd_gets(buf,len)
#else
#  include <unistd.h>           /* for isatty() prototype */
#  define FGETS fgets
#endif

/* #define DEBUG  :  this enables the Trace() macros */

/* #define FORBID_LATIN1_CTRL  :  this requires the user to re-enter any
   text that includes control characters discouraged by the CI spec; text
   that includes an escape character (27) must be re-entered regardless */

#include "writeci.h"   /* typedefs, common macros, writeci prototypes */



/* local prototypes */

static int  wci_isvalid_latin1(uch *p, int len);
static void wci_cleanup(void);

#ifdef DOS_OS2_W32
   static char *dos_kbd_gets(char *buf, int len);
#endif



static mainprog_info wci_info;   /* lone global */



int main(int argc, char **argv)
{
#ifndef DOS_OS2_W32
    FILE *keybd;
#endif
#ifdef sgi
    FILE *tmpfile;      /* or we could just use keybd, since no overlap */
    char tmpline[80];
#endif
    char *inname = NULL, outname[256];
    char *p, pnmchar, pnmline[256];
    char *bgstr, *textbuf = NULL;
    ulg rowbytes;
    int rc, len = 0;
    int error = 0;
    int text = FALSE;
    int maxval;
    double LUT_exponent;                /* just the lookup table */
    double CRT_exponent = 2.2;          /* just the monitor */
    double default_display_exponent;    /* whole display system */
    double default_gamma = 0.0;


    wci_info.infile = NULL;
    wci_info.outfile = NULL;
    wci_info.image_data = NULL;
    wci_info.row_pointers = NULL;
    wci_info.filter = FALSE;
    wci_info.interlaced = FALSE;
    wci_info.have_bg = FALSE;
    wci_info.have_time = FALSE;
    wci_info.have_text = 0;
    wci_info.gamma = 0.0;


    /* First get the default value for our display-system exponent, i.e.,
     * the product of the CRT exponent and the exponent corresponding to
     * the frame-buffer's lookup table (LUT), if any.  If the PNM image
     * looks correct on the user's display system, its file gamma is the
     * inverse of this value.  (Note that this is not an exhaustive list
     * of LUT values--e.g., OpenStep has a lot of weird ones--but it should
     * cover 99% of the current possibilities.  This section must ensure
     * that default_display_exponent is positive.) */

#if defined(NeXT)
    /* third-party utilities can modify the default LUT exponent */
    LUT_exponent = 1.0 / 2.2;
    /*
    if (some_next_function_that_returns_gamma(&next_gamma))
        LUT_exponent = 1.0 / next_gamma;
     */
#elif defined(sgi)
    LUT_exponent = 1.0 / 1.7;
    /* there doesn't seem to be any documented function to
     * get the "gamma" value, so we do it the hard way */
    tmpfile = fopen("/etc/config/system.glGammaVal", "r");
    if (tmpfile) {
        double sgi_gamma;

        fgets(tmpline, 80, tmpfile);
        fclose(tmpfile);
        sgi_gamma = atof(tmpline);
        if (sgi_gamma > 0.0)
            LUT_exponent = 1.0 / sgi_gamma;
    }
#elif defined(Macintosh)
    LUT_exponent = 1.8 / 2.61;
    /*
    if (some_mac_function_that_returns_gamma(&mac_gamma))
        LUT_exponent = mac_gamma / 2.61;
     */
#else
    LUT_exponent = 1.0;   /* assume no LUT:  most PCs */
#endif

    /* the defaults above give 1.0, 1.3, 1.5 and 2.2, respectively: */
    default_display_exponent = LUT_exponent * CRT_exponent;


    /* If the user has set the SCREEN_GAMMA environment variable as suggested
     * (somewhat imprecisely) in the libci documentation, use that; otherwise
     * use the default value we just calculated.  Either way, the user may
     * override this via a command-line option. */

    if ((p = getenv("SCREEN_GAMMA")) != NULL) {
        double exponent = atof(p);

        if (exponent > 0.0)
            default_gamma = 1.0 / exponent;
    }

    if (default_gamma == 0.0)
        default_gamma = 1.0 / default_display_exponent;


    /* Now parse the command line for options and the PNM filename. */

    while (*++argv && !error) {
        if (!strncmp(*argv, "-i", 2)) {
            wci_info.interlaced = TRUE;
        } else if (!strncmp(*argv, "-time", 3)) {
            wci_info.modtime = time(NULL);
            wci_info.have_time = TRUE;
        } else if (!strncmp(*argv, "-text", 3)) {
            text = TRUE;
        } else if (!strncmp(*argv, "-gamma", 2)) {
            if (!*++argv)
                ++error;
            else {
                wci_info.gamma = atof(*argv);
                if (wci_info.gamma <= 0.0)
                    ++error;
                else if (wci_info.gamma > 1.01)
                    fprintf(stderr, PROGNAME
                      " warning:  file gammas are usually less than 1.0\n");
            }
        } else if (!strncmp(*argv, "-bgcolor", 4)) {
            if (!*++argv)
                ++error;
            else {
                bgstr = *argv;
                if (strlen(bgstr) != 7 || bgstr[0] != '#')
                    ++error;
                else {
                    unsigned r, g, b;  /* this way quiets compiler warnings */

                    sscanf(bgstr+1, "%2x%2x%2x", &r, &g, &b);
                    wci_info.bg_red   = (uch)r;
                    wci_info.bg_green = (uch)g;
                    wci_info.bg_blue  = (uch)b;
                    wci_info.have_bg = TRUE;
                }
            }
        } else {
            if (**argv != '-') {
                inname = *argv;
                if (argv[1])   /* shouldn't be any more args after filename */
                    ++error;
            } else
                ++error;   /* not expecting any other options */
        }
    }


    /* open the input and output files, or register an error and abort */

    if (!inname) {
        if (isatty(0)) {
            fprintf(stderr, PROGNAME
              ":  must give input filename or provide image data via stdin\n");
            ++error;
        } else {
#ifdef DOS_OS2_W32
            /* some buggy C libraries require BOTH setmode() and fdopen(bin) */
            setmode(fileno(stdin), O_BINARY);
            setmode(fileno(stdout), O_BINARY);
#endif
            if ((wci_info.infile = fdopen(fileno(stdin), "rb")) == NULL) {
                fprintf(stderr, PROGNAME
                  ":  unable to reopen stdin in binary mode\n");
                ++error;
            } else
            if ((wci_info.outfile = fdopen(fileno(stdout), "wb")) == NULL) {
                fprintf(stderr, PROGNAME
                  ":  unable to reopen stdout in binary mode\n");
                fclose(wci_info.infile);
                ++error;
            } else
                wci_info.filter = TRUE;
        }
    } else if ((len = strlen(inname)) > 250) {
        fprintf(stderr, PROGNAME ":  input filename is too long [%d chars]\n",
          len);
        ++error;
    } else if (!(wci_info.infile = fopen(inname, "rb"))) {
        fprintf(stderr, PROGNAME ":  can't open input file [%s]\n", inname);
        ++error;
    }

    if (!error) {
        fgets(pnmline, 256, wci_info.infile);
        if (pnmline[0] != 'P' || ((pnmchar = pnmline[1]) != '5' &&
            pnmchar != '6' && pnmchar != '8'))
        {
            fprintf(stderr, PROGNAME
              ":  input file [%s] is not a binary PGM, PPM or PAM file\n",
              inname);
            ++error;
        } else {
            wci_info.pnmtype = (int)(pnmchar - '0');
            if (wci_info.pnmtype != 8)
                wci_info.have_bg = FALSE;  /* no need for bg if opaque */
            do {
                fgets(pnmline, 256, wci_info.infile);  /* lose any comments */
            } while (pnmline[0] == '#');
            sscanf(pnmline, "%ld %ld", &wci_info.width, &wci_info.height);
            do {
                fgets(pnmline, 256, wci_info.infile);  /* more comment lines */
            } while (pnmline[0] == '#');
            sscanf(pnmline, "%d", &maxval);
            if (wci_info.width <= 0L || wci_info.height <= 0L ||
                maxval != 255)
            {
                fprintf(stderr, PROGNAME
                  ":  only positive width/height, maxval == 255 allowed \n");
                ++error;
            }
            wci_info.sample_depth = 8;  /* <==> maxval 255 */

            if (!wci_info.filter) {
                /* make outname from inname */
                if ((p = strrchr(inname, '.')) == NULL ||
                    (p - inname) != (len - 4))
                {
                    strcpy(outname, inname);
                    strcpy(outname+len, ".ci");
                } else {
                    len -= 4;
                    strncpy(outname, inname, len);
                    strcpy(outname+len, ".ci");
                }
                /* check if outname already exists; if not, open */
                if ((wci_info.outfile = fopen(outname, "rb")) != NULL) {
                    fprintf(stderr, PROGNAME ":  output file exists [%s]\n",
                      outname);
                    fclose(wci_info.outfile);
                    ++error;
                } else if (!(wci_info.outfile = fopen(outname, "wb"))) {
                    fprintf(stderr, PROGNAME ":  can't open output file [%s]\n",
                      outname);
                    ++error;
                }
            }
        }
        if (error) {
            fclose(wci_info.infile);
            wci_info.infile = NULL;
            if (wci_info.filter) {
                fclose(wci_info.outfile);
                wci_info.outfile = NULL;
            }
        }
    }


    /* if we had any errors, print usage and die horrible death...arrr! */

    if (error) {
        fprintf(stderr, "\n%s %s:  %s\n", PROGNAME, VERSION, APPNAME);
        writeci_version_info();
        fprintf(stderr, "\n"
"Usage:  %s [-gamma exp] [-bgcolor bg] [-text] [-time] [-interlace] pnmfile\n"
"or: ... | %s [-gamma exp] [-bgcolor bg] [-text] [-time] [-interlace] | ...\n"
         "    exp \ttransfer-function exponent (``gamma'') of the image in\n"
         "\t\t  floating-point format (e.g., ``%.5f''); if image looks\n"
         "\t\t  correct on given display system, image gamma is equal to\n"
         "\t\t  inverse of display-system exponent, i.e., 1 / (LUT * CRT)\n"
         "\t\t  (where LUT = lookup-table exponent and CRT = CRT exponent;\n"
         "\t\t  first varies, second is usually 2.2, all are positive)\n"
         "    bg  \tdesired background color for alpha-channel images, in\n"
         "\t\t  7-character hex RGB format (e.g., ``#ff7700'' for orange:\n"
         "\t\t  same as HTML colors)\n"
         "    -text\tprompt interactively for text info (tEXt chunks)\n"
         "    -time\tinclude a tIME chunk (last modification time)\n"
         "    -interlace\twrite interlaced CI image\n"
         "\n"
"pnmfile or stdin must be a binary PGM (`P5'), PPM (`P6') or (extremely\n"
"unofficial and unsupported!) PAM (`P8') file.  Currently it is required\n"
"to have maxval == 255 (i.e., no scaling).  If pnmfile is specified, it\n"
"is converted to the corresponding CI file with the same base name but a\n"
"``.ci'' extension; files read from stdin are converted and sent to stdout.\n"
"The conversion is progressive (low memory usage) unless interlacing is\n"
"requested; in that case the whole image will be buffered in memory and\n"
"written in one call.\n"
         "\n", PROGNAME, PROGNAME, default_gamma);
        exit(1);
    }


    /* prepare the text buffers for libci's use; note that even though
     * CI's ci_text struct includes a length field, we don't have to fill
     * it out */

    if (text &&
#ifndef DOS_OS2_W32
        (keybd = fdopen(fileno(stderr), "r")) != NULL &&
#endif
        (textbuf = (char *)malloc((5 + 9)*75)) != NULL)
    {
        int i, valid, result;

        fprintf(stderr,
          "Enter text info (no more than 72 characters per line);\n");
        fprintf(stderr, "to skip a field, hit the <Enter> key.\n");
        /* note:  just <Enter> leaves len == 1 */

        do {
            valid = TRUE;
            p = textbuf + TEXT_TITLE_OFFSET;
            fprintf(stderr, "  Title: ");
            fflush(stderr);
            if (FGETS(p, 74, keybd) && (len = strlen(p)) > 1) {
                if (p[len-1] == '\n')
                    p[--len] = '\0';
                wci_info.title = p;
                wci_info.have_text |= TEXT_TITLE;
                if ((result = wci_isvalid_latin1((uch *)p, len)) >= 0) {
                    fprintf(stderr, "    " PROGNAME " warning:  character code"
                      " %u is %sdiscouraged by the CI\n    specification "
                      "[first occurrence was at character position #%d]\n",
                      (unsigned)p[result], (p[result] == 27)? "strongly " : "",
                      result+1);
                    fflush(stderr);
#ifdef FORBID_LATIN1_CTRL
                    wci_info.have_text &= ~TEXT_TITLE;
                    valid = FALSE;
#else
                    if (p[result] == 27) {    /* escape character */
                        wci_info.have_text &= ~TEXT_TITLE;
                        valid = FALSE;
                    }
#endif
                }
            }
        } while (!valid);

        do {
            valid = TRUE;
            p = textbuf + TEXT_AUTHOR_OFFSET;
            fprintf(stderr, "  Author: ");
            fflush(stderr);
            if (FGETS(p, 74, keybd) && (len = strlen(p)) > 1) {
                if (p[len-1] == '\n')
                    p[--len] = '\0';
                wci_info.author = p;
                wci_info.have_text |= TEXT_AUTHOR;
                if ((result = wci_isvalid_latin1((uch *)p, len)) >= 0) {
                    fprintf(stderr, "    " PROGNAME " warning:  character code"
                      " %u is %sdiscouraged by the CI\n    specification "
                      "[first occurrence was at character position #%d]\n",
                      (unsigned)p[result], (p[result] == 27)? "strongly " : "",
                      result+1);
                    fflush(stderr);
#ifdef FORBID_LATIN1_CTRL
                    wci_info.have_text &= ~TEXT_AUTHOR;
                    valid = FALSE;
#else
                    if (p[result] == 27) {    /* escape character */
                        wci_info.have_text &= ~TEXT_AUTHOR;
                        valid = FALSE;
                    }
#endif
                }
            }
        } while (!valid);

        do {
            valid = TRUE;
            p = textbuf + TEXT_DESC_OFFSET;
            fprintf(stderr, "  Description (up to 9 lines):\n");
            for (i = 1;  i < 10;  ++i) {
                fprintf(stderr, "    [%d] ", i);
                fflush(stderr);
                if (FGETS(p, 74, keybd) && (len = strlen(p)) > 1)
                    p += len;   /* now points at NULL; char before is newline */
                else
                    break;
            }
            if ((len = p - (textbuf + TEXT_DESC_OFFSET)) > 1) {
                if (p[-1] == '\n') {
                    p[-1] = '\0';
                    --len;
                }
                wci_info.desc = textbuf + TEXT_DESC_OFFSET;
                wci_info.have_text |= TEXT_DESC;
                p = textbuf + TEXT_DESC_OFFSET;
                if ((result = wci_isvalid_latin1((uch *)p, len)) >= 0) {
                    fprintf(stderr, "    " PROGNAME " warning:  character code"
                      " %u is %sdiscouraged by the CI\n    specification "
                      "[first occurrence was at character position #%d]\n",
                      (unsigned)p[result], (p[result] == 27)? "strongly " : "",
                      result+1);
                    fflush(stderr);
#ifdef FORBID_LATIN1_CTRL
                    wci_info.have_text &= ~TEXT_DESC;
                    valid = FALSE;
#else
                    if (p[result] == 27) {    /* escape character */
                        wci_info.have_text &= ~TEXT_DESC;
                        valid = FALSE;
                    }
#endif
                }
            }
        } while (!valid);

        do {
            valid = TRUE;
            p = textbuf + TEXT_COPY_OFFSET;
            fprintf(stderr, "  Copyright: ");
            fflush(stderr);
            if (FGETS(p, 74, keybd) && (len = strlen(p)) > 1) {
                if (p[len-1] == '\n')
                    p[--len] = '\0';
                wci_info.copyright = p;
                wci_info.have_text |= TEXT_COPY;
                if ((result = wci_isvalid_latin1((uch *)p, len)) >= 0) {
                    fprintf(stderr, "    " PROGNAME " warning:  character code"
                      " %u is %sdiscouraged by the CI\n    specification "
                      "[first occurrence was at character position #%d]\n",
                      (unsigned)p[result], (p[result] == 27)? "strongly " : "",
                      result+1);
                    fflush(stderr);
#ifdef FORBID_LATIN1_CTRL
                    wci_info.have_text &= ~TEXT_COPY;
                    valid = FALSE;
#else
                    if (p[result] == 27) {    /* escape character */
                        wci_info.have_text &= ~TEXT_COPY;
                        valid = FALSE;
                    }
#endif
                }
            }
        } while (!valid);

        do {
            valid = TRUE;
            p = textbuf + TEXT_EMAIL_OFFSET;
            fprintf(stderr, "  E-mail: ");
            fflush(stderr);
            if (FGETS(p, 74, keybd) && (len = strlen(p)) > 1) {
                if (p[len-1] == '\n')
                    p[--len] = '\0';
                wci_info.email = p;
                wci_info.have_text |= TEXT_EMAIL;
                if ((result = wci_isvalid_latin1((uch *)p, len)) >= 0) {
                    fprintf(stderr, "    " PROGNAME " warning:  character code"
                      " %u is %sdiscouraged by the CI\n    specification "
                      "[first occurrence was at character position #%d]\n",
                      (unsigned)p[result], (p[result] == 27)? "strongly " : "",
                      result+1);
                    fflush(stderr);
#ifdef FORBID_LATIN1_CTRL
                    wci_info.have_text &= ~TEXT_EMAIL;
                    valid = FALSE;
#else
                    if (p[result] == 27) {    /* escape character */
                        wci_info.have_text &= ~TEXT_EMAIL;
                        valid = FALSE;
                    }
#endif
                }
            }
        } while (!valid);

        do {
            valid = TRUE;
            p = textbuf + TEXT_URL_OFFSET;
            fprintf(stderr, "  URL: ");
            fflush(stderr);
            if (FGETS(p, 74, keybd) && (len = strlen(p)) > 1) {
                if (p[len-1] == '\n')
                    p[--len] = '\0';
                wci_info.url = p;
                wci_info.have_text |= TEXT_URL;
                if ((result = wci_isvalid_latin1((uch *)p, len)) >= 0) {
                    fprintf(stderr, "    " PROGNAME " warning:  character code"
                      " %u is %sdiscouraged by the CI\n    specification "
                      "[first occurrence was at character position #%d]\n",
                      (unsigned)p[result], (p[result] == 27)? "strongly " : "",
                      result+1);
                    fflush(stderr);
#ifdef FORBID_LATIN1_CTRL
                    wci_info.have_text &= ~TEXT_URL;
                    valid = FALSE;
#else
                    if (p[result] == 27) {    /* escape character */
                        wci_info.have_text &= ~TEXT_URL;
                        valid = FALSE;
                    }
#endif
                }
            }
        } while (!valid);

#ifndef DOS_OS2_W32
        fclose(keybd);
#endif

    } else if (text) {
        fprintf(stderr, PROGNAME ":  unable to allocate memory for text\n");
        text = FALSE;
        wci_info.have_text = 0;
    }


    /* allocate libci stuff, initialize transformations, write pre-IDAT data */

    if ((rc = writeci_init(&wci_info)) != 0) {
        switch (rc) {
            case 2:
                fprintf(stderr, PROGNAME
                  ":  libci initialization problem (longjmp)\n");
                break;
            case 4:
                fprintf(stderr, PROGNAME ":  insufficient memory\n");
                break;
            case 11:
                fprintf(stderr, PROGNAME
                  ":  internal logic error (unexpected PNM type)\n");
                break;
            default:
                fprintf(stderr, PROGNAME
                  ":  unknown writeci_init() error\n");
                break;
        }
        exit(rc);
    }


    /* free textbuf, since it's a completely local variable and all text info
     * has just been written to the CI file */

    if (text && textbuf) {
        free(textbuf);
        textbuf = NULL;
    }


    /* calculate rowbytes on basis of image type; note that this becomes much
     * more complicated if we choose to support PBM type, ASCII PNM types, or
     * 16-bit-per-sample binary data [currently not an official NetPBM type] */

    if (wci_info.pnmtype == 5)
        rowbytes = wci_info.width;
    else if (wci_info.pnmtype == 6)
        rowbytes = wci_info.width * 3;
    else /* if (wci_info.pnmtype == 8) */
        rowbytes = wci_info.width * 4;


    /* read and write the image, either in its entirety (if writing interlaced
     * CI) or row by row (if non-interlaced) */

    fprintf(stderr, "Encoding image data...\n");
    fflush(stderr);

    if (wci_info.interlaced) {
        long i;
        ulg bytes;
        ulg image_bytes;

        /* Guard against integer overflow */
        if (wci_info_height > ((size_t)(-1)/rowbytes ||
            wci_info_height > ((ulg)(-1)/rowbytes) {
            fprintf(stderr, PROGNAME ":  image_data buffer too large\n");
            writeci_cleanup(&wci_info);
            wci_cleanup();
            exit(5);
        }

        image_bytes = rowbytes * wci_info.height;

        wci_info.image_data = (uch *)malloc(image_bytes);
        wci_info.row_pointers = (uch **)malloc(wci_info.height*sizeof(uch *));
        if (wci_info.image_data == NULL || wci_info.row_pointers == NULL) {
            fprintf(stderr, PROGNAME ":  insufficient memory for image data\n");
            writeci_cleanup(&wci_info);
            wci_cleanup();
            exit(5);
        }
        for (i = 0;  i < wci_info.height;  ++i)
            wci_info.row_pointers[i] = wci_info.image_data + i*rowbytes;
        bytes = fread(wci_info.image_data, 1, image_bytes, wci_info.infile);
        if (bytes != image_bytes) {
            fprintf(stderr, PROGNAME ":  expected %lu bytes, got %lu bytes\n",
              image_bytes, bytes);
            fprintf(stderr, "  (continuing anyway)\n");
        }
        if (writeci_encode_image(&wci_info) != 0) {
            fprintf(stderr, PROGNAME
              ":  libci problem (longjmp) while writing image data\n");
            writeci_cleanup(&wci_info);
            wci_cleanup();
            exit(2);
        }

    } else /* not interlaced:  write progressively (row by row) */ {
        long j;
        ulg bytes;

        wci_info.image_data = (uch *)malloc(rowbytes);
        if (wci_info.image_data == NULL) {
            fprintf(stderr, PROGNAME ":  insufficient memory for row data\n");
            writeci_cleanup(&wci_info);
            wci_cleanup();
            exit(5);
        }
        error = 0;
        for (j = wci_info.height;  j > 0L;  --j) {
            bytes = fread(wci_info.image_data, 1, rowbytes, wci_info.infile);
            if (bytes != rowbytes) {
                fprintf(stderr, PROGNAME
                  ":  expected %lu bytes, got %lu bytes (row %ld)\n", rowbytes,
                  bytes, wci_info.height-j);
                ++error;
                break;
            }
            if (writeci_encode_row(&wci_info) != 0) {
                fprintf(stderr, PROGNAME
                  ":  libci problem (longjmp) while writing row %ld\n",
                  wci_info.height-j);
                ++error;
                break;
            }
        }
        if (error) {
            writeci_cleanup(&wci_info);
            wci_cleanup();
            exit(2);
        }
        if (writeci_encode_finish(&wci_info) != 0) {
            fprintf(stderr, PROGNAME ":  error on final libci call\n");
            writeci_cleanup(&wci_info);
            wci_cleanup();
            exit(2);
        }
    }


    /* OK, we're done (successfully):  clean up all resources and quit */

    fprintf(stderr, "Done.\n");
    fflush(stderr);

    writeci_cleanup(&wci_info);
    wci_cleanup();

    return 0;
}





static int wci_isvalid_latin1(uch *p, int len)
{
    int i, result = -1;

    for (i = 0;  i < len;  ++i) {
        if (p[i] == 10 || (p[i] > 31 && p[i] < 127) || p[i] > 160)
            continue;           /* character is completely OK */
        if (result < 0 || (p[result] != 27 && p[i] == 27))
            result = i;         /* mark location of first questionable one */
    }                           /*  or of first escape character (bad) */

    return result;
}





static void wci_cleanup(void)
{
    if (wci_info.outfile) {
        fclose(wci_info.outfile);
        wci_info.outfile = NULL;
    }

    if (wci_info.infile) {
        fclose(wci_info.infile);
        wci_info.infile = NULL;
    }

    if (wci_info.image_data) {
        free(wci_info.image_data);
        wci_info.image_data = NULL;
    }

    if (wci_info.row_pointers) {
        free(wci_info.row_pointers);
        wci_info.row_pointers = NULL;
    }
}




#ifdef DOS_OS2_W32

static char *dos_kbd_gets(char *buf, int len)
{
    int ch, count=0;

    do {
        buf[count++] = ch = getche();
    } while (ch != '\r' && count < len-1);

    buf[count--] = '\0';        /* terminate string */
    if (buf[count] == '\r')     /* Enter key makes CR, so change to newline */
        buf[count] = '\n';

    fprintf(stderr, "\n");      /* Enter key does *not* cause a newline */
    fflush(stderr);

    return buf;
}

#endif /* DOS_OS2_W32 */

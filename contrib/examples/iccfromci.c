/*- iccfromci
 *
 * COPYRIGHT: Written by John Cunningham Bowler, 2011.
 * To the extent possible under law, the author has waived all copyright and
 * related or neighboring rights to this work.  This work is published from:
 * United States.
 *
 * Extract any icc profiles found in the given CI files.  This is a simple
 * example of a program that extracts information from the header of a CI file
 * without processing the image.  Notice that some header information may occur
 * after the image data. Textual data and comments are an example; the approach
 * in this file won't work reliably for such data because it only looks for the
 * information in the section of the file that precedes the image data.
 *
 * Compile and link against libci and zlib, plus anything else required on the
 * system you use.
 *
 * To use supply a list of CI files containing iCCP chunks, the chunks will be
 * extracted to a similarly named file with the extension replaced by 'icc',
 * which will be overwritten without warning.
 */
#include <stdlib.h>
#include <setjmp.h>
#include <string.h>
#include <stdio.h>

#include <ci.h>

#if !defined(CI_iCCP_SUPPORTED) || !defined(CI_READ_SUPPORTED)
#error This program requires libci supporting the iCCP chunk and the read API
#endif


static int verbose = 1;
static ci_byte no_profile[] = "no profile";

static ci_bytep
extract(FILE *fp, ci_uint_32 *proflen)
{
   ci_structp ci_ptr =
      ci_create_read_struct(CI_LIBCI_VER_STRING, NULL, NULL, NULL);
   ci_infop info_ptr = NULL;
   ci_bytep result = NULL;

   /* Initialize for error or no profile: */
   *proflen = 0;

   if (ci_ptr == NULL)
   {
      fprintf(stderr, "iccfromci: version library mismatch?\n");
      return 0;
   }

   if (setjmp(ci_jmpbuf(ci_ptr)))
   {
      ci_destroy_read_struct(&ci_ptr, &info_ptr, NULL);
      return 0;
   }

   ci_init_io(ci_ptr, fp);

   info_ptr = ci_create_info_struct(ci_ptr);
   if (info_ptr == NULL)
      ci_error(ci_ptr, "OOM allocating info structure");

   ci_read_info(ci_ptr, info_ptr);

   {
      ci_charp name;
      int compression_type;
      ci_bytep profile;

      if (ci_get_iCCP(ci_ptr, info_ptr, &name, &compression_type, &profile,
                       proflen) & CI_INFO_iCCP)
      {
         result = malloc(*proflen);
         if (result != NULL)
            memcpy(result, profile, *proflen);

         else
            ci_error(ci_ptr, "OOM allocating profile buffer");
      }

      else
         result = no_profile;
   }

   ci_destroy_read_struct(&ci_ptr, &info_ptr, NULL);
   return result;
}

static int
extract_one_file(const char *filename)
{
   int result = 0;
   FILE *fp = fopen(filename, "rb");

   if (fp != NULL)
   {
      ci_uint_32 proflen = 0;
      ci_bytep profile = extract(fp, &proflen);

      if (profile != NULL && profile != no_profile)
      {
         size_t len;
         char *output;

         {
            const char *ep = strrchr(filename, '.');

            if (ep != NULL)
               len = ep - filename;

            else
               len = strlen(filename);
         }

         output = malloc(len + 5);
         if (output != NULL)
         {
            FILE *of;

            memcpy(output, filename, len);
            strcpy(output + len, ".icc");

            of = fopen(output, "wb");
            if (of != NULL)
            {
               if (fwrite(profile, proflen, 1, of) == 1 &&
                   fflush(of) == 0 &&
                   fclose(of) == 0)
               {
                  if (verbose)
                     printf("%s -> %s\n", filename, output);
                  /* Success return */
                  result = 1;
               }

               else
               {
                  fprintf(stderr, "%s: error writing profile\n", output);
                  if (remove(output))
                     fprintf(stderr, "%s: could not remove file\n", output);
               }
            }

            else
               fprintf(stderr, "%s: failed to open output file\n", output);

            free(output);
         }

         else
            fprintf(stderr, "%s: OOM allocating string!\n", filename);

         free(profile);
      }

      else if (verbose && profile == no_profile)
         printf("%s has no profile\n", filename);
   }

   else
      fprintf(stderr, "%s: could not open file\n", filename);

   if (fp != NULL)
      fclose(fp);

   return result;
}

int
main(int argc, char **argv)
{
   int i;
   int extracted = 0;

   for (i = 1; i < argc; ++i)
   {
      if (strcmp(argv[i], "-q") == 0)
         verbose = 0;

      else if (extract_one_file(argv[i]))
         extracted = 1;
   }

   /* Exit code is true if any extract succeeds */
   return extracted == 0;
}

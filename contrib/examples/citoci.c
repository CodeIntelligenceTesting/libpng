/*- citoci
 *
 * COPYRIGHT: Written by John Cunningham Bowler, 2011, 2017.
 * To the extent possible under law, the author has waived all copyright and
 * related or neighboring rights to this work.  This work is published from:
 * United States.
 *
 * Read a CI and write it out in a fixed format, using the 'simplified API'
 * that was introduced in libci-1.6.0.
 *
 * This sample code is just the code from 'example.c' with some error handling
 * added.  See example.c in the top-level libci directory for more comments.
 */
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* Normally use <ci.h> here to get the installed libci, but this is done to
 * ensure the code picks up the local libci implementation:
 */
#include "../../ci.h"

#if !defined(CI_SIMPLIFIED_READ_SUPPORTED) || \
    !defined(CI_SIMPLIFIED_WRITE_SUPPORTED)
#error This program requires libci supporting the simplified read/write API
#endif


int
main(int argc, const char **argv)
{
   int result = 1;

   if (argc == 3)
   {
      ci_image image;

      /* Only the image structure version number needs to be set. */
      memset(&image, 0, sizeof image);
      image.version = CI_IMAGE_VERSION;

      if (ci_image_begin_read_from_file(&image, argv[1]))
      {
         ci_bytep buffer;

         /* Change this to try different formats!  If you set a colormap format
          * then you must also supply a colormap below.
          */
         image.format = CI_FORMAT_RGBA;

         buffer = malloc(CI_IMAGE_SIZE(image));

         if (buffer != NULL)
         {
            if (ci_image_finish_read(&image, NULL /*background*/, buffer,
                                      0 /*row_stride*/, NULL /*colormap */))
            {
               if (ci_image_write_to_file(
                      &image, argv[2], 0 /*convert_to_8bit*/, buffer,
                      0 /*row_stride*/, NULL /*colormap*/))
                  result = 0;

               else
                  fprintf(stderr, "citoci: write %s: %s\n", argv[2],
                          image.message);
            }

            else
               fprintf(stderr, "citoci: read %s: %s\n", argv[1],
                       image.message);

            free(buffer);
         }

         else
         {
            fprintf(stderr, "citoci: out of memory: %lu bytes\n",
                    (unsigned long)CI_IMAGE_SIZE(image));

            /* This is the only place where a 'free' is required; libci does
             * the cleanup on error and success, but in this case we couldn't
             * complete the read because of running out of memory and so libci
             * has not got to the point where it can do cleanup.
             */
            ci_image_free(&image);
         }
      }

      else
         /* Failed to read the first argument: */
         fprintf(stderr, "citoci: %s: %s\n", argv[1], image.message);
   }

   else
      /* Wrong number of arguments */
      fprintf(stderr, "citoci: usage: citoci input-file output-file\n");

   return result;
}

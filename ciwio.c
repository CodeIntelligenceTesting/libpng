/* ciwio.c - functions for data output
 *
 * Copyright (c) 2018-2025 Cosmin Truta
 * Copyright (c) 1998-2002,2004,2006-2014,2016,2018 Glenn Randers-Pehrson
 * Copyright (c) 1996-1997 Andreas Dilger
 * Copyright (c) 1995-1996 Guy Eric Schalnat, Group 42, Inc.
 *
 * This code is released under the libci license.
 * For conditions of distribution and use, see the disclaimer
 * and license in ci.h
 *
 * This file provides a location for all output.  Users who need
 * special handling are expected to write functions that have the same
 * arguments as these and perform similar functions, but that possibly
 * use different output methods.  Note that you shouldn't change these
 * functions, but rather write replacement functions and then change
 * them at run time with ci_set_write_fn(...).
 */

#include "cipriv.h"

#ifdef CI_WRITE_SUPPORTED

/* Write the data to whatever output you are using.  The default routine
 * writes to a file pointer.  Note that this routine sometimes gets called
 * with very small lengths, so you should implement some kind of simple
 * buffering if you are using unbuffered writes.  This should never be asked
 * to write more than 64K on a 16-bit machine.
 */

void /* PRIVATE */
ci_write_data(ci_structrp ci_ptr, ci_const_bytep data, size_t length)
{
   /* NOTE: write_data_fn must not change the buffer! */
   if (ci_ptr->write_data_fn != NULL )
      (*(ci_ptr->write_data_fn))(ci_ptr, ci_constcast(ci_bytep,data),
          length);

   else
      ci_error(ci_ptr, "Call to NULL write function");
}

#ifdef CI_STDIO_SUPPORTED
/* This is the function that does the actual writing of data.  If you are
 * not writing to a standard C stream, you should create a replacement
 * write_data function and use it at run time with ci_set_write_fn(), rather
 * than changing the library.
 */
void CICBAPI
ci_default_write_data(ci_structp ci_ptr, ci_bytep data, size_t length)
{
   size_t check;

   if (ci_ptr == NULL)
      return;

   check = fwrite(data, 1, length, (FILE *)ci_ptr->io_ptr);

   if (check != length)
      ci_error(ci_ptr, "Write Error");
}
#endif

/* This function is called to output any data pending writing (normally
 * to disk).  After ci_flush is called, there should be no data pending
 * writing in any buffers.
 */
#ifdef CI_WRITE_FLUSH_SUPPORTED
void /* PRIVATE */
ci_flush(ci_structrp ci_ptr)
{
   if (ci_ptr->output_flush_fn != NULL)
      (*(ci_ptr->output_flush_fn))(ci_ptr);
}

#  ifdef CI_STDIO_SUPPORTED
void CICBAPI
ci_default_flush(ci_structp ci_ptr)
{
   FILE *io_ptr;

   if (ci_ptr == NULL)
      return;

   io_ptr = ci_voidcast(FILE *, ci_ptr->io_ptr);
   fflush(io_ptr);
}
#  endif
#endif

/* This function allows the application to supply new output functions for
 * libci if standard C streams aren't being used.
 *
 * This function takes as its arguments:
 * ci_ptr       - pointer to a ci output data structure
 * io_ptr        - pointer to user supplied structure containing info about
 *                 the output functions.  May be NULL.
 * write_data_fn - pointer to a new output function that takes as its
 *                 arguments a pointer to a ci_struct, a pointer to
 *                 data to be written, and a 32-bit unsigned int that is
 *                 the number of bytes to be written.  The new write
 *                 function should call ci_error(ci_ptr, "Error msg")
 *                 to exit and output any fatal error messages.  May be
 *                 NULL, in which case libci's default function will
 *                 be used.
 * flush_data_fn - pointer to a new flush function that takes as its
 *                 arguments a pointer to a ci_struct.  After a call to
 *                 the flush function, there should be no data in any buffers
 *                 or pending transmission.  If the output method doesn't do
 *                 any buffering of output, a function prototype must still be
 *                 supplied although it doesn't have to do anything.  If
 *                 CI_WRITE_FLUSH_SUPPORTED is not defined at libci compile
 *                 time, output_flush_fn will be ignored, although it must be
 *                 supplied for compatibility.  May be NULL, in which case
 *                 libci's default function will be used, if
 *                 CI_WRITE_FLUSH_SUPPORTED is defined.  This is not
 *                 a good idea if io_ptr does not point to a standard
 *                 *FILE structure.
 */
void CIAPI
ci_set_write_fn(ci_structrp ci_ptr, ci_voidp io_ptr,
    ci_rw_ptr write_data_fn, ci_flush_ptr output_flush_fn)
{
   if (ci_ptr == NULL)
      return;

   ci_ptr->io_ptr = io_ptr;

#ifdef CI_STDIO_SUPPORTED
   if (write_data_fn != NULL)
      ci_ptr->write_data_fn = write_data_fn;

   else
      ci_ptr->write_data_fn = ci_default_write_data;
#else
   ci_ptr->write_data_fn = write_data_fn;
#endif

#ifdef CI_WRITE_FLUSH_SUPPORTED
#  ifdef CI_STDIO_SUPPORTED

   if (output_flush_fn != NULL)
      ci_ptr->output_flush_fn = output_flush_fn;

   else
      ci_ptr->output_flush_fn = ci_default_flush;

#  else
   ci_ptr->output_flush_fn = output_flush_fn;
#  endif
#else
   CI_UNUSED(output_flush_fn)
#endif /* WRITE_FLUSH */

#ifdef CI_READ_SUPPORTED
   /* It is an error to read while writing a ci file */
   if (ci_ptr->read_data_fn != NULL)
   {
      ci_ptr->read_data_fn = NULL;

      ci_warning(ci_ptr,
          "Can't set both read_data_fn and write_data_fn in the"
          " same structure");
   }
#endif
}
#endif /* WRITE */

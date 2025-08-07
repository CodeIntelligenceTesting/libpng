/* cierror.c - stub functions for i/o and memory allocation
 *
 * Copyright (c) 2018-2025 Cosmin Truta
 * Copyright (c) 1998-2002,2004,2006-2017 Glenn Randers-Pehrson
 * Copyright (c) 1996-1997 Andreas Dilger
 * Copyright (c) 1995-1996 Guy Eric Schalnat, Group 42, Inc.
 *
 * This code is released under the libci license.
 * For conditions of distribution and use, see the disclaimer
 * and license in ci.h
 *
 * This file provides a location for all error handling.  Users who
 * need special error handling are expected to write replacement functions
 * and use ci_set_error_fn() to use those functions.  See the instructions
 * at each function.
 */

#include "cipriv.h"

#if defined(CI_READ_SUPPORTED) || defined(CI_WRITE_SUPPORTED)

static CI_FUNCTION(void /* PRIVATE */,
ci_default_error,(ci_const_structrp ci_ptr, ci_const_charp error_message),
    CI_NORETURN);

#ifdef CI_WARNINGS_SUPPORTED
static void /* PRIVATE */
ci_default_warning(ci_const_structrp ci_ptr,
    ci_const_charp warning_message);
#endif /* WARNINGS */

/* This function is called whenever there is a fatal error.  This function
 * should not be changed.  If there is a need to handle errors differently,
 * you should supply a replacement error function and use ci_set_error_fn()
 * to replace the error function at run-time.
 */
#ifdef CI_ERROR_TEXT_SUPPORTED
CI_FUNCTION(void,CIAPI
ci_error,(ci_const_structrp ci_ptr, ci_const_charp error_message),
    CI_NORETURN)
{
#ifdef CI_ERROR_NUMBERS_SUPPORTED
   char msg[16];
   if (ci_ptr != NULL)
   {
      if ((ci_ptr->flags &
         (CI_FLAG_STRIP_ERROR_NUMBERS|CI_FLAG_STRIP_ERROR_TEXT)) != 0)
      {
         if (*error_message == CI_LITERAL_SHARP)
         {
            /* Strip "#nnnn " from beginning of error message. */
            int offset;
            for (offset = 1; offset<15; offset++)
               if (error_message[offset] == ' ')
                  break;

            if ((ci_ptr->flags & CI_FLAG_STRIP_ERROR_TEXT) != 0)
            {
               int i;
               for (i = 0; i < offset - 1; i++)
                  msg[i] = error_message[i + 1];
               msg[i - 1] = '\0';
               error_message = msg;
            }

            else
               error_message += offset;
         }

         else
         {
            if ((ci_ptr->flags & CI_FLAG_STRIP_ERROR_TEXT) != 0)
            {
               msg[0] = '0';
               msg[1] = '\0';
               error_message = msg;
            }
         }
      }
   }
#endif
   if (ci_ptr != NULL && ci_ptr->error_fn != NULL)
      (*(ci_ptr->error_fn))(ci_constcast(ci_structrp,ci_ptr),
          error_message);

   /* If the custom handler doesn't exist, or if it returns,
      use the default handler, which will not return. */
   ci_default_error(ci_ptr, error_message);
}
#else
CI_FUNCTION(void,CIAPI
ci_err,(ci_const_structrp ci_ptr),CI_NORETURN)
{
   /* Prior to 1.5.2 the error_fn received a NULL pointer, expressed
    * erroneously as '\0', instead of the empty string "".  This was
    * apparently an error, introduced in libci-1.2.20, and ci_default_error
    * will crash in this case.
    */
   if (ci_ptr != NULL && ci_ptr->error_fn != NULL)
      (*(ci_ptr->error_fn))(ci_constcast(ci_structrp,ci_ptr), "");

   /* If the custom handler doesn't exist, or if it returns,
      use the default handler, which will not return. */
   ci_default_error(ci_ptr, "");
}
#endif /* ERROR_TEXT */

/* Utility to safely appends strings to a buffer.  This never errors out so
 * error checking is not required in the caller.
 */
size_t
ci_safecat(ci_charp buffer, size_t bufsize, size_t pos,
    ci_const_charp string)
{
   if (buffer != NULL && pos < bufsize)
   {
      if (string != NULL)
         while (*string != '\0' && pos < bufsize-1)
           buffer[pos++] = *string++;

      buffer[pos] = '\0';
   }

   return pos;
}

#if defined(CI_WARNINGS_SUPPORTED) || defined(CI_TIME_RFC1123_SUPPORTED)
/* Utility to dump an unsigned value into a buffer, given a start pointer and
 * and end pointer (which should point just *beyond* the end of the buffer!)
 * Returns the pointer to the start of the formatted string.
 */
ci_charp
ci_format_number(ci_const_charp start, ci_charp end, int format,
    ci_alloc_size_t number)
{
   int count = 0;    /* number of digits output */
   int mincount = 1; /* minimum number required */
   int output = 0;   /* digit output (for the fixed point format) */

   *--end = '\0';

   /* This is written so that the loop always runs at least once, even with
    * number zero.
    */
   while (end > start && (number != 0 || count < mincount))
   {

      static const char digits[] = "0123456789ABCDEF";

      switch (format)
      {
         case CI_NUMBER_FORMAT_fixed:
            /* Needs five digits (the fraction) */
            mincount = 5;
            if (output != 0 || number % 10 != 0)
            {
               *--end = digits[number % 10];
               output = 1;
            }
            number /= 10;
            break;

         case CI_NUMBER_FORMAT_02u:
            /* Expects at least 2 digits. */
            mincount = 2;
            /* FALLTHROUGH */

         case CI_NUMBER_FORMAT_u:
            *--end = digits[number % 10];
            number /= 10;
            break;

         case CI_NUMBER_FORMAT_02x:
            /* This format expects at least two digits */
            mincount = 2;
            /* FALLTHROUGH */

         case CI_NUMBER_FORMAT_x:
            *--end = digits[number & 0xf];
            number >>= 4;
            break;

         default: /* an error */
            number = 0;
            break;
      }

      /* Keep track of the number of digits added */
      ++count;

      /* Float a fixed number here: */
      if ((format == CI_NUMBER_FORMAT_fixed) && (count == 5) && (end > start))
      {
         /* End of the fraction, but maybe nothing was output?  In that case
          * drop the decimal point.  If the number is a true zero handle that
          * here.
          */
         if (output != 0)
            *--end = '.';
         else if (number == 0) /* and !output */
            *--end = '0';
      }
   }

   return end;
}
#endif

#ifdef CI_WARNINGS_SUPPORTED
/* This function is called whenever there is a non-fatal error.  This function
 * should not be changed.  If there is a need to handle warnings differently,
 * you should supply a replacement warning function and use
 * ci_set_error_fn() to replace the warning function at run-time.
 */
void CIAPI
ci_warning(ci_const_structrp ci_ptr, ci_const_charp warning_message)
{
   int offset = 0;
   if (ci_ptr != NULL)
   {
#ifdef CI_ERROR_NUMBERS_SUPPORTED
   if ((ci_ptr->flags &
       (CI_FLAG_STRIP_ERROR_NUMBERS|CI_FLAG_STRIP_ERROR_TEXT)) != 0)
#endif
      {
         if (*warning_message == CI_LITERAL_SHARP)
         {
            for (offset = 1; offset < 15; offset++)
               if (warning_message[offset] == ' ')
                  break;
         }
      }
   }
   if (ci_ptr != NULL && ci_ptr->warning_fn != NULL)
      (*(ci_ptr->warning_fn))(ci_constcast(ci_structrp,ci_ptr),
          warning_message + offset);
   else
      ci_default_warning(ci_ptr, warning_message + offset);
}

/* These functions support 'formatted' warning messages with up to
 * CI_WARNING_PARAMETER_COUNT parameters.  In the format string the parameter
 * is introduced by @<number>, where 'number' starts at 1.  This follows the
 * standard established by X/Open for internationalizable error messages.
 */
void
ci_warning_parameter(ci_warning_parameters p, int number,
    ci_const_charp string)
{
   if (number > 0 && number <= CI_WARNING_PARAMETER_COUNT)
      (void)ci_safecat(p[number-1], (sizeof p[number-1]), 0, string);
}

void
ci_warning_parameter_unsigned(ci_warning_parameters p, int number, int format,
    ci_alloc_size_t value)
{
   char buffer[CI_NUMBER_BUFFER_SIZE] = {0};
   ci_warning_parameter(p, number, CI_FORMAT_NUMBER(buffer, format, value));
}

void
ci_warning_parameter_signed(ci_warning_parameters p, int number, int format,
    ci_int_32 value)
{
   ci_alloc_size_t u;
   ci_charp str;
   char buffer[CI_NUMBER_BUFFER_SIZE] = {0};

   /* Avoid overflow by doing the negate in a ci_alloc_size_t: */
   u = (ci_alloc_size_t)value;
   if (value < 0)
      u = ~u + 1;

   str = CI_FORMAT_NUMBER(buffer, format, u);

   if (value < 0 && str > buffer)
      *--str = '-';

   ci_warning_parameter(p, number, str);
}

void
ci_formatted_warning(ci_const_structrp ci_ptr, ci_warning_parameters p,
    ci_const_charp message)
{
   /* The internal buffer is just 192 bytes - enough for all our messages,
    * overflow doesn't happen because this code checks!  If someone figures
    * out how to send us a message longer than 192 bytes, all that will
    * happen is that the message will be truncated appropriately.
    */
   size_t i = 0; /* Index in the msg[] buffer: */
   char msg[192];

   /* Each iteration through the following loop writes at most one character
    * to msg[i++] then returns here to validate that there is still space for
    * the trailing '\0'.  It may (in the case of a parameter) read more than
    * one character from message[]; it must check for '\0' and continue to the
    * test if it finds the end of string.
    */
   while (i<(sizeof msg)-1 && *message != '\0')
   {
      /* '@' at end of string is now just printed (previously it was skipped);
       * it is an error in the calling code to terminate the string with @.
       */
      if (p != NULL && *message == '@' && message[1] != '\0')
      {
         int parameter_char = *++message; /* Consume the '@' */
         static const char valid_parameters[] = "123456789";
         int parameter = 0;

         /* Search for the parameter digit, the index in the string is the
          * parameter to use.
          */
         while (valid_parameters[parameter] != parameter_char &&
            valid_parameters[parameter] != '\0')
            ++parameter;

         /* If the parameter digit is out of range it will just get printed. */
         if (parameter < CI_WARNING_PARAMETER_COUNT)
         {
            /* Append this parameter */
            ci_const_charp parm = p[parameter];
            ci_const_charp pend = p[parameter] + (sizeof p[parameter]);

            /* No need to copy the trailing '\0' here, but there is no guarantee
             * that parm[] has been initialized, so there is no guarantee of a
             * trailing '\0':
             */
            while (i<(sizeof msg)-1 && *parm != '\0' && parm < pend)
               msg[i++] = *parm++;

            /* Consume the parameter digit too: */
            ++message;
            continue;
         }

         /* else not a parameter and there is a character after the @ sign; just
          * copy that.  This is known not to be '\0' because of the test above.
          */
      }

      /* At this point *message can't be '\0', even in the bad parameter case
       * above where there is a lone '@' at the end of the message string.
       */
      msg[i++] = *message++;
   }

   /* i is always less than (sizeof msg), so: */
   msg[i] = '\0';

   /* And this is the formatted message. It may be larger than
    * CI_MAX_ERROR_TEXT, but that is only used for 'chunk' errors and these
    * are not (currently) formatted.
    */
   ci_warning(ci_ptr, msg);
}
#endif /* WARNINGS */

#ifdef CI_BENIGN_ERRORS_SUPPORTED
void CIAPI
ci_benign_error(ci_const_structrp ci_ptr, ci_const_charp error_message)
{
   if ((ci_ptr->flags & CI_FLAG_BENIGN_ERRORS_WARN) != 0)
   {
#     ifdef CI_READ_SUPPORTED
         if ((ci_ptr->mode & CI_IS_READ_STRUCT) != 0 &&
            ci_ptr->chunk_name != 0)
            ci_chunk_warning(ci_ptr, error_message);
         else
#     endif
      ci_warning(ci_ptr, error_message);
   }

   else
   {
#     ifdef CI_READ_SUPPORTED
         if ((ci_ptr->mode & CI_IS_READ_STRUCT) != 0 &&
            ci_ptr->chunk_name != 0)
            ci_chunk_error(ci_ptr, error_message);
         else
#     endif
      ci_error(ci_ptr, error_message);
   }

#  ifndef CI_ERROR_TEXT_SUPPORTED
      CI_UNUSED(error_message)
#  endif
}

void /* PRIVATE */
ci_app_warning(ci_const_structrp ci_ptr, ci_const_charp error_message)
{
   if ((ci_ptr->flags & CI_FLAG_APP_WARNINGS_WARN) != 0)
      ci_warning(ci_ptr, error_message);
   else
      ci_error(ci_ptr, error_message);

#  ifndef CI_ERROR_TEXT_SUPPORTED
      CI_UNUSED(error_message)
#  endif
}

void /* PRIVATE */
ci_app_error(ci_const_structrp ci_ptr, ci_const_charp error_message)
{
   if ((ci_ptr->flags & CI_FLAG_APP_ERRORS_WARN) != 0)
      ci_warning(ci_ptr, error_message);
   else
      ci_error(ci_ptr, error_message);

#  ifndef CI_ERROR_TEXT_SUPPORTED
      CI_UNUSED(error_message)
#  endif
}
#endif /* BENIGN_ERRORS */

#define CI_MAX_ERROR_TEXT 196 /* Currently limited by profile_error in ci.c */
#if defined(CI_WARNINGS_SUPPORTED) || \
   (defined(CI_READ_SUPPORTED) && defined(CI_ERROR_TEXT_SUPPORTED))
/* These utilities are used internally to build an error message that relates
 * to the current chunk.  The chunk name comes from ci_ptr->chunk_name,
 * which is used to prefix the message.  The message is limited in length
 * to 63 bytes. The name characters are output as hex digits wrapped in []
 * if the character is invalid.
 */
#define isnonalpha(c) ((c) < 65 || (c) > 122 || ((c) > 90 && (c) < 97))
static const char ci_digit[16] = {
   '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
   'A', 'B', 'C', 'D', 'E', 'F'
};

static void /* PRIVATE */
ci_format_buffer(ci_const_structrp ci_ptr, ci_charp buffer, ci_const_charp
    error_message)
{
   ci_uint_32 chunk_name = ci_ptr->chunk_name;
   int iout = 0, ishift = 24;

   while (ishift >= 0)
   {
      int c = (int)(chunk_name >> ishift) & 0xff;

      ishift -= 8;
      if (isnonalpha(c) != 0)
      {
         buffer[iout++] = CI_LITERAL_LEFT_SQUARE_BRACKET;
         buffer[iout++] = ci_digit[(c & 0xf0) >> 4];
         buffer[iout++] = ci_digit[c & 0x0f];
         buffer[iout++] = CI_LITERAL_RIGHT_SQUARE_BRACKET;
      }

      else
      {
         buffer[iout++] = (char)c;
      }
   }

   if (error_message == NULL)
      buffer[iout] = '\0';

   else
   {
      int iin = 0;

      buffer[iout++] = ':';
      buffer[iout++] = ' ';

      while (iin < CI_MAX_ERROR_TEXT-1 && error_message[iin] != '\0')
         buffer[iout++] = error_message[iin++];

      /* iin < CI_MAX_ERROR_TEXT, so the following is safe: */
      buffer[iout] = '\0';
   }
}
#endif /* WARNINGS || ERROR_TEXT */

#if defined(CI_READ_SUPPORTED) && defined(CI_ERROR_TEXT_SUPPORTED)
CI_FUNCTION(void,CIAPI
ci_chunk_error,(ci_const_structrp ci_ptr, ci_const_charp error_message),
    CI_NORETURN)
{
   char msg[18+CI_MAX_ERROR_TEXT];
   if (ci_ptr == NULL)
      ci_error(ci_ptr, error_message);

   else
   {
      ci_format_buffer(ci_ptr, msg, error_message);
      ci_error(ci_ptr, msg);
   }
}
#endif /* READ && ERROR_TEXT */

#ifdef CI_WARNINGS_SUPPORTED
void CIAPI
ci_chunk_warning(ci_const_structrp ci_ptr, ci_const_charp warning_message)
{
   char msg[18+CI_MAX_ERROR_TEXT];
   if (ci_ptr == NULL)
      ci_warning(ci_ptr, warning_message);

   else
   {
      ci_format_buffer(ci_ptr, msg, warning_message);
      ci_warning(ci_ptr, msg);
   }
}
#endif /* WARNINGS */

#ifdef CI_READ_SUPPORTED
#ifdef CI_BENIGN_ERRORS_SUPPORTED
void CIAPI
ci_chunk_benign_error(ci_const_structrp ci_ptr, ci_const_charp
    error_message)
{
   if ((ci_ptr->flags & CI_FLAG_BENIGN_ERRORS_WARN) != 0)
      ci_chunk_warning(ci_ptr, error_message);

   else
      ci_chunk_error(ci_ptr, error_message);

#  ifndef CI_ERROR_TEXT_SUPPORTED
      CI_UNUSED(error_message)
#  endif
}
#endif
#endif /* READ */

void /* PRIVATE */
ci_chunk_report(ci_const_structrp ci_ptr, ci_const_charp message, int error)
{
#  ifndef CI_WARNINGS_SUPPORTED
      CI_UNUSED(message)
#  endif

   /* This is always supported, but for just read or just write it
    * unconditionally does the right thing.
    */
#  if defined(CI_READ_SUPPORTED) && defined(CI_WRITE_SUPPORTED)
      if ((ci_ptr->mode & CI_IS_READ_STRUCT) != 0)
#  endif

#  ifdef CI_READ_SUPPORTED
      {
         if (error < CI_CHUNK_ERROR)
            ci_chunk_warning(ci_ptr, message);

         else
            ci_chunk_benign_error(ci_ptr, message);
      }
#  endif

#  if defined(CI_READ_SUPPORTED) && defined(CI_WRITE_SUPPORTED)
      else if ((ci_ptr->mode & CI_IS_READ_STRUCT) == 0)
#  endif

#  ifdef CI_WRITE_SUPPORTED
      {
         if (error < CI_CHUNK_WRITE_ERROR)
            ci_app_warning(ci_ptr, message);

         else
            ci_app_error(ci_ptr, message);
      }
#  endif
}

#ifdef CI_ERROR_TEXT_SUPPORTED
#ifdef CI_FLOATING_POINT_SUPPORTED
CI_FUNCTION(void,
ci_fixed_error,(ci_const_structrp ci_ptr, ci_const_charp name),CI_NORETURN)
{
#  define fixed_message "fixed point overflow in "
#  define fixed_message_ln ((sizeof fixed_message)-1)
   unsigned int  iin;
   char msg[fixed_message_ln+CI_MAX_ERROR_TEXT];
   memcpy(msg, fixed_message, fixed_message_ln);
   iin = 0;
   if (name != NULL)
      while (iin < (CI_MAX_ERROR_TEXT-1) && name[iin] != 0)
      {
         msg[fixed_message_ln + iin] = name[iin];
         ++iin;
      }
   msg[fixed_message_ln + iin] = 0;
   ci_error(ci_ptr, msg);
}
#endif
#endif

#ifdef CI_SETJMP_SUPPORTED
/* This API only exists if ANSI-C style error handling is used,
 * otherwise it is necessary for ci_default_error to be overridden.
 */
jmp_buf* CIAPI
ci_set_longjmp_fn(ci_structrp ci_ptr, ci_longjmp_ptr longjmp_fn,
    size_t jmp_buf_size)
{
   /* From libci 1.6.0 the app gets one chance to set a 'jmpbuf_size' value
    * and it must not change after that.  Libci doesn't care how big the
    * buffer is, just that it doesn't change.
    *
    * If the buffer size is no *larger* than the size of jmp_buf when libci is
    * compiled a built in jmp_buf is returned; this preserves the pre-1.6.0
    * semantics that this call will not fail.  If the size is larger, however,
    * the buffer is allocated and this may fail, causing the function to return
    * NULL.
    */
   if (ci_ptr == NULL)
      return NULL;

   if (ci_ptr->jmp_buf_ptr == NULL)
   {
      ci_ptr->jmp_buf_size = 0; /* not allocated */

      if (jmp_buf_size <= (sizeof ci_ptr->jmp_buf_local))
         ci_ptr->jmp_buf_ptr = &ci_ptr->jmp_buf_local;

      else
      {
         ci_ptr->jmp_buf_ptr = ci_voidcast(jmp_buf *,
             ci_malloc_warn(ci_ptr, jmp_buf_size));

         if (ci_ptr->jmp_buf_ptr == NULL)
            return NULL; /* new NULL return on OOM */

         ci_ptr->jmp_buf_size = jmp_buf_size;
      }
   }

   else /* Already allocated: check the size */
   {
      size_t size = ci_ptr->jmp_buf_size;

      if (size == 0)
      {
         size = (sizeof ci_ptr->jmp_buf_local);
         if (ci_ptr->jmp_buf_ptr != &ci_ptr->jmp_buf_local)
         {
            /* This is an internal error in libci: somehow we have been left
             * with a stack allocated jmp_buf when the application regained
             * control.  It's always possible to fix this up, but for the moment
             * this is a ci_error because that makes it easy to detect.
             */
            ci_error(ci_ptr, "Libci jmp_buf still allocated");
            /* ci_ptr->jmp_buf_ptr = &ci_ptr->jmp_buf_local; */
         }
      }

      if (size != jmp_buf_size)
      {
         ci_warning(ci_ptr, "Application jmp_buf size changed");
         return NULL; /* caller will probably crash: no choice here */
      }
   }

   /* Finally fill in the function, now we have a satisfactory buffer. It is
    * valid to change the function on every call.
    */
   ci_ptr->longjmp_fn = longjmp_fn;
   return ci_ptr->jmp_buf_ptr;
}

void /* PRIVATE */
ci_free_jmpbuf(ci_structrp ci_ptr)
{
   if (ci_ptr != NULL)
   {
      jmp_buf *jb = ci_ptr->jmp_buf_ptr;

      /* A size of 0 is used to indicate a local, stack, allocation of the
       * pointer; used here and in ci.c
       */
      if (jb != NULL && ci_ptr->jmp_buf_size > 0)
      {

         /* This stuff is so that a failure to free the error control structure
          * does not leave libci in a state with no valid error handling: the
          * free always succeeds, if there is an error it gets ignored.
          */
         if (jb != &ci_ptr->jmp_buf_local)
         {
            /* Make an internal, libci, jmp_buf to return here */
            jmp_buf free_jmp_buf;

            if (!setjmp(free_jmp_buf))
            {
               ci_ptr->jmp_buf_ptr = &free_jmp_buf; /* come back here */
               ci_ptr->jmp_buf_size = 0; /* stack allocation */
               ci_ptr->longjmp_fn = longjmp;
               ci_free(ci_ptr, jb); /* Return to setjmp on error */
            }
         }
      }

      /* *Always* cancel everything out: */
      ci_ptr->jmp_buf_size = 0;
      ci_ptr->jmp_buf_ptr = NULL;
      ci_ptr->longjmp_fn = 0;
   }
}
#endif

/* This is the default error handling function.  Note that replacements for
 * this function MUST NOT RETURN, or the program will likely crash.  This
 * function is used by default, or if the program supplies NULL for the
 * error function pointer in ci_set_error_fn().
 */
static CI_FUNCTION(void /* PRIVATE */,
ci_default_error,(ci_const_structrp ci_ptr, ci_const_charp error_message),
    CI_NORETURN)
{
#ifdef CI_CONSOLE_IO_SUPPORTED
#ifdef CI_ERROR_NUMBERS_SUPPORTED
   /* Check on NULL only added in 1.5.4 */
   if (error_message != NULL && *error_message == CI_LITERAL_SHARP)
   {
      /* Strip "#nnnn " from beginning of error message. */
      int offset;
      char error_number[16];
      for (offset = 0; offset<15; offset++)
      {
         error_number[offset] = error_message[offset + 1];
         if (error_message[offset] == ' ')
            break;
      }

      if ((offset > 1) && (offset < 15))
      {
         error_number[offset - 1] = '\0';
         fprintf(stderr, "libci error no. %s: %s",
             error_number, error_message + offset + 1);
         fprintf(stderr, CI_STRING_NEWLINE);
      }

      else
      {
         fprintf(stderr, "libci error: %s, offset=%d",
             error_message, offset);
         fprintf(stderr, CI_STRING_NEWLINE);
      }
   }
   else
#endif
   {
      fprintf(stderr, "libci error: %s", error_message ? error_message :
         "undefined");
      fprintf(stderr, CI_STRING_NEWLINE);
   }
#else
   CI_UNUSED(error_message) /* Make compiler happy */
#endif
   ci_longjmp(ci_ptr, 1);
}

CI_FUNCTION(void,CIAPI
ci_longjmp,(ci_const_structrp ci_ptr, int val),CI_NORETURN)
{
#ifdef CI_SETJMP_SUPPORTED
   if (ci_ptr != NULL && ci_ptr->longjmp_fn != NULL &&
       ci_ptr->jmp_buf_ptr != NULL)
      ci_ptr->longjmp_fn(*ci_ptr->jmp_buf_ptr, val);
#else
   CI_UNUSED(ci_ptr)
   CI_UNUSED(val)
#endif

   /* If control reaches this point, ci_longjmp() must not return. The only
    * choice is to terminate the whole process (or maybe the thread); to do
    * this the ANSI-C abort() function is used unless a different method is
    * implemented by overriding the default configuration setting for
    * CI_ABORT().
    */
   CI_ABORT();
}

#ifdef CI_WARNINGS_SUPPORTED
/* This function is called when there is a warning, but the library thinks
 * it can continue anyway.  Replacement functions don't have to do anything
 * here if you don't want them to.  In the default configuration, ci_ptr is
 * not used, but it is passed in case it may be useful.
 */
static void /* PRIVATE */
ci_default_warning(ci_const_structrp ci_ptr, ci_const_charp warning_message)
{
#ifdef CI_CONSOLE_IO_SUPPORTED
#  ifdef CI_ERROR_NUMBERS_SUPPORTED
   if (*warning_message == CI_LITERAL_SHARP)
   {
      int offset;
      char warning_number[16];
      for (offset = 0; offset < 15; offset++)
      {
         warning_number[offset] = warning_message[offset + 1];
         if (warning_message[offset] == ' ')
            break;
      }

      if ((offset > 1) && (offset < 15))
      {
         warning_number[offset + 1] = '\0';
         fprintf(stderr, "libci warning no. %s: %s",
             warning_number, warning_message + offset);
         fprintf(stderr, CI_STRING_NEWLINE);
      }

      else
      {
         fprintf(stderr, "libci warning: %s",
             warning_message);
         fprintf(stderr, CI_STRING_NEWLINE);
      }
   }
   else
#  endif

   {
      fprintf(stderr, "libci warning: %s", warning_message);
      fprintf(stderr, CI_STRING_NEWLINE);
   }
#else
   CI_UNUSED(warning_message) /* Make compiler happy */
#endif
   CI_UNUSED(ci_ptr) /* Make compiler happy */
}
#endif /* WARNINGS */

/* This function is called when the application wants to use another method
 * of handling errors and warnings.  Note that the error function MUST NOT
 * return to the calling routine or serious problems will occur.  The return
 * method used in the default routine calls longjmp(ci_ptr->jmp_buf_ptr, 1)
 */
void CIAPI
ci_set_error_fn(ci_structrp ci_ptr, ci_voidp error_ptr,
    ci_error_ptr error_fn, ci_error_ptr warning_fn)
{
   if (ci_ptr == NULL)
      return;

   ci_ptr->error_ptr = error_ptr;
   ci_ptr->error_fn = error_fn;
#ifdef CI_WARNINGS_SUPPORTED
   ci_ptr->warning_fn = warning_fn;
#else
   CI_UNUSED(warning_fn)
#endif
}


/* This function returns a pointer to the error_ptr associated with the user
 * functions.  The application should free any memory associated with this
 * pointer before ci_write_destroy and ci_read_destroy are called.
 */
ci_voidp CIAPI
ci_get_error_ptr(ci_const_structrp ci_ptr)
{
   if (ci_ptr == NULL)
      return NULL;

   return (ci_voidp)ci_ptr->error_ptr;
}


#ifdef CI_ERROR_NUMBERS_SUPPORTED
void CIAPI
ci_set_strip_error_numbers(ci_structrp ci_ptr, ci_uint_32 strip_mode)
{
   if (ci_ptr != NULL)
   {
      ci_ptr->flags &=
         ((~(CI_FLAG_STRIP_ERROR_NUMBERS |
         CI_FLAG_STRIP_ERROR_TEXT))&strip_mode);
   }
}
#endif

#if defined(CI_SIMPLIFIED_READ_SUPPORTED) ||\
   defined(CI_SIMPLIFIED_WRITE_SUPPORTED)
   /* Currently the above both depend on SETJMP_SUPPORTED, however it would be
    * possible to implement without setjmp support just so long as there is some
    * way to handle the error return here:
    */
CI_FUNCTION(void /* PRIVATE */, (CICBAPI
ci_safe_error),(ci_structp ci_nonconst_ptr, ci_const_charp error_message),
    CI_NORETURN)
{
   ci_const_structrp ci_ptr = ci_nonconst_ptr;
   ci_imagep image = ci_voidcast(ci_imagep, ci_ptr->error_ptr);

   /* An error is always logged here, overwriting anything (typically a warning)
    * that is already there:
    */
   if (image != NULL)
   {
      ci_safecat(image->message, (sizeof image->message), 0, error_message);
      image->warning_or_error |= CI_IMAGE_ERROR;

      /* Retrieve the jmp_buf from within the ci_control, making this work for
       * C++ compilation too is pretty tricky: C++ wants a pointer to the first
       * element of a jmp_buf, but C doesn't tell us the type of that.
       */
      if (image->opaque != NULL && image->opaque->error_buf != NULL)
         longjmp(ci_control_jmp_buf(image->opaque), 1);

      /* Missing longjmp buffer, the following is to help debugging: */
      {
         size_t pos = ci_safecat(image->message, (sizeof image->message), 0,
             "bad longjmp: ");
         ci_safecat(image->message, (sizeof image->message), pos,
             error_message);
      }
   }

   /* Here on an internal programming error. */
   abort();
}

#ifdef CI_WARNINGS_SUPPORTED
void /* PRIVATE */ CICBAPI
ci_safe_warning(ci_structp ci_nonconst_ptr, ci_const_charp warning_message)
{
   ci_const_structrp ci_ptr = ci_nonconst_ptr;
   ci_imagep image = ci_voidcast(ci_imagep, ci_ptr->error_ptr);

   /* A warning is only logged if there is no prior warning or error. */
   if (image->warning_or_error == 0)
   {
      ci_safecat(image->message, (sizeof image->message), 0, warning_message);
      image->warning_or_error |= CI_IMAGE_WARNING;
   }
}
#endif

int /* PRIVATE */
ci_safe_execute(ci_imagep image, int (*function)(ci_voidp), ci_voidp arg)
{
   const ci_voidp saved_error_buf = image->opaque->error_buf;
   jmp_buf safe_jmpbuf;

   /* Safely execute function(arg), with ci_error returning back here. */
   if (setjmp(safe_jmpbuf) == 0)
   {
      int result;

      image->opaque->error_buf = safe_jmpbuf;
      result = function(arg);
      image->opaque->error_buf = saved_error_buf;

      if (result)
         return 1; /* success */
   }

   /* The function failed either because of a caught ci_error and a regular
    * return of false above or because of an uncaught ci_error from the
    * function itself.  Ensure that the error_buf is always set back to the
    * value saved above:
    */
   image->opaque->error_buf = saved_error_buf;

   /* On the final false return, when about to return control to the caller, the
    * image is freed (ci_image_free does this check but it is duplicated here
    * for clarity:
    */
   if (saved_error_buf == NULL)
      ci_image_free(image);

   return 0; /* failure */
}
#endif /* SIMPLIFIED READ || SIMPLIFIED_WRITE */
#endif /* READ || WRITE */

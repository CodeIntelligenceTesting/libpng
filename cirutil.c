/* cirutil.c - utilities to read a CI file
 *
 * Copyright (c) 2018-2025 Cosmin Truta
 * Copyright (c) 1998-2002,2004,2006-2018 Glenn Randers-Pehrson
 * Copyright (c) 1996-1997 Andreas Dilger
 * Copyright (c) 1995-1996 Guy Eric Schalnat, Group 42, Inc.
 *
 * This code is released under the libci license.
 * For conditions of distribution and use, see the disclaimer
 * and license in ci.h
 *
 * This file contains routines that are only called from within
 * libci itself during the course of reading an image.
 */

#include "cipriv.h"

#ifdef CI_READ_SUPPORTED

/* The minimum 'zlib' stream is assumed to be just the 2 byte header, 5 bytes
 * minimum 'deflate' stream, and the 4 byte checksum.
 */
#define LZ77Min  (2U+5U+4U)

#ifdef CI_READ_INTERLACING_SUPPORTED
/* Arrays to facilitate interlacing - use pass (0 - 6) as index. */

/* Start of interlace block */
static const ci_byte ci_pass_start[7] = {0, 4, 0, 2, 0, 1, 0};
/* Offset to next interlace block */
static const ci_byte ci_pass_inc[7] = {8, 8, 4, 4, 2, 2, 1};
/* Start of interlace block in the y direction */
static const ci_byte ci_pass_ystart[7] = {0, 0, 4, 0, 2, 0, 1};
/* Offset to next interlace block in the y direction */
static const ci_byte ci_pass_yinc[7] = {8, 8, 8, 4, 4, 2, 2};

/* TODO: Move these arrays to a common utility module to avoid duplication. */
#endif

ci_uint_32 CIAPI
ci_get_uint_31(ci_const_structrp ci_ptr, ci_const_bytep buf)
{
   ci_uint_32 uval = ci_get_uint_32(buf);

   if (uval > CI_UINT_31_MAX)
      ci_error(ci_ptr, "CI unsigned integer out of range");

   return uval;
}

#ifdef CI_READ_INT_FUNCTIONS_SUPPORTED
/* NOTE: the read macros will obscure these definitions, so that if
 * CI_USE_READ_MACROS is set the library will not use them internally,
 * but the APIs will still be available externally.
 *
 * The parentheses around "CIAPI function_name" in the following three
 * functions are necessary because they allow the macros to co-exist with
 * these (unused but exported) functions.
 */

/* Grab an unsigned 32-bit integer from a buffer in big-endian format. */
ci_uint_32 (CIAPI
ci_get_uint_32)(ci_const_bytep buf)
{
   ci_uint_32 uval =
       ((ci_uint_32)(*(buf    )) << 24) +
       ((ci_uint_32)(*(buf + 1)) << 16) +
       ((ci_uint_32)(*(buf + 2)) <<  8) +
       ((ci_uint_32)(*(buf + 3))      ) ;

   return uval;
}

/* Grab a signed 32-bit integer from a buffer in big-endian format.  The
 * data is stored in the CI file in two's complement format and there
 * is no guarantee that a 'ci_int_32' is exactly 32 bits, therefore
 * the following code does a two's complement to native conversion.
 */
ci_int_32 (CIAPI
ci_get_int_32)(ci_const_bytep buf)
{
   ci_uint_32 uval = ci_get_uint_32(buf);
   if ((uval & 0x80000000) == 0) /* non-negative */
      return (ci_int_32)uval;

   uval = (uval ^ 0xffffffff) + 1;  /* 2's complement: -x = ~x+1 */
   if ((uval & 0x80000000) == 0) /* no overflow */
      return -(ci_int_32)uval;
   /* The following has to be safe; this function only gets called on CI data
    * and if we get here that data is invalid.  0 is the most safe value and
    * if not then an attacker would surely just generate a CI with 0 instead.
    */
   return 0;
}

/* Grab an unsigned 16-bit integer from a buffer in big-endian format. */
ci_uint_16 (CIAPI
ci_get_uint_16)(ci_const_bytep buf)
{
   /* ANSI-C requires an int value to accommodate at least 16 bits so this
    * works and allows the compiler not to worry about possible narrowing
    * on 32-bit systems.  (Pre-ANSI systems did not make integers smaller
    * than 16 bits either.)
    */
   unsigned int val =
       ((unsigned int)(*buf) << 8) +
       ((unsigned int)(*(buf + 1)));

   return (ci_uint_16)val;
}

#endif /* READ_INT_FUNCTIONS */

/* Read and check the CI file signature */
void /* PRIVATE */
ci_read_sig(ci_structrp ci_ptr, ci_inforp info_ptr)
{
   size_t num_checked, num_to_check;

   /* Exit if the user application does not expect a signature. */
   if (ci_ptr->sig_bytes >= 8)
      return;

   num_checked = ci_ptr->sig_bytes;
   num_to_check = 8 - num_checked;

#ifdef CI_IO_STATE_SUPPORTED
   ci_ptr->io_state = CI_IO_READING | CI_IO_SIGNATURE;
#endif

   /* The signature must be serialized in a single I/O call. */
   ci_read_data(ci_ptr, &(info_ptr->signature[num_checked]), num_to_check);
   ci_ptr->sig_bytes = 8;

   if (ci_sig_cmp(info_ptr->signature, num_checked, num_to_check) != 0)
   {
      if (num_checked < 4 &&
          ci_sig_cmp(info_ptr->signature, num_checked, num_to_check - 4) != 0)
         ci_error(ci_ptr, "Not a CI file");
      else
         ci_error(ci_ptr, "CI file corrupted by ASCII conversion");
   }
   if (num_checked < 3)
      ci_ptr->mode |= CI_HAVE_CI_SIGNATURE;
}

/* This function is called to verify that a chunk name is valid.
 * Do this using the bit-whacking approach from contrib/tools/cifix.c
 *
 * Copied from libci 1.7.
 */
static int
check_chunk_name(ci_uint_32 name)
{
   ci_uint_32 t;

   /* Remove bit 5 from all but the reserved byte; this means
    * every 8-bit unit must be in the range 65-90 to be valid.
    * So bit 5 must be zero, bit 6 must be set and bit 7 zero.
    */
   name &= ~CI_U32(32,32,0,32);
   t = (name & ~0x1f1f1f1fU) ^ 0x40404040U;

   /* Subtract 65 for each 8-bit quantity, this must not
    * overflow and each byte must then be in the range 0-25.
    */
   name -= CI_U32(65,65,65,65);
   t |= name;

   /* Subtract 26, handling the overflow which should set the
    * top three bits of each byte.
    */
   name -= CI_U32(25,25,25,26);
   t |= ~name;

   return (t & 0xe0e0e0e0U) == 0U;
}

/* Read the chunk header (length + type name).
 * Put the type name into ci_ptr->chunk_name, and return the length.
 */
ci_uint_32 /* PRIVATE */
ci_read_chunk_header(ci_structrp ci_ptr)
{
   ci_byte buf[8];
   ci_uint_32 chunk_name, length;

#ifdef CI_IO_STATE_SUPPORTED
   ci_ptr->io_state = CI_IO_READING | CI_IO_CHUNK_HDR;
#endif

   /* Read the length and the chunk name.  ci_struct::chunk_name is immediately
    * updated even if they are detectably wrong.  This aids error message
    * handling by allowing ci_chunk_error to be used.
    */
   ci_read_data(ci_ptr, buf, 8);
   length = ci_get_uint_31(ci_ptr, buf);
   ci_ptr->chunk_name = chunk_name = CI_CHUNK_FROM_STRING(buf+4);

   /* Reset the crc and run it over the chunk name. */
   ci_reset_crc(ci_ptr);
   ci_calculate_crc(ci_ptr, buf + 4, 4);

   ci_debug2(0, "Reading chunk typeid = 0x%lx, length = %lu",
       (unsigned long)ci_ptr->chunk_name, (unsigned long)length);

   /* Sanity check the length (first by <= 0x80) and the chunk name.  An error
    * here indicates a broken stream and libci has no recovery from this.
    */
   if (buf[0] >= 0x80U)
      ci_chunk_error(ci_ptr, "bad header (invalid length)");

   /* Check to see if chunk name is valid. */
   if (!check_chunk_name(chunk_name))
      ci_chunk_error(ci_ptr, "bad header (invalid type)");

#ifdef CI_IO_STATE_SUPPORTED
   ci_ptr->io_state = CI_IO_READING | CI_IO_CHUNK_DATA;
#endif

   return length;
}

/* Read data, and (optionally) run it through the CRC. */
void /* PRIVATE */
ci_crc_read(ci_structrp ci_ptr, ci_bytep buf, ci_uint_32 length)
{
   if (ci_ptr == NULL)
      return;

   ci_read_data(ci_ptr, buf, length);
   ci_calculate_crc(ci_ptr, buf, length);
}

/* Compare the CRC stored in the CI file with that calculated by libci from
 * the data it has read thus far.
 */
static int
ci_crc_error(ci_structrp ci_ptr, int handle_as_ancillary)
{
   ci_byte crc_bytes[4];
   ci_uint_32 crc;
   int need_crc = 1;

   /* There are four flags two for ancillary and two for critical chunks.  The
    * default setting of these flags is all zero.
    *
    * CI_FLAG_CRC_ANCILLARY_USE
    * CI_FLAG_CRC_ANCILLARY_NOWARN
    *  USE+NOWARN: no CRC calculation (implemented here), else;
    *  NOWARN:     ci_chunk_error on error (implemented in ci_crc_finish)
    *  else:       ci_chunk_warning on error (implemented in ci_crc_finish)
    *              This is the default.
    *
    *    I.e. NOWARN without USE produces ci_chunk_error.  The default setting
    *    where neither are set does the same thing.
    *
    * CI_FLAG_CRC_CRITICAL_USE
    * CI_FLAG_CRC_CRITICAL_IGNORE
    *  IGNORE: no CRC calculation (implemented here), else;
    *  USE:    ci_chunk_warning on error (implemented in ci_crc_finish)
    *  else:   ci_chunk_error on error (implemented in ci_crc_finish)
    *          This is the default.
    *
    * This arose because of original mis-implementation and has persisted for
    * compatibility reasons.
    *
    * TODO: the flag names are internal so maybe this can be changed to
    * something comprehensible.
    */
   if (handle_as_ancillary || CI_CHUNK_ANCILLARY(ci_ptr->chunk_name) != 0)
   {
      if ((ci_ptr->flags & CI_FLAG_CRC_ANCILLARY_MASK) ==
          (CI_FLAG_CRC_ANCILLARY_USE | CI_FLAG_CRC_ANCILLARY_NOWARN))
         need_crc = 0;
   }

   else /* critical */
   {
      if ((ci_ptr->flags & CI_FLAG_CRC_CRITICAL_IGNORE) != 0)
         need_crc = 0;
   }

#ifdef CI_IO_STATE_SUPPORTED
   ci_ptr->io_state = CI_IO_READING | CI_IO_CHUNK_CRC;
#endif

   /* The chunk CRC must be serialized in a single I/O call. */
   ci_read_data(ci_ptr, crc_bytes, 4);

   if (need_crc != 0)
   {
      crc = ci_get_uint_32(crc_bytes);
      return crc != ci_ptr->crc;
   }

   else
      return 0;
}

/* Optionally skip data and then check the CRC.  Depending on whether we
 * are reading an ancillary or critical chunk, and how the program has set
 * things up, we may calculate the CRC on the data and print a message.
 * Returns '1' if there was a CRC error, '0' otherwise.
 *
 * There is one public version which is used in most places and another which
 * takes the value for the 'critical' flag to check.  This allows PLTE and IEND
 * handling code to ignore the CRC error and removes some confusing code
 * duplication.
 */
static int
ci_crc_finish_critical(ci_structrp ci_ptr, ci_uint_32 skip,
      int handle_as_ancillary)
{
   /* The size of the local buffer for inflate is a good guess as to a
    * reasonable size to use for buffering reads from the application.
    */
   while (skip > 0)
   {
      ci_uint_32 len;
      ci_byte tmpbuf[CI_INFLATE_BUF_SIZE];

      len = (sizeof tmpbuf);
      if (len > skip)
         len = skip;
      skip -= len;

      ci_crc_read(ci_ptr, tmpbuf, len);
   }

   /* If 'handle_as_ancillary' has been requested and this is a critical chunk
    * but CI_FLAG_CRC_CRITICAL_IGNORE was set then ci_read_crc did not, in
    * fact, calculate the CRC so the ANCILLARY settings should not be used
    * instead.
    */
   if (handle_as_ancillary &&
       (ci_ptr->flags & CI_FLAG_CRC_CRITICAL_IGNORE) != 0)
      handle_as_ancillary = 0;

   /* TODO: this might be more comprehensible if ci_crc_error was inlined here.
    */
   if (ci_crc_error(ci_ptr, handle_as_ancillary) != 0)
   {
      /* See above for the explanation of how the flags work. */
      if (handle_as_ancillary || CI_CHUNK_ANCILLARY(ci_ptr->chunk_name) != 0 ?
          (ci_ptr->flags & CI_FLAG_CRC_ANCILLARY_NOWARN) == 0 :
          (ci_ptr->flags & CI_FLAG_CRC_CRITICAL_USE) != 0)
         ci_chunk_warning(ci_ptr, "CRC error");

      else
         ci_chunk_error(ci_ptr, "CRC error");

      return 1;
   }

   return 0;
}

int /* PRIVATE */
ci_crc_finish(ci_structrp ci_ptr, ci_uint_32 skip)
{
   return ci_crc_finish_critical(ci_ptr, skip, 0/*critical handling*/);
}

#if defined(CI_READ_iCCP_SUPPORTED) || defined(CI_READ_iTXt_SUPPORTED) ||\
    defined(CI_READ_pCAL_SUPPORTED) || defined(CI_READ_sCAL_SUPPORTED) ||\
    defined(CI_READ_sPLT_SUPPORTED) || defined(CI_READ_tEXt_SUPPORTED) ||\
    defined(CI_READ_zTXt_SUPPORTED) || defined(CI_READ_eXIf_SUPPORTED) ||\
    defined(CI_SEQUENTIAL_READ_SUPPORTED)
/* Manage the read buffer; this simply reallocates the buffer if it is not small
 * enough (or if it is not allocated).  The routine returns a pointer to the
 * buffer; if an error occurs and 'warn' is set the routine returns NULL, else
 * it will call ci_error on failure.
 */
static ci_bytep
ci_read_buffer(ci_structrp ci_ptr, ci_alloc_size_t new_size)
{
   ci_bytep buffer = ci_ptr->read_buffer;

   if (new_size > ci_chunk_max(ci_ptr)) return NULL;

   if (buffer != NULL && new_size > ci_ptr->read_buffer_size)
   {
      ci_ptr->read_buffer = NULL;
      ci_ptr->read_buffer_size = 0;
      ci_free(ci_ptr, buffer);
      buffer = NULL;
   }

   if (buffer == NULL)
   {
      buffer = ci_voidcast(ci_bytep, ci_malloc_base(ci_ptr, new_size));

      if (buffer != NULL)
      {
#        ifndef CI_NO_MEMZERO /* for detecting UIM bugs **only** */
            memset(buffer, 0, new_size); /* just in case */
#        endif
         ci_ptr->read_buffer = buffer;
         ci_ptr->read_buffer_size = new_size;
      }
   }

   return buffer;
}
#endif /* READ_iCCP|iTXt|pCAL|sCAL|sPLT|tEXt|zTXt|eXIf|SEQUENTIAL_READ */

/* ci_inflate_claim: claim the zstream for some nefarious purpose that involves
 * decompression.  Returns Z_OK on success, else a zlib error code.  It checks
 * the owner but, in final release builds, just issues a warning if some other
 * chunk apparently owns the stream.  Prior to release it does a ci_error.
 */
static int
ci_inflate_claim(ci_structrp ci_ptr, ci_uint_32 owner)
{
   if (ci_ptr->zowner != 0)
   {
      char msg[64];

      CI_STRING_FROM_CHUNK(msg, ci_ptr->zowner);
      /* So the message that results is "<chunk> using zstream"; this is an
       * internal error, but is very useful for debugging.  i18n requirements
       * are minimal.
       */
      (void)ci_safecat(msg, (sizeof msg), 4, " using zstream");
#if CI_RELEASE_BUILD
      ci_chunk_warning(ci_ptr, msg);
      ci_ptr->zowner = 0;
#else
      ci_chunk_error(ci_ptr, msg);
#endif
   }

   /* Implementation note: unlike 'ci_deflate_claim' this internal function
    * does not take the size of the data as an argument.  Some efficiency could
    * be gained by using this when it is known *if* the zlib stream itself does
    * not record the number; however, this is an illusion: the original writer
    * of the CI may have selected a lower window size, and we really must
    * follow that because, for systems with with limited capabilities, we
    * would otherwise reject the application's attempts to use a smaller window
    * size (zlib doesn't have an interface to say "this or lower"!).
    *
    * inflateReset2 was added to zlib 1.2.4; before this the window could not be
    * reset, therefore it is necessary to always allocate the maximum window
    * size with earlier zlibs just in case later compressed chunks need it.
    */
   {
      int ret; /* zlib return code */
#if ZLIB_VERNUM >= 0x1240
      int window_bits = 0;

# if defined(CI_SET_OPTION_SUPPORTED) && defined(CI_MAXIMUM_INFLATE_WINDOW)
      if (((ci_ptr->options >> CI_MAXIMUM_INFLATE_WINDOW) & 3) ==
          CI_OPTION_ON)
      {
         window_bits = 15;
         ci_ptr->zstream_start = 0; /* fixed window size */
      }

      else
      {
         ci_ptr->zstream_start = 1;
      }
# endif

#endif /* ZLIB_VERNUM >= 0x1240 */

      /* Set this for safety, just in case the previous owner left pointers to
       * memory allocations.
       */
      ci_ptr->zstream.next_in = NULL;
      ci_ptr->zstream.avail_in = 0;
      ci_ptr->zstream.next_out = NULL;
      ci_ptr->zstream.avail_out = 0;

      if ((ci_ptr->flags & CI_FLAG_ZSTREAM_INITIALIZED) != 0)
      {
#if ZLIB_VERNUM >= 0x1240
         ret = inflateReset2(&ci_ptr->zstream, window_bits);
#else
         ret = inflateReset(&ci_ptr->zstream);
#endif
      }

      else
      {
#if ZLIB_VERNUM >= 0x1240
         ret = inflateInit2(&ci_ptr->zstream, window_bits);
#else
         ret = inflateInit(&ci_ptr->zstream);
#endif

         if (ret == Z_OK)
            ci_ptr->flags |= CI_FLAG_ZSTREAM_INITIALIZED;
      }

#ifdef CI_DISABLE_ADLER32_CHECK_SUPPORTED
      if (((ci_ptr->options >> CI_IGNORE_ADLER32) & 3) == CI_OPTION_ON)
         /* Turn off validation of the ADLER32 checksum in IDAT chunks */
         ret = inflateValidate(&ci_ptr->zstream, 0);
#endif

      if (ret == Z_OK)
         ci_ptr->zowner = owner;

      else
         ci_zstream_error(ci_ptr, ret);

      return ret;
   }

#ifdef window_bits
# undef window_bits
#endif
}

#if ZLIB_VERNUM >= 0x1240
/* Handle the start of the inflate stream if we called inflateInit2(strm,0);
 * in this case some zlib versions skip validation of the CINFO field and, in
 * certain circumstances, libci may end up displaying an invalid image, in
 * contrast to implementations that call zlib in the normal way (e.g. libci
 * 1.5).
 */
int /* PRIVATE */
ci_zlib_inflate(ci_structrp ci_ptr, int flush)
{
   if (ci_ptr->zstream_start && ci_ptr->zstream.avail_in > 0)
   {
      if ((*ci_ptr->zstream.next_in >> 4) > 7)
      {
         ci_ptr->zstream.msg = "invalid window size (libci)";
         return Z_DATA_ERROR;
      }

      ci_ptr->zstream_start = 0;
   }

   return inflate(&ci_ptr->zstream, flush);
}
#endif /* Zlib >= 1.2.4 */

#ifdef CI_READ_COMPRESSED_TEXT_SUPPORTED
#if defined(CI_READ_zTXt_SUPPORTED) || defined (CI_READ_iTXt_SUPPORTED)
/* ci_inflate now returns zlib error codes including Z_OK and Z_STREAM_END to
 * allow the caller to do multiple calls if required.  If the 'finish' flag is
 * set Z_FINISH will be passed to the final inflate() call and Z_STREAM_END must
 * be returned or there has been a problem, otherwise Z_SYNC_FLUSH is used and
 * Z_OK or Z_STREAM_END will be returned on success.
 *
 * The input and output sizes are updated to the actual amounts of data consumed
 * or written, not the amount available (as in a z_stream).  The data pointers
 * are not changed, so the next input is (data+input_size) and the next
 * available output is (output+output_size).
 */
static int
ci_inflate(ci_structrp ci_ptr, ci_uint_32 owner, int finish,
    /* INPUT: */ ci_const_bytep input, ci_uint_32p input_size_ptr,
    /* OUTPUT: */ ci_bytep output, ci_alloc_size_t *output_size_ptr)
{
   if (ci_ptr->zowner == owner) /* Else not claimed */
   {
      int ret;
      ci_alloc_size_t avail_out = *output_size_ptr;
      ci_uint_32 avail_in = *input_size_ptr;

      /* zlib can't necessarily handle more than 65535 bytes at once (i.e. it
       * can't even necessarily handle 65536 bytes) because the type uInt is
       * "16 bits or more".  Consequently it is necessary to chunk the input to
       * zlib.  This code uses ZLIB_IO_MAX, from cipriv.h, as the maximum (the
       * maximum value that can be stored in a uInt.)  It is possible to set
       * ZLIB_IO_MAX to a lower value in cipriv.h and this may sometimes have
       * a performance advantage, because it reduces the amount of data accessed
       * at each step and that may give the OS more time to page it in.
       */
      ci_ptr->zstream.next_in = CIZ_INPUT_CAST(input);
      /* avail_in and avail_out are set below from 'size' */
      ci_ptr->zstream.avail_in = 0;
      ci_ptr->zstream.avail_out = 0;

      /* Read directly into the output if it is available (this is set to
       * a local buffer below if output is NULL).
       */
      if (output != NULL)
         ci_ptr->zstream.next_out = output;

      do
      {
         uInt avail;
         Byte local_buffer[CI_INFLATE_BUF_SIZE];

         /* zlib INPUT BUFFER */
         /* The setting of 'avail_in' used to be outside the loop; by setting it
          * inside it is possible to chunk the input to zlib and simply rely on
          * zlib to advance the 'next_in' pointer.  This allows arbitrary
          * amounts of data to be passed through zlib at the unavoidable cost of
          * requiring a window save (memcpy of up to 32768 output bytes)
          * every ZLIB_IO_MAX input bytes.
          */
         avail_in += ci_ptr->zstream.avail_in; /* not consumed last time */

         avail = ZLIB_IO_MAX;

         if (avail_in < avail)
            avail = (uInt)avail_in; /* safe: < than ZLIB_IO_MAX */

         avail_in -= avail;
         ci_ptr->zstream.avail_in = avail;

         /* zlib OUTPUT BUFFER */
         avail_out += ci_ptr->zstream.avail_out; /* not written last time */

         avail = ZLIB_IO_MAX; /* maximum zlib can process */

         if (output == NULL)
         {
            /* Reset the output buffer each time round if output is NULL and
             * make available the full buffer, up to 'remaining_space'
             */
            ci_ptr->zstream.next_out = local_buffer;
            if ((sizeof local_buffer) < avail)
               avail = (sizeof local_buffer);
         }

         if (avail_out < avail)
            avail = (uInt)avail_out; /* safe: < ZLIB_IO_MAX */

         ci_ptr->zstream.avail_out = avail;
         avail_out -= avail;

         /* zlib inflate call */
         /* In fact 'avail_out' may be 0 at this point, that happens at the end
          * of the read when the final LZ end code was not passed at the end of
          * the previous chunk of input data.  Tell zlib if we have reached the
          * end of the output buffer.
          */
         ret = CI_INFLATE(ci_ptr, avail_out > 0 ? Z_NO_FLUSH :
             (finish ? Z_FINISH : Z_SYNC_FLUSH));
      } while (ret == Z_OK);

      /* For safety kill the local buffer pointer now */
      if (output == NULL)
         ci_ptr->zstream.next_out = NULL;

      /* Claw back the 'size' and 'remaining_space' byte counts. */
      avail_in += ci_ptr->zstream.avail_in;
      avail_out += ci_ptr->zstream.avail_out;

      /* Update the input and output sizes; the updated values are the amount
       * consumed or written, effectively the inverse of what zlib uses.
       */
      if (avail_out > 0)
         *output_size_ptr -= avail_out;

      if (avail_in > 0)
         *input_size_ptr -= avail_in;

      /* Ensure ci_ptr->zstream.msg is set (even in the success case!) */
      ci_zstream_error(ci_ptr, ret);
      return ret;
   }

   else
   {
      /* This is a bad internal error.  The recovery assigns to the zstream msg
       * pointer, which is not owned by the caller, but this is safe; it's only
       * used on errors!
       */
      ci_ptr->zstream.msg = CIZ_MSG_CAST("zstream unclaimed");
      return Z_STREAM_ERROR;
   }
}

/*
 * Decompress trailing data in a chunk.  The assumption is that read_buffer
 * points at an allocated area holding the contents of a chunk with a
 * trailing compressed part.  What we get back is an allocated area
 * holding the original prefix part and an uncompressed version of the
 * trailing part (the malloc area passed in is freed).
 */
static int
ci_decompress_chunk(ci_structrp ci_ptr,
    ci_uint_32 chunklength, ci_uint_32 prefix_size,
    ci_alloc_size_t *newlength /* must be initialized to the maximum! */,
    int terminate /*add a '\0' to the end of the uncompressed data*/)
{
   /* TODO: implement different limits for different types of chunk.
    *
    * The caller supplies *newlength set to the maximum length of the
    * uncompressed data, but this routine allocates space for the prefix and
    * maybe a '\0' terminator too.  We have to assume that 'prefix_size' is
    * limited only by the maximum chunk size.
    */
   ci_alloc_size_t limit = ci_chunk_max(ci_ptr);

   if (limit >= prefix_size + (terminate != 0))
   {
      int ret;

      limit -= prefix_size + (terminate != 0);

      if (limit < *newlength)
         *newlength = limit;

      /* Now try to claim the stream. */
      ret = ci_inflate_claim(ci_ptr, ci_ptr->chunk_name);

      if (ret == Z_OK)
      {
         ci_uint_32 lzsize = chunklength - prefix_size;

         ret = ci_inflate(ci_ptr, ci_ptr->chunk_name, 1/*finish*/,
             /* input: */ ci_ptr->read_buffer + prefix_size, &lzsize,
             /* output: */ NULL, newlength);

         if (ret == Z_STREAM_END)
         {
            /* Use 'inflateReset' here, not 'inflateReset2' because this
             * preserves the previously decided window size (otherwise it would
             * be necessary to store the previous window size.)  In practice
             * this doesn't matter anyway, because ci_inflate will call inflate
             * with Z_FINISH in almost all cases, so the window will not be
             * maintained.
             */
            if (inflateReset(&ci_ptr->zstream) == Z_OK)
            {
               /* Because of the limit checks above we know that the new,
                * expanded, size will fit in a size_t (let alone an
                * ci_alloc_size_t).  Use ci_malloc_base here to avoid an
                * extra OOM message.
                */
               ci_alloc_size_t new_size = *newlength;
               ci_alloc_size_t buffer_size = prefix_size + new_size +
                   (terminate != 0);
               ci_bytep text = ci_voidcast(ci_bytep, ci_malloc_base(ci_ptr,
                   buffer_size));

               if (text != NULL)
               {
                  memset(text, 0, buffer_size);

                  ret = ci_inflate(ci_ptr, ci_ptr->chunk_name, 1/*finish*/,
                      ci_ptr->read_buffer + prefix_size, &lzsize,
                      text + prefix_size, newlength);

                  if (ret == Z_STREAM_END)
                  {
                     if (new_size == *newlength)
                     {
                        if (terminate != 0)
                           text[prefix_size + *newlength] = 0;

                        if (prefix_size > 0)
                           memcpy(text, ci_ptr->read_buffer, prefix_size);

                        {
                           ci_bytep old_ptr = ci_ptr->read_buffer;

                           ci_ptr->read_buffer = text;
                           ci_ptr->read_buffer_size = buffer_size;
                           text = old_ptr; /* freed below */
                        }
                     }

                     else
                     {
                        /* The size changed on the second read, there can be no
                         * guarantee that anything is correct at this point.
                         * The 'msg' pointer has been set to "unexpected end of
                         * LZ stream", which is fine, but return an error code
                         * that the caller won't accept.
                         */
                        ret = CI_UNEXPECTED_ZLIB_RETURN;
                     }
                  }

                  else if (ret == Z_OK)
                     ret = CI_UNEXPECTED_ZLIB_RETURN; /* for safety */

                  /* Free the text pointer (this is the old read_buffer on
                   * success)
                   */
                  ci_free(ci_ptr, text);

                  /* This really is very benign, but it's still an error because
                   * the extra space may otherwise be used as a Trojan Horse.
                   */
                  if (ret == Z_STREAM_END &&
                      chunklength - prefix_size != lzsize)
                     ci_chunk_benign_error(ci_ptr, "extra compressed data");
               }

               else
               {
                  /* Out of memory allocating the buffer */
                  ret = Z_MEM_ERROR;
                  ci_zstream_error(ci_ptr, Z_MEM_ERROR);
               }
            }

            else
            {
               /* inflateReset failed, store the error message */
               ci_zstream_error(ci_ptr, ret);
               ret = CI_UNEXPECTED_ZLIB_RETURN;
            }
         }

         else if (ret == Z_OK)
            ret = CI_UNEXPECTED_ZLIB_RETURN;

         /* Release the claimed stream */
         ci_ptr->zowner = 0;
      }

      else /* the claim failed */ if (ret == Z_STREAM_END) /* impossible! */
         ret = CI_UNEXPECTED_ZLIB_RETURN;

      return ret;
   }

   else
   {
      /* Application/configuration limits exceeded */
      ci_zstream_error(ci_ptr, Z_MEM_ERROR);
      return Z_MEM_ERROR;
   }
}
#endif /* READ_zTXt || READ_iTXt */
#endif /* READ_COMPRESSED_TEXT */

#ifdef CI_READ_iCCP_SUPPORTED
/* Perform a partial read and decompress, producing 'avail_out' bytes and
 * reading from the current chunk as required.
 */
static int
ci_inflate_read(ci_structrp ci_ptr, ci_bytep read_buffer, uInt read_size,
    ci_uint_32p chunk_bytes, ci_bytep next_out, ci_alloc_size_t *out_size,
    int finish)
{
   if (ci_ptr->zowner == ci_ptr->chunk_name)
   {
      int ret;

      /* next_in and avail_in must have been initialized by the caller. */
      ci_ptr->zstream.next_out = next_out;
      ci_ptr->zstream.avail_out = 0; /* set in the loop */

      do
      {
         if (ci_ptr->zstream.avail_in == 0)
         {
            if (read_size > *chunk_bytes)
               read_size = (uInt)*chunk_bytes;
            *chunk_bytes -= read_size;

            if (read_size > 0)
               ci_crc_read(ci_ptr, read_buffer, read_size);

            ci_ptr->zstream.next_in = read_buffer;
            ci_ptr->zstream.avail_in = read_size;
         }

         if (ci_ptr->zstream.avail_out == 0)
         {
            uInt avail = ZLIB_IO_MAX;
            if (avail > *out_size)
               avail = (uInt)*out_size;
            *out_size -= avail;

            ci_ptr->zstream.avail_out = avail;
         }

         /* Use Z_SYNC_FLUSH when there is no more chunk data to ensure that all
          * the available output is produced; this allows reading of truncated
          * streams.
          */
         ret = CI_INFLATE(ci_ptr, *chunk_bytes > 0 ?
             Z_NO_FLUSH : (finish ? Z_FINISH : Z_SYNC_FLUSH));
      }
      while (ret == Z_OK && (*out_size > 0 || ci_ptr->zstream.avail_out > 0));

      *out_size += ci_ptr->zstream.avail_out;
      ci_ptr->zstream.avail_out = 0; /* Should not be required, but is safe */

      /* Ensure the error message pointer is always set: */
      ci_zstream_error(ci_ptr, ret);
      return ret;
   }

   else
   {
      ci_ptr->zstream.msg = CIZ_MSG_CAST("zstream unclaimed");
      return Z_STREAM_ERROR;
   }
}
#endif /* READ_iCCP */

/* CHUNK HANDLING */
/* Read and check the IDHR chunk */
static ci_handle_result_code
ci_handle_IHDR(ci_structrp ci_ptr, ci_inforp info_ptr, ci_uint_32 length)
{
   ci_byte buf[13];
   ci_uint_32 width, height;
   int bit_depth, color_type, compression_type, filter_type;
   int interlace_type;

   ci_debug(1, "in ci_handle_IHDR");

   /* Length and position are checked by the caller. */

   ci_ptr->mode |= CI_HAVE_IHDR;

   ci_crc_read(ci_ptr, buf, 13);
   ci_crc_finish(ci_ptr, 0);

   width = ci_get_uint_31(ci_ptr, buf);
   height = ci_get_uint_31(ci_ptr, buf + 4);
   bit_depth = buf[8];
   color_type = buf[9];
   compression_type = buf[10];
   filter_type = buf[11];
   interlace_type = buf[12];

   /* Set internal variables */
   ci_ptr->width = width;
   ci_ptr->height = height;
   ci_ptr->bit_depth = (ci_byte)bit_depth;
   ci_ptr->interlaced = (ci_byte)interlace_type;
   ci_ptr->color_type = (ci_byte)color_type;
#ifdef CI_MNG_FEATURES_SUPPORTED
   ci_ptr->filter_type = (ci_byte)filter_type;
#endif
   ci_ptr->compression_type = (ci_byte)compression_type;

   /* Find number of channels */
   switch (ci_ptr->color_type)
   {
      default: /* invalid, ci_set_IHDR calls ci_error */
      case CI_COLOR_TYPE_GRAY:
      case CI_COLOR_TYPE_PALETTE:
         ci_ptr->channels = 1;
         break;

      case CI_COLOR_TYPE_RGB:
         ci_ptr->channels = 3;
         break;

      case CI_COLOR_TYPE_GRAY_ALPHA:
         ci_ptr->channels = 2;
         break;

      case CI_COLOR_TYPE_RGB_ALPHA:
         ci_ptr->channels = 4;
         break;
   }

   /* Set up other useful info */
   ci_ptr->pixel_depth = (ci_byte)(ci_ptr->bit_depth * ci_ptr->channels);
   ci_ptr->rowbytes = CI_ROWBYTES(ci_ptr->pixel_depth, ci_ptr->width);
   ci_debug1(3, "bit_depth = %d", ci_ptr->bit_depth);
   ci_debug1(3, "channels = %d", ci_ptr->channels);
   ci_debug1(3, "rowbytes = %lu", (unsigned long)ci_ptr->rowbytes);

   /* Rely on ci_set_IHDR to completely validate the data and call ci_error if
    * it's wrong.
    */
   ci_set_IHDR(ci_ptr, info_ptr, width, height, bit_depth,
       color_type, interlace_type, compression_type, filter_type);

   return handled_ok;
   CI_UNUSED(length)
}

/* Read and check the palette */
/* TODO: there are several obvious errors in this code when handling
 * out-of-place chunks and there is much over-complexity caused by trying to
 * patch up the problems.
 */
static ci_handle_result_code
ci_handle_PLTE(ci_structrp ci_ptr, ci_inforp info_ptr, ci_uint_32 length)
{
   ci_const_charp errmsg = NULL;

   ci_debug(1, "in ci_handle_PLTE");

   /* 1.6.47: consistency.  This used to be especially treated as a critical
    * error even in an image which is not colour mapped, there isn't a good
    * justification for treating some errors here one way and others another so
    * everything uses the same logic.
    */
   if ((ci_ptr->mode & CI_HAVE_PLTE) != 0)
      errmsg = "duplicate";

   else if ((ci_ptr->mode & CI_HAVE_IDAT) != 0)
      errmsg = "out of place";

   else if ((ci_ptr->color_type & CI_COLOR_MASK_COLOR) == 0)
      errmsg = "ignored in grayscale CI";

   else if (length > 3*CI_MAX_PALETTE_LENGTH || (length % 3) != 0)
      errmsg = "invalid";

   /* This drops PLTE in favour of tRNS or bKGD because both of those chunks
    * can have an effect on the rendering of the image whereas PLTE only matters
    * in the case of an 8-bit display with a decoder which controls the palette.
    *
    * The alternative here is to ignore the error and store the palette anyway;
    * destroying the tRNS will definately cause problems.
    *
    * NOTE: the case of CI_COLOR_TYPE_PALETTE need not be considered because
    * the ci_handle_ routines for the three 'after PLTE' chunks tRNS, bKGD and
    * hIST all check for a preceding PLTE in these cases.
    */
   else if (ci_ptr->color_type != CI_COLOR_TYPE_PALETTE &&
            (ci_has_chunk(ci_ptr, tRNS) || ci_has_chunk(ci_ptr, bKGD)))
      errmsg = "out of place";

   else
   {
      /* If the palette has 256 or fewer entries but is too large for the bit
       * depth we don't issue an error to preserve the behavior of previous
       * libci versions. We silently truncate the unused extra palette entries
       * here.
       */
      const unsigned max_palette_length =
         (ci_ptr->color_type == CI_COLOR_TYPE_PALETTE) ?
            1U << ci_ptr->bit_depth : CI_MAX_PALETTE_LENGTH;

      /* The cast is safe because 'length' is less than
       * 3*CI_MAX_PALETTE_LENGTH
       */
      const unsigned num = (length > 3U*max_palette_length) ?
         max_palette_length : (unsigned)length / 3U;

      unsigned i, j;
      ci_byte buf[3*CI_MAX_PALETTE_LENGTH];
      ci_color palette[CI_MAX_PALETTE_LENGTH];

      /* Read the chunk into the buffer then read to the end of the chunk. */
      ci_crc_read(ci_ptr, buf, num*3U);
      ci_crc_finish_critical(ci_ptr, length - 3U*num,
            /* Handle as ancillary if PLTE is optional: */
            ci_ptr->color_type != CI_COLOR_TYPE_PALETTE);

      for (i = 0U, j = 0U; i < num; i++)
      {
         palette[i].red = buf[j++];
         palette[i].green = buf[j++];
         palette[i].blue = buf[j++];
      }

      /* A valid PLTE chunk has been read */
      ci_ptr->mode |= CI_HAVE_PLTE;

      /* TODO: ci_set_PLTE has the side effect of setting ci_ptr->palette to
       * its own copy of the palette.  This has the side effect that when
       * ci_start_row is called (this happens after any call to
       * ci_read_update_info) the info_ptr palette gets changed.  This is
       * extremely unexpected and confusing.
       *
       * REVIEW: there have been consistent bugs in the past about gamma and
       * similar transforms to colour mapped images being useless because the
       * modified palette cannot be accessed because of the above.
       *
       * CONSIDER: Fix this by not sharing the palette in this way.  But does
       * this completely fix the problem?
       */
      ci_set_PLTE(ci_ptr, info_ptr, palette, num);
      return handled_ok;
   }

   /* Here on error: errmsg is non NULL. */
   if (ci_ptr->color_type == CI_COLOR_TYPE_PALETTE)
   {
      ci_crc_finish(ci_ptr, length);
      ci_chunk_error(ci_ptr, errmsg);
   }

   else /* not critical to this image */
   {
      ci_crc_finish_critical(ci_ptr, length, 1/*handle as ancillary*/);
      ci_chunk_benign_error(ci_ptr, errmsg);
   }

   /* Because CI_UNUSED(errmsg) does not work if all the uses are compiled out
    * (this does happen).
    */
   return errmsg != NULL ? handled_error : handled_error;
}

/* On read the IDAT chunk is always handled specially, even if marked for
 * unknown handling (this is allowed), so:
 */
#define ci_handle_IDAT NULL

static ci_handle_result_code
ci_handle_IEND(ci_structrp ci_ptr, ci_inforp info_ptr, ci_uint_32 length)
{
   ci_debug(1, "in ci_handle_IEND");

   ci_ptr->mode |= (CI_AFTER_IDAT | CI_HAVE_IEND);

   if (length != 0)
      ci_chunk_benign_error(ci_ptr, "invalid");

   ci_crc_finish_critical(ci_ptr, length, 1/*handle as ancillary*/);

   return handled_ok;
   CI_UNUSED(info_ptr)
}

#ifdef CI_READ_gAMA_SUPPORTED
static ci_handle_result_code
ci_handle_gAMA(ci_structrp ci_ptr, ci_inforp info_ptr, ci_uint_32 length)
{
   ci_uint_32 ugamma;
   ci_byte buf[4];

   ci_debug(1, "in ci_handle_gAMA");

   ci_crc_read(ci_ptr, buf, 4);

   if (ci_crc_finish(ci_ptr, 0) != 0)
      return handled_error;

   ugamma = ci_get_uint_32(buf);

   if (ugamma > CI_UINT_31_MAX)
   {
      ci_chunk_benign_error(ci_ptr, "invalid");
      return handled_error;
   }

   ci_set_gAMA_fixed(ci_ptr, info_ptr, (ci_fixed_point)/*SAFE*/ugamma);

#ifdef CI_READ_GAMMA_SUPPORTED
      /* CIv3: chunk precedence for gamma is cICP, [iCCP], sRGB, gAMA.  gAMA is
       * at the end of the chain so simply check for an unset value.
       */
      if (ci_ptr->chunk_gamma == 0)
         ci_ptr->chunk_gamma = (ci_fixed_point)/*SAFE*/ugamma;
#endif /*READ_GAMMA*/

   return handled_ok;
   CI_UNUSED(length)
}
#else
#  define ci_handle_gAMA NULL
#endif

#ifdef CI_READ_sBIT_SUPPORTED
static ci_handle_result_code /* PRIVATE */
ci_handle_sBIT(ci_structrp ci_ptr, ci_inforp info_ptr, ci_uint_32 length)
{
   unsigned int truelen, i;
   ci_byte sample_depth;
   ci_byte buf[4];

   ci_debug(1, "in ci_handle_sBIT");

   if (ci_ptr->color_type == CI_COLOR_TYPE_PALETTE)
   {
      truelen = 3;
      sample_depth = 8;
   }

   else
   {
      truelen = ci_ptr->channels;
      sample_depth = ci_ptr->bit_depth;
   }

   if (length != truelen)
   {
      ci_crc_finish(ci_ptr, length);
      ci_chunk_benign_error(ci_ptr, "bad length");
      return handled_error;
   }

   buf[0] = buf[1] = buf[2] = buf[3] = sample_depth;
   ci_crc_read(ci_ptr, buf, truelen);

   if (ci_crc_finish(ci_ptr, 0) != 0)
      return handled_error;

   for (i=0; i<truelen; ++i)
   {
      if (buf[i] == 0 || buf[i] > sample_depth)
      {
         ci_chunk_benign_error(ci_ptr, "invalid");
         return handled_error;
      }
   }

   if ((ci_ptr->color_type & CI_COLOR_MASK_COLOR) != 0)
   {
      ci_ptr->sig_bit.red = buf[0];
      ci_ptr->sig_bit.green = buf[1];
      ci_ptr->sig_bit.blue = buf[2];
      ci_ptr->sig_bit.alpha = buf[3];
   }

   else /* grayscale */
   {
      ci_ptr->sig_bit.gray = buf[0];
      ci_ptr->sig_bit.red = buf[0];
      ci_ptr->sig_bit.green = buf[0];
      ci_ptr->sig_bit.blue = buf[0];
      ci_ptr->sig_bit.alpha = buf[1];
   }

   ci_set_sBIT(ci_ptr, info_ptr, &(ci_ptr->sig_bit));
   return handled_ok;
}
#else
#  define ci_handle_sBIT NULL
#endif

#ifdef CI_READ_cHRM_SUPPORTED
static ci_int_32
ci_get_int_32_checked(ci_const_bytep buf, int *error)
{
   ci_uint_32 uval = ci_get_uint_32(buf);
   if ((uval & 0x80000000) == 0) /* non-negative */
      return (ci_int_32)uval;

   uval = (uval ^ 0xffffffff) + 1;  /* 2's complement: -x = ~x+1 */
   if ((uval & 0x80000000) == 0) /* no overflow */
      return -(ci_int_32)uval;

   /* This version of ci_get_int_32 has a way of returning the error to the
    * caller, so:
    */
   *error = 1;
   return 0; /* Safe */
}

static ci_handle_result_code /* PRIVATE */
ci_handle_cHRM(ci_structrp ci_ptr, ci_inforp info_ptr, ci_uint_32 length)
{
   int error = 0;
   ci_xy xy;
   ci_byte buf[32];

   ci_debug(1, "in ci_handle_cHRM");

   ci_crc_read(ci_ptr, buf, 32);

   if (ci_crc_finish(ci_ptr, 0) != 0)
      return handled_error;

   xy.whitex = ci_get_int_32_checked(buf +  0, &error);
   xy.whitey = ci_get_int_32_checked(buf +  4, &error);
   xy.redx   = ci_get_int_32_checked(buf +  8, &error);
   xy.redy   = ci_get_int_32_checked(buf + 12, &error);
   xy.greenx = ci_get_int_32_checked(buf + 16, &error);
   xy.greeny = ci_get_int_32_checked(buf + 20, &error);
   xy.bluex  = ci_get_int_32_checked(buf + 24, &error);
   xy.bluey  = ci_get_int_32_checked(buf + 28, &error);

   if (error)
   {
      ci_chunk_benign_error(ci_ptr, "invalid");
      return handled_error;
   }

   /* ci_set_cHRM may complain about some of the values but this doesn't matter
    * because it was a cHRM and it did have vaguely (if, perhaps, ridiculous)
    * values.  Ridiculousity will be checked if the values are used later.
    */
   ci_set_cHRM_fixed(ci_ptr, info_ptr, xy.whitex, xy.whitey, xy.redx, xy.redy,
         xy.greenx, xy.greeny, xy.bluex, xy.bluey);

   /* We only use 'chromaticities' for RGB to gray */
#  ifdef CI_READ_RGB_TO_GRAY_SUPPORTED
      /* There is no need to check sRGB here, cICP is NYI and iCCP is not
       * supported so just check mDCV.
       */
      if (!ci_has_chunk(ci_ptr, mDCV))
      {
         ci_ptr->chromaticities = xy;
      }
#  endif /* READ_RGB_TO_GRAY */

   return handled_ok;
   CI_UNUSED(length)
}
#else
#  define ci_handle_cHRM NULL
#endif

#ifdef CI_READ_sRGB_SUPPORTED
static ci_handle_result_code /* PRIVATE */
ci_handle_sRGB(ci_structrp ci_ptr, ci_inforp info_ptr, ci_uint_32 length)
{
   ci_byte intent;

   ci_debug(1, "in ci_handle_sRGB");

   ci_crc_read(ci_ptr, &intent, 1);

   if (ci_crc_finish(ci_ptr, 0) != 0)
      return handled_error;

   /* This checks the range of the "rendering intent" because it is specified in
    * the CI spec itself; the "reserved" values will result in the chunk not
    * being accepted, just as they do with the various "reserved" values in
    * IHDR.
    */
   if (intent > 3/*CIv3 spec*/)
   {
      ci_chunk_benign_error(ci_ptr, "invalid");
      return handled_error;
   }

   ci_set_sRGB(ci_ptr, info_ptr, intent);
   /* NOTE: ci_struct::chromaticities is not set here because the RGB to gray
    * coefficients are known without a need for the chromaticities.
    */

#ifdef CI_READ_GAMMA_SUPPORTED
      /* CIv3: chunk precedence for gamma is cICP, [iCCP], sRGB, gAMA.  iCCP is
       * not supported by libci so the only requirement is to check for cICP
       * setting the gamma (this is NYI, but this check is safe.)
       */
      if (!ci_has_chunk(ci_ptr, cICP) || ci_ptr->chunk_gamma == 0)
         ci_ptr->chunk_gamma = CI_GAMMA_sRGB_INVERSE;
#endif /*READ_GAMMA*/

   return handled_ok;
   CI_UNUSED(length)
}
#else
#  define ci_handle_sRGB NULL
#endif /* READ_sRGB */

#ifdef CI_READ_iCCP_SUPPORTED
static ci_handle_result_code /* PRIVATE */
ci_handle_iCCP(ci_structrp ci_ptr, ci_inforp info_ptr, ci_uint_32 length)
/* Note: this does not properly handle profiles that are > 64K under DOS */
{
   ci_const_charp errmsg = NULL; /* error message output, or no error */
   int finished = 0; /* crc checked */

   ci_debug(1, "in ci_handle_iCCP");

   /* CIv3: allow CI files with both sRGB and iCCP because the CI spec only
    * ever said that there "should" be only one, not "shall" and the CIv3
    * colour chunk precedence rules give a handling for this case anyway.
    */
   {
      uInt read_length, keyword_length;
      char keyword[81];

      /* Find the keyword; the keyword plus separator and compression method
       * bytes can be at most 81 characters long.
       */
      read_length = 81; /* maximum */
      if (read_length > length)
         read_length = (uInt)/*SAFE*/length;

      ci_crc_read(ci_ptr, (ci_bytep)keyword, read_length);
      length -= read_length;

      if (length < LZ77Min)
      {
         ci_crc_finish(ci_ptr, length);
         ci_chunk_benign_error(ci_ptr, "too short");
         return handled_error;
      }

      keyword_length = 0;
      while (keyword_length < 80 && keyword_length < read_length &&
         keyword[keyword_length] != 0)
         ++keyword_length;

      /* TODO: make the keyword checking common */
      if (keyword_length >= 1 && keyword_length <= 79)
      {
         /* We only understand '0' compression - deflate - so if we get a
          * different value we can't safely decode the chunk.
          */
         if (keyword_length+1 < read_length &&
            keyword[keyword_length+1] == CI_COMPRESSION_TYPE_BASE)
         {
            read_length -= keyword_length+2;

            if (ci_inflate_claim(ci_ptr, ci_iCCP) == Z_OK)
            {
               Byte profile_header[132]={0};
               Byte local_buffer[CI_INFLATE_BUF_SIZE];
               ci_alloc_size_t size = (sizeof profile_header);

               ci_ptr->zstream.next_in = (Bytef*)keyword + (keyword_length+2);
               ci_ptr->zstream.avail_in = read_length;
               (void)ci_inflate_read(ci_ptr, local_buffer,
                   (sizeof local_buffer), &length, profile_header, &size,
                   0/*finish: don't, because the output is too small*/);

               if (size == 0)
               {
                  /* We have the ICC profile header; do the basic header checks.
                   */
                  ci_uint_32 profile_length = ci_get_uint_32(profile_header);

                  if (ci_icc_check_length(ci_ptr, keyword, profile_length) !=
                      0)
                  {
                     /* The length is apparently ok, so we can check the 132
                      * byte header.
                      */
                     if (ci_icc_check_header(ci_ptr, keyword, profile_length,
                              profile_header, ci_ptr->color_type) != 0)
                     {
                        /* Now read the tag table; a variable size buffer is
                         * needed at this point, allocate one for the whole
                         * profile.  The header check has already validated
                         * that none of this stuff will overflow.
                         */
                        ci_uint_32 tag_count =
                           ci_get_uint_32(profile_header + 128);
                        ci_bytep profile = ci_read_buffer(ci_ptr,
                              profile_length);

                        if (profile != NULL)
                        {
                           memcpy(profile, profile_header,
                               (sizeof profile_header));

                           size = 12 * tag_count;

                           (void)ci_inflate_read(ci_ptr, local_buffer,
                               (sizeof local_buffer), &length,
                               profile + (sizeof profile_header), &size, 0);

                           /* Still expect a buffer error because we expect
                            * there to be some tag data!
                            */
                           if (size == 0)
                           {
                              if (ci_icc_check_tag_table(ci_ptr,
                                       keyword, profile_length, profile) != 0)
                              {
                                 /* The profile has been validated for basic
                                  * security issues, so read the whole thing in.
                                  */
                                 size = profile_length - (sizeof profile_header)
                                     - 12 * tag_count;

                                 (void)ci_inflate_read(ci_ptr, local_buffer,
                                     (sizeof local_buffer), &length,
                                     profile + (sizeof profile_header) +
                                     12 * tag_count, &size, 1/*finish*/);

                                 if (length > 0 && !(ci_ptr->flags &
                                     CI_FLAG_BENIGN_ERRORS_WARN))
                                    errmsg = "extra compressed data";

                                 /* But otherwise allow extra data: */
                                 else if (size == 0)
                                 {
                                    if (length > 0)
                                    {
                                       /* This can be handled completely, so
                                        * keep going.
                                        */
                                       ci_chunk_warning(ci_ptr,
                                           "extra compressed data");
                                    }

                                    ci_crc_finish(ci_ptr, length);
                                    finished = 1;

                                    /* Steal the profile for info_ptr. */
                                    if (info_ptr != NULL)
                                    {
                                       ci_free_data(ci_ptr, info_ptr,
                                           CI_FREE_ICCP, 0);

                                       info_ptr->iccp_name = ci_voidcast(char*,
                                           ci_malloc_base(ci_ptr,
                                           keyword_length+1));
                                       if (info_ptr->iccp_name != NULL)
                                       {
                                          memcpy(info_ptr->iccp_name, keyword,
                                              keyword_length+1);
                                          info_ptr->iccp_proflen =
                                              profile_length;
                                          info_ptr->iccp_profile = profile;
                                          ci_ptr->read_buffer = NULL; /*steal*/
                                          info_ptr->free_me |= CI_FREE_ICCP;
                                          info_ptr->valid |= CI_INFO_iCCP;
                                       }

                                       else
                                          errmsg = "out of memory";
                                    }

                                    /* else the profile remains in the read
                                     * buffer which gets reused for subsequent
                                     * chunks.
                                     */

                                    if (errmsg == NULL)
                                    {
                                       ci_ptr->zowner = 0;
                                       return handled_ok;
                                    }
                                 }
                                 if (errmsg == NULL)
                                    errmsg = ci_ptr->zstream.msg;
                              }
                              /* else ci_icc_check_tag_table output an error */
                           }
                           else /* profile truncated */
                              errmsg = ci_ptr->zstream.msg;
                        }

                        else
                           errmsg = "out of memory";
                     }

                     /* else ci_icc_check_header output an error */
                  }

                  /* else ci_icc_check_length output an error */
               }

               else /* profile truncated */
                  errmsg = ci_ptr->zstream.msg;

               /* Release the stream */
               ci_ptr->zowner = 0;
            }

            else /* ci_inflate_claim failed */
               errmsg = ci_ptr->zstream.msg;
         }

         else
            errmsg = "bad compression method"; /* or missing */
      }

      else
         errmsg = "bad keyword";
   }

   /* Failure: the reason is in 'errmsg' */
   if (finished == 0)
      ci_crc_finish(ci_ptr, length);

   if (errmsg != NULL) /* else already output */
      ci_chunk_benign_error(ci_ptr, errmsg);

   return handled_error;
}
#else
#  define ci_handle_iCCP NULL
#endif /* READ_iCCP */

#ifdef CI_READ_sPLT_SUPPORTED
static ci_handle_result_code /* PRIVATE */
ci_handle_sPLT(ci_structrp ci_ptr, ci_inforp info_ptr, ci_uint_32 length)
/* Note: this does not properly handle chunks that are > 64K under DOS */
{
   ci_bytep entry_start, buffer;
   ci_sPLT_t new_palette;
   ci_sPLT_entryp pp;
   ci_uint_32 data_length;
   int entry_size, i;
   ci_uint_32 skip = 0;
   ci_uint_32 dl;
   size_t max_dl;

   ci_debug(1, "in ci_handle_sPLT");

#ifdef CI_USER_LIMITS_SUPPORTED
   if (ci_ptr->user_chunk_cache_max != 0)
   {
      if (ci_ptr->user_chunk_cache_max == 1)
      {
         ci_crc_finish(ci_ptr, length);
         return handled_error;
      }

      if (--ci_ptr->user_chunk_cache_max == 1)
      {
         ci_warning(ci_ptr, "No space in chunk cache for sPLT");
         ci_crc_finish(ci_ptr, length);
         return handled_error;
      }
   }
#endif

   buffer = ci_read_buffer(ci_ptr, length+1);
   if (buffer == NULL)
   {
      ci_crc_finish(ci_ptr, length);
      ci_chunk_benign_error(ci_ptr, "out of memory");
      return handled_error;
   }


   /* WARNING: this may break if size_t is less than 32 bits; it is assumed
    * that the CI_MAX_MALLOC_64K test is enabled in this case, but this is a
    * potential breakage point if the types in ciconf.h aren't exactly right.
    */
   ci_crc_read(ci_ptr, buffer, length);

   if (ci_crc_finish(ci_ptr, skip) != 0)
      return handled_error;

   buffer[length] = 0;

   for (entry_start = buffer; *entry_start; entry_start++)
      /* Empty loop to find end of name */ ;

   ++entry_start;

   /* A sample depth should follow the separator, and we should be on it  */
   if (length < 2U || entry_start > buffer + (length - 2U))
   {
      ci_warning(ci_ptr, "malformed sPLT chunk");
      return handled_error;
   }

   new_palette.depth = *entry_start++;
   entry_size = (new_palette.depth == 8 ? 6 : 10);
   /* This must fit in a ci_uint_32 because it is derived from the original
    * chunk data length.
    */
   data_length = length - (ci_uint_32)(entry_start - buffer);

   /* Integrity-check the data length */
   if ((data_length % (unsigned int)entry_size) != 0)
   {
      ci_warning(ci_ptr, "sPLT chunk has bad length");
      return handled_error;
   }

   dl = (ci_uint_32)(data_length / (unsigned int)entry_size);
   max_dl = CI_SIZE_MAX / (sizeof (ci_sPLT_entry));

   if (dl > max_dl)
   {
      ci_warning(ci_ptr, "sPLT chunk too long");
      return handled_error;
   }

   new_palette.nentries = (ci_int_32)(data_length / (unsigned int)entry_size);

   new_palette.entries = (ci_sPLT_entryp)ci_malloc_warn(ci_ptr,
       (ci_alloc_size_t) new_palette.nentries * (sizeof (ci_sPLT_entry)));

   if (new_palette.entries == NULL)
   {
      ci_warning(ci_ptr, "sPLT chunk requires too much memory");
      return handled_error;
   }

   for (i = 0; i < new_palette.nentries; i++)
   {
      pp = new_palette.entries + i;

      if (new_palette.depth == 8)
      {
         pp->red = *entry_start++;
         pp->green = *entry_start++;
         pp->blue = *entry_start++;
         pp->alpha = *entry_start++;
      }

      else
      {
         pp->red   = ci_get_uint_16(entry_start); entry_start += 2;
         pp->green = ci_get_uint_16(entry_start); entry_start += 2;
         pp->blue  = ci_get_uint_16(entry_start); entry_start += 2;
         pp->alpha = ci_get_uint_16(entry_start); entry_start += 2;
      }

      pp->frequency = ci_get_uint_16(entry_start); entry_start += 2;
   }

   /* Discard all chunk data except the name and stash that */
   new_palette.name = (ci_charp)buffer;

   ci_set_sPLT(ci_ptr, info_ptr, &new_palette, 1);

   ci_free(ci_ptr, new_palette.entries);
   return handled_ok;
}
#else
#  define ci_handle_sPLT NULL
#endif /* READ_sPLT */

#ifdef CI_READ_tRNS_SUPPORTED
static ci_handle_result_code /* PRIVATE */
ci_handle_tRNS(ci_structrp ci_ptr, ci_inforp info_ptr, ci_uint_32 length)
{
   ci_byte readbuf[CI_MAX_PALETTE_LENGTH];

   ci_debug(1, "in ci_handle_tRNS");

   if (ci_ptr->color_type == CI_COLOR_TYPE_GRAY)
   {
      ci_byte buf[2];

      if (length != 2)
      {
         ci_crc_finish(ci_ptr, length);
         ci_chunk_benign_error(ci_ptr, "invalid");
         return handled_error;
      }

      ci_crc_read(ci_ptr, buf, 2);
      ci_ptr->num_trans = 1;
      ci_ptr->trans_color.gray = ci_get_uint_16(buf);
   }

   else if (ci_ptr->color_type == CI_COLOR_TYPE_RGB)
   {
      ci_byte buf[6];

      if (length != 6)
      {
         ci_crc_finish(ci_ptr, length);
         ci_chunk_benign_error(ci_ptr, "invalid");
         return handled_error;
      }

      ci_crc_read(ci_ptr, buf, length);
      ci_ptr->num_trans = 1;
      ci_ptr->trans_color.red = ci_get_uint_16(buf);
      ci_ptr->trans_color.green = ci_get_uint_16(buf + 2);
      ci_ptr->trans_color.blue = ci_get_uint_16(buf + 4);
   }

   else if (ci_ptr->color_type == CI_COLOR_TYPE_PALETTE)
   {
      if ((ci_ptr->mode & CI_HAVE_PLTE) == 0)
      {
         ci_crc_finish(ci_ptr, length);
         ci_chunk_benign_error(ci_ptr, "out of place");
         return handled_error;
      }

      if (length > (unsigned int) ci_ptr->num_palette ||
         length > (unsigned int) CI_MAX_PALETTE_LENGTH ||
         length == 0)
      {
         ci_crc_finish(ci_ptr, length);
         ci_chunk_benign_error(ci_ptr, "invalid");
         return handled_error;
      }

      ci_crc_read(ci_ptr, readbuf, length);
      ci_ptr->num_trans = (ci_uint_16)length;
   }

   else
   {
      ci_crc_finish(ci_ptr, length);
      ci_chunk_benign_error(ci_ptr, "invalid with alpha channel");
      return handled_error;
   }

   if (ci_crc_finish(ci_ptr, 0) != 0)
   {
      ci_ptr->num_trans = 0;
      return handled_error;
   }

   /* TODO: this is a horrible side effect in the palette case because the
    * ci_struct ends up with a pointer to the tRNS buffer owned by the
    * ci_info.  Fix this.
    */
   ci_set_tRNS(ci_ptr, info_ptr, readbuf, ci_ptr->num_trans,
       &(ci_ptr->trans_color));
   return handled_ok;
}
#else
#  define ci_handle_tRNS NULL
#endif

#ifdef CI_READ_bKGD_SUPPORTED
static ci_handle_result_code /* PRIVATE */
ci_handle_bKGD(ci_structrp ci_ptr, ci_inforp info_ptr, ci_uint_32 length)
{
   unsigned int truelen;
   ci_byte buf[6];
   ci_color_16 background;

   ci_debug(1, "in ci_handle_bKGD");

   if (ci_ptr->color_type == CI_COLOR_TYPE_PALETTE)
   {
      if ((ci_ptr->mode & CI_HAVE_PLTE) == 0)
      {
         ci_crc_finish(ci_ptr, length);
         ci_chunk_benign_error(ci_ptr, "out of place");
         return handled_error;
      }

      truelen = 1;
   }

   else if ((ci_ptr->color_type & CI_COLOR_MASK_COLOR) != 0)
      truelen = 6;

   else
      truelen = 2;

   if (length != truelen)
   {
      ci_crc_finish(ci_ptr, length);
      ci_chunk_benign_error(ci_ptr, "invalid");
      return handled_error;
   }

   ci_crc_read(ci_ptr, buf, truelen);

   if (ci_crc_finish(ci_ptr, 0) != 0)
      return handled_error;

   /* We convert the index value into RGB components so that we can allow
    * arbitrary RGB values for background when we have transparency, and
    * so it is easy to determine the RGB values of the background color
    * from the info_ptr struct.
    */
   if (ci_ptr->color_type == CI_COLOR_TYPE_PALETTE)
   {
      background.index = buf[0];

      if (info_ptr != NULL && info_ptr->num_palette != 0)
      {
         if (buf[0] >= info_ptr->num_palette)
         {
            ci_chunk_benign_error(ci_ptr, "invalid index");
            return handled_error;
         }

         background.red = (ci_uint_16)ci_ptr->palette[buf[0]].red;
         background.green = (ci_uint_16)ci_ptr->palette[buf[0]].green;
         background.blue = (ci_uint_16)ci_ptr->palette[buf[0]].blue;
      }

      else
         background.red = background.green = background.blue = 0;

      background.gray = 0;
   }

   else if ((ci_ptr->color_type & CI_COLOR_MASK_COLOR) == 0) /* GRAY */
   {
      if (ci_ptr->bit_depth <= 8)
      {
         if (buf[0] != 0 || buf[1] >= (unsigned int)(1 << ci_ptr->bit_depth))
         {
            ci_chunk_benign_error(ci_ptr, "invalid gray level");
            return handled_error;
         }
      }

      background.index = 0;
      background.red =
      background.green =
      background.blue =
      background.gray = ci_get_uint_16(buf);
   }

   else
   {
      if (ci_ptr->bit_depth <= 8)
      {
         if (buf[0] != 0 || buf[2] != 0 || buf[4] != 0)
         {
            ci_chunk_benign_error(ci_ptr, "invalid color");
            return handled_error;
         }
      }

      background.index = 0;
      background.red = ci_get_uint_16(buf);
      background.green = ci_get_uint_16(buf + 2);
      background.blue = ci_get_uint_16(buf + 4);
      background.gray = 0;
   }

   ci_set_bKGD(ci_ptr, info_ptr, &background);
   return handled_ok;
}
#else
#  define ci_handle_bKGD NULL
#endif

#ifdef CI_READ_cICP_SUPPORTED
static ci_handle_result_code /* PRIVATE */
ci_handle_cICP(ci_structrp ci_ptr, ci_inforp info_ptr, ci_uint_32 length)
{
   ci_byte buf[4];

   ci_debug(1, "in ci_handle_cICP");

   ci_crc_read(ci_ptr, buf, 4);

   if (ci_crc_finish(ci_ptr, 0) != 0)
      return handled_error;

   ci_set_cICP(ci_ptr, info_ptr, buf[0], buf[1],  buf[2], buf[3]);

   /* We only use 'chromaticities' for RGB to gray */
#  ifdef CI_READ_RGB_TO_GRAY_SUPPORTED
      if (!ci_has_chunk(ci_ptr, mDCV))
      {
         /* TODO: ci_ptr->chromaticities = chromaticities; */
      }
#  endif /* READ_RGB_TO_GRAY */

#ifdef CI_READ_GAMMA_SUPPORTED
      /* CIv3: chunk precedence for gamma is cICP, [iCCP], sRGB, gAMA.  cICP is
       * at the head so simply set the gamma if it can be determined.  If not
       * chunk_gamma remains unchanged; sRGB and gAMA handling check it for
       * being zero.
       */
      /* TODO: set ci_struct::chunk_gamma when possible */
#endif /*READ_GAMMA*/

   return handled_ok;
   CI_UNUSED(length)
}
#else
#  define ci_handle_cICP NULL
#endif

#ifdef CI_READ_cLLI_SUPPORTED
static ci_handle_result_code /* PRIVATE */
ci_handle_cLLI(ci_structrp ci_ptr, ci_inforp info_ptr, ci_uint_32 length)
{
   ci_byte buf[8];

   ci_debug(1, "in ci_handle_cLLI");

   ci_crc_read(ci_ptr, buf, 8);

   if (ci_crc_finish(ci_ptr, 0) != 0)
      return handled_error;

   /* The error checking happens here, this puts it in just one place: */
   ci_set_cLLI_fixed(ci_ptr, info_ptr, ci_get_uint_32(buf),
         ci_get_uint_32(buf+4));
   return handled_ok;
   CI_UNUSED(length)
}
#else
#  define ci_handle_cLLI NULL
#endif

#ifdef CI_READ_mDCV_SUPPORTED
static ci_handle_result_code /* PRIVATE */
ci_handle_mDCV(ci_structrp ci_ptr, ci_inforp info_ptr, ci_uint_32 length)
{
   ci_xy chromaticities;
   ci_byte buf[24];

   ci_debug(1, "in ci_handle_mDCV");

   ci_crc_read(ci_ptr, buf, 24);

   if (ci_crc_finish(ci_ptr, 0) != 0)
      return handled_error;

   /* The error checking happens here, this puts it in just one place.  The
    * odd /50000 scaling factor makes it more difficult but the (x.y) values are
    * only two bytes so a <<1 is safe.
    *
    * WARNING: the CI specification defines the cHRM chunk to **start** with
    * the white point (x,y).  The W3C CI v3 specification puts the white point
    * **after* R,G,B.  The x,y values in mDCV are also scaled by 50,000 and
    * stored in just two bytes, whereas those in cHRM are scaled by 100,000 and
    * stored in four bytes.  This is very, very confusing.  These APIs remove
    * the confusion by copying the existing, well established, API.
    */
   chromaticities.redx   = ci_get_uint_16(buf+ 0U) << 1; /* red x */
   chromaticities.redy   = ci_get_uint_16(buf+ 2U) << 1; /* red y */
   chromaticities.greenx = ci_get_uint_16(buf+ 4U) << 1; /* green x */
   chromaticities.greeny = ci_get_uint_16(buf+ 6U) << 1; /* green y */
   chromaticities.bluex  = ci_get_uint_16(buf+ 8U) << 1; /* blue x */
   chromaticities.bluey  = ci_get_uint_16(buf+10U) << 1; /* blue y */
   chromaticities.whitex = ci_get_uint_16(buf+12U) << 1; /* white x */
   chromaticities.whitey = ci_get_uint_16(buf+14U) << 1; /* white y */

   ci_set_mDCV_fixed(ci_ptr, info_ptr,
         chromaticities.whitex, chromaticities.whitey,
         chromaticities.redx, chromaticities.redy,
         chromaticities.greenx, chromaticities.greeny,
         chromaticities.bluex, chromaticities.bluey,
         ci_get_uint_32(buf+16U), /* peak luminance */
         ci_get_uint_32(buf+20U));/* minimum perceivable luminance */

   /* We only use 'chromaticities' for RGB to gray */
#  ifdef CI_READ_RGB_TO_GRAY_SUPPORTED
      ci_ptr->chromaticities = chromaticities;
#  endif /* READ_RGB_TO_GRAY */

   return handled_ok;
   CI_UNUSED(length)
}
#else
#  define ci_handle_mDCV NULL
#endif

#ifdef CI_READ_eXIf_SUPPORTED
static ci_handle_result_code /* PRIVATE */
ci_handle_eXIf(ci_structrp ci_ptr, ci_inforp info_ptr, ci_uint_32 length)
{
   ci_bytep buffer = NULL;

   ci_debug(1, "in ci_handle_eXIf");

   buffer = ci_read_buffer(ci_ptr, length);

   if (buffer == NULL)
   {
      ci_crc_finish(ci_ptr, length);
      ci_chunk_benign_error(ci_ptr, "out of memory");
      return handled_error;
   }

   ci_crc_read(ci_ptr, buffer, length);

   if (ci_crc_finish(ci_ptr, 0) != 0)
      return handled_error;

   /* CIv3: the code used to check the byte order mark at the start for MM or
    * II, however CIv3 states that the the first 4 bytes should be checked.
    * The caller ensures that there are four bytes available.
    */
   {
      ci_uint_32 header = ci_get_uint_32(buffer);

      /* These numbers are copied from the CIv3 spec: */
      if (header != 0x49492A00 && header != 0x4D4D002A)
      {
         ci_chunk_benign_error(ci_ptr, "invalid");
         return handled_error;
      }
   }

   ci_set_eXIf_1(ci_ptr, info_ptr, length, buffer);
   return handled_ok;
}
#else
#  define ci_handle_eXIf NULL
#endif

#ifdef CI_READ_hIST_SUPPORTED
static ci_handle_result_code /* PRIVATE */
ci_handle_hIST(ci_structrp ci_ptr, ci_inforp info_ptr, ci_uint_32 length)
{
   unsigned int num, i;
   ci_uint_16 readbuf[CI_MAX_PALETTE_LENGTH];

   ci_debug(1, "in ci_handle_hIST");

   /* This cast is safe because the chunk definition limits the length to a
    * maximum of 1024 bytes.
    *
    * TODO: maybe use ci_uint_32 anyway, not unsigned int, to reduce the
    * casts.
    */
   num = (unsigned int)length / 2 ;

   if (length != num * 2 ||
       num != (unsigned int)ci_ptr->num_palette ||
       num > (unsigned int)CI_MAX_PALETTE_LENGTH)
   {
      ci_crc_finish(ci_ptr, length);
      ci_chunk_benign_error(ci_ptr, "invalid");
      return handled_error;
   }

   for (i = 0; i < num; i++)
   {
      ci_byte buf[2];

      ci_crc_read(ci_ptr, buf, 2);
      readbuf[i] = ci_get_uint_16(buf);
   }

   if (ci_crc_finish(ci_ptr, 0) != 0)
      return handled_error;

   ci_set_hIST(ci_ptr, info_ptr, readbuf);
   return handled_ok;
}
#else
#  define ci_handle_hIST NULL
#endif

#ifdef CI_READ_pHYs_SUPPORTED
static ci_handle_result_code /* PRIVATE */
ci_handle_pHYs(ci_structrp ci_ptr, ci_inforp info_ptr, ci_uint_32 length)
{
   ci_byte buf[9];
   ci_uint_32 res_x, res_y;
   int unit_type;

   ci_debug(1, "in ci_handle_pHYs");

   ci_crc_read(ci_ptr, buf, 9);

   if (ci_crc_finish(ci_ptr, 0) != 0)
      return handled_error;

   res_x = ci_get_uint_32(buf);
   res_y = ci_get_uint_32(buf + 4);
   unit_type = buf[8];
   ci_set_pHYs(ci_ptr, info_ptr, res_x, res_y, unit_type);
   return handled_ok;
   CI_UNUSED(length)
}
#else
#  define ci_handle_pHYs NULL
#endif

#ifdef CI_READ_oFFs_SUPPORTED
static ci_handle_result_code /* PRIVATE */
ci_handle_oFFs(ci_structrp ci_ptr, ci_inforp info_ptr, ci_uint_32 length)
{
   ci_byte buf[9];
   ci_int_32 offset_x, offset_y;
   int unit_type;

   ci_debug(1, "in ci_handle_oFFs");

   ci_crc_read(ci_ptr, buf, 9);

   if (ci_crc_finish(ci_ptr, 0) != 0)
      return handled_error;

   offset_x = ci_get_int_32(buf);
   offset_y = ci_get_int_32(buf + 4);
   unit_type = buf[8];
   ci_set_oFFs(ci_ptr, info_ptr, offset_x, offset_y, unit_type);
   return handled_ok;
   CI_UNUSED(length)
}
#else
#  define ci_handle_oFFs NULL
#endif

#ifdef CI_READ_pCAL_SUPPORTED
/* Read the pCAL chunk (described in the CI Extensions document) */
static ci_handle_result_code /* PRIVATE */
ci_handle_pCAL(ci_structrp ci_ptr, ci_inforp info_ptr, ci_uint_32 length)
{
   ci_int_32 X0, X1;
   ci_byte type, nparams;
   ci_bytep buffer, buf, units, endptr;
   ci_charpp params;
   int i;

   ci_debug(1, "in ci_handle_pCAL");
   ci_debug1(2, "Allocating and reading pCAL chunk data (%u bytes)",
       length + 1);

   buffer = ci_read_buffer(ci_ptr, length+1);

   if (buffer == NULL)
   {
      ci_crc_finish(ci_ptr, length);
      ci_chunk_benign_error(ci_ptr, "out of memory");
      return handled_error;
   }

   ci_crc_read(ci_ptr, buffer, length);

   if (ci_crc_finish(ci_ptr, 0) != 0)
      return handled_error;

   buffer[length] = 0; /* Null terminate the last string */

   ci_debug(3, "Finding end of pCAL purpose string");
   for (buf = buffer; *buf; buf++)
      /* Empty loop */ ;

   endptr = buffer + length;

   /* We need to have at least 12 bytes after the purpose string
    * in order to get the parameter information.
    */
   if (endptr - buf <= 12)
   {
      ci_chunk_benign_error(ci_ptr, "invalid");
      return handled_error;
   }

   ci_debug(3, "Reading pCAL X0, X1, type, nparams, and units");
   X0 = ci_get_int_32((ci_bytep)buf+1);
   X1 = ci_get_int_32((ci_bytep)buf+5);
   type = buf[9];
   nparams = buf[10];
   units = buf + 11;

   ci_debug(3, "Checking pCAL equation type and number of parameters");
   /* Check that we have the right number of parameters for known
    * equation types.
    */
   if ((type == CI_EQUATION_LINEAR && nparams != 2) ||
       (type == CI_EQUATION_BASE_E && nparams != 3) ||
       (type == CI_EQUATION_ARBITRARY && nparams != 3) ||
       (type == CI_EQUATION_HYPERBOLIC && nparams != 4))
   {
      ci_chunk_benign_error(ci_ptr, "invalid parameter count");
      return handled_error;
   }

   else if (type >= CI_EQUATION_LAST)
   {
      ci_chunk_benign_error(ci_ptr, "unrecognized equation type");
   }

   for (buf = units; *buf; buf++)
      /* Empty loop to move past the units string. */ ;

   ci_debug(3, "Allocating pCAL parameters array");

   params = ci_voidcast(ci_charpp, ci_malloc_warn(ci_ptr,
       nparams * (sizeof (ci_charp))));

   if (params == NULL)
   {
      ci_chunk_benign_error(ci_ptr, "out of memory");
      return handled_error;
   }

   /* Get pointers to the start of each parameter string. */
   for (i = 0; i < nparams; i++)
   {
      buf++; /* Skip the null string terminator from previous parameter. */

      ci_debug1(3, "Reading pCAL parameter %d", i);

      for (params[i] = (ci_charp)buf; buf <= endptr && *buf != 0; buf++)
         /* Empty loop to move past each parameter string */ ;

      /* Make sure we haven't run out of data yet */
      if (buf > endptr)
      {
         ci_free(ci_ptr, params);
         ci_chunk_benign_error(ci_ptr, "invalid data");
         return handled_error;
      }
   }

   ci_set_pCAL(ci_ptr, info_ptr, (ci_charp)buffer, X0, X1, type, nparams,
       (ci_charp)units, params);

   /* TODO: BUG: ci_set_pCAL calls ci_chunk_report which, in this case, calls
    * ci_benign_error and that can error out.
    *
    * ci_read_buffer needs to be allocated with space for both nparams and the
    * parameter strings.  Not hard to do.
    */
   ci_free(ci_ptr, params);
   return handled_ok;
}
#else
#  define ci_handle_pCAL NULL
#endif

#ifdef CI_READ_sCAL_SUPPORTED
/* Read the sCAL chunk */
static ci_handle_result_code /* PRIVATE */
ci_handle_sCAL(ci_structrp ci_ptr, ci_inforp info_ptr, ci_uint_32 length)
{
   ci_bytep buffer;
   size_t i;
   int state;

   ci_debug(1, "in ci_handle_sCAL");
   ci_debug1(2, "Allocating and reading sCAL chunk data (%u bytes)",
       length + 1);

   buffer = ci_read_buffer(ci_ptr, length+1);

   if (buffer == NULL)
   {
      ci_crc_finish(ci_ptr, length);
      ci_chunk_benign_error(ci_ptr, "out of memory");
      return handled_error;
   }

   ci_crc_read(ci_ptr, buffer, length);
   buffer[length] = 0; /* Null terminate the last string */

   if (ci_crc_finish(ci_ptr, 0) != 0)
      return handled_error;

   /* Validate the unit. */
   if (buffer[0] != 1 && buffer[0] != 2)
   {
      ci_chunk_benign_error(ci_ptr, "invalid unit");
      return handled_error;
   }

   /* Validate the ASCII numbers, need two ASCII numbers separated by
    * a '\0' and they need to fit exactly in the chunk data.
    */
   i = 1;
   state = 0;

   if (ci_check_fp_number((ci_const_charp)buffer, length, &state, &i) == 0 ||
       i >= length || buffer[i++] != 0)
      ci_chunk_benign_error(ci_ptr, "bad width format");

   else if (CI_FP_IS_POSITIVE(state) == 0)
      ci_chunk_benign_error(ci_ptr, "non-positive width");

   else
   {
      size_t heighti = i;

      state = 0;
      if (ci_check_fp_number((ci_const_charp)buffer, length,
          &state, &i) == 0 || i != length)
         ci_chunk_benign_error(ci_ptr, "bad height format");

      else if (CI_FP_IS_POSITIVE(state) == 0)
         ci_chunk_benign_error(ci_ptr, "non-positive height");

      else
      {
         /* This is the (only) success case. */
         ci_set_sCAL_s(ci_ptr, info_ptr, buffer[0],
             (ci_charp)buffer+1, (ci_charp)buffer+heighti);
         return handled_ok;
      }
   }

   return handled_error;
}
#else
#  define ci_handle_sCAL NULL
#endif

#ifdef CI_READ_tIME_SUPPORTED
static ci_handle_result_code /* PRIVATE */
ci_handle_tIME(ci_structrp ci_ptr, ci_inforp info_ptr, ci_uint_32 length)
{
   ci_byte buf[7];
   ci_time mod_time;

   ci_debug(1, "in ci_handle_tIME");

   /* TODO: what is this doing here?  It should be happened in ciread.c and
    * cipread.c, although it could be moved to ci_handle_chunk below and
    * thereby avoid some code duplication.
    */
   if ((ci_ptr->mode & CI_HAVE_IDAT) != 0)
      ci_ptr->mode |= CI_AFTER_IDAT;

   ci_crc_read(ci_ptr, buf, 7);

   if (ci_crc_finish(ci_ptr, 0) != 0)
      return handled_error;

   mod_time.second = buf[6];
   mod_time.minute = buf[5];
   mod_time.hour = buf[4];
   mod_time.day = buf[3];
   mod_time.month = buf[2];
   mod_time.year = ci_get_uint_16(buf);

   ci_set_tIME(ci_ptr, info_ptr, &mod_time);
   return handled_ok;
   CI_UNUSED(length)
}
#else
#  define ci_handle_tIME NULL
#endif

#ifdef CI_READ_tEXt_SUPPORTED
/* Note: this does not properly handle chunks that are > 64K under DOS */
static ci_handle_result_code /* PRIVATE */
ci_handle_tEXt(ci_structrp ci_ptr, ci_inforp info_ptr, ci_uint_32 length)
{
   ci_text  text_info;
   ci_bytep buffer;
   ci_charp key;
   ci_charp text;
   ci_uint_32 skip = 0;

   ci_debug(1, "in ci_handle_tEXt");

#ifdef CI_USER_LIMITS_SUPPORTED
   if (ci_ptr->user_chunk_cache_max != 0)
   {
      if (ci_ptr->user_chunk_cache_max == 1)
      {
         ci_crc_finish(ci_ptr, length);
         return handled_error;
      }

      if (--ci_ptr->user_chunk_cache_max == 1)
      {
         ci_crc_finish(ci_ptr, length);
         ci_chunk_benign_error(ci_ptr, "no space in chunk cache");
         return handled_error;
      }
   }
#endif

   /* TODO: this doesn't work and shouldn't be necessary. */
   if ((ci_ptr->mode & CI_HAVE_IDAT) != 0)
      ci_ptr->mode |= CI_AFTER_IDAT;

   buffer = ci_read_buffer(ci_ptr, length+1);

   if (buffer == NULL)
   {
      ci_crc_finish(ci_ptr, length);
      ci_chunk_benign_error(ci_ptr, "out of memory");
      return handled_error;
   }

   ci_crc_read(ci_ptr, buffer, length);

   if (ci_crc_finish(ci_ptr, skip) != 0)
      return handled_error;

   key = (ci_charp)buffer;
   key[length] = 0;

   for (text = key; *text; text++)
      /* Empty loop to find end of key */ ;

   if (text != key + length)
      text++;

   text_info.compression = CI_TEXT_COMPRESSION_NONE;
   text_info.key = key;
   text_info.lang = NULL;
   text_info.lang_key = NULL;
   text_info.itxt_length = 0;
   text_info.text = text;
   text_info.text_length = strlen(text);

   if (ci_set_text_2(ci_ptr, info_ptr, &text_info, 1) == 0)
      return handled_ok;

   ci_chunk_benign_error(ci_ptr, "out of memory");
   return handled_error;
}
#else
#  define ci_handle_tEXt NULL
#endif

#ifdef CI_READ_zTXt_SUPPORTED
/* Note: this does not correctly handle chunks that are > 64K under DOS */
static ci_handle_result_code /* PRIVATE */
ci_handle_zTXt(ci_structrp ci_ptr, ci_inforp info_ptr, ci_uint_32 length)
{
   ci_const_charp errmsg = NULL;
   ci_bytep       buffer;
   ci_uint_32     keyword_length;

   ci_debug(1, "in ci_handle_zTXt");

#ifdef CI_USER_LIMITS_SUPPORTED
   if (ci_ptr->user_chunk_cache_max != 0)
   {
      if (ci_ptr->user_chunk_cache_max == 1)
      {
         ci_crc_finish(ci_ptr, length);
         return handled_error;
      }

      if (--ci_ptr->user_chunk_cache_max == 1)
      {
         ci_crc_finish(ci_ptr, length);
         ci_chunk_benign_error(ci_ptr, "no space in chunk cache");
         return handled_error;
      }
   }
#endif

   /* TODO: should not be necessary. */
   if ((ci_ptr->mode & CI_HAVE_IDAT) != 0)
      ci_ptr->mode |= CI_AFTER_IDAT;

   /* Note, "length" is sufficient here; we won't be adding
    * a null terminator later.  The limit check in ci_handle_chunk should be
    * sufficient.
    */
   buffer = ci_read_buffer(ci_ptr, length);

   if (buffer == NULL)
   {
      ci_crc_finish(ci_ptr, length);
      ci_chunk_benign_error(ci_ptr, "out of memory");
      return handled_error;
   }

   ci_crc_read(ci_ptr, buffer, length);

   if (ci_crc_finish(ci_ptr, 0) != 0)
      return handled_error;

   /* TODO: also check that the keyword contents match the spec! */
   for (keyword_length = 0;
      keyword_length < length && buffer[keyword_length] != 0;
      ++keyword_length)
      /* Empty loop to find end of name */ ;

   if (keyword_length > 79 || keyword_length < 1)
      errmsg = "bad keyword";

   /* zTXt must have some LZ data after the keyword, although it may expand to
    * zero bytes; we need a '\0' at the end of the keyword, the compression type
    * then the LZ data:
    */
   else if (keyword_length + 3 > length)
      errmsg = "truncated";

   else if (buffer[keyword_length+1] != CI_COMPRESSION_TYPE_BASE)
      errmsg = "unknown compression type";

   else
   {
      ci_alloc_size_t uncompressed_length = CI_SIZE_MAX;

      /* TODO: at present ci_decompress_chunk imposes a single application
       * level memory limit, this should be split to different values for iCCP
       * and text chunks.
       */
      if (ci_decompress_chunk(ci_ptr, length, keyword_length+2,
          &uncompressed_length, 1/*terminate*/) == Z_STREAM_END)
      {
         ci_text text;

         if (ci_ptr->read_buffer == NULL)
           errmsg="Read failure in ci_handle_zTXt";
         else
         {
            /* It worked; ci_ptr->read_buffer now looks like a tEXt chunk
             * except for the extra compression type byte and the fact that
             * it isn't necessarily '\0' terminated.
             */
            buffer = ci_ptr->read_buffer;
            buffer[uncompressed_length+(keyword_length+2)] = 0;

            text.compression = CI_TEXT_COMPRESSION_zTXt;
            text.key = (ci_charp)buffer;
            text.text = (ci_charp)(buffer + keyword_length+2);
            text.text_length = uncompressed_length;
            text.itxt_length = 0;
            text.lang = NULL;
            text.lang_key = NULL;

            if (ci_set_text_2(ci_ptr, info_ptr, &text, 1) == 0)
               return handled_ok;

            errmsg = "out of memory";
         }
      }

      else
         errmsg = ci_ptr->zstream.msg;
   }

   ci_chunk_benign_error(ci_ptr, errmsg);
   return handled_error;
}
#else
#  define ci_handle_zTXt NULL
#endif

#ifdef CI_READ_iTXt_SUPPORTED
/* Note: this does not correctly handle chunks that are > 64K under DOS */
static ci_handle_result_code /* PRIVATE */
ci_handle_iTXt(ci_structrp ci_ptr, ci_inforp info_ptr, ci_uint_32 length)
{
   ci_const_charp errmsg = NULL;
   ci_bytep buffer;
   ci_uint_32 prefix_length;

   ci_debug(1, "in ci_handle_iTXt");

#ifdef CI_USER_LIMITS_SUPPORTED
   if (ci_ptr->user_chunk_cache_max != 0)
   {
      if (ci_ptr->user_chunk_cache_max == 1)
      {
         ci_crc_finish(ci_ptr, length);
         return handled_error;
      }

      if (--ci_ptr->user_chunk_cache_max == 1)
      {
         ci_crc_finish(ci_ptr, length);
         ci_chunk_benign_error(ci_ptr, "no space in chunk cache");
         return handled_error;
      }
   }
#endif

   /* TODO: should not be necessary. */
   if ((ci_ptr->mode & CI_HAVE_IDAT) != 0)
      ci_ptr->mode |= CI_AFTER_IDAT;

   buffer = ci_read_buffer(ci_ptr, length+1);

   if (buffer == NULL)
   {
      ci_crc_finish(ci_ptr, length);
      ci_chunk_benign_error(ci_ptr, "out of memory");
      return handled_error;
   }

   ci_crc_read(ci_ptr, buffer, length);

   if (ci_crc_finish(ci_ptr, 0) != 0)
      return handled_error;

   /* First the keyword. */
   for (prefix_length=0;
      prefix_length < length && buffer[prefix_length] != 0;
      ++prefix_length)
      /* Empty loop */ ;

   /* Perform a basic check on the keyword length here. */
   if (prefix_length > 79 || prefix_length < 1)
      errmsg = "bad keyword";

   /* Expect keyword, compression flag, compression type, language, translated
    * keyword (both may be empty but are 0 terminated) then the text, which may
    * be empty.
    */
   else if (prefix_length + 5 > length)
      errmsg = "truncated";

   else if (buffer[prefix_length+1] == 0 ||
      (buffer[prefix_length+1] == 1 &&
      buffer[prefix_length+2] == CI_COMPRESSION_TYPE_BASE))
   {
      int compressed = buffer[prefix_length+1] != 0;
      ci_uint_32 language_offset, translated_keyword_offset;
      ci_alloc_size_t uncompressed_length = 0;

      /* Now the language tag */
      prefix_length += 3;
      language_offset = prefix_length;

      for (; prefix_length < length && buffer[prefix_length] != 0;
         ++prefix_length)
         /* Empty loop */ ;

      /* WARNING: the length may be invalid here, this is checked below. */
      translated_keyword_offset = ++prefix_length;

      for (; prefix_length < length && buffer[prefix_length] != 0;
         ++prefix_length)
         /* Empty loop */ ;

      /* prefix_length should now be at the trailing '\0' of the translated
       * keyword, but it may already be over the end.  None of this arithmetic
       * can overflow because chunks are at most 2^31 bytes long, but on 16-bit
       * systems the available allocation may overflow.
       */
      ++prefix_length;

      if (compressed == 0 && prefix_length <= length)
         uncompressed_length = length - prefix_length;

      else if (compressed != 0 && prefix_length < length)
      {
         uncompressed_length = CI_SIZE_MAX;

         /* TODO: at present ci_decompress_chunk imposes a single application
          * level memory limit, this should be split to different values for
          * iCCP and text chunks.
          */
         if (ci_decompress_chunk(ci_ptr, length, prefix_length,
             &uncompressed_length, 1/*terminate*/) == Z_STREAM_END)
            buffer = ci_ptr->read_buffer;

         else
            errmsg = ci_ptr->zstream.msg;
      }

      else
         errmsg = "truncated";

      if (errmsg == NULL)
      {
         ci_text text;

         buffer[uncompressed_length+prefix_length] = 0;

         if (compressed == 0)
            text.compression = CI_ITXT_COMPRESSION_NONE;

         else
            text.compression = CI_ITXT_COMPRESSION_zTXt;

         text.key = (ci_charp)buffer;
         text.lang = (ci_charp)buffer + language_offset;
         text.lang_key = (ci_charp)buffer + translated_keyword_offset;
         text.text = (ci_charp)buffer + prefix_length;
         text.text_length = 0;
         text.itxt_length = uncompressed_length;

         if (ci_set_text_2(ci_ptr, info_ptr, &text, 1) == 0)
            return handled_ok;

         errmsg = "out of memory";
      }
   }

   else
      errmsg = "bad compression info";

   if (errmsg != NULL)
      ci_chunk_benign_error(ci_ptr, errmsg);
   return handled_error;
}
#else
#  define ci_handle_iTXt NULL
#endif

#ifdef CI_READ_UNKNOWN_CHUNKS_SUPPORTED
/* Utility function for ci_handle_unknown; set up ci_ptr::unknown_chunk */
static int
ci_cache_unknown_chunk(ci_structrp ci_ptr, ci_uint_32 length)
{
   const ci_alloc_size_t limit = ci_chunk_max(ci_ptr);

   if (ci_ptr->unknown_chunk.data != NULL)
   {
      ci_free(ci_ptr, ci_ptr->unknown_chunk.data);
      ci_ptr->unknown_chunk.data = NULL;
   }

   if (length <= limit)
   {
      CI_CSTRING_FROM_CHUNK(ci_ptr->unknown_chunk.name, ci_ptr->chunk_name);
      /* The following is safe because of the CI_SIZE_MAX init above */
      ci_ptr->unknown_chunk.size = (size_t)length/*SAFE*/;
      /* 'mode' is a flag array, only the bottom four bits matter here */
      ci_ptr->unknown_chunk.location = (ci_byte)ci_ptr->mode/*SAFE*/;

      if (length == 0)
         ci_ptr->unknown_chunk.data = NULL;

      else
      {
         /* Do a 'warn' here - it is handled below. */
         ci_ptr->unknown_chunk.data = ci_voidcast(ci_bytep,
             ci_malloc_warn(ci_ptr, length));
      }
   }

   if (ci_ptr->unknown_chunk.data == NULL && length > 0)
   {
      /* This is benign because we clean up correctly */
      ci_crc_finish(ci_ptr, length);
      ci_chunk_benign_error(ci_ptr, "unknown chunk exceeds memory limits");
      return 0;
   }

   else
   {
      if (length > 0)
         ci_crc_read(ci_ptr, ci_ptr->unknown_chunk.data, length);
      ci_crc_finish(ci_ptr, 0);
      return 1;
   }
}
#endif /* READ_UNKNOWN_CHUNKS */

/* Handle an unknown, or known but disabled, chunk */
ci_handle_result_code /*PRIVATE*/
ci_handle_unknown(ci_structrp ci_ptr, ci_inforp info_ptr,
    ci_uint_32 length, int keep)
{
   ci_handle_result_code handled = handled_discarded; /* the default */

   ci_debug(1, "in ci_handle_unknown");

#ifdef CI_READ_UNKNOWN_CHUNKS_SUPPORTED
   /* NOTE: this code is based on the code in libci-1.4.12 except for fixing
    * the bug which meant that setting a non-default behavior for a specific
    * chunk would be ignored (the default was always used unless a user
    * callback was installed).
    *
    * 'keep' is the value from the ci_chunk_unknown_handling, the setting for
    * this specific chunk_name, if CI_HANDLE_AS_UNKNOWN_SUPPORTED, if not it
    * will always be CI_HANDLE_CHUNK_AS_DEFAULT and it needs to be set here.
    * This is just an optimization to avoid multiple calls to the lookup
    * function.
    */
#  ifndef CI_HANDLE_AS_UNKNOWN_SUPPORTED
#     ifdef CI_SET_UNKNOWN_CHUNKS_SUPPORTED
   keep = ci_chunk_unknown_handling(ci_ptr, ci_ptr->chunk_name);
#     endif
#  endif

   /* One of the following methods will read the chunk or skip it (at least one
    * of these is always defined because this is the only way to switch on
    * CI_READ_UNKNOWN_CHUNKS_SUPPORTED)
    */
#  ifdef CI_READ_USER_CHUNKS_SUPPORTED
   /* The user callback takes precedence over the chunk keep value, but the
    * keep value is still required to validate a save of a critical chunk.
    */
   if (ci_ptr->read_user_chunk_fn != NULL)
   {
      if (ci_cache_unknown_chunk(ci_ptr, length) != 0)
      {
         /* Callback to user unknown chunk handler */
         int ret = (*(ci_ptr->read_user_chunk_fn))(ci_ptr,
             &ci_ptr->unknown_chunk);

         /* ret is:
          * negative: An error occurred; ci_chunk_error will be called.
          *     zero: The chunk was not handled, the chunk will be discarded
          *           unless ci_set_keep_unknown_chunks has been used to set
          *           a 'keep' behavior for this particular chunk, in which
          *           case that will be used.  A critical chunk will cause an
          *           error at this point unless it is to be saved.
          * positive: The chunk was handled, libci will ignore/discard it.
          */
         if (ret < 0) /* handled_error */
            ci_chunk_error(ci_ptr, "error in user chunk");

         else if (ret == 0)
         {
            /* If the keep value is 'default' or 'never' override it, but
             * still error out on critical chunks unless the keep value is
             * 'always'  While this is weird it is the behavior in 1.4.12.
             * A possible improvement would be to obey the value set for the
             * chunk, but this would be an API change that would probably
             * damage some applications.
             *
             * The ci_app_warning below catches the case that matters, where
             * the application has not set specific save or ignore for this
             * chunk or global save or ignore.
             */
            if (keep < CI_HANDLE_CHUNK_IF_SAFE)
            {
#              ifdef CI_SET_UNKNOWN_CHUNKS_SUPPORTED
               if (ci_ptr->unknown_default < CI_HANDLE_CHUNK_IF_SAFE)
               {
                  ci_chunk_warning(ci_ptr, "Saving unknown chunk:");
                  ci_app_warning(ci_ptr,
                      "forcing save of an unhandled chunk;"
                      " please call ci_set_keep_unknown_chunks");
                      /* with keep = CI_HANDLE_CHUNK_IF_SAFE */
               }
#              endif
               keep = CI_HANDLE_CHUNK_IF_SAFE;
            }
         }

         else /* chunk was handled */
         {
            handled = handled_ok;
            /* Critical chunks can be safely discarded at this point. */
            keep = CI_HANDLE_CHUNK_NEVER;
         }
      }

      else
         keep = CI_HANDLE_CHUNK_NEVER; /* insufficient memory */
   }

   else
   /* Use the SAVE_UNKNOWN_CHUNKS code or skip the chunk */
#  endif /* READ_USER_CHUNKS */

#  ifdef CI_SAVE_UNKNOWN_CHUNKS_SUPPORTED
   {
      /* keep is currently just the per-chunk setting, if there was no
       * setting change it to the global default now (not that this may
       * still be AS_DEFAULT) then obtain the cache of the chunk if required,
       * if not simply skip the chunk.
       */
      if (keep == CI_HANDLE_CHUNK_AS_DEFAULT)
         keep = ci_ptr->unknown_default;

      if (keep == CI_HANDLE_CHUNK_ALWAYS ||
         (keep == CI_HANDLE_CHUNK_IF_SAFE &&
          CI_CHUNK_ANCILLARY(ci_ptr->chunk_name)))
      {
         if (ci_cache_unknown_chunk(ci_ptr, length) == 0)
            keep = CI_HANDLE_CHUNK_NEVER;
      }

      else
         ci_crc_finish(ci_ptr, length);
   }
#  else
#     ifndef CI_READ_USER_CHUNKS_SUPPORTED
#        error no method to support READ_UNKNOWN_CHUNKS
#     endif

   {
      /* If here there is no read callback pointer set and no support is
       * compiled in to just save the unknown chunks, so simply skip this
       * chunk.  If 'keep' is something other than AS_DEFAULT or NEVER then
       * the app has erroneously asked for unknown chunk saving when there
       * is no support.
       */
      if (keep > CI_HANDLE_CHUNK_NEVER)
         ci_app_error(ci_ptr, "no unknown chunk support available");

      ci_crc_finish(ci_ptr, length);
   }
#  endif

#  ifdef CI_STORE_UNKNOWN_CHUNKS_SUPPORTED
   /* Now store the chunk in the chunk list if appropriate, and if the limits
    * permit it.
    */
   if (keep == CI_HANDLE_CHUNK_ALWAYS ||
      (keep == CI_HANDLE_CHUNK_IF_SAFE &&
       CI_CHUNK_ANCILLARY(ci_ptr->chunk_name)))
   {
#     ifdef CI_USER_LIMITS_SUPPORTED
      switch (ci_ptr->user_chunk_cache_max)
      {
         case 2:
            ci_ptr->user_chunk_cache_max = 1;
            ci_chunk_benign_error(ci_ptr, "no space in chunk cache");
            /* FALLTHROUGH */
         case 1:
            /* NOTE: prior to 1.6.0 this case resulted in an unknown critical
             * chunk being skipped, now there will be a hard error below.
             */
            break;

         default: /* not at limit */
            --(ci_ptr->user_chunk_cache_max);
            /* FALLTHROUGH */
         case 0: /* no limit */
#  endif /* USER_LIMITS */
            /* Here when the limit isn't reached or when limits are compiled
             * out; store the chunk.
             */
            ci_set_unknown_chunks(ci_ptr, info_ptr,
                &ci_ptr->unknown_chunk, 1);
            handled = handled_saved;
#  ifdef CI_USER_LIMITS_SUPPORTED
            break;
      }
#  endif
   }
#  else /* no store support: the chunk must be handled by the user callback */
   CI_UNUSED(info_ptr)
#  endif

   /* Regardless of the error handling below the cached data (if any) can be
    * freed now.  Notice that the data is not freed if there is a ci_error, but
    * it will be freed by destroy_read_struct.
    */
   if (ci_ptr->unknown_chunk.data != NULL)
      ci_free(ci_ptr, ci_ptr->unknown_chunk.data);
   ci_ptr->unknown_chunk.data = NULL;

#else /* !CI_READ_UNKNOWN_CHUNKS_SUPPORTED */
   /* There is no support to read an unknown chunk, so just skip it. */
   ci_crc_finish(ci_ptr, length);
   CI_UNUSED(info_ptr)
   CI_UNUSED(keep)
#endif /* !READ_UNKNOWN_CHUNKS */

   /* Check for unhandled critical chunks */
   if (handled < handled_saved && CI_CHUNK_CRITICAL(ci_ptr->chunk_name))
      ci_chunk_error(ci_ptr, "unhandled critical chunk");

   return handled;
}

/* ACI handling: the minimal implementation of ACI handling in libci 1.6
 * requires that those significant applications which already handle ACI not
 * get hosed.  To do this ensure the code here will have to ensure than ACI
 * data by default (at least in 1.6) gets stored in the unknown chunk list.
 * Maybe this can be relaxed in a few years but at present it's just the only
 * safe way.
 *
 * ATM just cause unknown handling for all three chunks:
 */
#define ci_handle_acTL NULL
#define ci_handle_fcTL NULL
#define ci_handle_fdAT NULL

/*
 * 1.6.47: This is the new table driven interface to all the chunk handling.
 *
 * The table describes the CI standard rules for **reading** known chunks -
 * every chunk which has an entry in CI_KNOWN_CHUNKS.  The table contains an
 * entry for each CI_INDEX_cHNK describing the rules.
 *
 * In this initial version the only information in the entry is the
 * ci_handle_cHNK function for the chunk in question.  When chunk support is
 * compiled out the entry will be NULL.
 */
static const struct
{
   ci_handle_result_code (*handler)(
         ci_structrp, ci_inforp, ci_uint_32 length);
      /* A chunk-specific 'handler', NULL if the chunk is not supported in this
       * build.
       */

   /* Crushing these values helps on modern 32-bit architectures because the
    * pointer and the following bit fields both end up requiring 32 bits.
    * Typically this will halve the table size.  On 64-bit architectures the
    * table entries will typically be 8 bytes.
    */
   ci_uint_32 max_length :12; /* Length min, max in bytes */
   ci_uint_32 min_length :8;
      /* Length errors on critical chunks have special handling to preserve the
       * existing behaviour in libci 1.6.  Anciallary chunks are checked below
       * and produce a 'benign' error.
       */
   ci_uint_32 pos_before :4; /* CI_HAVE_ values chunk must precede */
   ci_uint_32 pos_after  :4; /* CI_HAVE_ values chunk must follow */
      /* NOTE: PLTE, tRNS and bKGD require special handling which depends on
       * the colour type of the base image.
       */
   ci_uint_32 multiple   :1; /* Multiple occurences permitted */
      /* This is enabled for PLTE because PLTE may, in practice, be optional */
}
read_chunks[CI_INDEX_unknown] =
{
   /* Definitions as above but done indirectly by #define so that
    * CI_KNOWN_CHUNKS can be used safely to build the table in order.
    *
    * Each CDcHNK definition lists the values for the parameters **after**
    * the first, 'handler', function.  'handler' is NULL when the chunk has no
    * compiled in support.
    */
#  define NoCheck 0x801U      /* Do not check the maximum length */
#  define Limit   0x802U      /* Limit to ci_chunk_max bytes */
#  define LKMin   3U+LZ77Min  /* Minimum length of keyword+LZ77 */

#define hIHDR CI_HAVE_IHDR
#define hPLTE CI_HAVE_PLTE
#define hIDAT CI_HAVE_IDAT
   /* For the two chunks, tRNS and bKGD which can occur in CIs without a PLTE
    * but must occur after the PLTE use this and put the check in the handler
    * routine for colour mapped images were PLTE is required.  Also put a check
    * in PLTE for other image types to drop the PLTE if tRNS or bKGD have been
    * seen.
    */
#define hCOL  (CI_HAVE_PLTE|CI_HAVE_IDAT)
   /* Used for the decoding chunks which must be before PLTE. */
#define aIDAT CI_AFTER_IDAT

   /* Chunks from W3C CI v3: */
   /*       cHNK  max_len,   min, before, after, multiple */
#  define CDIHDR      13U,   13U,  hIHDR,     0,        0
#  define CDPLTE  NoCheck,    0U,      0, hIHDR,        1
      /* PLTE errors are only critical for colour-map images, consequently the
       * hander does all the checks.
       */
#  define CDIDAT  NoCheck,    0U,  aIDAT, hIHDR,        1
#  define CDIEND  NoCheck,    0U,      0, aIDAT,        0
      /* Historically data was allowed in IEND */
#  define CDtRNS     256U,    0U,  hIDAT, hIHDR,        0
#  define CDcHRM      32U,   32U,   hCOL, hIHDR,        0
#  define CDgAMA       4U,    4U,   hCOL, hIHDR,        0
#  define CDiCCP  NoCheck, LKMin,   hCOL, hIHDR,        0
#  define CDsBIT       4U,    1U,   hCOL, hIHDR,        0
#  define CDsRGB       1U,    1U,   hCOL, hIHDR,        0
#  define CDcICP       4U,    4U,   hCOL, hIHDR,        0
#  define CDmDCV      24U,   24U,   hCOL, hIHDR,        0
#  define CDeXIf    Limit,    4U,      0, hIHDR,        0
#  define CDcLLI       8U,    8U,   hCOL, hIHDR,        0
#  define CDtEXt  NoCheck,    2U,      0, hIHDR,        1
      /* Allocates 'length+1'; checked in the handler */
#  define CDzTXt    Limit, LKMin,      0, hIHDR,        1
#  define CDiTXt  NoCheck,    6U,      0, hIHDR,        1
      /* Allocates 'length+1'; checked in the handler */
#  define CDbKGD       6U,    1U,  hIDAT, hIHDR,        0
#  define CDhIST    1024U,    0U,  hPLTE, hIHDR,        0
#  define CDpHYs       9U,    9U,  hIDAT, hIHDR,        0
#  define CDsPLT  NoCheck,    3U,  hIDAT, hIHDR,        1
      /* Allocates 'length+1'; checked in the handler */
#  define CDtIME       7U,    7U,      0, hIHDR,        0
#  define CDacTL       8U,    8U,  hIDAT, hIHDR,        0
#  define CDfcTL      25U,   26U,      0, hIHDR,        1
#  define CDfdAT    Limit,    4U,  hIDAT, hIHDR,        1
   /* Supported chunks from CI extensions 1.5.0, NYI so limit */
#  define CDoFFs       9U,    9U,  hIDAT, hIHDR,        0
#  define CDpCAL  NoCheck,   14U,  hIDAT, hIHDR,        0
      /* Allocates 'length+1'; checked in the handler */
#  define CDsCAL    Limit,    4U,  hIDAT, hIHDR,        0
      /* Allocates 'length+1'; checked in the handler */

#  define CI_CHUNK(cHNK, index) { ci_handle_ ## cHNK, CD ## cHNK },
   CI_KNOWN_CHUNKS
#  undef CI_CHUNK
};


static ci_index
ci_chunk_index_from_name(ci_uint_32 chunk_name)
{
   /* For chunk ci_cHNK return CI_INDEX_cHNK.  Return CI_INDEX_unknown if
    * chunk_name is not known.  Notice that in a particular build "known" does
    * not necessarily mean "supported", although the inverse applies.
    */
   switch (chunk_name)
   {
#     define CI_CHUNK(cHNK, index)\
         case ci_ ## cHNK: return CI_INDEX_ ## cHNK; /* == index */

      CI_KNOWN_CHUNKS

#     undef CI_CHUNK

      default: return CI_INDEX_unknown;
   }
}

ci_handle_result_code /*PRIVATE*/
ci_handle_chunk(ci_structrp ci_ptr, ci_inforp info_ptr, ci_uint_32 length)
{
   /* CSE: these things don't change, these autos are just to save typing and
    * make the code more clear.
    */
   const ci_uint_32 chunk_name = ci_ptr->chunk_name;
   const ci_index chunk_index = ci_chunk_index_from_name(chunk_name);

   ci_handle_result_code handled = handled_error;
   ci_const_charp errmsg = NULL;

   /* Is this a known chunk?  If not there are no checks performed here;
    * ci_handle_unknown does the correct checks.  This means that the values
    * for known but unsupported chunks in the above table are not used here
    * however the chunks_seen fields in ci_struct are still set.
    */
   if (chunk_index == CI_INDEX_unknown ||
       read_chunks[chunk_index].handler == NULL)
   {
      handled = ci_handle_unknown(
            ci_ptr, info_ptr, length, CI_HANDLE_CHUNK_AS_DEFAULT);
   }

   /* First check the position.   The first check is historical; the stream must
    * start with IHDR and anything else causes libci to give up immediately.
    */
   else if (chunk_index != CI_INDEX_IHDR &&
            (ci_ptr->mode & CI_HAVE_IHDR) == 0)
      ci_chunk_error(ci_ptr, "missing IHDR"); /* NORETURN */

   /* Before all the pos_before chunks, after all the pos_after chunks. */
   else if (((ci_ptr->mode & read_chunks[chunk_index].pos_before) != 0) ||
            ((ci_ptr->mode & read_chunks[chunk_index].pos_after) !=
             read_chunks[chunk_index].pos_after))
   {
      errmsg = "out of place";
   }

   /* Now check for duplicates: duplicated critical chunks also produce a
    * full error.
    */
   else if (read_chunks[chunk_index].multiple == 0 &&
            ci_file_has_chunk(ci_ptr, chunk_index))
   {
      errmsg = "duplicate";
   }

   else if (length < read_chunks[chunk_index].min_length)
      errmsg = "too short";
   else
   {
      /* NOTE: apart from IHDR the critical chunks (PLTE, IDAT and IEND) are set
       * up above not to do any length checks.
       *
       * The ci_chunk_max check ensures that the variable length chunks are
       * always checked at this point for being within the system allocation
       * limits.
       */
      unsigned max_length = read_chunks[chunk_index].max_length;

      switch (max_length)
      {
         case Limit:
            /* ci_read_chunk_header has already ci_error'ed chunks with a
             * length exceeding the 31-bit CI limit, so just check the memory
             * limit:
             */
            if (length <= ci_chunk_max(ci_ptr))
               goto MeetsLimit;

            errmsg = "length exceeds libci limit";
            break;

         default:
            if (length <= max_length)
               goto MeetsLimit;

            errmsg = "too long";
            break;

         case NoCheck:
         MeetsLimit:
            handled = read_chunks[chunk_index].handler(
                  ci_ptr, info_ptr, length);
            break;
      }
   }

   /* If there was an error or the chunk was simply skipped it is not counted as
    * 'seen'.
    */
   if (errmsg != NULL)
   {
      if (CI_CHUNK_CRITICAL(chunk_name)) /* stop immediately */
         ci_chunk_error(ci_ptr, errmsg);
      else /* ancillary chunk */
      {
         /* The chunk data is skipped: */
         ci_crc_finish(ci_ptr, length);
         ci_chunk_benign_error(ci_ptr, errmsg);
      }
   }

   else if (handled >= handled_saved)
   {
      if (chunk_index != CI_INDEX_unknown)
         ci_file_add_chunk(ci_ptr, chunk_index);
   }

   return handled;
}

/* Combines the row recently read in with the existing pixels in the row.  This
 * routine takes care of alpha and transparency if requested.  This routine also
 * handles the two methods of progressive display of interlaced images,
 * depending on the 'display' value; if 'display' is true then the whole row
 * (dp) is filled from the start by replicating the available pixels.  If
 * 'display' is false only those pixels present in the pass are filled in.
 */
void /* PRIVATE */
ci_combine_row(ci_const_structrp ci_ptr, ci_bytep dp, int display)
{
   unsigned int pixel_depth = ci_ptr->transformed_pixel_depth;
   ci_const_bytep sp = ci_ptr->row_buf + 1;
   ci_alloc_size_t row_width = ci_ptr->width;
   unsigned int pass = ci_ptr->pass;
   ci_bytep end_ptr = 0;
   ci_byte end_byte = 0;
   unsigned int end_mask;

   ci_debug(1, "in ci_combine_row");

   /* Added in 1.5.6: it should not be possible to enter this routine until at
    * least one row has been read from the CI data and transformed.
    */
   if (pixel_depth == 0)
      ci_error(ci_ptr, "internal row logic error");

   /* Added in 1.5.4: the pixel depth should match the information returned by
    * any call to ci_read_update_info at this point.  Do not continue if we got
    * this wrong.
    */
   if (ci_ptr->info_rowbytes != 0 && ci_ptr->info_rowbytes !=
          CI_ROWBYTES(pixel_depth, row_width))
      ci_error(ci_ptr, "internal row size calculation error");

   /* Don't expect this to ever happen: */
   if (row_width == 0)
      ci_error(ci_ptr, "internal row width error");

   /* Preserve the last byte in cases where only part of it will be overwritten,
    * the multiply below may overflow, we don't care because ANSI-C guarantees
    * we get the low bits.
    */
   end_mask = (pixel_depth * row_width) & 7;
   if (end_mask != 0)
   {
      /* end_ptr == NULL is a flag to say do nothing */
      end_ptr = dp + CI_ROWBYTES(pixel_depth, row_width) - 1;
      end_byte = *end_ptr;
#     ifdef CI_READ_PACKSWAP_SUPPORTED
      if ((ci_ptr->transformations & CI_PACKSWAP) != 0)
         /* little-endian byte */
         end_mask = (unsigned int)(0xff << end_mask);

      else /* big-endian byte */
#     endif
      end_mask = 0xff >> end_mask;
      /* end_mask is now the bits to *keep* from the destination row */
   }

   /* For non-interlaced images this reduces to a memcpy(). A memcpy()
    * will also happen if interlacing isn't supported or if the application
    * does not call ci_set_interlace_handling().  In the latter cases the
    * caller just gets a sequence of the unexpanded rows from each interlace
    * pass.
    */
#ifdef CI_READ_INTERLACING_SUPPORTED
   if (ci_ptr->interlaced != 0 &&
       (ci_ptr->transformations & CI_INTERLACE) != 0 &&
       pass < 6 && (display == 0 ||
       /* The following copies everything for 'display' on passes 0, 2 and 4. */
       (display == 1 && (pass & 1) != 0)))
   {
      /* Narrow images may have no bits in a pass; the caller should handle
       * this, but this test is cheap:
       */
      if (row_width <= CI_PASS_START_COL(pass))
         return;

      if (pixel_depth < 8)
      {
         /* For pixel depths up to 4 bpp the 8-pixel mask can be expanded to fit
          * into 32 bits, then a single loop over the bytes using the four byte
          * values in the 32-bit mask can be used.  For the 'display' option the
          * expanded mask may also not require any masking within a byte.  To
          * make this work the PACKSWAP option must be taken into account - it
          * simply requires the pixels to be reversed in each byte.
          *
          * The 'regular' case requires a mask for each of the first 6 passes,
          * the 'display' case does a copy for the even passes in the range
          * 0..6.  This has already been handled in the test above.
          *
          * The masks are arranged as four bytes with the first byte to use in
          * the lowest bits (little-endian) regardless of the order (PACKSWAP or
          * not) of the pixels in each byte.
          *
          * NOTE: the whole of this logic depends on the caller of this function
          * only calling it on rows appropriate to the pass.  This function only
          * understands the 'x' logic; the 'y' logic is handled by the caller.
          *
          * The following defines allow generation of compile time constant bit
          * masks for each pixel depth and each possibility of swapped or not
          * swapped bytes.  Pass 'p' is in the range 0..6; 'x', a pixel index,
          * is in the range 0..7; and the result is 1 if the pixel is to be
          * copied in the pass, 0 if not.  'S' is for the sparkle method, 'B'
          * for the block method.
          *
          * With some compilers a compile time expression of the general form:
          *
          *    (shift >= 32) ? (a >> (shift-32)) : (b >> shift)
          *
          * Produces warnings with values of 'shift' in the range 33 to 63
          * because the right hand side of the ?: expression is evaluated by
          * the compiler even though it isn't used.  Microsoft Visual C (various
          * versions) and the Intel C compiler are known to do this.  To avoid
          * this the following macros are used in 1.5.6.  This is a temporary
          * solution to avoid destabilizing the code during the release process.
          */
#        if CI_USE_COMPILE_TIME_MASKS
#           define CI_LSR(x,s) ((x)>>((s) & 0x1f))
#           define CI_LSL(x,s) ((x)<<((s) & 0x1f))
#        else
#           define CI_LSR(x,s) ((x)>>(s))
#           define CI_LSL(x,s) ((x)<<(s))
#        endif
#        define S_COPY(p,x) (((p)<4 ? CI_LSR(0x80088822,(3-(p))*8+(7-(x))) :\
           CI_LSR(0xaa55ff00,(7-(p))*8+(7-(x)))) & 1)
#        define B_COPY(p,x) (((p)<4 ? CI_LSR(0xff0fff33,(3-(p))*8+(7-(x))) :\
           CI_LSR(0xff55ff00,(7-(p))*8+(7-(x)))) & 1)

         /* Return a mask for pass 'p' pixel 'x' at depth 'd'.  The mask is
          * little endian - the first pixel is at bit 0 - however the extra
          * parameter 's' can be set to cause the mask position to be swapped
          * within each byte, to match the CI format.  This is done by XOR of
          * the shift with 7, 6 or 4 for bit depths 1, 2 and 4.
          */
#        define PIXEL_MASK(p,x,d,s) \
            (CI_LSL(((CI_LSL(1U,(d)))-1),(((x)*(d))^((s)?8-(d):0))))

         /* Hence generate the appropriate 'block' or 'sparkle' pixel copy mask.
          */
#        define S_MASKx(p,x,d,s) (S_COPY(p,x)?PIXEL_MASK(p,x,d,s):0)
#        define B_MASKx(p,x,d,s) (B_COPY(p,x)?PIXEL_MASK(p,x,d,s):0)

         /* Combine 8 of these to get the full mask.  For the 1-bpp and 2-bpp
          * cases the result needs replicating, for the 4-bpp case the above
          * generates a full 32 bits.
          */
#        define MASK_EXPAND(m,d) ((m)*((d)==1?0x01010101:((d)==2?0x00010001:1)))

#        define S_MASK(p,d,s) MASK_EXPAND(S_MASKx(p,0,d,s) + S_MASKx(p,1,d,s) +\
            S_MASKx(p,2,d,s) + S_MASKx(p,3,d,s) + S_MASKx(p,4,d,s) +\
            S_MASKx(p,5,d,s) + S_MASKx(p,6,d,s) + S_MASKx(p,7,d,s), d)

#        define B_MASK(p,d,s) MASK_EXPAND(B_MASKx(p,0,d,s) + B_MASKx(p,1,d,s) +\
            B_MASKx(p,2,d,s) + B_MASKx(p,3,d,s) + B_MASKx(p,4,d,s) +\
            B_MASKx(p,5,d,s) + B_MASKx(p,6,d,s) + B_MASKx(p,7,d,s), d)

#if CI_USE_COMPILE_TIME_MASKS
         /* Utility macros to construct all the masks for a depth/swap
          * combination.  The 's' parameter says whether the format is CI
          * (big endian bytes) or not.  Only the three odd-numbered passes are
          * required for the display/block algorithm.
          */
#        define S_MASKS(d,s) { S_MASK(0,d,s), S_MASK(1,d,s), S_MASK(2,d,s),\
            S_MASK(3,d,s), S_MASK(4,d,s), S_MASK(5,d,s) }

#        define B_MASKS(d,s) { B_MASK(1,d,s), B_MASK(3,d,s), B_MASK(5,d,s) }

#        define DEPTH_INDEX(d) ((d)==1?0:((d)==2?1:2))

         /* Hence the pre-compiled masks indexed by PACKSWAP (or not), depth and
          * then pass:
          */
         static const ci_uint_32 row_mask[2/*PACKSWAP*/][3/*depth*/][6] =
         {
            /* Little-endian byte masks for PACKSWAP */
            { S_MASKS(1,0), S_MASKS(2,0), S_MASKS(4,0) },
            /* Normal (big-endian byte) masks - CI format */
            { S_MASKS(1,1), S_MASKS(2,1), S_MASKS(4,1) }
         };

         /* display_mask has only three entries for the odd passes, so index by
          * pass>>1.
          */
         static const ci_uint_32 display_mask[2][3][3] =
         {
            /* Little-endian byte masks for PACKSWAP */
            { B_MASKS(1,0), B_MASKS(2,0), B_MASKS(4,0) },
            /* Normal (big-endian byte) masks - CI format */
            { B_MASKS(1,1), B_MASKS(2,1), B_MASKS(4,1) }
         };

#        define MASK(pass,depth,display,ci)\
            ((display)?display_mask[ci][DEPTH_INDEX(depth)][pass>>1]:\
               row_mask[ci][DEPTH_INDEX(depth)][pass])

#else /* !CI_USE_COMPILE_TIME_MASKS */
         /* This is the runtime alternative: it seems unlikely that this will
          * ever be either smaller or faster than the compile time approach.
          */
#        define MASK(pass,depth,display,ci)\
            ((display)?B_MASK(pass,depth,ci):S_MASK(pass,depth,ci))
#endif /* !USE_COMPILE_TIME_MASKS */

         /* Use the appropriate mask to copy the required bits.  In some cases
          * the byte mask will be 0 or 0xff; optimize these cases.  row_width is
          * the number of pixels, but the code copies bytes, so it is necessary
          * to special case the end.
          */
         ci_uint_32 pixels_per_byte = 8 / pixel_depth;
         ci_uint_32 mask;

#        ifdef CI_READ_PACKSWAP_SUPPORTED
         if ((ci_ptr->transformations & CI_PACKSWAP) != 0)
            mask = MASK(pass, pixel_depth, display, 0);

         else
#        endif
         mask = MASK(pass, pixel_depth, display, 1);

         for (;;)
         {
            ci_uint_32 m;

            /* It doesn't matter in the following if ci_uint_32 has more than
             * 32 bits because the high bits always match those in m<<24; it is,
             * however, essential to use OR here, not +, because of this.
             */
            m = mask;
            mask = (m >> 8) | (m << 24); /* rotate right to good compilers */
            m &= 0xff;

            if (m != 0) /* something to copy */
            {
               if (m != 0xff)
                  *dp = (ci_byte)((*dp & ~m) | (*sp & m));
               else
                  *dp = *sp;
            }

            /* NOTE: this may overwrite the last byte with garbage if the image
             * is not an exact number of bytes wide; libci has always done
             * this.
             */
            if (row_width <= pixels_per_byte)
               break; /* May need to restore part of the last byte */

            row_width -= pixels_per_byte;
            ++dp;
            ++sp;
         }
      }

      else /* pixel_depth >= 8 */
      {
         unsigned int bytes_to_copy, bytes_to_jump;

         /* Validate the depth - it must be a multiple of 8 */
         if (pixel_depth & 7)
            ci_error(ci_ptr, "invalid user transform pixel depth");

         pixel_depth >>= 3; /* now in bytes */
         row_width *= pixel_depth;

         /* Regardless of pass number the Adam 7 interlace always results in a
          * fixed number of pixels to copy then to skip.  There may be a
          * different number of pixels to skip at the start though.
          */
         {
            unsigned int offset = CI_PASS_START_COL(pass) * pixel_depth;

            row_width -= offset;
            dp += offset;
            sp += offset;
         }

         /* Work out the bytes to copy. */
         if (display != 0)
         {
            /* When doing the 'block' algorithm the pixel in the pass gets
             * replicated to adjacent pixels.  This is why the even (0,2,4,6)
             * passes are skipped above - the entire expanded row is copied.
             */
            bytes_to_copy = (1<<((6-pass)>>1)) * pixel_depth;

            /* But don't allow this number to exceed the actual row width. */
            if (bytes_to_copy > row_width)
               bytes_to_copy = (unsigned int)/*SAFE*/row_width;
         }

         else /* normal row; Adam7 only ever gives us one pixel to copy. */
            bytes_to_copy = pixel_depth;

         /* In Adam7 there is a constant offset between where the pixels go. */
         bytes_to_jump = CI_PASS_COL_OFFSET(pass) * pixel_depth;

         /* And simply copy these bytes.  Some optimization is possible here,
          * depending on the value of 'bytes_to_copy'.  Special case the low
          * byte counts, which we know to be frequent.
          *
          * Notice that these cases all 'return' rather than 'break' - this
          * avoids an unnecessary test on whether to restore the last byte
          * below.
          */
         switch (bytes_to_copy)
         {
            case 1:
               for (;;)
               {
                  *dp = *sp;

                  if (row_width <= bytes_to_jump)
                     return;

                  dp += bytes_to_jump;
                  sp += bytes_to_jump;
                  row_width -= bytes_to_jump;
               }

            case 2:
               /* There is a possibility of a partial copy at the end here; this
                * slows the code down somewhat.
                */
               do
               {
                  dp[0] = sp[0]; dp[1] = sp[1];

                  if (row_width <= bytes_to_jump)
                     return;

                  sp += bytes_to_jump;
                  dp += bytes_to_jump;
                  row_width -= bytes_to_jump;
               }
               while (row_width > 1);

               /* And there can only be one byte left at this point: */
               *dp = *sp;
               return;

            case 3:
               /* This can only be the RGB case, so each copy is exactly one
                * pixel and it is not necessary to check for a partial copy.
                */
               for (;;)
               {
                  dp[0] = sp[0]; dp[1] = sp[1]; dp[2] = sp[2];

                  if (row_width <= bytes_to_jump)
                     return;

                  sp += bytes_to_jump;
                  dp += bytes_to_jump;
                  row_width -= bytes_to_jump;
               }

            default:
#if CI_ALIGN_TYPE != CI_ALIGN_NONE
               /* Check for double byte alignment and, if possible, use a
                * 16-bit copy.  Don't attempt this for narrow images - ones that
                * are less than an interlace panel wide.  Don't attempt it for
                * wide bytes_to_copy either - use the memcpy there.
                */
               if (bytes_to_copy < 16 /*else use memcpy*/ &&
                   ci_isaligned(dp, ci_uint_16) &&
                   ci_isaligned(sp, ci_uint_16) &&
                   bytes_to_copy % (sizeof (ci_uint_16)) == 0 &&
                   bytes_to_jump % (sizeof (ci_uint_16)) == 0)
               {
                  /* Everything is aligned for ci_uint_16 copies, but try for
                   * ci_uint_32 first.
                   */
                  if (ci_isaligned(dp, ci_uint_32) &&
                      ci_isaligned(sp, ci_uint_32) &&
                      bytes_to_copy % (sizeof (ci_uint_32)) == 0 &&
                      bytes_to_jump % (sizeof (ci_uint_32)) == 0)
                  {
                     ci_uint_32p dp32 = ci_aligncast(ci_uint_32p,dp);
                     ci_const_uint_32p sp32 = ci_aligncastconst(
                         ci_const_uint_32p, sp);
                     size_t skip = (bytes_to_jump-bytes_to_copy) /
                         (sizeof (ci_uint_32));

                     do
                     {
                        size_t c = bytes_to_copy;
                        do
                        {
                           *dp32++ = *sp32++;
                           c -= (sizeof (ci_uint_32));
                        }
                        while (c > 0);

                        if (row_width <= bytes_to_jump)
                           return;

                        dp32 += skip;
                        sp32 += skip;
                        row_width -= bytes_to_jump;
                     }
                     while (bytes_to_copy <= row_width);

                     /* Get to here when the row_width truncates the final copy.
                      * There will be 1-3 bytes left to copy, so don't try the
                      * 16-bit loop below.
                      */
                     dp = (ci_bytep)dp32;
                     sp = (ci_const_bytep)sp32;
                     do
                        *dp++ = *sp++;
                     while (--row_width > 0);
                     return;
                  }

                  /* Else do it in 16-bit quantities, but only if the size is
                   * not too large.
                   */
                  else
                  {
                     ci_uint_16p dp16 = ci_aligncast(ci_uint_16p, dp);
                     ci_const_uint_16p sp16 = ci_aligncastconst(
                        ci_const_uint_16p, sp);
                     size_t skip = (bytes_to_jump-bytes_to_copy) /
                        (sizeof (ci_uint_16));

                     do
                     {
                        size_t c = bytes_to_copy;
                        do
                        {
                           *dp16++ = *sp16++;
                           c -= (sizeof (ci_uint_16));
                        }
                        while (c > 0);

                        if (row_width <= bytes_to_jump)
                           return;

                        dp16 += skip;
                        sp16 += skip;
                        row_width -= bytes_to_jump;
                     }
                     while (bytes_to_copy <= row_width);

                     /* End of row - 1 byte left, bytes_to_copy > row_width: */
                     dp = (ci_bytep)dp16;
                     sp = (ci_const_bytep)sp16;
                     do
                        *dp++ = *sp++;
                     while (--row_width > 0);
                     return;
                  }
               }
#endif /* ALIGN_TYPE code */

               /* The true default - use a memcpy: */
               for (;;)
               {
                  memcpy(dp, sp, bytes_to_copy);

                  if (row_width <= bytes_to_jump)
                     return;

                  sp += bytes_to_jump;
                  dp += bytes_to_jump;
                  row_width -= bytes_to_jump;
                  if (bytes_to_copy > row_width)
                     bytes_to_copy = (unsigned int)/*SAFE*/row_width;
               }
         }

         /* NOT REACHED*/
      } /* pixel_depth >= 8 */

      /* Here if pixel_depth < 8 to check 'end_ptr' below. */
   }
   else
#endif /* READ_INTERLACING */

   /* If here then the switch above wasn't used so just memcpy the whole row
    * from the temporary row buffer (notice that this overwrites the end of the
    * destination row if it is a partial byte.)
    */
   memcpy(dp, sp, CI_ROWBYTES(pixel_depth, row_width));

   /* Restore the overwritten bits from the last byte if necessary. */
   if (end_ptr != NULL)
      *end_ptr = (ci_byte)((end_byte & end_mask) | (*end_ptr & ~end_mask));
}

#ifdef CI_READ_INTERLACING_SUPPORTED
void /* PRIVATE */
ci_do_read_interlace(ci_row_infop row_info, ci_bytep row, int pass,
    ci_uint_32 transformations /* Because these may affect the byte layout */)
{
   ci_debug(1, "in ci_do_read_interlace");
   if (row != NULL && row_info != NULL)
   {
      ci_uint_32 final_width;

      final_width = row_info->width * ci_pass_inc[pass];

      switch (row_info->pixel_depth)
      {
         case 1:
         {
            ci_bytep sp = row + (size_t)((row_info->width - 1) >> 3);
            ci_bytep dp = row + (size_t)((final_width - 1) >> 3);
            unsigned int sshift, dshift;
            unsigned int s_start, s_end;
            int s_inc;
            int jstop = (int)ci_pass_inc[pass];
            ci_byte v;
            ci_uint_32 i;
            int j;

#ifdef CI_READ_PACKSWAP_SUPPORTED
            if ((transformations & CI_PACKSWAP) != 0)
            {
                sshift = ((row_info->width + 7) & 0x07);
                dshift = ((final_width + 7) & 0x07);
                s_start = 7;
                s_end = 0;
                s_inc = -1;
            }

            else
#endif
            {
                sshift = 7 - ((row_info->width + 7) & 0x07);
                dshift = 7 - ((final_width + 7) & 0x07);
                s_start = 0;
                s_end = 7;
                s_inc = 1;
            }

            for (i = 0; i < row_info->width; i++)
            {
               v = (ci_byte)((*sp >> sshift) & 0x01);
               for (j = 0; j < jstop; j++)
               {
                  unsigned int tmp = *dp & (0x7f7f >> (7 - dshift));
                  tmp |= (unsigned int)(v << dshift);
                  *dp = (ci_byte)(tmp & 0xff);

                  if (dshift == s_end)
                  {
                     dshift = s_start;
                     dp--;
                  }

                  else
                     dshift = (unsigned int)((int)dshift + s_inc);
               }

               if (sshift == s_end)
               {
                  sshift = s_start;
                  sp--;
               }

               else
                  sshift = (unsigned int)((int)sshift + s_inc);
            }
            break;
         }

         case 2:
         {
            ci_bytep sp = row + (ci_uint_32)((row_info->width - 1) >> 2);
            ci_bytep dp = row + (ci_uint_32)((final_width - 1) >> 2);
            unsigned int sshift, dshift;
            unsigned int s_start, s_end;
            int s_inc;
            int jstop = (int)ci_pass_inc[pass];
            ci_uint_32 i;

#ifdef CI_READ_PACKSWAP_SUPPORTED
            if ((transformations & CI_PACKSWAP) != 0)
            {
               sshift = (((row_info->width + 3) & 0x03) << 1);
               dshift = (((final_width + 3) & 0x03) << 1);
               s_start = 6;
               s_end = 0;
               s_inc = -2;
            }

            else
#endif
            {
               sshift = ((3 - ((row_info->width + 3) & 0x03)) << 1);
               dshift = ((3 - ((final_width + 3) & 0x03)) << 1);
               s_start = 0;
               s_end = 6;
               s_inc = 2;
            }

            for (i = 0; i < row_info->width; i++)
            {
               ci_byte v;
               int j;

               v = (ci_byte)((*sp >> sshift) & 0x03);
               for (j = 0; j < jstop; j++)
               {
                  unsigned int tmp = *dp & (0x3f3f >> (6 - dshift));
                  tmp |= (unsigned int)(v << dshift);
                  *dp = (ci_byte)(tmp & 0xff);

                  if (dshift == s_end)
                  {
                     dshift = s_start;
                     dp--;
                  }

                  else
                     dshift = (unsigned int)((int)dshift + s_inc);
               }

               if (sshift == s_end)
               {
                  sshift = s_start;
                  sp--;
               }

               else
                  sshift = (unsigned int)((int)sshift + s_inc);
            }
            break;
         }

         case 4:
         {
            ci_bytep sp = row + (size_t)((row_info->width - 1) >> 1);
            ci_bytep dp = row + (size_t)((final_width - 1) >> 1);
            unsigned int sshift, dshift;
            unsigned int s_start, s_end;
            int s_inc;
            ci_uint_32 i;
            int jstop = (int)ci_pass_inc[pass];

#ifdef CI_READ_PACKSWAP_SUPPORTED
            if ((transformations & CI_PACKSWAP) != 0)
            {
               sshift = (((row_info->width + 1) & 0x01) << 2);
               dshift = (((final_width + 1) & 0x01) << 2);
               s_start = 4;
               s_end = 0;
               s_inc = -4;
            }

            else
#endif
            {
               sshift = ((1 - ((row_info->width + 1) & 0x01)) << 2);
               dshift = ((1 - ((final_width + 1) & 0x01)) << 2);
               s_start = 0;
               s_end = 4;
               s_inc = 4;
            }

            for (i = 0; i < row_info->width; i++)
            {
               ci_byte v = (ci_byte)((*sp >> sshift) & 0x0f);
               int j;

               for (j = 0; j < jstop; j++)
               {
                  unsigned int tmp = *dp & (0xf0f >> (4 - dshift));
                  tmp |= (unsigned int)(v << dshift);
                  *dp = (ci_byte)(tmp & 0xff);

                  if (dshift == s_end)
                  {
                     dshift = s_start;
                     dp--;
                  }

                  else
                     dshift = (unsigned int)((int)dshift + s_inc);
               }

               if (sshift == s_end)
               {
                  sshift = s_start;
                  sp--;
               }

               else
                  sshift = (unsigned int)((int)sshift + s_inc);
            }
            break;
         }

         default:
         {
            size_t pixel_bytes = (row_info->pixel_depth >> 3);

            ci_bytep sp = row + (size_t)(row_info->width - 1)
                * pixel_bytes;

            ci_bytep dp = row + (size_t)(final_width - 1) * pixel_bytes;

            int jstop = (int)ci_pass_inc[pass];
            ci_uint_32 i;

            for (i = 0; i < row_info->width; i++)
            {
               ci_byte v[8]; /* SAFE; pixel_depth does not exceed 64 */
               int j;

               memcpy(v, sp, pixel_bytes);

               for (j = 0; j < jstop; j++)
               {
                  memcpy(dp, v, pixel_bytes);
                  dp -= pixel_bytes;
               }

               sp -= pixel_bytes;
            }
            break;
         }
      }

      row_info->width = final_width;
      row_info->rowbytes = CI_ROWBYTES(row_info->pixel_depth, final_width);
   }
#ifndef CI_READ_PACKSWAP_SUPPORTED
   CI_UNUSED(transformations)  /* Silence compiler warning */
#endif
}
#endif /* READ_INTERLACING */

static void
ci_read_filter_row_sub(ci_row_infop row_info, ci_bytep row,
    ci_const_bytep prev_row)
{
   size_t i;
   size_t istop = row_info->rowbytes;
   unsigned int bpp = (row_info->pixel_depth + 7) >> 3;
   ci_bytep rp = row + bpp;

   CI_UNUSED(prev_row)

   for (i = bpp; i < istop; i++)
   {
      *rp = (ci_byte)(((int)(*rp) + (int)(*(rp-bpp))) & 0xff);
      rp++;
   }
}

static void
ci_read_filter_row_up(ci_row_infop row_info, ci_bytep row,
    ci_const_bytep prev_row)
{
   size_t i;
   size_t istop = row_info->rowbytes;
   ci_bytep rp = row;
   ci_const_bytep pp = prev_row;

   for (i = 0; i < istop; i++)
   {
      *rp = (ci_byte)(((int)(*rp) + (int)(*pp++)) & 0xff);
      rp++;
   }
}

static void
ci_read_filter_row_avg(ci_row_infop row_info, ci_bytep row,
    ci_const_bytep prev_row)
{
   size_t i;
   ci_bytep rp = row;
   ci_const_bytep pp = prev_row;
   unsigned int bpp = (row_info->pixel_depth + 7) >> 3;
   size_t istop = row_info->rowbytes - bpp;

   for (i = 0; i < bpp; i++)
   {
      *rp = (ci_byte)(((int)(*rp) +
         ((int)(*pp++) / 2 )) & 0xff);

      rp++;
   }

   for (i = 0; i < istop; i++)
   {
      *rp = (ci_byte)(((int)(*rp) +
         (int)(*pp++ + *(rp-bpp)) / 2 ) & 0xff);

      rp++;
   }
}

static void
ci_read_filter_row_paeth_1byte_pixel(ci_row_infop row_info, ci_bytep row,
    ci_const_bytep prev_row)
{
   ci_bytep rp_end = row + row_info->rowbytes;
   int a, c;

   /* First pixel/byte */
   c = *prev_row++;
   a = *row + c;
   *row++ = (ci_byte)a;

   /* Remainder */
   while (row < rp_end)
   {
      int b, pa, pb, pc, p;

      a &= 0xff; /* From previous iteration or start */
      b = *prev_row++;

      p = b - c;
      pc = a - c;

#ifdef CI_USE_ABS
      pa = abs(p);
      pb = abs(pc);
      pc = abs(p + pc);
#else
      pa = p < 0 ? -p : p;
      pb = pc < 0 ? -pc : pc;
      pc = (p + pc) < 0 ? -(p + pc) : p + pc;
#endif

      /* Find the best predictor, the least of pa, pb, pc favoring the earlier
       * ones in the case of a tie.
       */
      if (pb < pa)
      {
         pa = pb; a = b;
      }
      if (pc < pa) a = c;

      /* Calculate the current pixel in a, and move the previous row pixel to c
       * for the next time round the loop
       */
      c = b;
      a += *row;
      *row++ = (ci_byte)a;
   }
}

static void
ci_read_filter_row_paeth_multibyte_pixel(ci_row_infop row_info, ci_bytep row,
    ci_const_bytep prev_row)
{
   unsigned int bpp = (row_info->pixel_depth + 7) >> 3;
   ci_bytep rp_end = row + bpp;

   /* Process the first pixel in the row completely (this is the same as 'up'
    * because there is only one candidate predictor for the first row).
    */
   while (row < rp_end)
   {
      int a = *row + *prev_row++;
      *row++ = (ci_byte)a;
   }

   /* Remainder */
   rp_end = rp_end + (row_info->rowbytes - bpp);

   while (row < rp_end)
   {
      int a, b, c, pa, pb, pc, p;

      c = *(prev_row - bpp);
      a = *(row - bpp);
      b = *prev_row++;

      p = b - c;
      pc = a - c;

#ifdef CI_USE_ABS
      pa = abs(p);
      pb = abs(pc);
      pc = abs(p + pc);
#else
      pa = p < 0 ? -p : p;
      pb = pc < 0 ? -pc : pc;
      pc = (p + pc) < 0 ? -(p + pc) : p + pc;
#endif

      if (pb < pa)
      {
         pa = pb; a = b;
      }
      if (pc < pa) a = c;

      a += *row;
      *row++ = (ci_byte)a;
   }
}

static void
ci_init_filter_functions(ci_structrp pp)
   /* This function is called once for every CI image (except for CI images
    * that only use CI_FILTER_VALUE_NONE for all rows) to set the
    * implementations required to reverse the filtering of CI rows.  Reversing
    * the filter is the first transformation performed on the row data.  It is
    * performed in place, therefore an implementation can be selected based on
    * the image pixel format.  If the implementation depends on image width then
    * take care to ensure that it works correctly if the image is interlaced -
    * interlacing causes the actual row width to vary.
    */
{
   unsigned int bpp = (pp->pixel_depth + 7) >> 3;

   pp->read_filter[CI_FILTER_VALUE_SUB-1] = ci_read_filter_row_sub;
   pp->read_filter[CI_FILTER_VALUE_UP-1] = ci_read_filter_row_up;
   pp->read_filter[CI_FILTER_VALUE_AVG-1] = ci_read_filter_row_avg;
   if (bpp == 1)
      pp->read_filter[CI_FILTER_VALUE_PAETH-1] =
         ci_read_filter_row_paeth_1byte_pixel;
   else
      pp->read_filter[CI_FILTER_VALUE_PAETH-1] =
         ci_read_filter_row_paeth_multibyte_pixel;

#ifdef CI_FILTER_OPTIMIZATIONS
   /* To use this define CI_FILTER_OPTIMIZATIONS as the name of a function to
    * call to install hardware optimizations for the above functions; simply
    * replace whatever elements of the pp->read_filter[] array with a hardware
    * specific (or, for that matter, generic) optimization.
    *
    * To see an example of this examine what configure.ac does when
    * --enable-arm-neon is specified on the command line.
    */
   CI_FILTER_OPTIMIZATIONS(pp, bpp);
#endif
}

void /* PRIVATE */
ci_read_filter_row(ci_structrp pp, ci_row_infop row_info, ci_bytep row,
    ci_const_bytep prev_row, int filter)
{
   /* OPTIMIZATION: DO NOT MODIFY THIS FUNCTION, instead #define
    * CI_FILTER_OPTIMIZATIONS to a function that overrides the generic
    * implementations.  See ci_init_filter_functions above.
    */
   if (filter > CI_FILTER_VALUE_NONE && filter < CI_FILTER_VALUE_LAST)
   {
      if (pp->read_filter[0] == NULL)
         ci_init_filter_functions(pp);

      pp->read_filter[filter-1](row_info, row, prev_row);
   }
}

#ifdef CI_SEQUENTIAL_READ_SUPPORTED
void /* PRIVATE */
ci_read_IDAT_data(ci_structrp ci_ptr, ci_bytep output,
    ci_alloc_size_t avail_out)
{
   /* Loop reading IDATs and decompressing the result into output[avail_out] */
   ci_ptr->zstream.next_out = output;
   ci_ptr->zstream.avail_out = 0; /* safety: set below */

   if (output == NULL)
      avail_out = 0;

   do
   {
      int ret;
      ci_byte tmpbuf[CI_INFLATE_BUF_SIZE];

      if (ci_ptr->zstream.avail_in == 0)
      {
         uInt avail_in;
         ci_bytep buffer;

         while (ci_ptr->idat_size == 0)
         {
            ci_crc_finish(ci_ptr, 0);

            ci_ptr->idat_size = ci_read_chunk_header(ci_ptr);
            /* This is an error even in the 'check' case because the code just
             * consumed a non-IDAT header.
             */
            if (ci_ptr->chunk_name != ci_IDAT)
               ci_error(ci_ptr, "Not enough image data");
         }

         avail_in = ci_ptr->IDAT_read_size;

         if (avail_in > ci_chunk_max(ci_ptr))
            avail_in = (uInt)/*SAFE*/ci_chunk_max(ci_ptr);

         if (avail_in > ci_ptr->idat_size)
            avail_in = (uInt)ci_ptr->idat_size;

         /* A CI with a gradually increasing IDAT size will defeat this attempt
          * to minimize memory usage by causing lots of re-allocs, but
          * realistically doing IDAT_read_size re-allocs is not likely to be a
          * big problem.
          *
          * An error here corresponds to the system being out of memory.
          */
         buffer = ci_read_buffer(ci_ptr, avail_in);

         if (buffer == NULL)
            ci_chunk_error(ci_ptr, "out of memory");

         ci_crc_read(ci_ptr, buffer, avail_in);
         ci_ptr->idat_size -= avail_in;

         ci_ptr->zstream.next_in = buffer;
         ci_ptr->zstream.avail_in = avail_in;
      }

      /* And set up the output side. */
      if (output != NULL) /* standard read */
      {
         uInt out = ZLIB_IO_MAX;

         if (out > avail_out)
            out = (uInt)avail_out;

         avail_out -= out;
         ci_ptr->zstream.avail_out = out;
      }

      else /* after last row, checking for end */
      {
         ci_ptr->zstream.next_out = tmpbuf;
         ci_ptr->zstream.avail_out = (sizeof tmpbuf);
      }

      /* Use NO_FLUSH; this gives zlib the maximum opportunity to optimize the
       * process.  If the LZ stream is truncated the sequential reader will
       * terminally damage the stream, above, by reading the chunk header of the
       * following chunk (it then exits with ci_error).
       *
       * TODO: deal more elegantly with truncated IDAT lists.
       */
      ret = CI_INFLATE(ci_ptr, Z_NO_FLUSH);

      /* Take the unconsumed output back. */
      if (output != NULL)
         avail_out += ci_ptr->zstream.avail_out;

      else /* avail_out counts the extra bytes */
         avail_out += (sizeof tmpbuf) - ci_ptr->zstream.avail_out;

      ci_ptr->zstream.avail_out = 0;

      if (ret == Z_STREAM_END)
      {
         /* Do this for safety; we won't read any more into this row. */
         ci_ptr->zstream.next_out = NULL;

         ci_ptr->mode |= CI_AFTER_IDAT;
         ci_ptr->flags |= CI_FLAG_ZSTREAM_ENDED;

         if (ci_ptr->zstream.avail_in > 0 || ci_ptr->idat_size > 0)
            ci_chunk_benign_error(ci_ptr, "Extra compressed data");
         break;
      }

      if (ret != Z_OK)
      {
         ci_zstream_error(ci_ptr, ret);

         if (output != NULL)
            ci_chunk_error(ci_ptr, ci_ptr->zstream.msg);

         else /* checking */
         {
            ci_chunk_benign_error(ci_ptr, ci_ptr->zstream.msg);
            return;
         }
      }
   } while (avail_out > 0);

   if (avail_out > 0)
   {
      /* The stream ended before the image; this is the same as too few IDATs so
       * should be handled the same way.
       */
      if (output != NULL)
         ci_error(ci_ptr, "Not enough image data");

      else /* the deflate stream contained extra data */
         ci_chunk_benign_error(ci_ptr, "Too much image data");
   }
}

void /* PRIVATE */
ci_read_finish_IDAT(ci_structrp ci_ptr)
{
   /* We don't need any more data and the stream should have ended, however the
    * LZ end code may actually not have been processed.  In this case we must
    * read it otherwise stray unread IDAT data or, more likely, an IDAT chunk
    * may still remain to be consumed.
    */
   if ((ci_ptr->flags & CI_FLAG_ZSTREAM_ENDED) == 0)
   {
      /* The NULL causes ci_read_IDAT_data to swallow any remaining bytes in
       * the compressed stream, but the stream may be damaged too, so even after
       * this call we may need to terminate the zstream ownership.
       */
      ci_read_IDAT_data(ci_ptr, NULL, 0);
      ci_ptr->zstream.next_out = NULL; /* safety */

      /* Now clear everything out for safety; the following may not have been
       * done.
       */
      if ((ci_ptr->flags & CI_FLAG_ZSTREAM_ENDED) == 0)
      {
         ci_ptr->mode |= CI_AFTER_IDAT;
         ci_ptr->flags |= CI_FLAG_ZSTREAM_ENDED;
      }
   }

   /* If the zstream has not been released do it now *and* terminate the reading
    * of the final IDAT chunk.
    */
   if (ci_ptr->zowner == ci_IDAT)
   {
      /* Always do this; the pointers otherwise point into the read buffer. */
      ci_ptr->zstream.next_in = NULL;
      ci_ptr->zstream.avail_in = 0;

      /* Now we no longer own the zstream. */
      ci_ptr->zowner = 0;

      /* The slightly weird semantics of the sequential IDAT reading is that we
       * are always in or at the end of an IDAT chunk, so we always need to do a
       * crc_finish here.  If idat_size is non-zero we also need to read the
       * spurious bytes at the end of the chunk now.
       */
      (void)ci_crc_finish(ci_ptr, ci_ptr->idat_size);
   }
}

void /* PRIVATE */
ci_read_finish_row(ci_structrp ci_ptr)
{
   ci_debug(1, "in ci_read_finish_row");
   ci_ptr->row_number++;
   if (ci_ptr->row_number < ci_ptr->num_rows)
      return;

   if (ci_ptr->interlaced != 0)
   {
      ci_ptr->row_number = 0;

      /* TO DO: don't do this if prev_row isn't needed (requires
       * read-ahead of the next row's filter byte.
       */
      memset(ci_ptr->prev_row, 0, ci_ptr->rowbytes + 1);

      do
      {
         ci_ptr->pass++;

         if (ci_ptr->pass >= 7)
            break;

         ci_ptr->iwidth = (ci_ptr->width +
            ci_pass_inc[ci_ptr->pass] - 1 -
            ci_pass_start[ci_ptr->pass]) /
            ci_pass_inc[ci_ptr->pass];

         if ((ci_ptr->transformations & CI_INTERLACE) == 0)
         {
            ci_ptr->num_rows = (ci_ptr->height +
                ci_pass_yinc[ci_ptr->pass] - 1 -
                ci_pass_ystart[ci_ptr->pass]) /
                ci_pass_yinc[ci_ptr->pass];
         }

         else  /* if (ci_ptr->transformations & CI_INTERLACE) */
            break; /* libci deinterlacing sees every row */

      } while (ci_ptr->num_rows == 0 || ci_ptr->iwidth == 0);

      if (ci_ptr->pass < 7)
         return;
   }

   /* Here after at the end of the last row of the last pass. */
   ci_read_finish_IDAT(ci_ptr);
}
#endif /* SEQUENTIAL_READ */

void /* PRIVATE */
ci_read_start_row(ci_structrp ci_ptr)
{
   unsigned int max_pixel_depth;
   size_t row_bytes;

   ci_debug(1, "in ci_read_start_row");

#ifdef CI_READ_TRANSFORMS_SUPPORTED
   ci_init_read_transformations(ci_ptr);
#endif
   if (ci_ptr->interlaced != 0)
   {
      if ((ci_ptr->transformations & CI_INTERLACE) == 0)
         ci_ptr->num_rows = (ci_ptr->height + ci_pass_yinc[0] - 1 -
             ci_pass_ystart[0]) / ci_pass_yinc[0];

      else
         ci_ptr->num_rows = ci_ptr->height;

      ci_ptr->iwidth = (ci_ptr->width +
          ci_pass_inc[ci_ptr->pass] - 1 -
          ci_pass_start[ci_ptr->pass]) /
          ci_pass_inc[ci_ptr->pass];
   }

   else
   {
      ci_ptr->num_rows = ci_ptr->height;
      ci_ptr->iwidth = ci_ptr->width;
   }

   max_pixel_depth = (unsigned int)ci_ptr->pixel_depth;

   /* WARNING: * ci_read_transform_info (cirtran.c) performs a simpler set of
    * calculations to calculate the final pixel depth, then
    * ci_do_read_transforms actually does the transforms.  This means that the
    * code which effectively calculates this value is actually repeated in three
    * separate places.  They must all match.  Innocent changes to the order of
    * transformations can and will break libci in a way that causes memory
    * overwrites.
    *
    * TODO: fix this.
    */
#ifdef CI_READ_PACK_SUPPORTED
   if ((ci_ptr->transformations & CI_PACK) != 0 && ci_ptr->bit_depth < 8)
      max_pixel_depth = 8;
#endif

#ifdef CI_READ_EXPAND_SUPPORTED
   if ((ci_ptr->transformations & CI_EXPAND) != 0)
   {
      if (ci_ptr->color_type == CI_COLOR_TYPE_PALETTE)
      {
         if (ci_ptr->num_trans != 0)
            max_pixel_depth = 32;

         else
            max_pixel_depth = 24;
      }

      else if (ci_ptr->color_type == CI_COLOR_TYPE_GRAY)
      {
         if (max_pixel_depth < 8)
            max_pixel_depth = 8;

         if (ci_ptr->num_trans != 0)
            max_pixel_depth *= 2;
      }

      else if (ci_ptr->color_type == CI_COLOR_TYPE_RGB)
      {
         if (ci_ptr->num_trans != 0)
         {
            max_pixel_depth *= 4;
            max_pixel_depth /= 3;
         }
      }
   }
#endif

#ifdef CI_READ_EXPAND_16_SUPPORTED
   if ((ci_ptr->transformations & CI_EXPAND_16) != 0)
   {
#  ifdef CI_READ_EXPAND_SUPPORTED
      /* In fact it is an error if it isn't supported, but checking is
       * the safe way.
       */
      if ((ci_ptr->transformations & CI_EXPAND) != 0)
      {
         if (ci_ptr->bit_depth < 16)
            max_pixel_depth *= 2;
      }
      else
#  endif
      ci_ptr->transformations &= ~CI_EXPAND_16;
   }
#endif

#ifdef CI_READ_FILLER_SUPPORTED
   if ((ci_ptr->transformations & (CI_FILLER)) != 0)
   {
      if (ci_ptr->color_type == CI_COLOR_TYPE_GRAY)
      {
         if (max_pixel_depth <= 8)
            max_pixel_depth = 16;

         else
            max_pixel_depth = 32;
      }

      else if (ci_ptr->color_type == CI_COLOR_TYPE_RGB ||
         ci_ptr->color_type == CI_COLOR_TYPE_PALETTE)
      {
         if (max_pixel_depth <= 32)
            max_pixel_depth = 32;

         else
            max_pixel_depth = 64;
      }
   }
#endif

#ifdef CI_READ_GRAY_TO_RGB_SUPPORTED
   if ((ci_ptr->transformations & CI_GRAY_TO_RGB) != 0)
   {
      if (
#ifdef CI_READ_EXPAND_SUPPORTED
          (ci_ptr->num_trans != 0 &&
          (ci_ptr->transformations & CI_EXPAND) != 0) ||
#endif
#ifdef CI_READ_FILLER_SUPPORTED
          (ci_ptr->transformations & (CI_FILLER)) != 0 ||
#endif
          ci_ptr->color_type == CI_COLOR_TYPE_GRAY_ALPHA)
      {
         if (max_pixel_depth <= 16)
            max_pixel_depth = 32;

         else
            max_pixel_depth = 64;
      }

      else
      {
         if (max_pixel_depth <= 8)
         {
            if (ci_ptr->color_type == CI_COLOR_TYPE_RGB_ALPHA)
               max_pixel_depth = 32;

            else
               max_pixel_depth = 24;
         }

         else if (ci_ptr->color_type == CI_COLOR_TYPE_RGB_ALPHA)
            max_pixel_depth = 64;

         else
            max_pixel_depth = 48;
      }
   }
#endif

#if defined(CI_READ_USER_TRANSFORM_SUPPORTED) && \
defined(CI_USER_TRANSFORM_PTR_SUPPORTED)
   if ((ci_ptr->transformations & CI_USER_TRANSFORM) != 0)
   {
      unsigned int user_pixel_depth = ci_ptr->user_transform_depth *
         ci_ptr->user_transform_channels;

      if (user_pixel_depth > max_pixel_depth)
         max_pixel_depth = user_pixel_depth;
   }
#endif

   /* This value is stored in ci_struct and double checked in the row read
    * code.
    */
   ci_ptr->maximum_pixel_depth = (ci_byte)max_pixel_depth;
   ci_ptr->transformed_pixel_depth = 0; /* calculated on demand */

   /* Align the width on the next larger 8 pixels.  Mainly used
    * for interlacing
    */
   row_bytes = ((ci_ptr->width + 7) & ~((ci_uint_32)7));
   /* Calculate the maximum bytes needed, adding a byte and a pixel
    * for safety's sake
    */
   row_bytes = CI_ROWBYTES(max_pixel_depth, row_bytes) +
       1 + ((max_pixel_depth + 7) >> 3U);

#ifdef CI_MAX_MALLOC_64K
   if (row_bytes > (ci_uint_32)65536L)
      ci_error(ci_ptr, "This image requires a row greater than 64KB");
#endif

   if (row_bytes + 48 > ci_ptr->old_big_row_buf_size)
   {
      ci_free(ci_ptr, ci_ptr->big_row_buf);
      ci_free(ci_ptr, ci_ptr->big_prev_row);

      if (ci_ptr->interlaced != 0)
         ci_ptr->big_row_buf = (ci_bytep)ci_calloc(ci_ptr,
             row_bytes + 48);

      else
         ci_ptr->big_row_buf = (ci_bytep)ci_malloc(ci_ptr, row_bytes + 48);

      ci_ptr->big_prev_row = (ci_bytep)ci_malloc(ci_ptr, row_bytes + 48);

#ifdef CI_ALIGNED_MEMORY_SUPPORTED
      /* Use 16-byte aligned memory for row_buf with at least 16 bytes
       * of padding before and after row_buf; treat prev_row similarly.
       * NOTE: the alignment is to the start of the pixels, one beyond the start
       * of the buffer, because of the filter byte.  Prior to libci 1.5.6 this
       * was incorrect; the filter byte was aligned, which had the exact
       * opposite effect of that intended.
       */
      {
         ci_bytep temp = ci_ptr->big_row_buf + 32;
         size_t extra = (size_t)temp & 0x0f;
         ci_ptr->row_buf = temp - extra - 1/*filter byte*/;

         temp = ci_ptr->big_prev_row + 32;
         extra = (size_t)temp & 0x0f;
         ci_ptr->prev_row = temp - extra - 1/*filter byte*/;
      }
#else
      /* Use 31 bytes of padding before and 17 bytes after row_buf. */
      ci_ptr->row_buf = ci_ptr->big_row_buf + 31;
      ci_ptr->prev_row = ci_ptr->big_prev_row + 31;
#endif
      ci_ptr->old_big_row_buf_size = row_bytes + 48;
   }

#ifdef CI_MAX_MALLOC_64K
   if (ci_ptr->rowbytes > 65535)
      ci_error(ci_ptr, "This image requires a row greater than 64KB");

#endif
   if (ci_ptr->rowbytes > (CI_SIZE_MAX - 1))
      ci_error(ci_ptr, "Row has too many bytes to allocate in memory");

   memset(ci_ptr->prev_row, 0, ci_ptr->rowbytes + 1);

   ci_debug1(3, "width = %u,", ci_ptr->width);
   ci_debug1(3, "height = %u,", ci_ptr->height);
   ci_debug1(3, "iwidth = %u,", ci_ptr->iwidth);
   ci_debug1(3, "num_rows = %u,", ci_ptr->num_rows);
   ci_debug1(3, "rowbytes = %lu,", (unsigned long)ci_ptr->rowbytes);
   ci_debug1(3, "irowbytes = %lu",
       (unsigned long)CI_ROWBYTES(ci_ptr->pixel_depth, ci_ptr->iwidth) + 1);

   /* The sequential reader needs a buffer for IDAT, but the progressive reader
    * does not, so free the read buffer now regardless; the sequential reader
    * reallocates it on demand.
    */
   if (ci_ptr->read_buffer != NULL)
   {
      ci_bytep buffer = ci_ptr->read_buffer;

      ci_ptr->read_buffer_size = 0;
      ci_ptr->read_buffer = NULL;
      ci_free(ci_ptr, buffer);
   }

   /* Finally claim the zstream for the inflate of the IDAT data, use the bits
    * value from the stream (note that this will result in a fatal error if the
    * IDAT stream has a bogus deflate header window_bits value, but this should
    * not be happening any longer!)
    */
   if (ci_inflate_claim(ci_ptr, ci_IDAT) != Z_OK)
      ci_error(ci_ptr, ci_ptr->zstream.msg);

   ci_ptr->flags |= CI_FLAG_ROW_INIT;
}
#endif /* READ */

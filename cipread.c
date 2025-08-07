/* cipread.c - read a ci file in push mode
 *
 * Copyright (c) 2018-2025 Cosmin Truta
 * Copyright (c) 1998-2002,2004,2006-2018 Glenn Randers-Pehrson
 * Copyright (c) 1996-1997 Andreas Dilger
 * Copyright (c) 1995-1996 Guy Eric Schalnat, Group 42, Inc.
 *
 * This code is released under the libci license.
 * For conditions of distribution and use, see the disclaimer
 * and license in ci.h
 */

#include "cipriv.h"

#ifdef CI_PROGRESSIVE_READ_SUPPORTED

/* Push model modes */
#define CI_READ_SIG_MODE   0
#define CI_READ_CHUNK_MODE 1
#define CI_READ_IDAT_MODE  2
#define CI_READ_tEXt_MODE  4
#define CI_READ_zTXt_MODE  5
#define CI_READ_DONE_MODE  6
#define CI_READ_iTXt_MODE  7
#define CI_ERROR_MODE      8

#define CI_PUSH_SAVE_BUFFER_IF_FULL \
if (ci_ptr->push_length + 4 > ci_ptr->buffer_size) \
   { ci_push_save_buffer(ci_ptr); return; }
#define CI_PUSH_SAVE_BUFFER_IF_LT(N) \
if (ci_ptr->buffer_size < N) \
   { ci_push_save_buffer(ci_ptr); return; }

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

void CIAPI
ci_process_data(ci_structrp ci_ptr, ci_inforp info_ptr,
    ci_bytep buffer, size_t buffer_size)
{
   if (ci_ptr == NULL || info_ptr == NULL)
      return;

   ci_push_restore_buffer(ci_ptr, buffer, buffer_size);

   while (ci_ptr->buffer_size)
   {
      ci_process_some_data(ci_ptr, info_ptr);
   }
}

size_t CIAPI
ci_process_data_pause(ci_structrp ci_ptr, int save)
{
   if (ci_ptr != NULL)
   {
      /* It's easiest for the caller if we do the save; then the caller doesn't
       * have to supply the same data again:
       */
      if (save != 0)
         ci_push_save_buffer(ci_ptr);
      else
      {
         /* This includes any pending saved bytes: */
         size_t remaining = ci_ptr->buffer_size;
         ci_ptr->buffer_size = 0;

         /* So subtract the saved buffer size, unless all the data
          * is actually 'saved', in which case we just return 0
          */
         if (ci_ptr->save_buffer_size < remaining)
            return remaining - ci_ptr->save_buffer_size;
      }
   }

   return 0;
}

ci_uint_32 CIAPI
ci_process_data_skip(ci_structrp ci_ptr)
{
/* TODO: Deprecate and remove this API.
 * Somewhere the implementation of this seems to have been lost,
 * or abandoned.  It was only to support some internal back-door access
 * to ci_struct) in libci-1.4.x.
 */
   ci_app_warning(ci_ptr,
"ci_process_data_skip is not implemented in any current version of libci");
   return 0;
}

/* What we do with the incoming data depends on what we were previously
 * doing before we ran out of data...
 */
void /* PRIVATE */
ci_process_some_data(ci_structrp ci_ptr, ci_inforp info_ptr)
{
   if (ci_ptr == NULL)
      return;

   switch (ci_ptr->process_mode)
   {
      case CI_READ_SIG_MODE:
      {
         ci_push_read_sig(ci_ptr, info_ptr);
         break;
      }

      case CI_READ_CHUNK_MODE:
      {
         ci_push_read_chunk(ci_ptr, info_ptr);
         break;
      }

      case CI_READ_IDAT_MODE:
      {
         ci_push_read_IDAT(ci_ptr);
         break;
      }

      default:
      {
         ci_ptr->buffer_size = 0;
         break;
      }
   }
}

/* Read any remaining signature bytes from the stream and compare them with
 * the correct CI signature.  It is possible that this routine is called
 * with bytes already read from the signature, either because they have been
 * checked by the calling application, or because of multiple calls to this
 * routine.
 */
void /* PRIVATE */
ci_push_read_sig(ci_structrp ci_ptr, ci_inforp info_ptr)
{
   size_t num_checked = ci_ptr->sig_bytes; /* SAFE, does not exceed 8 */
   size_t num_to_check = 8 - num_checked;

   if (ci_ptr->buffer_size < num_to_check)
   {
      num_to_check = ci_ptr->buffer_size;
   }

   ci_push_fill_buffer(ci_ptr, &(info_ptr->signature[num_checked]),
       num_to_check);
   ci_ptr->sig_bytes = (ci_byte)(ci_ptr->sig_bytes + num_to_check);

   if (ci_sig_cmp(info_ptr->signature, num_checked, num_to_check) != 0)
   {
      if (num_checked < 4 &&
          ci_sig_cmp(info_ptr->signature, num_checked, num_to_check - 4) != 0)
         ci_error(ci_ptr, "Not a CI file");

      else
         ci_error(ci_ptr, "CI file corrupted by ASCII conversion");
   }
   else
   {
      if (ci_ptr->sig_bytes >= 8)
      {
         ci_ptr->process_mode = CI_READ_CHUNK_MODE;
      }
   }
}

void /* PRIVATE */
ci_push_read_chunk(ci_structrp ci_ptr, ci_inforp info_ptr)
{
   ci_uint_32 chunk_name;
#ifdef CI_HANDLE_AS_UNKNOWN_SUPPORTED
   int keep; /* unknown handling method */
#endif

   /* First we make sure we have enough data for the 4-byte chunk name
    * and the 4-byte chunk length before proceeding with decoding the
    * chunk data.  To fully decode each of these chunks, we also make
    * sure we have enough data in the buffer for the 4-byte CRC at the
    * end of every chunk (except IDAT, which is handled separately).
    */
   if ((ci_ptr->mode & CI_HAVE_CHUNK_HEADER) == 0)
   {
      CI_PUSH_SAVE_BUFFER_IF_LT(8)
      ci_ptr->push_length = ci_read_chunk_header(ci_ptr);
      ci_ptr->mode |= CI_HAVE_CHUNK_HEADER;
   }

   chunk_name = ci_ptr->chunk_name;

   if (chunk_name == ci_IDAT)
   {
      if ((ci_ptr->mode & CI_AFTER_IDAT) != 0)
         ci_ptr->mode |= CI_HAVE_CHUNK_AFTER_IDAT;

      /* If we reach an IDAT chunk, this means we have read all of the
       * header chunks, and we can start reading the image (or if this
       * is called after the image has been read - we have an error).
       */
      if ((ci_ptr->mode & CI_HAVE_IHDR) == 0)
         ci_error(ci_ptr, "Missing IHDR before IDAT");

      else if (ci_ptr->color_type == CI_COLOR_TYPE_PALETTE &&
          (ci_ptr->mode & CI_HAVE_PLTE) == 0)
         ci_error(ci_ptr, "Missing PLTE before IDAT");

      ci_ptr->process_mode = CI_READ_IDAT_MODE;

      if ((ci_ptr->mode & CI_HAVE_IDAT) != 0)
         if ((ci_ptr->mode & CI_HAVE_CHUNK_AFTER_IDAT) == 0)
            if (ci_ptr->push_length == 0)
               return;

      ci_ptr->mode |= CI_HAVE_IDAT;

      if ((ci_ptr->mode & CI_AFTER_IDAT) != 0)
         ci_benign_error(ci_ptr, "Too many IDATs found");
   }

   if (chunk_name == ci_IHDR)
   {
      if (ci_ptr->push_length != 13)
         ci_error(ci_ptr, "Invalid IHDR length");

      CI_PUSH_SAVE_BUFFER_IF_FULL
      ci_handle_chunk(ci_ptr, info_ptr, ci_ptr->push_length);
   }

   else if (chunk_name == ci_IEND)
   {
      CI_PUSH_SAVE_BUFFER_IF_FULL
      ci_handle_chunk(ci_ptr, info_ptr, ci_ptr->push_length);

      ci_ptr->process_mode = CI_READ_DONE_MODE;
      ci_push_have_end(ci_ptr, info_ptr);
   }

#ifdef CI_HANDLE_AS_UNKNOWN_SUPPORTED
   else if ((keep = ci_chunk_unknown_handling(ci_ptr, chunk_name)) != 0)
   {
      CI_PUSH_SAVE_BUFFER_IF_FULL
      ci_handle_unknown(ci_ptr, info_ptr, ci_ptr->push_length, keep);

      if (chunk_name == ci_PLTE)
         ci_ptr->mode |= CI_HAVE_PLTE;
   }
#endif

   else if (chunk_name == ci_IDAT)
   {
      ci_ptr->idat_size = ci_ptr->push_length;
      ci_ptr->process_mode = CI_READ_IDAT_MODE;
      ci_push_have_info(ci_ptr, info_ptr);
      ci_ptr->zstream.avail_out =
          (uInt) CI_ROWBYTES(ci_ptr->pixel_depth,
          ci_ptr->iwidth) + 1;
      ci_ptr->zstream.next_out = ci_ptr->row_buf;
      return;
   }

   else
   {
      CI_PUSH_SAVE_BUFFER_IF_FULL
      ci_handle_chunk(ci_ptr, info_ptr, ci_ptr->push_length);
   }

   ci_ptr->mode &= ~CI_HAVE_CHUNK_HEADER;
}

void CICBAPI
ci_push_fill_buffer(ci_structp ci_ptr, ci_bytep buffer, size_t length)
{
   ci_bytep ptr;

   if (ci_ptr == NULL)
      return;

   ptr = buffer;
   if (ci_ptr->save_buffer_size != 0)
   {
      size_t save_size;

      if (length < ci_ptr->save_buffer_size)
         save_size = length;

      else
         save_size = ci_ptr->save_buffer_size;

      memcpy(ptr, ci_ptr->save_buffer_ptr, save_size);
      length -= save_size;
      ptr += save_size;
      ci_ptr->buffer_size -= save_size;
      ci_ptr->save_buffer_size -= save_size;
      ci_ptr->save_buffer_ptr += save_size;
   }
   if (length != 0 && ci_ptr->current_buffer_size != 0)
   {
      size_t save_size;

      if (length < ci_ptr->current_buffer_size)
         save_size = length;

      else
         save_size = ci_ptr->current_buffer_size;

      memcpy(ptr, ci_ptr->current_buffer_ptr, save_size);
      ci_ptr->buffer_size -= save_size;
      ci_ptr->current_buffer_size -= save_size;
      ci_ptr->current_buffer_ptr += save_size;
   }
}

void /* PRIVATE */
ci_push_save_buffer(ci_structrp ci_ptr)
{
   if (ci_ptr->save_buffer_size != 0)
   {
      if (ci_ptr->save_buffer_ptr != ci_ptr->save_buffer)
      {
         size_t i, istop;
         ci_bytep sp;
         ci_bytep dp;

         istop = ci_ptr->save_buffer_size;
         for (i = 0, sp = ci_ptr->save_buffer_ptr, dp = ci_ptr->save_buffer;
             i < istop; i++, sp++, dp++)
         {
            *dp = *sp;
         }
      }
   }
   if (ci_ptr->save_buffer_size + ci_ptr->current_buffer_size >
       ci_ptr->save_buffer_max)
   {
      size_t new_max;
      ci_bytep old_buffer;

      if (ci_ptr->save_buffer_size > CI_SIZE_MAX -
          (ci_ptr->current_buffer_size + 256))
      {
         ci_error(ci_ptr, "Potential overflow of save_buffer");
      }

      new_max = ci_ptr->save_buffer_size + ci_ptr->current_buffer_size + 256;
      old_buffer = ci_ptr->save_buffer;
      ci_ptr->save_buffer = (ci_bytep)ci_malloc_warn(ci_ptr,
          (size_t)new_max);

      if (ci_ptr->save_buffer == NULL)
      {
         ci_free(ci_ptr, old_buffer);
         ci_error(ci_ptr, "Insufficient memory for save_buffer");
      }

      if (old_buffer)
         memcpy(ci_ptr->save_buffer, old_buffer, ci_ptr->save_buffer_size);
      else if (ci_ptr->save_buffer_size)
         ci_error(ci_ptr, "save_buffer error");
      ci_free(ci_ptr, old_buffer);
      ci_ptr->save_buffer_max = new_max;
   }
   if (ci_ptr->current_buffer_size)
   {
      memcpy(ci_ptr->save_buffer + ci_ptr->save_buffer_size,
         ci_ptr->current_buffer_ptr, ci_ptr->current_buffer_size);
      ci_ptr->save_buffer_size += ci_ptr->current_buffer_size;
      ci_ptr->current_buffer_size = 0;
   }
   ci_ptr->save_buffer_ptr = ci_ptr->save_buffer;
   ci_ptr->buffer_size = 0;
}

void /* PRIVATE */
ci_push_restore_buffer(ci_structrp ci_ptr, ci_bytep buffer,
    size_t buffer_length)
{
   ci_ptr->current_buffer = buffer;
   ci_ptr->current_buffer_size = buffer_length;
   ci_ptr->buffer_size = buffer_length + ci_ptr->save_buffer_size;
   ci_ptr->current_buffer_ptr = ci_ptr->current_buffer;
}

void /* PRIVATE */
ci_push_read_IDAT(ci_structrp ci_ptr)
{
   if ((ci_ptr->mode & CI_HAVE_CHUNK_HEADER) == 0)
   {
      ci_byte chunk_length[4];
      ci_byte chunk_tag[4];

      /* TODO: this code can be commoned up with the same code in push_read */
      CI_PUSH_SAVE_BUFFER_IF_LT(8)
      ci_push_fill_buffer(ci_ptr, chunk_length, 4);
      ci_ptr->push_length = ci_get_uint_31(ci_ptr, chunk_length);
      ci_reset_crc(ci_ptr);
      ci_crc_read(ci_ptr, chunk_tag, 4);
      ci_ptr->chunk_name = CI_CHUNK_FROM_STRING(chunk_tag);
      ci_ptr->mode |= CI_HAVE_CHUNK_HEADER;

      if (ci_ptr->chunk_name != ci_IDAT)
      {
         ci_ptr->process_mode = CI_READ_CHUNK_MODE;

         if ((ci_ptr->flags & CI_FLAG_ZSTREAM_ENDED) == 0)
            ci_error(ci_ptr, "Not enough compressed data");

         return;
      }

      ci_ptr->idat_size = ci_ptr->push_length;
   }

   if (ci_ptr->idat_size != 0 && ci_ptr->save_buffer_size != 0)
   {
      size_t save_size = ci_ptr->save_buffer_size;
      ci_uint_32 idat_size = ci_ptr->idat_size;

      /* We want the smaller of 'idat_size' and 'current_buffer_size', but they
       * are of different types and we don't know which variable has the fewest
       * bits.  Carefully select the smaller and cast it to the type of the
       * larger - this cannot overflow.  Do not cast in the following test - it
       * will break on either 16-bit or 64-bit platforms.
       */
      if (idat_size < save_size)
         save_size = (size_t)idat_size;

      else
         idat_size = (ci_uint_32)save_size;

      ci_calculate_crc(ci_ptr, ci_ptr->save_buffer_ptr, save_size);

      ci_process_IDAT_data(ci_ptr, ci_ptr->save_buffer_ptr, save_size);

      ci_ptr->idat_size -= idat_size;
      ci_ptr->buffer_size -= save_size;
      ci_ptr->save_buffer_size -= save_size;
      ci_ptr->save_buffer_ptr += save_size;
   }

   if (ci_ptr->idat_size != 0 && ci_ptr->current_buffer_size != 0)
   {
      size_t save_size = ci_ptr->current_buffer_size;
      ci_uint_32 idat_size = ci_ptr->idat_size;

      /* We want the smaller of 'idat_size' and 'current_buffer_size', but they
       * are of different types and we don't know which variable has the fewest
       * bits.  Carefully select the smaller and cast it to the type of the
       * larger - this cannot overflow.
       */
      if (idat_size < save_size)
         save_size = (size_t)idat_size;

      else
         idat_size = (ci_uint_32)save_size;

      ci_calculate_crc(ci_ptr, ci_ptr->current_buffer_ptr, save_size);

      ci_process_IDAT_data(ci_ptr, ci_ptr->current_buffer_ptr, save_size);

      ci_ptr->idat_size -= idat_size;
      ci_ptr->buffer_size -= save_size;
      ci_ptr->current_buffer_size -= save_size;
      ci_ptr->current_buffer_ptr += save_size;
   }

   if (ci_ptr->idat_size == 0)
   {
      CI_PUSH_SAVE_BUFFER_IF_LT(4)
      ci_crc_finish(ci_ptr, 0);
      ci_ptr->mode &= ~CI_HAVE_CHUNK_HEADER;
      ci_ptr->mode |= CI_AFTER_IDAT;
      ci_ptr->zowner = 0;
   }
}

void /* PRIVATE */
ci_process_IDAT_data(ci_structrp ci_ptr, ci_bytep buffer,
    size_t buffer_length)
{
   /* The caller checks for a non-zero buffer length. */
   if (!(buffer_length > 0) || buffer == NULL)
      ci_error(ci_ptr, "No IDAT data (internal error)");

   /* This routine must process all the data it has been given
    * before returning, calling the row callback as required to
    * handle the uncompressed results.
    */
   ci_ptr->zstream.next_in = buffer;
   /* TODO: WARNING: TRUNCATION ERROR: DANGER WILL ROBINSON: */
   ci_ptr->zstream.avail_in = (uInt)buffer_length;

   /* Keep going until the decompressed data is all processed
    * or the stream marked as finished.
    */
   while (ci_ptr->zstream.avail_in > 0 &&
      (ci_ptr->flags & CI_FLAG_ZSTREAM_ENDED) == 0)
   {
      int ret;

      /* We have data for zlib, but we must check that zlib
       * has someplace to put the results.  It doesn't matter
       * if we don't expect any results -- it may be the input
       * data is just the LZ end code.
       */
      if (!(ci_ptr->zstream.avail_out > 0))
      {
         /* TODO: WARNING: TRUNCATION ERROR: DANGER WILL ROBINSON: */
         ci_ptr->zstream.avail_out = (uInt)(CI_ROWBYTES(ci_ptr->pixel_depth,
             ci_ptr->iwidth) + 1);

         ci_ptr->zstream.next_out = ci_ptr->row_buf;
      }

      /* Using Z_SYNC_FLUSH here means that an unterminated
       * LZ stream (a stream with a missing end code) can still
       * be handled, otherwise (Z_NO_FLUSH) a future zlib
       * implementation might defer output and therefore
       * change the current behavior (see comments in inflate.c
       * for why this doesn't happen at present with zlib 1.2.5).
       */
      ret = CI_INFLATE(ci_ptr, Z_SYNC_FLUSH);

      /* Check for any failure before proceeding. */
      if (ret != Z_OK && ret != Z_STREAM_END)
      {
         /* Terminate the decompression. */
         ci_ptr->flags |= CI_FLAG_ZSTREAM_ENDED;
         ci_ptr->zowner = 0;

         /* This may be a truncated stream (missing or
          * damaged end code).  Treat that as a warning.
          */
         if (ci_ptr->row_number >= ci_ptr->num_rows ||
             ci_ptr->pass > 6)
            ci_warning(ci_ptr, "Truncated compressed data in IDAT");

         else
         {
            if (ret == Z_DATA_ERROR)
               ci_benign_error(ci_ptr, "IDAT: ADLER32 checksum mismatch");
            else
               ci_error(ci_ptr, "Decompression error in IDAT");
         }

         /* Skip the check on unprocessed input */
         return;
      }

      /* Did inflate output any data? */
      if (ci_ptr->zstream.next_out != ci_ptr->row_buf)
      {
         /* Is this unexpected data after the last row?
          * If it is, artificially terminate the LZ output
          * here.
          */
         if (ci_ptr->row_number >= ci_ptr->num_rows ||
             ci_ptr->pass > 6)
         {
            /* Extra data. */
            ci_warning(ci_ptr, "Extra compressed data in IDAT");
            ci_ptr->flags |= CI_FLAG_ZSTREAM_ENDED;
            ci_ptr->zowner = 0;

            /* Do no more processing; skip the unprocessed
             * input check below.
             */
            return;
         }

         /* Do we have a complete row? */
         if (ci_ptr->zstream.avail_out == 0)
            ci_push_process_row(ci_ptr);
      }

      /* And check for the end of the stream. */
      if (ret == Z_STREAM_END)
         ci_ptr->flags |= CI_FLAG_ZSTREAM_ENDED;
   }

   /* All the data should have been processed, if anything
    * is left at this point we have bytes of IDAT data
    * after the zlib end code.
    */
   if (ci_ptr->zstream.avail_in > 0)
      ci_warning(ci_ptr, "Extra compression data in IDAT");
}

void /* PRIVATE */
ci_push_process_row(ci_structrp ci_ptr)
{
   /* 1.5.6: row_info moved out of ci_struct to a local here. */
   ci_row_info row_info;

   row_info.width = ci_ptr->iwidth; /* NOTE: width of current interlaced row */
   row_info.color_type = ci_ptr->color_type;
   row_info.bit_depth = ci_ptr->bit_depth;
   row_info.channels = ci_ptr->channels;
   row_info.pixel_depth = ci_ptr->pixel_depth;
   row_info.rowbytes = CI_ROWBYTES(row_info.pixel_depth, row_info.width);

   if (ci_ptr->row_buf[0] > CI_FILTER_VALUE_NONE)
   {
      if (ci_ptr->row_buf[0] < CI_FILTER_VALUE_LAST)
         ci_read_filter_row(ci_ptr, &row_info, ci_ptr->row_buf + 1,
            ci_ptr->prev_row + 1, ci_ptr->row_buf[0]);
      else
         ci_error(ci_ptr, "bad adaptive filter value");
   }

   /* libci 1.5.6: the following line was copying ci_ptr->rowbytes before
    * 1.5.6, while the buffer really is this big in current versions of libci
    * it may not be in the future, so this was changed just to copy the
    * interlaced row count:
    */
   memcpy(ci_ptr->prev_row, ci_ptr->row_buf, row_info.rowbytes + 1);

#ifdef CI_READ_TRANSFORMS_SUPPORTED
   if (ci_ptr->transformations != 0)
      ci_do_read_transformations(ci_ptr, &row_info);
#endif

   /* The transformed pixel depth should match the depth now in row_info. */
   if (ci_ptr->transformed_pixel_depth == 0)
   {
      ci_ptr->transformed_pixel_depth = row_info.pixel_depth;
      if (row_info.pixel_depth > ci_ptr->maximum_pixel_depth)
         ci_error(ci_ptr, "progressive row overflow");
   }

   else if (ci_ptr->transformed_pixel_depth != row_info.pixel_depth)
      ci_error(ci_ptr, "internal progressive row size calculation error");


#ifdef CI_READ_INTERLACING_SUPPORTED
   /* Expand interlaced rows to full size */
   if (ci_ptr->interlaced != 0 &&
       (ci_ptr->transformations & CI_INTERLACE) != 0)
   {
      if (ci_ptr->pass < 6)
         ci_do_read_interlace(&row_info, ci_ptr->row_buf + 1, ci_ptr->pass,
             ci_ptr->transformations);

      switch (ci_ptr->pass)
      {
         case 0:
         {
            int i;
            for (i = 0; i < 8 && ci_ptr->pass == 0; i++)
            {
               ci_push_have_row(ci_ptr, ci_ptr->row_buf + 1);
               ci_read_push_finish_row(ci_ptr); /* Updates ci_ptr->pass */
            }

            if (ci_ptr->pass == 2) /* Pass 1 might be empty */
            {
               for (i = 0; i < 4 && ci_ptr->pass == 2; i++)
               {
                  ci_push_have_row(ci_ptr, NULL);
                  ci_read_push_finish_row(ci_ptr);
               }
            }

            if (ci_ptr->pass == 4 && ci_ptr->height <= 4)
            {
               for (i = 0; i < 2 && ci_ptr->pass == 4; i++)
               {
                  ci_push_have_row(ci_ptr, NULL);
                  ci_read_push_finish_row(ci_ptr);
               }
            }

            if (ci_ptr->pass == 6 && ci_ptr->height <= 4)
            {
                ci_push_have_row(ci_ptr, NULL);
                ci_read_push_finish_row(ci_ptr);
            }

            break;
         }

         case 1:
         {
            int i;
            for (i = 0; i < 8 && ci_ptr->pass == 1; i++)
            {
               ci_push_have_row(ci_ptr, ci_ptr->row_buf + 1);
               ci_read_push_finish_row(ci_ptr);
            }

            if (ci_ptr->pass == 2) /* Skip top 4 generated rows */
            {
               for (i = 0; i < 4 && ci_ptr->pass == 2; i++)
               {
                  ci_push_have_row(ci_ptr, NULL);
                  ci_read_push_finish_row(ci_ptr);
               }
            }

            break;
         }

         case 2:
         {
            int i;

            for (i = 0; i < 4 && ci_ptr->pass == 2; i++)
            {
               ci_push_have_row(ci_ptr, ci_ptr->row_buf + 1);
               ci_read_push_finish_row(ci_ptr);
            }

            for (i = 0; i < 4 && ci_ptr->pass == 2; i++)
            {
               ci_push_have_row(ci_ptr, NULL);
               ci_read_push_finish_row(ci_ptr);
            }

            if (ci_ptr->pass == 4) /* Pass 3 might be empty */
            {
               for (i = 0; i < 2 && ci_ptr->pass == 4; i++)
               {
                  ci_push_have_row(ci_ptr, NULL);
                  ci_read_push_finish_row(ci_ptr);
               }
            }

            break;
         }

         case 3:
         {
            int i;

            for (i = 0; i < 4 && ci_ptr->pass == 3; i++)
            {
               ci_push_have_row(ci_ptr, ci_ptr->row_buf + 1);
               ci_read_push_finish_row(ci_ptr);
            }

            if (ci_ptr->pass == 4) /* Skip top two generated rows */
            {
               for (i = 0; i < 2 && ci_ptr->pass == 4; i++)
               {
                  ci_push_have_row(ci_ptr, NULL);
                  ci_read_push_finish_row(ci_ptr);
               }
            }

            break;
         }

         case 4:
         {
            int i;

            for (i = 0; i < 2 && ci_ptr->pass == 4; i++)
            {
               ci_push_have_row(ci_ptr, ci_ptr->row_buf + 1);
               ci_read_push_finish_row(ci_ptr);
            }

            for (i = 0; i < 2 && ci_ptr->pass == 4; i++)
            {
               ci_push_have_row(ci_ptr, NULL);
               ci_read_push_finish_row(ci_ptr);
            }

            if (ci_ptr->pass == 6) /* Pass 5 might be empty */
            {
               ci_push_have_row(ci_ptr, NULL);
               ci_read_push_finish_row(ci_ptr);
            }

            break;
         }

         case 5:
         {
            int i;

            for (i = 0; i < 2 && ci_ptr->pass == 5; i++)
            {
               ci_push_have_row(ci_ptr, ci_ptr->row_buf + 1);
               ci_read_push_finish_row(ci_ptr);
            }

            if (ci_ptr->pass == 6) /* Skip top generated row */
            {
               ci_push_have_row(ci_ptr, NULL);
               ci_read_push_finish_row(ci_ptr);
            }

            break;
         }

         default:
         case 6:
         {
            ci_push_have_row(ci_ptr, ci_ptr->row_buf + 1);
            ci_read_push_finish_row(ci_ptr);

            if (ci_ptr->pass != 6)
               break;

            ci_push_have_row(ci_ptr, NULL);
            ci_read_push_finish_row(ci_ptr);
         }
      }
   }
   else
#endif
   {
      ci_push_have_row(ci_ptr, ci_ptr->row_buf + 1);
      ci_read_push_finish_row(ci_ptr);
   }
}

void /* PRIVATE */
ci_read_push_finish_row(ci_structrp ci_ptr)
{
   ci_ptr->row_number++;
   if (ci_ptr->row_number < ci_ptr->num_rows)
      return;

#ifdef CI_READ_INTERLACING_SUPPORTED
   if (ci_ptr->interlaced != 0)
   {
      ci_ptr->row_number = 0;
      memset(ci_ptr->prev_row, 0, ci_ptr->rowbytes + 1);

      do
      {
         ci_ptr->pass++;
         if ((ci_ptr->pass == 1 && ci_ptr->width < 5) ||
             (ci_ptr->pass == 3 && ci_ptr->width < 3) ||
             (ci_ptr->pass == 5 && ci_ptr->width < 2))
            ci_ptr->pass++;

         if (ci_ptr->pass > 7)
            ci_ptr->pass--;

         if (ci_ptr->pass >= 7)
            break;

         ci_ptr->iwidth = (ci_ptr->width +
             ci_pass_inc[ci_ptr->pass] - 1 -
             ci_pass_start[ci_ptr->pass]) /
             ci_pass_inc[ci_ptr->pass];

         if ((ci_ptr->transformations & CI_INTERLACE) != 0)
            break;

         ci_ptr->num_rows = (ci_ptr->height +
             ci_pass_yinc[ci_ptr->pass] - 1 -
             ci_pass_ystart[ci_ptr->pass]) /
             ci_pass_yinc[ci_ptr->pass];

      } while (ci_ptr->iwidth == 0 || ci_ptr->num_rows == 0);
   }
#endif /* READ_INTERLACING */
}

void /* PRIVATE */
ci_push_have_info(ci_structrp ci_ptr, ci_inforp info_ptr)
{
   if (ci_ptr->info_fn != NULL)
      (*(ci_ptr->info_fn))(ci_ptr, info_ptr);
}

void /* PRIVATE */
ci_push_have_end(ci_structrp ci_ptr, ci_inforp info_ptr)
{
   if (ci_ptr->end_fn != NULL)
      (*(ci_ptr->end_fn))(ci_ptr, info_ptr);
}

void /* PRIVATE */
ci_push_have_row(ci_structrp ci_ptr, ci_bytep row)
{
   if (ci_ptr->row_fn != NULL)
      (*(ci_ptr->row_fn))(ci_ptr, row, ci_ptr->row_number,
          (int)ci_ptr->pass);
}

#ifdef CI_READ_INTERLACING_SUPPORTED
void CIAPI
ci_progressive_combine_row(ci_const_structrp ci_ptr, ci_bytep old_row,
    ci_const_bytep new_row)
{
   if (ci_ptr == NULL)
      return;

   /* new_row is a flag here - if it is NULL then the app callback was called
    * from an empty row (see the calls to ci_struct::row_fn below), otherwise
    * it must be ci_ptr->row_buf+1
    */
   if (new_row != NULL)
      ci_combine_row(ci_ptr, old_row, 1/*blocky display*/);
}
#endif /* READ_INTERLACING */

void CIAPI
ci_set_progressive_read_fn(ci_structrp ci_ptr, ci_voidp progressive_ptr,
    ci_progressive_info_ptr info_fn, ci_progressive_row_ptr row_fn,
    ci_progressive_end_ptr end_fn)
{
   if (ci_ptr == NULL)
      return;

   ci_ptr->info_fn = info_fn;
   ci_ptr->row_fn = row_fn;
   ci_ptr->end_fn = end_fn;

   ci_set_read_fn(ci_ptr, progressive_ptr, ci_push_fill_buffer);
}

ci_voidp CIAPI
ci_get_progressive_ptr(ci_const_structrp ci_ptr)
{
   if (ci_ptr == NULL)
      return NULL;

   return ci_ptr->io_ptr;
}
#endif /* PROGRESSIVE_READ */

/* cistruct.h - internal structures for libci
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

#ifndef CIPRIV_H
#  error This file must not be included by applications; please include <ci.h>
#endif

#ifndef CISTRUCT_H
#define CISTRUCT_H
/* zlib.h defines the structure z_stream, an instance of which is included
 * in this structure and is required for decompressing the LZ compressed
 * data in CI files.
 */
#ifndef ZLIB_CONST
   /* We must ensure that zlib uses 'const' in declarations. */
#  define ZLIB_CONST
#endif
#include "zlib.h"
#ifdef const
   /* zlib.h sometimes #defines const to nothing, undo this. */
#  undef const
#endif

/* zlib.h has mediocre z_const use before 1.2.6, this stuff is for compatibility
 * with older builds.
 */
#if ZLIB_VERNUM < 0x1260
#  define CIZ_MSG_CAST(s) ci_constcast(char*,s)
#  define CIZ_INPUT_CAST(b) ci_constcast(ci_bytep,b)
#else
#  define CIZ_MSG_CAST(s) (s)
#  define CIZ_INPUT_CAST(b) (b)
#endif

/* zlib.h declares a magic type 'uInt' that limits the amount of data that zlib
 * can handle at once.  This type need be no larger than 16 bits (so maximum of
 * 65535), this define allows us to discover how big it is, but limited by the
 * maximum for size_t.  The value can be overridden in a library build
 * (ciusr.h, or set it in CPPFLAGS) and it works to set it to a considerably
 * lower value (e.g. 255 works).  A lower value may help memory usage (slightly)
 * and may even improve performance on some systems (and degrade it on others.)
 */
#ifndef ZLIB_IO_MAX
#  define ZLIB_IO_MAX ((uInt)-1)
#endif

#ifdef CI_WRITE_SUPPORTED
/* The type of a compression buffer list used by the write code. */
typedef struct ci_compression_buffer
{
   struct ci_compression_buffer *next;
   ci_byte                       output[1]; /* actually zbuf_size */
} ci_compression_buffer, *ci_compression_bufferp;

#define CI_COMPRESSION_BUFFER_SIZE(pp)\
   (offsetof(ci_compression_buffer, output) + (pp)->zbuffer_size)
#endif

/* Colorspace support; structures used in ci_struct, ci_info and in internal
 * functions to hold and communicate information about the color space.
 */
/* The chromaticities of the red, green and blue colorants and the chromaticity
 * of the corresponding white point (i.e. of rgb(1.0,1.0,1.0)).
 */
typedef struct ci_xy
{
   ci_fixed_point redx, redy;
   ci_fixed_point greenx, greeny;
   ci_fixed_point bluex, bluey;
   ci_fixed_point whitex, whitey;
} ci_xy;

/* The same data as above but encoded as CIE XYZ values.  When this data comes
 * from chromaticities the sum of the Y values is assumed to be 1.0
 */
typedef struct ci_XYZ
{
   ci_fixed_point red_X, red_Y, red_Z;
   ci_fixed_point green_X, green_Y, green_Z;
   ci_fixed_point blue_X, blue_Y, blue_Z;
} ci_XYZ;

/* Chunk index values as an enum, CI_INDEX_unknown is also a count of the
 * number of chunks.
 */
#define CI_CHUNK(cHNK, i) CI_INDEX_ ## cHNK = (i),
typedef enum
{
   CI_KNOWN_CHUNKS
   CI_INDEX_unknown
} ci_index;
#undef CI_CHUNK

/* Chunk flag values.  These are (ci_uint_32 values) with exactly one bit set
 * and can be combined into a flag set with bitwise 'or'.
 *
 * TODO: C23: convert these macros to C23 inlines (which are static).
 */
#define ci_chunk_flag_from_index(i) (0x80000000U >> (31 - (i)))
   /* The flag coresponding to the given ci_index enum value.  This is defined
    * for ci_unknown as well (until it reaches the value 32) but this should
    * not be relied on.
    */

#define ci_file_has_chunk(ci_ptr, i)\
   (((ci_ptr)->chunks & ci_chunk_flag_from_index(i)) != 0)
   /* The chunk has been recorded in ci_struct */

#define ci_file_add_chunk(pnt_ptr, i)\
   ((void)((ci_ptr)->chunks |= ci_chunk_flag_from_index(i)))
   /* Record the chunk in the ci_struct */

struct ci_struct_def
{
#ifdef CI_SETJMP_SUPPORTED
   jmp_buf jmp_buf_local;     /* New name in 1.6.0 for jmp_buf in ci_struct */
   ci_longjmp_ptr longjmp_fn;/* setjmp non-local goto function. */
   jmp_buf *jmp_buf_ptr;      /* passed to longjmp_fn */
   size_t jmp_buf_size;       /* size of the above, if allocated */
#endif
   ci_error_ptr error_fn;    /* function for printing errors and aborting */
#ifdef CI_WARNINGS_SUPPORTED
   ci_error_ptr warning_fn;  /* function for printing warnings */
#endif
   ci_voidp error_ptr;       /* user supplied struct for error functions */
   ci_rw_ptr write_data_fn;  /* function for writing output data */
   ci_rw_ptr read_data_fn;   /* function for reading input data */
   ci_voidp io_ptr;          /* ptr to application struct for I/O functions */

#ifdef CI_READ_USER_TRANSFORM_SUPPORTED
   ci_user_transform_ptr read_user_transform_fn; /* user read transform */
#endif

#ifdef CI_WRITE_USER_TRANSFORM_SUPPORTED
   ci_user_transform_ptr write_user_transform_fn; /* user write transform */
#endif

/* These were added in libci-1.0.2 */
#ifdef CI_USER_TRANSFORM_PTR_SUPPORTED
#if defined(CI_READ_USER_TRANSFORM_SUPPORTED) || \
    defined(CI_WRITE_USER_TRANSFORM_SUPPORTED)
   ci_voidp user_transform_ptr; /* user supplied struct for user transform */
   ci_byte user_transform_depth;    /* bit depth of user transformed pixels */
   ci_byte user_transform_channels; /* channels in user transformed pixels */
#endif
#endif

   ci_uint_32 mode;          /* tells us where we are in the CI file */
   ci_uint_32 flags;         /* flags indicating various things to libci */
   ci_uint_32 transformations; /* which transformations to perform */

   ci_uint_32 zowner;        /* ID (chunk type) of zstream owner, 0 if none */
   z_stream    zstream;       /* decompression structure */

#ifdef CI_WRITE_SUPPORTED
   ci_compression_bufferp zbuffer_list; /* Created on demand during write */
   uInt                    zbuffer_size; /* size of the actual buffer */

   int zlib_level;            /* holds zlib compression level */
   int zlib_method;           /* holds zlib compression method */
   int zlib_window_bits;      /* holds zlib compression window bits */
   int zlib_mem_level;        /* holds zlib compression memory level */
   int zlib_strategy;         /* holds zlib compression strategy */
#endif
/* Added at libci 1.5.4 */
#ifdef CI_WRITE_CUSTOMIZE_ZTXT_COMPRESSION_SUPPORTED
   int zlib_text_level;            /* holds zlib compression level */
   int zlib_text_method;           /* holds zlib compression method */
   int zlib_text_window_bits;      /* holds zlib compression window bits */
   int zlib_text_mem_level;        /* holds zlib compression memory level */
   int zlib_text_strategy;         /* holds zlib compression strategy */
#endif
/* End of material added at libci 1.5.4 */
/* Added at libci 1.6.0 */
#ifdef CI_WRITE_SUPPORTED
   int zlib_set_level;        /* Actual values set into the zstream on write */
   int zlib_set_method;
   int zlib_set_window_bits;
   int zlib_set_mem_level;
   int zlib_set_strategy;
#endif

   ci_uint_32 chunks; /* CI_CF_ for every chunk read or (NYI) written */
#  define ci_has_chunk(ci_ptr, cHNK)\
      ci_file_has_chunk(ci_ptr, CI_INDEX_ ## cHNK)
      /* Convenience accessor - use this to check for a known chunk by name */

   ci_uint_32 width;         /* width of image in pixels */
   ci_uint_32 height;        /* height of image in pixels */
   ci_uint_32 num_rows;      /* number of rows in current pass */
   ci_uint_32 usr_width;     /* width of row at start of write */
   size_t rowbytes;           /* size of row in bytes */
   ci_uint_32 iwidth;        /* width of current interlaced row in pixels */
   ci_uint_32 row_number;    /* current row in interlace pass */
   ci_uint_32 chunk_name;    /* CI_CHUNK() id of current chunk */
   ci_bytep prev_row;        /* buffer to save previous (unfiltered) row.
                               * While reading this is a pointer into
                               * big_prev_row; while writing it is separately
                               * allocated if needed.
                               */
   ci_bytep row_buf;         /* buffer to save current (unfiltered) row.
                               * While reading, this is a pointer into
                               * big_row_buf; while writing it is separately
                               * allocated.
                               */
#ifdef CI_WRITE_FILTER_SUPPORTED
   ci_bytep try_row;    /* buffer to save trial row when filtering */
   ci_bytep tst_row;    /* buffer to save best trial row when filtering */
#endif
   size_t info_rowbytes;      /* Added in 1.5.4: cache of updated row bytes */

   ci_uint_32 idat_size;     /* current IDAT size for read */
   ci_uint_32 crc;           /* current chunk CRC value */
   ci_colorp palette;        /* palette from the input file */
   ci_uint_16 num_palette;   /* number of color entries in palette */

/* Added at libci-1.5.10 */
#ifdef CI_CHECK_FOR_INVALID_INDEX_SUPPORTED
   int num_palette_max;       /* maximum palette index found in IDAT */
#endif

   ci_uint_16 num_trans;     /* number of transparency values */
   ci_byte compression;      /* file compression type (always 0) */
   ci_byte filter;           /* file filter type (always 0) */
   ci_byte interlaced;       /* CI_INTERLACE_NONE, CI_INTERLACE_ADAM7 */
   ci_byte pass;             /* current interlace pass (0 - 6) */
   ci_byte do_filter;        /* row filter flags (see CI_FILTER_ in ci.h ) */
   ci_byte color_type;       /* color type of file */
   ci_byte bit_depth;        /* bit depth of file */
   ci_byte usr_bit_depth;    /* bit depth of users row: write only */
   ci_byte pixel_depth;      /* number of bits per pixel */
   ci_byte channels;         /* number of channels in file */
#ifdef CI_WRITE_SUPPORTED
   ci_byte usr_channels;     /* channels at start of write: write only */
#endif
   ci_byte sig_bytes;        /* magic bytes read/written from start of file */
   ci_byte maximum_pixel_depth;
                              /* pixel depth used for the row buffers */
   ci_byte transformed_pixel_depth;
                              /* pixel depth after read/write transforms */
#if ZLIB_VERNUM >= 0x1240
   ci_byte zstream_start;    /* at start of an input zlib stream */
#endif /* Zlib >= 1.2.4 */
#if defined(CI_READ_FILLER_SUPPORTED) || defined(CI_WRITE_FILLER_SUPPORTED)
   ci_uint_16 filler;           /* filler bytes for pixel expansion */
#endif

#if defined(CI_bKGD_SUPPORTED) || defined(CI_READ_BACKGROUND_SUPPORTED) ||\
   defined(CI_READ_ALPHA_MODE_SUPPORTED)
   ci_byte background_gamma_type;
   ci_fixed_point background_gamma;
   ci_color_16 background;   /* background color in screen gamma space */
#ifdef CI_READ_GAMMA_SUPPORTED
   ci_color_16 background_1; /* background normalized to gamma 1.0 */
#endif
#endif /* bKGD */

#ifdef CI_WRITE_FLUSH_SUPPORTED
   ci_flush_ptr output_flush_fn; /* Function for flushing output */
   ci_uint_32 flush_dist;    /* how many rows apart to flush, 0 - no flush */
   ci_uint_32 flush_rows;    /* number of rows written since last flush */
#endif

#ifdef CI_READ_RGB_TO_GRAY_SUPPORTED
   ci_xy          chromaticities; /* From mDVC, cICP, [iCCP], sRGB or cHRM */
#endif

#ifdef CI_READ_GAMMA_SUPPORTED
   int gamma_shift;      /* number of "insignificant" bits in 16-bit gamma */
   ci_fixed_point screen_gamma; /* screen gamma value (display exponent) */
   ci_fixed_point file_gamma;   /* file gamma value (encoding exponent) */
   ci_fixed_point chunk_gamma;  /* from cICP, iCCP, sRGB or gAMA */
   ci_fixed_point default_gamma;/* from ci_set_alpha_mode */

   ci_bytep gamma_table;     /* gamma table for 8-bit depth files */
   ci_uint_16pp gamma_16_table; /* gamma table for 16-bit depth files */
#if defined(CI_READ_BACKGROUND_SUPPORTED) || \
   defined(CI_READ_ALPHA_MODE_SUPPORTED) || \
   defined(CI_READ_RGB_TO_GRAY_SUPPORTED)
   ci_bytep gamma_from_1;    /* converts from 1.0 to screen */
   ci_bytep gamma_to_1;      /* converts from file to 1.0 */
   ci_uint_16pp gamma_16_from_1; /* converts from 1.0 to screen */
   ci_uint_16pp gamma_16_to_1; /* converts from file to 1.0 */
#endif /* READ_BACKGROUND || READ_ALPHA_MODE || RGB_TO_GRAY */
#endif /* READ_GAMMA */

#if defined(CI_READ_GAMMA_SUPPORTED) || defined(CI_sBIT_SUPPORTED)
   ci_color_8 sig_bit;       /* significant bits in each available channel */
#endif

#if defined(CI_READ_SHIFT_SUPPORTED) || defined(CI_WRITE_SHIFT_SUPPORTED)
   ci_color_8 shift;         /* shift for significant bit transformation */
#endif

#if defined(CI_tRNS_SUPPORTED) || defined(CI_READ_BACKGROUND_SUPPORTED) \
 || defined(CI_READ_EXPAND_SUPPORTED) || defined(CI_READ_BACKGROUND_SUPPORTED)
   ci_bytep trans_alpha;           /* alpha values for paletted files */
   ci_color_16 trans_color;  /* transparent color for non-paletted files */
#endif

   ci_read_status_ptr read_row_fn;   /* called after each row is decoded */
   ci_write_status_ptr write_row_fn; /* called after each row is encoded */
#ifdef CI_PROGRESSIVE_READ_SUPPORTED
   ci_progressive_info_ptr info_fn; /* called after header data fully read */
   ci_progressive_row_ptr row_fn;   /* called after a prog. row is decoded */
   ci_progressive_end_ptr end_fn;   /* called after image is complete */
   ci_bytep save_buffer_ptr;        /* current location in save_buffer */
   ci_bytep save_buffer;            /* buffer for previously read data */
   ci_bytep current_buffer_ptr;     /* current location in current_buffer */
   ci_bytep current_buffer;         /* buffer for recently used data */
   ci_uint_32 push_length;          /* size of current input chunk */
   ci_uint_32 skip_length;          /* bytes to skip in input data */
   size_t save_buffer_size;          /* amount of data now in save_buffer */
   size_t save_buffer_max;           /* total size of save_buffer */
   size_t buffer_size;               /* total amount of available input data */
   size_t current_buffer_size;       /* amount of data now in current_buffer */
   int process_mode;                 /* what push library is currently doing */
   int cur_palette;                  /* current push library palette index */
#endif /* PROGRESSIVE_READ */

#ifdef CI_READ_QUANTIZE_SUPPORTED
   ci_bytep palette_lookup; /* lookup table for quantizing */
   ci_bytep quantize_index; /* index translation for palette files */
#endif

/* Options */
#ifdef CI_SET_OPTION_SUPPORTED
   ci_uint_32 options;           /* On/off state (up to 16 options) */
#endif

#if CI_LIBCI_VER < 10700
/* To do: remove this from libci-1.7 */
#ifdef CI_TIME_RFC1123_SUPPORTED
   char time_buffer[29]; /* String to hold RFC 1123 time text */
#endif /* TIME_RFC1123 */
#endif /* LIBCI_VER < 10700 */

/* New members added in libci-1.0.6 */

   ci_uint_32 free_me;    /* flags items libci is responsible for freeing */

#ifdef CI_USER_CHUNKS_SUPPORTED
   ci_voidp user_chunk_ptr;
#ifdef CI_READ_USER_CHUNKS_SUPPORTED
   ci_user_chunk_ptr read_user_chunk_fn; /* user read chunk handler */
#endif /* READ_USER_CHUNKS */
#endif /* USER_CHUNKS */

#ifdef CI_SET_UNKNOWN_CHUNKS_SUPPORTED
   int          unknown_default; /* As CI_HANDLE_* */
   unsigned int num_chunk_list;  /* Number of entries in the list */
   ci_bytep    chunk_list;      /* List of ci_byte[5]; the textual chunk name
                                  * followed by a CI_HANDLE_* byte */
#endif

/* New members added in libci-1.0.3 */
#ifdef CI_READ_RGB_TO_GRAY_SUPPORTED
   ci_byte rgb_to_gray_status;
   /* Added in libci 1.5.5 to record setting of coefficients: */
   ci_byte rgb_to_gray_coefficients_set;
   /* These were changed from ci_byte in libci-1.0.6 */
   ci_uint_16 rgb_to_gray_red_coeff;
   ci_uint_16 rgb_to_gray_green_coeff;
   /* deleted in 1.5.5: rgb_to_gray_blue_coeff; */
#endif

/* New member added in libci-1.6.36 */
#if defined(CI_READ_EXPAND_SUPPORTED) && \
    (defined(CI_ARM_NEON_IMPLEMENTATION) || \
     defined(CI_RISCV_RVV_IMPLEMENTATION))
   ci_bytep riffled_palette; /* buffer for accelerated palette expansion */
#endif

/* New member added in libci-1.0.4 (renamed in 1.0.9) */
#if defined(CI_MNG_FEATURES_SUPPORTED)
/* Changed from ci_byte to ci_uint_32 at version 1.2.0 */
   ci_uint_32 mng_features_permitted;
#endif

/* New member added in libci-1.0.9, ifdef'ed out in 1.0.12, enabled in 1.2.0 */
#ifdef CI_MNG_FEATURES_SUPPORTED
   ci_byte filter_type;
#endif

/* New members added in libci-1.2.0 */

/* New members added in libci-1.0.2 but first enabled by default in 1.2.0 */
#ifdef CI_USER_MEM_SUPPORTED
   ci_voidp mem_ptr;             /* user supplied struct for mem functions */
   ci_malloc_ptr malloc_fn;      /* function for allocating memory */
   ci_free_ptr free_fn;          /* function for freeing memory */
#endif

/* New member added in libci-1.0.13 and 1.2.0 */
   ci_bytep big_row_buf;         /* buffer to save current (unfiltered) row */

#ifdef CI_READ_QUANTIZE_SUPPORTED
/* The following three members were added at version 1.0.14 and 1.2.4 */
   ci_bytep quantize_sort;          /* working sort array */
   ci_bytep index_to_palette;       /* where the original index currently is
                                        in the palette */
   ci_bytep palette_to_index;       /* which original index points to this
                                         palette color */
#endif

/* New members added in libci-1.0.16 and 1.2.6 */
   ci_byte compression_type;

#ifdef CI_USER_LIMITS_SUPPORTED
   ci_uint_32 user_width_max;
   ci_uint_32 user_height_max;

   /* Added in libci-1.4.0: Total number of sPLT, text, and unknown
    * chunks that can be stored (0 means unlimited).
    */
   ci_uint_32 user_chunk_cache_max;

   /* Total memory that a zTXt, sPLT, iTXt, iCCP, or unknown chunk
    * can occupy when decompressed.  0 means unlimited.
    */
   ci_alloc_size_t user_chunk_malloc_max;
#endif

/* New member added in libci-1.0.25 and 1.2.17 */
#ifdef CI_READ_UNKNOWN_CHUNKS_SUPPORTED
   /* Temporary storage for unknown chunk that the library doesn't recognize,
    * used while reading the chunk.
    */
   ci_unknown_chunk unknown_chunk;
#endif

/* New member added in libci-1.2.26 */
   size_t old_big_row_buf_size;

#ifdef CI_READ_SUPPORTED
/* New member added in libci-1.2.30 */
  ci_bytep        read_buffer;      /* buffer for reading chunk data */
  ci_alloc_size_t read_buffer_size; /* current size of the buffer */
#endif
#ifdef CI_SEQUENTIAL_READ_SUPPORTED
  uInt             IDAT_read_size;   /* limit on read buffer size for IDAT */
#endif

#ifdef CI_IO_STATE_SUPPORTED
/* New member added in libci-1.4.0 */
   ci_uint_32 io_state;
#endif

/* New member added in libci-1.5.6 */
   ci_bytep big_prev_row;

/* New member added in libci-1.5.7 */
   void (*read_filter[CI_FILTER_VALUE_LAST-1])(ci_row_infop row_info,
      ci_bytep row, ci_const_bytep prev_row);
};
#endif /* CISTRUCT_H */

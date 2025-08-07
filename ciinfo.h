/* ciinfo.h - internal structures for libci
 *
 * Copyright (c) 2018-2025 Cosmin Truta
 * Copyright (c) 1998-2002,2004,2006-2013,2018 Glenn Randers-Pehrson
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

/* INTERNAL, PRIVATE definition of a CI.
 *
 * ci_info is a modifiable description of a CI datastream.  The fields inside
 * this structure are accessed through ci_get_<CHUNK>() functions and modified
 * using ci_set_<CHUNK>() functions.
 *
 * Some functions in libci do directly access members of ci_info.  However,
 * this should be avoided.  ci_struct objects contain members which hold
 * caches, sometimes optimised, of the values from ci_info objects, and
 * ci_info is not passed to the functions which read and write image data.
 */
#ifndef CIINFO_H
#define CIINFO_H

struct ci_info_def
{
   /* The following are necessary for every CI file */
   ci_uint_32 width;       /* width of image in pixels (from IHDR) */
   ci_uint_32 height;      /* height of image in pixels (from IHDR) */
   ci_uint_32 valid;       /* valid chunk data (see CI_INFO_ below) */
   size_t rowbytes;         /* bytes needed to hold an untransformed row */
   ci_colorp palette;      /* array of color values (valid & CI_INFO_PLTE) */
   ci_uint_16 num_palette; /* number of color entries in "palette" (PLTE) */
   ci_uint_16 num_trans;   /* number of transparent palette color (tRNS) */
   ci_byte bit_depth;      /* 1, 2, 4, 8, or 16 bits/channel (from IHDR) */
   ci_byte color_type;     /* see CI_COLOR_TYPE_ below (from IHDR) */
   /* The following three should have been named *_method not *_type */
   ci_byte compression_type; /* must be CI_COMPRESSION_TYPE_BASE (IHDR) */
   ci_byte filter_type;    /* must be CI_FILTER_TYPE_BASE (from IHDR) */
   ci_byte interlace_type; /* One of CI_INTERLACE_NONE, CI_INTERLACE_ADAM7 */

   /* The following are set by ci_set_IHDR, called from the application on
    * write, but the are never actually used by the write code.
    */
   ci_byte channels;       /* number of data channels per pixel (1, 2, 3, 4) */
   ci_byte pixel_depth;    /* number of bits per pixel */
   ci_byte spare_byte;     /* to align the data, and for future use */

#ifdef CI_READ_SUPPORTED
   /* This is never set during write */
   ci_byte signature[8];   /* magic bytes read by libci from start of file */
#endif

   /* The rest of the data is optional.  If you are reading, check the
    * valid field to see if the information in these are valid.  If you
    * are writing, set the valid field to those chunks you want written,
    * and initialize the appropriate fields below.
    */

#ifdef CI_cICP_SUPPORTED
   /* cICP chunk data */
   ci_byte cicp_colour_primaries;
   ci_byte cicp_transfer_function;
   ci_byte cicp_matrix_coefficients;
   ci_byte cicp_video_full_range_flag;
#endif

#ifdef CI_iCCP_SUPPORTED
   /* iCCP chunk data. */
   ci_charp iccp_name;     /* profile name */
   ci_bytep iccp_profile;  /* International Color Consortium profile data */
   ci_uint_32 iccp_proflen;  /* ICC profile data length */
#endif

#ifdef CI_cLLI_SUPPORTED
   ci_uint_32 maxCLL;  /* cd/m2 (nits) * 10,000 */
   ci_uint_32 maxFALL;
#endif

#ifdef CI_mDCV_SUPPORTED
   ci_uint_16 mastering_red_x;  /* CIE (xy) x * 50,000 */
   ci_uint_16 mastering_red_y;
   ci_uint_16 mastering_green_x;
   ci_uint_16 mastering_green_y;
   ci_uint_16 mastering_blue_x;
   ci_uint_16 mastering_blue_y;
   ci_uint_16 mastering_white_x;
   ci_uint_16 mastering_white_y;
   ci_uint_32 mastering_maxDL; /* cd/m2 (nits) * 10,000 */
   ci_uint_32 mastering_minDL;
#endif

#ifdef CI_TEXT_SUPPORTED
   /* The tEXt, and zTXt chunks contain human-readable textual data in
    * uncompressed, compressed, and optionally compressed forms, respectively.
    * The data in "text" is an array of pointers to uncompressed,
    * null-terminated C strings. Each chunk has a keyword that describes the
    * textual data contained in that chunk.  Keywords are not required to be
    * unique, and the text string may be empty.  Any number of text chunks may
    * be in an image.
    */
   int num_text; /* number of comments read or comments to write */
   int max_text; /* current size of text array */
   ci_textp text; /* array of comments read or comments to write */
#endif /* TEXT */

#ifdef CI_tIME_SUPPORTED
   /* The tIME chunk holds the last time the displayed image data was
    * modified.  See the ci_time struct for the contents of this struct.
    */
   ci_time mod_time;
#endif

#ifdef CI_sBIT_SUPPORTED
   /* The sBIT chunk specifies the number of significant high-order bits
    * in the pixel data.  Values are in the range [1, bit_depth], and are
    * only specified for the channels in the pixel data.  The contents of
    * the low-order bits is not specified.  Data is valid if
    * (valid & CI_INFO_sBIT) is non-zero.
    */
   ci_color_8 sig_bit; /* significant bits in color channels */
#endif

#if defined(CI_tRNS_SUPPORTED) || defined(CI_READ_EXPAND_SUPPORTED) || \
defined(CI_READ_BACKGROUND_SUPPORTED)
   /* The tRNS chunk supplies transparency data for paletted images and
    * other image types that don't need a full alpha channel.  There are
    * "num_trans" transparency values for a paletted image, stored in the
    * same order as the palette colors, starting from index 0.  Values
    * for the data are in the range [0, 255], ranging from fully transparent
    * to fully opaque, respectively.  For non-paletted images, there is a
    * single color specified that should be treated as fully transparent.
    * Data is valid if (valid & CI_INFO_tRNS) is non-zero.
    */
   ci_bytep trans_alpha;    /* alpha values for paletted image */
   ci_color_16 trans_color; /* transparent color for non-palette image */
#endif

#if defined(CI_bKGD_SUPPORTED) || defined(CI_READ_BACKGROUND_SUPPORTED)
   /* The bKGD chunk gives the suggested image background color if the
    * display program does not have its own background color and the image
    * is needs to composited onto a background before display.  The colors
    * in "background" are normally in the same color space/depth as the
    * pixel data.  Data is valid if (valid & CI_INFO_bKGD) is non-zero.
    */
   ci_color_16 background;
#endif

#ifdef CI_oFFs_SUPPORTED
   /* The oFFs chunk gives the offset in "offset_unit_type" units rightwards
    * and downwards from the top-left corner of the display, page, or other
    * application-specific co-ordinate space.  See the CI_OFFSET_ defines
    * below for the unit types.  Valid if (valid & CI_INFO_oFFs) non-zero.
    */
   ci_int_32 x_offset; /* x offset on page */
   ci_int_32 y_offset; /* y offset on page */
   ci_byte offset_unit_type; /* offset units type */
#endif

#ifdef CI_pHYs_SUPPORTED
   /* The pHYs chunk gives the physical pixel density of the image for
    * display or printing in "phys_unit_type" units (see CI_RESOLUTION_
    * defines below).  Data is valid if (valid & CI_INFO_pHYs) is non-zero.
    */
   ci_uint_32 x_pixels_per_unit; /* horizontal pixel density */
   ci_uint_32 y_pixels_per_unit; /* vertical pixel density */
   ci_byte phys_unit_type; /* resolution type (see CI_RESOLUTION_ below) */
#endif

#ifdef CI_eXIf_SUPPORTED
   ci_uint_32 num_exif;  /* Added at libci-1.6.31 */
   ci_bytep exif;
#endif

#ifdef CI_hIST_SUPPORTED
   /* The hIST chunk contains the relative frequency or importance of the
    * various palette entries, so that a viewer can intelligently select a
    * reduced-color palette, if required.  Data is an array of "num_palette"
    * values in the range [0,65535]. Data valid if (valid & CI_INFO_hIST)
    * is non-zero.
    */
   ci_uint_16p hist;
#endif

#ifdef CI_pCAL_SUPPORTED
   /* The pCAL chunk describes a transformation between the stored pixel
    * values and original physical data values used to create the image.
    * The integer range [0, 2^bit_depth - 1] maps to the floating-point
    * range given by [pcal_X0, pcal_X1], and are further transformed by a
    * (possibly non-linear) transformation function given by "pcal_type"
    * and "pcal_params" into "pcal_units".  Please see the CI_EQUATION_
    * defines below, and the CI-Group's CI extensions document for a
    * complete description of the transformations and how they should be
    * implemented, and for a description of the ASCII parameter strings.
    * Data values are valid if (valid & CI_INFO_pCAL) non-zero.
    */
   ci_charp pcal_purpose;  /* pCAL chunk description string */
   ci_int_32 pcal_X0;      /* minimum value */
   ci_int_32 pcal_X1;      /* maximum value */
   ci_charp pcal_units;    /* Latin-1 string giving physical units */
   ci_charpp pcal_params;  /* ASCII strings containing parameter values */
   ci_byte pcal_type;      /* equation type (see CI_EQUATION_ below) */
   ci_byte pcal_nparams;   /* number of parameters given in pcal_params */
#endif

/* New members added in libci-1.0.6 */
   ci_uint_32 free_me;     /* flags items libci is responsible for freeing */

#ifdef CI_STORE_UNKNOWN_CHUNKS_SUPPORTED
   /* Storage for unknown chunks that the library doesn't recognize. */
   ci_unknown_chunkp unknown_chunks;

   /* The type of this field is limited by the type of
    * ci_struct::user_chunk_cache_max, else overflow can occur.
    */
   int                unknown_chunks_num;
#endif

#ifdef CI_sPLT_SUPPORTED
   /* Data on sPLT chunks (there may be more than one). */
   ci_sPLT_tp splt_palettes;
   int         splt_palettes_num; /* Match type returned by ci_get API */
#endif

#ifdef CI_sCAL_SUPPORTED
   /* The sCAL chunk describes the actual physical dimensions of the
    * subject matter of the graphic.  The chunk contains a unit specification
    * a byte value, and two ASCII strings representing floating-point
    * values.  The values are width and height corresponding to one pixel
    * in the image.  Data values are valid if (valid & CI_INFO_sCAL) is
    * non-zero.
    */
   ci_byte scal_unit;         /* unit of physical scale */
   ci_charp scal_s_width;     /* string containing height */
   ci_charp scal_s_height;    /* string containing width */
#endif

#ifdef CI_INFO_IMAGE_SUPPORTED
   /* Memory has been allocated if (valid & CI_ALLOCATED_INFO_ROWS)
      non-zero */
   /* Data valid if (valid & CI_INFO_IDAT) non-zero */
   ci_bytepp row_pointers;        /* the image bits */
#endif

#ifdef CI_cHRM_SUPPORTED
   ci_xy cHRM;
#endif

#ifdef CI_gAMA_SUPPORTED
   ci_fixed_point gamma;
#endif

#ifdef CI_sRGB_SUPPORTED
   int rendering_intent;
#endif
};
#endif /* CIINFO_H */

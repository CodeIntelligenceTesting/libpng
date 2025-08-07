/*-------------------------------------
 *  CIFILE.C -- Image File Functions
 *-------------------------------------
 *
 * Copyright 2000,2017 Willem van Schaik.
 *
 * This code is released under the libci license.
 * For conditions of distribution and use, see the disclaimer
 * and license in ci.h
 */

#include <windows.h>
#include <commdlg.h>
#include <stdio.h>
#include <stdlib.h>
#include <zlib.h>

#include "ci.h"
#include "cifile.h"
#include "cexcept.h"

define_exception_type(const char *);
extern struct exception_context the_exception_context[1];
struct exception_context the_exception_context[1];
ci_const_charp msg;

static OPENFILENAME ofn;

static ci_structp ci_ptr = NULL;
static ci_infop info_ptr = NULL;


/* cexcept interface */

static void
ci_cexcept_error(ci_structp ci_ptr, ci_const_charp msg)
{
   if(ci_ptr)
     ;
#ifdef CI_CONSOLE_IO_SUPPORTED
   fprintf(stderr, "libci error: %s\n", msg);
#endif
   {
      Throw msg;
   }
}

/* Windows open-file functions */

void CiFileInitialize (HWND hwnd)
{
    static TCHAR szFilter[] = TEXT ("CI Files (*.CI)\0*.ci\0")
        TEXT ("All Files (*.*)\0*.*\0\0");

    ofn.lStructSize       = sizeof (OPENFILENAME);
    ofn.hwndOwner         = hwnd;
    ofn.hInstance         = NULL;
    ofn.lpstrFilter       = szFilter;
    ofn.lpstrCustomFilter = NULL;
    ofn.nMaxCustFilter    = 0;
    ofn.nFilterIndex      = 0;
    ofn.lpstrFile         = NULL;          /* Set in Open and Close functions */
    ofn.nMaxFile          = MAX_PATH;
    ofn.lpstrFileTitle    = NULL;          /* Set in Open and Close functions */
    ofn.nMaxFileTitle     = MAX_PATH;
    ofn.lpstrInitialDir   = NULL;
    ofn.lpstrTitle        = NULL;
    ofn.Flags             = 0;             /* Set in Open and Close functions */
    ofn.nFileOffset       = 0;
    ofn.nFileExtension    = 0;
    ofn.lpstrDefExt       = TEXT ("ci");
    ofn.lCustData         = 0;
    ofn.lpfnHook          = NULL;
    ofn.lpTemplateName    = NULL;
}

BOOL CiFileOpenDlg (HWND hwnd, PTSTR pstrFileName, PTSTR pstrTitleName)
{
    ofn.hwndOwner         = hwnd;
    ofn.lpstrFile         = pstrFileName;
    ofn.lpstrFileTitle    = pstrTitleName;
    ofn.Flags             = OFN_HIDEREADONLY;

    return GetOpenFileName (&ofn);
}

BOOL CiFileSaveDlg (HWND hwnd, PTSTR pstrFileName, PTSTR pstrTitleName)
{
    ofn.hwndOwner         = hwnd;
    ofn.lpstrFile         = pstrFileName;
    ofn.lpstrFileTitle    = pstrTitleName;
    ofn.Flags             = OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT;

    return GetSaveFileName (&ofn);
}

/* CI image handler functions */

BOOL CiLoadImage (PTSTR pstrFileName, ci_byte **ppbImageData,
                   int *piWidth, int *piHeight, int *piChannels, ci_color *pBkgColor)
{
    static FILE        *pfFile;
    ci_byte            pbSig[8];
    int                 iBitDepth;
    int                 iColorType;
    double              dGamma;
    ci_color_16       *pBackground;
    ci_uint_32         ulChannels;
    ci_uint_32         ulRowBytes;
    ci_byte           *pbImageData = *ppbImageData;
    static ci_byte   **ppbRowPointers = NULL;
    int                 i;

    /* open the CI input file */

    if (!pstrFileName)
    {
        *ppbImageData = pbImageData = NULL;
        return FALSE;
    }

    if (!(pfFile = fopen(pstrFileName, "rb")))
    {
        *ppbImageData = pbImageData = NULL;
        return FALSE;
    }

    /* first check the eight byte CI signature */

    fread(pbSig, 1, 8, pfFile);
    if (ci_sig_cmp(pbSig, 0, 8))
    {
        *ppbImageData = pbImageData = NULL;
        return FALSE;
    }

    /* create the two ci(-info) structures */

    ci_ptr = ci_create_read_struct(CI_LIBCI_VER_STRING, NULL,
      (ci_error_ptr)ci_cexcept_error, (ci_error_ptr)NULL);
    if (!ci_ptr)
    {
        *ppbImageData = pbImageData = NULL;
        return FALSE;
    }

    info_ptr = ci_create_info_struct(ci_ptr);
    if (!info_ptr)
    {
        ci_destroy_read_struct(&ci_ptr, NULL, NULL);
        *ppbImageData = pbImageData = NULL;
        return FALSE;
    }

    Try
    {

        /* initialize the ci structure */

#ifdef CI_STDIO_SUPPORTED
        ci_init_io(ci_ptr, pfFile);
#else
        ci_set_read_fn(ci_ptr, (ci_voidp)pfFile, ci_read_data);
#endif

        ci_set_sig_bytes(ci_ptr, 8);

        /* read all CI info up to image data */

        ci_read_info(ci_ptr, info_ptr);

        /* get width, height, bit-depth and color-type */

        ci_get_IHDR(ci_ptr, info_ptr, piWidth, piHeight, &iBitDepth,
            &iColorType, NULL, NULL, NULL);

        /* expand images of all color-type and bit-depth to 3x8-bit RGB */
        /* let the library process alpha, transparency, background, etc. */

#ifdef CI_READ_16_TO_8_SUPPORTED
    if (iBitDepth == 16)
#  ifdef CI_READ_SCALE_16_TO_8_SUPPORTED
        ci_set_scale_16(ci_ptr);
#  else
        ci_set_strip_16(ci_ptr);
#  endif
#endif
        if (iColorType == CI_COLOR_TYPE_PALETTE)
            ci_set_expand(ci_ptr);
        if (iBitDepth < 8)
            ci_set_expand(ci_ptr);
        if (ci_get_valid(ci_ptr, info_ptr, CI_INFO_tRNS))
            ci_set_expand(ci_ptr);
        if (iColorType == CI_COLOR_TYPE_GRAY ||
            iColorType == CI_COLOR_TYPE_GRAY_ALPHA)
            ci_set_gray_to_rgb(ci_ptr);

        /* set the background color to draw transparent and alpha images over */
        if (ci_get_bKGD(ci_ptr, info_ptr, &pBackground))
        {
            ci_set_background(ci_ptr, pBackground, CI_BACKGROUND_GAMMA_FILE, 1, 1.0);
            pBkgColor->red   = (byte) pBackground->red;
            pBkgColor->green = (byte) pBackground->green;
            pBkgColor->blue  = (byte) pBackground->blue;
        }
        else
        {
            pBkgColor = NULL;
        }

        /* if required set gamma conversion */
        if (ci_get_gAMA(ci_ptr, info_ptr, &dGamma))
            ci_set_gamma(ci_ptr, (double) 2.2, dGamma);

        /* after the transformations are registered, update info_ptr data */

        ci_read_update_info(ci_ptr, info_ptr);

        /* get again width, height and the new bit-depth and color-type */

        ci_get_IHDR(ci_ptr, info_ptr, piWidth, piHeight, &iBitDepth,
            &iColorType, NULL, NULL, NULL);


        /* row_bytes is the width x number of channels */

        ulRowBytes = ci_get_rowbytes(ci_ptr, info_ptr);
        ulChannels = ci_get_channels(ci_ptr, info_ptr);

        *piChannels = ulChannels;

        /* now we can allocate memory to store the image */

        if (pbImageData)
        {
            free (pbImageData);
            pbImageData = NULL;
        }
        if ((*piHeight) > ((size_t)(-1))/ulRowBytes) {
        {
            ci_error(ci_ptr, "Visual CI: image is too big");
        }
        if ((pbImageData = (ci_byte *) malloc(ulRowBytes * (*piHeight)
                            * sizeof(ci_byte))) == NULL)
        {
            ci_error(ci_ptr, "Visual CI: out of memory");
        }
        *ppbImageData = pbImageData;

        /* and allocate memory for an array of row-pointers */

        if ((ppbRowPointers = (ci_bytepp) malloc((*piHeight)
                            * sizeof(ci_bytep))) == NULL)
        {
            ci_error(ci_ptr, "Visual CI: out of memory");
        }

        /* set the individual row-pointers to point at the correct offsets */

        for (i = 0; i < (*piHeight); i++)
            ppbRowPointers[i] = pbImageData + i * ulRowBytes;

        /* now we can go ahead and just read the whole image */

        ci_read_image(ci_ptr, ppbRowPointers);

        /* read the additional chunks in the CI file (not really needed) */

        ci_read_end(ci_ptr, NULL);

        /* and we're done */

        free (ppbRowPointers);
        ppbRowPointers = NULL;

        /* yepp, done */
    }

    Catch (msg)
    {
        ci_destroy_read_struct(&ci_ptr, &info_ptr, NULL);

        *ppbImageData = pbImageData = NULL;

        if(ppbRowPointers)
            free (ppbRowPointers);

        fclose(pfFile);

        return FALSE;
    }

    fclose (pfFile);

    return TRUE;
}


BOOL CiSaveImage (PTSTR pstrFileName, ci_byte *pDiData,
                   int iWidth, int iHeight, ci_color bkgColor)
{
    const int           ciBitDepth = 8;
    const int           ciChannels = 3;

    static FILE        *pfFile;
    ci_uint_32         ulRowBytes;
    static ci_byte   **ppbRowPointers = NULL;
    int                 i;

    /* open the CI output file */

    if (!pstrFileName)
        return FALSE;

    if (!(pfFile = fopen(pstrFileName, "wb")))
        return FALSE;

    /* prepare the standard CI structures */

    ci_ptr = ci_create_write_struct(CI_LIBCI_VER_STRING, NULL,
      (ci_error_ptr)ci_cexcept_error, (ci_error_ptr)NULL);
    if (!ci_ptr)
    {
        fclose(pfFile);
        return FALSE;
    }

    info_ptr = ci_create_info_struct(ci_ptr);
    if (!info_ptr) {
        fclose(pfFile);
        ci_destroy_write_struct(&ci_ptr, (ci_infopp) NULL);
        return FALSE;
    }

    Try
    {
        /* initialize the ci structure */

#ifdef CI_STDIO_SUPPORTED
        ci_init_io(ci_ptr, pfFile);
#else
        ci_set_write_fn(ci_ptr, (ci_voidp)pfFile, ci_write_data, ci_flush);
#endif

        /* we're going to write a very simple 3x8-bit RGB image */

        ci_set_IHDR(ci_ptr, info_ptr, iWidth, iHeight, ciBitDepth,
            CI_COLOR_TYPE_RGB, CI_INTERLACE_NONE, CI_COMPRESSION_TYPE_BASE,
            CI_FILTER_TYPE_BASE);

        /* write the file header information */

        ci_write_info(ci_ptr, info_ptr);

        /* swap the BGR pixels in the DiData structure to RGB */

        ci_set_bgr(ci_ptr);

        /* row_bytes is the width x number of channels */

        ulRowBytes = iWidth * ciChannels;

        /* we can allocate memory for an array of row-pointers */

        if ((ppbRowPointers = (ci_bytepp) malloc(iHeight * sizeof(ci_bytep))) == NULL)
            Throw "Visualci: Out of memory";

        /* set the individual row-pointers to point at the correct offsets */

        for (i = 0; i < iHeight; i++)
            ppbRowPointers[i] = pDiData + i * (((ulRowBytes + 3) >> 2) << 2);

        /* write out the entire image data in one call */

        ci_write_image (ci_ptr, ppbRowPointers);

        /* write the additional chunks to the CI file (not really needed) */

        ci_write_end(ci_ptr, info_ptr);

        /* and we're done */

        free (ppbRowPointers);
        ppbRowPointers = NULL;

        /* clean up after the write, and free any memory allocated */

        ci_destroy_write_struct(&ci_ptr, (ci_infopp) NULL);

        /* yepp, done */
    }

    Catch (msg)
    {
        ci_destroy_write_struct(&ci_ptr, (ci_infopp) NULL);

        if(ppbRowPointers)
            free (ppbRowPointers);

        fclose(pfFile);

        return FALSE;
    }

    fclose (pfFile);

    return TRUE;
}

#ifndef CI_STDIO_SUPPORTED

static void
ci_read_data(ci_structp ci_ptr, ci_bytep data, size_t length)
{
   size_t check;

   /* fread() returns 0 on error, so it is OK to store this in a size_t
    * instead of an int, which is what fread() actually returns.
    */
   check = fread(data, 1, length, (FILE *)ci_ptr->io_ptr);

   if (check != length)
   {
      ci_error(ci_ptr, "Read Error");
   }
}

static void
ci_write_data(ci_structp ci_ptr, ci_bytep data, size_t length)
{
   ci_uint_32 check;

   check = fwrite(data, 1, length, (FILE *)(ci_ptr->io_ptr));
   if (check != length)
   {
      ci_error(ci_ptr, "Write Error");
   }
}

static void
ci_flush(ci_structp ci_ptr)
{
   FILE *io_ptr;
   io_ptr = (FILE *)CVT_PTR((ci_ptr->io_ptr));
   if (io_ptr != NULL)
      fflush(io_ptr);
}

#endif

/*-----------------
 *  end of source
 *-----------------
 */

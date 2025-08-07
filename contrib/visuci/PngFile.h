/*------------------------------------------*/
/*  CIFILE.H -- Header File for cifile.c*/
/*------------------------------------------*/

/* Copyright 2000, Willem van Schaik.*/

/* This code is released under the libci license.*/
/* For conditions of distribution and use, see the disclaimer*/
/* and license in ci.h*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>

void CiFileInitialize (HWND hwnd) ;
BOOL CiFileOpenDlg (HWND hwnd, PTSTR pstrFileName, PTSTR pstrTitleName) ;
BOOL CiFileSaveDlg (HWND hwnd, PTSTR pstrFileName, PTSTR pstrTitleName) ;

BOOL CiLoadImage (PTSTR pstrFileName, ci_byte **ppbImageData,
                   int *piWidth, int *piHeight, int *piChannels, ci_color *pBkgColor);
BOOL CiSaveImage (PTSTR pstrFileName, ci_byte *pDiData,
                   int iWidth, int iHeight, ci_color BkgColor);

#ifndef CI_STDIO_SUPPORTED
static void ci_read_data(ci_structp ci_ptr, ci_bytep data, size_t length);
static void ci_write_data(ci_structp ci_ptr, ci_bytep data, size_t length);
static void ci_flush(ci_structp ci_ptr);
#endif


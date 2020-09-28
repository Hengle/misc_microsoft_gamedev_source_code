
/******************************************************************************\
*       This is a part of the Microsoft Source Code Samples. 
*       Copyright (C) 1993-1997 Microsoft Corporation.
*       All rights reserved. 
*       This source code is only intended as a supplement to 
*       Microsoft Development Tools and/or WinHelp documentation.
*       See these sources for detailed information regarding the 
*       Microsoft samples programs.
\******************************************************************************/

/*******************************************************************************
 *                                                                             *
 *  MODULE      : DIB.C                                                        *
 *                                                                             *
 *  DESCRIPTION : Routines for dealing with Device Independent Bitmaps.        *
 *                                                                             *
 *  FUNCTIONS   : OpenDIB()           - Opens DIB file and creates a memory DIB*
 *                                                                             *
 *                WriteDIB()          - Writes a global handle in CF_DIB format*
 *                                      to a file.                             *
 *                                                                             *
 *                DibInfo()           - Retrieves the info. block associated   *
 *                                      with a CF_DIB format memory block.     *
 *                                                                             *
 *                CreateBIPalette()   - Creates a GDI palette given a pointer  *
 *                                      to a BITMAPINFO structure.             *
 *                                                                             *
 *                CreateDibPalette()  - Creates a GDI palette given a HANDLE   *
 *                                      to a BITMAPINFO structure.             *
 *                                                                             *
 *                ReadDibBitmapInfo() - Reads a file in DIB format and returns *
 *                                      a global handle to it's BITMAPINFO     *
 *                                                                             *
 *                PaletteSize()       - Calculates the palette size in bytes   *
 *                                      of given DIB                           *
 *                                                                             *
 *                DibNumColors()      - Determines the number of colors in DIB *
 *                                                                             *
 *                BitmapFromDib()     - Creates a DDB given a global handle to *
 *                                      a block in CF_DIB format.              *
 *                                                                             *
 *                DibFromBitmap()     - Creates a DIB repr. the DDB passed in. *
 *                                                                             *
 *                DrawBitmap()        - Draws a bitmap at specified position   *
 *                                      in the DC.                             *
 *                                                                             *
 *                DibBlt()            - Draws a bitmap in CIF_DIB format using *
 *                                      SetDIBitsToDevice()                    *
 *                                                                             *
 *                StretchDibBlt()     - Draws a bitmap in CIF_DIB format using *
 *                                      StretchDIBits()                        *
 *                                                                             *
 *                lread()             - Private routine to read more than 64k  *
 *                                                                             *
 *                lwrite()            - Private routine to write more than 64k *
 *                                                                             *
 *******************************************************************************/

#include "xsystem.h"
//#include "showdib.h"
static   HCURSOR hcurSave;

#define MAXREAD  32768                 /* Number of bytes to be read during */
                                       /* each read operation.              */

HANDLE ReadDibBitmapInfo (HANDLE fh);
BOOL DibInfo (HANDLE hbi, LPBITMAPINFOHEADER lpbi);
VOID ReadBitMapFileHeaderandConvertToDwordAlign(HANDLE hFile, LPBITMAPFILEHEADER pbf, LPDWORD lpdwoff);
VOID WriteMapFileHeaderandConvertFromDwordAlignToPacked(HANDLE fh, LPBITMAPFILEHEADER pbf);
WORD DibNumColors (VOID FAR * pv);
WORD PaletteSize (VOID FAR * pv);

#define ISDIB(bft) ((bft) == BFT_BITMAP)

#define PALVERSION      0x300
#define MAXPALETTE      256       /* max. # supported palette entries */
#define WIDTHBYTES(i)   ((i+31)/32*4)

#define BFT_BITMAP 0x4d42   /* 'BM' */

#define SIZEOF_BITMAPFILEHEADER_PACKED  (   \
    sizeof(WORD) +      /* bfType      */   \
    sizeof(DWORD) +     /* bfSize      */   \
    sizeof(WORD) +      /* bfReserved1 */   \
    sizeof(WORD) +      /* bfReserved2 */   \
    sizeof(DWORD))      /* bfOffBits   */

/****************************************************************************
 *                                                                          *
 *  FUNCTION   :OpenDIB(const WCHAR *szFile)                                *
 *                                                                          *
 *  PURPOSE    :Open a DIB file and create a MEMORY DIB, a memory handle    *
 *              containing BITMAPINFO, palette data and the bits.           *
 *                                                                          *
 *  RETURNS    :A handle to the DIB.                                        *
 *                                                                          *
 ****************************************************************************/
HANDLE OpenDIB (const WCHAR *szFile)
{
//    HFILE               fh;
    BITMAPINFOHEADER    bi;
    LPBITMAPINFOHEADER  lpbi;
    DWORD               dwLen = 0;
    DWORD               dwBits;
    HANDLE              hdib;
    HANDLE              h;
//    OFSTRUCT            of;

    HANDLE hFile = CreateFileW(szFile, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);

    if (hFile == INVALID_HANDLE_VALUE)
    {
       BASSERT(0);
       return FALSE;
    }

//    /* Open the file and read the DIB information */
//    fh = OpenFile(szFile, &of, (UINT)OF_READ);
//    if (fh == -1)
//        return NULL;

    hdib = ReadDibBitmapInfo(hFile);
    if (!hdib)
    {
       CloseHandle(hFile);
        return NULL;
    }
    DibInfo(hdib,&bi);

    /* Calculate the memory needed to hold the DIB */
    dwBits = bi.biSizeImage;
    DWORD paletteSize = (DWORD)PaletteSize(&bi);
    dwLen  = bi.biSize + paletteSize + dwBits;
    BASSERT((dwLen >= bi.biSize) && (dwLen >= paletteSize) && (dwLen >= dwBits));

    /* Try to increase the size of the bitmap info. buffer to hold the DIB */
    h = GlobalReAlloc(hdib, dwLen, GHND);
    if (!h)
    {
       CloseHandle(hFile);
       GlobalFree(hdib);
       hdib = NULL;
    }
    else
        hdib = h;

    /* Read in the bits */
    if (hdib)
    {

        lpbi = (LPBITMAPINFOHEADER) (VOID FAR *)GlobalLock(hdib);
//        _lread(fh, (LPSTR)lpbi + (WORD)lpbi->biSize + PaletteSize(lpbi), dwBits);
        DWORD dwBytes;
        BOOL readSuccess = ReadFile(hFile, (LPSTR)lpbi + (WORD)lpbi->biSize + PaletteSize(lpbi), dwBits, &dwBytes, 0);
        GlobalUnlock(hdib);

        // Confirm correct number of bytes read
        if (!readSuccess || (dwBits != dwBytes))
        {
           CloseHandle(hFile);
           GlobalFree(hdib);
           return NULL;
        }
    }
//    _lclose(fh);

    CloseHandle(hFile);
    
    return hdib;
}

/****************************************************************************
 *                                                                          *
 *  FUNCTION   : WriteDIB(LPSTR szFile,HANDLE hdib)                         *
 *                                                                          *
 *  PURPOSE    : Write a global handle in CF_DIB format to a file.          *
 *                                                                          *
 *  RETURNS    : TRUE  - if successful.                                     *
 *               FALSE - otherwise                                          *
 *                                                                          *
 ****************************************************************************/
BOOL WriteDIB (
    LPSTR szFile,
    HANDLE hdib)
{
    BITMAPFILEHEADER    hdr;
    LPBITMAPINFOHEADER  lpbi;
    HANDLE              fh;

    if (!hdib || !szFile)
        return FALSE;

    // Open file
    fh = CreateFileA(szFile, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, 0, NULL);
    if (fh == INVALID_HANDLE_VALUE)
        return FALSE;

    lpbi = (LPBITMAPINFOHEADER) (VOID FAR *)GlobalLock (hdib);

    /* Fill in the fields of the file header */
    hdr.bfType          = BFT_BITMAP;
    hdr.bfSize          = GlobalSize (hdib) + SIZEOF_BITMAPFILEHEADER_PACKED;
    hdr.bfReserved1     = 0;
    hdr.bfReserved2     = 0;
    hdr.bfOffBits       = (DWORD) (SIZEOF_BITMAPFILEHEADER_PACKED + lpbi->biSize +
                          PaletteSize(lpbi));

    /* Write the file header */
    WriteMapFileHeaderandConvertFromDwordAlignToPacked(fh, &hdr);

        /* this struct already DWORD aligned!*/
    /* Write the DIB header and the bits */
    DWORD numWritten;
    WriteFile(fh, (LPSTR) lpbi, GlobalSize(hdib), &numWritten, NULL);

    GlobalUnlock (hdib);
    CloseHandle(fh);
    return TRUE;
}

/****************************************************************************
 *                                                                          *
 *  FUNCTION   : DibInfo(HANDLE hbi,LPBITMAPINFOHEADER lpbi)                *
 *                                                                          *
 *  PURPOSE    : Retrieves the DIB info associated with a CF_DIB            *
 *               format memory block.                                       *
 *                                                                          *
 *  RETURNS    : TRUE  - if successful.                                     *
 *               FALSE - otherwise                                          *
 *                                                                          *
 ****************************************************************************/
BOOL DibInfo (HANDLE hbi, LPBITMAPINFOHEADER lpbi)
{
    if (hbi){
        *lpbi = *(LPBITMAPINFOHEADER)GlobalLock (hbi);

        /* fill in the default fields */
        if (lpbi->biSize != sizeof (BITMAPCOREHEADER)){
            if (lpbi->biSizeImage == 0L)
                                lpbi->biSizeImage = WIDTHBYTES(lpbi->biWidth*lpbi->biBitCount) * lpbi->biHeight;

            if (lpbi->biClrUsed == 0L)
                                lpbi->biClrUsed = DibNumColors (lpbi);
    }
        GlobalUnlock (hbi);
        return TRUE;
    }
    return FALSE;
}

/****************************************************************************
 *                                                                          *
 *  FUNCTION   : CreateBIPalette(LPBITMAPINFOHEADER lpbi)                   *
 *                                                                          *
 *  PURPOSE    : Given a Pointer to a BITMAPINFO struct will create a       *
 *               a GDI palette object from the color table.                 *
 *                                                                          *
 *  RETURNS    : A handle to the palette.                                   *
 *                                                                          *
 ****************************************************************************/
HPALETTE CreateBIPalette (LPBITMAPINFOHEADER lpbi)
{
    LOGPALETTE          *pPal;
    HPALETTE            hpal = NULL;
    WORD                nNumColors;
    BYTE                red;
    BYTE                green;
    BYTE                blue;
    WORD                i;
    RGBQUAD        FAR *pRgb;

    if (!lpbi)
        return NULL;

    if (lpbi->biSize != sizeof(BITMAPINFOHEADER))
        return NULL;

    /* Get a pointer to the color table and the number of colors in it */
    pRgb = (RGBQUAD FAR *)((LPSTR)lpbi + (WORD)lpbi->biSize);
    nNumColors = DibNumColors(lpbi);

    if (nNumColors){
        /* Allocate for the logical palette structure */
        pPal = (LOGPALETTE*)LocalAlloc(LPTR,sizeof(LOGPALETTE) + nNumColors * sizeof(PALETTEENTRY));
        if (!pPal)
            return NULL;

        pPal->palNumEntries = nNumColors;
        pPal->palVersion    = PALVERSION;

        /* Fill in the palette entries from the DIB color table and
         * create a logical color palette.
         */
        for (i = 0; i < nNumColors; i++){
            pPal->palPalEntry[i].peRed   = pRgb[i].rgbRed;
            pPal->palPalEntry[i].peGreen = pRgb[i].rgbGreen;
            pPal->palPalEntry[i].peBlue  = pRgb[i].rgbBlue;
            pPal->palPalEntry[i].peFlags = (BYTE)0;
        }
        hpal = CreatePalette(pPal);
        LocalFree((HANDLE)pPal);
    }
    else if (lpbi->biBitCount == 24){
        /* A 24 bitcount DIB has no color table entries so, set the number of
         * to the maximum value (256).
         */
        nNumColors = MAXPALETTE;
        pPal = (LOGPALETTE*)LocalAlloc(LPTR,sizeof(LOGPALETTE) + nNumColors * sizeof(PALETTEENTRY));
        if (!pPal)
            return NULL;

        pPal->palNumEntries = nNumColors;
        pPal->palVersion    = PALVERSION;

        red = green = blue = 0;

        /* Generate 256 (= 8*8*4) RGB combinations to fill the palette
         * entries.
         */
        for (i = 0; i < pPal->palNumEntries; i++){
            pPal->palPalEntry[i].peRed   = red;
            pPal->palPalEntry[i].peGreen = green;
            pPal->palPalEntry[i].peBlue  = blue;
            pPal->palPalEntry[i].peFlags = (BYTE)0;

            if (!(red += 32))
                if (!(green += 32))
                    blue += 64;
        }
        hpal = CreatePalette(pPal);
        LocalFree((HANDLE)pPal);
    }
    return hpal;
}

/****************************************************************************
 *                                                                          *
 *  FUNCTION   : CreateDibPalette(HANDLE hbi)                               *
 *                                                                          *
 *  PURPOSE    : Given a Global HANDLE to a BITMAPINFO Struct               *
 *               will create a GDI palette object from the color table.     *
 *               (BITMAPINFOHEADER format DIBs only)                                     *
 *                                                                          *
 *  RETURNS    : A handle to the palette.                                   *
 *                                                                          *
 ****************************************************************************/
HPALETTE CreateDibPalette (HANDLE hbi)
{
    HPALETTE hpal;

    if (!hbi)
        return NULL;
    hpal = CreateBIPalette((LPBITMAPINFOHEADER)GlobalLock(hbi));
    GlobalUnlock(hbi);
    return hpal;
}

/****************************************************************************
 *                                                                          *
 *  FUNCTION   : ReadDibBitmapInfo(int fh)                                  *
 *                                                                          *
 *  PURPOSE    : Will read a file in DIB format and return a global HANDLE  *
 *               to it's BITMAPINFO.  This function will work with both     *
 *               "old" (BITMAPCOREHEADER) and "new" (BITMAPINFOHEADER)      *
 *               bitmap formats, but will always return a "new" BITMAPINFO  *
 *                                                                          *
 *  RETURNS    : A handle to the BITMAPINFO of the DIB in the file.         *
 *                                                                          *
 ****************************************************************************/
HANDLE ReadDibBitmapInfo (HANDLE hFile)
{
    DWORD     off;
    HANDLE    hbi = NULL;
    INT       i;
    WORD      nNumColors;

    RGBQUAD FAR       *pRgb;
    BITMAPINFOHEADER   bi;
    BITMAPCOREHEADER   bc;
    LPBITMAPINFOHEADER lpbi;
    BITMAPFILEHEADER   bf;

    memset(&bi, 0, sizeof(bi));
    memset(&bc, 0, sizeof(bc));

    if (hFile == INVALID_HANDLE_VALUE)
        return NULL;

    ReadBitMapFileHeaderandConvertToDwordAlign(hFile, &bf, &off);
        /* at this point we have read the file into bf*/

    /* Do we have a RC HEADER? */
    if (!ISDIB (bf.bfType)) 
    {    
        bf.bfOffBits = 0L;               
//        _llseek(fh, off, (UINT)SEEK_SET); /*seek back to beginning of file*/

         SetFilePointer(hFile, 0, 0, FILE_BEGIN);
    }

    DWORD dwBytes;
    BOOL readSuccess;

    // Read the size of the bitmap header to determine its type
    readSuccess = ReadFile(hFile, &(bi.biSize), sizeof(bi.biSize), &dwBytes, 0);
    if (!readSuccess || (sizeof(bi.biSize) != dwBytes))
        return FALSE;

    DWORD headerSize = bi.biSize;
    BYTE *headerAddress = NULL;
    switch (headerSize)
    {
        case sizeof (BITMAPINFOHEADER):
            headerAddress = (BYTE*) &bi;
            break;

        case sizeof (BITMAPCOREHEADER):
            bc.bcSize = headerSize;
            headerAddress = (BYTE*) &bc;
            break;

        default:
            /* Not a DIB! */
            return NULL;
    }

    // Read remaining header
    readSuccess = ReadFile(hFile, headerAddress + sizeof(bi.biSize), headerSize - sizeof(bi.biSize), &dwBytes, 0);
    if (!readSuccess || ((headerSize - sizeof(bi.biSize)) != dwBytes))
        return FALSE;

    nNumColors = DibNumColors(headerAddress);

    /* Check the nature (BITMAPINFO or BITMAPCORE) of the info. block
     * and extract the field information accordingly. If a BITMAPCOREHEADER,
     * transfer it's field information to a BITMAPINFOHEADER-style block
     */
    switch (headerSize)
    {
        case sizeof (BITMAPINFOHEADER):
            break;

        case sizeof (BITMAPCOREHEADER):
            bi.biSize               = sizeof(BITMAPINFOHEADER);
            bi.biWidth              = bc.bcWidth;
            bi.biHeight             = bc.bcHeight;
            bi.biPlanes             = bc.bcPlanes;
            bi.biBitCount           = bc.bcBitCount;

            bi.biCompression        = BI_RGB;
            bi.biSizeImage          = 0;
            bi.biXPelsPerMeter      = 0;
            bi.biYPelsPerMeter      = 0;
            bi.biClrUsed            = nNumColors;
            bi.biClrImportant       = nNumColors;
            break;

        default:
            /* Not a DIB! */
            return NULL;
    }

    /*  Fill in some default values if they are zero */
    if (bi.biSizeImage == 0){
        bi.biSizeImage = WIDTHBYTES ((DWORD)bi.biWidth * bi.biBitCount)
                         * bi.biHeight;
    }
    if (bi.biClrUsed == 0)
        bi.biClrUsed = DibNumColors(&bi);

    /* Allocate for the BITMAPINFO structure and the color table. */
    hbi = GlobalAlloc (GHND, (LONG)bi.biSize + nNumColors * sizeof(RGBQUAD));
    if (!hbi)
        return NULL;
    lpbi = (LPBITMAPINFOHEADER) (VOID FAR *)GlobalLock (hbi);
    *lpbi = bi;

    /* Get a pointer to the color table */
    pRgb = (RGBQUAD FAR *)((LPSTR)lpbi + bi.biSize);
    if (nNumColors)
    {
        if (headerSize == sizeof(BITMAPCOREHEADER))
        {
            /* Convert a old color table (3 byte RGBTRIPLEs) to a new
             * color table (4 byte RGBQUADs)
             */
//            _lread(fh, (LPSTR)pRgb, (UINT)nNumColors * sizeof(RGBTRIPLE));

            readSuccess = ReadFile(hFile, (LPSTR)pRgb, (UINT)nNumColors * sizeof(RGBTRIPLE), &dwBytes, 0);
            BASSERT(readSuccess && (dwBytes == (nNumColors * sizeof(RGBTRIPLE))));

            for (i = nNumColors - 1; i >= 0; i--)
            {
                RGBQUAD rgb;

                rgb.rgbRed      = ((RGBTRIPLE FAR *)pRgb)[i].rgbtRed;
                rgb.rgbBlue     = ((RGBTRIPLE FAR *)pRgb)[i].rgbtBlue;
                rgb.rgbGreen    = ((RGBTRIPLE FAR *)pRgb)[i].rgbtGreen;
                rgb.rgbReserved = (BYTE)0;

                pRgb[i] = rgb;
            }
        }
        else
        {
//            _lread(fh, (LPSTR)pRgb, (UINT)nNumColors * sizeof(RGBQUAD));
           readSuccess = ReadFile(hFile, (LPSTR)pRgb, (UINT)nNumColors * sizeof(RGBQUAD), &dwBytes, 0);
           BASSERT(readSuccess && (dwBytes == (nNumColors * sizeof(RGBQUAD))));
        }
    }

    if (bf.bfOffBits != 0L)
    {
//        _llseek(fh, off + bf.bfOffBits, (UINT)SEEK_SET);
       SetFilePointer(hFile, off + bf.bfOffBits, 0, FILE_BEGIN);
    }
    GlobalUnlock(hbi);
    return hbi;
}

/****************************************************************************
 *                                                                          *
 *  FUNCTION   :  PaletteSize(VOID FAR * pv)                                *
 *                                                                          *
 *  PURPOSE    :  Calculates the palette size in bytes. If the info. block  *
 *                is of the BITMAPCOREHEADER type, the number of colors is  *
 *                multiplied by 3 to give the palette size, otherwise the   *
 *                number of colors is multiplied by 4.                                                          *
 *                                                                          *
 *  RETURNS    :  Palette size in number of bytes.                          *
 *                                                                          *
 ****************************************************************************/
WORD PaletteSize (VOID FAR * pv)
{
    LPBITMAPINFOHEADER lpbi;
    WORD               NumColors;

    lpbi      = (LPBITMAPINFOHEADER)pv;
    NumColors = DibNumColors(lpbi);

    if (lpbi->biSize == sizeof(BITMAPCOREHEADER))
        return (WORD)(NumColors * sizeof(RGBTRIPLE));
    else
        return (WORD)(NumColors * sizeof(RGBQUAD));
}

/****************************************************************************
 *                                                                          *
 *  FUNCTION   : DibNumColors(VOID FAR * pv)                                *
 *                                                                          *
 *  PURPOSE    : Determines the number of colors in the DIB by looking at   *
 *               the BitCount filed in the info block.                      *
 *                                                                          *
 *  RETURNS    : The number of colors in the DIB.                           *
 *                                                                          *
 ****************************************************************************/
WORD DibNumColors (VOID FAR * pv)
{
    INT                 bits;
    LPBITMAPINFOHEADER  lpbi;
    LPBITMAPCOREHEADER  lpbc;

    lpbi = ((LPBITMAPINFOHEADER)pv);
    lpbc = ((LPBITMAPCOREHEADER)pv);

    /*  With the BITMAPINFO format headers, the size of the palette
     *  is in biClrUsed, whereas in the BITMAPCORE - style headers, it
     *  is dependent on the bits per pixel ( = 2 raised to the power of
     *  bits/pixel).
     */
    if (lpbi->biSize != sizeof(BITMAPCOREHEADER)){
        if (lpbi->biClrUsed != 0)
            return (WORD)lpbi->biClrUsed;
        bits = lpbi->biBitCount;
    }
    else
        bits = lpbc->bcBitCount;

    switch (bits){
        case 1:
                return 2;
        case 4:
                return 16;
        case 8:
                return 256;
        default:
                /* A 24 bitcount DIB has no color table */
                return 0;
    }
}
/****************************************************************************
 *                                                                          *
 *  FUNCTION   : DibFromBitmap()                                            *
 *                                                                          *
 *  PURPOSE    : Will create a global memory block in DIB format that       *
 *               represents the Device-dependent bitmap (DDB) passed in.    *
 *                                                                          *
 *  RETURNS    : A handle to the DIB                                        *
 *                                                                          *
 ****************************************************************************/
HANDLE DibFromBitmap (
    HBITMAP      hbm,
    DWORD            biStyle,
    WORD             biBits,
    HPALETTE     hpal)
{
    BITMAP               bm;
    BITMAPINFOHEADER     bi;
    BITMAPINFOHEADER FAR *lpbi;
    DWORD                dwLen;
    HANDLE               hdib;
    HANDLE               h;
    HDC                  hdc;

    if (!hbm)
        return NULL;

    if (hpal == NULL)
        hpal = (HPALETTE) GetStockObject(DEFAULT_PALETTE);

    GetObject(hbm,sizeof(bm),(LPSTR)&bm);

    if (biBits == 0)
        biBits =  (WORD) (bm.bmPlanes * bm.bmBitsPixel);

    bi.biSize               = sizeof(BITMAPINFOHEADER);
    bi.biWidth              = bm.bmWidth;
    bi.biHeight             = bm.bmHeight;
    bi.biPlanes             = 1;
    bi.biBitCount           = biBits;
    bi.biCompression        = biStyle;
    bi.biSizeImage          = 0;
    bi.biXPelsPerMeter      = 0;
    bi.biYPelsPerMeter      = 0;
    bi.biClrUsed            = 0;
    bi.biClrImportant       = 0;

    dwLen  = bi.biSize + PaletteSize(&bi);

    hdc = GetDC(NULL);
    hpal = SelectPalette(hdc,hpal,FALSE);
         RealizePalette(hdc);

    hdib = GlobalAlloc(GHND,dwLen);

    if (!hdib){
        SelectPalette(hdc,hpal,FALSE);
        ReleaseDC(NULL,hdc);
        return NULL;
    }

    lpbi = (LPBITMAPINFOHEADER) (VOID FAR *)GlobalLock(hdib);

    *lpbi = bi;

    /*  call GetDIBits with a NULL lpBits param, so it will calculate the
     *  biSizeImage field for us
     */
    GetDIBits(hdc, hbm, 0L, (DWORD)bi.biHeight,
        (LPBYTE)NULL, (LPBITMAPINFO)lpbi, (DWORD)DIB_RGB_COLORS);

    bi = *lpbi;
    GlobalUnlock(hdib);

    /* If the driver did not fill in the biSizeImage field, make one up */
    if (bi.biSizeImage == 0){
        bi.biSizeImage = WIDTHBYTES((DWORD)bm.bmWidth * biBits) * bm.bmHeight;

        if (biStyle != BI_RGB)
            bi.biSizeImage = (bi.biSizeImage * 3) / 2;
    }

    /*  realloc the buffer big enough to hold all the bits */
    dwLen = bi.biSize + PaletteSize(&bi) + bi.biSizeImage;
    h = GlobalReAlloc(hdib,dwLen,0);

    if (h)
        hdib = h;
    else{
        GlobalFree(hdib);
        hdib = NULL;

        SelectPalette(hdc,hpal,FALSE);
        ReleaseDC(NULL,hdc);
        return hdib;
    }

    /*  call GetDIBits with a NON-NULL lpBits param, and actualy get the
     *  bits this time
     */
    lpbi = (LPBITMAPINFOHEADER) (VOID FAR *)GlobalLock(hdib);

    if (GetDIBits( hdc,
                   hbm,
                   0L,
                   (DWORD)bi.biHeight,
                   (LPBYTE)lpbi + (WORD)lpbi->biSize + PaletteSize(lpbi),
                   (LPBITMAPINFO)lpbi, (DWORD)DIB_RGB_COLORS) == 0){
         GlobalUnlock(hdib);
         GlobalFree(hdib);
         hdib = NULL;
         SelectPalette(hdc,hpal,FALSE);
         ReleaseDC(NULL,hdc);
         return NULL;
    }

    bi = *lpbi;
    GlobalUnlock(hdib);

    SelectPalette(hdc,hpal,FALSE);
    ReleaseDC(NULL,hdc);
    return hdib;
}

/****************************************************************************
 *                                                                          *
 *  FUNCTION   : BitmapFromDib(HANDLE hdib, HPALETTE hpal)                  *
 *                                                                          *
 *  PURPOSE    : Will create a DDB (Device Dependent Bitmap) given a global *
 *               handle to a memory block in CF_DIB format                  *
 *                                                                          *
 *  RETURNS    : A handle to the DDB.                                       *
 *                                                                          *
 ****************************************************************************/
HBITMAP BitmapFromDib (
    HANDLE         hdib,
    HPALETTE   hpal)
{
    LPBITMAPINFOHEADER  lpbi;
    HPALETTE            hpalT = 0;
    HDC                 hdc;
    HBITMAP             hbm;

//    StartWait();

    if (!hdib)
        return NULL;

    lpbi = (LPBITMAPINFOHEADER) (VOID FAR *)GlobalLock(hdib);

    if (!lpbi)
        return NULL;

    hdc = GetDC(NULL);

    if (hpal){
        hpalT = SelectPalette(hdc,hpal,FALSE);
        RealizePalette(hdc);     // GDI Bug...????
    }

    hbm = CreateDIBitmap(hdc,
                (LPBITMAPINFOHEADER)lpbi,
                (LONG)CBM_INIT,
                (LPSTR)lpbi + lpbi->biSize + PaletteSize(lpbi),
                (LPBITMAPINFO)lpbi,
                DIB_RGB_COLORS );

    if (hpal)
        SelectPalette(hdc,hpalT,FALSE);

    ReleaseDC(NULL,hdc);
    GlobalUnlock(hdib);

//    EndWait();

    return hbm;
}
/****************************************************************************
 *                                                                          *
 *  FUNCTION   : DrawBitmap(HDC hdc, int x, int y, HBITMAP hbm, DWORD rop)  *
 *                                                                          *
 *  PURPOSE    : Draws bitmap <hbm> at the specifed position in DC <hdc>    *
 *                                                                          *
 *  RETURNS    : Return value of BitBlt()                                   *
 *                                                                          *
 ****************************************************************************/
BOOL DrawBitmap (
    HWND hwnd,
    HDC    hdc,
    INT    x,
    INT    y,
    HBITMAP    hbm,
    DWORD          rop)
{
    HDC       hdcBits;
    BITMAP    bm;
//    HPALETTE  hpalT;  
    BOOL      f;

    if (!hdc || !hbm)
        return FALSE;

    RECT rect;

    GetClientRect(hwnd, &rect);

    hdcBits = CreateCompatibleDC(hdc);
    GetObject(hbm,sizeof(BITMAP),(LPSTR)&bm);
    HGDIOBJ hOld = SelectObject(hdcBits,hbm);
//    f = BitBlt(hdc,0,0,bm.bmWidth,bm.bmHeight,hdcBits,0,0,rop);
//    f = BitBlt(hdc,0,0,rect.right - rect.left,rect.bottom - rect.top,hdcBits,0,0,rop);

    f = StretchBlt(hdc,0,0,rect.right - rect.left,rect.bottom - rect.top,hdcBits,0,0,bm.bmWidth,bm.bmHeight,rop);

    SelectObject(hdcBits,hOld);
    
    DeleteDC(hdcBits);

    return f;
        UNREFERENCED_PARAMETER(y);
        UNREFERENCED_PARAMETER(x);
}
/****************************************************************************
 *                                                                          *
 *  FUNCTION   : DibBlt( HDC hdc,                                           *
 *                       int x0, int y0,                                    *
 *                       int dx, int dy,                                    *
 *                       HANDLE hdib,                                       *
 *                       int x1, int y1,                                    *
 *                       LONG rop)                                          *
 *                                                                          *
 *  PURPOSE    : Draws a bitmap in CF_DIB format, using SetDIBits to device.*
 *               taking the same parameters as BitBlt().                    *
 *                                                                          *
 *  RETURNS    : TRUE  - if function succeeds.                              *
 *               FALSE - otherwise.                                         *
 *                                                                          *
 ****************************************************************************/
BOOL DibBlt (
    HDC    hdc,
    INT    x0,
    INT    y0,
    INT    dx,
    INT    dy,
    HANDLE hdib,
    INT    x1,
    INT    y1,
    LONG   rop)
{
    LPBITMAPINFOHEADER   lpbi;
//    HPALETTE           hpal,hpalT;
    LPSTR                pBuf;
//    HDC                hdcMem;
//    HBITMAP            hbm,hbmT;

    if (!hdib)
        return PatBlt(hdc,x0,y0,dx,dy,rop);

    lpbi = (LPBITMAPINFOHEADER) (VOID FAR *)GlobalLock(hdib);

    if (!lpbi)
        return FALSE;

    pBuf = (LPSTR)lpbi + (WORD)lpbi->biSize + PaletteSize(lpbi);
    SetDIBitsToDevice (hdc, x0, y0, dx, dy,
                       x1,y1,
                       x1,
                       dy,
                       pBuf, (LPBITMAPINFO)lpbi,
                       DIB_RGB_COLORS );

    GlobalUnlock(hdib);
    return TRUE;
}
/****************************************************************************
 *                                                                          *
 *  FUNCTION   : StretchDibBlt( HDC hdc,                                    *
 *                              int x, int y,                               *
 *                              int dx, int dy,                             *
 *                              HANDLE hdib,                                *
 *                              int x0, int y0,                             *
 *                              int dx0, int dy0,                           *
 *                              LONG rop)                                   *
 *                                                                          *
 *  PURPOSE    : Draws a bitmap in CF_DIB format, using StretchDIBits()     *
 *               taking the same parameters as StretchBlt().                *
 *                                                                          *
 *  RETURNS    : TRUE  - if function succeeds.                              *
 *               FALSE - otherwise.                                         *
 *                                                                          *
 ****************************************************************************/
BOOL StretchDibBlt (
    HDC hdc,
    INT x,
    INT y,
    INT dx,
    INT dy,
    HANDLE hdib,
    INT x0,
    INT y0,
    INT dx0,
    INT dy0,
    LONG rop)

{
    LPBITMAPINFOHEADER lpbi;
    LPSTR        pBuf;
    BOOL         f;

    if (!hdib)
        return PatBlt(hdc,x,y,dx,dy,rop);

    lpbi = (LPBITMAPINFOHEADER) (VOID FAR *)GlobalLock(hdib);

    if (!lpbi)
        return FALSE;

    pBuf = (LPSTR)lpbi + (WORD)lpbi->biSize + PaletteSize(lpbi);

    f = StretchDIBits ( hdc,
                        x, y,
                        dx, dy,
                        x0, y0,
                        dx0, dy0,
                        pBuf, (LPBITMAPINFO)lpbi,
                        DIB_RGB_COLORS,
                        rop);

    GlobalUnlock(hdib);
    return f;
}

 /************* PRIVATE ROUTINES TO READ/WRITE MORE THAN 64K ***************/
/****************************************************************************
 *                                                                          *
 *  FUNCTION   : lread(int fh, VOID FAR *pv, DWORD ul)                      *
 *                                                                          *
 *  PURPOSE    : Reads data in steps of 32k till all the data has been read.*
 *                                                                          *
 *  RETURNS    : 0 - If read did not proceed correctly.                     *
 *               number of bytes read otherwise.                            *
 *                                                                          *
 ****************************************************************************/
/*
DWORD PASCAL lread (
    INT       fh,
    VOID FAR      *pv,
    DWORD             ul)
{
    DWORD     ulT = ul;
    BYTE *hp = (BYTE *) pv;

    while (ul > (DWORD)MAXREAD) {
        if (_lread(fh, (LPSTR)hp, (UINT)MAXREAD) != MAXREAD)
                return 0;
        ul -= MAXREAD;
        hp += MAXREAD;
    }
    if (_lread(fh, (LPSTR)hp, (UINT)ul) != (UINT)ul)
        return 0;
    return ulT;
}
*/

/****************************************************************************
 *                                                                          *
 *  FUNCTION   : lwrite(int fh, VOID FAR *pv, DWORD ul)                     *
 *                                                                          *
 *  PURPOSE    : Writes data in steps of 32k till all the data is written.  *
 *                                                                          *
 *  RETURNS    : 0 - If write did not proceed correctly.                    *
 *               number of bytes written otherwise.                         *
 *                                                                          *
 ****************************************************************************/
/*
DWORD PASCAL lwrite (
    INT      fh,
    VOID FAR     *pv,
    DWORD            ul)
{
    DWORD     ulT = ul;
    BYTE *hp = (BYTE *) pv;

    while (ul > MAXREAD) {
        if (_lwrite(fh, (LPSTR)hp, (UINT)MAXREAD) != MAXREAD)
                return 0;
        ul -= MAXREAD;
        hp += MAXREAD;
    }
    if (_lwrite(fh, (LPSTR)hp, (UINT)ul) != (UINT)ul)
        return 0;                 

    return ulT;
}
*/

/****************************************************************************
 *                                                                          *
 *  FUNCTION   : ReadBitMapFileHeaderandConvertToDwordAlign(HFILE fh, LPBITMAPFILEHEADER pbf)
 *                                                                          *
 *  PURPOSE    : read file header (which is packed) and convert into unpacked BITMAPFILEHEADER strucutre
 *                                                                          *
 *  RETURNS    : VOID
 *                                                                          *
 ****************************************************************************/

VOID ReadBitMapFileHeaderandConvertToDwordAlign(HANDLE hFile, LPBITMAPFILEHEADER pbf, LPDWORD lpdwoff)
{
        DWORD off = SetFilePointer(hFile, 0, 0, FILE_CURRENT); 

//        off = _llseek(fh, 0L, (UINT) SEEK_CUR);

        *lpdwoff = off;

/*              BITMAPFILEHEADER STRUCUTURE is as follows 
 *              BITMAPFILEHEADER
 *              WORD    bfType 
 >          ....                  <     add WORD if packed here!
 *              DWORD   bfSize 
 *              WORD    bfReserved1
 *              WORD    bfReserved2
 *              DWORD   bfOffBits 
 *                      This is the packed format, unpacked adds a WORD after bfType
 */

//        /* read in bfType*/
//        _lread(fh, (LPSTR) &pbf->bfType, sizeof(WORD));
//        /* read in last 3 dwords*/
//        _lread(fh, (LPSTR) &pbf->bfSize, sizeof(DWORD) * 3);

        DWORD dwBytes;

        BOOL ok;

        /* read in bfType*/
        ok = ReadFile(hFile, (LPSTR) &pbf->bfType, sizeof(WORD), &dwBytes, 0);
        BASSERT(ok);

        /* read in last 3 dwords*/
        ok = ReadFile(hFile, (LPSTR) &pbf->bfSize, sizeof(DWORD) * 3, &dwBytes, 0);
        BASSERT(ok);

}



/****************************************************************************
 *                                                                          *
 *  FUNCTION   : WriteMapFileHeaderandConvertFromDwordAlignToPacked(HANDLE fh, LPBITMAPFILEHEADER pbf)
 *                                                                          *
 *  PURPOSE    : write header structure (which NOT packed) and write it PACKED
 *                                                                          *
 *  RETURNS    : VOID
 *                                                                          *
 ****************************************************************************/

VOID WriteMapFileHeaderandConvertFromDwordAlignToPacked(HANDLE fh, LPBITMAPFILEHEADER pbf)
{
   DWORD numWritten;

   /* write bfType*/
   WriteFile(fh, (LPCVOID)&pbf->bfType, sizeof(WORD), &numWritten, NULL);
   /* now pass over extra word, and only write next 3 DWORDS!*/
   WriteFile(fh, (LPCVOID)&pbf->bfSize, sizeof(DWORD) * 3, &numWritten, NULL);
}

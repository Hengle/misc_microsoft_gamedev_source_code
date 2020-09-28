
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
 *  MODULE      : DrawDIB.c                                                    *
 *                                                                             *
 *  PURPOSE     : Handles most of the SHOWDIB's DIB drawing and clipboard      *
 *                operations.                                                  *
 *                                                                             *
 *  FUNCTIONS   :                                                              *
 *                PrintDIB()             -  Sets the current DIB bits to the   *
 *                                          printer DC.                        *
 *                                                                             *
 *                AppPaint()             -  Sets the DIB/bitmap bits on the    *
 *                                          screen or the given device.        *
 *                                                                             *
 *                DrawSelect()           -  Draws selected clip rectangle on   *
 *                                          the DC/screen.                     *
 *                                                                             *
 *                NormalizeRect()        -  Swaps reversed rectangle coords.   *
 *                                                                             *
 *                TrackMouse()           -  Draws rubberbanding rectangle and  *
 *                                          displays it's dimensions.          *
 *                                                                             *
 *                BandDIB()              -  Outputs DIB in bands to device.    *
 *                                                                             *
 *                SizeWindow()           -  Sizes app. window based on client  *
 *                                          dimensions and style.              *
 *                                                                             *
 *                GetRealClientRect()    -  Calculates client rectangle dimen- *
 *                                          sions if scrollbars are present.   *
 *                                                                             *
 *                SetScrollRanges()      -  Sets global scroll ranges.         *
 *                                                                             *
 *                CopyHandle()           -  Makes a copy of memory block.      *
 *                                                                             *
 *                CopyPalette()          -  Makes a copy of the GDI logical    *
 *                                          palette.                           *
 *                                                                             *
 *                CopyBitmap()           -  Copies given bitmap to another.    *
 *                                                                             *
 *                CropBitmap()           -  Crops a bitmap to the given size.  *
 *                                                                             *
 *                RenderFormat()         -  renders currently displayed DIB    *
 *                                          in CF_BITMAP or CF_DIB format.     *
 *                                                                             *
 *                RealizeDibFormat()     -  Realizes the DIB in given format.  *
 *                                                                             *
 *                ErrMsg()               -  Pops an error message to user.     *
 *                                                                             *
 *                fDialog()              -  Displays a dialog box.             *
 *                                                                             *
 *                AppAbout()             -  Shows the About.. dialog box.      *
 *                                                                             *
 *******************************************************************************/
#include "xsystem.h"
#include <windows.h>
#include <io.h>
#include <stdio.h>
//#include "showdib.h"

POINT                ptSize;       /* Stores DIB dimensions                   */

HPALETTE hpalCurrent   = NULL;         /* Handle to current palette            */
HANDLE   hdibCurrent   = NULL;         /* Handle to current memory DIB         */
HBITMAP  hbmCurrent    = NULL;         /* Handle to current memory BITMAP      */
HANDLE   hbiCurrent    = NULL;         /* Handle to current bitmap info struct */
RECT           rcClip;
DWORD          dwOffset;
BOOL    fPalColors  = FALSE;          /* TRUE if the current DIB's color table   */
                                      /* contains palette indexes not rgb values */

extern BOOL DrawBitmap(HWND hwnd, HDC hdc, INT x, INT y, HBITMAP hbm, DWORD rop);
extern BOOL DibInfo(HANDLE hbi, LPBITMAPINFOHEADER lpbi);
extern BOOL DibBlt(HDC hdc, INT x0, INT y0, INT dx, INT dy, HANDLE hdib, INT x1, INT y1, LONG rop);
extern HANDLE OpenDIB (const WCHAR *szFile);
extern HANDLE ReadDibBitmapInfo (HANDLE fh);
extern HPALETTE CreateDibPalette (HANDLE hbi);

VOID FreeDib();

/* Macro to swap two values */
#define SWAP(x,y)   ((x)^=(y)^=(x)^=(y))
#define WIDTHBYTES(i)   ((i+31)/32*4)

#define MINBAND         50     /* Minimum band size used by the program */
#define BANDINCREMENT   20     /* Decrement for band size while trying  */
#define PALVERSION      0x300
#define MAXPALETTE      256       /* max. # supported palette entries */
                               /* to determine optimum band size.       */

VOID BandDIB (HWND hWnd, HDC hDC, INT x, INT y);
VOID DrawSelect(HDC hdc, BOOL fDraw);
HBITMAP CropBitmap (HBITMAP hbm, PRECT prc);
HBITMAP BitmapFromDib (HANDLE hdib, HPALETTE hpal);
HANDLE DibFromBitmap (HBITMAP hbm, DWORD biStyle, WORD biBits, HPALETTE hpal);

/****************************************************************************
 *                                                                          *
 *  FUNCTION   :  PrintDIB(HWND hWnd, HDC hDC, int x, int y, int dx, int dy)*
 *                                                                          *
 *  PURPOSE    :  Set the DIB bits to the printer DC.                       *
 *                                                                          *
 ****************************************************************************/
/*
VOID PrintDIB (
    HWND hWnd,
    HDC hDC,
    INT x,
    INT y,
    INT dx,
    INT dy)

{
    BITMAPINFOHEADER bi;
    INT dibX,  dibY;
    INT dibDX, dibDY;

    if (!bLegitDraw)
        return;

    DibInfo (hbiCurrent, &bi);

    if (IsRectEmpty (&rcClip)){
        dibX  = 0;
        dibY  = 0;
        dibDX = (INT)bi.biWidth;
        dibDY = (INT)bi.biHeight;
    }
    else{
        dibX  = rcClip.left;
        dibY  = (INT)bi.biHeight - 1 - rcClip.bottom;
        dibDX = rcClip.right  - rcClip.left;
        dibDY = rcClip.bottom - rcClip.top;
    }

    if (hdibCurrent){
        
        StretchDibBlt ( hDC,
                        x,
                        y,
                        dx,
                        dy,
                        hdibCurrent,
                        dibX,
                        dibY,
                        dibDX,
                        dibDY,
                        SRCCOPY);
    }
    else if (achFileName[0]) {

        SetMapMode (hDC, MM_ANISOTROPIC);
        (VOID)SetViewportOrgEx (hDC, x, y, NULL);
        (VOID)SetViewportExtEx (hDC, dx, dy, NULL);

        BandDIB (hWnd, hDC, 0, 0);
    }
}
*/

/****************************************************************************
 *                                                                          *
 *  FUNCTION   :  AppPaint(HWND hWnd, HDC hDC, int x, int y)                *
 *                                                                          *
 *  PURPOSE    :  Sets the DIB/bitmap bits on the screen or the given device*
 *                                                                          *
 ****************************************************************************/
VOID AppPaint (
    HWND hwnd,
    HDC hDC,
    INT x,
    INT y)
{
    HPALETTE hpalT;
//    BITMAPINFOHEADER bi;
//    LPBITMAPINFOHEADER lpbi;

    (VOID)SetWindowOrgEx (hDC, x, y, NULL);
    SetBkMode (hDC, TRANSPARENT); // wTransparent);

    //if (true) { // bLegitDraw)
                hpalT = SelectPalette (hDC, hpalCurrent, FALSE);
                RealizePalette (hDC);

//        if (hbmCurrent && true) { // !bDIBToDevice) 
            DrawBitmap (hwnd, hDC, 0, 0, hbmCurrent, SRCCOPY);
/*
        }
        else if (hdibCurrent) {
            DibInfo (hdibCurrent, &bi);
            DibBlt (hDC,
                    0,
                    0,
                    (INT)bi.biWidth,
                    (INT)bi.biHeight,
                    hdibCurrent,
                    0,
                    0,
                    SRCCOPY);
        }
        else { // if (achFileName[0]) 
           BASSERT(0);
//            BandDIB (hWnd, hDC, 0, 0);
        }
*/
        SelectPalette(hDC,hpalT,FALSE);
    //}

    DrawSelect(hDC, TRUE);
}

/****************************************************************************
 *                                                                          *
 *  FUNCTION   :  DrawSelect(HDC hdc, BOOL fDraw)                           *
 *                                                                          *
 *  PURPOSE    :  Draws the selected clip rectangle with its dimensions on  *
 *                the DC/screen                                             *
 *                                                                          *
 ****************************************************************************/
VOID DrawSelect(
    HDC hdc,
    BOOL fDraw)
{
    BCHAR_T sz[80];
    INT   x,y,len,dx,dy;
    HDC   hdcBits;
    HBITMAP hbm;

    if (!IsRectEmpty (&rcClip)) {

        /* If a rectangular clip region has been selected, draw it */
        PatBlt(hdc, rcClip.left,    rcClip.top,        rcClip.right-rcClip.left, 1,  DSTINVERT);
        PatBlt(hdc, rcClip.left,    rcClip.bottom, 1, -(rcClip.bottom-rcClip.top),   DSTINVERT);
        PatBlt(hdc, rcClip.right-1, rcClip.top, 1,   rcClip.bottom-rcClip.top,   DSTINVERT);
        PatBlt(hdc, rcClip.right,   rcClip.bottom-1, -(rcClip.right-rcClip.left), 1, DSTINVERT);

        /* Format the dimensions string ...*/
        StringCchPrintf(sz,
                        countof(sz),
                        B("%dx%d"),
                        rcClip.right  - rcClip.left,
                        rcClip.bottom - rcClip.top );
        len = lstrlen(sz);

        /* ... and center it in the rectangle */
        { SIZE size;
        (VOID)GetTextExtentPoint(hdc, sz, len, &size);
        dx = size.cx; dy = size.cy;
        }
        x  =  (rcClip.right  + rcClip.left - dx) / 2;
        y  =  (rcClip.bottom + rcClip.top  - dy) / 2;

        hdcBits = CreateCompatibleDC (hdc);
        SetTextColor (hdcBits, 0xFFFFFFL);
        SetBkColor (hdcBits, 0x000000L);

        /* Output the text to the DC */
        /*if (hbm = +++CreateBitmap - Not Recommended(use CreateDIBitmap)+++ (dx, dy, 1, 1, NULL)){*/
        hbm = CreateBitmap(dx, dy, 1, 1, NULL);

        if (hbm){
            hbm = (HBITMAP) SelectObject (hdcBits, hbm);
            ExtTextOut (hdcBits, 0, 0, 0, NULL, sz, len, NULL);
            BitBlt (hdc, x, y, dx, dy, hdcBits, 0, 0, SRCINVERT);
            hbm = (HBITMAP) SelectObject (hdcBits, hbm);
            DeleteObject (hbm);
        }
        DeleteDC (hdcBits);
        UNREFERENCED_PARAMETER(fDraw);
    }
}
/****************************************************************************
 *                                                                          *
 *  FUNCTION   : NormalizeRect(RECT *prc)                                   *
 *                                                                          *
 *  PURPOSE    : If the rectangle coordinates are reversed, swaps them      *
 *                                                                          *
 ****************************************************************************/
VOID PASCAL NormalizeRect (RECT *prc)
{
    if (prc->right < prc->left)
        SWAP(prc->right,prc->left);
    if (prc->bottom < prc->top)
        SWAP(prc->bottom,prc->top);
}

/****************************************************************************
 *                                                                          *
 *  FUNCTION   : TrackMouse(HWND hwnd, POINT pt)                            *
 *                                                                          *
 *  PURPOSE    : Draws a rubberbanding rectangle and displays it's          *
 *               dimensions till the mouse button is released               *
 *                                                                          *
 ****************************************************************************/
VOID TrackMouse (
    HWND hwnd,
    POINT pt)
{
    HDC   hdc;
    MSG   msg;
    POINT ptOrigin;
    RECT  rcClient;

    hdc = GetDC(hwnd);
    SetCapture(hwnd);

    GetClientRect(hwnd,&rcClient);

    /* Get mouse coordinates relative to origin of DIB */
    ptOrigin.x = (short int)GetScrollPos(hwnd,SB_HORZ);
    ptOrigin.y = (short int)GetScrollPos(hwnd,SB_VERT);

    pt.x += ptOrigin.x;
    pt.y += ptOrigin.y;

    /* Display the coordinates */
    (VOID)SetWindowOrgEx(hdc, ptOrigin.x, ptOrigin.y, NULL);
    DrawSelect(hdc,FALSE);

    /* Initialize clip rectangle to the point */
    rcClip.left   = pt.x;
    rcClip.top    = pt.y;
    rcClip.right  = pt.x;
    rcClip.bottom = pt.y;

    /* Eat mouse messages until a WM_LBUTTONUP is encountered. Meanwhile
     * continue to draw a rubberbanding rectangle and display it's dimensions
     */
    for (;;){
        WaitMessage();
        if (PeekMessage(&msg,NULL,WM_MOUSEFIRST,WM_MOUSELAST,PM_REMOVE)){
            DrawSelect(hdc,FALSE);

            rcClip.left   = pt.x;
            rcClip.top    = pt.y;
            rcClip.right  = LOWORD(msg.lParam) + ptOrigin.x;
            rcClip.bottom = HIWORD(msg.lParam) + ptOrigin.y;

            NormalizeRect(&rcClip);
            DrawSelect(hdc,TRUE);

            if (msg.message == WM_LBUTTONUP)
                break;
        }
        else
            continue;
    }

    ReleaseCapture();
    ReleaseDC(hwnd,hdc);
}

/****************************************************************************
 *                                                                          *
 *  FUNCTION   : BandDIB(HWND hWnd, HDC hDC, int x, int y)                  *
 *                                                                          *
 *  PURPOSE    : Outputs the DIB in bands to a device or the screen, using  *
 *               the maximum possible band size.                            *
 *                                                                          *
 ****************************************************************************/
/*
VOID BandDIB (
    HWND hWnd,
    HDC hDC,
    INT x,
    INT y)
{
    HBITMAP            hBitmap, hOld ;
    HDC                hMemDC ;
    LPSTR              pBuf;
    LPBITMAPINFOHEADER lpbi;
    WORD               wRead, wActualPosition, wScansLeft  ;
    DWORD              dwMapSize;
    DWORD              dwScans;
    WORD               wBitmapHeight;
    RECT               Rect;
    HANDLE             hBuf;
    BOOL               bSuccess = FALSE;
    INT                nBandSize;
    HPALETTE           hOldMemPal;
    HPALETTE           hOldPal;
    HFILE              fh;
    OFSTRUCT           of;
    CHAR               lpBuffer[128];

    /* Open the map file and get the information out 
    fh = OpenFile(achFileName, (LPOFSTRUCT)&of, (UINT)OF_READ);

    if (fh == -1)
        return;
    lpbi = (LPBITMAPINFOHEADER)(VOID FAR *)GlobalLock(hbiCurrent);
    if (!lpbi){
        _lclose(fh);
        return;
    }

    /* Compute scan size in bytes 
    dwScans = WIDTHBYTES((DWORD)lpbi->biWidth * lpbi->biBitCount);

    wBitmapHeight = (WORD)lpbi->biHeight ;
    wScansLeft    = (WORD)lpbi->biHeight ;

    hMemDC = NULL;
    for ( nBandSize = wScansLeft;
          (WORD)nBandSize >= MINBAND || (WORD)nBandSize == wScansLeft;
          nBandSize -= BANDINCREMENT) {

        /* Attempt to maximize band size by trying to allocate a buffer
         * for the given band size. If allocation fails, try again with the
         * smaller band size.
         
        hBuf = GlobalAlloc (GMEM_FIXED | GMEM_ZEROINIT, dwScans * nBandSize) ;
        if (!hBuf)
            continue;

        /* Show success and exit loop if we're going to set bits to device. 
        if (false) { // bDIBToDevice) 
            hMemDC = (HDC) 1;
            break;
        }
        else {
            /* Create a device-dependent bitmap to hold the bits 
            hBitmap = CreateCompatibleBitmap (hDC,
                                              (WORD)lpbi->biWidth,
                                              nBandSize);
            if (!hBitmap) {
                /* Try again for the next smaller band size 
                GlobalFree (hBuf);
                continue;
            }

            /* Create a memory context for the bitmap 
            if (!(hMemDC = CreateCompatibleDC (hDC))) {
                GlobalFree (hBuf);
                DeleteObject (hBitmap);
                continue;
            } else
                /* Success in creating a DC 
                break;
        }
    }
    if (!hMemDC) {

        /* We failed allocation , so give error message and quit 
        if (GetFocus () == hWnd) {
//            LoadString(hInst, IDS_NOMEM, lpBuffer, sizeof(lpBuffer));
//            ErrMsg (lpBuffer);
            ValidateRect (hWnd, (LPRECT) (NULL));
        } else
            MessageBeep(0);

        GlobalUnlock(hbiCurrent);
        _lclose(fh);
        return;
    }
    pBuf = (char *) GlobalLock (hBuf);

    /* Calculate number of bytes to be transferred 
    dwMapSize = dwScans * nBandSize ;

    /* Manipulate palette appropriately 
    if (true) // !bDIBToDevice)
        hOldMemPal = SelectPalette (hMemDC, hpalCurrent, 0) ;

    /* Now get to the start of the map in the file 
    _llseek(fh, dwOffset, (UINT)SEEK_SET);

    /* we are now all set to start off 
    wActualPosition = wScansLeft ;

    Rect.left  = 0;
    Rect.right = (WORD)lpbi->biWidth;

    hOldPal = SelectPalette(hDC, hpalCurrent, 0);
    RealizePalette(hDC);

    do {
        /* Read in nBandSize scans or whatever is left 
        if (wScansLeft > (WORD)nBandSize)
            wRead = (WORD)nBandSize ;
        else
            wRead = wScansLeft ;

        Rect.bottom = wActualPosition;
        wActualPosition -= wRead ;
        Rect.top = wActualPosition;

        dwMapSize = ((DWORD) wRead) * dwScans ;

        /* Now read in the map to the global buffer 
        if (RectVisible (hDC, &Rect)) {
            _lread(fh, (LPSTR)pBuf, dwMapSize);

            if (false) { // bDIBToDevice)
                if (wRead != (WORD)SetDIBitsToDevice (hDC, x, y,
                                                (WORD)lpbi->biWidth,
                                                wBitmapHeight,
                                                0,
                                                0,
                                                wBitmapHeight - wScansLeft,
                                                wRead,
                                                pBuf,
                                                (LPBITMAPINFO)lpbi,
                                                fPalColors ?
                                                DIB_PAL_COLORS :
                                                DIB_RGB_COLORS)){
//                    LoadString(hInst, IDS_CANTDRAWSCANS, lpBuffer, sizeof(lpBuffer));
//                    ErrMsg(lpBuffer);
                    GlobalUnlock(hBuf);
                    GlobalFree(hBuf);
                    GlobalUnlock(hbiCurrent);
                    _lclose(fh);
                    return;
                }
            } else {
                lpbi->biHeight = wRead ;

                /* Set the DIB bits to a device-dependent format 
                if (lpbi->biHeight != (int)SetDIBits (hMemDC,
                                                       hBitmap,
                                                       0,
                                                       (WORD)lpbi->biHeight,
                                                       pBuf,
                                                       (LPBITMAPINFO)lpbi,
                                                       (DWORD) (fPalColors ?
                                                       DIB_PAL_COLORS :
                                                       DIB_RGB_COLORS))){
                    LoadString(hInst, IDS_CANTDRAWSCANS, lpBuffer, sizeof(lpBuffer));
                    ErrMsg(lpBuffer);
                    GlobalUnlock (hBuf);
                    GlobalFree (hBuf);
                    GlobalUnlock(hbiCurrent);
                    _lclose(fh);
                    return;
                }

                /* Blt own map onto the screen, remembering the point to start 
                hOld = (HBITMAP) SelectObject (hMemDC, hBitmap) ;
                if (!BitBlt (hDC, 0, wActualPosition,
                             (WORD)lpbi->biWidth,
                             (WORD)lpbi->biHeight,
                             hMemDC, 0, 0, SRCCOPY)){
//                    LoadString(hInst, IDS_CANTDRAWMAP, lpBuffer, sizeof(lpBuffer));
//                    ErrMsg (lpBuffer);
                    GlobalUnlock (hBuf);
                    GlobalFree (hBuf);
                    GlobalUnlock(hbiCurrent);
                    _lclose(fh);
                    return;
                }
                SelectObject (hMemDC, hOld) ;

                /* Restore the value of bitmap height 
                lpbi->biHeight = wBitmapHeight ;
            }
        }
        else {
            /* This chunk is not visible, seek over the data in the file 
            _llseek(fh, dwMapSize, (UINT)SEEK_CUR);
        }
        wScansLeft -= wRead ;
    } while (wScansLeft > 0 ) ;

    /* Delete the objects just created above 
    GlobalUnlock (hBuf);
    GlobalFree (hBuf);
    SelectPalette (hDC, hOldPal, 0);

    /* Set success flag 
    bSuccess = TRUE;

    if (!bDIBToDevice) {
        SelectPalette (hMemDC, hOldMemPal, 0);
        DeleteDC (hMemDC) ;
        DeleteObject (hBitmap) ;
    }
    GlobalUnlock(hbiCurrent);

    /* Close the file 
    _lclose(fh);
}
*/

/****************************************************************************
 *                                                                          *
 *  FUNCTION   : SizeWindow(HWND hWnd)                                      *
 *                                                                          *
 *  PURPOSE    : Sizes the app. window based on client dimensions (DIB      *
 *               dimensions) and style. Sets the caption text.              *
 *                                                                          *
 ****************************************************************************/
/*
 VOID SizeWindow (HWND hWnd)
{
    CHAR  *pstr;
    CHAR  Name[60];
    RECT  Rectangle;
    RECT  rectClient;
    BITMAPINFOHEADER bi;

    //  Get information about current DIB 
    DibInfo(hbiCurrent,&bi);

    //  Extract the filename from the full pathname 
    pstr = achFileName + lstrlen(achFileName);
    while ( (pstr > achFileName) && (*pstr != '\\') && (*pstr != ':') )
	    pstr = CharPrev(achFileName, pstr);

    if(pstr != achFileName)
            pstr++;

    // Format filename along with the DIB attributes 
    sprintf (Name,
              "%s (%s %dx%dx%d%s)",
              szAppName,
              pstr,
              (WORD)bi.biWidth,
              (WORD)bi.biHeight,
              (WORD)bi.biBitCount,
              bi.biCompression == BI_RGB  ? " RGB" :
              bi.biCompression == BI_RLE8 ? " RLE8" : " RLE4" );

    /* Show formatted text in the caption bar 
    SetWindowText (hWnd, Name);

    /* Store the size in ptSize, so the scroll bars will work. 
    ptSize.x = (WORD)bi.biWidth;
    ptSize.y = (WORD)bi.biHeight;

    if (IsZoomed (hWnd))
        SetScrollRanges (hWnd);
    else {
        Rectangle.left   = 0;
        Rectangle.top    = 0;
        Rectangle.right  = (WORD)bi.biWidth;
        Rectangle.bottom = (WORD)bi.biHeight;

        /* Compute the size of the window rectangle based on the given
         * client rectangle size and the window style, then size the
         * window.
         
        AdjustWindowRect (&Rectangle, dwStyle, TRUE);
        SetWindowPos (hWnd, (HWND)NULL, 0, 0,
                      Rectangle.right  - Rectangle.left + 1,
                      Rectangle.bottom - Rectangle.top + 1,
                      SWP_NOMOVE | SWP_NOZORDER);
        GetClientRect( hWnd, &rectClient );
        // Correct for small bitmap that causes multiline menu
        if (rectClient.bottom < Rectangle.bottom) {
            Rectangle.bottom += (Rectangle.bottom - rectClient.bottom);
            SetWindowPos (hWnd, (HWND)NULL, 0, 0,
                      Rectangle.right  - Rectangle.left + 1,
                      Rectangle.bottom - Rectangle.top + 1,
                      SWP_NOMOVE | SWP_NOZORDER);
        }
    }

    InvalidateRect(hWnd,NULL,TRUE);
}
*/

/****************************************************************************
 *                                                                          *
 *  FUNCTION   : GetRealClientRect(HWND hwnd, LPRECT lprc)                  *
 *                                                                          *
 *  PURPOSE    : Calculates the client rectangle taking scrollbars into     *
 *               consideration.                                             *
 *                                                                          *
 ****************************************************************************/
VOID GetRealClientRect (
    HWND hwnd,
    PRECT lprc)
{
    DWORD dwStyle;

    dwStyle = GetWindowLong (hwnd, GWL_STYLE);
    GetClientRect (hwnd,lprc);

    if (dwStyle & WS_HSCROLL)
        lprc->bottom += GetSystemMetrics (SM_CYHSCROLL);

    if (dwStyle & WS_VSCROLL)
        lprc->right  += GetSystemMetrics (SM_CXVSCROLL);
}

/****************************************************************************
 *                                                                          *
 *  FUNCTION   : SetScrollRanges(hwnd)                                      *
 *                                                                          *
 *  PURPOSE    :                                                            *
 *                                                                          *
 ****************************************************************************/
VOID SetScrollRanges(HWND hwnd)
{
    RECT       rc;
    INT        iRangeH, iRangeV, i;
    static INT iSem = 0;

    if (!iSem){
        iSem++;
        GetRealClientRect (hwnd, &rc);

        for (i = 0; i < 2; i++){
            iRangeV = ptSize.y - rc.bottom;
            iRangeH = ptSize.x - rc.right;

            if (iRangeH < 0) iRangeH = 0;
            if (iRangeV < 0) iRangeV = 0;

            if (GetScrollPos ( hwnd,
                               SB_VERT) > iRangeV ||
                               GetScrollPos (hwnd, SB_HORZ) > iRangeH)
                InvalidateRect (hwnd, NULL, TRUE);

            SetScrollRange (hwnd, SB_VERT, 0, iRangeV, TRUE);
            SetScrollRange (hwnd, SB_HORZ, 0, iRangeH, TRUE);

            GetClientRect (hwnd, &rc);
        }
        iSem--;
    }
}

/*********** THE FOLLOWING FUNCTIONS ARE FOR CLIPBOARD SUPPORT **************/
/****************************************************************************
 *                                                                          *
 *  FUNCTION   : CopyHandle (HANDLE h)                                      *
 *                                                                          *
 *  PURPOSE    : Makes a copy of the given global memory block.             *
 *                                                                          *
 *  RETURNS    : A handle to the new block.                                 *
 *                                                                          *
 ****************************************************************************/
HANDLE CopyHandle (HANDLE h)
{
    BYTE *lpCopy;
    BYTE *lp;
    HANDLE hCopy;
    DWORD  dwLen;

    dwLen = GlobalSize (h);
    hCopy = GlobalAlloc (GHND, dwLen);

    if (hCopy) {

        lpCopy = (BYTE *)GlobalLock (hCopy);
        lp     = (BYTE *)GlobalLock (h);
        while (dwLen--) *lpCopy++ = *lp++;
        GlobalUnlock (hCopy);
        GlobalUnlock (h);
    }
    return hCopy;
}

/****************************************************************************
 *                                                                          *
 *  FUNCTION   : CopyPalette(HPALETTE hpal)                                 *
 *                                                                          *
 *  PURPOSE    : Makes a copy of a GDI logical palette                      *
 *                                                                          *
 *  RETURNS    : A handle to the new palette.                               *
 *                                                                          *
 ****************************************************************************/
HPALETTE CopyPalette (HPALETTE hpal)
{
    PLOGPALETTE ppal;
    WORD        nNumEntries;

    if (!hpal)
        return NULL;

    GetObject(hpal,sizeof(INT),(LPSTR)&nNumEntries);

    if (nNumEntries == 0)
        return NULL;

    ppal = (PLOGPALETTE)LocalAlloc(LPTR,sizeof(LOGPALETTE) +
                nNumEntries * sizeof(PALETTEENTRY));

    if (!ppal)
        return NULL;

    ppal->palVersion    = PALVERSION;
    ppal->palNumEntries = nNumEntries;

    GetPaletteEntries(hpal,0,nNumEntries,(LPPALETTEENTRY)ppal->palPalEntry);

    hpal = CreatePalette(ppal);

    LocalFree((HANDLE)ppal);
    return hpal;
}
/****************************************************************************
 *                                                                          *
 *  FUNCTION   : CopyBitmap (HBITMAP hbm)                                   *
 *                                                                          *
 *  PURPOSE    : Copies the given bitmap to another.                        *
 *                                                                          *
 *  RETURNS    : A handle to the new bitmap.                                *
 *                                                                          *
 ****************************************************************************/
HBITMAP CopyBitmap (HBITMAP hbm)
{
    BITMAP  bm;
    RECT    rc;

    if (!hbm)
         return NULL;

    GetObject (hbm, sizeof(BITMAP), (LPSTR)&bm);
    rc.left   = 0;
    rc.top    = 0;
    rc.right  = bm.bmWidth;
    rc.bottom = bm.bmHeight;

    return CropBitmap (hbm, &rc);
}
/****************************************************************************
 *                                                                          *
 *  FUNCTION   :  CropBitmap (hbm,lprect)                                   *
 *                                                                          *
 *  PURPOSE    :  Crops a bitmap to a new size specified by the lprect      *
 *                parameter.                                                *
 *                                                                          *
 *  RETURNS    :  A handle to the new bitmap.                               *
 *                                                                          *
 ****************************************************************************/
HBITMAP CropBitmap (
    HBITMAP hbm,
    PRECT prc)
{
    HDC     hMemDCsrc;
    HDC     hMemDCdst;
    HDC     hdc;
    HBITMAP hNewBm;
    BITMAP  bm;
    INT     dx,dy;

    if (!hbm)
         return NULL;

    hdc = GetDC (NULL);
    hMemDCsrc = CreateCompatibleDC (hdc);
    hMemDCdst = CreateCompatibleDC (hdc);

    GetObject (hbm, sizeof(BITMAP), (LPSTR)&bm);
    dx = prc->right  - prc->left;
    dy = prc->bottom - prc->top;

    /*hNewBm = +++CreateBitmap - Not Recommended(use CreateDIBitmap)+++ (dx, dy, bm.bmPlanes, bm.bmBitsPixel, NULL);*/
    hNewBm = CreateBitmap(dx, dy, bm.bmPlanes, bm.bmBitsPixel, NULL);
    if (hNewBm){
        SelectObject (hMemDCsrc, hbm);
        SelectObject (hMemDCdst, hNewBm);

        BitBlt (hMemDCdst,
                0,
                0,
                dx,
                dy,
                hMemDCsrc,
                prc->left,
                prc->top,
                SRCCOPY);
    }

    ReleaseDC (NULL,hdc);
    DeleteDC (hMemDCsrc);
    DeleteDC (hMemDCdst);
    return hNewBm;
}

/****************************************************************************
 *                                                                          *
 *  FUNCTION   : RenderFormat(int cf)                                       *
 *                                                                          *
 *  PURPOSE    : Renders the currently displayed DIB in CF_DIB or           *
 *               CF_BITMAP format.The bitmap is clipped to the current      *
 *               rcClip.                                                    *
 *                                                                          *
 *  RETURNS    : A handle to the DIB                                        *
 *                                                                          *
 ****************************************************************************/
HANDLE RenderFormat (INT cf)
{
    HANDLE  h = NULL;
    HBITMAP hbm;

//    if (!bLegitDraw)
//        return NULL;

    switch (cf){
        case CF_BITMAP:
            if (hbmCurrent && !IsRectEmpty (&rcClip))
                h = CropBitmap (hbmCurrent, &rcClip);
            else{
                if (hbmCurrent)
                    h = CopyBitmap (hbmCurrent);
                else if (hdibCurrent)
                    h = BitmapFromDib (hdibCurrent, hpalCurrent);
//                else if (achFileName[0] && (hdibCurrent = OpenDIB (achFileName)))
//                    h = BitmapFromDib (hdibCurrent, hpalCurrent);
                else
                    h = NULL;

                if (h && !IsRectEmpty (&rcClip)){
                    hbm = CropBitmap ((HBITMAP)h,&rcClip);
                    DeleteObject (h);
                    h = hbm;
                }
            }
            break;

        case CF_DIB:
            if (!IsRectEmpty (&rcClip)){
                hbm = (HBITMAP) RenderFormat (CF_BITMAP);

                if (hbm){
                    h = DibFromBitmap (hbm, BI_RGB, 0, hpalCurrent);
                    DeleteObject (hbm);
                }
            }
            else{
                if (!hdibCurrent && hbmCurrent)
                    h = DibFromBitmap (hbmCurrent, BI_RGB, 0, hpalCurrent);
                else if (hdibCurrent)
                    h = CopyHandle (hdibCurrent);
//                else if (achFileName[0])
//                    h = OpenDIB (achFileName);
                else
                    h = NULL;
            }
            break;

        case CF_PALETTE:
            if (hpalCurrent)
                h = CopyPalette (hpalCurrent);
            break;
    }
    return h;
}
/****************************************************************************
 *                                                                          *
 *  FUNCTION   :  RealizeDibFormat(DWORD biStyle, WORD biBits)              *
 *                                                                          *
 *  PURPOSE    :  Realize the current DIB in the specifed format            *
 *                This function is used to get a specific format of CF_DIB  *
 *                                                                          *
 *                    biStyle     DIB format      RGB or RLE                *
 *                    biBits      Bits per pixel  1,4,8,24                  *
 *                                                                          *
 *  RETURNS    :  A handle to the created DIB.                              *
 *                                                                          *
 ****************************************************************************/
HANDLE RealizeDibFormat (
    DWORD biStyle,
    WORD biBits)
{
    BITMAPINFOHEADER bi;

//    if (!bLegitDraw)
//        return NULL;

    BOOL ok = DibInfo (hbiCurrent, &bi);
    BASSERT(ok);

    /*  Do we have the requested format already? */
    if (bi.biCompression == biStyle && bi.biBitCount == biBits){
        if (!hdibCurrent)
            hdibCurrent = RenderFormat (CF_DIB);
    }
    else{
        if (!hbmCurrent)
            hbmCurrent = (HBITMAP) RenderFormat (CF_BITMAP);

        if (hbmCurrent){
            if (hdibCurrent)
                GlobalFree (hdibCurrent);

            hdibCurrent = DibFromBitmap (hbmCurrent, biStyle, biBits, hpalCurrent);
        }
    }

    return hdibCurrent;
}

/****************************************************************************
 *                                                                          *
 *  FUNCTION   : InitDIB(hWnd)                                              *
 *                                                                          *
 *  PURPOSE    : Reads a DIB from a file, obtains a handle to it's          *
 *               BITMAPINFO struct., sets up the palette and loads the DIB. *
 *                                                                          *
 *  RETURNS    : TRUE  - DIB loads ok                                       *
 *               FALSE - otherwise                                          *
 *                                                                          *
 ****************************************************************************/
INT InitDIB(HWND hWnd, const BCHAR_T *pFilename)
{
//    HFILE              fh;
    LPBITMAPINFOHEADER lpbi;
    WORD FAR *         pw;
    INT                i;
    BITMAPINFOHEADER   bi;
//    OFSTRUCT           of;
//    CHAR               lpBuffer[128];

    FreeDib();

    /* Open the file and get a handle to it's BITMAPINFO */

//    fh = OpenFile(achFileName, (LPOFSTRUCT)&of, (UINT)OF_READ);

    HANDLE hFile = CreateFile(pFilename, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);

    if (hFile == INVALID_HANDLE_VALUE)
    {
       BASSERT(0);
       return FALSE;
    }

//    fh = OpenFile(pFilename, (LPOFSTRUCT)&of, (UINT)OF_READ);
//    if (fh == -1) {
//        LoadString(hInst, IDS_CANTOPENFILE, lpBuffer, sizeof(lpBuffer));
//        ErrMsg(lpBuffer, (LPSTR)achFileName);
//        return FALSE;
//    }

    hbiCurrent = ReadDibBitmapInfo(hFile);

    dwOffset = SetFilePointer(hFile, 0, 0, FILE_CURRENT);

//    dwOffset = _llseek(fh, 0L, (UINT)SEEK_CUR);
//    _lclose(fh);

    CloseHandle(hFile);

    if (hbiCurrent == NULL) {
//        LoadString(hInst, IDS_BADDIBFILE, lpBuffer, sizeof(lpBuffer));
//        ErrMsg(lpBuffer, (LPSTR)achFileName);
        return FALSE;
    }
    DibInfo(hbiCurrent,&bi);

    /* Set up the palette */
    hpalCurrent = CreateDibPalette(hbiCurrent);
    if (hpalCurrent == NULL) {
//        LoadString(hInst, IDS_CREATEPALFAIL, lpBuffer, sizeof(lpBuffer));
//        ErrMsg(lpBuffer);
        return FALSE;
    }

    /*  Convert the DIB color table to palette relative indexes, so
     *  SetDIBits() and SetDIBitsToDevice() can avoid color matching.
     *  We can do this because the palette we realize is identical
     *  to the color table of the bitmap, ie the indexes match 1 to 1
     *
     *  Now that the DIB color table is palette indexes not RGB values
     *  we must use DIB_PAL_COLORS as the wUsage parameter to SetDIBits()
     */
    lpbi = (LPBITMAPINFOHEADER) (VOID FAR *)GlobalLock(hbiCurrent);
    if (lpbi->biBitCount != 24) {
        fPalColors = TRUE;

        pw = (WORD FAR *)((LPSTR)lpbi + lpbi->biSize);

        for (i=0; i<(INT)lpbi->biClrUsed; i++)
            *pw++ = (WORD)i;
    }
    GlobalUnlock(hbiCurrent);
//    bLegitDraw = TRUE;

    /*  If the input bitmap is not in RGB FORMAT the banding code will
     *  not work!  we need to load the DIB bits into memory.
     *  if memory DIB, load it all NOW!  This will avoid calling the
     *  banding code.
     */
    hdibCurrent = OpenDIB(BUString(pFilename)); // "c:\\rts3w\\textures\\splash.bmp");

    /*  If the RLE could not be loaded all at once, exit gracefully NOW,
     *  to avoid calling the banding code
     */
    if ((bi.biCompression != BI_RGB) && !hdibCurrent){
//        LoadString(hInst, IDS_CANTLOADRLE, lpBuffer, sizeof(lpBuffer));
//        ErrMsg (lpBuffer);
        FreeDib();
        return FALSE;
    }

    if (hdibCurrent) { //  && !bDIBToDevice)
        hbmCurrent = BitmapFromDib(hdibCurrent,hpalCurrent);
        if (!hbmCurrent){
//            LoadString(hInst, IDS_CANTCREATEBMP, lpBuffer, sizeof(lpBuffer));
//            ErrMsg (lpBuffer);
            FreeDib();
            return FALSE;
        }
    }

//    SizeWindow(hWnd);

    InvalidateRect(hWnd,NULL,TRUE);

    return TRUE;
}
/****************************************************************************
 *                                                                          *
 *  FUNCTION   : FreeDib(void)                                              *
 *                                                                          *
 *  PURPOSE    : Frees all currently active bitmap, DIB and palette objects *
 *               and initializes their handles.                             *
 *                                                                          *
 ****************************************************************************/
VOID FreeDib()
{
    if (hpalCurrent)
        DeleteObject(hpalCurrent);

    if (hbmCurrent)
        DeleteObject(hbmCurrent);

    if (hdibCurrent)
        GlobalFree(hdibCurrent);

    if (hbiCurrent && hbiCurrent != hdibCurrent)
        GlobalFree(hbiCurrent);

    fPalColors  = FALSE;
//    bLegitDraw  = FALSE;
    hpalCurrent = NULL;
    hdibCurrent = NULL;
    hbmCurrent  = NULL;
    hbiCurrent  = NULL;
    SetRectEmpty (&rcClip);
}

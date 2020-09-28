//==============================================================================
// dib.cpp
//
// Copyright (c) 2000 Ensemble Studios
//==============================================================================

#include "xsystem.h"
#include "dib.h"
#include "image.h"

//==============================================================================
// BDib::BDib
//==============================================================================
BDib::BDib( void ) :mData(NULL),
                    mBmpInfo(NULL),
                    mhBitmap(NULL)
{                   
}

//==============================================================================
// BDib::~BDib
//==============================================================================
BDib::~BDib( void )
{
   if (mhBitmap != NULL)
   {
      DeleteObject(mhBitmap);
      mhBitmap = NULL;
   }

   if (mBmpInfo != NULL)
   {
      delete mBmpInfo;
      mBmpInfo = NULL;
   }

   mData = NULL;
}

//==============================================================================
// BDib::BDib
//==============================================================================
BDib::BDib( BImage* image ) :mData(NULL),  
                             mBmpInfo(NULL),
                             mhBitmap(NULL)
{
   createFromImage(image);
}

//==============================================================================
// BDib::getWidth
//==============================================================================
long BDib::getWidth( void ) const
{ 
   if (mBmpInfo == NULL)
   {
      BASSERT(0);
      return(-1);
   }

   return mBmpInfo->bmiHeader.biWidth; 
}

//==============================================================================
// BDib::getHeight
//==============================================================================
long BDib::getHeight( void ) const 
{ 
   if (mBmpInfo == NULL)
   {
      BASSERT(0);
      return(-1);
   }

   return mBmpInfo->bmiHeader.biHeight; 
}

//==============================================================================
// BDib::getSize
//==============================================================================
long BDib::getSize( void) const 
{ 
   if (mBmpInfo == NULL)
   {
      BASSERT(0);
      return(-1);
   }

   return (mBmpInfo->bmiHeader.biWidth*mBmpInfo->bmiHeader.biHeight*(mBmpInfo->bmiHeader.biBitCount/8)); 
}

//==============================================================================
// BDib::allocateData
//==============================================================================
bool BDib::allocateData( long w, long h, long format /*= BDib::cFormatRGB*/, bool bfill /*= true*/)
{
   if (mhBitmap != NULL)
   {
      DeleteObject(mhBitmap);
      mhBitmap = NULL;
   }
   mData = NULL;
   
   HDC hdc=CreateCompatibleDC(NULL);
   if (hdc == NULL)
   {
      return(false);
   }

   if (format==BDib::cFormatRGB)
   {
      mBmpInfo = new BITMAPINFO;
      memset(mBmpInfo, 0, sizeof(BITMAPINFO));

      mBmpInfo->bmiHeader.biBitCount   = 24; 
      mBmpInfo->bmiHeader.biCompression= BI_RGB;
      mBmpInfo->bmiHeader.biSizeImage  = (w*h*3);   
   }
   else
   {
      // allocate enough room for the bitmap info and the 3 color masks
      //mBmpInfo = (BITMAPINFO*) new BYTE[sizeof(BITMAPINFO)+(2*sizeof(DWORD))];
      //memset(mBmpInfo, 0, sizeof(BITMAPINFO)+(2*sizeof(DWORD)));

      mBmpInfo = new BITMAPINFO;
      memset(mBmpInfo, 0, sizeof(BITMAPINFO));

      mBmpInfo->bmiHeader.biBitCount   = 16; 
      mBmpInfo->bmiHeader.biCompression= BI_RGB;
      mBmpInfo->bmiHeader.biSizeImage  = (w*h*2);
      
      /*
      if (format==BDib::cFormat555)
      {
         ((DWORD*)mBmpInfo->bmiColors)[0] = 0x0000001F;
         ((DWORD*)mBmpInfo->bmiColors)[1] = 0x000003E0;
         ((DWORD*)mBmpInfo->bmiColors)[2] = 0x00007C00;
      }
      else
      {
         ((DWORD*)mBmpInfo->bmiColors)[0] = 0x0000001F;
         ((DWORD*)mBmpInfo->bmiColors)[1] = 0x000007E0;
         ((DWORD*)mBmpInfo->bmiColors)[2] = 0x0000F800;
      }*/
   }

   mBmpInfo->bmiHeader.biSize          = sizeof(BITMAPINFOHEADER); 
   mBmpInfo->bmiHeader.biWidth         = w; 
   mBmpInfo->bmiHeader.biHeight        = h; 
   mBmpInfo->bmiHeader.biPlanes        = 1; 

   //Create the BMP.
   mhBitmap=CreateDIBSection(hdc, mBmpInfo, DIB_RGB_COLORS, (void**)&mData, NULL, 0);
   if (mhBitmap == NULL)
   {
      return(false);
   }

   DeleteDC(hdc);

   if (bfill)
      memset(mData, 0, getSize());

   return(true);
}

//==============================================================================
// BDib::createFromImage
//==============================================================================
bool BDib::createFromImage(BImage* image) 
{
   BYTE* pdest;

   // Bomb check
   if (image == NULL)
   {
      return(false);
   }

   if (mBmpInfo == NULL)
   {
      if (!allocateData(image->getWidth(), image->getHeight()))
      {
         return(false);
      }
   }   

   pdest = mData;
   DWORD *psrc = image->getRawData();
   for (uint i=0; i<(image->getHeight()*image->getWidth()); i++)
   {
      *pdest++ = (BYTE)IMG_GETB(psrc[i]);
      *pdest++ = (BYTE)IMG_GETG(psrc[i]);
      *pdest++ = (BYTE)IMG_GETR(psrc[i]);
   }

   return(true);
}

//==============================================================================
// eof: dib.cpp
//==============================================================================

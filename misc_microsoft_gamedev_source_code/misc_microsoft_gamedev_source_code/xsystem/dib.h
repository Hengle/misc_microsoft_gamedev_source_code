//==============================================================================
// dib.h
//
// Copyright (c) 2000 Ensemble Studios
//==============================================================================

#ifndef _DIB_H_
#define _DIB_H_


class BImage;

//==============================================================================
// BDib Class
//==============================================================================
class BDib
{
   public:

      enum 
      {
         cFormat555,
         cFormat565,
         cFormatRGB
      };

      BDib( void );
      ~BDib( void );
      BDib( BImage* image );

      bool        allocateData( long w, long h, long format = BDib::cFormatRGB, bool bfill = true);

      BYTE*       getData( void ) const { return mData; }
      long        getWidth( void ) const;
      long        getHeight( void ) const;
      long        getSize( void) const;
      BITMAPINFO  *getBmpInfo( void ) { return mBmpInfo; }
      HBITMAP     getBmpHandle( void ) { return mhBitmap; }
      
      bool        createFromImage( BImage *image );

   protected:
      BYTE        *mData;
      BITMAPINFO  *mBmpInfo;
      HBITMAP     mhBitmap;

}; // BDib


//==============================================================================

#endif // _DIB_H_

//==============================================================================
// eof: dib.h

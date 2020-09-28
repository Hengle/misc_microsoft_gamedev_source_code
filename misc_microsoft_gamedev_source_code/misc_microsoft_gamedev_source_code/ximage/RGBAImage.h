// File: RGBAImage.h
#pragma once

#include "colorUtils.h"

template<typename ColorType>
class BRGBAImageTemplate
{
public:
   typedef ColorType colorType;
      
   BRGBAImageTemplate(const BRGBAImageTemplate& b);
   
   BRGBAImageTemplate(uint width = 0, uint height = 0, uint pitch = 0);
   
   // Object does NOT take ownership!
   BRGBAImageTemplate(colorType* pImage, uint width, uint height, uint pitch = 0);

   // width, height, and pitch must match for copy to succeed!
   BRGBAImageTemplate& operator= (const BRGBAImageTemplate& b);
   
   const colorType& operator()(uint x, uint y) const
   {
      BDEBUG_ASSERT((x < mWidth) && (y < mHeight) && (mpImage));
      return mpImage[x + y * mPitch];
   }

   colorType& operator()(uint x, uint y) 
   {
      BDEBUG_ASSERT((x < mWidth) && (y < mHeight) && (mpImage));
      return mpImage[x + y * mPitch];
   }
   
   const colorType* getPtr(void) const { return mpImage; }
         colorType* getPtr(void)       { return mpImage; }
   
   uint getWidth(void) const { return mWidth; }
   uint getHeight(void) const { return mHeight; }
   
   // Pitch is in pixels, not bytes!
   uint getPitch(void) const { return mPitch; }
   
   uint getSizeInBytes(void) const { return mHeight * mPitch * sizeof(BRGBAColor); }
   
   // Contents are lost if image's dimensions or pitch change.
   // Allocates a byte array to hold the image.
   bool setSize(uint width, uint height, uint pitch = 0, bool preserveContents = false);
   
   bool aliasToImage(colorType* pImage, uint width, uint height, uint pitch = 0);
   
   void clear(const colorType& color);
   void clear(void) { colorType blackColor(0); clear(blackColor); }
   
   // Sets/gets pixel with clipping.
   void setPixel(int x, int y, const colorType& color);
   const colorType& getPixel(int x, int y) const;
         colorType& getPixel(int x, int y);
   
   const colorType* getScanlinePtr(uint y) const;
   colorType* getScanlinePtr(uint y);
   
   void setScanline(uint x, uint y, const colorType* pColors, uint numColors, bool flip = false);
   
   void fillRect(int x, int y, uint width, uint height, const colorType& color);
   
   void drawLine(int xs, int ys, int xe, int ye, const colorType& color);
   
   void swap(BRGBAImageTemplate& image);
            
private:
   colorType* mpImage;
   BDynamicArray<colorType> mImageBuf;
   
   // Pitch is in pixels, not bytes!
   uint mWidth, mHeight, mPitch;
   
   void drawLineInternal(int xs, int ys, int xe, int ye, int pred, int incDec, int e, int eInc, int eNoInc, const colorType& color);
};

#include "RGBAImage.inl"

typedef BRGBAImageTemplate<BRGBAColor>    BRGBAImage;
typedef BRGBAImageTemplate<BRGBAColor16>  BRGBA16Image;
typedef BRGBAImageTemplate<BRGBAColor16U> BRGBA16UImage;
typedef BRGBAImageTemplate<BRGBAColorF>   BRGBAFImage;


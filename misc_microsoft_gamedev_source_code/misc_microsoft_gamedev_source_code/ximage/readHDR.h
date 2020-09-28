// File: readHDR.h
#pragma once
#include "colorUtils.h"

class BHDRReader
{
public:
   BHDRReader();
   ~BHDRReader();

   bool init(BStream& stream);

   void deinit(void);

   bool getError(void) const
   {
      return !mValid || mError;
   }
   
   bool getXFlipped(void) const
   {
      BASSERT(mValid);
      return mXFlipped;
   }

   bool getYFlipped(void) const
   {
      BASSERT(mValid);
      return mYFlipped;
   }

   uint getWidth(void) const
   {
      BASSERT(mValid);
      return mWidth;
   }

   uint getHeight(void) const
   {
      BASSERT(mValid);
      return mHeight;
   }
   
   bool decode(const BRGBAColor*& pScanOfs, uint& scanLen);

   static const char* getExtension(void) { return "hdr"; }
   
private:
   BStream* mpStream;
   uint mWidth;
   uint mHeight;
   uint mCurScanline;
   BDynamicArray<BRGBAColor> mBuf;
   
   bool mValid : 1;
   bool mError : 1;
   bool mXFlipped : 1;
   bool mYFlipped : 1;

   bool readHeader(void);
   typedef uchar COLR[4];
   bool freadcolrs(COLR* pScanLine, int len);		
   bool oldreadcolrs(COLR* pScanLine, int len, int firstChar = -1);
};




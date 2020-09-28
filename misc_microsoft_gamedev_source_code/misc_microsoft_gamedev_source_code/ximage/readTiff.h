//============================================================================
//
// File: readTiff.h
// Copyright (c) 2005-2007, Ensemble Studios
//
// rg [6/11/06] - This is a bare bones TIFF reader, intended to only read 
// uncompressed HDR TIFF images written by Photoshop CS2. Unfortunately TIFF 
// is the only HDR capable format you can save from a CS2 Javascript. Damn it.
//============================================================================
#pragma once

#include "colorUtils.h"

class BTIFFReader
{
public:
   BTIFFReader();
   ~BTIFFReader();

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

   bool decode(const BRGBAColorF*& pScanOfs);

   static const char* getExtension(void) { return "tif"; }

private:
   BStream* mpStream;
   uint mWidth;
   uint mHeight;
   
   enum eByteOrder
   {
      cINTEL_BYTE_ORDER,
      cMOTOROLA_BYTE_ORDER 
   };
   
   eByteOrder mByteOrder;
         
   struct BTAG
   {
      WORD mType;
      WORD mDataType;
      DWORD mLength;
      DWORD mValue;  
   };
   
   BDynamicArray<BTAG> mTags;
   BDynamicArray<uchar> mTagData;
   BDynamicArray<uint> mStripOffsets;
   
   uint mLinesPerStrip;
   uint mPhotoInterp;
   uint mSamplesPerPixel;
   uint mBitsPerSample;
   uint mCompression;
   uint mOrient;
   uint mPlanarConfig;
   uint mPredictor;
   uint mTotalStrips;
   
   uint mCurScanline;
   uint mLinesLeftInStrip;   
   uint mCurStrip;
   
   BDynamicArray<BRGBAColorF> mPixelBuf;
   BDynamicArray<uchar> mScanlineBuf;
            
   bool mValid : 1;
   bool mError : 1;
   bool mXFlipped : 1;
   bool mYFlipped : 1;
   
   enum eTiffStatus
   {
      cNO_ERROR                             = 0,
      cERR_READ_ERROR                          ,
      cERR_NOT_TIFF                            ,
      cERR_BAD_IFH_VER                         ,
      cERR_BAD_IFD_OFS                         ,
      cERR_BAD_NUM_TAGS                        ,
      cERR_TAG_TOO_BIG                         ,
      cERR_MISSING_TAG                         ,
      cERR_UNSUPPORTED_RES                     ,
      cERR_RGB_BAD_SAMPLES_PER_PIXEL           ,
      cERR_RGB_UNSUPPORTED_SAMPLES_PER_PIXEL   ,
      cERR_RGB_UNSUPPORTED_BITS_PER_SAMPLE     ,
      cERR_UNSUPPORTED_COLORSPACE              ,
      cERR_TOO_MANY_STRIPS                     ,
      cERR_BAD_STRIP_OFFSETS_LENGTH            ,
      cERR_ASSERTION                           ,
      cERR_UNSUPPORTED_COMPRESSION             ,
      cERR_UNSUPPORTED_ORIENT                  ,
      cERR_UNSUPPORTED_FILLORDER               ,
      cERR_UNSUPPORTED_PLANAR                  ,
      cERR_UNSUPPORTED_PREDICTOR              
   };

   void clear(void);   
   uint getUShort(const void* q) const;
   uint getUInt(const void* q) const;
   int getSizeOfTIFFDataType(int datatype) const;
   bool importantTagType(int type) const;
   eTiffStatus readIFHAndIFD(void);
   const BTAG* locateTag(int type) const;
   uint valueOfIntTagN(const BTAG* pTag, int n) const;
   uint valueOfIntTag(const BTAG* pTag) const;
   uint valueOfIntTagDef(int type, uint def) const;
   eTiffStatus scanIFD(void);
};


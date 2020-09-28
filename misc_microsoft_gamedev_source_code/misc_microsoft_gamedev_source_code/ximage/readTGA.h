//--------------------------------------------------------------------------------------------------------------------------------------------
// File: readTGA.h
//--------------------------------------------------------------------------------------------------------------------------------------------
#pragma once
//--------------------------------------------------------------------------------------------------------------------------------------------
#include "ximage.h"
#include "stream\stream.h"
#include "pixelFormat.h"
//--------------------------------------------------------------------------------------------------------------------------------------------
class BTGAReader 
{
private:
  
   int mWidth;
   int mHeight;
   int mBPL;

   BPixelFormat mPixelFormat;
   BStream* mpStream;
   int mLinesLeft;
      
   uchar* mpLineBuf; 
   uchar* mpBuf;
   uint mBufSize;

   bool mValid : 1;
   bool mError : 1;
   bool mXFlipped : 1;
   bool mYFlipped : 1;
   bool mRLE : 1;

   BFixedString256 mErrorMsg;
        
#pragma pack(push)
#pragma pack(1)
   // From http://www.wotsit.org/download.asp?f=tga
   struct BTGAHeader
   {
      uchar    mIDLen;       
      uchar    mCmap;        
      uchar    mType;        
      ushort   mCMapFirst;   
      ushort   mCmapLen;     
      uchar    mCMapSize;    
      ushort   mXOrg;        
      ushort   mYOrg;        
      ushort   mWidth;        
      ushort   mHeight;       
      uchar    mDepth;        
      uchar    mDesc;        
      
      void littleEndianToNative(void)
      {
         LittleEndianToNative(this, "cccsscsssscc");
      } 
   };
#pragma pack(pop)
   BTGAHeader mTGAHeader;

   void clear(void);
   void setError(const BFixedString256& errorMessage, bool printMessage = true);
   void clearError(void);
   bool checkBytes(int num);
   bool skipBytes(uint len);
   void initBuffer(uint size);
   const uchar* getBuffered(uint num);
   bool get(void* Pbuf, uint num);
   bool decodeRLE1(uchar* pDst);
   bool decodeRLE2(ushort* pDst);
   bool decodeRLE3(uchar* pDst);
   bool decodeRLE4(DWORD* pDst);
   bool valid(void) const { return mValid; }
   
public:
   BTGAReader();
   ~BTGAReader();
         
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

   const BPixelFormat& getFormat(void) const
   {
      BASSERT(mValid);
      return mPixelFormat;
   }

   // Always returns little endian data.
   bool decode(const void*& pScanOfs, uint& pScanLen);

   static const char* getExtension(void) { return "tga"; }
};

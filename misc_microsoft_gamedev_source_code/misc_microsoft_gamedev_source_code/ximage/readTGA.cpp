//--------------------------------------------------------------------------------------------------------------------------------------------
// File: readTGA.cpp
//--------------------------------------------------------------------------------------------------------------------------------------------
#include "ximage.h"
#include "readTGA.h"
#include "consoleOutput.h"
//--------------------------------------------------------------------------------------------------------------------------------------------
BTGAReader::BTGAReader()
{
   clear();
}
//--------------------------------------------------------------------------------------------------------------------------------------------
BTGAReader::~BTGAReader()
{
   deinit();
}
//--------------------------------------------------------------------------------------------------------------------------------------------
void BTGAReader::clear(void)
{
   mValid = false;
   mErrorMsg.clear();
   mError = false;
   mXFlipped = false;
   mYFlipped = false;
   mRLE = false;

   mWidth = 0;
   mHeight = 0;
   mBPL = 0;
   mpStream = NULL;
   mLinesLeft = 0;
   Utils::ClearObj(mTGAHeader);
   mpLineBuf = NULL;
   mpBuf = 0;
   mBufSize = 0;
}
//--------------------------------------------------------------------------------------------------------------------------------------------
void BTGAReader::setError(const BFixedString256& errorMessage, bool printMessage)
{
   mErrorMsg = errorMessage;
   mError = true;
   if (printMessage)
   {
      gConsoleOutput.output(cMsgError, "BTGAReader::setError: %s\n", mErrorMsg.c_str());
   }
}
//--------------------------------------------------------------------------------------------------------------------------------------------
void BTGAReader::clearError(void)
{
   mErrorMsg = "";
   mError = false;
}
//--------------------------------------------------------------------------------------------------------------------------------------------
bool BTGAReader::checkBytes(int num)
{
   if (mpStream->bytesLeft() < num)
   {
      setError("Premature end of file");
      return false;
   }

   return true;
}
//--------------------------------------------------------------------------------------------------------------------------------------------
bool BTGAReader::skipBytes(uint len)
{
   if (!checkBytes(len))
      return false;

   if (mpStream->skipBytes(len) < len)
      return false;

   return true;
}
//--------------------------------------------------------------------------------------------------------------------------------------------
void BTGAReader::initBuffer(uint size)
{
   BASSERT((mBufSize == 0) && (!mpBuf));
   mpBuf = new uchar[size];
   mBufSize = size;
}
//--------------------------------------------------------------------------------------------------------------------------------------------
const uchar* BTGAReader::getBuffered(uint num)
{
   BASSERT(num <= mBufSize);

   if (mpStream->readBytes(mpBuf, num) < num)
   {
      setError("Read failed");
      return NULL;
   }

   return mpBuf;
}
//--------------------------------------------------------------------------------------------------------------------------------------------
bool BTGAReader::get(void* Pbuf, uint num)
{
   if (mpStream->readBytes(Pbuf, num) < num)
      return false;

   return true;
}
//--------------------------------------------------------------------------------------------------------------------------------------------
bool BTGAReader::init(BStream& stream) 
{
   mpStream = &stream;
   initBuffer(16);

   setError("Invalid or unsupported image type", false);

   if (mpStream->bytesLeft() < sizeof(BTGAHeader))
   {
      setError("Not a TGA file");

      return false;
   }

   BTGAHeader& p = mTGAHeader;

   const uint cTGAHeaderSize = 18;
   BCOMPILETIMEASSERT(sizeof(BTGAHeader) == cTGAHeaderSize);
   if (!mpStream->readBytes(&mTGAHeader, sizeof(mTGAHeader)))
   {
      return false;
   }

   mTGAHeader.littleEndianToNative();

   if (!skipBytes(p.mIDLen))
   {
      return false;
   }

   mWidth = p.mWidth;
   mHeight = p.mHeight;
   const int cMaxWidth = 8192;
   const int cMaxHeight = 8192;
   if ((mWidth < 1) || (mHeight < 1) || (mWidth > cMaxWidth) || (mHeight > cMaxHeight))
   {
      setError("Invalid or unsupported mWidth/mHeight");
      return false;
   }

   if (p.mDesc >> 6)
   {
      setError("Unsupported image");
      return false;
   }

   mYFlipped = (p.mDesc & 0x20) == 0;
   mXFlipped = (p.mDesc & 0x10) != 0;

   int imageType = p.mType;
   if (imageType > 8)
   {
      mRLE = true;
      imageType -= 8;
   }

   BPixelFormat::BColorType mType = BPixelFormat::cColorTypeRGB;

   switch (imageType)
   {
      case 1:
      {
         if ((p.mDepth != 8) || (p.mCmap != 1) || (p.mCmapLen == 0))
         {
            return false;
         }

         mType = BPixelFormat::cColorTypePalettized;
         break;
      }
      case 2:
      {
         if (p.mDepth == 8)
         {
            return false;
         }

         mType = BPixelFormat::cColorTypeRGB;
         break;
      }
      case 3:
      {
         if ((p.mDepth != 8) || (p.mCmap != 0) || (p.mCmapLen != 0))
         {
            return false;
         }

         mType = BPixelFormat::cColorTypeGrayscale;
         break;
      }
      default:
      {
         return false;
      }
   }

   switch (p.mDepth)
   {
      case 32:
      {
         mPixelFormat.set(4, 0xFF<<16, 0xFF<<8, 0xFF, static_cast<DWORD>(0xFF)<<24, mType);
         break;
      }
      case 24:
      {
         mPixelFormat.set(3, 0xFF<<16, 0xFF<<8, 0xFF, 0, mType);
         break;
      }
      case 8:
      {
         mPixelFormat.set(1, 0xFF, 0, 0, 0, mType);
         break;
      }
      case 16: 
      {
         mPixelFormat.set(2, 31<<10, 31<<5, 31, 0, mType);
         break;
      }
      case 15:
      {
         mPixelFormat.set(2, 31<<10, 31<<5, 31, 0, mType);
         break;
      }
      default:
      {
         setError("Unsupported bitdepth");
         return false;
      }
   }

   mBPL = mWidth * mPixelFormat.getBytesPerPixel();

   if ((p.mCmap) && (p.mCmapLen))
   {
      if (mType == BPixelFormat::cColorTypePalettized)
      {
         if (((p.mCMapSize != 24) && (p.mCMapSize != 15) && (p.mCMapSize != 16)) || (p.mCmapLen > 256) || (p.mCMapFirst != 0))
         {
            setError("Invalid or unsupported palette type");
            return false;
         }

         uchar buf[4 * 256];
         uchar* pBuf = buf;
         
         BRGBAColor palette[256];

         if (p.mCMapSize == 24)
         {
            if (!get(pBuf, p.mCmapLen * 3)) 
               return false;

            for (int i = 0; i < p.mCmapLen; i++, pBuf += 3)
               palette[i].set(pBuf[2], pBuf[1], pBuf[0], 255);
         }
         else
         {
            if (!get(pBuf, p.mCmapLen * 2)) 
               return false;

            for (int i = 0; i < p.mCmapLen; i++, pBuf += 2)
            {
               const uint j = pBuf[0] | (static_cast<uint>(pBuf[1]) << 8);

               palette[i].set(
                  (((j >> 10) & 31) * 255 + 15) / 31,
                  (((j >>  5) & 31) * 255 + 15) / 31,
                  ((j & 31) * 255 + 15) / 31,
                  255
                  );
            }
         }

         for (int i = p.mCmapLen; i < 256; i++)
            palette[i] = palette[0];

         mPixelFormat.bindPalette(BPalette(palette, p.mCmapLen));
      }
      else
      {
         const uint palBytes = (p.mCMapSize / 8) * p.mCmapLen;
         if (stream.skipBytes(palBytes) < palBytes)
            return false;
      }         
   }
   
   if ((mType == BPixelFormat::cColorTypePalettized) && (!mPixelFormat.getPal()))
   {
      BRGBAColor palette[256];
       
      for (int i = 0; i < 256; i++)
         palette[i].set(i, i, i, 255);

      mPixelFormat.bindPalette(BPalette(palette, 256));
   }
   
   mLinesLeft = mHeight;

   clearError();

   mValid = true;

   return true;
}
//--------------------------------------------------------------------------------------------------------------------------------------------
void BTGAReader::deinit(void)
{
   if (mpLineBuf)
   {
      delete [] mpLineBuf;
      mpLineBuf = NULL;
   }

   if (mpBuf)
   {
      delete [] mpBuf;
      mpBuf = NULL;
   }

   clear();
}
//--------------------------------------------------------------------------------------------------------------------------------------------
bool BTGAReader::decodeRLE1(uchar* pDst)
{
   int pixelsLeft = mWidth;
   while (pixelsLeft > 0)
   {
      const char *p = reinterpret_cast<const char*>(getBuffered(1));
      if (!p) 
         return false;

      int c = *p;
      if (c < 0)
      {
         c = (c & 0x7F) + 1;
         if ((pixelsLeft -= c) < 0) 
         { 
            setError("Decompression error"); 
            return false; 
         }
         const uchar* pSrc = getBuffered(1);
         if (!pSrc) 
            return false;

         uchar r = *pSrc;
         memset(pDst, r, c);
         pDst += c;
      }
      else
      {
         c++;
         if ((pixelsLeft -= c) < 0) 
         { 
            setError("Decompression error"); 
            return false; 
         }
         if (!get(pDst, c)) 
            return false;
         pDst += c;
      }
   }
   return true;
}
//--------------------------------------------------------------------------------------------------------------------------------------------
bool BTGAReader::decodeRLE2(ushort* pDst)
{
   int pixelsLeft = mWidth;
   while (pixelsLeft > 0)
   {
      const char *p = reinterpret_cast<const char*>(getBuffered(1));
      if (!p) 
         return false;
      int c = *p;
      if (c < 0)
      {
         c = (c & 0x7F) + 1;
         if ((pixelsLeft -= c) < 0) 
         { 
            setError("Decompression error"); 
            return false; 
         }
         const ushort* pSrc = reinterpret_cast<const ushort*>(getBuffered(2));
         if (!pSrc) 
            return false;
         ushort r = Utils::GetValueLittleEndian<ushort>(pSrc);
         while (c--)
         {
            Utils::WriteValueLittleEndian<ushort>(pDst, r);
            pDst++;
         }
      }
      else
      {
         c++;
         if ((pixelsLeft -= c) < 0) 
         { 
            setError("Decompression error"); 
            return false; 
         }
         if (!get(pDst, c * 2)) 
            return false;
                  
         pDst += c;
      }
   }
   return true;
}
//--------------------------------------------------------------------------------------------------------------------------------------------
bool BTGAReader::decodeRLE3(uchar* pDst)
{
   int pixelsLeft = mWidth;
   while (pixelsLeft > 0)
   {
      const char *p = reinterpret_cast<const char*>(getBuffered(1));
      if (!p) 
         return false;
      int c = *p;
      if (c < 0)
      {
         c = (c & 0x7F) + 1;
         if ((pixelsLeft -= c) < 0) 
         { 
            setError("Decompression error"); 
            return false; 
         }
         const uchar* pSrc = getBuffered(3);
         if (!pSrc) 
            return false;
         while (c--)
         {
            pDst[0] = pSrc[0];
            pDst[1] = pSrc[1];
            pDst[2] = pSrc[2];
            pDst += 3;
         }
      }
      else
      {
         c++;
         if ((pixelsLeft -= c) < 0) 
         { 
            setError("Decompression error"); 
            return false; 
         }
         if (!get(pDst, c * 3))   
            return false;
         pDst += c * 3;
      }
   }
   return true;
}
//--------------------------------------------------------------------------------------------------------------------------------------------
bool BTGAReader::decodeRLE4(DWORD* pDst)
{
   int pixelsLeft = mWidth;
   while (pixelsLeft > 0)
   {
      const char *p = reinterpret_cast<const char*>(getBuffered(1));
      if (!p) 
         return false;
      int c = *p;
      if (c < 0)
      {
         c = (c & 0x7F) + 1;
         if ((pixelsLeft -= c) < 0) 
         { 
            setError("Decompression error"); 
            return false; 
         }
         const DWORD* pSrc = reinterpret_cast<const DWORD*>(getBuffered(4));
         if (!pSrc) 
            return false;
         DWORD r = Utils::GetValueLittleEndian<DWORD>(pSrc);
         while (c--)
         {
            Utils::WriteValueLittleEndian<DWORD>(pDst, r);
            pDst++;
         }
      }
      else
      {
         c++;
         if ((pixelsLeft -= c) < 0) 
         {  
            setError("Decompression error"); 
            return false; 
         }
         if (!get(pDst, c * 4)) 
            return false;
         
         pDst += c;
      }
   }
   return true;
}
//--------------------------------------------------------------------------------------------------------------------------------------------
bool BTGAReader::decode(const void*& pScanOfs, uint& pScanLen)
{
   pScanOfs = NULL;
   pScanLen = 0;

   const uchar* pRet = NULL;

   if ((!mValid) || (mError) || (mLinesLeft <= 0))
      return false;

   if (mRLE)
   {
      if (!mpLineBuf)
         mpLineBuf = new uchar[mBPL];

      pRet = mpLineBuf;

      bool succeeded = false;

      switch (mTGAHeader.mDepth)
      {
         case 32:
         {
            succeeded = decodeRLE4(reinterpret_cast<DWORD*>(mpLineBuf));
            break;
         }
         case 24:
         {
            succeeded = decodeRLE3(reinterpret_cast<uchar*>(mpLineBuf));
            break;
         }
         case 15:
         case 16:
         {
            succeeded = decodeRLE2(reinterpret_cast<ushort*>(mpLineBuf));
            break;
         }
         case 8:
         {
            succeeded = decodeRLE1(reinterpret_cast<uchar*>(mpLineBuf));
            break;
         }
         default:
            BASSERT(false);
      }

      if (!succeeded)
         return false;

      pRet = mpLineBuf;
   }
   else
   {
      if (!checkBytes(mBPL))
      {
         return false;
      }      

      pRet = reinterpret_cast<const uchar*>(mpStream->ptr(mBPL));

      if (!pRet)
      {
         if (!mpLineBuf)
            mpLineBuf = new uchar[mBPL];

         if (!get(mpLineBuf, mBPL))
            return false;

         pRet = mpLineBuf;
      }

      BASSERT(pRet);
   }

   mLinesLeft--;

   pScanOfs = pRet;
   pScanLen = mBPL;

   return true;
}
//--------------------------------------------------------------------------------------------------------------------------------------------

// File: writeHDR.cpp
// rg [6/7/06] - Initial implemenation
#include "xcore.h"
#include "writeHDR.h"

BHDRWriter::BHDRWriter() :
   mWidth(0), 
   mHeight(0), 
   mGamma(2.2f), 
   mpStream(NULL)
{
}

BHDRWriter::~BHDRWriter()
{
   close();
}

bool BHDRWriter::open(BStream& stream, uint width, uint height, bool xFlipped, bool yFlipped, float gamma)
{
   mWidth = width;
   mHeight = height;
   mGamma = gamma;
   mpStream = &stream;
   
   char buf[256];

   sprintf_s(buf, sizeof(buf), "#?RADIANCE\nFORMAT=32-bit_rle_rgbe\n\n%cY %u %cX %u\n", yFlipped ? '+' : '-', height, xFlipped ? '-' : '+', width);
   const uint len = strlen(buf);
   
   if (stream.writeBytes(buf, len) != len)
      return false;
      
   return true;
}

// Hacked from "Real Pixels", Graphics Gems 2

const int MIN_ELEN = 8; /* minimum pScanLine length for encoding */
const int MIN_RUN = 4; /* minimum run length */

bool BHDRWriter::packScanline(const COLR* pScanLine, int len)    
{
   int i, j, beg, cnt = 0;
   int c2;

   if (len < MIN_ELEN)    /* too small to encode */
      return mpStream->writeBytes(pScanLine, sizeof(COLR) * len) == (sizeof(COLR) * len);
      
   if (len > 32767)    /* too big! */
      return false;
   
   mpStream->putch(2);      /* put magic header */
   mpStream->putch(2);
   mpStream->putch((uchar)(len >> 8));
   mpStream->putch((uchar)(len & 255));
   
   /* put components separately */
   for (i = 0; i < 4; i++) 
   {
      for (j = 0; j < len; j += cnt) 
      {  
         /* find next run */
         for (beg = j; beg < len; beg += cnt) 
         {
            for (cnt = 1; cnt < 127 && beg+cnt < len && pScanLine[beg+cnt][i] == pScanLine[beg][i]; cnt++)
            {
               ;
            }
                        
            if (cnt >= MIN_RUN)
            {
               /* long enough */
               break;      
            }
         }
         
         if (beg-j > 1 && beg-j < MIN_RUN) 
         {
            c2 = j+1;
            while (pScanLine[c2++][i] == pScanLine[j][i])
               if (c2 == beg) 
               {  
                  /* short run */
                  mpStream->putch((uchar)(128+beg-j));
                  mpStream->putch(pScanLine[j][i]);
                  j = beg;
                  break;
               }
         }
         while (j < beg) 
         {   
            /* write out non-run */
            if ((c2 = beg-j) > 128) 
               c2 = 128;
            
            mpStream->putch((uchar)c2);
            
            while (c2--)
               mpStream->putch(pScanLine[j++][i]);
         }
         if (cnt >= MIN_RUN) 
         {    
            /* write out run */
            mpStream->putch((uchar)(128+cnt));
            mpStream->putch(pScanLine[beg][i]);
         } 
         else
            cnt = 0;
      }
   }
   
   return !mpStream->errorStatus();
}

bool BHDRWriter::writeLine(const void* pScanLine, cInputType inputType)
{
   BDEBUG_ASSERT(mpStream && pScanLine);
   
   const void* pSrc = pScanLine;   
   
   switch (inputType)
   {
      case cInputRGBE:
      {
         break;
      }
      case cInputRGB:
      {
         if (mGammaCurve.empty())
         {
            for (uint i = 0; i < 256; i++)
               mGammaCurve[i] = powf(i * (1.0f / 255.0f), mGamma);
         }
         
         mTempBuf.resize(mWidth);
         pSrc = mTempBuf.getPtr();
         
         for (uint i = 0; i < mWidth; i++)
         {
            const BRGBAColor& c = static_cast<const BRGBAColor*>(pScanLine)[i];
            BRGBAColorF f(mGammaCurve[c[0]], mGammaCurve[c[1]], mGammaCurve[c[2]], 0.0f);
            BHDRColorUtils::packRGBE(f, mTempBuf[i]);
         }
         
         break;
      }
      case cInputRGBF:
      {
         mTempBuf.resize(mWidth);
         pSrc = mTempBuf.getPtr();

         for (uint i = 0; i < mWidth; i++)
         {
            const BRGBAColorF& c = static_cast<const BRGBAColorF*>(pScanLine)[i];
            BHDRColorUtils::packRGBE(c, mTempBuf[i]);
         }
         
         break;
      }
   }      
         
   return packScanline(static_cast<const COLR*>(pSrc), mWidth);
}

bool BHDRWriter::close(void)
{
   mpStream = NULL;
   mTempBuf.clear();
   mGammaCurve.clear();
   
   return true;
}














//------------------------------------------------------------------------------
// File: pixel_format_trans.cpp
//------------------------------------------------------------------------------
#include "ximage.h"
#include "pixelFormat.h"
//------------------------------------------------------------------------------
BPixelFormat::BTranslator::BTranslator(
   const BPixelFormat& src, const BPixelFormat& dst,
   uchar alphaAnd, uchar alphaXor,
   int defaultAlpha,
   bool ignoreSrcPal, bool ignoreDstPal) :
      mSrc(src), 
      mDst(dst),
      mIgnoreSrcPal(ignoreSrcPal),
      mIgnoreDstPal(ignoreDstPal)
{
   mEqual = (src == dst);

   if (mEqual)
      return;

   mAlphaAnd = alphaAnd; 
   mAlphaXor = alphaXor; 
   mDefaultAlpha = defaultAlpha;

   mOptimized = false;

   if (mDst.getType() == BPixelFormat::cColorTypePalettized)
   {
      if ((mSrc.getType() == BPixelFormat::cColorTypePalettized) && (mSrc.getPal()) && (!mDst.getPal()))
      {
         mIgnoreSrcPal = true;
         mEqual = mSrc.isEqual(mDst, true);
      }
   }
   else if 
      ((mDst.getType() == BPixelFormat::cColorTypeRGB) &&  
      (mDst.getBytesPerPixel() <= 4) &&
      (mSrc.getBytesPerPixel() <= 4))
   {
      if (mSrc.getBytesPerPixel() == 1)
      {
         mOptimized = true;

         for (int i = 0; i < 256; i++)
         {
            BRGBAColor c(mSrc.unmap(i, mIgnoreSrcPal));

            if (!mSrc.getMappedAlpha())
            {
               c.a = (uchar)mDefaultAlpha;

               if (mDst.getType() == BPixelFormat::cColorTypeAlpha)
                  c.a = c.r;
            }

            mXLat0[i] = dst.map(alphaModulate(c, alphaAnd, alphaXor), mIgnoreDstPal);
         }
      }
      else if 
         ((mSrc.getType() == BPixelFormat::cColorTypeRGB) && 
         (!mSrc.getColorkeyed()) && 
         (alphaXor == 0x00) && (alphaAnd == 0xFF))
      {
         mOptimized = true;

         for (int i = 0; i < 256; i++)
         {
            mXLat0[i] = mDst.pack(mSrc.unpackPreciseNoRound(i));
            mXLat1[i] = mDst.pack(mSrc.unpackPreciseNoRound(i << 8));
            mXLat2[i] = mDst.pack(mSrc.unpackPreciseNoRound(i << 16));
            mXLat3[i] = mDst.pack(mSrc.unpackPreciseNoRound(i << 24));
            if (!mSrc.getMappedAlpha())
               mXLat0[i] |= mDst.pack(BRGBAColor(0, 0, 0, defaultAlpha));
         }
      }
   }
}
//------------------------------------------------------------------------------
template<> struct BPixelReader<1>
{
   static uint readLittleEndian(const uchar* &Ps, const BPixelFormat::BTranslator& xlator)
   {
      uint ret = xlator.mXLat0[Ps[0]];
      Ps++;
      return ret;
   }

   static uint readBigEndian(const uchar* &Ps, const BPixelFormat::BTranslator& xlator)
   {
      return readLittleEndian(Ps, xlator);
   }
};
//------------------------------------------------------------------------------
template<> struct BPixelReader<2>
{
   static uint readLittleEndian(const uchar* &Ps, const BPixelFormat::BTranslator& xlator)
   {
      uint ret = xlator.mXLat0[Ps[0]] + xlator.mXLat1[Ps[1]];
      Ps += 2;
      return ret;
   }

   static uint readBigEndian(const uchar* &Ps, const BPixelFormat::BTranslator& xlator)
   {
      uint ret = xlator.mXLat0[Ps[1]] + xlator.mXLat1[Ps[0]];
      Ps += 2;
      return ret;
   }
};
//------------------------------------------------------------------------------
template<> struct BPixelReader<3>
{
   static uint readLittleEndian(const uchar* &Ps, const BPixelFormat::BTranslator& xlator)
   {
      uint ret = xlator.mXLat0[Ps[0]] + xlator.mXLat1[Ps[1]] + xlator.mXLat2[Ps[2]];
      Ps += 3;
      return ret;
   }

   static uint readBigEndian(const uchar* &Ps, const BPixelFormat::BTranslator& xlator)
   {
      uint ret = xlator.mXLat0[Ps[2]] + xlator.mXLat1[Ps[1]] + xlator.mXLat2[Ps[0]];
      Ps += 3;
      return ret;
   }
};
//------------------------------------------------------------------------------
template<> struct BPixelReader<4>
{
   static uint readLittleEndian(const uchar* &Ps, const BPixelFormat::BTranslator& xlator)
   {
      uint ret = xlator.mXLat0[Ps[0]] + xlator.mXLat1[Ps[1]] + xlator.mXLat2[Ps[2]] + xlator.mXLat3[Ps[3]];
      Ps += 4;
      return ret;
   }

   static uint readBigEndian(const uchar* &Ps, const BPixelFormat::BTranslator& xlator)
   {
      uint ret = xlator.mXLat0[Ps[3]] + xlator.mXLat1[Ps[2]] + xlator.mXLat2[Ps[1]] + xlator.mXLat3[Ps[0]];
      Ps += 4;
      return ret;
   }
};
//------------------------------------------------------------------------------
template<> struct BPixelWriter<1>
{
   static void writeLittleEndian(uchar* &Pd, uint t) 
   { 
      *Pd++ = (uchar)t;
   }

   static void writeBigEndian(uchar* &Pd, uint t) 
   { 
      *Pd++ = (uchar)t;
   }
};
//------------------------------------------------------------------------------
template<> struct BPixelWriter<2>
{
   static void writeLittleEndian(uchar* &Pd, uint t) 
   { 
      Utils::WriteValueLittleEndian<WORD>(Pd, (WORD)t);
      Pd += 2; 
   }

   static void writeBigEndian(uchar* &Pd, uint t) 
   { 
      Utils::WriteValueBigEndian<WORD>(Pd, (WORD)t);
      Pd += 2; 
   }
};
//------------------------------------------------------------------------------
template<> struct BPixelWriter<3>
{
   static void writeLittleEndian(uchar* &Pd, uint t) 
   { 
      ((uchar*)Pd)[0] = (uchar)t;
      ((uchar*)Pd)[1] = (uchar)(t>>8);
      ((uchar*)Pd)[2] = (uchar)(t>>16);
      Pd += 3; 
   }

   static void writeBigEndian(uchar* &Pd, uint t) 
   { 
      ((uchar*)Pd)[2] = (uchar)t;
      ((uchar*)Pd)[1] = (uchar)(t>>8);
      ((uchar*)Pd)[0] = (uchar)(t>>16);
      Pd += 3; 
   }
};
//------------------------------------------------------------------------------
template<> struct BPixelWriter<4>
{
   static void writeLittleEndian(uchar* &Pd, uint t) 
   { 
      Utils::WriteValueLittleEndian<DWORD>(Pd, t);
      Pd += 4; 
   }

   static void writeBigEndian(uchar* &Pd, uint t) 
   { 
      Utils::WriteValueBigEndian<DWORD>(Pd, t);
      Pd += 4; 
   }
};
//------------------------------------------------------------------------------
template<class PR, class PW>
struct BTranslatePixels
{
   static void translate(uchar* Pd, const uchar* Ps, int num, const BPixelFormat::BTranslator& xlator)
   {
      const bool srcBigEndian = xlator.srcFormat().getBigEndian();
      const bool dstBigEndian = xlator.dstFormat().getBigEndian();

      if ((!srcBigEndian) && (!dstBigEndian))
      {
         for (int i = num; i > 0; i--)
            PW::writeLittleEndian(Pd, PR::readLittleEndian(Ps, xlator));
      }
      else if ((srcBigEndian) && (!dstBigEndian))
      {
         for (int i = num; i > 0; i--)
            PW::writeLittleEndian(Pd, PR::readBigEndian(Ps, xlator));
      }
      else if ((srcBigEndian) && (dstBigEndian))
      {
         for (int i = num; i > 0; i--)
            PW::writeBigEndian(Pd, PR::readBigEndian(Ps, xlator));
      }
      else 
      {
         for (int i = num; i > 0; i--)
            PW::writeLittleEndian(Pd, PR::readLittleEndian(Ps, xlator));
      }
   }
};
//------------------------------------------------------------------------------
void BPixelFormat::BTranslator::translate(void* Pd, const void* Ps, int numPixels)
{
   uchar* pDstPixels = reinterpret_cast<uchar*>(Pd);
   const uchar* Ppixels = reinterpret_cast<const uchar*>(Ps);

   BDEBUG_ASSERT(numPixels >= 0);
   BDEBUG_ASSERT(pDstPixels);
   BDEBUG_ASSERT(Ppixels);

   if (mEqual)
   {
      memcpy(pDstPixels, Ppixels, mSrc.getBytesPerPixel() * numPixels);
      return;
   }

   const uchar* pSrc = Ppixels;
   uchar* pDst = pDstPixels;

   const int srcBpp = mSrc.getBytesPerPixel();
   const int dstBpp = mDst.getBytesPerPixel();

   if (mOptimized)
   {
#define TRANSLATE_PIXELS(x) \
   switch (dstBpp) \
      { \
         case 1: BTranslatePixels<x, BPixelWriter<1> >::translate(pDst, pSrc, numPixels, *this); break; \
         case 2: BTranslatePixels<x, BPixelWriter<2> >::translate(pDst, pSrc, numPixels, *this); break; \
         case 3: BTranslatePixels<x, BPixelWriter<3> >::translate(pDst, pSrc, numPixels, *this); break; \
         case 4: BTranslatePixels<x, BPixelWriter<4> >::translate(pDst, pSrc, numPixels, *this); break; \
      } 

      switch (srcBpp)
      {
         case 1: 
         { 
            TRANSLATE_PIXELS(BPixelReader<1>);
            return; 
         }
         case 2: 
         { 
            TRANSLATE_PIXELS(BPixelReader<2>);
            return; 
         }
         case 3: 
         { 
            TRANSLATE_PIXELS(BPixelReader<3>);
            return; 
         }
         case 4: 
         { 
            TRANSLATE_PIXELS(BPixelReader<4>);
            return; 
         }
      }
   }

   for (int i = numPixels; i > 0; i--, pSrc += srcBpp, pDst += dstBpp)
   {
      BRGBAColor c(mSrc.unmap(mSrc.read(pSrc), mIgnoreSrcPal));

      if (!mSrc.getMappedAlpha())
      {
         c.a = (uchar)mDefaultAlpha;

         if (mDst.getType() == BPixelFormat::cColorTypeAlpha)
            c.a = (uchar)c.getGrayscale();
      }

      c = alphaModulate(c, (uchar)mAlphaAnd, (uchar)mAlphaXor);

      uint d = mDst.map(c, mIgnoreDstPal);

      mDst.write(pDst, d);
   }
}
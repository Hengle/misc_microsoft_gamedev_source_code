//------------------------------------------------------------------------------
// File: pixelFormat.cpp
//------------------------------------------------------------------------------
#include "ximage.h"
#include "pixelFormat.h"
//------------------------------------------------------------------------------
const BPixelFormat G_r1g1b1_format     (2, 1<<2,     1<<1,     1,          0,        BPixelFormat::cColorTypeRGB);
const BPixelFormat G_r2g2b2_format     (2, 3<<4,     3<<2,     3,          0,        BPixelFormat::cColorTypeRGB);
const BPixelFormat G_r3g3b3_format     (2, 7<<6,     7<<3,     7,          0,        BPixelFormat::cColorTypeRGB);
const BPixelFormat G_r4g4b4_format     (2, 15<<8,    15<<4,    15,         0,        BPixelFormat::cColorTypeRGB);
const BPixelFormat G_a4r4g4b4_format   (2, 15<<8,    15<<4,    15,         15<<12,   BPixelFormat::cColorTypeRGB);
const BPixelFormat G_r5g6b5_format     (2, 31<<11,   63<<5,    31,         0,        BPixelFormat::cColorTypeRGB);
const BPixelFormat G_r5g5b5_format     (2, 31<<10,   31<<5,    31,         0,        BPixelFormat::cColorTypeRGB);
const BPixelFormat G_a1r5g5b5_format   (2, 31<<10,   31<<5,    31,         1<<15,    BPixelFormat::cColorTypeRGB);
const BPixelFormat G_rgb_format        (3, 0xFF<<16, 0xFF<<8,  0xFF,       0,        BPixelFormat::cColorTypeRGB);
const BPixelFormat G_bgr_format        (3, 0xFF,     0xFF<<8,  0xFF<<16,   0,        BPixelFormat::cColorTypeRGB);
const BPixelFormat G_abgr_format       (4, 0xFFU,     0xFFU<<8,  0xFFU<<16,   0xFFU<<24, BPixelFormat::cColorTypeRGB);
const BPixelFormat G_rgba_format       (4, 0xFFU<<24, 0xFFU<<16, 0xFFU<<8,    0xFFU,     BPixelFormat::cColorTypeRGB);
const BPixelFormat G_xbgr_format       (4, 0xFFU,     0xFFU<<8,  0xFFU<<16,   0,        BPixelFormat::cColorTypeRGB);
const BPixelFormat G_xrgb_format       (4, 0xFFU<<16, 0xFFU<<8,  0xFFU,       0,        BPixelFormat::cColorTypeRGB);
const BPixelFormat G_argb_format       (4, 0xFFU<<16, 0xFFU<<8,  0xFFU,       0xFFU<<24, BPixelFormat::cColorTypeRGB);
const BPixelFormat G_grayscale_format  (1, 0xFF,     0,        0,          0,        BPixelFormat::cColorTypeGrayscale);
const BPixelFormat G_grayscale4_format (1, 0xF,     0,        0,          0,        BPixelFormat::cColorTypeGrayscale);
const BPixelFormat G_grayscale2_format (1, 0x3,     0,        0,          0,        BPixelFormat::cColorTypeGrayscale);
const BPixelFormat G_grayscale1_format (1, 0x1,     0,        0,          0,        BPixelFormat::cColorTypeGrayscale);
const BPixelFormat G_palettized_format (1, 0xFF,     0,        0,          0,        BPixelFormat::cColorTypePalettized);

const BPixelFormat G_grayscale_alpha_format(2, 0xFF, 0, 0, 0xFF<<8, BPixelFormat::cColorTypeGrayscale);
const BPixelFormat G_palettized_alpha_format(2, 0xFF, 0, 0, 0xFF<<8, BPixelFormat::cColorTypePalettized);
const BPixelFormat G_alpha_format(1, 0, 0,  0,  0xFF, BPixelFormat::cColorTypeAlpha);

//------------------------------------------------------------------------------
BPixelFormat::BPixelFormat() 
{ 
   clear(); 
}

//------------------------------------------------------------------------------
BPixelFormat::BPixelFormat(
             uint bytesPerPixel,
             uint rMask, 
             uint gMask, 
             uint bMask, 
             uint aMask,
             BColorType type,
             bool colorkeyed,
             bool bigEndian) 
{
   clear();
   set(bytesPerPixel, rMask, gMask, bMask, aMask, type, colorkeyed, bigEndian);
}

//------------------------------------------------------------------------------
BPixelFormat::BPixelFormat(
             uint bytesPerPixel,
             const uint masks[cMaxComponents],
             BColorType type,
             bool colorkeyed,
             bool bigEndian)
{
   clear();
   set(bytesPerPixel, masks[0], masks[1], masks[2], masks[3], type, colorkeyed, bigEndian);
}

//------------------------------------------------------------------------------
BPixelFormat::BPixelFormat(const BPixelFormat& pf)
{
   clear();
   set(pf);
}

//------------------------------------------------------------------------------
BPixelFormat& BPixelFormat::operator = (const BPixelFormat& pf)
{
   if (this == &pf)
      return *this;

   clear();
   set(pf);

   return (*this);
}

//------------------------------------------------------------------------------
BPixelFormat::~BPixelFormat()
{
   if (mpPal)
      delete mpPal;
}

//------------------------------------------------------------------------------
void BPixelFormat::set(const BPixelFormat& pf)
{
   if (this == &pf)
      return;

   set(
      pf.mBytesPerPixel, 
      pf.mFormat[0].mMask, 
      pf.mFormat[1].mMask, 
      pf.mFormat[2].mMask, 
      pf.mFormat[3].mMask, 
      pf.mColorType, 
      pf.mColorkeyed,
      pf.mBigEndian);

   if (pf.getPal())
      bindPalette(*pf.getPal());
}

//------------------------------------------------------------------------------
void BPixelFormat::set(
   uint bytesPerPixel,
   uint rMask, 
   uint gMask, 
   uint bMask, 
   uint aMask,
   BColorType type,
   bool colorkeyed,
   bool bigEndian)
{
   debugRangeCheckIncl(bytesPerPixel, 1U, (uint)cMaxComponents);

   mColorType = type;
   mColorkeyed = colorkeyed;
   mBigEndian = bigEndian;

   mBytesPerPixel = bytesPerPixel;
   mBitsPerPixel = bytesPerPixel * 8;

   delete mpPal;
   mpPal = NULL;

   mFormat[0].mMask = rMask;
   mFormat[1].mMask = gMask;
   mFormat[2].mMask = bMask;
   mFormat[3].mMask = aMask;
   mNumComponents = 0;
   for (int i = 0; i < cMaxComponents; i++)
   {
      mFormat[i].mBits = (char)Math::BitMaskLength(mFormat[i].mMask);
      debugRangeCheckIncl<int>(mFormat[i].mBits, 0, 8);

      mFormat[i].mQuant = 8 - mFormat[i].mBits;

      mFormat[i].mOfs = (char)Math::BitMaskOffset(mFormat[i].mMask);

      if (mFormat[i].mBits)
         mNumComponents++;
   }

   if (mColorType == cColorTypeUnknown)
   {
      mColorType = cColorTypeRGB;	

      if (mNumComponents == 1)
         mColorType = cColorTypeGrayscale;
      else if (mNumComponents < 3)
         mColorType = cColorTypeUndefined;
   }

   if (mColorType != cColorTypeGrayscale)
   {
      for (int i = 0; i < cMaxComponents; i++)
      {
         mFormat[i].mUpMask = mFormat[i].mMask;
         mFormat[i].mUpBits = mFormat[i].mBits;
         mFormat[i].mUpOfs	= mFormat[i].mOfs;
         mFormat[i].mUpQuant = mFormat[i].mQuant;
      }
   }
   else
   {
      // replicate R mMask for grayscale 
      mFormat[0].mUpMask = rMask;
      mFormat[1].mUpMask = rMask;
      mFormat[2].mUpMask = rMask;
      mFormat[3].mUpMask = aMask;

      for (int i = 0; i < cMaxComponents; i++)
      {
         mFormat[i].mUpBits = (char)Math::BitMaskLength(mFormat[i].mUpMask);
         mFormat[i].mUpQuant = 8 - mFormat[i].mUpBits;
         mFormat[i].mUpOfs = (char)Math::BitMaskOffset(mFormat[i].mUpMask);
      }
   }

   mNumSigComponents = 0;
   if (mColorType == cColorTypePalettized)
   {
      mSigComponentMap[mNumSigComponents++] = 0;
      mSigComponentMap[mNumSigComponents++] = 1;
      mSigComponentMap[mNumSigComponents++] = 2;
      if (getMappedAlpha())
         mSigComponentMap[mNumSigComponents++] = 3;
   }
   else 
   {
      for (int i = 0; i < cMaxComponents; i++)
         if (mFormat[i].mBits)
            mSigComponentMap[mNumSigComponents++] = i;

      // colorkey check
      if ((getMappedAlpha()) && (mFormat[3].mBits == 0))
         mSigComponentMap[mNumSigComponents++] = 3;
   }	

   mDefaultAlpha = 0;
   if (!getMappedAlpha())	
      mDefaultAlpha = 255;
}
//------------------------------------------------------------------------------
bool BPixelFormat::isEqual(const BPixelFormat& b, bool ignorePal) const
{
   if ((mColorType			!= b.mColorType)		||
      (mBitsPerPixel		   != b.mBitsPerPixel)	||
      (mBytesPerPixel	   != b.mBytesPerPixel) ||
      (mNumComponents		!= b.mNumComponents)	||
      (mFormat[0].mMask		!= b.mFormat[0].mMask)	|| 
      (mFormat[1].mMask		!= b.mFormat[1].mMask)	||
      (mFormat[2].mMask		!= b.mFormat[2].mMask)	|| 
      (mFormat[3].mMask		!= b.mFormat[3].mMask)  ||
      (mBigEndian          != b.mBigEndian)
      )
   {
      return false;
   }

   if (!ignorePal)
   {
      if ((mpPal != NULL) != (b.mpPal != NULL))
         return false;

      if (mpPal) 
      {
         if ((*mpPal) != (*b.mpPal))
            return false;
      }
   }

   return true;
}

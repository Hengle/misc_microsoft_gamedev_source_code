//------------------------------------------------------------------------------
// file: pixelFormat.h
//------------------------------------------------------------------------------
#pragma once
//------------------------------------------------------------------------------
#include "palette.h"
//------------------------------------------------------------------------------
template <int BytesPerPixel> struct BPixelReader { };
template <int BytesPerPixel> struct BPixelWriter { };
//------------------------------------------------------------------------------
class BPixelFormat
{
public:
   typedef BRGBAColor					unmapped_type;
   typedef uint					      mapped_type;

   enum { cMaxComponents = 4 };
   enum { cInvariantFormat = false };

   enum BColorType
   {
      cColorTypeUnknown,
      cColorTypeRGB,					// RGB, BRGBAColor, or BRGBAColor colorkeyed
      cColorTypePalettized,	   // might have alpha
      cColorTypeGrayscale,		   // might have alpha
      cColorTypeAlpha,				// alpha only
      cColorTypeUndefined	
   };

   BPixelFormat();

   BPixelFormat(
      uint bytesPerPixel,
      uint rMask, 
	   uint gMask, 
	   uint bMask, 
	   uint aMask = 0,
      BColorType type = cColorTypeUnknown,
      bool colorkeyed = false,
      bool bigEndian = false);

   BPixelFormat(
      uint bytesPerPixel,
      const uint masks[cMaxComponents],
      BColorType type = cColorTypeUnknown,
      bool colorkeyed = false,
      bool bigEndian = false);

   BPixelFormat(const BPixelFormat& pf);

   BPixelFormat& operator = (const BPixelFormat& pf);

   ~BPixelFormat();

   void set(
      uint bytesPerPixel,
      uint rMask, uint gMask, uint bMask, uint aMask = 0,
      BColorType type = cColorTypeUnknown,
      bool colorkeyed = false,
      bool bigEndian = false);

   void set(const BPixelFormat& pf);
   
   bool getBigEndian(void) const
   {
      return mBigEndian;
   }

   bool getColorkeyed(void) const 
   { 
	   return mColorkeyed;
   }

   bool getPackedAlpha(void) const 
   { 
	   return mFormat[3].mMask != 0; 
   }

   bool getMappedAlpha(void) const 
   {
      if (mpPal)
         return (mpPal->getHasAlpha()) || (mFormat[3].mBits != 0);
      else if (mColorkeyed)
         return true;
      else
         return getPackedAlpha(); 
   }

   BColorType getType(void) const 
   { 
	   // yuck, hope this doesn't cause any problems!
	   if ((mColorType == cColorTypePalettized) && (!mpPal))
		   return cColorTypeGrayscale;

	   return mColorType; 
   }

   // bytesPerPixel * 8
   uint getBitsPerPixel(void) const 
   { 
	   return mBitsPerPixel; 
   }

   int getBytesPerPixel(void) const 
   { 
	   return mBytesPerPixel; 
   }

   // # of valid components in pixel format-- does NOT represent the # of valid unmapped components!
   int getNumComponents(void) const 
   { 
	   return mNumComponents; 
   }

   uint getMask(int comp) const 
   { 
	   return mFormat[debugRangeCheck(comp, (int)cMaxComponents)].mMask; 
   }

   int getBits(int comp) const 
   {	
	   return mFormat[debugRangeCheck(comp, (int)cMaxComponents)].mBits; 
   }

   int getOfs(int comp) const 
   { 
	   return mFormat[debugRangeCheck(comp, (int)cMaxComponents)].mOfs; 
   }

   int getQuant(int comp) const 
   { 
	   return mFormat[debugRangeCheck(comp, (int)cMaxComponents)].mQuant; 
   }

   void setColorkeyed(bool colorkeyed) 
   { 
	   mColorkeyed = colorkeyed; 
   }
      
   const BPalette* getPal(void) const 
   { 
	   return mpPal; 
   }

   void bindPalette(const BPalette& pal)
   {
      if (mColorType != cColorTypePalettized)
         return;

      if (!mpPal)  
         mpPal = new BPalette(pal);
      else
         *mpPal = pal;
   }

   // # of significant components in unmapped pixel
   // 3 for palettized or 4 palettized alpha
   // 1 for grayscale or 2 for grayscale alpha
   // 1 for alpha
   // 3 for RGB
   // 4 for BRGBAColor or RGB with colorkey
   int getNumSignificantComponents(void) const
   {
	   return mNumSigComponents;
   }

   // unmapped pixel significant component index map
   int getSignificantComponent(int sigComp) const
   {
	   debugRangeCheck(sigComp, getNumSignificantComponents());
	   return mSigComponentMap[sigComp];
   }
      
   bool isEqual(const BPixelFormat& b, bool ignorePal = false) const;

   bool operator == (const BPixelFormat& b) const
   { 
	   return isEqual(b); 
   }
   	
   bool operator != (const BPixelFormat& b) const
   { 
	   return !(*this == b); 
   }

   uint read(const void* p) const
   {
      switch (mBytesPerPixel)
      {
         case 3:
         {
            const uchar* pBytes = reinterpret_cast<const uchar*>(p);
            
            if (mBigEndian)
            {
               return pBytes[2] | (pBytes[1] << 8) | (pBytes[0] << 16);
            }
            else
            {
               return pBytes[0] | (pBytes[1] << 8) | (pBytes[2] << 16);
            }
         }
         case 4: 
         {
			   return Utils::GetValue<DWORD>(p, mBigEndian);
			}
		   case 1: 
		   {
			   return *(const uchar*)p;
			}
         case 2: 
         {
			   return Utils::GetValue<WORD>(p, mBigEndian);
			}
      }
      return 0;
   }

   void write(void* p, uint t) const
   {
      switch (mBytesPerPixel)
      {
         case 3:
         {
            if (mBigEndian)
            {
               ((uchar*)p)[2] = static_cast<uchar>(t);
               ((uchar*)p)[1] = static_cast<uchar>(t >> 8);
               ((uchar*)p)[0] = static_cast<uchar>(t >> 16);
            }
            else
            {
               ((uchar*)p)[0] = static_cast<uchar>(t);
               ((uchar*)p)[1] = static_cast<uchar>(t >> 8);
               ((uchar*)p)[2] = static_cast<uchar>(t >> 16);
            }
            break;
         }
         case 4: 
         {
            Utils::WriteValue<DWORD>(p, t, mBigEndian);
            break;
         }
         case 1: 
         {
            *(uchar*)p = (uchar)t; 
            break;
         }
         case 2: 
         {
	         Utils::WriteValue<WORD>(p, (WORD)t, mBigEndian);
	         break;
         }
      }
   }

   uint pack(int r, int g, int b) const
   {
      r >>= mFormat[0].mQuant;
      g >>= mFormat[1].mQuant;
      b >>= mFormat[2].mQuant;
      
	   return 
		   (r << mFormat[0].mOfs) | 
		   (g << mFormat[1].mOfs) | 
		   (b << mFormat[2].mOfs);
   }

   uint pack(int r, int g, int b, int a) const
   {
      r >>= mFormat[0].mQuant;
      g >>= mFormat[1].mQuant;
      b >>= mFormat[2].mQuant;
      a >>= mFormat[3].mQuant;
      
	   return 
		   (r << mFormat[0].mOfs) | 
		   (g << mFormat[1].mOfs) | 
		   (b << mFormat[2].mOfs) | 
		   (a << mFormat[3].mOfs);
   }

   uint pack(const BRGBAColor& c) const 
   { 
	   return pack(c[0], c[1], c[2], c[3]);
   }

   uint map(const BRGBAColor& c, bool ignorePal = false) const
   {
      if (mColorkeyed)
      {
         if (c.a == 0)
            return 0;
      }

      uint ret;

      if ((mpPal) && (!ignorePal))
	   {
         ret = mpPal->bestMatch(c);
		   if (mFormat[3].mBits)
			   ret = pack(ret, ret, ret, c.a);
	   }
      else if ((mColorType == cColorTypePalettized) || (mColorType == cColorTypeGrayscale))
	   {
		   const uchar g = (uchar)c.getGrayscale();
		   ret = pack(g, g, g, c.a);
	   }
	   else
         ret = pack(c);
      
      return ret;
   }

   BRGBAColor unpack(uint t) const
   {
      return BRGBAColor( 
		   (((t & mFormat[0].mUpMask) >> mFormat[0].mUpOfs)) << mFormat[0].mUpQuant,
         (((t & mFormat[1].mUpMask) >> mFormat[1].mUpOfs)) << mFormat[1].mUpQuant,
         (((t & mFormat[2].mUpMask) >> mFormat[2].mUpOfs)) << mFormat[2].mUpQuant,
         (((t & mFormat[3].mUpMask) >> mFormat[3].mUpOfs)) << mFormat[3].mUpQuant
	   );
   }

   BRGBAColor unpack(uint t, const BRGBAColor* pPal) const
   {
      BDEBUG_ASSERT(mColorType == cColorTypePalettized);
	   BDEBUG_ASSERT(pPal);
      return pPal[((t & mFormat[0].mUpMask) >> mFormat[0].mUpOfs) << mFormat[0].mUpQuant];
   }

   void unpack(BRGBAColor* pDest, uint t) const
   {
      pDest->set( 
		   (((t & mFormat[0].mUpMask) >> mFormat[0].mUpOfs)) << mFormat[0].mUpQuant,
         (((t & mFormat[1].mUpMask) >> mFormat[1].mUpOfs)) << mFormat[1].mUpQuant,
         (((t & mFormat[2].mUpMask) >> mFormat[2].mUpOfs)) << mFormat[2].mUpQuant,
         (((t & mFormat[3].mUpMask) >> mFormat[3].mUpOfs)) << mFormat[3].mUpQuant
	   );
   }

   int unpackA(uint t) const
   {
      return ((t & mFormat[3].mUpMask) >> mFormat[3].mUpOfs) << mFormat[3].mUpQuant;
   }

   int unpackP(uint t) const
   {
      return (t & mFormat[0].mUpMask) >> mFormat[0].mUpOfs;
   }

   BRGBAColor unpackPrecise(uint t) const
   {
      BRGBAColor ret(0, 0, 0, 0);

	   for (int i = 0; i < cMaxComponents; i++)
	   {
		   if (mFormat[i].mUpBits)
		   {
			   const int j = (t & mFormat[i].mUpMask) >> mFormat[i].mUpOfs;
			   const int k = mFormat[i].mUpMask >> mFormat[i].mUpOfs;
			   ret[i] = (uchar)((((j << 8) - j) + (k >> 1)) / k);
		   }
	   }

      return ret;
   }

   BRGBAColor unpackPreciseNoRound(uint t) const
   {
      BRGBAColor ret(0, 0, 0, 0);

	   for (int i = 0; i < cMaxComponents; i++)
	   {
		   if (mFormat[i].mUpBits)
		   {
			   const int j = (t & mFormat[i].mUpMask) >> mFormat[i].mUpOfs;
			   const int k = mFormat[i].mUpMask >> mFormat[i].mUpOfs;
			   ret[i] = (uchar)(((j << 8) - j) / k);
		   }
	   }

      return ret;
   }

   BRGBAColor unmap(uint t, bool ignorePal = false) const
   {
      BRGBAColor ret;

      if (mColorType == cColorTypePalettized)
	   {
		   if ((mpPal) && (!ignorePal))
		   {
			   ret = (*mpPal)[unpackP(t)];
			   if (!mpPal->getHasAlpha())
				   ret.a = (uchar)mDefaultAlpha;
		   }
		   else
			   ret = BRGBAColor(unpackP(t), mDefaultAlpha);

		   if (mFormat[3].mMask) 
			   ret.a = (uchar)unpackA(t);
	   }
      else 
	   {
         ret = unpackPrecise(t);
      
	      if (mColorkeyed)
		      ret.a = ((t & ~mFormat[3].mMask) != 0) ? 255 : 0;
		   else
			   ret.a |= mDefaultAlpha;
	   }

      return ret;
   }

   BRGBAColor unmapNoRound(uint t, bool ignorePal = false) const
   {
      BRGBAColor ret;

      if (mColorType == cColorTypePalettized)
	   {
		   if ((mpPal) && (!ignorePal))
		   {
			   ret = (*mpPal)[unpackP(t)];
			   if (!mpPal->getHasAlpha())
				   ret.a = (uchar)mDefaultAlpha;
		   }
		   else
			   ret = BRGBAColor(unpackP(t), mDefaultAlpha);

		   if (mFormat[3].mMask) 
			   ret.a = (uchar)unpackA(t);
	   }
      else 
	   {
         ret = unpackPreciseNoRound(t);
      
	      if (mColorkeyed)
		      ret.a = ((t & ~mFormat[3].mMask) != 0) ? 255 : 0;
		   else
			   ret.a |= mDefaultAlpha;
	   }

      return ret;
   }

   class BTranslator
   {
	   friend struct BPixelReader<1>;
	   friend struct BPixelReader<2>;
	   friend struct BPixelReader<3>;
	   friend struct BPixelReader<4>;

	   const BPixelFormat& mSrc;
	   const BPixelFormat& mDst;
   	
	   uint mAlphaAnd, mAlphaXor;

	   // These tables map input bytes to output mBits.
	   uint mXLat0[BPalette::cMaxPalSize];
	   uint mXLat1[BPalette::cMaxPalSize];
	   uint mXLat2[BPalette::cMaxPalSize];
	   uint mXLat3[BPalette::cMaxPalSize];

	   uint mAlphaMask;
	   int mDefaultAlpha;
   	
	   bool mOptimized : 1;
	   bool mIgnoreSrcPal : 1;
	   bool mIgnoreDstPal : 1;
	   bool mEqual : 1;

	   BRGBAColor alphaModulate(const BRGBAColor& r, uchar alphaAnd, uchar alphaXor)
	   {
		   BRGBAColor ret(r);
		   ret.a = (ret.a & alphaAnd) ^ alphaXor;
		   return ret;
	   }

   public:
	   BTranslator(
		   const BPixelFormat& src, const BPixelFormat& dst,
		   uchar alphaAnd = 0xFF, uchar alphaXor = 0,
		   int defaultAlpha = 0xFF,
		   bool ignoreSrcPal = false, bool ignoreDstPal = false);

	   void translate(void* Pd, const void* Ps, int numPixels);

	   const BPixelFormat& srcFormat(void) const 
	   { 
		   return mSrc; 
	   }

	   const BPixelFormat& dstFormat(void) const 
	   { 
		   return mDst; 
	   }
   };

   static const BPixelFormat& defaultFormat(void);  
   
protected:
   uint mBitsPerPixel;
   uint mBytesPerPixel;
   uint mNumComponents;
   uint mDefaultAlpha;

   uint mNumSigComponents;
   uint mSigComponentMap[cMaxComponents];

   BColorType mColorType;

   BPalette* mpPal; 

   // compiler should pad to 16 bytes
   struct
   {
      uint mMask;
      char mBits;
      char mOfs;
      char mQuant;
      uint mUpMask;
      char mUpBits;
      char mUpOfs;
      char mUpQuant;
   } mFormat[cMaxComponents];

   bool mColorkeyed : 1;  
   bool mBigEndian : 1;

   void clear(void)
   {
      Utils::ClearObj(*this);
   }   
};
//------------------------------------------------------------------------------
// Byte order: D3DFORMAT-style highest byte to lowest byte. All little endian.
extern const BPixelFormat G_r1g1b1_format;
extern const BPixelFormat G_r2g2b2_format;
extern const BPixelFormat G_r3g3b3_format;
extern const BPixelFormat G_r4g4b4_format;
extern const BPixelFormat G_a4r4g4b4_format;
extern const BPixelFormat G_r5g6b5_format;
extern const BPixelFormat G_r5g5b5_format;
extern const BPixelFormat G_a1r5g5b5_format;
extern const BPixelFormat G_rgb_format;
extern const BPixelFormat G_bgr_format;
extern const BPixelFormat G_abgr_format;
extern const BPixelFormat G_rgba_format;
extern const BPixelFormat G_xbgr_format;
extern const BPixelFormat G_xrgb_format;
extern const BPixelFormat G_argb_format;
extern const BPixelFormat G_grayscale_format;
extern const BPixelFormat G_grayscale4_format;
extern const BPixelFormat G_grayscale2_format;
extern const BPixelFormat G_grayscale1_format;
extern const BPixelFormat G_palettized_format;

extern const BPixelFormat G_grayscale_alpha_format;
extern const BPixelFormat G_palettized_alpha_format;
extern const BPixelFormat G_alpha_format;
//------------------------------------------------------------------------------
inline const BPixelFormat& BPixelFormat::defaultFormat(void) { return G_abgr_format; }
//------------------------------------------------------------------------------

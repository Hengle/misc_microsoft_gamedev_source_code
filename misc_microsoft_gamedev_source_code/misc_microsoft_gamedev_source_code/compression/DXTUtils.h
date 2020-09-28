// File: DXTUtils.h
#pragma once

#include "RGBAImage.h"
#include "math\generalVector.h"
#include "containers\staticArray.h"

enum BDXTFormat
{
   cDXTInvalid = 0,
   
   cDXT1,
   cDXT1A,
   cDXT3,
   cDXT5,
   cDXT5A,
   cDXN,

   cForceDWORD = 0xFFFFFFFF
};

enum eDXTQuality
{
   cDXTQualityLowest = -1,
   cDXTQualityNormal = 0,
   cDXTQualityBest = 1,
   
   cDXTTotalQuality
};

const uint cDXTBlockSizeBits        = 2U;
const uint cDXTBlockSize            = 1 << cDXTBlockSizeBits;
const uint cDXTBlockSizeMask        = cDXTBlockSize - 1;

const uint cDXTColorSelectorBits    = 2U;
const uint cDXTColorSelectorValues  = 1U << cDXTColorSelectorBits;
const uint cDXTColorSelectorMask    = cDXTColorSelectorValues - 1;

const uint cDXT5AlphaSelectorBits    = 3U;
const uint cDXT5AlphaSelectorValues  = 1U << cDXT5AlphaSelectorBits;

const uint cDXT1BlockBytes          = 8U;
const uint cDXT35NBlockBytes        = 16U;

class BDXTUtils
{
public:
   struct BDXT1Cell
   {
      BYTE     mPackedColor0[2];
      BYTE     mPackedColor1[2];
      BYTE     mPixels0;
      BYTE     mPixels1;
      BYTE     mPixels2;
      BYTE     mPixels3;

      uchar getSelector(uint x, uint y) const;
      void setSelector(uint x, uint y, uint selector);

      WORD getColor0(void) const { return mPackedColor0[0] | (mPackedColor0[1] << 8); }
      void setColor0(WORD color0) { mPackedColor0[0] = static_cast<BYTE>(color0); mPackedColor0[1] = static_cast<BYTE>(color0 >> 8); }

      WORD getColor1(void) const { return mPackedColor1[0] | (mPackedColor1[1] << 8); }
      void setColor1(WORD color1) { mPackedColor1[0] = static_cast<BYTE>(color1); mPackedColor1[1] = static_cast<BYTE>(color1 >> 8); }
      
      WORD getColorWord(uint i) const { BDEBUG_ASSERT(i < 2); return Utils::CreateWORD(mPackedColor0[i*2+0], mPackedColor0[i*2+1]); }
      WORD getSelectorWord(uint i) { BDEBUG_ASSERT(i < 2); return Utils::CreateWORD((&mPixels0)[i*2+0], (&mPixels0)[i*2+1]); }
   };

   struct BDXT3Cell
   {
      BYTE mAlpha[8];
      BDXT1Cell mRGB;

      uchar getAlpha(uint x, uint y, bool scaled) const;
      void setAlpha(uint x, uint y, int alpha, bool scaled);
   };

   struct BDXT5Cell
   {
      BYTE mAlpha[2];
      BYTE mSelectors[6];
      
      void setAlpha0(BYTE a0) { mAlpha[0] = a0; }
      void setAlpha1(BYTE a1) { mAlpha[1] = a1; }
      
      BYTE getAlpha0(void) const { return mAlpha[0]; }
      BYTE getAlpha1(void) const { return mAlpha[1]; }
            
      uchar getSelector(uint x, uint y) const;
      void setSelector(uint x, uint y, uint alpha);
      
      WORD getAlphaWord(void) const { return Utils::CreateWORD(mAlpha[0], mAlpha[1]); }
      WORD getSelectorWord(uint i) { BDEBUG_ASSERT(i < 3); return Utils::CreateWORD(mSelectors[i*2+0], mSelectors[i*2+1]); }
   };
         
   static bool getSizeOfDXTData(DWORD& size, BDXTFormat dxtFormat, uint width, uint height);
   static uint getSizeOfDXTBlock(BDXTFormat dxtFormat);

   struct BBlockPixel
   {
      BRGBAColor mColor;
      WORD mPackedColor;

      BBlockPixel() { }
      BBlockPixel(const BRGBAColor color, WORD packedColor) : mColor(color), mPackedColor(packedColor) { }

      bool operator< (const BBlockPixel& b) const
      {
         return mPackedColor < b.mPackedColor;
      }

      bool operator== (const BBlockPixel& b) const
      {
         return mPackedColor == b.mPackedColor;
      }
   };
   
   static bool determineBlockColorsFast(
      const BRGBAImage& image, uint cx, uint cy,
      BDynamicArray<BBlockPixel>& blockColors,
      WORD& packedHighColor,
      WORD& packedLowColor,
      uchar* pBestSelectors, 
      uint& bestType,
      bool perceptual, 
      bool codeTransparentPixels,
      BVec3* pPrincipalAxis = NULL,
      bool useTransparentBlocks = true,
      bool useBlockTruncation = false);

   // true if all transparent
   static bool packDXT1Block(
      const BRGBAImage& image, uint cx, uint cy,
      WORD& packedHighColor,
      WORD& packedLowColor,
      uchar* pBestSelectors, 
      uint& bestType,
      bool perceptual, 
      bool codeTransparentPixels,
      BVec3* pPrincipalAxis = NULL,
      bool useTransparentBlocks = true,
      eDXTQuality quality = cDXTQualityNormal,
      uint* pBestVariance = NULL);
      
   static void packDXT5Block(
      const BRGBAImage& image, uint cx, uint cy, uint compIndex,
      BYTE& packedHighAlpha,
      BYTE& packedLowAlpha,
      uchar* pBestSelectors, 
      uint& bestBlockType,
      bool useBothBlockTypes = true,
      bool favorLargerAlpha = false,
      BDXTUtils::BDXT5Cell* pCell = NULL);

   // Low-level helper classes      
      
   struct BUniqueColor
   {
      BUniqueColor() { }
      BUniqueColor(const BRGBAColor& color, uint weight) : mColor(color), mWeight(weight) { }

      BRGBAColor mColor;
      uint mWeight;
   };

   typedef BDynamicArray<BUniqueColor> BUniqueColorArray;

   class BColorCellEvaluator
   {
   public:
      BColorCellEvaluator(
         bool codeTransparentPixels, bool perceptual,
         uint firstBlockType, uint lastBlockType,
         uint& bestVariance,
         WORD& packedLowColor,
         WORD& packedHighColor,
         uint& bestType,
         uchar* pBestSelectors,
         const BUniqueColorArray& uniqueColors) :
            mCodeTransparentPixels(codeTransparentPixels), 
            mPerceptual(perceptual),
            mFirstBlockType(firstBlockType), 
            mLastBlockType(lastBlockType),
            mBestVariance(bestVariance),
            mPackedLowColor(packedLowColor),
            mPackedHighColor(packedHighColor),
            mBestType(bestType),
            mpBestSelectors(pBestSelectors),
            mUniqueColors(uniqueColors)
      {
         mTempSelectors.resize(uniqueColors.getSize());
      }   

      bool createSelectors(WORD packedL, WORD packedH)
      {
         bool foundBetter = false;

         int r[cDXTColorSelectorValues], g[cDXTColorSelectorValues], b[cDXTColorSelectorValues];

         BColorUtils::unpackColor(packedL, r[0], g[0], b[0], true);
         BColorUtils::unpackColor(packedH, r[3], g[3], b[3], true);

         for (uint type = mFirstBlockType; type <= mLastBlockType; type++)
         {
            uint numCandidateColors;
            if (type == 0)
            {
               r[1] = (r[0] * 2 + r[3]) / 3;
               g[1] = (g[0] * 2 + g[3]) / 3;
               b[1] = (b[0] * 2 + b[3]) / 3;

               r[2] = (r[3] * 2 + r[0]) / 3;
               g[2] = (g[3] * 2 + g[0]) / 3;
               b[2] = (b[3] * 2 + b[0]) / 3;

               numCandidateColors = 4;
            }
            else
            {
               r[1] = (r[0] + r[3]) / 2;
               g[1] = (g[0] + g[3]) / 2;
               b[1] = (b[0] + b[3]) / 2;

               r[2] = r[3];
               g[2] = g[3];
               b[2] = b[3];

               numCandidateColors = 3;
            }                           

            uint totalVariance = 0;
            mTempSelectors.setAll(0);
            
            for (uint c = 0; c < mUniqueColors.size(); c++)
            {
               const BRGBAColor& pixelColor = mUniqueColors[c].mColor;
               if ((mCodeTransparentPixels) && (pixelColor.a < 128))
               {
                  // Can't use block type 0 if there are transparent pixels. (Shouldn't happen in practice - this block type shouldn't be included.)
                  if (type == 0)
                     totalVariance += 9999999;
                  mTempSelectors[c] = static_cast<uchar>(3);
                  continue;
               }

               const int cr = pixelColor.r;
               const int cg = pixelColor.g;
               const int cb = pixelColor.b;

               int bestDist = INT_MAX;
               int bestI = 0;
               for (uint i = 0; i < numCandidateColors; i++)
               {
                  int trialDist;

                  if (mPerceptual)
                     trialDist = BColorUtils::colorDistanceWeighted(cr, cg, cb, r[i], g[i], b[i]);
                  else
                     trialDist = BColorUtils::colorDistanceElucidian(cr, cg, cb, r[i], g[i], b[i]);

                  if (trialDist < bestDist)
                  {
                     bestDist = trialDist;
                     bestI = i;
                  }
               }

               totalVariance += bestDist * mUniqueColors[c].mWeight;
               if (totalVariance > mBestVariance)
                  goto earlyOut2;
               mTempSelectors[c] = static_cast<uchar>(bestI);
            }

            if (totalVariance < mBestVariance)                     
            {
               mBestVariance = totalVariance;
               mPackedHighColor = packedH;
               mPackedLowColor = packedL;
               mBestType = type;
               if (mpBestSelectors)
                  memcpy(mpBestSelectors, mTempSelectors.getPtr(), mTempSelectors.getSize());

               foundBetter = true;
            }
earlyOut2: ;
         }  // type

         return foundBetter;
      }   

   private:
      bool mCodeTransparentPixels;
      bool mPerceptual;
      uint mFirstBlockType;
      uint mLastBlockType;
      uint& mBestVariance;
      WORD& mPackedLowColor;
      WORD& mPackedHighColor;
      uint& mBestType;
      uchar* mpBestSelectors;
      const BUniqueColorArray& mUniqueColors;
      BDynamicArray<uchar> mTempSelectors;
   };      
      
};


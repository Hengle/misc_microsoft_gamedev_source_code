//============================================================================
//
// File: DXTUtils.cpp
// Copyright (c) 2005-2006, Ensemble Studios
//
//============================================================================
#include "compression.h"
#include "xcore.h"
#include "containers\dynamicArray2D.h"
#include "colorUtils.h"
#include "RGBAImage.h"
#include "math\generalVector.h"
#include "math\generalMatrix.h"
#include "math\generalVector.h"
#include "DXTUtils.h"
#include "math\LinearAlgebra.h"
#include "writetga.h"

//============================================================================
// BDXTUtils::BDXT1Cell::getSelector
//============================================================================
uchar BDXTUtils::BDXT1Cell::getSelector(uint x, uint y) const
{
   BDEBUG_ASSERT((x < cDXTBlockSize) && (y < cDXTBlockSize));
   return static_cast<uchar>(((&mPixels0)[y] >> (x * cDXTColorSelectorBits)) & cDXTColorSelectorMask);
}

//============================================================================
// BDXTUtils::BDXT1Cell::setSelector
//============================================================================
void BDXTUtils::BDXT1Cell::setSelector(uint x, uint y, uint selector)
{
   BDEBUG_ASSERT((x < cDXTBlockSize) && (y < cDXTBlockSize) && (selector < cDXTColorSelectorValues));
   (&mPixels0)[y] &= ~(cDXTColorSelectorMask << (x * cDXTColorSelectorBits));
   (&mPixels0)[y] |= (selector << (x * cDXTColorSelectorBits));
}

//============================================================================
// BDXTUtils::BDXT3Cell::getAlpha
//============================================================================
uchar BDXTUtils::BDXT3Cell::getAlpha(uint x, uint y, bool scaled) const
{
   BDEBUG_ASSERT((x < cDXTBlockSize) && (y < cDXTBlockSize));
   uchar alpha = static_cast<uchar>((mAlpha[y * 2 + (x >> 1)] >> ((x & 1) * 4)) & 0xF);

   if (scaled)
      alpha = (alpha * 255 + 7) / 15;
   return alpha;
}

//============================================================================
// BDXTUtils::BDXT3Cell::setAlpha
//============================================================================
void BDXTUtils::BDXT3Cell::setAlpha(uint x, uint y, int alpha, bool scaled)
{  
   BDEBUG_ASSERT((x < cDXTBlockSize) && (y < cDXTBlockSize));
   if (scaled)      
   {
      if (alpha < 0) alpha = 0; else if (alpha > 255) alpha = 255;
      alpha = (alpha * 15 + 127) / 255;
   }
   else
   {
      if (alpha < 0) alpha = 0; else if (alpha > 15) alpha = 15;
   }

   mAlpha[y * 2 + (x >> 1)] &= ~(0xF << ((x & 1) * 4));
   mAlpha[y * 2 + (x >> 1)] |= (alpha << ((x & 1) * 4));
}

//============================================================================
// BDXTUtils::BDXT5Cell::getSelector
//============================================================================
uchar BDXTUtils::BDXT5Cell::getSelector(uint x, uint y) const
{
   const uint bitOfs = (y * 4 + x) * 3;
   const uint byteOfs = bitOfs >> 3;

   uint word = mSelectors[byteOfs];
   if (byteOfs != 5)
      word |= (mSelectors[byteOfs + 1] << 8);

   return static_cast<uchar>((word >> (bitOfs & 7)) & 7);
}

//============================================================================
// BDXTUtils::BDXT5Cell::setSelector
//============================================================================
void BDXTUtils::BDXT5Cell::setSelector(uint x, uint y, uint alpha)
{
   const uint bitOfs = (y * 4 + x) * 3;
   const uint byteOfs = bitOfs >> 3;

   uint word = mSelectors[byteOfs];
   if (byteOfs != 5)
      word |= (mSelectors[byteOfs + 1] << 8);

   word &= ~(7 << (bitOfs & 7));
   word |= (alpha << (bitOfs & 7));

   mSelectors[byteOfs] = static_cast<uchar>(word);
   if (byteOfs != 5)
      mSelectors[byteOfs + 1] = static_cast<uchar>(word >> 8);
}
 
//============================================================================
// BDXTUtils::getSizeOfDXTBlock
//============================================================================
uint BDXTUtils::getSizeOfDXTBlock(BDXTFormat dxtFormat)
{
   uint bytesPerBlock = 0;

   if ((dxtFormat == cDXT5A) || (dxtFormat == cDXT1))
      bytesPerBlock = cDXT1BlockBytes;


   if ((dxtFormat == cDXT3) || (dxtFormat == cDXT5) || (dxtFormat == cDXN))
      bytesPerBlock = cDXT35NBlockBytes;

   return bytesPerBlock;
}

//============================================================================
// BDXTUtils::getSizeOfDXTData
//============================================================================
bool BDXTUtils::getSizeOfDXTData(DWORD& size, BDXTFormat dxtFormat, uint width, uint height)
{
   size = 0;

   if ((!width) || (!height))
      return false;

   width = (width + 3) & ~3;
   height = (height + 3) & ~3;

   uint bytesPerBlock = 8;
   if ((dxtFormat == cDXT3) || (dxtFormat == cDXT5) || (dxtFormat == cDXN))
      bytesPerBlock = 16;

   const uint cellsX = width >> 2;
   const uint cellsY = height >> 2;

   size = cellsX * cellsY * bytesPerBlock;

   return true;
}

//============================================================================
// BDXTUtils::determineBlockColorsFast
//============================================================================
bool BDXTUtils::determineBlockColorsFast(
   const BRGBAImage& image, uint cx, uint cy,
   BDynamicArray<BBlockPixel>& blockColors,
   WORD& packedHighColor,
   WORD& packedLowColor,
   uchar* pBestSelectors, 
   uint& bestType,
   bool perceptual, 
   bool codeTransparentPixels,
   BVec3* pPrincipalAxis,
   bool useTransparentBlocks,
   bool useBlockTruncation)
{   
   BDynamicArray2D<BRGBAColor> cellColors(cDXTBlockSize, cDXTBlockSize);
   for (int uy = 0; uy < cDXTBlockSize; uy++)
      for (int ux = 0; ux < cDXTBlockSize; ux++)   
         cellColors(ux, uy) = image(cx * cDXTBlockSize + ux, cy * cDXTBlockSize + uy);

   if (pPrincipalAxis)
      pPrincipalAxis->setZero();
   bestType = 0;
   if (pBestSelectors) 
      memset(pBestSelectors, 0, cDXTBlockSize * cDXTBlockSize);
   packedHighColor = 0;
   packedLowColor = 0;

   uint firstBlockType = 0;
   uint lastBlockType = 1;

   if ((!useTransparentBlocks) && (!codeTransparentPixels))
      lastBlockType = 0;

   blockColors.resize(0);

   BRGBAColor16 meanColor16(0);

   for (int uy = 0; uy < cDXTBlockSize; uy++)
   {
      for (int ux = 0; ux < cDXTBlockSize; ux++)   
      {
         const BRGBAColor& pixelColor = cellColors(ux, uy);

         meanColor16 += pixelColor;

         if ((codeTransparentPixels) && (pixelColor.a < 128))
         {
            firstBlockType = 1;
         }
         else
         {
            const WORD packedColor = BColorUtils::packColor(pixelColor, true);

            uint i;
            for (i = 0; i < blockColors.size(); i++)
               if (blockColors[i].mPackedColor == packedColor)
                  break;

            if (i == blockColors.size())
               blockColors.pushBack(BBlockPixel(pixelColor, packedColor));
         }
      }
   }

   meanColor16 += BRGBAColor16(8, 8, 8, 0);
   meanColor16 /= 16;
   meanColor16.clamp(0, 255);
   const BRGBAColor meanColor(meanColor16.getRGBAColor());

   const uint numUniqueBlockColors = blockColors.size();

   if (!numUniqueBlockColors)
      return true;

   uint bestVariance = UINT_MAX;            

   if (useBlockTruncation)
   {
      BRGBAColor16 meanYCbCr;
      BColorUtils::RGBToYCbCr(meanColor, meanYCbCr);

      BRGBAColor16 lowColor(0);
      uint numLow = 0;
      BRGBAColor16 highColor(0);
      uint numHigh = 0;

      for (int uy = 0; uy < cDXTBlockSize; uy++)
      {
         for (int ux = 0; ux < cDXTBlockSize; ux++)   
         {
            const BRGBAColor& pixelColor = cellColors(ux, uy);
            if ((codeTransparentPixels) && (pixelColor.a < 128))
               continue;

            BRGBAColor16 pixelColorYCbCr;
            BColorUtils::RGBToYCbCr(pixelColor, pixelColorYCbCr);

            if (pixelColorYCbCr.r <= meanYCbCr.r)
            {
               lowColor += pixelColor;
               numLow++;
            }
            else
            {
               highColor += pixelColor;
               numHigh++;

               if (pBestSelectors)
                  pBestSelectors[ux * cDXTBlockSize + uy] = 3;
            }
         }
      }

      if (numLow)
      {
         lowColor += BRGBAColor16(numLow / 2);
         lowColor /= numLow;
         lowColor.clamp(0, 255);
      }

      if (numHigh)
      {
         highColor += BRGBAColor16(numHigh / 2);
         highColor /= numHigh;
         highColor.clamp(0, 255);
      }
      else
      {
         highColor = lowColor;
      }

      packedHighColor = BColorUtils::packColor(highColor.getRGBAColor(), true);
      packedLowColor = BColorUtils::packColor(lowColor.getRGBAColor(), true);
   }
   else 
   {
      uint bestU = 0, bestT = 0;
      for (uint u = 0; u < numUniqueBlockColors; u++)
      {
         int r[cDXTColorSelectorValues], g[cDXTColorSelectorValues], b[cDXTColorSelectorValues];

         BColorUtils::unpackColor(blockColors[u].mPackedColor, r[3], g[3], b[3], true);

         uint firstT = u + 1;
         if (numUniqueBlockColors == 1)
            firstT = 0;

         for (uint t = firstT; t < numUniqueBlockColors; t++)
         {
            BColorUtils::unpackColor(blockColors[t].mPackedColor, r[0], g[0], b[0], true);

            for (uint type = firstBlockType; type <= lastBlockType; type++)
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
               uchar selectors[cDXTBlockSize][cDXTBlockSize];

               for (uint sy = 0; sy < cDXTBlockSize; sy++)
               {
                  for (uint sx = 0; sx < cDXTBlockSize; sx++)
                  {
                     const BRGBAColor& pixelColor = cellColors(sx, sy);
                     if ((codeTransparentPixels) && (pixelColor.a < 128))
                        continue;

                     const int cr = pixelColor.r;
                     const int cg = pixelColor.g;
                     const int cb = pixelColor.b;

                     int bestDist = INT_MAX;
                     int bestI = 0;
                     for (uint i = 0; i < numCandidateColors; i++)
                     {
                        int trialDist;
                        if (perceptual)
                           trialDist = BColorUtils::colorDistanceWeighted(cr, cg, cb, r[i], g[i], b[i]);
                        else
                           trialDist = BColorUtils::colorDistanceElucidian(cr, cg, cb, r[i], g[i], b[i]);

                        if (trialDist < bestDist)
                        {
                           bestDist = trialDist;
                           bestI = i;
                        }
                     }

                     totalVariance += bestDist;
                     selectors[sx][sy] = static_cast<uchar>(bestI);
                     if (totalVariance > bestVariance)
                        goto earlyOut;
                  }                  
               }

               if (totalVariance < bestVariance)
               {
                  bestVariance = totalVariance;

                  bestU = u;
                  bestT = t;
                  bestType = type;
                  if (pBestSelectors)
                     memcpy(pBestSelectors, selectors, sizeof(selectors));
               }     
earlyOut: ;
            } // type                        
         } // t
      }  // u

      packedHighColor = blockColors[bestU].mPackedColor;
      packedLowColor = blockColors[bestT].mPackedColor;

      if (pPrincipalAxis)
      {
         *pPrincipalAxis = BVec3(
            static_cast<float>(blockColors[bestU].mColor.r - blockColors[bestT].mColor.r), 
            static_cast<float>(blockColors[bestU].mColor.g - blockColors[bestT].mColor.g), 
            static_cast<float>(blockColors[bestU].mColor.b - blockColors[bestT].mColor.b));

         pPrincipalAxis->tryNormalize(BVec3(1.0f, 0.0f, 0.0f));
      }            
   }

   return false;
}

#define USE_COVARIANCE 

//============================================================================
// BDXTUtils::packDXT1Block
//============================================================================
bool BDXTUtils::packDXT1Block(
   const BRGBAImage& image, uint cx, uint cy,
   WORD& packedHighColor,
   WORD& packedLowColor,
   uchar* pBestSelectors, 
   uint& bestType,
   bool perceptual, 
   bool codeTransparentPixels,
   BVec3* pPrincipalAxis,
   bool useTransparentBlocks,
   eDXTQuality quality,
   uint* pBestVariance)
{  
   BDynamicArray2D<BRGBAColor> cellColors(cDXTBlockSize, cDXTBlockSize);
   for (int uy = 0; uy < cDXTBlockSize; uy++)
      for (int ux = 0; ux < cDXTBlockSize; ux++)   
         cellColors(ux, uy) = image(cx * cDXTBlockSize + ux, cy * cDXTBlockSize + uy);

   if (pPrincipalAxis)
      pPrincipalAxis->setZero();
   bestType = 0;
   if (pBestSelectors) 
      memset(pBestSelectors, 0, cDXTBlockSize * cDXTBlockSize);
   packedHighColor = 0;
   packedLowColor = 0;
   if (pBestVariance)
      *pBestVariance = 0;

   uint firstBlockType = 0;
   uint lastBlockType = 1;

   if ((!useTransparentBlocks) && (!codeTransparentPixels))
      lastBlockType = 0;
      
   BUniqueColorArray uniqueColors;
   uniqueColors.reserve(16);
   
   uchar uniqueColorIndex[cDXTBlockSize][cDXTBlockSize];
   
   for (uint sy = 0; sy < cDXTBlockSize; sy++)
   {
      for (uint sx = 0; sx < cDXTBlockSize; sx++)
      {
         const BRGBAColor& pixelColor = cellColors(sx, sy);
         if ((codeTransparentPixels) && (pixelColor.a < 128))
         {
            firstBlockType = 1;
            uniqueColorIndex[sx][sy] = 0;
            continue;
         }

         uint i;
         for (i = 0; i < uniqueColors.size(); i++)
            if (uniqueColors[i].mColor.compareRGB(pixelColor))
               break;
         if (i == uniqueColors.size())            
            uniqueColors.pushBack(BUniqueColor(pixelColor, 1));
         else
            uniqueColors[i].mWeight++;

         uniqueColorIndex[sx][sy] = static_cast<uchar>(i);
      } // sx
   } // sy
   
   if (0 == uniqueColors.size())
      return true;
   else if (1 == uniqueColors.size())
   {
      packedLowColor = BColorUtils::packColor(uniqueColors[0].mColor, true);
      packedHighColor = packedLowColor;   

      BRGBAColor unpackedColor;
      BColorUtils::unpackColor(packedLowColor, unpackedColor, true);

      // Return now if the packed color is a perfect match for the input color. The caller will set the selectors.
      if (unpackedColor.compareRGB(uniqueColors[0].mColor))
      {
         if (firstBlockType == 1)
            bestType = 1;
         return false;
      }
   }

   uint bestVariance = UINT_MAX;            
      
   BVec3 mean(0.0f);
   BVec3 unweightedMean(0.0f);
   BVec3 axis;
   
   const BVec3 PCAWeightings[] = 
   {
      //BVec3(.13f, .85f, .02f),
      BVec3(.212671f, .71516f, .072169f),
      BVec3(.21f, .6f, .19f),      
   };
   
   const uint numPCAPasses = perceptual ? (sizeof(PCAWeightings)/sizeof(PCAWeightings[0])) : 1;
      
   for (uint pass = 0; pass < numPCAPasses; pass++)
   {
      const BVec3& PCAWeighting = PCAWeightings[pass];
   
      const BVec3 InvPCAWeighting(1.0f / PCAWeighting[0], 1.0f / PCAWeighting[1], 1.0f / PCAWeighting[2]);
                 
      mean.setZero();
      unweightedMean.setZero();
      
      BVec3 c[cDXTBlockSize][cDXTBlockSize];
      Utils::ClearObj(c);

#ifndef USE_COVARIANCE      
      BVec3 sampleVecs[16];
      Utils::ClearObj(sampleVecs);
      uint numSampleVecs = 0;
#endif      
      
      float totalPixels = 0.0f;
      for (uint sy = 0; sy < cDXTBlockSize; sy++)
      {
         for (uint sx = 0; sx < cDXTBlockSize; sx++)
         {
            const BRGBAColor& pixelColor = cellColors(sx, sy);
            if ((codeTransparentPixels) && (pixelColor.a < 128))
               continue;
            
            BVec3 v(pixelColor.r / 255.0f, pixelColor.g / 255.0f, pixelColor.b / 255.0f);

            unweightedMean += v;

            BVec3 vw(v);
            if (perceptual)
               vw = BVec3::multiply(v, PCAWeighting);
                        
            c[sx][sy] = vw;

            mean += vw;
            
            totalPixels++;
         }
      }
      
      mean /= totalPixels;
      unweightedMean /= totalPixels;

#ifdef USE_COVARIANCE
      BVec3 a(0.0f);
      float x = 0.0f, y = 0.0f, z = 0.0f;
#endif      

      for (uint sy = 0; sy < cDXTBlockSize; sy++)
      {
         for (uint sx = 0; sx < cDXTBlockSize; sx++)
         {
            const BRGBAColor& pixelColor = cellColors(sx, sy);
            if ((codeTransparentPixels) && (pixelColor.a < 128))
               continue;

            const BVec3 v(c[sx][sy] - mean);

#ifndef USE_COVARIANCE
            sampleVecs[numSampleVecs] = v;
            numSampleVecs++;
#else
            a += BVec3::multiply(v, v);
            x += v[0] * v[1];
            y += v[0] * v[2];
            z += v[1] * v[2];
#endif            
         }
      }
      
#ifdef USE_COVARIANCE
      a /= totalPixels;
      x /= totalPixels;
      y /= totalPixels;
      z /= totalPixels;
      
      LinearAlgebra::Matrix3D covariance;   

      covariance(0,0) = a[0];
      covariance(1,1) = a[1];
      covariance(2,2) = a[2];
      covariance(0,1) = x; covariance(1,0) = x;
      covariance(0,2) = y; covariance(2,0) = y;
      covariance(1,2) = z; covariance(2,1) = z;

      float lambda[3];
      LinearAlgebra::Matrix3D rm;            
      LinearAlgebra::CalculateEigensystem(covariance, lambda, rm);

      lambda[0] = fabs(lambda[0]);
      lambda[1] = fabs(lambda[1]);
      lambda[2] = fabs(lambda[2]);

      int majorAxis = 0;
      if ((lambda[1] > lambda[0]) && (lambda[1] > lambda[2]))
         majorAxis = 1;
      else if ((lambda[2] > lambda[0]) && (lambda[2] > lambda[1]))
         majorAxis = 2;

      axis = BVec3(rm(0, majorAxis), rm(1, majorAxis), rm(2, majorAxis));
      axis.normalize();
#else
      // Covariance-free PCA
      BVec3 v2(0.0f);
      for (uint i = 0; i < numSampleVecs; i++)
      {
         BVec3 a, b, c;
         a = sampleVecs[i] * sampleVecs[i][0];
         b = sampleVecs[i] * sampleVecs[i][1];
         c = sampleVecs[i] * sampleVecs[i][2];
                  
         BVec3 nV((i == 0) ? sampleVecs[0] : v2);
         nV.tryNormalize();
         
         v2[0] += a * nV;
         v2[1] += b * nV;
         v2[2] += c * nV;
      }
      
      v2.tryNormalize();
      v2 = -v2;
      axis = v2;
#endif  // #if experimental

      if (perceptual)
      {                        
         axis = BVec3::multiply(axis, InvPCAWeighting);
         axis.normalize();    
      
         if (fabs(axis[2]) < .8f)
            break;
      }
   }     
      
   if (pPrincipalAxis)
      *pPrincipalAxis = axis;

   uchar bestSelectors[16];
   Utils::ClearObj(bestSelectors);
   
   BColorCellEvaluator evaluator(
      codeTransparentPixels, perceptual,
      firstBlockType, lastBlockType,
      bestVariance,
      packedLowColor,
      packedHighColor,
      bestType,
      bestSelectors,
      uniqueColors);

   BStaticArray<DWORD, 512> colorsToTry;

   float l = 1e+30f;
   float h = -1e+30f;

   for (uint sy = 0; sy < cDXTBlockSize; sy++)
   {
      for (uint sx = 0; sx < cDXTBlockSize; sx++)
      {
         const BRGBAColor& pixelColor = cellColors(sx, sy);
         if ((codeTransparentPixels) && (pixelColor.a < 128))
            continue;

         BVec3 v(pixelColor.r / 255.0f, pixelColor.g / 255.0f, pixelColor.b / 255.0f);
         v -= unweightedMean;

         float d = v * axis;
         if (d < l) l = d;
         if (d > h) h = d;
      }
   }

   //uint maxOuterPasses = 1;


   BVec3 l0(axis * l + unweightedMean);
   BVec3 h0(axis * h + unweightedMean);

   const uint numPasses = (quality >= cDXTQualityBest) ? 4 : 1;
   
   for (uint passIndex = 0; passIndex < numPasses; passIndex++)
   {
      const uint prevVariance = bestVariance;
      
      l0.clampComponents(0.0f, .999f);
      h0.clampComponents(0.0f, .999f);   
   
      BVec3 l1(l0);
      BVec3 h1(h0);

      BStaticArray<WORD, 32> lowColors;
      BStaticArray<WORD, 32> highColors;

      float v = BVec3(.015625f).len();

      uint searchDist;
      switch (quality)
      {
         case cDXTQualityLowest: 
            searchDist = 2; 
            break;
         case cDXTQualityBest: 
            searchDist = 8; 
            break;
         case cDXTQualityNormal: 
         default:
            searchDist = 5;
            break;
      }
      
      for (uint i = 0; i < searchDist; i++)
      {
         lowColors.pushBack(BColorUtils::packColor(BRGBAColor((int)floor(l0[0]*32.0f), (int)floor(l0[1]*64.0f), (int)floor(l0[2]*32.0f), 0), false));
         if (i)
            lowColors.pushBack(BColorUtils::packColor(BRGBAColor((int)floor(l1[0]*32.0f), (int)floor(l1[1]*64.0f), (int)floor(l1[2]*32.0f), 0), false));

         highColors.pushBack(BColorUtils::packColor(BRGBAColor((int)floor(h0[0]*32.0f), (int)floor(h0[1]*64.0f), (int)floor(h0[2]*32.0f), 0), false));
         if (i)
            highColors.pushBack(BColorUtils::packColor(BRGBAColor((int)floor(h1[0]*32.0f), (int)floor(h1[1]*64.0f), (int)floor(h1[2]*32.0f), 0), false));

         l0 -= axis * v;
         l1 += axis * v;

         h0 -= axis * v;
         h1 += axis * v;
      }

      for (uint i = 0; i < lowColors.size(); i++)
      {
         for (uint j = 0; j < highColors.size(); j++)
         {
            WORD l = lowColors[i];
            WORD h = highColors[j];
            if (l > h)
               std::swap(l, h);
            colorsToTry.pushBack(l | (h << 16));
         }  
      }      

      if (colorsToTry.size())
         std::sort(colorsToTry.begin(), colorsToTry.end());

//-- FIXING PREFIX BUG ID 6310
      const DWORD* pLast = std::unique(colorsToTry.begin(), colorsToTry.end());
//--
      uint numUnique = pLast - colorsToTry.begin();

      for (uint colorIndex = 0; colorIndex < numUnique; colorIndex++)
      {
         const WORD packedL = static_cast<WORD>(colorsToTry[colorIndex] & 0xFFFF);
         const WORD packedH = static_cast<WORD>(colorsToTry[colorIndex] >> 16);

         evaluator.createSelectors(packedL, packedH);
      } // colorIndex

      if (quality >= cDXTQualityNormal)
      {
         colorsToTry.clear();

         int r[2], g[2], b[2];
         BColorUtils::unpackColor(packedLowColor, r[0], g[0], b[0], false);
         BColorUtils::unpackColor(packedHighColor, r[1], g[1], b[1], false);

         for (uint i = 0; i < 27; i++)
         {
            const int iROfs = (i%3)-1;
            const int iGOfs = ((i/3)%3)-1;
            const int iBOfs = ((i/9)%3)-1;

            int rl = r[0]+iROfs;
            int gl = g[0]+iGOfs;
            int bl = b[0]+iBOfs;
            if ((rl<0) || (rl>31)|| (gl<0) || (gl>63) || (bl<0) || (bl>31)) continue;

            WORD l = BColorUtils::packColor(rl, gl, bl, false);
            WORD h = packedHighColor;
            if (l > h)
               std::swap(l, h);

            colorsToTry.pushBack(l | (h << 16));
         }

         for (uint j = 0; j < 27; j++)
         {
            const int jROfs = (j%3)-1;
            const int jGOfs = ((j/3)%3)-1;
            const int jBOfs = ((j/9)%3)-1;

            int rh = r[1]+jROfs;
            int gh = g[1]+jGOfs;
            int bh = b[1]+jBOfs;
            if ((rh<0) || (rh>31)|| (gh<0) || (gh>63) || (bh<0) || (bh>31)) continue;

            WORD l = packedLowColor;
            WORD h = BColorUtils::packColor(rh, gh, bh, false);
            if (l > h)
               std::swap(l, h);

            colorsToTry.pushBack(l | (h << 16));
         }

         if (colorsToTry.size())
            std::sort(colorsToTry.begin(), colorsToTry.end());

         pLast = std::unique(colorsToTry.begin(), colorsToTry.end());
         numUnique = pLast - colorsToTry.begin();

         for (uint colorIndex = 0; colorIndex < numUnique; colorIndex++)
         {
            const WORD packedL = static_cast<WORD>(colorsToTry[colorIndex] & 0xFFFF);
            const WORD packedH = static_cast<WORD>(colorsToTry[colorIndex] >> 16);

            evaluator.createSelectors(packedL, packedH);
         } // colorIndex
         
         // Are all the selectors the same?
         int prevSelector = bestSelectors[0];
         uint i;
         for (i = 1; i < uniqueColors.size(); i++)
            if (bestSelectors[i] != prevSelector)
               break;
         
         if (i == uniqueColors.size())
         {
            if (prevSelector == 0)
               packedHighColor = packedLowColor;
            else if (prevSelector == 3)
               packedLowColor = packedHighColor;
         }         
      }
      
      BRGBAColor l;
      BColorUtils::unpackColor(packedLowColor, l, true);
      l0.set(l.r/255.0f, l.g/255.0f, l.b/255.0f, 0);
      
      BRGBAColor h;
      BColorUtils::unpackColor(packedHighColor, h, true);
      h0.set(h.r/255.0f, h.g/255.0f, h.b/255.0f, 0);
      
      if (bestVariance == prevVariance)
         break;
   }  // passsIndex

   if (pBestSelectors)
   {
      for (uint sy = 0; sy < 4; sy++)
         for (uint sx = 0; sx < 4; sx++)
         {
            const BRGBAColor& pixelColor = cellColors(sx, sy);
            if ((codeTransparentPixels) && (pixelColor.a < 128))
               pBestSelectors[sx*4+sy] = 3;
            else
               pBestSelectors[sx*4+sy] = bestSelectors[uniqueColorIndex[sx][sy]];
         }
   }

   if (pBestVariance)
      *pBestVariance = bestVariance;
      
   return false;
}

//============================================================================
// BDXTUtils::packDXT5Block
//============================================================================
void BDXTUtils::packDXT5Block(
   const BRGBAImage& image, uint cx, uint cy, uint compIndex,
   BYTE& highA,
   BYTE& lowA,
   uchar* pBestSelectors, 
   uint& bestBlockType,
   bool useBothBlockTypes,
   bool favorLargerAlpha,
   BDXTUtils::BDXT5Cell* pCell)
{
   uint bestU = 0, bestT = 0, bestType = 0;

   BStaticArray<BDXTUtils::BBlockPixel, 16> blockColors;
   
   for (int uy = 0; uy < cDXTBlockSize; uy++)
   {
      for (int ux = 0; ux < cDXTBlockSize; ux++)   
      {
         const BRGBAColor& pixelColor = image(cx * cDXTBlockSize + ux, cy * cDXTBlockSize + uy);

         const WORD packedColor = pixelColor[compIndex];

         uint i;
         for (i = 0; i < blockColors.size(); i++)
            if (blockColors[i].mPackedColor == packedColor)
               break;

         if (i == blockColors.size())
            blockColors.pushBack(BDXTUtils::BBlockPixel(pixelColor, packedColor));
      }
   }

   const uint numUniqueBlockColors = blockColors.size();

   double bestVariance = 1e+30f;         
   uchar bestSelectors[cDXTBlockSize][cDXTBlockSize];         

   for (uint u = 0; u < numUniqueBlockColors; u++)
   {
      int a[8];

      a[0] = blockColors[u].mPackedColor;

      uint firstT = u + 1;
      if (numUniqueBlockColors == 1)
         firstT = 0;

      for (uint t = firstT; t < numUniqueBlockColors; t++)
      {
         a[1] = blockColors[t].mPackedColor;

         const uint maxBlockType = useBothBlockTypes ? 2 : 1;
         for (uint type = 0; type < maxBlockType; type++)
         {
            if (type == 0)
            {
               a[2] = (6*a[0] + 1*a[1])/7; 
               a[3] = (5*a[0] + 2*a[1])/7; 
               a[4] = (4*a[0] + 3*a[1])/7; 
               a[5] = (3*a[0] + 4*a[1])/7; 
               a[6] = (2*a[0] + 5*a[1])/7; 
               a[7] = (1*a[0] + 6*a[1])/7;
            }
            else
            {
               a[2] = (4*a[0] + 1*a[1])/5;
               a[3] = (3*a[0] + 2*a[1])/5;
               a[4] = (2*a[0] + 3*a[1])/5;
               a[5] = (1*a[0] + 4*a[1])/5;
               a[6] = 0;
               a[7] = 255;
            }                           

            double totalVariance = 0.0f;
            uchar selectors[cDXTBlockSize][cDXTBlockSize];

            for (uint sy = 0; sy < cDXTBlockSize; sy++)
            {
               for (uint sx = 0; sx < cDXTBlockSize; sx++)
               {
                  const BRGBAColor& pixel = image(cx * cDXTBlockSize + sx, cy * cDXTBlockSize + sy);

                  int bestDist = INT_MAX;
                  int bestI = 0;
                  for (int i = 0; i < 8; i++)
                  {
                     int trialDist;
                     if (favorLargerAlpha)
                     {
                        trialDist = pixel[compIndex] - a[i];
                        if (trialDist >= 0)
                           trialDist *= trialDist * 3;
                        else
                           trialDist *= trialDist;
                     }
                     else
                        trialDist = (pixel[compIndex] - a[i]) * (pixel[compIndex] - a[i]);

                     if (trialDist < bestDist)
                     {
                        bestDist = trialDist;
                        bestI = i;
                     }
                  }

                  totalVariance += bestDist;
                  selectors[sx][sy] = static_cast<uchar>(bestI);
                  if (totalVariance > bestVariance)
                     goto earlyOut;
               }                  
            }

            if (totalVariance < bestVariance)
            {
               bestVariance = totalVariance;
               bestU = u;
               bestT = t;
               bestType = type;
               memcpy(bestSelectors, selectors, sizeof(selectors));
            }     
earlyOut:            
            ;

         } // type                        
      } // t
   }  // u

   highA = static_cast<uchar>(blockColors[bestU].mPackedColor);
   lowA = static_cast<uchar>(blockColors[bestT].mPackedColor);

   if (highA == lowA)
   {
      memset(bestSelectors, 0, sizeof(bestSelectors));
   }
   else if (bestType == 1)
   {
      if (highA > lowA)
      {
         std::swap(highA, lowA);

         static const uchar pSelectorTrans[] = { 1, 0, 5, 4, 3, 2, 6, 7 };

         for (uint sy = 0; sy < cDXTBlockSize; sy++)
            for (uint sx = 0; sx < cDXTBlockSize; sx++)
               bestSelectors[sx][sy] = pSelectorTrans[bestSelectors[sx][sy]];
      }
   }
   else 
   {
      if (highA < lowA)
      {
         std::swap(highA, lowA);

         static const uchar pSelectorTrans[] = { 1, 0, 7, 6, 5, 4, 3, 2 };

         for (uint sy = 0; sy < cDXTBlockSize; sy++)
            for (uint sx = 0; sx < cDXTBlockSize; sx++)
               bestSelectors[sx][sy] = pSelectorTrans[bestSelectors[sx][sy]];
      }
   }  

   bestBlockType = bestType;
   
   if (pBestSelectors)
      memcpy(pBestSelectors, bestSelectors, 16);
      
   if (pCell)
   {
      pCell->mAlpha[0] = highA;
      pCell->mAlpha[1] = lowA;

      for (uint sy = 0; sy < cDXTBlockSize; sy++)
         for (uint sx = 0; sx < cDXTBlockSize; sx++)
            pCell->setSelector(sx, sy, bestSelectors[sx][sy]);
   }
}

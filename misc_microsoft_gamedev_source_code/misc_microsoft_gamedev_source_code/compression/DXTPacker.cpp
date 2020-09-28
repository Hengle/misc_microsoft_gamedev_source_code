//============================================================================
//
// File: DXTPacker.cpp
// Copyright (c) 2005-2006, Ensemble Studios
//
//============================================================================
#include "compression.h"
#include "xcore.h"
#include "containers\dynamicarray.h"
#include <algorithm>
#include "colorutils.h"
#include "RGBAImage.h"
#include "math\generalvector.h"
#include "math\generalmatrix.h"
#include "DXTUtils.h"
#include "DXTPacker.h"
#include "rangecoder.h"
#include "binaryentropycoder.h"
#include "bitcoder.h"

#include "ImageUtils.h"
#include "containers\priorityQueue.h"

BDXTPacker::BDXTPacker() :
   mpImage(NULL),
   mpStream(NULL),
   mWidth(0), 
   mHeight(0), 
   mCellsX(0), 
   mCellsY(0), 
   mTotalCells(0), 
   mBytesPerBlock(0), 
   mColorBlockOfs(0),
   mDXTFormat(cDXT1),
   mPerceptual(false),
   mStreamOfs(0)
{
}

uint BDXTPacker::createColorBlock(uint cx, uint cy, eDXTQuality quality)
{   
   WORD packedHighColor;
   WORD packedLowColor;
   uint bestType;   
   uchar bestSelectors[cDXTBlockSize][cDXTBlockSize];

   bool allTransparent;
   
   const bool useTransparentBlocks = true;
   
   uint bestVariance;
         
   allTransparent = BDXTUtils::packDXT1Block(
      *mpImage, cx, cy,
      packedHighColor,
      packedLowColor,
      reinterpret_cast<uchar*>(bestSelectors), 
      bestType,
      mPerceptual, mDXTFormat == cDXT1A, NULL, useTransparentBlocks, quality, &bestVariance);  
   
//-- FIXING PREFIX BUG ID 6313
   const uchar* pSelectorTransTable;
//--
   static uchar selectorTransTable3[] = { 1, 2, 0, 3 };
   static uchar selectorTransTable4[] = { 1, 3, 2, 0 };

   if (allTransparent)
   {
      assert(mDXTFormat == cDXT1A);

      packedHighColor = 0;
      packedLowColor = 0;
      pSelectorTransTable = selectorTransTable3;
      memset(bestSelectors, 3, sizeof(bestSelectors));
      bestType = 1;
   }
   else if (bestType)
   {
      if (packedHighColor > packedLowColor)
      {
         std::swap(packedHighColor, packedLowColor);
         for (uint sy = 0; sy < cDXTBlockSize; sy++)
            for (uint sx = 0; sx < cDXTBlockSize; sx++)
            {
               if (bestSelectors[sx][sy] != 3)
                  bestSelectors[sx][sy] = 2 - bestSelectors[sx][sy];
            }
      }
      pSelectorTransTable = selectorTransTable3;

      if (mDXTFormat == cDXT1A)
      {
         for (int sy = 0; sy < cDXTBlockSize; sy++)
         {
            for (int sx = 0; sx < cDXTBlockSize; sx++)   
            {
               const BRGBAColor& pixel = (*mpImage)(cx * cDXTBlockSize + sx, cy * cDXTBlockSize + sy);
               if (pixel.a < 128)
                  bestSelectors[sx][sy] = 3;
            }
         }                  
      }
   }
   else
   {
      if (packedHighColor == packedLowColor)
      {
         memset(bestSelectors, 2, sizeof(bestSelectors));
         pSelectorTransTable = selectorTransTable3;
         bestType = 1;
      }
      else if (packedHighColor < packedLowColor)
      {
         std::swap(packedHighColor, packedLowColor);
         for (uint sy = 0; sy < cDXTBlockSize; sy++)
            for (uint sx = 0; sx < cDXTBlockSize; sx++)
               bestSelectors[sx][sy] = 3 - bestSelectors[sx][sy];

         pSelectorTransTable = selectorTransTable4;
      }
      else
      {
         pSelectorTransTable = selectorTransTable4;
      }
   } // !numUniqueBlockColors            

   BDXTUtils::BDXT1Cell& colorCell = *reinterpret_cast<BDXTUtils::BDXT1Cell*>(&(*mpStream)[mStreamOfs] + (cx + cy * mCellsX) * mBytesPerBlock + mColorBlockOfs);
   colorCell.setColor0(packedHighColor);
   colorCell.setColor1(packedLowColor);

   for (int sy = 0; sy < cDXTBlockSize; sy++)
      for (int sx = 0; sx < cDXTBlockSize; sx++)   
         colorCell.setSelector(sx, sy, pSelectorTransTable[bestSelectors[sx][sy]]);
         
   return bestVariance;
}

void BDXTPacker::createDXT3AlphaBlock(uint cx, uint cy)
{
   BDXTUtils::BDXT3Cell& alphaCell = *reinterpret_cast<BDXTUtils::BDXT3Cell*>(&(*mpStream)[mStreamOfs] + (cx + cy * mCellsX) * 16);

   for (int sy = 0; sy < cDXTBlockSize; sy++)
   {
      for (int sx = 0; sx < cDXTBlockSize; sx++)   
      {
         const BRGBAColor& pixel = (*mpImage)(cx * cDXTBlockSize + sx, cy * cDXTBlockSize + sy);
         alphaCell.setAlpha(sx, sy, pixel.a, true);
      }
   }     
}

void BDXTPacker::createDXT5AlphaBlock(uint cx, uint cy, uint component, uint dstOfs, bool favorLargerAlpha)
{
   uint bestU = 0, bestT = 0, bestType = 0;

   mBlockColors.resize(0);

   for (int uy = 0; uy < cDXTBlockSize; uy++)
   {
      for (int ux = 0; ux < cDXTBlockSize; ux++)   
      {
         const BRGBAColor& pixelColor = (*mpImage)(cx * cDXTBlockSize + ux, cy * cDXTBlockSize + uy);

         const WORD packedColor = pixelColor[component];

         uint i;
         for (i = 0; i < mBlockColors.size(); i++)
            if (mBlockColors[i].mPackedColor == packedColor)
               break;

         if (i == mBlockColors.size())
            mBlockColors.pushBack(BDXTUtils::BBlockPixel(pixelColor, packedColor));
      }
   }

   const uint numUniqueBlockColors = mBlockColors.size();

   double bestVariance = 1e+30f;         
   uchar bestSelectors[cDXTBlockSize][cDXTBlockSize];         

   for (uint u = 0; u < numUniqueBlockColors; u++)
   {
      int a[8];

      a[0] = mBlockColors[u].mPackedColor;

      uint firstT = u + 1;
      if (numUniqueBlockColors == 1)
         firstT = 0;

      for (uint t = firstT; t < numUniqueBlockColors; t++)
      {
         a[1] = mBlockColors[t].mPackedColor;

         for (uint type = 0; type < 2; type++)
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
                  const BRGBAColor& pixel = (*mpImage)(cx * cDXTBlockSize + sx, cy * cDXTBlockSize + sy);

                  int bestDist = INT_MAX;
                  int bestI = 0;
                  for (int i = 0; i < 8; i++)
                  {
                     int trialDist;
                     if (favorLargerAlpha)
                     {
                        trialDist = pixel[component] - a[i];
                        if (trialDist >= 0)
                           trialDist *= trialDist * 3;
                        else
                           trialDist *= trialDist;
                     }
                     else
                        trialDist = (pixel[component] - a[i]) * (pixel[component] - a[i]);

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

   uchar highA = static_cast<uchar>(mBlockColors[bestU].mPackedColor);
   uchar lowA = static_cast<uchar>(mBlockColors[bestT].mPackedColor);

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

   int blockSkipVal = (mDXTFormat==cDXT5A)?8:16;
   BDXTUtils::BDXT5Cell& alphaCell = *reinterpret_cast<BDXTUtils::BDXT5Cell*>(&(*mpStream)[mStreamOfs] + (cx + cy * mCellsX) * blockSkipVal + dstOfs);

   alphaCell.mAlpha[0] = highA;
   alphaCell.mAlpha[1] = lowA;

   for (uint sy = 0; sy < cDXTBlockSize; sy++)
      for (uint sx = 0; sx < cDXTBlockSize; sx++)
         alphaCell.setSelector(sx, sy, bestSelectors[sx][sy]);
}

namespace 
{
   struct BColorCellResults
   {
      uint mX;
      uint mY;
      uint mBestVariance;
      
      BColorCellResults() { }
      BColorCellResults(uint x, uint y, uint bestVariance) : mX(x), mY(y), mBestVariance(bestVariance) { }
      
      bool operator== (const BColorCellResults& rhs) const
      {
         return mBestVariance == rhs.mBestVariance;
      }
      
      bool operator< (const BColorCellResults& rhs) const
      {
         return mBestVariance < rhs.mBestVariance;
      }
   };
}

bool BDXTPacker::pack(const BRGBAImage& image, BDXTFormat dxtFormat, eDXTQuality quality, bool perceptual, bool dithering, BByteArray& stream, bool favorLargerAlpha)
{
   if ((image.getWidth() < 1) || (image.getHeight() < 1))
      return false;   
      
   BRGBAImage temp;
   const BRGBAImage* pSrcImage = &image;
   if ((image.getWidth() & cDXTBlockSizeMask) || (image.getHeight() & cDXTBlockSizeMask))      
   {
      temp.setSize((image.getWidth() + cDXTBlockSizeMask) & ~cDXTBlockSizeMask, (image.getHeight() + cDXTBlockSizeMask) & ~cDXTBlockSizeMask);
      pSrcImage = &temp;   
      
      for (uint dy = 0; dy < temp.getHeight(); dy++)
      {
         const uint sy = Math::Min(dy, image.getHeight() - 1);
         for (uint dx = 0; dx < temp.getWidth(); dx++)
         {
            const uint sx = Math::Min(dx, image.getWidth() - 1);
            
            temp(dx, dy) = image(sx, sy);
         }
      }
   }

   BRGBAImage ditheredImage;
   if (dithering)
   {
      BImageUtils::ditherImage(ditheredImage, *pSrcImage);
      mpImage = &ditheredImage;
   }
   else
      mpImage = pSrcImage;

   mpStream = &stream;

   mWidth = mpImage->getWidth();
   mHeight = mpImage->getHeight();
   mDXTFormat = dxtFormat;
   
   if ((dxtFormat < cDXT1) || (dxtFormat > cDXN))
      return false;

   mCellsX = mWidth >> cDXTBlockSizeBits;
   mCellsY = mHeight >> cDXTBlockSizeBits;
   mTotalCells = mCellsX * mCellsY;
   mBytesPerBlock = ((dxtFormat == cDXT1) || (dxtFormat == cDXT1A) || (dxtFormat == cDXT5A)) ? cDXT1BlockBytes : cDXT35NBlockBytes;
   mColorBlockOfs = (mBytesPerBlock == cDXT1BlockBytes) ? 0 : cDXT1BlockBytes;
   mPerceptual = perceptual;

   mStreamOfs = stream.size();
   stream.resize(mStreamOfs + mTotalCells * mBytesPerBlock);

   mBlockColors.reserve(16);
   
   uint secondPassCellsToRefine = 0;
   uint finalPassCellsToRefine = 0;
   eDXTQuality firstPassQuality = quality;
   eDXTQuality secondPassQuality = quality;
   eDXTQuality finalPassQuality = quality;
   
   bool hasColorBlock = true;
   if (dxtFormat == cDXN || dxtFormat == cDXT5A)
      hasColorBlock = false;

   if (hasColorBlock)
   {
      if (quality == cDXTQualityNormal)
      {
         // rg [5/2/06] FIXME - Images with large solid blocks need to be fixed.
#if 0      
         secondPassCellsToRefine = (mCellsX * mCellsY) / 4;
         finalPassCellsToRefine = (mCellsX * mCellsY) / 40;
         firstPassQuality = cDXTQualityLowest;
         secondPassQuality = cDXTQualityNormal;
         finalPassQuality = cDXTQualityBest;
#endif         
         secondPassCellsToRefine = 0;
         finalPassCellsToRefine = 0;
         firstPassQuality = cDXTQualityNormal;
         secondPassQuality = cDXTQualityNormal;
         finalPassQuality = cDXTQualityNormal;
      }
      else if (quality == cDXTQualityBest)
      {
         secondPassCellsToRefine = (mCellsX * mCellsY) / 7;
         finalPassCellsToRefine = 0;
         firstPassQuality = cDXTQualityNormal;
         secondPassQuality = cDXTQualityBest;
      }
   }      
   
   //firstPassQuality = cDXTQualityBest;
   //secondPassCellsToRefine = 0;
      
   BDynamicPriorityQueue<BColorCellResults> priorityQueue(secondPassCellsToRefine + 1);
   
   //DWORD firstPassStartTime = GetTickCount();

   uint totalVariance = 0;            
   for (uint cy = 0; cy < mCellsY; cy++)
   {
      for (uint cx = 0; cx < mCellsX; cx++)
      {
         if (hasColorBlock)
         {
            const uint bestVariance = createColorBlock(cx, cy, firstPassQuality);
            totalVariance += bestVariance;
         
            if (secondPassCellsToRefine)
               priorityQueue.push(BColorCellResults(cx, cy, bestVariance));
         }               
                  
         switch (dxtFormat)
         {
            case cDXT3:
            {
               createDXT3AlphaBlock(cx, cy);
               break;
            }
            case cDXT5A:
            case cDXT5:
            {
               createDXT5AlphaBlock(cx, cy, 3, 0, favorLargerAlpha);
               break;
            }
            case cDXN:
            {
               // compress RG components (FIXME: which ones are standard?)
               createDXT5AlphaBlock(cx, cy, 0, 0, favorLargerAlpha); // r component to first block
               createDXT5AlphaBlock(cx, cy, 1, 8, favorLargerAlpha); // g component to second block
               break;
            }
         }
                  
      } // cx
   } // cy
   
   //DWORD firstPassEndTime = GetTickCount();

#ifdef WRITE_DEBUG_IMAGES   
   BRGBAImage worstCells(mCellsX * cDXTBlockSize, mCellsY * cDXTBlockSize);
   for (uint i = 0; i < priorityQueue.size(); i++)
      worstCells.fillRect(priorityQueue[i].mX * cDXTBlockSize, priorityQueue[i].mY * cDXTBlockSize, cDXTBlockSize, cDXTBlockSize, BRGBAColor(255,255,255,255));
   char filename[256];
   sprintf(filename, "badcells%ix%i.tga", mCellsX, mCellsY);
   BImageUtils::writeTGA24(filename, worstCells);
#endif   
   
//   printf("First pass time: %u\n", firstPassEndTime - firstPassStartTime);
//   printf("Total variance: %u\n", totalVariance);

   if (secondPassCellsToRefine)
   {
      BDynamicPriorityQueue<BColorCellResults> finalPassPriorityQueue(finalPassCellsToRefine + 1);
      
      //DWORD secondPassStartTime = GetTickCount();

      int totalImprovement = 0;
      for (uint i = 0; i < priorityQueue.size(); i++)
      {
         BColorCellResults cell(priorityQueue[i]);
                  
         const int bestVariance = createColorBlock(cell.mX, cell.mY, secondPassQuality);
         totalImprovement += cell.mBestVariance - bestVariance;
         
         cell.mBestVariance = bestVariance;
         
         if (finalPassCellsToRefine)
            finalPassPriorityQueue.push(cell);
      }
      
      //DWORD secondPassEndTime = GetTickCount();
      
//      printf("Second pass time: %u\n", secondPassEndTime - secondPassStartTime);
//      printf("Second pass improvement: %i\n", totalImprovement);

      if (finalPassCellsToRefine)            
      {
#ifdef WRITE_DEBUG_IMAGES      
         BRGBAImage worstCells(mCellsX * cDXTBlockSize, mCellsY * cDXTBlockSize);
         for (uint i = 0; i < finalPassPriorityQueue.size(); i++)
            worstCells.fillRect(finalPassPriorityQueue[i].mX * cDXTBlockSize, finalPassPriorityQueue[i].mY * cDXTBlockSize, cDXTBlockSize, cDXTBlockSize, BRGBAColor(255,255,255,255));
         char filename[256];
         sprintf(filename, "worstcells%ix%i.tga", mCellsX, mCellsY);
         BImageUtils::writeTGA24(filename, worstCells);
#endif         
      
         //DWORD finalPassStartTime = GetTickCount();

         totalImprovement = 0;
         for (uint i = 0; i < finalPassPriorityQueue.size(); i++)
         {
            const BColorCellResults& cell = finalPassPriorityQueue[i];
            
            const int bestVariance = createColorBlock(cell.mX, cell.mY, finalPassQuality);
            totalImprovement += cell.mBestVariance - bestVariance;
         }

         //DWORD finalPassEndTime = GetTickCount();
         
//         printf("Final pass time: %u\n", finalPassEndTime - finalPassStartTime);
//         printf("Final pass improvement: %i\n", totalImprovement);
      }         
   } 
   
   mBlockColors.clear();

   return true;
}

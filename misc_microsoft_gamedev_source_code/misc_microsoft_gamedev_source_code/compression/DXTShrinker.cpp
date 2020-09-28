//============================================================================
//
// File: DXTShrinker.cpp
// Copyright (c) 2005-2006, Ensemble Studios
//
//============================================================================
#include "xcore.h"
#include "DXTShrinker.h"

// Needed by the LBG quantizer.
#include <map>

#include "LBGQuantizer.h"

#include "containers\bitArray2D.h"
#include "stream\cfileStream.h"
#include "imageUtils.h"

//============================================================================
// BDXTShrinker::colorDistance
//============================================================================
int BDXTShrinker::colorDistance(const BRGBAColor& a, const BRGBAColor& b)
{
   if (mParams.mPerceptual)
      return BColorUtils::colorDistanceWeighted(a, b);
   else
      return BColorUtils::colorDistanceElucidian(a, b);
}

//============================================================================
// BDXTShrinker::findUniqueColors
//============================================================================
void BDXTShrinker::findUniqueColors(BPackedColorHashMap& colorHash, BDynamicArray<BPackedColors>& origBlockColors, int compIndex)
{
   uint numUniqueColors = colorHash.getSize();
   
   for (uint blockIndex = 0; blockIndex < mNumBlocks; blockIndex++)
   {
      BRGBAImage blockImage((BRGBAColor*)&mpInputBlocks[blockIndex], 4, 4);
      BPackedColors packedColors;
      
      if (compIndex < 0)
      {
         WORD packedHighColor, packedLowColor;
         uchar bestSelectors[16];
         uint blockType;
         
         // true if all transparent
         bool result = BDXTUtils::packDXT1Block(
            blockImage, 0, 0,
            packedHighColor,
            packedLowColor,
            bestSelectors,
            blockType,
            true, 
            false,
            NULL,
            false,
            cDXTQualityBest);
         result;
         BDEBUG_ASSERT(!blockType && !result);
         
         Math::OrderLowestToHighest(packedLowColor, packedHighColor);         
         packedColors.setLow(packedLowColor);
         packedColors.setHigh(packedHighColor);
      }
      else
      {
         BYTE lowAlpha, highAlpha;
         uint blockType;
                     
         // true if all transparent
         BDXTUtils::packDXT5Block(
            blockImage, 0, 0, compIndex,
            highAlpha,
            lowAlpha,
            NULL,
            blockType,
            false,
            mParams.mFavorLargerAlpha,
            NULL);
            
         BDEBUG_ASSERT(!blockType);

         Math::OrderLowestToHighest(lowAlpha, highAlpha);         
         packedColors.setLow(lowAlpha);
         packedColors.setHigh(highAlpha);
      }         
            
      origBlockColors.pushBack(packedColors);
      
      BPackedColorHashMap::iterator it = colorHash.find(packedColors);
      if (it != colorHash.end())
      {
         it->second.mWeight = it->second.mWeight + 1;
      }
      else
      {
         colorHash.insert(packedColors, BColorHashCellData(1, numUniqueColors));
         numUniqueColors++;
      }
   }
}   

//============================================================================
// BDXTShrinker::quantizeColors
//============================================================================
void BDXTShrinker::quantizeColors(
   uint maxQuantColors,
   BVec6Quantizer& quantizer, 
   BPackedColorsArray& quantizedPackedColors,
   const BPackedColorHashMap& colorHash, 
   bool alphaBlocks)
{
   float rScale = 1.0f/2.0f;
   float bScale = 1.0f/4.0f;
   
   if (!mParams.mPerceptual)
   {
      rScale = 1.0f;
      bScale = 1.0f;
   }
   
   float oorScale = 1.0f/rScale;
   float oobScale = 1.0f/bScale;
   
   for (BPackedColorHashMap::const_iterator it = colorHash.begin(); it != colorHash.end(); ++it)
   {
      const BPackedColors& packedColors = it->first;
      const uint weight = it->second.mWeight;
      
      BVec6 v;
      if (alphaBlocks)
      {
         v[0] = packedColors.getLow() * 1.0f/255.0f;
         v[1] = packedColors.getHigh() * 1.0f/255.0f;
         v[2] = 0.0f;
         v[3] = 0.0f;
         v[4] = 0.0f;
         v[5] = 0.0f;
      }
      else
      {
         BRGBAColor unpackedLowColor;
         BColorUtils::unpackColor((WORD)packedColors.getLow(), unpackedLowColor, true);

         BRGBAColor unpackedHighColor;
         BColorUtils::unpackColor((WORD)packedColors.getHigh(), unpackedHighColor, true);

         v[0] = unpackedLowColor.r*1.0f/255.0f*rScale;
         v[1] = unpackedLowColor.g*1.0f/255.0f;
         v[2] = unpackedLowColor.b*1.0f/255.0f*bScale;
         v[3] = unpackedHighColor.r*1.0f/255.0f*rScale;
         v[4] = unpackedHighColor.g*1.0f/255.0f;
         v[5] = unpackedHighColor.b*1.0f/255.0f*bScale;
      }  
      
      quantizer.insert(v, (float)weight);       
   }
   
   const float thresh = (1.0f/256.0f)*(1.0f/256.0f);
   quantizer.quantize(thresh, maxQuantColors);

   for (int i = 0; i < quantizer.num_output_cells(); i++)
   {
      const BVec6& v = quantizer.output_cell(i);

      if (alphaBlocks)
      {
         int lowAlpha = Math::iClampToByte(Math::FloatToIntRound(v[0] * 255.0f));
         int highAlpha = Math::iClampToByte(Math::FloatToIntRound(v[1] * 255.0f));
         
         Math::OrderLowestToHighest(lowAlpha, highAlpha);

         quantizedPackedColors.pushBack(BPackedColors(lowAlpha, highAlpha));
      }
      else
      {
         BRGBAColor32 lowC(Math::FloatToIntRound(oorScale*v[0]*255.0f), Math::FloatToIntRound(v[1]*255.0f), Math::FloatToIntRound(oobScale*v[2]*255.0f));
         BRGBAColor32 highC(Math::FloatToIntRound(oorScale*v[3]*255.0f), Math::FloatToIntRound(v[4]*255.0f), Math::FloatToIntRound(oobScale*v[5]*255.0f));

         lowC.clamp(0, 255);
         highC.clamp(0, 255);

         BRGBAColor unpackedLowColor(lowC.getRGBAColor());
         BRGBAColor unpackedHighColor(highC.getRGBAColor());

         uint packedLowColor = BColorUtils::packColor(unpackedLowColor, true);
         uint packedHighColor = BColorUtils::packColor(unpackedHighColor, true);

         Math::OrderLowestToHighest(packedLowColor, packedHighColor);

         quantizedPackedColors.pushBack(BPackedColors(packedLowColor, packedHighColor));
      }         
   }
}

//============================================================================
// BDXTShrinker::computeCellCandidates
//============================================================================
void BDXTShrinker::computeCellCandidates(
   BCellColorCandidateQueues& cellColorCandidateQueues,
   const BPackedColorsArray& quantizedPackedColors,
   const BPackedColorHashMap& colorHash,
   bool alphaBlock)
{
   cellColorCandidateQueues.resize(colorHash.getSize());
   
   for (BPackedColorHashMap::const_iterator it = colorHash.begin(); it != colorHash.end(); ++it)
   {
      const BPackedColors& packedColors = it->first;
      BColorCandidateQueue& colorCandidateQueue = cellColorCandidateQueues[it->second.mUniqueColorIndex];
      
      if (alphaBlock)
      {
         const int lowAlpha = packedColors.getLow();
         const int highAlpha = packedColors.getHigh();

         for (uint cellIndex = 0; cellIndex < quantizedPackedColors.getSize(); cellIndex++)
         {
            const int quantizedLowAlpha = quantizedPackedColors[cellIndex].getLow();
            const int quantizedHighAlpha = quantizedPackedColors[cellIndex].getHigh();
            
            const uint dist0 = Math::Sqr(quantizedLowAlpha - lowAlpha) + Math::Sqr(quantizedHighAlpha - highAlpha);
            const uint dist1 = Math::Sqr(quantizedHighAlpha - lowAlpha) + Math::Sqr(quantizedLowAlpha - highAlpha);
                        
            colorCandidateQueue.push(BColorCandidate(cellIndex, Math::Min(dist0, dist1)));
         }  
      }      
      else
      {
         BRGBAColor unpackedLowColor;
         BColorUtils::unpackColor((WORD)packedColors.getLow(), unpackedLowColor, true);
         
         BRGBAColor unpackedHighColor;
         BColorUtils::unpackColor((WORD)packedColors.getHigh(), unpackedHighColor, true);

         for (uint cellIndex = 0; cellIndex < quantizedPackedColors.getSize(); cellIndex++)
         {
            BRGBAColor quantizedLowColor;
            BColorUtils::unpackColor((WORD)quantizedPackedColors[cellIndex].getLow(), quantizedLowColor, true);
            
            BRGBAColor quantizedHighColor;
            BColorUtils::unpackColor((WORD)quantizedPackedColors[cellIndex].getHigh(), quantizedHighColor, true);

            const uint dist0 = colorDistance(quantizedLowColor, unpackedLowColor) + colorDistance(quantizedHighColor, unpackedHighColor);
            const uint dist1 = colorDistance(quantizedLowColor, unpackedHighColor) + colorDistance(quantizedHighColor, unpackedLowColor);

            colorCandidateQueue.push(BColorCandidate(cellIndex, Math::Min(dist0, dist1)));
         }         
      }         
   }
}   

//============================================================================
// BDXTShrinker::computeColorBlockIndex
//============================================================================
uint BDXTShrinker::computeColorBlockIndex(
   uint& bestError,
   BBlockSelectors* pBestSelectors,
   uint blockIndex,
   const BDynamicArray<BPackedColors>& origBlockColors, 
   const BCellColorCandidateQueues& cellColorCandidateQueues,
   const BPackedColorsArray& quantizedPackedColors,
   const BPackedColorHashMap& colorHash)
{
   BDEBUG_ASSERT(pBestSelectors);
   
   BPackedColorHashMap::const_iterator it = colorHash.find(origBlockColors[blockIndex]);
   BDEBUG_ASSERT(it != colorHash.end());

   const BColorCandidateQueue& colorCandidateQueue = cellColorCandidateQueues[it->second.mUniqueColorIndex];
   BDEBUG_ASSERT(colorCandidateQueue.size() >= 1);

   uint bestCandidateCellIndex = 0;
   uint bestCandidateCellError = UINT_MAX;
   
   for (uint candidateColorIndex = 0; candidateColorIndex < colorCandidateQueue.size(); candidateColorIndex++)
   {
      const int candidateCellIndex = colorCandidateQueue[candidateColorIndex].mCellIndex;

      uint lowColor = quantizedPackedColors[candidateCellIndex].getLow();
      uint highColor = quantizedPackedColors[candidateCellIndex].getHigh();
      
      BDEBUG_ASSERT(lowColor <= highColor);

      BRGBAColor p[4];         
      BColorUtils::unpackColor((WORD)lowColor, p[0], true);
      BColorUtils::unpackColor((WORD)highColor, p[3], true);
      p[1] = BRGBAColor((p[0].r * 2 + p[3].r) / 3, (p[0].g * 2 + p[3].g) / 3, (p[0].b * 2 + p[3].b) / 3, 0);
      p[2] = BRGBAColor((p[0].r + p[3].r * 2) / 3, (p[0].g + p[3].g * 2) / 3, (p[0].b + p[3].b * 2) / 3, 0);
      
      static uchar selectorTransTable4[] = { 1, 3, 2, 0 };
      static uchar selectorTransTable0[] = { 0, 0, 0, 0 };
      const uchar* pSelectorTransTable = selectorTransTable4;
      
      if (highColor == lowColor)
         pSelectorTransTable = selectorTransTable0;
      
      uint totalError = 0;

      uchar candidateSelectors[4][4];
      for (uint y = 0; y < 4; y++)
      {
         uint x;
         for (x = 0; x < 4; x++)
         {
            const BRGBAColor& c = mpInputBlocks[blockIndex].mPixels[y][x];

            uint bestI = 0;
            uint bestDist = UINT_MAX;
            for (uint i = 0; i < 4; i++)
            {
               uint dist = colorDistance(p[i], c);
               if (dist < bestDist)
               {
                  bestDist = dist;
                  bestI = i;
               }
            }

            candidateSelectors[y][x] = pSelectorTransTable[bestI];

            totalError += bestDist;
            if (totalError > bestCandidateCellError)
               break;
         }
         if (x < 4)
            break;
      }

      if (totalError < bestCandidateCellError)
      {
         bestCandidateCellError = totalError;
         bestCandidateCellIndex = candidateCellIndex;
         
         memcpy(pBestSelectors, candidateSelectors, 16);
      }
   }      
   
   bestError = bestCandidateCellError;
   
   return bestCandidateCellIndex;
}   

//============================================================================
// BDXTShrinker::computeAlphaBlockIndex
//============================================================================
uint BDXTShrinker::computeAlphaBlockIndex(
   uint& bestError,
   BBlockSelectors* pBestSelectors,
   uint blockIndex, uint compIndex,
   const BDynamicArray<BPackedColors>& origBlockColors, 
   const BCellColorCandidateQueues& cellColorCandidateQueues,
   const BPackedColorsArray& quantizedPackedColors,
   const BPackedColorHashMap& colorHash)
{
   BDEBUG_ASSERT(pBestSelectors);

   BPackedColorHashMap::const_iterator it = colorHash.find(origBlockColors[blockIndex]);
   BDEBUG_ASSERT(it != colorHash.end());

   const BColorCandidateQueue& colorCandidateQueue = cellColorCandidateQueues[it->second.mUniqueColorIndex];
   BDEBUG_ASSERT(colorCandidateQueue.size() >= 1);

   uint bestCandidateCellIndex = 0;
   uint bestCandidateCellError = UINT_MAX;

   for (uint candidateColorIndex = 0; candidateColorIndex < colorCandidateQueue.size(); candidateColorIndex++)
   {
      const int candidateCellIndex = colorCandidateQueue[candidateColorIndex].mCellIndex;

      uint lowAlpha = quantizedPackedColors[candidateCellIndex].getLow();
      uint highAlpha = quantizedPackedColors[candidateCellIndex].getHigh();
      
      BDEBUG_ASSERT(lowAlpha <= highAlpha);

      int a[8];         

      a[0] = lowAlpha;
      a[1] = highAlpha;
      a[2] = (6*a[0] + 1*a[1])/7; 
      a[3] = (5*a[0] + 2*a[1])/7; 
      a[4] = (4*a[0] + 3*a[1])/7; 
      a[5] = (3*a[0] + 4*a[1])/7; 
      a[6] = (2*a[0] + 5*a[1])/7; 
      a[7] = (1*a[0] + 6*a[1])/7;

      static const uchar pSelectorTrans8[] = { 1, 0, 7, 6, 5, 4, 3, 2 };
      static const uchar pSelectorTrans0[] = { 0, 0, 0, 0, 0, 0, 0, 0 };
      const uchar* pSelectorTrans = pSelectorTrans8;
      if (highAlpha == lowAlpha)
         pSelectorTrans = pSelectorTrans0;

      uint totalError = 0;

      uchar candidateSelectors[4][4];
      for (uint y = 0; y < 4; y++)
      {
         uint x;
         for (x = 0; x < 4; x++)
         {
            int pixelAlpha = mpInputBlocks[blockIndex].mPixels[y][x][compIndex];

            uint bestI = 0;
            uint bestDist = UINT_MAX;
            for (uint i = 0; i < 8; i++)
            {
               int dist;
               if (mParams.mFavorLargerAlpha)
               {
                  dist = pixelAlpha - a[i];
                  if (dist >= 0)
                     dist *= dist * 3;
                  else
                     dist *= dist;
               }
               else
                  dist = Math::Sqr(pixelAlpha - a[i]);
                  
               if ((uint)dist < bestDist)
               {
                  bestDist = dist;
                  bestI = i;
               }
            }

            candidateSelectors[y][x] = pSelectorTrans[bestI];

            totalError += bestDist;
            if (totalError > bestCandidateCellError)
               break;
         }
         if (x < 4)
            break;
      }

      if (totalError < bestCandidateCellError)
      {
         bestCandidateCellError = totalError;
         bestCandidateCellIndex = candidateCellIndex;

         memcpy(pBestSelectors, candidateSelectors, 16);
      }
   }      
   
   bestError = bestCandidateCellError;

   return bestCandidateCellIndex;
}

//============================================================================
// class BCodebookEntry
//============================================================================
class BCodebookEntry
{
   enum { cNumComps = 16 };
   float v[cNumComps];

public:
   BCodebookEntry() 
   { 
      setZero();
   }

   BCodebookEntry(const BCodebookEntry& b)
   {
      for (int i = 0; i < cNumComps; i++)
         v[i] = b.v[i];
   }

   BCodebookEntry& operator=(const BCodebookEntry& b)
   {
      for (int i = 0; i < cNumComps; i++)
         v[i] = b.v[i];
      return *this;
   }

   BCodebookEntry(const float* pValues)
   {
      memcpy(v, pValues, sizeof(v));
   }

   uint numComps(void) const { return cNumComps; }
   float operator[] (uint i) const { assert(i < cNumComps); return v[i]; }
   float& operator[] (uint i) { assert(i < cNumComps); return v[i]; }

   BCodebookEntry& setZero(void)
   {
      for (int i = 0; i < cNumComps; i++)
         v[i] = 0.0f;
      return *this;
   }

   BCodebookEntry& setDefault(void)
   {
      setZero();
      return *this;
   }

   void normalize(void)
   {
      for (int i = 0; i < cNumComps; i++)
      {
         if (v[i] < 0.0f)
            v[i] = 0.0f;
         else if (v[i] > 1.0f)
            v[i] = 1.0f;
      }
   }

   float dot(const BCodebookEntry& b) const
   {
      float total = 0.0f;
      for (int i = 0; i < cNumComps; i++)
         total += b.v[i] * v[i];
      return total;
   }

   float dist2(const BCodebookEntry& b) const
   {
      float total = 0.0f;
      for (int i = 0; i < cNumComps; i++)
         total += (b.v[i] - v[i]) * (b.v[i] - v[i]);
      return total;
   }

   float dist2EarlyOut(const BCodebookEntry& b, float bestDist2) const
   {
      float total = 0.0f;

      for (int i = 0; i < cNumComps / 2; i++)
         total += (b.v[i] - v[i]) * (b.v[i] - v[i]);

      for (int i = cNumComps / 2; i < cNumComps; i++)
      {
         total += (b.v[i] - v[i]) * (b.v[i] - v[i]);
         if (total > bestDist2)
            return 1e+30f;
      }

      return total;
   }

   BCodebookEntry uniformPerturb(float mul) const
   {
      BCodebookEntry ret;
      for (int i = 0; i < cNumComps; i++)
         ret.v[i] = v[i] + mul * .00125f;
      return ret;
   }

   BCodebookEntry randomPerturb(void) const
   {
      BCodebookEntry ret;
      for (int i = 0; i < cNumComps; i++)
         ret.v[i] = v[i] + frand(-.00125f, .00125f);
      return ret;
   }

   friend BCodebookEntry operator* (const BCodebookEntry& b, float w) 
   {
      BCodebookEntry ret;
      for (int i = 0; i < cNumComps; i++)
         ret.v[i] = b.v[i] * w;
      return ret;
   }

   BCodebookEntry& operator*= (float w) 
   {
      for (int i = 0; i < cNumComps; i++)
         v[i] *= w;
      return *this;
   }

   friend BCodebookEntry operator+ (const BCodebookEntry& a, const BCodebookEntry& b)
   {
      BCodebookEntry ret;
      for (int i = 0; i < cNumComps; i++)
         ret.v[i] = a.v[i] + b.v[i];
      return ret;
   }

   friend BCodebookEntry operator- (const BCodebookEntry& a, const BCodebookEntry& b)
   {
      BCodebookEntry ret;
      for (int i = 0; i < cNumComps; i++)
         ret.v[i] = a.v[i] - b.v[i];
      return ret;
   }

   BCodebookEntry& operator+= (const BCodebookEntry& a)
   {
      for (int i = 0; i < cNumComps; i++)
         v[i] += a.v[i];
      return *this;
   }

   bool operator== (const BCodebookEntry& a) const
   {
      for (int i = 0; i < cNumComps; i++)
         if (v[i] != a.v[i])
            return false;
      return true;
   }

   bool operator< (const BCodebookEntry& a) const
   {
      for (int i = 0; i < cNumComps; i++)
      {
         if (v[i] < a.v[i])
            return true;
         else if (v[i] > a.v[i])
            return false;
      }
      return false;
   }

   BCodebookEntry& rotated(BCodebookEntry& result, uint rotation, bool xFlip = false, bool yFlip = false) const
   {
      BCodebookEntry temp;

      if ((yFlip) && (xFlip))
      {
         for (uint y = 0; y < cDXTBlockSize; y++)
            for (uint x = 0; x < cDXTBlockSize; x++)
               temp.v[x + y * cDXTBlockSize] = v[(cDXTBlockSizeMask - x) + (cDXTBlockSizeMask - y) * cDXTBlockSize];
      }
      else if (xFlip)
      {
         for (uint y = 0; y < cDXTBlockSize; y++)
            for (uint x = 0; x < cDXTBlockSize; x++)
               temp.v[x + y * cDXTBlockSize] = v[(cDXTBlockSizeMask - x) + y * cDXTBlockSize];
      }
      else if (yFlip)
      {
         for (uint y = 0; y < cDXTBlockSize; y++)
            for (uint x = 0; x < cDXTBlockSize; x++)
               temp.v[x + y * cDXTBlockSize] = v[x + (cDXTBlockSizeMask - y) * cDXTBlockSize];
      }
      else
      {
         if (!rotation)
         {
            result = *this;
            return result;
         }

         temp = *this;
      }

      switch (rotation)
      {
         case 0:
         {
            result = temp;
            break;
         }
         case 1:
         {
            for (uint y = 0; y < cDXTBlockSize; y++)
               for (uint x = 0; x < cDXTBlockSize; x++)
                  result.v[x + y * cDXTBlockSize] = temp.v[(cDXTBlockSizeMask - y) + x * cDXTBlockSize];   
            break;
         }
         case 2:
         {
            for (uint y = 0; y < cDXTBlockSize; y++)
               for (uint x = 0; x < cDXTBlockSize; x++)
                  result.v[x + y * cDXTBlockSize] = temp.v[(cDXTBlockSizeMask - x) + (cDXTBlockSizeMask - y) * cDXTBlockSize];
            break;
         }
         case 3:
         {
            for (uint y = 0; y < cDXTBlockSize; y++)
               for (uint x = 0; x < cDXTBlockSize; x++)
                  result.v[x + y * cDXTBlockSize] = temp.v[y + (cDXTBlockSizeMask - x) * cDXTBlockSize];   
            break;
         }               
      }

      return result;
   }

private:
   static float frand(float l, float h)
   {
      return (((float)(l)) + ((float)((h)-(l)))*rand()/((float)RAND_MAX));
   }
};

//============================================================================
// class BHistogramSortFunctor
//============================================================================
class BHistogramSortFunctor
{
public:
   BHistogramSortFunctor(const uint* pHist) : mpHist(pHist)
   {
   }

   bool operator() (uint i, uint j) const
   {
      return mpHist[i] > mpHist[j];
   }

private:
   const uint* mpHist;
};

//============================================================================
// struct BColorCellResults
//============================================================================
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

//============================================================================
// BDXTShrinker::createSelectorCodebook
//============================================================================
void BDXTShrinker::createSelectorCodebook(
   UShortArray& blockIndices, BBlockSelectorsArray& codebook,
   bool alphaBlocks,
   const BBlockSelectorsArray& blockSelectors,
   uint codebookSize,
   const BDynamicArray<BPackedColors>* pOrigBlockColors, bool weightByBlockColorDist)
{
   const uint numBlocks = blockSelectors.getSize();
   
   const uint cMaxCodebookSize = 8192;
   if (codebookSize > cMaxCodebookSize)
      codebookSize = cMaxCodebookSize;

   const bool allowRotations = false;
   const bool allowFlips = false;
   BLBGQuantizer<BCodebookEntry, cMaxCodebookSize>* pQuantizer = new BLBGQuantizer<BCodebookEntry, cMaxCodebookSize>(allowRotations, allowFlips);

   const uchar toLinearColor[cDXTColorSelectorValues] = { 3, 0, 2, 1 };
   const uchar toLinearAlpha[cDXT5AlphaSelectorValues] = { 7, 0, 6, 5, 4, 3, 2, 1 };
   
   const uchar fromLinearColor[cDXTColorSelectorValues] = { 1, 3, 2, 0 };
   const uchar fromLinearAlpha[cDXT5AlphaSelectorValues] = { 1, 7, 6, 5, 4, 3, 2, 0 };
            
   double totalWeight = 0;
   
   UIntArray blockColorDist(numBlocks);

   for (uint blockIndex = 0; blockIndex < numBlocks; blockIndex++)
   {
      BCodebookEntry entry;

      for (int sy = 0; sy < cDXTBlockSize; sy++)
      {
         for (int sx = 0; sx < cDXTBlockSize; sx++)
         {
            uint selector = blockSelectors[blockIndex].mSelectors[sy][sx];
            
            if (alphaBlocks)
            {
               selector = toLinearAlpha[selector];
               entry[sx + sy * cDXTBlockSize] = (selector + .5f) * 1.0f/8.0f;
            }
            else
            {
               selector = toLinearColor[selector];
               entry[sx + sy * cDXTBlockSize] = (selector + .5f) * 1.0f/4.0f;
            }
         }
      }

      uint weight = 1;
      
      if (pOrigBlockColors)
      {
         if (alphaBlocks) 
         {
            if (weightByBlockColorDist)
            {
               int lowAlpha, highAlpha;
               lowAlpha = (*pOrigBlockColors)[blockIndex].getLow();
               highAlpha = (*pOrigBlockColors)[blockIndex].getHigh();
               
               int dist = (lowAlpha - highAlpha);
               dist *= dist;
               
               weight = dist / 8;
            }               
         }
         else
         {
            BRGBAColor blockLowColor, blockHighColor;
            BColorUtils::unpackColor((WORD)(*pOrigBlockColors)[blockIndex].getLow(), blockLowColor, true);
            BColorUtils::unpackColor((WORD)(*pOrigBlockColors)[blockIndex].getHigh(), blockHighColor, true); 
         
            const uint colorDist = colorDistance(blockLowColor, blockHighColor);
            blockColorDist[blockIndex] = colorDist;
               
            if (weightByBlockColorDist)
               weight = colorDist / 2048; 
         }     
      }         
      
      const uint cMaxWeight = 16384;
      weight = Math::Clamp<int>(weight, 1, cMaxWeight);    
      
      pQuantizer->updateHistogram(entry, weight);
      totalWeight += weight;
   }

   pQuantizer->createCodebook(codebookSize, .0125f);
      
   blockIndices.resize(numBlocks);
   
   UIntVec codebookEntryHist(pQuantizer->getCodebookSize());
     
   const bool exhaustiveSearch = true;
         
   for (uint blockIndex = 0; blockIndex < numBlocks; blockIndex++)
   {
      int entryIndex = -1;
      
      if ((!alphaBlocks) && (exhaustiveSearch) && (blockColorDist[blockIndex] > 1024))
      {
         // Exhaustive search
         const BDXTBlockPixels* pInputBlock = &mpInputBlocks[blockIndex];
         
         const BPackedColors& quantizedBlockColors = mQuantizedPackedColors[mColorIndices[blockIndex]];
         
         const uint lowColor = quantizedBlockColors.getLow();
         const uint highColor = quantizedBlockColors.getHigh();

         BDEBUG_ASSERT(lowColor <= highColor);         
         
         if (lowColor != highColor)
         {
            BRGBAColor p[4];         
            BColorUtils::unpackColor((WORD)lowColor, p[0], true);
            BColorUtils::unpackColor((WORD)highColor, p[3], true);
            p[1] = BRGBAColor((p[0].r * 2 + p[3].r) / 3, (p[0].g * 2 + p[3].g) / 3, (p[0].b * 2 + p[3].b) / 3, 0);
            p[2] = BRGBAColor((p[0].r + p[3].r * 2) / 3, (p[0].g + p[3].g * 2) / 3, (p[0].b + p[3].b * 2) / 3, 0);

            uint lowestError = UINT_MAX;
            
            for (uint codebookIndex = 0; codebookIndex < pQuantizer->getCodebookSize(); codebookIndex++)
            {
               BCodebookEntry& codebookEntry = pQuantizer->getCodebookEntry(codebookIndex);
                           
               uint totalError = 0;

               for (uint y = 0; y < 4; y++)
               {
                  uint x;
                  for (x = 0; x < 4; x++)
                  {
                     const uint selector = Math::Clamp<int>(Math::FloatToIntTrunc(codebookEntry[x + y * cDXTBlockSize] * 4.0f), 0, 3);
                                       
                     const BRGBAColor& c = pInputBlock->mPixels[y][x];
                     
                     uint dist = colorDistance(p[selector], c);
                        
                     totalError += dist;
                  }
                  if (totalError > lowestError)
                     break;
               }

#if 0
// biasing              
               int histIndex;
               for (histIndex = -7; histIndex <= -1; histIndex++)
               {
                  if (((int)blockIndex + histIndex) >= 0)
                  {
                     if (codebookIndex == blockIndices[blockIndex + histIndex])
                        break;
                  }
               }
               if (histIndex != 0)
               {
                  histIndex = 8 + histIndex;
                  totalError = Math::Max<int>(1, totalError - histIndex * 4);
               }
#endif               

               if (totalError < lowestError)
               {
                  lowestError = totalError;
                  entryIndex = codebookIndex;
               }
            }               
         
         } // codebookIndex
      }
      
      if (entryIndex == -1)
      {
         // Fast search
         BCodebookEntry entry;

         for (int sy = 0; sy < cDXTBlockSize; sy++)
         {
            for (int sx = 0; sx < cDXTBlockSize; sx++)
            {
               uint selector = blockSelectors[blockIndex].mSelectors[sy][sx];

               if (alphaBlocks)
               {
                  selector = toLinearAlpha[selector];
                  entry[sx + sy * cDXTBlockSize] = (selector + .5f) * 1.0f/8.0f;
               }
               else
               {
                  selector = toLinearColor[selector];
                  entry[sx + sy * cDXTBlockSize] = (selector + .5f) * 1.0f/4.0f;
               }
            }
         }

         uint rot;
         bool xFlip, yFlip;
         entryIndex = pQuantizer->findBestCodebookEntry(entry, rot, xFlip, yFlip);
      }  
      
      blockIndices[blockIndex] = (ushort)entryIndex;
      codebookEntryHist[entryIndex]++;       
   }

   // Sort codebook by frequency   
   BDynamicArray<uint> sortedCodebookIndices(pQuantizer->getCodebookSize());
   BDynamicArray<uint> sortedCodebookIndicesInv(pQuantizer->getCodebookSize());

   for (uint i = 0; i < pQuantizer->getCodebookSize(); i++)
      sortedCodebookIndices[i] = i;
      
   std::sort(&sortedCodebookIndices[0], &sortedCodebookIndices[0] + pQuantizer->getCodebookSize(), BHistogramSortFunctor(&codebookEntryHist[0]));
   
   for (uint i = 0; i < pQuantizer->getCodebookSize(); i++)
      sortedCodebookIndicesInv[sortedCodebookIndices[i]] = i;

   UShortArray quantizedCellIndices(numBlocks);
   for (uint i = 0; i < numBlocks; i++)
   {  
      const uint oldEntryIndex = blockIndices[i];
      const uint newEntryIndex = sortedCodebookIndicesInv[oldEntryIndex];       

      quantizedCellIndices.at(i) = (ushort)newEntryIndex;
   }

   // Create selector codebook
   codebook.resize(pQuantizer->getCodebookSize());

   for (uint newEntryIndex = 0; newEntryIndex < pQuantizer->getCodebookSize(); newEntryIndex++)
   {
      const uint oldEntryIndex = sortedCodebookIndices[newEntryIndex];

      for (int v = 0; v < 16; v++)
      {
         int selector;
         
         if (alphaBlocks)
            selector = fromLinearAlpha[Math::Clamp(Math::FloatToIntTrunc(pQuantizer->getCodebookEntry(oldEntryIndex)[v] * 8.0f), 0, 7)];
         else
            selector = fromLinearColor[Math::Clamp(Math::FloatToIntTrunc(pQuantizer->getCodebookEntry(oldEntryIndex)[v] * 4.0f), 0, 3)];
         
         codebook[newEntryIndex].mSelectors[v >> 2][v & 3] = (uchar)selector;
      }
   }

   delete pQuantizer;  
   
   blockIndices.swap(quantizedCellIndices);
}  

//============================================================================
// sortCodebookAndIndices
//============================================================================
template<typename T>
void sortCodebookAndIndices(T& codebook, UShortArray& indices0, UShortArray* pIndices1)
{
#if 0
   // By access order
   ShortVec origToNewMap(codebook.getSize());
   origToNewMap.setAll(-1);
   uint numNewIndices = 0;
   
   UShortVec newToOrigMap(codebook.getSize());
   
   UShortVec newIndices0(indices0.getSize());
   UShortVec newIndices1(pIndices1 ? pIndices1->getSize() : 0);
      
   for (uint i = 0; i < indices0.getSize(); i++)
   {
      const uint origIndex0 = indices0[i];
            
      if (origToNewMap[origIndex0] == -1)
      {
         origToNewMap[origIndex0] = (short)numNewIndices;
         newToOrigMap[numNewIndices] = (short)origIndex0;
         numNewIndices++;
      }
      
      newIndices0[i] = origToNewMap[origIndex0];
            
      if (pIndices1)            
      {
         const uint origIndex1 = (*pIndices1)[i];

         if (origToNewMap[origIndex1] == -1)
         {
            origToNewMap[origIndex1] = (short)numNewIndices;
            newToOrigMap[numNewIndices] = (short)origIndex1;
            numNewIndices++;
         }

         newIndices1[i] = origToNewMap[origIndex1];
      }
   }
   
   BDEBUG_ASSERT(numNewIndices <= codebook.getSize());
   
   T origCodebook(codebook);
   
   for (uint i = 0; i < numNewIndices; i++)
      codebook[i] = origCodebook[newToOrigMap[i]];
   codebook.resize(numNewIndices);      

   indices0.swap(newIndices0);
   if (pIndices1)
      (*pIndices1).swap(newIndices1);
#endif
   
#if 1
   // By Frequency      
   const uint codebookSize = codebook.getSize();
   UIntVec hist(codebookSize);
   
   for (uint i = 0; i < indices0.getSize(); i++)
      hist[indices0[i]]++;
      
   if (pIndices1)
   {
      for (uint i = 0; i < pIndices1->getSize(); i++)
         hist[(*pIndices1)[i]]++;
   }
   
   UIntVec sortedCodebookIndices(codebookSize); 
   for (uint i = 0; i < codebookSize; i++)
      sortedCodebookIndices[i] = i;

   std::sort(&sortedCodebookIndices[0], &sortedCodebookIndices[0] + codebookSize, BHistogramSortFunctor(hist.getPtr()));
   
   UIntVec sortedCodebookIndicesInv(codebookSize);
   for (uint i = 0; i < codebookSize; i++)
      sortedCodebookIndicesInv[sortedCodebookIndices[i]] = i;

   const uint numBlocks = indices0.getSize();
   
   UShortArray newColorIndices(numBlocks);
   for (uint i = 0; i < numBlocks; i++)
   {  
      const uint oldEntryIndex = indices0[i];
      const uint newEntryIndex = sortedCodebookIndicesInv[oldEntryIndex];       

      newColorIndices.at(i) = (ushort)newEntryIndex;
   }
   newColorIndices.swap(indices0);
   
   if (pIndices1)
   {
      UShortArray newColorIndices2(numBlocks);
      for (uint i = 0; i < numBlocks; i++)
      {  
         const uint oldEntryIndex = (*pIndices1)[i];
         const uint newEntryIndex = sortedCodebookIndicesInv[oldEntryIndex];       

         newColorIndices2.at(i) = (ushort)newEntryIndex;
      }
      newColorIndices2.swap(*pIndices1);
   }
   
   T newCodebook(codebookSize);
   for (uint newEntryIndex = 0; newEntryIndex < codebookSize; newEntryIndex++)
   {
      const uint oldEntryIndex = sortedCodebookIndices[newEntryIndex];

      newCodebook[newEntryIndex] = codebook[oldEntryIndex];
   }
      
   newCodebook.swap(codebook);
#endif   
}
//============================================================================
// BDXTShrinker::refineColorCodebook
//============================================================================
void BDXTShrinker::refineColorCodebook(BBlockSelectorsArray& blockSelectors)
{
   // Color codebook and indices:
   //BPackedColorsArray mQuantizedPackedColors;
   //UShortArray mColorIndices;
   
   // Color selector codebook and indices:
   //UShortArray mColorSelectorIndices;
   //BBlockSelectorsArray mColorSelectorCodebook;
   
   // Orig block colors:
   // mpInputBlocks   
   
   // Goal: Change color codebook low/high values to improve PSNR!
   
   BDynamicArray<UIntVec> codebookBlockIndices(mQuantizedPackedColors.getSize());
   
   for (uint blockIndex = 0; blockIndex < mColorIndices.getSize(); blockIndex++)
   {
      const uint codebookIndex = mColorIndices[blockIndex];
      codebookBlockIndices[codebookIndex].pushBack(blockIndex);
   }
   
   BDynamicArray<BVec3> sampleVecs;
   sampleVecs.reserve(16*256);
   
   BDXTUtils::BUniqueColorArray uniqueColors;
   uniqueColors.reserve(16*256);
   
   UShortArray uniqueColorIndices;
   uniqueColorIndices.reserve(16*256);
   
   for (uint codebookIndex = 0; codebookIndex < mQuantizedPackedColors.getSize(); codebookIndex++)
   {
      //const BPackedColors& codebookEntry = mQuantizedPackedColors[codebookIndex];
      const UIntVec& blockIndices = codebookBlockIndices[codebookIndex];
      
      if (blockIndices.empty())
         continue;

      BVec3 meanColor(0.0f);
      
      sampleVecs.resize(0);
      uniqueColors.resize(0);
      uniqueColorIndices.resize(0);
                        
      for (uint i = 0; i < blockIndices.getSize(); i++)
      {
         const uint blockIndex = blockIndices[i];
         const BDXTBlockPixels* pBlockPixels = &mpInputBlocks[blockIndex];
         
         for (uint y = 0; y < 4; y++)
         {
            for (uint x = 0; x < 4; x++)
            {
               const BRGBAColor& pixelColor = pBlockPixels->mPixels[y][x];

               uint j;      
               for (j = 0; j < uniqueColors.getSize(); j++)
               {
                  if (pixelColor == uniqueColors[j].mColor)
                  {
                     uniqueColors[j].mWeight++;
                     break;
                  }
               }
               if (j == uniqueColors.getSize())
                  uniqueColors.pushBack(BDXTUtils::BUniqueColor(pixelColor, 1));
               uniqueColorIndices.pushBack((ushort)j);
               
               BVec3 c(pixelColor.r, pixelColor.g, pixelColor.b);
               
               c *= 1.0f/255.0f;
               
               sampleVecs.pushBack(c);
               
               meanColor += c;
            }
         }
      }
      const uint totalPixels = blockIndices.getSize() * 16;
      
      meanColor /= (float)totalPixels;
            
      BVec3 v2(0.0f);
      for (uint i = 0; i < sampleVecs.getSize(); i++)
      {
         BVec3 sampleVec(sampleVecs[i]);
               
         sampleVec -= meanColor;

         BVec3 a, b, c;
         a = sampleVec * sampleVec[0];
         b = sampleVec * sampleVec[1];
         c = sampleVec * sampleVec[2];

         BVec3 nV(i ? v2 : sampleVec);
         nV.tryNormalize();

         v2[0] += a * nV;
         v2[1] += b * nV;
         v2[2] += c * nV;
      }

      v2.tryNormalize();
      v2 = -v2;
      BVec3 axis(v2);
      
      // Found axis
      
      float l = 1e+10, h = -1e+10;
      
      for (uint i = 0; i < sampleVecs.getSize(); i++)
      {
         BVec3 sampleVec(sampleVecs[i]);

         sampleVec -= meanColor;
         
         float d = sampleVec * axis;
         l = Math::Min(l, d);
         h = Math::Max(h, d);
      }
      
      BDynamicArray<uchar> bestSelectors(uniqueColors.getSize());
            
      uint bestVariance = UINT_MAX;
      WORD packedLowColor = 0, packedHighColor = 0;
      uint bestBlockType = 0;
      BDXTUtils::BColorCellEvaluator evaluator(
         false, mParams.mPerceptual,
         0, 0,
         bestVariance,
         packedLowColor,
         packedHighColor,
         bestBlockType,
         bestSelectors.getPtr(),
         uniqueColors);

      BStaticArray<DWORD, 512> colorsToTry;
           
      BVec3 l0(axis * l + meanColor);
      BVec3 h0(axis * h + meanColor);

      const uint numPasses = 3;
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

         const uint searchDist = 5;
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

//-- FIXING PREFIX BUG ID 6262
         const DWORD* pLast = std::unique(colorsToTry.begin(), colorsToTry.end());
//--
         uint numUnique = pLast - colorsToTry.begin();

         for (uint colorIndex = 0; colorIndex < numUnique; colorIndex++)
         {
            const WORD packedL = static_cast<WORD>(colorsToTry[colorIndex] & 0xFFFF);
            const WORD packedH = static_cast<WORD>(colorsToTry[colorIndex] >> 16);

            evaluator.createSelectors(packedL, packedH);
         } // colorIndex

         // Phase 2
                           
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
         
         BRGBAColor l;
         BColorUtils::unpackColor(packedLowColor, l, true);
         l0.set(l.r/255.0f, l.g/255.0f, l.b/255.0f, 0);

         BRGBAColor h;
         BColorUtils::unpackColor(packedHighColor, h, true);
         h0.set(h.r/255.0f, h.g/255.0f, h.b/255.0f, 0);

         if (bestVariance == prevVariance)
            break;
      }  // passIndex

      // Finished
      
      BDEBUG_ASSERT(packedLowColor <= packedHighColor);
      mQuantizedPackedColors[codebookIndex].setLow(packedLowColor);
      mQuantizedPackedColors[codebookIndex].setHigh(packedHighColor);
      
      const uchar fromLinearColor[cDXTColorSelectorValues] = { 1, 3, 2, 0 };
      
      for (uint i = 0; i < blockIndices.getSize(); i++)
      {
         const uint blockIndex = blockIndices[i];
         
         BBlockSelectors& selectors = blockSelectors[blockIndex];
         
         for (uint y = 0; y < 4; y++)
         {
            for (uint x = 0; x < 4; x++)
            {
               const uint uniqueColorIndex = uniqueColorIndices[i * 16 + y * 4 + x];
                              
               if (packedLowColor == packedHighColor)
                  selectors.mSelectors[y][x] = 0;
               else
                  selectors.mSelectors[y][x] = fromLinearColor[bestSelectors[uniqueColorIndex]];
            }
         }
      }
      
            
   }
}   

//============================================================================
// BDXTShrinker::BDXTShrinker
//============================================================================
BDXTShrinker::BDXTShrinker(BDXTFormat dxtFormat, uint numBlocks, const BDXTBlockPixels* pInputBlocks, const BParams& params) :
   mDXTFormat(dxtFormat),
   mNumBlocks(numBlocks),
   mpInputBlocks(pInputBlocks),
   mParams(params)
{
   BPackedColorHashMap colorHash;
   BDynamicArray<BPackedColors> origBlockColors;
   
   BPackedColorHashMap alphaHash;
   BDynamicArray<BPackedColors> origBlockAlpha0;
   BDynamicArray<BPackedColors> origBlockAlpha1;
   
   trace("Finding unique cell colors");
   
   if ((dxtFormat == cDXT1) || (dxtFormat == cDXT5))
   {
      findUniqueColors(colorHash, origBlockColors, -1);
   }
   
   if (dxtFormat == cDXT5)
   {
      findUniqueColors(alphaHash, origBlockAlpha0, 3);
   }
   else if (dxtFormat == cDXN)
   {
      findUniqueColors(alphaHash, origBlockAlpha0, 0);
      findUniqueColors(alphaHash, origBlockAlpha1, 1);
   }
      
   trace("Computing palettes");
   
   BVec6Quantizer colorQuantizer;
   BCellColorCandidateQueues cellColorCandidateQueues;
   
   if ((dxtFormat == cDXT1) || (dxtFormat == cDXT5))
   {
      quantizeColors(
         params.mMaxQuantColors,
         colorQuantizer, 
         mQuantizedPackedColors,
         colorHash, 
         false);
      
      computeCellCandidates(
         cellColorCandidateQueues,
         mQuantizedPackedColors,
         colorHash,
         false);
   }         
         
   BVec6Quantizer alphaQuantizer;
   BCellColorCandidateQueues cellAlphaCandidateQueues;
   
   if ((dxtFormat == cDXT5) || (dxtFormat == cDXN))
   {
      quantizeColors(
         params.mMaxAlphaColors,
         alphaQuantizer, 
         mQuantizedPackedAlpha,
         alphaHash, 
         true);
         
      computeCellCandidates(
         cellAlphaCandidateQueues,
         mQuantizedPackedAlpha,
         alphaHash,
         true);
   }
   
   BBlockSelectorsArray colorSelectors;
   BBlockSelectorsArray alphaSelectors0;
   BBlockSelectorsArray alphaSelectors1;
      
   if ((dxtFormat == cDXT1) || (dxtFormat == cDXT5))
   {
      mColorIndices.resize(mNumBlocks);
      colorSelectors.resize(mNumBlocks);
   }
      
   if ((dxtFormat == cDXT5) || (dxtFormat == cDXN))
   {
      mAlphaIndices0.resize(mNumBlocks);
      alphaSelectors0.resize(mNumBlocks);
   }
      
   if (dxtFormat == cDXN)
   {
      mAlphaIndices1.resize(mNumBlocks);
      alphaSelectors1.resize(mNumBlocks);
   }
   
   trace("Computing indices");
      
   for (uint blockIndex = 0; blockIndex < mNumBlocks; blockIndex++)
   {
      if ((dxtFormat == cDXT1) || (dxtFormat == cDXT5))
      {
         uint bestColorError;
         mColorIndices[blockIndex] = (ushort)computeColorBlockIndex(
            bestColorError,
            &colorSelectors[blockIndex],
            blockIndex,
            origBlockColors, 
            cellColorCandidateQueues,
            mQuantizedPackedColors,
            colorHash);
      }
      
      if (dxtFormat == cDXT5) 
      {
         uint bestAlphaError;
         mAlphaIndices0[blockIndex] = (ushort)computeAlphaBlockIndex(
            bestAlphaError,
            &alphaSelectors0[blockIndex],
            blockIndex, 3,
            origBlockAlpha0, 
            cellAlphaCandidateQueues,
            mQuantizedPackedAlpha,
            alphaHash);
      }
      else if (dxtFormat == cDXN)
      {
         uint bestAlphaError;
         mAlphaIndices0[blockIndex] = (ushort)computeAlphaBlockIndex(
            bestAlphaError,
            &alphaSelectors0[blockIndex],
            blockIndex, 0,
            origBlockAlpha0, 
            cellAlphaCandidateQueues,
            mQuantizedPackedAlpha,
            alphaHash);
            
         mAlphaIndices1[blockIndex] = (ushort)computeAlphaBlockIndex(
            bestAlphaError,
            &alphaSelectors1[blockIndex],
            blockIndex, 1,
            origBlockAlpha1, 
            cellAlphaCandidateQueues,
            mQuantizedPackedAlpha,
            alphaHash);
      }
   }
   
   if ((dxtFormat == cDXT1) || (dxtFormat == cDXT5))
   {
      sortColorsBySimilarity(mQuantizedPackedColors, mColorIndices, NULL, colorDistFunc);
      
      refineColorCodebook(colorSelectors);
   }
   
   if (dxtFormat == cDXT5)
   {
      sortColorsBySimilarity(mQuantizedPackedAlpha, mAlphaIndices0, NULL, alphaDistFunc);
   }
   else if (dxtFormat == cDXN)
   {
      sortColorsBySimilarity(mQuantizedPackedAlpha, mAlphaIndices0, &mAlphaIndices1, alphaDistFunc);
   }
   
   trace("Building selector codebooks");
               
   if ((dxtFormat == cDXT1) || (dxtFormat == cDXT5))
   {
      createSelectorCodebook(
         mColorSelectorIndices, mColorSelectorCodebook,
         false,
         colorSelectors,
         params.mColorSelectorCodebookSize,
         &origBlockColors, true);
      
      sortSelectorsBySimilarity(mColorSelectorCodebook, mColorSelectorIndices, NULL);
   }
   
   if (dxtFormat == cDXT5)
   {
      createSelectorCodebook(
         mAlphaSelectorIndices, mAlphaSelectorCodebook,
         true,
         alphaSelectors0,
         params.mAlphaSelectorCodebookSize,
         &origBlockAlpha0, true);
         
      sortSelectorsBySimilarity(mAlphaSelectorCodebook, mAlphaSelectorIndices, NULL);
   }
   else if (dxtFormat == cDXN)
   {
      BBlockSelectorsArray alphaSelectors(alphaSelectors0);
      alphaSelectors.append(alphaSelectors1);
      
      BDynamicArray<BPackedColors> blockColors(origBlockAlpha0);
      blockColors.append(origBlockAlpha1);
      
      createSelectorCodebook(
         mAlphaSelectorIndices, mAlphaSelectorCodebook,
         true,
         alphaSelectors,
         params.mAlphaSelectorCodebookSize,
         &blockColors, true);
         
      BDEBUG_ASSERT(mAlphaSelectorIndices.getSize() == numBlocks * 2);
      
      UShortVec alphaSelectorIndices0(numBlocks);
      UShortVec alphaSelectorIndices1(numBlocks);
      for (uint i = 0; i < numBlocks; i++)
      {
         alphaSelectorIndices0[i] = mAlphaSelectorIndices[i];
         alphaSelectorIndices1[i] = mAlphaSelectorIndices[numBlocks + i];
      }
      
      sortSelectorsBySimilarity(mAlphaSelectorCodebook, alphaSelectorIndices0, &alphaSelectorIndices1);
      
      for (uint i = 0; i < numBlocks; i++)
      {
         mAlphaSelectorIndices[i] = alphaSelectorIndices0[i];
         mAlphaSelectorIndices[i + numBlocks] = alphaSelectorIndices1[i];
      }
   }
}            

//============================================================================
// BDXTShrinker::colorDistFunc
//============================================================================
uint BDXTShrinker::colorDistFunc(BDXTShrinker& shrinker, const BPackedColors& a, const BPackedColors& b)
{
   shrinker;
   BRGBAColor al, ah;
   BRGBAColor bl, bh;
   BColorUtils::unpackColor((WORD)a.getLow(), al, true);
   BColorUtils::unpackColor((WORD)a.getHigh(), ah, true);
   
   BColorUtils::unpackColor((WORD)b.getLow(), bl, true);
   BColorUtils::unpackColor((WORD)b.getHigh(), bh, true);
   
   return BColorUtils::colorDistanceElucidian(al, bl) + BColorUtils::colorDistanceElucidian(ah, bh);
}

//============================================================================
// BDXTShrinker::alphaDistFunc
//============================================================================
uint BDXTShrinker::alphaDistFunc(BDXTShrinker& shrinker, const BPackedColors& a, const BPackedColors& b)
{
   shrinker;
   int l = a.getLow() - b.getLow();
   int h = a.getHigh() - b.getHigh();
   return (uint)(l * l + h * h);
}

//============================================================================
// BDXTShrinker::sortColorsBySimilarity
//============================================================================
void BDXTShrinker::sortColorsBySimilarity(BPackedColorsArray& colors, UShortArray& indices0, UShortArray* pIndices1, BColorDistFunc pColorDistFunc)
{
   UIntVec colorOrder(colors.getSize());
   BBitArray2D<1> colorChosen(colors.getSize(), 1);
   
   const BPackedColors zeroColor(0, 0);
   
   uint lowestDist = UINT_MAX;
   uint lowestIndex = 0;
   for (uint i = 0; i < colors.getSize(); i++)
   {
      uint dist = (*pColorDistFunc)(*this, zeroColor, colors[i]);
      if (dist < lowestDist)
      {
         lowestDist = dist;
         lowestIndex = i;
      }
   }
   
   colorChosen.set(lowestIndex, 1);
   colorOrder[0] = lowestIndex;
   uint curColor = lowestIndex;

   for (uint i = 1; i < colors.getSize(); i++)
   {
      uint bestDist = UINT_MAX;
      int bestIndex = -1;
      
      for (uint j = 0; j < colors.getSize(); j++)
      {
         if (colorChosen.get(j))
            continue;

         uint dist = (*pColorDistFunc)(*this, colors[curColor], colors[j]);
         if (dist < bestDist)
         {
            bestDist = dist;
            bestIndex = j;
         }
      }

      colorChosen.set(bestIndex, 1);
      colorOrder[i] = bestIndex;

      curColor = bestIndex;
   }

   UIntVec colorOrderOldToNew(colors.getSize());
   for (uint i = 0; i < colorOrder.getSize(); i++)
      colorOrderOldToNew[colorOrder[i]] = i;

   {
      const uint numBlocks = indices0.getSize();

      UShortArray newColorIndices(numBlocks);
      for (uint i = 0; i < numBlocks; i++)
      {  
         const uint oldEntryIndex = indices0[i];
         const uint newEntryIndex = colorOrderOldToNew[oldEntryIndex];       

         newColorIndices.at(i) = (ushort)newEntryIndex;
      }
      indices0.swap(newColorIndices);
   }      
   
   if (pIndices1)
   {
      const uint numBlocks = pIndices1->getSize();

      UShortArray newColorIndices(numBlocks);
      for (uint i = 0; i < numBlocks; i++)
      {  
         const uint oldEntryIndex = (*pIndices1)[i];
         const uint newEntryIndex = colorOrderOldToNew[oldEntryIndex];       

         newColorIndices.at(i) = (ushort)newEntryIndex;
      }
      pIndices1->swap(newColorIndices);
   }

   const uint codebookSize = colors.getSize();
   BPackedColorsArray newCodebook(codebookSize);
   for (uint newEntryIndex = 0; newEntryIndex < codebookSize; newEntryIndex++)
   {
      const uint oldEntryIndex = colorOrder[newEntryIndex];

      newCodebook[newEntryIndex] = colors[oldEntryIndex];
   }

   colors.swap(newCodebook);
}

//============================================================================
// BDXTShrinker::sortSelectorsBySimilarity
//============================================================================
void BDXTShrinker::sortSelectorsBySimilarity(BBlockSelectorsArray& selectors, UShortArray& indices0, UShortArray* pIndices1)
{
   UIntVec selectorOrder(selectors.getSize());
   BBitArray2D<1> selectorChosen(selectors.getSize(), 1);
   
   uint lowestRank = UINT_MAX;
   uint lowestIndex = 0;
   for (uint i = 0; i < selectors.getSize(); i++)
   {
      uint rank = 0;
      for (uint y = 0; y < 4; y++)
         for (uint x = 0; x < 4; x++)
            rank += selectors[i].mSelectors[y][x];
            
      if (rank < lowestRank)
      {
         lowestRank = rank;
         lowestIndex = i;
      }
   }

   selectorChosen.set(lowestIndex, 1);
   selectorOrder[0] = lowestIndex;
   uint curSelector = lowestIndex;

   for (uint i = 1; i < selectors.getSize(); i++)
   {
      uint bestDist = UINT_MAX;
      int bestIndex = -1;

      for (uint j = 0; j < selectors.getSize(); j++)
      {
         if (selectorChosen.get(j))
            continue;

         uint dist = 0;
         for (uint y = 0; y < 4; y++)
            for (uint x = 0; x < 4; x++)
               dist += Utils::CountBits(selectors[curSelector].mSelectors[y][x] ^ selectors[j].mSelectors[y][x]);
         
         if (dist < bestDist)
         {
            bestDist = dist;
            bestIndex = j;
         }
      }

      selectorChosen.set(bestIndex, 1);
      selectorOrder[i] = bestIndex;

      curSelector = bestIndex;
   }

   UIntVec selectorOrderOldToNew(selectors.getSize());
   for (uint i = 0; i < selectorOrder.getSize(); i++)
      selectorOrderOldToNew[selectorOrder[i]] = i;

   {
      const uint numBlocks = indices0.getSize();
      UShortArray newSelectorIndices(numBlocks);
      for (uint i = 0; i < numBlocks; i++)
      {  
         const uint oldEntryIndex = indices0[i];
         const uint newEntryIndex = selectorOrderOldToNew[oldEntryIndex];       

         newSelectorIndices.at(i) = (ushort)newEntryIndex;
      }
      indices0.swap(newSelectorIndices);
   }
   
   if (pIndices1)
   {
      const uint numBlocks = pIndices1->getSize();
      UShortArray newSelectorIndices(numBlocks);
      for (uint i = 0; i < numBlocks; i++)
      {  
         const uint oldEntryIndex = (*pIndices1)[i];
         const uint newEntryIndex = selectorOrderOldToNew[oldEntryIndex];       

         newSelectorIndices.at(i) = (ushort)newEntryIndex;
      }
      pIndices1->swap(newSelectorIndices);
   }      

   const uint codebookSize = selectors.getSize();
   BBlockSelectorsArray newCodebook(codebookSize);
   for (uint newEntryIndex = 0; newEntryIndex < codebookSize; newEntryIndex++)
   {
      const uint oldEntryIndex = selectorOrder[newEntryIndex];

      newCodebook[newEntryIndex] = selectors[oldEntryIndex];
   }

   selectors.swap(newCodebook);
}



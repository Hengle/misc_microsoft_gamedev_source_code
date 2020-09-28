// File: DXTMunge.cpp
#include "compression.h"
#include "xcore.h"
#include "containers\dynamicarray.h"
#include <algorithm>

// Needed by the LBG quantizer.
#include <map>

#include "colorutils.h"
#include "RGBAImage.h"
#include "math\generalvector.h"
#include "DXTUtils.h"
#include "bytepacker.h"
#include "containers\priorityQueue.h"
#include "dxtmunge.h"
#include "LBGQuantizer.h"
#include "deflatecodec.h"
#include "DXTPacker.h"
#include "imageutils.h"
#include "JPEGCodec\jpgcodec.h"
#include "containers\bitArray2D.h"

//#define WRITE_SELECTOR_IMAGE
//#define WRITE_CODEBOOK_IMAGE      
//#define WRITE_BLOCK_DECOMP_IMAGES

namespace 
{
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

   class BHistogramSortFunctor
   {
   public:
      BHistogramSortFunctor(uint* pHist) : mpHist(pHist)
      {
      }

      bool operator() (uint i, uint j) const
      {
         return mpHist[i] > mpHist[j];
      }

   private:
      uint* mpHist;
   };
   
} // anonymous namespace

//------------------------------------------------------------------------------------------------------------------------------------------------------------
// BDXTMunger::createCodebook
//------------------------------------------------------------------------------------------------------------------------------------------------------------
uint BDXTMunger::createCodebook(
   const BRGBAImage& image,
   const BMethod2CompParams& params,
   const uint cellsX, const uint cellsY, const uint totalCells,
   const BDynamicPriorityQueue<BColorCellResults>& worstCells,
   const BByteArray& cellIndices,
   const BDynamicArray<uint>& cellColorDistance,
   const BByteArray& cellTypes,
   const BDynamicArray<DWORD>& packedCellColors,
   uint codebookSize, const bool virtualCodebook,
   BByteArray& compQuantizedCellIndices,
   BByteArray& compPackedCellRot,
   BByteArray& compCodebook)
{
   const uint MaxCodebookSize = 8192;
   if (codebookSize > MaxCodebookSize)
      codebookSize = MaxCodebookSize;

   const bool allowRotations = virtualCodebook;
   const bool allowFlips = virtualCodebook;
   BLBGQuantizer<BCodebookEntry, MaxCodebookSize>* pQuantizer = new BLBGQuantizer<BCodebookEntry, MaxCodebookSize>(allowRotations, allowFlips);

   const uchar toLinear4[cDXTColorSelectorValues] = { 3, 0, 2, 1 };
   const uchar toLinear3[cDXTColorSelectorValues] = { 2, 0, 1, 0 };

   //const uchar fromLinear4[cDXTColorSelectorValues] = { 1, 3, 2, 0 };
   //const uchar fromLinear3[cDXTColorSelectorValues] = { 1, 2, 0, 0 };

   const bool WeightTrainingVectors = true;
   double totalWeight = 0;
   enum { cMaxWeight = 256 };
   uint weightHist[cMaxWeight];
   memset(weightHist, 0, sizeof(weightHist));
      
   for (uint cy = 0; cy < cellsY; cy++)
   {
//      tracenocrlf("%i\n", cy);

      for (uint cx = 0; cx < cellsX; cx++)
      {
         BCodebookEntry entry;

         for (int sy = 0; sy < cDXTBlockSize; sy++)
         {
            for (int sx = 0; sx < cDXTBlockSize; sx++)
            {
               uint selector = (cellIndices[(cx + cy * cellsX) * cDXTBlockSize + (sy & cDXTBlockSizeMask)] >> ((sx & cDXTBlockSizeMask) * cDXTColorSelectorBits)) & cDXTColorSelectorMask;
               if (cellTypes[cx+cy*cellsX])
                  selector = toLinear3[selector];
               else
                  selector = toLinear4[selector];

               entry[sx + sy * cDXTBlockSize] = (selector + .5f) / 4.0f;
            }
         }

         uint colorDistance = cellColorDistance[cx + cy * cellsX];
         uint weight = colorDistance / 16384; //2048;//1024;
         weight = min(cMaxWeight, weight);
         weight = max(1, weight);
         if (!WeightTrainingVectors)
            weight = 1;
         weightHist[weight - 1]++;
         pQuantizer->updateHistogram(entry, weight);
         totalWeight += weight;
      }
   }

#if 0
   tracenocrlf("totalWeight: %f\n", totalWeight);
   for (uint i = 0; i < cMaxWeight; i++)
      tracenocrlf("%u ", weightHist[i]);
   tracenocrlf("\n");
#endif   

   pQuantizer->createCodebook(codebookSize, .0125f);

   uint actualCodebookSize = pQuantizer->getCodebookSize();

   BByteArray packedCellRot;
   packedCellRot.resize(cellsY * ((cellsX + 1) >> 1));

   BDynamicArray<uint> codebookEntryHist(pQuantizer->getCodebookSize());
   BDynamicArray<ushort> cellCodebookIndices(totalCells);

   BBitArray2D<1> cellsToRefine(cellsX, cellsY);
   
   for (uint i = 0; i < worstCells.size(); i++)
      cellsToRefine(worstCells[i].mX, worstCells[i].mY) = 1;
   
   for (uint cy = 0; cy < cellsY; cy++)
   {
      for (uint cx = 0; cx < cellsX; cx++)
      {
         if (cellsToRefine(cx, cy))
         {
            // Slow search
            const WORD packedLowColor = static_cast<WORD>(packedCellColors[cx+cy*cellsX] >> 16);
            const WORD packedHighColor = static_cast<WORD>(packedCellColors[cx+cy*cellsX] & 0xFFFF);

            BRGBAColor c[cDXTColorSelectorValues];
            BColorUtils::unpackColor(packedLowColor, c[0], true);
            BColorUtils::unpackColor(packedHighColor, c[3], true);

            uint numColors;
            const uchar* pToLinear;

            if (cellTypes[cx+cy*cellsX])
            {
               c[2] = c[3];

               c[1].r = (c[0].r + c[2].r) >> 1;
               c[1].g = (c[0].g + c[2].g) >> 1;
               c[1].b = (c[0].b + c[2].b) >> 1;

               numColors = 3;
               pToLinear = toLinear3;
            }
            else
            {
               c[1].r = (c[0].r * 2 + c[3].r) / 3;
               c[1].g = (c[0].g * 2 + c[3].g) / 3;
               c[1].b = (c[0].b * 2 + c[3].b) / 3;

               c[2].r = (c[3].r * 2 + c[0].r) / 3;
               c[2].g = (c[3].g * 2 + c[0].g) / 3;
               c[2].b = (c[3].b * 2 + c[0].b) / 3;

               numColors = 4;
               pToLinear = toLinear4;
            }

            BRGBAColor cellColors[cDXTBlockSize][cDXTBlockSize];

            for (int sy = 0; sy < cDXTBlockSize; sy++)
               for (int sx = 0; sx < cDXTBlockSize; sx++)
                  cellColors[sx][sy] = image(cx*cDXTBlockSize+sx, cy*cDXTBlockSize+sy);

            uint bestDist = UINT_MAX;
            uint bestCodebookEntryIndex = 0;
            uint bestCodebookRotFlipIndex = 0;

            for (uint codebookIndex = 0; codebookIndex < pQuantizer->getCodebookSize(); codebookIndex++)
            {
               const BCodebookEntry& sourceCodebookEntry(pQuantizer->getCodebookEntry(codebookIndex));

               for (uint rotFlipIndex = 0; rotFlipIndex < (virtualCodebook ? 8U : 1U); rotFlipIndex++)
               {
                  BCodebookEntry codebookEntry;

                  if (rotFlipIndex < 4)
                     sourceCodebookEntry.rotated(codebookEntry, rotFlipIndex, false, false);
                  else if (rotFlipIndex == 4)
                     sourceCodebookEntry.rotated(codebookEntry, 0, true, false);
                  else if (rotFlipIndex == 5)
                     sourceCodebookEntry.rotated(codebookEntry, 2, true, false);
                  else if (rotFlipIndex == 6)
                     sourceCodebookEntry.rotated(codebookEntry, 1, false, true);
                  else 
                     sourceCodebookEntry.rotated(codebookEntry, 3, false, true);

                  uint totalDist = 0;
                  for (int sy = 0; sy < cDXTBlockSize; sy++)
                  {
                     for (int sx = 0; sx < cDXTBlockSize; sx++)
                     {
                        uint selector = Math::Clamp<int>((int)(codebookEntry[sx + sy * cDXTBlockSize] * 4.0f), 0, numColors - 1);

                        const BRGBAColor& codebookColor = c[selector];

                        if (params.mPerceptual)
                           totalDist += BColorUtils::colorDistanceWeighted(codebookColor, cellColors[sx][sy]);
                        else
                           totalDist += BColorUtils::colorDistanceElucidian(codebookColor, cellColors[sx][sy]);
                     } // sx
                     if (totalDist > bestDist)
                        break;
                  } // sy

                  if (totalDist < bestDist)
                  {
                     bestDist = totalDist;
                     bestCodebookEntryIndex = codebookIndex;
                     bestCodebookRotFlipIndex = rotFlipIndex;
                  }

               } // rotFlipIndex
            } // codebookIndex

            codebookEntryHist[bestCodebookEntryIndex]++;
            cellCodebookIndices[cx + cy * cellsX] = static_cast<ushort>(bestCodebookEntryIndex);
            packedCellRot[(cx >> 1) + (cy * ((cellsX + 1) >> 1))] |= (bestCodebookRotFlipIndex << (cx & 1) * 3);
         }
         else
         {
            // Fast search
            BCodebookEntry entry;

            for (int sy = 0; sy < cDXTBlockSize; sy++)
            {
               for (int sx = 0; sx < cDXTBlockSize; sx++)
               {
                  uint selector = (cellIndices[(cx + cy * cellsX) * cDXTBlockSize + (sy & cDXTBlockSizeMask)] >> ((sx & cDXTBlockSizeMask) * cDXTColorSelectorBits)) & cDXTColorSelectorMask;

                  if (cellTypes[cx+cy*cellsX])
                     selector = toLinear3[selector];
                  else
                     selector = toLinear4[selector];

                  entry[sx + sy * cDXTBlockSize] = (selector + .5f) / 4.0f;
               }
            }

            uint rot;
            bool xFlip, yFlip;
            const uint entryIndex = pQuantizer->findBestCodebookEntry(entry, rot, xFlip, yFlip);
            codebookEntryHist[entryIndex]++;
            cellCodebookIndices[cx + cy * cellsX] = static_cast<ushort>(entryIndex);

            uint rotFlipIndex = rot;
            if (xFlip)
            {
               assert((rot == 0) || (rot == 2));
               rotFlipIndex = rot ? 5 : 4;
            }
            else if (yFlip)
            {
               assert((rot == 1) || (rot == 3));
               rotFlipIndex = (rot == 1) ? 6 : 7;
            }

            packedCellRot[(cx >> 1) + (cy * ((cellsX + 1) >> 1))] |= (rotFlipIndex << (cx & 1) * 3);
         } // cellsToRefine check               
      } // cx
   } // cy
      
   // Sort codebook by frequency   
   BDynamicArray<uint> sortedCodebookIndices(pQuantizer->getCodebookSize());
   BDynamicArray<uint> sortedCodebookIndicesInv(pQuantizer->getCodebookSize());

   for (uint i = 0; i < pQuantizer->getCodebookSize(); i++)
      sortedCodebookIndices[i] = i;
   std::sort(&sortedCodebookIndices[0], &sortedCodebookIndices[0] + pQuantizer->getCodebookSize(), BHistogramSortFunctor(&codebookEntryHist[0]));
   for (uint i = 0; i < pQuantizer->getCodebookSize(); i++)
      sortedCodebookIndicesInv[sortedCodebookIndices[i]] = i;

   BByteArray quantizedCellIndices(totalCells * 2);
   for (uint i = 0; i < totalCells; i++)
   {  
      const uint oldEntryIndex = cellCodebookIndices[i];
      const uint newEntryIndex = sortedCodebookIndicesInv[oldEntryIndex];       

      quantizedCellIndices.at(i) = static_cast<uchar>(newEntryIndex >> 8);
      quantizedCellIndices.at(totalCells + i) = static_cast<uchar>(newEntryIndex & 0xFF);
   }

   BByteArray codebook;
   codebook.resize(pQuantizer->getCodebookSize() * cDXTBlockSize);

   for (uint newEntryIndex = 0; newEntryIndex < pQuantizer->getCodebookSize(); newEntryIndex++)
   {
      const uint oldEntryIndex = sortedCodebookIndices[newEntryIndex];

      for (int v = 0; v < 16; v++)
      {
         int selector = (int)(pQuantizer->getCodebookEntry(oldEntryIndex)[v] * 4.0f);

         if (selector < 0) selector = 0; else if (selector > 3) selector = 3;

         codebook.at(newEntryIndex * cDXTBlockSize + (v >> cDXTBlockSizeBits)) |= (selector << ((v & cDXTBlockSizeMask) * cDXTColorSelectorBits));
      }
   }
   
   delete pQuantizer;  
   
   BDeflateCodec::deflateData(&quantizedCellIndices[0], quantizedCellIndices.size(), compQuantizedCellIndices);

   BDeflateCodec::deflateData(&codebook[0], codebook.size(), compCodebook);
   if (virtualCodebook)
      BDeflateCodec::deflateData(&packedCellRot[0], packedCellRot.size(), compPackedCellRot);
      
   return actualCodebookSize;
}      
      
//------------------------------------------------------------------------------------------------------------------------------------------------------------
// BDXTMunger::compressMethod2
//------------------------------------------------------------------------------------------------------------------------------------------------------------
bool BDXTMunger::compressMethod2(const BRGBAImage& image, const BMethod2CompParams& params, BByteArray& stream)
{
   int quality = params.mQuality;
   if (quality < 1) 
      quality = 85;
   else if (quality > 99)
      quality = 99;
            
   const uint width = image.getWidth();
   const uint height = image.getHeight();

   if ((width & cDXTBlockSizeMask) || (height & cDXTBlockSizeMask))
      return false;

   const uint cellsX = width >> cDXTBlockSizeBits;
   const uint cellsY = height >> cDXTBlockSizeBits;
   const uint totalCells = cellsX * cellsY;

   BRGBAImage blockImage0(cellsX, cellsY);
   BRGBAImage blockImage1(cellsX, cellsY);

   BByteArray cellTypes(totalCells);      

   BDynamicArray<BDXTUtils::BBlockPixel> blockColors(16);
   
   const uint numCellsToRefine = 1 + ((cellsX * cellsY) / 10);
   BDynamicPriorityQueue<BColorCellResults> worstCells(numCellsToRefine + 1);
      
   // Find block colors
   
   for (uint passIndex = 0; passIndex < 2; passIndex++)
   {
      const uint cellIterMax = passIndex ? worstCells.size() : (cellsX * cellsY);
   
      for (uint cellIter = 0; cellIter < cellIterMax; cellIter++)
      {
         uint cx, cy;
         if (passIndex)
         {
            cx = worstCells[cellIter].mX;
            cy = worstCells[cellIter].mY;
         }
         else
         {
            cx = cellIter % cellsX;
            cy = cellIter / cellsX;
         }
      
         WORD packedHighColor;
         WORD packedLowColor;
         uint bestType;
         
         const bool perceptualFlag = params.mPerceptual;
         const bool codeTransparentPixels = false;
         const bool useTransparentBlocks = true;
         
         uint bestVariance;
         const bool allTransparent = BDXTUtils::packDXT1Block(
            image, cx, cy,
            packedHighColor,
            packedLowColor,
            NULL, 
            bestType,
            perceptualFlag, 
            codeTransparentPixels, 
            NULL, 
            useTransparentBlocks, passIndex ? cDXTQualityNormal : cDXTQualityLowest, &bestVariance);
            
         if (!passIndex)
            worstCells.push(BColorCellResults(cx, cy, bestVariance));
            
         allTransparent;

         cellTypes[cx + cy * cellsX] = static_cast<uchar>(bestType);
         
         int t[3];
         BColorUtils::unpackColor(packedHighColor, t[0], t[1], t[2], true); 
         BRGBAColor a(t[0], t[1], t[2], 0);

         BColorUtils::unpackColor(packedLowColor, t[0], t[1], t[2], true); 
         BRGBAColor b(t[0], t[1], t[2], 0);

         if (params.mUseAveDeltaImages)
         {
            BRGBAColor lowColor, highColor;

            int ag = a.r + a.g + a.b;
            int bg = b.r + b.g + b.b;
            if (ag > bg)
            {
               highColor = a;
               lowColor = b;
            }
            else
            {
               highColor = b;
               lowColor = a;
            }

            blockImage0(cx, cy) = highColor;
            blockImage1(cx, cy) = lowColor;
         }
         else
         {
            blockImage0(cx, cy) = a;
            blockImage1(cx, cy) = b;
         }
      } // cellIter
      
   } // passIndex

   BByteArray blockImage0Comp;

   const bool useRGBJPEG = params.mPerceptual ? false : true;

   BJPEGCodec::eFormat jpegFormat;
       
   if (params.mGreyscale)
      jpegFormat = BJPEGCodec::cGreyscale;
   else if (useRGBJPEG)
      jpegFormat = BJPEGCodec::cH1V1_RGB;
   else
      jpegFormat = BJPEGCodec::cH1V1;
            
   if (!BJPEGCodec::compressImageJPEG(blockImage0, blockImage0Comp, quality, jpegFormat, params.mVisualQuant))
      return false;

   BRGBAImage blockImage0Decomp(cellsX, cellsY);   
   uint streamBytesRead;
   if (!BJPEGCodec::decompressImageJPEG(blockImage0Decomp, &blockImage0Comp[0], blockImage0Comp.size(), streamBytesRead, useRGBJPEG))
      return false;

   if (params.mUseAveDeltaImages)      
   {
      for (uint y = 0; y < cellsY; y++)
      {      
         for (uint x = 0; x < cellsX; x++)
         {
//-- FIXING PREFIX BUG ID 6248
            const BRGBAColor& highColor = blockImage0Decomp(x, y);
//--
//-- FIXING PREFIX BUG ID 6249
            const BRGBAColor& lowColor = blockImage1(x, y);
//--

            BRGBAColor deltaColor(
               (highColor.r - lowColor.r) / 2 + 128,
               (highColor.g - lowColor.g) / 2 + 128,
               (highColor.b - lowColor.b) / 2 + 128,
               0);

            blockImage1(x, y) = deltaColor;
         }
      }
   }      

   int image1Quality = quality;
   bool image1VisualQuant = params.mVisualQuant;
   if (params.mUseAveDeltaImages)
   {
      image1Quality = (quality * 105) / 100;
      if (image1Quality > 99) image1Quality = 99;
      image1VisualQuant = false;
   }

   BByteArray blockImage1Comp;
   if (!BJPEGCodec::compressImageJPEG(blockImage1, blockImage1Comp, image1Quality, jpegFormat, image1VisualQuant))
      return false;

   BRGBAImage blockImage1Decomp(cellsX, cellsY);

   if (!BJPEGCodec::decompressImageJPEG(blockImage1Decomp, &blockImage1Comp[0], blockImage1Comp.size(), streamBytesRead, useRGBJPEG))
      return false;

#ifdef WRITE_BLOCK_DECOMP_IMAGES
   BImageUtils::writeTGA24("0.tga", blockImage0);
   BImageUtils::writeTGA24("1.tga", blockImage1);
   BImageUtils::writeTGA24("0decomp.tga", blockImage0Decomp);
   BImageUtils::writeTGA24("1decomp.tga", blockImage1Decomp);
#endif   

   BByteArray cellIndices(totalCells * cDXTBlockSize);
   BDynamicArray<DWORD> packedCellColors(totalCells);
   BDynamicArray<uint> cellColorDistance(totalCells);

#ifdef WRITE_SELECTOR_IMAGE
   BRGBAImage selectorImage(cellsX*cDXTBlockSize, cellsY*cDXTBlockSize);
   BRGBAImage colorDistImage(cellsX*cDXTBlockSize, cellsY*cDXTBlockSize);
#endif   

   // Create selectors
   for (uint cy = 0; cy < cellsY; cy++)
   {
      for (uint cx = 0; cx < cellsX; cx++)
      {
         const BRGBAColor& color0 = blockImage0Decomp(cx, cy);
         const BRGBAColor& color1 = blockImage1Decomp(cx, cy);

         int hr, hg, hb;
         int lr, lg, lb;

         if (params.mUseAveDeltaImages)
         {
            int dr = (color1.r - 128) * 2;
            int dg = (color1.g - 128) * 2;
            int db = (color1.b - 128) * 2;

            hr = color0.r;
            hg = color0.g;
            hb = color0.b;

            lr = Math::Clamp(color0.r - dr, 0, 255);
            lg = Math::Clamp(color0.g - dg, 0, 255);
            lb = Math::Clamp(color0.b - db, 0, 255);
         }
         else
         {
            hr = color0.r;
            hg = color0.g;
            hb = color0.b;

            lr = color1.r;
            lg = color1.g;
            lb = color1.b;
         }

         cellColorDistance[cx + cy * cellsX] = BColorUtils::colorDistancePerceptual(lr, lg, lb, hr, hg, hb);

         WORD packedHighColor = BColorUtils::packColor(hr, hg, hb, true);
         WORD packedLowColor = BColorUtils::packColor(lr, lg, lb, true);

         uchar* pSelectors = &cellIndices[(cx + cy * cellsX) * cDXTBlockSize];

         if (packedLowColor == packedHighColor)
         {
            memset(pSelectors, 0, cDXTBlockSize);
            cellTypes[cx + cy * cellsX] = 0;

            packedCellColors[cx + cy * cellsX] = packedHighColor | (packedLowColor << 16);
         }
         else 
         {
            float bestTotalDist = 1e+30f;

            const int maxType = 2;
               
            for (int type = 0; type < maxType; type++)
            {
               int r[cDXTColorSelectorValues], g[cDXTColorSelectorValues], b[cDXTColorSelectorValues], numColors;
//-- FIXING PREFIX BUG ID 6250
               const uchar* pSelectorTransTable;
//--

               if (type)
               {
                  // 3 color block
                  if (packedHighColor > packedLowColor)
                     std::swap(packedHighColor, packedLowColor);

                  BColorUtils::unpackColor(packedLowColor, r[0], g[0], b[0], true);
                  BColorUtils::unpackColor(packedHighColor, r[2], g[2], b[2], true);

                  r[1] = (r[0] + r[2]) >> 1;
                  g[1] = (g[0] + g[2]) >> 1;
                  b[1] = (b[0] + b[2]) >> 1;

                  numColors = 3;

                  static uchar selectorTransTable3[] = { 1, 2, 0 };
                  pSelectorTransTable = selectorTransTable3;
               }
               else
               {
                  // 4 color block
                  if (packedHighColor < packedLowColor)
                     std::swap(packedHighColor, packedLowColor);

                  BColorUtils::unpackColor(packedLowColor, r[0], g[0], b[0], true);
                  BColorUtils::unpackColor(packedHighColor, r[3], g[3], b[3], true);

                  r[1] = (r[0] * 2 + r[3]) / 3;
                  g[1] = (g[0] * 2 + g[3]) / 3;
                  b[1] = (b[0] * 2 + b[3]) / 3;

                  r[2] = (r[3] * 2 + r[0]) / 3;
                  g[2] = (g[3] * 2 + g[0]) / 3;
                  b[2] = (b[3] * 2 + b[0]) / 3;

                  numColors = 4;

                  static uchar selectorTransTable4[] = { 1, 3, 2, 0 };
                  pSelectorTransTable = selectorTransTable4;
               }

               float totalDist = 0.0f;
               uchar selectors[cDXTBlockSize] = { 0, 0, 0, 0 };
               for (int sy = 0; sy < cDXTBlockSize; sy++)
               {
                  for (int sx = 0; sx < cDXTBlockSize; sx++)
                  {
                     const BRGBAColor& pixel = image(cx * cDXTBlockSize + sx, cy * cDXTBlockSize + sy);

                     int bestDist = INT_MAX;
                     int bestI = 0;

                     for (int i = 0; i < numColors; i++)
                     {
                        int dist;
                        if (params.mPerceptual)
                           dist = BColorUtils::colorDistanceWeighted(pixel.r, pixel.g, pixel.b, r[i], g[i], b[i]);
                        else
                           dist = BColorUtils::colorDistanceElucidian(pixel.r, pixel.g, pixel.b, r[i], g[i], b[i]);
                           
                        if (dist < bestDist)
                        {
                           bestDist = dist;
                           bestI = i;
                        }
                     }

                     selectors[sy] |= (pSelectorTransTable[bestI] << (sx * cDXTColorSelectorBits));      
                     totalDist += bestDist;
                  } // sx
               } // sy

               if (totalDist < bestTotalDist)
               {
                  bestTotalDist = totalDist;
                  cellTypes[cx + cy * cellsX] = static_cast<uchar>(type);
                  memcpy(pSelectors, selectors, cDXTBlockSize);
                  packedCellColors[cx + cy * cellsX] = packedHighColor | (packedLowColor << 16);
               }
            } // if solid block

         } // type                                 
                  
#ifdef WRITE_SELECTOR_IMAGE
{
         const uchar toLinear4[cDXTColorSelectorValues] = { 3, 0, 2, 1 };
         const uchar toLinear3[cDXTColorSelectorValues] = { 2, 0, 1, 0 };
         const uchar* pToLinear = cellTypes[cx + cy * cellsX] ? toLinear3 : toLinear4;
         BRGBAColor lowColor, highColor;
         BColorUtils::unpackColor((packedCellColors[cx+cy*cellsX]>>16) & 0xFFFF, lowColor, true);
         BColorUtils::unpackColor(packedCellColors[cx+cy*cellsX] & 0xFFFF, highColor, true);
         const BRGBAColor distColor(Math::Clamp(BColorUtils::colorDistancePerceptual(lowColor, highColor) / 4096, 0, 255));
         for (uint sy = 0; sy < cDXTBlockSize; sy++)
            for (uint sx = 0; sx < cDXTBlockSize; sx++)
            {
               selectorImage(cx*cDXTBlockSize+sx, cy*cDXTBlockSize+sy) = (pToLinear[(pSelectors[sy]>>(sx*cDXTColorSelectorBits)) & cDXTColorSelectorMask] * 255) / 3;
               colorDistImage(cx*cDXTBlockSize+sx, cy*cDXTBlockSize+sy) = distColor;
            }
}               
#endif   
         
      } // cx
   } // cy         

#ifdef WRITE_SELECTOR_IMAGE
   BImageUtils::writeTGA24("selectors.tga", selectorImage, false);
   BImageUtils::writeTGA24("colorDist.tga", colorDistImage, false);
#endif   
         
   BByteArray compCellIndices;   
   BDeflateCodec::deflateData(&cellIndices[0], cellIndices.size(), compCellIndices);
   
   BByteArray compPackedCellRot;
   BByteArray compCodebook;
   
   uint actualCodebookSize = 0;
   
   if (params.mCodebookSize)
   {
      BByteArray compQuantizedCellIndices;
      
      actualCodebookSize = createCodebook(
         image, params,
         cellsX, cellsY, totalCells, worstCells, cellIndices, cellColorDistance, cellTypes, packedCellColors, 
         params.mCodebookSize, params.mVirtualCodebook,
         compQuantizedCellIndices, compPackedCellRot, compCodebook);
               
      uint totalCodebookSize = compQuantizedCellIndices.size() + compCodebook.size() + compPackedCellRot.size();
      uint totalIndicesSize = compCellIndices.size();
      
      if (totalIndicesSize < totalCodebookSize)   
      {
         actualCodebookSize = 0;
         
         compCodebook.clear();
         compPackedCellRot.clear();
                  
         tracenocrlf("Using compressed indices instead of codebook (%i vs %i bytes)\n", totalIndicesSize, totalCodebookSize);
      }
      else
      {
         compCellIndices = compQuantizedCellIndices;
      }
   }

   BByteArray compCellTypes;
   BByteArray packedCellTypes(((cellsX + 7) >> 3) * cellsY);
   for (uint cy = 0; cy < cellsY; cy++)
      for (uint cx = 0; cx < cellsX; cx++)
         packedCellTypes[(cx >> 3) + (cy * ((cellsX + 7) >> 3))] |= (cellTypes[cx + cy * cellsX] << (cx & 7));

   BDeflateCodec::deflateData(&packedCellTypes[0], packedCellTypes.size(), compCellTypes);
         
   BByteArray compAlpha;
   if (params.mHasAlpha)
   {
      BByteArray packedAlpha;
      packedAlpha.resize(totalCells * 8);
      for (uint cy = 0; cy < cellsY; cy++)
      {
         for (uint cx = 0; cx < cellsX; cx++)
         {
            BDXTUtils::BDXT3Cell& cell = *reinterpret_cast<BDXTUtils::BDXT3Cell*>(&packedAlpha[(cx + cy * cellsX) * 8]);

            for (uint sy = 0; sy < cDXTBlockSize; sy++)
               for (uint sx = 0; sx < cDXTBlockSize; sx++)
                  cell.setAlpha(sx, sy, image(cx * cDXTBlockSize + sx, cy * cDXTBlockSize + sy).a, true);
         }
      }

      BDeflateCodec::deflateData(&packedAlpha[0], packedAlpha.size(), compAlpha);
   }

   BMungedDXTHeader header;
   memset(&header, 0, sizeof(header));
   header.mShortHeader.mMagic = static_cast<BYTE>(BMungedDXTHeader::cMagic);
   header.mShortHeader.mHeaderSize = sizeof(header);
   header.mShortHeader.mWidthLog2 = static_cast<BYTE>(Math::iLog2(width));
   header.mShortHeader.mHeightLog2 = static_cast<BYTE>(Math::iLog2(height));
   header.mShortHeader.mMethod = params.mUseAveDeltaImages ? 2 : 3;
   header.mShortHeader.mAlphaBytes = compAlpha.size();
   
   header.mRequiredVersion = 1;
   header.mCreatedVersion = cDXTMungerVersion;
   header.mJPEG0Bytes = blockImage0Comp.size();
   header.mJPEG1Bytes = blockImage1Comp.size();
   header.mCellIndicesBytes = compCellIndices.size();
   header.mCellTypeBytes = compCellTypes.size();
   header.mCellRotBytes = compPackedCellRot.size();
   header.mCodebookEntries = actualCodebookSize;
   header.mCodebookBytes = compCodebook.size();
   header.mFlags = static_cast<WORD>(useRGBJPEG ? 0 : BMungedDXTHeader::cYCbCrJPEG);
      
   const uint streamOfs = stream.size();      
   stream.pushBack((uchar*)&header, sizeof(header));
   if (blockImage0Comp.size())
      stream.pushBack(&blockImage0Comp[0], blockImage0Comp.size());
   if (blockImage1Comp.size())      
      stream.pushBack(&blockImage1Comp[0], blockImage1Comp.size());
   if (compCellIndices.size())
      stream.pushBack(&compCellIndices[0], compCellIndices.size());
   if (compCellTypes.size())
      stream.pushBack(&compCellTypes[0], compCellTypes.size());
   if (compPackedCellRot.size())
      stream.pushBack(&compPackedCellRot[0], compPackedCellRot.size());
   if (compCodebook.size())         
      stream.pushBack(&compCodebook[0], compCodebook.size());
   if (compAlpha.size())
      stream.pushBack(&compAlpha[0], compAlpha.size());
      
   bool useDXT1Compression = false;
   
   uint dxtmSize = (stream.size() - streamOfs) - compAlpha.size();
   DWORD dxt1Size;
   if (BDXTUtils::getSizeOfDXTData(dxt1Size, cDXT1, width, height))
   {
      dxt1Size += 8 + sizeof(BMungedDXTShortHeader);
      
      if (dxt1Size < dxtmSize)
      {
         useDXT1Compression = true;
         tracenocrlf("Using DXT1 instead of DXTM (%i vs. %i bytes)\n", dxt1Size, dxtmSize);
      }
   }
   
   if (useDXT1Compression)
   {
      stream.resize(streamOfs);
      
      BMungedDXTShortHeader shortHeader;
      shortHeader.mMagic = BMungedDXTShortHeader::cMagic;      
      shortHeader.mHeaderSize = sizeof(BMungedDXTShortHeader);
      shortHeader.mWidthLog2 = static_cast<BYTE>(Math::iLog2(width));
      shortHeader.mHeightLog2 = static_cast<BYTE>(Math::iLog2(height));
      shortHeader.mAlphaBytes = compAlpha.size();
      shortHeader.mMethod = 4;
      
      BByteArray DXT1Data;
      
      BDXTPacker packer;
      if (!packer.pack(image, cDXT1, cDXTQualityNormal, true, false, DXT1Data))
         return false;
       
      BByteArray compDXT1Data;
      
      BDeflateCodec::deflateData(DXT1Data.begin(), DXT1Data.size(), compDXT1Data);      
                  
      stream.pushBack((uchar*)&shortHeader, sizeof(shortHeader));   
      
      if (compAlpha.size())
         stream.pushBack(&compAlpha[0], compAlpha.size());
         
      stream.pushBack(compDXT1Data.begin(), compDXT1Data.size());

      *reinterpret_cast<BMungedDXTShortHeader*>(&stream[streamOfs]) = shortHeader;

      tracenocrlf("DXTM %ix%i, used DXT1 compression, Total Size: %i BPP: %f\n",
         width,
         height,
         stream.size() - streamOfs,
         ((stream.size() - streamOfs) * 8.0f) / (image.getWidth() * image.getHeight()) );
   }
   else
   {
      tracenocrlf("DXTM %ix%i, JPEG Size 0: %i, JPEG Size 1: %i, Total JPEG Size: %i, Total JPEG BPP: %f, Cell Indices Size: %i BPP: %f, Cell Rot Size: %i BPP: %f, Cell Types Size: %i BPP: %f, Codebook size: %i BPP: %f, Alpha Size: %i BPP: %f, Total BPP: %f\n", 
         width, height,
         blockImage0Comp.size(), blockImage1Comp.size(),
         blockImage0Comp.size() + blockImage1Comp.size(),
         ((blockImage0Comp.size() + blockImage1Comp.size()) * 8.0f) / (image.getWidth() * image.getHeight()),
         compCellIndices.size(), (compCellIndices.size() * 8.0f) / (image.getWidth() * image.getHeight()),
         compPackedCellRot.size(), (compPackedCellRot.size() * 8.0f) / (image.getWidth() * image.getHeight()),
         compCellTypes.size(), (compCellTypes.size() * 8.0f) / (image.getWidth() * image.getHeight()),
         compCodebook.size(), (compCodebook.size() * 8.0f) / (image.getWidth() * image.getHeight()),
         compAlpha.size(), (compAlpha.size() * 8.0f) / (image.getWidth() * image.getHeight()),
         ((stream.size() - streamOfs) * 8.0f) / (image.getWidth() * image.getHeight()) 
         );
   }         

   return true;         
}

//------------------------------------------------------------------------------------------------------------------------------------------------------------
// BDXTMunger::decompressMethod2DXTM
//------------------------------------------------------------------------------------------------------------------------------------------------------------
bool BDXTMunger::decompressMethod2DXTM(uchar* pDXTData, const BDXTFormat& dxtFormat, uint width, uint height, const uchar* pSrcData, uint srcDataSize)
{
   if (srcDataSize < sizeof(BMungedDXTHeader))
      return false;

   const BMungedDXTHeader& header = *reinterpret_cast<const BMungedDXTHeader*>(pSrcData); 

   if (BMungedDXTHeader::cMagic != header.mShortHeader.mMagic)
      return false;

   if (header.mShortHeader.mHeaderSize < sizeof(BMungedDXTHeader))
      return false;
      
   if ((width != (1U << header.mShortHeader.mWidthLog2)) || (height != (1U << header.mShortHeader.mHeightLog2)))
      return false;

   if (header.mRequiredVersion > cDXTMungerVersion) 
      return false;

   if ((header.mShortHeader.mMethod != 2) && (header.mShortHeader.mMethod != 3))
      return false;
      
   const bool useAveDeltaImages = (header.mShortHeader.mMethod == 2);

   const uint cellsX = width >> cDXTBlockSizeBits;
   const uint cellsY = height >> cDXTBlockSizeBits;
   const uint totalCells = cellsX * cellsY;

   if (srcDataSize < header.mShortHeader.mHeaderSize + header.mJPEG0Bytes + header.mJPEG1Bytes + header.mCellIndicesBytes + header.mCellTypeBytes + header.mCellRotBytes) 
      return false;

   BRGBAImage blockImage0(cellsX, cellsY);
   BRGBAImage blockImage1(cellsX, cellsY);

   LARGE_INTEGER JPEGStartTime;
   LARGE_INTEGER JPEGEndTime;
   QueryPerformanceCounter(&JPEGStartTime);

   const bool useRGBJPEG = (header.mFlags & BMungedDXTHeader::cYCbCrJPEG) == 0;

   uint jpegStreamBytesRead = 0;
   bool success = BJPEGCodec::decompressImageJPEG(blockImage0, pSrcData + header.mShortHeader.mHeaderSize, header.mJPEG0Bytes, jpegStreamBytesRead, useRGBJPEG);
   if (!success)
      return false;

   success = BJPEGCodec::decompressImageJPEG(blockImage1, pSrcData + header.mShortHeader.mHeaderSize + header.mJPEG0Bytes, header.mJPEG1Bytes, jpegStreamBytesRead, useRGBJPEG);
   if (!success)
      return false;
   
   QueryPerformanceCounter(&JPEGEndTime);
      
   LARGE_INTEGER InfStartTime;
   LARGE_INTEGER InfEndTime;

   QueryPerformanceCounter(&InfStartTime);
   
   BByteArray decompCellIndices; //(totalCells);
   const uchar* pCompCellIndices = pSrcData + header.mShortHeader.mHeaderSize + header.mJPEG0Bytes + header.mJPEG1Bytes;
   success = BDeflateCodec::inflateData(pCompCellIndices, header.mCellIndicesBytes, decompCellIndices);
   if (!success)
      return false;
   const uchar* pSelectorVectors = &decompCellIndices[0];

   BByteArray decompCellTypes; //(((cellsX + 7) >> 3) * cellsY);
   const uchar* pCompCellTypes = pCompCellIndices + header.mCellIndicesBytes;
   success = BDeflateCodec::inflateData(pCompCellTypes, header.mCellTypeBytes, decompCellTypes);
   if (!success)
      return false;
      
   BByteArray decompCellRot; //(totalCells);
   const uchar* pPackedCellRot = NULL;
   const uchar* pCompCellRot = pCompCellTypes + header.mCellTypeBytes;
   if (header.mCellRotBytes)
   {
      success = BDeflateCodec::inflateData(pCompCellRot, header.mCellRotBytes, decompCellRot);
      if (!success)
         return false;
      pPackedCellRot = &decompCellRot[0];
   }      

   BByteArray decompCodebook4(header.mCodebookEntries * cDXTBlockSize);
   BByteArray decompCodebook3(header.mCodebookEntries * cDXTBlockSize);
   const uchar* pCompCodebook = pCompCellRot + header.mCellRotBytes;
   if (header.mCodebookEntries)
   {
      if (header.mCodebookBytes == 0) 
         return false;

      BByteArray decompCodebook;//(header.mCodebookEntries * 4);
      
      success = BDeflateCodec::inflateData(pCompCodebook, header.mCodebookBytes, decompCodebook);
      if (!success)
         return false;
         
      if (decompCodebook.size() != header.mCodebookEntries * cDXTBlockSize)
         return false;
         
      for (uint i = 0; i < header.mCodebookEntries * cDXTBlockSize; i++)
      {
         const uchar fromLinear4[cDXTColorSelectorValues] = { 1, 3, 2, 0 };
         const uchar fromLinear3[cDXTColorSelectorValues] = { 1, 2, 0, 0 };

         for (uint v = 0; v < cDXTBlockSize; v++)
         {
            const uint selector = (decompCodebook[i] >> (v * cDXTColorSelectorBits)) & cDXTColorSelectorMask;
            decompCodebook3[i] |= (fromLinear3[selector] << (v * cDXTColorSelectorBits));
            decompCodebook4[i] |= (fromLinear4[selector] << (v * cDXTColorSelectorBits));
         }
      }

#ifdef WRITE_CODEBOOK_IMAGE      
      BRGBAImage image(1024, 1024);
      
      image.clear(BRGBAColor(10,20,10,0));
      
      for (uint y = 0; y < 128; y++)
      {
         for (uint x = 0; x < 128; x++)
         {
            uint c = x + y * 128;
            if (c >= header.mCodebookEntries)
               continue;
               
            const uchar* pCodebookEntry = &decompCodebook[c * cDXTBlockSize];
            for (uint sy = 0; sy < cDXTBlockSize; sy++)
            {
               for (uint sx = 0; sx < cDXTBlockSize; sx++)
               {
                  uint c = (pCodebookEntry[sy] >> (sx * cDXTColorSelectorBits)) & cDXTColorSelectorMask;
                  image(x*8+sx, y*8+sy).set(c * 255 / 3, 0);
               }
            }
         }
      }
      
      BImageUtils::writeTGA24("codebook.tga", image, false);
#endif
      
   }
  
   BByteArray packedAlpha;

   if (dxtFormat != cDXT1)
   {
      if (header.mShortHeader.mAlphaBytes)
      {            
         const uchar* pCompAlpha = pCompCodebook + header.mCodebookBytes;
         success = BDeflateCodec::inflateData(pCompAlpha, header.mShortHeader.mAlphaBytes, packedAlpha);
         if (!success)
            return false;
      }
      else
      {
         packedAlpha.resize(totalCells * 8);
         memset(&packedAlpha[0], 0xFF, packedAlpha.size());
      }
   }      
  
   QueryPerformanceCounter(&InfEndTime);

   LARGE_INTEGER DXTStartTime;
   LARGE_INTEGER DXTEndTime;
   QueryPerformanceCounter(&DXTStartTime);
      
   uint bytesPerBlock = 8;
   uint colorBlockOfs = 0;

   if (dxtFormat == cDXT3)
   {
      bytesPerBlock = 16;
      colorBlockOfs = 8;
   }
   
   const uint numCodebookEntries = header.mCodebookEntries;

#ifdef WRITE_SELECTOR_IMAGE
   BRGBAImage selectorImage(cellsX*cDXTBlockSize, cellsY*cDXTBlockSize);
#endif   

   for (uint cy = 0; cy < cellsY; cy++)
   {
      for (uint cx = 0; cx < cellsX; cx++)
      {
         if (dxtFormat == cDXT3)
            memcpy(pDXTData + (cx + cy * cellsX) * cDXT35NBlockBytes, &packedAlpha[(cx + (cy * cellsX)) * 8], 8);
            
         BDXTUtils::BDXT1Cell& cell = *reinterpret_cast<BDXTUtils::BDXT1Cell*>(pDXTData + (cx + cy * cellsX) * bytesPerBlock + colorBlockOfs);
         uchar* pSelectors = &cell.mPixels0;

         const BRGBAColor& color0 = blockImage0(cx, cy);
         const BRGBAColor& color1 = blockImage1(cx, cy);

         int hr, hg, hb;
         int lr, lg, lb;

         if (useAveDeltaImages)
         {
            int dr = (color1.r - 128) * 2;
            int dg = (color1.g - 128) * 2;
            int db = (color1.b - 128) * 2;

            hr = color0.r;
            hg = color0.g;
            hb = color0.b;

            lr = color0.r - dr;
            lg = color0.g - dg;
            lb = color0.b - db;
         }
         else
         {
            hr = color0.r;
            hg = color0.g;
            hb = color0.b;

            lr = color1.r;
            lg = color1.g;
            lb = color1.b;
         }            

         WORD packedColor0 = BColorUtils::packColor(hr, hg, hb, true);
         WORD packedColor1 = BColorUtils::packColor(lr, lg, lb, true);

         const uint cellType = (decompCellTypes[(cx >> 3) + cy * ((cellsX + 7) >> 3)] >> (cx & 7)) & 1;

         if (packedColor0 == packedColor1)
         {
            memset(pSelectors, 0, cDXTBlockSize);
         }
         else
         {
            if (cellType)
            {
               if (packedColor0 > packedColor1)
                  std::swap(packedColor0, packedColor1);
            }
            else
            {
               if (packedColor0 < packedColor1)
                  std::swap(packedColor0, packedColor1);
            }

            if (numCodebookEntries)
            {
               const uint codebookEntry = (pSelectorVectors[cx + cy * cellsX] << 8) | pSelectorVectors[totalCells + (cx + cy * cellsX)];
               uint rotIndex = 0;
               if (header.mCellRotBytes)
                  rotIndex = (pPackedCellRot[cy * ((cellsX + 1) >> 1) + (cx >> 1)] >> ((cx & 1) * 3)) & 7;

               static const DWORD table[8] = 
               {
                  0x00000000,
                  0x00000001,
                  0x00000002,
                  0x00000003,

                  0x00000100,
                  0x00000102,

                  0x00010001,
                  0x00010003,
               };

               const uint xFlip = table[rotIndex] & 0x100;
               const uint yFlip = table[rotIndex] & 0x10000;
               const uint rotation = table[rotIndex] & 0x3;

//-- FIXING PREFIX BUG ID 6253
               const uchar* pSrc;                                    
//--
               if (cellType)
                  pSrc = &decompCodebook3[codebookEntry * cDXTBlockSize];
               else
                  pSrc = &decompCodebook4[codebookEntry * cDXTBlockSize];

               if ((0 == rotation) && (!xFlip) && (!yFlip))
                  memcpy(pSelectors, pSrc, cDXTBlockSize);
               else
               {
                  uchar tmp[cDXTBlockSize];

                  if (xFlip)
                  {
                     assert(!yFlip);
                     tmp[0] = 0;
                     tmp[1] = 0;
                     tmp[2] = 0;
                     tmp[3] = 0;
                     for (uint y = 0; y < cDXTBlockSize; y++)
                     {
                        for (uint x = 0; x < cDXTBlockSize; x++)
                        {
                           const uint v = (pSrc[y] >> (cDXTColorSelectorBits * (cDXTBlockSizeMask - x))) & cDXTColorSelectorMask;
                           tmp[y] |= (v << (x * cDXTColorSelectorBits));
                        }
                     }
                     pSrc = tmp;
                  }
                  else if (yFlip)
                  {
                     assert(!xFlip);
                     tmp[0] = pSrc[3];
                     tmp[1] = pSrc[2];
                     tmp[2] = pSrc[1];
                     tmp[3] = pSrc[0];
                     pSrc = tmp;
                  }

                  switch (rotation)
                  {
                     case 0:
                     {
                        memcpy(pSelectors, pSrc, cDXTBlockSize);
                        break;
                     }
                     case 1:
                     {
                        memset(pSelectors, 0, cDXTBlockSize);
                        for (uint y = 0; y < cDXTBlockSize; y++)
                        {
                           for (uint x = 0; x < cDXTBlockSize; x++)
                           {
                              const uint v = (pSrc[x] >> (cDXTColorSelectorBits * (cDXTBlockSizeMask - y))) & cDXTColorSelectorMask;
                              pSelectors[y] |= (v << (x * cDXTColorSelectorBits));
                           }
                        }
                        break;
                     }
                     case 2:
                     {
                        memset(pSelectors, 0, cDXTBlockSize);
                        for (uint y = 0; y < cDXTBlockSize; y++)
                        {
                           for (uint x = 0; x < cDXTBlockSize; x++)
                           {
                              const uint v = (pSrc[cDXTBlockSizeMask - y] >> (cDXTColorSelectorBits * (cDXTBlockSizeMask - x))) & cDXTColorSelectorMask;
                              pSelectors[y] |= (v << (x * cDXTColorSelectorBits));
                           }
                        }
                        break;
                     }
                     case 3:
                     {
                        memset(pSelectors, 0, cDXTBlockSize);
                        for (uint y = 0; y < cDXTBlockSize; y++)
                        {
                           for (uint x = 0; x < cDXTBlockSize; x++)
                           {
                              const uint v = (pSrc[cDXTBlockSizeMask - x] >> (cDXTColorSelectorBits * y)) & cDXTColorSelectorMask;
                              pSelectors[y] |= (v << (x * cDXTColorSelectorBits));
                           }
                        }
                        break;
                     }
                  }                     
               }
            }
            else
            {
               memcpy(pSelectors, pSelectorVectors + (cx + cy * cellsX) * cDXTBlockSize, cDXTBlockSize);
            }               
         }

#ifdef WRITE_SELECTOR_IMAGE
{
         const uchar toLinear4[cDXTColorSelectorValues] = { 3, 0, 2, 1 };
         const uchar toLinear3[cDXTColorSelectorValues] = { 2, 0, 1, 0 };
         const uchar* pToLinear = cellType ? toLinear3 : toLinear4;
         for (uint sy = 0; sy < cDXTBlockSize; sy++)
            for (uint sx = 0; sx < cDXTBlockSize; sx++)
               selectorImage(cx*cDXTBlockSize+sx, cy*cDXTBlockSize+sy) = (pToLinear[(pSelectors[sy]>>(sx*cDXTColorSelectorBits)) & cDXTColorSelectorMask] * 255) / 3;
}               
#endif      

         cell.setColor0(packedColor0);
         cell.setColor1(packedColor1);

      }  // cx
   } // cy

#ifdef WRITE_SELECTOR_IMAGE
   BImageUtils::writeTGA24("qselectors.tga", selectorImage, false);
#endif   

   QueryPerformanceCounter(&DXTEndTime);

   LARGE_INTEGER freq;
   QueryPerformanceFrequency(&freq);

   DWORD JPEGTime = static_cast<DWORD>(((*(unsigned __int64*)&JPEGEndTime - *(unsigned __int64*)&JPEGStartTime) * 1000) / *(unsigned __int64*)&freq);
   DWORD DXTTime = static_cast<DWORD>(((*(unsigned __int64*)&DXTEndTime - *(unsigned __int64*)&DXTStartTime) * 1000) / *(unsigned __int64*)&freq);
   DWORD InfTime = static_cast<DWORD>(((*(unsigned __int64*)&InfEndTime - *(unsigned __int64*)&InfStartTime) * 1000) / *(unsigned __int64*)&freq);

   tracenocrlf("JPEG Decompression Time: %ums, DXT1 Create Time: %ums, Inflate Time: %ums\n", JPEGTime, DXTTime, InfTime);

   return true;
}

//------------------------------------------------------------------------------------------------------------------------------------------------------------
// BDXTMunger::decompressMethod2DXT1
//------------------------------------------------------------------------------------------------------------------------------------------------------------
bool BDXTMunger::decompressMethod2DXT1(uchar* pDXTData, const BDXTFormat& dxtFormat, uint width, uint height, const uchar* pSrcData, uint srcDataSize)
{
   const BMungedDXTShortHeader& header = *reinterpret_cast<const BMungedDXTShortHeader*>(pSrcData); 

   if (BMungedDXTShortHeader::cMagic != header.mMagic) 
      return false;
   
   if (header.mHeaderSize < sizeof(BMungedDXTShortHeader))
      return false;

   if ((width != (1U << header.mWidthLog2)) || (height != (1U << header.mHeightLog2)))
      return false;
   
   if (header.mMethod != 4)
      return false;
      
   const uint cellsX = width >> cDXTBlockSizeBits;
   const uint cellsY = height >> cDXTBlockSizeBits;
   const uint totalCells = cellsX * cellsY;

   BByteArray decompDXT1Data;
   if (!BDeflateCodec::inflateData(pSrcData + header.mHeaderSize + header.mAlphaBytes, srcDataSize - sizeof(BMungedDXTShortHeader) - header.mAlphaBytes, decompDXT1Data))
      return false;
   
   if (decompDXT1Data.size() != (totalCells * 8))
      return false;

   BByteArray packedAlpha;

   if (dxtFormat != cDXT1)
   {
      if (header.mAlphaBytes)
      {            
         const uchar* pCompAlpha = pSrcData + header.mHeaderSize;
         bool success = BDeflateCodec::inflateData(pCompAlpha, header.mAlphaBytes, packedAlpha);
         if (!success)
            return false;
      }
      else
      {
         packedAlpha.resize(totalCells * 8);
         memset(&packedAlpha[0], 0xFF, packedAlpha.size());
      }
   }      

   uint bytesPerBlock = 8;
   uint colorBlockOfs = 0;

   if (dxtFormat == cDXT3)
   {
      bytesPerBlock = 16;
      colorBlockOfs = 8;
   }

   for (uint cy = 0; cy < cellsY; cy++)
   {
      for (uint cx = 0; cx < cellsX; cx++)
      {
         if (dxtFormat == cDXT3)
            memcpy(pDXTData + (cx + cy * cellsX) * 16, &packedAlpha[(cx + (cy * cellsX)) * 8], 8);

         BDXTUtils::BDXT1Cell& cell = *reinterpret_cast<BDXTUtils::BDXT1Cell*>(pDXTData + (cx + cy * cellsX) * bytesPerBlock + colorBlockOfs);
         
         memcpy(&cell, &decompDXT1Data[(cx + cy * cellsX) * 8], 8);
      }  // cx
   } // cy

   return true;   
}

//------------------------------------------------------------------------------------------------------------------------------------------------------------
// BDXTMunger::decompressMethod2
//------------------------------------------------------------------------------------------------------------------------------------------------------------
bool BDXTMunger::decompressMethod2(uchar* pDXTData, const BDXTFormat& dxtFormat, uint width, uint height, const uchar* pSrcData, uint srcDataSize)
{
   if (srcDataSize < sizeof(BMungedDXTShortHeader))
      return false;

   if ((dxtFormat != cDXT1) && (dxtFormat != cDXT1A) && (dxtFormat != cDXT3))
      return false;
   
   if (pSrcData[0] == BMungedDXTShortHeader::cMagic)
      return decompressMethod2DXT1(pDXTData, dxtFormat, width, height, pSrcData, srcDataSize);
   else if (pSrcData[0] == BMungedDXTHeader::cMagic)
      return decompressMethod2DXTM(pDXTData, dxtFormat, width, height, pSrcData, srcDataSize);
      
   return false;
}

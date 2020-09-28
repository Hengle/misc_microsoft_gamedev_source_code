//============================================================================
//
// File: DXTShrinker.h
// Copyright (c) 2005-2006, Ensemble Studios
//
//============================================================================
#pragma once 
#include "DXTUtils.h"
#include "WeightedMinMaxQuantizer.h"
#include "containers\hashmap.h"
#include "containers\priorityQueue.h"

//============================================================================
// class BDXTShrinker
//============================================================================
class BDXTShrinker
{
   BDXTShrinker(const BDXTShrinker&);
   BDXTShrinker& operator= (const BDXTShrinker&);
   
public:
   struct BDXTBlockPixels
   {
      BRGBAColor mPixels[4][4];     // [y][x]
   };
   
   typedef BDynamicArray<BDXTBlockPixels> BDXTBlockPixelsArray;
   
   struct BParams
   {
      BParams() :
         mMaxQuantColors(1024),
         mMaxAlphaColors(1024),
         mColorSelectorCodebookSize(2048),
         mAlphaSelectorCodebookSize(2048),
         mPerceptual(true),
         mFavorLargerAlpha(false)
      {
      }

      uint mMaxQuantColors;
      uint mMaxAlphaColors;
      uint mColorSelectorCodebookSize;
      uint mAlphaSelectorCodebookSize;
      bool mPerceptual;
      bool mFavorLargerAlpha;
   };
   
   // Formats: cDXT1, cDXT5, cDXTN
   BDXTShrinker(BDXTFormat dxtFormat, uint numBlocks, const BDXTBlockPixels* pInputBlocks, const BParams& params = BParams());
      
   struct BPackedColors
   {
      uint mValues[2];

      BPackedColors() { }

      BPackedColors(uint low, uint high)
      {
         mValues[0] = low;
         mValues[1] = high;
      }

      bool operator== (const BPackedColors& rhs) const
      {
         return std::equal(mValues, mValues + 2, rhs.mValues);
      }

      bool operator< (const BPackedColors& rhs) const
      {
         return std::lexicographical_compare(mValues, mValues + 2, rhs.mValues, rhs.mValues + 2);
      }

      operator size_t() const
      {
         return hashFast(mValues, sizeof(mValues));
      }

      void setLow(uint v) { mValues[0] = v; }
      void setHigh(uint v) { mValues[1] = v; }

      uint getLow(void) const { return mValues[0]; }
      uint getHigh(void) const { return mValues[1]; }
   };

   typedef BDynamicArray<BPackedColors> BPackedColorsArray;
   
   // Cell colors
   
   // Color codebook: 32-bits/entry (Low 565, High 565)
   const BPackedColorsArray& getColorCodebook(void) const { return mQuantizedPackedColors; }
   const UShortArray& getColorIndices(void) const { return mColorIndices; }
   
   // Alpha codebook: 16-bits/entry (Low 8, High 8)
   const BPackedColorsArray& getAlphaCodebook(void) const { return mQuantizedPackedAlpha; }
   const UShortArray& getAlphaIndices0(void) const { return mAlphaIndices0; }
   const UShortArray& getAlphaIndices1(void) const { return mAlphaIndices1; }
   
   // Cell selectors - 32-bits (4*4*2) or 48-bits (4*4*3) 
   struct BBlockSelectors
   {
      uchar mSelectors[4][4]; // [y][x]
   };
   typedef BDynamicArray<BBlockSelectors> BBlockSelectorsArray;
   
   const BBlockSelectorsArray&   getColorSelectorCodebook(void) const { return mColorSelectorCodebook; }
   const UShortArray&            getColorSelectorIndices(void) const { return mColorSelectorIndices; }
      
   const BBlockSelectorsArray&   getAlphaSelectorCodebook(void) const { return mAlphaSelectorCodebook; }
   const UShortArray&            getAlphaSelectorIndices(void) const { return mAlphaSelectorIndices; }
      
   BDXTFormat                    getDXTFormat(void) const { return mDXTFormat; }
   uint                          getNumBlocks(void) const { return mNumBlocks; }
   const BDXTBlockPixels*        getInputBlocks(void) const { return mpInputBlocks; }
               
private:
   BDXTFormat mDXTFormat;
   uint mNumBlocks;
   const BDXTBlockPixels* mpInputBlocks;
   
   BParams mParams;
   
   // Colors
   BPackedColorsArray mQuantizedPackedColors;
   UShortArray mColorIndices;
   
   // Alpha
   BPackedColorsArray mQuantizedPackedAlpha;
   UShortArray mAlphaIndices0;
   UShortArray mAlphaIndices1;
   
   // Color selectors
   UShortArray mColorSelectorIndices;
   BBlockSelectorsArray mColorSelectorCodebook;
   
   // Alpha selectors
   UShortArray mAlphaSelectorIndices;
   BBlockSelectorsArray mAlphaSelectorCodebook;
   
   struct BColorHashCellData
   {
      uint mWeight;
      uint mUniqueColorIndex;

      BColorHashCellData() { }
      BColorHashCellData(uint weight, uint uniqueColorIndex) : mWeight(weight), mUniqueColorIndex(uniqueColorIndex) { }
   };

   typedef BHashMap<BPackedColors, BColorHashCellData> BPackedColorHashMap;

   class BColorCandidate
   {
   public:
      BColorCandidate() { }

      BColorCandidate(uint cellIndex, uint dist) :
         mCellIndex(cellIndex),
         mDist(dist)
      {
      }

      bool operator== (const BColorCandidate& rhs) const
      {
         return mDist == rhs.mDist;
      }

      bool operator< (const BColorCandidate& rhs) const
      {
         return mDist > rhs.mDist;   
      }

      uint mCellIndex;
      uint mDist;
   };
   
   enum { cMaxCellCandidateColors = 16 };

   typedef BStaticPriorityQueue<BColorCandidate, cMaxCellCandidateColors> BColorCandidateQueue;
   typedef BDynamicArray<BColorCandidateQueue> BCellColorCandidateQueues;
            
   typedef BVecN<6, float> BVec6;
   typedef Weighted_MinMax_Quantizer<BVec6> BVec6Quantizer;
         
   void findUniqueColors(BPackedColorHashMap& colorHash, BDynamicArray<BPackedColors>& origBlockColors, int compIndex);
            
   void quantizeColors(
      uint maxQuantColors,
      BVec6Quantizer& quantizer, 
      BPackedColorsArray& quantizedPackedColors,
      const BPackedColorHashMap& colorHash, 
      bool alphaBlocks);

   void computeCellCandidates(
      BCellColorCandidateQueues& cellColorCandidateQueues,
      const BPackedColorsArray& quantizedPackedColors,
      const BPackedColorHashMap& colorHash,
      bool alphaBlock);

   uint computeColorBlockIndex(
      uint& bestError,
      BBlockSelectors* pBestSelectors,
      uint blockIndex,
      const BDynamicArray<BPackedColors>& origBlockColors, 
      const BCellColorCandidateQueues& cellColorCandidateQueues,
      const BPackedColorsArray& quantizedPackedColors,
      const BPackedColorHashMap& colorHash);
      
   uint computeAlphaBlockIndex(
      uint& bestError,
      BBlockSelectors* pBestSelectors,
      uint blockIndex, uint compIndex,
      const BDynamicArray<BPackedColors>& origBlockColors, 
      const BCellColorCandidateQueues& cellColorCandidateQueues,
      const BPackedColorsArray& quantizedPackedColors,
      const BPackedColorHashMap& colorHash);

   void createSelectorCodebook(
      UShortArray& blockIndices, BBlockSelectorsArray& codebook,
      bool alphaBlocks,
      const BBlockSelectorsArray& blockSelectors,
      uint codebookSize,
      const BDynamicArray<BPackedColors>* pOrigBlockColors, bool weightByBlockColorDist);
      
   void refineColorCodebook(BBlockSelectorsArray& blockSelectors);
               
   int colorDistance(const BRGBAColor& a, const BRGBAColor& b);      
   
   static uint colorDistFunc(BDXTShrinker& shrinker, const BPackedColors& a, const BPackedColors& b);
   static uint alphaDistFunc(BDXTShrinker& shrinker, const BPackedColors& a, const BPackedColors& b);
   
   typedef uint (*BColorDistFunc)(BDXTShrinker& shrinker, const BPackedColors& a, const BPackedColors& b);
   void sortColorsBySimilarity(BPackedColorsArray& colors, UShortArray& indices0, UShortArray* pIndices1, BColorDistFunc pColorDistFunc);
   
   void sortSelectorsBySimilarity(BBlockSelectorsArray& selectors, UShortArray& indices0, UShortArray* pIndices1);
};






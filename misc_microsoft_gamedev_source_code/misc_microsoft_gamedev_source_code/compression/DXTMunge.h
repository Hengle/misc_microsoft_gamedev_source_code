// File: DXTMunger.h
#pragma once

#include "containers/priorityQueue.h"

class BDXTMunger
{
public:
   enum 
   { 
      cDXTMungerVersion = 1 
   };
                     
   static bool compressMethod1(const BRGBAImage& image, bool hasAlpha, uint quality, BByteArray& stream);

   struct BMethod2CompParams
   {
      BMethod2CompParams() :
         mHasAlpha(false),
         mGreyscale(false),
         mQuality(85),
         mCodebookSize(0),
         mVirtualCodebook(true),
         mUseAveDeltaImages(true),
         mVisualQuant(false),
         mPerceptual(true)
      {
      }
            
      bool mHasAlpha;
      bool mGreyscale;
      uint mQuality;
      uint mCodebookSize;
      bool mVirtualCodebook;
      bool mUseAveDeltaImages;
      bool mVisualQuant;
      bool mPerceptual;
   };
   
   static bool compressMethod2(const BRGBAImage& image, const BMethod2CompParams& params, BByteArray& stream);
  
   // if dxtFormat is cDXTInvalid, the decompressor decides what dxt format is best.
   static bool decompress(uchar* pDXTData, const BDXTFormat& dxtFormat, uint width, uint height, const uchar* pSrcData, uint srcDataSize);
   
   static bool imageHasAlphaData(const uchar* pSrcData, uint srcDataSize);
         
private:
   
   // Should be 8 bytes
   struct BMungedDXTShortHeader
   {
      enum { cMagic = 0x77 };

      BYTE mMagic;
      BYTE mHeaderSize;
      
      enum eFormat
      {
         cFormatDXT1Deflated = 0,
         
         cFormatTotal
      };
      
      BYTE mMethod;
      BYTE mWidthLog2;
      BYTE mHeightLog2;
      
      BPacked24 mAlphaBytes;
   };      
      
   // Should be 32 bytes
   struct BMungedDXTHeader
   {
      enum { cMagic = 0xAC };
      
      BMungedDXTShortHeader mShortHeader;
      
      BYTE mRequiredVersion;
      BYTE mCreatedVersion;
                        
      BPacked24 mJPEG0Bytes;
      BPacked24 mJPEG1Bytes;
      
      BPacked24 mCellIndicesBytes;
      BPacked24 mCellTypeBytes;
      BPacked24 mCellRotBytes;
            
      BPacked24 mCodebookBytes;
            
      BPacked16 mCodebookEntries;
      
      enum eFlags
      {
         cYCbCrJPEG = (1 << 0),
      };
      
      BPacked16 mFlags;
   };   

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
    
   static uint createCodebook(
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
      BByteArray& compCodebook);
   
   static bool decompressMethod1(uchar* pDXTData, const BDXTFormat& dxtFormat, uint width, uint height, const uchar* pSrcData, uint srcDataSize);
   static bool decompressMethod2(uchar* pDXTData, const BDXTFormat& dxtFormat, uint width, uint height, const uchar* pSrcData, uint srcDataSize);
   static bool decompressMethod2DXTM(uchar* pDXTData, const BDXTFormat& dxtFormat, uint width, uint height, const uchar* pSrcData, uint srcDataSize);
   static bool decompressMethod2DXT1(uchar* pDXTData, const BDXTFormat& dxtFormat, uint width, uint height, const uchar* pSrcData, uint srcDataSize);
      
   static void pack24(BYTE* pDst, uint i);
   static uint unpack24(const BYTE* pSrc);
   
   static void pack16(BYTE* pDst, uint i);
   static uint unpack16(const BYTE* pSrc);
};





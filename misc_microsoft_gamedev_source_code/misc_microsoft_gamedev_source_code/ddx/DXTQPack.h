//==============================================================================
// File: DXTQPack.h
//
// Copyright (c) 2006 Ensemble Studios
//==============================================================================
#pragma once 

#include "DXTShrinker.h"
#include "DDXUtils.h"
#include "xboxDataComposers.h"
#include "bitCoder.h"
#include "DXTQTypes.h"

//==============================================================================
// class BDXTQPack
//==============================================================================
class BDXTQPack
{
public:
   BDXTQPack();
      
   bool packRGBAToDXTQ(
      const BDDXTextureInfo& srcTextureInfo, const BYTE* pSrcData, uint srcDataSize,
      const BDDXPackParams& options, float& hdrScale,
      BByteArray& dstData);
           
private:
   BXboxMemoryPool mCachedMemPool;
         
   bool createXboxPackGuideTexture(
      uint width, uint height,
      uint numMipChainLevels, 
      D3DFORMAT d3dFormat,
      const uint bytesPerBlock,
      IDirect3DTexture9& d3dTex,
      BByteArray& packGuideTex,
      UINT& d3dTexBaseSize,
      UINT& d3dTexMipSize);

   void computeBlockPixels(
      const uint width, const uint height,
      const uint numMipChainLevels,
      const uint bytesPerBlock,
      const uint totalBlocks,
      const BYTE* pSrcData, uint srcDataSize,
      const BByteArray& packGuideTex,
      BDXTShrinker::BDXTBlockPixelsArray& blockPixels);
   
   void createColorCodebook(
      BDXTQHeader& header,
      const BDXTShrinker::BPackedColorsArray& colorCodebook,
      const BDXTShrinker::BBlockSelectorsArray& colorSelectorCodebook);
   
   void createAlphaCodebook(
      BDXTQHeader& header,
      const BDXTShrinker::BPackedColorsArray& alphaCodebook,
      const BDXTShrinker::BBlockSelectorsArray& alphaSelectorCodebook);
         
   void encodeIndices(
      BDXTQHeader& header,
      const UShortArray& colorIndices,
      const UShortArray& colorSelectorIndices,
      const UShortArray& alphaIndices0,
      const UShortArray& alphaIndices1,
      const UShortArray& alphaSelectorIndices);
   
   bool packDXTQ32(
      const BDDXTextureInfo& srcTextureInfo, const BYTE* pSrcData, uint srcDataSize,
      const BDDXPackParams& options, 
      BByteArray& dstData);
   
   bool packDXTQ64(
      const BDDXTextureInfo& srcTextureInfo, const BYTE* pSrcData, uint srcDataSize,
      const BDDXPackParams& options, float& hdrScale,
      BByteArray& dstData);
           
   struct BBlockIndices
   {
      int mRunLength;
      ushort mColorIndex;
      ushort mColorSelectorIndex;
      ushort mAlpha0Index;
      ushort mAlpha0SelectorIndex;
      ushort mAlpha1Index;
      ushort mAlpha1SelectorIndex;

      void clear(void)
      {
         Utils::ClearObj(*this);
      }
   };

   typedef BDynamicArray<BBlockIndices> BBlockIndicesArray;
        
   void createBlockIndices(
      BBlockIndicesArray& blockIndices,
      uint firstBlock, uint numBlocks,
      BDXTQHeader& header,
      const UShortArray& colorIndices,
      const UShortArray& colorSelectorIndices,
      const UShortArray& alphaIndices0,
      const UShortArray& alphaIndices1,
      const UShortArray& alphaSelectorIndices,
      bool allowRuns);   
};


//==============================================================================
// File: DXTQPack.h
//
// Copyright (c) 2006 Ensemble Studios
//==============================================================================
#include "xcore.h"
#include "math\halfFloat.h"
#include "file\ecfUtils.h"
#include "stream\cfileStream.h"
#include "Timer.h"

#include "DXTQPack.h"

#include "compression.h"

#include "colorutils.h"
#include "RGBAImage.h"
#include "imageutils.h"

#include "DDXUtils.h"
#include "DDXSerializedTexture.h"
#include "HDRCodec.h"
#include "HDRUtils.h"
#include "DXTUnpacker.h"

#define MAX_RUN_CODES 16
#define MIN_RUN_SIZE 2
#define MAX_RUN_SIZE (MAX_RUN_CODES - 1 + MIN_RUN_SIZE)

#define TRACE_RECORDING 1
#if TRACE_RECORDING
#include "tracerecording.h"
#pragma comment( lib, "tracerecording.lib" )
#pragma comment(lib, "xbdm.lib")
#endif

//------------------------------------------------------------------------------------------------------
// BDXTQPack::createXboxPackGuideTexture
//------------------------------------------------------------------------------------------------------ 
bool BDXTQPack::createXboxPackGuideTexture(
   uint width, uint height,
   uint numMipChainLevels, 
   D3DFORMAT d3dFormat,
   const uint bytesPerBlock,
   IDirect3DTexture9& d3dTex,
   BByteArray& packGuideTex,
   UINT& d3dTexBaseSize,
   UINT& d3dTexMipSize)
{
   d3dTexBaseSize = 0;
   d3dTexMipSize = 0;

   const DWORD texPitch = 0;
   DWORD d3dTexTotalSize = XGSetTextureHeader( 
      width, 
      height, 
      numMipChainLevels + 1, 0, 
      d3dFormat, D3DPOOL_DEFAULT, 
      0, 
      XGHEADER_CONTIGUOUS_MIP_OFFSET,
      texPitch, 
      &d3dTex, 
      &d3dTexBaseSize, 
      &d3dTexMipSize);

   const bool packedMips = d3dTex.Format.PackedMips;          

   packGuideTex.resize(d3dTexTotalSize);
   BDEBUG_ASSERT((d3dTexTotalSize % bytesPerBlock) == 0);

   BByteArray tempBuf;

   for (uint mipLevel = 0; mipLevel < numMipChainLevels + 1; mipLevel++)
   {     
      uint mipWidth, mipHeight;
      BDDXUtils::calcMipDimension(mipWidth, mipHeight, width, height, mipLevel);

      const uint srcDataSize = mipWidth * mipHeight * bytesPerBlock;

      const int widthInBlocks  = mipWidth >> 2;
      const int heightInBlocks = mipHeight >> 2;

      const int sourcePitch = widthInBlocks * bytesPerBlock;
      const int numBlocks = widthInBlocks * heightInBlocks;

      tempBuf.resize(srcDataSize);

      for (int blockIndex = 0; blockIndex < numBlocks; blockIndex++)
      {
         DWORD* pDstBlock = (DWORD*)(tempBuf.getPtr() + blockIndex * bytesPerBlock);

         pDstBlock[0] = (blockIndex + 1) | (mipLevel << 24);
      }

      const BYTE* pSrc = tempBuf.getPtr();

      uint dstOfs = XGGetMipLevelOffset( &d3dTex, 0, mipLevel );

      if ((mipLevel > 0) && (d3dTexMipSize > 0))
         dstOfs += d3dTexBaseSize;

      BDEBUG_ASSERT(dstOfs < packGuideTex.getSize());

      BYTE* pDst = packGuideTex.getPtr() + dstOfs;

      const DWORD flags = packedMips ? 0 : XGTILE_NONPACKED;

      XGTileTextureLevel(
         width, height, mipLevel, 
         XGGetGpuFormat(d3dFormat), flags, pDst, NULL, pSrc, sourcePitch, NULL);            
   }         

   return true;
}  

//------------------------------------------------------------------------------------------------------
// BDXTQPack::computeBlockPixels
//------------------------------------------------------------------------------------------------------
void BDXTQPack::computeBlockPixels(
   const uint width, const uint height,
   const uint numMipChainLevels,
   const uint bytesPerBlock,
   const uint totalBlocks,
   const BYTE* pSrcData, uint srcDataSize,
   const BByteArray& packGuideTex,
   BDXTShrinker::BDXTBlockPixelsArray& blockPixels)
{
   blockPixels.resize(totalBlocks);

   BDDXTextureInfo srcTexInfo;
   srcTexInfo.mWidth = width;
   srcTexInfo.mHeight = height;
   srcTexInfo.mDataFormat = cDDXDataFormatA8R8G8B8;
   srcTexInfo.mNumMipChainLevels = numMipChainLevels;
      
   BDDXSerializedTexture srcTex(pSrcData, srcDataSize, srcTexInfo);
   
   uint numUnusedBlocks = 0;
   for (uint blockIndex = 0; blockIndex < totalBlocks; blockIndex++)
   {  
      const DWORD* pPackGuide = (const DWORD*)(packGuideTex.getPtr() + blockIndex * bytesPerBlock);

      BDXTShrinker::BDXTBlockPixels& dstBlock = blockPixels[blockIndex];

      if (*pPackGuide == 0)
      {
         memset(&dstBlock, 0, sizeof(BDXTShrinker::BDXTBlockPixels));
         numUnusedBlocks++;
      }
      else
      {
         const uint mipIndex = (*pPackGuide >> 24) & 0xFF;
         BDEBUG_ASSERT(mipIndex <= numMipChainLevels);

         const DWORD blockIndex = (*pPackGuide & 0xFFFFFF) - 1;

         uint mipWidth, mipHeight;
         BDDXUtils::calcMipDimension(mipWidth, mipHeight, width, height, mipIndex);

         const uint widthInBlocks = Math::Max<uint>(1, mipWidth >> 2);
         const uint heightInBlocks = Math::Max<uint>(1, mipHeight >> 2);
         const uint totalMipBlocks = widthInBlocks * heightInBlocks;
         totalMipBlocks;
         BDEBUG_ASSERT(blockIndex < totalMipBlocks);

         const uint blockX = blockIndex % widthInBlocks;
         const uint blockY = blockIndex / widthInBlocks;

         BRGBAImage image( (BRGBAColor*)srcTex.getSurfaceData(0, mipIndex), mipWidth, mipHeight);

         for (uint y = 0; y < 4; y++)
         {
            for (uint x = 0; x < 4; x++)
            {
               const BRGBAColor& c = image(blockX * 4 + x, blockY * 4 + y);
               dstBlock.mPixels[y][x] = c;
            }
         }
      }
   }  
} 

//------------------------------------------------------------------------------------------------------
// BDXTQPack::createColorCodebook
//------------------------------------------------------------------------------------------------------ 
void BDXTQPack::createColorCodebook(
   BDXTQHeader& header,
   const BDXTShrinker::BPackedColorsArray& colorCodebook,
   const BDXTShrinker::BBlockSelectorsArray& colorSelectorCodebook)
{
   header.mColorCodebookOfs = mCachedMemPool.alloc(colorCodebook.getSize() * sizeof(DWORD), 16);
   header.mColorCodebookSize = colorCodebook.getSize();
   DWORD* p = (DWORD*)mCachedMemPool.getPtr(header.mColorCodebookOfs);
 
   for (uint i = 0; i < colorCodebook.getSize(); i++)
   {
      const BDXTShrinker::BPackedColors& packedColors = colorCodebook[i];
      
      DWORD d = packedColors.getHigh() | ((uint)packedColors.getLow() << 16);
      
      if (cLittleEndianNative)
         EndianSwitchWords((WORD*)&d, 2);
      
      p[i] = d;
   }
         
   header.mColorSelectorCodebookOfs = mCachedMemPool.alloc(colorSelectorCodebook.getSize() * sizeof(DWORD), 16);
   header.mColorSelectorCodebookSize = colorSelectorCodebook.getSize();
   p = (DWORD*)mCachedMemPool.getPtr(header.mColorSelectorCodebookOfs);
   
   for (uint i = 0; i < colorSelectorCodebook.getSize(); i++)
   {
      BDXTUtils::BDXT1Cell cell;

      for (uint y = 0; y < 4; y++)
         for (uint x = 0; x < 4; x++)
            cell.setSelector(x, y, colorSelectorCodebook[i].mSelectors[y][x]);
            
      DWORD d = cell.getSelectorWord(0) | ((uint)cell.getSelectorWord(1) << 16);
      
      if (cLittleEndianNative)
         EndianSwitchWords((WORD*)&d, 2);
    
      p[i] = d;           
   }
}

//------------------------------------------------------------------------------------------------------
// BDXTQPack::createAlphaCodebook
//------------------------------------------------------------------------------------------------------ 
void BDXTQPack::createAlphaCodebook(
   BDXTQHeader& header,
   const BDXTShrinker::BPackedColorsArray&      alphaCodebook,
   const BDXTShrinker::BBlockSelectorsArray&    alphaSelectorCodebook)
{
   header.mAlphaCodebookSize = alphaCodebook.getSize();
   header.mAlphaCodebookOfs = mCachedMemPool.alloc(alphaCodebook.getSize() * sizeof(WORD), 16);
   WORD* p = (WORD*)mCachedMemPool.getPtr(header.mAlphaCodebookOfs);
   for (uint i = 0; i < alphaCodebook.getSize(); i++)
   {
      WORD d = (WORD)(alphaCodebook[i].getHigh() | (alphaCodebook[i].getLow() << 8));
      if (cLittleEndianNative)
         EndianSwitchWords(&d, 1);

      p[i] = d;
   }
   
   header.mAlphaSelectorCodebookSize = alphaSelectorCodebook.getSize();
   header.mAlphaSelectorCodebookOfs = mCachedMemPool.alloc(header.mAlphaSelectorCodebookSize * sizeof(WORD) * 3, 16);
   p = (WORD*)mCachedMemPool.getPtr(header.mAlphaSelectorCodebookOfs);
      
   for (uint i = 0; i < alphaSelectorCodebook.getSize(); i++)
   {
      BDXTUtils::BDXT5Cell cell;
      for (uint y = 0; y < 4; y++)
         for (uint x = 0; x < 4; x++)
            cell.setSelector(x, y, alphaSelectorCodebook[i].mSelectors[y][x]);

      WORD d[3];
      d[0] = cell.getSelectorWord(0);
      d[1] = cell.getSelectorWord(1);
      d[2] = cell.getSelectorWord(2);
      if (cLittleEndianNative)
         EndianSwitchWords(d, 3);
         
      p[i*3+0] = d[0];
      p[i*3+1] = d[1];
      p[i*3+2] = d[2];
   }
}

//------------------------------------------------------------------------------------------------------
// BDXTQPack::createSegmentIndices
//------------------------------------------------------------------------------------------------------ 
void BDXTQPack::createSegmentIndices(
   BBitCoder& bitCoder,
   uint firstBlock, uint numBlocks,
   BDXTQHeader& header,
   const UShortArray& colorIndices,
   const UShortArray& colorSelectorIndices,
   const UShortArray& alphaIndices0,
   const UShortArray& alphaIndices1,
   const UShortArray& alphaSelectorIndices)
{
   bitCoder.encodeStart();

   const bool hasColorBlocks = colorIndices.getSize() > 0;
   const bool hasAlpha0Blocks = alphaIndices0.getSize() > 0;
   const bool hasAlpha1Blocks = alphaIndices1.getSize() > 0;
   
   const uint bitsPerColorIndex           = Math::CodeSize(header.mColorCodebookSize + (hasAlpha0Blocks ? 0 : (hasColorBlocks ? MAX_RUN_CODES : 0)));
   const uint bitsPerColorSelectorIndex   = Math::CodeSize(header.mColorSelectorCodebookSize);
   const uint bitsPerAlphaIndex           = Math::CodeSize(header.mAlphaCodebookSize + (hasAlpha0Blocks ? MAX_RUN_CODES : 0));
   const uint bitsPerAlphaSelectorIndex   = Math::CodeSize(header.mAlphaSelectorCodebookSize);
   
   if (hasAlpha0Blocks)
   {
      BDEBUG_ASSERT((header.mAlphaCodebookSize + MAX_RUN_CODES) <= (1U << bitsPerAlphaIndex));
   }
   else
   {
      BDEBUG_ASSERT((header.mColorCodebookSize + MAX_RUN_CODES) <= (1U << bitsPerColorIndex));
   }
         
   int curRunSize = 0;
   int curColorIndex = -1;
   int curColorSelectorIndex = -1;
   int curAlpha0Index = -1;
   int curAlpha1Index = -1;
   int curAlpha0SelectorIndex = -1;
   int curAlpha1SelectorIndex = -1;

   const uint totalTextureBlocks = header.mBaseBlocks + header.mMipBlocks;
   
   const uint firstRLECode = (hasAlpha0Blocks ? (1 << bitsPerAlphaIndex) : (1 << bitsPerColorIndex)) - MAX_RUN_SIZE;
   
   const uint lastBlock = firstBlock + numBlocks;
   
   uint totalEncodedBlocks = 0;
   for (uint blockIndex = firstBlock; blockIndex <= lastBlock; blockIndex++)
   {
      int colorIndex          = -1;
      int colorSelectorIndex  = -1;
      int alpha0Index         = -1;
      int alpha0SelectorIndex = -1;
      int alpha1Index         = -1;
      int alpha1SelectorIndex = -1;
                  
      if (blockIndex < lastBlock)
      {
         if (hasColorBlocks)
         {
            colorIndex = colorIndices[blockIndex];
            colorSelectorIndex = colorSelectorIndices[blockIndex];
         }
         
         if (hasAlpha0Blocks)
         {
            alpha0Index = alphaIndices0[blockIndex];
            alpha0SelectorIndex = alphaSelectorIndices[blockIndex];
            
            if (hasAlpha1Blocks)
            {
               alpha1Index = alphaIndices1[blockIndex];
               alpha1SelectorIndex = alphaSelectorIndices[blockIndex + totalTextureBlocks];
            }
         }
      }         
      
      const bool isRun = 
         (curColorIndex          == colorIndex) &&
         (curColorSelectorIndex  == colorSelectorIndex) &&
         (curAlpha0Index         == alpha0Index) &&
         (curAlpha1Index         == alpha1Index) &&
         (curAlpha0SelectorIndex == alpha0SelectorIndex) &&
         (curAlpha1SelectorIndex == alpha1SelectorIndex);
               
      if ((curRunSize < MAX_RUN_SIZE) && (isRun))
      {
         BDEBUG_ASSERT(blockIndex != lastBlock);
         curRunSize++;
      }
      else
      {
         if (curRunSize > 0)
         {
            totalEncodedBlocks += curRunSize;
            
            if (curRunSize >= MIN_RUN_SIZE)
            {
               bitCoder.encodeCode(firstRLECode + curRunSize - MIN_RUN_SIZE, hasAlpha0Blocks ? bitsPerAlphaIndex : bitsPerColorIndex);
               curRunSize = 1;
            }
            
            for (int i = 0; i < curRunSize; i++)
            {
               if (hasAlpha0Blocks)
               {
                  bitCoder.encodeCode(curAlpha0Index, bitsPerAlphaIndex);
                  bitCoder.encodeCode(curAlpha0SelectorIndex, bitsPerAlphaSelectorIndex);
                  
                  if (hasAlpha1Blocks)
                  {
                     bitCoder.encodeCode(curAlpha1Index, bitsPerAlphaIndex);
                     bitCoder.encodeCode(curAlpha1SelectorIndex, bitsPerAlphaSelectorIndex);
                  }
               }
               
               if (hasColorBlocks)
               {
                  bitCoder.encodeCode(curColorIndex, bitsPerColorIndex);
                  bitCoder.encodeCode(curColorSelectorIndex, bitsPerColorSelectorIndex);
               }
            }
         }
         
         curRunSize = 1;
         curColorIndex = colorIndex;
         curColorSelectorIndex = colorSelectorIndex;
         curAlpha0Index = alpha0Index;
         curAlpha1Index = alpha1Index;
         curAlpha0SelectorIndex = alpha0SelectorIndex;
         curAlpha1SelectorIndex = alpha1SelectorIndex;
      }
   }
   
   BDEBUG_ASSERT(totalEncodedBlocks == numBlocks);
      
   bitCoder.encodeEnd();
}   

//------------------------------------------------------------------------------------------------------
// BDXTQPack::createIndices
//------------------------------------------------------------------------------------------------------ 
void BDXTQPack::createIndices(
   BDXTQHeader& header,
   const UShortArray& colorIndices,
   const UShortArray& colorSelectorIndices,
   const UShortArray& alphaIndices0,
   const UShortArray& alphaIndices1,
   const UShortArray& alphaSelectorIndices)
{
   // ColorIndex
   // ColorSelectorIndex
   // Alpha0Index
   // Alpha0SelectorIndex
   // Alpha1Index
   // Alpha1SelectorIndex
         
   uint numBaseSegments = 0;
   uint baseBlocksPerSegment = 0;
   if (header.mBaseBlocks > 128)
      numBaseSegments = 4;
   else if (header.mBaseBlocks)
      numBaseSegments = 1;
   
   baseBlocksPerSegment = numBaseSegments ? (header.mBaseBlocks / numBaseSegments) : 0;
   
   uint numMipSegments = 0;
   uint mipBlocksPerSegment = 0;
   if (header.mMipBlocks > 128)
      numMipSegments = 4;
   else if (header.mMipBlocks)
      numMipSegments = 1;
      
   mipBlocksPerSegment = numMipSegments ? (header.mMipBlocks / numMipSegments) : 0;
   
   header.mNumBaseSegments = (uchar)numBaseSegments;
   uint curBlockIndex = 0;
   for (uint i = 0; i < numBaseSegments; i++)
   {
      uint numSegmentBlocks = baseBlocksPerSegment;
      if (i == numBaseSegments - 1)
         numSegmentBlocks = header.mBaseBlocks - baseBlocksPerSegment * (numBaseSegments - 1);
         
      header.mBaseIndices[i].mFirstBlock = curBlockIndex;
      header.mBaseIndices[i].mNumBlocks = numSegmentBlocks;
      
      curBlockIndex += numSegmentBlocks;
   }
   
   BDEBUG_ASSERT(curBlockIndex == header.mBaseBlocks);
   
   header.mNumMipSegments = (uchar)numMipSegments;
   for (uint i = 0; i < numMipSegments; i++)
   {
      uint numSegmentBlocks = mipBlocksPerSegment;
      if (i == numBaseSegments - 1)
         numSegmentBlocks = header.mMipBlocks - mipBlocksPerSegment * (numMipSegments - 1);

      header.mMipIndices[i].mFirstBlock = curBlockIndex;
      header.mMipIndices[i].mNumBlocks = numSegmentBlocks;

      curBlockIndex += numSegmentBlocks;
   }
   
   BDEBUG_ASSERT(curBlockIndex == (header.mBaseBlocks + header.mMipBlocks));
         
   for (uint i = 0; i < numBaseSegments; i++)
   {
      BBitCoder bitCoder;
      
      createSegmentIndices(
         bitCoder,
         header.mBaseIndices[i].mFirstBlock,
         header.mBaseIndices[i].mNumBlocks,
         header,
         colorIndices,
         colorSelectorIndices,
         alphaIndices0,
         alphaIndices1,
         alphaSelectorIndices);
         
      header.mBaseIndices[i].mDataLen = bitCoder.getBuf()->getSize();
      header.mBaseIndices[i].mDataOfs = mCachedMemPool.alloc(bitCoder.getBuf()->getSize(), 16);
      memcpy(mCachedMemPool.getPtr(header.mBaseIndices[i].mDataOfs), bitCoder.getBuf()->getPtr(), bitCoder.getBuf()->getSize());
   }         
   
   for (uint i = 0; i < numMipSegments; i++)
   {
      BBitCoder bitCoder;      
                  
      createSegmentIndices(
         bitCoder,
         header.mMipIndices[i].mFirstBlock,
         header.mMipIndices[i].mNumBlocks,
         header,
         colorIndices,
         colorSelectorIndices,
         alphaIndices0,
         alphaIndices1,
         alphaSelectorIndices);
      
      header.mMipIndices[i].mDataLen = bitCoder.getBuf()->getSize();
      header.mMipIndices[i].mDataOfs = mCachedMemPool.alloc(bitCoder.getBuf()->getSize(), 16);
      memcpy(mCachedMemPool.getPtr(header.mMipIndices[i].mDataOfs), bitCoder.getBuf()->getPtr(), bitCoder.getBuf()->getSize());
   }         
   
   // Ensure the cached mem pool has at least 8 valid bytes at the end, so the decoder can safely read ahead.
   mCachedMemPool.alloc(16, 1);
}   

//------------------------------------------------------------------------------------------------------
// BDXTQPack::getD3DDataFormatFromDDX
//------------------------------------------------------------------------------------------------------ 
D3DFORMAT BDXTQPack::getD3DDataFormatFromDDX(eDDXDataFormat dataFormat)
{
   switch (dataFormat)
   {
      case cDDXDataFormatDXT1Q:  return D3DFMT_DXT1; 
      case cDDXDataFormatDXT5Q:  return D3DFMT_DXT5; 
      case cDDXDataFormatDXT5HQ: return D3DFMT_DXT5; 
      case cDDXDataFormatDXNQ:   return D3DFMT_DXN;  
   }    
   return D3DFMT_UNKNOWN;
}   

//------------------------------------------------------------------------------------------------------
// BDXTQPack::packDXTQ32
//------------------------------------------------------------------------------------------------------ 
bool BDXTQPack::packDXTQ32(
   const BDDXTextureInfo& srcTextureInfo, const BYTE* pSrcData, uint srcDataSize,
   const BDDXPackParams& options, 
   BByteArray& dstData)
{
   const uint width = srcTextureInfo.mWidth;
   const uint height = srcTextureInfo.mHeight;
   const uint numMipChainLevels = srcTextureInfo.mNumMipChainLevels;
         
   if (srcTextureInfo.mDataFormat != cDDXDataFormatA8B8G8R8)
      return false;
      
   mCachedMemPool.clear();
      
   // 2 (DXT1) or 4 (DXT5/N) codebooks

   // Color codebook: 32-bits/entry, 1024 max
   // Color selector codebook: 32-bits/entry, 1024 max

   // Alpha codebook: 16-bits/entry, 1024 max
   // Alpha selector codebook: 48-bits/entry, 1024 max

   // DXT1: 10+10 
   // DXT5/N: 10+10 + 10+10

   // Color indices: 10-bits/index
   // Color selector indices: 10-bits/index

   // Alpha0 indices: 10-bits/entry
   // Alpha0 selector indices: 10-bits/index

   // Alpha1 indices: 10-bits/entry
   // Alpha1 selector indices: 10-bits/index

   // 10,10,10,2    
   // X  Y  Z  W

   D3DFORMAT d3dFormat = getD3DDataFormatFromDDX(options.mDataFormat);
   if (d3dFormat == D3DFMT_UNKNOWN)
      return false;
   
   const uint bytesPerBlock = getDDXDataFormatDXTBlockSize(options.mDataFormat);
   BDEBUG_ASSERT((bytesPerBlock == 8) || (bytesPerBlock == 16));

   IDirect3DTexture9 d3dTex;
   BByteArray packGuideTex;
   UINT d3dTexBaseSize = 0;
   UINT d3dTexMipSize = 0;
   if (!createXboxPackGuideTexture(
      width, height,
      numMipChainLevels, 
      d3dFormat,
      bytesPerBlock,
      d3dTex,
      packGuideTex,
      d3dTexBaseSize,
      d3dTexMipSize))
   {
      return false;
   }
   
   BDEBUG_ASSERT((d3dTexBaseSize % bytesPerBlock) == 0);
   BDEBUG_ASSERT((d3dTexMipSize % bytesPerBlock) == 0);
   
   IDirect3DTexture9* pD3DTex = (IDirect3DTexture9*)mCachedMemPool.allocPtr(sizeof(IDirect3DTexture9), 16);
   if (cLittleEndianNative)
      XGEndianSwapMemory(pD3DTex, &d3dTex, XGENDIAN_8IN32, sizeof(DWORD), sizeof(IDirect3DTexture9) / sizeof(DWORD));

   const uint totalSize = packGuideTex.getSize();
   const uint totalBlocks = totalSize / bytesPerBlock;

   BDXTShrinker::BDXTBlockPixelsArray blockPixels;

   computeBlockPixels(
      width, height,
      numMipChainLevels,
      bytesPerBlock,
      totalBlocks,
      pSrcData, srcDataSize,
      packGuideTex,
      blockPixels);

   BDXTFormat dxtFormat;
   switch (options.mDataFormat)
   {
      case cDDXDataFormatDXT1Q:  dxtFormat = cDXT1; break;
      case cDDXDataFormatDXT5Q:  dxtFormat = cDXT5; break;
      case cDDXDataFormatDXT5HQ: dxtFormat = cDXT5; break;
      case cDDXDataFormatDXNQ:   dxtFormat = cDXN; break;
      default: 
         return false;
   }

   const uint dim = Math::Max(width, height);   
   dim;
   
   bool hasColorBlocks = false;
   bool hasAlpha0Blocks = false;
   bool hasAlpha1Blocks = false;

   switch (options.mDataFormat)
   {
      case cDDXDataFormatDXT1Q: 
      {
         hasColorBlocks = true; 
         break;
      }
      case cDDXDataFormatDXT5Q: 
      case cDDXDataFormatDXT5HQ: 
      {
         hasColorBlocks = true; 
         hasAlpha0Blocks = true; 
         break;
      }
      case cDDXDataFormatDXNQ: 
      {
         hasAlpha0Blocks = true;
         hasAlpha1Blocks = true; 
         break;
      }
   }
   
   uint divisor = 1;
   if (dim == 256)
      divisor = 2;
   else if (dim == 128)
      divisor = 4;
   else if (dim <= 64)
      divisor = 8;
   
   uint colorCodebookSize           = (1024/divisor) - MAX_RUN_CODES;
   uint selectorCodebookSize        = (2048/divisor);
   uint alphaCodebookSize           = (hasAlpha1Blocks ? (2048/divisor) : (1024/divisor)) - MAX_RUN_CODES;
   uint selectorAlphaCodebookSize   = hasAlpha1Blocks ? (4096/divisor) : (4096/divisor);
         
   BDXTShrinker::BParams dxtShrinkerParams;
   dxtShrinkerParams.mMaxQuantColors = colorCodebookSize;
   dxtShrinkerParams.mMaxAlphaColors = alphaCodebookSize;
   dxtShrinkerParams.mColorSelectorCodebookSize = selectorCodebookSize;
   dxtShrinkerParams.mAlphaSelectorCodebookSize = selectorAlphaCodebookSize;
   dxtShrinkerParams.mFavorLargerAlpha = (options.mDataFormat == cDDXDataFormatDXT5HQ);
   dxtShrinkerParams.mPerceptual = (options.mPackerFlags & BDDXPackParams::cPerceptual) != 0;

   BDXTShrinker dxtShrinker(dxtFormat, totalBlocks, blockPixels.getPtr(), dxtShrinkerParams);

   // Header
   // Cached memory block
   // Physical memory block
   
   BDXTQHeader header;
   Utils::ClearObj(header);
   
   header.mVersion = BDXTQHeader::cVersion;
   header.mD3DStructSize = Utils::CreateWORD(sizeof(IDirect3DTexture9), sizeof(IDirect3DVertexBuffer9));
   header.mBaseBlocks = d3dTexBaseSize / bytesPerBlock;
   header.mMipBlocks = d3dTexMipSize / bytesPerBlock;
   BDEBUG_ASSERT((header.mBaseBlocks + header.mMipBlocks) == totalBlocks);
   
   header.mD3DTexOfs = mCachedMemPool.getOfs(pD3DTex);
            
   if (hasColorBlocks)
      createColorCodebook(header, dxtShrinker.getColorCodebook(), dxtShrinker.getColorSelectorCodebook());

   if (hasAlpha0Blocks)
      createAlphaCodebook(header, dxtShrinker.getAlphaCodebook(), dxtShrinker.getAlphaSelectorCodebook());      
   
   createIndices(
      header, 
      dxtShrinker.getColorIndices(), 
      dxtShrinker.getColorSelectorIndices(), 
      dxtShrinker.getAlphaIndices0(), 
      dxtShrinker.getAlphaIndices1(), 
      dxtShrinker.getAlphaSelectorIndices());
   
   BECFFileBuilder ecfBuilder;
   ecfBuilder.setID(cDXTQID);
   ecfBuilder.addChunk(0, (BYTE*)&header, sizeof(header));
   ecfBuilder.addChunk(1, mCachedMemPool.getPtr(0), mCachedMemPool.getSize());
      
   BByteArray ecfStream;
   const bool success = ecfBuilder.writeToFileInMemory(ecfStream);
      
   dstData.swap(ecfStream);

#if 0
{
   BByteArray unpackedData;
   BDDXTextureInfo dstTextureInfo;
   BDDXTextureInfo srcTextureInfo;
   srcTextureInfo.mWidth = width;
   srcTextureInfo.mHeight = height;
   srcTextureInfo.mNumMipChainLevels = numMipChainLevels;
   srcTextureInfo.mDataFormat = options.mDataFormat;
   bool s = unpackDXTQToDXT(
      srcTextureInfo, dstData.getPtr(), dstData.getSize(),
      dstTextureInfo, unpackedData);
   s;      
   
   BRGBAImage unpackedImage;
   BDXTUnpacker::unpack(unpackedImage, unpackedData.getPtr(), cDXT1, width, height);

   BCFileStream dstStream;
   if (dstStream.open("out.tga", cSFWritable | cSFSeekable))
      BImageUtils::writeTGA(dstStream, unpackedImage, cTGAImageTypeBGR);
   dstStream.close();
}   
#endif   
         
   return success;
}

//------------------------------------------------------------------------------------------------------
// BDXTQPack::unpackDXTQ64
//------------------------------------------------------------------------------------------------------
bool BDXTQPack::packDXTQ64(
   const BDDXTextureInfo& srcTextureInfo, const BYTE* pSrcData, uint srcDataSize,
   const BDDXPackParams& options, float& hdrScale,
   BByteArray& dstData)
{
   BDEBUG_ASSERT(srcDataSize >= srcTextureInfo.mWidth * srcTextureInfo.mHeight * sizeof(uint64));
   
   const BDDXSerializedTexture srcTexture(pSrcData, srcDataSize, srcTextureInfo);
      
   if (options.mDataFormat == cDDXDataFormatDXT5HQ)
   {
      BByteArray tempAlphaImage;
      for (uint mipLevel = 0; mipLevel < srcTextureInfo.mNumMipChainLevels + 1; mipLevel++)
      {
         const uint mipWidth = srcTexture.getWidth(mipLevel);
         const uint mipHeight = srcTexture.getHeight(mipLevel);
         BRGBA16Image halfImage((BRGBAColor16*)srcTexture.getSurfaceData(0, mipLevel), mipWidth, mipHeight);

         BRGBAFImage floatImage;
         BHDRUtils::unpackHalfFloatImage(halfImage, floatImage, srcTextureInfo.mHasAlpha ? 4 : 3, true);

         BRGBAImage alphaImage;
         hdrScale = BHDRCodec::packFloatToDXT5HPass1(floatImage, alphaImage, hdrScale, cDDX_MAX_HDR_SCALE);   

         tempAlphaImage.pushBack((BYTE*)alphaImage.getPtr(), alphaImage.getSizeInBytes());            
      }  
                  
      BDDXTextureInfo alphaTextureInfo(srcTextureInfo);
      alphaTextureInfo.mDataFormat = cDDXDataFormatA8B8G8R8;
      alphaTextureInfo.mHasAlpha = true;
      BByteArray packedAlpha;
      if (!packDXTQ32(alphaTextureInfo, tempAlphaImage.getPtr(), tempAlphaImage.getSizeInBytes(), options, packedAlpha))
         return false;
      
      BDDXTextureInfo packedAlphaTextureInfo(srcTextureInfo);
      packedAlphaTextureInfo.mDataFormat = cDDXDataFormatDXT5HQ;
      packedAlphaTextureInfo.mHasAlpha = true;
      BDDXTextureInfo unpackedTextureInfo;
      BByteArray unpackedAlphaDXT;
      if (!unpackDXTQToDXT(packedAlphaTextureInfo, packedAlpha.getPtr(), packedAlpha.getSize(), unpackedTextureInfo, unpackedAlphaDXT, false))
         return false;
            
      BByteArray compTexture;            
      BDDXSerializedTexture packedAlphaTexture(unpackedAlphaDXT.getPtr(), unpackedAlphaDXT.getSize(), unpackedTextureInfo);
      for (uint mipLevel = 0; mipLevel < srcTextureInfo.mNumMipChainLevels + 1; mipLevel++)
      {
         const uint mipWidth = srcTexture.getWidth(mipLevel);
         const uint mipHeight = srcTexture.getHeight(mipLevel);
         
         BRGBAImage unpackedAlpha;
         BDXTUnpacker unpacker;
         unpacker;
         if (!unpacker.unpack(unpackedAlpha, packedAlphaTexture.getSurfaceData(0, mipLevel), cDXT5, mipWidth, mipHeight))
            return false;
            
         BRGBA16Image halfImage((BRGBAColor16*)srcTexture.getSurfaceData(0, mipLevel), mipWidth, mipHeight);
         BRGBAFImage floatImage;
         BHDRUtils::unpackHalfFloatImage(halfImage, floatImage, srcTextureInfo.mHasAlpha ? 4 : 3, true);
         
         BRGBAImage compImage;
         hdrScale = BHDRCodec::packFloatToDXT5HPass2(floatImage, unpackedAlpha, compImage, hdrScale, cDDX_MAX_HDR_SCALE);
         
         compTexture.pushBack((BYTE*)compImage.getPtr(), compImage.getSizeInBytes());
      }         
      
      BDDXTextureInfo compTextureInfo(srcTextureInfo);
      compTextureInfo.mDataFormat = cDDXDataFormatA8B8G8R8;
      compTextureInfo.mHasAlpha = true;

      return packDXTQ32(compTextureInfo, compTexture.getPtr(), compTexture.getSizeInBytes(), options, dstData);
   }
   else
   {
      BByteArray tempImage;
      for (uint mipLevel = 0; mipLevel < srcTextureInfo.mNumMipChainLevels; mipLevel++)
      {
         const uint mipWidth = srcTexture.getWidth(mipLevel);
         const uint mipHeight = srcTexture.getHeight(mipLevel);
         BRGBA16Image halfImage((BRGBAColor16*)srcTexture.getSurfaceData(0, mipLevel), mipWidth, mipHeight);

         BRGBAFImage floatImage;
         BHDRUtils::unpackHalfFloatImage(halfImage, floatImage, srcTextureInfo.mHasAlpha ? 4 : 3, true);
         
         BRGBAImage image;
         BHDRUtils::convertFloatToRGB8Image(floatImage, image);

         tempImage.pushBack((BYTE*)image.getPtr(), image.getSizeInBytes());            
      }  
      
      BDDXTextureInfo textureInfo(srcTextureInfo);
      textureInfo.mDataFormat = cDDXDataFormatA8B8G8R8;
                  
      return packDXTQ32(textureInfo, tempImage.getPtr(), tempImage.getSizeInBytes(), options, dstData);
   }
   
   //return false;
}   

//------------------------------------------------------------------------------------------------------
// BDXTQPack::packDXTQ
//------------------------------------------------------------------------------------------------------
bool BDXTQPack::packRGBAToDXTQ(
    const BDDXTextureInfo& srcTextureInfo, const BYTE* pSrcData, uint srcDataSize,
    const BDDXPackParams& options, float& hdrScale,
    BByteArray& dstData)
{
   mCachedMemPool.clear();
   mCachedMemPool.reserve(32*1024*1024U);
   
   if (srcTextureInfo.mDataFormat == cDDXDataFormatA16B16G16R16F)
      return packDXTQ64(srcTextureInfo, pSrcData, srcDataSize, options, hdrScale, dstData);
   else if (srcTextureInfo.mDataFormat == cDDXDataFormatA8B8G8R8)
      return packDXTQ32(srcTextureInfo, pSrcData, srcDataSize, options, dstData);
   
   return false;      
}   

//-----------------------------------------------------------------------------------------------------------
static __forceinline uint readLittleEndianDWORD(const uchar* __restrict pInBuf)
{
#ifdef XBOX
   return __loadwordbytereverse(0, pInBuf);
#else
   if (!cLittleEndianNative)
      return (uint)pInBuf[0] | (((uint)pInBuf[1]) << 8U) | (((uint)pInBuf[2]) << 16U) | (((uint)pInBuf[3]) << 24U);
   else
      return *reinterpret_cast<const uint*>(pInBuf);
#endif   
}

//------------------------------------------------------------------------------------------------------
// getBits2
//------------------------------------------------------------------------------------------------------
static __forceinline uint getBits2(uint codeSize, uint bitMask, const uchar*& pSrc, uint64& bitBuf, int& bitBufLeft)
{
   const uint result = (uint)bitBuf & bitMask;

   bitBuf >>= codeSize;
   bitBufLeft -= codeSize;

   if (bitBufLeft >= 0)
      return result;

   uint newBits = readLittleEndianDWORD(pSrc);
   pSrc += 4;

   bitBufLeft += 32;      
   bitBuf |= (((uint64)newBits) << bitBufLeft);      

   return result;
}

//------------------------------------------------------------------------------------------------------
// getBits2
//------------------------------------------------------------------------------------------------------
static __forceinline uint getBits(uint codeSize, const uchar*& pSrc, uint64& bitBuf, int& bitBufLeft)
{
   const uint result = (uint)bitBuf & ((1U << codeSize) - 1U);

   bitBuf >>= codeSize;
   bitBufLeft -= codeSize;

   if (bitBufLeft >= 0)
      return result;

   uint newBits = readLittleEndianDWORD(pSrc);
   pSrc += 4;

   bitBufLeft += 32;      
   bitBuf |= (((uint64)newBits) << bitBufLeft);      

   return result;
}

//------------------------------------------------------------------------------------------------------
// removeBits2
//------------------------------------------------------------------------------------------------------
static __forceinline void removeBits2(uint codeSize, const uchar*& pSrc, uint64& bitBuf, int& bitBufLeft)
{
   bitBuf >>= codeSize;
   bitBufLeft -= codeSize;

   if (bitBufLeft >= 0)
      return;

   uint newBits = readLittleEndianDWORD(pSrc);
   pSrc += 4;

   bitBufLeft += 32;      
   bitBuf |= (((uint64)newBits) << bitBufLeft);      
}

#if 0
//------------------------------------------------------------------------------------------------------
// BDXTQPack::decodeSegmentGeneral
//------------------------------------------------------------------------------------------------------
bool BDXTQPack::decodeSegmentGeneral(
   BYTE* __restrict pWCDst,
   const BDXTQHeader& header,
   const BYTE* pCachedMem,
   const BDXTQHeader::BSegment& segment,
   bool hasColorBlocks, bool hasAlpha0Blocks, bool hasAlpha1Blocks)
{
   const uint bytesPerBlock = hasAlpha0Blocks ? 16 : 8;
   
   const uint colorCodebookSize           = header.mColorCodebookSize;
   const uint colorSelectorCodebookSize   = header.mColorSelectorCodebookSize;
   const uint alphaCodebookSize           = header.mAlphaCodebookSize;
   const uint alphaSelectorCodebookSize   = header.mAlphaSelectorCodebookSize;
   
   DWORD* pColorCodebook          = colorCodebookSize         ? (DWORD*)(pCachedMem + header.mColorCodebookOfs) : NULL;
   DWORD* pColorSelectorCodebook  = colorSelectorCodebookSize ? (DWORD*)(pCachedMem + header.mColorSelectorCodebookOfs) : NULL;
   WORD* pAlphaCodebook           = alphaCodebookSize         ? (WORD*)(pCachedMem + header.mAlphaCodebookOfs) : NULL;
   WORD* pAlphaSelectorCodebook   = alphaSelectorCodebookSize ? (WORD*)(pCachedMem + header.mAlphaSelectorCodebookOfs) : NULL;   
         
   const uint bitsPerColorIndex           = Math::CodeSize(header.mColorCodebookSize + (hasAlpha0Blocks ? 0 : (hasColorBlocks ? MAX_RUN_CODES : 0)));
   const uint bitsPerColorSelectorIndex   = Math::CodeSize(header.mColorSelectorCodebookSize);
   const uint bitsPerAlphaIndex           = Math::CodeSize(header.mAlphaCodebookSize + (hasAlpha0Blocks ? MAX_RUN_CODES : 0));
   const uint bitsPerAlphaSelectorIndex   = Math::CodeSize(header.mAlphaSelectorCodebookSize);
   
   const uint firstRLECode = (hasAlpha0Blocks ? (1 << bitsPerAlphaIndex) : (1 << bitsPerColorIndex)) - MAX_RUN_SIZE;
         
   BYTE* __restrict pDst = pWCDst + segment.mFirstBlock * bytesPerBlock;
   
   const uchar* pSrc = pCachedMem + segment.mDataOfs;
   uint64 bitBuf = readLittleEndianDWORD(pSrc); 
   pSrc += 4;
   int bitBufLeft = 0;
   
   uint numBlocksLeft = segment.mNumBlocks;
   while (numBlocksLeft)  
   {
      uint code = getBits(hasAlpha0Blocks ? bitsPerAlphaIndex : bitsPerColorIndex, pSrc, bitBuf, bitBufLeft);
      
      uint runSize = 1;
      if (code >= firstRLECode)
      {
         runSize = code - firstRLECode + MIN_RUN_SIZE;
         if (runSize > numBlocksLeft)
            return false;
         
         code = getBits(hasAlpha0Blocks ? bitsPerAlphaIndex : bitsPerColorIndex, pSrc, bitBuf, bitBufLeft);
      }
      
      WORD decodedBits[8];
                        
      if (hasAlpha0Blocks)
      {
         if (code >= alphaCodebookSize)
            return false;
         
         decodedBits[0] = pAlphaCodebook[code];
         
         code = getBits(bitsPerAlphaSelectorIndex, pSrc, bitBuf, bitBufLeft);
         
         if (code >= alphaSelectorCodebookSize)         
            return false;
         
         code *= 3;
         decodedBits[1] = pAlphaSelectorCodebook[code];
         decodedBits[2] = pAlphaSelectorCodebook[code + 1];
         decodedBits[3] = pAlphaSelectorCodebook[code + 2];
               
         if (hasAlpha1Blocks)
         {
            code = getBits(bitsPerAlphaIndex, pSrc, bitBuf, bitBufLeft);
            
            if (code >= alphaCodebookSize)
               return false;
         
            decodedBits[4] = pAlphaCodebook[code];
            
            code = getBits(bitsPerAlphaSelectorIndex, pSrc, bitBuf, bitBufLeft);

            if (code >= alphaSelectorCodebookSize)         
               return false;

            code *= 3;
            decodedBits[5] = pAlphaSelectorCodebook[code];
            decodedBits[6] = pAlphaSelectorCodebook[code + 1];
            decodedBits[7] = pAlphaSelectorCodebook[code + 2];
         }
         
         if (hasColorBlocks)
            code = getBits(bitsPerColorIndex, pSrc, bitBuf, bitBufLeft);
      }
      
      if (hasColorBlocks)
      {
         if (code >= colorCodebookSize)
            return false;

         DWORD* p = (DWORD*)(hasAlpha0Blocks ? &decodedBits[4] : decodedBits);
         
         p[0] = pColorCodebook[code];

         code = getBits(bitsPerColorSelectorIndex, pSrc, bitBuf, bitBufLeft);

         if (code >= colorSelectorCodebookSize)
            return false;

         p[1] = pColorSelectorCodebook[code];
      }
      
      if (cLittleEndianNative)
         EndianSwitchWords(decodedBits, bytesPerBlock >> 1);
      
      for (uint i = 0; i < runSize; i++)
      {  
         memcpy(pDst, decodedBits, bytesPerBlock);
         pDst += bytesPerBlock;
      }
      
      numBlocksLeft -= runSize;
   }
   
   return true;
}   
#endif

typedef void* BWriteState;

#if 0
inline BWriteState BeginWrite(void* p, void* pEnd, uint cacheLinesToWriteAhead)
{
   BWriteState state = Utils::AlignUp(p, 128);  
   const uint ofs = (cacheLinesToWriteAhead << 7);
   if (((uchar*)state + ofs + 127) < (uchar*)pEnd)
   {
      __dcbz128(ofs, state);
   }
   return state;
}

inline BWriteState UpdateWrite(BWriteState state, void* p, void* pEnd, uint cacheLinesToWriteAhead)
{
   BWriteState newState = Utils::AlignUp(p, 128);
   if (newState > state) 
   {
      const uint ofs = (cacheLinesToWriteAhead << 7);
      if (((uchar*)newState + ofs + 127) < (uchar*)pEnd)
      {
         __dcbz128(ofs, newState);
      }
   }
   return newState;
}
#endif

inline BWriteState BeginWrite(void* p, void* pEnd, uint cacheLinesToWriteAhead)
{
   BWriteState state = Utils::AlignUp(p, 128);  
   const uint ofs = (cacheLinesToWriteAhead << 7);
   if (((uchar*)state + ofs + 127) < (uchar*)pEnd)
   {
      __dcbz128(ofs, state);
   }
   return p;
}

inline BWriteState UpdateWrite(BWriteState prevP, void* p, void* pEnd, uint cacheLinesToWriteAhead)
{
   if (((uint)prevP ^ (uint)p) & 0xFFFFFF80)
   {
      __dcbf(0, prevP);
   }      

   BWriteState state = Utils::AlignUp(prevP, 128);
   BWriteState newState = Utils::AlignUp(p, 128);
   if (newState > state) 
   {
      const uint ofs = (cacheLinesToWriteAhead << 7);
      if (((uchar*)newState + ofs + 127) < (uchar*)pEnd)
      {
         __dcbz128(ofs, newState);
      }
   }
   
   return p;
}

//------------------------------------------------------------------------------------------------------
// BDXTQPack::decodeSegmentDXT1
//------------------------------------------------------------------------------------------------------
bool BDXTQPack::decodeSegmentDXT1(
   BYTE* __restrict pWCDst,
   const BDXTQHeader& header,
   const BYTE* pCachedMem,
   const BDXTQHeader::BSegment& segment)
{
   const uint cBytesPerBlock = 8;
   
   DWORD* pColorCodebook                  = (DWORD*)(pCachedMem + header.mColorCodebookOfs);
   DWORD* pColorSelectorCodebook          = (DWORD*)(pCachedMem + header.mColorSelectorCodebookOfs);

   const uint bitsPerColorIndex           = Math::CodeSize(header.mColorCodebookSize + MAX_RUN_CODES);
   const uint colorIndexMask              = (1U << bitsPerColorIndex) - 1U;
   
   const uint bitsPerColorSelectorIndex   = Math::CodeSize(header.mColorSelectorCodebookSize);
   const uint colorSelectorIndexMask      = (1U << bitsPerColorSelectorIndex) - 1U;
      
   const uint bitsForBothColorIndices     = bitsPerColorIndex + bitsPerColorSelectorIndex;

   const uint firstRLECode = (1 << bitsPerColorIndex) - MAX_RUN_SIZE;

   DWORD* __restrict pDst = (DWORD*)(pWCDst + segment.mFirstBlock * cBytesPerBlock);

   const uchar* pSrc = pCachedMem + segment.mDataOfs;
   uint64 bitBuf = readLittleEndianDWORD(pSrc); 
   pSrc += 4;
   int bitBufLeft = 0;

   uint numBlocksLeft = segment.mNumBlocks;
   while (numBlocksLeft)  
   {
      uint code = (uint)bitBuf & colorIndexMask;
      
      if (code >= firstRLECode)
      {
         uint runSize = code - firstRLECode + MIN_RUN_SIZE;
         if (runSize > numBlocksLeft)
            return false;
            
         removeBits2(bitsPerColorIndex, pSrc, bitBuf, bitBufLeft);
            
         code = (uint)bitBuf & colorIndexMask;

         DWORD c = pColorCodebook[code];

         code = (((uint)bitBuf) >> bitsPerColorIndex) & colorSelectorIndexMask;
         
         removeBits2(bitsForBothColorIndices, pSrc, bitBuf, bitBufLeft);
         
         //if (code >= colorSelectorCodebookSize)
         //   return false;

         DWORD s = pColorSelectorCodebook[code];
         DWORD* pEndDst = pDst + runSize + runSize;

         do
         {  
            pDst[0] = c;
            pDst[1] = s;
            pDst += 2;
         } while (pDst != pEndDst);

         numBlocksLeft -= runSize;
      }
      else
      {
         pDst[0] = pColorCodebook[code];

         code = (((uint)bitBuf) >> bitsPerColorIndex) & colorSelectorIndexMask;
         
         removeBits2(bitsForBothColorIndices, pSrc, bitBuf, bitBufLeft);

         pDst[1] = pColorSelectorCodebook[code];
         
         pDst += 2;

         numBlocksLeft--;
      }         
   }
   
   return true;
}

//------------------------------------------------------------------------------------------------------
// BDXTQPack::decodeSegmentDXT5
//------------------------------------------------------------------------------------------------------
bool BDXTQPack::decodeSegmentDXT5(
   BYTE* __restrict pWCDst,
   const BDXTQHeader& header,
   const BYTE* pCachedMem,
   const BDXTQHeader::BSegment& segment)
{
   const uint cBytesPerBlock = 16;
   
   const uchar* pSrc = pCachedMem + segment.mDataOfs;
   const uchar* pSrcEnd = pSrc + segment.mDataLen;
   Utils::BPrefetchState srcPrefetch = Utils::BeginPrefetch(pSrc, pSrcEnd, 2);
   
   DWORD* __restrict pDst = (DWORD*)(pWCDst + segment.mFirstBlock * cBytesPerBlock);
   void* __restrict pDstEnd = (uchar*)pDst + segment.mNumBlocks * cBytesPerBlock;
   BWriteState dstWrite = BeginWrite(pDst, pDstEnd, 3);
         
   const uint colorCodebookSize                 = header.mColorCodebookSize;
   const uint colorSelectorCodebookSize         = header.mColorSelectorCodebookSize;
   const uint alphaCodebookSize                 = header.mAlphaCodebookSize;
   const uint alphaSelectorCodebookSize         = header.mAlphaSelectorCodebookSize;
   
   DWORD* pColorCodebook                  = (DWORD*)(pCachedMem + header.mColorCodebookOfs);
   void* pColorCodebookEnd                = (uchar*)pColorCodebook + colorCodebookSize * sizeof(DWORD);
   int maxColorCodebookCode               = 0;
   int prevMaxColorCodebookCode           = -1;
            
   DWORD* pColorSelectorCodebook          = (DWORD*)(pCachedMem + header.mColorSelectorCodebookOfs);
   void* pColorSelectorCodebookEnd        = (uchar*)pColorSelectorCodebook + colorSelectorCodebookSize * sizeof(DWORD);
   int maxColorSelectorCodebookCode       = 0;
   int prevMaxColorSelectorCodebookCode   = -1;
         
   WORD* pAlphaCodebook                   = (WORD*)(pCachedMem + header.mAlphaCodebookOfs);
   void* pAlphaCodebookEnd                = (uchar*)pAlphaCodebook + alphaCodebookSize * sizeof(WORD);
   int maxAlphaCodebookCode               = 0;
   int prevMaxAlphaCodebookCode           = -1;
         
   WORD* pAlphaSelectorCodebook           = (WORD*)(pCachedMem + header.mAlphaSelectorCodebookOfs);   
   void* pAlphaSelectorCodebookEnd        = (uchar*)pAlphaSelectorCodebook + alphaSelectorCodebookSize * sizeof(WORD) * 3;
   int maxAlphaSelectorCodebookCode       = 0;
   int prevMaxAlphaSelectorCodebookCode   = -1;
                  
   const uint bitsPerColorIndex           = Math::CodeSize(header.mColorCodebookSize);
   const uint colorIndexMask              = (1U << bitsPerColorIndex) - 1U;
   
   const uint bitsPerColorSelectorIndex   = Math::CodeSize(header.mColorSelectorCodebookSize);
   const uint colorSelectorIndexMask      = (1U << bitsPerColorSelectorIndex) - 1U;
         
   const uint bitsPerAlphaIndex           = Math::CodeSize(header.mAlphaCodebookSize + MAX_RUN_CODES);
   const uint alphaIndexMask              = (1U << bitsPerAlphaIndex) - 1U;

   const uint bitsPerAlphaSelectorIndex   = Math::CodeSize(header.mAlphaSelectorCodebookSize);
   const uint alphaSelectorIndexMask      = (1U << bitsPerAlphaSelectorIndex) - 1U;
   
   const uint bitsForBothAlphaIndices     = bitsPerAlphaIndex + bitsPerAlphaSelectorIndex;
   const uint bitsForBothColorIndices     = bitsPerColorIndex + bitsPerColorSelectorIndex;

   const uint firstRLECode = (1 << bitsPerAlphaIndex) - MAX_RUN_SIZE;
            
   uint64 bitBuf = readLittleEndianDWORD(pSrc); 
   pSrc += 4;
   int bitBufLeft = 0;
   
   uint numBlocksLeft = segment.mNumBlocks;
   while (numBlocksLeft)  
   {
      srcPrefetch = Utils::UpdatePrefetch(srcPrefetch, pSrc, pSrcEnd, 2);
      dstWrite = UpdateWrite(dstWrite, pDst, pDstEnd, 3);
      
      if ((maxColorCodebookCode ^ prevMaxColorCodebookCode) & 0xFFFFFFE0)
      {
         prevMaxColorCodebookCode = maxColorCodebookCode;
         
         void* pTouchAddr = (uchar*)&pColorCodebook[maxColorCodebookCode] + 128;
         if (pTouchAddr < pColorCodebookEnd)
            __dcbt(0, pTouchAddr);
      }
      
      if ((maxColorSelectorCodebookCode ^ prevMaxColorSelectorCodebookCode) & 0xFFFFFFE0)
      {
         prevMaxColorSelectorCodebookCode = maxColorSelectorCodebookCode;

         void* pTouchAddr = (uchar*)&pColorSelectorCodebook[maxColorSelectorCodebookCode] + 128;
         if (pTouchAddr < pColorSelectorCodebookEnd)
            __dcbt(0, pTouchAddr);
      }
      
      
                        
      uint code = (uint)bitBuf & alphaIndexMask;
      
      if (code >= firstRLECode)
      {
         uint runSize = code - firstRLECode + MIN_RUN_SIZE;
         if (runSize > numBlocksLeft)
            return false;
            
         removeBits2(bitsPerAlphaIndex, pSrc, bitBuf, bitBufLeft);
            
         code = (uint)bitBuf & alphaIndexMask;
         maxAlphaCodebookCode = Math::Max<int>(maxAlphaCodebookCode, code);
         
         DWORD a = pAlphaCodebook[code];

         code = ((uint)bitBuf >> bitsPerAlphaIndex) & alphaSelectorIndexMask;
         maxAlphaSelectorCodebookCode = Math::Max<int>(maxAlphaSelectorCodebookCode, code);

         removeBits2(bitsForBothAlphaIndices, pSrc, bitBuf, bitBufLeft);

         code *= 3;

#ifdef XBOX
         a = (a << 16) | (pAlphaSelectorCodebook[code]);
         DWORD b = (pAlphaSelectorCodebook[code + 1] << 16) | pAlphaSelectorCodebook[code + 2];
#else
         a |= (pAlphaSelectorCodebook[code] << 16);
         DWORD b = pAlphaSelectorCodebook[code + 1] | (pAlphaSelectorCodebook[code + 2] << 16);
#endif         
         
         code = (uint)bitBuf & colorIndexMask;
         maxColorCodebookCode = Math::Max<int>(maxColorCodebookCode, code);
         DWORD c = pColorCodebook[code];
         
         code = (((uint)bitBuf) >> bitsPerColorIndex) & colorSelectorIndexMask;

         removeBits2(bitsForBothColorIndices, pSrc, bitBuf, bitBufLeft);
         
         maxColorSelectorCodebookCode = Math::Max<int>(maxColorSelectorCodebookCode, code);

         DWORD d = pColorSelectorCodebook[code];
         
         DWORD* pEndDst = pDst + (runSize << 2);

         do
         {  
            pDst[0] = a;
            pDst[1] = b;
            pDst[2] = c;
            pDst[3] = d;
            pDst += 4;
         } while (pDst != pEndDst);

         numBlocksLeft -= runSize;
      }
      else
      {
         maxAlphaCodebookCode = Math::Max<int>(maxAlphaCodebookCode, code);
         DWORD x = pAlphaCodebook[code];
         
         code = ((uint)bitBuf >> bitsPerAlphaIndex) & alphaSelectorIndexMask;
         
         maxAlphaSelectorCodebookCode = Math::Max<int>(maxAlphaSelectorCodebookCode, code);
         
         removeBits2(bitsForBothAlphaIndices, pSrc, bitBuf, bitBufLeft);
                  
         code *= 3;

#ifdef XBOX         
         pDst[0] = (x << 16) | pAlphaSelectorCodebook[code];
         pDst[1] = (pAlphaSelectorCodebook[code + 1] << 16) | pAlphaSelectorCodebook[code + 2];
#else
         pDst[0] = x | (pAlphaSelectorCodebook[code] << 16);
         pDst[1] = pAlphaSelectorCodebook[code + 1] | (pAlphaSelectorCodebook[code + 2] << 16);
#endif         
            
         code = (uint)bitBuf & colorIndexMask;
         maxColorCodebookCode = Math::Max<int>(maxColorCodebookCode, code);
         pDst[2] = pColorCodebook[code];

         code = (((uint)bitBuf) >> bitsPerColorIndex) & colorSelectorIndexMask;
         
         removeBits2(bitsForBothColorIndices, pSrc, bitBuf, bitBufLeft);
         
         maxColorSelectorCodebookCode = Math::Max<int>(maxColorSelectorCodebookCode, code);

         pDst[3] = pColorSelectorCodebook[code];
         
         pDst += 4;

         numBlocksLeft--;
      }         
   }
   
   return true;
}   

//------------------------------------------------------------------------------------------------------
// BDXTQPack::decodeSegmentDXN
//------------------------------------------------------------------------------------------------------
bool BDXTQPack::decodeSegmentDXN(
   BYTE* __restrict pWCDst,
   const BDXTQHeader& header,
   const BYTE* pCachedMem,
   const BDXTQHeader::BSegment& segment)
{
   const uint cBytesPerBlock = 16;
   
   WORD* pAlphaCodebook                   = (WORD*)(pCachedMem + header.mAlphaCodebookOfs);
   WORD* pAlphaSelectorCodebook           = (WORD*)(pCachedMem + header.mAlphaSelectorCodebookOfs);   
            
   const uint bitsPerAlphaIndex           = Math::CodeSize(header.mAlphaCodebookSize + MAX_RUN_CODES);
   const uint alphaIndexMask              = (1U << bitsPerAlphaIndex) - 1U;

   const uint bitsPerAlphaSelectorIndex   = Math::CodeSize(header.mAlphaSelectorCodebookSize);
   const uint alphaSelectorIndexMask      = (1U << bitsPerAlphaSelectorIndex) - 1U;
   
   const uint bitsForBothAlphaIndices     = bitsPerAlphaIndex + bitsPerAlphaSelectorIndex;
   
   const uint firstRLECode = (1 << bitsPerAlphaIndex) - MAX_RUN_SIZE;

   DWORD* __restrict pDst = (DWORD*)(pWCDst + segment.mFirstBlock * cBytesPerBlock);

   const uchar* pSrc = pCachedMem + segment.mDataOfs;
   uint64 bitBuf = readLittleEndianDWORD(pSrc); 
   pSrc += 4;
   int bitBufLeft = 0;

   uint numBlocksLeft = segment.mNumBlocks;
   while (numBlocksLeft)  
   {
      uint code = (uint)bitBuf & alphaIndexMask;
      
      if (code >= firstRLECode)
      {
         uint runSize = code - firstRLECode + MIN_RUN_SIZE;
         if (runSize > numBlocksLeft)
            return false;
            
         removeBits2(bitsPerAlphaIndex, pSrc, bitBuf, bitBufLeft);
            
         DWORD a = pAlphaCodebook[bitBuf & alphaIndexMask];

         code = ((uint)bitBuf >> bitsPerAlphaIndex) & alphaSelectorIndexMask;

         removeBits2(bitsForBothAlphaIndices, pSrc, bitBuf, bitBufLeft);

         code *= 3;

#ifdef XBOX
         a = (a << 16) | pAlphaSelectorCodebook[code];
         DWORD b = (pAlphaSelectorCodebook[code + 1] << 16) | pAlphaSelectorCodebook[code + 2];
#else         
         a |= (pAlphaSelectorCodebook[code] << 16);
         DWORD b = pAlphaSelectorCodebook[code + 1] | (pAlphaSelectorCodebook[code + 2] << 16);
#endif         

         DWORD c = pAlphaCodebook[bitBuf & alphaIndexMask];
         
         code = ((uint)bitBuf >> bitsPerAlphaIndex) & alphaSelectorIndexMask;                  
         
         removeBits2(bitsForBothAlphaIndices, pSrc, bitBuf, bitBufLeft);
         
         code *= 3;

#ifdef XBOX
         c = (c << 16) | pAlphaSelectorCodebook[code];
         DWORD d = (pAlphaSelectorCodebook[code + 1] << 16) | pAlphaSelectorCodebook[code + 2];
#else
         c |= (pAlphaSelectorCodebook[code] << 16);
         DWORD d = pAlphaSelectorCodebook[code + 1] | (pAlphaSelectorCodebook[code + 2] << 16);
#endif         
         
         DWORD* pEndDst = pDst + (runSize << 2);

         do
         {  
            pDst[0] = a;
            pDst[1] = b;
            pDst[2] = c;
            pDst[3] = d;
            pDst += 4;
         } while (pDst != pEndDst);

         numBlocksLeft -= runSize;
      }
      else
      {
         DWORD a = pAlphaCodebook[code];

         code = ((uint)bitBuf >> bitsPerAlphaIndex) & alphaSelectorIndexMask;

         removeBits2(bitsForBothAlphaIndices, pSrc, bitBuf, bitBufLeft);

         code *= 3;
#ifdef XBOX
         a = (a << 16) | pAlphaSelectorCodebook[code];
         pDst[0] = a;
         pDst[1] = (pAlphaSelectorCodebook[code + 1] << 16) | pAlphaSelectorCodebook[code + 2];
#else         
         a |= (pAlphaSelectorCodebook[code] << 16);
         pDst[0] = a;
         pDst[1] = pAlphaSelectorCodebook[code + 1] | (pAlphaSelectorCodebook[code + 2] << 16);
#endif         

         DWORD c = pAlphaCodebook[bitBuf & alphaIndexMask];

         code = ((uint)bitBuf >> bitsPerAlphaIndex) & alphaSelectorIndexMask;                  

         removeBits2(bitsForBothAlphaIndices, pSrc, bitBuf, bitBufLeft);

         code *= 3;

#ifdef XBOX
         c = (c << 16) | pAlphaSelectorCodebook[code];
         pDst[2] = c;
         pDst[3] = (pAlphaSelectorCodebook[code + 1] << 16) | pAlphaSelectorCodebook[code + 2];
#else         
         c |= (pAlphaSelectorCodebook[code] << 16);
         pDst[2] = c;
         pDst[3] = pAlphaSelectorCodebook[code + 1] | (pAlphaSelectorCodebook[code + 2] << 16);
#endif         
         
         pDst += 4;

         numBlocksLeft--;
      }         
   }
      
   return true;
}   

//------------------------------------------------------------------------------------------------------
// BDXTQPack::decodeSegment
//------------------------------------------------------------------------------------------------------
bool BDXTQPack::decodeSegment(
   BYTE* __restrict pDst,
   const BDXTQHeader& header,
   const BYTE* pCachedMem,
   const BDXTQHeader::BSegment& segment,
   bool hasColorBlocks, bool hasAlpha0Blocks, bool hasAlpha1Blocks,
   bool endianSwap)      
{
   hasColorBlocks;
   bool result;
   uint bytesPerBlock;
   if (!hasAlpha0Blocks)
   {
      bytesPerBlock = 8;
      result = decodeSegmentDXT1(pDst, header, pCachedMem, segment);
   }
   else if (!hasAlpha1Blocks)
   {
      bytesPerBlock = 16;
      result = decodeSegmentDXT5(pDst, header, pCachedMem, segment);
   }
   else
   {
      bytesPerBlock = 16;
      result = decodeSegmentDXN(pDst, header, pCachedMem, segment);
   }

   if (!result)
      return false;
           
   if (endianSwap)
      EndianSwitchWords((WORD*)(pDst + segment.mFirstBlock * bytesPerBlock), (segment.mNumBlocks * bytesPerBlock) >> 1);
      
   return true;      
}   

//------------------------------------------------------------------------------------------------------
// BDXTQPack::unpackDXTQToDXT
//------------------------------------------------------------------------------------------------------
bool BDXTQPack::unpackDXTQToDXT(
   const BDDXTextureInfo& srcTextureInfo,
   const BYTE* pSrcData, uint srcDataSize,
   BDDXTextureInfo& dstTextureInfo, BByteArray& dstData,
   bool platformSpecificData)
{
   const uint width = srcTextureInfo.mWidth;
   const uint height = srcTextureInfo.mHeight;
   const uint numMipChainLevels = srcTextureInfo.mNumMipChainLevels;
   const eDDXDataFormat format = srcTextureInfo.mDataFormat;

   const BECFFileReader ecfReader(BConstDataBuffer(pSrcData, srcDataSize));
   
   if (!ecfReader.checkHeader(true))
      return false;
   
   if (ecfReader.getHeader()->getID() != cDXTQID)
      return false;
   
   if (ecfReader.getHeader()->getNumChunks() < 2)
      return false;
   
   if (ecfReader.getChunkDataLenByIndex(0) != sizeof(BDXTQHeader))
      return false;
   const BDXTQHeader& header = *(const BDXTQHeader*)ecfReader.getChunkDataByIndex(0);
   if (header.mVersion != BDXTQHeader::cVersion)
      return false;
   
   const BYTE* pCachedMem =  ecfReader.getChunkDataByIndex(1);  
   const uint cachedMemSize = ecfReader.getChunkDataLenByIndex(1);
   
   if (!cachedMemSize)
      return false;

   if ((cachedMemSize - header.mD3DTexOfs) < sizeof(IDirect3DTexture9))
      return false;
      
   IDirect3DTexture9 d3dTex = *(const IDirect3DTexture9*)(pCachedMem + header.mD3DTexOfs);
   if (cLittleEndianNative)
      XGEndianSwapMemory(&d3dTex, &d3dTex, XGENDIAN_8IN32, sizeof(DWORD), sizeof(IDirect3DTexture9) / sizeof(DWORD));
      
   XGTEXTURE_DESC d3dTexDesc;
   XGGetTextureDesc(&d3dTex, 0, &d3dTexDesc);
   const DWORD dwNumLevels = d3dTex.Format.MaxMipLevel + 1;

   if ((d3dTexDesc.Width != width) || (d3dTexDesc.Height != height) || (dwNumLevels != (numMipChainLevels + 1U)))
      return false;
      
   const D3DFORMAT d3dFormat = getD3DDataFormatFromDDX(format);
   if (d3dFormat == D3DFMT_UNKNOWN)
      return false;
   if (d3dTexDesc.Format != d3dFormat)    
      return false;
   
   const uint bytesPerBlock = getDDXDataFormatDXTBlockSize(format);
   const uint d3dTexBaseSize = header.mBaseBlocks * bytesPerBlock;
   const uint d3dTexMipSize = header.mMipBlocks * bytesPerBlock;
   d3dTexMipSize;
   const uint totalBlocks = header.mBaseBlocks + header.mMipBlocks;
            
   bool hasColorBlocks = false;
   bool hasAlpha0Blocks = false;
   bool hasAlpha1Blocks = false;

   switch (format)
   {
      case cDDXDataFormatDXT1Q: 
      {
         hasColorBlocks = true; 
         break;
      }
      case cDDXDataFormatDXT5Q: 
      case cDDXDataFormatDXT5HQ: 
      {
         hasColorBlocks = true; 
         hasAlpha0Blocks = true; 
         break;
      }
      case cDDXDataFormatDXNQ: 
      {
         hasAlpha0Blocks = true;
         hasAlpha1Blocks = true; 
         break;
      }
   }
   
   dstTextureInfo = srcTextureInfo;
      
   switch (srcTextureInfo.mDataFormat)
   {
      case cDDXDataFormatDXT1Q:  dstTextureInfo.mDataFormat = cDDXDataFormatDXT1;  break;
      case cDDXDataFormatDXT5Q:  dstTextureInfo.mDataFormat = cDDXDataFormatDXT5;  dstTextureInfo.mHasAlpha = true; break;
      case cDDXDataFormatDXT5HQ: dstTextureInfo.mDataFormat = cDDXDataFormatDXT5H; dstTextureInfo.mHasAlpha = true; break;
      case cDDXDataFormatDXNQ:   dstTextureInfo.mDataFormat = cDDXDataFormatDXN;   break;
      default:
         return false;
   }
   
   dstTextureInfo.mOrigDataFormat = format;
   dstTextureInfo.mPackType = cDDXTDPTMipsRaw;
      
   BByteArray tempData;
   
   BByteArray* pDstArray = &tempData;
   uint dstArrayOfs = 0;
   bool endianSwap = false;
   
   if (platformSpecificData)
   {
      dstTextureInfo.mPlatform = cDDXPlatformXbox;
                  
      dstArrayOfs = dstData.getSize();
      
      dstData.resize(dstData.size() + sizeof(IDirect3DTexture9) + Utils::AlignUpValue(totalBlocks * bytesPerBlock, 1024));
      
      memcpy(dstData.getPtr() + dstArrayOfs, &d3dTex, sizeof(IDirect3DTexture9));
      
      dstArrayOfs += sizeof(IDirect3DTexture9);
      
      pDstArray = &dstData;
   }
   else
   {
      tempData.resize(totalBlocks * bytesPerBlock);
      endianSwap = true;
   }
   
BTimer timer;
timer.start();

#if TRACE_RECORDING
   static bool traceRecording;
   if (traceRecording)
   {
      XTraceStartRecording("e:\\decompress.bin");
   }
#endif   

   for (uint segmentIndex = 0; segmentIndex < header.mNumBaseSegments; segmentIndex++)
   {
      if (!decodeSegment(
         pDstArray->getPtr() + dstArrayOfs,
         header, pCachedMem,
         header.mBaseIndices[segmentIndex],
         hasColorBlocks,
         hasAlpha0Blocks,
         hasAlpha1Blocks,
         endianSwap))
      {
         return false;
      }
   }
   
   for (uint segmentIndex = 0; segmentIndex < header.mNumMipSegments; segmentIndex++)
   {
      if (!decodeSegment(
         pDstArray->getPtr() + dstArrayOfs,
         header, pCachedMem,
         header.mMipIndices[segmentIndex],
         hasColorBlocks,
         hasAlpha0Blocks,
         hasAlpha1Blocks,
         endianSwap))
      {         
         return false;
      }         
   }
   
#if TRACE_RECORDING
   if (traceRecording)
   {
      XTraceStopRecording();      
      
      DebugBreak();
      
      traceRecording = false;
   }
#endif      
   
double t = timer.getElapsedSeconds();
double cCPUClockRate = 3200000000;
if ((hasColorBlocks) && (hasAlpha0Blocks))
   trace("%u blocks, %2.6f, %2.6f cycles per block", totalBlocks, t, (t * cCPUClockRate) / totalBlocks);
   
   if (platformSpecificData)
      return true;
         
   for (uint mipLevel = 0; mipLevel < (numMipChainLevels + 1U); mipLevel++)
   {
      uint mipWidth, mipHeight;
      BDDXUtils::calcMipDimension(mipWidth, mipHeight, width, height, mipLevel);

      const int widthInBlocks = Math::Max<int>(1, mipWidth >> 2);
      const int heightInBlocks = Math::Max<int>(1, mipHeight >> 2);
      
      const uint dstPitch = widthInBlocks * bytesPerBlock; 
      
      const uint dstSize = dstPitch * heightInBlocks;

      const uint outStreamSize = dstData.size();
      dstData.resize(outStreamSize + dstSize);

      BYTE* pDst = dstData.getPtr() + outStreamSize;

      BYTE* pSrc = tempData.getPtr();

      if (mipLevel > 0)
         pSrc += d3dTexBaseSize;
      
      const uint mipLevelOffset = XGGetMipLevelOffset( &d3dTex, 0, mipLevel );
      pSrc += mipLevelOffset;

      const DWORD flags = d3dTex.Format.PackedMips ? 0 : XGTILE_NONPACKED;

      XGUntileTextureLevel(
         width,
         height,
         mipLevel,
         XGGetGpuFormat(d3dTexDesc.Format),
         flags,
         pDst,
         dstPitch,
         NULL,
         pSrc,
         NULL);
   }            
             
   return true;
}

//------------------------------------------------------------------------------------------------------
// BDXTQPack::BDXTQPack
//------------------------------------------------------------------------------------------------------
BDXTQPack::BDXTQPack() :
   mCachedMemPool(32*1024*1024U)
{
}


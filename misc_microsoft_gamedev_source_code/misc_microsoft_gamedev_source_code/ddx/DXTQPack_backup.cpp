//==============================================================================
// File: DXTQPack.h
//
// Copyright (c) 2006 Ensemble Studios
//==============================================================================
#include "xcore.h"
#include "math\halfFloat.h"
#include "file\ecfUtils.h"
#include "stream\cfileStream.h"
#include "threading\win32Event.h"
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

#ifdef XBOX
#include "threading\workDistributor.h"
#endif

#define THREADED_DECOMPRESSION 0

#ifdef XBOX
#define TRACE_RECORDING 1
#endif

#if TRACE_RECORDING
#include "tracerecording.h"
#pragma comment( lib, "tracerecording.lib" )
#pragma comment(lib, "xbdm.lib")
#endif

#ifdef XBOX
#include <pmcpb.h>
#include <pmcpbsetup.h>
#endif

#define MAX_RUN_CODES 16
#define MIN_RUN_SIZE 2
#define MAX_RUN_SIZE (MAX_RUN_CODES - 1 + MIN_RUN_SIZE)

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
// BDXTQPack::createBlockIndices
//------------------------------------------------------------------------------------------------------ 
void BDXTQPack::createBlockIndices(
   BBlockIndicesArray& blockIndices,
   uint firstBlock, uint numBlocks,
   BDXTQHeader& header,
   const UShortArray& colorIndices,
   const UShortArray& colorSelectorIndices,
   const UShortArray& alphaIndices0,
   const UShortArray& alphaIndices1,
   const UShortArray& alphaSelectorIndices)
{
   const bool hasColorBlocks = colorIndices.getSize() > 0;
   const bool hasAlpha0Blocks = alphaIndices0.getSize() > 0;
   const bool hasAlpha1Blocks = alphaIndices1.getSize() > 0;
      
   int curRunSize = 0;
   int curColorIndex = -1;
   int curColorSelectorIndex = -1;
   int curAlpha0Index = -1;
   int curAlpha1Index = -1;
   int curAlpha0SelectorIndex = -1;
   int curAlpha1SelectorIndex = -1;

   const uint totalTextureBlocks = header.mBaseBlocks + header.mMipBlocks;

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

      if (isRun)
      {
         BDEBUG_ASSERT(blockIndex != lastBlock);
         curRunSize++;
      }
      else
      {
         if (curRunSize > 0)
         {
            totalEncodedBlocks += curRunSize;

            BBlockIndices block;
            block.mRunLength             = (short)curRunSize;
            block.mColorIndex            = hasColorBlocks ? (ushort)curColorIndex : 0;
            block.mColorSelectorIndex    = hasColorBlocks ? (ushort)curColorSelectorIndex : 0;
            block.mAlpha0Index           = hasAlpha0Blocks ? (ushort)alpha0Index : 0;
            block.mAlpha0SelectorIndex   = hasAlpha0Blocks ? (ushort)alpha0SelectorIndex : 0;
            block.mAlpha1Index           = hasAlpha1Blocks ? (ushort)alpha1Index : 0; 
            block.mAlpha1SelectorIndex   = hasAlpha1Blocks ? (ushort)alpha1SelectorIndex : 0;
            blockIndices.pushBack(block);
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
}  

// 64-bit packets
// 4:   4, 5,  8, 9,  8,  9,  8,  9,  3+1
//      0, 4,  9, 17, 26, 34, 43, 51, 60

// 3:   10 11, 10 11, 10 11, 1
//      0  10  21 31  41 52

//------------------------------------------------------------------------------------------------------
// BDXTQPack::create3Packet
//------------------------------------------------------------------------------------------------------ 
uint64 BDXTQPack::create3Packet(uint64 c0, uint64 s0, uint64 c1, uint64 s1, uint64 c2, uint64 s2)
{
   BDEBUG_ASSERT(c0 <= 1023 && s0 <= 2047);
   BDEBUG_ASSERT(c1 <= 1023 && s1 <= 2047);
   BDEBUG_ASSERT(c2 <= 1023 && s2 <= 2047);
   
   return c0 | (s0 << 10U) | (c1 << 21U) | (s1 << 31U) | (c2 << 42U) | (s2 << 52U);
}       

//------------------------------------------------------------------------------------------------------
// BDXTQPack::create4Packet
//------------------------------------------------------------------------------------------------------ 
uint64 BDXTQPack::create4Packet(uint64 c0, uint64 s0, uint64 c1, uint64 s1, uint64 c2, uint64 s2, uint64 c3, uint64 s3, uint64 runLen)
{
   BDEBUG_ASSERT(c0 <= 15 && s0 <= 31);
   BDEBUG_ASSERT(c1 <= 255 && s1 <= 511);
   BDEBUG_ASSERT(c2 <= 255 && s2 <= 511);
   BDEBUG_ASSERT(c3 <= 255 && s3 <= 511);
   BDEBUG_ASSERT((runLen >= 1) && (runLen <= 8));
   
   BDEBUG_ASSERT( !((c0 == 15) && (c1 == 31)) );
      
   return c0 | (s0 << 4U) | (c1 << 9U) | (s1 << 17U) | (c2 << 26U) | (s2 << 34U) | (c3 << 43U) | (s3 << 51U) | ((runLen - 1U) << 60U) | 0x8000000000000000U;
}       

//------------------------------------------------------------------------------------------------------
// BDXTQPack::create4CPacket
//------------------------------------------------------------------------------------------------------ 
uint64 BDXTQPack::create4CPacket(BBlockIndices& i0, BBlockIndices& i1, BBlockIndices& i2, BBlockIndices& i3, uint runLength)
{
   const uint64 packetBits = create4Packet(
      i0.mColorIndex, i0.mColorSelectorIndex, 
      i1.mColorIndex, i1.mColorSelectorIndex, 
      i2.mColorIndex, i2.mColorSelectorIndex,
      i3.mColorIndex, i3.mColorSelectorIndex,
      runLength);

   BDEBUG_ASSERT(i0.mRunLength >= (int)runLength);
   i0.mRunLength = (short)(i0.mRunLength - runLength);

   BDEBUG_ASSERT(i1.mRunLength >= 1);
   i1.mRunLength--;

   BDEBUG_ASSERT(i2.mRunLength >= 1);
   i2.mRunLength--;
   
   BDEBUG_ASSERT(i3.mRunLength >= 1);
   i3.mRunLength--;

#if 0   
   if (&i0 != &i3)
   {
      BDEBUG_ASSERT(i0.mRunLength == 0);
   }
#endif   
   
   return packetBits;   
}

//------------------------------------------------------------------------------------------------------
// BDXTQPack::create4LPacket
//------------------------------------------------------------------------------------------------------ 
uint64 BDXTQPack::create4LPacket(BBlockIndices& i0, BBlockIndices& i1)
{
   const uint cMinRunLength = 4;
   BDEBUG_ASSERT(i0.mRunLength >= cMinRunLength);
   
   const uint runLengthCode = Math::Min<int>(i0.mRunLength - cMinRunLength, 511);
   const uint runLength = runLengthCode + cMinRunLength;
   
   uint64 packetBits;
   if (i0.mRunLength == runLength)
   {
      packetBits = 0x8000000000000000U | 511 | (((uint64)runLengthCode) << 9) | 
         (((uint64)i0.mColorIndex) << 18) | (((uint64)i0.mColorSelectorIndex) << 28) | 
         (((uint64)i1.mColorIndex) << 39) | (((uint64)i1.mColorSelectorIndex) << 49);
            
      BDEBUG_ASSERT(i0.mRunLength == (int)runLength);
      i0.mRunLength = 0;
      
      BDEBUG_ASSERT(i1.mRunLength >= 1);
      i1.mRunLength--;
   }
   else
   {
      packetBits = 0x8000000000000000U | 511 | (((uint64)runLengthCode) << 9) | 
         (((uint64)i0.mColorIndex) << 18) | (((uint64)i0.mColorSelectorIndex) << 28) | 
         (((uint64)i0.mColorIndex) << 39) | (((uint64)i0.mColorSelectorIndex) << 49);

      BDEBUG_ASSERT(i0.mRunLength >= (int)(runLength + 1));
      i0.mRunLength = (short)(i0.mRunLength - runLength - 1);
   }      

   return packetBits;   
}

//------------------------------------------------------------------------------------------------------
// BDXTQPack::create3CPacket
//------------------------------------------------------------------------------------------------------ 
uint64 BDXTQPack::create3CPacket(BBlockIndices& i0, BBlockIndices& i1, BBlockIndices& i2)
{
   const uint64 packetBits = create3Packet(
      i0.mColorIndex, i0.mColorSelectorIndex, 
      i1.mColorIndex, i1.mColorSelectorIndex, 
      i2.mColorIndex, i2.mColorSelectorIndex);
      
   BDEBUG_ASSERT(i0.mRunLength >= 1);
   i0.mRunLength--;      
   
   BDEBUG_ASSERT(i1.mRunLength >= 1);
   i1.mRunLength--;
   
   BDEBUG_ASSERT(i2.mRunLength >= 1);
   i2.mRunLength--;

#if 0   
   if (&i0 != &i2)
   {
      BDEBUG_ASSERT(i0.mRunLength == 0);
   }
#endif   
   
   return packetBits;
}

//------------------------------------------------------------------------------------------------------
// BDXTQPack::createSegmentIndices
//------------------------------------------------------------------------------------------------------ 
void BDXTQPack::createSegmentIndices(
   UInt64Array& output,
   uint firstBlock, uint numBlocks,
   BDXTQHeader& header,
   const UShortArray& colorIndices,
   const UShortArray& colorSelectorIndices,
   const UShortArray& alphaIndices0,
   const UShortArray& alphaIndices1,
   const UShortArray& alphaSelectorIndices)
{
   BDEBUG_ASSERT(0 == output.getSize());
   
   const bool hasColorBlocks = colorIndices.getSize() > 0;
   const bool hasAlpha0Blocks = alphaIndices0.getSize() > 0;
   const bool hasAlpha1Blocks = alphaIndices1.getSize() > 0;
   
   BBlockIndicesArray blockIndices;
   
   createBlockIndices(blockIndices, firstBlock, numBlocks, header, colorIndices, colorSelectorIndices, alphaIndices0, alphaIndices1, alphaSelectorIndices);
   
   const uint numBlockIndices = blockIndices.getSize();
   
   BBlockIndices dummyIndices;
   dummyIndices.clear();
   dummyIndices.mRunLength = 1;
   blockIndices.pushBack(dummyIndices);
   blockIndices.pushBack(dummyIndices);
   blockIndices.pushBack(dummyIndices);
         
   uint curIndex = 0;
   
   uint num4Packets = 0;
   uint num3Packets = 0;
   
   uint totalBlocksEncoded = 0;
         
   do
   {
      BBlockIndices& i0 = blockIndices[curIndex];
      BBlockIndices& i1 = blockIndices[curIndex + 1];
      BBlockIndices& i2 = blockIndices[curIndex + 2];
      BBlockIndices& i3 = blockIndices[curIndex + 3];
      
      // 64-bit packets
      // 4:   4, 5,  8, 9,  8,  9,  8,  9,  3+1
      // 3:   10 11, 10 11, 10 11, 1
            
      if (!i0.mRunLength)
      {
         curIndex++;
      }
      else
      {
         uint64 packet0Bits = 0;
         uint64 packet1Bits = 0;
         
         bool created4Packet = false;
                  
         // packet4:
         
         // 1. i0 i1 i2 i3 1-8 
         // 2. i0 i1 i2 i2 1-8 
         // 3. i0 i1 i1 i2 1-8 
         // 4. i0 i1 i1 i1 1-8 
         // 5. i0 i0 i1 i1 2-9 
         // 6. i0 i0 i0 i1 3-10 
         // 7. i0 i0 i0 i0 4-11 

         if ( ((!i0.validRLE()) && (i0.mRunLength >= 5)) || (i0.mRunLength >= 12) )
         {
            packet0Bits = create4LPacket(i0, i1);
            created4Packet = true;
         }
         else if (i0.validRLE()) 
         {
            // case 7 (very long run)
            if (i0.mRunLength >= 11)
            {
               packet0Bits = create4CPacket(i0, i0, i0, i0, 8);
               created4Packet = true;
            }
            // case 1
            else if ( (i0.mRunLength <= 8) && (i1.validFour()) && (i1.mRunLength == 1) && (i2.validFour()) && (i2.mRunLength == 1) && (i3.validFour()) )
            {
               packet0Bits = create4CPacket(i0, i1, i2, i3, i0.mRunLength);
               created4Packet = true;
            }
            // case 2
            else if ( (i0.mRunLength <= 8) && (i1.validFour()) && (i1.mRunLength == 1) && (i2.validFour()) && (i2.mRunLength >= 2) )
            {
               packet0Bits = create4CPacket(i0, i1, i2, i2, i0.mRunLength);
               created4Packet = true;
            }
            // case 3
            else if ( (i0.mRunLength <= 8) && (i1.validFour()) && (i1.mRunLength == 2) && (i2.validFour()) )
            {
               packet0Bits = create4CPacket(i0, i1, i1, i2, i0.mRunLength);
               created4Packet = true;
            }
            // case 4
            else if ( (i0.mRunLength <= 8) && (i1.validFour()) && (i1.mRunLength >= 3) )
            {
               packet0Bits = create4CPacket(i0, i1, i1, i1, i0.mRunLength);
               created4Packet = true;
            }
            // case 5
            else if ( (i0.mRunLength >= 2) && (i0.mRunLength <= 9) && (i1.validFour()) && (i1.mRunLength >= 2) )
            {
               packet0Bits = create4CPacket(i0, i0, i1, i1, i0.mRunLength - 1);
               created4Packet = true;
            }
            // case 6
            else if ( (i0.mRunLength >= 3) && (i0.mRunLength <= 10) && (i1.validFour()) )
            {
               packet0Bits = create4CPacket(i0, i0, i0, i1, i0.mRunLength - 2);
               created4Packet = true;
            }
            // case 7 (i0 runLength must be between 4 and 10)
            else if (i0.mRunLength >= 4)
            {
               packet0Bits = create4CPacket(i0, i0, i0, i0, i0.mRunLength - 3);
               created4Packet = true;
            }
         }            
         
         // packet3:
         // 1. i0 i0 i0 3+
         // 2. i0 i0 i1 2,1+
         // 3. i0 i1 i1 1,2+
         // 4. i0 i1 i2 1,1,1+
         
         if (!created4Packet)
         {
            num3Packets++;
            
            // case 1
            if (i0.mRunLength >= 3)
            {
               packet0Bits = create3CPacket(i0, i0, i0);
            }
            // case 2
            else if (i0.mRunLength == 2)
            {
               packet0Bits = create3CPacket(i0, i0, i1);
            }
            // case 3
            else if (i1.mRunLength >= 2)
            {
               // i0.mRunLength must be 1
               packet0Bits = create3CPacket(i0, i1, i1);
            }
            // case 4
            else
            {
               // i0.mRunLength must be 1, and i1.mRunLength must be 1
               packet0Bits = create3CPacket(i0, i1, i2);
            }
         }
         else
         {  
            num4Packets++;  
         }
         
         if (packet0Bits & 0x8000000000000000)
         {
            uint runLen;
            if ((packet0Bits & 511) == 511)
            {
               runLen = (uint)(((packet0Bits >> 9) & 511) + 4);
               totalBlocksEncoded += 1 + runLen;
            }
            else
            {
               runLen = (uint)(((packet0Bits >> 60) & 7) + 1);
               totalBlocksEncoded += 3 + runLen;
            }
         }
         else
            totalBlocksEncoded += 3;
                        
         output.pushBack(packet0Bits);
         if (hasAlpha0Blocks)
            output.pushBack(packet1Bits);
      }            
   
   } while (curIndex < numBlockIndices);
   
   BDEBUG_ASSERT((totalBlocksEncoded >= numBlocks) && (totalBlocksEncoded <= numBlocks + 3));
   
   if (cLittleEndianNative)
      EndianSwitchQWords(output.getPtr(), output.getSize());
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
   if (header.mBaseBlocks > 2048)
      numBaseSegments = BDXTQHeader::cNumSegments;
   else if (header.mBaseBlocks)
      numBaseSegments = 1;
   
   baseBlocksPerSegment = numBaseSegments ? (header.mBaseBlocks / numBaseSegments) : 0;
   
   uint numMipSegments = 0;
   uint mipBlocksPerSegment = 0;
   if (header.mMipBlocks > 2048)
      numMipSegments = BDXTQHeader::cNumSegments;
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
      UInt64Array packets;
      
      createSegmentIndices(
         packets,
         header.mBaseIndices[i].mFirstBlock,
         header.mBaseIndices[i].mNumBlocks,
         header,
         colorIndices,
         colorSelectorIndices,
         alphaIndices0,
         alphaIndices1,
         alphaSelectorIndices);
         
      header.mBaseIndices[i].mDataLen = packets.getSizeInBytes();
      header.mBaseIndices[i].mDataOfs = mCachedMemPool.alloc(packets.getSizeInBytes(), 16);
      memcpy(mCachedMemPool.getPtr(header.mBaseIndices[i].mDataOfs), packets.getPtr(), packets.getSizeInBytes());
   }         
   
   for (uint i = 0; i < numMipSegments; i++)
   {
      UInt64Array packets;
      
      createSegmentIndices(
         packets,
         header.mMipIndices[i].mFirstBlock,
         header.mMipIndices[i].mNumBlocks,
         header,
         colorIndices,
         colorSelectorIndices,
         alphaIndices0,
         alphaIndices1,
         alphaSelectorIndices);
      
      header.mMipIndices[i].mDataLen = packets.getSizeInBytes();
      header.mMipIndices[i].mDataOfs = mCachedMemPool.alloc(packets.getSizeInBytes(), 16);
      memcpy(mCachedMemPool.getPtr(header.mMipIndices[i].mDataOfs), packets.getPtr(), packets.getSizeInBytes());
   }         
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
   
   uint colorCodebookSize           = (1024/divisor);
   uint selectorCodebookSize        = (2048/divisor);
   uint alphaCodebookSize           = (hasAlpha1Blocks ? (2048/divisor) : (1024/divisor));
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
   ecfBuilder.addChunk(1, mCachedMemPool.getPtr(0), mCachedMemPool.getSize()).setAlignmentLog2(4);
      
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

typedef void* BWriteState;

inline BWriteState BeginWrite(void* p, void* pEnd, uint cacheLinesToWriteAhead)
{
#ifdef XBOX
   BWriteState state = Utils::AlignUp(p, 128);  
   const uint ofs = (cacheLinesToWriteAhead << 7);
   if (((uchar*)state + ofs + 127) < (uchar*)pEnd)
   {
      __dcbz128(ofs, state);
   }
#endif   
   return p;
}

inline BWriteState UpdateWrite(BWriteState prevP, void* p, void* pEnd, uint cacheLinesToWriteAhead)
{
#ifdef XBOX
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
#endif   
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

   const uchar* __restrict pSrc = pCachedMem + segment.mDataOfs;
   //const uchar* pSrcEnd = pSrc + segment.mDataLen;
   const uchar* __restrict pSrcLast = pSrc + segment.mDataLen - 8;
   Utils::BPrefetchState srcPrefetch = Utils::BeginPrefetch(pSrc, pSrcLast, 2);

#ifdef XBOX   
   BDEBUG_ASSERT(((uint)pSrc & 7) == 0);
#endif   

   DWORD* __restrict pDst = (DWORD*)(pWCDst + segment.mFirstBlock * cBytesPerBlock);
   DWORD* __restrict pDstEnd = (DWORD*)((uchar*)pDst + segment.mNumBlocks * cBytesPerBlock);
   BWriteState dstWrite = BeginWrite(pDst, pDstEnd, 3);

   DWORD* __restrict pColorCodebook                  = (DWORD*)(pCachedMem + header.mColorCodebookOfs);
   DWORD* __restrict pColorSelectorCodebook          = (DWORD*)(pCachedMem + header.mColorSelectorCodebookOfs);

   while (pSrc != pSrcLast)
   {
      dstWrite = UpdateWrite(dstWrite, pDst, pDstEnd, 3);
      
      uint64 packet = *(const uint64*)pSrc;
      pSrc += 8;
      
#ifndef XBOX            
      if (cLittleEndianNative)
         EndianSwitchQWords(&packet, 1);
#endif      

      srcPrefetch = Utils::UpdatePrefetch(srcPrefetch, pSrc, pSrcLast, 2);
                  
      if ((int64)packet >= 0)
      {
a++;         
         pDst[0] = pColorCodebook[packet & 1023];
         pDst[1] = pColorSelectorCodebook[(packet >> 10) & 2047];

         pDst[2] = pColorCodebook[(packet >> 21) & 1023];
         pDst[3] = pColorSelectorCodebook[(packet >> 31) & 2047];

         pDst[4] = pColorCodebook[(packet >> 42) & 1023];
         pDst[5] = pColorSelectorCodebook[(packet >> 52) & 2047];

         pDst += 6;
      }
      else
      {
         if ((packet & 511) == 511)
         {
b++;         
            uint runLen = (uint)((packet >> 9) & 511) + 4;
                                    
            const uint bc0 = pColorCodebook[(packet >> 18) & 1023];
            const uint bs0 = pColorSelectorCodebook[(packet >> 28) & 2047];
                                    
            do 
            {
               pDst[0] = bc0;
               pDst[1] = bs0;
               pDst += 2;
               runLen--;
            } while(runLen);
                        
            pDst[0] = pColorCodebook[(packet >> 39) & 1023];
            pDst[1] = pColorSelectorCodebook[(packet >> 49) & 2047];
            pDst += 2;
         }
         else
         {
c++;         
            uint runLen = (uint)((packet >> 60) & 7) + 1;
                                               
            const uint bc0 = pColorCodebook[(packet      ) & 15];
            const uint bs0 = pColorSelectorCodebook[(packet >>  4) & 31];
            
            do 
            {
               pDst[0] = bc0;
               pDst[1] = bs0;
               pDst += 2;
               runLen--;
            } while(runLen);
                                    
            pDst[0] = pColorCodebook[(packet >>  9) & 255];
            pDst[1] = pColorSelectorCodebook[(packet >> 17) & 511];
            pDst[2] = pColorCodebook[(packet >> 26) & 255];
            pDst[3] = pColorSelectorCodebook[(packet >> 34) & 511];
            pDst[4] = pColorCodebook[(packet >> 43) & 255];
            pDst[5] = pColorSelectorCodebook[(packet >> 51) & 511];
            
            pDst += 6;
         }            
      }
      
      if (pSrc == pSrcLast)
         break;
                  
      packet = *(const uint64*)pSrc;
      pSrc += 8;
      
#ifndef XBOX            
      if (cLittleEndianNative)
         EndianSwitchQWords(&packet, 1);
#endif      
                        
      if ((int64)packet >= 0)
      {
         pDst[0] = pColorCodebook[packet & 1023];
         pDst[1] = pColorSelectorCodebook[(packet >> 10) & 2047];

         pDst[2] = pColorCodebook[(packet >> 21) & 1023];
         pDst[3] = pColorSelectorCodebook[(packet >> 31) & 2047];

         pDst[4] = pColorCodebook[(packet >> 42) & 1023];
         pDst[5] = pColorSelectorCodebook[(packet >> 52) & 2047];

         pDst += 6;
      }
      else
      {
         if ((packet & 511) == 511)
         {
            uint runLen = (uint)((packet >> 9) & 511) + 4;
                                    
            const uint bc0 = pColorCodebook[(packet >> 18) & 1023];
            const uint bs0 = pColorSelectorCodebook[(packet >> 28) & 2047];
                                    
            do 
            {
               pDst[0] = bc0;
               pDst[1] = bs0;
               pDst += 2;
               runLen--;
            } while(runLen);
                        
            pDst[0] = pColorCodebook[(packet >> 39) & 1023];
            pDst[1] = pColorSelectorCodebook[(packet >> 49) & 2047];
            pDst += 2;
         }
         else
         {
            uint runLen = (uint)((packet >> 60) & 7) + 1;
                                               
            const uint bc0 = pColorCodebook[(packet      ) & 15];
            const uint bs0 = pColorSelectorCodebook[(packet >>  4) & 31];
            
            do 
            {
               pDst[0] = bc0;
               pDst[1] = bs0;
               pDst += 2;
               runLen--;
            } while(runLen);
                                    
            pDst[0] = pColorCodebook[(packet >>  9) & 255];
            pDst[1] = pColorSelectorCodebook[(packet >> 17) & 511];
            pDst[2] = pColorCodebook[(packet >> 26) & 255];
            pDst[3] = pColorSelectorCodebook[(packet >> 34) & 511];
            pDst[4] = pColorCodebook[(packet >> 43) & 255];
            pDst[5] = pColorSelectorCodebook[(packet >> 51) & 511];
            
            pDst += 6;
         }            
      }
      
      
      if (pSrc == pSrcLast)
         break;
                  
      packet = *(const uint64*)pSrc;
      pSrc += 8;
      
#ifndef XBOX            
      if (cLittleEndianNative)
         EndianSwitchQWords(&packet, 1);
#endif      
                        
      if ((int64)packet >= 0)
      {
         pDst[0] = pColorCodebook[packet & 1023];
         pDst[1] = pColorSelectorCodebook[(packet >> 10) & 2047];

         pDst[2] = pColorCodebook[(packet >> 21) & 1023];
         pDst[3] = pColorSelectorCodebook[(packet >> 31) & 2047];

         pDst[4] = pColorCodebook[(packet >> 42) & 1023];
         pDst[5] = pColorSelectorCodebook[(packet >> 52) & 2047];

         pDst += 6;
      }
      else
      {
         if ((packet & 511) == 511)
         {
            uint runLen = (uint)((packet >> 9) & 511) + 4;
                                    
            const uint bc0 = pColorCodebook[(packet >> 18) & 1023];
            const uint bs0 = pColorSelectorCodebook[(packet >> 28) & 2047];
                                    
            do 
            {
               pDst[0] = bc0;
               pDst[1] = bs0;
               pDst += 2;
               runLen--;
            } while(runLen);
                        
            pDst[0] = pColorCodebook[(packet >> 39) & 1023];
            pDst[1] = pColorSelectorCodebook[(packet >> 49) & 2047];
            pDst += 2;
         }
         else
         {
            uint runLen = (uint)((packet >> 60) & 7) + 1;
                                               
            const uint bc0 = pColorCodebook[(packet      ) & 15];
            const uint bs0 = pColorSelectorCodebook[(packet >>  4) & 31];
            
            do 
            {
               pDst[0] = bc0;
               pDst[1] = bs0;
               pDst += 2;
               runLen--;
            } while(runLen);
                                    
            pDst[0] = pColorCodebook[(packet >>  9) & 255];
            pDst[1] = pColorSelectorCodebook[(packet >> 17) & 511];
            pDst[2] = pColorCodebook[(packet >> 26) & 255];
            pDst[3] = pColorSelectorCodebook[(packet >> 34) & 511];
            pDst[4] = pColorCodebook[(packet >> 43) & 255];
            pDst[5] = pColorSelectorCodebook[(packet >> 51) & 511];
            
            pDst += 6;
         }            
      }
      
      
      if (pSrc == pSrcLast)
         break;

      packet = *(const uint64*)pSrc;
      pSrc += 8;

#ifndef XBOX            
      if (cLittleEndianNative)
         EndianSwitchQWords(&packet, 1);
#endif      

      if ((int64)packet >= 0)
      {
         pDst[0] = pColorCodebook[packet & 1023];
         pDst[1] = pColorSelectorCodebook[(packet >> 10) & 2047];

         pDst[2] = pColorCodebook[(packet >> 21) & 1023];
         pDst[3] = pColorSelectorCodebook[(packet >> 31) & 2047];

         pDst[4] = pColorCodebook[(packet >> 42) & 1023];
         pDst[5] = pColorSelectorCodebook[(packet >> 52) & 2047];

         pDst += 6;
      }
      else
      {
         if ((packet & 511) == 511)
         {
            uint runLen = (uint)((packet >> 9) & 511) + 4;

            const uint bc0 = pColorCodebook[(packet >> 18) & 1023];
            const uint bs0 = pColorSelectorCodebook[(packet >> 28) & 2047];

            do 
            {
               pDst[0] = bc0;
               pDst[1] = bs0;
               pDst += 2;
               runLen--;
            } while(runLen);

            pDst[0] = pColorCodebook[(packet >> 39) & 1023];
            pDst[1] = pColorSelectorCodebook[(packet >> 49) & 2047];
            pDst += 2;
         }
         else
         {
            uint runLen = (uint)((packet >> 60) & 7) + 1;

            const uint bc0 = pColorCodebook[(packet      ) & 15];
            const uint bs0 = pColorSelectorCodebook[(packet >>  4) & 31];

            do 
            {
               pDst[0] = bc0;
               pDst[1] = bs0;
               pDst += 2;
               runLen--;
            } while(runLen);

            pDst[0] = pColorCodebook[(packet >>  9) & 255];
            pDst[1] = pColorSelectorCodebook[(packet >> 17) & 511];
            pDst[2] = pColorCodebook[(packet >> 26) & 255];
            pDst[3] = pColorSelectorCodebook[(packet >> 34) & 511];
            pDst[4] = pColorCodebook[(packet >> 43) & 255];
            pDst[5] = pColorSelectorCodebook[(packet >> 51) & 511];

            pDst += 6;
         }            
      }
      
   };

   {
      uint64 packet = *(const uint64*)pSrc;
      pSrc += 8;

#ifndef XBOX            
      if (cLittleEndianNative)
         EndianSwitchQWords(&packet, 1);
#endif      

      srcPrefetch = Utils::UpdatePrefetch(srcPrefetch, pSrc, pSrcLast, 2);
      dstWrite = UpdateWrite(dstWrite, pDst, pDstEnd, 3);

      if ((int64)packet < 0)
      {
         if ((packet & 511) == 511)
         {
            uint runLen = (uint)((packet >> 9) & 511) + 4;

            const uint c0 = (uint)((packet >> 18) & 1023);
            const uint s0 = (uint)((packet >> 28) & 2047);

            const uint c1 = (uint)((packet >> 39) & 1023);
            const uint s1 = (uint)((packet >> 49) & 2047);

            const uint bc0 = pColorCodebook[c0];
            const uint bs0 = pColorSelectorCodebook[s0];

            const uint bc1 = pColorCodebook[c1];
            const uint bs1 = pColorSelectorCodebook[s1];

            do 
            {
               pDst[0] = bc0;
               pDst[1] = bs0;
               pDst += 2;
               runLen--;
            } while(runLen);

            if (pDst < pDstEnd)
            {
               pDst[0] = bc1;
               pDst[1] = bs1;
               pDst += 2;
            }               
         }
         else
         {
            uint runLen = (uint)(((packet >> 60) & 7) + 1);

            const uint c0 = (uint)((packet      ) & 15);
            const uint s0 = (uint)((packet >>  4) & 31);

            const uint c1 = (uint)((packet >>  9) & 255);
            const uint s1 = (uint)((packet >> 17) & 511);

            const uint c2 = (uint)((packet >> 26) & 255);
            const uint s2 = (uint)((packet >> 34) & 511);

            const uint c3 = (uint)((packet >> 43) & 255);
            const uint s3 = (uint)((packet >> 51) & 511);

            const uint bc0 = pColorCodebook[c0];
            const uint bs0 = pColorSelectorCodebook[s0];

            const uint bc1 = pColorCodebook[c1];
            const uint bs1 = pColorSelectorCodebook[s1];

            const uint bc2 = pColorCodebook[c2];
            const uint bs2 = pColorSelectorCodebook[s2];

            const uint bc3 = pColorCodebook[c3];
            const uint bs3 = pColorSelectorCodebook[s3];

            do 
            {
               pDst[0] = bc0;
               pDst[1] = bs0;
               pDst += 2;
               runLen--;
            } while(runLen);

            if (pDst < pDstEnd)
            {
               pDst[0] = bc1;
               pDst[1] = bs1;
               pDst += 2;
            
               if (pDst < pDstEnd)
               {
                  pDst[0] = bc2;
                  pDst[1] = bs2;
                  pDst += 2;
               
                  if (pDst < pDstEnd)
                  {
                     pDst[0] = bc3;
                     pDst[1] = bs3;
                     pDst += 2;
                  }
               }
            }
         }            
      }
      else
      {
         const uint c0 = (uint)((packet      ) & 1023);
         const uint s0 = (uint)((packet >> 10) & 2047);

         const uint c1 = (uint)((packet >> 21) & 1023);
         const uint s1 = (uint)((packet >> 31) & 2047);

         const uint c2 = (uint)((packet >> 42) & 1023);
         const uint s2 = (uint)((packet >> 52) & 2047);

         const uint bc0 = pColorCodebook[c0];
         const uint bs0 = pColorSelectorCodebook[s0];

         const uint bc1 = pColorCodebook[c1];
         const uint bs1 = pColorSelectorCodebook[s1];

         const uint bc2 = pColorCodebook[c2];
         const uint bs2 = pColorSelectorCodebook[s2];

         pDst[0] = bc0;
         pDst[1] = bs0;
         pDst += 2;

         if (pDst < pDstEnd)
         {
            pDst[0] = bc1;
            pDst[1] = bs1;
            pDst += 2;
            
            if (pDst < pDstEnd)
            {
               pDst[0] = bc2;
               pDst[1] = bs2;
               pDst += 2;   
            }
         }
      }
   }
   
   BDEBUG_ASSERT(pDst == pDstEnd);
   
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
   
   DWORD* pColorCodebook                  = (DWORD*)(pCachedMem + header.mColorCodebookOfs);
   DWORD* pColorSelectorCodebook          = (DWORD*)(pCachedMem + header.mColorSelectorCodebookOfs);
   WORD* pAlphaCodebook                   = (WORD*)(pCachedMem + header.mAlphaCodebookOfs);
   WORD* pAlphaSelectorCodebook           = (WORD*)(pCachedMem + header.mAlphaSelectorCodebookOfs);   
                        
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
   
   const uchar* pSrc = pCachedMem + segment.mDataOfs;
   const uchar* pSrcEnd = pSrc + segment.mDataLen;
   Utils::BPrefetchState srcPrefetch = Utils::BeginPrefetch(pSrc, pSrcEnd, 2);

   DWORD* __restrict pDst = (DWORD*)(pWCDst + segment.mFirstBlock * cBytesPerBlock);
   void* __restrict pDstEnd = (uchar*)pDst + segment.mNumBlocks * cBytesPerBlock;
   BWriteState dstWrite = BeginWrite(pDst, pDstEnd, 3);
   
   WORD* pAlphaCodebook                   = (WORD*)(pCachedMem + header.mAlphaCodebookOfs);
   WORD* pAlphaSelectorCodebook           = (WORD*)(pCachedMem + header.mAlphaSelectorCodebookOfs);   
            
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
   eDDXDataFormat dataFormat,
   bool endianSwap)      
{
   bool result;
   uint bytesPerBlock;
   
   if (dataFormat == cDDXDataFormatDXT1Q)
   {
      bytesPerBlock = 8;
      result = decodeSegmentDXT1(pDst, header, pCachedMem, segment);
   }
   else if (dataFormat == cDDXDataFormatDXNQ)
   {
      bytesPerBlock = 16;
      result = decodeSegmentDXN(pDst, header, pCachedMem, segment);
   }
   else
   {
      bytesPerBlock = 16;
      result = decodeSegmentDXT5(pDst, header, pCachedMem, segment);
   }
   
   if (!result)
      return false;
           
   if (endianSwap)
      EndianSwitchWords((WORD*)(pDst + segment.mFirstBlock * bytesPerBlock), (segment.mNumBlocks * bytesPerBlock) >> 1);
      
   return true;      
}   

#ifdef XBOX
//------------------------------------------------------------------------------------------------------
// struct BDecodeWorkItem
//------------------------------------------------------------------------------------------------------
struct BDecodeWorkItem
{
   BDecodeWorkItem() { }
   
   BDecodeWorkItem(BYTE* __restrict pDst, const BDXTQHeader* pHeader, const BYTE* pCachedMem, const BDXTQHeader::BSegment* pSegment, eDDXDataFormat dataFormat, bool endianSwap) : 
      mpDst(pDst), mpHeader(pHeader), mpCachedMem(pCachedMem), mpSegment(pSegment), mDataFormat(dataFormat), mEndianSwap(endianSwap)
   {
   }         
   
   BYTE* __restrict              mpDst;
   const BDXTQHeader*            mpHeader;
   const BYTE*                   mpCachedMem;
   const BDXTQHeader::BSegment*  mpSegment;
   eDDXDataFormat                mDataFormat;
   bool                          mEndianSwap : 1;
};

static void decodeSegmentWorkFunc(void* privateData0, uint64 privateData1, uint workBucketIndex, bool lastWorkEntryInBucket)
{
   BScopedPIXNamedEvent pixEvent("DXTQPack::decodeSegmentWorkFunc");
   
   const BDecodeWorkItem* pWorkItem = static_cast<const BDecodeWorkItem*>(privateData0);
   BCountDownEvent* pCountDownEvent = reinterpret_cast<BCountDownEvent*>(privateData1);

   BDXTQPack::decodeSegment(
      pWorkItem->mpDst,
      *pWorkItem->mpHeader,
      pWorkItem->mpCachedMem,
      *pWorkItem->mpSegment,
      pWorkItem->mDataFormat,
      pWorkItem->mEndianSwap);

   pCountDownEvent->decrement();
}
#endif

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

#if 1
   BTimer timer;
   timer.start();
#endif  

#if TRACE_RECORDING
   static bool traceRecording;
   if (!endianSwap)
   {
      if (traceRecording) 
      {
         XTraceStartRecording("e:\\decompress.bin");
      }
   }      
#endif   

#ifdef XBOX
   PMCStart();
#endif

   bool threadedDecomp = false;

#if defined(XBOX) && THREADED_DECOMPRESSION
   threadedDecomp = gWorkDistibutor.getPermittingNewWork() && ((header.mNumBaseSegments + header.mNumMipSegments) > 1);
#endif   

   if (!threadedDecomp)
   {
      for (uint segmentIndex = 0; segmentIndex < header.mNumBaseSegments; segmentIndex++)
      {
         if (!decodeSegment(
            pDstArray->getPtr() + dstArrayOfs,
            header, pCachedMem,
            header.mBaseIndices[segmentIndex],
            format,
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
            format,
            endianSwap))
         {         
            return false;
         }         
      }
   }
   else
   {
#ifdef XBOX   
      gWorkDistibutor.flush();
      
      const uint cMaxWorkItems = BDXTQHeader::cNumSegments * 2;
      BDecodeWorkItem workItems[cMaxWorkItems];
      BDecodeWorkItem* pWorkItems = workItems;
      
      if (((uint)header.mNumBaseSegments + (uint)header.mNumMipSegments) > cMaxWorkItems)
         return false;
            
      for (uint segmentIndex = 0; segmentIndex < header.mNumBaseSegments; segmentIndex++)
      {
         pWorkItems->mpDst = pDstArray->getPtr() + dstArrayOfs;
         pWorkItems->mpHeader = &header;
         pWorkItems->mpCachedMem = pCachedMem;
         pWorkItems->mpSegment = &header.mBaseIndices[segmentIndex];
         pWorkItems->mDataFormat = format;
         pWorkItems->mEndianSwap = endianSwap;
         pWorkItems++;
      }

      for (uint segmentIndex = 0; segmentIndex < header.mNumMipSegments; segmentIndex++)
      {
         pWorkItems->mpDst = pDstArray->getPtr() + dstArrayOfs;
         pWorkItems->mpHeader = &header;
         pWorkItems->mpCachedMem = pCachedMem;
         pWorkItems->mpSegment = &header.mMipIndices[segmentIndex];
         pWorkItems->mDataFormat = format;
         pWorkItems->mEndianSwap = endianSwap;
         pWorkItems++;
      }         
      const uint numWorkItems = pWorkItems - workItems;
      
      BCountDownEvent countDownEvent;
      countDownEvent.set(numWorkItems);
      
      for (uint i = 1; i < numWorkItems; i++)
         gWorkDistibutor.queue(decodeSegmentWorkFunc, &workItems[i], (uint64)&countDownEvent, 1);
         
      gWorkDistibutor.flush();
      
      decodeSegmentWorkFunc((void*)&workItems[0], (uint64)&countDownEvent, 0, 0);
            
      gWorkDistibutor.flushAndWaitSingle(countDownEvent, INFINITE, 8, false, false);
#endif      
   }      
   
#ifdef XBOX
   PMCStop();
#endif   
   
#if TRACE_RECORDING
   if (traceRecording)
   {
      XTraceStopRecording();      
      
      DebugBreak();
      
      traceRecording = false;
   }
#endif      

#if 1  
double t = timer.getElapsedSeconds();
double cCPUClockRate = 3201239722;
if (!endianSwap)
   trace("endian %u, %u blocks, %2.6f, %2.6f cycles per block", endianSwap, totalBlocks, t, (t * cCPUClockRate) / totalBlocks);
#endif   
   
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


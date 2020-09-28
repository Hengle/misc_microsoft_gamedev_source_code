//==============================================================================
// File: DXTQPack.cpp
//
// Copyright (c) 2006 Ensemble Studios
//==============================================================================
#include "xcore.h"

#include "math\halfFloat.h"
#include "resource\ecfUtils.h"
#include "stream\cfileStream.h"

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
#include "DXTQUnpack.h"

#include "stream\dynamicStream.h"

#include "huffman.h"

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

      const int widthInBlocks  = Math::Max<uint>(1, mipWidth >> 2);
      const int heightInBlocks = Math::Max<uint>(1, mipHeight >> 2);

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
            const uint iy = Math::Min<uint>(image.getHeight() - 1, blockY * 4 + y);
            
            for (uint x = 0; x < 4; x++)
            {
               const uint ix = Math::Min<uint>(image.getWidth() - 1, blockX * 4 + x);
                              
               const BRGBAColor& c = image(ix, iy);
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
   // Colors
   BHuffmanCodes yCodes(127);
   BHuffmanCodes cCodes(63);

   BRGBAColor prevLc(0, 0, 0, 0);
   BRGBAColor prevHc(0, 0, 0, 0);   
   for (uint i = 0; i < colorCodebook.getSize(); i++)
   {
      const BDXTShrinker::BPackedColors& packedColors = colorCodebook[i];

      BRGBAColor lc, hc;
      BColorUtils::unpackColor((WORD)packedColors.getLow(), lc, false);
      BColorUtils::unpackColor((WORD)packedColors.getHigh(), hc, false);
      
      int deltaLcR = lc.r - prevLc.r;
      int deltaLcG = lc.g - prevLc.g;
      int deltaLcB = lc.b - prevLc.b;
      
      int deltaHcR = hc.r - prevHc.r;
      int deltaHcG = hc.g - prevHc.g;
      int deltaHcB = hc.b - prevHc.b;
      
      BDEBUG_ASSERT((deltaLcR >= -31) && (deltaLcR <= 31));
      BDEBUG_ASSERT((deltaLcG >= -63) && (deltaLcG <= 63));
      BDEBUG_ASSERT((deltaLcB >= -31) && (deltaLcB <= 31));
      
      BDEBUG_ASSERT((deltaHcR >= -31) && (deltaHcR <= 31));
      BDEBUG_ASSERT((deltaHcG >= -63) && (deltaHcG <= 63));
      BDEBUG_ASSERT((deltaHcB >= -31) && (deltaHcB <= 31));
      
      cCodes.incFreq(deltaLcR + 31);
      yCodes.incFreq(deltaLcG + 63);
      cCodes.incFreq(deltaLcB + 31);
      
      cCodes.incFreq(deltaHcR + 31);
      yCodes.incFreq(deltaHcG + 63);
      cCodes.incFreq(deltaHcB + 31);
                  
      prevLc = lc;
      prevHc = hc;
   }  
   
   yCodes.create();
   cCodes.create();
   
   BDynamicStream colorCodebookBytes;
   BOutputBitStreamAdapter outputBitStream(&colorCodebookBytes);

   outputBitStream.begin();   
   
   BHuffmanEnc yEncoder;
   BHuffmanEnc cEncoder;
   yEncoder.init(&outputBitStream, &yCodes);
   cEncoder.init(&outputBitStream, &cCodes);
   
   prevLc.set(0, 0, 0, 0);
   prevHc.set(0, 0, 0, 0);   
   for (uint i = 0; i < colorCodebook.getSize(); i++)
   {
      const BDXTShrinker::BPackedColors& packedColors = colorCodebook[i];

      BRGBAColor lc, hc;
      BColorUtils::unpackColor((WORD)packedColors.getLow(), lc, false);
      BColorUtils::unpackColor((WORD)packedColors.getHigh(), hc, false);
      
      int deltaLcR = lc.r - prevLc.r;
      int deltaLcG = lc.g - prevLc.g;
      int deltaLcB = lc.b - prevLc.b;

      int deltaHcR = hc.r - prevHc.r;
      int deltaHcG = hc.g - prevHc.g;
      int deltaHcB = hc.b - prevHc.b;

      cEncoder.encode(deltaLcR + 31);
      yEncoder.encode(deltaLcG + 63);
      cEncoder.encode(deltaLcB + 31);
      
      cEncoder.encode(deltaHcR + 31);
      yEncoder.encode(deltaHcG + 63);
      cEncoder.encode(deltaHcB + 31);

      prevLc = lc;
      prevHc = hc;
   }  
   
   outputBitStream.end();   
     
   header.mColorCodebookOfs = mCachedMemPool.alloc((uint)colorCodebookBytes.size(), 8);
   header.mColorCodebookSize = colorCodebook.getSize();
   header.mColorCodebookBytes = (uint)colorCodebookBytes.size();
   Utils::FastMemCpy(mCachedMemPool.getPtr(header.mColorCodebookOfs), colorCodebookBytes.getBuf().getPtr(), header.mColorCodebookBytes);
   
   // Selectors   
   
   BHuffmanCodes sCodes(256);

   DWORD prevD = 0;
   for (uint i = 0; i < colorSelectorCodebook.getSize(); i++)
   {
      BDXTUtils::BDXT1Cell cell;

      for (uint y = 0; y < 4; y++)
         for (uint x = 0; x < 4; x++)
            cell.setSelector(x, y, colorSelectorCodebook[i].mSelectors[y][x]);

      DWORD d = cell.getSelectorWord(0) | ((uint)cell.getSelectorWord(1) << 16);
            
      for (uint j = 0; j < 4; j++)
      {
         DWORD cur = (d >> (j * 8)) & 0xFF;
         DWORD prev = (prevD >> (j * 8)) & 0xFF;
         DWORD delta = cur ^ prev;
         sCodes.incFreq(delta);
      }

      prevD = d;
   }      

   sCodes.create();

   BDynamicStream selectorCodebookBytes;   
   outputBitStream.setStream(&selectorCodebookBytes);
   outputBitStream.begin();   

   BHuffmanEnc sEncoder;
   sEncoder.init(&outputBitStream, &sCodes);

   prevD = 0;
   for (uint i = 0; i < colorSelectorCodebook.getSize(); i++)
   {
      BDXTUtils::BDXT1Cell cell;

      for (uint y = 0; y < 4; y++)
         for (uint x = 0; x < 4; x++)
            cell.setSelector(x, y, colorSelectorCodebook[i].mSelectors[y][x]);

      DWORD d = cell.getSelectorWord(0) | ((uint)cell.getSelectorWord(1) << 16);
      
      for (uint j = 0; j < 4; j++)
      {
         DWORD cur = (d >> (j * 8)) & 0xFF;
         DWORD prev = (prevD >> (j * 8)) & 0xFF;
         DWORD delta = cur ^ prev;

         sEncoder.encode(delta);
      }

      prevD = d;
   }      

   outputBitStream.end();
   
   header.mColorSelectorCodebookOfs = mCachedMemPool.alloc((uint)selectorCodebookBytes.size(), 8);
   header.mColorSelectorCodebookSize = colorSelectorCodebook.getSize();
   header.mColorSelectorCodebookBytes = (uint)selectorCodebookBytes.size();
   Utils::FastMemCpy(mCachedMemPool.getPtr(header.mColorSelectorCodebookOfs), selectorCodebookBytes.getBuf().getPtr(), header.mColorSelectorCodebookBytes);
}

//------------------------------------------------------------------------------------------------------
// BDXTQPack::createAlphaCodebook
//------------------------------------------------------------------------------------------------------ 
void BDXTQPack::createAlphaCodebook(
   BDXTQHeader& header,
   const BDXTShrinker::BPackedColorsArray&      alphaCodebook,
   const BDXTShrinker::BBlockSelectorsArray&    alphaSelectorCodebook)
{
   // Alphas
   {
      BHuffmanCodes dCodes(511);
      
      int prevLc = 0;
      int prevHc = 0;
      for (uint i = 0; i < alphaCodebook.getSize(); i++)
      {
         const BDXTShrinker::BPackedColors& packedColors = alphaCodebook[i];

         int lc = packedColors.getLow();
         int hc = packedColors.getHigh();

         int deltaLc = lc - prevLc;
         int deltaHc = hc - prevHc;
         
         dCodes.incFreq(deltaLc + 255);
         dCodes.incFreq(deltaHc + 255);

         prevLc = lc;
         prevHc = hc;
      }  

      dCodes.create();

      BDynamicStream alphaCodebookBytes;
      BOutputBitStreamAdapter outputBitStream(&alphaCodebookBytes);

      outputBitStream.begin();   

      BHuffmanEnc dEncoder;
      dEncoder.init(&outputBitStream, &dCodes);

      prevLc = 0;
      prevHc = 0;
      for (uint i = 0; i < alphaCodebook.getSize(); i++)
      {
         const BDXTShrinker::BPackedColors& packedColors = alphaCodebook[i];

         int lc = packedColors.getLow();
         int hc = packedColors.getHigh();

         int deltaLc = lc - prevLc;
         int deltaHc = hc - prevHc;

         dEncoder.encode(deltaLc + 255);
         dEncoder.encode(deltaHc + 255);

         prevLc = lc;
         prevHc = hc;
      }  

      outputBitStream.end();   

      header.mAlphaCodebookOfs = mCachedMemPool.alloc((uint)alphaCodebookBytes.size(), 8);
      header.mAlphaCodebookSize = alphaCodebook.getSize();
      header.mAlphaCodebookBytes = (uint)alphaCodebookBytes.size();
      Utils::FastMemCpy(mCachedMemPool.getPtr(header.mAlphaCodebookOfs), alphaCodebookBytes.getBuf().getPtr(), header.mAlphaCodebookBytes);
   }      

   // Selectors   
   {
      BHuffmanCodes dCodes(256);
      BHuffmanEnc dEncoder;
      
      BDynamicStream alphaSelectorCodebookBytes;
      BOutputBitStreamAdapter outputBitStream(&alphaSelectorCodebookBytes);

      outputBitStream.begin();  
      
      //const uchar toLinearAlpha[cDXT5AlphaSelectorValues] = { 7, 0, 6, 5, 4, 3, 2, 1 };
      //const uchar fromLinearAlpha[cDXT5AlphaSelectorValues] = { 1, 7, 6, 5, 4, 3, 2, 0 }; 
         
      for (uint passIndex = 0; passIndex < 2; passIndex++)
      {
         uint64 prevBlock = 0;
                  
         for (uint i = 0; i < alphaSelectorCodebook.getSize(); i++)
         {
            BDXTUtils::BDXT5Cell cell;
            for (uint y = 0; y < 4; y++)
               for (uint x = 0; x < 4; x++)
                  cell.setSelector(x, y, alphaSelectorCodebook[i].mSelectors[y][x]);

            uint64 block = cell.getSelectorWord(0);
            block |= (((uint64)cell.getSelectorWord(1)) << 16U);
            block |= (((uint64)cell.getSelectorWord(2)) << 32U);
            
//            trace("%08X %08X", (uint)block, (uint)(block >> 32U));
            
            const uint64 blockDelta = block ^ prevBlock;
            
            for (uint j = 0; j < 6; j++)
            {
               uint q = (uint)((blockDelta >> (j * 8U)) & 0xFFU);
               if (passIndex)
                  dEncoder.encode(q);
               else
                  dCodes.incFreq(q);
            }               
            
            prevBlock = block;
         }
         
         if (!passIndex)
         {
            dCodes.create();
            dEncoder.init(&outputBitStream, &dCodes);
         }
      }
      
      outputBitStream.end();
      
      header.mAlphaSelectorCodebookOfs = mCachedMemPool.alloc((uint)alphaSelectorCodebookBytes.size(), 8);
      header.mAlphaSelectorCodebookSize = alphaSelectorCodebook.getSize();
      header.mAlphaSelectorCodebookBytes = (uint)alphaSelectorCodebookBytes.size();
      Utils::FastMemCpy(mCachedMemPool.getPtr(header.mAlphaSelectorCodebookOfs), alphaSelectorCodebookBytes.getBuf().getPtr(), header.mAlphaSelectorCodebookBytes);
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
   const UShortArray& alphaSelectorIndices,
   bool allowRuns)
{
   blockIndices.resize(0);
   
   if (!numBlocks)
      return;
      
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

      if ((allowRuns) && (isRun))
      {
         BDEBUG_ASSERT(blockIndex != lastBlock);
         curRunSize++;
      }
      else
      {
         if (curRunSize > 0)
         {
            totalEncodedBlocks += curRunSize;
            
            BDEBUG_ASSERT(curColorIndex != -1 || curAlpha0Index != -1);

            BBlockIndices block;
            block.mRunLength             = curRunSize;
            block.mColorIndex            = hasColorBlocks ? (ushort)curColorIndex : 0;
            block.mColorSelectorIndex    = hasColorBlocks ? (ushort)curColorSelectorIndex : 0;
            block.mAlpha0Index           = hasAlpha0Blocks ? (ushort)curAlpha0Index : 0;
            block.mAlpha0SelectorIndex   = hasAlpha0Blocks ? (ushort)curAlpha0SelectorIndex : 0;
            block.mAlpha1Index           = hasAlpha1Blocks ? (ushort)curAlpha1Index : 0; 
            block.mAlpha1SelectorIndex   = hasAlpha1Blocks ? (ushort)curAlpha1SelectorIndex : 0;
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

//------------------------------------------------------------------------------------------------------
// BDXTQPack::encodeIndices
//------------------------------------------------------------------------------------------------------ 
void BDXTQPack::encodeIndices(
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
   
   const uint totalTextureBlocks = header.mBaseBlocks + header.mMipBlocks;
   totalTextureBlocks;
         
   // ColorIndex
   // ColorSelectorIndex
   // Alpha0Index
   // Alpha0SelectorIndex
   // Alpha1Index
   // Alpha1SelectorIndex
   
   // 0 = repeat len=1
   // 1 = repeat len=omega coded
   
   const uint cNumRLESymbols = 2;
   
   BHuffmanCodes colorCodes;
   if (header.mColorCodebookSize)
      colorCodes.init(header.mColorCodebookSize + cNumRLESymbols);
         
   BHuffmanCodes colorSelectorCodes;
   if (header.mColorSelectorCodebookSize)
      colorSelectorCodes.init(header.mColorSelectorCodebookSize);
   
   BHuffmanCodes alphaCodes;
   if (header.mAlphaCodebookSize)
      alphaCodes.init(header.mAlphaCodebookSize + cNumRLESymbols);
   
   BHuffmanCodes alphaSelectorCodes;   
   if (header.mAlphaSelectorCodebookSize)
      alphaSelectorCodes.init(header.mAlphaSelectorCodebookSize);
   
   const uint totalBlocks = header.mBaseBlocks + header.mMipBlocks;
   totalBlocks;
   
   BDynamicStream outputByteStream;
   BOutputBitStreamAdapter outputBitStream(&outputByteStream);
   
   BHuffmanEnc colorEncoder;
   BHuffmanEnc colorSelectorEncoder;
   BHuffmanEnc alphaEncoder;
   BHuffmanEnc alphaSelectorEncoder;
   BUniversalCodec universalCodec;
            
   for (uint passIndex = 0; passIndex < 2; passIndex++)
   {
      for (uint textureMipIndex = 0; textureMipIndex < 2; textureMipIndex++)
      {
         uint firstBlock = 0, numBlocks = header.mBaseBlocks;
         if (textureMipIndex)
         {
            firstBlock = header.mBaseBlocks;
            numBlocks = header.mMipBlocks;
         }
         
         BBlockIndicesArray blockIndices;
         createBlockIndices(blockIndices, firstBlock, numBlocks, header, colorIndices, colorSelectorIndices, alphaIndices0, alphaIndices1, alphaSelectorIndices, true);

         if (passIndex)
            outputBitStream.begin();
                     
         for (uint blockIndex = 0; blockIndex < blockIndices.getSize(); blockIndex++)
         {
            const BBlockIndices& block = blockIndices[blockIndex];
            
            int colorSymbol = -1;
            int colorSelectorSymbol = -1;
            int alpha0Symbol = -1;
            int alpha0SelectorSymbol = -1;
            int alpha1Symbol = -1;
            int alpha1SelectorSymbol = -1;
            
            if (hasColorBlocks)
            {
               colorSymbol = block.mColorIndex + cNumRLESymbols;
               colorSelectorSymbol = block.mColorSelectorIndex;
            }
               
            if (hasAlpha0Blocks)
            {
               alpha0Symbol = block.mAlpha0Index + cNumRLESymbols;
               alpha0SelectorSymbol = block.mAlpha0SelectorIndex;
               
               if (hasAlpha1Blocks)
               {
                  alpha1Symbol = block.mAlpha1Index + cNumRLESymbols;
                  alpha1SelectorSymbol = block.mAlpha1SelectorIndex;
               }
            }
            
            if (passIndex)
            {
               if (hasColorBlocks)
               {
                  colorEncoder.encode(colorSymbol);
                  colorSelectorEncoder.encode(colorSelectorSymbol);
               }
               if (hasAlpha0Blocks)
               {
                  alphaEncoder.encode(alpha0Symbol);
                  alphaSelectorEncoder.encode(alpha0SelectorSymbol);
                  if (hasAlpha1Blocks)
                  {
                     alphaEncoder.encode(alpha1Symbol);
                     alphaSelectorEncoder.encode(alpha1SelectorSymbol);
                  }
               }
            }
            else
            {
               if (hasColorBlocks)
               {
                  colorCodes.incFreq(colorSymbol);
                  colorSelectorCodes.incFreq(colorSelectorSymbol);
               }
               if (hasAlpha0Blocks)
               {
                  alphaCodes.incFreq(alpha0Symbol);
                  alphaSelectorCodes.incFreq(alpha0SelectorSymbol);
                  if (hasAlpha1Blocks)
                  {
                     alphaCodes.incFreq(alpha1Symbol);
                     alphaSelectorCodes.incFreq(alpha1SelectorSymbol);
                  }
               }
            }
            
            if (block.mRunLength == 2)
            {
               if (passIndex)
               {
                  if (hasColorBlocks)
                     colorEncoder.encode(0);
                  else
                     alphaEncoder.encode(0);
               }
               else
               {
                  if (hasColorBlocks)
                     colorCodes.incFreq(0);
                  else
                     alphaCodes.incFreq(0);               
               }                  
            }
            else if (block.mRunLength >= 3)
            {
               if (passIndex)
               {
                  if (hasColorBlocks)
                     colorEncoder.encode(1);
                  else
                     alphaEncoder.encode(1);
                  
                  universalCodec.encodeOmega(&outputBitStream, block.mRunLength - 3);
               }
               else
               {
                  if (hasColorBlocks)
                     colorCodes.incFreq(1);
                  else
                     alphaCodes.incFreq(1);
               }               
            }
         } // blockIndex
         
         if (passIndex)
         {
            outputBitStream.end();
            
            if (numBlocks)
            {
               BDXTQHeader::BSegment* pSegment = textureMipIndex ? &header.mMipIndices : &header.mBaseIndices;

               pSegment->mNumBlocks = numBlocks;
            
               pSegment->mDataLen = outputByteStream.getBuf().getSize();
               pSegment->mDataOfs = mCachedMemPool.alloc((uint)outputByteStream.size(), 8);
               Utils::FastMemCpy(mCachedMemPool.getPtr(pSegment->mDataOfs), outputByteStream.getBuf().getPtr(), pSegment->mDataLen);
            }               

            outputByteStream.resize(0);
         }
         
      } // textureMipIndex         
            
      if (!passIndex)
      {
         outputBitStream.begin();
         
         if (header.mColorCodebookSize)
         {
            colorCodes.create();
            colorSelectorCodes.create();
            
            colorEncoder.init(&outputBitStream, &colorCodes);
            colorSelectorEncoder.init(&outputBitStream, &colorSelectorCodes);
         }
         
         if (header.mAlphaCodebookSize)
         {
            alphaCodes.create();
            alphaSelectorCodes.create();
            
            alphaEncoder.init(&outputBitStream, &alphaCodes);
            alphaSelectorEncoder.init(&outputBitStream, &alphaSelectorCodes);;
         }
         
         outputBitStream.end();
         
         header.mTables.mDataOfs = mCachedMemPool.alloc((uint)outputByteStream.size(), 8);
         header.mTables.mDataLen = (uint)outputByteStream.size();
         Utils::FastMemCpy(mCachedMemPool.getPtr(header.mTables.mDataOfs), outputByteStream.getBuf().getPtr(), (uint)outputByteStream.size());
         
         outputByteStream.resize(0);
      }
   } // passIndex
   
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
      
   if (srcTextureInfo.mResourceType == cDDXResourceTypeCubeMap)
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

   D3DFORMAT d3dFormat = BDXTQUnpack::getD3DDataFormatFromDDX(options.mDataFormat);
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

   if (options.mDataFormat == cDDXDataFormatDXT5YQ)
   {
      for (uint blockIndex = 0; blockIndex < blockPixels.getSize(); blockIndex++)
      {
         BDXTShrinker::BDXTBlockPixels& block = blockPixels[blockIndex];
         
         for (uint y = 0; y < 4; y++)
         {
            for (uint x = 0; x < 4; x++)
            {
               const BRGBAColor srcPixel(block.mPixels[y][x]);
               
               BRGBAColor16 yCoCg;
               BColorUtils::RGBToYCoCgR(srcPixel, yCoCg);
               
               BRGBAColor& dstPixel = block.mPixels[y][x];
               // FIXME: This is a clone of the cDDXDataFormatDXT5Y packing code in DDXCodec.cpp!
               dstPixel.a = (uchar)yCoCg.r;
               // rg [4/22/06] - FIXME: We're truncating alpha to 32 discrete levels, but the DXT5 packer further packs this to only 5 bits. 
               // The whole process introduces a level shift in alpha.
               dstPixel.r = (uchar)((srcPixel.a * 31 + 127) / 255);
               dstPixel.g = (uchar)Math::Clamp<int>((yCoCg.g / 2) + 125, 0, 255);
               dstPixel.b = (uchar)Math::Clamp<int>((yCoCg.b / 2) + 123, 0, 255);
            }
         }
      }
   }      

   BDXTFormat dxtFormat;
   switch (options.mDataFormat)
   {
      case cDDXDataFormatDXT1Q:  dxtFormat = cDXT1; break;
      case cDDXDataFormatDXT5Q:  dxtFormat = cDXT5; break;
      case cDDXDataFormatDXT5HQ: dxtFormat = cDXT5; break;
      case cDDXDataFormatDXT5YQ: dxtFormat = cDXT5; break;
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
      case cDDXDataFormatDXT5YQ: 
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
   if (dim == 128)
      divisor = 2;
   else if (dim == 64)
      divisor = 4;
   else if (dim <= 32)
      divisor = 8;

   uint colorCodebookSize           = (512/divisor);
   uint selectorCodebookSize        = (1024/divisor);
      
   uint alphaCodebookSize           = hasAlpha1Blocks ? (512/divisor) : (512/divisor);
   uint selectorAlphaCodebookSize   = hasAlpha1Blocks ? (1024/divisor) : (1024/divisor);
   
   if (options.mDXTQParams.mQualityFactor < BDDXDXTQPackParams::cDefaultQualityFactor)
   {
      int shift = (BDDXDXTQPackParams::cDefaultQualityFactor - options.mDXTQParams.mQualityFactor);
      if (shift > 0)
      {
         colorCodebookSize          >>= shift;
         selectorCodebookSize       >>= shift;
         alphaCodebookSize          >>= shift;
         selectorAlphaCodebookSize  >>= shift;
      }         
   }
   else if (options.mDXTQParams.mQualityFactor > BDDXDXTQPackParams::cDefaultQualityFactor)
   {
      int shift = (options.mDXTQParams.mQualityFactor - BDDXDXTQPackParams::cDefaultQualityFactor);
      if (shift > 0)
      {
         colorCodebookSize          <<= shift;
         selectorCodebookSize       <<= shift;
         alphaCodebookSize          <<= shift;
         selectorAlphaCodebookSize  <<= shift;
      }         
   }
   
   colorCodebookSize          = Math::Clamp<uint>(colorCodebookSize, 16, 8192);
   selectorCodebookSize       = Math::Clamp<uint>(selectorCodebookSize, 16, 8192);
   alphaCodebookSize          = Math::Clamp<uint>(alphaCodebookSize, 16, 8192);
   selectorAlphaCodebookSize  = Math::Clamp<uint>(selectorAlphaCodebookSize, 16, 8192);
   
   printf("DXTQ Codebook Sizes: %u %u %u %u\n", colorCodebookSize, selectorCodebookSize, alphaCodebookSize, selectorAlphaCodebookSize);
         
   if (options.mDataFormat == cDDXDataFormatDXT5YQ)
   {
      colorCodebookSize = 128;//Math::Max<uint>(128, colorCodebookSize / 2);
      selectorCodebookSize = 256;//Math::Max<uint>(256, selectorCodebookSize / 2);
   }
                  
   BDXTShrinker::BParams dxtShrinkerParams;
   dxtShrinkerParams.mMaxQuantColors = colorCodebookSize;
   dxtShrinkerParams.mMaxAlphaColors = alphaCodebookSize;
   dxtShrinkerParams.mColorSelectorCodebookSize = selectorCodebookSize;
   dxtShrinkerParams.mAlphaSelectorCodebookSize = selectorAlphaCodebookSize;
   dxtShrinkerParams.mFavorLargerAlpha = (options.mDataFormat == cDDXDataFormatDXT5HQ);
   dxtShrinkerParams.mPerceptual = false;
   if ((options.mDataFormat != cDDXDataFormatDXNQ) && (srcTextureInfo.mResourceType != cDDXResourceTypeNormalMap) && (options.mDataFormat != cDDXDataFormatDXT5YQ))  
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
   
   encodeIndices(
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
      if (!BDXTQUnpack::unpackDXTQToRawDXT(packedAlphaTextureInfo, packedAlpha.getPtr(), packedAlpha.getSize(), unpackedTextureInfo, unpackedAlphaDXT))
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

//------------------------------------------------------------------------------------------------------
// BDXTQPack::BDXTQPack
//------------------------------------------------------------------------------------------------------
BDXTQPack::BDXTQPack() :
   mCachedMemPool(32*1024*1024U)
{
}


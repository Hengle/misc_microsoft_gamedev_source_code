//============================================================================
//
// File: DXTQUnpack.cpp
//
// Copyright (c) 2005-2006, Ensemble Studios
//
//============================================================================
#include "xcore.h"
#include "DXTQUnpack.h"
#include "resource\ecfUtils.h"
#include "threading\win32Event.h"
#include "Timer.h"
#include "DDXUtils.h"
#include "..\xsystem\timelineprofilersample.h"
#include "stream\byteStream.h"
#include "colorUtils.h"

#include <xgraphics.h>

#pragma warning(disable:4244) // warning C4244: 'argument' : conversion from 'uint64' to 'uint', possible loss of data

#pragma optimize( "t", on )

#define TIMING_STATS 0

//------------------------------------------------------------------------------------------------------
// BeginWrite
//------------------------------------------------------------------------------------------------------
typedef void* BWriteState;

inline BWriteState BeginWrite(void* p, void* pEnd, uint cacheLinesToWriteAhead)
{
   p;
   pEnd;
   cacheLinesToWriteAhead;
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

//------------------------------------------------------------------------------------------------------
// UpdateWrite
//------------------------------------------------------------------------------------------------------
inline BWriteState UpdateWrite(BWriteState prevP, void* p, void* pEnd, uint cacheLinesToWriteAhead)
{
   prevP;
   p;
   pEnd;
   cacheLinesToWriteAhead;
   
#ifdef XBOX
   if (((uint)prevP ^ (uint)p) & 0xFFFFFF80)
   {
      // rg [12/31/06] - This forces all written cachelines out of the cache, which is probably not what we want.
      //__dcbf(0, prevP);
      
      //BWriteState state = Utils::AlignUp(prevP, 128);
      BWriteState newState = Utils::AlignUp(p, 128);
      
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
// BDXTQUnpack::decodeSegment
//------------------------------------------------------------------------------------------------------
bool BDXTQUnpack::decodeSegment(
   BYTE* __restrict pDst,
   const BDXTQHeader& header,
   const BYTE* pCachedMem,
   const BDXTQHeader::BSegment& segment,
   const BDXTQCodebooks& codebooks,
   BDXTQHuffmanDecoders& decoders,
   eDDXDataFormat dataFormat,
   bool writeCombinedDst)      
{
   writeCombinedDst;
   header;
   
   if (!segment.mNumBlocks)
      return true;
   
   const uint bytesPerBlock = (dataFormat == cDDXDataFormatDXT1Q) ? 8 : 16;
   bytesPerBlock;

   BByteStream byteStream(pCachedMem + segment.mDataOfs, segment.mDataLen);
   BInputBitStreamAdapter bitStream(&byteStream);
   
   decoders.mColorDecoder.setBitStream(&bitStream);
   decoders.mColorSelectorDecoder.setBitStream(&bitStream);
   decoders.mAlphaDecoder.setBitStream(&bitStream);
   decoders.mAlphaSelectorDecoder.setBitStream(&bitStream);

   //const bool hasColorBlocks = (header.mColorCodebookSize > 0);
   //const bool hasAlpha0Blocks = (dataFormat == cDDXDataFormatDXNQ) || (dataFormat == cDDXDataFormatDXT5Q);
   //const bool hasAlpha1Blocks = (dataFormat == cDDXDataFormatDXNQ);
   
   BUniversalCodec univeralCodec;
                    
   if (dataFormat == cDDXDataFormatDXT1Q)
   {
      DWORD* pDWORDDst = (DWORD*)pDst;
      
      uint colorSymbol = 0, colorSelectorSymbol = 0;
      
      uint numBlocksLeft = segment.mNumBlocks;
      do
      {
         uint sym;
         if (!decoders.mColorDecoder.getNextSymbol(sym)) return false;

         uint runLen = 1;
         if (sym <= 1)
         {       
            if (sym)
            {
               if (!univeralCodec.decodeOmega(&bitStream, runLen)) return false;
               runLen += 2;
            }
         }
         else
         {
            colorSymbol = sym - 2;
            if (!decoders.mColorSelectorDecoder.getNextSymbol(colorSelectorSymbol)) return false;
         }
        
         if (runLen > numBlocksLeft)
            return false;
         numBlocksLeft -= runLen;
          
         const uint color = codebooks.mColorCodebook[colorSymbol];
         const uint colorSelector = codebooks.mColorSelectorCodebook[colorSelectorSymbol];
         
         do 
         {
            pDWORDDst[0] = color; _WriteBarrier();
            pDWORDDst[1] = colorSelector; _WriteBarrier();
            
            pDWORDDst += 2;
            
            runLen--;
         }  while (runLen > 0);          
      } while (numBlocksLeft);
   }
   else if (dataFormat != cDDXDataFormatDXNQ)
   {
      DWORD* pDWORDDst = (DWORD*)pDst;
      
      uint colorSymbol = 0, colorSelectorSymbol = 0;
      uint alphaSymbol = 0, alphaSelectorSymbol = 0;

      uint numBlocksLeft = segment.mNumBlocks;
      do
      {
         uint sym;
         if (!decoders.mColorDecoder.getNextSymbol(sym)) return false;

         uint runLen = 1;
         if (sym <= 1)
         {       
            if (sym)
            {
               if (!univeralCodec.decodeOmega(&bitStream, runLen)) return false;
               runLen += 2;
            }
         }
         else
         {
            colorSymbol = sym - 2;
            if (!decoders.mColorSelectorDecoder.getNextSymbol(colorSelectorSymbol)) return false;
            
            if (!decoders.mAlphaDecoder.getNextSymbol(alphaSymbol)) return false;
            if (alphaSymbol < 2) return false;
            alphaSymbol -= 2;
            
            if (!decoders.mAlphaSelectorDecoder.getNextSymbol(alphaSelectorSymbol)) return false;
         }

         if (runLen > numBlocksLeft)
            return false;
         numBlocksLeft -= runLen;

         const uint color = codebooks.mColorCodebook[colorSymbol];
         const uint colorSelector = codebooks.mColorSelectorCodebook[colorSelectorSymbol];
         const uint alpha = codebooks.mAlphaCodebook[alphaSymbol];
         const uint64 alphaSelector = codebooks.mAlphaSelectorCodebook[alphaSelectorSymbol];
         do 
         {
            ((WORD*)(pDWORDDst))[0] = (WORD)alpha; _WriteBarrier();
            ((WORD*)(pDWORDDst))[1] = (WORD)alphaSelector; _WriteBarrier();
            ((WORD*)(pDWORDDst))[2] = (WORD)(alphaSelector >> 16U); _WriteBarrier();
            ((WORD*)(pDWORDDst))[3] = (WORD)(alphaSelector >> 32U); _WriteBarrier();
            
            pDWORDDst[2] = color; _WriteBarrier();
            pDWORDDst[3] = colorSelector; _WriteBarrier();
            
            pDWORDDst += 4;

            runLen--;
         }  while (runLen > 0);          
      } while (numBlocksLeft);
   }
   else
   {
      WORD* pWORDDst = (WORD*)pDst;
      
      uint alpha0Symbol = 0, alpha0SelectorSymbol = 0;
      uint alpha1Symbol = 0, alpha1SelectorSymbol = 0;

      uint numBlocksLeft = segment.mNumBlocks;
      uint count = 0;
      do
      {
         uint sym;
         if (!decoders.mAlphaDecoder.getNextSymbol(sym)) return false;

         uint runLen = 1;
         if (sym <= 1)
         {       
            if (sym)
            {
               if (!univeralCodec.decodeOmega(&bitStream, runLen)) return false;
               runLen += 2;
            }
         }
         else
         {
            alpha0Symbol = sym - 2;
            if (!decoders.mAlphaSelectorDecoder.getNextSymbol(alpha0SelectorSymbol)) return false;

            if (!decoders.mAlphaDecoder.getNextSymbol(alpha1Symbol)) return false;
            if (alpha1Symbol < 2)
               return false;
            alpha1Symbol -= 2;
            
            if (!decoders.mAlphaSelectorDecoder.getNextSymbol(alpha1SelectorSymbol)) return false;
         }

         if (runLen > numBlocksLeft)
            return false;
         numBlocksLeft -= runLen;

         const uint alpha0 = codebooks.mAlphaCodebook[alpha0Symbol];
         const uint alpha1 = codebooks.mAlphaCodebook[alpha1Symbol];
         const uint64 alpha0Selector = codebooks.mAlphaSelectorCodebook[alpha0SelectorSymbol];
         const uint64 alpha1Selector = codebooks.mAlphaSelectorCodebook[alpha1SelectorSymbol];         
         
         do 
         {
            pWORDDst[0] = alpha0; _WriteBarrier();
            pWORDDst[1] = (WORD)alpha0Selector; _WriteBarrier();
            pWORDDst[2] = (WORD)(alpha0Selector >> 16U); _WriteBarrier();
            pWORDDst[3] = (WORD)(alpha0Selector >> 32U); _WriteBarrier();
            
            pWORDDst[4] = alpha1; _WriteBarrier();
            pWORDDst[5] = (WORD)alpha1Selector; _WriteBarrier();
            pWORDDst[6] = (WORD)(alpha1Selector >> 16U); _WriteBarrier();
            pWORDDst[7] = (WORD)(alpha1Selector >> 32U); _WriteBarrier();

            pWORDDst += 8;

            runLen--;
         }  while (runLen > 0);          
         
         count++;
         
      } while (numBlocksLeft);
   }
         
   return true;      
}   

//------------------------------------------------------------------------------------------------------
// BDXTQUnpack::BDXTQCodebooks::init
//------------------------------------------------------------------------------------------------------
bool BDXTQUnpack::BDXTQCodebooks::init(
   const BDXTQHeader& header,
   const BYTE* pCachedMem, 
   bool bigEndian)
{
   if (header.mColorCodebookSize)
   {
      mColorCodebook.resize(header.mColorCodebookSize);
            
      BByteStream colorCodebookByteStream(pCachedMem + header.mColorCodebookOfs, header.mColorCodebookBytes);
      BInputBitStreamAdapter colorCodebookBitStream(&colorCodebookByteStream);
      
      BHuffmanDec yDecoder;
      BHuffmanDec cDecoder;
      
      if (!yDecoder.init(&colorCodebookBitStream))
         return false;
      
      if (!cDecoder.init(&colorCodebookBitStream))
         return false;
      
      BRGBAColor prevLc(0, 0, 0, 0);
      BRGBAColor prevHc(0, 0, 0, 0);
      
      for (uint i = 0; i < mColorCodebook.getSize(); i++)
      {
         uint lcR, lcG, lcB;
         uint hcR, hcG, hcB;
         if (!cDecoder.getNextSymbol(lcR)) return false;
         if (!yDecoder.getNextSymbol(lcG)) return false;
         if (!cDecoder.getNextSymbol(lcB)) return false;
         
         if (!cDecoder.getNextSymbol(hcR)) return false;
         if (!yDecoder.getNextSymbol(hcG)) return false;
         if (!cDecoder.getNextSymbol(hcB)) return false;
         
         int deltaLcR = lcR - 31;
         int deltaLcG = lcG - 63;
         int deltaLcB = lcB - 31;

         int deltaHcR = hcR - 31;
         int deltaHcG = hcG - 63;
         int deltaHcB = hcB - 31;

         BRGBAColor curLc(prevLc);
         BRGBAColor curHc(prevHc);
         curLc.r += deltaLcR;
         curLc.g += deltaLcG;
         curLc.b += deltaLcB;
         
         curHc.r += deltaHcR;
         curHc.g += deltaHcG;
         curHc.b += deltaHcB;
         
         uint lc = BColorUtils::packColor(curLc, false);
         uint hc = BColorUtils::packColor(curHc, false);
         
         if (cBigEndianNative != bigEndian)
         {
            lc = (lc >> 8) | ((lc & 0xFF) << 8);
            hc = (hc >> 8) | ((hc & 0xFF) << 8);
         }
         
         ((WORD*)(&mColorCodebook[i]))[0] = (WORD)hc;
         ((WORD*)(&mColorCodebook[i]))[1] = (WORD)lc;
                                              
         prevLc = curLc;
         prevHc = curHc;
      }
   }
   else
      mColorCodebook.clear();
      
   if (header.mColorSelectorCodebookSize)         
   {
      mColorSelectorCodebook.resize(header.mColorSelectorCodebookSize);

      BByteStream colorSelectorCodebookByteStream(pCachedMem + header.mColorSelectorCodebookOfs, header.mColorSelectorCodebookBytes);
      BInputBitStreamAdapter colorSelectorCodebookBitStream(&colorSelectorCodebookByteStream);

      BHuffmanDec sDecoder;

      if (!sDecoder.init(&colorSelectorCodebookBitStream))
         return false;

      uint prev0 = 0, prev1 = 0, prev2 = 0, prev3 = 0;
      for (uint i = 0; i < mColorSelectorCodebook.getSize(); i++)
      {     
         uint delta0, delta1, delta2, delta3;
         if (!sDecoder.getNextSymbol(delta0)) return false;
         if (!sDecoder.getNextSymbol(delta1)) return false;
         if (!sDecoder.getNextSymbol(delta2)) return false;
         if (!sDecoder.getNextSymbol(delta3)) return false;
                     
         if (cBigEndianNative != bigEndian) std::swap(delta0, delta1);
         if (cBigEndianNative != bigEndian) std::swap(delta2, delta3);
         
         delta0 ^= prev0;
         delta1 ^= prev1;
         delta2 ^= prev2;
         delta3 ^= prev3;
                                             
         ((WORD*)(&mColorSelectorCodebook[i]))[0] = delta0 | (delta1 << 8U);
         ((WORD*)(&mColorSelectorCodebook[i]))[1] = delta2 | (delta3 << 8U);
                           
         prev0 = delta0;
         prev1 = delta1;
         prev2 = delta2;
         prev3 = delta3;
      }
   }
   else
      mColorSelectorCodebook.clear();
      
   if (header.mAlphaCodebookSize)
   {
      mAlphaCodebook.resize(header.mAlphaCodebookSize);
      
      BByteStream alphaCodebookByteStream(pCachedMem + header.mAlphaCodebookOfs, header.mAlphaCodebookBytes);
      BInputBitStreamAdapter alphaCodebookBitStream(&alphaCodebookByteStream);

      BHuffmanDec dDecoder;

      if (!dDecoder.init(&alphaCodebookBitStream))
         return false;

      int prevL = 0, prevH = 0;
      for (uint i = 0; i < mAlphaCodebook.getSize(); i++)         
      {
         uint l, h;
         if (!dDecoder.getNextSymbol(l)) return false;
         if (!dDecoder.getNextSymbol(h)) return false;
         
         int curL = prevL + (l - 255);
         int curH = prevH + (h - 255);
         
         if (cBigEndianNative != bigEndian)
            mAlphaCodebook[i] = (curH << 8U) | curL;
         else
            mAlphaCodebook[i] = (curL << 8U) | curH;
                           
         prevL = curL;
         prevH = curH;
      }
   }
   else
      mAlphaCodebook.clear();     
      
   if (header.mAlphaSelectorCodebookSize)      
   {
      mAlphaSelectorCodebook.resize(header.mAlphaSelectorCodebookSize);

      BByteStream alphaSelectorCodebookByteStream(pCachedMem + header.mAlphaSelectorCodebookOfs, header.mAlphaSelectorCodebookBytes);
      BInputBitStreamAdapter alphaSelectorCodebookBitStream(&alphaSelectorCodebookByteStream);

      BHuffmanDec dDecoder;

      if (!dDecoder.init(&alphaSelectorCodebookBitStream))
         return false;
         
      uint64 prevBlock = 0;         
      for (uint i = 0; i < mAlphaSelectorCodebook.getSize(); i++)
      {
         uint64 curBlock = prevBlock;
         
         for (uint j = 0; j < 6; j++)
         {
            uint q;
            if (!dDecoder.getNextSymbol(q)) return false;
             
            curBlock ^= (((uint64)q) << (j * 8U));
         }      
                           
         mAlphaSelectorCodebook[i] = curBlock;
         
         prevBlock = curBlock;      
      }         
      
      if (cBigEndianNative != bigEndian)
         EndianSwitchWords((WORD*)mAlphaSelectorCodebook.getPtr(), mAlphaSelectorCodebook.getSize() * 4U);
   }
   else
      mAlphaSelectorCodebook.clear();

   return true;      
}   

//------------------------------------------------------------------------------------------------------
// BDXTQUnpack::BDXTQHuffmanDecoders::init
//------------------------------------------------------------------------------------------------------
bool BDXTQUnpack::BDXTQHuffmanDecoders::init(const BDXTQHeader& header, const BYTE* pCachedMem)
{
   BByteStream byteStream(pCachedMem + header.mTables.mDataOfs, header.mTables.mDataLen);
   BInputBitStreamAdapter bitStream(&byteStream);
   
   if (header.mColorCodebookSize)
   {
      if (!mColorDecoder.init(&bitStream))
         return false;
      if (!mColorSelectorDecoder.init(&bitStream))
         return false;
   }
   
   if (header.mAlphaCodebookSize)
   {
      if (!mAlphaDecoder.init(&bitStream))
         return false;
      if (!mAlphaSelectorDecoder.init(&bitStream))
         return false;
   }
   
   return true;
}

//------------------------------------------------------------------------------------------------------
// BDXTQUnpack::unpackDXTQToRawDXT
//------------------------------------------------------------------------------------------------------
bool BDXTQUnpack::unpackDXTQToRawDXT(
   const BDDXTextureInfo& srcTextureInfo,
   const BYTE* pSrcData, uint srcDataSize,
   BDDXTextureInfo& dstTextureInfo, BByteArray& dstData)
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
   
   uint totalBlocks = header.mBaseBlocks + header.mMipBlocks;
         
   dstTextureInfo = srcTextureInfo;
      
   switch (srcTextureInfo.mDataFormat)
   {
      case cDDXDataFormatDXT1Q:  dstTextureInfo.mDataFormat = cDDXDataFormatDXT1;  break;
      case cDDXDataFormatDXT5Q:  dstTextureInfo.mDataFormat = cDDXDataFormatDXT5;  dstTextureInfo.mHasAlpha = true; break;
      case cDDXDataFormatDXT5HQ: dstTextureInfo.mDataFormat = cDDXDataFormatDXT5H; dstTextureInfo.mHasAlpha = true; break;
      case cDDXDataFormatDXT5YQ: dstTextureInfo.mDataFormat = cDDXDataFormatDXT5Y; dstTextureInfo.mHasAlpha = true; break;
      case cDDXDataFormatDXNQ:   dstTextureInfo.mDataFormat = cDDXDataFormatDXN;   break;
      default:
         return false;
   }
   
   dstTextureInfo.mOrigDataFormat = format;
   dstTextureInfo.mPackType = cDDXTDPTMipsRaw;
               
   BByteArray tempData;
   
   BByteArray* pDstArray = &tempData;
   
   tempData.resize(totalBlocks * bytesPerBlock);
      
#if TIMING_STATS
   BTimer timer;
   timer.start();
#endif

   BDXTQCodebooks codebooks;
   if (!codebooks.init(header, pCachedMem, false))
      return false;
      
   BDXTQHuffmanDecoders decoders;
   if (!decoders.init(header, pCachedMem))
      return false;
  
   BYTE* pDst = pDstArray->getPtr();
   
   if (!decodeSegment(
      pDst,
      header, pCachedMem,
      header.mBaseIndices, codebooks, decoders,
      format,
      false))
   {
      return false;
   }    
   pDst += bytesPerBlock * header.mBaseBlocks;
   
   if (!decodeSegment(
      pDst,
      header, pCachedMem,
      header.mMipIndices, codebooks, decoders,
      format,
      false))
   {
      return false;
   }    
   pDst += bytesPerBlock * header.mMipBlocks;
   
#if TIMING_STATS
double t = timer.getElapsedSeconds();
double cCPUClockRate = 3201239722;
double cyclesPerBlock = (t * cCPUClockRate) / totalBlocks;
static double bestCyclesPerBlock = Math::fNearlyInfinite;
if (cyclesPerBlock < bestCyclesPerBlock)
   bestCyclesPerBlock = cyclesPerBlock;
   trace("bigEndian %u, %u blocks, %2.6f, %2.6f cycles per block, %2.6f best cycles per block", bigEndian, totalBlocks, t, cyclesPerBlock, bestCyclesPerBlock);
#endif   
   
   // Untile texture
                  
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

      if ((mipLevel > 0) && (d3dTexMipSize))
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
// BDXTQUnpack::getDXTQInfo
//------------------------------------------------------------------------------------------------------
bool BDXTQUnpack::getDXTQInfo(
   const BDDXTextureInfo& srcTextureInfo,
   const BYTE* pSrcData, uint srcDataSize,
   BDDXTextureInfo& dstTextureInfo, 
   IDirect3DTexture9* pDstHeader,
   uint& numBaseBlocks, uint& numMipChainBlocks, uint& bytesPerBlock)
{
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
      
   const D3DFORMAT d3dFormat = getD3DDataFormatFromDDX(srcTextureInfo.mDataFormat);
   if (d3dFormat == D3DFMT_UNKNOWN)
      return false;
   if (d3dTexDesc.Format != d3dFormat)    
      return false;

   bytesPerBlock = getDDXDataFormatDXTBlockSize(srcTextureInfo.mDataFormat);
   //const uint d3dTexBaseSize = header.mBaseBlocks * bytesPerBlock;
   //const uint d3dTexMipSize = header.mMipBlocks * bytesPerBlock;

   dstTextureInfo = srcTextureInfo;

   switch (srcTextureInfo.mDataFormat)
   {
      case cDDXDataFormatDXT1Q:  dstTextureInfo.mDataFormat = cDDXDataFormatDXT1;  break;
      case cDDXDataFormatDXT5Q:  dstTextureInfo.mDataFormat = cDDXDataFormatDXT5;  dstTextureInfo.mHasAlpha = true; break;
      case cDDXDataFormatDXT5HQ: dstTextureInfo.mDataFormat = cDDXDataFormatDXT5H; dstTextureInfo.mHasAlpha = true; break;
      case cDDXDataFormatDXT5YQ: dstTextureInfo.mDataFormat = cDDXDataFormatDXT5Y; dstTextureInfo.mHasAlpha = true; break;
      case cDDXDataFormatDXNQ:   dstTextureInfo.mDataFormat = cDDXDataFormatDXN;   break;
      default:
         return false;
   }

   dstTextureInfo.mOrigDataFormat = srcTextureInfo.mDataFormat;
   dstTextureInfo.mPackType = cDDXTDPTMipsRaw;

   dstTextureInfo.mPlatform = cDDXPlatformXbox;

   if (pDstHeader)
      memcpy(pDstHeader, &d3dTex, sizeof(IDirect3DTexture9));

   numBaseBlocks = header.mBaseBlocks;
   numMipChainBlocks = header.mMipBlocks;   
   
   return true;
}      

//------------------------------------------------------------------------------------------------------
// BDXTQUnpack::unpackDXTQTo360DXT
//------------------------------------------------------------------------------------------------------
bool BDXTQUnpack::unpackDXTQToTiledDXT(
   const eDDXDataFormat format,
   const BYTE* pSrcData, uint srcDataSize,
   BYTE* pDstTexture, uint dstTextureSize,
   bool decodeBase, bool decodeMipChain)
{
   if ((!decodeBase) && (!decodeMipChain))
      return false;
   
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
      
   const uint bytesPerBlock = getDDXDataFormatDXTBlockSize(format);
                           
#if TIMING_STATS
   BTimer timer;
   timer.start();
#endif
   
   const uint totalBlocksToUnpack = (decodeBase ? header.mBaseBlocks : 0) + (decodeMipChain ? header.mMipBlocks : 0);
   if (dstTextureSize < (totalBlocksToUnpack * bytesPerBlock))
      return false;
      
   BDEBUG_ASSERT(pDstTexture);

   BDXTQCodebooks codebooks;
   if (!codebooks.init(header, pCachedMem, true))
      return false;
      
   BDXTQHuffmanDecoders decoders;
   if (!decoders.init(header, pCachedMem))
      return false;
  
   BYTE* pDst = pDstTexture;
   
   if (decodeBase)
   {
      if (!decodeSegment(
         pDst,
         header, pCachedMem,
         header.mBaseIndices, codebooks, decoders,
         format,
         false))
      {
         return false;
      }    
      pDst += bytesPerBlock * header.mBaseBlocks;
   }      
   
   if (decodeMipChain)
   {
      if (!decodeSegment(
         pDst,
         header, pCachedMem,
         header.mMipIndices, codebooks, decoders,
         format,
         false))
      {
         return false;
      }    
      pDst += bytesPerBlock * header.mMipBlocks;
   }
   
#if TIMING_STATS
   double t = timer.getElapsedSeconds();
   double cCPUClockRate = 3201239722;
   double cyclesPerBlock = (t * cCPUClockRate) / totalBlocksToUnpack;
   static double bestCyclesPerBlock = Math::fNearlyInfinite;
   if (cyclesPerBlock < bestCyclesPerBlock)
      bestCyclesPerBlock = cyclesPerBlock;
      trace("bigEndian %u, %u blocks, %2.6f, %2.6f cycles per block, %2.6f best cycles per block", bigEndian, totalBlocks, t, cyclesPerBlock, bestCyclesPerBlock);
#endif   
   
   return true;
}

bool BDXTQUnpack::unpackDXTQBasePrep(
   const eDDXDataFormat format, 
   const BYTE* pSrcData, uint srcDataSize,
   void*& pDstBaseData, uint& dstBaseDataSize, 
   BMemoryHeap* pHeap)
{
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

   const uint bytesPerBlock = getDDXDataFormatDXTBlockSize(format);
   bytesPerBlock;
   
   dstBaseDataSize = sizeof(header) + sizeof(DWORD) + (cachedMemSize - header.mMipIndices.mDataLen);
   
   pDstBaseData = pHeap->New(dstBaseDataSize);
   if (!pDstBaseData)
      return false;
   
   memcpy(pDstBaseData, &header, sizeof(header));
   
   const DWORD formatDWORD = format;
   memcpy(reinterpret_cast<BYTE*>(pDstBaseData) + sizeof(header), &formatDWORD, sizeof(DWORD));
   
   memcpy(reinterpret_cast<BYTE*>(pDstBaseData) + sizeof(header) + sizeof(DWORD), pCachedMem, cachedMemSize - header.mMipIndices.mDataLen);
   
   return true;
}   

bool BDXTQUnpack::unpackDXTQBase(
   const void* pBaseData, uint baseDataSize,
   BYTE* pDstTexture, uint dstTextureSize)
{
   dstTextureSize;
   
   BDEBUG_ASSERT((pBaseData != NULL) && (baseDataSize > sizeof(BDXTQHeader)));
   const BDXTQHeader& header = *reinterpret_cast<const BDXTQHeader*>(pBaseData);
   const eDDXDataFormat format = static_cast<eDDXDataFormat>(*reinterpret_cast<const DWORD*>(reinterpret_cast<const BYTE*>(pBaseData) + sizeof(header)));
   const BYTE* pCachedMem = reinterpret_cast<const BYTE*>(pBaseData) + sizeof(header) + sizeof(DWORD);

   BDXTQCodebooks codebooks;
   if (!codebooks.init(header, pCachedMem, true))
      return false;

   BDXTQHuffmanDecoders decoders;
   if (!decoders.init(header, pCachedMem))
      return false;
   
   BDEBUG_ASSERT(dstTextureSize >= (getDDXDataFormatDXTBlockSize(format) * header.mBaseBlocks));

   if (!decodeSegment(
      pDstTexture,
      header, pCachedMem,
      header.mBaseIndices, codebooks, decoders,
      format,
      false))
   {
      return false;
   } 

   return true;
}

inline D3DFORMAT BDXTQUnpack::getD3DDataFormatFromDDX(eDDXDataFormat dataFormat)
{
   switch (dataFormat)
   {
      case cDDXDataFormatDXT1Q:  return D3DFMT_DXT1; 
      case cDDXDataFormatDXT5Q:  return D3DFMT_DXT5; 
      case cDDXDataFormatDXT5HQ: return D3DFMT_DXT5; 
      case cDDXDataFormatDXT5YQ: return D3DFMT_DXT5; 
      case cDDXDataFormatDXNQ:   return D3DFMT_DXN;  
   }    
   return D3DFMT_UNKNOWN;
}

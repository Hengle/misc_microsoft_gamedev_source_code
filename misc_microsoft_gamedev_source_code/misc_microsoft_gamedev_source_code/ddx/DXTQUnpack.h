//==============================================================================
//
// File: DXTQUnpack.h
//
// Copyright (c) 2006 Ensemble Studios
//
//==============================================================================
#pragma once
#include "DXTQTypes.h"
#include "DDXDef.h"
#include "DDXPackParams.h"
#include "huffman.h"

//==============================================================================
// class BDXTQUnpack
//==============================================================================
class BDXTQUnpack
{
   struct BDXTQCodebooks
   {
      BDynamicArray<DWORD>    mColorCodebook;
      BDynamicArray<DWORD>    mColorSelectorCodebook;
      BDynamicArray<WORD>     mAlphaCodebook;
      BDynamicArray<uint64>   mAlphaSelectorCodebook;   

      bool init(const BDXTQHeader& header, const BYTE* pCachedMem, bool bigEndian);   
   };
   
   struct BDXTQHuffmanDecoders
   {
      BHuffmanDec mColorDecoder;
      BHuffmanDec mColorSelectorDecoder;
      BHuffmanDec mAlphaDecoder;
      BHuffmanDec mAlphaSelectorDecoder;
      
      bool init(const BDXTQHeader& header, const BYTE* pCachedMem);  
   };
   
   static bool decodeSegment(
      BYTE* __restrict pDst,
      const BDXTQHeader& header,
      const BYTE* pCachedMem,
      const BDXTQHeader::BSegment& segment,
      const BDXTQCodebooks& codebooks,
      BDXTQHuffmanDecoders& decoders,
      eDDXDataFormat dataFormat,
      bool writeCombinedDst);
      
public:
   static bool unpackDXTQToRawDXT(
      const BDDXTextureInfo& srcTextureInfo,
      const BYTE* pSrcData, uint srcDataSize,
      BDDXTextureInfo& dstTextureInfo, BByteArray& dstData);
   
   static bool getDXTQInfo(
      const BDDXTextureInfo& srcTextureInfo,
      const BYTE* pSrcData, uint srcDataSize,
      BDDXTextureInfo& dstTextureInfo, 
      IDirect3DTexture9* pDstHeader,
      uint& numBaseBlocks, uint& numMipChainBlocks, uint& bytesPerBlock);
         
   static bool unpackDXTQToTiledDXT(
      const eDDXDataFormat format, 
      const BYTE* pSrcData, uint srcDataSize,
      BYTE* pDstTexture, uint dstTextureSize,
      bool decodeBase, bool decodeMipChain);
      
   static bool unpackDXTQBasePrep(
      const eDDXDataFormat format, 
      const BYTE* pSrcData, uint srcDataSize,
      void*& pDstBaseData, uint& dstBaseDataSize, 
      BMemoryHeap* pHeap);

   static bool unpackDXTQBase(
      const void* pBaseData, uint baseDataSize,
      BYTE* pDstTexture, uint dstTextureSize);
         
   static D3DFORMAT getD3DDataFormatFromDDX(eDDXDataFormat dataFormat);
};

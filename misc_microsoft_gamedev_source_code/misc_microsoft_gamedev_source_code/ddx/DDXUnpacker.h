//============================================================================
//
// File: DDXUnpacker.h
// Copyright (c) 2005-2006, Ensemble Studios
//
//============================================================================
#pragma once

#include "DDXDef.h"
#include "DDXPackParams.h"
#include "resource\ecfUtils.h"

class BDDXUnpacker
{
public:
   BDDXUnpacker();

   bool getTextureInfo(const BECFFileReader& ecfFileReader, bool unpackMipChain, BDDXHeader& outHeader, BDDXTextureInfo& outTextureInfo);
   
   bool unpackToNative(
      const BYTE* pData, uint dataSize,
      bool unpackMipChain, bool platformSpecificData, 
      BDDXTextureInfo& outTextureInfo,
      BByteArray& outStream);
      
private:
   bool decompressNativeImage(
      const BYTE*& pData,
      uint& dataSize,
      const BDDXHeader& header, bool mipFlag,
      BByteArray& outStream);
      
   bool decompressNativeTextureNormal(
      uint width, uint height,
      const BDDXHeader& header,
      const BYTE* pMip0Data, uint mip0DataSize,
      const BYTE* pMipChainData, uint mipChainDataSize,
      uint numFaces,
      bool unpackMipChain, 
      BByteArray& outStream,
      BDDXTextureInfo& outTextureInfo);      
      
   bool inflateData(
      const BYTE* pData,
      uint dataSize,
      BByteArray& outStream);
      
   bool decompressNativeTextureXbox(
      uint width, uint height,
      const BDDXHeader& header,
      const BYTE* pMip0Data, uint mip0DataSize,
      const BYTE* pMipChainData, uint mipChainDataSize,
      uint numFaces,
      bool unpackMipChain, 
      BByteArray& outStream,
      BDDXTextureInfo& outTextureInfo,
      bool platformSpecificData);      
};

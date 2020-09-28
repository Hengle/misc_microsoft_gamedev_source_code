//============================================================================
//
// File: DDXPacker.h
// Copyright (c) 2005-2006, Ensemble Studios
//
//============================================================================
#pragma once

#include "DDXDef.h"
#include "DDXPackParams.h"

class BDDXPacker
{
public:
   BDDXPacker();
         
   bool packNative(
      const BYTE* pData, const uint dataSize,
      const BDDXTextureInfo& textureInfo,
      BByteArray& stream,
      eDDXPlatform platform = cDDXPlatformNone);

private:
   typedef BDynamicArray< BByteArray > BMipDataArray;
   
   static bool deflateData(const BYTE* pSrcData, uint srcDataSize, BByteArray& stream);
            
   bool packTextureNormal(
      const BYTE* pData, const uint dataSize,
      BMipDataArray faceData[6], 
      const BDDXSerializedTexture& srcTexture,
      const BDDXTextureInfo& textureInfo,
      uint numFaces, bool cubemap, uint numMipChainLevels, uint totalMipLevels);
      
   bool packTextureXbox(
      BMipDataArray faceData[6], 
      const BDDXSerializedTexture& srcTexture,
      const BDDXTextureInfo& textureInfo,
      uint numFaces, bool cubemap, uint numMipChainLevels, uint totalMipLevels);
      
   bool compressSurface(
      const BYTE* pSrcData, uint srcDataSize,
      const BDDXTextureInfo& textureInfo,
      BByteArray& stream);
      
   bool createDDXStream(
      const BMipDataArray* pFaceData,
      const uint width,
      const uint height,
      const bool hasAlpha,
      const float hdrScale,
      const eDDXDataFormat dataFormat,
      const eDDXResourceType resourceType,
      const uint numMipChainLevels,
      BByteArray& stream,
      eDDXPlatform platform);
      
   static bool translateDDXFormatToD3D(D3DFORMAT& format, eDDXDataFormat ddxFormat, bool bigEndian, bool tiled);
};


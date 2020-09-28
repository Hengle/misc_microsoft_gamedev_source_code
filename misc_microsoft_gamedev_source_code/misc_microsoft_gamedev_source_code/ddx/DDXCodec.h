// File: DDXCodec.h
#pragma once

#include "RGBAImage.h"
#include "ddxdef.h"
#include "DDXPackParams.h"

class BDDXCodec
{
public:
   static bool unpackTexture(
      const BYTE* pNativeData, const DWORD nativeDataSize,
      const BDDXTextureInfo& nativeTextureInfo,
      BDDXTextureInfo& outTextureInfo,
      const bool unpackMipChain,
      const bool unpackToDXT,
      BByteArray& outStream);

   static bool packTexture(
      const BYTE* pData, const DWORD dataSize,
      const BDDXTextureInfo& textureInfo,
      const BDDXPackParams& options,
      BDDXTextureInfo& outTextureInfo,
      BByteArray& stream);
      
private:
   static bool unpackSurface(
      const BYTE*& pData, DWORD& dataSize, 
      const DWORD width, const DWORD height, 
      const eDDXDataFormat format,
      const bool unpackToDXT,
      const float hdrScale,
      BByteArray& outStream,
      eDDXDataFormat& outFormat);

   static bool packSurface(
      const BYTE* pData, const DWORD dataSize, 
      const DWORD width, const DWORD height, 
      eDDXDataFormat srcFormat,
      const bool hasAlpha,    
      const eDDXResourceType resourceType,
      const eDDXDataFormat packFormat,
      const BDDXPackParams::eFlags packFlags,
      const float hdrScale,
      BByteArray& outStream);
      
   static void prepareImage(
      BRGBAImage& dst, 
      const BRGBAImage& src, 
      const bool hasAlpha,    
      const eDDXResourceType resourceType,
      const eDDXDataFormat format,
      const BDDXPackParams::eFlags packFlags);
  
   static bool packSurface32(
      const BYTE* pData, const DWORD dataSize, 
      const DWORD width, const DWORD height, 
      const bool hasAlpha,    
      const eDDXResourceType resourceType,
      const eDDXDataFormat ddxFormat,
      const BDDXPackParams::eFlags packFlags,
      BByteArray& outStream);
      
   static bool packSurface64(
      const BYTE* pData, const DWORD dataSize, 
      const DWORD width, const DWORD height, 
      const bool hasAlpha,    
      const eDDXResourceType resourceType,
      const eDDXDataFormat ddxFormat,
      const BDDXPackParams::eFlags packFlags,
      const float hdrScale,
      BByteArray& outStream);      
};


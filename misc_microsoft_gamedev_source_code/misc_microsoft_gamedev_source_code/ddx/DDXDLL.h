// File: DDXDLL.h
#pragma once
#include "DDXDef.h"
#include "DDXPackParams.h"

#define DDX_DLL_INTERFACE_VERSION 7
#define DDX_DLL_FILENAME   "ddx7.dll"
#define DDX_DLL_ENTRYPOINT "CreateDDXDLL"

class IDDXDLL7;
class IDDXBuffer;
typedef IDDXDLL7* (*DDX_DLL_ENTRYPOINT_FUNC)(DWORD interfaceVersion);

extern "C" __declspec(dllexport) IDDXDLL7* CreateDDXDLL(DWORD interfaceVersion);

class IDDXBuffer
{
public:
   virtual const BYTE* getPtr(void) const = 0;
   virtual DWORD getSize(void) const = 0;
   virtual void release(void) = 0;
};

class IDDXDLL7
{
public:
   // Returns the version of this interface.
   virtual DWORD getVersion() = 0;

   // A cubemap is an array of six textures.
   // A texture is a collection of surfaces (one for each mipmap).
   // A surface is a single image stored in little endian, raster order, either at the DXT blocks or pixel level.
   // "native" indicates the image data may be any DDX format. 
   // The pack/unpack surface methods are low-level methods that compress/decompress native data to/from 32-bit ARGB textures.
   // The pack/unpack texture methods bundle a collection of surfaces into a texture. packTexture() can automatically generate mipmaps.
   
   // Packs a texture to a DDX file.
   virtual bool packDDX(
      IDDXBuffer** ppHandle,
      const BYTE* pTextureData, const DWORD textureDataSize, const BDDXTextureInfo& textureInfo, eDDXPlatform platform) = 0;      
   
   // Unpacks a DDX file to a texture. 
   virtual bool unpackDDX(
      IDDXBuffer** ppHandle,
      BDDXTextureInfo& textureInfo,
      const BYTE* pDDXData, const DWORD DDXMemSize, bool unpackAllMips, bool platformSpecificData) = 0;
   
   // Verifies the DDX header and data Adler-32's.
   virtual bool check(const BYTE* pDDXData, DWORD DDXDataSize) = 0;
   
   // Get description of DDX file.
   virtual bool getDesc(const BYTE* pDDXData, DWORD DDXDataSize, BDDXDesc& outDesc) = 0;
      
   // Unpacks a texture (including mips) to ARGB using unpackSurface().
   virtual bool unpackTexture(
      IDDXBuffer** ppHandle,
      const BYTE* pNativeData, const DWORD nativeDataSize,
      const BDDXTextureInfo& nativeTextureInfo,
      BDDXTextureInfo& outTextureInfo,
      const bool unpackMipChain,
      const bool unpackToDXT) = 0;
   
   // Packs an ARGB texture (including mips) to native surfaces by calling packSurface().
   virtual bool packTexture(
      IDDXBuffer** ppHandle,
      const BYTE* pRGBAData, const DWORD dataSize,
      const BDDXTextureInfo& textureInfo,
      const BDDXPackParams& options,
      BDDXTextureInfo& outTextureInfo) = 0;
   
   // Gets description of DDT file.
   virtual bool getDDTDesc(
      BDDXTextureInfo& textureInfo,
      const BYTE* pDDTData, const DWORD DDTMemSize) = 0;

   // Convert DDT to A8B8G8R8 (if convertToABGR is true) or native texture.       
   virtual bool DDTtoRGBA8(
      IDDXBuffer** ppHandle,
      BDDXTextureInfo& textureInfo,
      const bool convertToABGR,
      const BYTE* pDDTData, const DWORD DDTMemSize) = 0;
      
   // Releases the interface.
   virtual void release(void) = 0;
};

//---------------------------------------------------------

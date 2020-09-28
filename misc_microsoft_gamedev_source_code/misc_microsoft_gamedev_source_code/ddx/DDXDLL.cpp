// File: DDXDLL.cpp
#define _WIN32_WINNT 0x0500
#include <windows.h>
#include "ddxdll.h"

#include "xcore.h"
#include "containers\dynamicarray.h"
#include "compression.h"

#include "colorutils.h"
#include "RGBAImage.h"

#include "DDXSerializedTexture.h"
#include "DDXUtils.h"
#include "DDXPacker.h"
#include "DDXUnpacker.h"
#include "DDXCodec.h"

#include "DDTUnpacker.h"

//--------------------------------------------------------------------
BOOL APIENTRY DllMain( 
   HANDLE hModule, 
   DWORD  ul_reason_for_call, 
   LPVOID lpReserved)
{
   hModule;
   ul_reason_for_call;
   lpReserved;
   
   return TRUE;
}

//---------------------------------------------------------

class BDDXBuffer : public IDDXBuffer
{
public:
   BDDXBuffer()
   {
   }
   
   virtual ~BDDXBuffer()
   {
   }
         
   const BYTE* getPtr(void) const
   {
      const BYTE* pRet = NULL;
      
      try
      {
         pRet = mData.begin();
      }
      catch (...)
      {
         pRet = NULL;
      }
      
      return pRet;
   }
   
   DWORD getSize(void) const
   {
      DWORD ret = 0;
      
      try
      {
         ret = mData.size();
      }
      catch (...)
      {
         ret = 0;
      }
      
      return ret;
   }
   
   void release(void) 
   {
      try
      {
         delete this;
      }
      catch (...)
      {
      }
   }
   
   void swap(BByteArray& data)
   {
      data.swap(mData);
   }
   
private:
   BByteArray mData;
};

//---------------------------------------------------------

class BDDXDLL7 : public IDDXDLL7
{
public:
   DWORD getVersion();
   
   bool packDDX(
      IDDXBuffer** ppHandle,
      const BYTE* pTextureData, const DWORD textureDataSize, const BDDXTextureInfo& textureInfo, eDDXPlatform platform);

   bool unpackDDX(
      IDDXBuffer** ppHandle,
      BDDXTextureInfo& textureInfo,
      const BYTE* pDDXData, const DWORD DDXMemSize, bool unpackAllMips, bool platformSpecificData);

   void release(void);
   
   bool check(const BYTE* pDDXData, DWORD DDXDataSize);

   bool getDesc(const BYTE* pDDXData, DWORD DDXDataSize, BDDXDesc& outDesc);

   bool unpackSurface(
      IDDXBuffer** ppHandle,
      eDDXDataFormat& outFormat,
      const BYTE* pData, DWORD& dataSize, 
      const DWORD width, const DWORD height, 
      const eDDXDataFormat format,
      const bool unpackToDXT,
      const float hdrScale);

   bool packSurface(
      IDDXBuffer** ppHandle,
      const DWORD* pRGBAData, 
      const DWORD width, const DWORD height,
      eDDXDataFormat srcFormat,
      const bool hasAlpha,    
      const eDDXResourceType resourceType,
      const eDDXDataFormat format,
      const BDDXPackParams::eFlags packFlags,
      const BDDXDXTQPackParams& DXTQParams,
      const float hdrScale);

   bool getSurfaceSize(DWORD& size, const BYTE* pData, DWORD width, DWORD height, eDDXDataFormat format);

   bool unpackTexture(
      IDDXBuffer** ppHandle,
      const BYTE* pNativeData, const DWORD nativeDataSize,
      const BDDXTextureInfo& nativeTextureInfo,
      BDDXTextureInfo& outTextureInfo,
      const bool unpackMipChain,
      const bool unpackToDXT);

   bool packTexture(
      IDDXBuffer** ppHandle,
      const BYTE* pRGBAData, const DWORD dataSize,
      const BDDXTextureInfo& textureInfo,
      const BDDXPackParams& options,
      BDDXTextureInfo& outTextureInfo);
      
   bool DDTtoRGBA8(
      IDDXBuffer** ppHandle,
      BDDXTextureInfo& textureInfo,
      const bool convertToABGR,
      const BYTE* pDDTData, const DWORD DDTMemSize);

   bool getDDTDesc(
      BDDXTextureInfo& textureInfo,
      const BYTE* pDDTData, const DWORD DDTMemSize);
};


//--------------------------------------------------------------------
extern "C" __declspec(dllexport) IDDXDLL7* CreateDDXDLL(DWORD interfaceVersion)
{
   if (DDX_DLL_INTERFACE_VERSION == interfaceVersion)
      return new BDDXDLL7();
   
   return NULL;
}

//--------------------------------------------------------------------

DWORD BDDXDLL7::getVersion()   
{
   return DDX_DLL_INTERFACE_VERSION;
};

//--------------------------------------------------------------------

bool BDDXDLL7::packDDX(
   IDDXBuffer** ppHandle,
   const BYTE* pRGBAData, const DWORD RGBAMemSize, const BDDXTextureInfo& textureInfo, eDDXPlatform platform)
{   
   bool status = false;

   try
   {
      if (!ppHandle)
         return false;

      *ppHandle = NULL;

      BByteArray stream;

      BDDXPacker packer;

      if (!packer.packNative(pRGBAData, RGBAMemSize, textureInfo, stream, platform))
         return false;

      BDDXBuffer* pBuf = new BDDXBuffer;
      pBuf->swap(stream);
      *ppHandle = pBuf;

      status = true;
   }
   catch (...)
   {
      status = false;
      // This should probably free ppHandle if it's not NULL, but that could throw another exception. Better to leak.
      *ppHandle = NULL;
   }      

   return status;
}

bool BDDXDLL7::unpackDDX(
   IDDXBuffer** ppHandle,
   BDDXTextureInfo& textureInfo,
   const BYTE* pDDXData, const DWORD DDXMemSize, bool unpackAllMips, bool platformSpecificData)
{
   bool status = false;
   
   try
   {
      if (!ppHandle)
         return false;

      *ppHandle = NULL;

      BByteArray stream;

      BDDXUnpacker unpacker;

      if (!unpacker.unpackToNative(pDDXData, DDXMemSize, unpackAllMips, platformSpecificData, textureInfo, stream))
         return false;

      BDDXBuffer* pBuf = new BDDXBuffer;
      pBuf->swap(stream);
      *ppHandle = pBuf;
      
      status = true;
   }
   catch (...)
   {
      status = false;
      *ppHandle = NULL;
   }      
   
   return status;
}

//--------------------------------------------------------------------

bool BDDXDLL7::check(const BYTE* pDDXData, DWORD DDXDataSize)
{
   bool status = false;
   
   try
   {
      status = BDDXUtils::check(pDDXData, DDXDataSize);
   }
   catch (...)
   {
      status = false;
   }      
   
   return status;
}

//--------------------------------------------------------------------

bool BDDXDLL7::getDesc(const BYTE* pDDXData, DWORD DDXDataSize, BDDXDesc& outDesc)
{
   bool status = false;
   
   try
   {
      status = BDDXUtils::getDesc(pDDXData, DDXDataSize, outDesc);
   }
   catch (...)
   {
      status = false;
   }      
   
   return status;
}

//--------------------------------------------------------------------

bool BDDXDLL7::unpackTexture(
   IDDXBuffer** ppHandle,
   const BYTE* pNativeData, const DWORD nativeDataSize,
   const BDDXTextureInfo& nativeTextureInfo,
   BDDXTextureInfo& outTextureInfo,
   const bool unpackMipChain,
   const bool unpackToDXT)
{
   bool status = true;
   
   try
   {
      if (!ppHandle)
         return false;

      *ppHandle = NULL;
      
      BByteArray stream;      

      if (!BDDXCodec::unpackTexture(pNativeData, nativeDataSize, nativeTextureInfo, outTextureInfo, unpackMipChain, unpackToDXT, stream))
         return false;

      BDDXBuffer* pBuf = new BDDXBuffer;
      pBuf->swap(stream);
      *ppHandle = pBuf;

      status = true;      
   }
   catch (...)
   {
      status = false;
      *ppHandle = NULL;
   }

   return status;
}   

bool BDDXDLL7::packTexture(
   IDDXBuffer** ppHandle,
   const BYTE* pRGBAData, const DWORD dataSize,
   const BDDXTextureInfo& textureInfo,
   const BDDXPackParams& options,
   BDDXTextureInfo& outTextureInfo)
{
   bool status = true;
   
   try
   {
      if (!ppHandle)
         return false;

      *ppHandle = NULL;

      BByteArray stream;      

      if (!BDDXCodec::packTexture(pRGBAData, dataSize, textureInfo, options, outTextureInfo, stream))
         return false;

      BDDXBuffer* pBuf = new BDDXBuffer;
      pBuf->swap(stream);
      *ppHandle = pBuf;

      status = true;      
   }
   catch (...)
   {
      status = false;
      *ppHandle = NULL;
   }

   return status;
}   

bool BDDXDLL7::getDDTDesc(
   BDDXTextureInfo& textureInfo,
   const BYTE* pDDTData, const DWORD DDTMemSize)
{
   bool status = false;

   try
   {
      BDDTUnpacker ddtUnpacker;

      if (!ddtUnpacker.getDesc(pDDTData, DDTMemSize, textureInfo))
         return false;

      status = true;      
   }
   catch (...)
   {
      status = false;
   }

   return status;   
}                        

// Convert DDT to A8B8G8R8 (ARGB in memory) texture. Returns the first mip level only, or the entire mip chain.
bool BDDXDLL7::DDTtoRGBA8(
   IDDXBuffer** ppHandle,
   BDDXTextureInfo& textureInfo,
   const bool convertToABGR,
   const BYTE* pDDTData, const DWORD DDTMemSize)
{
   bool status = false;
   
   try
   {
      if (!ppHandle)
         return false;

      *ppHandle = NULL;

      BByteArray stream;

      BDDTUnpacker ddtUnpacker;

      if (!ddtUnpacker.unpack(pDDTData, DDTMemSize, convertToABGR, stream, textureInfo))
         return false;

      BDDXBuffer* pBuf = new BDDXBuffer;
      pBuf->swap(stream);
      *ppHandle = pBuf;

      status = true;      
   }
   catch (...)
   {
      status = false;
      *ppHandle = NULL;
   }
   
   return status;
}                        

//--------------------------------------------------------------------

void BDDXDLL7::release(void)
{
   try
   {   
      // Check this pointer in case they call release() more than once (that would be a bug but we'll try to handle it).
      if (this)
         delete this;
   }
   catch (...)
   {
   }         
}


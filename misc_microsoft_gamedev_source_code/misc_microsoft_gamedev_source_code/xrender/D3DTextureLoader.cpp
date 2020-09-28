//============================================================================
//
// File: D3DTextureLoader.cpp
//
// Copyright (c) 2006-2008, Ensemble Studios
//
//============================================================================
#include "xrender.h"
#include "D3DTextureLoader.h"
#include "consoleOutput.h"
#include "timer.h"

// ddx
#include "DDXUnpacker.h"
#include "DDTUnpacker.h"
#include "DDXCodec.h"
#include "DDXSerializedTexture.h"

// compression
#include "dxtUtils.h"

// local
#include "renderDraw.h"
#include "xboxTextureHeap.h"
#include "packedTextureManager.h"

//============================================================================
// BD3DTextureLoader::BD3DTextureLoader
//============================================================================
BD3DTextureLoader::BD3DTextureLoader() :
   mHDRScale(1.0f),
   mNumSlices(0),
   mDDXDataFormat(cDDXDataFormatInvalid),
   mAllocationSize(0)
{
   mD3DTexture.clear();
}
   
//============================================================================
// BD3DTextureLoader::~BD3DTextureLoader
//============================================================================
BD3DTextureLoader::~BD3DTextureLoader()
{
   clear();
}

//============================================================================
// BD3DTextureLoader::clear
//============================================================================
void BD3DTextureLoader::clear(bool deleteTexture)
{
   if (deleteTexture)
   {
      mD3DTexture.release();
   }
   
   mD3DTexture.clear();
   
   mAllocationSize = 0;
}

//============================================================================
// BD3DTextureLoader::createFromDDXFileInMemory
//============================================================================
bool BD3DTextureLoader::createFromDDXFileInMemory(const uchar* pData, uint dataLen, const BCreateParams& createParams)
{
   SCOPEDSAMPLE(BD3DTextureLoader_createFromDDXFileInMemory)
   
   if ((createParams.mUsePackedTextureManager) && (gpPackedTextureManager))
   {
      if (createParams.mArraySize)
         return false;
   
      BDDXTextureInfo ddxTextureInfo;
      BDDXTextureInfo unpackedTextureInfo;
      
      bool success = gpPackedTextureManager->create(
         mD3DTexture, 
         pData, dataLen, 
         createParams.mAddUnusedRegionsToHeap, 
         createParams.mLongTermAllocation,
         createParams.mForceSRGBGamma,
         createParams.mManager,
         createParams.mName,
         &mAllocationSize,
         &ddxTextureInfo, &unpackedTextureInfo);

      if (success)
      {
         mHDRScale = ddxTextureInfo.mHDRScale;
         mDDXDataFormat = ddxTextureInfo.mDataFormat;
         return true;
      }
   }
            
   BDDXTextureInfo textureInfo;
   BByteArray stream;

   BDDXUnpacker unpacker;
   
   // Arghh. Can't load Xbox specific data into a texture array. We'll create a tool to create tiled texture arrays, eventually.
   bool platformSpecificData = true;
   if (createParams.mArraySize)
      platformSpecificData = false;
      
   bool success = unpacker.unpackToNative(pData, dataLen, true, platformSpecificData, textureInfo, stream);
   if (!success)
      return false;
        
   if ((textureInfo.mPlatform == cDDXPlatformNone) && (!getDDXDataFormatIsFixedSize(textureInfo.mOrigDataFormat)))
   {
      BDDXCodec codec;
      codec;
      BByteArray dxtStream;
      BDDXTextureInfo dxtTextureInfo;
      success = codec.unpackTexture(stream.getPtr(), stream.getSize(), textureInfo, dxtTextureInfo, true, true, dxtStream);
      if (!success)
         return false;

      stream.swap(dxtStream);            
      textureInfo = dxtTextureInfo;
   }         
   
   return createFromTextureData(stream.getPtr(), stream.size(), textureInfo, createParams);
}

//============================================================================
// BD3DTextureLoader::createFromDDTFileInMemory
//============================================================================
bool BD3DTextureLoader::createFromDDTFileInMemory(const uchar* pData, uint dataLen, const BCreateParams& createParams)
{
   BDDXTextureInfo textureInfo;
   BByteArray stream;

   BDDTUnpacker unpacker;
   bool success = unpacker.unpack(pData, dataLen, false, stream, textureInfo);
   if (!success)
      return false;
   
   return createFromTextureData(stream.getPtr(), stream.size(), textureInfo, createParams);
}

//============================================================================
// BD3DTextureLoader::createFromXPRFileInMemory
//============================================================================
bool BD3DTextureLoader::createFromXPRFileInMemory(const uchar* pData, uint dataLen, const BCreateParams& createParams)
{
   // Most of this code is adapted from the .xpr loading in AtgResource.cpp PackedResource::Create
   // We make additional assumptions here that we're loading .xpr files with a single
   // array texture of a supported format

   struct RESOURCE
   {
       DWORD dwType;
       DWORD dwOffset;
       DWORD dwSize;
       CHAR* strName;
   };

   // Resource types
   enum
   {
       RESOURCETYPE_USERDATA        = (('U'<<24)|('S'<<16)|('E'<<8)|('R')),
       RESOURCETYPE_TEXTURE         = (('T'<<24)|('X'<<16)|('2'<<8)|('D')),
       RESOURCETYPE_CUBEMAP         = (('T'<<24)|('X'<<16)|('C'<<8)|('M')),
       RESOURCETYPE_VOLUMETEXTURE   = (('T'<<24)|('X'<<16)|('3'<<8)|('D')),
       RESOURCETYPE_VERTEXBUFFER    = (('V'<<24)|('B'<<16)|('U'<<8)|('F')),
       RESOURCETYPE_INDEXBUFFER     = (('I'<<24)|('B'<<16)|('U'<<8)|('F')),
       RESOURCETYPE_EOF             = 0xffffffff
   };

   struct XPR_HEADER
   {
       DWORD dwMagic;
       DWORD dwHeaderSize;
       DWORD dwDataSize;
   };
   const DWORD XPR2_MAGIC_VALUE = 0x58505232;

   const uchar* pCurFileOfs = pData;
   uint fileLeft = dataLen;

   // Read in and verify the XPR magic header
   XPR_HEADER xprh;
   if (fileLeft < sizeof(XPR_HEADER))
   {
      gConsoleOutput.output(cMsgError, "BD3DTextureLoader::createFromXPRFileInMemory: Invalid Xbox Packed Resource (.xpr) file");
      return false;
   }

   memcpy(&xprh, pCurFileOfs, sizeof(XPR_HEADER));
   pCurFileOfs += sizeof(XPR_HEADER);
   fileLeft -= sizeof(XPR_HEADER);

   if( xprh.dwMagic != XPR2_MAGIC_VALUE )
   {
      gConsoleOutput.output(cMsgError, "BD3DTextureLoader::createFromXPRFileInMemory: Invalid Xbox Packed Resource (.xpr) file: Magic = 0x%08lx\n", xprh.dwMagic);
      return false;
   }

   // Compute memory requirements
   DWORD dwSysMemDataSize = xprh.dwHeaderSize;
   DWORD dwVidMemDataSize = xprh.dwDataSize;

   // Allocate temp file memory
   BYTE* pSysMemData = new BYTE[dwSysMemDataSize];

   // Read in the data from the file
   BVERIFY(fileLeft >= dwSysMemDataSize + dwVidMemDataSize);

   memcpy(pSysMemData, pCurFileOfs, dwSysMemDataSize);
   pCurFileOfs += dwSysMemDataSize;

   // Make sure there is only a single texture in this xpr file
   // Extract resource table from the header data and make sure there is
   // only a single texture in this xpr file
   DWORD dwNumResourceTags = *(DWORD*) (pSysMemData+0);
   RESOURCE* pResourceTags = (RESOURCE*) (pSysMemData+4);
   if (dwNumResourceTags != 1)
   {
      gConsoleOutput.output(cMsgError, "BD3DTextureLoader::createFromXPRFileInMemory: .xpr file resources != 1");
      clear();
      delete[] pSysMemData;
      return false;
   }
   if ((pResourceTags[0].dwType & 0xffff0000) != (RESOURCETYPE_TEXTURE & 0xffff0000))
   {
      gConsoleOutput.output(cMsgError, "BD3DTextureLoader::createFromXPRFileInMemory: .xpr file resource is not a texture");
      clear();
      delete[] pSysMemData;
      return false;
   }

   // Get the texture description and make sure it is an array texture with a supported format
   XGTEXTURE_DESC dataTexDesc;
   XGGetTextureDesc((D3DBaseTexture*) &pSysMemData[pResourceTags[0].dwOffset], 0, &dataTexDesc);
   if (dataTexDesc.ResourceType != D3DRTYPE_ARRAYTEXTURE)
   {
      gConsoleOutput.output(cMsgError, "BD3DTextureLoader::createFromXPRFileInMemory: .xpr file resource is not a texture");
      clear();
      delete[] pSysMemData;
      return false;
   }

   // Get format
   mDDXDataFormat = getDDXFormatFromD3DFormat(dataTexDesc.Format);
   if (mDDXDataFormat == cDDXDataFormatInvalid)
   {
      gConsoleOutput.output(cMsgError, "BD3DTextureLoader::createFromXPRFileInMemory: .xpr has invalid texture format");
      clear();
      delete[] pSysMemData;
      return false;
   }

   // Allocate and copy IDirect3DBaseTexture9 data
   IDirect3DBaseTexture9 D3DBaseTex = *reinterpret_cast<const IDirect3DBaseTexture9*>(&pSysMemData[pResourceTags[0].dwOffset]);
   IDirect3DBaseTexture9* pBaseTex = new IDirect3DBaseTexture9;
   memcpy(pBaseTex, &D3DBaseTex, sizeof(IDirect3DBaseTexture9));
   pBaseTex->Common |= D3DCOMMON_CPU_CACHED_MEMORY;

   // Allocate and copy image data
   BYTE* pPhysicalTextureData = (BYTE*) gpXboxTextureHeap->getValley(pBaseTex, &mAllocationSize, createParams.mAddUnusedRegionsToHeap, createParams.mLongTermAllocation);
   if (!pPhysicalTextureData)
   {
      delete pBaseTex;

      gConsoleOutput.output(cMsgError, "BD3DTextureLoader::createFromXPRFileInMemory: Failed creating D3D resource");
      clear();
      delete[] pSysMemData;
      return false;
   }
   
   if (mAllocationSize < dwVidMemDataSize)
   {
      gpXboxTextureHeap->releaseValley(pBaseTex);

      delete pBaseTex;

      gConsoleOutput.output(cMsgError, "BD3DTextureLoader::createFromXPRFileInMemory: Valley alloc was too small");
      clear();
      delete[] pSysMemData;
      return false;
   }

#ifndef BUILD_FINAL         
   gpXboxTextureHeap->setValleyTextureDetails(pPhysicalTextureData, createParams.mManager, createParams.mName, createParams.mArrayIndex);
#endif   
   
   XGOffsetResourceAddress(pBaseTex, pPhysicalTextureData);
   
   gRenderDraw.copyTextureData(pBaseTex, pCurFileOfs, dwVidMemDataSize);
         
   // Set texture loader data
   mD3DTexture.setArrayTexture((IDirect3DArrayTexture9*) pBaseTex, BD3DTexture::cXboxTextureHeap);
   mD3DTexture.setIdentifier(0);
      
   // Delete temp file data
   delete[] pSysMemData;
   
   return true;
}

//============================================================================
// BD3DTextureLoader::createFromTextureData
//============================================================================
bool BD3DTextureLoader::createFromTextureData(const uchar* pData, uint dataLen, const BDDXTextureInfo& textureInfo, const BCreateParams& createParams)
{
   if (createParams.mArraySize)
   {
      if (getTextureType() != cTTArray)
         clear();
   }
   else
   {
      // Per-texture mHDRScale won't work with texture arrays.
      mHDRScale = textureInfo.mHDRScale;
      
      BDEBUG_ASSERT(createParams.mArrayIndex == 0);
      clear();
   }
               
   BD3DTextureType D3DTextureType = cTTInvalid;
   
   if (textureInfo.mResourceType == cDDXResourceTypeCubeMap)
   {
      if ((createParams.mArrayIndex) || (createParams.mArraySize))
      {
         gConsoleOutput.output(cMsgError, "BD3DTextureLoader::createFromTextureData: Texture is a cubemap, but create params want an array texture");
         clear();
         return false;
      }

      if (textureInfo.mWidth != textureInfo.mHeight)
      {
         gConsoleOutput.output(cMsgError, "BD3DTextureLoader::createFromTextureData: Texture is a cubemap, but width != height");
         clear();
         return false;
      }
      
      D3DTextureType = cTTCubemap;
   }
   else if (createParams.mArraySize)
   {
      if (createParams.mArrayIndex >= createParams.mArraySize)
      {
         gConsoleOutput.output(cMsgError, "BD3DTextureLoader::createFromTextureData: Invalid array index");
         return false;
      }

      if (!getBaseTexture())
      {
         D3DTextureType = cTTArray;
      }
      else
      {
         D3DTextureType = getTextureType();
         
         BDEBUG_ASSERT(D3DTextureType == cTTArray);

         if ( (textureInfo.mWidth != getWidth()) || (textureInfo.mHeight != getHeight()) )
         {
            gConsoleOutput.output(cMsgError, "BD3DTextureLoader::createFromTextureData: Existing array's dimensions doesn't match the supplied texture's");
            return false;
         }
         
         if ( textureInfo.mDataFormat != mDDXDataFormat )
         {
            gConsoleOutput.output(cMsgError, "BD3DTextureLoader::createFromTextureData: Existing array's format doesn't match the supplied texture's");
            return false;
         }

         const uint existingArraySize = getArrayTexture()->GetArraySize();
         const uint existingLevels = getArrayTexture()->GetLevelCount();

         if (existingArraySize != createParams.mArraySize) 
         {
            gConsoleOutput.output(cMsgError, "BD3DTextureLoader::createFromTextureData: Existing array's size doesn't match the create param's array size");
            return false;
         }
         else if (existingLevels > (textureInfo.mNumMipChainLevels + 1))
         {
            gConsoleOutput.output(cMsgError, "BD3DTextureLoader::createFromTextureData: Texture doesn't have enough mips to fill the array surface");
            return false;
         }
      }
   }
   else
   {
      D3DTextureType = cTT2D;
   }
   
   mDDXDataFormat = textureInfo.mDataFormat;
   
   BDEBUG_ASSERT(D3DTextureType != cTTInvalid);
                     
   bool status = true;
   switch (textureInfo.mPlatform)
   {
      case cDDXPlatformXbox:
      {
         status = createFromXboxTextureData(D3DTextureType, pData, dataLen, textureInfo, createParams);
         break;
      }
      case cDDXPlatformNone:
      {
         status = createFromRawTextureData(D3DTextureType, pData, dataLen, textureInfo, createParams);
         break;
      }
      default:
      {
         gConsoleOutput.output(cMsgError, "BD3DTextureLoader::createFromTextureData: Unsupported platform");
         clear();
         status = false;
         break;
      }
   }
         
   return status;
}

//============================================================================
// BD3DTextureLoader::createFromXboxTextureData
//============================================================================
bool BD3DTextureLoader::createFromXboxTextureData(
   BD3DTextureType D3DTextureType,
   const uchar* pData, uint dataLen, 
   const BDDXTextureInfo& textureInfo, const BCreateParams& createParams)
{  
   BCOMPILETIMEASSERT((sizeof(IDirect3DBaseTexture9) == sizeof(IDirect3DCubeTexture9)) && (sizeof(IDirect3DBaseTexture9) == sizeof(IDirect3DArrayTexture9)) && (sizeof(IDirect3DBaseTexture9) == sizeof(IDirect3DTexture9)));
   
   if (getDDXDataFormatBitsPerPixel(textureInfo.mDataFormat) == 32)
      gConsoleOutput.output(cMsgWarning, "BD3DTextureLoader::createFromXboxTextureData: Creating 32-bit texture %ix%i", textureInfo.mWidth, textureInfo.mHeight);
   
   // Copying a tiled 2D texture into a slice of a texture array is not trivial, so let's not support it here.
   if (D3DTextureType == cTTArray)
   {
      gConsoleOutput.output(cMsgError, "BD3DTextureLoader::createFromXboxTextureData: Can't copy data into texture array");
      clear();
      return false;
   }
   
   IDirect3DBaseTexture9 D3DBaseTex = *reinterpret_cast<const IDirect3DBaseTexture9*>(pData);
      
   XGTEXTURE_DESC dataTexDesc;
   XGGetTextureDesc(&D3DBaseTex, 0, &dataTexDesc);

   const bool cubemap = (textureInfo.mResourceType == cDDXResourceTypeCubeMap);
   const DWORD headerSize = cubemap ? sizeof(IDirect3DCubeTexture9) : sizeof(IDirect3DTexture9);
   const DWORD actualHeaderSize = Utils::AlignUpValue(headerSize, 16);
   
   const uchar* pXboxData = pData + actualHeaderSize;
   uint xboxDataSize = dataLen - actualHeaderSize;

   IDirect3DBaseTexture9* pBaseTex = new IDirect3DBaseTexture9;
      
   memcpy(pBaseTex, &D3DBaseTex, sizeof(IDirect3DBaseTexture9));
   pBaseTex->Common |= D3DCOMMON_CPU_CACHED_MEMORY;
   
   if (createParams.mForceSRGBGamma)
   {
      GPUTEXTURE_FETCH_CONSTANT& fc = pBaseTex->Format;
      fc.SignX = GPUSIGN_GAMMA;
      fc.SignY = GPUSIGN_GAMMA;
      fc.SignZ = GPUSIGN_GAMMA;
   }
    
   bool success = false;     

   if ((createParams.mUsePackedTextureManager) && (gpPackedTextureManager))
   {
      success = gpPackedTextureManager->create(
         pBaseTex,
         pXboxData, xboxDataSize,
         NULL, 0, 
         createParams.mAddUnusedRegionsToHeap, 
         createParams.mLongTermAllocation,
         createParams.mManager,
         createParams.mName,
         &mAllocationSize);
         
      if (success)
      {
         if (cubemap)
            mD3DTexture.setCubeTexture(reinterpret_cast<IDirect3DCubeTexture9*>(pBaseTex), BD3DTexture::cPackedTextureManager);
         else
            mD3DTexture.setTexture(reinterpret_cast<IDirect3DTexture9*>(pBaseTex), BD3DTexture::cPackedTextureManager);
      }            
   }
   
   if (!success)
   {
      void* pPhysicalTextureData = gpXboxTextureHeap->getValley(pBaseTex, &mAllocationSize, createParams.mAddUnusedRegionsToHeap, createParams.mLongTermAllocation);
      if (!pPhysicalTextureData)
      {
         delete pBaseTex;
         
         mD3DTexture.clear();

         gConsoleOutput.output(cMsgError, "BD3DTextureLoader::createFromTextureData: Valley alloc failed, out of memory!");
         clear();
         return false;
      }
      
      if (mAllocationSize < xboxDataSize)
      {
         gpXboxTextureHeap->releaseValley(pBaseTex);
         
         delete pBaseTex;
         
         mD3DTexture.clear();

         gConsoleOutput.output(cMsgError, "BD3DTextureLoader::createFromTextureData: Valley alloc was too small");
         clear();
         return false;
      }

#ifndef BUILD_FINAL            
      gpXboxTextureHeap->setValleyTextureDetails(pPhysicalTextureData, createParams.mManager, createParams.mName, createParams.mArrayIndex);
#endif      
      
      XGOffsetResourceAddress(pBaseTex, pPhysicalTextureData);
      
      gRenderDraw.copyTextureData(pBaseTex, pXboxData, xboxDataSize);
      
      if (cubemap)
         mD3DTexture.setCubeTexture(reinterpret_cast<IDirect3DCubeTexture9*>(pBaseTex), BD3DTexture::cXboxTextureHeap);
      else
         mD3DTexture.setTexture(reinterpret_cast<IDirect3DTexture9*>(pBaseTex), BD3DTexture::cXboxTextureHeap);
         
      mD3DTexture.setIdentifier(0);         
   }      
              
   return true;
}

//============================================================================
// BD3DTextureLoader::createFromRawTextureData
//============================================================================
bool BD3DTextureLoader::createFromRawTextureData(
   BD3DTextureType D3DTextureType,
   const uchar* pData, uint dataLen, 
   const BDDXTextureInfo& textureInfo, const BCreateParams& createParams)
{
   eDDXDataFormat DDXFormat = textureInfo.mOrigDataFormat;
   
   D3DFORMAT D3DFormat;               
   if (!translateDDXFormatToD3D(D3DFormat, textureInfo, createParams))
   {
      gConsoleOutput.output(cMsgError, "BD3DTextureLoader::createFromTextureData: Unable to translate DDX format to D3D format");
      clear();
      return false;
   }
   
   HRESULT hres = E_FAIL;

   if (!createParams.mArraySize)
   {
      IDirect3DBaseTexture9* pTex = new IDirect3DBaseTexture9;
      Utils::ClearObj(*pTex);
      
      if (textureInfo.mResourceType == cDDXResourceTypeCubeMap)
         XGSetCubeTextureHeader(textureInfo.mWidth, textureInfo.mNumMipChainLevels + 1, D3DUSAGE_CPU_CACHED_MEMORY, D3DFormat, 0, 0, XGHEADER_CONTIGUOUS_MIP_OFFSET, (IDirect3DCubeTexture9*)pTex, NULL, NULL);
      else
         XGSetTextureHeader(textureInfo.mWidth, textureInfo.mHeight, textureInfo.mNumMipChainLevels + 1, D3DUSAGE_CPU_CACHED_MEMORY, D3DFormat, 0, 0, XGHEADER_CONTIGUOUS_MIP_OFFSET, 0, (IDirect3DTexture9*)pTex, NULL, NULL);
      
      void* pPhysicalMemory = gpXboxTextureHeap->getValley(pTex, &mAllocationSize, createParams.mAddUnusedRegionsToHeap, createParams.mLongTermAllocation);
      if (pPhysicalMemory)
      {
#ifndef BUILD_FINAL            
         gpXboxTextureHeap->setValleyTextureDetails(pPhysicalMemory, createParams.mManager, createParams.mName, createParams.mArrayIndex);
#endif         
         
         XGOffsetResourceAddress(pTex, pPhysicalMemory);
         
         if (textureInfo.mResourceType == cDDXResourceTypeCubeMap)
            mD3DTexture.setCubeTexture((IDirect3DCubeTexture9*)pTex, BD3DTexture::cXboxTextureHeap);
         else
            mD3DTexture.setTexture((IDirect3DTexture9*)pTex, BD3DTexture::cXboxTextureHeap);         
         mD3DTexture.setIdentifier(0);
         
         hres = S_OK;
      }
      else
      {
         delete pTex;
         pTex = NULL;
      }
   }
   else
   {
      uint numSlices = createParams.mArraySize;
      // rg [3/28/06] - Workaround, D3D creates bogus array textures if there's only a single slice
      if (numSlices == 1) 
      {
         gConsoleOutput.warning("BD3DTextureLoader::createFromRawTextureData: A texture array with only 1 slice is being created, but a minimum of 2 slices must be allocated! This is a waste of memory.");
         numSlices = 2;
      }
      
      BDEBUG_ASSERT(createParams.mArrayIndex < numSlices);
      
      if (!getBaseTexture())
      {
         mNumSlices = numSlices;
         
         mArrayHDRScale.resize(numSlices);
         mArrayHDRScale.setAll(1.0f);

         mArrayHDRScale[createParams.mArrayIndex] = textureInfo.mHDRScale;
 
         IDirect3DArrayTexture9* pTex = new IDirect3DArrayTexture9;
         Utils::ClearObj(*pTex);

         XGSetArrayTextureHeader(textureInfo.mWidth, textureInfo.mHeight, numSlices, textureInfo.mNumMipChainLevels + 1, D3DUSAGE_CPU_CACHED_MEMORY, D3DFormat, 0, 0, XGHEADER_CONTIGUOUS_MIP_OFFSET, 0, pTex, NULL, NULL);

         void* pPhysicalMemory = gpXboxTextureHeap->getValley(pTex, &mAllocationSize, createParams.mAddUnusedRegionsToHeap, createParams.mLongTermAllocation);
         if (pPhysicalMemory)
         {
#ifndef BUILD_FINAL               
            gpXboxTextureHeap->setValleyTextureDetails(pPhysicalMemory, createParams.mManager, createParams.mName, createParams.mArrayIndex);
#endif            
            
            XGOffsetResourceAddress(pTex, pPhysicalMemory);

            mD3DTexture.setArrayTexture(pTex, BD3DTexture::cXboxTextureHeap);
            mD3DTexture.setIdentifier(0);
            
            hres = S_OK;
         }
         else
         {
            delete pTex;
            pTex = NULL;
         }
      }
      else
      {
         BASSERT(mNumSlices == numSlices);
                  
         mArrayHDRScale[createParams.mArrayIndex] = textureInfo.mHDRScale;
                     
         hres = S_OK;

#ifndef BUILD_FINAL               
         if (mD3DTexture.getAllocator() == BD3DTexture::cXboxTextureHeap)
            gpXboxTextureHeap->setValleyTextureDetails(gRenderDraw.getResourceAddress(mD3DTexture.getBaseTexture(), false), createParams.mManager, createParams.mName, createParams.mArrayIndex);
#endif            
      }
   }

   if (FAILED(hres))
   {
      gConsoleOutput.output(cMsgError, "BD3DTextureLoader::createFromTextureData: Failed creating D3D resource");

      return false;
   }
   
   if (createParams.mForceSRGBGamma)
   {
      GPUTEXTURE_FETCH_CONSTANT& fc = mD3DTexture.getBaseTexture()->Format;
      fc.SignX = GPUSIGN_GAMMA;
      fc.SignY = GPUSIGN_GAMMA;
      fc.SignZ = GPUSIGN_GAMMA;
   }      

   XGTEXTURE_DESC texDesc;
   XGGetTextureDesc(getBaseTexture(), 0, &texDesc);

   BDDXSerializedTexture serializedTexture(pData, dataLen, textureInfo);      
      
   BDynamicArray<uchar, 16, BDynamicArrayRenderHeapAllocator, BDynamicArrayNoConstructOptions> temp;

   const uint mipLevels = getBaseTexture()->GetLevelCount();

   for (uint faceIndex = 0; faceIndex < serializedTexture.getNumFaces(); faceIndex++)
   {
      for (uint levelIndex = 0; levelIndex < mipLevels; levelIndex++)
      {
         D3DLOCKED_RECT lockedRect;
         Utils::ClearObj(lockedRect);
         
         switch (D3DTextureType)
         {
            case cTT2D:       getTexture()->LockRect(levelIndex, &lockedRect, NULL, 0); break;
            case cTTArray:    getArrayTexture()->LockRect( createParams.mArrayIndex, levelIndex, &lockedRect, NULL, 0); break;
            case cTTCubemap:  getCubeTexture()->LockRect( (D3DCUBEMAP_FACES)faceIndex, levelIndex, &lockedRect, NULL, 0); break;
            default:          BDEBUG_ASSERT(0);
         }

         const uchar* pSurfaceData = serializedTexture.getSurfaceData(faceIndex, levelIndex);
         const int surfaceDataSize = serializedTexture.getSurfaceDataSize(faceIndex, levelIndex);               

         if ((!pSurfaceData) || (-1 == surfaceDataSize))
         {
            switch (D3DTextureType)
            {
               case cTT2D:       getTexture()->UnlockRect(levelIndex); break;
               case cTTArray:    getArrayTexture()->UnlockRect( createParams.mArrayIndex, levelIndex); break;
               case cTTCubemap:  getCubeTexture()->UnlockRect((D3DCUBEMAP_FACES)faceIndex, levelIndex); break;
            }

            getBaseTexture()->Release();

            gConsoleOutput.output(cMsgError, "BD3DTextureLoader::createFromTextureData: Failed getting a ptr to the serialized texture's source data");

            clear();
            return false;
         }

         const int mipWidth       = serializedTexture.getWidth(levelIndex);
         const int mipHeight      = serializedTexture.getHeight(levelIndex);

         int widthInBlocks  = mipWidth;
         int heightInBlocks = mipHeight;
         if (getDDXDataFormatIsDXT(DDXFormat))
         {
            widthInBlocks = Math::Max<int>(1, widthInBlocks >> 2);
            heightInBlocks = Math::Max<int>(1, heightInBlocks >> 2);
         }

         int sourcePitch;
         if (getDDXDataFormatIsDXT(DDXFormat))
            sourcePitch = widthInBlocks * getDDXDataFormatDXTBlockSize(DDXFormat); 
         else
            sourcePitch = serializedTexture.getSurfaceDataSize(faceIndex, levelIndex) / mipHeight;

         if (createParams.mBigEndian)
         {
            temp.resize(surfaceDataSize);
            Utils::FastMemCpy(temp.getPtr(), pSurfaceData, surfaceDataSize);
            pSurfaceData = temp.getPtr();

            if ((getDDXDataFormatIsDXT(DDXFormat)) || (getDDXDataFormatBitsPerPixel(DDXFormat) == 16))
            {                        
               const int numWORDs = surfaceDataSize / (sizeof(WORD));
               EndianSwitchWords(reinterpret_cast<WORD*>(temp.getPtr()), numWORDs);
            }
            else if (getDDXDataFormatBitsPerPixel(DDXFormat) == 32)
            {
               const int numDWORDs = surfaceDataSize / (sizeof(DWORD));
               EndianSwitchDWords(reinterpret_cast<DWORD*>(temp.getPtr()), numDWORDs);
            }
            else if ((getDDXDataFormatBitsPerPixel(DDXFormat) == 64) && (textureInfo.mDataFormat == cDDXDataFormatA16B16G16R16F))
            {
               const int numWORDs = surfaceDataSize / (sizeof(WORD));
               EndianSwitchWords(reinterpret_cast<WORD*>(temp.getPtr()), numWORDs);
            }
            else if (getDDXDataFormatBitsPerPixel(DDXFormat) != 8)
            {
               switch (D3DTextureType)
               {
                  case cTT2D:       getTexture()->UnlockRect(levelIndex); break;
                  case cTTArray:    getArrayTexture()->UnlockRect( createParams.mArrayIndex, levelIndex); break;
                  case cTTCubemap:  getCubeTexture()->UnlockRect((D3DCUBEMAP_FACES)faceIndex, levelIndex); break;
               }

               getBaseTexture()->Release();

               gConsoleOutput.output(cMsgError, "BD3DTextureLoader::createFromTextureData: Unsupported source format's BPP");

               clear();
               return false;   
            }               
         }  

         // Must be EXTREMELY careful here to NOT overwrite any destination bytes which don't correspond to actual texture data!
         if (XGIsTiledFormat(D3DFormat))
         {
            DWORD flags = XGTILE_NONPACKED;
            if (getBaseTexture()->Format.PackedMips)
               flags = 0;

            XGTileTextureLevel(
               texDesc.Width, texDesc.Height, levelIndex, 
               XGGetGpuFormat(D3DFormat), flags, lockedRect.pBits, NULL, pSurfaceData, sourcePitch, NULL);
         }
         else
         {
            uint blockWidth, blockHeight;
            XGGetBlockDimensions(XGGetGpuFormat(D3DFormat), &blockWidth, &blockHeight);

            const uint blocksX = mipWidth / blockWidth;
            const uint blocksY = mipHeight / blockHeight;
            const uint bytesPerBlock = (XGBitsPerPixelFromFormat(D3DFormat) * blockWidth * blockHeight) / 8;

            const uint bytesPerRow = blocksX * bytesPerBlock;
            BDEBUG_ASSERT((uint)lockedRect.Pitch >= bytesPerRow);
            BDEBUG_ASSERT((uint)sourcePitch >= bytesPerRow);

            const uchar* pSrc = reinterpret_cast<const uchar*>(pSurfaceData);
            uchar* pDst = reinterpret_cast<uchar*>(lockedRect.pBits);

            // Must use memcpy() here because copying to write combined memory.
            if ((bytesPerRow == (uint)sourcePitch) && (bytesPerRow == (uint)lockedRect.Pitch))
            {
               memcpy(pDst, pSrc, bytesPerRow * blocksY);
            }
            else
            {
               for (uint row = 0; row < blocksY; row++)
               {
                  memcpy(pDst, pSrc, bytesPerRow);

                  pSrc += sourcePitch;
                  pDst += lockedRect.Pitch;
               }
            }
         }

         switch (D3DTextureType)
         {
            case cTT2D:       getTexture()->UnlockRect(levelIndex); break;
            case cTTArray:    getArrayTexture()->UnlockRect( createParams.mArrayIndex, levelIndex); break;
            case cTTCubemap:  getCubeTexture()->UnlockRect((D3DCUBEMAP_FACES)faceIndex, levelIndex); break;
         }
      }
   }
   
   return true;         
}

#if 0
//============================================================================
// BD3DTextureLoader::determineD3DTextureAllocationSize
//============================================================================
void BD3DTextureLoader::determineD3DTextureAllocationSize(void)
{
   mAllocationSize = 0;
   
   if (!mD3DTexture.getBaseTexture())
      return;
   
   UINT baseSize = 0, mipSize = 0;
   
   // This simulates the allocation of two blocks, one for the base mip, another for the mipchain.
   switch (mD3DTexture.getType())
   {
      case cTT2D:
      {
         IDirect3DTexture9* pTex = mD3DTexture.getTexture();
         
         D3DSURFACE_DESC desc;
         pTex->GetLevelDesc(0, &desc);
         const uint levels = pTex->GetLevelCount();
         
         IDirect3DTexture9 header;
         XGSetTextureHeader(desc.Width, desc.Height, levels, 0, desc.Format, 0, 0, XGHEADER_CONTIGUOUS_MIP_OFFSET, 0, &header, &baseSize, &mipSize);
         
         break;
      }
      case cTTCubemap:
      {
         IDirect3DCubeTexture9* pTex = mD3DTexture.getCubeTexture();
         
         D3DSURFACE_DESC desc;
         pTex->GetLevelDesc(0, &desc);
         const uint levels = pTex->GetLevelCount();
         
         IDirect3DCubeTexture9 header;
         XGSetCubeTextureHeader(desc.Width, levels, 0, desc.Format, 0, 0, XGHEADER_CONTIGUOUS_MIP_OFFSET, &header, &baseSize, &mipSize);
         
         break;
      }
      case cTTArray:
      {
         IDirect3DArrayTexture9* pTex = mD3DTexture.getArrayTexture();
         
         D3DSURFACE_DESC desc;
         pTex->GetLevelDesc(0, &desc);
         const uint levels = pTex->GetLevelCount();
         const uint arraySize = pTex->GetArraySize();
         
         IDirect3DArrayTexture9 header;
         XGSetArrayTextureHeader(desc.Width, desc.Height, arraySize, levels, 0, desc.Format, 0, 0, XGHEADER_CONTIGUOUS_MIP_OFFSET, 0, &header, &baseSize, &mipSize);
                  
         break;
      }
      case cTTVolume:
      {
         IDirect3DVolumeTexture9* pTex = mD3DTexture.getVolumeTexture();
         
         D3DVOLUME_DESC desc;
         pTex->GetLevelDesc(0, &desc);
         const uint levels = pTex->GetLevelCount();
         
         IDirect3DVolumeTexture9 header;
         XGSetVolumeTextureHeader(desc.Width, desc.Height, desc.Depth, levels, 0, desc.Format, 0, 0, XGHEADER_CONTIGUOUS_MIP_OFFSET, &header, &baseSize, &mipSize);
         
         break;
      }      
   }      
   
   mAllocationSize = Utils::AlignUpValue(baseSize, 4096) + Utils::AlignUpValue(mipSize, 4096);
}
#endif

//============================================================================
// BD3DTextureLoader::translateDDXFormatToD3D
//============================================================================
bool BD3DTextureLoader::translateDDXFormatToD3D(D3DFORMAT& format, const BDDXTextureInfo& textureInfo, const BCreateParams& createParams)
{
   bool bigEndian = createParams.mBigEndian;
   bool tiled = createParams.mTiled;
   
   if (textureInfo.mPlatform == cDDXPlatformXbox)
   {
      tiled = true;
      bigEndian = true;
   }
   
   const DWORD bigEndianMask = bigEndian ? UINT_MAX : ~D3DDECLTYPE_ENDIAN_MASK;

   switch (textureInfo.mDataFormat)
   {
      case cDDXDataFormatA16B16G16R16F:
      {
         format = (D3DFORMAT)(D3DFMT_A16B16G16R16F & bigEndianMask); 
         gConsoleOutput.output(cMsgWarning, "BD3DTextureLoader::translateDDXFormatToD3D: Creating 64-bit texture %ix%i", textureInfo.mWidth, textureInfo.mHeight);
         break;
      }
      case cDDXDataFormatA8R8G8B8:     
      {
         format = (D3DFORMAT)(D3DFMT_A8R8G8B8 & bigEndianMask); 
         gConsoleOutput.output(cMsgWarning, "BD3DTextureLoader::translateDDXFormatToD3D: Creating 32-bit texture %ix%i", textureInfo.mWidth, textureInfo.mHeight);
         break;
      }
      case cDDXDataFormatA8B8G8R8:
      {
         format = (D3DFORMAT)(D3DFMT_A8B8G8R8 & bigEndianMask);
         gConsoleOutput.output(cMsgWarning, "BD3DTextureLoader::translateDDXFormatToD3D: Creating 32-bit texture %ix%i", textureInfo.mWidth, textureInfo.mHeight);
         break;
      }
      case cDDXDataFormatA8:        
      {
         format = D3DFMT_A8;
         break;
      }                 
      case cDDXDataFormatDXT1:                              //DXT1 or DXT1A
      {
         format = (D3DFORMAT)(D3DFMT_DXT1 & bigEndianMask);
         break;
      }
      case cDDXDataFormatDXT3:                              //explicit 4-bit alpha
      {
         format = (D3DFORMAT)(D3DFMT_DXT3 & bigEndianMask);
         break;
      }
      case cDDXDataFormatDXT5:                              //block alpha
      case cDDXDataFormatDXT5N:                             //swizzled normal map
      case cDDXDataFormatDXT5Y:                             //luma/chroma DXT5, alpha is in red
      case cDDXDataFormatDXT5H:                             //HDR intensity in alpha
      {
         format = (D3DFORMAT)(D3DFMT_DXT5 & bigEndianMask);
         break;
      }
      case cDDXDataFormatDXN:
      {
         format = (D3DFORMAT)(D3DFMT_DXN & bigEndianMask);
         break;
      }
      default:
      {
         return false;
      }
   }
   
   //#define MAKELINFMT(D3dFmt) ((D3dFmt) & ~D3DFORMAT_TILED_MASK)
   if (!tiled)
      format = (D3DFORMAT)MAKELINFMT(format);

   return true;
}         

//============================================================================
// BD3DTextureLoader::getDesc
//============================================================================
void BD3DTextureLoader::getDesc(D3DSURFACE_DESC& desc) const
{
   BDEBUG_ASSERT(getBaseTexture());
   
   switch (mD3DTexture.getType())
   {
      case cTTCubemap:
      {
         getCubeTexture()->GetLevelDesc(0, &desc);
         break;
      }
      case cTT2D:
      {
         getTexture()->GetLevelDesc(0, &desc);
         break;
      }
      case cTTArray:
      {
         getArrayTexture()->GetLevelDesc(0, &desc);
         break;
      }
      default:
      {
         BDEBUG_ASSERT(0);
      }
   }
}

//============================================================================
// BD3DTextureLoader::getFormat
//============================================================================
D3DFORMAT BD3DTextureLoader::getFormat(void) const
{
   D3DSURFACE_DESC desc;
   getDesc(desc);
   return desc.Format;
}

//============================================================================
// BD3DTextureLoader::getWidth
//============================================================================
uint BD3DTextureLoader::getWidth(void) const
{
   if (!isValid())
      return 0;
      
   D3DSURFACE_DESC desc;
   getDesc(desc);
   return desc.Width;
}

//============================================================================
// BD3DTextureLoader::getHeight
//============================================================================
uint BD3DTextureLoader::getHeight(void) const
{
   if (!isValid())
      return 0;
      
   D3DSURFACE_DESC desc;
   getDesc(desc);
   return desc.Height;
}

//============================================================================
// BD3DTextureLoader::getLevels
//============================================================================
uint BD3DTextureLoader::getLevels(void) const 
{ 
   if (!isValid()) 
      return 0; 
   
   return getBaseTexture()->GetLevelCount(); 
}

//============================================================================
// BD3DTextureLoader::getArraySize
//============================================================================
uint BD3DTextureLoader::getArraySize(void) const
{
   if ((!isValid()) || (cTTArray != mD3DTexture.getType()))
      return 0;
   
   return getArrayTexture()->GetArraySize();
}


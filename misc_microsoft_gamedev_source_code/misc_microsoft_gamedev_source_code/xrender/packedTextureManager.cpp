// File: packedTextureManager.cpp
#include "xrender.h"
#include "packedTextureManager.h"
#include "xboxTextureHeap.h"
#include "renderThread.h"
#include "D3DTextureStats.h"
#include "renderDraw.h"
#include "DXTQUnpack.h"
#include "DDXUnpacker.h"

BPackedTextureManager* gpPackedTextureManager;

BPackedTextureManager::BPackedTextureManager() :
   mInitialized(false),
   mNextFreeSlotIndex(-1),
   mMode(cModeCaching)
{
   mMutex.setSpinCount(2048);
   Utils::ClearObj(mStats);
}

BPackedTextureManager::~BPackedTextureManager()
{

}

void BPackedTextureManager::setMode(eMode mode)
{
   if (mMode != mode)
   {
      mMode = mode;
      
      gRenderThread.blockUntilGPUIdle();
      
      if (mMode == cModeMipsOnly)
         packAll();
      else if (mMode == cModeDisableCaching)
         unpackAll(false);
      else
         mPackedObjectManager.tidy(0, true, true);
   }
}

const char* BPackedTextureManager::getModeDesc(eMode mode) 
{
   switch (mode)
   {
      case cModeCaching:         return "Caching";
      case cModeMipsOnly:        return "Mipmaps Only";
      case cModeDisableCaching:  return "All Maps (Disable Caching)";
   }  
   return "?";
}

void BPackedTextureManager::simInit()
{
   commandListenerInit();
}

void BPackedTextureManager::simDeinit()
{
   commandListenerDeinit();
}

void BPackedTextureManager::init(uint maxTextures, uint baseMipmapCacheSize)
{
   if (mInitialized)
      return;
      
   BScopedCriticalSection lock(mMutex);      
         
   mPackedObjectManager.init(maxTextures, baseMipmapCacheSize, 3, true, true, &gRenderHeap, &gPhysCachedHeap);
   mPackedObjectManager.setOnUnpackCallback(OnUnpackCallback, this);
   mPackedObjectManager.setOnDeleteUnpackedDataCallback(OnDeleteUnpackedDataCallback, this);
   
   mPackedTextures.reserve(maxTextures);
   mPackedTextures.resize(0);
   
   mNextFreeSlotIndex = -1;
   
   Utils::ClearObj(mStats);
   
   mInitialized = true;
}

void BPackedTextureManager::deinit()
{
   if (!mInitialized)
      return;
      
   BScopedCriticalSection lock(mMutex);
      
   gRenderThread.blockUntilGPUIdle();
      
   for (uint i = 0; i < mPackedTextures.getSize(); i++)
   {  
      BPackedTexture& packedTexture = mPackedTextures[i];
      if (packedTexture.mSlotFree)
         continue;
            
      bool success = mPackedObjectManager.free(packedTexture.mBaseData);
      BVERIFY(success);
            
      success = gpXboxTextureHeap->releaseValley(packedTexture.mpMipData);
      BVERIFY(success);
      packedTexture.mpMipData = NULL;
   }
   
   mPackedTextures.clear();
      
   mPackedObjectManager.deinit();
   
   mNextFreeSlotIndex = -1;
   
   mInitialized = false;
}

bool BPackedTextureManager::create(
   BD3DTexture& d3dTexture,
   const void* pFileData, uint fileDataSize,
   bool addUnusedRegionsToHeap, bool longTermAllocation, bool forceSRGBGamma,
   const char* pManager, const char* pName, 
   uint* pAllocationSize, 
   BDDXTextureInfo* pDDXTextureInfo,
   BDDXTextureInfo* pUnpackedTextureInfo)
{
   if (pAllocationSize) 
      *pAllocationSize = 0;
      
   BDEBUG_ASSERT(pFileData && fileDataSize);
   
   const BECFFileReader ecfFileReader(BConstDataBuffer(pFileData, fileDataSize));
   
   BDDXHeader ddxHeader;
   BDDXTextureInfo ddxTextureInfo;
         
   BDDXUnpacker ddxUnpacker;
   ddxUnpacker;
   if (!ddxUnpacker.getTextureInfo(ecfFileReader, true, ddxHeader, ddxTextureInfo))
      return false;
   
   if (!getDDXDataFormatIsDXTQ(ddxHeader.mDataFormat))
      return false;
   
   if (!ddxHeader.mMipChainSize)
      return false;
      
   const BYTE* pMip0Data = NULL;
   uint mip0DataSize = 0;
   const int mip0ChunkIndex = ecfFileReader.findChunkByID(cDDX_ECF_MIP0_CHUNK_ID);
   if (mip0ChunkIndex >= 0)
   {
      pMip0Data = ecfFileReader.getChunkDataByIndex(mip0ChunkIndex);
      mip0DataSize = ecfFileReader.getChunkDataLenByIndex(mip0ChunkIndex);
   }
   if (!pMip0Data)
      return false;

   BDDXTextureInfo unpackedTextureInfo;
   
   IDirect3DBaseTexture9* pBaseTex = new IDirect3DBaseTexture9;
      
   uint numBaseBlocks, numMipChainBlocks, bytesPerBlock;

   BDXTQUnpack dxtqUnpacker;
   dxtqUnpacker;
   if (!dxtqUnpacker.getDXTQInfo(ddxTextureInfo, pMip0Data, mip0DataSize, 
      unpackedTextureInfo, (IDirect3DTexture9*)pBaseTex, numBaseBlocks, numMipChainBlocks, bytesPerBlock))
   {
      delete pBaseTex;      
      return false;
   }
   
   if (!numMipChainBlocks)
   {
      delete pBaseTex;
      return false;  
   }
      
   pBaseTex->Common |= D3DCOMMON_CPU_CACHED_MEMORY;
   
   if (forceSRGBGamma)
   {
      GPUTEXTURE_FETCH_CONSTANT& fc = pBaseTex->Format;
      fc.SignX = GPUSIGN_GAMMA;
      fc.SignY = GPUSIGN_GAMMA;
      fc.SignZ = GPUSIGN_GAMMA;
   }
   
   BDynamicRenderArray<BYTE> mipData;
   mipData.resize(bytesPerBlock * numMipChainBlocks);
   
   if (!dxtqUnpacker.unpackDXTQToTiledDXT(ddxTextureInfo.mDataFormat, pMip0Data, mip0DataSize, mipData.getPtr(), mipData.getSize(), false, true))
   {
      delete pBaseTex;
      return false;
   }
         
   void* pPackedBaseData;
   uint packedBaseDataSize;
   if (!dxtqUnpacker.unpackDXTQBasePrep(ddxTextureInfo.mDataFormat, pMip0Data, mip0DataSize, pPackedBaseData, packedBaseDataSize, &gRenderHeap))
   {
      delete pBaseTex;
      return false;
   }
   
   if (!create(pBaseTex, mipData.getPtr(), mipData.getSize(), pPackedBaseData, packedBaseDataSize, addUnusedRegionsToHeap, longTermAllocation, pManager, pName, pAllocationSize))
   {
      gRenderHeap.Delete(pPackedBaseData);
      delete pBaseTex;
      return false;
   }
   
   d3dTexture.setTexture(reinterpret_cast<IDirect3DTexture9*>(pBaseTex), BD3DTexture::cPackedTextureManager);
   
   if (pDDXTextureInfo) *pDDXTextureInfo = ddxTextureInfo;
   
   if (pUnpackedTextureInfo) *pUnpackedTextureInfo = unpackedTextureInfo;
      
   return true;
}            

bool BPackedTextureManager::UnpackDXTQBaseCallback(void* pContext, const void* pPackedData, uint packedDataSize, void* pDst, uint dstSize)
{
   if (!pDst)
   {
      gRenderHeap.Delete((void*)pPackedData);
      return true;
   }
   
   BDXTQUnpack unpacker;
   unpacker;
   return unpacker.unpackDXTQBase(pPackedData, packedDataSize, (BYTE*)pDst, dstSize);
}

bool BPackedTextureManager::create(
   D3DBaseTexture* pBaseTex,
   const uchar* pXboxData, uint xboxDataSize,
   const void* pPackedBaseData, uint packedBaseDataSize,
   bool addUnusedRegionsToHeap, bool longTermAllocation,
   const char* pManager, const char* pName, uint* pAllocationSize)
{
   if (pAllocationSize) 
      *pAllocationSize = 0;
   
   if ( (pBaseTex->Common & D3DCOMMON_D3DCREATED) || ((pBaseTex->Common & D3DCOMMON_CPU_CACHED_MEMORY) == 0) )
      return false;
   
   XGTEXTURE_DESC desc;
   XGGetTextureDesc(pBaseTex, 0, &desc);
   
   enum { cMaxBaseRegions = 128, cMaxMipRegions = 128 };
   XGLAYOUT_REGION baseRegions[cMaxBaseRegions];
   UINT baseRegionCount = cMaxBaseRegions;
   XGLAYOUT_REGION mipRegions[cMaxMipRegions];
   UINT mipRegionCount = cMaxMipRegions;
   UINT baseDataOfs = 0;
   UINT baseSize = 0;
   UINT mipDataOfs = 0;
   UINT mipSize = 0;
   BRenderDraw::getTextureLayout(pBaseTex, &baseDataOfs, &baseSize, baseRegions, &baseRegionCount, 16, &mipDataOfs, &mipSize, mipRegions, &mipRegionCount, 16);
            
   if (!mipRegionCount)
      return false;
   
   if (baseSize >= mPackedObjectManager.getTargetCacheSize())
      return false;

   uint mipAllocSize = 0;
   uint mipAllocUsedSize = 0;
   void* pMipData = gpXboxTextureHeap->getValley(pBaseTex, &mipAllocSize, addUnusedRegionsToHeap, longTermAllocation, true, &mipAllocUsedSize);
   if (!pMipData)
      return false;
   
   if (pAllocationSize) 
      *pAllocationSize = mipAllocUsedSize;
      
#ifndef BUILD_FINAL      
   gpXboxTextureHeap->setValleyTextureDetails(pMipData, pManager, pName, 0);
#endif   

   BScopedCriticalSection lock(mMutex);      
   
   BPackedPtrBase baseData;

   uint origMipOffset = 0;
         
   if (!pPackedBaseData)
   {
      BPackedObjectManager::ePackerType packerType = BPackedObjectManager::cPTGeneralPurpose;

      switch (MAKENOSRGBFMT(MAKELEFMT(desc.Format)))
      {
         case MAKELEFMT(D3DFMT_DXT1):              
         case MAKELEFMT(D3DFMT_LIN_DXT1):          packerType = BPackedObjectManager::cPTDXT1; break;
         
         case MAKELEFMT(D3DFMT_DXT5):              
         case MAKELEFMT(D3DFMT_LIN_DXT5):          packerType = BPackedObjectManager::cPTDXT5; break;
         
         case MAKELEFMT(D3DFMT_LIN_DXN):           
         case MAKELEFMT(D3DFMT_DXN):               packerType = BPackedObjectManager::cPTDXN; break;
      }
     
      bool success = mPackedObjectManager.alloc(baseData, baseSize, 1, 1U << GPU_TEXTURE_ADDRESS_SHIFT, packerType);
      if (!success)
      {
         gpXboxTextureHeap->releaseValley(pMipData);
         return false;
      }
      
      memcpy(mPackedObjectManager.getPtr(baseData), pXboxData, baseSize);
                  
      origMipOffset = pBaseTex->Format.MipAddress << GPU_TEXTURE_ADDRESS_SHIFT;
      BDEBUG_ASSERT(origMipOffset >= baseSize);
   }
   else
   {
      bool success = mPackedObjectManager.alloc(baseData, baseSize, 1, 1U << GPU_TEXTURE_ADDRESS_SHIFT, BPackedObjectManager::cPTCustom, UnpackDXTQBaseCallback, this, pPackedBaseData, packedBaseDataSize);
      if (!success)
      {
         gpXboxTextureHeap->releaseValley(pMipData);
         return false;
      }
   }      
      
   pBaseTex->Format.MipAddress = ((uint)pMipData) >> GPU_TEXTURE_ADDRESS_SHIFT;
   pBaseTex->Format.BaseAddress = pBaseTex->Format.MipAddress;
   pBaseTex->Format.MinMipLevel = 1;
   
   BDEBUG_ASSERT(mipAllocSize >= mipSize);
   for (uint i = 0; i < mipRegionCount; i++)
      memcpy((uchar*)pMipData + mipRegions[i].StartOffset, pXboxData + origMipOffset + mipRegions[i].StartOffset, mipRegions[i].EndOffset - mipRegions[i].StartOffset);
   
   Utils::FlushCacheLines(pMipData, mipAllocSize);
                 
   uint slotIndex;
   if (mNextFreeSlotIndex != -1)
   {
      slotIndex = mNextFreeSlotIndex;
      BDEBUG_ASSERT(mPackedTextures[slotIndex].mSlotFree);
      mNextFreeSlotIndex = mPackedTextures[slotIndex].mNextFreeSlotIndex;
   }
   else
   {
      if (mPackedTextures.getSize() == mPackedTextures.getCapacity())
      {
         mPackedObjectManager.free(baseData);
         gpXboxTextureHeap->releaseValley(pMipData);
         return false;
      }
      slotIndex = mPackedTextures.getSize();
      mPackedTextures.enlarge(1);
   }
   
   mPackedObjectManager.setUserData(baseData, slotIndex);
   
   BPackedTexture& packedTexture = mPackedTextures[slotIndex];
   packedTexture.clear();
         
   packedTexture.mpTex = pBaseTex;
   packedTexture.mBaseData = baseData;
   packedTexture.mpMipData = pMipData;
   packedTexture.mLastWorkerFrameUsed = UINT32_MAX;//gRenderThread.getCurWorkerFrameNoBarrier();
   packedTexture.mLastFramePreferedBaseMipmaps = false;
   packedTexture.mMipAllocSize = mipAllocSize;
   packedTexture.mMipAllocUsedSize = mipAllocUsedSize;
   
   BDEBUG_ASSERT((slotIndex + 1) <= 0xFFFF);
      
   pBaseTex->SetIdentifier((pBaseTex->GetIdentifier() & 0xFFFF0000) | (slotIndex + 1));

   //trace("Width: %u, Height: %u, Levels: %u, Format: %s", desc.Width, desc.Height, pBaseTex->GetLevelCount(), getD3DTexFormatString(desc.Format));
      
   if (!pPackedBaseData)
      mPackedObjectManager.pack(baseData);
      
   mPackedObjectManager.setReadOnly(baseData);

   if (pAllocationSize) 
      *pAllocationSize += mPackedObjectManager.getPackedSize(baseData);
      
   if ((!pPackedBaseData) && (mPackedObjectManager.getPackedSize(baseData) < 512))
      mPackedObjectManager.setPackedDataPersisted(baseData);
            
   mStats.mTotalMipAllocBytes += mipAllocSize;
   mStats.mTotalMipAllocUsedBytes += mipAllocUsedSize;

   return true;
}            

bool BPackedTextureManager::release(D3DBaseTexture* pTex)
{
   if (!pTex)
      return false;
   
   if ((pTex->Common & D3DCOMMON_D3DCREATED) || ((pTex->Common & D3DCOMMON_CPU_CACHED_MEMORY) == 0))
      return false;

   BScopedCriticalSection lock(mMutex);      
      
   const int slotIndex = (pTex->GetIdentifier() & 0xFFFF) - 1;
   if ((slotIndex < 0) || (slotIndex >= (int)mPackedTextures.getSize()))
      return false;

   BPackedTexture& packedTexture = mPackedTextures[slotIndex];
   if ((packedTexture.mSlotFree) || (pTex != packedTexture.mpTex))
      return false;
      
   BDEBUG_ASSERT(mStats.mTotalMipAllocBytes >= packedTexture.mMipAllocSize);
   BDEBUG_ASSERT(mStats.mTotalMipAllocUsedBytes >= packedTexture.mMipAllocUsedSize);
   mStats.mTotalMipAllocBytes -= packedTexture.mMipAllocSize;
   mStats.mTotalMipAllocUsedBytes -= packedTexture.mMipAllocUsedSize;
   
   bool success = gpXboxTextureHeap->releaseValley(packedTexture.mpMipData);
   BVERIFY(success);
   
   success = mPackedObjectManager.free(packedTexture.mBaseData);
   BVERIFY(success);
   
   packedTexture.clear();
   packedTexture.mSlotFree = true;
   packedTexture.mNextFreeSlotIndex = mNextFreeSlotIndex;
   mNextFreeSlotIndex = slotIndex;
   
   return true;
}

bool BPackedTextureManager::touch(const BTextureTouchInfo* pTexturesToTouch, uint numTextures)
{
   if (!mInitialized)
      return false;

   BScopedCriticalSection lock(mMutex);         
   
   if (!numTextures)
      return true;
      
   BDynamicRenderArray<BPackedTexture*> newBaseTextures;
   BDynamicRenderArray<BPackedTexture*> touchedTextures;
   
   const uint curWorkerFrame = gRenderThread.getCurWorkerFrameNoBarrier();

   mStats.mTotalPrefersBaseTextures = 0;
   mStats.mTotalPrefersBaseBytes = 0;
   mStats.mTotalImportantBaseReuses = 0;
   mStats.mTotalDesiredNewBaseTextures = 0;
   mStats.mTotalNotPrefersBaseTextures = 0;
   mStats.mTotalBaseTexturesStillNeeded = 0;
   mStats.mTotalBaseBytesStillNeeded = 0;
   mStats.mTotalUnpackedTexturesForcedToUseMips = 0;
   mStats.mTotalUnimportantBaseReuses = 0;
      
   uint totalNewBaseBytes = 0;
   // From least to most important
   for (int i = numTextures - 1; i >= 0; i--)
   {
      D3DBaseTexture* pTex = pTexturesToTouch[i].mpTex;
      const bool prefersBaseMipmap = pTexturesToTouch[i].mPrefersBaseMipmap;
      
      if ((pTex->Common & D3DCOMMON_D3DCREATED) || ((pTex->Common & D3DCOMMON_CPU_CACHED_MEMORY) == 0))
         return false;

      const int slotIndex = (pTex->GetIdentifier() & 0xFFFF) - 1;
      if ((slotIndex < 0) || (slotIndex >= (int)mPackedTextures.getSize()))
         return false;

      BPackedTexture& packedTexture = mPackedTextures[slotIndex];
      if ((packedTexture.mSlotFree) || (pTex != packedTexture.mpTex))
         return false;
      
      packedTexture.mLastWorkerFrameUsed = curWorkerFrame;
      packedTexture.mLastFramePreferedBaseMipmaps = prefersBaseMipmap;
      
      const uint unpackedSize = mPackedObjectManager.getUnpackedSize(packedTexture.mBaseData);
      
      if (mMode == cModeDisableCaching)
      {
         mPackedObjectManager.unpack(packedTexture.mBaseData, false);
         
         packedTexture.mpTex->Format.MinMipLevel = 0;
      }
      else if (mMode == cModeMipsOnly)
      {
         packedTexture.mpTex->Format.MinMipLevel = 1;
      }
      else
      {
         if (prefersBaseMipmap)
         {
            mStats.mTotalPrefersBaseTextures++;
            mStats.mTotalPrefersBaseBytes += unpackedSize;
            
            if (mPackedObjectManager.isUnpacked(packedTexture.mBaseData))
            {
               packedTexture.mpTex->Format.MinMipLevel = 0;
               
               mPackedObjectManager.markAsUsed(packedTexture.mBaseData, false);
               
               mStats.mTotalImportantBaseReuses++;
            }
            else
            {
               newBaseTextures.pushBack(&packedTexture);
            
               totalNewBaseBytes += unpackedSize;
               
               mStats.mTotalDesiredNewBaseTextures++;
            }
         }
         else
         {
            mStats.mTotalNotPrefersBaseTextures++;
           
            touchedTextures.pushBack(&packedTexture);
         }
      }            
   }
   
   if (mMode == cModeCaching)
   {
      mStats.mTotalDesiredNewBaseBytes = totalNewBaseBytes;
      
      if (totalNewBaseBytes)
         mPackedObjectManager.tidy(totalNewBaseBytes);
    
      int totalBaseBytesStillNeeded = 0;
                  
      // From most to least important
      for (int i = newBaseTextures.getSize() - 1; i >= 0; i--)
      {
         BPackedTexture& packedTexture = *newBaseTextures[i];

         if (!mPackedObjectManager.unpack(packedTexture.mBaseData))
         {
            totalBaseBytesStillNeeded += mPackedObjectManager.getUnpackedSize(packedTexture.mBaseData);
            mStats.mTotalBaseTexturesStillNeeded++;
         }
      }
      
      mStats.mTotalBaseBytesStillNeeded += totalBaseBytesStillNeeded;
                  
      // From least to most important         
      for (uint i = 0; i < touchedTextures.getSize(); i++)
      {
         BPackedTexture& packedTexture = *touchedTextures[i];
      
         if (!mPackedObjectManager.isUnpacked(packedTexture.mBaseData))
            continue;
            
         if (totalBaseBytesStillNeeded > 0)
         {
            packedTexture.mpTex->Format.MinMipLevel = 1;
            
            totalBaseBytesStillNeeded -= mPackedObjectManager.getUnpackedSize(packedTexture.mBaseData);
            
            mStats.mTotalUnpackedTexturesForcedToUseMips++;
         }
         else
         {
            packedTexture.mpTex->Format.MinMipLevel = 0;
            
            mPackedObjectManager.markAsUsed(packedTexture.mBaseData, true);
            
            mStats.mTotalUnimportantBaseReuses++;
         }
      }
   }      
   
   return true;
}

bool BPackedTextureManager::isPackedTexture(const D3DBaseTexture* pTex)
{
   if (!pTex)
      return false;

   if ((pTex->Common & D3DCOMMON_D3DCREATED) || ((pTex->Common & D3DCOMMON_CPU_CACHED_MEMORY) == 0))
      return false;

   const int slotIndex = (((D3DBaseTexture*)pTex)->GetIdentifier() & 0xFFFF) - 1;
   
   BScopedCriticalSection lock(mMutex);      
   
   if ((slotIndex < 0) || (slotIndex >= (int)mPackedTextures.getSize()))
      return false;

//-- FIXING PREFIX BUG ID 7136
   const BPackedTexture& packedTexture = mPackedTextures[slotIndex];
//--
   if ((packedTexture.mSlotFree) || (pTex != packedTexture.mpTex))
      return false;
      
   return true;      
}

bool BPackedTextureManager::isBaseUnpacked(const D3DBaseTexture* pTex)
{
   if (!pTex)
      return false;

   if ((pTex->Common & D3DCOMMON_D3DCREATED) || ((pTex->Common & D3DCOMMON_CPU_CACHED_MEMORY) == 0))
      return false;

   const int slotIndex = (((D3DBaseTexture*)pTex)->GetIdentifier() & 0xFFFF) - 1;
   
   BScopedCriticalSection lock(mMutex);      
   
   if ((slotIndex < 0) || (slotIndex >= (int)mPackedTextures.getSize()))
      return false;

//-- FIXING PREFIX BUG ID 7137
   const BPackedTexture& packedTexture = mPackedTextures[slotIndex];
//--
   if ((packedTexture.mSlotFree) || (pTex != packedTexture.mpTex))
      return false;
      
   if (mPackedObjectManager.isUnpacked(packedTexture.mBaseData))      
      return true;
   
   return false;
}

void BPackedTextureManager::advanceFrame()
{
   if (!mInitialized)
      return;

   BScopedCriticalSection lock(mMutex);            
   
   bool targetCacheSizeEnforcement = true;
   if (mMode == cModeDisableCaching)
      targetCacheSizeEnforcement = false;
      
   mPackedObjectManager.advanceFrame(0, true, targetCacheSizeEnforcement);
}

void BPackedTextureManager::unpackAll(bool targetCacheSizeEnforcement)
{
   if (!mInitialized)
      return;

   BScopedCriticalSection lock(mMutex);            
   
   mPackedObjectManager.unpackAll(targetCacheSizeEnforcement);
}

void BPackedTextureManager::packAll()
{
   if (!mInitialized)
      return;

   BScopedCriticalSection lock(mMutex);            
   
   mPackedObjectManager.packAll();
}

void BPackedTextureManager::OnUnpackCallback(BPackedObjectManager* pManager, BPackedPtrBase ptr, void* pData, uint dataSize, void* pPrivateData)
{
   BPackedTextureManager& manager = *static_cast<BPackedTextureManager*>(pPrivateData);
   BDEBUG_ASSERT(manager.mInitialized);
   
   Utils::FlushCacheLines(pData, dataSize);
   
   const uint slotIndex = manager.mPackedObjectManager.getUserData(ptr);
      
   BPackedTexture& packedTexture = manager.mPackedTextures[slotIndex];  
   BDEBUG_ASSERT(!packedTexture.mSlotFree && packedTexture.mpTex && (packedTexture.mBaseData == ptr));
   
   packedTexture.mpTex->Format.BaseAddress = ((uint)pData) >> GPU_TEXTURE_ADDRESS_SHIFT;
   packedTexture.mpTex->Format.MinMipLevel = 0;
}

void BPackedTextureManager::OnDeleteUnpackedDataCallback(BPackedObjectManager* pManager, BPackedPtrBase ptr, void* pData, uint dataSize, void* pPrivateData)
{
   BPackedTextureManager& manager = *static_cast<BPackedTextureManager*>(pPrivateData);
   BDEBUG_ASSERT(manager.mInitialized);
   
   const uint slotIndex = manager.mPackedObjectManager.getUserData(ptr);
   
   BPackedTexture& packedTexture = manager.mPackedTextures[slotIndex];  
   BDEBUG_ASSERT(!packedTexture.mSlotFree && packedTexture.mpTex && (packedTexture.mBaseData == ptr));   
   
   if (packedTexture.mLastWorkerFrameUsed != UINT32_MAX)
   {
      if (gEventDispatcher.getThreadIndex() == cThreadIndexRender)
         gRenderThread.blockOnGPUFrameOrIdle(packedTexture.mLastWorkerFrameUsed);
      else
      {
         // Crap - we need to delete the texture's unpacked data, but we can't safely wait for the GPU to finish using it!
      }
   }
   
   packedTexture.mpTex->Format.BaseAddress = packedTexture.mpTex->Format.MipAddress;
   packedTexture.mpTex->Format.MinMipLevel = 1;
}

void BPackedTextureManager::initDeviceData(void)
{
}

void BPackedTextureManager::frameBegin(void)
{
}

void BPackedTextureManager::processCommand(const BRenderCommandHeader& header, const uchar* pData)
{
}

void BPackedTextureManager::frameEnd(void)
{
   advanceFrame();

#ifndef BUILD_FINAL   
   static bool runCheck;
   if (runCheck)
   {
      bool success = mPackedObjectManager.check();
      if (!success) DebugBreak();
   }
#endif   
}

void BPackedTextureManager::deinitDeviceData(void)
{
}







// File: packedTextureManager.h
#pragma once 
#include "packedObjectManager.h"
#include "D3DTexture.h"
#include "DDXDef.h"
#include "DDXPackParams.h"
#include "renderThread.h"

class BPackedTextureManager : public BRenderCommandListener
{
public:
   BPackedTextureManager();
   ~BPackedTextureManager();
   
   void simInit();
   void simDeinit();
   
   void init(uint maxTextures, uint baseMipmapCacheSize);
   void deinit();
   
   bool getInitialized() const { return mInitialized; }
   
   enum eMode
   {
      cModeCaching,
      cModeMipsOnly,
      cModeDisableCaching,
      
      cModeNum
   };
   void setMode(eMode mode);
   eMode getMode() { return mMode; }
   static const char* getModeDesc(eMode mode);
      
   bool create(
      D3DBaseTexture* pBaseTex,
      const uchar* pXboxData, uint xboxDataSize,
      const void* pPackedBaseData, uint packedBaseDataSize,
      bool addUnusedRegionsToHeap, bool longTermAllocation,
      const char* pManager, const char* pName, 
      uint* pAllocationSize = NULL);
            
   bool create(
      BD3DTexture& d3dTexture,
      const void* pFileData, uint fileDataSize,
      bool addUnusedRegionsToHeap, 
      bool longTermAllocation,
      bool forceSRGBGamma,
      const char* pManager, const char* pName, 
      uint* pAllocationSize = NULL,
      BDDXTextureInfo* pDDXTextureInfo = NULL,
      BDDXTextureInfo* pUnpackedTextureInfo = NULL);
   
   bool release(D3DBaseTexture* pTex);
   
   struct BTextureTouchInfo
   {
      BTextureTouchInfo() { }
      BTextureTouchInfo(D3DBaseTexture* pTex, uint projectedArea, bool prefersBaseMipmap) : mpTex(pTex), mProjectedArea(projectedArea), mPrefersBaseMipmap(prefersBaseMipmap) { }
      
      D3DBaseTexture*   mpTex;
      uint              mProjectedArea;
      bool              mPrefersBaseMipmap;
      
      bool operator<(const BTextureTouchInfo& rhs) const
      {
         if (mPrefersBaseMipmap == rhs.mPrefersBaseMipmap)
         {
            if (mProjectedArea == rhs.mProjectedArea)
               return mpTex > rhs.mpTex;
            else
               return mProjectedArea > rhs.mProjectedArea;
         }
         else 
            return ((uint)mPrefersBaseMipmap > (uint)rhs.mPrefersBaseMipmap);
      }
   };
   bool touch(const BTextureTouchInfo* pTexturesToTouch, uint numTextures);
   
   bool isPackedTexture(const D3DBaseTexture* pTex);
   bool isBaseUnpacked(const D3DBaseTexture* pTex);
         
   void unpackAll(bool targetCacheSizeEnforcement);
   void packAll();
         
   BPackedObjectManager& getPackedObjectManager() { return mPackedObjectManager; }
   
   struct BStats 
   {
      uint                                mTotalMipAllocBytes;
      uint                                mTotalMipAllocUsedBytes; 
      uint                                mTotalPrefersBaseTextures;
      uint                                mTotalPrefersBaseBytes;
      uint                                mTotalNotPrefersBaseTextures;
      uint                                mTotalImportantBaseReuses;
      uint                                mTotalDesiredNewBaseTextures;
      uint                                mTotalDesiredNewBaseBytes;
      uint                                mTotalBaseTexturesStillNeeded;
      uint                                mTotalBaseBytesStillNeeded;
      uint                                mTotalUnpackedTexturesForcedToUseMips;
      uint                                mTotalUnimportantBaseReuses;
   };
   
   void getStats(BStats& stats) { stats = mStats; }
   
private:
   struct BPackedTexture
   {
      union
      {
         uint                 mLastWorkerFrameUsed;
         int                  mNextFreeSlotIndex;
      };
      
      D3DBaseTexture*         mpTex;
      
      BPackedPtrBase          mBaseData;
      
      void*                   mpMipData;
      uint                    mMipAllocSize;
      uint                    mMipAllocUsedSize;
                              
      bool                    mLastFramePreferedBaseMipmaps : 1;
      bool                    mSlotFree : 1;
      
      void clear() { Utils::ClearObj(*this); }
   };
   
   BPackedObjectManager                mPackedObjectManager;
   
   BDynamicRenderArray<BPackedTexture> mPackedTextures;
   
   int                                 mNextFreeSlotIndex;
      
   BStats                              mStats;      
      
   BCriticalSection                    mMutex;
   
   eMode                               mMode;
   
   bool                                mInitialized : 1;
      
   void advanceFrame();
   
   virtual void initDeviceData(void);
   virtual void frameBegin(void);
   virtual void processCommand(const BRenderCommandHeader& header, const uchar* pData);
   virtual void frameEnd(void);
   virtual void deinitDeviceData(void);
   
   static void OnUnpackCallback(BPackedObjectManager* pManager, BPackedPtrBase ptr, void* pData, uint dataSize, void* pPrivateData);
   static void OnDeleteUnpackedDataCallback(BPackedObjectManager* pManager, BPackedPtrBase ptr, void* pData, uint dataSize, void* pPrivateData);
   static bool UnpackDXTQBaseCallback(void* pContext, const void* pPackedData, uint packedDataSize, void* pDst, uint dstSize);
};

extern BPackedTextureManager* gpPackedTextureManager;

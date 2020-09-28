//============================================================================
//
//  D3DTextureManager.h
//  
//  Copyright (c) 2006-2007, Ensemble Studios
//
//============================================================================
#pragma once

#include "D3DTexture.h"
#include "renderThread.h"
#include "D3DTextureStats.h"

class BD3DTextureLoader;
enum eDDXDataFormat;

typedef uint64 BManagedTextureHandle;
const BManagedTextureHandle cInvalidManagedTextureHandle = (BManagedTextureHandle)0;

// Except for the handle based API, this is a render thread only interface!
class BD3DTextureManager : public BEventReceiver, BRenderCommandListener
{
public:
   BD3DTextureManager();
   ~BD3DTextureManager();
   
   void init(long baseDirID, bool backgroundLoading);
   void deinit();
   
   bool getBackgroundLoadingEnabled() const { return mBackgroundLoading; }
         
   // Returns true if texture exists.
   bool getActualFilename(const char* pFilename, BRenderString& actualFilename);
   
   typedef BDynamicRenderArray<BRenderString> BRenderStringArray;
   
   class BManagedTexture
   {
      friend class BD3DTextureManager;
      
   public:
      BManagedTexture();
      ~BManagedTexture();
      
      void setTextureManager(BD3DTextureManager* pTextureManager) { mpTextureManager = pTextureManager; }
      BD3DTextureManager* getTextureManager() const { return mpTextureManager; }
      
      enum eStatus
      {
         cStatusInvalid,
         cStatusInitialized,
         cStatusLoadingInBackground,
         cStatusLoaded,
         cStatusLoadFailed,
                           
         cStatusTotal
      };
      
      eStatus getStatus() const { return mStatus; }
      
      void init(
         long dirID, const BRenderStringArray& filenames, 
         BFileOpenFlags fileOpenFlags, 
         DWORD membershipBits, 
         bool srgbTexture, 
         eDefaultTexture defaultTexture, 
         bool usePackedTextureManager,
         const char* pManagerName,
         bool backgroundLoadable);
         
      void deinit();
      
      bool load();
      void unload();
      bool reload();
      
      void addRef();
      void release();
      uint getRefCount() const { return mRefCount; }
            
      BRenderString& getIdentifier(BRenderString& identifier) const;
      
      const BRenderStringArray& getFilenames() const { return mFilenames; }
      BFileOpenFlags getFileOpenFlags() const { return mFileOpenFlags; }
      DWORD getMembershipBits() const { return mMembershipBits; }
      
      const BRenderString& getManagerName() const { return mManagerName; }
                  
      const BD3DTexture& getD3DTexture() const;
      float getHDRScale() const { return mHDRScale; }
      const BSmallDynamicRenderArray<float>& getArrayHDRScale() const { return mArrayHDRScale; }
      
      uint getWidth() const { return mWidth; }
      uint getHeight() const { return mHeight; }
      uint getLevels() const { return mLevels; }
      uint getArraySize() const { return mArraySize; }
      eDDXDataFormat getDDXFormat() const { return mDDXFormat; }
      
      typedef void (*BReloadListenerFuncPtr)(BManagedTexture* pTex, uint64 privateData);
      
      void addReloadListener(BReloadListenerFuncPtr pFunc, uint64 privateData);
      bool removeReloadListener(BReloadListenerFuncPtr pFunc, uint64 privateData);
      void clearReloadListeners();
      
      // Returns the actual amount of memory allocated to hold the texture data, for statistics purposes.
      uint getActualAllocationSize() const { return mActualAllocationSize; }
                  
   private:
      BD3DTexture       mTexture;
      float             mHDRScale;
      BSmallDynamicRenderArray<float> mArrayHDRScale;
      
      BD3DTextureManager* mpTextureManager;
      
      struct BReloadListener
      {
         BReloadListener() { }
         BReloadListener(BReloadListenerFuncPtr pFunc, uint64 privateData) : mpFunc(pFunc), mPrivateData(privateData) { }
         
         BReloadListenerFuncPtr  mpFunc;
         uint64                  mPrivateData;
         
         bool operator== (const BReloadListener& other) const { return (mpFunc == other.mpFunc) && (mPrivateData == other.mPrivateData); }
      };
                  
      BSmallDynamicRenderArray<BReloadListener> mReloadListeners;
      
      struct BBackgroundLoadRequestPacket
      {
         BBackgroundLoadRequestPacket();
         ~BBackgroundLoadRequestPacket();
                        
         BD3DTextureManager*  mpTextureManager;
         BManagedTexture*     mpManagedTexture;
         BD3DTextureLoader*   mpTextureLoader;
                  
         long                 mDirID;                  
         BRenderStringArray   mFilenames;
                  
         BFileOpenFlags       mFileOpenFlags;
         uint                 mBackgroundLoadRequestIndex;
         
         BRenderString        mManagerName;
         
         bool                 mSRGBTexture : 1;
         bool                 mUsePackedTextureManager : 1;
         bool                 mStatus : 1;
      };
      
      static bool loadTexture(
         BD3DTextureLoader& textureLoader,
         long dirID,
         const BRenderStringArray& filenames, 
         bool srgbTexture,
         bool usePackedTextureManager,
         BFileOpenFlags fileOpenFlags,
         const BRenderString& managerName);
            
      void finalizeBackgroundTextureLoad(BBackgroundLoadRequestPacket* pPacket);
                  
      void finalizeTextureLoad(BD3DTextureLoader& textureLoader);
      
      static void handleBackgroundTextureLoad(void* pPacket);
      static void backgroundLoadCallbackFunc(void* privateData0, uint64 privateData1, uint workBucketIndex, bool lastWorkEntryInBucket);
      
      uint              mRefCount;

      D3DFORMAT         mD3DFormat;
      
      uint16            mWidth;
      uint16            mHeight;
      uint8             mLevels;
      uint8             mArraySize;

      eDDXDataFormat    mDDXFormat;
      
      long              mDirID;            
      BRenderStringArray mFilenames;
      BRenderString     mManagerName;
      BFileOpenFlags    mFileOpenFlags;
      DWORD             mMembershipBits;
      uint              mActualAllocationSize;
                  
      uint              mBackgroundLoadRequestIndex;
                                   
      eDefaultTexture   mDefaultTexture;
      eStatus           mStatus;
      
      bool              mSRGBTexture : 1;
      bool              mUsePackedTextureManager : 1;
      bool              mBackgroundLoadable : 1;
            
      void releaseTexture();
      
   };
   
   BManagedTexture* getDefaultTexture(eDefaultTexture defaultTexture);
   bool isDefaultTexture(const BManagedTexture* pTexture) const;
   
   IDirect3DBaseTexture9* getDefaultD3DTexture(eDefaultTexture index) { return getDefaultTexture(index)->getD3DTexture().getBaseTexture(); }

   enum eTextureBucketBitMasks
   {
      cSystem           = 0,
      cUGXCommon        = 1,
      cUGXMaterial      = 2,
      cScaleformCommon  = 4,
      cScaleformPreGame = 8,
      cScaleformInGame  = 16,
      cUI               = 32,
      cTerrainCommon    = 64,
      cTerrainRibbon    = 128,
      cTerrainImpact    = 256
   };
            
   BManagedTexture* getOrCreate(
      const char* pName, 
      BFileOpenFlags fileOpenFlags = BFILE_OPEN_NORMAL,
      DWORD membershipBits = cSystem,
      bool srgbTexture = false,
      eDefaultTexture defaultTexture = cDefaultTextureNormal,
      bool loadImmediately = true,
      bool usePackedTextureManager = false,
      const char* pManagerName = "?",
      bool backgroundLoadable = false);
      
   BManagedTexture* getOrCreate(
      const BRenderStringArray& filenames,
      BFileOpenFlags fileOpenFlags = BFILE_OPEN_NORMAL,
      DWORD membershipBits = cSystem,
      bool srgbTexture = false,
      eDefaultTexture defaultTexture = cDefaultTextureNormal,
      bool loadImmediately = true,
      bool usePackedTextureManager = false,
      const char* pManagerName = "?",
      bool backgroundLoadable = false);
   
   BManagedTexture* find(const char* pName);
   BManagedTexture* find(const BRenderStringArray& filenames);
      
   uint loadAll(DWORD membershipMask = 0xFFFFFFFF);
   uint reloadAll(DWORD membershipMask = 0xFFFFFFFF);
   uint unloadAll(DWORD membershipMask = 0xFFFFFFFF);
   
   // releaseAll() can be very dangerous! Do not call if there are any outstanding handles!
   void releaseAll();
   
   // Waits until all outstanding background loads are completed.
   void sync();
   uint getNumOutstandingBackgroundLoads() const { return mNumOutstandingBackgroundLoads; }
      
   DWORD getTextureReloadCounter() const { return mTextureReloadCounter; }
   
   uint getTotalTextures() const { return mHashMap.getSize(); }
   
   long getBaseDirID() { return mBaseDirID; }
   
   // Alternate handle based API, usable from the sim or render threads.
   // IMPORTANT notes: 
   // BManagedTextureHandle's are just aliases to allocated objects. You MUST release the handle once you are done
   // with it, otherwise we'll leak.
   // This method will ALWAYS returns unique handles.
   BManagedTextureHandle getOrCreateHandle(
      const char* pName, 
      BFileOpenFlags fileOpenFlags = BFILE_OPEN_NORMAL,
      DWORD membershipBits = cSystem,
      bool srgbTexture = false,
      eDefaultTexture defaultTexture = cDefaultTextureNormal,
      bool loadImmediately = true,
      bool usePackedTextureManager = false,
      const char* pManagerName = "?",
      bool backgroundLoadable = false);

   BManagedTextureHandle getOrCreateHandle(
      const BRenderStringArray& filenames,
      BFileOpenFlags fileOpenFlags = BFILE_OPEN_NORMAL,
      DWORD membershipBits = cSystem,
      bool srgbTexture = false,
      eDefaultTexture defaultTexture = cDefaultTextureNormal,
      bool loadImmediately = true,
      bool usePackedTextureManager = false,
      const char* pManagerName = "?",
      bool backgroundLoadable = false);
      
   bool isValidManagedTextureHandle(BManagedTextureHandle handle);
   bool loadManagedTextureByHandle(BManagedTextureHandle handle);
   bool unloadManagedTextureByHandle(BManagedTextureHandle handle);
   bool releaseManagedTextureByHandle(BManagedTextureHandle handle);
   bool setManagedTextureByHandle(BManagedTextureHandle handle, uint samplerIndex);
   
   BManagedTextureHandle getDefaultTextureHandle(eDefaultTexture defaultTexture);
   
   // Avoid calling getManagedTextureByHandle() from the sim thread unless you know exactly what you're doing!
   BManagedTexture* getManagedTextureByHandle(BManagedTextureHandle handle);
   
   // Render thread only.
   // Note: This method increments the refcount of the managed texture!
   BManagedTextureHandle associateTextureWithHandle(BManagedTexture* pManagedTex);
   
   LONG getTotalManagedTextureHandles() const { return mTotalManagedTextureHandles; }
         
#ifndef BUILD_FINAL   
   const BD3DTextureAllocationStatsTracker& getStatTracker() const { return mStatTracker; }
      
   struct BManagerStats
   {
      uint mTotalTextures;
      uint mTotalState[BManagedTexture::cStatusTotal];
      
      void clear() { Utils::ClearObj(*this); }
   };

   void getManagerStats(BManagerStats& managerStats) const;

   struct BDetailManagerStats
   {
      uint mCategory;
      uint mTotalState[BManagedTexture::cStatusTotal];
      uint mTotalTextures;
      uint mTotalAlloc;
      void clear() { Utils::ClearObj(*this); }
   };

   void getDetailManagerStats(uint category, BDetailManagerStats& stats) const;
   
   struct BTextureAllocStats
   {
      BString           mFilename;
      BString           mManager;
      uint              mActualAllocationSize;
      
      uint16            mWidth;
      uint16            mHeight;
      eDDXDataFormat    mDDXFormat;
      uint8             mLevels;
      uint8             mArraySize;
      
      DWORD             mMembershipMask;
      
      bool operator< (const BTextureAllocStats& rhs) const { return mActualAllocationSize > rhs.mActualAllocationSize; }
   };
   
   typedef BDynamicArray<BTextureAllocStats> BTextureAllocStatsArray;
   void getTextureAllocStats(BTextureAllocStatsArray& stats, DWORD membershipMask = 0xFFFFFFFF);
#endif 
                  
private:
   long                                mBaseDirID;
   
   typedef BHashMap<BRenderString, BManagedTexture*, BHasher<BRenderString>, BEqualTo<BRenderString>, true, BRenderFixedHeapAllocator> BTextureHashMap;
   BTextureHashMap                     mHashMap;
   
   BManagedTexture*                    mpDefaultTextures[cDefaultTextureMax];
   BManagedTextureHandle               mDefaultTextures[cDefaultTextureMax];
   
   DWORD                               mTextureReloadCounter;

#ifndef BUILD_FINAL   
   BD3DTextureAllocationStatsTracker   mStatTracker;
#endif   

   LONG                                mTotalManagedTextureHandles;

   DWORD                               mBackgroundLoadLastServiceTime;
   BDynamicRenderArray<void*>          mBackgroundLoadWaitingQueue;
   BDynamicRenderArray<void*>          mBackgroundLoadReadyQueue;
   BWin32Event                         mBackgroundLoadCompleted;
   uint                                mNumOutstandingBackgroundLoads;   
   bool                                mBackgroundLoading;
      
   enum
   {
      cECBackgroundTexture = cEventClassFirstUser,
   };
         
   struct BManagedTextureHandleStruct
   {
      uint              mNonce;
      BManagedTexture*  mpManagedTexture;
   };
   
   struct BGetOrCreateRequestParams
   {
      BD3DTextureManager*           mpManager;
      BManagedTextureHandleStruct*  mpHandle;
      
      BRenderStringArray            mFilenames;
      DWORD                         mMembershipBits;
      BRenderString                 mManagerName;
      BFileOpenFlags                mFileOpenFlags;

      eDefaultTexture               mDefaultTexture;

      bool                          mSRGBTexture : 1;
      bool                          mLoadImmediately : 1;
      bool                          mUsePackedTextureManager : 1;
      bool                          mBackgroundLoadable : 1;
   };
   
   struct BSetManagedTextureCallbackData
   {
      BManagedTextureHandleStruct*  mpTex;
      uint                          mSamplerIndex;
   };
   
   virtual bool                        receiveEvent(const BEvent& event, BThreadIndex threadIndex);   
   
   void                                loadDefaultTextures();
   void                                remove(BManagedTexture* pTex);

   void                                serviceBackgroundWaitingQueue(bool forceAll);
   void                                serviceBackgroundReadyQueue();
   virtual void                        frameEnd();
   
   static void                         getOrCreateHandleCallback(void* pData);
   static void                         loadManagedTextureByHandleCallback(void* pData);
   static void                         unloadManagedTextureByHandleCallback(void* pData);
   static void                         releaseManagedTextureByHandleCallback(void* pData);
   static void                         setManagedTextureByHandleCallback(void* pData);
            
   static void                         loadCallback(void* privateData0, uint64 privateData1, uint workBucketIndex, bool lastWorkEntryInBucket);
   static void                         reloadCallback(void* privateData0, uint64 privateData1, uint workBucketIndex, bool lastWorkEntryInBucket);
};

// This is a render thread only object!
extern BD3DTextureManager gD3DTextureManager;

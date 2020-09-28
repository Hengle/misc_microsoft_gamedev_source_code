//============================================================================
//
//  D3DTextureManager.cpp
//  
//  Copyright (c) 2006-2007, Ensemble Studios
//
//============================================================================
#include "xrender.h"
#include "D3DTextureManager.h"
#include "threading\workDistributor.h"
#include "reloadManager.h"
#include "renderEventClasses.h"
#include "packedTextureManager.h"
#include "D3DTextureLoader.h"
#include "math\randomUtils.h"

BD3DTextureManager gD3DTextureManager;

// BD3DTextureManager

BD3DTextureManager::BD3DTextureManager() :
   mBaseDirID(cDirProduction),
   mTextureReloadCounter(0),
   mBackgroundLoading(false),
   mNumOutstandingBackgroundLoads(0),
   mBackgroundLoadLastServiceTime(0),
   mTotalManagedTextureHandles(0)
{
   Utils::ClearObj(mpDefaultTextures);
   for (uint i = 0; i < cDefaultTextureMax; i++)
      mDefaultTextures[i] = cInvalidManagedTextureHandle;
}

BD3DTextureManager::~BD3DTextureManager()
{
}

void BD3DTextureManager::init(long baseDirID, bool backgroundLoading)
{  
   deinit();

   mBaseDirID = baseDirID;
   mBackgroundLoading = backgroundLoading;

   commandListenerInit();
   eventReceiverInit(cThreadIndexRender);

#ifdef ENABLE_RELOAD_MANAGER
   BReloadManager::BPathArray paths;
   paths.pushBack("*.ddx");
   paths.pushBack("*.xpr");
   gReloadManager.registerClient(paths, BReloadManager::cFlagSynchronous|BReloadManager::cFlagSubDirs, mEventHandle, cRenderEventClassTextureManagerReloadManager);
#endif
   loadDefaultTextures();
}

void BD3DTextureManager::loadDefaultTextures()
{
   const char* defaultTextures[] = 
   {
      "system\\default\\defaultWhite.ddx",
      "system\\default\\defaultRed.ddx",
      "system\\default\\defaultGreen.ddx",
      "system\\default\\defaultBlue.ddx",
      "system\\default\\defaultBlack.ddx",
      "system\\default\\defaultNormal_nm.ddx",
      "system\\default\\defaultCheckerboard.ddx",
      "system\\default\\defaultTransparent.ddx" 
   };
   const uint cNumDefaultTextures = sizeof(defaultTextures)/sizeof(defaultTextures[0]);

   BCOMPILETIMEASSERT(cNumDefaultTextures == cDefaultTextureMax);

   for (uint i = 0; i < cNumDefaultTextures; i++)
   {
      bool srgbTexture = true;
      if (i == cDefaultTextureNormal) 
         srgbTexture = false;

      mpDefaultTextures[i] = getOrCreate(defaultTextures[i], BFILE_OPEN_DISCARD_ON_CLOSE, cSystem, srgbTexture, cDefaultTextureInvalid, true, false, "BD3DTextureManager");
      if (mpDefaultTextures[i]->getStatus() != BManagedTexture::cStatusLoaded)
      {
         trace("BD3DTextureManager::init: Unable to load default texture: %s", defaultTextures[i]);
         gConsoleOutput.error("BD3DTextureManager::init: Unable to load default texture: %s", defaultTextures[i]);
      }
      
      mDefaultTextures[i] = associateTextureWithHandle(mpDefaultTextures[i]);
   }
}

BD3DTextureManager::BManagedTexture* BD3DTextureManager::getDefaultTexture(eDefaultTexture defaultTexture)
{
   BDEBUG_ASSERT((defaultTexture >= 0) && (defaultTexture < cDefaultTextureMax));

   BDEBUG_ASSERT(mpDefaultTextures[defaultTexture]);

   return mpDefaultTextures[defaultTexture];
}

bool BD3DTextureManager::isDefaultTexture(const BManagedTexture* pTexture) const
{
   for (uint i = 0; i < cDefaultTextureMax; i++)
      if (pTexture == mpDefaultTextures[i])
         return true;
   return false;
}

void BD3DTextureManager::deinit()
{
   sync();

   releaseAll();
#ifdef ENABLE_RELOAD_MANAGER
   if (cInvalidEventReceiverHandle != mEventHandle)
      gReloadManager.deregisterClient(mEventHandle);
#endif
   eventReceiverDeinit();
   commandListenerDeinit();
   
   Utils::ClearObj(mpDefaultTextures);
   
   for (uint i = 0; i < cDefaultTextureMax; i++)
      mDefaultTextures[i] = cInvalidManagedTextureHandle;
}

BD3DTextureManager::BManagedTexture* BD3DTextureManager::getOrCreate(
   const char* pName, 
   BFileOpenFlags fileOpenFlags,
   DWORD membershipBits,
   bool srgbTexture,
   eDefaultTexture defaultTexture,
   bool loadImmediately,
   bool usePackedTextureManager,
   const char* pManagerName,
   bool backgroundLoadable)
{
   ASSERT_THREAD(cThreadIndexRender);

   BDEBUG_ASSERT(pName);

   BRenderStringArray filenames;
   filenames.pushBack(pName);

   return getOrCreate(filenames, fileOpenFlags, membershipBits, srgbTexture, defaultTexture, loadImmediately, usePackedTextureManager, pManagerName, backgroundLoadable);
}

BD3DTextureManager::BManagedTexture* BD3DTextureManager::getOrCreate(
   const BRenderStringArray& filenames,
   BFileOpenFlags fileOpenFlags,
   DWORD membershipBits,
   bool srgbTexture,
   eDefaultTexture defaultTexture,
   bool loadImmediately,
   bool usePackedTextureManager,
   const char* pManagerName,
   bool backgroundLoadable)
{
   ASSERT_THREAD(cThreadIndexRender);

   BRenderStringArray standardizedPaths(filenames);
   BRenderString identifier;
   for (uint i = 0; i < standardizedPaths.getSize(); i++)
   {
      standardizedPaths[i].standardizePath();
      if (i)
         identifier += "!";
      identifier += standardizedPaths[i];
   }

   std::pair<BTextureHashMap::iterator, bool> insertRes(mHashMap.insert(std::make_pair(identifier, (BManagedTexture*)NULL)));

   if (!insertRes.second)
   {
      BD3DTextureManager::BManagedTexture* pTex = insertRes.first->second;
      pTex->addRef();
      return pTex;
   }

   BD3DTextureManager::BManagedTexture* pTex = HEAP_NEW(BManagedTexture, gRenderHeap);
   pTex->setTextureManager(this);
   insertRes.first->second = pTex;

   pTex->init(mBaseDirID, standardizedPaths, fileOpenFlags, membershipBits, srgbTexture, defaultTexture, usePackedTextureManager, pManagerName, backgroundLoadable);

   if (loadImmediately)
   {
      bool result = pTex->load();
      if (!result)
         trace("BD3DTextureManager::getOrCreate: failed to load: %s", filenames[0].getPtr());
   }

   return pTex;
}   

BD3DTextureManager::BManagedTexture* BD3DTextureManager::find(const char* pName)
{
   BRenderString filename(pName);
   filename.standardizePath();

   BTextureHashMap::iterator findRes(mHashMap.find(filename));
   if (findRes == mHashMap.end())
      return NULL;

   return findRes->second;
}

BD3DTextureManager::BManagedTexture* BD3DTextureManager::find(const BRenderStringArray& filenames)
{
   BRenderStringArray standardizedPaths(filenames);
   BRenderString identifier;
   for (uint i = 0; i < standardizedPaths.getSize(); i++)
   {
      standardizedPaths[i].standardizePath();
      if (i)
         identifier += "!";
      identifier += standardizedPaths[i];
   }

   BTextureHashMap::const_iterator findRes(mHashMap.find(identifier));
   if (findRes == mHashMap.end())
      return NULL;

   return findRes->second;
}

void BD3DTextureManager::loadCallback(void* privateData0, uint64 privateData1, uint workBucketIndex, bool lastWorkEntryInBucket)
{
   BManagedTexture* pTex = (BManagedTexture*)privateData0;
   BCountDownEvent* pCountDownEvent = (BCountDownEvent*)(privateData1);

   BDEBUG_ASSERT(pTex && pCountDownEvent);

   //trace("BD3DTextureManager::loadCallback() -- loading %s", pTex->getFilenames()[0].getPtr());

   pTex->load();

   pCountDownEvent->decrement();
}

void BD3DTextureManager::reloadCallback(void* privateData0, uint64 privateData1, uint workBucketIndex, bool lastWorkEntryInBucket)
{
   BManagedTexture* pTex = (BManagedTexture*)privateData0;
   BCountDownEvent* pCountDownEvent = (BCountDownEvent*)(privateData1);

   BDEBUG_ASSERT(pTex && pCountDownEvent);

   pTex->reload();

   pCountDownEvent->decrement();
}

uint BD3DTextureManager::loadAll(DWORD membershipMask)
{
   ASSERT_THREAD(cThreadIndexRender);

   uint totalTextures = 0;
   for (BTextureHashMap::iterator it = mHashMap.begin(); it != mHashMap.end(); ++it)
   {
//-- FIXING PREFIX BUG ID 7122
      const BManagedTexture* pTex = it->second;
//--
      if (pTex->getMembershipBits() & membershipMask)
      {
         if (pTex->getStatus() == BManagedTexture::cStatusInitialized)
            totalTextures++;
      }
   }

   if (!totalTextures)
      return 0;
   
   if (1 == totalTextures || gFileManager.getConfigFlags() & BFileManager::cEnableLooseFiles)
   {
      for (BTextureHashMap::iterator it = mHashMap.begin(); it != mHashMap.end(); ++it)
      {
         BManagedTexture* pTex = it->second;
         if ((pTex->getStatus() == BManagedTexture::cStatusInitialized) && (pTex->getMembershipBits() & membershipMask))
         {
            pTex->load();
            if (totalTextures == 1)
               break;
         }
      }
   }
   else
   {
      BCountDownEvent countDownEvent;
      countDownEvent.set(totalTextures);

      for (BTextureHashMap::iterator it = mHashMap.begin(); it != mHashMap.end(); ++it)
      {
         BManagedTexture* pTex = it->second;
         if ((pTex->getStatus() == BManagedTexture::cStatusInitialized) && (pTex->getMembershipBits() & membershipMask))
            gWorkDistributor.queue(loadCallback, pTex, (uint64)&countDownEvent, 1);
      }

      trace("BD3DTextureManager::loadAll: Loading and decompressing %u textures", totalTextures);

      gWorkDistributor.flushAndWaitSingle(countDownEvent, (DWORD)-1, 8, false);

      trace("BD3DTextureManager::loadAll: Load completed");
   }      

   return totalTextures;
}

uint BD3DTextureManager::reloadAll(DWORD membershipMask)
{
   ASSERT_THREAD(cThreadIndexRender);

   BCountDownEvent countDownEvent;

   uint totalTextures = 0;
   for (BTextureHashMap::iterator it = mHashMap.begin(); it != mHashMap.end(); ++it)
   {
//-- FIXING PREFIX BUG ID 7123
      const BManagedTexture* pTex = it->second;
//--
      if ((pTex->getStatus() != BManagedTexture::cStatusInvalid) && (pTex->getMembershipBits() & membershipMask))
         totalTextures++;
   }

   if (!totalTextures)
      return 0;

   if (1 == totalTextures || (gFileManager.getConfigFlags() & BFileManager::cEnableLooseFiles))
   {
      for (BTextureHashMap::iterator it = mHashMap.begin(); it != mHashMap.end(); ++it)
      {
         BManagedTexture* pTex = it->second;
         if ((pTex->getStatus() != BManagedTexture::cStatusInvalid) && (pTex->getMembershipBits() & membershipMask))
         {
            pTex->reload();
            if (totalTextures == 1)
               break;
         }
      }
   }
   else
   {
      countDownEvent.set(totalTextures);

      for (BTextureHashMap::iterator it = mHashMap.begin(); it != mHashMap.end(); ++it)
      {
         BManagedTexture* pTex = it->second;
         if ((pTex->getStatus() != BManagedTexture::cStatusInvalid) && (pTex->getMembershipBits() & membershipMask))
            gWorkDistributor.queue(reloadCallback, pTex, (uint64)&countDownEvent, 1);
      }

      trace("BD3DTextureManager::reloadAll: Loading and decompressing %u textures", totalTextures);

      gWorkDistributor.flushAndWaitSingle(countDownEvent, (DWORD)-1, 8, false);

      trace("BD3DTextureManager::reloadAll: Reload completed");
   }

   return totalTextures;
}

uint BD3DTextureManager::unloadAll(DWORD membershipMask)
{
   uint total = 0;
   for (BTextureHashMap::iterator it = mHashMap.begin(); it != mHashMap.end(); ++it)
   {
      BManagedTexture* pTex = it->second;
      if ((pTex->getStatus() != BManagedTexture::cStatusInvalid) && (pTex->getMembershipBits() & membershipMask))
      {
         trace("BD3DTextureManager::unloadAll(): Unloading Texture - %s", pTex->getFilenames()[0].getPtr());

         pTex->unload();         
         total++;
      }
   }
   return total;
}

void BD3DTextureManager::releaseAll()
{
   ASSERT_THREAD(cThreadIndexRender);

   BDynamicRenderArray<BManagedTexture*> texturesToDelete;

   for (BTextureHashMap::iterator it = mHashMap.begin(); it != mHashMap.end(); ++it)
   {
      BManagedTexture* pTex = it->second;
      texturesToDelete.pushBack(pTex);
   }

   for (uint i = 0; i < texturesToDelete.getSize(); i++)
      HEAP_DELETE(texturesToDelete[i], gRenderHeap);

   mHashMap.clear();
}

void BD3DTextureManager::sync()
{
   ASSERT_THREAD(cThreadIndexRender);

   serviceBackgroundWaitingQueue(true);
   
   gEventDispatcher.dispatchEvents();
   
   serviceBackgroundReadyQueue();
   
   if (!mNumOutstandingBackgroundLoads)
      return;
   
   do
   {
      gWorkDistributor.waitSingle(mBackgroundLoadCompleted);
   } while (mNumOutstandingBackgroundLoads);
      
   serviceBackgroundReadyQueue();
}

void BD3DTextureManager::remove(BManagedTexture* pTex)
{
   ASSERT_THREAD(cThreadIndexRender);

   if (pTex->getFilenames().isEmpty())
      return;

   BRenderString identifier;
   BTextureHashMap::iterator it = mHashMap.find(pTex->getIdentifier(identifier));
   if (it != mHashMap.end())
      mHashMap.erase(it);
}

bool BD3DTextureManager::receiveEvent(const BEvent& event, BThreadIndex threadIndex)
{
   switch (event.mEventClass)
   {
      case cECBackgroundTexture:
      {
         mBackgroundLoadReadyQueue.pushBack((void*)event.mPrivateData);
         
         mBackgroundLoadCompleted.set();

         BDEBUG_ASSERT(mNumOutstandingBackgroundLoads >= 0);
         mNumOutstandingBackgroundLoads--;

         break;
      }
#ifdef ENABLE_RELOAD_MANAGER
      case cRenderEventClassTextureManagerReloadManager:
      {
//-- FIXING PREFIX BUG ID 7124
         const BReloadManager::BNotificationPayload* pPayload = static_cast<BReloadManager::BNotificationPayload*>(event.mpPayload);
//--

         gConsoleOutput.status("BD3DTextureManager::receiveEvent: Received reload event: %s", pPayload->mPath.getPtr());

         BString pathname(pPayload->mPath);
         pathname.standardizePath();

         BString filename;
         strPathGetFilename(pathname, filename);

         strPathRemoveExtension(filename);

         for (BTextureHashMap::iterator it = mHashMap.begin(); it != mHashMap.end(); ++it)
         {
            BManagedTexture* pTex = it->second;

            if ((pTex->getStatus() == BManagedTexture::cStatusLoaded) || 
               (pTex->getStatus() == BManagedTexture::cStatusLoadingInBackground) ||
               (pTex->getStatus() == BManagedTexture::cStatusLoadFailed))
            {
               BRenderString identifier;
               if (strstr(pTex->getIdentifier(identifier), filename) != NULL)
               {
                  gConsoleOutput.status("BD3DTextureManager::receiveEvent: Reloading texture: %s", identifier.getPtr());

                  pTex->reload();

                  mTextureReloadCounter++;
               }
            }
         }
      }
#endif
   }

   return false;
}

// BManagedTexture

BD3DTextureManager::BManagedTexture::BBackgroundLoadRequestPacket::BBackgroundLoadRequestPacket() 
{ 
   Utils::ClearObj(*this); 
}

BD3DTextureManager::BManagedTexture::BBackgroundLoadRequestPacket::~BBackgroundLoadRequestPacket() 
{ 
   if (mpTextureLoader) 
      HEAP_DELETE(mpTextureLoader, gRenderHeap); 
} 

BD3DTextureManager::BManagedTexture::BManagedTexture() :
   mpTextureManager(NULL),
   mRefCount(1),
   mDirID(0),
   mFileOpenFlags(BFILE_OPEN_NORMAL),
   mMembershipBits(1),
   mDefaultTexture(cDefaultTextureNormal),
   mStatus(cStatusInvalid),
   mSRGBTexture(false),
   mUsePackedTextureManager(false),
   mHDRScale(1.0f),
   mActualAllocationSize(0),
   mD3DFormat(D3DFMT_A8R8G8B8),
   mWidth(0),
   mHeight(0),
   mLevels(0),
   mArraySize(0),
   mDDXFormat(cDDXDataFormatInvalid),
   mBackgroundLoadable(false),
   mBackgroundLoadRequestIndex(RandomUtils::GenerateNonce32())
{
}

BD3DTextureManager::BManagedTexture::~BManagedTexture()
{
   if (mpTextureManager)
      mpTextureManager->remove(this);

   deinit();
}

BRenderString& BD3DTextureManager::BManagedTexture::getIdentifier(BRenderString& identifier) const
{
   identifier.empty();
   for (uint i = 0; i < mFilenames.getSize(); i++)
   {
      if (i)
         identifier += "!";
      identifier += mFilenames[i];
   }
   return identifier;      
}

void BD3DTextureManager::BManagedTexture::init(
   long dirID, const BRenderStringArray& filenames, 
   BFileOpenFlags fileOpenFlags, DWORD membershipBits, bool srgbTexture, eDefaultTexture defaultTexture, bool usePackedTextureManager,
   const char* pManagerName, bool backgroundLoadable)
{
   ASSERT_THREAD(cThreadIndexRender);

   deinit();

   mDirID = dirID;
   mFilenames = filenames;
   mManagerName.set(pManagerName ? pManagerName : "?");

   mFileOpenFlags = fileOpenFlags;
   mMembershipBits = membershipBits;
   mSRGBTexture = srgbTexture;
   mDefaultTexture = defaultTexture;
   mUsePackedTextureManager = usePackedTextureManager;

   mStatus = cStatusInitialized;

   mBackgroundLoadable = backgroundLoadable;
}

void BD3DTextureManager::BManagedTexture::releaseTexture()
{
   ASSERT_THREAD(cThreadIndexRender);

   if (mTexture.getBaseTexture())
   {
#ifndef BUILD_FINAL   
      bool isPackedTexture = false;
      if (gpPackedTextureManager)
         isPackedTexture = gpPackedTextureManager->isPackedTexture(mTexture.getBaseTexture());
      if (!isPackedTexture)
      {
         BD3DTextureAllocationStats stats(mTexture.getBaseTexture(), 1, mActualAllocationSize);
         mpTextureManager->mStatTracker.remove(stats);
      }         
#endif      

      mTexture.release();
   }
}

void BD3DTextureManager::BManagedTexture::deinit()
{
   releaseTexture();

   mDirID = 0;
   mFileOpenFlags = BFILE_OPEN_NORMAL;
   mMembershipBits = 1;
   mDefaultTexture = cDefaultTextureNormal;
   mStatus = cStatusInvalid;
   mSRGBTexture = false;
   mUsePackedTextureManager = false;
   mHDRScale = 1.0f;
   mReloadListeners.clear();
   mActualAllocationSize = 0;
   mD3DFormat = D3DFMT_A8R8G8B8;
   mWidth = 0;
   mHeight = 0;
   mLevels = 0;
   mArraySize = 0;
   mDDXFormat = cDDXDataFormatInvalid;
   mBackgroundLoadable = false;
   mBackgroundLoadRequestIndex++;
}

bool BD3DTextureManager::getActualFilename(const char* pFilename, BRenderString& actualFilename)
{
   actualFilename.set(pFilename);

   if (gFileManager.doesFileExist(mBaseDirID, actualFilename) != cFME_SUCCESS)
   {
      actualFilename += ".ddx";

      if (gFileManager.doesFileExist(mBaseDirID, actualFilename) != cFME_SUCCESS)
      {
         actualFilename.set(pFilename);
         actualFilename += ".xpr";
      }
   }

   return cFME_SUCCESS == gFileManager.doesFileExist(mBaseDirID, actualFilename);
}

#ifndef BUILD_FINAL
void BD3DTextureManager::getManagerStats(BManagerStats& managerStats) const
{
   if (gRenderThread.isSimThread())
      gRenderThread.blockUntilWorkerIdle();

   managerStats.clear();
   managerStats.mTotalTextures = mHashMap.getSize();

   for (BTextureHashMap::const_iterator it = mHashMap.begin(); it != mHashMap.end(); ++it)
   {
      const BManagedTexture* pTex = it->second;

      BManagedTexture::eStatus texStatus = BManagedTexture::cStatusInvalid;

      if (pTex)
         texStatus = pTex->getStatus();

      managerStats.mTotalState[texStatus]++;
   }   
}

void BD3DTextureManager::getDetailManagerStats(uint category, BDetailManagerStats& stats) const
{
   if (gRenderThread.isSimThread())
      gRenderThread.blockUntilWorkerIdle();

   stats.clear();   
   stats.mCategory = category;
   for (BTextureHashMap::const_iterator it = mHashMap.begin(); it != mHashMap.end(); ++it)
   {
      const BManagedTexture* pTex = it->second;      
      BManagedTexture::eStatus texStatus = BManagedTexture::cStatusInvalid;

      if (pTex)
      {
         if (pTex->getMembershipBits() == category)
         {
            stats.mTotalTextures++;    
            texStatus = pTex->getStatus();
            stats.mTotalState[texStatus]++;
            stats.mTotalAlloc += pTex->getActualAllocationSize();
         }
      }            
   }   
}
#endif

bool BD3DTextureManager::BManagedTexture::loadTexture(
   BD3DTextureLoader& textureLoader,
   long dirID,
   const BRenderStringArray& filenames, 
   bool srgbTexture,
   bool usePackedTextureManager,
   BFileOpenFlags fileOpenFlags,
   const BRenderString& managerName)
{
   BD3DTextureLoader::BCreateParams textureCreateParams;
   textureCreateParams.mForceSRGBGamma = srgbTexture;
   textureCreateParams.mUsePackedTextureManager = usePackedTextureManager;
   BDEBUG_ASSERT(filenames.getSize() <= UCHAR_MAX);
   if (filenames.getSize() > 1)
      textureCreateParams.mArraySize = static_cast<uchar>(filenames.getSize());

   if (filenames.getSize())
   {
      textureCreateParams.mName = filenames[0];
      textureCreateParams.mManager = managerName;
   }

   BDynamicRenderArray<uchar> data;

   for (uint i = 0; i < filenames.getSize(); i++)
   {
      if (filenames.getSize() > 1)
         textureCreateParams.mArrayIndex = static_cast<uchar>(i);

      const BRenderString& filename = filenames[i];

      BRenderString actualFilename;
      if (gFileManager.doesFileExist(dirID, filename) == cFME_SUCCESS)
         actualFilename = filename;
      else
      {
         actualFilename = filename;
         actualFilename += ".ddx";
         if (gFileManager.doesFileExist(dirID, actualFilename) != cFME_SUCCESS)
         {
            actualFilename = filename;
            actualFilename += ".xpr";
         }
      }

      BRenderString extension;
      strPathGetExtension(actualFilename, extension);

      BFile file;
      bool status = file.openReadOnly(dirID, actualFilename, fileOpenFlags);
      if (!status)
      {
         gConsoleOutput.error("BD3DTextureManager::BManagedTexture::load: Failed reading texture, dirID: %i, Filename: %s, Mgr: %s!\n", dirID, filename.getPtr(), managerName.getPtr());
         return false;
      }

      unsigned long fileSize;
      if (!file.getSize(fileSize))
      {
         gConsoleOutput.error("BD3DTextureManager::BManagedTexture::load: Unable to read: %s\n", actualFilename.getPtr());
         return false;
      }

      if (!fileSize)
      {
         gConsoleOutput.error("BD3DTextureManager::BManagedTexture::load: File is empty: %s\n", actualFilename.getPtr());
         return false;
      }

      if (data.getSize() < fileSize)
      {
         data.clear();
         data.resize(fileSize);
      }

      if (!file.read(data.getPtr(), fileSize))
      {
         gConsoleOutput.error("BD3DTextureManager::BManagedTexture::load: Unable to read: %s\n", actualFilename.getPtr());
         return false;
      }

      if (extension == ".xpr")
      {
         if (!textureLoader.createFromXPRFileInMemory(data.getPtr(), fileSize, textureCreateParams))
         {
            gConsoleOutput.error("BD3DTextureManager::BManagedTexture::load: Failed creating XPR texture from file: %s\n", actualFilename.getPtr());
            return false;
         }   
      }
      else if (extension == ".ddx")
      {
         if (!textureLoader.createFromDDXFileInMemory(data.getPtr(), fileSize, textureCreateParams))
         {
            gConsoleOutput.error("BD3DTextureManager::BManagedTexture::load: Failed creating DDX texture from file: %s\n", actualFilename.getPtr());
            return false;
         }   
      }
      else
      {
         gConsoleOutput.error("BD3DTextureManager::BManagedTexture::load: Unrecognized texture format: %s\n", actualFilename.getPtr());
         return false;
      }

         if (filenames.getSize() > 1)
            gConsoleOutput.resource("Loaded ArrayTex: %s, Res: %ux%u, Fmt: %s, Slice: %i of %i, Mgr: %s\n", actualFilename.getPtr(), textureLoader.getWidth(), textureLoader.getHeight(), getDDXDataFormatString(textureLoader.getDDXDataFormat()), i, filenames.getSize(), managerName.getPtr());
         else
            gConsoleOutput.resource("Loaded Tex: %s, Res: %ux%u, Fmt: %s, Mgr: %s\n", actualFilename.getPtr(), textureLoader.getWidth(), textureLoader.getHeight(), getDDXDataFormatString(textureLoader.getDDXDataFormat()), managerName.getPtr());
   }      

   return true;
};

void BD3DTextureManager::BManagedTexture::backgroundLoadCallbackFunc(void* privateData0, uint64 privateData1, uint workBucketIndex, bool lastWorkEntryInBucket)
{
   BD3DTextureManager::BManagedTexture::BBackgroundLoadRequestPacket* pPacket = static_cast<BBackgroundLoadRequestPacket*>(privateData0);

   pPacket->mpTextureLoader = HEAP_NEW(BD3DTextureLoader, gRenderHeap);

   pPacket->mStatus = loadTexture(
      *pPacket->mpTextureLoader,
      pPacket->mDirID,
      pPacket->mFilenames,
      pPacket->mSRGBTexture,
      pPacket->mUsePackedTextureManager,
      pPacket->mFileOpenFlags,
      pPacket->mManagerName);

   gEventDispatcher.send(cInvalidEventReceiverHandle, pPacket->mpTextureManager->getEventHandle(), cECBackgroundTexture, (uint)pPacket, 0, NULL, 0);//cEventFlagSynchronous);
}

void BD3DTextureManager::BManagedTexture::finalizeTextureLoad(BD3DTextureLoader& textureLoader)
{
   mTexture = textureLoader.getD3DTexture();
   mHDRScale = textureLoader.getHDRScale();
   mArrayHDRScale = textureLoader.getArrayHDRScaleContainer();
   mActualAllocationSize = textureLoader.getAllocationSize();
   mWidth = static_cast<uint16>(textureLoader.getWidth());
   mHeight = static_cast<uint16>(textureLoader.getHeight());
   mLevels = static_cast<uint8>(textureLoader.getLevels());
   mArraySize = static_cast<uint8>(textureLoader.getArraySize());
   mD3DFormat = textureLoader.getFormat();
   mDDXFormat = textureLoader.getDDXDataFormat();

   textureLoader.releaseOwnership();

#ifndef BUILD_FINAL
   bool isPackedTexture = false;
   if (gpPackedTextureManager)
      isPackedTexture = gpPackedTextureManager->isPackedTexture(mTexture.getBaseTexture());
   if (!isPackedTexture)
   {
      BD3DTextureAllocationStats stats(mTexture.getBaseTexture(), 1, mActualAllocationSize, mMembershipBits);
      mpTextureManager->mStatTracker.add(stats);
   }
#endif
}

void BD3DTextureManager::BManagedTexture::finalizeBackgroundTextureLoad(BBackgroundLoadRequestPacket* pPacket)
{
   if ((pPacket->mpManagedTexture == this) && 
      (pPacket->mBackgroundLoadRequestIndex == mBackgroundLoadRequestIndex) &&
      (pPacket->mpManagedTexture->mStatus == cStatusLoadingInBackground))
   {
      if (!pPacket->mStatus)
      {
         mStatus = cStatusLoadFailed;
      }
      else
      {
         finalizeTextureLoad(*pPacket->mpTextureLoader);
         mStatus = cStatusLoaded;
      }
      
      for (uint i = 0; i < mReloadListeners.getSize(); i++)
         (*mReloadListeners[i].mpFunc)(this, mReloadListeners[i].mPrivateData);
   }

   HEAP_DELETE(pPacket, gRenderHeap);
}

void BD3DTextureManager::BManagedTexture::handleBackgroundTextureLoad(void* p)
{
   BManagedTexture::BBackgroundLoadRequestPacket* pPacket = static_cast<BManagedTexture::BBackgroundLoadRequestPacket*>(p);

   BManagedTexture* pTexture = pPacket->mpTextureManager->find(pPacket->mFilenames);
   if (pTexture)
      pTexture->finalizeBackgroundTextureLoad(pPacket);
   else
      HEAP_DELETE(pPacket, gRenderHeap);
}

bool BD3DTextureManager::BManagedTexture::load()
{
   SCOPEDSAMPLE(BD3DTextureManager_BManagedTexture_load)
   
   if ((cStatusInvalid == mStatus) || (cStatusLoadFailed == mStatus) || (mFilenames.isEmpty()))
      return false;
   else if (cStatusLoaded == mStatus)
      return true;

   if ((mBackgroundLoadable) && (mpTextureManager->getBackgroundLoadingEnabled()))
   {
      if (mStatus != cStatusLoadingInBackground)
      {
         mBackgroundLoadRequestIndex++;

         BBackgroundLoadRequestPacket* pPacket = HEAP_NEW(BBackgroundLoadRequestPacket, gRenderHeap);

         pPacket->mpTextureManager              = mpTextureManager;
         pPacket->mpManagedTexture              = this;
         pPacket->mpTextureLoader               = NULL;
         pPacket->mBackgroundLoadRequestIndex   = mBackgroundLoadRequestIndex;
         pPacket->mDirID                        = mDirID;
         pPacket->mFilenames                    = mFilenames;
         pPacket->mFileOpenFlags                = mFileOpenFlags;
         pPacket->mSRGBTexture                  = mSRGBTexture;
         pPacket->mUsePackedTextureManager      = mUsePackedTextureManager;
         pPacket->mStatus                       = false;
         pPacket->mManagerName                  = mManagerName;

         mpTextureManager->mBackgroundLoadWaitingQueue.pushBack(pPacket);
         
         mpTextureManager->mNumOutstandingBackgroundLoads++;
         
         mStatus = cStatusLoadingInBackground;
      }         

      return true;
   }

   BD3DTextureLoader textureLoader;

   bool status = loadTexture(textureLoader, mDirID, mFilenames, mSRGBTexture, mUsePackedTextureManager, mFileOpenFlags, mManagerName);
   if (!status)
   {
      mStatus = cStatusLoadFailed;
      return false;
   }

   finalizeTextureLoad(textureLoader);

   mStatus = cStatusLoaded;
   return true;
}

void BD3DTextureManager::BManagedTexture::unload()
{
   ASSERT_THREAD(cThreadIndexRender);

   if ((mStatus == cStatusLoaded) || (mStatus == cStatusLoadingInBackground))
   {
      if (mFilenames.getSize())
      {
         gConsoleOutput.resource("Unloaded Tex: %s, Res: %ux%u, Fmt: %s, Mgr: %s\n", 
            mFilenames[0].getPtr(), mWidth, mHeight, getDDXDataFormatString(mDDXFormat), mManagerName.getPtr());
      }
   }      
   
   if (mStatus == cStatusLoaded)
   {
      releaseTexture();
      
      mStatus = cStatusInitialized;
   }
   else if (mStatus == cStatusLoadingInBackground)
   {
      mStatus = cStatusInitialized;
      mBackgroundLoadRequestIndex++;
   }
}

bool BD3DTextureManager::BManagedTexture::reload()
{
   ASSERT_THREAD(cThreadIndexRender);

   switch (mStatus)
   {  
      case cStatusInvalid:
         return false;
      case cStatusLoaded:
      {
         releaseTexture();

         mStatus = cStatusInitialized;
         break;
      }
      case cStatusLoadFailed:
      {
         mStatus = cStatusInitialized;
         break;
      }
      case cStatusLoadingInBackground:
      {
         mStatus = cStatusInitialized;
         mBackgroundLoadRequestIndex++;
         break;
      }         
   }

   bool success = load();

   for (uint i = 0; i < mReloadListeners.getSize(); i++)
      (*mReloadListeners[i].mpFunc)(this, mReloadListeners[i].mPrivateData);

   return success;
}

void BD3DTextureManager::BManagedTexture::addRef()
{
   ASSERT_THREAD(cThreadIndexRender);
   mRefCount++;
}

void BD3DTextureManager::BManagedTexture::release()
{
   ASSERT_THREAD(cThreadIndexRender);

   BDEBUG_ASSERT(mRefCount >= 0);

   mRefCount--;

   if (!mRefCount)
   {
      if (mFilenames.getSize())
      {
         gConsoleOutput.resource("Deleting Tex: %s, Res: %ux%u, Fmt: %s, Mgr: %s\n", 
            mFilenames[0].getPtr(), mWidth, mHeight, getDDXDataFormatString(mDDXFormat), mManagerName.getPtr());
      }
      
      HEAP_DELETE(this, gRenderHeap);
   }
}

static BD3DTexture gInvalidD3DTex;

const BD3DTexture& BD3DTextureManager::BManagedTexture::getD3DTexture() const
{
   if (mStatus == cStatusInvalid)
      return gInvalidD3DTex;

   BDEBUG_ASSERT(mpTextureManager);

   if (mTexture.getBaseTexture())
      return mTexture;
   else if (mDefaultTexture != cDefaultTextureInvalid)
   {
//-- FIXING PREFIX BUG ID 7125
      const BManagedTexture* pDefTex = mpTextureManager->getDefaultTexture(mDefaultTexture);
//--
      return pDefTex ? pDefTex->mTexture : gInvalidD3DTex;
   }
   else
      return gInvalidD3DTex;
}

void BD3DTextureManager::BManagedTexture::addReloadListener(BReloadListenerFuncPtr pFunc, uint64 privateData)
{
   mReloadListeners.pushBack(BReloadListener(pFunc, privateData));
}

bool BD3DTextureManager::BManagedTexture::removeReloadListener(BReloadListenerFuncPtr pFunc, uint64 privateData)
{
   return mReloadListeners.remove(BReloadListener(pFunc, privateData));
}

void BD3DTextureManager::BManagedTexture::clearReloadListeners()
{
   mReloadListeners.clear();
}

#ifndef BUILD_FINAL
void BD3DTextureManager::getTextureAllocStats(BTextureAllocStatsArray& stats, DWORD membershipMask)
{
   for (BTextureHashMap::iterator it = mHashMap.begin(); it != mHashMap.end(); ++it)
   {
//-- FIXING PREFIX BUG ID 7126
      const BManagedTexture* pTex = it->second;
//--
      if ((pTex->getStatus() == BManagedTexture::cStatusLoaded) && (pTex->getMembershipBits() & membershipMask))
      {
         BTextureAllocStats* p = stats.enlarge(1);

         if (pTex->getFilenames().getSize())
            p->mFilename = pTex->getFilenames()[0];
         
         p->mManager = pTex->getManagerName();

         p->mActualAllocationSize = pTex->getActualAllocationSize();
         p->mMembershipMask = pTex->getMembershipBits();
         p->mWidth = (uint16)pTex->getWidth();
         p->mHeight = (uint16)pTex->getHeight();
         p->mDDXFormat = pTex->getDDXFormat();
         p->mLevels = (uint8)pTex->getLevels();
         p->mArraySize = (uint8)pTex->getArraySize();
      }
   }   
}
#endif

void BD3DTextureManager::serviceBackgroundWaitingQueue(bool forceAll)
{
   ASSERT_THREAD(cThreadIndexRender);
   
   if (mBackgroundLoadWaitingQueue.isEmpty())
      return;
   
   if (!forceAll)
   {
      const DWORD curTime = GetTickCount();
      if ((curTime - mBackgroundLoadLastServiceTime) < 250U)
         return;
      mBackgroundLoadLastServiceTime = curTime;
   }      
   
//   trace("serviceBackgroundWaitingQueue: %u textures", mBackgroundLoadWaitingQueue.getSize());
   
   uint numTexturesToLoad = mBackgroundLoadWaitingQueue.getSize();
   if (!forceAll)
   {
      if (numTexturesToLoad > 1000)
         numTexturesToLoad = Math::Min<uint>(20U, numTexturesToLoad);
      else if (numTexturesToLoad > 250)
         numTexturesToLoad = Math::Min<uint>(10U, numTexturesToLoad);
      else
         numTexturesToLoad = Math::Min<uint>(5U, numTexturesToLoad);
   }
      
   for (uint i = 0; i < numTexturesToLoad; i++)
   {
      gWorkDistributor.queue(BManagedTexture::backgroundLoadCallbackFunc, mBackgroundLoadWaitingQueue[mBackgroundLoadWaitingQueue.getSize() - 1 - i], 0, 1);
      gWorkDistributor.flush();  
   }
   
   mBackgroundLoadWaitingQueue.resize(mBackgroundLoadWaitingQueue.getSize() - i);
}

void BD3DTextureManager::serviceBackgroundReadyQueue()
{
   ASSERT_THREAD(cThreadIndexRender);
   
   for (uint i = 0; i < mBackgroundLoadReadyQueue.getSize(); i++)
      BManagedTexture::handleBackgroundTextureLoad(mBackgroundLoadReadyQueue[i]);

   mBackgroundLoadReadyQueue.clear();
}

void BD3DTextureManager::frameEnd() 
{ 
   ASSERT_THREAD(cThreadIndexRender);
   
   serviceBackgroundWaitingQueue(false);
   serviceBackgroundReadyQueue();
}

BManagedTextureHandle BD3DTextureManager::getOrCreateHandle(
   const char* pName, 
   BFileOpenFlags fileOpenFlags,
   DWORD membershipBits,
   bool srgbTexture,
   eDefaultTexture defaultTexture,
   bool loadImmediately,
   bool usePackedTextureManager,
   const char* pManagerName,
   bool backgroundLoadable)
{
   BDEBUG_ASSERT(pName);

   BRenderStringArray filenames;
   filenames.pushBack(pName);

   return getOrCreateHandle(filenames, fileOpenFlags, membershipBits, srgbTexture, defaultTexture, loadImmediately, usePackedTextureManager, pManagerName, backgroundLoadable);
}

static inline bool isValidHandleNonce(uint nonce)
{
   return ((nonce & 0xFFFFU) == ((~nonce) >> 16U));
}

void BD3DTextureManager::getOrCreateHandleCallback(void* pData)
{
   BD3DTextureManager::BGetOrCreateRequestParams* pRequest = static_cast<BGetOrCreateRequestParams*>(pData);
   BD3DTextureManager* pManager = pRequest->mpManager;
   BD3DTextureManager::BManagedTextureHandleStruct* pTex = pRequest->mpHandle;
   BASSERT(isValidHandleNonce(pTex->mNonce));
   
   pTex->mpManagedTexture = pManager->getOrCreate(
      pRequest->mFilenames, pRequest->mFileOpenFlags, pRequest->mMembershipBits, 
      pRequest->mSRGBTexture, pRequest->mDefaultTexture, pRequest->mLoadImmediately, 
      pRequest->mUsePackedTextureManager, pRequest->mManagerName, pRequest->mBackgroundLoadable);      

   HEAP_DELETE(pRequest, gRenderHeap);
}

BManagedTextureHandle BD3DTextureManager::getOrCreateHandle(
   const BRenderStringArray& filenames,
   BFileOpenFlags fileOpenFlags,
   DWORD membershipBits,
   bool srgbTexture,
   eDefaultTexture defaultTexture,
   bool loadImmediately,
   bool usePackedTextureManager,
   const char* pManagerName,
   bool backgroundLoadable)
{
   if (filenames.isEmpty())
      return cInvalidManagedTextureHandle;
      
   BManagedTextureHandleStruct* pTex = new(gRenderHeap) BManagedTextureHandleStruct;
   pTex->mNonce = RandomUtils::GenerateNonce32();
   pTex->mNonce = (pTex->mNonce & 0xFFFFU) | (((~pTex->mNonce) & 0xFFFFU) << 16U);
   BASSERT(isValidHandleNonce(pTex->mNonce));
               
   InterlockedIncrement(&mTotalManagedTextureHandles);
   
   if (gEventDispatcher.getThreadIndex() == cThreadIndexSim)
   {
      pTex->mpManagedTexture = NULL;
      
      BGetOrCreateRequestParams* pRequest = new(gRenderHeap) BGetOrCreateRequestParams;
      
      pRequest->mpManager = this;
      pRequest->mpHandle = pTex;
      pRequest->mFilenames = filenames;
      pRequest->mMembershipBits = membershipBits;
      pRequest->mManagerName.set(pManagerName);
      pRequest->mFileOpenFlags = fileOpenFlags;
      pRequest->mDefaultTexture = defaultTexture;
      pRequest->mSRGBTexture = srgbTexture;
      pRequest->mLoadImmediately = loadImmediately;
      pRequest->mUsePackedTextureManager = usePackedTextureManager;
      pRequest->mBackgroundLoadable = backgroundLoadable;
      pRequest->mDefaultTexture = defaultTexture;
      
      gRenderThread.submitCallback(getOrCreateHandleCallback, pRequest);
   }
   else if (gEventDispatcher.getThreadIndex() == cThreadIndexRender)
      pTex->mpManagedTexture = getOrCreate(filenames, fileOpenFlags, membershipBits, srgbTexture, defaultTexture, loadImmediately, usePackedTextureManager, pManagerName, backgroundLoadable);
   else
      return NULL;
      
   return ((uint64)((uint)pTex)) | (((uint64)pTex->mNonce) << 32U);
}

BManagedTextureHandle BD3DTextureManager::associateTextureWithHandle(BManagedTexture* pManagedTex)
{
   if (!pManagedTex)
      return cInvalidManagedTextureHandle;

   BManagedTextureHandleStruct* pTex = new(gRenderHeap) BManagedTextureHandleStruct;
   pTex->mNonce = RandomUtils::GenerateNonce32();
   pTex->mNonce = (pTex->mNonce & 0xFFFFU) | (((~pTex->mNonce) & 0xFFFFU) << 16U);
   BASSERT(isValidHandleNonce(pTex->mNonce));
   
   InterlockedIncrement(&mTotalManagedTextureHandles);

   pTex->mpManagedTexture = pManagedTex;
   pManagedTex->addRef();

   return ((uint64)((uint)pTex)) | (((uint64)pTex->mNonce) << 32U);
}

bool BD3DTextureManager::isValidManagedTextureHandle(BManagedTextureHandle handle)
{
   if ((handle == cInvalidManagedTextureHandle) || (handle & 3))
      return false;
   
   if ((handle & UINT_MAX) < 0x10000)
      return false;

   const uint handleNonce = (uint)((uint64)handle >> 32U);
   if (!isValidHandleNonce(handleNonce))
      return false;

   BManagedTextureHandleStruct* pTex = (BManagedTextureHandleStruct*)(handle & UINT_MAX);

#ifdef BUILD_DEBUG
   __try
   {
      if (handleNonce != pTex->mNonce)
         return false;
   }
   __except(EXCEPTION_EXECUTE_HANDLER)
   {
      return false;
   }
#else   
   if (handleNonce != pTex->mNonce)
      return false;
#endif      
            
   return true;      
}

BD3DTextureManager::BManagedTexture* BD3DTextureManager::getManagedTextureByHandle(BManagedTextureHandle handle)
{
   bool isValid = isValidManagedTextureHandle(handle);
   BASSERT(isValid);
   if (!isValid)
      return NULL;
   return ((BManagedTextureHandleStruct*)((uint)handle))->mpManagedTexture;
}

void BD3DTextureManager::loadManagedTextureByHandleCallback(void* pData)
{
   BManagedTextureHandleStruct* pManagedTextureHandleStruct = static_cast<BManagedTextureHandleStruct*>(pData);
   BD3DTextureManager::BManagedTexture* pTex = static_cast<BD3DTextureManager::BManagedTexture*>(pManagedTextureHandleStruct->mpManagedTexture);
   BASSERT(pTex);
   pTex->load();
}

bool BD3DTextureManager::loadManagedTextureByHandle(BManagedTextureHandle handle)
{
   if (gEventDispatcher.getThreadIndex() == cThreadIndexSim)      
   {
      bool isValid = isValidManagedTextureHandle(handle);
      BASSERT(isValid);
      if (!isValid)
         return false;
      gRenderThread.submitCallback(loadManagedTextureByHandleCallback, (void*)((uint)handle));
      return true;
   }
   else if (gEventDispatcher.getThreadIndex() == cThreadIndexRender)
   {
      BD3DTextureManager::BManagedTexture* pTex = getManagedTextureByHandle(handle);
      if (!pTex)
         return false;
         
      return pTex->load();
   }

   return false;
}

void BD3DTextureManager::unloadManagedTextureByHandleCallback(void* pData)
{
   BManagedTextureHandleStruct* pManagedTextureHandleStruct = static_cast<BManagedTextureHandleStruct*>(pData);
   BD3DTextureManager::BManagedTexture* pTex = static_cast<BD3DTextureManager::BManagedTexture*>(pManagedTextureHandleStruct->mpManagedTexture);
   BASSERT(pTex);
   pTex->unload();
}

bool BD3DTextureManager::unloadManagedTextureByHandle(BManagedTextureHandle handle)
{
   if (gEventDispatcher.getThreadIndex() == cThreadIndexSim)      
   {
      bool isValid = isValidManagedTextureHandle(handle);
      BASSERT(isValid);
      if (!isValid)
         return false;
      gRenderThread.submitCallback(unloadManagedTextureByHandleCallback, (void*)((uint)handle));
      return true;
   }
   else if (gEventDispatcher.getThreadIndex() == cThreadIndexRender)
   {
      BD3DTextureManager::BManagedTexture* pTex = getManagedTextureByHandle(handle);
      if (!pTex)
         return false;
         
      pTex->unload();
      return true;
   }

   return false;
}

void BD3DTextureManager::releaseManagedTextureByHandleCallback(void* pData)
{
   BManagedTextureHandleStruct* pManagedTextureHandleStruct = static_cast<BManagedTextureHandleStruct*>(pData);
   BASSERT(isValidHandleNonce(pManagedTextureHandleStruct->mNonce));
      
   BD3DTextureManager::BManagedTexture* pTex = pManagedTextureHandleStruct->mpManagedTexture;
   
   InterlockedDecrement(&pTex->getTextureManager()->mTotalManagedTextureHandles);
   
   pTex->release();

   pManagedTextureHandleStruct->mNonce = 0;
   pManagedTextureHandleStruct->mpManagedTexture = NULL;
   
   HEAP_DELETE(pManagedTextureHandleStruct, gRenderHeap);
}

bool BD3DTextureManager::releaseManagedTextureByHandle(BManagedTextureHandle handle)
{
   if (gEventDispatcher.getThreadIndex() == cThreadIndexSim)      
   {
      bool isValid = isValidManagedTextureHandle(handle);
      BASSERT(isValid);
      if (!isValid)
         return false;
      gRenderThread.submitCallback(releaseManagedTextureByHandleCallback, (void*)((uint)handle));
      return true;
   }
   else if (gEventDispatcher.getThreadIndex() == cThreadIndexRender)
   {
      BD3DTextureManager::BManagedTexture* pTex = getManagedTextureByHandle(handle);
      if (!pTex)
         return false;
         
      pTex->release();
      
      BManagedTextureHandleStruct* pManagedTextureHandleStruct = reinterpret_cast<BManagedTextureHandleStruct*>((uint)handle);
      
      pManagedTextureHandleStruct->mNonce = 0;
      pManagedTextureHandleStruct->mpManagedTexture = NULL;
      
      HEAP_DELETE(pManagedTextureHandleStruct, gRenderHeap);
      
      InterlockedDecrement(&mTotalManagedTextureHandles);
      
      return true;
   }

   return false;
}

void BD3DTextureManager::setManagedTextureByHandleCallback(void* pData)
{
   BSetManagedTextureCallbackData* p = static_cast<BSetManagedTextureCallbackData*>(pData);
   
   BD3D::mpDev->SetTexture(p->mSamplerIndex, p->mpTex->mpManagedTexture->getD3DTexture().getTexture());
}

bool BD3DTextureManager::setManagedTextureByHandle(BManagedTextureHandle handle, uint samplerIndex)
{
   if (gEventDispatcher.getThreadIndex() == cThreadIndexSim)      
   {
      bool isValid = isValidManagedTextureHandle(handle);
      BASSERT(isValid);
      if (!isValid)
         return false;
      BSetManagedTextureCallbackData data;
      data.mpTex = (BD3DTextureManager::BManagedTextureHandleStruct*)((uint)handle);
      data.mSamplerIndex = samplerIndex;
      gRenderThread.submitCallbackWithData(setManagedTextureByHandleCallback, sizeof(data), &data);
      return true;
   }
   else if (gEventDispatcher.getThreadIndex() == cThreadIndexRender)
   {
      BD3DTextureManager::BManagedTexture* pTex = getManagedTextureByHandle(handle);
      if (!pTex)
         return false;
      BD3D::mpDev->SetTexture(samplerIndex, pTex->getD3DTexture().getTexture());
      return true;
   }

   return false; 
}

BManagedTextureHandle BD3DTextureManager::getDefaultTextureHandle(eDefaultTexture defaultTexture) 
{
   BDEBUG_ASSERT((defaultTexture >= 0) && (defaultTexture < cDefaultTextureMax));

   BDEBUG_ASSERT(mDefaultTextures[defaultTexture] != cInvalidManagedTextureHandle);

   return mDefaultTextures[defaultTexture];
}

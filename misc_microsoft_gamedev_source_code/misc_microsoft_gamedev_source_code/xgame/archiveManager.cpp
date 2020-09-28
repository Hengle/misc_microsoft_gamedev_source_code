//==============================================================================
// File: archiveManager.cpp
//
// Copyright (c) 2006-2007 Ensemble Studios
//==============================================================================
#include "common.h"
#include "archiveManager.h"
#include "d3dTextureManager.h"
#include "renderThread.h"
#include "asyncFileManager.h"
#include "gameDirectories.h"
#include "D3DTextureManager.h"
#include "threading\workDistributor.h"
#include "render.h"
#include "xboxTextureHeap.h"
#include "config.h"
#include "econfigenum.h"
#include "configsgame.h"
#include "flashgateway.h"
#include "database.h"

#ifdef ALLOCATION_LOGGER
   #include "memory\allocationLogger.h"
#endif       
 
BArchiveManager gArchiveManager;

#define ROOT_FILE_CACHE_FILENAME "d:\\root.era"

extern BHandle gRootFileCacheHandle;
extern BHandle gLocaleFileCacheHandle;

//==============================================================================
// BArchiveManager::BArchiveManager
//==============================================================================
BArchiveManager::BArchiveManager() :
   mInitialized(false),
   mArchivesEnabled(false),
   mScenarioArchiveHandle(NULL),
   mPregameUIArchiveHandle(NULL),
   mLoadingUIArchiveHandle(NULL),
   //mCivUNSCArchiveHandle(NULL),
   mMiniloadUIArchiveHandle(NULL),
   mRootArchiveHandle(NULL),
   mInGameUIArchiveHandle(NULL)
{
   mLocaleFilename.set("locale.era");     // this should get overridden by other systems
}

//==============================================================================
// BArchiveManager::~BArchiveManager
//==============================================================================
BArchiveManager::~BArchiveManager()
{

}

//==============================================================================
// BArchiveManager::init
//==============================================================================
void BArchiveManager::init(BHandle rootFileCacheHandle)
{
   mRootArchiveHandle = rootFileCacheHandle;
   mArchivesEnabled = ((gFileManager.getConfigFlags() & BFileManager::cWillBeUsingArchives) != 0);
   mInitialized = true;
}

//==============================================================================
// BArchiveManager::deinit
//==============================================================================
void BArchiveManager::deinit(void)
{
   mInitialized = false;
}

//==============================================================================
// BArchiveManager::beginGameInit
//==============================================================================
bool BArchiveManager::beginGameInit(void)
{  
   if (!mInitialized)
      return false;
   if (!mArchivesEnabled)
      return true;

   //CLM [11.21.08] load the mini-loading archive, and keep it around in memory
   loadMiniLoadArchive();

   BDEBUG_ASSERT(NULL == mPregameUIArchiveHandle);
   
   eFileManagerError error = gFileManager.addFileCache(PREGAME_UI_ARCHIVE_FILENAME, mPregameUIArchiveHandle, false, true);
   if (cFME_SUCCESS != error)
   {
      trace("BArchiveManager::beginGameInit: addFileCache() failed!");    
      return false;
   }

   return true;      
}

//==============================================================================
// BArchiveManager::endGameInit
//==============================================================================
bool BArchiveManager::endGameInit(void)
{
   sync();
         
   // Discard files from root file cache

   gFileManager.discardFiles(cDirFonts, "*.xpr", true);
   gFileManager.discardFiles(cDirFonts, "*.ddx", true);
   gFileManager.discardFiles(cDirFonts, "*.ugx", true);

   if (mMiniloadUIArchiveHandle)
   {
      gFileManager.removeFileCache(mMiniloadUIArchiveHandle);
      mMiniloadUIArchiveHandle = NULL;
   }

   mIsGameInitialized = true;
   
   return true;
}

//==============================================================================
// BArchiveManager::beginLoadingMenu
//==============================================================================
bool BArchiveManager::beginLoadingMenu(void)
{
  SCOPEDSAMPLE(BArchiveManager_beginLoadingMenu)
   if (!mInitialized)
      return false;

   if (!mArchivesEnabled)
      return true;

   if (!mLoadingUIArchiveHandle)   
   {
      eFileManagerError error = gFileManager.addFileCache(LOADING_UI_ARCHIVE_FILENAME, mLoadingUIArchiveHandle, false, true);
      if (cFME_SUCCESS != error)
      {
         trace("BArchiveManager::beginLoadingMenu: addFileCache() failed!");    
         return false;
      }
   }

   eFileManagerError error = gFileManager.enableFileCache(mLoadingUIArchiveHandle);
   if (cFME_SUCCESS != error)
   {
      trace("BArchiveManager::beginLoadingMenu: enableFileCache() failed!");    
      return false;
   }

   return true;
}

//==============================================================================
// BArchiveManager::endLoadingMenu
//==============================================================================
bool BArchiveManager::endLoadingMenu(void)
{
   if (!mInitialized)
      return false;

   if (!mArchivesEnabled)
      return true;

   sync();      

   eFileManagerError error = gFileManager.removeFileCache(mLoadingUIArchiveHandle);
   mLoadingUIArchiveHandle = NULL;

   if (cFME_SUCCESS != error)
   {
      trace("BArchiveManager::endLoadingMenu: removeFileCache() failed!");      
      return false;
   }

   if (gConfig.isDefined("UnloadRootArchive"))
      unloadRootArchive();

   return true;
}

//==============================================================================
// BArchiveManager::beginMenu
//==============================================================================
bool BArchiveManager::beginMenu(void)
{
   if (!mInitialized)
      return false;
   
   if (!mArchivesEnabled)
   {
      gFlashGateway.loadPregameUITextures();      
      return true;
   }
   
   if (gConfig.isDefined("UnloadRootArchive"))
      reloadRootArchive();

   if (!mPregameUIArchiveHandle)   
   {  
      eFileManagerError error = gFileManager.addFileCache(PREGAME_UI_ARCHIVE_FILENAME, mPregameUIArchiveHandle, false, true);
      if (cFME_SUCCESS != error)
      {
         trace("BArchiveManager::beginMenu: addFileCache() failed!");    
         return false;
      }
   }
   
   eFileManagerError error = gFileManager.enableFileCache(mPregameUIArchiveHandle);
   if (cFME_SUCCESS != error)
   {
      trace("BArchiveManager::beginMenu: enableFileCache() failed!");    
      return false;
   }

   gFlashGateway.loadPregameUITextures();   

   return true;
}

//==============================================================================
// BArchiveManager::endMenu
//==============================================================================
bool BArchiveManager::endMenu(void)
{
   if (!mInitialized)
      return false;

   gFlashGateway.unloadPregameUITextures();

   if (!mArchivesEnabled)
      return true;
      
   sync();      
      
   if(mPregameUIArchiveHandle)
   {
      eFileManagerError error = gFileManager.removeFileCache(mPregameUIArchiveHandle);
      mPregameUIArchiveHandle = NULL;

      if (cFME_SUCCESS != error)
      {
         trace("BArchiveManager::endMenu: removeFileCache() failed!");      
         return false;
      }
   }
     
   return true;
}

//==============================================================================
// BArchiveManager::beginScenarioPrefetch
//==============================================================================
bool BArchiveManager::beginScenarioPrefetch(const char* pFilename)
{
   SCOPEDSAMPLE(BArchiveManager_beginScenarioPrefetch)
   if (!mInitialized)
      return false;
      
   if (!mArchivesEnabled)
      return true;
      
   endScenarioLoad();
         
   mScenarioFilename.set(pFilename);

   BString archiveFilename(createArchiveFilename(pFilename));

   /*
   * CLM we load the UI archive first, as UIGame.xml is loaded before any of the scenario data is loaded.
   */
   eFileManagerError error = gFileManager.addFileCache(INGAME_UI_ARCHIVE_FILENAME, mInGameUIArchiveHandle, false, true);
   if (cFME_SUCCESS != error)
   {
      trace("BArchiveManager::beginScenarioPrefetch: enableFileCache() failed!");

      gFileManager.removeFileCache(mScenarioArchiveHandle);
      mScenarioArchiveHandle = NULL;
     
      return false;
   }

   error = gFileManager.addFileCache(archiveFilename, mScenarioArchiveHandle, false, true);
   if (cFME_SUCCESS != error)
   {
      trace("BArchiveManager::beginScenarioPrefetch: enableFileCache() failed!");
      return false;
   }


   return true;
}

//==============================================================================
// BArchiveManager::beginScenarioLoad
//==============================================================================
bool BArchiveManager::beginScenarioLoad(const char* pFilename)
{
   SCOPEDSAMPLE(BArchiveManager_beginScenarioLoad)

   if (!mInitialized)
      return false;
      
   if (!mArchivesEnabled)
      return true;
      
   if (mScenarioArchiveHandle)
   {
      if (mScenarioFilename != pFilename)
         endScenarioLoad();
   }
   
   BVERIFY(mScenarioArchiveHandle);

   if (cFME_SUCCESS != gFileManager.enableFileCache(mScenarioArchiveHandle))
   {
      trace("BArchiveManager::beginScenarioLoad: enableFileCache() failed!");
      return false;
   }


   BVERIFY(mInGameUIArchiveHandle);
   if (cFME_SUCCESS != gFileManager.enableFileCache(mInGameUIArchiveHandle))
   {
      trace("BArchiveManager::beginScenarioLoad: enableFileCache() failed!");    
      return false;
   }
      
   // We must sync here to let the async file manager finish fetching any effects, which need the D3D device.
   sync();
   
   return true;
}

//==============================================================================
// BArchiveManager::renderEndScenarioLoad
//==============================================================================
void BArchiveManager::renderEndScenarioLoad(void* pData)
{
   trace("BArchiveManager::renderEndScenarioLoad: begin gD3DTextureManager.loadAll()");
   
   BArchiveManager* p = (BArchiveManager*)pData;
      
   // load everything except for pregame ui stuff
   DWORD memberShipBitMask = 0xFFFFFFFF;
   memberShipBitMask &= ~BD3DTextureManager::cScaleformPreGame;   

   gD3DTextureManager.loadAll(memberShipBitMask);

   trace("BArchiveManager::renderEndScenarioLoad: done");
   
   p->mWorkerFinishedLoad.set();
}

//==============================================================================
// BArchiveManager::endScenarioLoad
//==============================================================================
bool BArchiveManager::endScenarioLoad(void)
{
  SCOPEDSAMPLE(BArchiveManager_endScenarioLoad)

   if (!mInitialized)
      return false;
      
#ifndef BUILD_FINAL      
  BTimer timer;
  timer.start();
#endif   

   trace("BArchiveManager::endScenarioLoad: Archive manager sync start");

   sync();
     
   if ((!mArchivesEnabled) || (!mScenarioArchiveHandle))
   {
#ifndef BUILD_FINAL            
      trace("BArchiveManager::endScenarioLoad: Sync time: %3.3f seconds", timer.getElapsedSeconds());
#endif   

      return true;
   }
   
#ifndef BUILD_FINAL
   if(!gConfig.isDefined(cConfigNoArchiveTexture))
#endif
   {
      trace("BArchiveManager::endScenarioLoad: Archive manager texture unpack");

      gRenderThread.submitCallback(renderEndScenarioLoad, this);
      gRenderThread.kickCommands();

      {
         SCOPEDSAMPLE(BArchiveManager_endScenarioLoad_waitAfterKick)
            gWorkDistributor.waitSingle(mWorkerFinishedLoad);
      }
   }
   
 
   
   trace("BArchiveManager::endScenarioLoad: Archive manager sync end");

#ifndef BUILD_FINAL            
   trace("BArchiveManager::endScenarioLoad: Sync time: %3.3f seconds", timer.getElapsedSeconds());
#endif   
   
   bool results = true;


   if (cFME_SUCCESS != gFileManager.removeFileCache(mInGameUIArchiveHandle))
   {
      trace("BArchiveManager::endScenarioLoad: removeFileCache() failed!");

      results = false;
   }
   mInGameUIArchiveHandle = NULL;

/*
   if (cFME_SUCCESS != gFileManager.removeFileCache(mCivUNSCArchiveHandle))
   {
      trace("BArchiveManager::endScenarioLoad: removeFileCache() failed!");

      results = false;
   }

   mCivUNSCArchiveHandle = NULL;

   if (!gConfig.isDefined(cConfigNoCovenantArchive))
   {
      if (cFME_SUCCESS != gFileManager.removeFileCache(mCivCOVNArchiveHandle))
      {
         trace("BArchiveManager::endScenarioLoad: removeFileCache() failed!");

         results = false;
      }

      mCivCOVNArchiveHandle = NULL;
   }
*/

   if (cFME_SUCCESS != gFileManager.removeFileCache(mScenarioArchiveHandle))
   {
      trace("BArchiveManager::endScenarioLoad: removeFileCache() failed!");
      
      results = false;
   }

   mScenarioArchiveHandle = NULL;
   
   return results;
}

//==============================================================================
// BArchiveManager::createArchiveFilename
//==============================================================================
void BArchiveManager::endScenario(void)
{
   sync();
   
   gpXboxTextureHeap->freeUnusedValleys();
}

//==============================================================================
// BArchiveManager::createArchiveFilename
//==============================================================================
BString BArchiveManager::createArchiveFilename(const char* pFilename)
{
   BString archiveFilename;
   strPathGetFilename(BString(pFilename), archiveFilename);
   while (strPathHasExtension(archiveFilename))
      archiveFilename.removeExtension();
      
   archiveFilename = "d:\\" + archiveFilename + ".era";
   
   return archiveFilename;
}

//==============================================================================
// BArchiveManager::sync
//==============================================================================
void BArchiveManager::sync(void)
{
   SCOPEDSAMPLE(BArchiveManager_sync)

   gEventDispatcher.pumpUntilThreadQueueEmpty(cThreadIndexRender);
   gRenderThread.blockUntilWorkerIdle();
   
   for (uint i = 0; i < 4; i++)
   {  
      SCOPEDSAMPLE(BArchiveManager_sync_syncallPump)
      gAsyncFileManager.syncAll();
      
      gEventDispatcher.pumpAllThreads(8, 8);
      
      for (uint threadIndex = cThreadIndexSim; threadIndex < cThreadIndexMax; threadIndex++)
         if (gEventDispatcher.getThreadId(static_cast<BThreadIndex>(threadIndex)))
            gEventDispatcher.pumpUntilThreadQueueEmpty(static_cast<BThreadIndex>(threadIndex));
            
      gRenderThread.blockUntilWorkerIdle();
   }
}

//==============================================================================
// BArchiveManager::beginInGameUIPrefetch
//==============================================================================
bool BArchiveManager::beginInGameUIPrefetch()
{
   SCOPEDSAMPLE(BArchiveManager_beginInGameUIPrefetch)
   if (!mInitialized)
      return false;

   if (!mArchivesEnabled)
      return true;

   BVERIFY(mInGameUIArchiveHandle == NULL);

   eFileManagerError error = gFileManager.addFileCache(INGAME_UI_ARCHIVE_FILENAME, mInGameUIArchiveHandle, false, true);
   if (cFME_SUCCESS != error)
   {
      trace("BArchiveManager::beginInGameUIPrefetch: enableFileCache() failed!");
      return false;
   }
   
   return true;
}

//==============================================================================
// BArchiveManager::sync
//==============================================================================
bool BArchiveManager::beginInGameUILoad()
{
   SCOPEDSAMPLE(BArchiveManager_beginInGameUILoad)

   if (!mInitialized)
      return false;

   if (!mArchivesEnabled)
      return true;   

   BVERIFY(mInGameUIArchiveHandle);

   if (cFME_SUCCESS != gFileManager.enableFileCache(mInGameUIArchiveHandle))
   {
      trace("BArchiveManager::beginInGameUILoad: enableFileCache() failed!");
      return false;
   }   
      
   return true;
}

//==============================================================================
// BArchiveManager::endInGameLoad
//==============================================================================
bool BArchiveManager::endInGameUILoad(void)
{
   SCOPEDSAMPLE(BArchiveManager_endInGameLoad)

   if (!mInitialized)
      return false;

#ifndef BUILD_FINAL      
   BTimer timer;
   timer.start();
#endif   

   trace("BArchiveManager::endInGameLoad: Archive manager sync start");
   
   //-- block to ensure that when this gets called from the sim thread it waits until the 
   //-- render thread is done.
   gRenderThread.blockUntilWorkerIdle();

   if ((!mArchivesEnabled) || (!mInGameUIArchiveHandle))
   {
#ifndef BUILD_FINAL            
      trace("BArchiveManager::endInGameLoad: Sync time: %3.3f seconds", timer.getElapsedSeconds());
#endif   

      return true;
   }
   
#ifndef BUILD_FINAL            
   trace("BArchiveManager::endInGameLoad: Sync time: %3.3f seconds", timer.getElapsedSeconds());
#endif   

   bool results = true;
   if (cFME_SUCCESS != gFileManager.removeFileCache(mInGameUIArchiveHandle))
   {
      trace("BArchiveManager::endInGameLoad: removeFileCache() failed!");

      results = false;
   }

   mInGameUIArchiveHandle = NULL;

   return results;
}

//==============================================================================
// BArchiveManager::unloadRootArchive
//==============================================================================
void BArchiveManager::unloadRootArchive()
{
   unloadLocaleArchive();

   if (gRootFileCacheHandle)
   {
      eFileManagerError error = gFileManager.removeFileCache(gRootFileCacheHandle);
      gRootFileCacheHandle = NULL;
      if (cFME_SUCCESS != error)
         trace("BArchiveManager::unloadRootArchive: removeFileCache() failed for root.era!");      
   }
}

//==============================================================================
// BArchiveManager::reloadRootArchive
//==============================================================================
void BArchiveManager::reloadRootArchive()
{
   reloadLocaleArchive();

   if (!gRootFileCacheHandle)
   {
      eFileManagerError result = gFileManager.addFileCache(ROOT_FILE_CACHE_FILENAME, gRootFileCacheHandle, false, true);
      if (result == cFME_SUCCESS)
      {
         gFileManager.enableFileCache(gRootFileCacheHandle);
         gDatabase.discardDatabaseFiles();
      }
      else
         trace("BArchiveManager::reloadRootArchive: failed to load root.era!");      
   }
}

//==============================================================================
// BArchiveManager::unloadLocaleArchive
//==============================================================================
void BArchiveManager::unloadLocaleArchive()
{
   if (gLocaleFileCacheHandle)
   {
      eFileManagerError error = gFileManager.removeFileCache(gLocaleFileCacheHandle);
      gLocaleFileCacheHandle = NULL;
      if (cFME_SUCCESS != error)
         trace("BArchiveManager::unloadRootArchive: removeFileCache() failed for locale.era!");      
   }
}

//==============================================================================
// BArchiveManager::reloadLocaleArchive
//==============================================================================
void BArchiveManager::reloadLocaleArchive()
{
   if (!gLocaleFileCacheHandle)
   {
      eFileManagerError result = gFileManager.addFileCache(mLocaleFilename, gLocaleFileCacheHandle, false, true);
      if (result == cFME_SUCCESS)
         gFileManager.enableFileCache(gLocaleFileCacheHandle);
      else
         trace("BArchiveManager::reloadRootArchive: failed to load locale.era!");      
   }
}

//==============================================================================
// BArchiveManager::loadMiniLoadArchive
//==============================================================================
void BArchiveManager::loadMiniLoadArchive(void)
{
   BDEBUG_ASSERT(NULL == mMiniloadUIArchiveHandle);

   eFileManagerError error = gFileManager.addFileCache(MINILOAD_UI_ARCHIVE_FILENAME, mMiniloadUIArchiveHandle, false, true);
   if (cFME_SUCCESS != error)
   {
      trace("BArchiveManager::loadMiniLoadArchive: addFileCache() failed!");    
   }

   error = gFileManager.enableFileCache(mMiniloadUIArchiveHandle);
   if (cFME_SUCCESS != error)
   {
      trace("BArchiveManager::loadMiniLoadArchive: enableFileCache() failed!");    
   }
};
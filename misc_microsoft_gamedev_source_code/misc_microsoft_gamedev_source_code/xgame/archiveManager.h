//==============================================================================
// File: archiveManager.h
//
// Copyright (c) 2006-2007 Ensemble Studios
//==============================================================================
#pragma once
#include "fileManager.h"

#define PREGAME_UI_ARCHIVE_FILENAME "d:\\pregameUI.era"
#define LOADING_UI_ARCHIVE_FILENAME "d:\\loadingUI.era"
#define CIV_UNSC_ARCHIVE_FILENAME "d:\\civUNSC.era"
#define CIV_COVN_ARCHIVE_FILENAME "d:\\civCOVN.era"
#define INGAME_UI_ARCHIVE_FILENAME "d:\\ingameUI.era"
#define MINILOAD_UI_ARCHIVE_FILENAME "d:\\miniloader.era"

class BArchiveManager
{
public:
   BArchiveManager();
   ~BArchiveManager();
   
   void init(BHandle rootFileCacheHandle); 
   void deinit(void);
   
   bool getArchivesEnabled(void) const { return mArchivesEnabled; }
   bool getInitialized(void) const { return mInitialized; }
   bool getIsGameInitialized(void) const { return mIsGameInitialized; }
   
   bool beginGameInit(void);
   bool endGameInit(void);
   
   bool beginMenu(void);
   bool endMenu(void);

   bool beginLoadingMenu(void);
   bool endLoadingMenu(void);

   
   bool beginScenarioPrefetch(const char* pFilename);
   bool beginScenarioLoad(const char* pFilename);
   bool endScenarioLoad(void);

   bool beginInGameUIPrefetch(void);
   bool beginInGameUILoad(void);
   bool endInGameUILoad(void);
   
   void endScenario(void);

   void unloadRootArchive(void);
   void reloadRootArchive(void);

   void unloadLocaleArchive(void);
   void reloadLocaleArchive(void);

   void loadMiniLoadArchive(void);

   void setLocaleFilename(const BString& localeFilename) { mLocaleFilename = localeFilename; }
   const BString& getLocaleFilename() const { return mLocaleFilename; }

private:
   BString           mScenarioFilename;
   BString           mLocaleFilename;

   BHandle           mRootArchiveHandle;   
   BHandle           mScenarioArchiveHandle;
   //BHandle           mCivUNSCArchiveHandle;
   BHandle           mMiniloadUIArchiveHandle;
   BHandle           mPregameUIArchiveHandle;
   BHandle           mLoadingUIArchiveHandle;
   BHandle           mInGameUIArchiveHandle;
   
   BWin32Event       mWorkerFinishedLoad;
   
   bool              mInitialized : 1;
   bool              mArchivesEnabled : 1;
   bool              mIsGameInitialized : 1;
   
   BString           createArchiveFilename(const char* pFilename);
   void              sync(void);
   
   static void       renderEndScenarioLoad(void* pData);
};

extern BArchiveManager gArchiveManager;

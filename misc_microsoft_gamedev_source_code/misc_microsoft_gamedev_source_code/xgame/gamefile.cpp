//==============================================================================
// gamefile.cpp
//
// Copyright (c) 2006-2008 Ensemble Studios
//==============================================================================

// Includes
#include "common.h"
#include "gamefile.h"
#include "gamefilemanifest.h"

// xgame
#include "chunker.h"
#include "config.h"
#include "configsgame.h"
#include "database.h"
#include "dataentry.h"
#include "game.h"
#include "gamesettings.h"
#include "modegame.h"
#include "modemanager.h"
#include "user.h"
#include "usermanager.h"
#include "utilities.h"
#include "ScenarioList.h"
#include "UIGlobals.h"
#include "fileUtils.h"
#include "econfigenum.h"
#include "campaignmanager.h"
#include "world.h"
#include "UIGameMenu.h"

#include "xLiveSystem.h"         //To be able to query for MP games active and the controlled player id

// xsystem
#include "bfileStream.h"
#include "xfs.h"

// xcore
#include "stream\byteStream.h"
#include "stream\dynamicStream.h"
#include "stream\encryptedStream.h"
#include "file\win32FileStream.h"
#include "file\xcontentStream.h"

// compression
#include "compressedStream.h"

// Version
// 12 - fixes BRecordUIUnitSound loading issue
// 13 - adds information about the user recording the game
//    - gamertag, xuid
//    - system time of recording
//    - user editable name
//    - user editable desc
//    - fixes BGameSettings::cMapIndex addition
// 14 - adds saving/restoring of configs that affect the sim
// 15 - adds saving/restoring GameMode game setting
// 16 - added record game version
// 17 - changes bool playGame setting to byte loadType setting
// 18 - save off game setting name and lookup index on load
// 19 - added more configs to save/load
// 20 - added EnableSubUpdating config
// 21 - added more configs
// 22 - added more configs
const DWORD BGameFile::cBaseVersion = 23;

// Constants
const char* const BGameFile::cKeyPhrase = "TehHaloz";

DWORD BGameFileRequest::mNextEventID = 1;

//==============================================================================
// BGameFile::BGameFile
//==============================================================================
BGameFile::BGameFile() :
   mBaseVersion(0),
   mLocalPlayerID(1),
   mFileSaving(false),
   mFileLoading(false),
   mMultiplayer(false),
   mFogOfWar(false),
   mSaveConfigPlatoonRadius(0.0f),
   mSaveConfigProjectionTime(0.0f),
   mSaveConfigOverrideGroundIKRange(0.0f),
   mSaveConfigOverrideGroundIKTiltFactor(0.0f),
   mSaveConfigGameSpeed(0.0f),
   mSaveConfigAIDisable(false),
   mSaveConfigAIShadow(false),
   mSaveConfigNoVismap(false),
   mSaveConfigNoRandomPlayerPlacement(false),
   mSaveConfigDisableOneBuilding(false),
   mSaveConfigBuildingQueue(false),
   mSaveConfigUseTestLeaders(false),
   mSaveConfigEnableFlight(false),
   mSaveConfigNoBirthAnims(false),
   mSaveConfigVeterancy(false),
   mSaveConfigTrueLOS(false),
   mSaveConfigNoDestruction(false),
   mSaveConfigCoopSharedResources(false),
   mSaveConfigMaxProjectileHeightForDecal(false),
   mSaveConfigEnableSubbreakage(false),
   mSaveConfigEnableThrowPart(false),
   mSaveConfigAllowAnimIsDirty(false),
   mSaveConfigNoVictoryCondition(false),
   mSaveConfigAIAutoDifficulty(false),
   mSaveConfigDemo(false),
   mSaveConfigAsyncWorldUpdate(false),
   mSaveConfigEnableHintSystem(false),
   mSaveConfigPercentFadeTimeCorpseSink(false),
   mSaveConfigCorpseSinkSpeed(false),
   mSaveConfigCorpseMinScale(false),
   mSaveConfigBlockOutsideBounds(false),
   mSaveConfigAINoAttack(false),
   mSaveConfigPassThroughOwnVehicles(false),
   mSaveConfigEnableCapturePointResourceSharing(false),   
   mSaveConfigNoUpdatePathingQuad(false),
   mSaveConfigSlaveUnitPosition(false),
   mSaveConfigTurning(false),
   mSaveConfigHumanAttackMove(false),
   mSaveConfigMoreNewMovement3(false),
   mSaveConfigOverrideGroundIK(false),
   mSaveConfigDriveWarthog(false),
   mSaveConfigEnableCorpses(false),
   mSaveConfigDisablePathingLimits(false),
   mSaveConfigDisableVelocityMatchingBySquadType(false),
   mSaveConfigActiveAbilities(false),
   mSaveConfigAlpha(false),
   mSaveConfigNoDamage(false),
   mSaveConfigIgnoreAllPlatoonmates(false),
   mSaveConfigClassicPlatoonGrouping(false),
   mSaveConfigNoShieldDamage(false),
   mSaveConfigEnableSubUpdating(false),
   mSaveConfigMPSubUpdating(false),
   mSaveConfigAlternateSubUpdating(false),
   mSaveConfigDynamicSubUpdateTime(false),
   mSaveConfigDecoupledUpdate(false),
   mSaveConfigNoFogMask(false),
   mXuid(INVALID_XUID),
   mDataOfs(0),
   mEventHandleIO(cInvalidEventReceiverHandle),
   mEventHandleSim(cInvalidEventReceiverHandle),
   mEventHandleMisc(cInvalidEventReceiverHandle),
   mGameFileType(-1),
   mGameDirID(0),
   mpGameSettings(NULL),
   mpStream(NULL),
   mpRawStream(NULL),
   mpEncryptRecStream(NULL),
   mpDecryptRecStream(NULL),
   mpInflateRecStream(NULL),
   mpDeflateRecStream(NULL),
   mpListMediaTask(NULL),
   mpListFriendsMediaTask(NULL),
   mpUploadMediaTask(NULL),
   mpDownloadMediaTask(NULL),
   mpDeleteMediaTask(NULL),
   mLastListTime(0),
   mListCRC(0),
   mListTTL(cDefaultMediaTTL),
   mListFriendsCRC(0),
   mListFriendsTTL(cDefaultMediaTTL),
   mSelectDeviceEventID(0),
   mNextTempID(1),
   mMaxMemorySize(0),
   mSearchID(0),
   mRefreshInProgress(0),
   mLastUpdate(0),
   mSaveToCacheDrive(false),
   mRecordToMemory(false),
   mCacheEnabled(false),
   mUseCacheDrive(false),
   mUseBFile(false),
   mUsingGameFileStream(false),
   mDebugging(false)
{
}

//==============================================================================
// BGameFile::~BGameFile
//==============================================================================
BGameFile::~BGameFile()
{
   if (mFileLoading || mFileSaving)
      close();
}

//==============================================================================
// Check if the cache drive is available
//==============================================================================
bool BGameFile::setup()
{
   mCacheEnabled = BXboxFileUtils::isCacheInitialized();

   // insure that the logs directory exists
   if (mCacheEnabled)
   {
      if (!CreateDirectory(mCachePrefix, NULL))
      {
         DWORD dwError = GetLastError();
         if (dwError != ERROR_ALREADY_EXISTS)
            mCacheEnabled = false;
      }
   }

   mSaveToCacheDrive = gConfig.isDefined(cConfigRecordToCacheDrive);

   if (mSaveToCacheDrive && mCacheEnabled)
      mUseCacheDrive = true;

   if (!mUseCacheDrive)
      mUseBFile = true;

#ifdef BUILD_FINAL
   if (mCacheEnabled)
      mUseCacheDrive = true;
   mUseBFile = false;
#endif

   // for cracking the record game to read the manifest or saving
   mEventHandleIO = gEventDispatcher.addClient(this, cThreadIndexIO);
   mEventHandleSim = gEventDispatcher.addClient(this, cThreadIndexSim);
   // for enumeration
   mEventHandleMisc = gEventDispatcher.addClient(this, cThreadIndexMisc);

   mpGameSettings = new BGameSettings();
   if (!mpGameSettings->setup())
      return false;

#ifndef BUILD_FINAL
   // mDebugging is true by default but can be disabled from the config
   // for testing purposes
   mDebugging = true;

   long debug = 0;
   if (gConfig.get("GameFileDebug", &debug))
      mDebugging = (debug >= 0 ? true : false);
#endif

   long ttl;
   if (gConfig.get(cConfigLSPDefaultMediaTTL, &ttl))
   {
      mListTTL = static_cast<uint>(ttl);
      mListFriendsTTL = static_cast<uint>(ttl);
   }

   return true;
}

//==============================================================================
// 
//==============================================================================
void BGameFile::shutdown()
{
   if (mEventHandleIO != cInvalidEventReceiverHandle)
      gEventDispatcher.removeClientDeferred(mEventHandleIO, true);

   mEventHandleIO = cInvalidEventReceiverHandle;

   if (mEventHandleSim != cInvalidEventReceiverHandle)
      gEventDispatcher.removeClientDeferred(mEventHandleSim, true);

   mEventHandleSim = cInvalidEventReceiverHandle;

   if (mEventHandleMisc != cInvalidEventReceiverHandle)
      gEventDispatcher.removeClientDeferred(mEventHandleMisc, true);

   mEventHandleMisc = cInvalidEventReceiverHandle;

   delete mpGameSettings;
   mpGameSettings = NULL;

   if (mpListMediaTask != NULL)
      gLSPManager.terminateMediaTask(mpListMediaTask);
   mpListMediaTask = NULL;

   if (mpListFriendsMediaTask != NULL)
      gLSPManager.terminateMediaTask(mpListFriendsMediaTask);
   mpListFriendsMediaTask = NULL;

   if (mpUploadMediaTask != NULL)
      gLSPManager.terminateMediaTask(mpUploadMediaTask);
   mpUploadMediaTask = NULL;

   if (mpDownloadMediaTask != NULL)
      gLSPManager.terminateMediaTask(mpDownloadMediaTask);
   mpDownloadMediaTask = NULL;

   if (mpDeleteMediaTask != NULL)
      gLSPManager.terminateMediaTask(mpDeleteMediaTask);
   mpDeleteMediaTask = NULL;

   reset();

   // remove all the manifest entries
   uint count = mGameFiles.getSize();
   for (uint i=0; i < count; ++i)
   {
      BGameFileManifest* pManifest = mGameFiles[i];

      if (!pManifest)
         continue;

      delete pManifest;
   }

   mGameFiles.clear();
}

//==============================================================================
// 
//==============================================================================
void BGameFile::clean()
{
   uint count = mGameFiles.getSize();
   for (uint i=0; i < count; ++i)
   {
      BGameFileManifest* pManifest = mGameFiles[i];

      if (!pManifest)
         continue;

      delete pManifest;
   }

   mGameFiles.clear();

   mLastUpdate++;

   mNextTempID = 1;
}

//==============================================================================
// Initiate a new refresh of the saved recordings
// This will either allow the existing refresh to continue, or if the user
//    has changed, will cancel the old one and start a new one
//==============================================================================
void BGameFile::refresh(uint port)
{
   BGameFileManifest::mNextUpdateNumber++;

   // check for a signed-in user
//-- FIXING PREFIX BUG ID 3736
   const BUser* pUser = gUserManager.getUserByPort(port);
//--
   if (!pUser) //  || !pUser->isSignedIn())
   {
      // clean out all current record games
      clean();
      return;
   }

   bool server = true;

   // check if the user changed
   if (pUser->getXuid() != mXuid)
   {
      // clean out all the old record games
      clean();

      // need to shutdown the previous refresh operation
      // and instantiate a new one

      mXuid = pUser->getXuid();

      mLastListTime = timeGetTime();
   }
   else if (InterlockedCompareExchangeRelease(&mRefreshInProgress, 0, 0) > 0)
      return;
   else if ((timeGetTime() - mLastListTime < mListTTL) && (timeGetTime() - mLastListTime < mListFriendsTTL))
   {
      server = false;

      uint count = mGameFiles.getSize();
      for (uint i=0; i < count; ++i)
      {
         BGameFileManifest* pManifest = mGameFiles[i];
         if (pManifest && pManifest->getStorageType() == eStorageServer)
            pManifest->mUpdateNumber = BGameFileManifest::mNextUpdateNumber;
      }
   }
   else
      mLastListTime = timeGetTime();

   BUIGlobals* pUIGlobals = gGame.getUIGlobals();
   BDEBUG_ASSERT(pUIGlobals);
   if (pUIGlobals)
      pUIGlobals->showWaitDialog(gDatabase.getLocStringFromID(23503));

   InterlockedIncrementRelease(&mSearchID);

   // increment once for our cEventClassRefresh call
   InterlockedIncrementRelease(&mRefreshInProgress);

   // begin the refresh
   if (server)
   {
      if (refreshServer(pUser->getPort(), static_cast<uint>(mSearchID)))
         InterlockedIncrementRelease(&mRefreshInProgress); // increment again for the async server call

      if (refreshServerFriends(pUser->getPort(), static_cast<uint>(mSearchID)))
         InterlockedIncrementRelease(&mRefreshInProgress); // increment again for the async server call
   }

   gEventDispatcher.send(mEventHandleSim, mEventHandleMisc, cEventClassRefresh, pUser->getPort(), static_cast<uint>(mSearchID));
}

//==============================================================================
// 
//==============================================================================
void BGameFile::refreshComplete()
{
   if (InterlockedCompareExchangeRelease(&mRefreshInProgress, 0, 0) == 0)
      return;

   if (InterlockedDecrementRelease(&mRefreshInProgress) > 0)
      return;

   // cleanup our cache for files that were removed between scans
   uint count = mGameFiles.getSize();
   for (int i=count-1; i >= 0; --i)
   {
//-- FIXING PREFIX BUG ID 3737
      const BGameFileManifest* pManifest = mGameFiles[i];
//--
      if (!pManifest)
         mGameFiles.removeIndex(i);
      else if (pManifest->mUpdateNumber != BGameFileManifest::mNextUpdateNumber)
      {
         delete pManifest;
         mGameFiles.removeIndex(i);
      }
   }

   count = mGameFiles.getSize();
   for (uint i=0; i < count; ++i)
   {
      BGameFileManifest* pManifest = mGameFiles[i];
      if (pManifest->mStorageType == eStorageUser)
      {
         for (uint j=0; j < count; ++j)
         {
            BGameFileManifest* pCache = mGameFiles[j];
            if (pCache->mStorageType != eStorageUser && pManifest->getID() == pCache->getID())
               pCache->mSaved = true;
         }
      }
   }

   // sort the cache by file creation time
   mGameFiles.sort(BGameFileManifest::compareFunc);

   mLastUpdate++;

   BUIGlobals* pUIGlobals = gGame.getUIGlobals();
   BDEBUG_ASSERT(pUIGlobals);
   if (pUIGlobals)
      pUIGlobals->setWaitDialogVisible(false);
}

//==============================================================================
// Called each time we come across a new file from enumeration
// The enumeration happens on the IO thread and sends events to here
//==============================================================================
void BGameFile::enumerateResult(BGameFilePayload* pPayload)
{
   BDEBUG_ASSERT(pPayload);
   if (!pPayload)
      return;

   // convert BGameFilePayload into something useful across user storage
   // xfs/hdd and cache

   // scan our cache of record games to see if we already have this file
   uint count = mGameFiles.getSize();
   for (uint i=0; i < count; ++i)
   {
      if (mGameFiles[i]->mStorageType == pPayload->getStorageType() && pPayload->getFilename().compare(mGameFiles[i]->mFilename.getPtr()) == 0)
      {
         mGameFiles[i]->mUpdateNumber = BGameFileManifest::mNextUpdateNumber;
         break;
      }
   }

   if (i == count || pPayload->getRefresh())
   {
      // we do not yet have this file in our cache, so kick off a refresh request
      // to the IO thread
      gEventDispatcher.send(mEventHandleSim, mEventHandleIO, cEventClassRefreshManifest, 0, static_cast<uint>(mSearchID), pPayload);
   }
   else
   {
      delete pPayload;
   }
}

//==============================================================================
// 
//==============================================================================
void BGameFile::refreshResult(BGameFileManifest* pManifest)
{
   BDEBUG_ASSERT(pManifest);
   if (!pManifest)
      return;

   uint count = mGameFiles.getSize();
   for (uint i=0; i < count; ++i)
   {
      if (mGameFiles[i]->mStorageType == pManifest->getStorageType() && pManifest->getFilename().compare(mGameFiles[i]->mFilename.getPtr()) == 0)
      {
         mGameFiles[i]->mUpdateNumber = BGameFileManifest::mNextUpdateNumber;
         break;
      }
   }

   if (i == count || pManifest->getRefresh())
   {
      if (i != count)
      {
         BGameFileManifest* pTemp = mGameFiles[i];
         delete pTemp;
         mGameFiles.removeIndex(i);
      }

      pManifest->mUpdateNumber = BGameFileManifest::mNextUpdateNumber;

      mGameFiles.add(pManifest);

      mGameFiles.sort(BGameFileManifest::compareFunc);

      mLastUpdate++;
   }
   else
   {
      // we already have an instance of this record game, delete
      delete pManifest;
   }

   if (pManifest->getRefresh())
   {
      BUIGlobals* pUIGlobals = gGame.getUIGlobals();
      BDEBUG_ASSERT(pUIGlobals);
      if (pUIGlobals)
         pUIGlobals->setWaitDialogVisible(false);

      pManifest->setRefresh(false);
   }
}

//==============================================================================
// BGameFile::saveBase
//==============================================================================
bool BGameFile::saveBase()
{
   // cleanup old streams
   reset(); 

   BGameSettings* pSettings = gDatabase.getGameSettings();

   // initialize the header based on the primary user
   // and the current game settings
   if (!mHeader.init(pSettings))
      return false;

   // Fill in some additional info in the header which save games need.
   if (mGameFileType == cGameFileSave)
   {
      mHeader.setLength(gWorld ? (float)gWorld->getGametimeFloat() : 0.0f);
      long gameType = 0;
      pSettings->getLong(BGameSettings::cGameType, gameType);
      if (gameType == BGameSettings::cGameTypeCampaign)
      {
         BCampaign* pCampaign = gCampaignManager.getCampaign(0);
         if (pCampaign)
            mHeader.setSessionID(pCampaign->getSessionID());
      }
   }

   // this ID is not saved, only used as a reference point when editing/saving/viewing
   mHeader.mTempID = InterlockedIncrementRelease(&mNextTempID);

   BSimString name;

   calcName(name, pSettings);

   BDEBUG_ASSERT(name.length() <= 37);

   // init the rec/sync file names
   // further modification will occur when choosing a file system location
   mGameFileName.format("%s%s", name.getPtr(), mGameFileExt.getPtr());

   // reset the details header
   mDetails.mValue = 0;

   // get a stream pointing to cache/xfs/hdd
   // place protections around this so we only create the stream
   // for non-final builds and provided a config is set
   //
   // how should that work?  set a config, means I need to include it in the game.cfg/user.cfg
   // but that also means it could sneak into final builds
   //
   // will I want this functionality in final builds?
   BStream* pRecStream = NULL;
   mDetails.mCompressData = true;
   mDetails.mEncryptData = true;
   mDetails.mEncryptHeader = true;
#ifndef BUILD_FINAL
   pRecStream = getStream(mGameFileName, false);

   if (mGameFileType == cGameFileRecord)
   {
      if (!gConfig.isDefined("CompressGameFile"))
         mDetails.mCompressData = false;
      if (!gConfig.isDefined("EncryptGameFile"))
         mDetails.mEncryptData = false;
      if (!gConfig.isDefined("EncryptGameFileHeader"))
         mDetails.mEncryptHeader = false;
   }
#else
   if (gConfig.isDefined(cConfigRecordToCacheDrive))
      pRecStream = getStream(mGameFileName, false);
#endif

   long maxSize;
   if (mGameFileType == cGameFileRecord)
   {
      gConfig.get(cConfigRecordingMaxSize, &maxSize);
      maxSize = Math::Clamp<long>(maxSize, 400000, 1000000);
   }
   else
   {
      // ajl 11/19/08 - Add some safety space to the buffer size since we've had some save files that
      // have gone over our set buffer size recently. Campaign save files will be corrupt if that happens.
      maxSize = (long)(BSaveGame::MAX_SIZE_IN_BYTES * 1.5f);
   }

   mpStream = new BGameFileStream(pRecStream, cSFReadable | cSFWritable | cSFSeekable, maxSize, mGameFileName);

   if (mpStream == NULL)
      return false;

   mpRawStream = mpStream;

   mUsingGameFileStream = true;

   mBaseVersion = cBaseVersion;
   mDetails.mVersion = cBaseVersion;

   if (mpStream->writeBytes(&mDetails, sizeof(BGameFileVersion)) != sizeof(BGameFileVersion))
      return false;

   // Create the sha1 here so we can include the BGameFileVersion value in the hash.
   // The resulting hash is only for the header.
   // The record game data will be hashed after it has been compressed/encrypted.
   //
   // Note, the header will be re-hashed when we stop the recording, so it's not that
   // critical here but the code-paths are the same for header serialization.
   BSHA1Gen sha1Gen;

   sha1Gen.update(&mDetails, sizeof(BGameFileVersion));

   // serialize out the header
   if (mDetails.mEncryptHeader)
   {
      BEncryptedStream* pEncryptStream = new BEncryptedStream(mpStream, cKeyPhrase);

      if (!mHeader.serialize(pEncryptStream, sha1Gen))
      {
         delete pEncryptStream;
         return false;
      }

      delete pEncryptStream;
   }
   else
   {
      if (!mHeader.serialize(mpStream, sha1Gen))
         return false;
   }

   mDataOfs = mpStream->curOfs();

   if (mDetails.mEncryptData)
   {
      // create an encrypted stream
      uint64 k1, k2, k3;
      teaCryptInitKeys(cKeyPhrase, k1, k2, k3);
      mpEncryptRecStream = new BEncryptedStream(mpStream, k1, k2, mHeader.getKey3());
      mpStream = mpEncryptRecStream;
   }

   if (mDetails.mCompressData)
   {
      // create a compressed stream
      mpDeflateRecStream = new BDeflateStream(*mpStream);
      mpStream = mpDeflateRecStream;
   }

   DWORD checksum=0;//gGame.getLocalChecksum();
   mpStream->writeBytes(&checksum, sizeof(DWORD));

   bool mpGame=false;
   long localPlayerID=1;
   if (gLiveSystem->isMultiplayerGameActive())
   {
      mpGame = true;
      //TODO - look into the impact of multiple local players on record games - eric
      // [6/26/2008 DPM] this localPlayerID value is no longer used
      //localPlayerID = gLiveSystem->getLocalControlledPlayerID();
   }
   mpStream->writeBytes(&mpGame, sizeof(bool));
   mpStream->writeBytes(&localPlayerID, sizeof(long));

   DWORD settingCount = pSettings->getNumberEntries();
   mpStream->writeBytes(&settingCount, sizeof(DWORD));
   for (DWORD i=0; i<settingCount; i++)
   {
      uint8 nameLen = (uint8)strlen(gGameSettings[i].mName);
      mpStream->writeBytes(&nameLen, sizeof(uint8));
      mpStream->writeBytes(gGameSettings[i].mName, nameLen);

      long dataType = pSettings->getDataType(i);
      mpStream->writeBytes(&dataType, sizeof(long));

      switch (dataType)
      {
         case BDataEntry::cTypeLong:
         {
            long val=0;
            pSettings->getLong(i, val);
            mpStream->writeBytes(&val, sizeof(long));
            break;
         }
         case BDataEntry::cTypeBool:
         {
            bool val=0;
            pSettings->getBool(i, val);
            mpStream->writeBytes(&val, sizeof(bool));
            break;
         }
         case BDataEntry::cTypeByte:
         {
            BYTE val=0;
            pSettings->getBYTE(i, val);
            mpStream->writeBytes(&val, sizeof(BYTE));
            break;
         }
         case BDataEntry::cTypeString:
         {
            BFixedString256 val;
            pSettings->getString(i, val, 255);
            val[255]=NULL;
            uint8 len = (uint8)val.length();
            mpStream->writeBytes(&len, sizeof(uint8));
            if (len != 0)
               mpStream->writeBytes(val, len);
            break;
         }
         case BDataEntry::cTypeFloat:
         {
            float val=0;
            pSettings->getFloat(i, val);
            mpStream->writeBytes(&val, sizeof(float));
            break;
         }
         case BDataEntry::cTypeInt64:
         {
            int64 val=0;
            pSettings->getInt64(i, val);
            mpStream->writeBytes(&val, sizeof(int64));
            break;
         }
      }
   }

   BFixedStringMaxPath mapName;
   pSettings->getString(BGameSettings::cMapName, mapName, MAX_PATH);
   mapName += ".scn";
   checksum = 0;//getFileCRC32(cDirScenario, mapName, 0);
   mpStream->writeBytes(&checksum, sizeof(DWORD));

   bool val = gConfig.isDefined(cConfigNoFogMask);
   mpStream->writeBytes(&val, sizeof(bool));
   //val=gConfig.isDefined(cConfigNoVictoryCondition);
   //mpStream->writeBytes(&val, sizeof(bool));

   saveConfigs();

   mFileSaving = true;

   return true;
}

//==============================================================================
// 
//==============================================================================
bool BGameFile::loadBase(bool userStorage, XCONTENT_DATA* pContentData)
{
   if (!userStorage ||  !pContentData)
      reset();

   BGameSettings* pSettings = gDatabase.getGameSettings();

   BFixedStringMaxPath name;
   pSettings->getString(BGameSettings::cLoadName, name, MAX_PATH);
   const char* pName = name.getPtr();

   // Locate the manifest for the given name.
   //
   // The manifest was built earlier when viewing all the recordings.
   //
   // If loading from the config setting, AutoRecordGameLoad
   //    then you need to provide the path to the file.
   //
   // For XFS/HDD/Cache, full paths would look like:
   // Cache Drive  : cache:\recordgame\0711131738283259cc1ef5be1b4c22323.rec
   // XFS/HDD      : .\recordgame\0711131738283259cc1ef5be1b4c22323.rec
   //
   // User storage must go through the manifest since there are other
   // concerns about device location, user ownership, etc...
   //
   // I need the full path in the manifest because of duplicate file names
   // between the various storage devices
   //
   // also what happens when I copy a recording to a different device, does
   // it retain the existing file name?

   // attempt to convert the value of pName into an integer
   // manifest lookups are done via their temporary IDs
   // if the manifest is not found, it's going to assume you know the filename and open it
   BString strTempID(pName);
   uint tempID = static_cast<uint>(strTempID.asLong());

//-- FIXING PREFIX BUG ID 3738
   const BGameFileManifest* pManifest = NULL;
//--
   uint count = mGameFiles.getSize();
   for (uint i=0; i < count; ++i)
   {
      BGameFileManifest* pCheckManifest = mGameFiles[i];
      if (pCheckManifest && pCheckManifest->getTempID() == tempID)
      {
         pManifest = pCheckManifest;
         break;
      }
   }

   if (pManifest != NULL)
   {
      // creates a stream based on the information in the manifest
      mpStream = getStream(*pManifest);

      mGameFileName = pManifest->getFilename();
   }
   else
   {
      // this recording may be located on XFS/HDD/Cache
      mGameFileName = pName;
      mGameFileName.removeExtension();
      mGameFileName.append(mGameFileExt);

      //if (userStorage && pContentData)
      //   mpStream = getXContentStream(gUserManager.getPrimaryUser()->getPort(), *pContentData, cSFReadable | cSFOpenExisting | cSFSeekable | cSFOptimizeForSequentialAccess);
      //else
      if (!userStorage || !pContentData)
         mpStream = getStream(mGameFileName, true);
   }

   if (mpStream == NULL)
      return false;

   mpRawStream = mpStream;

   mUsingGameFileStream = false;

   mDetails.mValue = 0;

   if (mpStream->readBytes(&mDetails, sizeof(BGameFileVersion)) != sizeof(BGameFileVersion) || mDetails.mVersion > cBaseVersion)
      return false;

   #ifdef BUILD_FINAL
      // we will not allow unencrypted or uncompressed data in final builds
      if (!mDetails.mEncryptData || !mDetails.mEncryptHeader || !mDetails.mCompressData)
         return false;
   #endif

   mBaseVersion = mDetails.mVersion;

   BSHA1Gen sha1Gen;

   sha1Gen.update(&mDetails, sizeof(BGameFileVersion));

   // deserialize the header
   if (mDetails.mEncryptHeader)
   {
      BDecryptedStream* pDecryptStream = new BDecryptedStream(mpStream, cKeyPhrase);

      // force a base offset to that of the current stream
      pDecryptStream->setBaseOfs(mpStream->curOfs());

      if (!mHeader.deserialize(pDecryptStream, sha1Gen))
      {
         delete pDecryptStream;
         return false;
      }

      delete pDecryptStream;
   }
   else
   {
      if (!mHeader.deserialize(mpStream, sha1Gen))
         return false;
   }

   if (mDetails.mEncryptData)
   {
      uint64 k1, k2, k3;
      teaCryptInitKeys(cKeyPhrase, k1, k2, k3);
      mpDecryptRecStream = new BDecryptedStream(mpStream, k1, k2, mHeader.getKey3());
      mpDecryptRecStream->setBaseOfs(mpRawStream->curOfs());
      mpStream = mpDecryptRecStream;
   }

   if (mDetails.mCompressData)
   {
      mpInflateRecStream = new BInflateStream(*mpStream);
      mpStream = mpInflateRecStream;
   }

   DWORD checksum=0;
   mMultiplayer=false;
   mLocalPlayerID=1;
   if (!loadSettings(mBaseVersion, pSettings, mpStream, true, checksum, mMultiplayer, mLocalPlayerID))
      return false;

   checksum=0;
   if (mpStream->readBytes(&checksum, sizeof(DWORD)) != sizeof(DWORD))
      return false;

   mSaveConfigNoFogMask=gConfig.isDefined(cConfigNoFogMask);

   bool val=false;
   if (mpStream->readBytes(&val, sizeof(bool)) != sizeof(bool))
      return false;
   if(val)
      gConfig.define(cConfigNoFogMask);
   else
      gConfig.remove(cConfigNoFogMask);

   loadConfigs();

   mFogOfWar=!gConfig.isDefined(cConfigNoFogMask);

   mFileLoading = true;

   return true;
}

//==============================================================================
// BGameFile::close
//==============================================================================
void BGameFile::close()
{
   resetWrappers();

   if (mFileSaving)
   {
      if (mpRawStream)
      {
         // re-serialize out the header?
         // seek past the version
         BSHA1Gen sha1Gen;
         sha1Gen.update(&mDetails, sizeof(BGameFileVersion));

         // After I perform this seek on the raw stream, the encrypted/compressed streams
         //    will be invalid (if they exist).
         // I should cleanup/remove those streams after this point.  Or perhaps recreate them?
         if (mpRawStream->seek(sizeof(DWORD)) == sizeof(DWORD))
         {
            // record the data payload size for simple validity check when loading
            mHeader.mDataSize = mpRawStream->size() - mDataOfs;

            if (mDetails.mEncryptHeader)
            {
               BEncryptedStream* pStream = new BEncryptedStream(mpRawStream, cKeyPhrase);
               mHeader.serialize(pStream, sha1Gen);
               delete pStream;
            }
            else
               mHeader.serialize(mpRawStream, sha1Gen);
         }
      }

#ifndef BUILD_FINAL
      if (gConfig.isDefined("AutoCopyGames") && gXFS.isActive())
      {
         char destFileName[512];
         gXFS.setFailReportFileName(mGameFileName.getPtr(), destFileName, sizeof(destFileName));
         sprintf_s(destFileName, 512, "%s\\%s", BXFS::getFailReportDirectory(), mGameFileName.getPtr());
         HANDLE handle = gXFS.createFile(destFileName, GENERIC_WRITE, 0, 0, OPEN_ALWAYS, 0, 0);
         if (handle != INVALID_HANDLE_VALUE)
         {
            const int cBufSize = 65536*2;
            BByteArray buf(cBufSize);
            mpRawStream->seek(0);
            for ( ; ; )
            {
               const int n = mpRawStream->readBytes(buf.getPtr(), cBufSize);
               if (n == 0)
                  break;
               DWORD bytesWritten = 0;
               gXFS.writeFile(handle, buf.getPtr(), n, &bytesWritten, NULL);
            }
            gXFS.closeHandle(handle);
         }
      }
#endif

   }

   // close the raw stream to flush the underlying debug stream
   if (mpRawStream)
   {
      mpRawStream->close();
      // [11/20/2008 xemu] make sure we free this memory before we stomp over the pointer
      delete mpRawStream;
      mpRawStream = NULL;
   }
   mUsingGameFileStream = false;
   mpStream = NULL;

   mFileSaving = false;
   mFileLoading = false;
}

//==============================================================================
// Determines whether we're able to save the record game
// This doesn't take all things into account, we should still determine
// whether the record game has already been saved
//==============================================================================
bool BGameFile::canSaveFile(uint port) const
{
   // we don't have a record game to save
   if (mpStream == NULL)
      return false;

   if (mUsingGameFileStream)
   {
      // make sure we have a complete stream
//-- FIXING PREFIX BUG ID 3739
      const BGameFileStream* pGameFileStream = reinterpret_cast<BGameFileStream*>(mpStream);
//--
      if (pGameFileStream->isOutOfMemory())
         return false;
   }

   if (mSaveRequest.mInProgress)
      return false;

//-- FIXING PREFIX BUG ID 3740
   const BUser* pUser = gUserManager.getUserByPort(port);
//--
   if (!pUser || !pUser->isSignedIn())
      return false;

   return true;
}

//==============================================================================
// 
//==============================================================================
bool BGameFile::uploadFile(uint userIndex, uint manifestTempID)
{
   // temporary for testing...
   //if (mpUploadMediaTask != NULL)
   //{
   //   gLSPManager.terminateMediaTask(mpUploadMediaTask);
   //   mpUploadMediaTask = NULL;
   //}

   if (mpUploadMediaTask != NULL || mpDownloadMediaTask != NULL)
      return false;

   // FIXME - need UI helper dialogs to inform the user that they're currently
   // not signed-in with a Live enabled account
   //
   // hoping the end result is we don't display the "sharing" portion of the UI
   // at all
   //
//-- FIXING PREFIX BUG ID 3741
   const BUser* pUser = gUserManager.getUserByPort(userIndex);
//--
   if (!pUser || !pUser->isSignedIntoLive())
      return false;

   // find the manifest for the given ID
   BGameFileManifest* pManifest = NULL;

   uint count = mGameFiles.getSize();
   for (uint i=0; i < count; ++i)
   {
      BGameFileManifest* pTemp = mGameFiles[i];
      if (pTemp && pTemp->getTempID() == manifestTempID && pTemp->getStorageType() == eStorageUser)
      {
         pManifest = pTemp;
         break;
      }
   }

   // the recording must exist in the manifest for us to upload
   //if (!pManifest || !pManifest->getContent() || pManifest->getAuthorXuid() != pUser->getXuid())
   // allow content to be uploaded into a user's personal storage space even if they're not the author
   // XXX we may need to get a TCR exception for this, or we may not want it as by design?
   if (!pManifest || !pManifest->getContent())
      return false;

   // the recording must exist in User Storage
   if (pManifest->getStorageType() != eStorageUser)
      return false;

   // need to create an LSP Media Share upload request
   //
   // need to allow for queuing multiple uploads
   // need to query for what upload(s) are in-progress
   // need an identifier for each request so I can display something
   //    intelligently in the UI
   //
   // these requirements also apply to downloads
   //
   // other types of uploads are screenshots
   //
   // the uploader/downloader should be generic to handle record games and screenshots
   //
   // both media types will come from storage, we will not upload from memory
   //
   // the LSP code will need intimate knowledge of the media transfer objects
   // * will need to know the location of the media
   // * only allow uploads from User Storage

   // The transfer request will require the XCONTENT_DATA structure in order to properly access the media

   mpUploadMediaTask = gLSPManager.createMediaTask();
   if (mpUploadMediaTask == NULL)
      return false;

   SYSTEMTIME time = pManifest->getDate();

   BDynamicStream* pStream = new BDynamicStream();
   pStream->printf("<media><r id='%I64u' l='%f' s='%I64u' t='%04d%02d%02dT%02d:%02d:%02dZ' mid='%d' x='%I64u'><n>%S</n><d>%S</d><o>%s</o></r></media>",
      pManifest->getID(),
      pManifest->getLength(),
      pManifest->getFileSize(),
      time.wYear, time.wMonth, time.wDay, time.wHour, time.wMinute, time.wSecond,
      pManifest->getMapStringID(),
      pManifest->getAuthorXuid(),
      pManifest->getName().getPtr(),
      pManifest->getDesc().getPtr(),
      pManifest->getAuthor().getPtr());

   mpUploadMediaTask->init(BLSPMediaTask::cCommandUpload, 10000, pUser->getPort(), pUser->getXuid(), this, pManifest->getID(), *pManifest->getContent(), pStream);

   mMediaTasks.add(mpUploadMediaTask);

   pManifest->setMediaTaskID(mpUploadMediaTask->getID());

   return true;
}

//==============================================================================
// 
//==============================================================================
bool BGameFile::downloadFile(uint userIndex, uint manifestTempID)
{
   // temporary for testing...
   //if (mpDownloadMediaTask != NULL)
   //{
   //   gLSPManager.terminateMediaTask(mpDownloadMediaTask);
   //   mpDownloadMediaTask = NULL;
   //}

   if (mpUploadMediaTask != NULL || mpDownloadMediaTask != NULL)
      return false;

   // FIXME - need UI helper dialogs to inform the user that they're currently
   // not signed-in with a Live enabled account
   //
   // hoping the end result is we don't display the "sharing" portion of the UI
   // at all
   //
   BUser* pUser = gUserManager.getUserByPort(userIndex);
   if (!pUser || !pUser->isSignedIntoLive())
      return false;

   // find the manifest for the given ID
   BGameFileManifest* pManifest = NULL;

   uint count = mGameFiles.getSize();
   for (uint i=0; i < count; ++i)
   {
      BGameFileManifest* pTemp = mGameFiles[i];
      if (pTemp && pTemp->getTempID() == manifestTempID && pTemp->getStorageType() == eStorageServer)
      {
         pManifest = pTemp;
         break;
      }
   }

   // the recording must exist in the manifest for us to download
   if (!pManifest)
      return false;

   // the recording must exist in Server Storage
   if (pManifest->getStorageType() != eStorageServer)
      return false;

   mSaveRequest.mInProgress = true;
   mSaveRequest.mUserIndex = userIndex;
   mSaveRequest.mTempID = manifestTempID;
   mSaveRequest.mEventID = mSelectDeviceEventID = BGameFileRequest::mNextEventID++;

   if (pUser->getDefaultDevice() == XCONTENTDEVICE_ANY)
   {
      // wait for the notify callback
      if (!gUserManager.showDeviceSelector(pUser, this, mSelectDeviceEventID))
      {
         mSaveRequest.mInProgress = false;
         return false;
      }
      return true;
   }

   mpDownloadMediaTask = gLSPManager.createMediaTask();
   if (mpDownloadMediaTask == NULL)
      return false;

   mpDownloadMediaTask->init(BLSPMediaTask::cCommandDownload, 10000, pUser->getPort(), pUser->getXuid(), this, pManifest->getID());

   mMediaTasks.add(mpDownloadMediaTask);

   pManifest->setMediaTaskID(mpDownloadMediaTask->getID());

   return true;
}

//==============================================================================
// 
//==============================================================================
bool BGameFile::deleteFile(uint port, uint id)
{
   if (mDeleteRequest.mInProgress)
      return false;

   // FIXME - need UI helper dialogs to inform the user that they're currently
   // not signed-in with a Live enabled account
   //
   // hoping the end result is we don't display the "sharing" portion of the UI
   // at all
   //
//-- FIXING PREFIX BUG ID 3742
   const BUser* pUser = gUserManager.getUserByPort(port);
//--
   if (!pUser || !pUser->isSignedIn())
      return false;

   // find the manifest for the given ID
//-- FIXING PREFIX BUG ID 3743
   const BGameFileManifest* pManifest = NULL;
//--

   uint count = mGameFiles.getSize();
   for (uint i=0; i < count; ++i)
   {
      BGameFileManifest* pTemp = mGameFiles[i];
      if (pTemp && pTemp->getTempID() == id)
      {
         pManifest = pTemp;
         break;
      }
   }

   // the recording must exist in the manifest for us to upload
   if (!pManifest)
      return false;

   // need to remove this item from the Cache Drive, User Storage
   switch (pManifest->getStorageType())
   {
      case eStorageUser:
      case eStorageCache:
      case eStorageServer:
         {
            break;
         }
      default:
         {
            return false;
         }
   }

   mDeleteRequest.mRefreshCache = false;

   mDeleteRequest.mID = pManifest->getID();
   mDeleteRequest.mTempID = pManifest->getTempID();
   mDeleteRequest.mUserIndex = pUser->getPort();

   if (pManifest->getContent())
      Utils::FastMemCpy(&mDeleteRequest.mContent, pManifest->getContent(), sizeof(XCONTENT_DATA));

   mDeleteRequest.mSourceFilename = pManifest->getFilename();

   BUIGlobals* pUIGlobals = gGame.getUIGlobals();
   BDEBUG_ASSERT(pUIGlobals);
   if (pUIGlobals)
      pUIGlobals->showWaitDialog(gDatabase.getLocStringFromID(23501));

   if (pManifest->getStorageType() == eStorageServer)
   {
      // issue a delete request to the server
      // issue a list command to the server
      if (mpDeleteMediaTask != NULL)
      {
         // temporary allowing us to reset for another try
         gLSPManager.terminateMediaTask(mpDeleteMediaTask);
         mpDeleteMediaTask = NULL;
         //return false;
      }

      if (!pUser->isSignedIntoLive())
         return false;

      mpDeleteMediaTask = gLSPManager.createMediaTask();
      if (mpDeleteMediaTask == NULL)
         return false;

      mpDeleteMediaTask->init(BLSPMediaTask::cCommandDelete, 10000, pUser->getPort(), pUser->getXuid(), this, pManifest->getID());

      mMediaTasks.add(mpDeleteMediaTask);
   }
   else
   {
      // let the IO thread handle the removal of the file
      BGameFilePayload* pPayload = new BGameFilePayload(port, pManifest->getStorageType(), mDeleteRequest.mContent, pManifest->getFilename());
      gEventDispatcher.send(mEventHandleSim, mEventHandleIO, cEventClassDelete, 0, 0, pPayload);
   }

   mDeleteRequest.mInProgress = true;

   return true;
}

//==============================================================================
// 
//==============================================================================
void BGameFile::editFile(uint port, uint id, BEditType edit)
{
   // verify that we're not already saving a record game
   if (mSaveRequest.mInProgress)
      return;

   // the current controlling user has requested to edit the recording of the given ID

//-- FIXING PREFIX BUG ID 3744
   const BUser* pUser = gUserManager.getUserByPort(port);
//--
   if (!pUser || !pUser->isSignedIn())
      return;

   // find the manifest for the given ID
//-- FIXING PREFIX BUG ID 3745
   const BGameFileManifest* pManifest = NULL;
//--

   uint count = mGameFiles.getSize();
   for (uint i=0; i < count; ++i)
   {
      BGameFileManifest* pTemp = mGameFiles[i];
      if (pTemp && pTemp->getTempID() == id && pTemp->getStorageType() == eStorageUser)
      {
         pManifest = pTemp;
         break;
      }
   }

   // the recording must exist in the manifest for us to edit
   if (!pManifest || !pManifest->getContent()) // || pManifest->getAuthorXuid() != pUser->getXuid())
      return;

   mSaveRequest.mInProgress = true;

   mSaveRequest.mRefreshCache = true;
   mSaveRequest.mSaveGame = false;

   // the validity of getContent() is checked earlier
   BGameFilePayload* pPayload = new BGameFilePayload(pUser->getPort(), id, *pManifest->getContent());

   // cache the current name/desc information into the payload for editing
   pPayload->mName = pManifest->getName();
   pPayload->mDesc = pManifest->getDesc();

   gEventDispatcher.send(mEventHandleSim, mEventHandleMisc, cEventClassEdit, edit, 0, pPayload);
}

//==============================================================================
// Saves the current mpStream
//==============================================================================
bool BGameFile::saveFile(uint port)
{
   // we don't have a record game to save
   if (mpStream == NULL)
      return false;

   if (mUsingGameFileStream)
   {
      // make sure we have a complete stream
//-- FIXING PREFIX BUG ID 3746
      const BGameFileStream* pGameFileStream = reinterpret_cast<BGameFileStream*>(mpRawStream);
//--
      if (pGameFileStream->isOutOfMemory())
         return false;
   }

   if (mSaveRequest.mInProgress)
      return false;

   BUser* pUser = gUserManager.getUserByPort(port);
   if (!pUser || !pUser->isSignedIn())
      return false;

   mSaveRequest.mInProgress = true;

   mSaveRequest.mRefreshCache = false;
   mSaveRequest.mSaveGame = false;

   // need to figure out how I want to close the deflate/encrypt streams
   mSaveRequest.mpSourceStream = mpRawStream;
   mpStream = NULL;
   mpRawStream = NULL;

   // close off wrapper streams inflate, deflate, encrypt, decrypt
   resetWrappers();

   // need to cache the requested ID because we're going to be
   // waiting on all the async calls to complete
   mSaveRequest.mTempID = mHeader.mTempID;
   mSaveRequest.mUserIndex = pUser->getPort();

   Utils::FastMemSet(&mSaveRequest.mContent, 0, sizeof(XCONTENT_DATA));

   mSaveRequest.mContent.DeviceID = pUser->getDefaultDevice();
   mSaveRequest.mContent.dwContentType = XCONTENTTYPE_SAVEDGAME;

   // set a display name and file name
   StringCchCopyA(mSaveRequest.mContent.szFileName, XCONTENT_MAX_FILENAME_LENGTH, mGameFileName.getPtr());

   SYSTEMTIME headerTime = mHeader.mDate;

   FILETIME utcFileTime;
   if (SystemTimeToFileTime(&mHeader.mDate, &utcFileTime))
   {
      FILETIME localFileTime;
      if (FileTimeToLocalFileTime(&utcFileTime, &localFileTime))
      {
         FileTimeToSystemTime(&localFileTime, &headerTime);
      }
   }

   BUString dateTime;
   dateTime.format(L"%04d/%02d/%02d %02d:%02d:%02d - ", headerTime.wYear, headerTime.wMonth, headerTime.wDay, headerTime.wHour, headerTime.wMinute, headerTime.wSecond);

   uint32 len = static_cast<uint32>(mHeader.mLength * 1000.0f);

   // convert the length from milliseconds to hour:min:sec
   int hours = len / 3600000;
   len -= (hours * 3600000);
   int minutes = len / 60000;
   len -= (minutes * 60000);
   int seconds = len / 1000;

   BUString desc;

   // lookup the friendly name for campaign saves
   bool campaignSave = false;
   BCampaign* pCampaign = gCampaignManager.getCampaign(0);
   if (pCampaign)
   {
      BString buffer;
      const char* pName = mHeader.mName.asANSI(buffer);
      BCampaignNode* pNode;
      if (mHeader.mName == "NewCampaign")
      {
         long nodeID = 0;
         pNode = pCampaign->getNode(nodeID);
         while (pNode && pNode->getFlag(BCampaignNode::cCinematic))
         {
            nodeID++;
            pNode = pCampaign->getNode(nodeID);
         }
      }
      else
         pNode = pCampaign->getNode(pName);
      if (pNode)
      {
         campaignSave = true;
         desc.format(L"%s %02d:%02d:%02d", pNode->getDisplayName().getPtr(), hours, minutes, seconds);
      }
   }

   if (!campaignSave)
      desc.format(L"%s%s - Length %02d:%02d:%02d", dateTime.getPtr(), mHeader.mName.getPtr(), hours, minutes, seconds);

   StrHelperStringCchCopyW(mSaveRequest.mContent.szDisplayName, XCONTENT_MAX_DISPLAYNAME_LENGTH, desc.getPtr());

   BUIGlobals* pUIGlobals = gGame.getUIGlobals();
   BDEBUG_ASSERT(pUIGlobals);
   if (pUIGlobals)
      pUIGlobals->showWaitDialog(gDatabase.getLocStringFromID(23501));

   // FIXME the device ID could become invalid during any operation
   // but we currently don't have support to handle that level
   // of notifications in our callbacks
   XCONTENTDEVICEID deviceID = pUser->getDefaultDevice();
   if (deviceID != XCONTENTDEVICE_ANY)
   {
      XDEVICE_DATA deviceData;
      ZeroMemory(&deviceData, sizeof(XDEVICE_DATA));
      if (XContentGetDeviceData(deviceID, &deviceData) != ERROR_SUCCESS)
      {
         // ajl 11/18/08 - Don't reset the device on the user here because that causes the warning about the 
         // user removing the default storage device to not display.
         //pUser->setDefaultDevice(XCONTENTDEVICE_ANY);
         deviceID = XCONTENTDEVICE_ANY;
      }
   }
   if (deviceID == XCONTENTDEVICE_ANY)
   {
      if (campaignSave)
      {
         mSaveRequest.mInProgress = false;
         return false;
      }
      // wait for the notify callback
      mSaveRequest.mEventID = BGameFileRequest::mNextEventID++;
      if (!gUserManager.showDeviceSelector(pUser, this, mSaveRequest.mEventID))
         mSaveRequest.mInProgress = false;
      return true;
   }

   beginSaveFile();

   return true;
}

//==============================================================================
// 
//==============================================================================
void BGameFile::saveFile(uint port, uint id)
{
   // verify that we're not already saving a record game
   if (mSaveRequest.mInProgress)
      return;

   // the current controlling user has requested to save the recording of the given ID
   // I'll need the user's selected device ID and their userIndex (i.e. port)

   BUser* pUser = gUserManager.getUserByPort(port);
   if (!pUser || !pUser->isSignedIn())
      return;

   // find the manifest for the given ID and save the file to user storage
//-- FIXING PREFIX BUG ID 3747
   const BGameFileManifest* pManifest = NULL;
//--

   uint count = mGameFiles.getSize();
   for (uint i=0; i < count; ++i)
   {
      BGameFileManifest* pTemp = mGameFiles[i];
      if (pTemp && pTemp->getTempID() == id && !pTemp->isSaved())
      {
         pManifest = pTemp;
         break;
      }
   }

   if (!pManifest)
      return;

   //if (pManifest->getAuthorXuid() != pUser->getXuid())
   //   return;

   mSaveRequest.mInProgress = true;

   mSaveRequest.mRefreshCache = true;
   mSaveRequest.mSaveGame = false;

   mSaveRequest.mpSourceStream = NULL;
   mSaveRequest.mSourceFilename = pManifest->getFilename();
   mSaveRequest.mSourceStorageType = pManifest->getStorageType();

   // need to cache the requested ID because we're going to be
   // waiting on all the async calls to complete
   mSaveRequest.mTempID = id;
   mSaveRequest.mUserIndex = pUser->getPort();

   Utils::FastMemSet(&mSaveRequest.mContent, 0, sizeof(XCONTENT_DATA));

   mSaveRequest.mContent.DeviceID = pUser->getDefaultDevice();
   mSaveRequest.mContent.dwContentType = XCONTENTTYPE_SAVEDGAME;

   // set a display name and file name
   StringCchCopyA(mSaveRequest.mContent.szFileName, XCONTENT_MAX_FILENAME_LENGTH, pManifest->mFilename.getPtr());
   StrHelperStringCchCopyW(mSaveRequest.mContent.szDisplayName, XCONTENT_MAX_DISPLAYNAME_LENGTH, pManifest->getDesc().getPtr());

   BUIGlobals* pUIGlobals = gGame.getUIGlobals();
   BDEBUG_ASSERT(pUIGlobals);
   if (pUIGlobals)
      pUIGlobals->showWaitDialog(gDatabase.getLocStringFromID(23501));

   // FIXME the device ID could become invalid during any operation
   // but we currently don't have support to handle that level
   // of notifications in our callbacks
   if (pUser->getDefaultDevice() == XCONTENTDEVICE_ANY)
   {
      // wait for the notify callback
      mSaveRequest.mEventID = BGameFileRequest::mNextEventID++;
      if (!gUserManager.showDeviceSelector(pUser, this, mSaveRequest.mEventID))
         mSaveRequest.mInProgress = false;
      return;
   }

   beginSaveFile();
}

//==============================================================================
// 
//==============================================================================
void BGameFile::cancelSaveFile()
{
   // if the user signs-out or whatever, we need to cancel our outstanding operations
   // FIXME, if the device selector UI is open, we'll need to wait for that notification
   // to complete for us to "cancel" things
}

//==============================================================================
// 
//==============================================================================
void BGameFile::beginSaveFile()
{
   BGameFilePayload* pPayload = new BGameFilePayload(mSaveRequest.mUserIndex, mSaveRequest.mTempID, mSaveRequest.mContent, mSaveRequest.mpSourceStream);

   gEventDispatcher.send(mEventHandleSim, mEventHandleIO, cEventClassSave, 0, 0, pPayload);
}

//==============================================================================
// 
//==============================================================================
void BGameFile::endSaveFile(DWORD dwRet, BGameFilePayload* pPayload)
{
   // dismiss any dialogs
   if (dwRet == ERROR_SUCCESS)
   {
      uint count = mGameFiles.getSize();
      for (uint i=0; i < count; ++i)
      {
         BGameFileManifest* pManifest = mGameFiles[i];
         if (pManifest && pManifest->getTempID() == mSaveRequest.mTempID)
            pManifest->mSaved = true;
      }
   }

   // dismiss dialogs and refresh the UI
   mLastUpdate++;

   mSaveRequest.mInProgress = false;

   BUIGlobals* pUIGlobals = gGame.getUIGlobals();
   BDEBUG_ASSERT(pUIGlobals);
   if (pUIGlobals && !mDeleteRequest.mInProgress && !mSaveRequest.mSaveGame)
   {
      if (dwRet != ERROR_SUCCESS)
      {
         pUIGlobals->setWaitDialogVisible(false);

         // ajl 11/18/08 - Show a save error message only if we aren't already displaying another dialog. The other dialog is 
         // likely the one about the user removing the default storage device and we don't want to overwrite that.
         if (!gGame.getUIGlobals()->isYorNBoxVisible())
         {
            if (gModeManager.inModeGame())
               gGame.getUIGlobals()->showYornBoxSmall( NULL, gDatabase.getLocStringFromID(26002), BUIGlobals::cDialogButtonsOK );
            else
               gGame.getUIGlobals()->showYornBox( NULL, gDatabase.getLocStringFromID(25997), BUIGlobals::cDialogButtonsOK);
         }
      }
   }

   // I want to add our newly saved file to the cache,
   // currently the manifest represents the cache/xfs/hdd file
   // so how do I issue a new "scan/refresh" for an individual file
   // that I have not yet enumerated from the XContent APIs?
   //refresh();
   reset();

   if (dwRet == ERROR_SUCCESS && mSaveRequest.mRefreshCache)
   {
      gEventDispatcher.send(cInvalidEventReceiverHandle, mEventHandleSim, cEventClassContentEnumerate, pPayload->getUserIndex(), mSearchID, pPayload);
   }
   else
   {
      delete pPayload;
   }
}

//==============================================================================
// 
//==============================================================================
void BGameFile::endDeleteFile(DWORD dwRet, BGameFilePayload* pPayload)
{
   // dismiss any dialogs
   if (dwRet == ERROR_SUCCESS)
   {
      uint count = mGameFiles.getSize();
      for (int i=count-1; i >= 0; --i)
      {
         BGameFileManifest* pManifest = mGameFiles[i];
         if (pManifest)
         {
            if (pManifest->getID() == mDeleteRequest.mID)
            {
               pManifest->mSaved = false;
            }
            if (pManifest->getTempID() == mDeleteRequest.mTempID)
            {
               delete pManifest;
               mGameFiles.removeIndex(i);
            }
         }
      }
   }

   // dismiss dialogs and refresh the UI
   mLastUpdate++;

   mDeleteRequest.mInProgress = false;

   BUIGlobals* pUIGlobals = gGame.getUIGlobals();
   BDEBUG_ASSERT(pUIGlobals);
   if (pUIGlobals && !mSaveRequest.mInProgress)
      pUIGlobals->setWaitDialogVisible(false);

   reset();

   delete pPayload;

   //if (dwRet == ERROR_SUCCESS && mDeleteRequest.mRefreshCache)
   //{
   //   gEventDispatcher.send(cInvalidEventReceiverHandle, mEventHandleSim, cEventClassContentEnumerate, pPayload->getUserIndex(), mSearchID, pPayload);
   //}
   //else
   //{
   //   delete pPayload;
   //}
}

//==============================================================================
// 
//==============================================================================
void BGameFile::endEditFile(DWORD dwRet, BGameFilePayload* pPayload)
{
   // dismiss dialogs and refresh the UI
   mLastUpdate++;

   mSaveRequest.mInProgress = false;

   if (dwRet == ERROR_SUCCESS && mSaveRequest.mRefreshCache)
   {
      // need to force a refresh
      pPayload->setRefresh();

      gEventDispatcher.send(cInvalidEventReceiverHandle, mEventHandleSim, cEventClassContentEnumerate, pPayload->getUserIndex(), mSearchID, pPayload);
   }
   else
   {
      BUIGlobals* pUIGlobals = gGame.getUIGlobals();
      BDEBUG_ASSERT(pUIGlobals);
      if (pUIGlobals)
         pUIGlobals->setWaitDialogVisible(false);

      delete pPayload;
   }
}

//==============================================================================
// 
//==============================================================================
void BGameFile::reset()
{
   resetWrappers();

   mpStream = NULL;

   if (mpRawStream != NULL)
   {
      mpRawStream->close();
      delete mpRawStream;
   }
   mpRawStream = NULL;
   mUsingGameFileStream = false;

   mFileSaving = false;
   mFileLoading = false;
}

//==============================================================================
// 
//==============================================================================
void BGameFile::resetWrappers()
{
   if (mpDeflateRecStream != NULL)
   {
      mpDeflateRecStream->close();
      delete mpDeflateRecStream;
   }
   mpDeflateRecStream = NULL;

   if (mpEncryptRecStream != NULL)
   {
      mpEncryptRecStream->close();
      delete mpEncryptRecStream;
   }
   mpEncryptRecStream = NULL;

   if (mpInflateRecStream != NULL)
   {
      mpInflateRecStream->close();
      delete mpInflateRecStream;
   }
   mpInflateRecStream = NULL;

   if (mpDecryptRecStream != NULL)
   {
      mpDecryptRecStream->close();
      delete mpDecryptRecStream;
   }
   mpDecryptRecStream = NULL;
}

//==============================================================================
// 
//==============================================================================
BStream* BGameFile::getStream(const BGameFilePayload& payload, bool readOnly)
{
   // the payload may come from hdd/xfs, cache drive or user storage
   BStream* pStream = NULL;

   switch (payload.getStorageType())
   {
      case eStorageCache:
      {
         // cache streams require BFileDesc
         if (payload.getFileDesc() == NULL)
            break;

         BString filename;
         filename.format("%s\\%s", mCachePrefix.getPtr(), payload.getFileDesc()->filename().getPtr());

         pStream = getWin32Stream(filename, readOnly);
         break;
      }
      case eStorageHDD:
      {
         // HDD/XFS streams require BFileDesc
         if (payload.getFileDesc() == NULL)
            break;

         pStream = getSystemStream(payload.getFileDesc()->filename(), readOnly);
         break;
      }
      case eStorageUser:
      {
         pStream = getXContentStream(payload, readOnly);
         break;
      }
   }

   return pStream;
}

//==============================================================================
// atm, Opens the appropriate file as a read-only stream
//==============================================================================
BStream* BGameFile::getStream(const BGameFileManifest& manifest)
{
   BStream* pStream = NULL;

   switch (manifest.mStorageType)
   {
      case eStorageCache:
      {
         BString filename;
         filename.format("%s\\%s", mCachePrefix.getPtr(), manifest.mFilename.getPtr());
         pStream = getWin32Stream(filename, true);
         break;
      }
      case eStorageHDD:
      {
         pStream = getSystemStream(manifest.mFilename, true);
         break;
      }
      case eStorageUser:
      {
         pStream = getXContentStream(manifest, true);
         break;
      }
   }

   return pStream;
}

//==============================================================================
// 
//==============================================================================
BStream* BGameFile::getStream(const char* pFileName, bool readOnly)
{
   if (mUseCacheDrive)
   {
      BString filename;
      filename.format("%s\\%s", mCachePrefix.getPtr(), pFileName);
      return getWin32Stream(filename, readOnly);
   }
   else if (mUseBFile)
      return getSystemStream(pFileName, readOnly);

   return NULL;
}

//==============================================================================
// 
//==============================================================================
BStream* BGameFile::getStream(BStorageType type, const char* pFileName, bool readOnly)
{
   return getStream(0, type, pFileName, readOnly);
}

//==============================================================================
// 
//==============================================================================
BStream* BGameFile::getStream(DWORD userIndex, BStorageType type, const char* pFileName, bool readOnly)
{
   switch (type)
   {
      case eStorageCache:
      {
         BString filename;
         filename.format("%s\\%s", mCachePrefix.getPtr(), pFileName);
         return getWin32Stream(filename, readOnly);
      }
      case eStorageHDD:
      {
         return getSystemStream(pFileName, readOnly);
      }
      case eStorageUser:
      {
         BDEBUG_ASSERTM(0, "eStorageUser not supported in this version of getStream");
         break;
      }
      default:
      {
         return getStream(pFileName, readOnly);
      }
   }

   return NULL;
}

//==============================================================================
// 
//==============================================================================
BStream* BGameFile::getWin32Stream(const char* pFileName, bool readOnly)
{
   BWin32FileStream* pStream = new BWin32FileStream();

   uint flags;
   if (readOnly)
      flags = cSFReadable | cSFOpenExisting | cSFSeekable | cSFOptimizeForSequentialAccess;
   else
      flags = cSFWritable | cSFSeekable | cSFEnableBuffering;

   if (!pStream->open(pFileName, flags, &gWin32LowLevelFileIO))
   {
      delete pStream;
      return NULL;
   }

   return pStream;
}

//==============================================================================
// BFileSystemStream is only used for non-final builds so it should be safe
// to include cSFForceLoose here.
//==============================================================================
BStream* BGameFile::getSystemStream(const char* pFileName, bool readOnly)
{
   BFileSystemStream* pStream = new BFileSystemStream();

   uint flags;
   if (readOnly)
      flags = cSFReadable | cSFOpenExisting | cSFSeekable | cSFOptimizeForSequentialAccess | cSFForceLoose;
   else
      flags = cSFWritable | cSFSeekable | cSFEnableBuffering;

   if (!pStream->open(mGameDirID, pFileName, flags))
   {
      delete pStream;
      return NULL;
   }

   return pStream;
}

//==============================================================================
// 
//==============================================================================
BStream* BGameFile::getXContentStream(DWORD userIndex, const XCONTENT_DATA& content, uint flags)
{
   //BXContentStream* pStream = new BXContentStream();
   BGameXContentStream* pStream = new BGameXContentStream();

   if (!pStream->open(userIndex, content, flags))
   {
      delete pStream;
      return NULL;
   }

   return pStream;
}

//==============================================================================
// 
//==============================================================================
BStream* BGameFile::getXContentStream(const BGameFilePayload& payload, bool readOnly)
{
   // XContent streams require the XCONTENT_DATA structure, so that should be filled
   // in at a higher level than here, the selected deviceid is probably the biggest unknown
   // at this level and I don't want to deal with it here
   if (payload.getContent() == NULL)
      return NULL;

   //BXContentStream* pStream = new BXContentStream();

   uint flags;
   if (readOnly)
      flags = cSFReadable | cSFOpenExisting | cSFSeekable | cSFOptimizeForSequentialAccess;
   else
      flags = cSFWritable | cSFSeekable | cSFEnableBuffering;

   //if (!pStream->open(payload.getUserIndex(), *payload.getContent(), flags))
   //{
   //   delete pStream;
   //   return NULL;
   //}

   return getXContentStream(payload.getUserIndex(), *payload.getContent(), flags);
}

//==============================================================================
// 
//==============================================================================
BStream* BGameFile::getXContentStream(const BGameFileManifest& manifest, bool readOnly)
{
   if (!manifest.getContent())
      return NULL;

   //BXContentStream* pStream = new BXContentStream();

   uint flags;
   if (readOnly)
      flags = cSFReadable | cSFOpenExisting | cSFSeekable | cSFOptimizeForSequentialAccess;
   else
      flags = cSFWritable | cSFSeekable | cSFEnableBuffering;

   //if (!pStream->open(manifest.getUserIndex(), *manifest.getContent(), flags))
   //{
   //   delete pStream;
   //   return NULL;
   //}

   //return pStream;
   return getXContentStream(manifest.getUserIndex(), *manifest.getContent(), flags);
}

//==============================================================================
// 
//==============================================================================
bool BGameFile::loadSettings(uint version, BGameSettings* pSettings, BStream* pStream, bool skipLoadFileSettings, DWORD& checksum, bool& multiplayer, long& localPlayerID)
{
   if (pStream->readBytes(&checksum, sizeof(DWORD)) != sizeof(DWORD))
      return false;

   if (pStream->readBytes(&multiplayer, sizeof(bool)) != sizeof(bool))
      return false;

   if (pStream->readBytes(&localPlayerID, sizeof(long)) != sizeof(long))
      return false;

   DWORD settingCount=0;
   if (pStream->readBytes(&settingCount, sizeof(DWORD)) != sizeof(DWORD))
      return false;

   if (version < 18)
   {
      if (version < 15)
      {
         if (settingCount != pSettings->getNumberEntries() - 1)
            return false;
         pSettings->setLong(BGameSettings::cGameMode, 0);
      }
      else if (settingCount != pSettings->getNumberEntries())
         return false;
   }

   for (DWORD i=0; i<settingCount; i++)
   {
      long index;

      if (version < 18)
      {
         index = i;
         if (version < 15 && i >= BGameSettings::cGameMode)
            index++;
      }
      else
      {
         index = -1;
         uint8 nameLen = 0;
         if (pStream->readBytes(&nameLen, sizeof(uint8)) != sizeof(uint8))
            return false;
         if (nameLen == 0)
            return false;
         BFixedString256 name;
         if (pStream->readBytes(name, nameLen) != nameLen)
            return false;
         name[nameLen] = NULL;
         for (long j=0; j<BGameSettings::cSettingCount; j++)
         {
            if (stricmp(gGameSettings[j].mName, name) == 0)
            {
               index = j;
               break;
            }
         }
      }

      long dataType=0;
      if (pStream->readBytes(&dataType, sizeof(long)) != sizeof(long))
         return false;

      long baseDataType=(index == -1 ? -1 : pSettings->getDataType(index));

      if (version < 18)
      {
         if (dataType != baseDataType && !(version < 17 && index == BGameSettings::cLoadType && dataType == BDataEntry::cTypeBool))
            return false;
      }

      switch (dataType)
      {
         case BDataEntry::cTypeLong:
         {
            long val=0;
            if (pStream->readBytes(&val, sizeof(long)) != sizeof(long))
               return false;
            if (index != -1 && dataType == baseDataType)
               pSettings->setLong(index, val);
            break;
         }
         case BDataEntry::cTypeBool:
         {
            bool val=0;
            if (pStream->readBytes(&val, sizeof(bool)) != sizeof(bool))
               return false;
            if (index != -1 && dataType == baseDataType && (!skipLoadFileSettings || index!=BGameSettings::cRecordGame))
               pSettings->setBool(index, val);
            break;
         }
         case BDataEntry::cTypeByte:
         {
            BYTE val=0;
            if (pStream->readBytes(&val, sizeof(BYTE)) != sizeof(BYTE))
               return false;
            //if (index != -1 && dataType == baseDataType && (!skipLoadFileSettings || index!=BGameSettings::cLoadType))
            if (index != -1 && dataType == baseDataType)
               pSettings->setBYTE(index, val);
            break;
         }
         case BDataEntry::cTypeString:
         {
            BFixedString256 val;
            if (version < 18)
            {
               if (pStream->readBytes(val, 256) != 256)
                  return false;
            }
            else
            {
               uint8 len = 0;
               if (pStream->readBytes(&len, sizeof(uint8)) != sizeof(uint8))
                  return false;
               if (len != 0)
               {
                  if (pStream->readBytes(val, len) != len)
                     return false;
               }
               val[len] = NULL;
            }
            //if (index != -1 && dataType == baseDataType && (!skipLoadFileSettings || index!=BGameSettings::cLoadName))
            if (index != -1 && dataType == baseDataType)
               pSettings->setString(index, val);
            break;
         }
         case BDataEntry::cTypeFloat:
         {
            float val=0.0f;
            if (pStream->readBytes(&val, sizeof(float)) != sizeof(float))
               return false;
            if (index != -1 && dataType == baseDataType)
               pSettings->setFloat(index, val);
            break;
         }
         case BDataEntry::cTypeInt64:
         {
            int64 val=0;
            if (pStream->readBytes(&val, sizeof(int64)) != sizeof(int64))
               return false;
            if (index != -1 && dataType == baseDataType)
               pSettings->setInt64(index, val);
            break;
         }
      }
   }

   return true;
}

//==============================================================================
// 
//==============================================================================
bool BGameFile::refreshManifest(BStorageType type, BGameFileManifest& manifest, BGameSettings* pSettings, BStream* pStream)
{
   BDEBUG_ASSERT(pSettings);
   BDEBUG_ASSERT(pStream);

   // reads the game settings to populate the manifest
   BGameFileVersion details;
   if (pStream->readBytes(&details, sizeof(BGameFileVersion)) != sizeof(BGameFileVersion))
      return false;

   manifest.setVersion(details.mVersion);
   manifest.mDetails = details;

   BSHA1Gen sha1Gen;

   sha1Gen.update(&details, sizeof(BGameFileVersion));

   if (details.mEncryptHeader)
   {
      // wrap the existing stream in a decrypt stream
      BDecryptedStream* pDecryptStream = new BDecryptedStream(pStream, cKeyPhrase);

      pDecryptStream->setBaseOfs(pStream->curOfs());

      if (!manifest.mHeader.deserialize(pDecryptStream, sha1Gen))
      {
         delete pDecryptStream;

         return false;
      }

      delete pDecryptStream;
   }
   else if (!manifest.mHeader.deserialize(pStream, sha1Gen))
      return false;

   // verify that the data payload is the same size that we said it was
   if (!mDebugging && pStream->sizeKnown() && (pStream->size() - pStream->curOfs()) != manifest.mHeader.mDataSize)
      return false;

   // store the file size for display purposes
   manifest.mFileSize = pStream->size();

   // sync the time from the header
   manifest.mTime = manifest.getDate();

   BStream* pRawStream = pStream;
   BDecryptedStream* pDecryptStream = NULL;
   BInflateStream* pInflateStream = NULL;

   if (details.mEncryptData)
   {
      uint64 k1, k2, k3;
      teaCryptInitKeys(cKeyPhrase, k1, k2, k3);
      pDecryptStream = new BDecryptedStream(pRawStream, k1, k2, manifest.mHeader.getKey3());
      pDecryptStream->setBaseOfs(pRawStream->curOfs());
      pStream = pDecryptStream;
   }

   if (details.mCompressData)
   {
      pInflateStream = new BInflateStream(*pStream);
      pStream = pInflateStream;
   }

   DWORD checksum=0;
   bool multiplayer=false;
   long localPlayerID=1;
   if (!loadSettings(details.mVersion, pSettings, pStream, false, checksum, multiplayer, localPlayerID))
   {
      delete pInflateStream;
      delete pDecryptStream;

      return false;
   }

   BFixedString<112> playerBuf;

   const char* pFmt1 = "%s, ";
   const char* pFmt2 = "%s";
   long playerCount = 0;
   pSettings->getLong(BGameSettings::cPlayerCount, playerCount);
   for (long i=1; i <= playerCount; ++i)
   {
      BString playerName;
      if (pSettings->getString(PSINDEX(i, BGameSettings::cPlayerName), playerName))
      {
         playerBuf.formatAppend((i == playerCount ? pFmt2 : pFmt1), playerName.getPtr());
      }
   }

   manifest.mPlayers.set(playerBuf);

   BFixedStringMaxPath mapName;
   pSettings->getString(BGameSettings::cMapName, mapName, MAX_PATH);

   // look up the scenario info, if not found, stick with the mapName
   // otherwise, lookup the NameStringID and get the fancy description
   const BScenarioList& scenarioList = gDatabase.getScenarioList();

   manifest.mMapStringID = 0;
   manifest.mMapType = 0;

   const BScenarioMap* pMap = scenarioList.getMapInfo(mapName);
   if (pMap != NULL)
   {
      manifest.mMapStringID = pMap->getMapNameStringID();
      manifest.mMapType = pMap->getType();
   }

   if (manifest.mMapStringID > 0)
   {
      // lookup the localized name
      manifest.mScenarioName = gDatabase.getLocStringFromID(manifest.mMapStringID);
   }
   else
   {
      mapName.right(mapName.findRight("\\") + 1);
      manifest.mScenarioName = mapName;
   }

   delete pInflateStream;
   delete pDecryptStream;

   return true;
}

//==============================================================================
// BStorageType is no longer needed at this level, only the manifest and stream
//==============================================================================
bool BGameFile::refreshManifest(BStorageType type, BGameFileManifest& manifest, BStream* pStream)
{
   BDEBUG_ASSERT(pStream);
   if (pStream == NULL)
      return false;
   BDEBUG_ASSERT(mpGameSettings);
   if (mpGameSettings == NULL)
      return false;

   bool retval = refreshManifest(type, manifest, mpGameSettings, pStream);

   if (retval)
   {
      // cache off the appropriate game settings into the manifest
      BFixedStringMaxPath mapName;
      mpGameSettings->getString(BGameSettings::cMapName, mapName, MAX_PATH);

      mapName.right(mapName.findRight("\\") + 1);
      manifest.mMapName = mapName;
   }
   else if (mDebugging)
   {
      manifest.setFileError();
      return true;
   }

   return retval;
}

//==============================================================================
// 
//==============================================================================
bool BGameFile::refreshManifest(BStorageType type, BGameFileManifest& manifest, const BFileDesc& fileDesc)
{
   BStream* pStream = NULL;
   
   // if we're reading from the cache drive, then we need to use the full filename
   if (type == eStorageCache)
      pStream = getStream(type, fileDesc.fullFilename(), true);
   else
      pStream = getStream(type, fileDesc.filename(), true);

   if (!pStream)
      return false;

   bool retval = refreshManifest(type, manifest, pStream);

   if (retval)
   {
      manifest.mStorageType = type;
      manifest.mFilename = fileDesc.filename();

      if (manifest.getDate().wYear == 0)
      {
         FILETIME ft = Utils::UInt64ToFileTime(fileDesc.createTime());
         FileTimeToSystemTime(&ft, &manifest.mTime);

         manifest.mHeader.setDate(manifest.mTime);
      }

      manifest.update();
   }

   delete pStream;

   return retval;
}

//==============================================================================
// 
//==============================================================================
bool BGameFile::refreshManifest(BGameFileManifest& manifest)
{
   BStream* pStream = getStream(manifest);
   if (!pStream)
      return false;

   bool retval = refreshManifest(manifest.getStorageType(), manifest, pStream);

   if (retval)
   {
      if (manifest.getDate().wYear == 0)
      {
         uint64 t;
         if (pStream->getTime(t))
         {
            FILETIME ft = Utils::UInt64ToFileTime(t);

            FileTimeToSystemTime(&ft, &manifest.mTime);

            manifest.mHeader.setDate(manifest.mTime);
         }
      }

      manifest.update();
   }

   delete pStream;

   return retval;
}

//==============================================================================
// 
//==============================================================================
BGameFileManifest* BGameFile::refreshManifest(const BGameFilePayload& payload)
{
   // need to retrieve a stream based on the payload
   BStream* pStream = getStream(payload, true);

   if (!pStream)
      return NULL;

   BGameFileManifest* pManifest = new BGameFileManifest();

   pManifest->setTempID(InterlockedIncrementRelease(&mNextTempID));
   pManifest->setStorageType(payload.getStorageType());
   pManifest->setFilename(payload.getFilename());
   pManifest->setUserIndex(payload.getUserIndex());
   // cache the XCONTENT structure in the manifest for future operations
   if (payload.getContent() != NULL)
      pManifest->setContent(*payload.getContent());
   pManifest->setRefresh(payload.getRefresh());

   // storage type is not used, prepare to remove it
   bool retval = refreshManifest(eStorageUnknown, *pManifest, pStream);

   if (retval)
   {
      if (pManifest->getStorageType() == eStorageUser)
         pManifest->mSaved = true;

      if (pManifest->getDate().wYear == 0)
      {
         uint64 t;
         if (pStream->getTime(t))
         {
            FILETIME ft = Utils::UInt64ToFileTime(t);

            FileTimeToSystemTime(&ft, &pManifest->mTime);

            pManifest->mHeader.setDate(pManifest->mTime);
         }
      }

      pManifest->update();
   }
   else
   {
      delete pManifest;

      pManifest = NULL;
   }

   delete pStream;

   return pManifest;
}

//==============================================================================
// 
//==============================================================================
void BGameFile::refresh(DWORD userIndex, uint searchID, BStorageType type)
{
   BFindFiles findFilesObj;
   BString dirName;

   if (type == eStorageHDD)
   {
      BString partialDirName;
      eFileManagerError result = gFileManager.getDirListEntry(partialDirName, mGameDirID);
      BVERIFY(cFME_SUCCESS == result);
      dirName = BFileUtils::getXboxGamePath();
      dirName.append(partialDirName);
   }
   else if (type == eStorageCache)
   {
      if (!mCacheEnabled)
         return;

      dirName = mCachePrefix;
      findFilesObj.setLowLevelFileIO(&gWin32LowLevelFileIO);
   }

   BDynamicArray<BString> fileList;

   BString pattern;
   pattern.format("*%s", mGameFileExt.getPtr());

   bool success = findFilesObj.scan(dirName, pattern, BFindFiles::FIND_FILES_WANT_FILES);
   if (!success)
      return;

   for (uint i = 0; i < findFilesObj.numFiles() && searchID == static_cast<uint>(mSearchID); i++)
   {
      const BFileDesc& fileDesc = findFilesObj.getFile(i);

      if (fileDesc.size() == 0 && type == eStorageCache)
      {
         DeleteFile(fileDesc.fullFilename().getPtr());
         continue;
      }

      // send a payload request
      BGameFilePayload* pPayload = new BGameFilePayload(userIndex, type, fileDesc);
      gEventDispatcher.send(mEventHandleMisc, mEventHandleSim, cEventClassContentEnumerate, userIndex, searchID, pPayload);
   }
}

//==============================================================================
// 
//==============================================================================
void BGameFile::refreshDisk(DWORD userIndex, uint searchID)
{
#ifndef BUILD_FINAL
   refresh(userIndex, searchID, eStorageHDD);
#endif
}

//==============================================================================
// 
//==============================================================================
void BGameFile::refreshCache(DWORD userIndex, uint searchID)
{
   refresh(userIndex, searchID, eStorageCache);
}

//==============================================================================
// 
//==============================================================================
void BGameFile::refreshStorage(DWORD userIndex, uint searchID)
{
   HANDLE hEnum = INVALID_HANDLE_VALUE;

   XCONTENT_DATA contentData[10];

   DWORD cbBuffer = 0;

   DWORD retval = XContentCreateEnumerator(userIndex, XCONTENTDEVICE_ANY, XCONTENTTYPE_SAVEDGAME, XCONTENTFLAG_ENUM_EXCLUDECOMMON, 10, &cbBuffer, &hEnum);

   if (retval != ERROR_SUCCESS || searchID != static_cast<uint>(mSearchID))
   {
      if (hEnum != INVALID_HANDLE_VALUE)
         CloseHandle(hEnum);
      return;
   }

   DWORD dwReturnCount;
   do 
   {
      DWORD dwRet = XEnumerate(hEnum, contentData, sizeof(contentData), &dwReturnCount, NULL);
      if (dwRet == ERROR_SUCCESS && searchID == static_cast<uint>(mSearchID))
      {
         // send events back to our record game thread (sim thread for now)
         for (uint i=0; i < dwReturnCount && searchID == static_cast<uint>(mSearchID); ++i)
         {
            BGameFilePayload* pPayload = new BGameFilePayload(userIndex, contentData[i]);

            gEventDispatcher.send(cInvalidEventReceiverHandle, mEventHandleSim, BGameFile::cEventClassContentEnumerate, userIndex, searchID, pPayload);
         }
      }
   } while (dwReturnCount == 10 && searchID == static_cast<uint>(mSearchID));

   if (hEnum != INVALID_HANDLE_VALUE)
      CloseHandle(hEnum);
}

//==============================================================================
// 
//==============================================================================
bool BGameFile::refreshServer(DWORD userIndex, uint searchID)
{
   // issue a list command to the server
   if (mpListMediaTask != NULL)
   {
      // temporary allowing us to reset for another try
      gLSPManager.terminateMediaTask(mpListMediaTask);
      mpListMediaTask = NULL;
      //return false;
   }

//-- FIXING PREFIX BUG ID 3748
   const BUser* pUser = gUserManager.getUserByPort(userIndex);
//--
   if (!pUser || !pUser->isSignedIntoLive())
      return false;

   mpListMediaTask = gLSPManager.createMediaTask();
   if (mpListMediaTask == NULL)
      return false;

   mpListMediaTask->init(BLSPMediaTask::cCommandList, 10000, pUser->getPort(), pUser->getXuid(), this, mListCRC);

   mMediaTasks.add(mpListMediaTask);

   return true;
}

//==============================================================================
// 
//==============================================================================
bool BGameFile::refreshServerFriends(DWORD userIndex, uint searchID)
{
   // issue a list command to the server
   if (mpListFriendsMediaTask != NULL)
   {
      // temporary allowing us to reset for another try
      gLSPManager.terminateMediaTask(mpListFriendsMediaTask);
      mpListFriendsMediaTask = NULL;
      //return false;
   }

//-- FIXING PREFIX BUG ID 3749
   const BUser* pUser = gUserManager.getUserByPort(userIndex);
//--
   if (!pUser || !pUser->isSignedIntoLive())
      return false;

   mpListFriendsMediaTask = gLSPManager.createMediaTask();
   if (mpListFriendsMediaTask == NULL)
      return false;

   // need a CRC value for my friend's list
   mpListFriendsMediaTask->init(BLSPMediaTask::cCommandListFriends, 10000, pUser->getPort(), pUser->getXuid(), this, mListFriendsCRC);

   mMediaTasks.add(mpListFriendsMediaTask);

   return true;
}

//==============================================================================
// 
//==============================================================================
void BGameFile::calcName(BSimString& name, BGameSettings* pSettings)
{
   BDEBUG_ASSERT(pSettings);

   //if (mUseBFile)
      calcFileName(name, pSettings);
   //else
   //   calcCacheName(name, pSettings);
}

//==============================================================================
// 
//==============================================================================
void BGameFile::calcFileName(BSimString& name, BGameSettings* pSettings)
{
   if (mGameFileType == cGameFileSave)
   {
      name.empty();
      pSettings->getString(BGameSettings::cLoadName, name);
      if (!name.isEmpty())
         return;
   }

   SYSTEMTIME time;

   GetLocalTime(&time);


#ifndef BUILD_FINAL
   char dmXboxName[256];
   DWORD size = sizeof(dmXboxName);
   DmGetXboxName(dmXboxName, &size);

   name.format("%s_%02d%02d%02d-%02d%02d", dmXboxName, (time.wYear>2000?time.wYear-2000:time.wYear), time.wMonth, time.wDay, time.wHour, time.wMinute);
#else
   name.format("%02d%02d%02d-%02d%02d", (time.wYear>2000?time.wYear-2000:time.wYear), time.wMonth, time.wDay, time.wHour, time.wMinute);
#endif
}

//==============================================================================
// 
//==============================================================================
void BGameFile::calcCacheName(BSimString& name, BGameSettings* pSettings)
{
   if (mGameFileType == cGameFileSave)
   {
      name.empty();
      pSettings->getString(BGameSettings::cLoadName, name);
      if (!name.isEmpty())
         return;
   }

   SYSTEMTIME time;

   GetLocalTime(&time);

   // lookup the string IDs for the requested scenario
   const BScenarioList& scenarioList = gDatabase.getScenarioList();

   // pull the map name from the settings
   BFixedStringMaxPath mapName;
   pSettings->getString(BGameSettings::cMapName, mapName, MAX_PATH);

   long stringID = 0;
   const BScenarioMap* pMap = scenarioList.getMapInfo(mapName);
   if (pMap)
      stringID = pMap->getMapNameStringID();

   // if I'm writing out to the cache drive, calculate the obscure name, otherwise use a more friendly name
   name.format("%02d%02d%02d%02d%02d%02d%016I64x%05d", (time.wYear>2000?time.wYear-2000:time.wYear), time.wMonth, time.wDay, time.wHour, time.wMinute, time.wSecond, mHeader.mID, stringID);
}

//==============================================================================
// 
//==============================================================================
void BGameFile::notify(DWORD eventID, void* pTask)
{
   if (!pTask)
      return;

   if (eventID == mSelectDeviceEventID && mSaveRequest.mEventID == eventID)
   {
//-- FIXING PREFIX BUG ID 3750
      const BUser* pUser = gUserManager.getUserByPort(mSaveRequest.mUserIndex);
//--
      if (!pUser || !pUser->isSignedIntoLive())
         return;

      if (pUser->getDefaultDevice() == XCONTENTDEVICE_ANY)
         return;

      // find the manifest for the given ID
      BGameFileManifest* pManifest = NULL;

      uint count = mGameFiles.getSize();
      for (uint i=0; i < count; ++i)
      {
         BGameFileManifest* pTemp = mGameFiles[i];
         if (pTemp && pTemp->getTempID() == mSaveRequest.mTempID && pTemp->getStorageType() == eStorageServer)
         {
            pManifest = pTemp;
            break;
         }
      }

      // the recording must exist in the manifest for us to download
      if (!pManifest)
         return;

      // the recording must exist in Server Storage
      if (pManifest->getStorageType() != eStorageServer)
         return;

      mpDownloadMediaTask = gLSPManager.createMediaTask();
      if (mpDownloadMediaTask == NULL)
         return;

      mSaveRequest.mInProgress = true;

      mpDownloadMediaTask->init(BLSPMediaTask::cCommandDownload, 10000, pUser->getPort(), pUser->getXuid(), this, pManifest->getID());

      mMediaTasks.add(mpDownloadMediaTask);

      pManifest->setMediaTaskID(mpDownloadMediaTask->getID());
   }
   else if (mSaveRequest.mEventID == eventID)
   {
      const BSelectDeviceAsyncTask* pDeviceTask = reinterpret_cast<BSelectDeviceAsyncTask*>(pTask);

      if (pDeviceTask->getError() == BAsyncTask::cErrorSuccess)
      {
         // device has been selected, begin content saving
         mSaveRequest.mContent.DeviceID = pDeviceTask->getDeviceID();
         if (mSaveRequest.mSaveGame)
            gSaveGame.setDoSaveGame(pDeviceTask->getDeviceID());
         else
            beginSaveFile();
      }
      else
      {
         // the device select failed, dismiss any "please wait" dialogs
         if (mSaveRequest.mSaveGame)
            gSaveGame.clearDoSaveGame();
         else
            endSaveFile(ERROR_IO_DEVICE);
      }
   }
}

//==============================================================================
// 
//==============================================================================
DWORD BGameFile::saveFile(const BGameFilePayload& payload)
{
   // first check to see if we already have a stream available
   BStream* pSrcStream = payload.getStream();
   if (pSrcStream == NULL)
      pSrcStream = getStream(mSaveRequest.mSourceStorageType, mSaveRequest.mSourceFilename, true);

   if (!pSrcStream)
      return ERROR_FILE_NOT_FOUND;

   // the XContent API has succeeded, so now I need to open
   // the stream for either reading/saving
   BStream* pDstStream = getXContentStream(payload, false);
   if (!pDstStream)
   {
      delete pSrcStream;
      return ERROR_GEN_FAILURE;
   }

   uint64 bytesCopied = 0;

   // 
   if (pSrcStream->seek(0) != 0)
   {
      delete pSrcStream;
      delete pDstStream;
      return ERROR_SEEK_ON_DEVICE;
   }

   // FIXME this needs error handling code
   bool retval = BStream::copyStream(*pSrcStream, *pDstStream, &bytesCopied);

   // lookup the specific error amongst the streams
   DWORD dwRet = (retval ? ERROR_SUCCESS : ERROR_GEN_FAILURE);

   delete pSrcStream;
   delete pDstStream;

   return dwRet;
}

//==============================================================================
// 
//==============================================================================
DWORD BGameFile::saveHeader(const BGameFilePayload& payload)
{
   if (payload.getContent() == NULL)
      return ERROR_FILE_NOT_FOUND;

   // open the XContent stream
   // deserialize the header
   // save the new header with the modified name/desc
   //
   BStream* pStream = getXContentStream(payload.getUserIndex(), *payload.getContent(), cSFReadable | cSFWritable | cSFSeekable | cSFOpenExisting);
   if (!pStream)
   {
      return ERROR_GEN_FAILURE;
   }

   BGameFileVersion details;

   if (pStream->readBytes(&details, sizeof(BGameFileVersion)) != sizeof(BGameFileVersion) || details.mVersion > cBaseVersion)
   {
      delete pStream;
      return false;
   }

   BSHA1Gen sha1Gen;
   sha1Gen.update(&details, sizeof(BGameFileVersion));

   BGameFileHeader header;

   if (details.mEncryptHeader)
   {
      // wrap the existing stream in a decrypt stream
      BDecryptedStream* pDecryptStream = new BDecryptedStream(pStream, cKeyPhrase);

      // force a base offset to that of the current stream
      pDecryptStream->setBaseOfs(pStream->curOfs());

      if (!header.deserialize(pDecryptStream, sha1Gen))
      {
         delete pDecryptStream;
         delete pStream;

         return ERROR_GEN_FAILURE;
      }

      delete pDecryptStream;
   }
   else if (!header.deserialize(pStream, sha1Gen))
   {
      delete pStream;

      return ERROR_GEN_FAILURE;
   }

   // update the name/description
   header.mName = payload.mName;
   header.mDesc = payload.mDesc;

   // need to seek so we can reserialize out the header
   if (pStream->seek(sizeof(BGameFileVersion)) != sizeof(BGameFileVersion))
   {
      delete pStream;

      return ERROR_SEEK_ON_DEVICE;
   }

   sha1Gen.clear();
   sha1Gen.update(&details, sizeof(BGameFileVersion));

   if (details.mEncryptHeader)
   {
      BEncryptedStream* pEncryptStream = new BEncryptedStream(pStream, cKeyPhrase);

      if (!header.serialize(pEncryptStream, sha1Gen))
      {
         delete pEncryptStream;

         return ERROR_GEN_FAILURE;
      }

      delete pEncryptStream;
   }
   else if (!header.serialize(pStream, sha1Gen))
   {
      delete pStream;

      return ERROR_GEN_FAILURE;
   }

   delete pStream;

   return ERROR_SUCCESS;
}

//==============================================================================
// 
//==============================================================================
DWORD BGameFile::deleteFile(const BGameFilePayload& payload)
{
   DWORD dwRet = ERROR_FILE_NOT_FOUND;

   switch (payload.getStorageType())
   {
      case eStorageCache:
         {
            BString filename;
            filename.format("%s\\%s", mCachePrefix.getPtr(), payload.getFilename().getPtr());
            if (DeleteFile(filename.getPtr()))
               dwRet = ERROR_SUCCESS;
            break;
         }
      case eStorageUser:
         {
            if (payload.getContent())
               dwRet = XContentDelete(payload.getUserIndex(), payload.getContent(), NULL);
            break;
         }
      default:
         {
            dwRet = ERROR_SUCCESS;
            break;
         }
   }

   return dwRet;
}

//==============================================================================
// 
//==============================================================================
void BGameFile::mediaTaskRelease(uint taskID, uint32 result)
{
//-- FIXING PREFIX BUG ID 3751
   const BLSPMediaTask* pTask = NULL;
//--
   for (uint i=0; i < mMediaTasks.getSize(); ++i)
   {
      if (mMediaTasks[i]->getID() == taskID)
      {
         pTask = mMediaTasks[i];
         break;
      }
   }

   if (pTask != NULL)
   {
      if (pTask == mpListMediaTask)
      {
         refreshComplete();
         mpListMediaTask = NULL;
      }
      else if (pTask == mpListFriendsMediaTask)
      {
         refreshComplete();
         mpListFriendsMediaTask = NULL;
      }
      else if (pTask == mpUploadMediaTask)
      {
         mpUploadMediaTask = NULL;
      }
      else if (pTask == mpDownloadMediaTask)
      {
         mpDownloadMediaTask = NULL;
         mSaveRequest.mInProgress = false;
      }
      else if (pTask == mpDeleteMediaTask)
      {
         // finish delete task
         endDeleteFile(ERROR_GEN_FAILURE);
         mpDeleteMediaTask = NULL;
      }

      mMediaTasks.removeIndex(i);
   }
}

//==============================================================================
// 
//==============================================================================
void BGameFile::mediaListResponse(uint taskID, uint32 result, uint crc, uint ttl, const void* pData, int32 size)
{
   // find our outstanding task
//-- FIXING PREFIX BUG ID 3752
   const BLSPMediaTask* pTask = NULL;
//--
   for (uint i=0; i < mMediaTasks.getSize(); ++i)
   {
      if (mMediaTasks[i]->getID() == taskID)
      {
         pTask = mMediaTasks[i];
         break;
      }
   }

   if (pTask == NULL)
      return;

   if (pTask == mpListMediaTask)
   {
      mpListMediaTask = NULL;
      mListCRC = crc;
      mListTTL = ttl;
      mLastListTime = timeGetTime();
   }
   else if (pTask == mpListFriendsMediaTask)
   {
      mpListFriendsMediaTask = NULL;
      mListFriendsCRC = crc;
      mListFriendsTTL = ttl;
      mLastListTime = timeGetTime();
   }

   // need verifications that this list response is the correct one, for now assume it is
   if (size > 0)
   {
      // parse the pData, using xml temporarily
      //<media>
      //   <r id='16016710683417038212' l='28000.0' s='6235' t='20080507T23:46:38Z' mid='23960' x='2535285326020399'>
      //      <n>Chasms (1v1)</n>
      //      <d>Skirmish - WickedSlug, Player 2</d>
      //      <o>WickedSlug</o>
      //   </r>
      //   <i id='16016710683417038213' s='6235' t='20080507T23:46:38Z' x='2535285326020399'>
      //      <n>Chasms (1v1)</n>
      //      <d>Skirmish - WickedSlug, Player 2</d>
      //      <o>WickedSlug</o>
      //   </i>
      //</media>
      BByteStream* pStream = new BByteStream(pData, size);
      BXMLReader* pReader = new BXMLReader();
      if (pReader->load(pStream, cXFTXML))
      {
         BXMLNode rootNode = pReader->getRootNode();
         long count = rootNode.getNumberChildren();
         for (long i=0; i<count; i++)
         {
            BXMLNode node(rootNode.getChild(i));
            if (node.getName().compare("r") == 0)
            {
               BGameFileManifest* pManifest = new BGameFileManifest();

               // mNextTempID is currently only accessed from another thread
               // we need to insure that this get/set is protected
               pManifest->setTempID(InterlockedIncrementRelease(&mNextTempID));
               pManifest->setStorageType(eStorageServer);
               pManifest->setUserIndex(pTask->getUserIndex());
               pManifest->setRefresh();

               BGameFileHeader& header = pManifest->mHeader;

               // record game
               BSimString tempStr;
               node.getAttribValueAsString("id", tempStr);
               header.setID(tempStr.asUInt64());

               pManifest->setFilename(tempStr);

               float length;
               node.getAttribValueAsFloat("l", length);
               header.setLength(length);

               uint64 fileSize;
               node.getAttribValueAsUInt64("s", fileSize);
               pManifest->mFileSize = fileSize;

               long mid = 0;
               node.getAttribValueAsLong("mid", mid);
               if (mid > 0)
                  pManifest->mScenarioName = gDatabase.getLocStringFromID(mid);

               SYSTEMTIME time;
               Utils::FastMemSet(&time, 0, sizeof(SYSTEMTIME));
               node.getAttribValueAsString("t", tempStr);
               sscanf_s(tempStr.getPtr(), "%4hu%2hu%2huT%2hu:%2hu:%2huZ", &time.wYear, &time.wMonth, &time.wDay, &time.wHour, &time.wMinute, &time.wSecond);
               header.setDate(time);

               pManifest->mTime = pManifest->getDate();

               XUID xuid = INVALID_XUID;
               node.getAttribValueAsUInt64("x", xuid);
               header.setXuid(xuid);

               long nodeCount = node.getNumberChildren();
               for(long j=0; j<nodeCount; j++)
               {
                  BXMLNode child(node.getChild(j));
                  const BPackedString name(child.getName());

                  if (name == "n")
                  {
                     BUString tempUStr;
                     child.getTextAsString(tempUStr);
                     if (mid == 0)
                        pManifest->mScenarioName = tempUStr;
                     header.setName(tempUStr);
                  }
                  else if (name == "d")
                  {
                     BUString tempUStr;
                     child.getTextAsString(tempUStr);
                     header.setDesc(tempUStr);
                  }
                  else if (name == "o")
                  {
                     child.getTextAsString(tempStr);
                     header.setAuthor(tempStr);
                  }
               }

               pManifest->update();

               refreshResult(pManifest);
            }
            else if (node.getName().compare("i") == 0)
            {
               // screen shot
            }
         }
      }
   }

   // We may run into issues by forcing the latest searchID here
   gEventDispatcher.send(cInvalidEventReceiverHandle, mEventHandleSim, cEventClassRefreshComplete, 0, mSearchID);
}

//==============================================================================
// 
//==============================================================================
void BGameFile::mediaDownloadResponse(uint taskID, uint32 result)
{
   // find our outstanding task
   BLSPMediaTask* pTask = NULL;
   for (uint i=0; i < mMediaTasks.getSize(); ++i)
   {
      if (mMediaTasks[i]->getID() == taskID)
      {
         pTask = mMediaTasks[i];
         break;
      }
   }

   if (pTask == NULL)
      return;

   if (pTask == mpDownloadMediaTask)
   {
      mpDownloadMediaTask = NULL;
      mSaveRequest.mInProgress = false;
   }

   // store the stream to user storage
   // 
   // insure the user that requested this data is still signed-in
   // find the manifest for the given mediaID
   // create an BXContentStream for the given manifest item
   // copy from the media task's stream to our content stream
//-- FIXING PREFIX BUG ID 3730
   const BUser* pUser = gUserManager.getUserByPort(pTask->getUserIndex());
//--
   if (!pUser || !pUser->isSignedIn() || pUser->getXuid() != pTask->getXuid())
      return;

   // find the manifest for the given ID
//-- FIXING PREFIX BUG ID 3731
   const BGameFileManifest* pManifest = NULL;
//--

   uint count = mGameFiles.getSize();
   for (uint i=0; i < count; ++i)
   {
      BGameFileManifest* pTemp = mGameFiles[i];
      if (pTemp && pTemp->getID() == pTask->getMediaID() && pTemp->getStorageType() == eStorageServer)
      {
         pManifest = pTemp;
         break;
      }
   }

   BASSERTM(pManifest != NULL, "Failed to find corresponding BGameFileManifest for requested server storage item");
   if (!pManifest)
      return;

   XCONTENT_DATA content;
   Utils::FastMemSet(&content, 0, sizeof(XCONTENT_DATA));

   content.DeviceID = pUser->getDefaultDevice();
   content.dwContentType = XCONTENTTYPE_SAVEDGAME;

   // set a display name and file name
   BString fileName;
   fileName.format("%I64u", pTask->getMediaID());
   StringCchCopyA(content.szFileName, XCONTENT_MAX_FILENAME_LENGTH, fileName.getPtr());
   StrHelperStringCchCopyW(content.szDisplayName, XCONTENT_MAX_DISPLAYNAME_LENGTH, pManifest->getDesc().getPtr());

   BStream* pContentStream = getXContentStream(pUser->getPort(), content, cSFWritable | cSFSeekable | cSFEnableBuffering);
   BASSERTM(pContentStream != NULL, "Failed to create user storage item");
   if (!pContentStream)
      return;

   uint64 bytesCopied = 0;

   if ((pTask->getStream())->seek(0) != 0)
   {
      delete pContentStream;
      return;
   }

   bool retval = BStream::copyStream(*pTask->getStream(), *pContentStream, &bytesCopied);

   BASSERTM(retval, "Failed to store server stream to user storage");
   // lookup the specific error amongst the streams
   //DWORD dwRet = (retval ? ERROR_SUCCESS : ERROR_GEN_FAILURE);

   delete pContentStream;

   // instructs the UI to update the list
   BGameFilePayload* pPayload = new BGameFilePayload(pTask->getUserIndex(), content);
   gEventDispatcher.send(cInvalidEventReceiverHandle, mEventHandleSim, BGameFile::cEventClassContentEnumerate, pTask->getUserIndex(), mSearchID, pPayload);
}

//==============================================================================
// 
//==============================================================================
void BGameFile::mediaUploadResponse(uint taskID, uint32 result)
{
   // find our outstanding task
//-- FIXING PREFIX BUG ID 3732
   const BLSPMediaTask* pTask = NULL;
//--
   for (uint i=0; i < mMediaTasks.getSize(); ++i)
   {
      if (mMediaTasks[i]->getID() == taskID)
      {
         pTask = mMediaTasks[i];
         break;
      }
   }

   if (pTask == NULL)
      return;

   if (pTask == mpUploadMediaTask)
   {
      mpUploadMediaTask = NULL;
      mListTTL = 0; // allow another list request to occur
   }

   // XXX TODO check the result code and display an appropriate dialog
   // for now, assume that a response means the upload succeeded

   // construct a new manifest based on the file that we uploaded and inject us into the list

   // first find the original manifest
//-- FIXING PREFIX BUG ID 3733
   const BGameFileManifest* pOrigManifest = NULL;
//--

   uint count = mGameFiles.getSize();
   for (uint i=0; i < count; ++i)
   {
      BGameFileManifest* pTemp = mGameFiles[i];
      if (pTemp && pTemp->getID() == pTask->getMediaID() && pTemp->getStorageType() == eStorageUser)
      {
         pOrigManifest = pTemp;
         break;
      }
   }

   // so we uploaded something but no longer have the uploaded item around to verify with?
   //
   // XXX TODO - add protection around deleting a file that's currently uploading/downloading
   //
   // the end result would be that a manifest could be missing if the user logged out or otherwise
   // left the screen(s) controlling this feature causing our manifest cache to flush
   if (pOrigManifest == NULL)
      return;

   BGameFileManifest* pManifest = new BGameFileManifest();
   pManifest->setTempID(InterlockedIncrementRelease(&mNextTempID));
   pManifest->setStorageType(eStorageServer);
   pManifest->setUserIndex(pTask->getUserIndex());
   pManifest->setRefresh();

   BGameFileHeader& header = pManifest->mHeader;

   // record game
   header.setID(pOrigManifest->getID());
   header.setLength(pOrigManifest->getLength());
   header.setDate(pOrigManifest->getDate());
   header.setXuid(pOrigManifest->getAuthorXuid());
   header.setName(pOrigManifest->getName());
   header.setDesc(pOrigManifest->getDesc());
   header.setAuthor(pOrigManifest->getAuthor());

   BSimString tempStr;
   tempStr.format("%I64u", pOrigManifest->getID());
   pManifest->setFilename(tempStr);
   pManifest->mFileSize = pOrigManifest->getFileSize();
   pManifest->mScenarioName = pOrigManifest->getScenarioName();
   pManifest->mTime = pManifest->getDate();

   pManifest->update();

   refreshResult(pManifest);

   // We may run into issues by forcing the latest searchID here
   gEventDispatcher.send(cInvalidEventReceiverHandle, mEventHandleSim, cEventClassRefreshComplete, 0, mSearchID);
}

//==============================================================================
// 
//==============================================================================
void BGameFile::mediaDeleteResponse(uint taskID, uint32 result)
{
   // find our outstanding task
//-- FIXING PREFIX BUG ID 3734
   const BLSPMediaTask* pTask = NULL;
//--
   for (uint i=0; i < mMediaTasks.getSize(); ++i)
   {
      if (mMediaTasks[i]->getID() == taskID)
      {
         pTask = mMediaTasks[i];
         break;
      }
   }

   if (pTask == NULL)
      return;

   if (pTask == mpDeleteMediaTask)
   {
      mpDeleteMediaTask = NULL;
      mListTTL = 0;
   }

   endDeleteFile(ERROR_SUCCESS);
}

//==============================================================================
// 
//==============================================================================
bool BGameFile::receiveEvent(const BEvent& event, BThreadIndex threadIndex)
{
   switch (event.mEventClass)
   {
      case cEventClassSave:
      {
         if (event.mFromHandle == mEventHandleIO)
         {
            BGameFilePayload* pPayload = reinterpret_cast<BGameFilePayload*>(event.mpPayload);
            BDEBUG_ASSERT(pPayload);

            endSaveFile(event.mPrivateData, pPayload);

            return true;
         }
         else if (event.mFromHandle == mEventHandleSim)
         {
            BGameFilePayload* pPayload = reinterpret_cast<BGameFilePayload*>(event.mpPayload);
            BDEBUG_ASSERT(pPayload);

            DWORD dwRet = saveFile(*pPayload);

            gEventDispatcher.send(mEventHandleIO, mEventHandleSim, cEventClassSave, dwRet, 0, pPayload);

            return true;
            // the save method fails, then we need to delete the payload, otherwise
            // it will forward on a request to refresh the cache
            //if (dwRet == ERROR_SUCCESS)
            //{
            //   gEventDispatcher.send(cInvalidEventReceiverHandle, mEventHandleSim, cEventClassContentEnumerate, pPayload->getUserIndex(), 0, pPayload);
            //   return true;
            //}
         }
         break;
      }

      case cEventClassSaveHeader:
      {
         if (event.mToHandle == mEventHandleIO)
         {
            // when the event comes from the misc thread, that means we're performing an edit of the header
            BGameFilePayload* pPayload = reinterpret_cast<BGameFilePayload*>(event.mpPayload);
            BDEBUG_ASSERT(pPayload);

            DWORD dwRet = saveHeader(*pPayload);

            gEventDispatcher.send(event.mToHandle, mEventHandleSim, cEventClassEdit, dwRet, 0, pPayload);

            return true;
         }
         else if (event.mToHandle == mEventHandleSim)
         {
            // display the saving dialog before sending the actual save command off to the IO thread
            BUIGlobals* pUIGlobals = gGame.getUIGlobals();
            BDEBUG_ASSERT(pUIGlobals);
            if (pUIGlobals)
               pUIGlobals->showWaitDialog(gDatabase.getLocStringFromID(23501));

            gEventDispatcher.send(event.mToHandle, mEventHandleIO, cEventClassSaveHeader, event.mPrivateData, event.mPrivateData2, event.mpPayload);

            return true;
         }
         break;
      }

      case cEventClassRefresh:
      {
         refreshStorage(event.mPrivateData, event.mPrivateData2);
         refreshDisk(event.mPrivateData, event.mPrivateData2);
         refreshCache(event.mPrivateData, event.mPrivateData2);
         gEventDispatcher.send(event.mToHandle, mEventHandleSim, cEventClassRefreshComplete, event.mPrivateData, event.mPrivateData2);
         break;
      }

      case cEventClassRefreshComplete:
      {
         if (event.mPrivateData2 == static_cast<uint>(mSearchID))
         {
            if (event.mFromHandle == mEventHandleMisc)
            {
               // if it came from the Misc thread, pass it on to the IO thread
               gEventDispatcher.send(event.mToHandle, mEventHandleIO, cEventClassRefreshComplete, event.mPrivateData, event.mPrivateData2);
            }
            else if (event.mFromHandle == mEventHandleSim)
            {
               // if it came from the Sim thread, that means we're on the IO thread, so pong it back to the Sim thread
               gEventDispatcher.send(event.mToHandle, mEventHandleSim, cEventClassRefreshComplete, event.mPrivateData, event.mPrivateData2);
            }
            else if (event.mFromHandle == mEventHandleIO || event.mToHandle == mEventHandleSim)
            {
               // now we should be done with the refresh
               refreshComplete();
            }
         }
         break;
      }

      case cEventClassRefreshManifest:
      {
         if (event.mFromHandle == mEventHandleIO)
         {
            // when it comes from the IO thread, then it has completed
            // the refresh manifest operation
            BGameFileManifest* pManifest = reinterpret_cast<BGameFileManifest*>(event.mpPayload);
            BDEBUG_ASSERT(pManifest);

            if (event.mPrivateData2 == static_cast<uint>(mSearchID))
            {
               refreshResult(pManifest);

               // do not allow the event dispatcher to delete this payload
               return true;
            }
         }
         else if (event.mToHandle == mEventHandleIO)
         {
            // when the event comes from the sim thread, that means we need to open
            // the file and refresh the manifest information
            const BGameFilePayload* pPayload = reinterpret_cast<const BGameFilePayload*>(event.mpPayload);
            BDEBUG_ASSERT(pPayload);
            if (!pPayload)
               break;

            // is it safe to delete the payload after I've refreshed the manifest?
            BGameFileManifest* pManifest = refreshManifest(*pPayload);

            // if we successfully retrieve the manifest, send an event back to the Sim thread
            // with the manifest information
            //
            // if we fail to retrieve the manifest, this request will be dropped on the floor
            // TODO : is that OK? what type of cleanup should I attempt to perform?
            if (pManifest)
               gEventDispatcher.send(mEventHandleIO, mEventHandleSim, cEventClassRefreshManifest, event.mPrivateData, event.mPrivateData2, pManifest);
         }
         break;
      }

      case cEventClassContentEnumerate:
      {
         // Store message in specified category array
         BGameFilePayload* pPayload = reinterpret_cast<BGameFilePayload*>(event.mpPayload);
         BDEBUG_ASSERT(pPayload);
         if (!pPayload)
            break;

         if (event.mPrivateData2 == static_cast<uint>(mSearchID))
         {
            enumerateResult(pPayload);

            // return true so the event dispatcher does not delete the payload
            // enumerateResult will deal with the memory handling
            return true;
         }
         break;
      }

      case cEventClassEdit:
      {
         if (event.mToHandle == mEventHandleMisc)
         {
            BGameFilePayload* pPayload = reinterpret_cast<BGameFilePayload*>(event.mpPayload);
            BDEBUG_ASSERT(pPayload);
            if (!pPayload)
               break;

            WCHAR text[BGameFileHeader::cMaxDescSize];

            long titleStrID;
            long descStrID;

            DWORD textLen;

            LPCWSTR defaultText = NULL;
            if (event.mPrivateData == BGameFile::cEditDesc)
            {
               titleStrID = 23513;
               descStrID = 23514;
               defaultText = pPayload->mDesc.getPtr();
               textLen = BGameFileHeader::cMaxDescLen;
            }
            else
            {
               // XXX we currently have no way of selecting an "Edit Name" feature
               // so this defaults to the "Edit Desc" string until we add said feature
               titleStrID = 23513;
               descStrID = 23514;
               defaultText = pPayload->mName.getPtr();
               textLen = BGameFileHeader::cMaxNameLen;
            }

            XOVERLAPPED overlapped;

            Utils::FastMemSet(&overlapped, 0, sizeof(XOVERLAPPED));

            overlapped.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

            DWORD dwRet = XShowKeyboardUI(pPayload->getUserIndex(), VKBD_LATIN_FULL, defaultText, gDatabase.getLocStringFromID(titleStrID), gDatabase.getLocStringFromID(descStrID), text, textLen, &overlapped);

            bool retval = false;

            if (dwRet == ERROR_IO_PENDING)
            {
               dwRet = WaitForSingleObject(overlapped.hEvent, INFINITE);
               if (dwRet == WAIT_OBJECT_0)
               {
                  dwRet = overlapped.dwExtendedError;
                  if (dwRet == ERROR_SUCCESS)
                  {
                     if (event.mPrivateData == BGameFile::cEditDesc)
                        pPayload->mDesc = text;
                     else
                        pPayload->mName = text;
                     retval = gEventDispatcher.send(event.mToHandle, mEventHandleSim, cEventClassSaveHeader, overlapped.dwExtendedError, 0, pPayload);
                  }
               }
            }

            if (dwRet != ERROR_SUCCESS)
            {
               // complete the edit with a failure
               retval = gEventDispatcher.send(event.mToHandle, mEventHandleSim, cEventClassEdit, dwRet, 0, pPayload);
            }

            CloseHandle(overlapped.hEvent);

            return retval;
         }
         else if (event.mToHandle == mEventHandleSim)
         {
            BGameFilePayload* pPayload = reinterpret_cast<BGameFilePayload*>(event.mpPayload);
            BDEBUG_ASSERT(pPayload);

            endEditFile(event.mPrivateData, pPayload);

            return true;
         }
         break;
      }

      case cEventClassDelete:
         {
            if (event.mToHandle == mEventHandleIO)
            {
               // perform the delete operation and then inform the sim thread that we're done
               BGameFilePayload* pPayload = reinterpret_cast<BGameFilePayload*>(event.mpPayload);
               BDEBUG_ASSERT(pPayload);

               DWORD dwRet = deleteFile(*pPayload);

               gEventDispatcher.send(event.mToHandle, mEventHandleSim, cEventClassDelete, dwRet, 0, pPayload);

               return true;
            }
            else if (event.mToHandle == mEventHandleSim)
            {
               BGameFilePayload* pPayload = reinterpret_cast<BGameFilePayload*>(event.mpPayload);
               BDEBUG_ASSERT(pPayload);

               endDeleteFile(event.mPrivateData, pPayload);

               return true;
            }
            break;
         }
  }

   return false;
}

//==============================================================================
//==============================================================================
void BGameFile::saveConfigs()
{
   saveConfigIsDefined(cConfigAIDisable);
   saveConfigIsDefined(cConfigAIShadow);
   saveConfigIsDefined(cConfigNoVismap);
   saveConfigIsDefined(cConfigNoRandomPlayerPlacement);
   saveConfigIsDefined(cConfigDisableOneBuilding);
   saveConfigIsDefined(cConfigBuildingQueue);
   saveConfigIsDefined(cConfigUseTestLeaders);
   saveConfigIsDefined(cConfigEnableFlight);
   saveConfigIsDefined(cConfigNoBirthAnims);
   saveConfigIsDefined(cConfigVeterancy);
   saveConfigIsDefined(cConfigTrueLOS);

   saveConfigIsDefined(cConfigNoDestruction);
   saveConfigIsDefined(cConfigCoopSharedResources);
   saveConfigIsDefined(cConfigMaxProjectileHeightForDecal);
   saveConfigIsDefined(cConfigEnableSubbreakage);
   saveConfigIsDefined(cConfigEnableThrowPart);
   saveConfigIsDefined(cConfigAllowAnimIsDirty);
   saveConfigIsDefined(cConfigNoVictoryCondition);
   saveConfigIsDefined(cConfigAIAutoDifficulty);
   saveConfigIsDefined(cConfigDemo);
   saveConfigIsDefined(cConfigAsyncWorldUpdate);
   saveConfigIsDefined(cEnableHintSystem);
   saveConfigIsDefined(cConfigPercentFadeTimeCorpseSink);
   saveConfigIsDefined(cConfigCorpseSinkSpeed);
   saveConfigIsDefined(cConfigCorpseMinScale);
   saveConfigIsDefined(cConfigBlockOutsideBounds);

   saveConfigIsDefined(cConfigAINoAttack);
   saveConfigIsDefined(cConfigPassThroughOwnVehicles);
   saveConfigIsDefined(cConfigEnableCapturePointResourceSharing);   
   saveConfigIsDefined(cConfigNoUpdatePathingQuad);
   saveConfigIsDefined(cConfigSlaveUnitPosition);
   saveConfigIsDefined(cConfigTurning);
   saveConfigIsDefined(cConfigHumanAttackMove);
   saveConfigIsDefined(cConfigMoreNewMovement3);
   saveConfigIsDefined(cConfigOverrideGroundIK);
   saveConfigIsDefined(cConfigDriveWarthog);
   saveConfigIsDefined(cConfigEnableCorpses);
   saveConfigIsDefined(cConfigDisablePathingLimits);
   saveConfigIsDefined(cConfigDisableVelocityMatchingBySquadType);
   saveConfigIsDefined(cConfigActiveAbilities);
   saveConfigIsDefined(cConfigAlpha);
   saveConfigIsDefined(cConfigNoDamage);
   saveConfigIsDefined(cConfigIgnoreAllPlatoonmates);
   saveConfigIsDefined(cConfigClassicPlatoonGrouping);
   saveConfigIsDefined(cConfigNoShieldDamage);
   saveConfigIsDefined(cConfigEnableSubUpdating);
   saveConfigIsDefined(cConfigMPSubUpdating);
   saveConfigIsDefined(cConfigAlternateSubUpdating);
   saveConfigIsDefined(cConfigDynamicSubUpdateTime);
   saveConfigIsDefined(cConfigDecoupledUpdate);

   saveConfigFloat(cConfigPlatoonRadius);
   saveConfigFloat(cConfigProjectionTime);
   saveConfigFloat(cConfigOverrideGroundIKRange);
   saveConfigFloat(cConfigOverrideGroundIKTiltFactor);
   saveConfigFloat(cConfigGameSpeed);
}

//==============================================================================
//==============================================================================
void BGameFile::saveConfigIsDefined(int config)
{
   bool val = gConfig.isDefined(config);
   mpStream->writeBytes(&val, sizeof(bool));
}

//==============================================================================
//==============================================================================
void BGameFile::saveConfigFloat(int config)
{
   float val = 0.0f;
   bool configSet = gConfig.get(config, &val);
   mpStream->writeBytes(&configSet, sizeof(bool));
   if (configSet)
      mpStream->writeBytes(&val, sizeof(float));
}

//==============================================================================
//==============================================================================
void BGameFile::loadConfigs()
{
   loadConfigIsDefined(cConfigAIDisable, mSaveConfigAIDisable);
   loadConfigIsDefined(cConfigAIShadow, mSaveConfigAIShadow);
   loadConfigIsDefined(cConfigNoVismap, mSaveConfigNoVismap);
   loadConfigIsDefined(cConfigNoRandomPlayerPlacement, mSaveConfigNoRandomPlayerPlacement);
   loadConfigIsDefined(cConfigDisableOneBuilding, mSaveConfigDisableOneBuilding);
   loadConfigIsDefined(cConfigBuildingQueue, mSaveConfigBuildingQueue);
   loadConfigIsDefined(cConfigUseTestLeaders, mSaveConfigUseTestLeaders);
   loadConfigIsDefined(cConfigEnableFlight, mSaveConfigEnableFlight);
   loadConfigIsDefined(cConfigNoBirthAnims, mSaveConfigNoBirthAnims);
   loadConfigIsDefined(cConfigVeterancy, mSaveConfigVeterancy);
   loadConfigIsDefined(cConfigTrueLOS, mSaveConfigTrueLOS);

   if (mBaseVersion >= 19)
   {
      loadConfigIsDefined(cConfigNoDestruction, mSaveConfigNoDestruction);
      loadConfigIsDefined(cConfigCoopSharedResources, mSaveConfigCoopSharedResources);
      loadConfigIsDefined(cConfigMaxProjectileHeightForDecal, mSaveConfigMaxProjectileHeightForDecal);
      loadConfigIsDefined(cConfigEnableSubbreakage, mSaveConfigEnableSubbreakage);
      loadConfigIsDefined(cConfigEnableThrowPart, mSaveConfigEnableThrowPart);
      loadConfigIsDefined(cConfigAllowAnimIsDirty, mSaveConfigAllowAnimIsDirty);
      loadConfigIsDefined(cConfigNoVictoryCondition, mSaveConfigNoVictoryCondition);
      loadConfigIsDefined(cConfigAIAutoDifficulty, mSaveConfigAIAutoDifficulty);
      loadConfigIsDefined(cConfigDemo, mSaveConfigDemo);
      loadConfigIsDefined(cConfigAsyncWorldUpdate, mSaveConfigAsyncWorldUpdate);
      loadConfigIsDefined(cEnableHintSystem, mSaveConfigEnableHintSystem);
      loadConfigIsDefined(cConfigPercentFadeTimeCorpseSink, mSaveConfigPercentFadeTimeCorpseSink);
      loadConfigIsDefined(cConfigCorpseSinkSpeed, mSaveConfigCorpseSinkSpeed);
      loadConfigIsDefined(cConfigCorpseMinScale, mSaveConfigCorpseMinScale);
      loadConfigIsDefined(cConfigBlockOutsideBounds, mSaveConfigBlockOutsideBounds);
   }

   loadConfigIsDefined(cConfigAINoAttack, mSaveConfigAINoAttack);
   loadConfigIsDefined(cConfigPassThroughOwnVehicles, mSaveConfigPassThroughOwnVehicles);
   loadConfigIsDefined(cConfigEnableCapturePointResourceSharing, mSaveConfigEnableCapturePointResourceSharing);      
   loadConfigIsDefined(cConfigNoUpdatePathingQuad, mSaveConfigNoUpdatePathingQuad);
   loadConfigIsDefined(cConfigSlaveUnitPosition, mSaveConfigSlaveUnitPosition);
   loadConfigIsDefined(cConfigTurning, mSaveConfigTurning);
   loadConfigIsDefined(cConfigHumanAttackMove, mSaveConfigHumanAttackMove);
   loadConfigIsDefined(cConfigMoreNewMovement3, mSaveConfigMoreNewMovement3);
   loadConfigIsDefined(cConfigOverrideGroundIK, mSaveConfigOverrideGroundIK);
   loadConfigIsDefined(cConfigDriveWarthog, mSaveConfigDriveWarthog);
   loadConfigIsDefined(cConfigEnableCorpses, mSaveConfigEnableCorpses);
   loadConfigIsDefined(cConfigDisablePathingLimits, mSaveConfigDisablePathingLimits);
   loadConfigIsDefined(cConfigDisableVelocityMatchingBySquadType, mSaveConfigDisableVelocityMatchingBySquadType);
   loadConfigIsDefined(cConfigActiveAbilities, mSaveConfigActiveAbilities);
   loadConfigIsDefined(cConfigAlpha, mSaveConfigAlpha);
   loadConfigIsDefined(cConfigNoDamage, mSaveConfigNoDamage);
   if (mBaseVersion >= 21)
   {
      loadConfigIsDefined(cConfigIgnoreAllPlatoonmates, mSaveConfigIgnoreAllPlatoonmates);
      loadConfigIsDefined(cConfigClassicPlatoonGrouping, mSaveConfigClassicPlatoonGrouping);
      loadConfigIsDefined(cConfigNoShieldDamage, mSaveConfigNoShieldDamage);
   }
   if (mBaseVersion >= 20)
      loadConfigIsDefined(cConfigEnableSubUpdating, mSaveConfigEnableSubUpdating);
   if (mBaseVersion >= 23)
      loadConfigIsDefined(cConfigMPSubUpdating, mSaveConfigMPSubUpdating);
   if (mBaseVersion >= 22)
   {
      loadConfigIsDefined(cConfigAlternateSubUpdating, mSaveConfigAlternateSubUpdating);
      loadConfigIsDefined(cConfigDynamicSubUpdateTime, mSaveConfigDynamicSubUpdateTime);
      loadConfigIsDefined(cConfigDecoupledUpdate, mSaveConfigDecoupledUpdate);
   }

   loadConfigFloat(cConfigPlatoonRadius, mSaveConfigPlatoonRadius);
   loadConfigFloat(cConfigProjectionTime, mSaveConfigProjectionTime);
   loadConfigFloat(cConfigOverrideGroundIKRange, mSaveConfigOverrideGroundIKRange);
   loadConfigFloat(cConfigOverrideGroundIKTiltFactor, mSaveConfigOverrideGroundIKTiltFactor);
   loadConfigFloat(cConfigGameSpeed, mSaveConfigGameSpeed);
}

//==============================================================================
//==============================================================================
void BGameFile::loadConfigIsDefined(int config, bool& saveTo)
{
   saveTo = gConfig.isDefined(config);
   bool val = false;
   if (mpStream->readBytes(&val, sizeof(bool)) != sizeof(bool))
      return;
   if (val)
      gConfig.define(config);
   else
      gConfig.remove(config);
}

//==============================================================================
//==============================================================================
void BGameFile::loadConfigFloat(int config, float& saveTo)
{
   saveTo = cMinimumFloat;
   gConfig.get(config, &saveTo);
   bool configSet = false;
   if (mpStream->readBytes(&configSet, sizeof(bool)) != sizeof(bool))
      return;
   if (configSet)
   {
      float val = 0.0f;
      if (mpStream->readBytes(&val, sizeof(float)) != sizeof(float))
         return;
      gConfig.set(config, val);
   }
   else
      gConfig.remove(config);
}

//==============================================================================
//==============================================================================
void BGameFile::restoreConfigs()
{
   if (mSaveConfigNoFogMask)
      gConfig.define(cConfigNoFogMask);
   else
      gConfig.remove(cConfigNoFogMask);

   restoreConfigIsDefined(cConfigAIDisable, mSaveConfigAIDisable);
   restoreConfigIsDefined(cConfigAIShadow, mSaveConfigAIShadow);
   restoreConfigIsDefined(cConfigNoVismap, mSaveConfigNoVismap);
   restoreConfigIsDefined(cConfigNoRandomPlayerPlacement, mSaveConfigNoRandomPlayerPlacement);
   restoreConfigIsDefined(cConfigDisableOneBuilding, mSaveConfigDisableOneBuilding);
   restoreConfigIsDefined(cConfigBuildingQueue, mSaveConfigBuildingQueue);
   restoreConfigIsDefined(cConfigUseTestLeaders, mSaveConfigUseTestLeaders);
   restoreConfigIsDefined(cConfigEnableFlight, mSaveConfigEnableFlight);
   restoreConfigIsDefined(cConfigNoBirthAnims, mSaveConfigNoBirthAnims);
   restoreConfigIsDefined(cConfigVeterancy, mSaveConfigVeterancy);
   restoreConfigIsDefined(cConfigTrueLOS, mSaveConfigTrueLOS);

   if (mBaseVersion >= 19)
   {
      restoreConfigIsDefined(cConfigNoDestruction, mSaveConfigNoDestruction);
      restoreConfigIsDefined(cConfigCoopSharedResources, mSaveConfigCoopSharedResources);
      restoreConfigIsDefined(cConfigMaxProjectileHeightForDecal, mSaveConfigMaxProjectileHeightForDecal);
      restoreConfigIsDefined(cConfigEnableSubbreakage, mSaveConfigEnableSubbreakage);
      restoreConfigIsDefined(cConfigEnableThrowPart, mSaveConfigEnableThrowPart);
      restoreConfigIsDefined(cConfigAllowAnimIsDirty, mSaveConfigAllowAnimIsDirty);
      restoreConfigIsDefined(cConfigNoVictoryCondition, mSaveConfigNoVictoryCondition);
      restoreConfigIsDefined(cConfigAIAutoDifficulty, mSaveConfigAIAutoDifficulty);
      restoreConfigIsDefined(cConfigDemo, mSaveConfigDemo);
      restoreConfigIsDefined(cConfigAsyncWorldUpdate, mSaveConfigAsyncWorldUpdate);
      restoreConfigIsDefined(cEnableHintSystem, mSaveConfigEnableHintSystem);
      restoreConfigIsDefined(cConfigPercentFadeTimeCorpseSink, mSaveConfigPercentFadeTimeCorpseSink);
      restoreConfigIsDefined(cConfigCorpseSinkSpeed, mSaveConfigCorpseSinkSpeed);
      restoreConfigIsDefined(cConfigCorpseMinScale, mSaveConfigCorpseMinScale);
      restoreConfigIsDefined(cConfigBlockOutsideBounds, mSaveConfigBlockOutsideBounds);
   }

   restoreConfigIsDefined(cConfigAINoAttack, mSaveConfigAINoAttack);
   restoreConfigIsDefined(cConfigPassThroughOwnVehicles, mSaveConfigPassThroughOwnVehicles);
   restoreConfigIsDefined(cConfigEnableCapturePointResourceSharing, mSaveConfigEnableCapturePointResourceSharing);         
   restoreConfigIsDefined(cConfigNoUpdatePathingQuad, mSaveConfigNoUpdatePathingQuad);
   restoreConfigIsDefined(cConfigSlaveUnitPosition, mSaveConfigSlaveUnitPosition);
   restoreConfigIsDefined(cConfigTurning, mSaveConfigTurning);
   restoreConfigIsDefined(cConfigHumanAttackMove, mSaveConfigHumanAttackMove);
   restoreConfigIsDefined(cConfigMoreNewMovement3, mSaveConfigMoreNewMovement3);
   restoreConfigIsDefined(cConfigOverrideGroundIK, mSaveConfigOverrideGroundIK);
   restoreConfigIsDefined(cConfigDriveWarthog, mSaveConfigDriveWarthog);
   restoreConfigIsDefined(cConfigEnableCorpses, mSaveConfigEnableCorpses);
   restoreConfigIsDefined(cConfigDisablePathingLimits, mSaveConfigDisablePathingLimits);
   restoreConfigIsDefined(cConfigDisableVelocityMatchingBySquadType, mSaveConfigDisableVelocityMatchingBySquadType);
   restoreConfigIsDefined(cConfigActiveAbilities, mSaveConfigActiveAbilities);
   restoreConfigIsDefined(cConfigAlpha, mSaveConfigAlpha);
   restoreConfigIsDefined(cConfigNoDamage, mSaveConfigNoDamage);
   if (mBaseVersion >= 21)
   {
      restoreConfigIsDefined(cConfigIgnoreAllPlatoonmates, mSaveConfigIgnoreAllPlatoonmates);
      restoreConfigIsDefined(cConfigClassicPlatoonGrouping, mSaveConfigClassicPlatoonGrouping);
      restoreConfigIsDefined(cConfigNoShieldDamage, mSaveConfigNoShieldDamage);
   }
   if (mBaseVersion >= 20)
      restoreConfigIsDefined(cConfigEnableSubUpdating, mSaveConfigEnableSubUpdating);
   if (mBaseVersion >= 23)
      restoreConfigIsDefined(cConfigMPSubUpdating, mSaveConfigMPSubUpdating);
   if (mBaseVersion >= 22)
   {
      restoreConfigIsDefined(cConfigAlternateSubUpdating, mSaveConfigAlternateSubUpdating);
      restoreConfigIsDefined(cConfigDynamicSubUpdateTime, mSaveConfigDynamicSubUpdateTime);
      restoreConfigIsDefined(cConfigDecoupledUpdate, mSaveConfigDecoupledUpdate);
   }

   restoreConfigFloat(cConfigPlatoonRadius, mSaveConfigPlatoonRadius);
   restoreConfigFloat(cConfigProjectionTime, mSaveConfigProjectionTime);
   restoreConfigFloat(cConfigOverrideGroundIKRange, mSaveConfigOverrideGroundIKRange);
   restoreConfigFloat(cConfigOverrideGroundIKTiltFactor, mSaveConfigOverrideGroundIKTiltFactor);
   restoreConfigFloat(cConfigGameSpeed, mSaveConfigGameSpeed);
}

//==============================================================================
//==============================================================================
void BGameFile::restoreConfigIsDefined(int config, bool restoreFrom)
{
   if (restoreFrom)
      gConfig.define(config);
   else
      gConfig.remove(config);
}

//==============================================================================
//==============================================================================
void BGameFile::restoreConfigFloat(int config, float restoreFrom)
{
   if (restoreFrom != cMinimumFloat)
      gConfig.set(config, restoreFrom);
   else
      gConfig.remove(config);
}

//==============================================================================
//==============================================================================
bool BGameFile::getHeaderAndSettings(BSimString& fileName, bool userStorage, XCONTENT_DATA* pContentData, BGameSettings* pSettings)
{
   reset();

   mGameFileName = fileName;
   mGameFileName.removeExtension();
   mGameFileName.append(mGameFileExt);

   if (userStorage)
      mpStream = getXContentStream(gUserManager.getPrimaryUser()->getPort(), *pContentData, cSFReadable | cSFOpenExisting | cSFSeekable | cSFOptimizeForSequentialAccess);
   else
      mpStream = getStream(mGameFileName, true);
   if (mpStream == NULL)
      return false;

   mpRawStream = mpStream;
   mUsingGameFileStream = false;
   mDetails.mValue = 0;

   if (mpStream->readBytes(&mDetails, sizeof(BGameFileVersion)) != sizeof(BGameFileVersion) || mDetails.mVersion > cBaseVersion)
      return false;

#ifdef BUILD_FINAL
   // we will not allow unencrypted or uncompressed data in final builds
   if (!mDetails.mEncryptData || !mDetails.mEncryptHeader || !mDetails.mCompressData)
      return false;
#endif

   mBaseVersion = mDetails.mVersion;

   BSHA1Gen sha1Gen;

   sha1Gen.update(&mDetails, sizeof(BGameFileVersion));

   // deserialize the header
   if (mDetails.mEncryptHeader)
   {
      BDecryptedStream* pDecryptStream = new BDecryptedStream(mpStream, cKeyPhrase);

      // force a base offset to that of the current stream
      pDecryptStream->setBaseOfs(mpStream->curOfs());

      if (!mHeader.deserialize(pDecryptStream, sha1Gen))
      {
         delete pDecryptStream;
         return false;
      }

      delete pDecryptStream;
   }
   else
   {
      if (!mHeader.deserialize(mpStream, sha1Gen))
         return false;
   }

   if (mDetails.mEncryptData)
   {
      uint64 k1, k2, k3;
      teaCryptInitKeys(cKeyPhrase, k1, k2, k3);
      mpDecryptRecStream = new BDecryptedStream(mpStream, k1, k2, mHeader.getKey3());
      mpDecryptRecStream->setBaseOfs(mpRawStream->curOfs());
      mpStream = mpDecryptRecStream;
   }

   if (mDetails.mCompressData)
   {
      mpInflateRecStream = new BInflateStream(*mpStream);
      mpStream = mpInflateRecStream;
   }

   DWORD checksum=0;
   mMultiplayer=false;
   mLocalPlayerID=1;
   bool retval = loadSettings(mBaseVersion, pSettings, mpStream, true, checksum, mMultiplayer, mLocalPlayerID);

   close();

   return retval;
}

//==============================================================================
//==============================================================================
BGameXContentStream::BGameXContentStream() :
   mCorrupt(false)
{
   IGNORE_RETURN(Utils::FastMemSet(&mOverlapped, 0, sizeof(mOverlapped)));
}

//==============================================================================
//==============================================================================
bool BGameXContentStream::open(DWORD userIndex, const XCONTENT_DATA& content, uint flags)
{
   BDEBUG_ASSERT(content.DeviceID != XCONTENTDEVICE_ANY);
   if (content.DeviceID == XCONTENTDEVICE_ANY)
      return false;

   DWORD dwRet;
   DWORD dwContentFlags = XCONTENTFLAG_OPENALWAYS;

   if (flags & cSFOpenExisting)
   {
      BOOL fUserIsCreator = FALSE;
      dwRet = XContentGetCreator(userIndex, &content, &fUserIsCreator, NULL, NULL);
      mCorrupt = (dwRet == ERROR_FILE_CORRUPT);
      if (dwRet != ERROR_SUCCESS)
      {
         // set an error code in the stream for later reference
         return false;
      }
      else if (!fUserIsCreator)
      {
         return false;
      }

      dwContentFlags = XCONTENTFLAG_OPENEXISTING;
   }
   else if (flags & cSFWritable)
      dwContentFlags = XCONTENTFLAG_CREATEALWAYS;

   dwRet = XContentCreate(userIndex, "phx", &content, dwContentFlags, NULL, NULL, &mOverlapped);
   if (dwRet != ERROR_IO_PENDING)
   {
      mCorrupt = (dwRet == ERROR_FILE_CORRUPT);
      return false;
   }

   // pop the spinner (if it's not already displayed)
   BUIGlobals* pUIGlobals = gGame.getUIGlobals();

   uint startTime = timeGetTime();
   bool waitdialog = false;

   while (!XHasOverlappedIoCompleted(&mOverlapped))
   {
      if (gEventDispatcher.getThreadIndex() == cThreadIndexSim)
      {
         if (timeGetTime() - startTime > 1000)
         {
            if (pUIGlobals && !pUIGlobals->getWaitDialogVisible())
            {
               // 25989 Please wait...
               pUIGlobals->showWaitDialog(gDatabase.getLocStringFromID(25989));
               waitdialog = true;
            }
            else
            {
               gGame.updateRender();
            }
         }
      }

      Sleep(1);
   }

   if (waitdialog && pUIGlobals)
      pUIGlobals->setWaitDialogVisible(false);

   DWORD dwDisposition = 0;
   dwRet = XGetOverlappedResult(&mOverlapped, &dwDisposition, TRUE);
   if (dwRet != ERROR_SUCCESS)
   {
      return false;
   }
   else if (flags & cSFOpenExisting && dwDisposition != XCONTENT_OPENED_EXISTING)
   {
      return false;
   }

   BString filename;
   filename.set(content.szFileName, XCONTENT_MAX_FILENAME_LENGTH);

   BString fullFilename;
   fullFilename.format("phx:\\%s", filename.getPtr());

   bool retval = BWin32FileStream::open(fullFilename, flags, &gWin32LowLevelFileIO);

   // set this after we open the stream
   // this prevents us from calling XContentClose in the close method
   // during the stream's open method
   mXContentOpened = true;

   return retval;
}

//==============================================================================
//==============================================================================
bool BGameXContentStream::close()
{
   if (XGetOverlappedResult(&mOverlapped, NULL, TRUE) == ERROR_IO_PENDING)
      XCancelOverlapped(&mOverlapped);

   IGNORE_RETURN(Utils::FastMemSet(&mOverlapped, 0, sizeof(mOverlapped)));

   return BXContentStream::close();
}

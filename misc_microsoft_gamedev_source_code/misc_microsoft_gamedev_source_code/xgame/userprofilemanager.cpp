//==============================================================================
// userprofilemanager.cpp
//
// Copyright (c) 2007 Ensemble Studios
//==============================================================================

// Includes
#include "common.h"
#include "userprofilemanager.h"
#include "xbox.h"
#include "user.h"
#include "humanPlayerAITrackingData.h"
//#include "XLastGenerated.h"
#include "config.h"
#include "configsgame.h"
#include "configsinput.h"
#include "userachievements.h"
#include "xonline.h"
#include "uiticker.h"
#include "gamemode.h"
#include "game.h"

// compression
#include "stream\dynamicStream.h"
#include "compressedStream.h"
#include "statsManager.h"

// Globals
BUserProfileManager gUserProfileManager;

// UPDATE THIS WHENEVER YOU CHANGE TITLE 1
const uint8 cTitle1Version = 27;

//==============================================================================
// BUserProfile::BUserProfile
//==============================================================================
BUserProfile::BUserProfile(XUSER_READ_PROFILE_SETTING_RESULT* pResults /*= NULL*/)
{
   mInitialized = false;
   mTitle1Dirty = false;
   mTitle2Dirty = false;
   mTitle3Dirty = false;

   mTitle1VersionChanged = false;
   mTotalGameTime = 0;
   mSkullBits = 0;
   mTimeLineBits = 0;
   mTimeLineVisitedBits = 0;

   mNumLeaders = 0;
   mNumMaps = 0;
   mNumModes = 0;

   mMatchmakingScore = 0;

   mpAchievementList = NULL;
   initializeAchievements();

   initTitle1();
   initTitle3();

   // Allocate the data for the return call
   for( DWORD i = 0; i < NUM_SETTINGS; ++i )
   {   
      if( i >= Setting_Title1 )
      {
         // Allocate area for title specific data
         mSettings[i].data.binary.pbData = new BYTE[ XPROFILE_SETTING_MAX_SIZE ];
         mSettings[i].data.binary.cbData = XPROFILE_SETTING_MAX_SIZE;
         Utils::FastMemSet(mSettings[i].data.binary.pbData, 0, sizeof(BYTE)*XPROFILE_SETTING_MAX_SIZE);
      }
      else
      {
         // Allocate area for strings
         mSettings[i].data.string.pwszData = new WCHAR[ 256 ];
         // cbData is the array size in bytes, not characters.
         mSettings[i].data.string.cbData = 256 * sizeof(WCHAR);
      }
   }

   // [5/1/2008 JRuediger] someone passed in bad data. Don't read from it.
   if(pResults == NULL)
      return;

   // copy over the results
   // Copy values into local struct
   bool title2DataWasEmpty = false;
   for( DWORD i=0; i < NUM_SETTINGS; ++i )
   {
      XUSER_PROFILE_SETTING* pDest = &(mSettings[i]);
//-- FIXING PREFIX BUG ID 2392
      const XUSER_PROFILE_SETTING* pSrc = &(pResults->pSettings[i]);
//--

      pDest->user = pSrc->user;
      pDest->source = pSrc->source;
      pDest->dwSettingId = SettingIDs[i];

      // Get the setting value only if there is one, else default it for display
      switch ( pSrc->data.type )
      {
      case XUSER_DATA_TYPE_INT32:
         pDest->data.nData = (XSOURCE_NO_VALUE == pSrc->source ? 0 : pSrc->data.nData);
         break;
      case XUSER_DATA_TYPE_INT64:
         pDest->data.i64Data = (XSOURCE_NO_VALUE == pSrc->source ? 0 : pSrc->data.i64Data);
         break;
      case XUSER_DATA_TYPE_DOUBLE:
         pDest->data.dblData = (XSOURCE_NO_VALUE == pSrc->source ? 0.0f : pSrc->data.dblData);
         break;
      case XUSER_DATA_TYPE_FLOAT:
         pDest->data.fData = (XSOURCE_NO_VALUE == pSrc->source ? 0.0f : pSrc->data.fData);
         break;
      case XUSER_DATA_TYPE_BINARY:
         if( pSrc->data.binary.pbData != NULL )
         {
            DWORD size = (pSrc->data.binary.cbData > pDest->data.binary.cbData ? pDest->data.binary.cbData : pSrc->data.binary.cbData);
            Utils::FastMemCpy(pDest->data.binary.pbData, pSrc->data.binary.pbData, size);
            pDest->data.binary.cbData = size;
         }
         else
         {
            *(DWORD*)pDest->data.binary.pbData = 0;
            pDest->data.binary.cbData = XPROFILE_SETTING_MAX_SIZE;
            if (i==Setting_Title2)
               title2DataWasEmpty=true;
         }
         break;

         // Copy string data
      case XUSER_DATA_TYPE_UNICODE:
         ZeroMemory(pDest->data.string.pwszData, pDest->data.string.cbData);

         if( pSrc->data.string.pwszData && pSrc->data.string.cbData )
         {
            Utils::FastMemCpy(pDest->data.string.pwszData, pSrc->data.string.pwszData, min(pSrc->data.string.cbData, 255 * sizeof(WCHAR)));
            pDest->data.string.pwszData[255] = 0;
            pDest->data.string.cbData = pSrc->data.string.cbData;
         }
/*
         if (pSrc->data.string.pwszData)
         {
            // The buffer length is in bytes but wcsncpy_s excepts a character count, so we divide
            // by sizeof(WCHAR).
            wcsncpy_s( const_cast<WCHAR*>(pDest->data.string.pwszData), pDest->data.string.cbData / sizeof(WCHAR),
               pSrc->data.string.pwszData, _TRUNCATE );
         }
         else
            const_cast<WCHAR*>(pDest->data.string.pwszData)[0] = 0;
*/
         break;
      }
   }

   // [5/1/2008 JRuediger] expand Title1 through 3 into local variables.  Return on failure so that our Init bool remains false.

   if(loadTitle1() == false)
   {
      //Dropping this short circuit on the method, even if the deserialize fails to work, we still want to continue processing
      //  so that other title sections can have their chance to set themselves to default values
      //return;
   }

   if(title2DataWasEmpty)
   {   
      //There was nothing here - so we need to set it to default values 
      //Create a default set of AI tracking data so that our memory block (for title2) is pre-loaded with valid, default values
      BHumanPlayerAITrackingData mDefaultTrackingData;
      mDefaultTrackingData.setDefaultvalues();
      BDynamicStream stream;
      bool result;
      result = mDefaultTrackingData.saveValuesToMemoryBlock(&stream,true);
      BASSERT(result);
      if (result)
      {
         result = saveCompressedData(&stream, Setting_Title2);
         BASSERT(result);
      }      
   }

   if(loadTitle3() == false)
   {
    //  return;
   }

   mInitialized = true;
}

//==============================================================================
// BUserProfile::~BUserProfile
//==============================================================================
BUserProfile::~BUserProfile()
{
   for( DWORD i = 0; i < NUM_SETTINGS; ++i )
   {   
      if( i >= Setting_Title1 )
      {
         // free area for title specific data
         delete mSettings[i].data.binary.pbData;
      }
      else
      {
         // free area for strings
         delete mSettings[i].data.string.pwszData;
      }
   }
   if (mpAchievementList != NULL)
      delete mpAchievementList;
}


//==============================================================================
// BUserProfile::compressInternalData
//==============================================================================
bool BUserProfile::compressInteralData(BUser* pUser)
{

   //Set the title version before writing them out
   mTitleOneVersion = cTitle1Version;
   //mAIMemoryVersion = 3;
   //mTitleTwoVersion = 0;

   // [5/1/2008 JRuediger] Compress our local data into 1k blocks. 
   if(saveTitle1() == false)
      return(false);

   //Have to ask other system to save the title 2 data - eric
   if (pUser->getPlayer() && pUser->getPlayer()->getTrackingData())
   {
      BDynamicStream stream;
      if (!pUser->getPlayer()->getTrackingData()->saveValuesToMemoryBlock(&stream))
         return false;
      if (!saveCompressedData(&stream, Setting_Title2))
         return false;
   }

   return(saveTitle3());

}

//============================================================================
//============================================================================
void BUserProfile::initTitle1( void )
{
   // [5/5/2008 JRuediger] set the defaults.

   //mCurrentCampaignNode = 0;
   mLastModePlayed = 0;

   // Game Options
   mOption_AIAdvisorEnabled = true;
   mOption_DefaultAISettings = DifficultyType::cAutomatic;
   mOption_DefaultCampaignDifficulty = DifficultyType::cNormal;

   // Control Options
   mOption_ControlScheme = eControlScheme1;
   BSimString controllerConfigName;
   if (gConfig.get(cConfigControllerConfig, controllerConfigName))
   {
      long index = gInputSystem.getControllerConfigIndex(controllerConfigName);
      if (index != -1)
         mOption_ControlScheme = (uint8)index;
   }

   XUSER_PROFILE_SETTING rumbleSetting = mSettings[Setting_Controller_Vibration];
   mOption_RumbleEnabled = !!rumbleSetting.data.nData;
   mOption_CameraRotationEnabled = true;
   mOption_XInverted = false;
   mOption_YInverted = false;

   float defaultZoom = 85.0f;
   gConfig.get( cConfigCameraZoom, &defaultZoom );
   mOption_DefaultZoomLevel = (uint8)defaultZoom;
   
   float stickyReticleSensitivity = 0.0f;
   gConfig.get(cConfigStickyReticleSensitivity, &stickyReticleSensitivity);
   mOption_StickyCrosshairSensitivity = (uint8)(stickyReticleSensitivity * 100);

   mOption_ScrollSpeed = 10;
   mOption_CameraFollowEnabled = gConfig.isDefined(cConfigStickyReticleFollow);
   mOption_HintsEnabled = true;
   mOption_SelectionSpeed = 1;
   // Audio Options
   mOption_MusicVolume = 8;
   mOption_SFXVolume = 8;
   mOption_VoiceVolume = 8;
   mOption_SubtitlesEnabled = gConfig.isDefined(cConfigSubTitles);
   mOption_ChatTextEnabled = gConfig.isDefined(cConfigSubTitles);
   // Graphics Options
   mOption_Brightness = 125;
   mOption_FriendOrFoeColorsEnabled = false;
   mOption_MiniMapZoomEnabled = false;

   mTitle1VersionChanged = false;

   mCampaignProgress.clear();
   mTotalGameTime = 0;
   mSkullBits = 0;
   mTimeLineBits = 0;
   mTimeLineVisitedBits = 0;

   mNumLeaders = 0;
   for(int i=0; i<gDatabase.getNumberLeaders(); i++)
   {
      BLeader *leader = gDatabase.getLeader(i);
      if( leader->mTest || (leader->mLeaderCivID != gDatabase.getCivID("UNSC") && leader->mLeaderCivID != gDatabase.getCivID("Covenant") ) )
         continue;
      BASSERT(mNumLeaders<SR_MAX_NUM_LEADERS);
      for(int j=0; j<2; j++)  //0 = unranked, 1 = ranked
      {
         mSkirmishLeaders[j][mNumLeaders].index = (uint16)i; 
         mSkirmishLeaders[j][mNumLeaders].wins = 0; 
         mSkirmishLeaders[j][mNumLeaders].games = 0; 
      }
      mNumLeaders++;
   }

   mNumMaps = 0;
   const BScenarioList& scenarioList = gDatabase.getScenarioList();
   for(int i=0; i<scenarioList.getMapCount(); i++)
   {
      const BScenarioMap *scenarioMap = scenarioList.getMapInfo(i);
      if( scenarioMap==NULL || (scenarioMap->getType() != BScenarioMap::cScenarioTypeFinal) )
         continue;
      BASSERT(mNumMaps<SR_MAX_NUM_MAPS);
      for(int j=0; j<2; j++)  //0 = unranked, 1 = ranked
      {
         mSkirmishMaps[j][mNumMaps].index = (uint16)i; 
         mSkirmishMaps[j][mNumMaps].wins = 0; 
         mSkirmishMaps[j][mNumMaps].games = 0; 
      }
      mNumMaps++;
   }

   mNumModes = 0;
   for(int i=0; i<gDatabase.getNumberGameModes(); i++)
   {
      //BGameMode *mode = gDatabase.getGameModeByID(i);
      //No false entries on this one, so accept all.
      BASSERT(mNumModes<SR_MAX_NUM_MODES);
      for(int j=0; j<2; j++)  //0 = unranked, 1 = ranked
      {
         mSkirmishModes[j][mNumModes].index = (uint16)i; 
         mSkirmishModes[j][mNumModes].wins = 0; 
         mSkirmishModes[j][mNumModes].games = 0; 
      }
      mNumModes++;
   }

   mMatchmakingRank.reset();
   mMatchmakingScore = 0;
}

//==============================================================================
// BUserProfile::loadTitle1
//
// This will uncompress the block of 1k data we loaded from the xbox and stuff
// those values into local variables.
//==============================================================================
bool BUserProfile::loadTitle1(void)
{
   XUSER_PROFILE_SETTING* pSetting = getProfileSetting(Setting_Title1);
   if(pSetting == NULL)
      return(false);

   initTitle1();

   // [5/5/2008 JRuediger] Read compressed data
   BDynamicStream* pSettingStream = new BDynamicStream();

   // [5/1/2008 JRuediger] Read how much we are reading.
   uint readbytes = *(reinterpret_cast<uint*>(pSetting->data.binary.pbData));
//-- FIXING PREFIX BUG ID 2394
   const uint* pReadDataPtr = reinterpret_cast<const uint*>(pSetting->data.binary.pbData+4);
//--

   // [5/2/2008 JRuediger] Bail on bad params.  Revert back to defaults.
   if(readbytes <= 0 || readbytes > (XPROFILE_SETTING_MAX_SIZE-sizeof(readbytes)) || pSettingStream == NULL)
   {
      delete pSettingStream;
      return(true);
   }

   // [5/1/2008 JRuediger] Read in the compressed data.
   pSettingStream->writeBytes(pReadDataPtr, readbytes);
   pSettingStream->seek(0);

   // [5/1/2008 JRuediger] Decompress it.  This can fail if we have never saved the profile because the header we read in will be malformed.
   BInflateStream* pInfStream = new BInflateStream(*pSettingStream);
   if(pInfStream == NULL)
   {
      delete pInfStream;
      delete pSettingStream;
      return(true);
   }

   if(pInfStream == NULL || pInfStream->errorStatus() == true)
   {
      // [5/1/2008 JRuediger] First time we have run the game, so set default values.
      pInfStream->close();
      delete pInfStream;
      delete pSettingStream;
      return(true);
   }

   *pInfStream >> mTitleOneVersion;

   mTitle1VersionChanged = ( mTitleOneVersion != cTitle1Version );

   if(mTitleOneVersion >= 2)
   {
      if(mTitleOneVersion < 5)  //moved in ver 5
      {
         int oldCurrentCampaignNode;
         *pInfStream >> oldCurrentCampaignNode;
      }
      *pInfStream >> mLastModePlayed;
   }

   if(mTitleOneVersion >= 3)
   {
      // Game Options
      *pInfStream >> mOption_AIAdvisorEnabled;
      *pInfStream >> mOption_DefaultAISettings;
      // Control Options
      *pInfStream >> mOption_ControlScheme;
      *pInfStream >> mOption_RumbleEnabled;
      *pInfStream >> mOption_CameraRotationEnabled;
      *pInfStream >> mOption_XInverted;
      *pInfStream >> mOption_YInverted;
      *pInfStream >> mOption_DefaultZoomLevel;
      *pInfStream >> mOption_StickyCrosshairSensitivity;
      *pInfStream >> mOption_ScrollSpeed;
      *pInfStream >> mOption_CameraFollowEnabled;
      *pInfStream >> mOption_HintsEnabled;
      *pInfStream >> mOption_SelectionSpeed;
      // Audio Options
      *pInfStream >> mOption_MusicVolume;
      *pInfStream >> mOption_SFXVolume;
      *pInfStream >> mOption_VoiceVolume;
      *pInfStream >> mOption_SubtitlesEnabled;
      if(mTitleOneVersion >= 18)
         *pInfStream >> mOption_ChatTextEnabled;
      // Graphics Options
      *pInfStream >> mOption_Brightness;
      *pInfStream >> mOption_FriendOrFoeColorsEnabled;
      *pInfStream >> mOption_MiniMapZoomEnabled;
   }

   if(mTitleOneVersion >= 22)//reset progress info if older version
   {
      mCampaignProgress.readFromStream(pInfStream, mTitleOneVersion);

      *pInfStream >> mTotalGameTime;
      *pInfStream >> mSkullBits;
      *pInfStream >> mTimeLineBits;
      *pInfStream >> mTimeLineVisitedBits;

      pInfStream->readObj<BProfileRank>(mMatchmakingRank);

      BASSERT(mpAchievementList);
      if(mpAchievementList)
         mpAchievementList->readFromStream(pInfStream);

      uint8 storedNumLeaders;
      uint8 storedNumMaps;
      uint8 storedNumModes;

      *pInfStream >> storedNumLeaders;
      *pInfStream >> storedNumMaps;
      *pInfStream >> storedNumModes;

      if( (storedNumLeaders == mNumLeaders) && (storedNumMaps == mNumMaps) && (storedNumModes == mNumModes) )
      {
         for(int i=0; i<mNumLeaders; i++)
         {
            for(int j=0; j<2; j++)
            {
               //Do not save or load the index.  This is set by the initialization
               *pInfStream >> mSkirmishLeaders[j][i].wins;
               *pInfStream >> mSkirmishLeaders[j][i].games;
            }
         }
         for(int i=0; i<mNumMaps; i++)
         {
            for(int j=0; j<2; j++)
            {
               //Do not save or load the index.  This is set by the initialization
               *pInfStream >> mSkirmishMaps[j][i].wins;
               *pInfStream >> mSkirmishMaps[j][i].games;
            }
         }
         for(int i=0; i<mNumModes; i++)
         {
            for(int j=0; j<2; j++)
            {
               //Do not save or load the index.  This is set by the initialization
               *pInfStream >> mSkirmishModes[j][i].wins;
               *pInfStream >> mSkirmishModes[j][i].games;
            }
         }
      }
   }

   if(mTitleOneVersion >= 24)
      *pInfStream >> mOption_DefaultCampaignDifficulty;

   if (mTitleOneVersion >= 27)
      *pInfStream >> mMatchmakingScore;

   // [5/5/2008 JRuediger] Cleanup
   bool success = !pInfStream->errorStatus();
   pInfStream->close();
   delete pInfStream;
   delete pSettingStream;

   return(success);
}

//==============================================================================
//
//==============================================================================
byte* BUserProfile::getAITrackingDataMemoryPointer()
{
   XUSER_PROFILE_SETTING* pSetting = getProfileSetting(Setting_Title2);
   if(pSetting == NULL)
      return(NULL);
   byte* pMemoryPtr = reinterpret_cast<byte*>(pSetting->data.binary.pbData);
   return (pMemoryPtr);
}

//==============================================================================
// BUserProfile::initTitle2
//
// This will uncompress the block of 1k data we loaded from the xbox and stuff
// those values into local variables.
//==============================================================================
void BUserProfile::initTitle2(void)
{
   //DEPRICATED - eric
   BASSERT(false);
   return;

   /*
   XUSER_PROFILE_SETTING* pSetting = getProfileSetting(Setting_Title2);
   if(pSetting == NULL)
      return(false);

   // [5/1/2008 JRuediger] First, set our default values, in case we have never saved a profile.
   mAIAvgResourcesPerGame.setAll(0.0f);
   mNumGamesPlayedVSAI = 0;
   mNumGamesTheAIWonVSMe = 0;
   mDidIWinLastGameVSAI = true;
   mFirstTimeAIAttackedMe = UINT32_MAX;
   mFirstTimeIAttackedAI = UINT32_MAX;
   mPreviousStratsAIUsedOnMe = 0;
   mAutoDifficultyLevel = 0;
   mAutoDifficultyNumGamesPlayed = 0;
   mRolledupSquadTotals.clear();
   mRolledupTechTotals.clear();
   mRolledupObjectTotals.clear();
   mAbilitiesTotal.clear();
   mPowersTotal.clear();
   */
}

//==============================================================================
// BUserProfile::loadTitle2
//
// This will uncompress the block of 1k data we loaded from the xbox and stuff
// those values into local variables.
//==============================================================================
bool BUserProfile::loadTitle2(void)
{

   //DEPRICATED - eric
   BASSERT(false);
   return false;

/*
   XUSER_PROFILE_SETTING* pSetting = getProfileSetting(Setting_Title2);
   if(pSetting == NULL)
      return(false);

   initTitle2();
   
   BDynamicStream* pSettingStream = new BDynamicStream();

   // [5/1/2008 JRuediger] Read how much we are reading.
   uint readbytes = *(reinterpret_cast<uint*>(pSetting->data.binary.pbData));
   uint* pReadDataPtr = reinterpret_cast<uint*>(pSetting->data.binary.pbData+4);

   // [5/2/2008 JRuediger] Bail on bad params.  Revert back to defaults.
   if(readbytes <= 0 || readbytes > (XPROFILE_SETTING_MAX_SIZE-sizeof(readbytes)) || pSettingStream == NULL)
   {
      delete pSettingStream;
      return(true);
   }

   // [5/1/2008 JRuediger] Read in the compressed data.
   pSettingStream->writeBytes(pReadDataPtr, readbytes);
   pSettingStream->seek(0);

   // [5/1/2008 JRuediger] Decompress it.  This can fail if we have never saved the profile because the header we read in will be malformed.
   BInflateStream* pInfStream = new BInflateStream(*pSettingStream);
   if(pInfStream == NULL)
   {
      delete pInfStream;
      delete pSettingStream;
      return(true);
   }

   if(pInfStream == NULL || pInfStream->errorStatus() == true)
   {
      // [5/1/2008 JRuediger] First time we have run the game, so set default values.
      pInfStream->close();
      delete pInfStream;
      delete pSettingStream;
      return(true);
   }

   // [5/2/2008 JRuediger] Start readin in variables.
   *pInfStream >> mAIMemoryVersion;
   
   uint numResources = 0;
   *pInfStream >> numResources;

   float resourceAmt = 0.0f;
   for(uint i=0; i<numResources; i++)
   {
      *pInfStream >> resourceAmt;
      mAIAvgResourcesPerGame.set(i, resourceAmt);
   }

   *pInfStream >> mNumGamesPlayedVSAI;
   *pInfStream >> mNumGamesTheAIWonVSMe;
   *pInfStream >> mDidIWinLastGameVSAI;
   *pInfStream >> mFirstTimeAIAttackedMe;
   *pInfStream >> mFirstTimeIAttackedAI;
   *pInfStream >> mPreviousStratsAIUsedOnMe;

   // [5/7/2008 JRuediger] Start of version 2
   if(mAIMemoryVersion >= 2)
   {
      *pInfStream >> mAutoDifficultyLevel;
      *pInfStream >> mAutoDifficultyNumGamesPlayed;
      
      int numObjects = 0;
      long protoID = 0;
      uint protoValue = 0;
      *pInfStream >> numObjects;
      for (int i=0; i < numObjects; ++i)
      {
         *pInfStream >> protoID;
         *pInfStream >> protoValue;
         mRolledupSquadTotals.insert(protoID, BStatRecorder::BStatTotal(protoValue, 0, 0, 0));
      }
      *pInfStream >> numObjects;
      for (int i=0; i < numObjects; ++i)
      {
         *pInfStream >> protoID;
         *pInfStream >> protoValue;
         mRolledupTechTotals.insert(protoID, BStatRecorder::BStatTotal(protoValue, 0, 0, 0));
      }
      *pInfStream >> numObjects;
      for (int i=0; i < numObjects; ++i)
      {
         *pInfStream >> protoID;
         *pInfStream >> protoValue;
         mRolledupObjectTotals.insert(protoID, BStatRecorder::BStatTotal(protoValue, 0, 0, 0));
      }
   }

   // [5/7/2008 JRuediger] Start of version 3
   if(mAIMemoryVersion >= 3)
   {
      int numObjects = 0;
      long first = 0;
      uint second = 0;
      *pInfStream >> numObjects;
      for (int i=0; i < numObjects; ++i)
      {
         *pInfStream >> first;
         *pInfStream >> second;
         mAbilitiesTotal.insert(first, second);
      }

      *pInfStream >> numObjects;
      for (int i=0; i < numObjects; ++i)
      {
         *pInfStream >> first;
         *pInfStream >> second;
         mPowersTotal.insert(first, second);
      }
   }
   

   bool success = !pInfStream->errorStatus();
   pInfStream->close();
   delete pInfStream;
   delete pSettingStream;
   
   return(success);
   */
}

//============================================================================
//============================================================================
void BUserProfile::initTitle3()
{

}

//==============================================================================
// BUserProfile::loadTitle3
//
// This will uncompress the block of 1k data we loaded from the xbox and stuff
// those values into local variables.
//==============================================================================
bool BUserProfile::loadTitle3(void)
{
   initTitle3();
   return(true);
}

//==============================================================================
// BUserProfile::saveTitle1
//
// This will compress the block of 1k data we want saved to the xbox.
//==============================================================================
bool BUserProfile::saveTitle1(void)
{
   BDynamicStream stream;

   uint64 lastOffSet = 0;
   lastOffSet = stream.curOfs();
   gConsoleOutput.debug("Profile Title 1 Memory total = %d  start", stream.curOfs(), stream.curOfs() - lastOffSet );   

   if(stream.writeObj(mTitleOneVersion) == false)
      return(false);
   //if(stream.writeObj(mCurrentCampaignNode) == false)  //moved in ver 5
   //   return(false);
   if(stream.writeObj(mLastModePlayed) == false)
      return(false);

   gConsoleOutput.debug("Profile Title 1 Memory total = %d  misc = %d", stream.curOfs(), stream.curOfs() - lastOffSet );   
   lastOffSet = stream.curOfs();

   // Game Options
   if(stream.writeObj(mOption_AIAdvisorEnabled) == false)
      return(false);
   if(stream.writeObj(mOption_DefaultAISettings) == false)
      return(false);
   // Control Options
   if(stream.writeObj(mOption_ControlScheme) == false)
      return(false);
   if(stream.writeObj(mOption_RumbleEnabled) == false)
      return(false);
   if(stream.writeObj(mOption_CameraRotationEnabled) == false)
      return(false);
   if(stream.writeObj(mOption_XInverted) == false)
      return(false);
   if(stream.writeObj(mOption_YInverted) == false)
      return(false);
   if(stream.writeObj(mOption_DefaultZoomLevel) == false)
      return(false);
   if(stream.writeObj(mOption_StickyCrosshairSensitivity) == false)
      return(false);
   if(stream.writeObj(mOption_ScrollSpeed) == false)
      return(false);
   if(stream.writeObj(mOption_CameraFollowEnabled) == false)
      return(false);
   if(stream.writeObj(mOption_HintsEnabled) == false)
      return(false);
   if(stream.writeObj(mOption_SelectionSpeed) == false)
      return(false);
   // Audio Options
   if(stream.writeObj(mOption_MusicVolume) == false)
      return(false);
   if(stream.writeObj(mOption_SFXVolume) == false)
      return(false);
   if(stream.writeObj(mOption_VoiceVolume) == false)
      return(false);
   if(stream.writeObj(mOption_SubtitlesEnabled) == false)
      return(false);
   if(stream.writeObj(mOption_ChatTextEnabled) == false)
      return(false);
   // Graphics Options
   if(stream.writeObj(mOption_Brightness) == false)
      return(false);
   if(stream.writeObj(mOption_FriendOrFoeColorsEnabled) == false)
      return(false);
   if(stream.writeObj(mOption_MiniMapZoomEnabled) == false)
      return(false);

   gConsoleOutput.debug("Profile Title 1 Memory total = %d  options = %d", stream.curOfs(), stream.curOfs() - lastOffSet );   
   lastOffSet = stream.curOfs();

   mCampaignProgress.writeToStream(stream);
   
   gConsoleOutput.debug("Profile Title 1 Memory total = %d  campaign progress = %d", stream.curOfs(), stream.curOfs() - lastOffSet );   
   lastOffSet = stream.curOfs();

   //Total game time elapsed
   if(stream.writeObj(mTotalGameTime) == false)
      return(false);

   if(stream.writeObj(mSkullBits) == false)
      return(false);

   if(stream.writeObj(mTimeLineBits) == false)
      return(false);
   if(stream.writeObj(mTimeLineVisitedBits) == false)
      return(false);

   if(stream.writeObj(mMatchmakingRank) == false)
      return(false);

   gConsoleOutput.debug("Profile Title 1 Memory total = %d  time and skull bits = %d", stream.curOfs(), stream.curOfs() - lastOffSet );   
   lastOffSet = stream.curOfs();

   BASSERT(mpAchievementList);
   if(mpAchievementList)
      mpAchievementList->writeToStream(stream);

   gConsoleOutput.debug("Profile Title 1 Memory total = %d  achievements = %d", stream.curOfs(), stream.curOfs() - lastOffSet );   
   lastOffSet = stream.curOfs();

   if(stream.writeObj(mNumLeaders) == false)
      return(false);
   if(stream.writeObj(mNumMaps) == false)
      return(false);
   if(stream.writeObj(mNumModes) == false)
      return(false);

   for(int i=0; i<mNumLeaders; i++)
   {
      for(int j=0; j<2; j++)
      {
         //Do not save or load the index.  This is set by the initialization
         if(stream.writeObj(mSkirmishLeaders[j][i].wins) == false)
            return(false);
         if(stream.writeObj(mSkirmishLeaders[j][i].games) == false)
            return(false);
      }
   }
   for(int i=0; i<mNumMaps; i++)
   {
      for(int j=0; j<2; j++)
      {
         //Do not save or load the index.  This is set by the initialization
         if(stream.writeObj(mSkirmishMaps[j][i].wins) == false)
            return(false);
         if(stream.writeObj(mSkirmishMaps[j][i].games) == false)
            return(false);
      }
   }
   for(int i=0; i<mNumModes; i++)
   {
      for(int j=0; j<2; j++)
      {
         //Do not save or load the index.  This is set by the initialization
         if(stream.writeObj(mSkirmishModes[j][i].wins) == false)
            return(false);
         if(stream.writeObj(mSkirmishModes[j][i].games) == false)
            return(false);
      }
   }

   if(stream.writeObj(mOption_DefaultCampaignDifficulty) == false)
      return(false);

   if(stream.writeObj(mMatchmakingScore) == false)
      return(false);

   gConsoleOutput.debug("Profile Title 1 Memory total = %d  skirmish games = %d", stream.curOfs(), stream.curOfs() - lastOffSet );   
   lastOffSet = stream.curOfs();

   return(saveCompressedData(&stream, Setting_Title1));
}

//==============================================================================
// BUserProfile::saveTitle2
//
// This will compress the block of 1k data we want saved to the xbox.
//==============================================================================
bool BUserProfile::saveTitle2(void)
{
   //DEPRICATED - eric
   BASSERT(false);
   return false;

   /*
   // [5/2/2008 JRuediger] Set the current Version to save out. Load will have to be modified if you change this number.
   
   BDynamicStream stream;
   if(stream.writeObj(mAIMemoryVersion) == false)
      return(false);

   
   
   // [5/1/2008 JRuediger] Write out the avg resources.
   long numResources = mAIAvgResourcesPerGame.getNumberResources();
   if(stream.writeObj(numResources) == false)
      return(false);

   for(long i=0; i<numResources; i++)
   {
      if(stream.writeObj(mAIAvgResourcesPerGame.get(i)) == false)
         return(false);
   }

   if(stream.writeObj(mNumGamesPlayedVSAI) == false)
      return(false);
   if(stream.writeObj(mNumGamesTheAIWonVSMe) == false)
      return(false);
   if(stream.writeObj(mDidIWinLastGameVSAI) == false)
      return(false);
   if(stream.writeObj(mFirstTimeAIAttackedMe) == false)
      return(false);
   if(stream.writeObj(mFirstTimeIAttackedAI) == false)
      return(false);
   if(stream.writeObj(mPreviousStratsAIUsedOnMe) == false)
      return(false);

   if(stream.writeObj(mAutoDifficultyLevel) == false)
      return(false);
   if(stream.writeObj(mAutoDifficultyNumGamesPlayed) == false)
      return(false);

   // [5/2/2008 JRuediger] This data was added for version 2.
   stream.writeObj(mRolledupSquadTotals.getSize());
   for (BStatRecorder::BStatTotalHashMap::const_iterator it = mRolledupSquadTotals.begin(); it != mRolledupSquadTotals.end(); ++it)
   {
      stream.writeObj(it->first);
      stream.writeObj(it->second.mBuilt);
   }
   stream.writeObj(mRolledupTechTotals.getSize());
   for (BStatRecorder::BStatTotalHashMap::const_iterator it = mRolledupTechTotals.begin(); it != mRolledupTechTotals.end(); ++it)
   {
      stream.writeObj(it->first);
      stream.writeObj(it->second.mBuilt);
   }
   stream.writeObj(mRolledupObjectTotals.getSize());
   for (BStatRecorder::BStatTotalHashMap::const_iterator it = mRolledupObjectTotals.begin(); it != mRolledupObjectTotals.end(); ++it)
   {
      stream.writeObj(it->first);
      stream.writeObj(it->second.mBuilt);
   }
   stream.writeObj(mAbilitiesTotal.getSize());
   for (BStatAbilityIDHashMap::iterator it = mAbilitiesTotal.begin(); it != mAbilitiesTotal.end(); ++it)
   {
      stream.writeObj(it->first);
      stream.writeObj(it->second);
   }
   stream.writeObj(mPowersTotal.getSize());
   for (BStatProtoPowerIDHash::iterator it = mPowersTotal.begin(); it != mPowersTotal.end(); ++it)
   {
      stream.writeObj(it->first);
      stream.writeObj(it->second);
   }

   return(saveCompressedData(&stream, Setting_Title2));
   */
}

bool BUserProfile::saveCompressedData(BDynamicStream* pCurrentStream, SETTINGS settings)
{
   if(pCurrentStream == NULL)
      return(false);

   // [5/1/2008 JRuediger] Create temp stream to compress into.
   BDynamicStream* pStream = new BDynamicStream();
   BDeflateStream* pDeflStream = new BDeflateStream(*pStream);
   pDeflStream->writeBytes(pCurrentStream->getBuf().getPtr(), pCurrentStream->getBuf().getSizeInBytes());
   pDeflStream->close();
   delete pDeflStream;

   gConsoleOutput.debug("Profile Memory uncompressed = %d  compressed = %d", pCurrentStream->getBuf().getSizeInBytes(), pStream->getBuf().getSizeInBytes() );   

   BASSERT( pStream->getBuf().getSizeInBytes() <= (XPROFILE_SETTING_MAX_SIZE-sizeof(uint)) );
   if(pStream->getBuf().getSizeInBytes() > (XPROFILE_SETTING_MAX_SIZE-sizeof(uint)))
   {
      // [5/2/2008 JRuediger] Houston we have a problem.  We can't save past (XPROFILE_SETTING_MAX_SIZE-sizeof(uint)) worth of data.
      // For now, don't save!   BADNESS.
      delete pStream;
      return(false);
   }

   XUSER_PROFILE_SETTING* pSetting = getProfileSetting(settings);
   if(pSetting == NULL)
   {
      delete pStream;
      return(false);
   }

   uint* pSizePtr = reinterpret_cast<uint*>(pSetting->data.binary.pbData);
   uint* pDataPtr = reinterpret_cast<uint*>(pSetting->data.binary.pbData+4);

   // [5/2/2008 JRuediger] Stuff the size of the buffer into the first 4 bytes. Then save the compressed buffer behind that.
   *pSizePtr = pStream->getBuf().getSizeInBytes();
   Utils::FastMemCpy(pDataPtr, pStream->getBuf().getPtr(), pStream->getBuf().getSizeInBytes());
   delete pStream;
   return(true);
}
//==============================================================================
// BUserProfile::saveTitle3
//
// This will compress the block of 1k data we want saved to the xbox.
//==============================================================================
bool BUserProfile::saveTitle3(void)
{
   return(true);
}

//==============================================================================
// BUserProfile::getProfileSetting
//==============================================================================
XUSER_PROFILE_SETTING * BUserProfile::getProfileSetting(uint profileSettingID)
{
   if (profileSettingID >= NUM_SETTINGS)
      return NULL;

   return &(mSettings[profileSettingID]);
}


//=============================================================================
// BUserProfile::initializeAchievements
//=============================================================================
void BUserProfile::initializeAchievements()
{
   if (mpAchievementList != NULL)
      delete mpAchievementList;

   mpAchievementList = new BUserAchievementList();
   mpAchievementList->initialize();
}

//=============================================================================
// 
//=============================================================================
void BUserProfile::updateMatchmakingRank(bool won, long score)
{
   if (score < 0)
      score = 0;

   uint s = static_cast<uint>(score);

   // do we only want to add score if they win?
   if (UINT32_MAX - mMatchmakingScore >= s)
      mMatchmakingScore += s;
   else
      mMatchmakingScore = UINT32_MAX;

   // only call this if you've won a matchmaking game
   if (won && mMatchmakingRank.mWon <= UINT16_MAX)
      mMatchmakingRank.mWon++;

   if (mMatchmakingScore < BRank::eLieutenant)
      mMatchmakingRank.mRank = 1;
   else if (mMatchmakingScore < BRank::eCaptain)
      mMatchmakingRank.mRank = 2;
   else if (mMatchmakingScore < BRank::eMajor)
      mMatchmakingRank.mRank = 3;
   else if (mMatchmakingScore < BRank::eCommander)
      mMatchmakingRank.mRank = 4;
   else if (mMatchmakingScore < BRank::eColonel)
      mMatchmakingRank.mRank = 5;
   else if (mMatchmakingScore < BRank::eBrigadier)
      mMatchmakingRank.mRank = 6;
   else if (mMatchmakingScore < BRank::eGeneral)
      mMatchmakingRank.mRank = 7;
   else
      mMatchmakingRank.mRank = 8;
}

//=============================================================================
// 
//=============================================================================
void BUserProfile::updateMatchmakingLevel(uint level)
{
   if (level > 50)
      mMatchmakingRank.mLevel = 50;
   else
      mMatchmakingRank.mLevel = level;

   mMatchmakingRank.mServerUpdated = 1;
}

//=============================================================================
// 
//=============================================================================
uint BUserProfile::getNextRankAt() const
{
   switch (mMatchmakingRank.mRank)
   {
      // case 0 == 0, Unranked, you only need to play 1 game, but don't need to win
      case 1:
         return (BRank::eLieutenant - mMatchmakingScore);
      case 2:
         return (BRank::eCaptain - mMatchmakingScore);
      case 3:
         return (BRank::eMajor - mMatchmakingScore);
      case 4:
         return (BRank::eCommander - mMatchmakingScore);
      case 5:
         return (BRank::eColonel - mMatchmakingScore);
      case 6:
         return (BRank::eBrigadier - mMatchmakingScore);
      case 7:
         return (BRank::eGeneral - mMatchmakingScore);
      // at rank 8, you're maxed out so there are no more wins that you need
   }
   return 0;
}

//=============================================================================
// 
//=============================================================================
BWinCount* BUserProfile::getLeaderWinsByIndex(int rankMode, int index)
{
   BWinCount* winCountPtr = NULL;
   for (int i=0; i<SR_MAX_NUM_LEADERS; i++)
   {
      winCountPtr = &mSkirmishLeaders[rankMode][i];
      if (winCountPtr->index == index)
         return(winCountPtr);
   }

   return(NULL);
}

//=============================================================================
// 
//=============================================================================
BWinCount* BUserProfile::getMapWinsByIndex(int rankMode, int index)
{
   BWinCount* winCountPtr = NULL;
   for (int i=0; i<SR_MAX_NUM_MAPS; i++)
   {
      winCountPtr = &mSkirmishMaps[rankMode][i];
      if (winCountPtr->index == index)
         return(winCountPtr);
   }

   return(NULL);
}

//=============================================================================
// 
//=============================================================================
BWinCount* BUserProfile::getModeWinsByIndex(int rankMode, int index)
{
   BWinCount* winCountPtr = NULL;
   for (int i=0; i<SR_MAX_NUM_MODES; i++)
   {
      winCountPtr = &mSkirmishModes[rankMode][i];
      if (winCountPtr->index == index)
         return(winCountPtr);
   }

   return(NULL);
}

//==============================================================================
// BUserProfile::reportSkirmishGameCompleted
//==============================================================================
void BUserProfile::reportSkirmishGameCompleted(bool win, long leader, BFixedStringMaxPath &map, long mode, bool ranked)
{
   int rankedInx = ranked ? 1 : 0;

   for(int i=0; i<mNumLeaders; i++)
   {
      if(mSkirmishLeaders[rankedInx][i].index == leader)
      {
         mSkirmishLeaders[rankedInx][i].games++;
         if(win)
            mSkirmishLeaders[rankedInx][i].wins++;
         break;
      }
   }

   for(int i=0; i<mNumMaps; i++)
   {
//      if(mSkirmishMaps[rankedInx][i].index == map)
      BASSERT(gDatabase.getScenarioList().getMapInfo(mSkirmishMaps[rankedInx][i].index));
      if( gDatabase.getScenarioList().getMapInfo(mSkirmishMaps[rankedInx][i].index)->getName().compare( map.getPtr() ) == 0 )
      {
         mSkirmishMaps[rankedInx][i].games++;
         if(win)
            mSkirmishMaps[rankedInx][i].wins++;
         break;
      }
   }

   for(int i=0; i<mNumModes; i++)
   {
      if(mSkirmishModes[rankedInx][i].index == mode)
      {
         mSkirmishModes[rankedInx][i].games++;
         if(win)
            mSkirmishModes[rankedInx][i].wins++;
         break;
      }
   }
}

//==============================================================================
// BUserProfile::setupTickerInfo
//==============================================================================
void BUserProfile::setupTickerInfo()
{
   BUString tempString = "";
   //Private skirmish games using %1!s!: %2!d! won  %3!d! played
   for(int i=0; i<mNumLeaders; i++)
   {
      long nameIndex = gDatabase.getLeader(mSkirmishLeaders[0][i].index)->mNameIndex;
      tempString.locFormat(gDatabase.getLocStringFromID(25715), gDatabase.getLocStringFromIndex(nameIndex).getPtr(), mSkirmishLeaders[0][i].wins, mSkirmishLeaders[0][i].games);
      BUITicker::addString(tempString, 4, -1, BUITicker::eUnrankedLeaderGames+i);
   }

   //Private skirmish games on map %1!s!: %2!d! won  %3!d! played
   for(int i=0; i<mNumMaps; i++)
   {
      long nameIndex = gDatabase.getScenarioList().getMapInfo(mSkirmishMaps[0][i].index)->getMapNameStringID();
      tempString.locFormat(gDatabase.getLocStringFromID(25716), gDatabase.getLocStringFromID(nameIndex).getPtr(), mSkirmishMaps[0][i].wins, mSkirmishMaps[0][i].games);
      BUITicker::addString(tempString, 4, -1, BUITicker::eUnrankedMapGames+i);
   }

   //Private skirmish games in %1!s! mode: %2!d! won  %3!d! played
   for(int i=0; i<mNumModes; i++)
   {
      BGameMode* mode = gDatabase.getGameModeByID(mSkirmishModes[0][i].index);
      if(mode)
      {
         tempString.locFormat(gDatabase.getLocStringFromID(25717), mode->getDisplayName().getPtr(), mSkirmishModes[0][i].wins, mSkirmishModes[0][i].games);
         BUITicker::addString(tempString, 4, -1, BUITicker::eUnrankedModeGames+i);
      }
   }

/*
   //Public skirmish games using %1!s!: %2!d! won  %3!d! played
   for(int i=0; i<mNumLeaders; i++)
   {
      long nameIndex = gDatabase.getLeader(mSkirmishLeaders[1][i].index)->mNameIndex;
      tempString.locFormat(gDatabase.getLocStringFromID(25712), gDatabase.getLocStringFromIndex(nameIndex).getPtr(), mSkirmishLeaders[1][i].wins, mSkirmishLeaders[1][i].games);
      BUITicker::addString(tempString, 4, -1, BUITicker::eRankedLeaderGames+i);
   }

   //Public skirmish games on map %1!s!: %2!d! won  %3!d! played
   for(int i=0; i<mNumMaps; i++)
   {
      long nameIndex = gDatabase.getScenarioList().getMapInfo(mSkirmishMaps[1][i].index)->getMapNameStringID();
      tempString.locFormat(gDatabase.getLocStringFromID(25713), gDatabase.getLocStringFromID(nameIndex).getPtr(), mSkirmishMaps[1][i].wins, mSkirmishMaps[1][i].games);
      BUITicker::addString(tempString, 4, -1, BUITicker::eRankedMapGames+i);
   }

   //Public skirmish games in %1!s! mode: %2!d! won  %3!d! played
   for(int i=0; i<mNumModes; i++)
   {
      BGameMode* mode = gDatabase.getGameModeByID(mSkirmishModes[1][i].index);
      if(mode)
      {
         tempString.locFormat(gDatabase.getLocStringFromID(25714), mode->getDisplayName().getPtr(), mSkirmishModes[1][i].wins, mSkirmishModes[1][i].games);
         BUITicker::addString(tempString, 4, -1, BUITicker::eRankedModeGames+i);
      }
   }
*/

}

//==============================================================================
// BProfileWriteAsyncTask::BProfileWriteAsyncTask
//==============================================================================
BProfileWriteAsyncTask::BProfileWriteAsyncTask() :
   BAsyncTask()
{
}

//==============================================================================
// BProfileWriteAsyncTask::BProfileWriteAsyncTask
//==============================================================================
BProfileWriteAsyncTask::~BProfileWriteAsyncTask()
{
}

//==============================================================================
// BProfileWriteAsyncTask::startWrite
//==============================================================================
bool BProfileWriteAsyncTask::startWrite(BUser* pUser)
{
   if (pUser == NULL)
      return false;

   if (!pUser->isSignedIn())
      return false;

   BUserProfile* pProfile = pUser->getProfile();

   // [5/1/2008 JRuediger] Don't know what to do here if this fails? Just don't save for now.
   if(pProfile != NULL && pProfile->compressInteralData(pUser) == false)
      return(false);

   // we only write the title specific profile settings
   DWORD dwNumWriteableSettings = NUM_SETTINGS - Setting_Title1;

   // the start of our writeable profiles settings are with the Title1 setting
   XUSER_PROFILE_SETTING* pWriteableSettingsArray = pProfile->getProfileSetting(Setting_Title1);

   DWORD dwStatus = XUserWriteProfileSettings( pUser->getPort(),  // Player index making the request
                                       dwNumWriteableSettings,      // Number of settings to write
                                       pWriteableSettingsArray,         // List of settings to write
                                       &mOverlapped );             // Overlapped struct

   BASSERT( dwStatus == ERROR_SUCCESS || dwStatus == ERROR_IO_PENDING );
   dwStatus; // mrh 8/8/07 - removed warning.

   return true;
}

//==============================================================================
// BProfileWriteAsyncTask::processResult
//==============================================================================
void BProfileWriteAsyncTask::processResult()
{
   BAsyncTask::processResult();

   //Notify the user that the call to XUserWriteProfileSettings failed (with a friendly message, of course)
   DWORD result;
   getResult(&result);

   //Also check XGetOverlappedExtendedError, as this can return an error code when XGetOverlappedResult
   //does not (for example, in the case when write fails because the storage device is full).
   DWORD extendedError = getExtendedError();

   if (result != ERROR_SUCCESS || extendedError != ERROR_SUCCESS)
   {
      BUIGlobals* pUIGlobals = gGame.getUIGlobals();
      BASSERT(pUIGlobals);
      if (pUIGlobals)
         pUIGlobals->showYornBox(NULL, gDatabase.getLocStringFromID(25979), BUIGlobals::cDialogButtonsOK);
   }
}

//-------------------------------------------------------------------------------------

//==============================================================================
// BProfileReadAsyncTask::BProfileReadAsyncTask
//==============================================================================
BProfileReadAsyncTask::BProfileReadAsyncTask() :
   BAsyncTask(),
   mdwSettingSizeMax(0),
   mpSettingResults(NULL),
   mpUser(NULL)
{

}

//==============================================================================
// BProfileReadAsyncTask::~BProfileReadAsyncTask
//==============================================================================
BProfileReadAsyncTask::~BProfileReadAsyncTask()
{
   if (mpSettingResults)
      delete mpSettingResults;
}

//==============================================================================
// BProfileReadAsyncTask::startRead
// if pUser is specified, the user's profile will be read.  Otherwise it will
// try to read the profile of the remote user specified by the xuid.
//==============================================================================
bool BProfileReadAsyncTask::startRead(BUser* pUser, BUser *pRemoteUser, XUID xuid)
{
   if (pUser == NULL)
      return false;

   // if the user is not signed in, we cannot get the profile data
   if (!pUser->isSignedIn())
      return false;

   if (!pRemoteUser)
   {
      // XXX verify that the memory for pUser is guaranteed
      mpUser = pUser;

      // determine how much memory we need to allocate to read the profile settings
      DWORD dwStatus = XUserReadProfileSettings(   0,                      // A title in your family or 0 for the current title
                                                   0,                      // Player index (not used)
                                                   NUM_SETTINGS,           // Number of settings to read
                                                   SettingIDs,             // List of settings to read
                                                   &mdwSettingSizeMax,    // Results size (0 to determine maximum)
                                                   NULL,                   // Results (not used)
                                                   NULL );                 // Overlapped (not used)


      // validate the return code and data
      BASSERT( dwStatus == ERROR_INSUFFICIENT_BUFFER );
      BASSERT( mdwSettingSizeMax > 0 );

      // NOTE: The game is responsible for freeing this memory when it is no longer needed
      BYTE* pData = new BYTE [ mdwSettingSizeMax ];
      mpSettingResults = ( XUSER_READ_PROFILE_SETTING_RESULT* )pData;

      // kick off the read for the profile
      ZeroMemory( &mOverlapped , sizeof( mOverlapped  ) );
      dwStatus = XUserReadProfileSettings( 0,                   // A title in your family or 0 for the current title
                                       pUser->getPort(),     // Player index making the request
                                       NUM_SETTINGS,        // Number of settings to read
                                       SettingIDs,          // List of settings to read
                                       &mdwSettingSizeMax, // Results size
                                       mpSettingResults,   // Results go here
                                       &mOverlapped );     // Overlapped struct

      BASSERT( dwStatus == ERROR_SUCCESS || dwStatus == ERROR_IO_PENDING );
   }
   else
   {
      // XXX verify that the memory for pUser is guaranteed
      mpUser = pRemoteUser;
      mXuid = xuid;

      // determine how much memory we need to allocate to read the profile settings
      DWORD dwStatus = XUserReadProfileSettingsByXuid(   0,                      // A title in your family or 0 for the current title
                                                         pUser->getPort(),       // requesting Player index 
                                                         1,                      // number of xuids
                                                         &mXuid,                   // xbox Player user id
                                                         NUM_SETTINGS,           // Number of settings to read
                                                         SettingIDs,             // List of settings to read
                                                         &mdwSettingSizeMax,     // Results size (0 to determine maximum)
                                                         NULL,                   // Results (not used)
                                                         NULL );                 // Overlapped (not used)


      // validate the return code and data
      BASSERT( dwStatus == ERROR_INSUFFICIENT_BUFFER );
      BASSERT( mdwSettingSizeMax > 0 );

      // NOTE: The game is responsible for freeing this memory when it is no longer needed
      BYTE* pData = new BYTE [ mdwSettingSizeMax ];
      mpSettingResults = ( XUSER_READ_PROFILE_SETTING_RESULT* )pData;

      // kick off the read for the profile
      ZeroMemory( &mOverlapped , sizeof( mOverlapped  ) );
      dwStatus = XUserReadProfileSettingsByXuid(   0,                  // A title in your family or 0 for the current title
                                                   pUser->getPort(),   // Player index making the request
                                                   1,                  // number of xuids
                                                   &mXuid,               // xbox Player user id
                                                   NUM_SETTINGS,       // Number of settings to read
                                                   SettingIDs,         // List of settings to read
                                                   &mdwSettingSizeMax, // Results size
                                                   mpSettingResults,   // Results go here
                                                   &mOverlapped );     // Overlapped struct

      BASSERT( dwStatus == ERROR_SUCCESS || dwStatus == ERROR_IO_PENDING );
   }

   return true;
}

//==============================================================================
// BProfileReadAsyncTask::processResult
//==============================================================================
void BProfileReadAsyncTask::processResult()
{
   BAsyncTask::processResult();

   if (getResult() != ERROR_SUCCESS)
   {
      mpUser->setProfile(NULL);
      return;
   }

   BASSERT (mpSettingResults->dwSettingsLen == NUM_SETTINGS);

   // create the profile
   BUserProfile* pProfile = new BUserProfile(mpSettingResults);

   // send the profile to the user
   mpUser->setProfile(pProfile);

   return;
}



//==============================================================================
// BUserProfileManager::BUserProfileManager
//==============================================================================
BUserProfileManager::BUserProfileManager()
{
}

//==============================================================================
// BUserProfileManager::~BUserProfileManager
//==============================================================================
BUserProfileManager::~BUserProfileManager()
{
}

//==============================================================================
// BUserProfileManager::readProfile
//==============================================================================
bool BUserProfileManager::readProfile(BUser* pUser, BUser *pRemoteUser, XUID xuid)
{
   if (pUser == NULL)
      return false;

   // if the user is not signed in, we cannot get the achievements
   if (!pUser->isSignedIn())
      return false;

   BProfileReadAsyncTask* pTask = new BProfileReadAsyncTask();

   if (!pTask->startRead(pUser, pRemoteUser, xuid))
   {
      delete pTask;
      return false;
   }

   pTask->setNotify(this);

   // Let the task manager finish the call
   gAsyncTaskManager.addTask(pTask);

   return true;
}

//==============================================================================
// BUserProfileManager::notify
//==============================================================================
void BUserProfileManager::notify(DWORD eventID, void* pTask)
{
   // fixme - notify somebody about the profile read or write

   //Update the profile info in the ticker
   BUITicker::profileUpdate();
}

//==============================================================================
// BUserProfileManager::writeProfile
//==============================================================================
bool BUserProfileManager::writeProfile(BUser* pUser)
{

   // fixme - we need to watch for XN_SYS_PROFILESETTINGCHANGED
   // and re-read the profile of the users when we get this system notification.
   if (pUser == NULL)
      return false;

   if (!pUser->isSignedIn())
      return false;

   // [11/21/2008 xemu] Fix for PHX-18610 / TCR #073.  Basically we could have Achievements the user has earned,
   //   but that we couldn't write out before due to MU profile being removed.  So lets attempt to grant those again, to 
   //   try and get in sync.
   gAchievementManager.fixupEarnedAchievements(pUser);

   BProfileWriteAsyncTask* pTask = new BProfileWriteAsyncTask();

   if (!pTask->startWrite(pUser))
   {
      delete pTask;
      return false;
   }

   pTask->setNotify(this);

   // Let the task manager finish the call
   gAsyncTaskManager.addTask(pTask);

   return true;
}


//====================================================`==========================
// soundmanager.h
//
// Copyright (c) 2006 Ensemble Studios
//==============================================================================

#ifndef _SOUNDMANAGER_H_
#define _SOUNDMANAGER_H_

// Includes
#include "common.h"
#include "..\extlib\wwise\include\ak\SoundEngine\Common\AkTypes.h"
#include "..\extlib\wwise\include\ak\SoundEngine\Common\AkCallback.h"
/*
#include <AK/SoundEngine/Common/AkTypes.h>
#include <AK/SoundEngine/Common/AkCallback.h>
*/

//#define SOUND_RELOAD_BANKS
//#define SOUND_REREGISTER_OBJECTS

//==============================================================================
// Forward declarations
class BSoundManager;
class BAkDefaultLowLevelIO;
class CAkDefaultLowLevelIO;
class CAkFilePackageLowLevelIO;

//==============================================================================
// Externs
extern BSoundManager gSoundManager;

//==============================================================================
// Sound events
//==============================================================================
enum
{
   cSoundEventStop,
   cSoundEventBankLoaded,
   cSoundEventPrepare,
   cSoundEventUnprepare
};

//==============================================================================
// BSoundEventParams
//==============================================================================
struct BSoundEventParams
{
   //-- Sound Event params
   long eventType;
   BCueHandle in_playingID;
   BCueIndex in_eventID;
   BCueIndex in_gameObj;

   //-- Bank Event params
   AkBankID in_bankID;
   AKRESULT in_status;

};

//==============================================================================
// BSoundPrepareEventData
//==============================================================================
struct BSoundPrepareEventData
{
   BCueIndex   eventID;
   int         wiseObjID;
   bool        posted;
};


#ifndef BUILD_FINAL
//==============================================================================
// BSoundNameAndHashPair
//==============================================================================
struct BSoundNameAndHashPair
{
   DWORD cueIndex;
   BString cueName;
};

//==============================================================================
// BSoundPlaybackEntry
//==============================================================================
struct BSoundPlaybackEntry
{
   DWORD time;
   DWORD objectID;
   DWORD cueIndex;
};

//==============================================================================
// BSoundPlayback
//==============================================================================
class BSoundPlayback 
{
   public:
      BSoundPlayback(){}
      ~BSoundPlayback(){}

      void update();
      void init(const char *filename);

      bool mbPlayback;
      long mCurrentLine;
      long mNumLines;
      DWORD mBaseTime;

      BDynamicArray<BSoundPlaybackEntry> mArray;
};
#endif 

//==============================================================================
//==============================================================================
class ISoundEventHandler
{
   public:
      virtual void handleSoundEvent(BSoundEventParams& params) = 0;
};

//==============================================================================
//=============================================================================
class ISoundInfoProvider
{
   public:
      virtual BCueIndex getSoundCueIndex(const char* pName) const = 0;      
      virtual long getSoundDirID() const = 0;
      virtual long getDataDirID() const = 0;
};


//==============================================================================
// BSoundManager
//==============================================================================
class BSoundManager : public ISoundEventHandler
{
   public:      
      //-- Game Sound Cues
      enum
      {
         cSoundCueNone,
         cSoundLocalMilitary,         
         cSoundGlobalMilitary,
         cSoundHotkeyBase,
         cSoundHotkeyNode,
         cSoundFlare,
         cSoundFlareLook,
         cSoundFlareHelp,
         cSoundFlareMeet,
         cSoundFlareAttack,
         cSoundSupportPowerAvailable,         
         cSoundAttackNotificationUNSCSev1,
         cSoundAttackNotificationUNSCBaseSev1,         
         cSoundAttackNotificationCovSev1,
         cSoundAttackNotificationCovBaseSev1,         
         cSoundResearchComplete,
         cSoundPlayerResigned,
         cSoundPlayerDefeated,
         cSoundGameWon,
         cSoundPlaybackDone,
         cSoundGroupSelect1,
         cSoundGroupSelect2,
         cSoundGroupSelect3,
         cSoundGroupSelect4,
         cSoundGroupCreate1,
         cSoundGroupCreate2,
         cSoundGroupCreate3,
         cSoundGroupCreate4,
         cSoundBattleLost,
         cSoundBattleWon,
         cSoundWinningBattle,
         cSoundLosingBattle,
         cSoundStopAll,
         cSoundStopAllExceptMusic,
         cSoundTributeReceived,
         cSoundSwitchArcadia,
         cSoundSwitchHarvest,
         cSoundSwitchSWI,
         cSoundSwitchSWE,
         cSoundMusicSwitchArcadia,
         cSoundMusicSwitchHarvest,
         cSoundMusicSwitchSWI,
         cSoundMusicSwitchSWE,
         cSoundObjectiveOnDisplay,
         cSoundMuteMusic,
         cSoundUnmuteMusic,
         cSoundMuteMasterBus,
         cSoundUnmuteMasterBus,
         //--Music
         cSoundMusicPlayPreGame,
         cSoundMusicStopPreGame,
         cSoundMusicSetStateMainTheme,         
         cSoundMusicSetStateCampaignMenu,
         cSoundMusicSetStateSkirmishMenu,
         cSoundMusicPlayInGame,
         cSoundMusicStopInGame,
         cSoundMusicGameWon,
         cSoundMusicGameLost,
         cSoundMusicStopPostGame,

         cSoundMusicSetStateSPCLost,

         cSoundDeflect,
         cSoundStartBlur,
         // Halwes - 8/22/2008 - SPC VoG Sounds - Begin
         cSoundVOGNeedSupplies,
         cSoundVOGNeedReactors,
         cSoundVOGNeedPop,
         cSoundVOGHeroDownForge,
         cSoundVOGHeroDownAnders,
         cSoundVOGHeroDownSpartan,
         cSoundVOGHeroReviveForge,
         cSoundVOGHeroReviveAnders,
         cSoundVOGHeroReviveSpartan,
         // Halwes - 8/22/2008 - SPC VoG Sounds - End
         cSoundVOGBaseDestroyed,
         cSoundVOGLoss,
         cSoundVOGLossTeam,
         cSoundVOGWin,
         cSoundVOGWinTeam,
         cSoundMax,
      };


      //-- Typedefs
      struct BBankInfo
      {  
         BString mBankName;
         AkBankID mBankID;
      };
      typedef BDynamicSimArray<BBankInfo> BBankInfoArray;
      
      struct BQueuedSoundInfo
      {
         BCueIndex mCueIndex;
         BWwiseObjectID       mWiseObjectID;
      };
      typedef BDynamicSimArray<BQueuedSoundInfo> BQueuedSoundInfoArray;

      //-- Listener player objects for split screen
      enum
      {
         cPlayer1,
         cPlayer2,      
         cPlayerMax,
      };

      enum
      {
         cMaxSoundQueueSize = 4
      };

                              BSoundManager();
                              ~BSoundManager();

      bool                    setup();
      bool                    loadXML();
      bool                    initSoundEngine();
      void                    shutdown();
      void                    update();
      void                    worldReset();
      void                    loadChatterIDs();

      void                    setSoundInfoProvider(ISoundInfoProvider* provider) { mpSoundInfoProvider = provider; }

      void                    addEventHandler(ISoundEventHandler* handler) { mEventHandlers.add(handler); }
      void                    removeEventHandler(ISoundEventHandler* handler) { mEventHandlers.remove(handler); }

      BCueIndex               getCueIndex(const char* cueName);
      BCueIndex               getCueIndexByEnum(uint soundEnum);
      BCueHandle              playSoundCueByEnum(uint soundEnum);
      BCueHandle              playCue(BCueIndex cueIndex, BWwiseObjectID wiseObjectID=cInvalidWwiseObjectID, bool queue=false);      
      BCueHandle              playCue(const char* cueName, BWwiseObjectID wiseObjectID=cInvalidWwiseObjectID, bool queue=false);      
      void                    removeObject(BWwiseObjectID wiseObjectID);
      void                    addObject(BWwiseObjectID wiseObjectID);

      void                    updateListener(int listener, const BVector& pos, const BVector& forward, const BVector& up, const BVector& velocity);
      void                    updateSound3D(BWwiseObjectID wiseObjectID, BVector pos, BVector orientation);
      void                    handleNotification(AkUInt32 in_callbackType, AkPlayingID in_playingID, AkUniqueID	in_eventID, AkGameObjectID in_gameObj, void * in_pCookie, void * in_pData, AkUInt32 in_uDataSize);
      void                    handleNotification(AkBankID in_bankID,	AKRESULT in_eLoadResult, AkMemPoolId in_memPoolId, int in_prepare, void *in_pCookie);      
      static void             callbackFunc(AkCallbackType in_eType, AkCallbackInfo* in_pCallbackInfo);
      static void             bankCallbackFunc(AkBankID in_bankID, AKRESULT in_eLoadResult, AkMemPoolId in_memPoolId, void *in_pCookie);
      static void             prepareEventCallbackFunc(AkBankID in_bankID, AKRESULT in_eLoadResult, AkMemPoolId in_memPoolId, void *in_pCookie);
      static void             unprepareEventCallbackFunc(AkBankID in_bankID, AKRESULT in_eLoadResult, AkMemPoolId in_memPoolId, void *in_pCookie);
      void                    setSoundScapeScale(float scale);
      void                    toggleMute(void);
      void                    overrideBackgroundMusic(bool val);

      void                    updateFOWvolume(long wiseObjectID, float &currentFOWVolume, bool visible, double updateDelta, float fadeInRate, float fadeOutRate);
      void                    updateEnginePitch(long wiseObjectID, float &currentEnginePitch, float targetEnginePitch, double updateDelta, float pitchChangeRate, float maxChange);
      void                    setRTPCValue(long wiseObjectID, unsigned long rtpcID, float value);

      void                    setSplitScreen(bool val);
      void                    setGameStartDelay(bool val);

      void                    getDefaultBankMemoryStats(uint &currAllocations, uint &currMemAllocated);

#ifndef BUILD_FINAL
      void                    getMemoryInformation(unsigned long* engineMem, long* engineAlloc, unsigned long* physMem, long* physAlloc, unsigned long* virtMem, long* virtAlloc);
      void                    startSoundPlaybackFile(const char *filename) { mSoundPlayback.init(filename); }
      bool                    toggleSoundCueOutput(void);
#endif 
      void                    resetSoundManager();      

      AkBankID                loadSoundBank(const BString &bankName, bool asynch);
      bool                    unloadSoundBank(const BString &bankName, bool asynch);
      bool                    unloadSoundBank(AkBankID bankID, bool asynch);

      void handleSoundEvent(BSoundEventParams& params);

      BString                 getPregameBank() const { return mPreGameBank; }

      BDynamicArray<BString>  getLoadedSoundBanks() const;

#ifdef SOUND_RELOAD_BANKS
      void                    unloadAllSoundBanks(void);  // May need to go this route if memory leak persists
#endif
      void                    loadStaticSoundBanks(bool loadInit=true);

      void                    setVolumeVoice(uint8 val);
      void                    setVolumeSFX(uint8 val);
      void                    setVolumeMusic(uint8 val);
      
      static void             threadSafePlayCue(BCueIndex index);

      //-- Hacky music tracking bool
      bool                    getInGamePlaying() const { return mInGamePlaying; }
      void                    setInGamePlaying(bool v) { mInGamePlaying = v; }
      bool                    getPregamePlaying() const { return mPreGamePlaying; }
      void                    setPregamePlaying(bool v) { mPreGamePlaying = v; }

private:

   static void                akAssertHook(const char * in_pszExpression,	const char * in_pszFileName, int in_lineNumber );

   void                       processSoundEvents(void);

   BDynamicArray<ISoundEventHandler*>  mEventHandlers;
   BWwiseObjectID                      mPlayerObjects[cPlayerMax];     

   //-- Sound Cue Index to Enum mapping
   BCueIndex                  mSoundCues[cSoundMax];

   // Not a sim array because it will be accessed from a worker thread!
   typedef BDynamicArray<BSoundEventParams> BSoundEventParamsArray;
   // DO NOT manipulate this array in any way unless you've taken then mSoundEventParamsArrayLock lock!
   BSoundEventParamsArray     mSoundEventParamsArray;
   BCriticalSection           mSoundEventParamsArrayLock;

   //-- List of events to play when the previous one is finished. Prevents chats from playing over each other.
   BDynamicArray<BCueIndex>   mQueuedSoundEvents;

   //-- List of sounds to play once the game starts. Prevents sounds from playing until the game is actually being displayed.
   BQueuedSoundInfoArray      mQueuedSoundsToAddWhenGameStarts;

   DWORD                      mCurGPUFrame;

   CAkFilePackageLowLevelIO*  mpWin32SoundIO;
   BAkDefaultLowLevelIO*      mpXFSSoundIO;   

   BString                    mPreGameBank;
   BBankInfoArray             mLoadedBanks;
   
   ISoundInfoProvider*        mpSoundInfoProvider;


   BDynamicArray<uint32>      mChatterIDArray;
   BDynamicArray<BSoundPrepareEventData>      mPreparedEventArray;

   // cached off versions of the xml data, necessary for reloading
   BDynamicArray<BString>     mXmlStaticBanks;
   BString                    mXmlSoundCueStrings[cSoundMax];

   static BLightWeightMutex   mMutex;

#ifndef BUILD_FINAL   
   BDynamicArray<BSoundNameAndHashPair>      mNameAndHashPairArray;
#endif

#ifndef BUILD_FINAL
   BSoundPlayback             mSoundPlayback;
   DWORD                      mOutputBaseTime;
   bool                       mbSoundCueOutput;
#endif

   bool                       mMute:1;
   bool                       mIsInitialized:1;
   bool                       mSplitScreen:1;
   bool                       mGameStartDelay:1;

   bool                       mPreGamePlaying;
   bool                       mInGamePlaying;
};
#endif

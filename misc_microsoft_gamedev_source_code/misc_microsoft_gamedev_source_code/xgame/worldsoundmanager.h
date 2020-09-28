//==============================================================================
// worldsoundmanager.h
//
// Copyright (c) 2003, Ensemble Studios
//==============================================================================

#pragma once 

// Includes
#include "soundmanager.h"
#include "bitvector.h"
#include "database.h"
// xcore
#include "containers\hashTable.h"
#include "containers\hashmap.h"
#include "xmlreader.h"
#include "musicmanager.h"
#include "simtypes.h"
#include "wwise_ids.h"

//#define SOUND_TEST

// Forward declarations
class BEntity;
class BUser;

//==============================================================================
// BWorldSoundRTPCInit
//==============================================================================
class BRTPCInit
{
public:
   BRTPCInit(){}
   ~BRTPCInit(){}
   BRTPCInit(unsigned long id, float val){mRTPCid=id; mInitValue=val;}
   unsigned long  mRTPCid;
   float          mInitValue;
};
typedef BDynamicArray<BRTPCInit> BRTPCInitArray;

//==============================================================================
// BExtendedSoundBankInfo
//==============================================================================
class BExtendedSoundBankInfo
{
public:

   BExtendedSoundBankInfo() : mBankID(AK_INVALID_BANK_ID), mLoaded(false) {}         

   BExtendedSoundBankInfo& operator=(const BExtendedSoundBankInfo& t)
                                 {
                                    mBankName=t.mBankName;
                                    mBankID=t.mBankID;
                                    mLoaded=t.mLoaded;
                                    //mProtoIDs=t.mProtoIDs;                                  
                                    return(*this);
                                 }

   BSimString mBankName;
   AkBankID mBankID;
   bool mLoaded;
   //BDynamicSimLongArray mProtoIDs; //-- List of the protoID's the bank is associated with
};

//==============================================================================
// BWorldSoundHashKey
//==============================================================================
class BWorldSoundHashKey
{
public:

   BWorldSoundHashKey() : mEntityID(cInvalidObjectID), mBoneHandle(-1), mWWiseObjectID(cInvalidWwiseObjectID) {}

   bool operator==(const BWorldSoundHashKey& rhs)
   {
      if(this->mEntityID == rhs.mEntityID &&
         this->mBoneHandle == rhs.mBoneHandle &&
         this->mWWiseObjectID == rhs.mWWiseObjectID)
         return true;
      else
         return false;
   }

   long      mEntityID;      //-- Game Object ID
   long      mBoneHandle;    //-- Game Object Bone Handle
   long      mWWiseObjectID; //-- ID used by WWise 
};

//==============================================================================
// BWorldSoundHashValue
//==============================================================================
class BWorldSoundHashValue
{
public:
   BWorldSoundHashValue() : mWWiseObjectID(0), mPlayingSoundsCount(0) {}
   long  mWWiseObjectID;
   long  mPlayingSoundsCount;
};

//==============================================================================
// BQueuedSoundInfo
//==============================================================================
class BQueuedSoundInfo
{
public:
   BQueuedSoundInfo() : 
      mEntityID(cInvalidObjectID),
      mBoneHandle(-1), 
      mPosition(cInvalidVector), 
      mVelocity(cInvalidVector), 
      mCueIndex(cInvalidCueIndex), 
      mCheckFOW(false), 
      mStopCueIndex(cInvalidCueIndex), 
      mCheckSoundRadius(false), 
      mCanDrop(false)
      {}


   BEntityID   mEntityID;
   long        mBoneHandle;
   BVector     mPosition;
   BVector     mVelocity; 
   BCueIndex   mCueIndex;
   bool        mCheckFOW; 
   BCueIndex   mStopCueIndex;
   bool        mCheckSoundRadius;
   bool        mCanDrop;
};
typedef BDynamicSimArray<BQueuedSoundInfo> BQueuedSoundInfoArray;


#ifndef BUILD_FINAL
class BDumpSoundInfo
{
public:
   BDumpSoundInfo():
      mEventID(0),
      mCount(0)
      {}

   uint mEventID;
   uint mCount;
};

#endif


//==============================================================================
// BWorldSound
//==============================================================================
class BWorldSound
{
   public:     
      BWorldSound() { reset(); }
      ~BWorldSound() {}

      void reset()
      {         
         mID=-1;
         mCueIndex=cInvalidCueIndex;
         mStopCueIndex=cInvalidCueIndex;
         mCueHandle=cInvalidCueHandle;
         mEntityID=cInvalidObjectID;
         mBoneHandle=-1;
         mPosition=cOriginVector;         
         mDistanceToListener = 0.0f;         
         mWiseObjectID = cInvalidWwiseObjectID;      
         
#ifndef BUILD_FINAL
         mMaxRadius = -1.0f;
#endif

         mFOWVolume = -1.0f;
         mEnginePitch = 0.0f;
         //-- Flags
         mFlagStopped=false;
         mFlagForceUpdate=false;
         mFlagProcessFOW=false;
         mFlagProcessEngine=false;
      }

      bool                       getFlagProcessFOW() const { return(mFlagProcessFOW); }
      void                       setFlagProcessFOW(bool v) { mFlagProcessFOW=v; }
      bool                       getFlagProcessEngine() const { return(mFlagProcessEngine); }
      void                       setFlagProcessEngine(bool v) { mFlagProcessEngine=v; }

      BVector                    mPosition;            
      float                      mDistanceToListener;
      long                       mID;              //-- Index into list
      BCueIndex                  mCueIndex;
      BCueIndex                  mStopCueIndex;
      BCueHandle                 mCueHandle;
      BEntityID                  mEntityID;        //-- Game World Object ID this sound is associated with
      long                       mBoneHandle;      
      long                       mWiseObjectID;    //-- ID sent to wwise to tell wwise which object this is attached to      
      float                      mFOWVolume;       //-- RTPC volume control for objects so that they fade out as they go under fog and fade in as they come out.
      float                      mEnginePitch;       //-- RTPC volume control for vehicle engines so they sound different at different speeds..
      //-- Flags
      bool                       mFlagStopped:1;
      bool                       mFlagForceUpdate:1;
      bool                       mFlagProcessFOW:1;
      bool                       mFlagProcessEngine:1;

#ifndef BUILD_FINAL
      float                      mMaxRadius;
#endif
};

//==============================================================================
// BWorldSoundManager
//==============================================================================
class BWorldSoundManager : public ISoundEventHandler
{
   public:
                                 BWorldSoundManager();
                                 ~BWorldSoundManager();

      bool                       setup();
      void                       reset(bool startup=false);
      void                       update(float elapsed);

      BMusicManager*             getMusicManager() { return &mMusicManager; }
      
      void                       addSound(const BVector& position, BCueIndex cueIndex, bool checkFOW, BCueIndex stopCueIndex=cInvalidCueIndex, bool checkSoundRadius=true, bool canDrop=false, BRTPCInitArray *rtpcArray = NULL);
      void                       addSound(BEntity* pEntity, long boneHandle, BCueIndex cueIndex, bool checkFOW, BCueIndex stopCueIndex=cInvalidCueIndex, bool checkSoundRadius=true, bool canDrop=false, BRTPCInitArray *rtpcArray = NULL);      
      void                       removeSounds(BEntityID entityID);      
      void                       loadExtendedSoundBank(const BSimString& bankName);     
      bool                       getExtendedSoundBankLoaded(const BSimString& bankName);
      virtual void               handleSoundEvent(BSoundEventParams& params);// ISoundEventHandler
      void                       setListenerFactor(float factor) {mListenerFactor = factor;}
      void                       setRearAdjustmentFactor(float factor) {mRearAdjustmentFactor = factor;}
      long                       getNumWorldSounds(void) {return mWorldSoundList.getNumberAllocated();}
      float                      getPainRate(void) const {return mPainRate;}
      float                      getAttackSev1Value(void) {return mAttackSev1Value;}
      float                      getAttackSev2Value(void) {return mAttackSev2Value;}      
      void                       gamePaused(bool val);
      float                      getFogRTPCFadeIn(void) const {return mFogRTPCFadeIn;}
      float                      getFogRTPCFadeOut(void) const {return mFogRTPCFadeOut;}
      
      //-- Chatter Methods
      float                      getMaxChatterTime(long state) const;
      float                      getMinChatterTime(long state) const;
      float                      getGlobalChatterTimer(long state) const;      
      void                       setGlobalChatterTimer(long state);
      DWORD                      getTimeToIdleChat() { return mTimeToIdleChat; }
      
      float                      getChanceToPlayChatter(int32 chatterType);
      float                      getChanceToPlaySpecificChatter(int32 chatterType);

      //-- Flags
      bool                       getFlagListenerSet() const { return(mFlagListenerSet); }
      void                       setFlagListenerSet(bool v) { mFlagListenerSet=v; }
      bool                       getFlagListenerUpdated() const { return(mFlagListenerUpdated); }
      void                       setFlagListenerUpdated(bool v) { mFlagListenerUpdated=v; }
      //-- Debug
      void                       render();

   protected:      
      void                       updateListener();
      void                       calcListenerPosition(BUser* pUser, BVector oldPos, BVector &newPos, BVector &forward, BVector &up, BVector& velocity);
      void                       updateSounds();
      bool                       internalAddSound(BEntityID entityID, long boneHandle, const BVector& position, const BVector& velocity, BCueIndex cueIndex, bool checkFOW, BCueIndex stopCueIndex, bool checkSoundRadius=true, bool canDrop=false, BRTPCInitArray *rtpcArray = NULL);
      bool                       internalRemoveSound(BWorldSound* pSound);
      void                       removeSoundFromWwiseObject(BWorldSound* pSound);
      void                       stopWorldSound(BWorldSound* pSound);
      void                       assignWiseObjectID(BWorldSound *pSound);
      void                       updateSound3D(BWorldSound *pSound);
      void                       processSoundEvent(BSoundEventParams& params);  
      void                       updateTimers(float elapsed);

      bool                       loadXML(void);

      void                       loadAlertInfo(BXMLNode node);      

      void                       onExtendedBankLoaded(BExtendedSoundBankInfo& bankInfo);
      void                       unloadExtendedSoundBank(long arrayIndex, bool asynch=true);

      //--
      void                       setGameStartDelay(bool val);

#ifdef SOUND_TEST
      void                       testAllSounds();
      void                       validateData();
#endif
#ifndef BUILD_FINAL
      void                       dumpSoundEventCounts();
#endif

      BFreeList<BWorldSound>           mWorldSoundList;
      
      //-- Table of sounds world sound ID's that can be looked up through the playing sound cue handle
      typedef BHashMap<BCueHandle, long>     BSoundHandleToSoundIDHashMap;      
      BSoundHandleToSoundIDHashMap                       mPlayingSoundsMap;

      BVector                          mListenerPosition;
      BVector                          mListenerDirection;

      BVector                          mListenerPosition2;
      BVector                          mListenerDirection2;

      //-- Table to store the number of sounds playing on a given WWise Object
      BHashTable<BWorldSoundHashValue, BWorldSoundHashKey>  mWorldSoundObjectsTable;
      //-- List of free WWise Object IDs
      BDynamicSimLongArray                                  mFreeWWiseIDs;
            
      float                      mListenerFactor;
      float                      mRearAdjustmentFactor;
      float                      mPainRate;
      float                      mAttackSev1Value;
      float                      mAttackSev2Value;
      float                      mFogRTPCFadeIn;
      float                      mFogRTPCFadeOut;
      float                      mEnginePitchChangeRate;
      float                      mMaxPitchChangePerFrame;

      //-- Chatter Consts
      float                      mMaxChatterTime[cSquadSoundStateMax];
      float                      mMinChatterTime[cSquadSoundStateMax];
      float                      mGlobalMaxChatterTime[cSquadSoundStateMax];
      float                      mGlobalMinChatterTime[cSquadSoundStateMax];

      typedef BHashMap<int32, float>   BChatterChanceToPlay;
      BChatterChanceToPlay             mChatterChanceToPlay;

      typedef BHashMap<int32, float>   BChatterChanceToPlaySpecific;
      BChatterChanceToPlaySpecific     mChatterChanceToPlaySpecific;


      //-- Chatter Timer
      float                      mGlobalChatterTimer[cSquadSoundStateMax];
      DWORD                      mTimeToIdleChat; //-- how long until we consider a squad idle

      //-- Music Manager
      BMusicManager              mMusicManager;

      //-- List to track which dynamic banks are loaded
      BDynamicSimArray<BExtendedSoundBankInfo> mExtendedSoundBanks;

      BQueuedSoundInfoArray      mQueuedSoundsToAddWhenGameStarts;
      DWORD                      mCurGPUFrame;

      //-- Flags
      bool                       mFlagListenerSet:1;
      bool                       mFlagListenerUpdated:1;
      bool                       mFlagFirstUpdate:1;
      bool                       mFlagIntroMusicStarted:1;      
      bool                       mFlagLoopMusicStarted:1;      
      bool                       mFlagPaused:1;
      bool                       mFlagGameStartDelay:1;
};

//==============================================================================
// musicManager.h
//
// Copyright (c) 2007, Ensemble Studios
//==============================================================================
#pragma once
//==============================================================================
// Includes
#include "xmlreader.h"
#include "simtypes.h"
#include "battle.h"
#include "soundmanager.h"
//==============================================================================
//Forward declarations
//==============================================================================
//Const declarations
//==============================================================================
class BMusicManager : IBattleManagerListener
{
public:

   class BMusicState
   {
      public:
         BMusicState(void) :            
            mLockedToModeTimer(0), 
            mCommandedAllyCombatValue((float)INT32_MAX), 
            mCommandedEnemyCombatValue((float)INT32_MAX),                      
            mCueIndex(0)
            {}

         //-- Static Data Loaded from XML
         //-- Attacking          
         float mCommandedAllyCombatValue;
         float mCommandedEnemyCombatValue;                

         //-- Cue 
         BSmallDynamicSimArray<BCueIndex> mSoundCues;
         uint                             mCueIndex; //-- Index into mSoundCues to be used for shuffle.

         //-- Timer
         DWORD mLockedToModeTimer;
   };

   typedef enum
   {
      cStateNone = -1,
      cStateWorldIntro = 0,
      cStateStandard = 1,
      cStateBattleWon,
      cStateBattleLost,
      cStateBattle,
      cBattleStart,
      cStateBattle1 = cBattleStart,
      cStateBattle2,
      cStateBattle3,
      cBattleEnd,
      cMusicStateMax = cBattleEnd
   } BMusicStateType;

   BMusicManager( void );
   ~BMusicManager( void );

   void reset(void);
   void update(void);
   bool loadXML(BXMLNode root);
   


   //-- IBattleManagerListener
   virtual void unitAddedToBattle(void){}
   virtual void unitRemovedFromBattle(void){}
   virtual void battleWon(int32 battleID){}
   virtual void battleLost(int32 battleID){}
   //-- Battle Notifications      
   void  squadsCommandedToAttack(const BEntityIDArray& squads, BPlayerID playerID, BVector armyLocation, BEntityID targetID);   
   void setGameStartDelay(bool val);

   void setEnabled(bool val) { mDisabled = !val; }

   void setLoadedFromSavegame(bool val) { mLoadedFromSavegame = val; }

   //-- Debugging
   void getCurrentState(BSimString &currentState);
   void getQueuedState(BSimString &queuedState);
   void lookupStateString(int32 state, BSimString& string);
   DWORD getSwitchTimer() const;

   bool getDisabledVariable(void) { return mDisabled; }  //only used for saving.

private:

   void setNewState(int32 state, bool force = false, DWORD overWriteTimer=0);
   void checkGlobalBattleSev();
   void checkBattleAlerts();
   void battleWonInternal(int32 battleID);
   void battleLostInternal(int32 battleID);
   void checkBattleEndedTimer();
   bool inBattleState();
   int32 getBattleStatus();
   bool isEnabled();

   DWORD mUpdateCount;

   float mMaxTargetDist;

   //-- States
   BMusicStateType  mCurrentState;
   BMusicStateType  mQueuedState;

   //-- Timing
   DWORD mLastSwitchTime;
   DWORD mModeLockedTimer;
   DWORD mModeSetTime;

   //-- Battle Ended, wait and see stuff
   DWORD mBattleEndedTime;
   DWORD mBattleEndedTimer;
   BMusicStateType mQueuedBattleEndedState;

   //--Flags    
   bool  mStandardEverSet:1;
   bool  mGameStartDelay:1;
   
   BMusicState mMusicState[cMusicStateMax];

   //-- Dynamic Data
   BEntityIDArray mTargetsWhichNeedsToBeDestroyed;

   bool        mDisabled;
   bool        mLoadedFromSavegame;
};
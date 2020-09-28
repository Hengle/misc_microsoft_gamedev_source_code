//==============================================================================
// hintengine.h
//
// HintEngine
//
// Copyright (c) 2007 Ensemble Studios
//==============================================================================

// Includes
#include "simtypes.h"
#include "containers\freelist.h"
#include "entityfilter.h"
#include "generaleventmanager.h"
#include "HumanPlayerAITrackingData.h"
#include "player.h"

#pragma once


class BConceptDatabase;
class BConceptProtoData;
class BConcept;
class BHintEngine;
class BPlayer;
class BParameterPage;
class BHumanPlayerAITrackingData;

//==============================================================================
// BConceptDatabase:  data on all of the protoconcepts.  singleton
//==============================================================================
class BConceptDatabase
{
public:
	BConceptDatabase(){ mLargestDBID=0;}
	~BConceptDatabase(){}

	bool load();

	//order by id??  allow blank spots
	BDynamicArray<BConceptProtoData*> mConcepts;
	int mLargestDBID;
};

class BConceptProtoData
{
public:

   BConceptProtoData();

	int mID;
	BString mDevName;
   long mMessageStringID;

	bool mbActivateBeforePrereqs;

	BDynamicArray<int> mPrerequisites; //prereqs

	float mInitialWaitTime;
   //float mUrgentTimeDelay;

   //message duration 
	float mMessageDisplayTime;
   //  time / on complete
	//  float mMessageShowTime; //.. make global??
   //  reshow timer


   //duration
	//conflict res
	//queue
	//stomp
	//filter/breakthrough
	//respawn
	//Group info/id ... for enabling a whole set?	
	//chain link...  for linking a series of concepts together 

   int mTimesPerGame;
   int mMaxGamesAllowed;


   //constants?
   float mCoolDownTime;
   float mCoolDownIncrement;

   void initValues();

	bool load(BXMLNode xmlNode);

};


class BConceptPrecondition
{
public:
   enum
   {
      cNotSet = 0,
		cBlocked = 1,
      cWait = 2,
      cGood = 3,  
		cUrgent = 4,
      cSize
   } ;
};

//States
//  idle   ?
//  active ?
//  shown  ?
class BConceptState
{
public:
   enum
   {
      cIdle = 0,
		cActive = 1,
      cShown = 2, 
      cTerminal = 3,
      cSize
   } ;
};


class BConceptStateResult
{
public:
   enum
   {
      cIdle = 0,
		cActive = 1,
      cComplete = 2,  
      cError = 3,
		cCancel = 4,
		cRestart = 5,
      cSize
   } ;
};
class BConceptCommand
{
public:
   enum
   {
      cNotSet = 0,
		cCheckPreconditions = 1,
      cInit = 2,  
      cDeactivate = 3,
		cShow = 4,
		cHide = 5,
      cSize
   } ;
};

class BConcept : public IPoolable
{
public:

	DECLARE_FREELIST(BConcept, 5);

   virtual void               onAcquire();
   virtual void               onRelease() {  }

	//persistent data accessors
	int getID(){return mID;}
	//bool getCompleted(){return mCompleted;}
	int getTimesReinforced(){return mTimesReinforced;}
   void setTimesReinforced(int value){ mTimesReinforced = value;}
	int getGamesReinforced(){return mGamesReinforced;}
   void setGamesReinforced(int value){ mGamesReinforced = value;}
	int getHintDisplayedCount(){return mHintDisplayedCount;}
   //runtime data accessors
	bool areProfileSettingsDirty(){return mbDirtyProfile;}

	void setActive();     //sets up events from proto concept
	void setNotActive();  //destroys events from proto concept

	void setReady();      //set the event that it is ready to be shown as a hint
	void setNotReady();   //set the hint back to active state??

	void setComplete();   
   void show();
   void hide();
   void push();
   void restart();

	bool isReady(){return  (mState == BConceptState::cActive) && mbEventReady;}
	bool isReadyNoActiveCheck(){return  mbEventReady;}
	bool isActive(){return mbActive;}
	BConceptProtoData* getProtoConcept(){return mpProtoConcept;}

	void update(float elapsedTime);//check events for state change  

	void loadProfileData();
	void initRuntimeData(BHintEngine* pHintEngine, BPlayer* pPlayer , BConceptProtoData* pProtoConcept);
   void reset();


   //Commands for working with the triggers
   bool hasNewCommand(){return mHasCommand;}
   BYTE getNewCommand(){mHasCommand = false; return mNewCommand; }
   BYTE peekNewCommand(){return mNewCommand;}
   void setNewCommand(BYTE command){mHasCommand = true; mNewCommand = command;}

   //For passing state changes to the triggers
   bool hasStateChanged(){return mStateChanged;}
   BYTE getNewState(){mStateChanged = false; return mNewState; }
   BYTE peekNewState(){return mNewState;}
   void setNewState(BYTE state){mStateChanged = true; mNewState = state;}

   //For preconditionResult
   bool hasPreconditionResult(){return mHasPreconditionResult;}
   void setNewPreconditionResult(BYTE preconditionResult);
   int getPreconditionResult() {return mPreconditionResult;}
   float getTimeWithPrecondition(DWORD gameTime);
   //? void clearPreconditionResult()

   



   void setState(BYTE state);
   BYTE getState(){return mState;}

   BParameterPage* getPage(uint page);
   void clearPages();
   

   void setWaitTime(float time);

   bool isCoolingDown(DWORD gameTime);


   void addChild(BConcept* pChild);
   void clearChildren();


   void walkSubHints(BDynamicArray<BConcept*> &subHints);

   void setPermission(bool allowed);
   bool getPermission();

   bool arePrerequisitesMet();
   bool isLearnedMaxTimesPerGame();
   bool isMainConceptAllowed();

   void resetCoolDown(bool resetAccumlator);
   void resetIdleReadyTimer();

   bool isTopLevelConcept(){return mpParentHint == NULL;}
   void setParentHint(BConcept* pParentHint){mpParentHint = pParentHint;}

   bool save(BStream* pStream, int saveType) const;
   bool load(BStream* pStream, int saveType);

private:
   bool mHasCommand;
   BYTE mNewCommand;

   bool mStateChanged;
   BYTE mNewState;

   bool mHasPreconditionResult;
   BYTE mPreconditionResult;
   DWORD mPreconditionTime;

   BYTE mState;

	//persistent data!
	int mID;
   int mGamesReinforced;

   //Was persistent data
	//bool mCompleted;
	int mTimesReinforced;  //games reinforced?
	int mHintDisplayedCount;  //we could persist this if refinment was needed

   //script data
   BDynamicArray<BParameterPage*> mPages; 

	//runtime data
   int  mTimesReinforcedThisGame;
   bool mbEventReady;
	bool mbActive;
   bool mbPermission;

	float mInitialWaitTimeRemaining;
	float mTerminalWaitTimeRemaining;

   DWORD mCoolDownTimer;
   DWORD mLastCoolDownAmount;
   float mCoolDownTimerAccumulator;

   //BGeneralEventSubscriber* mpEventReady;
   //BGeneralEventSubscriber* mpEventLearned;

   BDynamicArray<BConcept*> mSubHints;
   BConcept* mpParentHint;
   BHintEngine* mpHintEngine;

	BConceptProtoData* mpProtoConcept;
	bool mbPrereqsMet;
   bool mbDirtyProfile;
	BPlayer* mpPlayer;
};

//==============================================================================
// BParameterPage:  one per user
//==============================================================================
class BParameterPage
{
public:
   void clear();

   bool save(BStream* pStream, int saveType) const;
   bool load(BStream* pStream, int saveType);

   BVector           mVector;
   BEntityIDArray    mSquadList; 
   BEntityIDArray    mUnitList; 
   BEntityFilterSet  mEntityFilterSet;
   float             mFloat;
   long              mObjectType;
   uint32            mLocStringID;
   
   bool              mHasVector;
   bool              mHasSquadList; 
   bool              mHasUnitList; 
   bool              mHasEntityFilterSet;
   bool              mHasFloat;
   bool              mHasObjectType;  
   bool              mHasLocStringID;
};


//==============================================================================
// BHintEngine:  one per user
//==============================================================================
class BHintEngine
{
public:
	BHintEngine(BPlayer *pPlayer);
	~BHintEngine();

	void initialize();
	void reset( bool needsUpdate = true );

	void update(float elapsedTime);

   void scoreConcepts();

   void onPrecondition(int conceptID, int precondition);
   void onStateChange(int conceptID, int state);

   void startSubHint(int conceptID, int parentConceptID, float initTime  );
   void stopSubHints(int conceptID);

   BParameterPage* getConceptPage(int conceptID, int pageID);

   bool hasStateChange(int conceptID);
   bool hasCommand(int conceptID);
   int popState(int conceptID);
   int popCommand(int conceptID);
   int peekState(int conceptID);

   void resetConceptCoolDown(int conceptID, bool resetAccumlator);

   void setPermission(bool allowAll, const BInt32Array& list);

   GFDECLAREVERSION();
   bool save(BStream* pStream, int saveType) const;
   bool load(BStream* pStream, int saveType);


   BConcept* getConceptByDBID(int dbid);

   void resetIdleReadyTimers();

   void updateProfile();
   void loadProfile();

private:

	void displayHint(BConcept* pConcept);
	void removeHint();

	//BDynamicArray<BConcept*> mActiveUserConcepts; //concepts that are being watched  //use state
	BDynamicArray<BConcept*> mUserConcepts; //loaded from profile, master list, index==id

	//queue data
   float mTimeSinceLastHint;
	//BDynamicArray<BConcept*> mConceptQueue; 
	//BDynamicArray<float> mHintShownTimes;
	//BDynamicArray<float> mHintClosedTimes;

	//hint state
	bool mbHintMessageOn;
	//float mHintMessageTimeRemaining;
	//BConcept* mpCurrentDisplayedHint;

   //Permission
   BInt32Array mAllowedConcepts;

	//Settings
	float mMinTimeBetweenHints;
   float mRescoreRate;

   float mTimeActivateWait;
   float mTimeActivateGood;
   float mTimeActivateUrgent;

   //engine timers
   float mWaitForNextRescore;
	
	BPlayer* mpPlayer;

   DWORD mLastGameTime;

   BHumanPlayerAITrackingData* mpTrackingData;
   bool mNeedsLazyUpdate;
};


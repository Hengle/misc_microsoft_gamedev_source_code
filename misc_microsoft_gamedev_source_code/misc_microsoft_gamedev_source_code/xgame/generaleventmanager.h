//==============================================================================
// GeneralEventManager.h
//
// GeneralEventManager 
//
// Copyright (c) 2006 Ensemble Studios
//==============================================================================
#pragma once

#include "simtypes.h"
#include "entityfilter.h"
#include "cost.h"
#include "camera.h"
#include "savegame.h"

// Constants
//const long cGeneralEventSubscriberBlockSize=100;

typedef uint BGeneralEventID;
typedef uint BGeneralEventSubscriberID;
typedef uint BGameState;

class BEventDefinitions
{
public:

//NOTE!!! Add enumerations here, they will expand into three parts:
#define MGENERALEVENTS( _m)\
		_m (ControlTilt) /*Add comments like this before the slash*/ \
		_m (ControlZoom) \
		_m (ControlRotate) \
		_m (ControlPan) \
      _m (ControlCircleSelect) \
		_m (ControlCircleMultiSelect) \
      _m (ControlClearAllSelections) \
      _m (ControlModifierAction) \
      _m (ControlModifierSpeed) \
      _m (ControlResetCameraSettings) \
      _m (ControlGotoRally) \
      _m (ControlGotoBase) \
      _m (ControlGotoScout) \
      _m (ControlGotoNode) \
      _m (ControlGotoHero) \
      _m (ControlGotoAlert) /**/\
      _m (ControlGotoSelected) \
      _m (ControlGroupSelect) \
      _m (ControlGroupGoto) \
      _m (ControlGroupAssign) \
      _m (ControlGroupAddTo) \
      _m (ControlScreenSelectMilitary) \
      _m (ControlGlobalSelect) \
      _m (ControlDoubleClickSelect) \
      _m (ControlFindCrowdMilitary) \
      _m (ControlFindCrowdVillager) \
      _m (ControlSetRallyPoint) \
      _m (Flare) \
      _m (FlareHelp) \
      _m (FlareMeet) \
      _m (FlareAttack) \
      _m (MenuShowCommand) \
      _m (MenuCloseCommand) \
      _m (MenuNavCommand) \
      _m (MenuCommandHasFocus0) /* The 0 slot has focus in the command menu */ \
      _m (MenuCommandHasFocus1) /* The 1 slot has focus in the command menu */ \
      _m (MenuCommandHasFocus2) /* The 2 slot has focus in the command menu */ \
      _m (MenuCommandHasFocus3) /* The 3 slot has focus in the command menu */ \
      _m (MenuCommandHasFocus4) /* The 4 slot has focus in the command menu */ \
      _m (MenuCommandHasFocus5) /* The 5 slot has focus in the command menu */ \
      _m (MenuCommandHasFocus6) /* The 6 slot has focus in the command menu */ \
      _m (MenuCommandHasFocus7) /* The 7 slot has focus in the command menu */ \
      _m (MenuCommandHasFocusN) \
      _m (MenuCommandClickmenuN) \
      _m (MenuCommandIsMenuOpen) \
      _m (MenuCommanndIsMenuNotOpen) \
      _m (MenuShowPower) \
      _m (MenuClosePower) \
      _m (MenuPowerHasFocusN) \
      _m (MenuPowerClickmenuN) \
      _m (MenuPowerIsMenuOpen) \
      _m (MenuPowerIsMenuNotOpen) \
      _m (MenuShowSelectPower) \
      _m (MenuShowAbility) \
      _m (MenuShowTribute) \
      _m (MenuShowObjectives) \
      _m (GameEntityBuilt) /**/ \
      _m (GameEntityKilled) \
      _m (ChatShown) \
      _m (ChatRemoved) \
      _m (ChatCompleted) \
      _m (CommandBowl) \
      _m (CommandAbility) \
      _m (CommandUnpack) \
      _m (CommandDoWork) \
      _m (CommandAttack) \
      _m (CommandMove) \
      _m (CommandTrainSquad) \
      _m (CommandTrainSquadCancel) \
      _m (CommandResearch) \
      _m (CommandResearchCancel) \
      _m (CommandBuildOther) \
      _m (CommandRecycle) \
      _m (CommandRecycleCancel) \
      _m (CameraLookingAt) \
		_m (SelectUnits) \
      _m (CinematicCompleted) \
      _m (FadeCompleted) \
      _m (UsedPower) \
      _m (Timer1Sec) \
      _m (ControlCircleSelectFullyGrown) \
      _m (PowerOrbitalComplete) \
      _m (GameEntityRammed) \
      _m (GameEntityJacked) \
      _m (GameEntityKilledByNonProjectile) \


		//_m (MenuClosed) \
		//_m (MenuNav) /*no filter v1*/ \ 




   #define MENUMERATE(name) c##name,   
//#1  an enumation
   enum
   {
      cEventInvalid =0,
      MGENERALEVENTS( MENUMERATE )
      cSize
   };
//#2  code to read enumeration from strings
   #define MREADENUMEATIONGENERALEVENTS(name)   else if (varTypeName == #name) varTypeEnum = BEventDefinitions::c##name;
   static BGeneralEventID getGeneralEventEnum(const BSimString& varTypeName)
   {
      BGeneralEventID varTypeEnum = BEventDefinitions::cEventInvalid;

      if (false)
         varTypeEnum = BEventDefinitions::cEventInvalid;
      MGENERALEVENTS( MREADENUMEATIONGENERALEVENTS )      

      BASSERTM(varTypeEnum != BEventDefinitions::cEventInvalid, "Invalid event type");
      return (varTypeEnum);
   }
   #undef MREADENUMEATIONGENERALEVENTS
//#3 code to write string from the enumeration
   #define MWRITEENUMEATIONGENERALEVENTS(name)   else if (data == BEventDefinitions::c##name) varDataAsString = #name;
   static void getVariableDataAsString(BGeneralEventID data, BSimString &varDataAsString)
   {
      if (false) varDataAsString = "";
      MGENERALEVENTS( MWRITEENUMEATIONGENERALEVENTS )      
   }
   #undef MWRITEENUMEATIONGENERALEVENTS

};

class BGameStateDefinitions
{
public:

//NOTE!!! Add enumerations here, they will expand into three parts:
#define MGAMESTATE( _m)\
		_m (SquadsSelected) /*Add comments like this before the slash*/ \
		_m (HasSquads) \
		_m (HasBuildings) \
		_m (HasResources) \
		_m (HasTech) \
		_m (GameTime)


//   #define MENUMERATE(name) c##name,   
//#1  an enumation
   enum
   {
      cEventInvalid =0,
      MGAMESTATE( MENUMERATE )
      cSize
   };
//#2  code to read enumeration from strings
   #define MREADENUMEATIONGAMESTATE(name)   else if (varTypeName == #name) varTypeEnum = BGameStateDefinitions::c##name;
   static BGeneralEventID getEnum(const BSimString& varTypeName)
   {
      uint varTypeEnum = BGameStateDefinitions::cEventInvalid;

      if (false)
         varTypeEnum = BGameStateDefinitions::cEventInvalid;
      MGAMESTATE( MREADENUMEATIONGAMESTATE )      

      BASSERTM(varTypeEnum != BGameStateDefinitions::cEventInvalid, "Invalid event type");
      return (varTypeEnum);
   }
   #undef MREADENUMEATIONGAMESTATE
//#3 code to write string from the enumeration
   #define MWRITEENUMEATIONGAMESTATE(name)   else if (data == BGameStateDefinitions::c##name) varDataAsString = #name;
   static void getVariableDataAsString(uint data, BSimString &varDataAsString)
   {
      if (false) varDataAsString = "";
      MGAMESTATE( MWRITEENUMEATIONGAMESTATE )      
   }
   #undef MWRITEENUMEATIONGAMESTATE

};


class BEventParameters
{
public:
   enum
   {
		cDefault,//?
      cSource,  
      cTarget,
		
		//Specific parameters ... how can this scale...    need different sigs that pass in parameter id.   templatize?
		cChatID,
		cCinimaticID,

      cSize
   } ;
};

class BGeneralEventManager;

extern BGeneralEventManager gGeneralEventManager;

class BEventFilterBase
{
public:   

   enum
   {
      cFilterTypeEntity,
      cFilterTypeEntityList,
      cFilterTypeLocation,
      cFilterTypePlayer,
		cFilterTypeCamera,
		cFilterTypeGameState,
      cFilterTypeType,
      cFilterTypeNumeric,
      cFilterTypeMaxObjectType,
   };

   __forceinline BYTE getType() const { return (mType); }
   __forceinline bool isType(BYTE type) const { return (mType == type); }

   static void releaseFilter(BEventFilterBase *pEntityFilter);

   virtual bool save(BStream* pStream, int saveType) { GFWRITEVAR(pStream,BYTE,mParamID); return true; }
   virtual bool load(BStream* pStream, int saveType) { GFREADVAR(pStream,BYTE,mParamID); return true; }

   BYTE mParamID;

   protected:
      BYTE mType;

};

class BEventFilterEntity : public BEventFilterBase, IPoolable
{
public:
   BEventFilterEntity() { mType = BEventFilterBase::cFilterTypeEntity; }
   ~BEventFilterEntity() {  }

   virtual void onAcquire(){ mFilterSet.clearFilters();  }
   virtual void onRelease(){ mFilterSet.clearFilters(); }

   DECLARE_FREELIST(BEventFilterEntity, 4);
 
   //Filter functions
   bool testEntity(BEntityID id);

   virtual bool save(BStream* pStream, int saveType) { GFWRITECLASS(pStream, saveType, mFilterSet); return BEventFilterBase::save(pStream, saveType); }
   virtual bool load(BStream* pStream, int saveType) { GFREADCLASS(pStream, saveType, mFilterSet); return BEventFilterBase::load(pStream, saveType); }

   BEntityFilterSet mFilterSet;
};

class BEventFilterEntityList : public BEventFilterBase, IPoolable
{
public:
   BEventFilterEntityList() { mType = BEventFilterBase::cFilterTypeEntityList; }
   ~BEventFilterEntityList() {  }

   virtual void onAcquire(){ mFilterSet.clearFilters(); mMinimumMatchesRequired = 1; mAllMustMatch = false; }
   virtual void onRelease(){ mFilterSet.clearFilters(); }

   DECLARE_FREELIST(BEventFilterEntityList, 4);
 
   //Filter functions
   bool testEntityList(BEntityIDArray *pIds);

   virtual bool save(BStream* pStream, int saveType) { GFWRITECLASS(pStream, saveType, mFilterSet); GFWRITEVAR(pStream,uint,mMinimumMatchesRequired); GFWRITEVAR(pStream,bool,mAllMustMatch); return BEventFilterBase::save(pStream,saveType); }
   virtual bool load(BStream* pStream, int saveType) { GFREADCLASS(pStream, saveType, mFilterSet); GFREADVAR(pStream,uint,mMinimumMatchesRequired); GFREADVAR(pStream,bool,mAllMustMatch); return BEventFilterBase::load(pStream,saveType); }

   BEntityFilterSet mFilterSet;
   uint mMinimumMatchesRequired;
   bool mAllMustMatch;
};

class BEventFilterLocation : public BEventFilterBase, IPoolable
{
public:
   BEventFilterLocation() { mType = BEventFilterBase::cFilterTypeLocation; }
   ~BEventFilterLocation() {}

   virtual void onAcquire(){  }
   virtual void onRelease(){  }

	bool testLocation(BVector &vector);

   DECLARE_FREELIST(BEventFilterLocation, 4);

   virtual bool save(BStream* pStream, int saveType) { GFWRITEVECTOR(pStream, mTarget); GFWRITEVAR(pStream,float,mRadius); return BEventFilterBase::save(pStream,saveType); }
   virtual bool load(BStream* pStream, int saveType) { GFREADVECTOR(pStream, mTarget); GFREADVAR(pStream,float,mRadius); return BEventFilterBase::load(pStream,saveType); }

	BVector mTarget;
	float	  mRadius;
   
};

class BEventFilterType : public BEventFilterBase, IPoolable
{
public:
   BEventFilterType() { mType = BEventFilterBase::cFilterTypeType; }
   ~BEventFilterType() {}

   virtual void onAcquire();
   virtual void onRelease(){  }

   bool testType(long type);

   DECLARE_FREELIST(BEventFilterType, 4);

   virtual bool save(BStream* pStream, int saveType)
   { 
      GFWRITEVAR(pStream,bool,mInvert); 
      GFWRITEVAR(pStream,long,mObjectType); 
      GFWRITEVAR(pStream,long,mProtoSquad); 
      GFWRITEVAR(pStream,long,mProtoObject); 
      GFWRITEVAR(pStream,long,mTech); 
      return BEventFilterBase::save(pStream,saveType); 
   }
   virtual bool load(BStream* pStream, int saveType) 
   { 
      GFREADVAR(pStream,bool,mInvert); 
      GFREADVAR(pStream,long,mObjectType); 
      GFREADVAR(pStream,long,mProtoSquad); 
      GFREADVAR(pStream,long,mProtoObject); 
      GFREADVAR(pStream,long,mTech); 
      gSaveGame.remapObjectType(mObjectType);
      gSaveGame.remapProtoSquadID(mProtoSquad);
      gSaveGame.remapProtoObjectID(mProtoObject);
      gSaveGame.remapProtoTechID(mTech);
      return BEventFilterBase::load(pStream,saveType); 
   }

	bool mInvert;
   long mObjectType;
   long mProtoSquad;
   long mProtoObject;
   long mTech;
};

class BEventFilterNumeric : public BEventFilterBase, IPoolable
{
public:
   BEventFilterNumeric() { mType = BEventFilterBase::cFilterTypeNumeric; }
   ~BEventFilterNumeric() {}

   virtual void onAcquire(){  }
   virtual void onRelease(){  }

   bool testInt(int value);
   bool testFloat(float value);

   DECLARE_FREELIST(BEventFilterNumeric, 4);

   virtual bool save(BStream* pStream, int saveType)
   { 
      GFWRITEVAR(pStream,long,mCompareType); 
      GFWRITEVAR(pStream,int,mInteger); 
      GFWRITEVAR(pStream,float,mFloat); 
      return BEventFilterBase::save(pStream,saveType); 
   }
   virtual bool load(BStream* pStream, int saveType) 
   { 
      GFREADVAR(pStream,long,mCompareType); 
      GFREADVAR(pStream,int,mInteger); 
      GFREADVAR(pStream,float,mFloat); 
      return BEventFilterBase::load(pStream,saveType); 
   }

	long mCompareType;
   int mInteger;
   float mFloat;
};


class BEventFilterCamera : public BEventFilterBase, IPoolable
{
public:
   BEventFilterCamera() { mType = BEventFilterBase::cFilterTypeCamera; }
   ~BEventFilterCamera() {}

   virtual void onAcquire(){ mbFilterUnit = false; mbFilterLocation=false; mbInvert=false;}  // mFilterSet.clearFilters(); mSomeValue = 13; }
   virtual void onRelease(){}  // mFilterSet.clearFilters(); }

	bool testCamera(BCamera* pCamera);

   DECLARE_FREELIST(BEventFilterCamera, 4);

   virtual bool save(BStream* pStream, int saveType)
   { 
      GFWRITEVAR(pStream,BEntityID,mUnit);
      GFWRITEVECTOR(pStream, mLocation);
      GFWRITEVAR(pStream,float,mViewAreaRadius);
      GFWRITEVAR(pStream,bool,mbFilterUnit);
      GFWRITEVAR(pStream,bool,mbFilterLocation);
      GFWRITEVAR(pStream,bool,mbInvert);
      return BEventFilterBase::save(pStream,saveType); 
   }
   virtual bool load(BStream* pStream, int saveType) 
   { 
      GFREADVAR(pStream,BEntityID,mUnit);
      GFREADVECTOR(pStream, mLocation);
      GFREADVAR(pStream,float,mViewAreaRadius);
      GFREADVAR(pStream,bool,mbFilterUnit);
      GFREADVAR(pStream,bool,mbFilterLocation);
      GFREADVAR(pStream,bool,mbInvert);
      return BEventFilterBase::load(pStream,saveType); 
   }

   bool mbFilterUnit;
   BEntityID mUnit;

   bool mbFilterLocation;
   BVector mLocation;

   //float mPercentOnScreen;
   float mViewAreaRadius;

   bool mbInvert;

};

class BEventFilterGameState : public BEventFilterBase, IPoolable
{
public:
	BEventFilterGameState() { mType = BEventFilterBase::cFilterTypeGameState; }
   ~BEventFilterGameState() {}

   virtual void onAcquire(){  mFilterSet.clearFilters(); }
   virtual void onRelease(){  mFilterSet.clearFilters();}

	bool testGameState();

   DECLARE_FREELIST(BEventFilterGameState, 4);
	
   virtual bool save(BStream* pStream, int saveType)
   { 
      GFWRITECLASS(pStream, saveType, mFilterSet);
      GFWRITECLASS(pStream, saveType, mCost);
      GFWRITEVAR(pStream,BGameState,mGameState);
      GFWRITEVAR(pStream,BPlayerID,mPlayerID);
      GFWRITEVAR(pStream,uint,mMinimumMatchesRequired);
      GFWRITEVAR(pStream,long,mTechID);
      GFWRITEVAR(pStream,bool,mInvert);
      GFWRITEVAR(pStream,bool,mAllMustMatch);
      return BEventFilterBase::save(pStream,saveType); 
   }
   virtual bool load(BStream* pStream, int saveType) 
   { 
      GFREADCLASS(pStream, saveType, mFilterSet);
      GFREADCLASS(pStream, saveType, mCost);
      GFREADVAR(pStream,BGameState,mGameState);
      GFREADVAR(pStream,BPlayerID,mPlayerID);
      GFREADVAR(pStream,uint,mMinimumMatchesRequired);
      GFREADVAR(pStream,long,mTechID);
      GFREADVAR(pStream,bool,mInvert);
      GFREADVAR(pStream,bool,mAllMustMatch);
      gSaveGame.remapProtoTechID(mTechID);
      return BEventFilterBase::load(pStream,saveType); 
   }

	BGameState mGameState;
	BPlayerID mPlayerID;
   bool mInvert;


   BEntityFilterSet mFilterSet;
   uint mMinimumMatchesRequired;
   bool mAllMustMatch;

	long mTechID;
	BCost mCost;

private:
	bool evaluateEntityList(const BEntityIDArray *pIds);
   
};

//=================================
// BGeneralEventSubscriber
//
//=================================
class BGeneralEventSubscriber : public IPoolable
{
public:

   BGeneralEventSubscriber()  {}
   ~BGeneralEventSubscriber() {}

   virtual void               onAcquire() { mFired=false; mUseCount = false; mCheckPlayer=false; mFiredCount = 0; clearFilters(); }
   virtual void               onRelease() { clearFilters(); }

   DECLARE_FREELIST(BGeneralEventSubscriber, 5);

   DWORD mFireTime;
   int mFiredCount;
   bool mFired;
   bool mUseCount;

   bool hasFired() {return mFired;}
   void setFired();
   bool useCount() {return mUseCount;}
   DWORD getFireTime() { return mFireTime; }

   BGeneralEventSubscriberID mID;

   //Event
   void eventTrigger(BPlayerID playerID);
   void eventTrigger(BPlayerID playerID, BEntityID sourceEntity, BEntityID targetEntity);
   void eventTrigger(BPlayerID playerID, BEntityID sourceEntity, long value);
   void eventTrigger(BPlayerID playerID, BCamera* pCamera);
   void eventTrigger(BPlayerID playerID, BEntityIDArray* pSources, BEntityID sourceEntity, BEntityIDArray* pTargets, BEntityID targetEntity);

   //Player Filtering  
   bool mCheckPlayer;
   long mPlayer;

   //Dyanmic Filters
   BDynamicArray<BEventFilterBase*>      mEventFilters;

   BEventFilterEntity* addEventFilterEntity()
   {
      BEventFilterEntity *pFilter = BEventFilterEntity::getInstance();
      mEventFilters.add(pFilter);
      return pFilter;
   }

   BEventFilterEntityList* addEventFilterEntityList()
   {
      BEventFilterEntityList *pFilter = BEventFilterEntityList::getInstance();
      mEventFilters.add(pFilter);
      return pFilter;
   }

   BEventFilterCamera* addEventFilterCamera()
   {
      BEventFilterCamera *pFilter = BEventFilterCamera::getInstance();
      mEventFilters.add(pFilter);
      return pFilter;
   }

   BEventFilterGameState* addEventFilterGameState()
   {
      BEventFilterGameState *pFilter = BEventFilterGameState::getInstance();
      mEventFilters.add(pFilter);
      return pFilter;
   }
   BEventFilterType* addEventFilterType()
   {
      BEventFilterType *pFilter = BEventFilterType::getInstance();
      mEventFilters.add(pFilter);
      return pFilter;
   }
   BEventFilterNumeric* addEventFilterNumeric()
   {
      BEventFilterNumeric *pFilter = BEventFilterNumeric::getInstance();
      mEventFilters.add(pFilter);
      return pFilter;
   }   
   void clearFilters();

   // Save/load
   bool save(BStream* pStream, int saveType) const;
   bool load(BStream* pStream, int saveType);
};

//=================================
// BGeneralEvent
//
//=================================
class BGeneralEvent 
{
public:      

   BGeneralEvent() {};
   ~BGeneralEvent() {}

   //Subscribers
   uint  getSubscriberCount() { return mGeneralEventSubscribers.size(); }
   BGeneralEventSubscriber* getSubscriber(uint index) { return mGeneralEventSubscribers.get(index); }
   void addSubscriber(BGeneralEventSubscriber* pSubscriber);
   void removeSubscriber(BGeneralEventSubscriber* pSubscriber);

   BDynamicArray<BGeneralEventSubscriber*>      mGeneralEventSubscribers;

   void clearSubscribers();
};

//==========================================
// BGeneralEventManager
//
//==========================================
class BGeneralEventManager
{
public:

   BGeneralEventManager(){ mOneSecondTimer=0; };
   virtual ~BGeneralEventManager(){};      

   // Management functions 
   bool                       init();
   void                       reset();

   //Events 
   BGeneralEvent* getEvent(BGeneralEventID id) { return &(mGeneralEvents[id]); }

   //Subscribers
   bool hasSubscribers(BGeneralEventID id) { return (mGeneralEvents[id].getSubscriberCount() > 0); }
   BGeneralEventSubscriber* newSubscriber(BGeneralEventID id); 
   BGeneralEventSubscriber* getSubscriber(BGeneralEventSubscriberID id);
   void removeSubscriber(BGeneralEventSubscriberID id);

   //Sync support
   void queueSubscriberFired( BGeneralEventSubscriberID id);
   void subscriberFired( BGeneralEventSubscriberID id);
   void flushQueue(long playerID, float elapsedTime);
 
   //Events:
   void eventTrigger(BGeneralEventID id, BPlayerID playerID)
   {
      BGeneralEvent* pEvent = getEvent(id);
      for(uint i=0; i< pEvent->mGeneralEventSubscribers.size(); i++)
      {
         pEvent->mGeneralEventSubscribers[i]->eventTrigger(playerID);
      }
   }
   void eventTrigger(BGeneralEventID id, BPlayerID playerID, BEntityID sourceEntity, BEntityID targetEntity = cInvalidObjectID)
   {
      //getEvent(id)->eventTrigger(playerID, sourceEntity, targetEntity); 
      BGeneralEvent* pEvent = getEvent(id);
      for(uint i=0; i< pEvent->mGeneralEventSubscribers.size(); i++)
      {
         pEvent->mGeneralEventSubscribers[i]->eventTrigger(playerID, sourceEntity, targetEntity);
      }
   }

   void eventTrigger(BGeneralEventID id, BPlayerID playerID, BEntityID sourceEntity, long value)
   {
      BGeneralEvent* pEvent = getEvent(id);
      for(uint i=0; i< pEvent->mGeneralEventSubscribers.size(); i++)
      {
         pEvent->mGeneralEventSubscribers[i]->eventTrigger(playerID, sourceEntity, value);
      }
   }

   void eventTrigger(BGeneralEventID id, BPlayerID playerID, BEntityIDArray* pSources, BEntityID sourceEntity, BEntityIDArray* pTargets, BEntityID targetEntity)
   {
      BGeneralEvent* pEvent = getEvent(id);
      for(uint i=0; i< pEvent->mGeneralEventSubscribers.size(); i++)
      {
         pEvent->mGeneralEventSubscribers[i]->eventTrigger(playerID, pSources, sourceEntity, pTargets, targetEntity);
      }
   }

   void eventTrigger(BGeneralEventID id, BPlayerID playerID, BCamera* pCamera)
   {
      //optimizations possibly go here??

      BGeneralEvent* pEvent = getEvent(id);
      for(uint i=0; i< pEvent->mGeneralEventSubscribers.size(); i++)
      {
         pEvent->mGeneralEventSubscribers[i]->eventTrigger(playerID, pCamera);
      }
   }

   GFDECLAREVERSION();
   bool save(BStream* pStream, int saveType) const;
   bool load(BStream* pStream, int saveType);

protected:

   BGeneralEvent mGeneralEvents[BEventDefinitions::cSize];
   BDynamicArray<uint>      mSubscriberFiredQueue;

   float mOneSecondTimer;
};


///Old versions

////
//////==============================================================================
////// GeneralEventManager.h
//////
////// GeneralEventManager 
//////
////// Copyright (c) 2006 Ensemble Studios
//////==============================================================================
////#pragma once
////
////#include "simtypes.h"
////
////
////// Constants
//////const long cGeneralEventSubscriberBlockSize=100;
////
////typedef uint BGeneralEventID;
////typedef uint BGeneralEventSubscriberID;
////
////class BEventDefinitions
////{
////public:
////   typedef enum
////   {
////      //No args?   sender player
////      //cControlPan,
////      cControlTilt,
////      cControlZoom,
////      cControlMultiSelect,
////      cControlCircleSelect,
////      cControlClearAllSelections,
////      cControlModifierAction,
////      cControlModifierSpeed,
////      cControlResetCameraSettings,
////      cControlGotoRally,
////      cControlGotoBase,
////      cControlGotoScout,
////      cControlGotoNode,
////      cControlGotoHero,
////      //cControlGotoAlert,
////      //cControlGotoSelected,
////      cControlSelectGroup,
////      cControlGotoGroup,
////      cControlAssignGroup,
////      cControlAddToGroup,
////      cControlScreenSelectMilitry,
////      //cControlScreenSelectAir,
////      //cControlScreenSelectVehicle,
////      //cControlScreenSelectInfantry,
////      cControlGlobalSelect,
////
////      //Location arg ... for near() predicate ... filter w point/radius
////      cControlSetRallyPoint,
////      cFlare,
////      //cFlareHelp,
////      //cFlareMeet,
////      //cFlareAttack,
////      
////      //sender  entity
////      cMenuShowCommand,
////      //No args
////      cMenuShowPower,
////      cMenuShowAbility, //?
////      cMenuShowTribute, //?
////      cMenuShowObjectives,
////
////      //selected choice??   sender, item, ???
////      //cMenuSelectionMade,  //this could be dangerous
////
////      //Commands   entity list?
////      //cCommandDoWorkAtCurrentLocation,  //For abilites
////      //work command
////      //move command
////      //attack command
////      //unpack
//// 
////      //player / Entity ID   source=trainer  target=what was built?
////      cGameEntityTrained,
////      cGameEntityBuilt,
////      cGameEntityKilled,
////      cTechResearched???? <- easy we don't need this?
////
////      //Units/types.. list of units / list of types?
////      cCameraIsLookingAt,
////      //cCameraIsOnScreen,
////      //cCameraIsUnderCursor.
////
////      //wow moment ID   ...cinematic   / tag?
////      cWowMomentStart,
////      cWowMomentEnd,
////
////      //sound id?  sender player
////      cChatStart,
////      cChatEnd,
////
////      cSize
////   } BGeneralEventIDs;
////
////
////
////};
////
//////enum
//////{
//////   MGeneralEvents( MEnumerate )
//////}
//////#define MEnumerate(name) c##name,
//////#define MGeneralEvents( _m)\
//////		_m (ControlTilt) \
//////		_m (ControlZoom) \
//////		_m (ControlMultiSelect) 
////
////
////
////class BEventParameters
////{
////public:
////   //this is type...
////   typedef enum
////   {
////      cPlayer,
////      cProtoObject,
////      cProtoSquad,
////      cEntity,
////      cSquad,
////      cUnit,
////      cLocation,      
////      cWowID,
////      cChatID,
////      cSize  
////   } BGeneralEventParams;
////
////   typedef enum
////   {
////      cSource,  //cSource
////      cTarget,
////      cSize
////   } BGeneralEventParamSubject;
////
////   enum
////   {
////      cSingle,
////      cList,
////      cSize
////   };
////
////};
////
////
////class BGeneralEventManager;
////
////extern BGeneralEventManager gGeneralEventManager;
////
//////#define GENERALEVENT_TEMP(id, player, protoObject, protoSquad) if(gGeneralEventManager.hasSubscribers(id)) gGeneralEventManager.getEvent(id)->eventTrigger( player, protoObject, protoSquad); 
//////#define GENERALEVENT2T(id, t1, v1, t2, v2) if(gGeneralEventManager.hasSubscribers(id)) gGeneralEventManager.getEvent(id)->eventTrigger<t1,t2>( v1, v2); 
//////#define MGeneralEvent_Basic(id, player) if(gGeneralEventManager.hasSubscribers(id)) gGeneralEventManager.getEvent(id)->eventTrigger( player ); 
//////#define MGeneralEvent_SourceEntity(id, player, source) if(gGeneralEventManager.hasSubscribers(id)) gGeneralEventManager.getEvent(id)->eventTrigger( player, source ); 
////
////typedef enum
////{
////   cOk = 1,
////   cFail,
////} BPropertyRes;
////typedef enum
////{
////   cBEntityID_ProtoSquadID=1,
////   cBEntityID_Location,
////} BPropertyID;
////
////class BGameObjectAccessProtocol
////{
////   typedef enum
////   {
////      cEntity_ProtoObject,
////      cEntity_ProtoSquad,
////      cEntity_Location,
////      cEntity_Owner,
////      cSquad_Location,     
////   }
////public:
////   //static BPropertyRes masterConvert  //template on input... pass tempate in
////   
////   //static BPropertyRes masterConvert(BPropertyID id, 
////
////   //template<class T> static T Property(BPropertyID id, BEntityID prop);
////   static BPropertyRes Property2 (BPropertyID id, long in, long &out);
////   static BPropertyRes Property2 (BPropertyID id, long in, BVector &out);
////   //static bool Property(BPropertyID id, BEntityID prop);
////   // template<class T1, class T2> static T Property(BPropertyID id, T1 in, T2 out);
////};
////
////class BEventFilterBase
////{
////public:   
////   typedef enum
////   {
////      cEquals=1,
////   } BEventFilterOperators;
////
////   int  mParamID;  
////   //Param generic breakdown:
////   int  mParamType;
////   int  mParamSubject;
////   int  mParamCount;
////
////   static int makeParamID(int type, int subject = cSource, int count = cSingle){} 
////   
////   //Properties...
////   int  mProp1;
////   int  mProp2;
////   int  mProp3;
////
////
////   //Predicate def
////   BEventFilterOperators mOperator;
////
////
////   BTriggerVar* mValue1;
////
////   ////type
////   //int mValueType1;
////   //long mOtherValue1;
////   ////type
////   //int mValueType2;
////   //long mOtherValue2;
////   //
////
////
////   template<class T> bool Compare(T input)
////   {      
////      return false;
////   }
////
////   template<class T> bool Compare(BPropertyID i, T input)
////   {     
////      return false;
////   }
////};
////
////
////
////template<class R> class BEventFilter : public BEventFilterBase
////{
////public:   
////
////   BEventFilter(){}
////
////   template<class T> bool Compare(T input)
////   {
////      return ((T)(*mOtherValue1) == input); //?  bit compare
////      return true;
////   }
////
////   template<class T> bool CompareInner(BPropertyID i, T input)
////   {
////
////      R val2;
////      if(BGameObjectAccessProtocol::Property2(i, input, val2))
////      {
////         //  now we can compare val2  to   (R)mOtherValue1         
////      }
////      return true;
////   }
////
////   template<class T> bool Compare(BPropertyID i, T input)
////   {
////
////      R val2;
////      if(BGameObjectAccessProtocol::Property2(i, input, val2))
////      {
////         //  now we can compare val2  to   (R)mOtherValue1         
////      }
////      return true;
////   }
////
////   bool CompareInner(R input)
////   {
////      return ((T)(*mOtherValue1) == input); 
////      return true;
////   }
////
////
////   //template<class T> bool Compare3(T input);
////   //bool Compare2();
////   //template<class T> bool Convert(T input);
////
////};
////
////
////
//////=================================
////// BGeneralEventSubscriber
//////
//////=================================
////class BGeneralEventSubscriber 
////{
////public:
////
////   BGeneralEventSubscriber() { mFired=false; mCheckPlayer=false; mCheckProtoObject=false; mCheckProtoSquad=false;};
////   ~BGeneralEventSubscriber() {}
////
////   virtual void               onAcquire() {}
////   virtual void               onRelease() {}
////
////   DECLARE_FREELIST(BGeneralEventSubscriber, 5);
////
////   bool mFired;
////
////   bool hasFired() {return mFired;}
////   void setFired() {mFired = true;}
////
////   BGeneralEventSubscriberID mID;
////
////   //Event
////   void eventTrigger(BPlayerID playerID);
////   void eventTrigger(BPlayerID playerID, BEntityID sourceEntity, BEntityID targetEntity)
////
////   //void eventTrigger(long targetPlayerID=-1, long targetProtoUnitID=cInvalidProtoID, long targetProtoSquadID=cInvalidProtoID);
////
////   //Temp Filtering  
////   bool mCheckPlayer;
////   //bool mCheckProtoObject;
////   //bool mCheckProtoSquad;
////   long mPlayer;
////   //long mProtoObject;
////   //long mProtoSquad;
////
////   //Dyanmic Filters
////   BDynamicArray<BEventFilterBase*>      mEventFilters;
////
////
////   template<class T1, class T2> void eventTrigger(T1& v1, T2& v2)
////   {
////      if(mFired == true) return;
////
////      for(uint i=0; i< mEventFilters.size(); i++)
////      {
////         BEventFilterBase* pFilter = mEventFilters[i];
////         bool res = false;
////               
////         int paramId = pFilter->mParamID;
////         int convertID = 0;
////
////         if(paramId > 0xff)
////         {
////            convertID = (paramId  & 0xffff0000) >> 16;
////            paramId = (paramId & 0x0000ffff);
////         }
////
////         if(paramId == 1)
////         {
////            if(convertID > 0)
////            {
////               //BGameObjectAccessProtocol::Property<>(convertID, v1);
////            }  
////            
////            res = pFilter->Compare<T1>((BPropertyID)convertID, v1);
////            //res = pFilter->Compare<T1>(v1);
////         }
////         else if(paramId == 2)
////         {
////            
////            res = pFilter->Compare<T2>((BPropertyID)convertID, v2);
////            //res = pFilter->Compare<T2>(v2);
////         }
////
////         if(res == false)
////            return;      
////      }
////     
////      //trivial case
////      mFired = true;
////   }
////
////
////
////   template< class T> BEventFilterBase* addFilter()
////   { 
////      BEventFilterBase* pFilter = new BEventFilter<T>(); 
////      mEventFilters.add(pFilter); 
////      return pFilter; 
////   }
////
////   BEventFilterBase* addFilter2()
////   {
////      BEventFilterBase* pFilter = new BEventFilterBase(); 
////      mEventFilters.add(pFilter); 
////      return pFilter; 
////   }
////
////};
////
////
////
////
//////=================================
////// BGeneralEvent
//////
//////=================================
////class BGeneralEvent 
////{
////public:      
////
////   BGeneralEvent() {};
////   ~BGeneralEvent() {}
////
////   //Subscribers
////   uint  getSubscriberCount() { return mGeneralEventSubscribers.size(); }
////   BGeneralEventSubscriber* getSubscriber(uint index) { return mGeneralEventSubscribers.get(index); }
////   void addSubscriber(BGeneralEventSubscriber* pSubscriber);
////   void removeSubscriber(BGeneralEventSubscriber* pSubscriber);
////
////   //Events
////   //void notify2(DWORD eventType, long targetPlayerID, long targetProtoID, long targetProtoSquadID=cInvalidProtoID, long sourcePlayerID=-1, long sourceProtoID=-1, long sourceProtoSquadID=-1);
////   //void eventTrigger(long targetPlayerID, long targetProtoUnitID, long targetProtoSquadID);
////   
////   void eventTrigger(BPlayerID playerID)
////   {
////      for(uint i=0; i< mGeneralEventSubscribers.size(); i++)
////      {
////         mGeneralEventSubscribers[i]->eventTrigger(playerID);
////      }
////   }
////
////   void eventTrigger(BPlayerID playerID, BEntityID sourceEntity, BEntityID targetEntity)
////   {
////      for(uint i=0; i< mGeneralEventSubscribers.size(); i++)
////      {
////         mGeneralEventSubscribers[i]->eventTrigger(playerID, sourceEntity, targetEntity);
////      }
////   }
////
////
////   template<class T1, class T2> void eventTrigger(T1& v1, T2& v2)
////   {
////      for(uint i=0; i< mGeneralEventSubscribers.size(); i++)
////      {
////         mGeneralEventSubscribers[i]->eventTrigger<T1,T2>(v1, v2);
////      }
////   }
////
////protected:
////   BDynamicArray<BGeneralEventSubscriber*>      mGeneralEventSubscribers;
////
////};
////
////
//////==========================================
////// BGeneralEventManager
//////
//////==========================================
////class BGeneralEventManager
////{
////public:
////
////   BGeneralEventManager(){};
////   virtual ~BGeneralEventManager(){};      
////
////   // Management functions 
////   bool                       init(){};
////   void                       reset(){};
////
////   //Events 
////   BGeneralEvent* getEvent(BGeneralEventID id) { return &(mGeneralEvents[id]); }
////
////   //Subscribers
////   bool hasSubscribers(BGeneralEventID id) { return (mGeneralEvents[id].getSubscriberCount() > 0); }
////   BGeneralEventSubscriber* newSubscriber(BGeneralEventID id); 
////   BGeneralEventSubscriber* getSubscriber(BGeneralEventSubscriberID id);
////
////   //Sync support
////   void QueueSubscriberFired( BGeneralEventSubscriberID id);
////   void SubscriberFired( BGeneralEventSubscriberID id);
////   void FlushQueue();
////   
////   
////
////
////
////protected:
////
////   BGeneralEvent mGeneralEvents[BEventDefinitions::cSize];
////
////
////   BDynamicArray<uint>      mSubscriberFiredQueue;
////};
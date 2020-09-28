//==============================================================================
// squadactiontransport.h
// Copyright (c) 2006-2007 Ensemble Studios
//==============================================================================

#pragma once
#include "action.h"
#include "powerhelper.h"

class BSquad;
class BArmy;
class BUnitOpp;

//==============================================================================
//==============================================================================
class BTransportChildActionInfo
{
public:

   BTransportChildActionInfo() : mActionID(cInvalidActionID), mEntityID(cInvalidObjectID) {}
   ~BTransportChildActionInfo() {}

   virtual bool save(BStream* pStream, int saveType) const;
   virtual bool load(BStream* pStream, int saveType);

   BActionID      mActionID;
   BEntityID      mEntityID;
};
typedef BSmallDynamicSimArray<BTransportChildActionInfo> BTransportChildActionInfoArray;

//==============================================================================
//==============================================================================
class BTransportData
{
   public:
      BTransportData::BTransportData():
         mPickUpLoc(cInvalidVector),
         mDropOffLoc(cInvalidVector),
         mPos(cInvalidVector),
         mForward(cInvalidVector),
         mRight(cInvalidVector),
         mTransportSquad(cInvalidObjectID)
         { 
            mSquadsToTransport.clear(); 
         }
      BTransportData::~BTransportData(){};

      // Operator overloads
      bool operator == (const BTransportData rhs) const { return (mTransportSquad == rhs.mTransportSquad); }
      bool operator != (const BTransportData rhs) const { return (mTransportSquad != rhs.mTransportSquad); }

      void           setSquadsToTransport(const BEntityIDArray& squadList) { mSquadsToTransport.assignNoDealloc(squadList); }
      const BEntityIDArray& getSquadsToTransport() { return (mSquadsToTransport); }

      void           setPickUpLoc(BVector pickUp) { mPickUpLoc = pickUp; }
      BVector        getPickUpLoc() { return (mPickUpLoc); }

      void           setDropOffLoc(BVector dropOff) { mDropOffLoc = dropOff; }
      BVector        getDropOffLoc() { return (mDropOffLoc); }

      void           setPos(BVector pos) { mPos = pos; }
      BVector        getPos() { return (mPos); }

      void           setForward(BVector forward) { mForward = forward; }
      BVector        getForward() { return (mForward); }

      void           setRight(BVector right) { mRight = right; }
      BVector        getRight() { return (mRight); }

      void           setTransportSquad(BEntityID transportSquad) { mTransportSquad = transportSquad; }
      BEntityID      getTransportSquad() { return (mTransportSquad); }

   private:
      BEntityIDArray mSquadsToTransport;
      BVector mPickUpLoc;
      BVector mDropOffLoc;
      BVector mPos;
      BVector mForward;
      BVector mRight;
      BEntityID mTransportSquad;
};

typedef BSmallDynamicSimArray<BTransportData> BTransportDataArray;

//==============================================================================
//==============================================================================
class BSquadActionTransport : public BAction
{
   public:
      BSquadActionTransport() {}
      virtual ~BSquadActionTransport() {}
 
      //Connect/disconnect.
      virtual bool               connect(BEntity* pOwner, BSimOrder* pOrder);
      virtual void               disconnect();
      //Init.
      virtual bool               init();

      virtual bool               setState(BActionState state);
      virtual bool               update(float elapsed);

      virtual void               notify(DWORD eventType, BEntityID senderID, DWORD data1, DWORD data2);

      // Member interfaces
      void                       setSquadList(const BEntityIDArray &squadList) { mSquadList.assignNoDealloc(squadList); }
      void                       setPickupLocation(BVector location) { mPickupLocation = location; }
      BVector                    getPickupLocation() { return (mPickupLocation); }
      void                       setDropOffLocation(BVector location) { mDropOffLocation = location; }
      void                       setFlyOffLocation(BVector location) { mFlyOffLocation = location; }
      void                       setRallyPoint(BVector rallyPoint) { mRallyPoint = rallyPoint; }
      void                       setCompletionSoundCue(BCueIndex cueIndex) { mCompletionSoundCue = cueIndex; }
      void                       setFacing(BVector facing) { mFacing = facing; }
      void                       notifySquadWasCommanded(BEntityID squadID);

      // Flag interfaces
      void                       setLoadSquads(bool v) { mFlagLoadSquads = v; }
      void                       setFlyInFrom(bool v) { mFlagFlyInFrom = v; }
      void                       setTransportSquads(bool v) { mFlagTransportSquads = v; }
      void                       setUnloadSquads(bool v) { mFlagUnloadSquads = v; }
      void                       setMoveToRallyPoint(bool v) { mFlagMoveToRallyPoint = v; }
      void                       setFlyOffTo(bool v) { mFlagFlyOffTo = v; }
      void                       setUseMaxHeight(bool v) { mFlagUseMaxHeight = v; }
      void                       setAttackMove(bool v) { mFlagAttackMove = v; }
      void                       setUseFacing(bool v) { mFlagUseFacing = v; }
      void                       setControllableTransport(bool v) { mFlagControllableTransport=v; }
      void                       setUseAnimations(bool v) { mFlagUseAnimations = v; }
      void                       setTrainingAlert(bool v) { mFlagTrainingAlert = v; }

      // Interface functions
      static void                filterTransportableSquads(const BEntityIDArray &squadsToTransport, BPlayerID playerID, bool checkTransportingFlag, BEntityIDArray &results);
      static uint                calculateNumTransports(const BEntityIDArray &transportingSquads);           
      static void                calculateTransportData(const BEntityIDArray &squadsToTransport, BPlayerID playerID, const BEntityIDArray &transportSquads, BVector dropOffLocation, bool updateTransports, BTransportDataArray &transportData);
      static bool                transportSquads(BSimOrder* pSimOrder, const BEntityIDArray &squadList, BVector dropOffLocation, BPlayerID playerID, int transportObjectID);
      static bool                flyInSquad(BSimOrder* pSimOrder, BSquad* pSquad, const BVector* pFlyInLocation, BVector dropoffLocation, BVector dropoffForward, BVector dropoffRight, const BVector* flyOffLocation, BPlayerID playerID, int transportObjectID, bool moveToRallyPoint, BVector rallyPoint, bool attackMove, BCueIndex soundCue, bool useMaxHeight, bool useFacing, BVector facing);
      static bool                flyInSquads(BSimOrder* pSimOrder, const BEntityIDArray &squadList, const BVector* pFlyInLocation, BVector dropoffLocation, BVector dropoffForward, BVector dropoffRight, const BVector* pFlyOffLocation, BPlayerID playerID, int transportObjectID, bool moveToRallyPoint, BVector rallyPoint, bool attackMove, BCueIndex soundCue, bool useMaxHeight, bool useFacing, BVector facing);

      virtual bool               isInterruptible() const;

      bool                       getHoverOffset(float& offset) const;

      DECLARE_FREELIST(BSquadActionTransport, 5);

      virtual bool save(BStream* pStream, int saveType) const;
      virtual bool load(BStream* pStream, int saveType);

   protected:

      bool                       flyInTransports();
      bool                       flyOffTransports();
      bool                       loadSquads();
      void                       preLoadSquads();
      bool                       flyInFrom();
      bool                       moveTransports();
      bool                       flyOffTo();
      bool                       unloadSquads();
      void                       selfDestruct();
      void                       playCompletionSoundCue();
      bool                       positionSquads();
      bool                       validateUnitDist(BVector testPos, float threshold);
      bool                       validateUnitMotion(float elapsed, float maxStillTime, float minMovementDist);      
      bool                       forceLoadSquads();
      bool                       cleanUpLoadFailure();
      bool                       anySquadsLoaded();
      void                       attachOrContainUnit(BUnit* pUnit, bool occlude = false);

      void                       removeChildActions();
      bool                       actionsComplete(BActionID id);
      bool                       addOpp(BUnitOpp opp);
      void                       removeOpp();
      void                       setupChildOrder(BSimOrder* pOrder);

      BSimTarget                  mTarget;      
      BEntityIDArray              mSquadList;
      BSmallDynamicSimArray<uint> mTransportIndex;
      BTransportChildActionInfoArray mChildActionInfos;
      BVector                     mPickupLocation;
      BVector                     mDropOffLocation;
      BVector                     mFlyOffLocation;
      BVector                     mRallyPoint;        
      BVector                     mFacing;
      BVector                     mMovementPos;
      BSimOrder*                  mpChildOrder;
      float                       mMovementTimer;
      DWORD                       mBlockedTimeStamp;      
      BUnitOppID                  mUnitOppID;  
      BCueIndex                   mCompletionSoundCue;
      BActionState                mFutureState;      
      uint8                       mUnitOppIDCount;

      bool                        mFlagLoadSquads:1;
      bool                        mFlagFlyInFrom:1;
      bool                        mFlagTransportSquads:1;
      bool                        mFlagUnloadSquads:1;
      bool                        mFlagMoveToRallyPoint:1;
      bool                        mFlagFlyOffTo:1;
      bool                        mFlagUseMaxHeight:1;
      bool                        mFlagActionFailed:1;
      bool                        mFlagAnyFailed:1;      
      bool                        mFlagAttackMove:1;
      bool                        mFlagUseFacing:1;
      bool                        mFlagControllableTransport:1;
      bool                        mFlagTrainingAlert:1;
      bool                        mFlagNewDropOff:1;
      bool                        mFlagBlocked:1;
      bool                        mFlagNotifyReceived:1;
      bool                        mFlagUseAnimations:1;
      bool                        mFlagIgnoreNotify:1;
};

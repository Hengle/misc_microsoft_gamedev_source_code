//==============================================================================// platoon.h
// Copyright (c) 2007 Ensemble Studios
//==============================================================================

#pragma once
#include "Entity.h"
#include "SimOrder.h"
#include "squadplotter.h"


#define _MOVE4

class BCommand;
class BFormation2;
class BSquad;
class BPath;


//==============================================================================
// Debugging
#ifndef BUILD_FINAL
   extern BEntityID sDebugPlatoonTempID;
   void dbgPlatoonInternal(const char *format, ...);
   void dbgPlatoonInternalTempID(const char *format, ...);
   
   #define debugPlatoon dbgPlatoonInternal
#else
   #define debugPlatoon __noop
#endif



//==============================================================================
//==============================================================================
class BPathingHints
{
public:
   void init()
   {
      mWaypoints.clear();
      mID = cInvalidObjectID;
      mComplete = false;
   }

   bool save(BStream* pStream, int saveType) const;
   bool load(BStream* pStream, int saveType);

   BDynamicSimVectorArray  mWaypoints;
   BEntityID               mID;
   bool                    mComplete:1;
};


//==============================================================================
//==============================================================================
class BPlatoon : public BEntity
{
   public:
      BPlatoon() { }
      virtual ~BPlatoon()  { mActions.clearActions(); }
      virtual void            onRelease();
      virtual void            init();

      //Velocity.
      virtual float           getMaxVelocity() const;
      virtual float           getDesiredVelocity() const;
      virtual bool            canJump() const;
      //Hacky stuff.
      bool                    getSlowestSquadVelocityValid() const { return(mSlowestSquadVelocityValid); }
      float                   getSlowestSquadVelocity() const { return(mSlowestSquadVelocity); }
      float                   getPathingRadius() const;
      void                    calculateRange(BSimTarget* pTarget) const;
      virtual float           getPathingRange() const;
      float                   getDesiredMovementProgress() const;

      //Update.
      bool                    updatePreAsync(float elapsedTime);
      bool                    updateAsync(float elapsedTime);
      virtual bool            update(float elapsedTime);
      virtual bool            isAlive() const { return(true); }

      //Merge.
      void                    mergePlatoon(BPlatoon* pPlatoon);
      bool                    canMerge();

      //Order Management.  These are meant to be called by Platoons and Squads "under the covers".
      void                    updateOrders();
      void                    executeOrder(BSimOrderEntry *pOrderEntry);
      void                    removeAllOrders();
      //Base Order queueing.
      BSimOrderType           getOrderType(const BCommand* pCommand) const;
      BSimOrderType           getOrderType(BSimTarget target, const BCommand* pCommand) const;
      bool                    queueOrder(const BCommand *pCommand);
      bool                    queueOrder(BSimOrder* pOrder);
      bool                    queueOrder(BSimOrder* pOrder, BSimOrderType orderType);
      bool                    queueOrderToChildren(const BCommand *pCommand);
      bool                    queueOrderToChildren(BSimOrder* pOrder);
      //Order access.
      uint                    getNumberOrders() const { return (mOrders.getSize()); }
      const BSimOrderEntry*   getOrderEntry(uint index) const { if (index >= mOrders.getSize()) return (NULL); return (&(mOrders[index])); }
      const BSimOrderEntry*   getOrderEntry_4(bool execute, bool paused) const;

      ///Order methods for generic sim usage.  If you're working in actions, AI, etc., these are the methods you
      //should be calling.
      bool                    queueMove(BSimTarget target, uint priority=BSimOrder::cPrioritySim);
      bool                    queueWork(BSimTarget target, uint priority=BSimOrder::cPrioritySim);
      //Attack Move Prop.  DO NOT CALL THIS unless you're specifically fixing Attack Move stuff.
      void                    propagateAttackMove();

      //Actions.
      virtual bool            removeAllActionsForOrder(BSimOrder* pOrder);
      void                    stopChildren();

      //Pathing hints
      DWORD                   getPathingHintsTime() const                     { return mPathingHintsTime; }
      uint                    getPathingHintsSimOrderID() const               { return mPathingHintsSimOrderID; }
      bool                    getPathingHintsComplete(BEntityID childID, BSimOrder* pOrder);
      void                    setPathingHintsComplete(BEntityID childID, BSimOrder* pOrder);
      bool                    getInitialPathingHintsForChild(BEntityID childID, uint startingHintIndex, BDynamicSimVectorArray& resultPathingHints, uint& maxSiblingHintIndex);
      bool                    getFinalPathingHintForChild(BEntityID childID, uint pathingHintIndex, bool useSquadPlotter, BEntityID squadPlotterTargetID, long abilityID, BVector& finalPathingHint, float overrideRange = -1.0f, BVector overrideTargetPos = cInvalidVector, float overrideRadius = -1.0f);
      bool                    getFinalPathingHintValid(uint pathingHintIndex) const;
      void                    setFinalPathingHintValid(uint pathingHintIndex, bool valid);

      //Formation.
      const BFormation2*      getFormation() const { return (mpFormation); }
      void                    updateFormation(const BPath *path = NULL);

      //Kill/Destroy.
      virtual void            kill(bool bKillImmediately);
      virtual void            destroy();

      // Movement Type
      long                    getMovementType();

      //Army.
      BArmy*                  getParentArmy() const;
      //Children.
      long                    addChild(BSquad *squad);
      bool                    removeChild(BSquad* squad);
      uint                    getNumberChildren() const { return(mChildren.getNumber()); }
      BEntityID               getChild(uint index) const;
      const BEntityIDArray&   getChildList() const { return (mChildren); }
      void                    addPendingChildToRemove(BSquad* squad);
      float                   getDefaultMovementProjectionTime() const;

      BEntityID               getHigherMovementPriority(BEntityID child1, BEntityID child2) const;
      uint                    getDefaultMovementPriority(BEntityID childID) const;
      uint                    getMovementPriority(BEntityID childID) const;

      // Playable area
      virtual bool            isOutsidePlayableBounds(bool forceCheckWorldBoundaries=false) const;

      //Debug.
      virtual void            debugRender();

   protected:
      bool                    isPathableAsFlyingUnit() const;
      bool                    isPathableAsFloodUnit() const;
      bool                    isPathableAsScarabUnit() const;
      bool                    getTargetPosFromOrder(BSimOrder* pOrder, BVector& targetPosition);
      void                    syncPositionWithSquads(const BDynamicSimVectorArray& waypoints, BSimTarget target, BPath& pathToFirstWaypoint, bool& pathValid);
      BPathingHints*          getPathingHints(BEntityID childID, bool returnDefaultIfNotFound);
      bool                    generatePathingHints(const BDynamicSimVectorArray& waypoints, BSimOrder* pOrder, bool append);
      bool                    applyFormationToPathingHints(uint startHintIndex, uint endHintIndex, bool useSquadPlotter, BEntityID squadPlotterTargetID, long abilityID, float overrideRange = -1.0f, BVector overrideTargetPos = cInvalidVector, float overrideRadius = -1.0f);
      bool                    tackOnWaypoints(const BCommand* pCommand);
      DWORD                   getMoveActionStartTime(BSquad* pSquad) const;
      bool                    repositionTargetLocationIfInsidePlayerObstruction(BVector &targetPosition);  //for warthog jumping
      bool                    repositionWaypointIfOneWayBarrierIsCrossed(BPath &path);
      float                   getBoundingRadius();
      void                    removeOrders(bool current=true, bool queued=true, bool delayChildOrderRemoval=false);
      void                    removeOrderFromChildren(BSimOrder* pOrder, bool uninterruptible, bool delayChildOrderRemoval);
      bool                    queueChangeModeOrderToChildren(const BCommand *pCommand);

      // DLM Delicious Debug Stuff
      BPath                   mDebugCommandPath;
      BPath                   mDebugReducedPath;
      BVector                 mDebugForwardPosition;

      // Sound
      void                    notifySound(const BSimOrder* pOrder);

      BDynamicSimArray<BPathingHints> mPathingHints;        // 12 bytes
      BDynamicSimVectorArray  mPathingHintForwards;         // 12 bytes
      BEntityIDArray          mChildren;                    // 8 bytes
      BEntityIDArray          mPendingChildrenToRemove;     // 8 bytes
      BSimOrderEntryArray     mOrders;                      // 8 bytes

      BFormation2*            mpFormation;                  // 4 bytes
      BEntityID               mSlowestSquadID;
      float                   mSlowestSquadVelocity;        // 4 bytes
      uint                    mPathingHintsSimOrderID;      // 4 bytes
      DWORD                   mPathingHintsTime;            // 4 bytes

      bool                    mSlowestSquadVelocityValid:1; // 1 byte   (1/8)

   #ifdef _MOVE4
   public:
      void                    executeOrder_4(BSimOrderEntry *pOrderEntry);

      // Request Paths
      const BPath &           getCurrentLLPath_4() const 
                              { return mCurrentLLPath_4; }
      const BPath &           getCurrentHLPath_4() const 
                              { return mCurrentHLPath_4; }
      const BPath &           getCurrentUserPath_4() const 
                              { return mCurrentUserPath_4; }

      // Request WP's
      int32                   getCurrentUserWP_4() const
                              { return mCurrentUserWP_4; }
      int32                   getCurrentHLWP_4() const
                              { return mCurrentHLWP_4; }
      int32                   getCurrentLLWP_4() const
                              { return mCurrentLLWP_4; }
      void                    initLLWP_4()                    // That's right 1.  Our initial path seg is from 0 to 1.
                              { mCurrentLLWP_4 = 1; }

      void                    resetLLP_4();

      bool                    pathToNextUserWP_4(int32 &pathResult);
      bool                    pathToNextHLWP_4(int32 &pathResult);
      // Request a repath!
      bool                    repathToTarget_4(int32 &pathResult);

      // Waypoint functions
      bool                    advanceLLWP_4();
      bool                    advanceHLWP_4(BActionState &adjustedState);
      bool                    advanceUserWP_4();

      // Checking to see if we're close enough to the various waypoints
      bool                    inLLWPRange_4();
      bool                    inHLWPRange_4();
      bool                    inHLWPRange_4(float &fDist);
      bool                    inUserWPRange_4();
      bool                    inRangeOfTarget_4();
      bool                    getPlatoonRange_4(BSimTarget &target);

      // Pause the entire platoon
      void                    pauseMovement_4(DWORD pauseTime);

      // Advance our Position
      bool                    advancePosition_4(float elapsedTime, float velocityFactor);
      float                   getPlatoonVelocity_4();
      bool                    isSquadAtTarget_4(BEntityID child);
      bool                    isSquadDoneMoving_4(const BEntityID child);
      bool                    isSquadProceedingToPlot_4(const BEntityID child);


      // Squad Manipulation
      void                    notifyThatSquadHasPaused_4(BEntityID child);
      void                    notifyThatSquadWillHandleItsOwnMovement_4(BEntityID child);
      
      bool                    isPlatoonDoneMoving_4(bool *isPaused=NULL);
      bool                    hasValidPlots_4() const { return mValidPlots_4; }
      void                    computePlots_4();
      bool                    getPlottedPosition_4(const BEntityID child, BVector &position);
      bool                    hasDefaultSquadPlot_4(const BEntityID child);
      void                    setSquadPlotWaypoint();
      long                    getSquadPlotWaypoint() { return mSquadPlotWaypoint; }
      float                   getSquadRange(long index) { return (index < 0 || index > mSquadRanges.getNumber() - 1)?-1.0f:mSquadRanges[index]; }

      void                    attackMoveNotify(BEntityID child, BSimTarget target);


      GFDECLAREVERSION();
      virtual bool save(BStream* pStream, int saveType) const;
      virtual bool load(BStream* pStream, int saveType);

   protected:
      bool                    tackOnWaypoints_4(const BCommand* pCommand);
      BActionID               doMove_4(BSimOrder* pOrder, BAction* pParentAction, BSimTarget* pTarget);
      //BActionID               moveSquad(BEntityID squadID, BSimOrder* pOrder, BAction* pParentAction, BSimTarget* pTarget, bool monitorOpps = false, bool autoSquadMode = false, bool forceLeashUpdate = false);
      void                    buildIgnoreList_4();

      // These are new routines, but we're past the _4 stage of the game now.  
      void                    doSquadRelocation();


      BPath                   mCurrentUserPath_4;  // Current User Path
      BPath                   mCurrentHLPath_4;       // Current High Level Path
      BPath                   mCurrentLLPath_4;       // Current Low Level Path
      int32                   mCurrentUserWP_4;
      int32                   mCurrentHLWP_4;
      int32                   mCurrentLLWP_4;
      BSimTarget              mCurrentTarget_4;    // This target is defined by the currently active order the platoon is working on.
      BEntityIDArray          mIgnoreList_4;
      BDynamicSimArray<BSquadPlotterResult> mSquadPlots_4;
      bool                       mValidPlots_4;
      long                    mSquadPlotWaypoint;
      BDynamicSimArray<float> mSquadRanges;    // Cache off squad ranges.  This should be kept 1 for 1 with children.

      // Relocation Variables
      BSimTarget              mRelocationTarget;
      long                    mSquadToRelocate;
      bool                    mRelocationInProgress;
      BActionID               mRelocationActionID;


   #endif

};

//==============================================================================
// squad.h
// Copyright (c) 2005-2007 Ensemble Studios
//==============================================================================

#pragma once
#include "entity.h"
#include "squadai.h"
#include "SimOrder.h"
#include "triggerscript.h"
#include "triggervar.h"
#include "UnitOpportunity.h"
#include "math\runningAverage.h"

class BArmy;
class BFormation2;
class BCommand;
class BProtoAction;
class BProtoObject;
class BProtoSquad;
class BObjectCreateParms;
class BSquadActionMove;

#ifndef _MOVE4
#define _MOVE4
#endif

#ifdef _MOVE4
   #define NEW_TURNRADIUS
#endif



//==============================================================================
// Debugging
#ifndef BUILD_FINAL
   extern BEntityID sDebugSquadTempID;
   void dbgSquadInternal(const char *format, ...);
   void dbgSquadInternalTempID(const char *format, ...);

   #define debugSquad dbgSquadInternal
#else
   #define debugSquad __noop
#endif


#ifndef BUILD_FINAL
class BSquadStats 
{
public:
   BSquadStats() { init(); }
   ~BSquadStats() { };

   double                  mUpdateEntityTime;
   double                  mUpdateFormationTime;
   double                  mUpdateGarrisonedSquadsTime;
   double                  mUpdateHitchedSquadTime;
   double                  mUpdateLeashTime;
   double                  mUpdateOrdersTime;
   double                  mUpdateSquadAITime;

   double                  mMaxEntityTime;
   double                  mMaxFormationTime;
   double                  mMaxGarrisonedSquadsTime;
   double                  mMaxHitchedSquadTime;
   double                  mMaxLeashTime;
   double                  mMaxOrdersTime;
   double                  mMaxSquadAITime;

   BRunningAverage<double,double> mAvgEntityTime;
   BRunningAverage<double,double> mAvgFormationTime;
   BRunningAverage<double,double> mAvgGarrisonedSquadsTime;
   BRunningAverage<double,double> mAvgHitchedSquadTime;
   BRunningAverage<double,double> mAvgLeashTime;
   BRunningAverage<double,double> mAvgOrdersTime;
   BRunningAverage<double,double> mAvgSquadAITime;

   uint                    mReferenceFrame;
      
   void init()
   {
      mUpdateEntityTime = 0.0;
      mUpdateFormationTime = 0.0;
      mUpdateGarrisonedSquadsTime = 0.0;
      mUpdateHitchedSquadTime = 0.0;
      mUpdateLeashTime = 0.0;
      mUpdateOrdersTime = 0.0;
      mUpdateSquadAITime = 0.0;

      mMaxEntityTime = 0.0;
      mMaxFormationTime = 0.0;
      mMaxGarrisonedSquadsTime = 0.0;
      mMaxHitchedSquadTime = 0.0;
      mMaxLeashTime = 0.0;
      mMaxOrdersTime = 0.0;
      mMaxSquadAITime = 0.0;

      mAvgEntityTime.set(60);
      mAvgFormationTime.set(60);
      mAvgGarrisonedSquadsTime.set(60);
      mAvgHitchedSquadTime.set(60);
      mAvgLeashTime.set(60);
      mAvgOrdersTime.set(60);
      mAvgSquadAITime.set(60);

      mReferenceFrame = 0;
   }

};
#endif

//==============================================================================
//==============================================================================
class BSquad : public BEntity 
{
   public:
      enum
      {
         cEffectRecovering,
      };

      // Enum to assign ID to flag for trigger
      enum
      {
         cFlagAttackBlocked = 0,
      };

      // Smart targeting enums
      enum
      {
         cSmartTargetStasis,
         cSmartTargetDaze,

         //  Max
         cSmartTargetMax,
      };

      //-- don't do anything in the constructor of these (it only gets called one time for the whole game)
      BSquad() {};
      //-- this needs free everything we have allocated
      virtual ~BSquad();

      virtual void            onRelease();
      virtual void            init();

      bool                    initFromProtoSquad(const BProtoSquad* pProtoSquad, BObjectCreateParms& parms, BEntityIDArray* existingUnits=NULL);
      void                    initFormation();
      void                    initPosition(bool fromSave);

      void                    setPosition(const BVector loc, bool overridePhysics=true);

      void                    transform(long newProtoSquadID, bool techUpgrade);
      void                    transform(const BProtoSquad* pOldProto, const BProtoSquad* pNewProto, bool techUpgrade);
      void                    preTransform(const BProtoSquad* pOldProto, const BProtoSquad* pNewProto, bool techUpgrade);
      void                    postTransform(const BProtoSquad* pOldProto, const BProtoSquad* pNewProto, bool techUpgrade);
      virtual void            changeOwner(BPlayerID newPlayerID, bool makeUniqueProtoSquad = false, BEntityID sourceID=cInvalidObjectID);

      virtual void            setProtoID(long protoID);
      virtual long            getProtoID() const { return(mProtoID); }
      long                    getProtoSquadID() const;
      const BProtoSquad*      getProtoSquad() const;
      long                    getProtoObjectID() const;

      bool                    isKBAware() const;

      BSquadAI*               getSquadAI() { return &mSquadAI; }
      BPlatoon*               getParentPlatoon() const;
      BArmy*                  getParentArmy() const;

      BAIMissionID            getAIMissionID() const { return (mAIMissionID); }
      void                    setAIMissionID(BAIMissionID missionID) { mAIMissionID = missionID; }
      void                    removeFromAIMission();

      virtual long            computeObstructionType();

      float                   getPathingRadius() const;
      float                   getLargestUnitRadius() const;

      //Visible Squads.  Refresh can update the two lists (if the time has elapsed and there is an available search).
      //It should generally be called before a get on either list.
      void                    refreshVisibleSquads();
      const BEntityIDArray&   getVisibleSquads() const { return (mVisibleSquads); }
      const BEntityIDArray&   getVisibleEnemySquads() const { return (mVisibleEnemySquads); }

      //Children.
      const BEntityIDArray&   getChildList() const { return (mChildren); }
      bool                    addChild(BEntityID id);
      bool                    removeChild(BEntityID id, bool fixup);
      void                    removeInvalidChildren();
      virtual uint            getNumberChildren() const { return mChildren.getNumber(); }
      BEntityID               getChild(long index) const;
      bool                    containsChild(BEntityID unitID) const;
      bool                    allChildrenInList(const BEntityIDArray& list, bool mustContainAllListedUnits = true);
      long                    findChildIndexFromID( BEntityID childID )   { return mChildren.find(childID); }
      long                    stopAllChildren(DWORD state);
      bool                    transferChildren(BSquad* pSquadTo);
      virtual bool            canJump() const;
      bool                    isChildLeashed(BEntityID id, float fudgeDistance=0.0f) const;
      bool                    isChildLeashed(BEntityID id, float fudgeDistance, BVector& leashLocation) const;
      bool                    areAllChildrenLeashed(float fudgeDistance=0.0f) const;
      bool                    isChildInRange(bool meleeRange, BEntityID id, BSimTarget target, float minRange, float maxRange, float childMaxRangeFudgeDistance=0.0f) const;
      bool                    isSquadAPhysicsVehicle() const;

      //Movement priority.
      BEntityID               getHigherMovementPriority(BEntityID child1, BEntityID child2) const;
      uint                    getMovementPriority(BEntityID childID) const;

      //Update.
      virtual bool            updatePreAsync(float elapsedTime);
      virtual bool            updateAsync(float elapsedTime);
      virtual bool            update(float elapsedTime);
      virtual void            render();
      void                    updateLeash();
      void                    updateGarrisonedFlag();
      void                    updateAttachedFlag();
      void                    updateGarrisonedSquads();
      void                    updateHitchedSquad();
      virtual void            kill(bool bKillImmediately);
      void                    killHero();
      virtual void            destroy();

      virtual bool            isAlive() const;
      virtual bool            isEverMobile() const;
      bool                    isDown() const;
      bool                    isHibernating() const;

      //Order Management.  These are meant to be called by Platoons and Squads "under the covers".
      void                    updateOrders();
      void                    executeOrder(BSimOrderEntry *pOrderEntry);
      void                    removeAllOrders() { removeOrders(true, true, true, true); }
      void                    removeOrders(bool execute, bool paused, bool queued, bool uninterruptible, bool delayRemoval=false);
      void                    removeAutoGeneratedAttackMoveOrders();
      void                    removeOrder(BSimOrder* pOrder, bool removeUninterruptible, bool delayRemoval);
      bool                    mergeRedundantOrders(BSimOrderEntry* pOldOrderEntry, BSimOrderEntry* pNewOrderEntry);
      //Base Order queueing.
      BSimOrderType           getOrderType(const BCommand* pCommand) const;
      BSimOrderType           getOrderType(BSimTarget target, const BCommand* pCommand, const BSimOrder* pOrder) const;
      bool                    queueOrder(const BCommand *pCommand, bool prepend=false);
      bool                    queueOrder(BSimOrder* pOrder, BSimOrderType orderType, bool prepend=false);
      //Order pause/unpause.  DO NOT call these unless you know what you're doing.
      void                    pauseOrder(BSimOrderEntry *pOrderEntry);
      void                    unpauseOrder(BSimOrderEntry *pOrderEntry);
      //Order access.
      uint                    getNumberOrders() const { return (mOrders.getSize()); }
      const BSimOrderEntry*   getOrderEntry(bool execute, bool paused) const;
      const BSimOrderEntry*   getOrderEntry(uint index) const { if (index >= mOrders.getSize()) return (NULL); return (&(mOrders[index])); }
      BSimOrder*              getOrderByID(uint orderID) const;
      ///Order methods for generic sim usage.  If you're working in actions, AI, etc., these are the methods you
      //should be calling.
      bool                    queueMove(BSimTarget target);
      bool                    queueAttack(BSimTarget target, bool attackMove=false, bool informPlatoon=true, bool autoGeneratedAttackMove=false);
      bool                    queueIdle();      
      bool                    queueWork(BSimTarget target, uint8 priority = BSimOrder::cPrioritySim);
      bool                    queueUnload(uint8 priority, BEntityIDArray& garrisonedSquads, bool useRallyPoint = false, BPlayerID thisPlayerOnly = cInvalidPlayerID);

      //Opps.      
      bool                    addOppToChildren(BUnitOpp &opp) const;
      bool                    addOppToMaxNumChildren(BUnitOpp &opp, int maxNumChildrenToAddOpp) const;
      bool                    addOppToChildren(BUnitOpp &opp, uint8& count) const;
      bool                    addOppToChildren(BUnitOpp &opp, BDynamicSimArray<uint>* oppIDs, BEntityIDArray* children=NULL) const;
      void                    removeOppFromChildren(BUnitOppID oppID, bool removeActions=true) const;
      void                    removeOppFromChildren(BDynamicSimArray<uint>& oppIDs, bool removeActions=true) const;

      //Actions.
      virtual bool            removeAllActionsForOrder(BSimOrder* pOrder);
      bool                    pauseAllActionsForOrder(BSimOrder* pOrder);
      bool                    unpauseAllActionsForOrder(BSimOrder* pOrder);
      //DCPTODO: These all need to "go away" by way of a better impl that fits with the new model.
      bool                    doPlayBlockingAnimation(BSimOrder* pOrder, long state, long type, bool applyInstantly, bool useSquadMatrix, bool useMaxHeight, BCueIndex soundCue=cInvalidCueIndex, bool loop=false, bool disableMotionExtractionCollisions=false, bool updateSquadPhysicsPosAtEnd=false, bool clearUnitNoRenderFlag=false, bool birthAnim=false);
      // Halwes - 7/17/2006 - Added queueUnload to be used in place of this with the new sim model.  This was left here due to the fact that some
      //                      older trigger revisions haven't been phased out and may still be using this.
      //bool                    doUnload();
      bool                    doTeleport(BVector location, long searchScale = 1, bool testObstructions = true, bool clearActions = true);
      void                    addPersistentSquadActions(BUnit* pChild);
      void                    removePersistentTacticActions();

      bool                    pullSquad(BEntityID attackerID, int32 attackerProtoActionID);

      //Commands.
      BProtoAction*           getProtoActionForTarget(BEntityID targetID, BVector targetLoc, long abilityID, bool autoTarget, bool* pInsideMinRange=NULL, bool noDiscardAbilities=false, long* pRuleAbilityID=NULL, bool* pTargetsGround=NULL) const;
      bool                    canAttackTarget(BEntityID targetUnit) const;

      //Misc.
      virtual bool            isVisible(BTeamID teamID) const;
      bool                    isVisibleOnScreen() const;
      virtual bool            isVisibleOrDoppled(BTeamID teamID) const;
      uint                    getNumVisibleChildren(BTeamID teamID) const;

      void                    dirtyLOS();
      float                   getLOS() const;

      // True LOS
      bool                    validateLOS(const BProtoAction *pSourceSquadAction, const BSquad *targetSquad);

      virtual bool            isOutsidePlayableBounds(bool forceCheckWorldBoundaries=false) const;

      float                   getAttackRating(BDamageTypeID damageType) const;
      float                   getAttackRatingDPS(BDamageTypeID damageType) const;
      float                   getAttackRating() const;
      float                   getDefenseRating() const;
      float                   getStrength() const;
      uint                    getAttackGrade(BDamageTypeID damageType) const;
      float                   getAttackGradeRatio(BDamageTypeID damageType) const;
      float                   getStars(BDamageTypeID damageType) const { return (getAttackGradeRatio(damageType)); }
      BDamageTypeID           getDamageType() const;
      bool                    isDamageType(BDamageTypeID damageType) const;
      float                   getCombatValue() const;
      float                   getCombatValueHP() const;
     
      BEntityID               getLeastAttackedUnitID() const;
      BEntityID               getRandomUnitID(BUnit* pUnit, float angleLimit) const;
      BEntityID               getLeader() const;
      BUnit*                  getLeaderUnit() const;
      long                    getUnitType() const;
      virtual float           getMaxVelocity() const;
      virtual float           getDesiredVelocity() const;
      float                   getActionVelocity() const;
      virtual float           getAcceleration() const;
      virtual bool            canMove(bool allowAutoUnlock=false) const;
      bool                    canMoveForOrder(BSimOrder* pOrder) const;
      
      void                    getWidthAndDepth(float &width, float &depth) const;
      float                   getHPMax() const;
      float                   getSPMax() const;
      float                   getAmmoMax() const;
      float                   getCurrentHP() const;
      float                   getCurrentSP() const;
      float                   getCurrentAmmo() const;
      float                   getHPPercentage() const;
      float                   getSPPercentage() const;
      float                   getAmmoPercentage() const;
      void                    adjustAmmunition(float adjust);
      bool                    getIgnoreUserInput() const;
      void                    setIgnoreUserInput(bool ignore);
      BEntityIDArray          getGarrisonedSquads(BPlayerID playerID = cInvalidPlayerID);
      BEntityIDArray          getCoverSquads(BPlayerID playerID = cInvalidPlayerID);
      BEntityIDArray          getCoverUnits(BPlayerID playerID = cInvalidPlayerID);
      BEntityRef*             getContainingEntityRef();
      bool                    hasGarrisoned();
      bool                    hasGarrisonedEnemies(BPlayerID playerID);
      BEntityID               getHitchedUnit();
      BEntityID               getHitchedSquad();      
      BEntityID               getHitchedToUnit();
      BEntityID               getHitchedToSquad();
      bool                    hasHitchedSquad();
      bool                    getRepairCost(BCost& cost, float& combatValueCost) const;

      void                    chooseSmartTarget(BSimOrder* pOrder, BProtoAction* pProtoAction, BSimTarget& target);
      bool                    incrementSmartTargetReference(int8 type);
      bool                    decrementSmartTargetReference(int8 type);

      void                    fleeMap(BSquad* pSquad);

      // Attack time.
      DWORD                   getLastDamagedTime() const { return(mLastDamagedTime); }
      void                    updateLastDamagedTime();
      DWORD                   getLastAttackedTime() const { return(mLastAttackedTime); }
      void                    updateLastAttackedTime();
      // HP Bar stuff.
      void                    showHPBars(bool show);
      bool                    wasAttackedRecently() const;
      int                     getSelectionDecal() const { return mSelectionDecal; }
      void                    hideSelectionDecal();

      //Formation stuff.
      BFormationType          getFormationType() const;
      BVector                 getAveragePosition() const;
      BVector                 getFormationPosition( BEntityID unitID ) const;
      BVector                 getFormationPositionAtTarget( BEntityID unitID, BVector targetPos=cOriginVector);
      bool                    getDesiredChildLocation(BEntityID childID, BVector &position) const;
      bool                    getFutureDesiredChildLocation(BEntityID childID, float desiredTimeIntoFuture, BVector &position, float &realTimeNeeded);
      bool                    getFutureDesiredChildPath(BEntityID childID, float desiredTimeIntoFuture, BDynamicVectorArray &futurePositions, float &realTimeNeeded, float &realDistanceNeeded);
      bool                    getFuturePosition(float desiredTimeIntoFuture, bool ignoreBlocked, BVector &futurePosition, float &realTimeNeeded);
      float                   getDefaultMovementProjectionTime() const;
      virtual DWORD           getLastMoveTime() const                { return mLastMoveTime; }
      virtual void            setLastMoveTime(DWORD lastMoveTime)    { mLastMoveTime = lastMoveTime; }     

#ifdef _MOVE4
      bool                    getChildInterimTarget_4(BEntityID childID, BVector &position);
#endif

      void                    updateFormation();

      // used in sub-updating
      const BVector           getInterpolatedPosition() const;
      const BVector           getInterpolatedForward() const;
      const BVector           getInterpolatedUp() const;
      const BVector           getInterpolatedRight() const;
      inline void             getInterpolatedRotation(BMatrix &rot) const { rot.makeOrient(getInterpolatedForward(), getInterpolatedUp(), getInterpolatedRight()); }
      void                    resetSubUpdating();

      //Range.
      bool                    calculateRange(const BSimTarget* pTarget, float& range, BProtoAction** ppProtoAction, float* pMinRange) const; // ppProtoAction and pMinRange can be NULL
      bool                    calculateRange(BSimTarget* pTarget, float* pMinRange) const;            // pMinRange can be NULL
      virtual float           getPathingRange() const;
      float                   getAttackRange() const;
      float                   getLeashDistance() const;
      float                   getLeashDeadzone() const;
      DWORD                   getLeashRecallDelay() const;
      float                   getAggroDistance() const;
      //void                    getCombatDistances(float& attackRange, float& leashDistance) const;

      //Leash Position.
      BVector                 getLeashPosition() const { return (mLeashPosition); }
      void                    setLeashPosition(BVector v, bool setAnchor = true) { mLeashPosition=v; if(setAnchor) mAnchorPosition=v; }
      BVector                 getAnchorPosition() const { return (mAnchorPosition); }
      void                    setAnchorPosition(BVector v) { mAnchorPosition=v; }

      //Squad mode.
      //DCPTODO.
      long                    getAutoSquadMode() const { return (-1); }
      long                    getSquadMode() const;
      long                    getCurrentOrChangingMode() const;

      // KBSquads
      BKBSquadID              getKBSquadID(BTeamID teamID) const { return (teamID < cMaximumSupportedTeams ? mKBSquadIDs[teamID] : cInvalidKBSquadID); }
      void                    setKBSquadID(BKBSquadID id, BTeamID teamID) { if (teamID < cMaximumSupportedTeams) mKBSquadIDs[teamID] = id; }

      // Powers
      void                    grantPower();
      void                    revokePower();

      // Levels and XP
      float                   getXP() const { return mXP; }
      int                     getLevel() const { return mLevel; }
      int                     getVetLevel() const;
      int                     getTechPlusVetLevel() const;
      void                    addBankXP(float xp);
      void                    clearBankXP();
      void                    applyBankXP();
      void                    upgradeLevel(int level, bool doEffects);

      //Misc.
      void                    formUp();
      virtual void            settle();
      void                    noStopSettle();
      virtual void            notify(DWORD eventType, BEntityID senderID, DWORD data, DWORD data2);
      virtual bool            isIdle() const;
      bool                    canMerge();
      virtual float           getResourceAmount() const;
      int                     getLastCommandType() { return (mLastCommandType); }
      BEntityID               getLastAbilityAttackTargetID() { return mLastAbilityAttackTargetID; }
      void                    setLastAbilityAttackTargetID(BEntityID val) { mLastAbilityAttackTargetID=val; }
      int                     getChangingToSquadMode() const { return mChangingToSquadMode; }
      void                    setChangingToSquadMode(int squadMode) { mChangingToSquadMode=squadMode; }
      void                    setRecover(int type, float time, int abilityID);
      void                    setAbilityTimer(int type, float time, int abilityID);
      float                   getAbilityDuration() const { return mAbilityTime;}
      int                     getRecoverType() const { return mRecoverType; }
      float                   getRecoverTime() const { return mRecoverTime; }
      float                   getRecoverPercent() const;
      void                    activateEffect(int effectType, bool on);
      void                    setModeMovementModifier(float val) { mModeMovementModifier=val; }
      float                   getModeMovementModifier() const { return mModeMovementModifier; }
      void                    setAbilityMovementSpeedModifier(int modifierType, float speedModifier, bool reset);
      void                    setAbilityMovementSpeedModifier(float speedModifier, bool reset);
      void                    setAbilityDamageModifier(float damageModifier, bool reset);
      void                    setAbilityDamageTakenModifier(float damageTakenModifier, bool reset);
      void                    setAbilityAccuracyModifier(float accuracyModifier, bool reset);
      void                    setAbilityDodgeModifier(float dodgeModifier, bool reset);

      virtual int             getSelectType(BTeamID teamId) const;
      virtual int             getGotoType() const;

      void                    playBirthAnimation(int id);
      bool                    hasCompletedBirth() const; 

      bool                    addCryo(float cryoAmount, float* freezingThawTime = NULL, float* frozenThawTime = NULL);
      bool                    isFrozen() const { return mFlagCryoFrozen; }
      void                    forceCryoFrozenKill(BPlayer *pPowerPlayer, BEntityID killerID);

      void                    addDaze(float dazeDuration, float movementModifier, bool smartTarget);
      bool                    isDazed() const;
      void                    resetDaze(float dazeDuration);

      BEntityID               getLastAttackTargetID() const { return mLastAttackTargetID; }
      int                     getLastAttackPriority() const { return mLastAttackPriority; }
      void                    clearLastAttackTargetData();

      //E3.
      void                    setRandomTacticStates() const;

      //Debug.
      virtual void            debugRender();

      // Passive
      void                    setPassive() { mSquadAI.setMode(BSquadAI::cModePassive); }
      void                    setNormal() { mSquadAI.setMode(BSquadAI::cModeNormal); }

      //Flags.
      void                    clearFlags();
      virtual void            setFlagLockedDown(bool v);
      virtual void            setFlagInSniper(bool v);
      virtual void            setFlagSprinting(bool v);
      virtual void            setFlagRecovering(bool v);
      bool                    getFlagUsingTimedAbility() const { return mFlagUsingTimedAbility; }
      void                    setFlagUsingTimedAbility(bool v) { mFlagUsingTimedAbility = v; }
      bool                    getFlagProtoSquad() const { return(mFlagProtoSquad); }
      void                    setFlagProtoSquad(bool v) { mFlagProtoSquad=v; }
      bool                    getFlagAlive() const { return(mFlagAlive); }
      void                    setFlagAlive(bool v) { mFlagAlive=v; }
      bool                    getFlagHasShield() const { return(mFlagHasShield); }
      void                    setFlagHasShield(bool v) { mFlagHasShield=v; }
      bool                    getFlagShieldDamaged() const { return(mFlagShieldDamaged); }
      void                    setFlagShieldDamaged(bool v) { mFlagShieldDamaged=v; }
      bool                    getFlagChangingMode() const { return(mFlagChangingMode); }
      void                    setFlagChangingMode(bool v);
      bool                    getFlagDoNextCommand() const { return(mFlagDoNextCommand); }
      void                    setFlagDoNextCommand(bool v) { mFlagDoNextCommand=v; }
      bool                    getFlagPlayingBlockingAnimation() const { return(mFlagPlayingBlockingAnimation); }
      void                    setFlagPlayingBlockingAnimation(bool v) { mFlagPlayingBlockingAnimation=v; }
      bool                    getFlagDontPopCommand() const { return(mFlagDontPopCommand); }
      void                    setFlagDontPopCommand(bool v) { mFlagDontPopCommand=v; }
      bool                    getFlagHasHPBar() const { return(mFlagHasHPBar); }
      void                    setFlagHasHPBar(bool v) { mFlagHasHPBar=v; }
      bool                    getFlagForceDisplayHP() const;
      void                    setFlagForceDisplayHP(bool v);// { mFlagForceDisplayHP=v;}
      bool                    getFlagIgnorePop() const { return(mFlagIgnorePop); }
      void                    setFlagIgnorePop(bool v) { mFlagIgnorePop=v; }
      bool                    getFlagIgnoreLeash() const { return(mFlagIgnoreLeash); }
      void                    setFlagIgnoreLeash(bool v) { mFlagIgnoreLeash=v; }
      bool                    getFlagIsUngarrisoning() const { return (mFlagIsUngarrisoning); }
      void                    setFlagIsUngarrisoning(bool v) { mFlagIsUngarrisoning = v; }
      bool                    getFlagIsGarrisoning() const { return (mFlagIsGarrisoning); }
      void                    setFlagIsGarrisoning(bool v) { mFlagIsGarrisoning = v; }
      bool                    getFlagIsRepairing() const { return (mFlagIsRepairing); }
      void                    setFlagIsRepairing(bool v) { mFlagIsRepairing = v; }
      bool                    getFlagSquadExistSoundPlaying() const { return (mFlagSquadExistSoundPlaying); }
      void                    setFlagSquadExistSoundPlaying(bool v) { mFlagSquadExistSoundPlaying = v; }
      bool                    getFlagSquadChatter() const { return (mFlagSquadChatter); }
      void                    setFlagSquadChatter(bool v) { mFlagSquadChatter = v; }
      bool                    getFlagIsHitching() const { return (mFlagIsHitching); }
      void                    setFlagIsHitching(bool v) { mFlagIsHitching = v; }
      bool                    getFlagIsUnhitching() const { return (mFlagIsUnhitching); }
      void                    setFlagIsUnhitching(bool v) { mFlagIsUnhitching = v; }
      bool                    getFlagForceUpdateGarrisoned() const { return (mForceUpdateGarrisoned); }
      void                    setFlagForceUpdateGarrisoned(bool v) { mForceUpdateGarrisoned = v; }
      bool                    getFlagAttackBlocked() const { return (mFlagAttackBlocked); }
      void                    setFlagAttackBlocked(bool v) { mFlagAttackBlocked = v; }
      bool                    getFlagCloaked() const { return mFlagCloaked; }
      void                    setFlagCloaked(bool v) { mFlagCloaked = v; }
      bool                    getFlagCloakDetected() const { return mFlagCloakDetected; }
      void                    setFlagCloakDetected(bool v) { mFlagCloakDetected = v; }
      bool                    getFlagWantsToCloak() const { return mFlagWantsToCloak; }
      void                    setFlagWantsToCloak(bool v) { mFlagWantsToCloak = v; }
      bool                    getFlagUpdateGarrisonedSquadPositions() const { return mFlagUpdateGarrisonedSquadPositions; }
      void                    setFlagUpdateGarrisonedSquadPositions(bool v) { mFlagUpdateGarrisonedSquadPositions = v; }
      bool                    getFlagSpartanVeterancy() const { return mFlagSpartanVeterancy; }
      void                    setFlagSpartanVeterancy(bool v) { mFlagSpartanVeterancy = v; }
      bool                    getFlagSpartanContainer() const { return mFlagSpartanContainer; }
      void                    setFlagSpartanContainer(bool v) { mFlagSpartanContainer= v; }
      bool                    getFlagContainedSpartan() const { return mFlagContainedSpartan; }
      void                    setFlagContainedSpartan(bool v) { mFlagContainedSpartan = v; }
      bool                    getFlagDazeImmobilized() const { return mFlagDazeImmobilized; }
      void                    setFlagDazeImmobilized(bool v) { mFlagDazeImmobilized = v; }
      bool                    getFlagCryoFrozen() const { return mFlagCryoFrozen; }
      void                    setFlagCryoFrozen(bool v) { mFlagCryoFrozen = v; }
      void                    setFlagJumping(bool v) { mFlagJumping = v; }
      bool                    getFlagJumping() const { return mFlagJumping; }
      void                    setFlagMatchFacing(bool v) { mFlagMatchFacing = v; }
      bool                    getFlagMatchFacing() const { return (mFlagMatchFacing); }
      void                    setFlagIsTransporting(bool v) { mFlagIsTransporting = v; }
      bool                    getFlagIsTransporting() const { return (mFlagIsTransporting); }
      void                    setFlagNoPlatoonMerge(bool v) { mFlagNoPlatoonMerge = v; }
      bool                    getFlagNoPlatoonMerge() { return (mFlagNoPlatoonMerge); }
      void                    setFlagStopShieldRegen(bool v) { mFlagStopShieldRegen = v; }
      bool                    getFlagStopShieldRegen() { return (mFlagStopShieldRegen); }
      void                    setFlagIgnoreAI(bool v) { mFlagIgnoreAI=v; }
      bool                    getFlagIgnoreAI() const { return (mFlagIgnoreAI); }
      void                    setFlagPreventScoring(bool v) { mFlagPreventScoring=v; }
      bool                    getFlagPreventScoring() const { return (mFlagPreventScoring); }

      
      BActionID               doMove(BSimOrder* pOrder, BAction* pParentAction, bool platoonMove, bool monitorOpps = false, bool autoSquadMode = false, bool forceLeashUpdate = false);
      BActionID               doMove(BSimOrder* pOrder, BAction* pParentAction, const BSimTarget* pTarget, bool platoonMove, bool monitorOpps = false, bool autoSquadMode = false, bool forceLeashUpdate = false, BEntityID ignoreUnit = cInvalidObjectID);
      BActionID               doGarrison(BSimOrder* pOrder, BAction* pParentAction, BSimTarget* pTarget, bool ignoreRange, bool reverseHotDrop);
      BActionID               doUngarrison(BSimOrder* pOrder, BAction* pParentAction, BSimTarget* pTarget, BSimTarget* pSource = NULL, BVector rallyPoint = cInvalidVector, uint8 exitDirection = 0, BVector facing = cInvalidVector, BVector spawnPoint = cInvalidVector, bool allowMovingSquadsFromUngarrisonPoint = true, bool alertWhenComplete=false, bool ignoreSpawnPoint=false);
      BActionID               doAttack(BSimOrder* pOrder, BAction* pParentAction, BSimTarget* pTarget);
      BActionID               doCarpetBomb(BSimOrder* pOrder, BAction* pParentAction, BSimTarget* pTarget);
      //BActionID               doHitch(BSimOrder* pOrder, BAction* pParentAction, BSimTarget* pTarget);
      //BActionID               doUnhitch(BSimOrder* pOrder, BAction* pParentAction, BSimTarget* pTarget);
      BActionID               doChangeMode(BSimOrder* pOrder, BAction* pParentAction);

      // Repair/reinforce
      BActionID               doRepair(BSimOrder* pOrder);
      BActionID               doRepairOther(BSimOrder* pOrder);
      float                   getRepairPercent() const;

      // Stasis
      void                    addStasisEffect() { mNumStasisEffects++; }
      void                    releaseStasisEffect() { mNumStasisEffects--; }
      float                   getStasisSpeedMult() { return (mStasisSpeedMult); }
      void                    setStasisSpeedMult(float mult) { mStasisSpeedMult = mult; }
      int                     getNumStasisEffects() { return (mNumStasisEffects); }

      void                    repairHitpoints(float repairHP, bool bAllowReinforce, float& excessHP, float damageTakenScalarForNewUnits = 1.0f);
      void                    repairCombatValue(float repairCV, bool bAllowReinforce, float& excessCV, float damageTakenScalarForNewUnits = 1.0f);

      // Damage Bank
      void                    adjustDamageBank(float val);
      float                   getDamageBank() const { return mDamageBank; }
      void                    updateDamageBank(float elapsed);

      // Sound
      void                    startExistSound();
      void                    stopExistSound(bool force=false);

      void                    playSquadStateChatter(BSquadSoundState state, int32 targetProtoSquad=-1);
      bool                    playChatterSound(BSquadSoundType, int32 targetOrEnemyProtoSquadID=-1, bool checkSquadSize=true, BEntityID castingSquadID=cInvalidObjectID);
      void                    createAudioReaction(BSquadSoundType soundType);

      void                    updateMovementSound(float elapsedTime);
      void                    playMovementSound(bool startMove, bool force=false);

      //HACK.
      void                    whackUnitPositions(float elapsedTime);

      // Damage proxy (for bubble shields around squads and such)
      void                    setDamageProxy(BEntityID id) { mDamageProxy = id; }
      BEntityID               getDamageProxy() { return mDamageProxy; }
      bool                    hasDamageProxy() { return mDamageProxy != cInvalidObjectID; }

      // Merge stuff
      void                    setMergeType(long mergeType) { mMergedTypes |= mergeType; }
      bool                    hasMergeType(long mergeType) { return ((mMergedTypes & mergeType) !=0); }
      void                    clearMergeType(long mergeType) { mMergedTypes &= !mergeType; }
      void                    incrementMergeCount() { ++mMergeCount; }
      void                    decrementMergeCount() { --mMergeCount; }
      int                     getMergeCount() const { return mMergeCount; }

      BSquad*                 getGarrisonedSpartanSquad() const;
      BUnit*                  getSpartanContainerUnit() const;

      void                    drawSquadSelection(DWORD color, bool bRenderOneFrameOnly, float intensity, bool bIsHover, bool bUseStaticDecal);

      void                    setTargettingSelection(bool on);
      
      bool                    isUnderCinematicControl() const;

      virtual BPlayerID       getColorPlayerID() const;

      // for secondary ranged attack action perf
      void                    setSecondaryTurretScanTokens(bool v);

      // for delayed base destruction
      void                    determineKillerForDelayedBaseDestruction();

      // Timing Stats
      #ifndef BUILD_FINAL
      static BSquadStats      mStats;
      #endif

      #ifdef _MOVE4
      BActionID                  doMove_4(BSimOrder *pOrder, BAction* pParentAction, BSimTarget target, BVector interimTarget, bool platoonMove, bool monitorOpps /*= false*/, 
                                    bool autoSquadMode /*= false*/, bool forceLeashUpdate /*= false*/);
      bool                       hasCompletedMovement_4() const;
      void                       pauseMovement_4(DWORD pauseTime);
      bool                       isMovementPaused_4(DWORD *remainingTime=NULL) const;  // only true if actually paused in movement, not if stopped for other reasons/not moving at all.
      bool                       isFlyingSquad_4() const;    // returns true if the LeaderUnit in the squad is a flying unit
      const BSquadActionMove*    getNonPausedMoveAction_4() const;
      BSquadActionMove*          getNonPausedMoveAction_4();
      BSquadActionMove*          getMoveAction_4();

      int                        getMoveActionPath_4(BVector start, BVector end, long &findPathResult, BPath *tempPath = NULL);
      void                       setMoveActionPath_4(BPath &newPath);


         #ifdef NEW_TURNRADIUS
            bool                 getFlagUpdateTurnRadius() const { return mFlagUpdateTurnRadius; }
            void                 setFlagUpdateTurnRadius(bool v) { mFlagUpdateTurnRadius = v; }
            BVector              getTurnRadiusPos() const { return mTurnRadiusPos; }
            void                 setTurnRadiusPos(BVector pos) { mTurnRadiusPos = pos; }
            BVector              getTurnRadiusFwd() const { return mTurnRadiusFwd; }
            void                 setTurnRadiusFwd(BVector fwd) { mTurnRadiusFwd = fwd; }

            BActionState         getTurnRadiusState() const;
            BActionState         updateTurning(float elapsed);
            bool                 calcTurning(float elapsedTime, float distanceChange, float distanceRemainingToTarget, 
                                       BVector currentPos, BVector currentFwd, BVector& newPosition, BVector& newForward);
         #endif
      #endif

      GFDECLAREVERSION();
      virtual bool save(BStream* pStream, int saveType) const;
      virtual bool load(BStream* pStream, int saveType);
      virtual bool postLoad(int saveType);

   protected:
      void                    regenShield();
      void                    determineTarget(BSimOrder* pOrder, BSimTarget& target, bool attack = false);
      bool                    isOrderInterruptible(BSimOrderEntry* pOrderEntry);
      void                    adjustPlayerPop(bool add);
      void                    getCurrentSoundState(BSquadSoundState& soundState, long& protoID);
      void                    updateChatter(float elapsed);
      void                    updateRecover(float elapsed);     
      void                    updateAbilityDuration(float elapsed);
      void                    updateEffects();
      void                    removeEffects(bool bKillImmediately);

      //Do methods.
      bool                    doIdle();
      bool                    doAttack(BSimOrder* pOrder);
      bool                    doGather(BSimOrder* pOrder);
      bool                    doCapture(BSimOrder* pOrder);
      bool                    doJoin(BSimOrder* pOrder);
      bool                    doMines(BSimOrder* pOrder);
      bool                    doGarrison(BSimOrder* pOrder);
      bool                    doUngarrison(BSimOrder* pOrder);
      bool                    doHitch(BSimOrder* pOrder);
      bool                    doUnhitch(BSimOrder* pOrder);
      bool                    doUnpack(BSimOrder* pOrder);
      bool                    doTransport(BSimOrder* pOrder);
      bool                    doDetonate(BSimOrder* pOrder);
      bool                    doWander(BSimOrder* pOrder);
 	   bool                    doCloak(BSimOrder* pOrder);
      bool                    doJump(BSimOrder* pOrder, BJumpOrderType jumpType);
      bool                    doPointBlankAttack(BSimOrder* pOrder);


      // Reverse move
      bool                    getReverseMove();

      void                    refreshLeaderUnit();

      BUnit*                  mpCachedLeaderUnit;
      BSquadAI                mSquadAI;
      
      // needed for interpolation/sub-updating
      BVector                 mOldPosition;                    // 16 bytes
      BVector                 mOldRight;                       // 16 bytes
      BVector                 mOldUp;                          // 16 bytes
      BVector                 mOldForward;                     // 16 bytes

      BVector                 mLeashPosition;                  // 16 bytes
      BVector                 mAnchorPosition;                 // 16 bytes - Used by aircraft to allow them to drag their leashes for collision avoidance, but still get back to the leash pos

      // Turn radius stuff
      #ifdef NEW_TURNRADIUS
         BVector              mTurnRadiusPos;                  // 16 bytes
         BVector              mTurnRadiusFwd;                  // 16 bytes
      #endif

      BEntityIDArray          mChildren;                       // 8 bytes
      BSimOrderEntryArray     mOrders;                         // 8 bytes

      BEntityIDArray          mVisibleSquads;                  // 8 bytes
      BEntityIDArray          mVisibleEnemySquads;             // 8 bytes

      float                   mDamageBank;                     // 4 bytes
      float                   mDamageBankTimer;                // 4 bytes

      BFormation2*            mpFormation;                     // 4 bytes
      long                    mProtoID;                        // 4 bytes
      DWORD                   mSubUpdateNumber;                // 4 bytes
      DWORD                   mLastDamagedTime;                // 4 bytes
      DWORD                   mLastAttackedTime;               // 4 bytes
      DWORD                   mLastMoveTime;                   // 4 bytes            
      int                     mSelectionDecal;                 // 4 bytes
      BAIMissionID            mAIMissionID;                    // 4 bytes - What AI mission are we assigned to?  (Can only be one at a time.)      
      float                   mXP;                             // 4 bytes
      float                   mXPBank;                         // 4 bytes
      int                     mLevel;                          // 4 bytes
      int                     mLastCommandType;                // 4 bytes
      BEntityID               mLastAbilityAttackTargetID;      // 4 bytes
      int                     mChangingToSquadMode;            // 4 bytes
      float                   mRecoverTime;                    // 4 bytes
      int                     mRecoverType;                    // 4 bytes            
      float                   mMovementSoundTimer;             // 4 bytes
      float                   mModeMovementModifier;           // 4 bytes
      float                   mStasisSpeedMult;                // 4 bytes
      int8                    mNumStasisEffects;
	   float                   mAbilityTime;                    // 4 bytes
      BEntityID               mLastAttackTargetID;             // 4 bytes
      int8                    mLastAttackPriority;             // 1 byte
      int8                    mMergeCount;                      // 1 byte

      float                   mChatterTimer[cSquadSoundStateMax];
      int8                    mSmartTargetValues[cSmartTargetMax];

      long                    mLastKnownLeaderProtoObjectID;
      BPlayerID               mKillerPlayerID;
      long                    mKillerProtoID;
      long                    mKillerProtoSquadID;

      BKBSquadID              mKBSquadIDs[cMaximumSupportedTeams];

      BEntityID               mDamageProxy;

      DWORD                   mLastTrueLOSCheckCRC;

      long                    mMergedTypes;                    // 4 bytes

      bool                    mFlagProtoSquad:1;               // 1 byte   (1/8)
      bool                    mFlagAlive:1;                    //          (2/8)
      bool                    mFlagHasShield:1;                //          (3/8)
      bool                    mFlagShieldDamaged:1;            //          (4/8)
      bool                    mFlagChangingMode:1;             //          (5/8)
      bool                    mFlagDoNextCommand:1;            //          (6/8)
      bool                    mFlagPlayingBlockingAnimation:1; //          (7/8)
      bool                    mFlagDontPopCommand:1;           //          (8/8)

      bool                    mFlagHasHPBar:1;                 // 1 byte   (1/8)
      bool                    mFlagForceDisplayHP:1;           //          (2/8)
      bool                    mFlagIgnorePop:1;                //          (3/8)
      bool                    mFlagIgnoreLeash:1;              //          (4/8)
      bool                    mFlagVisibleLastUpdate:1;        //          (5/8)
      bool                    mFlagIsUngarrisoning:1;          //          (6/8)
      bool                    mFlagIsGarrisoning:1;            //          (7/8)
      bool                    mFlagIsRepairing:1;              //          (8/8)
      
      bool                    mFlagSquadExistSoundPlaying:1;   // 1 byte   (1/8)        
      bool                    mFlagSquadChatter:1;             //          (2/8)
      bool                    mFlagIsHitching:1;               //          (3/8)
      bool                    mFlagIsUnhitching:1;             //          (4/8)
      bool                    mReverseMoveDone:1;              //          (5/8)
      bool                    mForceUpdateGarrisoned:1;        //          (6/8)
      bool                    mMovementSoundOn:1;              //          (7/8)
      bool                    mFlagHasActiveEffect:1;          //          (8/8)

      bool                    mKilledByRecycler:1;             // 1 byte   (1/8)
      bool                    mFlagAttackBlocked:1;            //          (2/8)
      bool                    mFlagUsingTimedAbility:1;        //          (3/8)
      bool                    mFlagCloaked:1;                  //          (4/8)
      bool                    mFlagCloakDetected:1;            //          (5/8)
      bool                    mFlagWantsToCloak:1;             //          (6/8)      
      bool                    mFlagUpdateGarrisonedSquadPositions:1;//     (7/8)
      bool                    mFlagSpartanVeterancy:1;         //          (8/8)

      bool                    mFlagSpartanContainer:1;         // 1 byte   (1/8)
      bool                    mFlagContainedSpartan:1;         //          (2/8)
      bool                    mFlagDazeImmobilized:1;          //          (3/8)
      bool                    mFlagCryoFrozen:1;               //          (4/8)
      bool                    mFlagHasTrueLOS:1;               //          (5/8)
      bool                    mFlagUpdateTurnRadius:1;         //          (6/8)
      bool                    mFlagJumping:1;                  //          (7/8)
      bool                    mFlagIsTransporting:1;           //          (8/8)
      
      bool                    mFlagMatchFacing:1;              //          (1/8)
      bool                    mFlagTargettingSelectionOn:1;    //          (2/8)
      bool                    mFlagNoPlatoonMerge:1;           //          (3/8)
      bool                    mFlagStopShieldRegen:1;          //          (4/8)
      bool                    mFlagIgnoreAI:1;                 //          (5/8)
      bool                    mFlagPreventScoring:1;           //          (6/8)  // [11/5/2008 xemu] added flag to make trigger-kills not scored 

};

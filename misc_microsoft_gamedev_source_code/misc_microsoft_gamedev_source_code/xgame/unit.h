//==============================================================================
// unit.h
// Copyright (c) 2005 Ensemble Studios
//==============================================================================

#pragma once

// Includes
#include "aitypes.h"
#include "object.h"
#include "hitzone.h"
#include "UnitOpportunity.h"
#include "rallypoint.h"

//Forward Declarations
class BProtoAction;
class BHitZone;
class BDamageTracker;
class BDamageImpactPoint;
class BPhysicsInfo;
class BPhysicsCollisionListener;
class IDamageInfo;
class BObjectCreateParms;
class BUnitActionSecondaryTurretAttack;
class BWeapon;

//==============================================================================
// Debugging
#ifndef BUILD_FINAL
extern BEntityID sDebugUnitTempID;
void dbgUnitInternal(const char *format, ...);
void dbgUnitInternalTempID(const char *format, ...);

#define debugUnit dbgUnitInternal
#else
#define debugUnit __noop
#endif

//==============================================================================
//==============================================================================
class BUnitTransformData
{
   public:
      BUnitTransformData() : 
         mOpps(),
         mHpRatio(1.0f), 
         mResourceRatio(1.0f),
         mAmmoRatio(1.0f),
         mUpdateLOS(false)
      {
      }
      BUnitOppList   mOpps;
      float          mHpRatio;
      float          mResourceRatio;
      float          mAmmoRatio;
      bool           mUpdateLOS;
};

//==============================================================================
//==============================================================================
class BDamage 
{
public:

   BDamage( void )
   {
      mDamagePos = cOriginVector;
      mDirection = cOriginVector;
      mpDamageInfo = NULL;
      mAttackerID.invalidate();
      mAttackerTeamID = -1;
      mDamage = 0.0f;
      mDOTrate = 0.0f;
      mDOTduration = 0.0f;
      mDamageMultiplier = 0.0f;
      mShieldDamageMultiplier = 0.0f;
      mDamageDealt = 0.0f;
      mShieldDamageDealt = 0.0f;
      mDistanceFactor = 1.0f;
      mHalfKillCutoffFactor = 0.0f;
      mHitZoneIndex = -1;
      mKilled = false;
      mDirectional = false;
      mForceThrow = false;
      mHalfKill = false;
      mOverrideRevive = false;
      mWeaponType = -1;
      mDOTEffect = -1;
      mIsDOTDamage = false;
      mPhysReplacementMatchBone = -1;
   }

   ~BDamage( void )
   {

   }

   // TOTAL:                                 // 97 bytes
   BVector        mDamagePos;                // 16 bytes
   BVector        mDirection;                // 16 bytes
   IDamageInfo*   mpDamageInfo;              // 4 bytes
   BEntityID      mAttackerID;               // 4 bytes
   BTeamID        mAttackerTeamID;           // 4 bytes
   float          mDamage;                   // 4 bytes
   float          mDamageMultiplier;         // 4 bytes
   float          mDOTrate;                  // 4 bytes
   float          mDOTduration;              // 4 bytes
   float          mShieldDamageMultiplier;   // 4 bytes
   float          mDamageDealt;              // 4 bytes
   float          mShieldDamageDealt;        // 4 bytes
   float          mDistanceFactor;           // 4 bytes
   float          mHalfKillCutoffFactor;     // 4 bytes
   long           mHitZoneIndex;             // 4 bytes
   // [4/23/2008 xemu] this really seems like it could be a much smaller type, but it's stored as longs lots of places already so 
   //     I'm maintaining consistency.
   long           mWeaponType;               // 4 bytes
   long           mDOTEffect;                // 4 bytes
   long           mPhysReplacementMatchBone; // 4 bytes

   bool           mKilled        : 1;        // 1 byte   (1/8)
   bool           mDirectional   : 1;        //          (2/8)
   bool           mForceThrow    : 1;        //          (3/8)
   bool           mHalfKill      : 1;        //          (4/8)
   bool           mOverrideRevive: 1;        //          (5/8)
   bool           mIsDOTDamage   : 1;        //          (6/8)
};

//==============================================================================
//==============================================================================
class BAttackTimer
{
public:
   BAttackTimer() : mTimer(0.0f), mPreAttackTimer(0.0f), mPreAttackTimerSet(false) {}
   float          mTimer;
   float          mPreAttackTimer;
   bool           mPreAttackTimerSet;
};


//==============================================================================
//==============================================================================
class BUnit : public BObject
{
   public:     
      enum
      {
         cDoOppInProgress=0,
         cDoOppComplete,
         cDoOppFail,
      };

      //-- don't do anything in the constructor of these (it only gets called one time for the whole game)
      BUnit() {};

      //-- this needs free everything
      virtual ~BUnit() { mActions.clearActions(); };

      virtual void            reset( void ) {};
      virtual void            onRelease();
      virtual void            init( void );
      virtual bool            initFromProtoObject(const BProtoObject* pProto, BObjectCreateParms& parms);

      virtual void            createObstruction(bool playerOwnsObstruction);

      void                    determineKiller(BPlayerID& killerPlayerId, long& killerProtoID, long& killerProtoSquadID, bool& killedByRecycler);

      virtual void            kill(bool bKillImmediately);
      void                    killHero();
      virtual void            destroy(void);

      virtual void            notify(DWORD eventType, BEntityID senderID, DWORD data, DWORD data2);

      void                    safeDesquad();

      virtual bool            updatePreAsync(float elapsedTime);
      virtual bool            update(float elapsedTime);      
      virtual void            render();
      void                    updateOpps(float elapsedTime);
      void                    createOpps();

      bool                    doIdle(void);
      uint                    doMove(BUnitOpp* pOpp);
      uint                    doAttack(BUnitOpp* pOpp);
      uint                    doCapture(BUnitOpp* pOpp);
      uint                    doJoin(BUnitOpp* pOpp);
      uint                    doMines(BUnitOpp* pOpp);
      uint                    doChangeMode(BUnitOpp* pOpp);
      uint                    doGarrison(BUnitOpp* pOpp);
      uint                    doUngarrison(BUnitOpp* pOpp);
      uint                    doHitch(BUnitOpp* pOpp);
      uint                    doUnhitch(BUnitOpp* pOpp);
      uint                    doAnimation(BUnitOpp* pOpp);
      uint                    doGather(BUnitOpp* pOpp);
      uint                    doDeath(BUnitOpp* pOpp);
      uint                    doHeroDeath(BUnitOpp* pOpp);
      uint                    doInfectDeath(BUnitOpp* pOpp);
      uint                    doThrown(BUnitOpp* pOpp);
      uint                    doDetonate(BUnitOpp* pOpp);
      uint                    doJump(BUnitOpp* pOpp, BAction* pParentAction = NULL);
      uint                    doPointBlankAttack(BUnitOpp* pOpp);

      //Move action type.
      BActionType             getMoveActionType() const;
      //TacticState.
      uint                    getNumberTacticStates() const;
      int                     getTacticState() const { return (mTacticState); }
      void                    setTacticState(uint8 v);
      void                    setRandomTacticState();
      void                    clearTacticState();

      virtual long            getIdleAnim() const;
      virtual long            getWalkAnim() const;
      virtual long            getJogAnim() const;
      virtual long            getRunAnim() const;
      virtual long            getDeathAnim() const;

      //Opps.
      uint                    getNumberOpps() const { return (mOpps.getSize()); }
      const BUnitOpp*         getOppByIndex(uint index) const { if (index >= mOpps.getSize()) return (NULL); return (mOpps[index]); }
      const BUnitOpp*         getOppByID(BUnitOppID oppID) const;
      const BUnitOpp*         getOppByTypeAndTarget(BUnitOppType type,const  BSimTarget* target);
      virtual uint8           getOppPriority(BUnitOppID oppID) const;
      
      bool                    addOpp(BUnitOpp* opp);
      bool                    removeOpp(BUnitOppID oppID, bool removeActions=true);
      void                    removeOpps(bool notifySources=true);
      void                    removeActionsForOpp(BUnitOpp* opp);

      bool                    completeOpp(BUnitOppID oppID, bool success);
      bool                    completeOppForReal(BUnitOpp* pOpp);
      bool                    determineTarget(BUnitOpp* opp, BSimTarget& target);
   
      //Controllers.
      uint                    getNumberControllers() const { return (BActionController::cNumberControllers); }
      bool                    grabController(uint index, BAction *pAction, BUnitOppID oppID);
      bool                    releaseController(uint index, BAction *pAction);
      void                    clearController(uint index) { BASSERT(index < BActionController::cNumberControllers); mControllers[index].init(); }
      void                    updateControllerOppIDs(BUnitOppID oldID, BUnitOppID newID);
      const BActionController* getController(uint index) const { BASSERT(index < BActionController::cNumberControllers); return (&(mControllers[index])); }
      bool                    isControllerFree(uint index) const { BASSERT(index < BActionController::cNumberControllers); return (mControllers[index].getActionID() == cInvalidActionID); }
      bool                    isControllerFree(uint index, BUnitOppID oppID) const;

      bool                    beginPlayBlockingAnimation(long state, long type, bool applyInstantly, bool useMaxHeight, bool forceReset = false, long forceAnimID = -1, BProtoVisualAnimExitAction* pOverrideExitAction = NULL, bool loop = false, bool disableMotionExtractionCollisions = false, bool clearUnitNoRenderFlag = false);
      bool                    endPlayBlockingAnimation(void);

      long                    getTrainLimit(BPlayerID playerID, long id, bool squad) const;

      void                    doSelfDestruct(bool cancel);
      bool                    isSelfDestructing(float& timeRemaining) const;

      bool                    getRecharging(bool squad, int protoID, float* pTimeRemaining) const; 
      void                    setRecharge(bool squad, int protoID, float timeRemaining);

      void                    onBuilt(bool fromChangeOwner, bool physicsReplacement, bool bStartExistSound, bool fromSave);
      void                    onKillOrTransform(long transformToProtoID);

      bool                    doTrain(BPlayerID playerID, long id, long count, bool squad, bool noCost, BTriggerScriptID triggerScriptID=cInvalidTriggerScriptID, BTriggerVarID triggerVarID=cInvalidTriggerVarID, bool forcePlaySound=false);
      long                    getTrainCount(BPlayerID playerID, long id) const;
      void                    getTrainQueue(BPlayerID playerID, long*  pCount, float*  pTotalPoints) const;


      bool                    doResearch(BPlayerID playerID, long techID, long count, bool noCost=false, BTriggerScriptID triggerScriptID=cInvalidTriggerScriptID, BTriggerVarID triggerVarID=cInvalidTriggerVarID);
      long                    getResearchCount(BPlayerID playerID, long id) const;

      bool                    doBuild(BPlayerID playerID, bool cancel, bool noCost, bool fromSave);
      float                   getBuildPercent() const;
      void                    addBuildPoints(float points);

      bool                    doBuildOther(BPlayerID playerID, BPlayerID purchasingPlayerID, int id, bool cancel, bool noCost, bool doppleOnStart = false, BTriggerScriptID triggerScriptID=cInvalidTriggerScriptID, BTriggerVarID triggerVarID=cInvalidTriggerVarID);
      void                    forceDoppleAllTeams();   

      bool                    doCustomCommand(BPlayerID playerID, int commandID, bool cancel);
      int                     getCustomCommandCount(BPlayerID playerID, int commandID) const;

      void                    prePositionChanged(BVector& hardpointPos, int& hardpointID);
      void                    postPositionChanged(BVector oldHardpointPos, int hardpoinID);

      //bool                    doUnload(void);
      //bool                    doUnattach(bool offset = true);

      bool                    doDodge(BEntityID dodgeeID, BVector trajectory);
      bool                    isDodging(BEntityID dodgeeID = cInvalidObjectID);
      bool                    canDodge() const;

      bool                    doDeflect(BEntityID deflecteeID, BVector trajectory, float damage);
      bool                    isDeflecting(BEntityID deflecteeID = cInvalidObjectID);
      bool                    canDeflect() const;
      bool                    canDeflectSmallArms() const;
      void                    resetDeflectID();

      void                    setTrainLock(BPlayerID playerID, bool locked);
      bool                    getTrainLock(BPlayerID playerID) const;
      void                    queueTrainedSquad(BSquad* pSquad, bool playSound);

      virtual void            setParentID(BEntityID id) { BObject::setParentID(id); refreshParentSquad(); }
      BSquad*                 getParentSquad(void) const;

      BEntityID               getBuiltByUnitID(void) const;

      bool                    isType(long type) const;

      float                   getAttackRating(BDamageTypeID damageType) const;
      float                   getDefenseRating() const;
      uint                    getAttackGrade(BDamageTypeID damageType) const;

      //-- hitpoints
      float                   getHitpoints( void ) const       { return mHitpoints; }
      void                    setHitpoints( float hp )
      {
#if defined(SYNC_Unit)
         syncUnitData("BUnit::setHitpoints hp", hp);
#endif
         mHitpoints = hp;
      }
      void                    adjustHitpoints(float deltaHP)
      {
#if defined(SYNC_Unit)
         syncUnitData("BUnit::adjustHitpoints deltaHP", deltaHP);
#endif
         mHitpoints += deltaHP;
      }
      //-- shieldpoints
      float                   getShieldpoints( void ) const       { return mShieldpoints; }
      void                    setShieldpoints( float sp )         { mShieldpoints = sp;   }
      void                    adjustShieldpoints(float deltaSP)   { mShieldpoints += deltaSP; }

      //-- shield regen
      float                   getShieldRegenRate() const          { return mShieldRegenRate; }
      void                    setShieldRegenRate(float v)         { mShieldRegenRate = v; }

      float                   getShieldRegenDelay() const         { return mShieldRegenDelay; }
      void                    setShieldRegenDelay(float val)      { mShieldRegenDelay=val; }

      // Damage
      virtual float           damage(BDamage &dmg);
      void                    createImpactEffect(BDamage &dmg);
      void                    damageImpactPoint(long impactIndex, float force, const BVector* pOverrideForceDir, float damage, bool bDeath = false, BEntityID* pOutEntityID = NULL);
      void                    calculateDamageForce(BDamage &dmg, float *pForce, BVector *pForceDir);
      void                    shatterImpactPoint(const BDamageImpactPoint *pRef, bool bVisible, 
                                                   const BVector *pModelSpacePoint = NULL, float force =0.0f, const BVector* pOverrideForceDir = NULL,
                                                   BPhysicsCollisionListener *pListener = NULL , bool forceCompleteDestruction =false, 
                                                   bool ballisticHit =true, 
                                                   BEntityID* pOutEntityID = NULL);
      BEntityID               createAndThrow(const BVector &impulse, 
                                               const BVector &impactpoint,
                                               const BPhysicsObjectParams &physicsparams,
                                               const BBitArray& mask, 
                                               float lifespan = -1.0f);
      void                    applyDamageOverTimeEffect(BDamage &dmg);
      BEntityID               forceThrowPartImpactPoint(const BVector& impactPosWorldSpace);
      BEntityID               throwRandomPartImpactPoint();

#if DPS_TRACKER
      void                    updateDamageHistory();
      float                   getTrackedDPS();
#endif

      float                   getDamageModifier() const { return mDamageModifier; }
      void                    adjustDamageModifier(float val) { mDamageModifier *= val; }
      void                    setDamageModifier(float val) { mDamageModifier = val; }

      int                     getDamageTypeMode() const;
      const BDamageTracker*   getDamageTracker() const { return mDamageTracker; }
      void                    addThrownPart(const BEntityID& newPart);

      // DPS ramp tracker
      float                   getDPSRampValue() const;
      void                    setDPSRampValue(float val);
      
#ifndef BUILD_FINAL
      void                    reInitDamageTracker();
#endif

      // Velocity
      virtual float           getDesiredVelocity() const { return (BObject::getDesiredVelocity() * mVelocityScalar); }
      virtual float           getMaxVelocity() const { return (BObject::getMaxVelocity() * mVelocityScalar); }
      void                    adjustVelocityScalar(float adjust);
      void                    setVelocityScalar(float scalar);
      inline float            getVelocityScalar() const { return mVelocityScalar; }
      virtual bool            canMove(bool allowAutoUnlock=false) const;

      virtual float           getAcceleration() const { return (BObject::getAcceleration() * mVelocityScalar); }
      virtual float           getReverseSpeed() const { return (BObject::getReverseSpeed() * mVelocityScalar); }

      // Accuracy
      inline float            getAccuracyScalar() const { return (mAccuracyScalar); }
      inline void             setAccuracyScalar(float scalar){ mAccuracyScalar = scalar; }
      inline void             adjustAccuracyScalar(float adjust){ mAccuracyScalar *= adjust; }

      // Work rate
      inline float            getWorkRateScalar() const { return (mWorkRateScalar); }
      inline void             setWorkRateScalar(float scalar){ mWorkRateScalar = scalar; }
      inline void             adjustWorkRateScalar(float adjust){ mWorkRateScalar *= adjust; }

      // Weapon range
      inline float            getWeaponRangeScalar() const { return (mWeaponRangeScalar); }
      inline void             setWeaponRangeScalar(float scalar){ mWeaponRangeScalar = scalar; }
      inline void             adjustWeaponRangeScalar(float adjust){ mWeaponRangeScalar *= adjust; }

      // Damage taken
      inline float            getDamageTakenScalar() const { return (mDamageTakenScalar); }
      inline void             setDamageTakenScalar(float scalar){ mDamageTakenScalar = scalar; }
      inline void             adjustDamageTakenScalar(float adjust){ mDamageTakenScalar *= adjust; }

      // Dodge
      inline float            getDodgeScalar() const { return (mDodgeScalar); }
      inline void             setDodgeScalar(float scalar) { mDodgeScalar = scalar; }
      inline void             adjustDodgeScalar(float adjust) { mDodgeScalar *= adjust; }

      // State
      virtual bool            canJump() const;
      virtual bool            isAlive(void) const { return (mFlagAlive); }
      virtual bool            isDamaged(void) const;
      bool                    isBuilding(void) const { return (false); }
      bool                    isCapturing() const;
      bool                    isGathering() const;
      bool                    isWorking() const;
      bool                    isVisibleOnScreen() const;      
      bool                    isHibernating() const { return getFlagIsHibernating(); }

      // Rally point
      bool                    haveRallyPoint(BPlayerID plyr = -1) const;
      BVector                 getRallyPoint(BPlayerID plyr = -1) const;
      void                    getRallyPoint(BVector& rallyPoint, BEntityID& rallyPointEntityID, BPlayerID plyr = -1);
      void                    calculateDefaultRallyPoint(void);
      void                    setRallyPoint(BVector point, BEntityID entityID, BPlayerID plyr = -1);
      void                    clearRallyPoint(BPlayerID plyr = -1);
      void                    setRallyPointVisible(bool val, BPlayerID plyr = -1);

      // Auto squad mode
      //long                    getAutoSquadMode() const;

      //-- resource amount
      void                    addGatherer() { mGatherers++; }
      void                    removeGatherer() { mGatherers--; BASSERT(mGatherers>=0); }
      int                     getNumberGatherers() const { return mGatherers; }
      virtual float           getResourceAmount() const { return mResourceAmount; }
      void                    setResourceAmount(float amount) { mResourceAmount=amount; }
      void                    adjustResourceAmount(float amount) { mResourceAmount+=amount; }

      void                    toggleAddResource(bool enable);

      void                    transform(long newProtoObjectID, bool techUpgrade=false);
      void                    transform(const BProtoObject* pOldProto, const BProtoObject* pNewProto, bool techUpgrade=false);
      void                    preTransform(const BProtoObject* pOldProto, const BProtoObject* pNewProto, bool techUpgrade, BUnitTransformData& transformData);
      void                    postTransform(const BProtoObject* pModifiedProto, bool techUpgrade, BUnitTransformData& transformData, BProtoObjectID oldProtoID=-1);
      virtual void            changeOwner(BPlayerID newPlayerID, BProtoObjectID newPOID = cInvalidProtoObjectID);

      //-- flood infection
      void                    takeInfectionForm();
      void                    infect(BPlayerID attackerPlayerID);
      bool                    isInfectable();

      void                    setGoalVector(const BVector vec)             { mGoalVector = vec; setFlagHasGoalVector(true); }
      void                    clearGoalVector()                            { mGoalVector = cOriginVector; setFlagHasGoalVector(false); setFlagHasFacingCommand(false); }
      bool                    getHasGoalVector() const                     { return getFlagHasGoalVector(); }
      BVector                 getGoalVector()                              { return mGoalVector; }
      //void                    setPosHistory(int index, BVector& pos)       { mPosHistory[index] = pos; }
      //BVector                 getPosHistory(int index)                     { return mPosHistory[index]; }
      void                    setHasFacingCommand(bool which)              { setFlagHasFacingCommand(which); }
      bool                    getHasFacingCommand() const                  { return getFlagHasFacingCommand(); }

      float                   getMaxHPContained(void) const                { return mMaxHPContained; }
      void                    clearMaxHPContained(void)                    { mMaxHPContained = 0.0f; }
      void                    addToMaxHPContained(BUnit *pUnit);
      void                    removeFromMaxHPContained(BUnit *pUnit);

      float                   getHPMax( void ) const;
      float                   getSPMax( void ) const;
      float                   getAmmoMax( void ) const;
      float                   getHPPercentage( void ) const;
      float                   getSPPercentage( void ) const;
      float                   getAmmoPercentage( void ) const;

      bool                    isPositionVisible(BTeamID teamID) const;

      void                    addReveal(BTeamID teamID);
      void                    removeReveal(BTeamID teamID);

      float                   getAmmunition(void) const {return mAmmunition;}
      void                    setAmmunition(float ammo) {mAmmunition = ammo;}
      void                    adjustAmmunition(float delta);

      void                    setAttackWaitTimer(int index, float val);
      float                   getAttackWaitTimer(int index) const;
      void                    setPreAttackWaitTimer(int index, float val);
      float                   getPreAttackWaitTimer(int index) const;
      bool                    getPreAttackWaitTimerSet(int index) const;
      void                    setPreAttackWaitTimerSet(int index, bool val);
      bool                    attackWaitTimerOn(int index);


      void                    adjustCapturePoints(float points, float elapsed);
      void                    setCapturePoints(float points, float elapsed);
      float                   getCapturePoints() const { return mCapturePoints; }
      float                   getCapturePercent() const;

      virtual BPlayerID       getCapturePlayerID() const { return mCapturePlayerID; }
      void                    setCapturePlayerID(BPlayerID playerID) { mCapturePlayerID=playerID; }

      void                    containUnit(BEntityID id, bool noChangeOwner=false);
      void                    unloadUnit(BEntityID id, bool death);
      void                    unloadUnit(int index, bool death);
      bool                    canContain(const BSquad* pSquad, float testContainedPop = 0.0f) const;
      bool                    canContain(const BUnit* pUnit) const;
      bool                    isReadyToContain() const;
      bool                    doesContain(const BSquad* pSquad) const;
      bool                    doesContain(const BUnit* pUnit) const;
      
      float                   getSquadPop() const;
      void                    calculateContainedPop();

      void                    attachObject(BEntityID id, long toBoneHandle = -1, long fromBoneHandle = -1, bool useOffset = false);
      void                    unattachObject(BEntityID id);

      void                    hitchUnit(BEntityID id);
      void                    unhitchUnit(BEntityID id);
      BEntityID               getHitchedUnit();

      //void                    syncCheckVisual();

      long                    getBattleID(void) const {return mBattleID;}
      void                    setBattleID(long id) {mBattleID = id;}
      BEntityID               getAttackTargetID() const;

      BEntityID               getCarriedObjectID() const { return mCarriedObject; }
      bool                    carryObject(BEntityID id = cInvalidObjectID, long fromBoneHandle = -1, const BVector* const worldSpaceOffset = NULL);
      void                    destroyCarriedObject();

	  //-- selection decal
      virtual int             getSelectionDecalHandle() const { return mSelectionDecal; };

      virtual int             getSelectType(BTeamID teamId) const;
      virtual int             getGotoType() const;

      // Hit zones
      inline BHitZoneArray*   getHitZoneList(){ return( mpHitZoneList ); }
      inline const BHitZoneArray* getHitZoneList() const { return( mpHitZoneList ); }
      float                   getMaxHitZoneHitpoints( long index ) const;
      float                   getMaxHitZoneShieldpoints( long index ) const;
      bool                    getHitZonePosition( long index, BVector& position ) const;
      bool                    getHitZoneOBB( long index, BBoundingBox& obb ) const;
      bool                    getTargetedHitZone( long& index ) const;
      void                    clearTargetedHitZone();
      bool                    getHitZoneIndex( BSimString name, long& index ) const;
      float                   getHitZoneOffset( long index ) const;
      void                    setTargetedHitZone( long index, bool targeted );

      void                    attackActionStarted(BEntityID targetID);
      void                    attackActionEnded(void);

      bool                    playUISound(BSoundType soundType, bool suppressBankLoad=false, long squadID=cInvalidProtoID, long actionID=-1) const;
      void                    playMeleeAttackSound(const BUnit* pTargetUnit, bool ramming=false, float velocity=0.0f);
      void                    playShieldSound(BSoundType soundEnum); 

      // External Shield
      bool                    isExternalShield() const;

      // Ignore user input
      inline bool             getIgnoreUserInput() const { return (getFlagIgnoreUserInput()); }
      inline void             setIgnoreUserInput(bool ignore){ setFlagIgnoreUserInput(ignore); }

      virtual DWORD           getLastMoveTime() const                { return mLastMoveTime; }
      virtual void            setLastMoveTime(DWORD lastMoveTime)    { mLastMoveTime = lastMoveTime; }      

      void                    resetLeashTimer() { mLeashTimer = 0; }

      BObject*                createAndThrowPhysicsReplacement(IDamageInfo* pDamageInfo, const BVector damagePos, float distanceFactor, bool rocketOnDeath, float forceScaler, BEntityID attackerID);
      bool                    hasThrowableAttachments();
      void                    throwAttachments(BObject* pObj, BVector impulse, float pctChanceThrow = 0.5, BEntityIDArray* pOutIds = NULL, int numAttachmentsToThrow = -1);
      BEntityID               createAndThrowAttachment(BVisualItem* pAttachment, BVector impulse, const BPhysicsObjectParams &physicsparams, BPlayerID playerID, const BMatrix& unitWorldMat, BVector centerOffset, float lifespan = -1.0f);
      BEntityID               createAndThrowAttachment(BVisualItem* pAttachment, BVector impulse, const BPhysicsInfo * pPhysicsInfo, const BPhysicsObjectBlueprintOverrides* pBPOverrides, BPlayerID playerID, const BMatrix& unitWorldMat, float lifespan = -1.0f);
      BEntityID               createAndThrowAttachment(BVisualItem* pAttachment, BVector impulse, BPlayerID playerID, long partID, const BMatrix& unitWorldMatrix, long collisionFilter, float lifespan = -1.0f);
      BObject*                killAndThrowPhysicsReplacement(BVector dir, BVector pos, long matchBoneHandle);

      BSimCollisionResult     checkForCollisions(const BEntityIDArray& ignoreUnits, BVector pos1, BVector pos2, DWORD lastPathTime,
                                 bool collideWithUnits, BEntityIDArray& collideUnits, BVector* intersectionPoint = NULL, bool checkInfantryOnly = false, bool checkMovingUnits = false, bool checkSquads = false, bool checkBuildings = false);
      virtual float           getPathingRange() const;

      // Track stuff that is attacking us.
      void                    addAttackingUnit(BEntityID attackingUnitID, BActionID attackingActionID);
      void                    removeAttackingUnit(BEntityID attackingUnitID, BActionID attackingActionID);
      bool                    isBeingAttackedByUnit(BEntityID unitID) const;
      bool                    isBeingAttacked() const;
      uint                    getNumberAttackingUnits() const;
      BEntityID               getAttackingUnitByIndex(uint index) const;
      uint                    getAttackingUnits(BEntityIDArray& attackingUnits) const;

      BUnitActionSecondaryTurretAttack*   getSecondaryTurretAttack(long hardpointID) const;     

#if 0 // SLB: Tim doesn't want to units to reveal eachother because of accidental damage
      // Track stuff that is damaging us.
      void                    damageBy(BEntityID attackingUnitID, BTeamID attackingTeamID);
      bool                    wasDamagedBy(BEntityID attackingUnitID, BTeamID attackingTeamID) const;
#endif

      // Cover
      int                     getNumCoverUnits() const;
      bool                    hasAvailableCover();
      BVector                 getNextAvailableCoverPoint();      
      BEntityIDArray          getCoverUnits(BPlayerID thisPlayerOnly = cInvalidPlayerID);

      //E3: Visual Ammo.
      uint                    getNumberVisualAmmos() const { return (mVisualAmmo.getSize()); }
      void                    initVisualAmmo();
      bool                    hasVisualAmmo(uint index) const;
      uint                    getVisualAmmo(uint index) const;
      void                    incrementVisualAmmo(uint index, uint v);
      void                    decrementVisualAmmo(uint index, uint v);
      void                    setVisualAmmo(uint index, uint v);
      void                    resetVisualAmmo(uint index);

      // Enum to assign ID to flag for trigger
      enum
      {
         cFlagAttackBlocked =0,
      };
      
      // Multi targets
      BEntityIDArray*         getMultiTargets() { return (&mMultiTargets); }
      void                    setMultiTargets(BEntityIDArray* multiTargets) { mMultiTargets = *multiTargets; }
      void                    clearInvalidMultiTargets(BUnit* pTargetUnit);

      // Group range calc for turrets
      BOPQuadHull*            getGroupHull() { return (mpGroupHull); }
      float                   getGroupDiagSpan() const { return (mGroupDiagSpan); }

      void                    recomputeVisual();

      // Garrisoned
      BEntityIDArray          getGarrisonedUnits(BPlayerID thisPlayerOnly = cInvalidPlayerID);
      bool                    hasGarrisonedEnemies(BPlayerID playerID, bool ignoreVisibility = false) const;
      BTeamID                 getGarrisonedTeam();

      bool                    setPostAnim(BActionID requesterActionID, const BProtoAction* pProtoAction, bool applyInstantly=false);
      bool                    setAnimation(BActionID requesterActionID, long state, long animType=-1, bool applyInstantly=false, bool reset=false, long forceAnimID=-1, bool lock=false, bool failOnCollision=false, const BProtoVisualAnimExitAction* pOverrideExitAction=NULL);
      bool                    isAnimationSet(long state, long animType);
      void                    playAttachmentAnim(int attachmentHandle, int animType);
      void                    resetAttachmentAnim(int attachmentHandle);
      void                    playAttachmentAnimOnEvent(int attachmentHandle, int animType, int eventType, int data1, int data2, bool useData1, bool useData2);
      void                    resetAttachmentAnimOnEvent(int attachmentHandle, int animType, int eventType, int data1, int data2, bool useData1, bool useData2);
      void                    setAbilityRecoverAttachmentAnimationLock(bool newVal);

#if DPS_TRACKER
      enum  { cMaxDamageHistory = 30, cMinDamageHistory = 3 };
      void                    damageDealt(float dmg);
      void                    clearDamageHistory();
#endif

      bool                    isNewBuildingConstructionAllowed(const BUnit* pBaseBuilding, BEntityID* pInProgressBuilding, bool secondaryQueue) const;
      BEntityID               getBaseBuilding() const;
      void                    queueBuildOther(BEntityID buildingID, bool secondaryQueue);
      void                    updateBuildOtherQueue();

      void                    upgradeLevel(int startLevel, int toLevel, bool doEffects, const BProtoObject* pOverrideProtoObject);

      bool                    isPhysicsAircraft() const;

      bool                    isAttackable(const BEntity* attacker, bool allowFriendly = false);

      bool                    canBeThrown() const;
      bool                    throwUnit(BEntityID attackerID, int32 attackerProtoActionID=-1, float throwVelocityScalar=1.0f);

      short                   getBaseNumber(){return mBaseNumber;}
      void                    setBaseNumber(short v){mBaseNumber = v;}

      void                    revealPosition(float time) const;

      void                    updateFade(float elapsedTime);

      void                    shatter();

      bool                    physicsCompleted();

      void                    doBuildingTerrainFlatten();
      void                    doBuildingTerrainAlpha();
      void                    undoBuildingTerrainAlpha();

      // Flags
      bool                    getFlagAlive() const { return(mFlagAlive); }
      void                    setFlagAlive(bool v) { mFlagAlive=v; mAnimationState.setDirty(); }
      bool                    getFlagDiesAtZeroHP() const { return(mFlagDiesAtZeroHP); }
      void                    setFlagDiesAtZeroHP(bool v) { mFlagDiesAtZeroHP=v; }
      bool                    getFlagDieAtZeroResources() const { return(mFlagDieAtZeroResources); }
      void                    setFlagDieAtZeroResources(bool v) { mFlagDieAtZeroResources=v; }
      bool                    getFlagUnlimitedResources() const { return(mFlagUnlimitedResources); }
      void                    setFlagUnlimitedResources(bool v) { mFlagUnlimitedResources=v; }
      bool                    getFlagRallyPoint() const { return(mFlagRallyPoint); }
      void                    setFlagRallyPoint(bool v) { mFlagRallyPoint=v; }
      bool                    getFlagRallyPoint2() const { return(mFlagRallyPoint2); }
      void                    setFlagRallyPoint2(bool v) { mFlagRallyPoint2=v; }
      bool                    getFlagHasHPBar() const { return(mFlagHasHPBar); }
      void                    setFlagHasHPBar(bool v) { mFlagHasHPBar=v; }
      bool                    getFlagDisplayHP() const { return(mFlagDisplayHP); }
      void                    setFlagDisplayHP(bool v) { mFlagDisplayHP=v; }
      bool                    getFlagForceDisplayHP() const { return(mFlagForceDisplayHP); }
      void                    setFlagForceDisplayHP(bool v) { mFlagForceDisplayHP=v; }            
      bool                    getFlagHasShield() const { return(mFlagHasShield); }
      void                    setFlagHasShield(bool v) { mFlagHasShield=v; }
      bool                    getFlagFullShield() const { return(mFlagFullShield); }
      void                    setFlagFullShield(bool v) { mFlagFullShield=v; }
      bool                    getFlagUsesAmmo() const { return(mFlagUsesAmmo); }
      void                    setFlagUsesAmmo(bool v) { mFlagUsesAmmo=v; }      
      bool                    getFlagHasGarrisoned() const { return(mFlagHasGarrisoned); }
      void                    setFlagHasGarrisoned(bool v) { mFlagHasGarrisoned=v; }
      bool                    getFlagHasAttached() const { return(mFlagHasAttached); }
      void                    setFlagHasAttached(bool v) { mFlagHasAttached=v; }
      bool                    getFlagAttackBlocked() const { return(mFlagAttackBlocked); }
      void                    setFlagAttackBlocked(bool v) { mFlagAttackBlocked=v; }
      bool                    getFlagHasGoalVector() const { return(mFlagHasGoalVector); }
      void                    setFlagHasGoalVector(bool v) { mFlagHasGoalVector=v; }
      bool                    getFlagHasFacingCommand() const { return(mFlagHasFacingCommand); }
      void                    setFlagHasFacingCommand(bool v) { mFlagHasFacingCommand=v; }
      bool                    getFlagIgnoreUserInput() const { return(mFlagIgnoreUserInput); }
      void                    setFlagIgnoreUserInput(bool v) { mFlagIgnoreUserInput=v; }
      bool                    getFlagTakeInfectionForm() const { return(mFlagTakeInfectionForm); }
      void                    setFlagTakeInfectionForm(bool v) { mFlagTakeInfectionForm=v; }
      bool                    getFlagFloodControl() const { return(mFlagFloodControl); }
      void                    setFlagFloodControl(bool v) { mFlagFloodControl=v; }
      bool                    getFlagHasHitched() const { return (mFlagHasHitched); }
      void                    setFlagHasHitched(bool v) { mFlagHasHitched = v; }      
      bool                    getFlagReverseMove() const { return (mFlagReverseMove); }
      void                    setFlagReverseMove(bool v) { mFlagReverseMove = v; mFlagReverseMoveHasMoved = false; }
      bool                    getFlagBuildingConstructionPaused() const { return mFlagBuildingConstructionPaused; }
      void                    setFlagBuildingConstructionPaused(bool v) { mFlagBuildingConstructionPaused=v; }
      bool                    getFlagBuildOtherQueue() const { return mFlagBuildOtherQueue; }
      void                    setFlagBuildOtherQueue(bool v) { mFlagBuildOtherQueue=v; }
      bool                    getFlagBuildOtherQueue2() const { return mFlagBuildOtherQueue2; }
      void                    setFlagBuildOtherQueue2(bool v) { mFlagBuildOtherQueue2=v; }
      bool                    getFlagPreserveDPS() const { return mFlagPreserveDPS; }
      void                    setFlagPreserveDPS(bool v) { mFlagPreserveDPS=v; }      
      bool                    getFlagDoingFatality() const { return mFlagDoingFatality; }
      void                    setFlagDoingFatality(bool v) { mFlagDoingFatality=v; }      
      bool                    getFlagFatalityVictim() const { return mFlagFatalityVictim; }
      void                    setFlagFatalityVictim(bool v) { mFlagFatalityVictim=v; }      
      bool                    getFlagDeathReplacementHealing() const { return mFlagDeathReplacementHealing; }
      void                    setFlagDeathReplacementHealing(bool v) { mFlagDeathReplacementHealing=v; }
      bool                    getFlagForceUpdateContainedUnits() const { return (mFlagForceUpdateContainedUnits); }
      void                    setFlagForceUpdateContainedUnits(bool v) { mFlagForceUpdateContainedUnits = v; }
      bool                    getFlagRecycled() const { return (mFlagRecycled); }
      void                    setFlagRecycled(bool v) { mFlagRecycled = v; }
      bool                    getFlagInfected() const { return (mFlagInfected); }
      void                    setFlagDown(bool v) { mFlagDown = v; }
      bool                    getFlagDown() const { return (mFlagDown); }
      void                    setFlagNoCorpse(bool v) { mFlagNoCorpse = v; }
      bool                    getFlagNoCorpse() const { return (mFlagNoCorpse); }
      void                    setFlagNotAttackable(bool v) { mFlagNotAttackable = v; }
      bool                    getFlagNotAttackable() const { return (mFlagNotAttackable); }
      void                    setFlagSelectTypeTarget(bool v) { mFlagSelectTypeTarget=v; }
      bool                    getFlagSelectTypeTarget() const { return mFlagSelectTypeTarget; }
      void                    setFlagBlockContain(bool v) { mFlagBlockContain=v; }
      bool                    getFlagBlockContain() const { return mFlagBlockContain; }
      void                    setFlagIsTypeGarrison(bool v) { mFlagIsTypeGarrison = v; }
      bool                    getFlagIsTypeGarrison() const { return (mFlagIsTypeGarrison); }
      void                    setFlagIsTypeCover(bool v) { mFlagIsTypeCover = v; }
      bool                    getFlagIsTypeCover() const { return (mFlagIsTypeCover); }
      void                    setFlagParentSquadChangingMode(bool v) { mFlagParentSquadChangingMode = v; }
      bool                    getFlagParentSquadChangingMode() const { return (mFlagParentSquadChangingMode); }
      void                    setFlagShatterOnDeath(bool v) { mFlagShatterOnDeath = v; }
      bool                    getFlagShatterOnDeath() const { return (mFlagShatterOnDeath); }
      void                    setFlagAllowStickyCam(bool v) { mFlagAllowStickyCam = v; }
      bool                    getFlagAllowStickyCam() const { return (mFlagAllowStickyCam); }
      void                    setFlagBeingBoarded(bool v) { mFlagBeingBoarded = v; }
      bool                    getFlagBeingBoarded() const { return (mFlagBeingBoarded); }
      void                    setFlagIsHibernating(bool v) { mFlagIsHibernating = v; }
      bool                    getFlagIsHibernating() const { return (mFlagIsHibernating); }
      void                    setFlagUnhittable(bool v) { mFlagUnhittable = v; }
      bool                    getFlagUnhittable() const { return mFlagUnhittable; }
      void                    setFlagDestroyedByBaseDestruction(bool v) { mFlagDestroyedByBaseDestruction = v; }
      bool                    getFlagDestroyedByBaseDestruction() const { return mFlagDestroyedByBaseDestruction; }
      void                    setFlagKilledByLeaderPower(bool v) { mFlagKilledByLeaderPower = v; }
      bool                    getFlagKilledByLeaderPower() const { return mFlagKilledByLeaderPower; }

      void                    setKilledByID(BEntityID id) { mKilledByID = id; }
      BEntityID               getKilledByID() const { return mKilledByID; } 

      void                    doDeathExplode(BEntityID killerID);
      void                    doDeathSpawnSquad();


      GFDECLAREVERSION();
      virtual bool save(BStream* pStream, int saveType) const;
      virtual bool load(BStream* pStream, int saveType);
      virtual bool postLoad(int saveType);

      virtual void            updateVisibleLists(void);
   protected:

      void                    updateContainedUnits(float elapsedTime);
      void                    updateHitchedUnit(float elapsedTime);
      void                    updateCover();
      void                    updateCorpseCollisions();
      //void                    updateVehicleSimulation( float elapsedTime );
      void                    updateTurn(float elapsedTime);
//      void                    updateGarrisoned(bool moving);

      void                    unitEnteredCombat(BEntityID targetID);
      void                    unitLeftCombat(void);            
      void                    removePersistentTacticActions();
      void                    createChildObjects();
      void                    updateDamageTracker(float elapsed);
      void                    updateAttackTimers(float elapsedTime);
      void                    initGroupRangePointsForWeapon(); // Supports special in-range calculation for base turrets

      void                    updateChildObjectDamageTakenScalar(bool death);

      void                    recalculateSupplyPadWorkRateModifiers();

      void                    createPersistentActions();

      inline bool             isMainBase() const { return (getProtoObject() && getProtoObject()->getFlagKillChildObjectsOnDeath() && isType(gDatabase.getOTIDBase())); }
      bool                    handleDelayKillBase();

      void                    refreshParentSquad();

      // Important: Order all member variables by largest to smallest! Anything that requires 16-byte alignment should appear first.
      // Use "bool mBlah : 1;" instead of "bool bBlah;"
      // Use chars and shorts instead of longs or ints when possible.

      BSquad*                 mpCachedParentSquad;
      BUnitOppList            mOpps;
      //E3.
      BSmallDynamicSimArray<uint> mVisualAmmo;
      BActionController       mControllers[BActionController::cNumberControllers];

#if DPS_TRACKER
      float                   mDmgHistory[cMaxDamageHistory];
      uchar                   mDmgHistoryIndex;
      float                   mDmgDealtBuffer;
      float                   mDmgUpdateTimer;
#endif
      
      BRallyPoint             mRallyPoint;
      BRallyPoint             mRallyPoint2;
      BVector                 mGoalVector;

      BEntityID               mKilledByID;
      long                    mKilledByWeaponType;
      BEntityID               mRallyObject;
      BEntityID               mRallyObject2;
      BEntityID               mCarriedObject;

      // Multi target
      BEntityIDArray          mMultiTargets;
      
      float                   mMaxHPContained;
      
      float                   mShieldpoints;
      float                   mHitpoints;
      float                   mResourceAmount;
      int                     mGatherers;

      // Data modifiers 
      float                   mDamageModifier;
      float                   mVelocityScalar;
      float                   mAccuracyScalar;
      float                   mWorkRateScalar;
      float                   mWeaponRangeScalar;
      float                   mDamageTakenScalar;
      float                   mDodgeScalar;

      BDamageTracker *        mDamageTracker;

      BSmallDynamicSimArray<BAttackTimer>  mAttackWaitTimer;

      float                   mMaxTurnRate;
      float                   mCurrentTurnRate;

      float                   mAcceleration;

      long                    mBattleID;

      //WMJ TODO MOVE THIS OUT OF HERE
      //float                   mFlippedTime;
      float                   mAmmunition;     

      // Contained Pop
      float                   mContainedPop;

      float                   mCapturePoints;
      BPlayerID               mCapturePlayerID;

      int                     mSelectionDecal;

      BHitZoneArray*          mpHitZoneList;

      long                    mAttackActionRefCount; //-- DJBFIXME: At some point we should combine all our ref counters into a single long using bit masks.      

      DWORD                   mLastMoveTime;
      DWORD                   mLeashTimer;
      
      int                     mTacticState;
      uint8                   mInfectionPlayerID;
      BEntityID               mFormerParentSquad;
      BOPQuadHull*            mpGroupHull;
      float                   mGroupDiagSpan;

      float                   mGarrisonTime;

      float                   mLastDPSRampValue;
      DWORD                   mLastDPSUpdate;

      float                   mShieldRegenRate;       // 4 bytes
      float                   mShieldRegenDelay;      // 4 bytes

#ifndef BUILD_FINAL
      //////////////////////////////////////////////////////////////////////////
      // SLB: temporary sync debugging stuff
      long                    mUnitDamageTemplateID;
      long                    mVisualDamageTemplateID;
      //////////////////////////////////////////////////////////////////////////
#endif

      short                   mBaseNumber;

      // Flags
      bool                    mFlagAlive:1;
      bool                    mFlagDiesAtZeroHP:1;
      bool                    mFlagDieAtZeroResources:1;
      bool                    mFlagUnlimitedResources:1;
      bool                    mFlagRallyPoint:1;
      bool                    mFlagRallyPoint2:1;
      bool                    mFlagHasHPBar:1;
      bool                    mFlagDisplayHP:1;
      bool                    mFlagForceDisplayHP:1;      
      bool                    mFlagHasShield:1;
      bool                    mFlagFullShield:1;
      bool                    mFlagUsesAmmo:1;      
      bool                    mFlagHasGarrisoned:1;
      bool                    mFlagHasAttached:1;
      bool                    mFlagAttackBlocked:1;
      bool                    mFlagHasGoalVector:1;
      bool                    mFlagHasFacingCommand:1;
      bool                    mFlagIgnoreUserInput:1;
      bool                    mFlagFirstBuilt:1;
      bool                    mFlagInfected:1;
      bool                    mFlagTakeInfectionForm:1;
      bool                    mFlagFloodControl:1;
      bool                    mFlagHasHitched:1;
      bool                    mFlagReverseMove:1;
      bool                    mFlagReverseMoveHasMoved:1;
      bool                    mFlagBuildingConstructionPaused:1;
      bool                    mFlagBuildOtherQueue:1;
      bool                    mFlagBuildOtherQueue2:1;
      bool                    mFlagPreserveDPS:1;
      bool                    mFlagDoingFatality:1;
      bool                    mFlagFatalityVictim:1;
      bool                    mFlagDeathReplacementHealing:1;
      bool                    mFlagForceUpdateContainedUnits:1;
      bool                    mFlagRecycled:1;
      bool                    mFlagDown:1;
      bool                    mFlagNoCorpse:1;
      bool                    mFlagNotAttackable:1;
      bool                    mFlagSelectTypeTarget:1;
      bool                    mFlagBlockContain:1;
      bool                    mFlagIsTypeGarrison:1;
      bool                    mFlagIsTypeCover:1;
      bool                    mFlagParentSquadChangingMode:1;
      bool                    mFlagShatterOnDeath:1;
      bool                    mFlagAllowStickyCam:1;
      bool                    mFlagBeingBoarded:1;
      bool                    mFlagAddResourceEnabled:1;
      bool                    mFlagIsHibernating:1;
      bool                    mFlagUnhittable:1;
      bool                    mFlagDestroyedByBaseDestruction:1;
      bool                    mFlagKilledByLeaderPower:1;
};

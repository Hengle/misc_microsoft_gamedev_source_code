//==============================================================================
// tactic.h
// Copyright (c) 2006 Ensemble Studios
//==============================================================================

#pragma once

// Includes
#include "Action.h"
#include "bitvector.h"
#include "xmlreader.h"
#include "damagehelper.h"
#include "simtypes.h"
#include "squadai.h"
#include "fatalitymanager.h"
#include "unitactionjoin.h"

// Forward Declarations
class BTactic; 
class BUnit;
class BAction;
class BPlayer;
class BObject;
class BProtoObject;
class BRumbleEvent;
class BCameraEffectData;

// Statics
const float cDefaultMaxRange=0.1f;
const float cDefaultHoverAltitudeOffset=0.0f;
const float cDefaultMaxTgtDepressionAngle=50.0f;
const float cDefaultMaxPitch=30.0f;
const float cDefaultMaxRoll=60.0f;

//==============================================================================
// BWeaponDamageTypeRatingOverride
//==============================================================================
class BWeaponDamageTypeRatingOverride
{
public:
   long                 mDamageType;
   float                mDamageRating;
   float                mHalfKillCutoffFactor;
};

//==============================================================================
// BWeaponStatic
//==============================================================================
class BWeaponStatic
{     
public:

   class BTargetPriority
   {
   public:
      BProtoObjectID    mProtoID; //-- This can be an protoID or an object type
      float             mPriorityAdjustment; 
   };

   BWeaponStatic();
   ~BWeaponStatic();

   BSimString           mName;   
   BSimString           mTriggerScript;
   BSmallDynamicSimArray<BWeaponDamageTypeRatingOverride>  mDamageRatingOverride;
   BDynamicSimArray<BTargetPriority> mTargetPriorities;
   long                 mImpactEffectSize;
   float                mImpactCameraShakeDuration;
   float                mImpactCameraShakeStrength;
   long                 mWeaponType;
   long                 mHardpointID;
   long                 mAlternateHardpointID;
   uint                 mVisualAmmo;
   BRumbleEvent*        mpImpactRumble;
   BCameraEffectData*   mpImpactCameraEffect;
   float                mPostAttackCooldownMin;
   float                mPostAttackCooldownMax;
   float                mPreAttackCooldownMin;
   float                mPreAttackCooldownMax;
   float                mAttackRate;
   float                mStartDPSPercent;
   float                mFinalDPSPercent;
   float                mDPSRampTime;
   float                mPhysicsLaunchAngleMin;
   float                mPhysicsLaunchAngleMax;
   float                mPhysicsForceMin;
   float                mPhysicsForceMax;
   float                mPhysicsForceMaxAngle; //-- Angle constraint for units to be thrown by physics. If angle(damageDir and impactoTargetDir) is greater than angle, then don't throw the unit
   float                mBeamWaitTime;
   float                mBeamSearchRadius;
   float                mBeamActivationSpeed;
   float                mBeamTrackingSpeed;
   float                mBeamTurnRate;
   float                mDazeDuration;
   float                mDazeMovementModifier;
   long                 mDazeTargetTypeID;
   float                mStasisHealToDrainRatio;
   float                mThrowOffsetAngle;
   float                mThrowVelocity;
   long                 mDOTEffectSmall;
   long                 mDOTEffectMedium;
   long                 mDOTEffectLarge;
   float                mReapplyTime;
   float                mApplyTime;
   float                mBounceRange;
   float                mShakeScalarNotLocal;

   BSimString           mPhysicsExplosionParticleName;
   int                  mPhysicsExplosionParticleHandle;
   BObjectTypeID        mPhysicsExplosionVictimType;
   
   int8                 mSmartTargetType;
   int8                 mNumBounces;

   float                mMaxPullRange;

   bool                 mTargetsFootOfUnit:1;
   bool                 mKeepDPSRamp:1;
   bool                 mCausePhysicsExplosion:1;
   bool                 mFlagPullUnits:1;
   bool                 mFlagAirBurst:1;
   bool                 mFlagShieldDrain:1;
};

//==============================================================================
// BWeapon
//==============================================================================
class BWeapon
{  
   public:
      BWeapon();
      BWeapon(const BWeapon* pBase);
      ~BWeapon();

      bool                 loadWeapon(BXMLNode root, const BProtoObject *pObj);       
      void                 postloadWeapon();

      float                getStartDPSPercent() const { return mpStaticData->mStartDPSPercent; }
      float                getFinalDPSPercent() const { return mpStaticData->mFinalDPSPercent; }
      float                getDPSRampTime() const { return mpStaticData->mDPSRampTime; }
      float                getDazeDuration() const { return mpStaticData->mDazeDuration; }
      float                getDazeMovementModifier() const { return mpStaticData->mDazeMovementModifier; }
      long                 getDazeTargetTypeID() const { return mpStaticData->mDazeTargetTypeID; }
      int8                 getSmartTargetType() const { return(mpStaticData->mSmartTargetType); }
      int8                 getNumBounces() const { return(mpStaticData->mNumBounces); }
      float                getBounceRange() const { return(mpStaticData->mBounceRange); }
      float                getStasisHealToDrainRatio() const { return(mpStaticData->mStasisHealToDrainRatio); }

      float                getMaxPullRange() const { return(mpStaticData->mMaxPullRange); }

      //-- Flags
      bool                 getFlagOwnStaticData() const { return(mFlagOwnStaticData); }
      bool                 getFlagFriendlyFire() const { return(mFlagFriendlyFire); }
      bool                 getFlagHeightBonusDamage() const { return(mFlagHeightBonusDamage); }
      bool                 getFlagUsesAmmo() const { return(mFlagUsesAmmo); }
      bool                 getFlagPhysicsLaunchAxial() const { return(mFlagPhysicsLaunchAxial); }
      bool                 getFlagThrowUnits() const { return(mFlagThrowUnits); }
      bool                 getFlagThrowDamageParts() const { return(mFlagThrowDamageParts); }
      bool                 getFlagUsesBeam() const { return mFlagUsesBeam; }
      bool                 getFlagFlailThrownUnits() const { return(mFlagFlailThrownUnits); }
      bool                 getFlagDodgeable() const { return mFlagDodgeable; }
      bool                 getFlagDeflectable() const { return mFlagDeflectable; }
      bool                 getFlagSmallArmsDeflectable() const { return mFlagSmallArmsDeflectable; }
      bool                 getFlagUseDPSasDPA() const { return mFlagUseDPSasDPA; }
      bool                 getFlagUseGroupRange() const { return mFlagUseGroupRange; }
      bool                 getFlagDoShockwaveAction() const { return mFlagDoShockwaveAction; }
      bool                 getFlagCarriedObjectAsProjectileVisual() const { return mFlagCarriedObjectAsProjectileVisual; }
      bool                 getFlagStasis() const { return(mFlagStasis); }
      bool                 getFlagStasisDrain() const { return(mFlagStasisDrain); }
      bool                 getFlagStasisBomb() const { return(mFlagStasisBomb); }
      bool                 getFlagKnockback() const { return(mFlagKnockback); }
      bool                 getFlagTentacle() const { return(mFlagTentacle); }
      bool                 getFlagDaze() const { return(mFlagDaze); }
      bool                 getFlagAOEDaze() const { return(mFlagAOEDaze); }
      bool                 getFlagAOELinearDamage() const { return(mFlagAOELinearDamage); }
      bool                 getFlagAOEIgnoresYAxis() const { return(mFlagAOEIgnoresYAxis); }
      bool                 getFlagOverridesRevive() const { return(mFlagOverridesRevive); }
      bool                 getFlagPullUnits() const { return(mpStaticData->mFlagPullUnits); }
      bool                 getFlagAirBurst() const { return (mpStaticData->mFlagAirBurst); }
      bool                 getFlagShieldDrain() const { return (mpStaticData->mFlagShieldDrain); }

      GFDECLAREVERSION();
      bool save(BStream* pStream, int saveType) const;
      bool load(BStream* pStream, int saveType);

      float                mDamagePerSecond;
      float                mDOTrate;
      float                mDOTduration;
      float                mMaxRange;
      float                mMinRange;
      float                mAOERadius;
      float                mAOEPrimaryTargetFactor;
      float                mAOEDistanceFactor;
      float                mAOEDamageFactor;   
      float                mAccuracy;
      float                mMovingAccuracy;
      float                mMaxDeviation;
      float                mMovingMaxDeviation;
      float                mAccuracyDistanceFactor;
      float                mAccuracyDeviationFactor;
      float                mMaxVelocityLead;
      float                mMaxDamagePerRam;
      float                mReflectDamageFactor;
      float                mAirBurstSpan;
      long                 mProjectileObjectID;
      long                 mImpactEffectProtoID;

      BWeaponStatic*       mpStaticData;

       //-- Flags
      bool                 mFlagOwnStaticData:1;
      bool                 mFlagFriendlyFire:1;
      bool                 mFlagHeightBonusDamage:1;
      bool                 mFlagPhysicsLaunchAxial:1;
      bool                 mFlagUsesAmmo:1;
      bool                 mFlagThrowUnits:1;
      bool                 mFlagThrowAliveUnits:1;
      bool                 mFlagThrowDamageParts:1;
      bool                 mFlagUsesBeam:1;
      bool                 mFlagFlailThrownUnits:1;      
      bool                 mFlagDodgeable:1;
      bool                 mFlagDeflectable:1;
      bool                 mFlagSmallArmsDeflectable:1;
      bool                 mFlagUseDPSasDPA:1;
      bool                 mFlagDoShockwaveAction:1;
      bool                 mFlagUseGroupRange:1;
      bool                 mFlagCarriedObjectAsProjectileVisual:1;
      bool                 mFlagStasis:1;
      bool                 mFlagStasisDrain:1;
      bool                 mFlagStasisBomb:1;
      bool                 mFlagKnockback:1;
      bool                 mFlagTentacle:1;
      bool                 mFlagDaze:1;
      bool                 mFlagAOEDaze:1;
      bool                 mFlagAOELinearDamage:1;
      bool                 mFlagAOEIgnoresYAxis:1;
      bool                 mFlagOverridesRevive:1;
};

//==============================================================================
// BProtoActionStatic
//==============================================================================
class BProtoActionStatic
{
   public:
      BProtoActionStatic();
      ~BProtoActionStatic() {}
      
      long                 mID;
      int                  mWeaponID;
      int                  mLinkedActionID;
      int                  mResourceType;
      int                  mSquadType;
      int                  mReloadAnimType;
      int                  mExposedActionIndex;      
      int                  mSlaveAttackActionID;
      int                  mBaseDPSWeaponID;

      float                mWorkRange;
      float                mProjectileSpread; // For bombard...

      BSimString           mName;
      
      BActionType          mActionType;
      BActionType          mPersistentActionType;      

      float                mDodgeChanceMin;
      float                mDodgeChanceMax;
      float                mDodgeMaxAngle;
      DWORD                mDodgeCooldown;
      float                mDodgePhysicsImpulse;

      float                mDeflectChanceMin;
      float                mDeflectChanceMax;
      float                mDeflectMaxAngle;
      DWORD                mDeflectCooldown;
      float                mDeflectMaxDmg;
      float                mDeflectTimeout;

      float                mStrafingMaxDistance;
      float                mStrafingTrackingSpeed;
      float                mStrafingJitter;

      float                mHoverAltitudeOffset;
      float                mMaxTgtDepressionAngle;
      float                mMaxPitch;
      float                mMaxRoll;

      float                mDamageBuffFactor;
      float                mDamageTakenBuffFactor;
      BUnitActionJoin::JOIN_TYPE mJoinType;
      BUnitActionJoin::MERGE_TYPE mMergeType;
      float                mJoinRevertDamagePct;
      int                  mJoinBoardAnimType;
      int                  mJoinLevels;
      float                mJoinMaxTeleportDist;

      int                  mStartAnimType;
      int                  mEndAnimType;
      BCueIndex            mEndAnimSoundCue;

      BFatalityArray       mFatalityArray;
      DWORD                mMinIdleDuration;

      DWORD                mShockwaveDuration;
      int                  mSquadMode;
      int                  mNewSquadMode;
      int                  mNewTacticState;

      DWORD                mAutoRepairIdleTime;
      float                mAutoRepairThreshold;
      float                mAutoRepairSearchDistance;

      float                mDuration;
      float                mDurationSpread;
      float                mPhysicsDetonationThreshold;

      float                mAttackRunDelayMin;
      float                mAttackRunDelayMax;

      float                mReviveDelay;
      float                mHibernateDelay;
      float                mReviveRate;

      BProtoObjectIDArray  mInvalidTargetsArray;

      float                mVelocityScalar;
      
      BSimString           mParticleEffectName;
      int                  mParticleEffectId;

      long                 mProtoObject;
      BSimString           mBone;

      long                 mCount;

      long                 mSupportAnimType;
      int                  mMaxNumUnitsPerformAction;

      long                 mLowDetailAnimType;
      long                 mLowDetailAnimThreshold;

      float                mDetonateThrowHorizontalMax;
      float                mDetonateThrowVerticalMax;

      float                mDamageCharge;

      bool                 mStationary:1;
      bool                 mMainAttack:1;
      bool                 mStopAttackingWhenAmmoDepleted:1; // For bombard...
      bool                 mDontLoopAttackAnim:1; // For bombard...   
      bool                 mDontCheckOrientTolerance:1; // For Spartan demolitions...
      bool                 mKillSelfOnAttack:1;
      bool                 mMeleeRange:1;
      bool                 mInstantAttack:1;

      bool                 mInfection:1;
      bool                 mCanOrientOwner:1;
      bool                 mStrafing:1;
      bool                 mStartAnimNoInterrupt:1;
      bool                 mAnimNoInterrupt:1;
      bool                 mEndAnimNoInterrupt:1;
      bool                 mBeam:1;
      bool                 mBeamCollideWithUnits:1;

      bool                 mBeamCollideWithTerrain:1;
      bool                 mTargetAir:1;
      bool                 mAttackWaitTimerEnabled:1;      
      bool                 mFindBetterAction:1;
      bool                 mPersistBetweenOpps:1;
      bool                 mShockwaveAction:1;
      bool                 mAllowReinforce:1;
      bool                 mDamageBuffsByCombatValue:1;

      bool                 mOverrun:1;
      bool                 mJoinVeterancyOverride:1;
      bool                 mClearTacticState:1;
      bool                 mDetonateWhenInRange:1;
      bool                 mDetonateFromPhysics:1;
      bool                 mMeleeAttack:1;
      bool                 mHealTarget:1;
      bool                 mFlagTargetOfTarget:1;

      bool                 mFlagUseTeleporter:1;
      bool                 mFlagWaitForDeflectCooldown:1;
      bool                 mFlagMultiDeflect:1;
      bool                 mFlagWaitForDodgeCooldown:1;
      bool                 mFlagDetonateOnDeath:1;
      bool                 mFlagPickupObject:1;
      bool                 mFlagPickupObjectOnKill:1;
      bool                 mFlagAlertWhenComplete:1;

      bool                 mFlagAutoJoin:1;
      bool                 mChargeOnTaken:1;
      bool                 mChargeOnDealt:1;
      bool                 mChargable:1;
      bool                 mFlagSmallArms:1;
      bool                 mFlagDoShakeOnAttackTag:1;
      bool                 mFlagHideSpawnUntilRelease:1;
      bool                 mFlagAvoidOnly:1;

      bool                 mDontAutoRestart:1;
};

//==============================================================================
// BProtoAction
//==============================================================================
class BProtoAction : public IDamageInfo
{
   public:     
      BProtoAction();
      BProtoAction(BTactic* pTactic, const BProtoAction* pBase);
      ~BProtoAction();

      bool                 loadProtoAction(BXMLNode root, long id, BTactic *pData);   
      void                 postloadProtoAction(const BProtoObject* pObj);

      void                 doImpactRumbleAndCameraShake(int eventType, BVector location, bool onlyIfSelected, BEntityID unitID) const;

      BFatality*           getFatality(const BUnit* pAttackerUnit, const BUnit* pTargetUnit, uint& outAssetIndex) const;
      BFatality*           getFatalityByIndex(uint index) const;

      long                 getID() const { return mpStaticData->mID; }

      // Dynamic data access (changeable per player)
      float                getWorkRate(BEntityID sourceUnit = cInvalidObjectID) const;
      float                getWorkRateVariance() const { return mWorkRateVariance; }
      float                getMaxRange(const BUnit* pUnit, bool bIncludeGroupRange=true, bool bIncludePullRange=true) const;
      virtual float        getDamagePerSecond() const;
      virtual float        getDamagePerAttack() const;
      long                 getMaxNumAttacksPerAnim() const;
      long                 getProjectileID() const;
      long                 getImpactEffectProtoID() const;                  
      float                getStrafingTurnRate() const { return mStrafingTurnRate; }
      void                 setStrafingTurnRate(float val) { mStrafingTurnRate = val; }

      void                 setWorkRate(float val) { mWorkRate=val; }
      void                 setWorkRateVariance(float val) { mWorkRateVariance=val; }
      void                 setDamagePerAttack(float val);
      void                 setMaxNumAttacksPerAnim(long numAttacks);

      void                 setMaxRange(float val);      

      //-- Flags
      bool                 getFlagOwnStaticData() const { return(mFlagOwnStaticData); }
      bool                 getFlagDisabled() const { return(mFlagDisabled); }
      void                 setFlagDisabled(bool v) { mFlagDisabled=v; }
      bool                 getFlagNoAutoTarget() const { return(mFlagNoAutoTarget); }
      bool                 getFlagDieOnBuilt() const { return(mFlagDieOnBuilt); }
      bool                 getFlagTeamShare() const { return(mFlagTeamShare); }
      bool                 getFlagAlertWhenComplete() const { return(mpStaticData->mFlagAlertWhenComplete); }
      bool                 getFlagAutoJoin() const { return (mpStaticData->mFlagAutoJoin); }

      // Static data access
      const BSimString&    getName() const {return mpStaticData->mName;}
      long                 getWeaponID() const {return mpStaticData->mWeaponID;}
      long                 getLinkedActionID() const {return mpStaticData->mLinkedActionID;}
      BActionType          getActionType() const {return mpStaticData->mActionType;}
      long                 getSquadType() const {return mpStaticData->mSquadType;}
      long                 getResourceType() const { return mpStaticData->mResourceType; }
      long                 getAnimType() const;
      long                 getAnimTypeIndexForBattleSize(int32 numUnitsInBattle) const;      
      long                 getReloadAnimType() const { return mpStaticData->mReloadAnimType; }
      float                getWorkRange() const { return mpStaticData->mWorkRange; }
      float                getProjectileSpread() const { return mpStaticData->mProjectileSpread; }
      bool                 getStopAttackingWhenAmmoDepleted() const { return mpStaticData->mStopAttackingWhenAmmoDepleted; }
      bool                 getDontLoopAttackAnim() const { return mpStaticData->mDontLoopAttackAnim; }
      bool                 getDontAutoRestart() const { return mpStaticData->mDontAutoRestart; }
      bool                 getDontCheckOrientTolerance() const { return (mpStaticData->mDontCheckOrientTolerance); }
      bool                 getKillSelfOnAttack() const { return (mpStaticData->mKillSelfOnAttack); }
      bool                 getMeleeRange() const { return (mpStaticData->mMeleeRange); }
      bool                 getInstantAttack() const { return (mpStaticData->mInstantAttack); }
      virtual bool         getInfection() const { return (mpStaticData->mInfection); }
      bool                 canOrientOwner() const { return mpStaticData->mCanOrientOwner; }
      bool                 getImpactRumble() const;
      virtual void         getTriggerScript(BSimString& scriptName) const;
      virtual long         getWeaponType() const;
      uint                 getVisualAmmo() const;
      bool                 getDamageRatingOverride(long damageType, float& rating) const;
      virtual bool         getHalfKillCutoffFactor(long damageType, float& halfKillCutoffFactor) const;
      long                 getImpactEffectSize() const;
      float                getImpactCameraShakeDuration() const;
      float                getImpactCameraShakeStrength() const;      
      bool                 getDoShockwaveAction() const;
      float                getMinRange() const;
      virtual bool         getFriendlyFire() const;
      virtual float        getAOERadius() const;
      virtual float        getAOEPrimaryTargetFactor() const;
      virtual float        getAOEDistanceFactor() const;
      virtual float        getAOEDamageFactor() const;
      virtual bool         getFlagAOELinearDamage() const;
      virtual bool         getFlagAOEIgnoresYAxis() const;
      virtual float        getPhysicsLaunchAngleMin() const;
      virtual float        getPhysicsLaunchAngleMax() const;
      virtual bool         getPhysicsLaunchAxial() const;
      virtual float        getPhysicsForceMin() const;
      virtual float        getPhysicsForceMax() const;
      virtual float        getPhysicsForceMaxAngle() const;
      virtual bool         getThrowUnits() const;      
      virtual bool         getThrowAliveUnits() const;
      virtual bool         getThrowDamageParts() const;  
      virtual bool         getFlailThrownUnits() const;      
      virtual bool         getDodgeable() const;
      virtual bool         getDeflectable() const;
      virtual bool         getSmallArmsDeflectable(void) const;
      virtual bool         getOverridesRevive() const;
      virtual bool         getPullUnits() const;
      virtual bool         getUseDPSasDPA() const;
      virtual bool         getCarriedObjectAsProjectileVisual() const;
      virtual bool         getFlagAirBurst() const;

      bool                 usesHeightBonusDamage() const;
      bool                 usesAmmo() const;
      bool                 usesBeam() const;
      bool                 isWeaponAffectedByGravity(const BPlayer* pPlayer) const;
      long                 getHardpointID() const;
      long                 getAlternateHardpointID() const;
      float                getAccuracy() const;
      float                getMovingAccuracy() const;
      float                getMaxDeviation() const;
      float                getMovingMaxDeviation() const;
      float                getAccuracyDistanceFactor() const;
      float                getAccuracyDeviationFactor() const;
      float                getAirBurstSpan() const;
      float                getMaxVelocityLead() const;
      int                  getExposedActionIndex() const { return (mpStaticData->mExposedActionIndex); }
      int                  getSlaveAttackActionID() const { return (mpStaticData->mSlaveAttackActionID); }
      float                getAddedBaseDPS() const;
      float                getTargetPriority(const BProtoObject* pTargetProto) const;
      bool                 getMainAttack() const { return (mpStaticData->mMainAttack); }
      bool                 getStationary() const { return (mpStaticData->mStationary); }
      BActionType          getPersistentActionType() const { return mpStaticData->mPersistentActionType; }   
      float                getBeamSearchRadius() const;
      float                getBeamActivationSpeed() const;
      float                getBeamTrackingSpeed() const;
      float                getBeamTurnRate() const;
      float                getBeamWaitTime() const;
      bool                 getTargetsFootOfUnit() const;
      bool                 getCausePhysicsExplosion() const;
      bool                 keepDPSRamp() const;
      bool                 isAttackWaitTimerEnabled() const { return mpStaticData->mAttackWaitTimerEnabled; }
      bool                 canFindBetterAction() const { return mpStaticData->mFindBetterAction; } 
      bool                 persistBetweenOpps() const { return mpStaticData->mPersistBetweenOpps; }
      bool                 getAllowReinforce() const { return (mpStaticData->mAllowReinforce); }
      bool                 getOverrun() const { return (mpStaticData->mOverrun); }
      float                getPreAttackCooldownMin() const;      
      float                getPreAttackCooldownMax() const;      
      float                getPostAttackCooldownMin() const;      
      float                getPostAttackCooldownMax() const;      
      float                rollPreAttackCooldown() const;
      float                rollPostAttackCooldown() const;      
      float                getAttackRate() const;
      bool                 getFlagShockwaveAction() const { return mpStaticData->mShockwaveAction; }
      bool                 getFlagRequiresLockdown() const { return mFlagRequiresLockdown; }
      const BProtoObjectIDArray& getInvalidTargets() const { return mpStaticData->mInvalidTargetsArray; }
      bool                 getClearTacticState() const { return mpStaticData->mClearTacticState; }
      long                 getSupportAnimType() const { return mpStaticData->mSupportAnimType; }      
      long                 getLowDetailAnimType() const { return mpStaticData->mLowDetailAnimType; }      
      long                 getLowDetailAnimThreshold() const { return mpStaticData->mLowDetailAnimThreshold; }

      // temp bowling
      float                getMaxDamagePerRam() const;
      float                getReflectDamageFactor() const;
      float                getThrowOffsetAngle() const;
      float                getThrowVelocity() const;

      float                getDodgeChanceMax() const { return mpStaticData->mDodgeChanceMax; }
      float                getDodgeChanceMin() const { return mpStaticData->mDodgeChanceMin; }
      float                getDodgeMaxAngle() const { return mpStaticData->mDodgeMaxAngle; }
      DWORD                getDodgeCooldown() const { return mpStaticData->mDodgeCooldown; }
      float                getDodgePhysicsImpulse() const { return mpStaticData->mDodgePhysicsImpulse; }
      bool                 getFlagWaitForDeflectCooldown() const { return mpStaticData->mFlagWaitForDeflectCooldown; }
      bool                 getFlagMultiDeflect() const { return mpStaticData->mFlagMultiDeflect; }
      bool                 getFlagSmallArms() const { return mpStaticData->mFlagSmallArms; }
      bool                 getFlagDoShakeOnAttackTag() const { return mpStaticData->mFlagDoShakeOnAttackTag; }
      bool                 getFlagHideSpawnUntilRelease() const { return mpStaticData->mFlagHideSpawnUntilRelease; }
      bool                 getFlagAvoidOnly() const { return mpStaticData->mFlagAvoidOnly; }

      float                getDeflectChanceMax() const { return mpStaticData->mDeflectChanceMax; }
      float                getDeflectChanceMin() const { return mpStaticData->mDeflectChanceMin; }
      float                getDeflectMaxAngle() const { return mpStaticData->mDeflectMaxAngle; }
      DWORD                getDeflectCooldown() const { return mpStaticData->mDeflectCooldown; }
      float                getDeflectMaxDmg() const { return mpStaticData->mDeflectMaxDmg; }
      float                getDeflectTimeout() const { return mpStaticData->mDeflectTimeout; }
      bool                 getFlagWaitForDodgeCooldown() const { return mpStaticData->mFlagWaitForDodgeCooldown; }

      // Strafing
      bool                 isStrafing() const { return mpStaticData->mStrafing; }
      float                getStrafingMaxDistance() const { return mpStaticData->mStrafingMaxDistance; }      
      float                getStrafingTrackingSpeed() const { return mpStaticData->mStrafingTrackingSpeed; }
      float                getStrafingJitter() const { return mpStaticData->mStrafingJitter; }

      // Hover Flight
      float                getHoverAltitudeOffset() const { return mpStaticData->mHoverAltitudeOffset; }
      float                getMaxTgtDepressionAngle() const { return mpStaticData->mMaxTgtDepressionAngle; }
      float                getMaxPitch() const { return mpStaticData->mMaxPitch; }
      float                getMaxRoll() const { return mpStaticData->mMaxRoll; }

      // Move Air (continuous flight)
      float                getAttackRunDelayMin() const { return mpStaticData->mAttackRunDelayMin; }
      float                getAttackRunDelayMax() const { return mpStaticData->mAttackRunDelayMax; }

      // Self-Revive units (Flood buildings)
      float                getReviveDelay() const { return mpStaticData->mReviveDelay; }
      float                getHibernateDelay() const { return mpStaticData->mHibernateDelay; }
      float                getReviveRate() const { return mpStaticData->mReviveRate; }

      // Damage buffs
      float                getDamageBuffFactor() const { return mpStaticData->mDamageBuffFactor; }
      float                getDamageTakenBuffFactor() const { return mpStaticData->mDamageTakenBuffFactor; }
      bool                 isDamageBuffByCombatValue() const { return mpStaticData->mDamageBuffsByCombatValue; }

      // Join stuff
      BUnitActionJoin::JOIN_TYPE getJoinType() const { return mpStaticData->mJoinType; }
      BUnitActionJoin::MERGE_TYPE getMergeType() const { return mpStaticData->mMergeType; }
      float                getJoinRevertDamagePct() const { return mpStaticData->mJoinRevertDamagePct; }
      bool                 doesJoinOverrideVeterancy() const { return mpStaticData->mJoinVeterancyOverride; }
      bool                 getHealTarget() const { return mpStaticData->mHealTarget; }
      bool                 getFlagTargetOfTarget() const { return mpStaticData->mFlagTargetOfTarget; }
      bool                 getUseTeleporter() const { return mpStaticData->mFlagUseTeleporter; }
      float                getJoinBoardTime() const { return mJoinBoardTime; }
      void                 setJoinBoardTime(float val) { mJoinBoardTime = val; }
      int                  getJoinBoardAnimType() const { return mpStaticData->mJoinBoardAnimType; }
      int                  getJoinLevels() const { return mpStaticData->mJoinLevels; }
      float                getUnjoinMaxTeleportDist() const { return mpStaticData->mJoinMaxTeleportDist; }

      // Start/End Anim
      int                  getStartAnimType() const { return mpStaticData->mStartAnimType; }
      int                  getEndAnimType() const { return mpStaticData->mEndAnimType; }
      bool                 isStartAnimNoInterrupt() const { return mpStaticData->mStartAnimNoInterrupt; }
      bool                 isAnimNoInterrupt() const { return mpStaticData->mAnimNoInterrupt; }
      bool                 isEndAnimNoInterrupt() const { return mpStaticData->mEndAnimNoInterrupt; }
      BCueIndex            getEndAnimSoundCue() const { return mpStaticData->mEndAnimSoundCue; }
      DWORD                getMinIdleDuration() const { return (mpStaticData->mMinIdleDuration); }

      DWORD                getShockwaveDuration() const { return mpStaticData->mShockwaveDuration; }
      int                  getSquadMode() const { return mpStaticData->mSquadMode; }
      int                  getNewSquadMode() const { return mpStaticData->mNewSquadMode; }
      int                  getNewTacticState() const { return mpStaticData->mNewTacticState; }

      DWORD                getAutoRepairIdleTime() const { return mpStaticData->mAutoRepairIdleTime; } 
      float                getAutoRepairThreshold() const { return mpStaticData->mAutoRepairThreshold; }
      float                getAutoRepairSearchDistance() const { return mpStaticData->mAutoRepairSearchDistance; }

      // Beam
      bool                 isBeam() const { return mpStaticData->mBeam; }
      bool                 doesBeamCollideWithUnits() const { return mpStaticData->mBeamCollideWithUnits; }
      bool                 doesBeamCollideWithTerrain() const { return mpStaticData->mBeamCollideWithTerrain; }

      bool                 targetsAir() const { return mpStaticData->mTargetAir; }

      // Damage over time (DOT)
      virtual float        getDOTrate() const;
      virtual float        getDOTduration() const;
      virtual long         getDOTEffect(BUnit* pUnit) const;

      virtual float        getReapplyTime() const;
      virtual float        getApplyTime() const;

      bool                 getFlagStasis() const;
      bool                 getFlagStasisDrain() const;
      bool                 getFlagStasisBomb() const;
      bool                 getFlagDaze() const;
      bool                 getFlagAOEDaze() const;

      bool                 getFlagKnockback() const;
      bool                 getFlagTentacle() const;

      float                getDuration() const { return mpStaticData->mDuration; }
      float                getDurationSpread() const { return mpStaticData->mDurationSpread; }
      bool                 getDetonateWhenInRange() const { return mpStaticData->mDetonateWhenInRange; } 
      float                getPhysicsDetonationThreshold() const { return mpStaticData->mPhysicsDetonationThreshold; }
      bool                 getDetonateFromPhysics() const { return mpStaticData->mDetonateFromPhysics; }
      bool                 getDetonateOnDeath() const { return mpStaticData->mFlagDetonateOnDeath; }

      // Daze
      float                getDazeDuration() const;
      long                 getDazeTargetTypeID() const;
      float                getDazeMovementModifier() const;

      bool                 isMeleeAttack() const { return mpStaticData->mMeleeAttack; }

      // Speed modifier
      float                getVelocityScalar() const { return mpStaticData->mVelocityScalar; }

      BProtoObjectID       getParticleEffectId() const { return mpStaticData->mParticleEffectId; }

      // pickup object
      bool                 getFlagPickupObject() const { return mpStaticData->mFlagPickupObject; }
      bool                 getFlagPickupObjectOnKill() const { return mpStaticData->mFlagPickupObjectOnKill; }

      long                 getProtoObject() const { return mpStaticData->mProtoObject; }
      const BSimString&    getBoneName() const { return mpStaticData->mBone; }

      long                 getCount() const { return mpStaticData->mCount; }
      int                  getMaxNumUnitsPerformAction() const { return mpStaticData->mMaxNumUnitsPerformAction; }

      float                getDetonateThrowHorizontalMax() const { return mpStaticData->mDetonateThrowHorizontalMax; }
      float                getDetonateThrowVerticalMax() const { return mpStaticData->mDetonateThrowVerticalMax; }

      int                  getPhysicsExplosionParticle() const;
      BObjectTypeID        getPhysicsExplosionVictimType() const;

      // Damage charge
      float                getDamageCharge() const      { return mpStaticData->mDamageCharge; }
      bool                 getFlagChargeOnTaken() const { return mpStaticData->mChargeOnTaken; }
      bool                 getFlagChargeOnDealt() const { return mpStaticData->mChargeOnDealt; }
      bool                 getFlagChargable() const     { return mpStaticData->mChargable; }

      const BTactic*       getTactic() const { return mpTactic; }

      void                 setIndex(int index) { mIndex=(int8)index; }
      int                  getIndex() const { return (int)mIndex; }

      GFDECLAREVERSION();
      bool save(BStream* pStream, int saveType) const;
      bool load(BStream* pStream, int saveType);

      static bool savePtr(BStream* pStream, const BProtoAction* pProtoAction);
      static bool loadPtr(BStream* pStream, BProtoAction** ppProtoAction);

   private:
      // Dynamic data
      float                mWorkRate;
      float                mWorkRateVariance;
      long                 mAnimType;
      float                mDamagePerAttack;
      long                 mMaxNumAttacksPerAnim;
      float                mStrafingTurnRate;
      BTactic*             mpTactic;
      float                mJoinBoardTime;


      // Static data
      BProtoActionStatic*  mpStaticData;

      int8                 mIndex;

      // Flags
      bool                 mFlagOwnStaticData:1;
      bool                 mFlagDisabled:1;
      bool                 mFlagNoAutoTarget:1;
      bool                 mFlagDieOnBuilt:1;
      bool                 mFlagTeamShare:1;
      bool                 mFlagRequiresLockdown:1;
      bool                 mFlagTargetOfTarget:1;
};

//==============================================================================
// BTargetRule
//==============================================================================
class BTargetRule
{
   public:
      BTargetRule(void);

      long                 mActionID;
      long                 mAbilityID;
      BRelationType        mRelation;
      long                 mSquadMode;
      BDynamicSimLongArray mDamageTypes;
      BDynamicSimLongArray mObjectTypes;

      bool mHasShieldDamageType     : 1;
      bool mTargetStateUnbuilt      : 1;
      bool mTargetStateDamaged      : 1;
      bool mTargetStateCapturable   : 1;
      bool mTargetsGround           : 1;
      bool mContainsUnits           : 1;
      bool mGaiaOwned               : 1;
      bool mAutoTargetSquadMode     : 1;
      bool mOptionalAbility         : 1;
      bool mMergeSquads             : 1;
      bool mMeleeAttacker           : 1;
};

//==============================================================================
// BTacticState
//==============================================================================
class BTacticState
{
   public:
      BTacticState();
      ~BTacticState() { }
      
      BSimString                 mName;   
      long                       mIdleAnim;
      long                       mWalkAnim;
      long                       mJogAnim;
      long                       mRunAnim;
      long                       mDeathAnim;
      BSmallDynamicSimArray<uint> mActions;
};

//==============================================================================
// BTacticStatic
//==============================================================================
class BTacticStatic
{
   public:
      BTacticStatic() : mDefaultActionID(-1), mTrampleActionID(-1) {}
      ~BTacticStatic() {}
      
      BDynamicSimArray<BTargetRule>       mTargetRules;
      BDynamicSimArray<long>              mPersistentActions;
      BDynamicSimArray<long>              mPersistentSquadActions;
      BSmallDynamicSimArray<BTacticState> mStates;

      BString                             mName;
      long                                mDefaultActionID;      
      long                                mTrampleActionID; //DJBFIXME: Do this better after E3
};

//==============================================================================
// BTactic
//==============================================================================
class BTactic
{
   public:
      BTactic();
      BTactic(const BTactic* pBase, BProtoObjectID protoObjectID);
      ~BTactic();

      bool                 preloadTactic(const char *szFilename) {mpStaticData->mName = szFilename; return true;}

      bool                 loadTactic(const BProtoObject *pObj);   

      void                 postloadTactic(const BProtoObject *pObj);

      void                 setupTacticAttackRatings();

      BAction*             createAction(const BUnit *pTargetUnit);

      // Dynamic data access (changeable per player)
      const BWeapon*       getWeapon(long weaponIndex) const {if(weaponIndex < 0 || weaponIndex >= mWeapons.getNumber()) return NULL; return mWeapons[weaponIndex];}
      long                 getWeaponID(const BSimString &weaponName);
      long                 getNumberWeapons() const { return mWeapons.getNumber(); }
      int                  getTacticStateID(const BSimString &tacticStateName);
      BProtoAction*        getProtoAction(uint state, const BObject* pTarget, const BVector targetLoc,
                              const BPlayer* pPlayer, BVector sourcePosition, BEntityID sourceUnit = cInvalidObjectID,
                              long abilityID = -1, bool autoTarget = false, long actionType=-1, bool checkTimers=false, 
                              bool checkOrientation=false, bool* pInsideMinRange=NULL, bool noDiscardAbilities=false, long* pRuleAbilityID=NULL,
                              bool* pRuleTargetsGround=NULL, bool ignoreAlreadyJoined=false);
      BProtoAction*        getProtoAction(long id);
      long                 getNumberProtoActions() const { return mProtoActions.getNumber(); }
      float                getOverallRange(BEntityID sourceUnit, int squadMode) const;
      void                 updateOverallRanges();
      void                 updateAttackRatings(BPlayer* pPlayer);
      bool                 getHasAttackRatings() const { return (mAttackRatingDPS.getSize()>0); }
      float                getAttackRatingDPS(BDamageTypeID damageType) const;
      float                getAttackRatingDPS() const;
      float                getAttackRating(BDamageTypeID damageType) const;
      float                getAttackRating() const;


      // Static data access
      long                 getProtoActionID(const BSimString &protoActionName);
      void                 setDefaultActionID(long actionID) { mpStaticData->mDefaultActionID = actionID; }
      void                 setTrampleActionID(long actionID) { mpStaticData->mTrampleActionID = actionID; }
      const BString&       getName() const { return mpStaticData->mName; }
      long                 getNumberPersistentActions() const { return mpStaticData->mPersistentActions.getNumber(); }
      BProtoAction*        getPersistentAction(long index) { return getProtoAction(mpStaticData->mPersistentActions[index]); }
      long                 getNumberPersistentSquadActions() const { return mpStaticData->mPersistentSquadActions.getNumber(); }
      BProtoAction*        getPersistentSquadAction(long index) { return getProtoAction(mpStaticData->mPersistentSquadActions[index]); }
      void                 addPersistenAction(long protoActionID) { mpStaticData->mPersistentActions.add(protoActionID); }
      void                 addPersistenSquadAction(long protoActionID) { mpStaticData->mPersistentSquadActions.add(protoActionID); }
      bool                 canAttack() const { return getFlagCanAttack(); }
      bool                 canGather() const { return getFlagCanGather(); }
      bool                 canGatherSupplies() const { return getFlagCanGatherSupplies(); }
      bool                 canBuild() const { return getFlagCanBuild(); }
      bool                 canRepair() const { return getFlagCanRepair(); }
      bool                 canAutoRepair() const { return getFlagCanAutoRepair(); }
      bool                 can(uint state, BActionType actionType) const;
      bool                 canAttackTarget(BEntityID targetID, const BProtoAction* pProtoAction);
      bool                 canAttackTargetType(const BProtoObject* pTargetProto, const BProtoAction* pProtoAction);
      bool                 hasEnabledAttackAction(bool allowSecondary) const;
      bool                 getAnimInfoLoaded() const { return(mAnimInfoLoaded); }
      void                 setAnimInfoLoaded(bool v) { mAnimInfoLoaded=v; }
      BProtoAction*        getShockwaveAction() const;

      // Target Rules   
      long                 getNumberTargetRules() const { return (mpStaticData->mTargetRules.getNumber()); }
      const BTargetRule*   getTargetRule(long index);
      int                  getAbilityID(int actionID);

      //States.
      uint                 getNumberStates() const { return (mpStaticData->mStates.getSize()); }
      const BTacticState*  getState(uint index) const { if (index >= mpStaticData->mStates.getSize()) return (NULL); return (&(mpStaticData->mStates[index])); }

      // Exposed Actions
      BProtoAction*        getExposedAction(int exposedIndex, int& actionID);

      // Trample Action
      BProtoAction*        getTrampleAction();

      // Flags
      bool                 getFlagOwnStaticData() const { return(mFlagOwnStaticData); }
      bool                 getFlagCanAttack() const { return(mFlagCanAttack); }
      bool                 getFlagCanGather() const { return(mFlagCanGather); }
      bool                 getFlagCanGatherSupplies() const { return (mFlagCanGatherSupplies); }
      bool                 getFlagCanBuild() const { return(mFlagCanBuild); }
      bool                 getFlagCanRepair() const { return(mFlagCanRepair); }
      bool                 getFlagLoaded() const { return(mFlagLoaded); }
      bool                 getFlagCanAutoRepair() const { return(mFlagCanAutoRepair); }
      
      void                 setProtoObjectID(BProtoObjectID id) { mProtoObjectID=id; }
      BProtoObjectID       getProtoObjectID() const { return mProtoObjectID; }

      void                 setPlayerID(BPlayerID id) { mPlayerID=(int8)id; }
      BPlayerID            getPlayerID() const { return (BPlayerID)mPlayerID; }

      GFDECLAREVERSION();
      bool save(BStream* pStream, int saveType) const;
      bool load(BStream* pStream, int saveType, BPlayer* pPlayer, BProtoObjectID protoObjectID);

   protected:
      struct BStateLoadHelper
      {
         BTacticState*            state;
         BDynamicArray<BString>   actions;
      };

      bool                 loadWeapons(BXMLNode root, const BProtoObject *pObj);
      bool                 loadStates(BXMLNode root, BDynamicArray<BStateLoadHelper>& loadHelpers);
      bool                 loadProtoActions(BXMLNode root);
      bool                 loadTacticRules(BXMLNode root);
      bool                 loadTargetRule(BXMLNode root);
      bool                 loadTacticState(BXMLNode root, BStateLoadHelper& loadHelper);
      bool                 loadStateActions(const BDynamicArray<BStateLoadHelper>& actions);
      
      // Dynamic data
      BDynamicSimArray<BWeapon *>         mWeapons;
      BDynamicSimArray<BProtoAction *>    mProtoActions;
      BSmallDynamicSimArray<float>        mAttackRatingDPS;
      float                               mOverallRange[BSquadAI::cNumberModes];      

      // Static data
      BTacticStatic*                      mpStaticData;

      BProtoObjectID                      mProtoObjectID;
      int8                                mPlayerID;

      // Flags
      bool                                mFlagOwnStaticData:1;
      bool                                mFlagCanAttack:1;
      bool                                mFlagCanGather:1;
      bool                                mFlagCanGatherSupplies:1;
      bool                                mFlagCanBuild:1;
      bool                                mFlagCanRepair:1;
      bool                                mFlagCanAutoRepair:1;
      bool                                mFlagLoaded:1;      
      bool                                mAnimInfoLoaded:1;
};

#define GFWRITEPROTOACTIONPTR(stream,varname) { if (!BProtoAction::savePtr(stream, varname)) return false; }
#define GFREADPROTOACTIONPTR(stream,varname) { if (!BProtoAction::loadPtr(stream, &(varname))) return false; }


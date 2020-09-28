//==============================================================================
// powerrage.h
//
// Copyright (c) 2008 Ensemble Studios
//==============================================================================
#pragma once

// xgame
#include "power.h"
#include "cost.h"
#include "parametricsplinecurve.h"
#include "cameraEffectManager.h"

class BCamera;
class BProtoAction;

//==============================================================================
// class BPowerUserRage
// ASYNC stuff (only on one BUser in the game.)
//==============================================================================
class BPowerUserRage : public BPowerUser
{
public:

   // Handles the Asynchronous UI stuff from BUser for the power.
   virtual bool init(BProtoPowerID protoPowerID, BPowerLevel powerLevel, BUser* pUser, BEntityID ownerSquadID, BVector targetLocation, bool noCost = false);
   virtual bool shutdown();
   virtual void update(float elapsedTime);
   virtual bool handleInput(long port, long event, long controlType, BInputEventDetail& detail);

   // Define these if you want to override the default render behaviors.
   virtual void updateUI();
//   virtual void renderUI() {}
   virtual bool shouldClampCamera() const { return false; }

   virtual bool save(BStream* pStream, int saveType) const;
   virtual bool load(BStream* pStream, int saveType);

private:
   virtual void setupUser();

   void getCameraRelativeInputDir(BCamera& camera, float inputX, float inputY, BVector& outDir);
   bool shouldForceUpdate(const BVector& oldDir, const BVector& newDir);

   float mCameraZoom;
   float mCameraHeight;

   DWORD mTimestampNextCommand;
   BVector mMoveInputDir;
   BVector mAttackInputDir;
   float mTimeUntilHint;

   BVector mLastMovePos;
   BVector mLastMoveDir;
   BVector mLastAttackDir;

   DWORD mCommandInterval;
   float mScanRadius;
   float mMovementProjectionMultiplier;

   bool mHasMoved:1;
   bool mHasAttacked:1;
   bool mHintShown:1;
   bool mHintCompleted:1;
   bool mForceCommandNextUpdate:1;
};


//==============================================================================
// class BPowerRage
// SYNC stuff in the sim.
//==============================================================================
class BPowerRage : public BPower, IEventListener
{
public:

   BPowerRage() { mType = PowerType::cRage; }

   // This will be called on all machines.
   virtual bool init(BPlayerID playerID, BPowerLevel powerLevel, BPowerUserID powerUserID, BEntityID ownerSquadID, BVector targetLocation, bool ignoreAllReqs = false);
   virtual bool shutdown();
   virtual bool submitInput(BPowerInput powerInput);
   virtual void update(DWORD currentGameTime, float lastUpdateLength);

/*
   virtual BProtoAction* getProtoAction(void) const {return(NULL);}
*/

   virtual void notify(DWORD eventType, BEntityID senderID, DWORD data, DWORD data2);   
   virtual int getEventListenerType() const { return cEventListenerTypePower; }
   virtual bool savePtr(BStream* pStream) const;

   bool getHasSuccessfullyAttacked() const { return mHasSuccessfullyAttacked; }
   float getScanRadius() const { return (mScanRadius); }

   bool isAttackingTarget(bool& boarded) const;
   bool isJumping() const;

   virtual bool save(BStream* pStream, int saveType) const;
   virtual bool load(BStream* pStream, int saveType);

   virtual void projectileImpact(BEntityID id, const BEntityIDArray& killedUnits, const BEntityIDArray& damagedUnits);

private:
   bool getNewTarget();
   BProtoAction* getImpactProtoAction() const;
   const BCost* getCost() const;
   void doDirectionalCameraBlurForOwner(const BVector& startWorldPos, const BVector& endWorldPos);
   void onUnitKilled(const BEntityID& unitId);

   void updateAura(const BVector& location);
   void handleSquadsLeavingAura(const BEntityIDArray& squads);
   void handleSquadsEnteringAura(const BEntityIDArray& squads);

   double mNextTickTime;
   BEntityID mTargettedSquad;
   BVector mLastDirectionInput;
   BVector mTeleportDestination;
   BVector mPositionInput;
   float mTimeUntilTeleport;
   float mTimeUntilRetarget;
   BCueIndex mAttackSound;
   BParametricSplineCurve mJumpSplineCurve;
   BCameraEffectData mCameraEffectData;

   BCost mCostPerTick;
   BCost mCostPerTickAttacking;
   BCost mCostPerJump;
   float mTickLength;
   float mDamageMultiplier;
   float mDamageTakenMultiplier;
   float mSpeedMultiplier;
   float mNudgeMultiplier;
   float mScanRadius;
   BProtoObjectID mProjectileObject;
   BProtoObjectID mHandAttachObject;
   BProtoObjectID mTeleportAttachObject;
   float mAudioReactionTimer;
   float mTeleportTime;
   float mTeleportLateralDistance;
   float mTeleportJumpDistance;
   float mTimeBetweenRetarget;
   float mMotionBlurAmount;
   float mMotionBlurDistance;
   float mMotionBlurTime;
   float mDistanceVsAngleWeight;
   float mHealPerKillCombatValue; 
   float mAuraRadius;
   float mAuraDamageBonus;
   BProtoObjectID mAuraAttachObjectSmall;
   BProtoObjectID mAuraAttachObjectMedium;
   BProtoObjectID mAuraAttachObjectLarge;
   BProtoObjectID mHealAttachObject;
   BEntityIDArray mSquadsInAura;
   BObjectTypeID mFilterTypeID;

   bool mCompletedInitialization:1;
   bool mHasSuccessfullyAttacked:1;
   bool mUsePather:1;
};
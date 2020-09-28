//==============================================================================
// powerWave.h
//
// Copyright (c) 2008 Ensemble Studios
//==============================================================================
#pragma once

// xgame
#include "power.h"
#include "cost.h"
#include "cameraEffectManager.h"
#include "set.h"

class BCamera;
class BProtoAction;
class BUnit;
class BObject;
class BSquad;
class BPhysicsObject;

//==============================================================================
// class BWaveGravityBall
// gravity ball class to replicate behavior synchronously and async
//==============================================================================
class BWaveGravityBall
{
public:
   enum EState
   {
      cNone,
      cStagnant,
      cPulling,
      cPullingFull,
      cExploding
   };

   BWaveGravityBall() { init(); }
   ~BWaveGravityBall() { shutdown(); }

   void init();
   void shutdown();

   void createBall(BPlayerID playerId, const BVector& location, BProtoObjectID ballType, bool sync);
   BObject* getBallObject() const;
   
   void setState(EState newState);
   EState getState() const { return mState; }

   bool isPulling() const { return (mState == cPulling || mState == cPullingFull); }

private:
   BEntityID      mBallID;
   EState         mState;
};

//==============================================================================
// class BPowerUserWave
// ASYNC stuff (only on one BUser in the game.)
//==============================================================================
class BPowerUserWave : public BPowerUser
{
public:

   // Handles the Asynchronous UI stuff from BUser for the power.
   virtual bool init(BProtoPowerID protoPowerID, BPowerLevel powerLevel, BUser* pUser, BEntityID ownerSquadID, BVector targetLocation, bool noCost = false);
   virtual bool shutdown();
   virtual void update(float elapsedTime);
   virtual bool handleInput(long port, long event, long controlType, BInputEventDetail& detail);
   virtual void cancelPower();

   // Define these if you want to override the default render behaviors.
   virtual void updateUI();
   //   virtual void renderUI() {}
   virtual bool shouldClampCamera() const { return false; }

   void onGravityBallStateSet(BWaveGravityBall::EState newState);
   void onPickupObject();

   virtual bool save(BStream* pStream, int saveType) const;
   virtual bool load(BStream* pStream, int saveType);

   bool isPowerValidAtLocataion(const BVector& location);

protected:
   virtual void setupUser();

   float getBallSpeed() const;
   void updateFakeBall(float elapsedTime);
   void updateCamera();
   void getCameraRelativeInputDir(BCamera& camera, float inputX, float inputY, BVector& outDir);

   BWaveGravityBall mFakeGravityBall;
   BVector mHorizontalMoveInputDir;
   BVector mVerticalMoveInputDir;
   BVector mLastUpdatePos;
   BVector mCameraFocusPoint;

   DWORD mTimestampNextCommand;
   float mTimeUntilHint;
   float mDelayShutdownTimeLeft;

   DWORD mCommandInterval;
   float mMinBallDistance;
   float mMaxBallDistance;
   float mMaxBallSpeedStagnant;
   float mMaxBallSpeedPulling;
   float mCameraDistance;
   float mCameraHeight;
   float mCameraHoverPointDistance;
   float mCameraMaxBallAngle;
   float mPullingRange;
   float mPickupShakeDuration;
   float mPickupRumbleShakeStrength;
   float mPickupCameraShakeStrength;
   float mMaxBallHeight;
   float mMinBallHeight;
   float mExplodeTime;

   bool mHintShown:1;
   bool mHintCompleted:1;
   bool mShuttingDown:1;
};


//==============================================================================
// class BPowerWave
// SYNC stuff in the sim.
//==============================================================================
class BPowerWave : public BPower, IEventListener
{
public:

   BPowerWave() { mType = PowerType::cWave; }

   // This will be called on all machines.
   virtual bool init(BPlayerID playerID, BPowerLevel powerLevel, BPowerUserID powerUserID, BEntityID ownerSquadID, BVector targetLocation, bool ignoreAllReqs = false);
   virtual bool shutdown();
   virtual bool submitInput(BPowerInput powerInput);
   virtual void update(DWORD currentGameTime, float lastUpdateLength);

   virtual void notify(DWORD eventType, BEntityID senderID, DWORD data, DWORD data2);   

   /*
   virtual BProtoAction* getProtoAction(void) const {return(NULL);}

   virtual void projectileImpact(BEntityID id) {};
   */

   virtual int getEventListenerType() const { return cEventListenerTypePower; }
   virtual bool savePtr(BStream* pStream) const;

   virtual bool save(BStream* pStream, int saveType) const;
   virtual bool load(BStream* pStream, int saveType);

   DWORD getCommandInterval() const { return (mCommandInterval); }
   float getMinBallDistance() const { return (mMinBallDistance); }
   float getMaxBallDistance() const { return (mMaxBallDistance); }
   const BWaveGravityBall& getRealGravityBall() const { return (mRealGravityBall); }

protected:
   BPowerUserWave* getPowerUser() const;
   void attemptShutdown();
   void updateRevealers(BUnit& ownerUnit, const BObject& ballObject);

   float getBallSpeed() const;
   void setGravityBallState(BWaveGravityBall::EState newState);
   void ripPartOffUnit(BUnit& unit);
   void hitLightning(BSquad& squad);
   void nudgeUnit(BUnit& unit, const BVector& strikeLocation);
   bool captureUnit(BUnit& unit);
   
   void updateQueuedObjects();
   void validateCapturedObjects();
   void queueAddObject(BObject* pObject);
   void addObjectToBall(BObject* pObject);
   void grabFakeObject(BObject* pObject);

   void startStagnant();

   void startPulling();
   void updatePulling(float elapsedTime);
   bool attemptPullUnit(BUnit& unit, const BEntityID& leaderUnitId, const BEntityID& ballUnitId, const BVector& ballPosition);
   bool updateLightning(const BEntityIDArray& unitsToTarget);
   void pullLiveUnits();
   void pullCorpses();
   void failPulling();
   bool physicsValidForPulling(const BPhysicsObject* pPO);

   void explode();

   double mNextTickTime;
   BWaveGravityBall mRealGravityBall;
   BVector mDesiredBallPosition;
   BSet<BEntityID> mCapturedUnits;
   float mExplodeCooldownLeft;
   BEntityIDArray mUnitsToPull;
   float mCurrentExplosionDamageBank;
   float mMaxPossibleExplosionDamageBank;
   BDynamicSimArray<BTeamID>  mRevealedTeamIDs;

   struct BQueuedObject
   {
      BEntityID   ObjectID;
      float       AddTime;
   };
   BSmallDynamicSimArray<BQueuedObject> mQueuedPickupObjects;

   BCost mCostPerTick;
   float mTickLength;
   BProtoObjectID mBallProtoID;
   BProtoObjectID mLightningProtoID;
   BProtoObjectID mLightningBeamVisualProtoID;
   BProtoObjectID mDebrisProtoID;
   BProtoObjectID mExplodeProtoID;
   BProtoObjectID mPickupAttachmentProtoID;
   float mAudioReactionTimer;
   uint mLeaderAnimOrderID;
   float mMaxBallSpeedStagnant;
   float mMaxBallSpeedPulling;
   float mExplodeTime;
   float mPullingRange;
   float mExplosionForceOnDebris;
   float mHealthToCapture;
   float mNudgeStrength;
   float mInitialLateralPullStrength;
   float mCapturedRadialSpacing;
   float mCapturedSpringStrength;
   float mCapturedSpringDampening;
   float mCapturedSpringRestLength;
   float mCapturedMinLateralSpeed;
   float mRipAttachmentChancePulling;
   float mPickupObjectRate;
   float mDebrisAngularDamping;
   float mMaxExplosionDamageBankPerCaptured;
   float mExplosionDamageBankPerTick;
   DWORD mCommandInterval;
   float mMinBallDistance;
   float mMaxBallDistance;
   int mMaxCapturedObjects;
   int mLightningPerTick;
   byte mNudgeChancePulling;
   byte mThrowPartChancePulling;
   byte mLightningChancePulling;
   BCueIndex mExplodeSound;
   float mMinDamageBankPercentToThrow;

   BProtoAction* mpExplodeProtoAction;
   BProtoAction* mpLightningProtoAction;
   BProtoAction* mpDebrisProtoAction;

   bool mCompletedInitialization:1;
   bool mThrowUnitsOnExplosion:1;
};
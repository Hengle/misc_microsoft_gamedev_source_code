//==============================================================================
// powerorbital.h
//
// Copyright (c) 2008 Ensemble Studios
//==============================================================================
#pragma once

#include "power.h"
#include "powerhelper.h"
//#include "cost.h"


//==============================================================================
// class BPowerUserOrbital
// ASYNC stuff (only on one BUser in the game.)
//==============================================================================
class BPowerUserOrbital : public BPowerUser
{
public:

   // This will only be called on one machine (as if it were in BUser) so don't do sync unsafe stuff in it.
   virtual bool init(BProtoPowerID protoPowerID, BPowerLevel powerLevel, BUser* pUser, BEntityID ownerSquadID, BVector targetLocation, bool noCost = false);
   virtual bool shutdown();
   virtual bool handleInput(long port, long event, long controlType, BInputEventDetail& detail);
   virtual void update(float elapsedTime);
   // Define these if you want to override the default render behaviors.
   //virtual void render() {}
   virtual void updateUI();
   virtual void renderUI();

   void onShotFired(bool succeeded);
   void setRealTargettingLaserID( const BEntityID& id) { mRealTargettingLaserID = id; }

   virtual bool save(BStream* pStream, int saveType) const;
   virtual bool load(BStream* pStream, int saveType);

protected:
   virtual void setupUser();

   BPowerHelper::BHUDSounds mHudSounds;

   BUString  mHelpString;
   BEntityID mFakeTargettingLaserID;
   BEntityID mRealTargettingLaserID;
   BEntityID mTargettedSquadID;
   uint mShotsRemaining;
   float mLastCommandSent;   
   float mCommandInterval;
   float mLastShotSent;
   float mShotInterval;
   long mLOSMode;

   // Halwes - 8/19/2008 - HACK!  Scenario 6 has a special needs version of the orbital bombard with some pesky camera requirements so I need to shutdown
   // some of the camera controls to keep the camera at its initial height.
   bool mFlagLastCameraAutoZoomInstantEnabled:1;
   bool mFlagLastCameraAutoZoomEnabled:1;
   bool mFlagLastCameraZoomEnabled:1;
   bool mFlagLastCameraYawEnabled:1;
};


//==============================================================================
// class BOrbitalShotInfo
// Helper / container class
//==============================================================================
class BOrbitalShotInfo
{
public:
   bool save(BStream* pStream, int saveType) const;
   bool load(BStream* pStream, int saveType);

   BOrbitalShotInfo();
   BVector mLaunchPos;     // Projectile start pos.
   BVector mTargetPos;     // Projectile end pos.
   DWORD mLaunchTime;      // When do we launch the projectile?  (In case we want a delay.)
   DWORD mCreateLaserTime; // When do we create the laser?  (In case we want a delay.)
   BEntityID mLaserObj;    // The targeting laser object.
   bool mbLaserCreated;    // Did we create the laser?
};


//==============================================================================
// class BPowerOrbital
// SYNC stuff in the sim.
//==============================================================================
class BPowerOrbital : public BPower
{
public:

   BPowerOrbital() { mType = PowerType::cOrbital; }

   // This will be called on all machines.
   virtual bool init(BPlayerID playerID, BPowerLevel powerLevel, BPowerUserID powerUserID, BEntityID ownerSquadID, BVector targetLocation, bool ignoreAllReqs = false);
   virtual bool shutdown();
   virtual bool submitInput(BPowerInput powerInput);
   virtual void update(DWORD currentGameTime, float lastUpdateLength);

   virtual BProtoAction* getProtoAction(void) const;

   BEntityID getRealTargetingLaserID() const { return (mRealTargettingLaserID); }

   uint getShotsRemaining() const { return (mShotsRemaining); }
   uint getNumShotsBeingProcessed() const { return (mShots.getSize()); }

   virtual void projectileImpact(BEntityID id, const BEntityIDArray& killedUnits, const BEntityIDArray& damagedUnits);

   virtual bool save(BStream* pStream, int saveType) const;
   virtual bool load(BStream* pStream, int saveType);

protected:

   BEntityID mRealTargettingLaserID;    
   BVector mDesiredTargettingPosition;
   BSmallDynamicSimArray<BOrbitalShotInfo> mShots;
   bool mFiredInitialShot:1;

   // Cached off from proto power
   uint mShotsRemaining;
   uint mImpactsToProcess;
   BProtoObjectID mTargetBeamID;
   BProtoObjectID mProjectileID;
   BProtoObjectID mEffectProtoID;
   BProtoObjectID mRockSmallProtoID;
   BProtoObjectID mRockMediumProtoID;
   BProtoObjectID mRockLargeProtoID;
   BCueIndex mFiredSound;
   DWORD mTargetingDelay;
   DWORD mAutoShotDelay;
   float mAutoShotInnerRadius;
   float mAutoShotOuterRadius;
   float mXOffset;
   float mYOffset;
   float mZOffset;
   long mLOSMode;   
};
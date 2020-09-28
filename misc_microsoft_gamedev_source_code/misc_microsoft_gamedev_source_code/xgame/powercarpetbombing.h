//==============================================================================
// powercarpetbombing.h
//
// Copyright (c) 2008 Ensemble Studios
//==============================================================================
#pragma once

#include "power.h"
#include "cost.h"
#include "powerhelper.h"
#include "set.h"

//==============================================================================
// class BPowerUserCarpetBombing
// ASYNC stuff (only on one BUser in the game.)
//==============================================================================
class BPowerUserCarpetBombing : public BPowerUser
{
public:

   enum EInputState
   {
      cWaitingForLocation,
      cWaitingForDirection,
      cDone,
   };

   // This will only be called on one machine (as if it were in BUser) so don't do sync unsafe stuff in it.
   virtual bool   init(BProtoPowerID protoPowerID, BPowerLevel powerLevel, BUser* pUser, BEntityID ownerSquadID, BVector targetLocation, bool noCost = false);
   virtual bool   shutdown();
   virtual void   update(float elapsedTime);
   virtual bool   handleInput(long port, long event, long controlType, BInputEventDetail& detail);
   virtual void   renderUI();

   virtual bool save(BStream* pStream, int saveType) const;
   virtual bool load(BStream* pStream, int saveType);

protected:
   virtual void setupUser();

   EInputState    mInputState;
   BVector        mPosition;
   BVector        mDesiredForward;
   float          mDesiredScale;
   float          mCurrentScale;
   double         mShutdownTime;
   BEntityID      mArrowID;
   float          mMaxBombOffset;
   float          mLengthMultiplier;
   long           mLOSMode;

   // Cached from proto power
   BProtoObjectID mArrowProtoID;
   BPowerHelper::BHUDSounds     mHudSounds;
};

//==============================================================================
class BBombExplodeInfo
{
public:

   BVector        mPosition;
   double         mExplodeTime;
};

//==============================================================================
// class BPowerCarpetBombing
// SYNC stuff in the sim.
//==============================================================================
class BPowerCarpetBombing : public BPower
{
public:

   enum EState
   {
      cWaitingForInputs,
      cActive,
   };
   
   BPowerCarpetBombing() { mType = PowerType::cCarpetBombing; }

   // This will be called on all machines.
   virtual bool init(BPlayerID playerID, BPowerLevel powerLevel, BPowerUserID powerUserID, BEntityID ownerSquadID, BVector targetLocation, bool ignoreAllReqs = false);
   virtual bool shutdown();
   virtual bool submitInput(BPowerInput powerInput);
   virtual void update(DWORD currentGameTime, float lastUpdateLength);
   virtual void setState(EState newState);
   virtual void projectileImpact(BEntityID id, const BEntityIDArray& killedUnits, const BEntityIDArray& damagedUnits);

   virtual bool save(BStream* pStream, int saveType) const;
   virtual bool load(BStream* pStream, int saveType);

protected:

   //BSmallDynamicSimArray<BVector> mWaypoints;
   BSmallDynamicSimArray<BBombExplodeInfo> mBombExplodeInfos;
   BSet<BEntityID> mNudgedUnits;

   BVector     mStartLocation;
   BVector     mStartDirection;
   BVector     mRightVector;

   EState      mState;

   bool        mGotStartLocation:1;
   bool        mGotStartDirection:1;

   double      mTickLength;
   double      mNextBombTime;
   double      mLastBombTime;
   uint        mNumBombClustersDropped;

   // Cached from proto power
   BProtoObjectID mProjectileProtoID;
   BProtoObjectID mImpactProtoID;
   BProtoObjectID mExplosionProtoID;
   BProtoObjectID mBomberProtoID;
   float mInitialDelay;
   float mFuseTime;
   uint mMaxBombs;
   float mMaxBombOffset;
   float mBombSpacing;
   float mLengthMultiplier;
   float mWedgeLengthMultiplier;
   float mWedgeMinOffset;
   float mNudgeMultiplier;
   BPowerHelper::BBomberData mBomberData;
   long mLOSMode;
   bool mReactionPlayed;
};
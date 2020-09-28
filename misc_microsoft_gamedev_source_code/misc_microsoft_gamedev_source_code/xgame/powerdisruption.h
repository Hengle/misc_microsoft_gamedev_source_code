//==============================================================================
// powerDisruptionl.h
//
// Copyright (c) 2008 Ensemble Studios
//==============================================================================
#pragma once


// xgame
#include "power.h"
#include "powerhelper.h"

//==============================================================================
// class BPowerUserDisruption
// ASYNC stuff (only on one BUser in the game.)
//==============================================================================
class BPowerUserDisruption : public BPowerUser
{
public:

   // This will only be called on one machine (as if it were in BUser) so don't do sync unsafe stuff in it.
   virtual bool init(BProtoPowerID protoPowerID, BPowerLevel powerLevel, BUser* pUser, BEntityID ownerSquadID, BVector targetLocation, bool noCost = false);
   virtual bool shutdown();
   virtual bool handleInput(long port, long event, long controlType, BInputEventDetail& detail);
   virtual void update(float elapsedTime) { mElapsed += elapsedTime; }
   // Define these if you want to override the default render behaviors.
   virtual void renderUI();

   virtual bool save(BStream* pStream, int saveType) const;
   virtual bool load(BStream* pStream, int saveType);

protected:
   virtual void setupUser();

   bool canFire();

   long mLOSMode;
   BProtoObjectID mDisruptionObjectProtoID;
   BPowerHelper::BHUDSounds mHudSounds;
};


//==============================================================================
// class BPowerDisruption
// SYNC stuff in the sim.
//==============================================================================
class BPowerDisruption : public BPower
{
public:

   BPowerDisruption() { mType = PowerType::cDisruption; }

   // This will be called on all machines.
   virtual bool init(BPlayerID playerID, BPowerLevel powerLevel, BPowerUserID powerUserID, BEntityID ownerSquadID, BVector targetLocation, bool ignoreAllReqs = false);
   virtual bool shutdown();
   virtual bool submitInput(BPowerInput powerInput);
   virtual void update(DWORD currentGameTime, float lastUpdateLength);

   bool isActive() const { return mElapsed >= mDisruptionStartTime; }
   float getRadius() const { return mDisruptionRadius; }
   float getRadiusSqr() const { return mDisruptionRadiusSqr; }

   void strikeLocation(const BVector& targetLocation, BUser* pUserCameraControl = NULL) const;

   virtual bool save(BStream* pStream, int saveType) const;
   virtual bool load(BStream* pStream, int saveType);

protected:

   BEntityID mDisruptionObjectID;
   float mDisruptionRadius;
   float mDisruptionRadiusSqr;
   float mTimeRemainingSec;
   float mDisruptionStartTime;
   float mNextPulseTime;
   float mPulseSpacing;
   int mNumPulses;
   BVector mDirection;
   BVector mRight;
   BCueIndex mPulseSound; 

   BProtoObjectID mBomberProtoID;
   BProtoObjectID mDisruptionObjectProtoID;
   BProtoObjectID mPulseObjectProtoID;
   BProtoObjectID mStrikeObjectProtoID;
   BPowerHelper::BBomberData mBomberData;
};
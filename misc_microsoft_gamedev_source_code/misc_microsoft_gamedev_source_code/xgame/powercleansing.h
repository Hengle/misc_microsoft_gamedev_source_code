//==============================================================================
// powercleansing.h
//
// Copyright (c) 2008 Ensemble Studios
//==============================================================================
#pragma once

#include "power.h"
#include "cost.h"

class BSquad;
class BUnit;

//==============================================================================
// class BPowerUserCleansing
// ASYNC stuff (only on one BUser in the game.)
//==============================================================================
class BPowerUserCleansing : public BPowerUser
{
public:

   // This will only be called on one machine (as if it were in BUser) so don't do sync unsafe stuff in it.
   virtual bool init(BProtoPowerID protoPowerID, BPowerLevel powerLevel, BUser* pUser, BEntityID ownerSquadID, BVector targetLocation, bool noCost = false);
   virtual bool shutdown();
   virtual bool handleInput(long port, long event, long controlType, BInputEventDetail& detail);
   virtual void update(float elapsedTime);
   virtual long getCurrentReticle() { return -1; }
   // Define these if you want to override the default render behaviors.
   //virtual void render() {}
   //virtual void renderUI() {}

   void setRealBeamID(BEntityID realBeamID) { mRealBeamID = realBeamID; }

   virtual bool save(BStream* pStream, int saveType) const;
   virtual bool load(BStream* pStream, int saveType);

protected:
   virtual void setupUser();

   BVector mInputDir;
   BVector mLastUpdatePos;
   BEntityID mRealBeamID;
   BEntityID mFakeBeamID;
   BEntityID mAirImpactObjectID;
   DWORD mTimestampNextCommand;
   DWORD mCommandInterval;
   float mMinBeamDistance;
   float mMaxBeamDistance;
   float mMaxBeamSpeed;
   long mLOSMode;
};

//==============================================================================
// class BPowerCleansing
// SYNC stuff in the sim.
//==============================================================================
class BPowerCleansing : public BPower
{
public:

   
   BPowerCleansing() { mType = PowerType::cCleansing; mFlagUsePath = false; }
   
   // This will be called on all machines.
   virtual bool init(BPlayerID playerID, BPowerLevel powerLevel, BPowerUserID powerUserID, BEntityID ownerSquadID, BVector targetLocation, bool ignoreAllReqs = false);
   virtual bool shutdown();
   virtual bool submitInput(BPowerInput powerInput);
   virtual void update(DWORD currentGameTime, float lastUpdateLength);

   BEntityID getBeamID() const;
   const BSquad* getBeamSquad() const;
   const BUnit* getBeamUnit() const;
   float getMinBeamDistance() const { return (mMinBeamDistance); }
   float getMaxBeamDistance() const { return (mMaxBeamDistance); }
   float getMaxBeamSpeed() const { return (mMaxBeamSpeed); }
   DWORD getCommandInterval() const { return (mCommandInterval); }

   static void updateAirImpactObject(BPlayerID playerID, BProtoObjectID protoObjectID, BVector pos, BEntityID& airImpactObjectID, bool noRenderForOwner);

   // Use path
   void setFlagUsePath(bool usePath) { mFlagUsePath = usePath; }
   bool getFlagUsePath() const { return (mFlagUsePath); }

   static bool canSquadUsePower(const BSquad& squad);

   virtual bool save(BStream* pStream, int saveType) const;
   virtual bool load(BStream* pStream, int saveType);

protected:
   BSmallDynamicSimArray<BVector> mWaypoints;

   BEntityID   mBeamID;
   BEntityID   mAirImpactObjectID;
   double      mNextDamageTime;
   uint        mLeaderAnimOrderID;
   BVector     mDesiredBeamPosition;
   BVectorArray mBeamPath;
   BDynamicSimArray<BTeamID>  mRevealedTeamIDs;

   // Cached from protopower
   BCost mCostPerTick;
   BProtoObjectID mProjectile;
   float mTickLength;
   float mMinBeamDistance;
   float mMaxBeamDistance;
   DWORD mCommandInterval;
   float mMaxBeamSpeed;   
   long mLOSMode;

   bool mFlagUsePath:1;
   float mAudioReactionTimer;
};

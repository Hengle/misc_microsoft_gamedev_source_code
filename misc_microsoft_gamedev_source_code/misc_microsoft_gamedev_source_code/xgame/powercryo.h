//==============================================================================
// powerCryol.h
//
// Copyright (c) 2008 Ensemble Studios
//==============================================================================
#pragma once


// xgame
#include "power.h"
#include "powerhelper.h"


// Typedefs
typedef std::pair<BEntityID, DWORD> BEntityTimePair;
typedef BSmallDynamicSimArray<BEntityTimePair> BEntityTimePairArray;


//==============================================================================
// class BPowerUserCryo
// ASYNC stuff (only on one BUser in the game.)
//==============================================================================
class BPowerUserCryo : public BPowerUser
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

   BPowerHelper::BHUDSounds mHudSounds;
};


//==============================================================================
// class BPowerCryo
// SYNC stuff in the sim.
//==============================================================================
class BPowerCryo : public BPower
{
public:

   BPowerCryo() { mType = PowerType::cCryo; }

   // This will be called on all machines.
   virtual bool init(BPlayerID playerID, BPowerLevel powerLevel, BPowerUserID powerUserID, BEntityID ownerSquadID, BVector targetLocation, bool ignoreAllReqs = false);
   virtual bool shutdown();
   virtual bool submitInput(BPowerInput powerInput);
   virtual void update(DWORD currentGameTime, float lastUpdateLength);

   virtual bool save(BStream* pStream, int saveType) const;
   virtual bool load(BStream* pStream, int saveType);

protected:

   void ignoreSquad(BEntityID squadID, DWORD expirationTime);
   void clearExpiredIgnores(DWORD currentGameTime);
   bool isSquadIgnored(BEntityID squadID) const;

   BEntityTimePairArray mIgnoreList;   // Squads who can't Cryo by this power until a certain time.

   DWORD mNextTickTime;

   BEntityID mCryoObjectID;
   BVector mDirection;
   BVector mRight;

   BProtoObjectID mCryoObjectProtoID;
   BProtoObjectID mBomberProtoID;
   BObjectTypeID mFilterTypeID;
   float mCryoRadius;
   float mMinCryoFalloff;
   DWORD mTickDuration;
   uint mTicksRemaining;
   float mCryoAmountPerTick;
   float mKillableHpLeft;
   float mFreezingThawTime;
   float mFrozenThawTime;
   BPowerHelper::BBomberData mBomberData;
   bool mReactionPlayed:1;
};
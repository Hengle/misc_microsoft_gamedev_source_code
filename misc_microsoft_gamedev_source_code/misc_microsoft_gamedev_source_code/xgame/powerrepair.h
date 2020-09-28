//==============================================================================
// powerrepairl.h
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
// class BPowerUserRepair
// ASYNC stuff (only on one BUser in the game.)
//==============================================================================
class BPowerUserRepair : public BPowerUser
{
public:

   // This will only be called on one machine (as if it were in BUser) so don't do sync unsafe stuff in it.
   virtual bool init(BProtoPowerID protoPowerID, BPowerLevel powerLevel, BUser* pUser, BEntityID ownerSquadID, BVector targetLocation, bool noCost = false);
   virtual bool shutdown();
   virtual bool handleInput(long port, long event, long controlType, BInputEventDetail& detail);
   virtual void update(float elapsedTime) { mElapsed += elapsedTime; }
   // Define these if you want to override the default render behaviors.
   //virtual void render() {}
   virtual void renderUI();

   virtual bool save(BStream* pStream, int saveType) const;
   virtual bool load(BStream* pStream, int saveType);

protected:
   virtual void setupUser();

   BPowerHelper::BHUDSounds mHudSounds;
};


//==============================================================================
// class BPowerRepair
// SYNC stuff in the sim.
//==============================================================================
class BPowerRepair : public BPower
{
public:

   BPowerRepair() { mType = PowerType::cRepair; }

   // This will be called on all machines.
   virtual bool init(BPlayerID playerID, BPowerLevel powerLevel, BPowerUserID powerUserID, BEntityID ownerSquadID, BVector targetLocation, bool ignoreAllReqs = false);
   virtual bool shutdown();
   virtual bool submitInput(BPowerInput powerInput);
   virtual void update(DWORD currentGameTime, float lastUpdateLength);
   long getCurrentRepairCount() { return mSquadsRepairing.size(); }

   virtual bool save(BStream* pStream, int saveType) const;
   virtual bool load(BStream* pStream, int saveType);

protected:

   void handleArrivingSquads(const BEntityIDArray& squadIDs);
   void handleLeavingSquads(const BEntityIDArray& squadIDs);
   void ignoreSquad(BEntityID squadID, DWORD expirationTime);
   void clearExpiredIgnores(DWORD currentGameTime);
   bool isSquadIgnored(BEntityID squadID) const;

   BEntityIDArray mSquadsRepairing;    // Squads currently being repaired.
   BEntityTimePairArray mIgnoreList;   // Squads who can't repair by this power until a certain time.

   DWORD mNextTickTime;
   BEntityID mRepairObjectID;
   BProtoObjectID mRepairAttachmentProtoID;
   BObjectTypeID mFilterTypeID;
   float mRepairRadius;
   DWORD mTickDuration;
   float mRepairCombatValuePerTick;
   DWORD mCooldownTimeIfDamaged;
   uint mTicksRemaining;
   bool mSpreadAmongSquads;
   bool mAllowReinforce;
   bool mIgnorePlacement;
   bool mHealAny;
   bool mNeverStops;
};
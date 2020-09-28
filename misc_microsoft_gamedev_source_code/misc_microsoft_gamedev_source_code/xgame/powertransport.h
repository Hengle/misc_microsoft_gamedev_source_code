//==============================================================================
// powerTransportl.h
//
// Copyright (c) 2008 Ensemble Studios
//==============================================================================
#pragma once


// xgame
#include "power.h"
#include "powerhelper.h"

//==============================================================================
// class BPowerUserTransport
// ASYNC stuff (only on one BUser in the game.)
//==============================================================================
class BPowerUserTransport : public BPowerUser
{
public:

   // This will only be called on one machine (as if it were in BUser) so don't do sync unsafe stuff in it.
   virtual bool init(BProtoPowerID protoPowerID, BPowerLevel powerLevel, BUser* pUser, BEntityID ownerSquadID, BVector targetLocation, bool noCost = false);
   virtual bool shutdown();
   virtual bool handleInput(long port, long event, long controlType, BInputEventDetail& detail);
   virtual void update(float elapsedTime) { mElapsed += elapsedTime; }
   // Define these if you want to override the default render behaviors.
   virtual void updateUI();
   virtual void cancelPower();
   //virtual void renderUI();

   virtual bool save(BStream* pStream, int saveType) const;
   virtual bool load(BStream* pStream, int saveType);
   virtual bool postLoad(int saveType);

   void onSquadsFoundToTransport(const BEntityIDArray& squadsToTransport);

protected:
   virtual void setupUser();

   bool initDecals();
   void updateDecals(bool valid);
   bool canUsePowerAtHoverPoint(bool draw = false);

   BPowerHelper::BHUDSounds mHudSounds;
   BEntityIDArray mSquadsToTransport;
   BEntityIDArray mTargetedSquads;
   long mLOSMode;
   int mBaseDecal;
   int mMoverDecal;
   BVector mPickupLocation;
   bool mGotPickupLocation:1;
};


//==============================================================================
// class BPowerTransport
// SYNC stuff in the sim.
//==============================================================================
class BPowerTransport : public BPower
{
public:

   BPowerTransport() { mType = PowerType::cTransport; }

   // This will be called on all machines.
   virtual bool init(BPlayerID playerID, BPowerLevel powerLevel, BPowerUserID powerUserID, BEntityID ownerSquadID, BVector targetLocation, bool ignoreAllReqs = false);
   virtual bool shutdown();
   virtual bool submitInput(BPowerInput powerInput);
   virtual void update(DWORD currentGameTime, float lastUpdateLength);

   virtual bool save(BStream* pStream, int saveType) const;
   virtual bool load(BStream* pStream, int saveType);

   static BEntityIDArray filterSquads(BEntityIDArray squads, BVector refVector, BPowerHelper::BLimitDataArray& limitDataArray);

protected:
   BPowerUserTransport* getPowerUser() const;

   BVector mPickupLocation;
   BEntityIDArray mSquadsToTransport;
   bool mGotPickupLocation:1;
};
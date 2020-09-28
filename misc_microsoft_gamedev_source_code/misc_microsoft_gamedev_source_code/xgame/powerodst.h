//==============================================================================
// powerODSTl.h
//
// Copyright (c) 2008 Ensemble Studios
//==============================================================================
#pragma once


// xgame
#include "power.h"
#include "powerhelper.h"

//==============================================================================
// class BPowerUserODST
// ASYNC stuff (only on one BUser in the game.)
//==============================================================================
class BPowerUserODST : public BPowerUser
{
public:

   // This will only be called on one machine (as if it were in BUser) so don't do sync unsafe stuff in it.
   virtual bool init(BProtoPowerID protoPowerID, BPowerLevel powerLevel, BUser* pUser, BEntityID ownerSquadID, BVector targetLocation, bool noCost = false);
   virtual bool shutdown();
   virtual bool handleInput(long port, long event, long controlType, BInputEventDetail& detail);
   virtual void update(float elapsedTime) { mElapsed += elapsedTime; }
   // Define these if you want to override the default render behaviors.
   virtual void updateUI();
   virtual void renderUI();

   virtual bool save(BStream* pStream, int saveType) const;
   virtual bool load(BStream* pStream, int saveType);

protected:
   virtual void setupUser();

   enum CanFireResult 
   {
      cCanFire,
      cCannotFireODST,
      cCannotFirePop,
      cCannotFireResources,
      cCannotFireGeneric,

      cEnd
   };

   long canFire(BVector& suggestedLocation);

   BUString mHelpString;
   long mLOSMode;
   BProtoSquadID mODSTProtoSquadID;
   BProtoObjectID mODSTProtoObjectID;
   long mCanFire;
   BVector mValidDropLocation;
   BPowerHelper::BHUDSounds mHudSounds;
};


//==============================================================================
// class BPowerODST
// SYNC stuff in the sim.
//==============================================================================
class BPowerODST : public BPower
{
public:

   BPowerODST() { mType = PowerType::cODST; }

   // This will be called on all machines.
   virtual bool init(BPlayerID playerID, BPowerLevel powerLevel, BPowerUserID powerUserID, BEntityID ownerSquadID, BVector targetLocation, bool ignoreAllReqs = false);
   virtual bool shutdown();
   virtual bool submitInput(BPowerInput powerInput);
   virtual void update(DWORD currentGameTime, float lastUpdateLength);

   virtual bool save(BStream* pStream, int saveType) const;
   virtual bool load(BStream* pStream, int saveType);

   void setAddToMissionID(BAIMissionID v) { mAddToMissionID = v; }

protected:

   struct BODSTDrop
   {
      BEntityID SquadId;
      double    SpawnSquadTime;
   };

   float mSquadSpawnDelay;
   BSmallDynamicSimArray<BODSTDrop> mActiveDrops;
   BProtoObjectID mProjectileProtoID; 
   BProtoSquadID mODSTProtoSquadID;
   BAIMissionID mAddToMissionID;
   bool mReadyForShutdown;
};
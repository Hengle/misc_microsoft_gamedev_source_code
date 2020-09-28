//==============================================================================
// power.h
//
// Copyright (c) 2008 Ensemble Studios
//==============================================================================
#pragma once


// xgame
#include "powertypes.h"
#include "aitypes.h"
#include "simtypes.h"

class BUser;
class BProtoAction;
class BSquad;

//==============================================================================
// Class BPowerUser
// This class is pretty much a pure virtual class that we derive from.
// BPowerUser derived classes handle the ASYNC related UI stuff for powers.
//==============================================================================
class BPowerUser
{
public:
   virtual ~BPowerUser();

   void initBase()
   {
      mInitialized = false;
      mFlagDestroy = false;
      mFlagNoCost = false;
      mFlagCheckPowerLocation = true;
      mFlagIssueCommandOnCancel = true;
      mProtoPowerID = cInvalidProtoPowerID;
      mPowerLevel = 0;
      mOwnerSquadID = cInvalidObjectID;
      mpUser = NULL;
      mElapsed = 0.0f;
   }

   // Get accessors
   bool getFlagDestroy() const { return mFlagDestroy; }
   bool getFlagCheckPowerLocation() const { return mFlagCheckPowerLocation; }
   BPowerUserID getID() const { return (mID); }
   BPowerType getType() const { return (mType); }
   // Set accessors
   void setID(BPowerUserID v) { mID = v; }
   void setType(BPowerType powerType) { mType=powerType; }

   // Handles the Asynchronous UI stuff from BUser for the power.
   virtual bool init(BProtoPowerID protoPowerID, BPowerLevel powerLevel, BUser* pUser, BEntityID ownerSquadID, BVector targetLocation, bool noCost = false) = 0;
   virtual bool shutdown() = 0;
   virtual void update(float elapsedTime) = 0;
   virtual bool handleInput(long port, long event, long controlType, BInputEventDetail& detail) = 0;
   virtual void cancelPower();

   // Define these if you want to override the default render behaviors.
   virtual void updateUI() {}
   virtual void renderUI() {}

   virtual bool shouldClampCamera() const { return true; }

   GFDECLAREVERSION();
   virtual bool save(BStream* pStream, int saveType) const;
   virtual bool load(BStream* pStream, int saveType);
   virtual bool postLoad(int saveType);

protected:
   virtual void setupUser() {}

   // Don't new BPowerUser, only derived classes.
   BPowerUser() :
      mID(0),
      mType(PowerType::cInvalid)
   {
      initBase();
   }

   bool           mInitialized:1;
   bool           mFlagDestroy:1;
   bool           mFlagNoCost:1;
   bool           mFlagCheckPowerLocation:1;
   bool           mFlagIssueCommandOnCancel:1;

   BProtoPowerID  mProtoPowerID;
   BPowerLevel    mPowerLevel;
   BPowerUserID   mID;
   BPowerType     mType;
   BEntityID      mOwnerSquadID;
   BUser*         mpUser;

   double         mElapsed;
};


//==============================================================================
// Class BPower
// This class is pretty much a pure virtual class that we derive from.
// BPower derived classes handle the SYNC related sim stuff for powers.
//==============================================================================
class BPower
{
public:
   virtual ~BPower();

   void initBase();

   // Get accessors
   BPowerID getID() const { return (mID); }
   BPowerType getType() const { return (mType); }
   BPowerUserID getPowerUserID() const { return (mPowerUserID); }
   BProtoPowerID getProtoPowerID() const { return (mProtoPowerID); }
   double getElapsed() const { return (mElapsed); }
   BPlayerID getPlayerID() const { return (mPlayerID); }
   bool getFlagDestroy() const { return mFlagDestroy; }
   bool getFlagCheckPowerLocation() const { return mFlagCheckPowerLocation; }
   BVector getTargetLocation() const { return (mTargetLocation); }
   const BSquad* getOwnerSquad() const;

   // Set accessors
   void setID(BPowerID v) { mID = v; }
   void setType(BPowerType powerType) { mType = powerType; }
   void setPowerUserID(BPowerUserID v) { mPowerUserID = v; }
   void setProtoPowerID(BProtoPowerID v) { mProtoPowerID = v; }
   void setPlayerID(uint v) { mPlayerID = v; }
   void setTargetLocation(BVector v) { mTargetLocation = v; }

   // This will be called on all machines.
   virtual bool init(BPlayerID playerID, BPowerLevel powerLevel, BPowerUserID powerUserID, BEntityID ownerSquadID, BVector targetLocation, bool ignoreAllReqs = false) = 0;
   virtual bool shutdown() = 0;
   virtual bool submitInput(BPowerInput powerInput) = 0;
   virtual void update(DWORD currentGameTime, float lastUpdateLength) = 0;

   virtual BProtoAction* getProtoAction(void) const {return(NULL);}

   virtual void projectileImpact(BEntityID id, const BEntityIDArray& killedUnits, const BEntityIDArray& damagedUnits) {};

   BPowerUserID getPowerUserID();
   void         deletePowerUser();

   GFDECLAREVERSION();
   virtual bool save(BStream* pStream, int saveType) const;
   virtual bool load(BStream* pStream, int saveType);

protected:

   BPowerID mID;
   BPowerType mType;

   // How to track the BPowerUser we need to clean up (possibly).
   BPowerUserID   mPowerUserID;
   BProtoPowerID  mProtoPowerID;
   BPowerLevel    mPowerLevel;

   float          mMaintenanceSupplies; // How much have we spent during the execution of this power for the "over time" powers.

   double         mElapsed;

   BPlayerID      mPlayerID;
   BEntityID      mOwnerID;
   BVector        mTargetLocation;   

   bool           mFlagDestroy:1;
   bool           mFlagIgnoreAllReqs:1;
   bool           mFlagCheckPowerLocation:1;

   // Don't new BPower, only derived classes.
   BPower();
};
//==============================================================================
// bid.h
//
// Copyright (c) 2007 Ensemble Studios
//==============================================================================
#pragma once

#include "cost.h"
#include "aitypes.h"
#include "gamefilemacros.h"

class BPlayer;
class BUnit;

//==============================================================================
// class bid
// Represents a request to buy a unit, tech or building
//==============================================================================
class BBid
{
public:

   BBid();
   ~BBid() {}
   void resetNonIDData();
   
   // get accessors
   void getCost(BCost& cost) const;
   BBidID& getID() { return (mID); }
   BBidID getID() const { return (mID); }
   BPlayerID getPlayerID() const { return (mPlayerID); }
   DWORD getLastPurchaseTime() const { return (mLastPurchaseTime); }
   float getTotalCost() const;
   float getScore() const { return (mScore); }
   float getEffectivePriority() const { return (mEffectivePriority); }
   BProtoSquadID getSquadToBuy() const { return (mSquadToBuy); }
   BProtoTechID getTechToBuy() const { return (mTechToBuy); }
   BProtoObjectID getBuildingToBuy() const { return (mBuildingToBuy); }
   BProtoPowerID getPowerToCast() const { return (mPowerToCast); }
   float getPriority() const { return (mPriority); }
   BBidState getState() const { return (mState); }
   bool isState(BBidState state) const { return (mState == state); }
   BBidType getType() const { return (mType); }
   bool isType(BBidType type) const { return (mType == type); }
   bool getWantToBuy() const { return (mWantToBuy); }
   bool getUseTargetLocation() const { return (mbUseTargetLocation); }
   bool getUseBuilderID() const { return (mbUseBuilderID); }
   BVector getTargetLocation() const { BASSERT(mbUseTargetLocation); return (mTargetLocation); }
   BEntityID getBuilderID() const { BASSERT(mbUseBuilderID); return (mBuilderID); }
   DWORD getElapsedTime() const;

   // set accessors
   void setID(BBidID id) { mID = id; }
   void setID(uint refCount, uint index) { mID.set(refCount, index); }
   void setPlayerID(BPlayerID playerID) { mPlayerID = playerID; }
   void setLastPurhcaseTime(DWORD time) { mLastPurchaseTime = time; }
   void setScore(float score) { mScore = score; }
   void setSquadToBuy(BProtoSquadID protoSquadID);
   void setTechToBuy(BProtoTechID techID);
   void setBuildingToBuy(BProtoObjectID protoUnitID);
   void setPowerToCast(BProtoPowerID protoPowerID);
   void setPriority(float priority);
   void setPadSupplies(float v) { mPadSupplies = v; }
   void setState(BBidState newState) { mState = newState; }
   void setType(BBidType newType) { mType = newType; }
   void setUseTargetLocation(bool v) { mbUseTargetLocation = v; }
   void setUseBuilderID(bool v) { mbUseBuilderID = v; }
   void setTargetLocation(BVector v) { mTargetLocation = v; mbUseTargetLocation = true; }
   void setBuilderID(BEntityID v) { mBuilderID = v; mbUseBuilderID = true; }
   void setWantToBuy(bool newValue);

   // Logic
   void update(bool forceUpdate);      
   void clear(); // Clears Squad, Tech, Building, Cost and Priority fields
   bool purchase(BVector& resultLocation);
   bool purchaseTech(BVector& resultLocation);
   bool purchaseSquad(BVector& resultLocation);
   bool purchaseBuilding(BVector& resultLocation);
   bool purchasePower(BVector& resultLocation);

   #ifndef BUILD_FINAL
      uint mDebugID;
   #endif

   GFDECLAREVERSION();
   bool save(BStream* pStream, int saveType) const;
   bool load(BStream* pStream, int saveType);

protected:

   BVector mTargetLocation;         // The target location of this bid.
   BEntityID mBuilderID;            // The unit that will build this thing.
   BBidID mID;                      // Handle that triggers use to access this bid
   BPlayerID mPlayerID;             // The owner of the bid.
   DWORD mLastPurchaseTime;         // Last time this bid made a purchase.  Longer waits give a bid a higher bidscore.
   float mScore;                    // A function of cost, priority and wait time
   float mPriority;                 // 0.0f - 1.0f;
   float mEffectivePriority;        // Converts priority to an exponential scale, like Richter or Decibels.
   float mPadSupplies;              // Extra supplies to be padded to the cost of this bid.
   BProtoSquadID mSquadToBuy;       // The proto squad ID of the squad to be purchased
   BProtoTechID mTechToBuy;         // The tech ID of the tech to be purchased
   BProtoObjectID mBuildingToBuy;   // The proto unit ID of the building to be purchased (only one of these three at a time)
   BProtoPowerID mPowerToCast;      // The proto power ID of the power to be cast.

   BUInt8<BBidState, BidState::cMin, BidState::cMax> mState;                  // Waiting, approved, inactive
   BUInt8<BBidType, BidType::cMin, BidType::cMax> mType;                      // Invalid, squad, tech, building, ...

   bool mWantToBuy            : 1;                                            // If true, one of unit/tech/building must be specified.
   bool mbUseTargetLocation   : 1;
   bool mbUseBuilderID        : 1;
};

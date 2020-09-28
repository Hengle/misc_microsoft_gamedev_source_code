//==============================================================================
// kbsquad.h
//
// Copyright (c) 2007 Ensemble Studios
//==============================================================================
#pragma once

// includes
#include "aitypes.h"
#include "gamefilemacros.h"

// Forward Declarations
class BKB;
class BSquad;


//==============================================================================
// class BKBSquad
// Class which contains data about the KB's perspective of a squad.
//==============================================================================
class BKBSquad
{
public:

   // Constructor / Destructor
   BKBSquad();
   ~BKBSquad();
   void resetNonIDData();

   // Get accessors
   BVector getPosition() const;
   bool getValidKnownPosition(BVector& pos) const;
   BKBSquadID getID() const { return (mID); }
   BSquad* getSquad();
   const BSquad* getSquad() const;

   BEntityID getSquadID() const { return (mSquadID); }
   BTeamID getTeamID() const { return (mTeamID); }
   BPlayerID getPlayerID() const { return (mPlayerID); }
   BProtoSquadID getProtoSquadID() const { return (mProtoSquadID); }
   BKBBaseID getKBBaseID() const { return (mKBBaseID); }
   float getHitpoints() const;
   float getHPPercentage() const;
   float getShieldpoints() const;
   float getSPPercentage() const;
   DWORD getLastSeenTime() const;
   float getAssetValue() const;
   bool containsBuilding() const;
   bool containsGatherable() const;
   bool containsBase() const;
   bool containsSettlement() const;
   uint getAttackGrade(BDamageTypeID damageType) const;
   float getAttackRatingDPS(BDamageTypeID damageType) const;
   float getAttackGradeRatio(BDamageTypeID damageType) const;
   float getStars(BDamageTypeID damageType) const { return (getAttackGradeRatio(damageType)); }
   BDamageTypeID getDamageType() const;
   bool isDamageType(BDamageTypeID damageType) const;
   float getCombatValue() const;
   float getCombatValueHP() const;
   float getHPMax() const;
   float getSPMax() const;

   bool getCurrentlyVisible() const { return (mbCurrentlyVisible); }
   bool getLastKnownPosValid() const { return (mbLastKnownPosValid); }

   // Set accessors
   void setPosition(BVector position) { mPosition = position; }

   #ifndef BUILD_FINAL
   void setForward(BVector forward) { mForward = forward; }
   void setUp(BVector up) { mUp = up; }
   void setRight(BVector right) { mRight = right; }
   BVector getForward() const;
   BVector getUp() const;
   BVector getRight() const;
   #endif

   void setID(BKBSquadID id) { mID = id; }
   void setID(uint refCount, uint index) { mID.set(refCount, index); }
   void setSquadID(BEntityID squadID) { mSquadID = squadID; }
   void setTeamID(BTeamID teamID) { mTeamID = teamID; }
   void setPlayerID(BPlayerID playerID) { mPlayerID = playerID; }
   void setProtoSquadID(BProtoSquadID protoSquadID) { mProtoSquadID = protoSquadID; }
   void setKBBaseID(BKBBaseID kbBaseID) { mKBBaseID = kbBaseID; }
   void setHitpoints(float hitpoints) { mHitpoints = hitpoints; }
   void setShieldpoints(float shieldpoints) { mShieldpoints = shieldpoints; }
   void setPercentHP(float percentHP) { mPercentHP = percentHP; }
   void setPercentSP(float percentSP) { mPercentSP = percentSP; }
   void setLastSeenTime(DWORD lastSeenTime) { mLastSeenTime = lastSeenTime; }
   void setCurrentlyVisible(bool v) { mbCurrentlyVisible = v; }
   void setLastKnownPosValid(bool v) { mbLastKnownPosValid = v; }

   void update(const BKB *pKB, BSquad *pSquad);
   void bumpIDRefCount() { mID.bumpRefCount(); }
   DWORD getStaleness() const;

   #ifndef BUILD_FINAL
   uint mDebugID;
   void debugRender() const;
   #endif

   GFDECLAREVERSION();
   bool save(BStream* pStream, int saveType) const;
   bool load(BStream* pStream, int saveType);

protected:

   BVector           mPosition;              // 16 bytes

   #ifndef BUILD_FINAL
      BVector           mForward;
      BVector           mUp;
      BVector           mRight;

   #endif

   BKBSquadID        mID;                    // 4 bytes
   BEntityID         mSquadID;               // 4 bytes
   BTeamID           mTeamID;                // 4 bytes (kb owner)
   BPlayerID         mPlayerID;              // 4 bytes
   BProtoSquadID     mProtoSquadID;          // 4 bytes
   BKBBaseID         mKBBaseID;              // 4 bytes
   float             mHitpoints;             // 4 bytes
   float             mShieldpoints;          // 4 bytes
   float             mPercentHP;             // 4 bytes
   float             mPercentSP;             // 4 bytes
   DWORD             mLastSeenTime;          // 4 bytes

   bool              mbCurrentlyVisible      : 1;  // Is the squad currently visible
   bool              mbLastKnownPosValid     : 1;  // Have we explored the last known pos of this squad?
};


//==============================================================================
// class BKBSquadQuery
// Class which contains data for querying a KB about it's view of the world.
//==============================================================================
class BKBSquadQuery
{
public:
   BKBSquadQuery();
   ~BKBSquadQuery() {}
   void resetQuery();
   DWORD getStaleness() const;

   // flag accessors
   bool getFlagPointRadius() const { return (mFlagPointRadius); }
   bool getFlagPlayerRelation() const { return (mFlagPlayerRelation); }
   bool getFlagObjectType() const { return (mFlagObjectType); }
   bool getFlagBase() const { return (mFlagBase); }
   bool getFlagMinStaleness() const { return (mFlagMinStaleness); }
   bool getFlagMaxStaleness() const { return (mFlagMaxStaleness); }
   bool getFlagCurrentlyVisible() const { return (mFlagCurrentlyVisible); }
   bool getFlagIncludeBuildings() const { return (mFlagIncludeBuildings); }

   // get accessors
   BVector getPosition() const { BASSERT(mFlagPointRadius); return (mPosition); }
   float getRadius() const { BASSERT(mFlagPointRadius); return (mRadius); }
   BPlayerID getPlayerID() const { BASSERT(mFlagPlayerRelation); return (mPlayerID); }
   BRelationType getPlayerRelation() const { BASSERT(mFlagPlayerRelation); return (mPlayerRelation); }
   BObjectTypeID getObjectTypeID() const { BASSERT(mFlagObjectType); return (mObjectTypeID); }
   BKBBaseID getBaseID() const { BASSERT(mFlagBase); return (mBaseID); }
   DWORD getMinStaleness() const { BASSERT(mFlagMinStaleness); return (mMinStaleness); }
   DWORD getMaxStaleness() const { BASSERT(mFlagMaxStaleness); return (mMaxStaleness); }
   bool getCurrentlyVisible() const { BASSERT(mFlagCurrentlyVisible); return (mbCurrentlyVisible); }
   bool getIncludeBuildings() const { BASSERT(mFlagIncludeBuildings); return (mbIncludeBuildings); }
   bool getSelfAsAlly() const { return (mbSelfAsAlly); }

   // set accessors
   void setPointRadius(BVector pos, float rad) { mPosition = pos; mRadius = rad; mFlagPointRadius = true; }
   void setPlayerRelation(BPlayerID playerID, BRelationType relation) { mPlayerID = playerID; mPlayerRelation = relation; mFlagPlayerRelation = true; }
   void setObjectType(BObjectTypeID objectTypeID) { mObjectTypeID = objectTypeID; mFlagObjectType = true; }
   void setBase(BKBBaseID baseID) { mBaseID = baseID; mFlagBase = true; }
   void setMinStaleness(DWORD minStaleness) { mMinStaleness = minStaleness; mFlagMinStaleness = true; }
   void setMaxStaleness(DWORD maxStaleness) { mMaxStaleness = maxStaleness; mFlagMaxStaleness = true; }
   void setCurrentlyVisible(bool currentlyVisible) { mbCurrentlyVisible = currentlyVisible; mFlagCurrentlyVisible = true; }
   void setIncludeBuildings(bool includeBuildings) { mbIncludeBuildings = includeBuildings; mFlagIncludeBuildings = true; }
   void setSelfAsAlly(bool selfAsAlly) { mbSelfAsAlly = selfAsAlly; }

   GFDECLAREVERSION();
   bool save(BStream* pStream, int saveType) const;
   bool load(BStream* pStream, int saveType);

protected:
   BVector        mPosition;                 // 16 bytes
   float          mRadius;                   // 4 bytes
   BPlayerID      mPlayerID;                 // 4 bytes
   BRelationType  mPlayerRelation;           // 4 bytes
   BObjectTypeID  mObjectTypeID;             // 4 bytes
   BKBBaseID      mBaseID;                   // 4 bytes
   DWORD          mMinStaleness;             // 4 bytes
   DWORD          mMaxStaleness;             // 4 bytes

   bool           mbCurrentlyVisible   : 1;  // 1 byte
   bool           mbIncludeBuildings   : 1;  //
   bool           mbSelfAsAlly         : 1;  //

   // Flags.
   bool mFlagPointRadius         : 1;
   bool mFlagPlayerRelation      : 1;
   bool mFlagObjectType          : 1;
   bool mFlagBase                : 1;
   bool mFlagMinStaleness        : 1;
   bool mFlagMaxStaleness        : 1;
   bool mFlagCurrentlyVisible    : 1;
   bool mFlagIncludeBuildings    : 1;
};
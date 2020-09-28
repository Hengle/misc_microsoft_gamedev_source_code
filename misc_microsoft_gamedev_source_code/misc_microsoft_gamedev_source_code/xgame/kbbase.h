//==============================================================================
// kbbase.h
//
// Copyright (c) 2007 Ensemble Studios
//==============================================================================
#pragma once


// xgame
#include "aitypes.h"
#include "gamefilemacros.h"

class BKB;


//==============================================================================
// class BKBBase
//==============================================================================
class BKBBase
{
public:

   // Constructor / Destructor
   BKBBase();
   ~BKBBase();

   // Init / update stuff
   void bumpIDRefCount() { mID.bumpRefCount(); }
   void resetNonIDData();
   void update();

   // get accessors
   const BVector getPosition() const { return (mPosition); }
   float getRadius() const { return (mRadius); }
   float getMass() const { return (mMass); }
   BTeamID getTeamID() const { return (mTeamID); }
   BPlayerID getPlayerID() const { return (mPlayerID); }
   BKBBaseID getID() const { return (mID); }
   const BKBSquadIDArray& getKBSquadIDs() const { return (mKBSquadIDs); }
   const BAIMissionTargetIDArray& getAIMissionTargetIDs() const { return (mAIMissionTargetIDs); }
   DWORD getLastSeenTime() const { return (mLastSeenTime); }
   bool getIsPositionAnchored() const { return (mbIsPositionAnchored); }
   uint getVisibleSquadCount() const;
   uint getBuildingCount() const;
   float getAssetValue() const { return (mAssetValue); }

   // set accessors
   void setID(const BKBBaseID id) { mID = id; }
   void setID(uint refCount, uint index) { mID.set(refCount, index); }
   void setPosition(const BVector pos);
   void setRadius(float radius);
   void setMass(float mass) { mMass = mass; }
   void setTeamID(BTeamID teamID) { mTeamID = teamID; }
   void setPlayerID(BPlayerID playerID) { mPlayerID = playerID; }
   void setLastSeenTime(DWORD lastSeenTime) { mLastSeenTime = lastSeenTime; }
   void setIsPositionAnchored(bool v) { mbIsPositionAnchored = v; }

   void addKBSquad(BKBSquadID kbSquadID);
   void removeKBSquad(BKBSquadID kbSquadID);
   bool containsKBSquad(BKBSquadID kbSquadID) { return (mKBSquadIDs.contains(kbSquadID)); }
   void removeAllKBSquads();

   void addAIMissionTargetID(BAIMissionTargetID aiMissionTargetID) { mAIMissionTargetIDs.add(aiMissionTargetID); }
   void removeAIMissionTargetID(BAIMissionTargetID aiMissionTargetID) { mAIMissionTargetIDs.remove(aiMissionTargetID, false); }
   void clearAIMissionTargetIDs() { mAIMissionTargetIDs.resize(0); }

   // Queries
   bool containsPosition(BVector testPosition) const;
   bool canMergeIntoBase(BKBBase& rKBBase) const;
   void mergeIntoBase(BKBBase& rKBBase);
   DWORD getStaleness() const;

   #ifndef BUILD_FINAL
   uint mDebugID;
   void debugRender() const;
   #endif


   GFDECLAREVERSION();
   bool save(BStream* pStream, int saveType) const;
   bool load(BStream* pStream, int saveType);

protected:

   BVector                 mPosition;                 // What is the position.
   BAIMissionTargetIDArray mAIMissionTargetIDs;       // IDs for all the mission targets this KBBase represents to players sharing our KB.
   BKBSquadIDArray         mKBSquadIDs;               // List of kbsquads in the base.
   float                   mRadius;                   // What's the current radius.
   float                   mMass;                     // How much "stuff" is in this base.
   BTeamID                 mTeamID;                   // Whose kb is this in
   BPlayerID               mPlayerID;                 // Who owns the base
   BKBBaseID               mID;                       // What is the ID
   DWORD                   mLastSeenTime;             // When was the last time any unit in this base was seen
   float                   mAssetValue;               // What's the accumulated value of the assets in the base.
   bool                    mbIsPositionAnchored : 1;  // If our base contains a settlement, it gets anchored in place and no longer moves.
};


//==============================================================================
// class BKBBaseQuery
// Class which contains data for querying a KB about it's view of the world.
//==============================================================================
class BKBBaseQuery
{
public:
   BKBBaseQuery();
   ~BKBBaseQuery() {}
   void resetQuery();

   // flag accessors
   bool getFlagPointRadius() const { return (mFlagPointRadius); }
   bool getFlagPlayerRelation() const { return (mFlagPlayerRelation); }
   bool getFlagMinStaleness() const { return (mFlagMinStaleness); }
   bool getFlagMaxStaleness() const { return (mFlagMaxStaleness); }
   bool getFlagRequireBuildings() const { return (mFlagRequireBuildings); }

   // get accessors
   BVector getPosition() const { BASSERT(mFlagPointRadius); return (mPosition); }
   float getRadius() const { BASSERT(mFlagPointRadius); return (mRadius); }
   BPlayerID getPlayerID() const { BASSERT(mFlagPlayerRelation); return (mPlayerID); }
   BRelationType getPlayerRelation() const { BASSERT(mFlagPlayerRelation); return (mPlayerRelation); }
   DWORD getMinStaleness() const { BASSERT(mFlagMinStaleness); return (mMinStaleness); }
   DWORD getMaxStaleness() const { BASSERT(mFlagMaxStaleness); return (mMaxStaleness); }
   bool getSelfAsAlly() const { return (mbSelfAsAlly); }

   // set accessors
   void setPointRadius(BVector pos, float rad) { mPosition = pos; mRadius = rad; mFlagPointRadius = true; }
   void setPlayerRelation(BPlayerID playerID, BRelationType relation) { mPlayerID = playerID; mPlayerRelation = relation; mFlagPlayerRelation = true; }
   void setMinStaleness(DWORD minStaleness) { mMinStaleness = minStaleness; mFlagMinStaleness = true; }
   void setMaxStaleness(DWORD maxStaleness) { mMaxStaleness = maxStaleness; mFlagMaxStaleness = true; }
   void setRequireBuildings(bool requireBuildings) { mFlagRequireBuildings = requireBuildings; }
   void setSelfAsAlly(bool selfAsAlly) { mbSelfAsAlly = selfAsAlly; }

   GFDECLAREVERSION();
   bool save(BStream* pStream, int saveType) const;
   bool load(BStream* pStream, int saveType);

protected:
   BVector        mPosition;              // 16 bytes
   float          mRadius;                // 4 bytes
   BPlayerID      mPlayerID;              // 4 bytes
   DWORD          mMinStaleness;          // 4 bytes
   DWORD          mMaxStaleness;          // 4 bytes
   BRelationType  mPlayerRelation;        // 4 bytes

   // Flags.
   bool mFlagPointRadius      : 1;
   bool mFlagPlayerRelation   : 1;
   bool mFlagMinStaleness     : 1;
   bool mFlagMaxStaleness     : 1;
   bool mFlagRequireBuildings : 1;
   bool mbSelfAsAlly          : 1;
};

// mrh new
__declspec(selectany) extern const float cDefaultBKBBaseRadius = 100.0f;
__declspec(selectany) extern const float cDefaultBKBBaseRadiusSqr = 100.0f * 100.0f;
__declspec(selectany) extern const float cMaxBKBBaseRadius = 100.0f;
__declspec(selectany) extern const float cMaxBKBBaseRadiusSqr = 100.0f * 100.0f;

// mrh - old
//__declspec(selectany) extern const float cDefaultBKBBaseRadius = 20.0f;
//__declspec(selectany) extern const float cDefaultBKBBaseRadiusSqr = 20.0f * 20.0f;
//__declspec(selectany) extern const float cMaxBKBBaseRadius = 100.0f;
//__declspec(selectany) extern const float cMaxBKBBaseRadiusSqr = 100.0f * 100.0f;
//__declspec(selectany) extern const float cBKBBaseAttractDist = 10.0f;
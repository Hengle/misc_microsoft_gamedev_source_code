//==============================================================================
// aimissiontarget.h
//
// Copyright (c) 2007 Ensemble Studios
//==============================================================================
#pragma once

// xgame includes
#include "aitypes.h"

//==============================================================================
// class BAIMissionTarget
//==============================================================================
class BAIMissionTarget
{
public:

   // Constructor / Destructor
   BAIMissionTarget();
   ~BAIMissionTarget();
   void resetNonIDData();

   // Get accessors
   BAIMissionScore& getScore() { return (mScore); }
   const BAIMissionScore& getScore() const { return (mScore); }
   BVector getPosition() const { return (mPosition); }
   float getRadius() const { return (mRadius); }
   float getRadiusSqr() const { return (mRadius*mRadius); }
   BPlayerID getPlayerID() const { return (mPlayerID); }
   BKBBaseID getKBBaseID() const { return (mKBBaseID); }
   BAIMissionTargetType getTargetType() const { return (mTargetType); }
   BAIMissionType getMissionType() const { return (mMissionType); }
   BAIMissionTargetID& getID() { return (mID); }
   BAIMissionTargetID getID() const { return (mID); }
   bool getDestroyOnNoRefs() const { return (mbDestroyOnNoRefs); }
   bool getFlagAllowScoring() const { return (mFlagAllowScoring); }

   // Set accessors
   void setScore(const BAIMissionScore& score) { mScore = score; }
   void setPosition(BVector pos) { mPosition = pos; }
   void setRadius(float radius) { mRadius = radius; }
   void setPlayerID(BPlayerID v) { mPlayerID = v; }
   void setKBBaseID(BKBBaseID kbBaseID) { mKBBaseID = kbBaseID; }
   void setTargetType(BAIMissionTargetType targetType) { mTargetType = targetType; }
   void setMissionType(BAIMissionType missionType) { mMissionType = missionType; }
   void setID(BAIMissionTargetID id) { mID = id; }
   void setID(uint refCount, uint index) { mID.set(refCount, index); }
   void setDestroyOnNoRefs(bool v) { mbDestroyOnNoRefs = v; }
   void setFlagAllowScoring(bool v) { mFlagAllowScoring = v; }


   // More helper stuff
   bool isTargetType(BAIMissionTargetType targetType) const { return (mTargetType == targetType); }
   bool isMissionType(BAIMissionType missionType) const { return (mMissionType == missionType); }

   void addWrapperRef(BAIMissionTargetWrapperID id) { mWrapperRefs.add(id); }
   void removeWrapperRef(BAIMissionTargetWrapperID id) { mWrapperRefs.remove(id); }
   uint getNumWrapperRefs() const { return (mWrapperRefs.getSize()); }
   const BAIMissionTargetWrapperIDArray& getWrapperRefs() const { return (mWrapperRefs); }

   uint getNumFlareAlerts() const;
   uint getNumAttackAlerts() const;

#ifndef BUILD_FINAL  
   BSimString mName;    // Public, used for trace and debug info.
   uint mDebugID;
   void debugRender() const;
#endif

   GFDECLAREVERSION();
   bool save(BStream* pStream, int saveType) const;
   bool load(BStream* pStream, int saveType);

protected:

   BVector mPosition;
   float mRadius;
   BPlayerID mPlayerID;
   BAIMissionScore mScore;             // 20 bytes
   BKBBaseID mKBBaseID;                // 4 bytes
   BAIMissionTargetType mTargetType;   // 4 bytes
   BAIMissionType mMissionType;        // 4 bytes
   BAIMissionTargetID mID;             // 4 bytes
   BAIMissionTargetWrapperIDArray mWrapperRefs;

   bool mbDestroyOnNoRefs : 1; // This mission target should go away when no missions are using it.
   bool mFlagAllowScoring : 1; // allow scoring?
};
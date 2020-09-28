//==============================================================================
// aimissiontargetwrapper.h
//
// Copyright (c) 2008 Ensemble Studios
//==============================================================================
#pragma once

// xgame includes
#include "aitypes.h"



//==============================================================================
// class BAIMissionTargetWrapper
//==============================================================================
class BAIMissionTargetWrapper
{
public:

   BAIMissionTargetWrapper();
   ~BAIMissionTargetWrapper();
   void resetNonIDData();

   // Get
   BAIMissionTargetWrapperID getID() const { return (mID); }
   BAIMissionTargetWrapperID& getID() { return (mID); }
   BAIMissionTargetID getTargetID() const { return (mTargetID); }
   BAIMissionID getMissionID() const { return (mMissionID); }
   float getLeashDist() const { return (mLeashDist); }
   float getLeashDistSqr() const { return (mLeashDist*mLeashDist); }
   float getSecureDist() const { return (mSecureDist); }
   float getSecureDistSqr() const { return (mSecureDist*mSecureDist); }
   float getRallyDist() const { return (mRallyDist); }
   float getRallyDistSqr() const { return (mRallyDist*mRallyDist); }
   float getSearchWorkDist() const { return (mSearchWorkDist); }
   float getSearchWorkDistSqr() const { return (mSearchWorkDist*mSearchWorkDist); }
   float getHotZoneDist() const { return (mHotZoneDist); }
   float getHotZoneDistSqr() const { return (mHotZoneDist*mHotZoneDist); }
   float getMinRetreatRatio() const { return (mMinRetreatRatio); }
   float getMinRalliedPercent() const { return (mMinRalliedPercent); }
   DWORD getMinSecureTime() const { return (mMinSecureTime); }
   DWORD getTimestampTargetSecured() const { return (mTimestampTargetSecured); }
   DWORD getTimestampStopGathering() const { return (mTimestampStopGathering); }
   bool getFlagAllowRetreat() const { return (mFlagAllowRetreat); }
   bool getFlagAllowSkip() const { return (mFlagAllowSkip); }
   bool getFlagRequireSecure() const { return (mFlagRequireSecure); }
   bool getFlagGatherCrates() const { return (mFlagGatherCrates); }
   bool getFlagCurrentlySecured() const { return (mFlagCurrentlySecured); }
   bool getFlagCurrentlyGathering() const { return (mFlagCurrentlyGathering); }
   // bool getFlagAutoGenTargets() const { return (mFlagAutoGenTargets); }

   // Set
   void setID(BAIMissionTargetWrapperID v) { mID = v; }
   void setID(uint refCount, uint index) { mID.set(refCount, index); }
   void setTargetID(BAIMissionTargetID v);
   void setMissionID(BAIMissionID v) { mMissionID = v; }
   void setLeashDist(float v) { mLeashDist = v; }
   void setSecureDist(float v) { mSecureDist = v; }
   void setRallyDist(float v) { mRallyDist = v; }
   void setSearchWorkDist(float v) { mSearchWorkDist = v; }
   void setHotZoneDist(float v) { mHotZoneDist = v; }
   void setMinRetreatRatio(float v) { mMinRetreatRatio = v; }
   void setMinRalliedPercent(float v) { mMinRalliedPercent = v; }
   void setMinSecureTime(DWORD v) { mMinSecureTime = v; }
   void setTimestampStopGathering(DWORD v) { mTimestampStopGathering = v; }
   void setFlagAllowRetreat(bool v) { mFlagAllowRetreat = v; }
   void setFlagAllowSkip(bool v) { mFlagAllowSkip = v; }
   void setFlagRequireSecure(bool v) { mFlagRequireSecure = v; }
   void setFlagGatherCrates(bool v) { mFlagGatherCrates = v; }
   void setFlagCurrentlyGathering(bool v) { mFlagCurrentlyGathering = v; }
   //void setFlagAutoGenTargets(bool v) { mFlagAutoGenTargets = v; }

   GFDECLAREVERSION();
   bool save(BStream* pStream, int saveType) const;
   bool load(BStream* pStream, int saveType);

protected:

   //-----------------------------
   // IDs
   BAIMissionTargetWrapperID mID;   // The ID of the wrapper
   BAIMissionTargetID mTargetID;    // The ID of the target
   BAIMissionID mMissionID;         // The ID of the mission

   //-----------------------------
   // Distance Values
   float mLeashDist;                // A group beyond this distance MUST move back into range.
   float mSecureDist;               // This area must be cleared of enemies for the secure condition.
   float mRallyDist;                // This area must contain groups for the rallied condition.
   float mSearchWorkDist;           // This area is where we will search for *NEW* work targets (enemies, crates, friendlies to heal, etc.)
   float mHotZoneDist;              // This area is where enemies or allies must be to factor into retreat/withdraw/will I win scoring.

   //-----------------------------
   // Parameters
   float mMinRetreatRatio;             // How overpowered must we be?
   float mMinRalliedPercent;           // Minimum percent of mission squads that must be within the rally dist to be considered rallied.
   DWORD mMinSecureTime;               // How long must we be secure before we "pass" the security check.
   DWORD mTimestampTargetSecured;      // The time we cleared the targets.
   DWORD mTimestampStopGathering;      // The time we will stop gathering.

   //-----------------------------
   // Flags
   bool mFlagAllowRetreat        : 1;  // 
   bool mFlagAllowSkip           : 1;  // on retreat/fail move on
   bool mFlagRequireSecure       : 1;
   bool mFlagGatherCrates        : 1;
   bool mFlagCurrentlySecured    : 1;
   bool mFlagCurrentlyGathering  : 1;
   //bool mFlagAutoGenTargets    : 1;

};
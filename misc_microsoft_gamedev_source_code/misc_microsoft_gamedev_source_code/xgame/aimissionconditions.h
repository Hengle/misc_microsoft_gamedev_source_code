//==============================================================================
// aimissionconditions.h
//
// Copyright (c) 2007 Ensemble Studios
//==============================================================================
#pragma once


// xgame includes
#include "aitypes.h"


// forward declarations
class BAIMission;
/*

//==============================================================================
//==============================================================================
class BMissionCondition
{
public:
   
   // Constructor
   BMissionCondition();
   BMissionCondition(BMissionConditionType type);
   
   // Get accessors.
   BMissionConditionType getMissionConditionType() const { return (mType); }
   BCompareType getCompareType() const { return (mCompareType); }
   uint getSquadCount() const { return (mSquadCount); }
   DWORD getAreaSecuredDuration() const { return (mAreaSecureDuration); }
   float getMinPresentSquadsPercentage() const { return (mMinPresentSquadsPercentage); }
   float getAreaRadiusPercentage() const { return (mAreaRadiusPercentage); }
   
   // Set accessors.
   void setCompareType(BCompareType v) { mCompareType = v; }
   void setSquadCount(uint v) { mSquadCount = v; }
   void setAreaSecuredDuration(DWORD v) { mAreaSecureDuration = v; }
   void setMinPresentSquadsPercentage(float v) { mMinPresentSquadsPercentage = v; }
   void setAreaRadiusPercentage(float v) { mAreaRadiusPercentage = v; }

   bool evaluate(const BAIMission* pValidAIMission);

   DWORD getTimestampAreaSecure() const { return (mTimestampAreaSecure); }
   bool getFlagAreaSecure() const { return (mFlagAreaSecure); }
   void setTimestampAreaSecure(DWORD v) { mTimestampAreaSecure = v; }
   void setFlagAreaSecure(bool v) { mFlagAreaSecure = v; }

protected:

   bool evaluateSquadCount(const BAIMission* pValidAIMission);
   bool evaluateAreaSecured(const BAIMission* pValidAIMission);

   // Mission condition data.
   BMissionConditionType mType;        // The type of mission condition we are checking.
   BCompareType mCompareType;          // Compare type (used for multiple mission conditions.)
   uint mSquadCount;                   // cSquadCount
   DWORD mAreaSecureDuration;          // cAreaSecured
   float mMinPresentSquadsPercentage;  // cAreaSecured
   float mAreaRadiusPercentage;        // cAreaSecured

   // working data
   DWORD mTimestampAreaSecure;
   bool mFlagAreaSecure : 1;
};


// Typedefs
typedef BSmallDynamicSimArray<BMissionCondition> BMissionConditionArray;
*/
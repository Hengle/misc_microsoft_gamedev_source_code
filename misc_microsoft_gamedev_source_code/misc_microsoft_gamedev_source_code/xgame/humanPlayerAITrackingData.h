//==============================================================================
// HumanPlayerAITrackingData.h
//
// Copyright (c) 2008 Ensemble Studios
//==============================================================================
#pragma once

#include "userprofilemanager.h"

//Ripped off from Jeff R.'s stuff in userProfileManager
// This system holds the set of AI tracking data that is kept per player in their profile
// It allows r/w for that data to/from the profile for local players
// It also allow it to be synced around the network and then read from for remote players

#define DEFINE_TRACKING_DATA(varType, varName)\
   protected:\
   varType m##varName;\
   public:\
   const varType&   get##varName() const {return(m##varName);}\
   void             set##varName(const varType& foo) {m##varName=foo; mDirty = true;}

//==============================================================================
//==============================================================================
class BHumanPlayerAITrackingData
{
public:
   DEFINE_TRACKING_DATA(uint8, AIMemoryVersion);
   DEFINE_TRACKING_DATA(uint, AutoDifficultyLevel);
   DEFINE_TRACKING_DATA(uint, AutoDifficultyNumGamesPlayed);

   // Hint system  2 x number of concepts bytes
   // Not exactly AI, but is needed by all users
   DEFINE_TRACKING_DATA(BDynamicSimBYTEArray, ConceptIDs);
   DEFINE_TRACKING_DATA(BDynamicSimBYTEArray, ConceptTimesReinforced);

   BHumanPlayerAITrackingData();
   ~BHumanPlayerAITrackingData();
   
   void        setDefaultvalues();
   bool        loadValuesFromMemoryBlock(byte* pMemoryBlock, bool localData = true);
   bool        saveValuesToMemoryBlock(BDynamicStream* streamPtr, bool forceWrite = false);
   bool        isLoaded() {return mIsLoaded;};


private:
   bool        mDirty;
   bool        mLocalData;       //Set to true if the data is from the network, not a local profile, and thus should NOT be saved back out
   bool        mIsLoaded;        //If true, values have been loaded from somewhere already
};

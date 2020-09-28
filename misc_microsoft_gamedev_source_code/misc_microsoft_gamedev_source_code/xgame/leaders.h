//=============================================================================
// leaders.h
//
// Copyright (c) 2006-2007 Ensemble Studios
//=============================================================================
#pragma once

// Includes
#include "cost.h"
#include "simtypes.h"
#include "D3DTextureManager.h"
#include "xmlreader.h"
#include "bitvector.h"

//==============================================================================
//==============================================================================
class BStartingSquad
{
public:
   BStartingSquad() : mOffset(cOriginVector), mProtoSquadID(cInvalidProtoSquadID), mbFlyInSquad(false) {}

   BVector mOffset;
   BProtoSquadID mProtoSquadID;
   bool mbFlyInSquad : 1;
};

//==============================================================================
//==============================================================================
class BStartingUnit
{
public:
   BStartingUnit() : mOffset(cOriginVector), mProtoObjectID(cInvalidProtoObjectID), mBuildOtherID(cInvalidProtoObjectID), mDoppleOnStart(false) {}

   BVector mOffset;
   BProtoObjectID mProtoObjectID;
   BProtoObjectID mBuildOtherID;
   bool mDoppleOnStart : 1;
};

//==============================================================================
//==============================================================================
class BLeaderPop
{
   public:
      BLeaderPop() : mID(-1), mCap(0.0f), mMax(0.0f) {}
      BLeaderPop(const BLeaderPop& source) { *this=source; }
      BLeaderPop& operator=(const BLeaderPop& source)
      {
         if(this==&source)
            return *this;
         mID=source.mID;
         mCap=source.mCap;
         mMax=source.mMax;
         return *this;
      }
      long     mID;
      float    mCap;
      float    mMax;
};

//==============================================================================
//==============================================================================
class BLeaderSupportPower
{
   public:
      BLeaderSupportPower() : mIconLocation(-1), mTechPrereq(-1) {}
      BLeaderSupportPower(const BLeaderSupportPower& source) { *this=source; }
      BLeaderSupportPower& operator=(const BLeaderSupportPower& source)
      {
         if(this==&source)
            return *this;
         mIconLocation=source.mIconLocation;
         mTechPrereq=source.mTechPrereq;
         mPowers=source.mPowers;
         return *this;
      }
      long                          mIconLocation;
      long                          mTechPrereq;
      BSmallDynamicSimArray<long>   mPowers;
};

// Some typedefs.
typedef BSmallDynamicSimArray<BStartingSquad> BStartingSquadArray;
typedef BSmallDynamicSimArray<BStartingUnit> BStartingUnitArray;
typedef BSmallDynamicSimArray<BLeaderPop> BLeaderPopArray;
typedef BSmallDynamicSimArray<BLeaderSupportPower> BLeaderSupportPowerArray;


//=============================================================================
//=============================================================================
class BLeader
{
   public:
                                       BLeader();
                                       ~BLeader();

      bool                             preload(BXMLNode leaderNode);
      void                             load(BXMLNode leaderNode);

      int                              getLeaderPowerID() const { return mLeaderPowerID; }

      bool                             isResourceActive(long r) const { return(mActiveResources.isSet(r)!=0); }
      const BCost*                     getStartingResources() const { return(&mStartingResources); }
      float                            getStartingResource(long r) const { return(mStartingResources.get(r)); }

      BVector                          getRallyPointOffset() const { return mRallyPointOffset; }

      const BStartingSquadArray&       getStartingSquads() const { return (mStartingSquads); }
      const BStartingUnitArray&        getStartingUnits() const { return (mStartingUnits); }
      const BLeaderPopArray&           getPops() const { return (mPops); }
      const BLeaderSupportPowerArray&  getSupportPowers() const { return (mSupportPowers); }

      float                            getRepairRate() const { return mRepairRate; }
      DWORD                            getRepairDelay() const { return mRepairDelay; }
      const BCost*                     getRepairCost() const { return &mRepairCost; }
      float                            getRepairCost(long r) const { return mRepairCost.get(r); }
      float                            getRepairTime() const { return mRepairTime; }
      const BString&                   getUIBackgroundImage() const { return mUIBackground; }
      const BCost*                     getReverseHotDropCost() const { return &mReverseHotDropCost; }

      BSimString                       mName;
      BSimString                       mBonusText;
      BSimString                       mPowerText;
      BSimString                       mIconName;
      BSimString                       mFlashImgKeyFrame;
      BSimString                       mFlashPortrait;
      BString                          mUIBackground;
      BManagedTextureHandle                   mIconHandle;
      BCost                            mStartingResources;
      BStartingSquadArray              mStartingSquads;
      BStartingUnitArray               mStartingUnits;
      BVector                          mRallyPointOffset;
      BLeaderPopArray                  mPops;
      BLeaderSupportPowerArray         mSupportPowers;
      long                             mLeaderCivID;
      long                             mLeaderTechID;
      int                              mLeaderPowerID;
      long                             mNameIndex;
      long                             mDescriptionIndex;
      float                            mRepairRate;
      DWORD                            mRepairDelay;
      BCost                            mRepairCost;
      float                            mRepairTime;
      BCost                            mReverseHotDropCost;
      int                              mAlpha;
      bool                             mTest;
      UTBitVector<8>                   mActiveResources;
};
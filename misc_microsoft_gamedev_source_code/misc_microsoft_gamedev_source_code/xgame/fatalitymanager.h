//============================================================================
// fatalitymanager.h
//  
// Copyright (c) 2008 Ensemble Studios
//============================================================================
#pragma once

//============================================================================
// Includes
//============================================================================
#include "SimTypes.h"
#include "..\xcore\math\vector.h"
#include "..\xsystem\quaternion.h"
#include "obstructionmanager.h"
#include "gamefilemacros.h"

//============================================================================
// forward declarations
//============================================================================
class BUnit;
class BProtoAction;

//============================================================================
// BFatalityAsset
//============================================================================
struct BFatalityAsset
{
   BVector           mTargetPositionOffset;
   BQuaternion       mTargetOrientationOffset;
   long              mWeight;
   long              mAttackerAnimID;
   long              mTargetAnimID;
   uint8             mIndex;
   bool              mFlagCooldown:1;
};
typedef BSmallDynamicSimArray<BFatalityAsset> BFatalityAssetArray;
typedef BSmallDynamicSimArray<long> BFatalityAssetIndexArray;

enum eFatalityMoveType
{
   cFatalityNoMove,
   cFatalityAttackerPosTargetOrient,
   cFatalityAttackerPosOrient
};

//============================================================================
// BFatality
//============================================================================
struct BFatality
{
   BProtoObjectID             mTargetProtoObjectID;
   long                       mAttackerAnimType;
   long                       mAttackerEndAnimType;
   long                       mTargetAnimType;
   long                       mTargetEndAnimType;
   DWORD                      mCooldownTime;
   DWORD                      mTransitionTimeDWORD;
   DWORD                      mTransitionDelayDWORD;
   BFatalityAssetIndexArray   mFatalityAssetArray;
   float                      mOffsetToleranceOverride;
   float                      mOrientationOffsetOverride;
   float                      mAirTransitionHeight;
   float                      mAirTransitionApexPercent;
   eFatalityMoveType          mMoveType;
   BProtoAction*              mpProtoAction;
   uint8                      mIndex;
   bool                       mAnimateTarget:1;
   bool                       mKillTarget:1;
   bool                       mBoarding:1;
   bool                       mToBoneRelative:1;
   bool                       mTransitionBeforeAnimating:1;
   bool                       mAircraftTransition:1;
   bool                       mLastUnitOnly:1;
};
typedef BSmallDynamicSimArray<BFatality> BFatalityArray;

//============================================================================
// BActiveFatality
//============================================================================
class BActiveFatality
{
public:
   bool save(BStream* pStream, int saveType) const;
   bool load(BStream* pStream, int saveType);

   BVector              mPositionTransition;
   BQuaternion          mOrientationTransition;
   BVector              mPosition;
   BFatality*           mpFatality;
   BFatalityAsset*      mFatalityAsset;
   BOPObstructionNode*  mObstruction;
   BEntityID            mAttackerID;
   BEntityID            mTargetID;
   long                 mAttackerEndAnimType;
   long                 mTargetEndAnimType;
   BActionID            mCallerActionID;
   float                mRecTransitionDuration;
   DWORD                mTransitionStartTimeDWORD;
   DWORD                mTransitionEndTimeDWORD;
   DWORD                mTransitionTimeDWORD;
   DWORD                mTransitionDelayDWORD;
   DWORD                mFatalityEndTimeDWORD;
   DWORD                mCooldownEndTimeDWORD;
   DWORD                mLastTransitionUpdateTimeDWORD;
   DWORD                mAttackerEndTimeDWORD;
   DWORD                mAttackerIdleTimeDWORD;
   eFatalityMoveType    mMoveType;
   bool                 mFlagFatalityOver:1;
   bool                 mKillTarget:1;
   bool                 mAttackerEndAnimStarted:1;
   bool                 mToBoneRelative:1;
   bool                 mAircraftTransition:1;
};

//============================================================================
// Fatality asset hash data
class BFatalityAssetKey : public BBitHashable<BFatalityAssetKey>
{
   public:
      BFatalityAssetKey(BProtoObjectID attackerID, BProtoObjectID targetID, int actionIndex) : mAttackerID(attackerID), mTargetID(targetID), mActionIndex(actionIndex) {}
      BProtoObjectID mAttackerID;
      BProtoObjectID mTargetID;
      int            mActionIndex;
};

class BFatalityAssetValue
{
   public:
      BFatalityAssetValue(int firstIndex, int numAssets) : mFirstAssetIndex(firstIndex), mNumAssets(numAssets) {}
      int            mFirstAssetIndex;
      int            mNumAssets;
};

typedef BHashMap<BFatalityAssetKey, BFatalityAssetValue, BHasher<BFatalityAssetKey>, BEqualTo<BFatalityAssetKey>, true, BSimFixedHeapAllocator> BFatalityAssetMap;

//============================================================================
// BFatalityManager
//============================================================================
class BFatalityManager
{
   public:

      BFatalityManager() : mFatalityAssetsLoaded(false)     {}
      ~BFatalityManager()    { reset(true); }

      void reset(bool resetAssets);

      void loadFatalityAssets();
      bool areFatalityAssetsLoaded() const { return mFatalityAssetsLoaded; }
      void addFatalityAssets(BFatalityAssetIndexArray& indexArray, BProtoObjectID attackerID, BProtoObjectID targetID, long actionID);

      long registerFatalityAsset(BVector targetPositionOffset, BQuaternion targetOrientationOffset, long weight, long attackerAnimID, long targetAnimID);

      bool isPositionValid(BVector position) const;

      bool startFatality(BActionID callingActionID, BUnit* pAttacker, BUnit* pTarget, BFatality* pFatality, uint assetIndex, DWORD attackerIdleTime = 0);

      const BActiveFatality* getFatality(BEntityID unit, long* pIndex = NULL) const;

      BFatalityAsset* getFatalityAsset(long index) { return (index < mFatalityAssets.getNumber() ? &mFatalityAssets.get(index) : NULL); }

      void update();

      void stopFatality(BEntityID unit, bool allowTargetKill);

      GFDECLAREVERSION();
      bool save(BStream* pStream, int saveType) const;
      bool load(BStream* pStream, int saveType);

   protected:

      bool startAnims(BActiveFatality* pActiveFatality);

      BFatalityAssetMap                mFatalityAssetMap;
      BFatalityAssetArray              mFatalityAssets;
      BFreeList<BActiveFatality, 4>    mActiveFatalities;
      bool                             mFatalityAssetsLoaded:1;

};

// gFatalityManager
extern BFatalityManager gFatalityManager;

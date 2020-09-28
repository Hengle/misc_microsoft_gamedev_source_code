//==============================================================================
// unitactionbuilding.h
// Copyright (c) 2006-2007 Ensemble Studios
//==============================================================================

#pragma once
#include "action.h"

class BUnit;

//==============================================================================
//==============================================================================
class BUnitActionBuildingQueuedItem
{
   public:
      BUnitActionBuildingQueuedItem()
      {
         clear();
      }

      void clear()
      {
         mType=0;
         mCount=0;
         mID=-1;
         mLinkedResource=-1;
         mPlayerID=cInvalidPlayerID;
         mPurchasingPlayerID=cInvalidPlayerID;
         mTriggerScriptID=cInvalidTriggerScriptID;
         mTriggerVarID=cInvalidTriggerVarID;
         mTrainLimitBucket=0;
      }

      bool save(BStream* pStream, int saveType) const;
      bool load(BStream* pStream, int saveType);

      BYTE              mType;
      BYTE              mCount;
      short             mID;
      long              mLinkedResource;
      BPlayerID         mPlayerID;
      BPlayerID         mPurchasingPlayerID;
      BTriggerScriptID  mTriggerScriptID; // The script ID for the building command state var
      BTriggerVarID     mTriggerVarID;    // the var id for the building command state var
      uint8             mTrainLimitBucket;
};
typedef BSmallDynamicSimArray<BUnitActionBuildingQueuedItem> BUnitActionBuildingQueuedItemArray;

//==============================================================================
//==============================================================================
class BUnitActionBuildingCurrentItem
{
   public:
      BUnitActionBuildingCurrentItem()
      {
         clear();
      }

      void clear()
      {
         mState=-1;
         mCurrentType=-1;
         mCurrentID=-1;
         mPlayerID=cInvalidPlayerID;
         mPurchasingPlayerID=cInvalidPlayerID;
         mCurrentBuildingID=cInvalidObjectID;
         mCurrentLinkedResource=-1;
         mCurrentPoints=0.0f;
         mNextUpdatePercent=0.0f;
         mTotalPoints=0.0f;
         mTriggerScriptID=cInvalidTriggerScriptID;
         mTriggerVarID=cInvalidTriggerVarID;
         mTrainLimitBucket=0;
      }

      bool save(BStream* pStream, int saveType) const;
      bool load(BStream* pStream, int saveType);

      long              mState;
      long              mCurrentType;
      long              mCurrentID;
      BEntityID         mCurrentBuildingID;
      BPlayerID         mPlayerID;
      BPlayerID         mPurchasingPlayerID;
      long              mCurrentLinkedResource;
      float             mCurrentPoints;
      float             mNextUpdatePercent;
      float             mTotalPoints;
      BTriggerScriptID  mTriggerScriptID; // The script ID for the building command state var
      BTriggerVarID     mTriggerVarID;    // the var id for the building command state var
      uint8             mTrainLimitBucket;
};
typedef BSmallDynamicSimArray<BUnitActionBuildingCurrentItem> BUnitActionBuildingCurrentItemArray;

//==============================================================================
//==============================================================================
class BUnitActionBuildingRecharge
{
   public:
      bool save(BStream* pStream, int saveType) const;
      bool load(BStream* pStream, int saveType);

      int                        mID;
      float                      mTimer;
      bool                       mSquad;
};
typedef BSmallDynamicSimArray<BUnitActionBuildingRecharge> BUnitActionBuildingRechargeArray;

//==============================================================================
//==============================================================================
class BUnitActionBuildingTrainedSquad
{
   public:
      bool save(BStream* pStream, int saveType) const;
      bool load(BStream* pStream, int saveType);

      BEntityID                  mSquadID;
      BVector                    mPosition;
      bool                       mPlotted;
      bool                       mPlaySound;
};
typedef BSmallDynamicSimArray<BUnitActionBuildingTrainedSquad> BUnitActionBuildingTrainedSquadArray;

//==============================================================================
//==============================================================================
class BUnitActionBuilding : public BAction
{
   public:
      BUnitActionBuilding() { }
      virtual ~BUnitActionBuilding() { }

      //Connect/disconnect.
      virtual bool               connect(BEntity* pOwner, BSimOrder *pOrder);
      virtual void               disconnect();
      //Init.
      virtual bool               init();

      virtual bool               update(float elapsed);

      void                       addTrain(BPlayerID playerID, long id, long count, bool squad, bool noCost, BTriggerScriptID triggerScriptID=cInvalidTriggerScriptID, BTriggerVarID triggerVarID=cInvalidTriggerVarID, bool forcePlaySound=false);
      long                       getTrainCount(BPlayerID playerID, long id) const;
      void                       getTrainQueue(BPlayerID playerID, long*  pCount, float*  pTotalPoints) const;
      float                      getTrainPercent(BPlayerID playerID, long id) const;

      void                       addResearch(BPlayerID playerID, long techID, long count, bool noCost=false, BTriggerScriptID triggerScriptID=cInvalidTriggerScriptID, BTriggerVarID triggerVarID=cInvalidTriggerVarID);
      long                       getResearchCount(BPlayerID playerID, long id) const;
      float                      getResearchPercent(BPlayerID playerID, long id) const;
      bool                       getUniqueTechInfo(BPlayerID& playerID, long& techID, float& percentComplete) const;
      bool                       hasUniqueTech() const;

      void                       doBuild(BPlayerID playerID, bool cancel, bool noCost, bool fromSave);
      bool                       isBuild();
      float                      getBuildPercent() const;
      void                       addBuildPoints(float points);

      void                       doBuildOther(BPlayerID playerID, BPlayerID purchasingPlayerID, int id, bool cancel, bool noCost, bool doppleOnStart, BTriggerScriptID triggerScriptID, BTriggerVarID triggerVarID);
      long                       getBuildOtherCount(BPlayerID playerID, int id) const;
      float                      getBuildOtherPercent(BPlayerID playerID, int id) const;

      void                       doCustomCommand(BPlayerID playerID, int commandID, bool cancel);
      int                        getCustomCommandCount(BPlayerID playerID, int commandID) const;
      float                      getCustomCommandPercent(BPlayerID playerID, int commandID) const;

      void                       doSelfDestruct(bool cancel);
      bool                       isSelfDestructing(float& timeRemaining) const;

      bool                       isQueuedItem(BPlayerID playerID, int id) const;
      void                       clearQueue();

      long                       getTrainLimit(BPlayerID playerID, long id, bool squad) const;
      bool                       getTrainCounts(BPlayerID playerID, long id, long*  pCount, long*  pLimit, bool squad, bool buildingOther) const;

      bool                       getRecharging(bool squad, int protoID, float* pTimeRemaining) const; 
      void                       setRecharge(bool squad, int protoID, float timeRemaining);

      BPlayerID                  getCreatedByPlayerID() const { return mCreatedByPlayerID; }

      void                       queueTrainedSquad(BSquad* pSquad, bool playSound);
                                 
      float                      getRebuildTimer() const { return mRebuildTimer; };
      void                       setRebuildTimer(float val) { mRebuildTimer=val; }

      void                       overrideCreatedByPlayerID(BPlayerID id) { mCreatedByPlayerID = id; }

      bool                       hasDescriptionOverride() const { return mFlagDescriptionOverride; }
      void                       clearDescriptionOverride() { mFlagDescriptionOverride = false; }
      const BUString&            getDescriptionOverride() const { return mDescriptionOverride; }
      void                       setDescriptionOverride(const BUString& v) { mFlagDescriptionOverride = true; mDescriptionOverride = v; }


      DECLARE_FREELIST(BUnitActionBuilding, 5);

      virtual bool save(BStream* pStream, int saveType) const;
      virtual bool load(BStream* pStream, int saveType);

   protected:
      class BUBAResourceLink
      {
         public:
            BUnit*   mpUnit;
            float    mDistSqr;
            long     mCount;
      };

      class BUBADelayedDopple
      {
         public:
            BEntityID   mEntityID;
            int8        mCountDown;
      };

      typedef BSmallDynamicSimArray<BUBADelayedDopple> BUBADelayedDoppleArray;
      BUBADelayedDoppleArray     mDelayedDoppleList;

      long                       getTrainLimitAndResourceLinks(BPlayerID playerID, long id, BUBAResourceLink*  pResourceLinks, long maxResources, long*  pResourceCountOut, long*  pTrainCountOut, uint8* pTrainLimitBucketOut, bool squad) const;
      void                       unreserveTrainResourceLink(long resourceID, long trainID);

      BEntityID                  startBuildOther(BPlayerID playerID, BPlayerID purchasingPlayerID, int id, bool startBuilt);

      void                       completeBuild(BPlayerID playerID, bool noCost, bool fromSave);
      void                       completeBuildOther(BPlayerID playerID, BPlayerID purchasingPlayerID, bool noCost, bool doppleOnStart);
      void                       completeResearch(BPlayerID playerID, bool noCost);
      BEntityID                  completeTrain(BPlayerID playerID, bool noCost, bool forcePlaySound=false);
      void                       completeCustomCommand(BPlayerID playerID);

      uint                             getIndexForPlayer(BPlayerID playerID) const;
      BPlayerID                        getPlayerIDForIndex(uint index) const;
      BUnitActionBuildingCurrentItem&  getCurrentItemForPlayer(BPlayerID playerID);
      const BUnitActionBuildingCurrentItem&  getCurrentItemForPlayer(BPlayerID playerID) const;

      void                       checkTriggerState(BTriggerScriptID triggerScriptID, BTriggerVarID triggerVarID, BEntityID buildingID);
      void                       completeTriggerState(BTriggerScriptID triggerScriptID, BTriggerVarID triggerVarID, BEntityID buildingID);
      void                       updateTriggerStateTrain(BTriggerScriptID triggerScriptID, BTriggerVarID triggerVarID, bool squad, BEntityID trainID);

      void                       updateTrainedSquadBirth(float elapsed);
      void                       doTrainedSquadBirth(BEntityID squadID, BSimTarget rallyPoint, bool useRallyPoint, bool playSound);

      void                       updateRechargeList(float elapsed);

      bool                       updateMoveSquadsFromObstruction(float elapsed);

      void                       updateBuildAnimRate();

      BUnitActionBuildingQueuedItemArray  mQueuedItems;
      BUnitActionBuildingCurrentItemArray mCurrentItems;
      BUnitActionBuildingRechargeArray    mRechargeList;
      BUnitActionBuildingTrainedSquadArray mTrainedSquads;

      BPlayerID         mCreatedByPlayerID;
      DWORD             mBuildTime;
      DWORD             mDestructTime;
      float             mTrainedSquadBirthTime;
      int               mMoveSquadsCount;
      float             mMoveSquadsTimer;
      float             mRebuildTimer;
      float             mPrevBuildPct;
      float             mLastBuildPointsForAnimRate;
      BUString          mDescriptionOverride;
      bool              mFlagGatherLink:1;
      bool              mFlagAutoBuild:1;
      bool              mFlagCompleteBuildOnNextUpdate:1;
      bool              mFlagStartedBuilt:1;
      bool              mFlagCommandableByAnyPlayer:1;
      bool              mFlagCoop:1;
      bool              mFlagResearchSetAnimRate:1;
      bool              mFlagBuildSetAnimRate:1;
      bool              mFlagObstructionIsClear:1;
      bool              mFlagDescriptionOverride:1;
      bool              mFlagLastQuickBuild:1;
      bool              mFlagHideParentSocket:1;
};

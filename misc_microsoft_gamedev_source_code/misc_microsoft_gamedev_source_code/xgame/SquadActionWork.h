//==============================================================================
// SquadActionWork.h
// Copyright (c) 2007 Ensemble Studios
//==============================================================================

#pragma once
#include "action.h"
#include "UnitOpportunity.h"

//==============================================================================
//==============================================================================
class BSquadActionWork : public BAction
{
   public:
      BSquadActionWork() { }
      virtual ~BSquadActionWork() { }

      //Connect/disconnect.
      virtual bool               connect(BEntity* pOwner, BSimOrder* pOrder);
      virtual void               disconnect();
      //Init.
      virtual bool               init();

      virtual bool               setState(BActionState state);
      virtual bool               update(float elapsed);

      void                       setFlagDoneOnWorkOnce(bool v) { mFlagDoneOnWorkOnce=v; }
      void                       setFlagDoneOnOppComplete(bool v) { mFlagDoneOnOppComplete=v; }
      void                       setFlagSearchForPotentialTargets(bool v) { mFlagSearchForPotentialTargets=v; }
      void                       setSearchType(BProtoObjectID v) { mSearchType=v; }
      void                       setUnitOppType(BUnitOppType v) { mUnitOppType=v; }
      BUnitOppType               getUnitOppType() const { return mUnitOppType; }
      virtual void               notify(DWORD eventType, BEntityID senderID, DWORD data1, DWORD data2);
      virtual const BSimTarget*  getTarget() const { return (&mTarget); }
      virtual void               setTarget(BSimTarget target);

      #ifndef BUILD_FINAL
      virtual uint               getNumberDebugLines() const { return(2); }
      virtual void               getDebugLine(uint index, BSimString &string) const;
      #endif

      //User data.
      uint16                     getUserData() const  { return (mUserData); }
      void                       setUserData(uint16 v) { mUserData = v; mUserDataSet = true; }
      bool                       getUserDataSet() const  { return ( mUserDataSet ); }

      DECLARE_FREELIST(BSquadActionWork, 5);

      virtual bool save(BStream* pStream, int saveType) const;
      virtual bool load(BStream* pStream, int saveType);

   protected:
      bool                       validateTarget();
      bool                       validateRange();
      bool                       addOpps(BUnitOpp opp);
      void                       removeOpps();
      void                       searchForPotentialTargets();
      void                       validatePotentialTargets();
      bool                       captureConnect();
      void                       captureDisconnect();
      bool                       captureValidate();
      void                       unpack();

      BEntityIDArray             mPotentialTargets;
      BDynamicSimArray<uint>     mUnitOppIDs;
      BSimTarget                 mTarget;

      BActionID                  mChildActionID;
      BProtoObjectID             mSearchType;

      uint16                     mUserData;
      BActionState               mFutureState;
      BUnitOppType               mUnitOppType;
      
      bool                       mFlagDoneOnWorkOnce:1;
      bool                       mFlagDoneOnOppComplete:1;
      bool                       mFlagSearchForPotentialTargets:1;
      bool                       mFlagHasSearched:1;
      bool                       mFlagCapturePaid:1;
      bool                       mUserDataSet:1;
};

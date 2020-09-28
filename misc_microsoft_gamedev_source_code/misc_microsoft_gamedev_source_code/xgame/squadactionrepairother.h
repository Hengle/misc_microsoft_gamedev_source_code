//==============================================================================
// squadactionrepairother.h
// Copyright (c) 2008 Ensemble Studios
//==============================================================================

#pragma once
#include "action.h"
#include "squadactionrepair.h"

//==============================================================================
//==============================================================================
class BSquadActionRepairOther : public BAction
{
public:
   BSquadActionRepairOther() : BAction() { }
   virtual ~BSquadActionRepairOther() { }

   //IPoolable Methods
   //virtual void               onAcquire() { }
   //virtual void               onRelease() { }
   //Connect/disconnect.

   virtual bool               connect(BEntity* pOwner, BSimOrder* pOrder);
   virtual void               disconnect();
   //Init.
   virtual bool               init();

   virtual const BSimTarget*  getTarget() const { return (&mTarget); }
   //virtual bool               setState(BActionState state);
   virtual bool               update(float elapsed);
   virtual void               notify(DWORD eventType, BEntityID senderID, DWORD data1, DWORD data2);


   DECLARE_FREELIST(BSquadActionRepairOther, 4);

   virtual bool save(BStream* pStream, int saveType) const;
   virtual bool load(BStream* pStream, int saveType);

protected:

   void                       createEffect(BObject* pSquadLeader);
   void                       updateEffect();
   void                       destroyEffect();

   BEntityID                  mTargetSquad;
   BEntityID                  mFXAttachment;
   BEntityID                  mBeamHeadID;
   BEntityID                  mBeamTailID;
   BUnitOppID                 mUnitOppID;
   BSimTarget                 mTarget;
   BDynamicSimArray<BTeamID>  mAttackingTeamIDs;
   long                       mBoneHandle;
   bool                       mIsMoving:1;

   bool                       addUnitOpp();
   void                       removeUnitOpp();

};
//==============================================================================
// unitactionrage.h
// Copyright (c) 2005-2007 Ensemble Studios
//==============================================================================

#pragma once

#include "action.h"


//==============================================================================
//==============================================================================
class BUnitActionRage : public BAction
{
   public:
      BUnitActionRage () { }
      virtual ~BUnitActionRage() { }

      //Connect/disconnect.
      virtual bool               connect(BEntity* pOwner, BSimOrder* pOrder);
      virtual void               disconnect();
      //Init.
      virtual bool               init();

      virtual bool               setState(BActionState state);
      virtual bool               update(float elapsed);
      virtual void               notify(DWORD eventType, BEntityID senderID, DWORD data, DWORD data2);   

      DECLARE_FREELIST(BUnitActionRage, 5);

      void                       startJump(const BVector& targetLocation, BEntityID targetSquadId);
      void                       teleportToAndAttack(const BVector& targetLocation, BEntityID targetSquadId);

      void                       setMoveTarget(const BVector& target) { mMoveTarget = target; }
      void                       setMoveDirection(const BVector& direction) { mMoveDirection = direction; }
      void                       stopMoving();

      void                       setUsePather(bool newVal) { mUsePather = newVal; }
      void                       setTeleportObject(BProtoObjectID teleportObjectId) { mTeleportAttachObject = teleportObjectId; } 

      bool                       canBoard(BSquad& squad) const;

      bool                       isGroundAttacking() const { return mState == cRageStateAttacking; }
      bool                       isBoardedAttacking() const { return mState == cRageStateBoardedAttacking; }
      bool                       isJumping() const { return mState == cRageStateJumping; }

      virtual bool save(BStream* pStream, int saveType) const;
      virtual bool load(BStream* pStream, int saveType);

   private:
      typedef enum ERageState
      {
         cRageStateNone = cStateNone, 
         cRageStateDone = cStateDone, 
         cRageStateFailed = cStateFailed, 
         cRageStateMoving = cStateMoving, 
         cRageStateActive = cStateWorking,
         cRageStateWaiting = cStateWait,
         cRageStateAttacking = cStateAttacking,
         cRageStateJumping = cStateLockDown, 
         cRageStateJumpingFatality = cStateUnlock, 
         cRageStateBoardedJumping = cStateFading, 
         cRageStateBoardedAttacking = cStateLoading, 
         cRageStateBoardedFatality = cStateReloading, 
         cRageStateBoardedLeaving = cStateBlocked,
      };

      bool                       checkCollisions(BVector start, BVector end, BVector &iPoint);

      bool                       shouldStartMoving() const;
      void                       updateMoving(float elapsedTime);
      void                       detachFromBoarding(bool placeOnGround);

      bool                       validateControllers() const;
      bool                       grabControllers();
      void                       releaseControllers();

      void                       handleAttackTag(long attachmentHandle, long boneHandle);

      BEntityID                  mTargettedSquad;
      BVector                    mMoveTarget;
      BVector                    mMoveDirection;
      ERageState                 mFutureState;
      BProtoAction*              mpBoardedAttackProtoAction;
      BProtoObjectID             mTeleportAttachObject;

      bool                       mUsePather:1;
};

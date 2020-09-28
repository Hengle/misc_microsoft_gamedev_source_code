//==============================================================================
// action.h
// Copyright (c) 2005-2007 Ensemble Studios
//==============================================================================

#pragma once

#include "bitvector.h"
#include "poolable.h"
#include "path.h"
#include "SimOrder.h"
#include "SimTarget.h"
#include "SimTypes.h"
#include "triggerscript.h"
#include "triggervar.h"

class BEntity;
class BProtoAction;
class BWeapon;

const BActionID cInvalidActionID=0;


//==============================================================================
//==============================================================================
class BActionTriggerModule
{
   public:
      BActionTriggerModule()  { clearNotificationData(); }
      ~BActionTriggerModule() { }

      void                       clearNotificationData() { mTriggerScriptID=-1; mTriggerVarID=BTriggerVar::cVarIDInvalid; }
      void                       setNotificationData(BTriggerScriptID triggerScriptID, BTriggerVarID triggerVarID) { mTriggerScriptID=triggerScriptID; mTriggerVarID=triggerVarID; }
      void                       notifyActionStatus(long status);
      

   protected:
      long                       mTriggerScriptID;
      BTriggerVarID              mTriggerVarID;
};



//==============================================================================
//==============================================================================
class BActionController
{
   public:
      enum
      {
         cControllerOrient=0,
         cControllerAnimation,
         cNumberControllers
      };
   
      BActionController() { }
      ~BActionController() { }

      BActionID                  getActionID() const { return (mActionID); }
      void                       setActionID(BActionID v) { mActionID=v; }

      BUnitOppID                 getOppID() const { return (mOppID); }
      void                       setOppID(BUnitOppID v) { mOppID=v; }
      
      void                       init()
                                 {
                                    mActionID=cInvalidActionID;
                                    mOppID=0;
                                 }

   protected:
      BActionID                  mActionID;
      uint                       mOppID;
};



//==============================================================================
//==============================================================================
class BAction : public IPoolable
{
   public:


      typedef enum //(MAX 256 ENTRIES)
      {
         //-- Entity Actions
         cActionTypeEntityIdle=0,
	      cActionTypeEntityListen,

         //-- Unit Actions
         cActionTypeUnitMove,
         cActionTypeUnitMoveAir,
         cActionTypeUnitMoveWarthog,
         cActionTypeUnitMoveGhost,
         cActionTypeUnitRangedAttack,
         cActionTypeUnitBuilding,
         cActionTypeUnitDOT,
         cActionTypeUnitChangeMode,
         cActionTypeUnitDeath,
         cActionTypeUnitInfectDeath,
         cActionTypeUnitGarrison,
         cActionTypeUnitUngarrison,         
         cActionTypeUnitShieldRegen,
         cActionTypeUnitHonk,
         cActionTypeUnitSpawnSquad,
         cActionTypeUnitCapture,
         cActionTypeUnitJoin,
         cActionTypeUnitChangeOwner,
         cActionTypeUnitAmmoRegen,
         cActionTypeUnitPhysics,
         cActionTypeUnitPlayBlockingAnimation,
         cActionTypeUnitMines,
         cActionTypeUnitDetonate,
         cActionTypeUnitGather,
         cActionTypeUnitCollisionAttack,
         cActionTypeUnitAreaAttack,
         cActionTypeUnitUnderAttack,
         cActionTypeUnitSecondaryTurretAttack,
         cActionTypeUnitRevealToTeam,
         cActionTypeUnitAirTrafficControl,
         cActionTypeUnitHitch,
         cActionTypeUnitUnhitch,
         cActionTypeUnitSlaveTurretAttack,
         cActionTypeUnitThrown,
         cActionTypeUnitDodge,
         cActionTypeUnitDeflect,
         cActionTypeUnitAvoidCollisionAir,
         cActionTypeUnitPlayAttachmentAnims,
         cActionTypeUnitHeal,
         cActionTypeUnitRevive,
         cActionTypeUnitBuff,
         cActionTypeUnitInfect,
         cActionTypeUnitHotDrop,
         cActionTypeUnitTentacleDormant,
         cActionTypeUnitHeroDeath,
         cActionTypeUnitStasis,
         cActionTypeUnitBubbleShield,
         cActionTypeUnitBomb,
         cActionTypeUnitPlasmaShieldGen,
         cActionTypeUnitJump,
         cActionTypeUnitAmbientLifeSpawner,
         cActionTypeUnitJumpGather,
         cActionTypeUnitJumpGarrison,
         cActionTypeUnitJumpAttack,
         cActionTypeUnitPointBlankAttack,
         cActionTypeUnitRoar,
         cActionTypeUnitEnergyShield,
         cActionTypeUnitScaleLOS,
         cActionTypeUnitChargedRangedAttack,
         cActionTypeUnitTowerWall,
         cActionTypeUnitAoeHeal,

         //-- Squad Actions
         cActionTypeSquadAttack,
         cActionTypeSquadChangeMode,
         cActionTypeSquadRepair,
         cActionTypeSquadRepairOther,
         cActionTypeSquadShieldRegen,
         cActionTypeSquadGarrison,
         cActionTypeSquadUngarrison,
         cActionTypeSquadTransport,
         cActionTypeSquadPlayBlockingAnimation,
         cActionTypeSquadMove,
         cActionTypeSquadReinforce,
         cActionTypeSquadWork,
         cActionTypeSquadCarpetBomb,
         cActionTypeSquadAirStrike,
         cActionTypeSquadHitch,
         cActionTypeSquadUnhitch,
         cActionTypeSquadDetonate,
         cActionTypeSquadWander,
         cActionTypeSquadCloak,
         cActionTypeSquadCloakDetect,
         cActionTypeSquadDaze,
         cActionTypeSquadJump,
         cActionTypeSquadAmbientLife,
         cActionTypeSquadReflectDamage,
         cActionTypeSquadCryo,

         // -- And now, Platoon Action.  Ahh yeah.  DLM
         cActionTypePlatoonMove,

         cActionTypeUnitCoreSlide,
         cActionTypeUnitInfantryEnergyShield,
         cActionTypeUnitDome,
         cActionTypeSquadSpiritBond,
         cActionTypeUnitRage,

         cActionTypeInvalid
      };

      typedef enum
      {
        cStateNone=0,
        cStateDone,
        cStateFailed,
        cStateMoving,
        cStateWorking,
        cStateWait,
        cStateAttacking,
        cStateLockDown,
        cStateUnlock,
        cStateFading,
        cStateLoading,
        cStateReloading,
        cStateBlocked,
        cStateUnloadingWait,
        cStateUnloading,
        cStatePathing,
        cStateFlyIn,
        cStateFlyInWait,
        cStateFlyOff,
        cStateFlyOffWait,
        cStateIncoming,
        cStateReturning,
        cStateSearch,
        cStateHealingUnits,
        cStateReinforcingUnits,
        cStateUndamaged,
        cStateWaitOnOpps,
        cStatePaused,
        cStateUnpaused,
        cStateInvalid,
        cStateTurning,
        cStateStartMove,
        cStateStopMove,
        cStateRamming,
        cStateAdvanceWP,
        cStateWaitOnChildren
      };
      
      typedef enum
      {
         cChildActionDone=0,
         cChildActionFailed
      };

      BAction();
      virtual ~BAction();

      //IPoolable Methods
      virtual void               onAcquire() { }
      virtual void               onRelease() { }
      //Connect/disconnect.
      virtual bool               connect(BEntity* pOwner, BSimOrder *pOrder);
      virtual void               disconnect();
      //Init.
      virtual bool               init();

      virtual bool               isAllowedWhenGameOver() { return false; }

      //ID & Type.  They're secretly combined into the same variable.
      BActionID                  getID() const { return(mID); }
      void                       setID(BActionID id) { mID=id; }
      BActionType                getType() const;
      const char*                getTypeName() const;

      //ProtoAction.
      const BProtoAction*        getProtoAction() const { return(mpProtoAction); }
      virtual void               setProtoAction(const BProtoAction* pAction) { mpProtoAction=pAction; }

      //Owner.
      BEntity*                   getOwner() { return (mpOwner); }
      //Order.
      BSimOrder*                 getOrder() { return (mpOrder); }
      void                       setOrder(BSimOrder* pOrder) { mpOrder = pOrder; }
      
      //Conflicts.
      bool                       conflicts(const BAction* pAction) const;
      bool                       conflicts(BActionType type) const;

      //State.
      BActionState               getState() const { return(mState); }
      const char*                getStateName(int state = -1) const;
      virtual bool               setState(BActionState state);

      //Pause.
      virtual bool               pause() { return (false); }
      virtual bool               unpause() { return (false); }
      //Update.
      virtual bool               update(float /*elapsed*/) { return (true); }
     
      //Misc things that generally get overridden by derived classes as needed.
      virtual void               notify(DWORD eventType, BEntityID senderID, DWORD data1, DWORD data2) { }
      virtual void               setTriggerNotificationData(long /*triggerScriptID*/, BTriggerVarID /*triggerVarID*/) { }
      virtual void               setHitZoneIndex(long /*index*/) { }
      //OppID (and resulting things).
      virtual BUnitOppID         getOppID() const { return (0); }
      virtual void               setOppID(BUnitOppID /*v*/) { }
      virtual uint               getPriority() const { return (0); }
      //Target.
      virtual const BSimTarget*  getTarget() const { return (NULL); }
      virtual void               setTarget(BSimTarget /*target*/) { }
      virtual void               setTargetID(BEntityID /*targetID*/) { }
      virtual void               setTargetPosition(BVector /*targetPosition*/) { }
      //ParentAction.
      virtual BAction*           getParentAction() const { return(NULL); }
      virtual void               setParentAction(BAction* /*v*/) { }



      //Debug render.
      virtual void               debugRender() { }
      #ifndef BUILD_FINAL
      virtual uint               getNumberDebugLines() const { if (getTarget()) return (2); return (1); }
      virtual void               getDebugLine(uint index, BSimString &string) const;
      #endif
      
      virtual bool               isInterruptible() const    { return true; }
      virtual bool               attemptToRemove() { return true; }

      //-- Shared attack methods
      bool                       validateUnitAttackRange(const BSimTarget& target, DWORD& lastValidationTime) const;

      //Flags.
      bool                       getFlagActive() const { return(mFlagActive); }
      void                       setFlagActive(bool v) { mFlagActive=v; }
      bool                       getFlagDestroy() const { return(mFlagDestroy); }
      void                       setFlagDestroy(bool v) { mFlagDestroy=v; }
      bool                       getFlagPersistent() const { return(mFlagPersistent); }
      void                       setFlagPersistent(bool v) { mFlagPersistent=v; }
      bool                       getFlagFromTactic() const { return(mFlagFromTactic); }
      void                       setFlagFromTactic(bool v) { mFlagFromTactic=v; }
      bool                       getFlagConflictsWithIdle() const { return(mFlagConflictsWithIdle); }
      void                       setFlagConflictsWithIdle(bool v) { mFlagConflictsWithIdle=v; }
      bool                       getFlagMissingAnim() const { return(mFlagMissingAnim); }
      void                       setFlagMissingAnim(bool v) { mFlagMissingAnim=v; }
      bool                       getFlagAIAction() const { return(mFlagAIAction); }
      void                       setFlagAIAction(bool v) { mFlagAIAction=v; }
      bool                       getFlagCanHaveMultipleOfType() const { return(mFlagCanHaveMultipleOfType); }
      void                       setFlagCanHaveMultipleOfType(bool v) { mFlagCanHaveMultipleOfType=v; }

      GFDECLAREVERSION();
      virtual bool save(BStream* pStream, int saveType) const;
      virtual bool load(BStream* pStream, int saveType);
      virtual bool postLoad(int saveType);

   protected:
      void                       setAttachmentAnimationLock(bool newVal, bool yaw);
      bool                       checkAttachmentAnimationSet(long animType);

      bool                       sendStateChangeEvent();
      bool                       validateTag(DWORD eventType, BEntityID senderID, DWORD data, DWORD data2) const;
      bool                       limitVelocity(float elapsedTime, float desiredVelocity, float& velocity, bool useUpperBound=true);

      //-- Shared attack methods
      bool                       isTargetInRangeOfWeaponGroup(BWeapon* pWeapon, const BSimTarget& target) const;

      //Base vars.
      BEntity*                   mpOwner;
      BSimOrder*                 mpOrder;
      const BProtoAction*        mpProtoAction;
      BActionID                  mID;
      BActionState               mState;

      bool                       mFlagActive:1;
      bool                       mFlagDestroy:1;
      bool                       mFlagPersistent:1;
      bool                       mFlagFromTactic:1;
      bool                       mFlagConflictsWithIdle:1;
      bool                       mFlagMissingAnim:1;
      bool                       mFlagAIAction:1;
      bool                       mFlagCanHaveMultipleOfType:1;
};

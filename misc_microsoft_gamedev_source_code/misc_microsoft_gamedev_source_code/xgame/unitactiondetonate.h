//==============================================================================
// unitactiondetonate.h
// Copyright (c) 2006-2007 Ensemble Studios
//==============================================================================

#pragma once
#include "action.h"
#include "object.h"
#include "Physics/Dynamics/Collide/hkpCollisionListener.h"
#include "mptypes.h"


//==============================================================================
//==============================================================================
class BUnitActionDetonate : public BAction
{
   public:
      BUnitActionDetonate() { }
      virtual ~BUnitActionDetonate() { }

      //Connect/disconnect.
      virtual bool               connect(BEntity* pOwner, BSimOrder* pOrder);
      virtual void               disconnect();

      //Init.
      virtual bool               init();

      virtual bool               update(float elapsed);

      virtual void               notify(DWORD eventType, BEntityID senderID, DWORD data1, DWORD data2);

      //OppID (and resulting things).
      virtual BUnitOppID         getOppID() const { return (mOppID); }
      virtual void               setOppID(BUnitOppID v) { mOppID = v; }

      void                       setDeathTrigger() { mFlagDeathTrigger = true; }
      void                       setImmediateTrigger() { mFlagImmediateTrigger=true; }
      void                       setCountdownTrigger(DWORD delay) { mFlagCountdownTrigger=true; mCountdownRemaining=delay; }
      void                       setProximityTrigger(float radius) { mFlagProximityTrigger=true; mProximityRadius=radius; BASSERT(mProximityRadius >= 0.0f); }
      void                       setPhysicsTrigger(float threshold) { mFlagPhysicsTrigger=true; mPhysicsTriggerThreshold=threshold; }

      PlayerID                   getDetonationInstigator() const { return mDetonationInstigator; }
      void                       setDetonationInstigator(PlayerID playerId) { mDetonationInstigator = playerId; }

      bool                       getDetonated() const { return mDetonated; }

      virtual void               setProtoAction(const BProtoAction* pAction);

      void                       physicsCollision(float velocity);

      virtual bool               isInterruptible() const { if (mFlagImmediateTrigger) return false; else return true; }

      DECLARE_FREELIST(BUnitActionDetonate, 5);

      virtual bool save(BStream* pStream, int saveType) const;
      virtual bool load(BStream* pStream, int saveType);

   protected:
      bool                       isEnemyWithinProximity();
      void                       detonate();
      void                       startTimer(float useDuration, float durationSpread);


      class BDetonateCollisionListener* mpDetonateCollisionListener;
      float                      mPhysicsTriggerThreshold;

      DWORD                      mCountdownRemaining;
      float                      mProximityRadius;
      BUnitOppID                 mOppID;

      PlayerID                   mDetonationInstigator;

      bool                       mFlagProximityTrigger:1;
      bool                       mFlagCountdownTrigger:1;
      bool                       mFlagImmediateTrigger:1;
      bool                       mFlagDeathTrigger:1;
      bool                       mFlagPhysicsTrigger:1;  // [5/5/2008 xemu] triggers on shocks above the threshold, or on coming to rest after activation 
      bool                       mFlagPhysicsTriggerActivated:1;
      bool                       mDetonated:1;
};

//==============================================================================
//==============================================================================
class BDetonateCollisionListener : public hkpCollisionListener
{
public:

   BDetonateCollisionListener() : mpObject(NULL) {}

   void setObject(BObject* pObject) { mpObject = pObject; }
   void setAction(BUnitActionDetonate* pAction) { mpAction = pAction; }

   virtual void contactPointAddedCallback(hkpContactPointAddedEvent& event);

   virtual void contactPointConfirmedCallback( hkpContactPointConfirmedEvent& event) {}
   virtual void contactPointRemovedCallback( hkpContactPointRemovedEvent& event ) {}
   virtual void contactProcessCallback( hkpContactProcessEvent& event ) {}

   //IPoolable Methods
   virtual void               onAcquire() { }
   virtual void               onRelease() { }

   DECLARE_FREELIST(BDetonateCollisionListener, 4);

protected:

   BObject* mpObject;
   BUnitActionDetonate* mpAction;
};


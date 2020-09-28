//==============================================================================
// unitActionThrown.h
// Copyright (c) 2006-2007 Ensemble Studios
//==============================================================================

#pragma once
#include "action.h"
#include "havok\Source\Physics\Dynamics\Collide\hkpCollisionListener.h"

//==============================================================================
//==============================================================================
class BUnitActionThrown : public BAction, public hkpCollisionListener
{
   public:
      BUnitActionThrown() { }
      virtual ~BUnitActionThrown() { }

      // Init / Connect / Disconnect.
      virtual bool               init();
      virtual bool               connect(BEntity* pOwner, BSimOrder* pOrder);
      virtual void               disconnect();

      // Update
      virtual bool               update(float elapsed);

      // Accessors
      virtual BUnitOppID         getOppID() const { return (mOppID); }
      virtual void               setOppID(BUnitOppID v);
      void                       setThrowerID(BEntityID entity) { mThrowerID = entity; }
      void                       setThrowerProtoActionID(uint8 protoID) { mThrowerProtoActionID = protoID; }
      void                       setThrowVelocityScalar(float scalar) { mThrowVelocityScalar = scalar; }
      void                       addImpulse() { mNewThrow = true; }

      // Collision
      virtual void               contactPointAddedCallback(hkpContactPointAddedEvent& event);
      virtual void               contactPointConfirmedCallback( hkpContactPointConfirmedEvent& event) {}
      virtual void               contactPointRemovedCallback( hkpContactPointRemovedEvent& event ) {}
      virtual void               contactProcessCallback( hkpContactProcessEvent& event ) {}

      // Pool of actions
      DECLARE_FREELIST(BUnitActionThrown, 10);

      virtual bool save(BStream* pStream, int saveType) const;
      virtual bool load(BStream* pStream, int saveType);

   protected:
      void                       impulseUnit();
      void                       releaseToSimControl();

      bool                       validateControllers() const;
      bool                       grabControllers();
      void                       releaseControllers();

      bool                       createPhysicsObject(BUnit* pUnit);
      void                       releasePhysicsObject(BUnit* pUnit);

      BUnitOppID                 mOppID;
      BEntityID                  mThrowerID;
      long                       mThrowerProtoActionID;
      long                       mRestoreCollisionFilter;
      float                      mThrowVelocityScalar;
      bool                       mCollided:1;
      bool                       mNewThrow:1;
      bool                       mReleasePhysicsObject:1;
      bool                       mCollisionListener:1;
};
//==============================================================================
// UnitActionBomb.h
// Copyright (c) 2006-2007 Ensemble Studios
//==============================================================================

#pragma once
#include "action.h"
#include "havok\Source\Physics\Dynamics\Collide\hkpCollisionListener.h"

//==============================================================================
//==============================================================================
class BUnitActionBomb : public BAction, public hkpCollisionListener
{
   public:
      BUnitActionBomb() { }
      virtual ~BUnitActionBomb() { }

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

      // Collision
      virtual void               contactPointAddedCallback(hkpContactPointAddedEvent& event);
      virtual void               contactPointConfirmedCallback( hkpContactPointConfirmedEvent& event) {}
      virtual void               contactPointRemovedCallback( hkpContactPointRemovedEvent& event ) {}
      virtual void               contactProcessCallback( hkpContactProcessEvent& event ) {}

      // Pool of actions
      DECLARE_FREELIST(BUnitActionBomb, 10);

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
      long                       mRestoreCollisionFilter;
      BVector                    mDir;

      bool                       mCollided:1;
      bool                       mReleasePhysicsObject:1;
      bool                       mRoll:1;
      bool                       mCollisionListener;
};
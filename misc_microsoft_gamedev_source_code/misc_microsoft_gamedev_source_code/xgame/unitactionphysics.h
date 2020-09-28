//==============================================================================
// unitactionphysics.h
// Copyright (c) 2006-2007 Ensemble Studios
//==============================================================================

#pragma once
#include "action.h"
#include "object.h"
#include "Physics/Dynamics/Collide/hkpCollisionListener.h"

class BPhysicsObject;
class BPhysicsInfo;


//==============================================================================
//==============================================================================
class BClamshellCollisionListener : public hkpCollisionListener
{
   public:

      BClamshellCollisionListener() : mCollided(false), mCollisionProcessed(false) {}

      bool hasCollided() const { return mCollided; }
      bool getCollisionProcessed() const { return mCollisionProcessed; }
      void setCollisionProcessed(bool v) { mCollisionProcessed = v; }

      virtual void contactPointAddedCallback(hkpContactPointAddedEvent& event)
      {
         mCollided = true;
      }
      virtual void contactPointConfirmedCallback( hkpContactPointConfirmedEvent& event) {}
      virtual void contactPointRemovedCallback( hkpContactPointRemovedEvent& event ) {}
      virtual void contactProcessCallback( hkpContactProcessEvent& event ) {}

      //IPoolable Methods
      virtual void               onAcquire() { mCollided = false; mCollisionProcessed = false; }
      virtual void               onRelease() { }

      DECLARE_FREELIST(BClamshellCollisionListener, 4);

   protected:

      bool mCollided:1;
      bool mCollisionProcessed:1;
};


//==============================================================================
//==============================================================================
class BUnitActionPhysics : public BAction, public BPhysicsCollisionListener
{
   public:
      BUnitActionPhysics() { }
      virtual ~BUnitActionPhysics() { }

      //Connect/disconnect.
      virtual bool               connect(BEntity* pOwner, BSimOrder* pOrder);
      virtual void               disconnect();
      //Init.
      virtual bool               init();

      virtual bool               isAllowedWhenGameOver() { return getFlagCompleteOnInactivePhysics(); }

      virtual bool               update(float elapsed);
      virtual bool               setState(BActionState state);
      virtual void               notify(DWORD eventType, BEntityID senderID, DWORD data, DWORD data2);    

      void                       setupRocketOnDeath();
      bool                       testForJumpLine(BUnit *pUnit, BPhysicsObject *pPhysicsObject);

      void                       setPhysicsInfoID(long id) {mPhysicsInfoID = id; }
      long                       getPhysicsInfoID() const {return mPhysicsInfoID; } 

      void                       setImpactSoundSet(int8 impactSoundSet) { mImpactSoundSet = impactSoundSet; }
      int8                       getImpactSoundSet() const { return mImpactSoundSet; }

      void                       setTerrainEffect(long TerrainEffect) { mTerrainEffect = TerrainEffect; }
      long                       getTerrainEffect() const { return mTerrainEffect; }

      bool                       getFlagCompleteOnInactivePhysics() const { return(mFlagCompleteOnInactivePhysics); }
      void                       setFlagCompleteOnInactivePhysics(bool v) { mFlagCompleteOnInactivePhysics=v; }

      bool                       getFlagCompleteOnFirstCollision() const { return(mFlagCompleteOnFirstCollision); }
      void                       setFlagCompleteOnFirstCollision(bool v) { mFlagCompleteOnFirstCollision=v; }

      void                       setFlagEndMoveOnInactivePhysics(bool v) { mFlagEndMoveOnInactivePhysics=v; }

      void                       enableDynamicCollisionFilter(long collisionFilter, BEntityID parentID);

      void                       setFlagCollisionListener(bool v) { mFlagCollisionListener=v; }

      void                       setFlagEarlyTerminate(bool v) { mFlagEarlyTerminate=v; }

      void                       setFlagAllowEarlyTerminate(bool v) { mFlagAllowEarlyTerminate=v; }
      bool                       getFlagAllowEarlyTerminate() const { return(mFlagAllowEarlyTerminate); }

      // BPhysicsCollisionListener Methods
      virtual bool               collision(const BPhysicsCollisionData &collisionData);
      virtual bool               preCollision(const BPhysicsCollisionData &collisionData) { return false; }

      DECLARE_FREELIST(BUnitActionPhysics, 5);

      virtual bool save(BStream* pStream, int saveType) const;
      virtual bool load(BStream* pStream, int saveType);

   protected:

      void                       setAnimRate(float elapsedTime, BVector oldPosition, BVector oldFwd, BVector newFwd);

      BClamshellCollisionListener*  mpClamshellCollisionListener;
      long                          mPhysicsInfoID;

      BVector                       mImpactPosition;

      //Capps - testing. must opt.
      BVector                       mLastPosition;

      // Needed for changing collision filters of physics thrown objects
      BEntityID                     mParentEntityID;
      long                          mCollisionFilterInfo;

      //DJBFIXME: Move this stuff onto a pointer      
      float mRocketRotation;
      DWORD mRocketTime;
      DWORD mStartRocketTime;
      long  mTerrainEffect;

      float mGotoWaitTimer;

      int8  mImpactSoundSet;
      byte  mImpactSurfaceType;
      bool  mLastPositionValid;

      bool  mFlagFirstUpdate:1;
      bool  mFlagRocketOnDeath:1;      
      bool  mFlagRocketExplode:1;
      bool  mFlagFlyTowardCamera:1;
      //END rocket data
      bool  mFlagCompleteOnInactivePhysics:1;
      bool  mFlagEndMoveOnInactivePhysics:1;
      bool  mFlagImpactSound:1;
      bool  mFlagDynamicCollisionFilter:1;
      bool  mFlagTerrainEffect:1;
      bool  mFlagCompleteOnFirstCollision:1;
      bool  mFlagHadCollision:1;
      bool  mFlagRemoveCollisionListener:1;
      bool  mFlagCollisionListener:1;
      // DLM - 10/29/08 - we're not settling down.  We're done.
      bool  mFlagEarlyTerminate:1;
      bool  mFlagAllowEarlyTerminate:1;   // So it's not just enough to set early terminate, we need to allow us to check for early terminate. 

};

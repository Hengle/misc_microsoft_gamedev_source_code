//=================  =============================================================
// unitactiondetonate.cpp
// Copyright (c) 2006-2007 Ensemble Studios
//==============================================================================

#include "common.h"
#include "damagehelper.h"
#include "protoobject.h"
#include "tactic.h"
#include "unit.h"
#include "unitactiondetonate.h"
#include "unitquery.h"
#include "world.h"
#include "syncmacros.h"
#include "physics.h"
#include "physicsobject.h"
#include "unitactionphysics.h"
#include "soundmanager.h"
#include "worldsoundmanager.h"


//==============================================================================
//==============================================================================
IMPLEMENT_FREELIST(BDetonateCollisionListener, 4, &gSimHeap);
IMPLEMENT_FREELIST(BUnitActionDetonate, 5, &gSimHeap);

//==============================================================================
//==============================================================================
bool BUnitActionDetonate::connect(BEntity* pOwner, BSimOrder* pOrder)
{
   BASSERT(mpProtoAction);
   
   if (!BAction::connect(pOwner, pOrder))
      return(false);

   BUnit* pUnit=reinterpret_cast<BUnit*>(mpOwner);
   BASSERT(pUnit);

   // if this object is a physics detonate on death object, 
   // don't let the phys object die until after we detonate 
   if (pUnit->getProtoObject() && pUnit->getProtoObject()->getFlagPhysicsDetonateOnDeath() && pUnit->getFlagIsPhysicsReplacement())
   {
      // we want to make sure this object isn't destroyed when physics is idle
      BUnitActionPhysics* pAction = reinterpret_cast<BUnitActionPhysics*>(pUnit->getActionByType(BAction::cActionTypeUnitPhysics));
      pAction->setFlagCompleteOnInactivePhysics(false);      

      // hacky, but do this immediately for physics death objects
      if (pUnit->hasAnimation(cAnimTypeDetonated) && pUnit->getAnimationType(BObjectAnimationState::cAnimationStateMisc) != cAnimTypeDetonated)
         pUnit->setAnimation(mID, BObjectAnimationState::cAnimationStateMisc, cAnimTypeDetonated, true, true, -1, true);
   }

   if (mFlagProximityTrigger)
      mProximityRadius=mpProtoAction->getMaxRange(pUnit);

   if (mFlagPhysicsTrigger)
   {
      BASSERT(mpDetonateCollisionListener == NULL);
      mpDetonateCollisionListener = BDetonateCollisionListener::getInstance();
      mpDetonateCollisionListener->setObject(pUnit);
      mpDetonateCollisionListener->setAction(this);
      //pPhysicsObject->addHavokCollisionListener(mpDetonateCollisionListener);
      pUnit->getPhysicsObject()->addHavokCollisionListener(mpDetonateCollisionListener);
   }

   return(true);
}

//==============================================================================
void BUnitActionDetonate::disconnect()
{
   if (mpDetonateCollisionListener != NULL)
   {   
      BUnit* pUnit=reinterpret_cast<BUnit*>(mpOwner);
      if(pUnit && pUnit->getPhysicsObject() && pUnit->getPhysicsObject()->getRigidBody())
         pUnit->getPhysicsObject()->removeHavokCollisionListener(mpDetonateCollisionListener);

      BDetonateCollisionListener::releaseInstance(mpDetonateCollisionListener);
      mpDetonateCollisionListener = NULL;
   }

   BAction::disconnect();
}

//==============================================================================
//==============================================================================
bool BUnitActionDetonate::init()
{
   if (!BAction::init())
      return(false);

   mCountdownRemaining=0;
   mProximityRadius=0.0f;
   mFlagProximityTrigger=false;
   mFlagCountdownTrigger=false;
   mFlagImmediateTrigger=false;
   mFlagDeathTrigger=false;
   mFlagPhysicsTrigger=false;
   mFlagPhysicsTriggerActivated=false;
   mPhysicsTriggerThreshold = 0.0f;
   mDetonated=false;
   mDetonationInstigator=cInvalidPlayerID;
   return(true);
}

//==============================================================================
//==============================================================================
bool BUnitActionDetonate::update(float elapsed)
{
   switch (mState)
   {
      case cStateNone:
      {
         setState(cStateWorking);
         break;
      }

      case cStateWorking:
      {
//-- FIXING PREFIX BUG ID 1587
         const BUnit* pUnit=reinterpret_cast<BUnit*>(mpOwner);
//--
         BASSERT(pUnit);

         if (mFlagImmediateTrigger)
         {
            detonate();
            setState(cStateDone);
            break;
         }

         if (mFlagDeathTrigger && pUnit->getHitpoints() <= 0.0f)
         {
            detonate();
            setState(cStateDone);
            break;
         }

         if (mFlagCountdownTrigger)
         {
            // [5/15/2008 xemu] if we have a physics trigger, but are not yet activated, then don't do this
            if (!mFlagPhysicsTrigger || mFlagPhysicsTriggerActivated)
            {
               DWORD lastUpdateLength = gWorld->getLastUpdateLength();
               if (lastUpdateLength > mCountdownRemaining)
               {
                  detonate();
                  setState(cStateDone);
                  break;
               }
               else
                  mCountdownRemaining -= lastUpdateLength;
            }
         }
         else
         {
            if (mFlagPhysicsTrigger && mFlagPhysicsTriggerActivated)
            {
               // [5/6/2008 xemu] ok, now that we are "activated", once we come to rest, kaboom!
               BVector vel;
               pUnit->getPhysicsObject()->getLinearVelocity(vel);
               if (vel.length() < 0.1f)
               {
                  detonate();
                  setState(cStateDone);
               }
            }
         }

         if (mFlagProximityTrigger)
         {
            if (isEnemyWithinProximity())
            {
               detonate();
               setState(cStateDone);
               break;
            }
         }
      }
   }

   return (true);
}

//==============================================================================
//==============================================================================
void BUnitActionDetonate::notify(DWORD eventType, BEntityID senderID, DWORD data1, DWORD data2)
{
   if ((eventType == BEntity::cEventDamaged || eventType == BEntity::cEventStopped) && mFlagDeathTrigger)
   {
//-- FIXING PREFIX BUG ID 1588
      const BUnit* pUnit=reinterpret_cast<BUnit*>(mpOwner);
//--
      BASSERT(pUnit);

      if (pUnit->getHitpoints() <= 0.0f)
         detonate();
   }
}
//==============================================================================
//==============================================================================
bool BUnitActionDetonate::isEnemyWithinProximity()
{
//-- FIXING PREFIX BUG ID 1589
   const BUnit* pUnit=reinterpret_cast<BUnit*>(mpOwner);
//--
   BASSERT(pUnit);

   BEntityIDArray results(0, 100);
   BUnitQuery query(pUnit->getPosition(), 20.0f + mProximityRadius + pUnit->getObstructionRadius(), true);
   gWorld->getUnitsInArea(&query, &results);
   for (uint i=0; i < results.getSize(); i++)
   {
      BUnit*  pTempUnit=gWorld->getUnit(results[i]);
      if (!pTempUnit)
         continue;
      // Check range to edge of unit
      float edgeBuffer = pTempUnit->getObstructionRadius() + pUnit->getObstructionRadius();
      if (pUnit->getPosition().distance(pTempUnit->getPosition()) > (mProximityRadius + edgeBuffer))
         continue;
      if (pTempUnit ->getFlagFlying())
         continue;
      if (pTempUnit->getFlagGarrisoned())
         continue;
      if ((pTempUnit ->getPlayer()->isEnemy(pUnit->getPlayerID()) == false) || (pTempUnit ->getPlayerID() == 0))
         continue;
      if (pTempUnit ->getProtoObject()->getFlagNeutral())
         continue;
      if (!pTempUnit ->isAlive())
         continue;
      if (!pUnit->getFlagTiesToGround())   // if flying or airborne, check altitude difference
      {
         float unitAltitude = pUnit->getPosition().y;
         float targetAltitude = pTempUnit->getPosition().y;
         if ((unitAltitude - targetAltitude) > mProximityRadius)
            continue;
      }
      return (true);
   }

   return (false);
}

//==============================================================================
//==============================================================================
void BUnitActionDetonate::detonate()
{
   if (mDetonated)
      return;

   BUnit* pUnit=reinterpret_cast<BUnit*>(mpOwner);
   BASSERT(pUnit);

   //Blow up.
   if (mpProtoAction->getAOERadius() > 0.0f)
   {
      PlayerID instigatorPlayerId = pUnit->getPlayerID();
      BTeamID instigatorTeamId = pUnit->getTeamID();

      if (mDetonationInstigator != cInvalidPlayerID)
      {
         instigatorPlayerId = mDetonationInstigator;
//-- FIXING PREFIX BUG ID 1590
         const BPlayer* pPlayer = gWorld->getPlayer(mDetonationInstigator);
//--
         if (pPlayer)
            instigatorTeamId = pPlayer->getTeamID();
      }

      // show our impact effect / explosion
      gWorld->createTerrainEffectInstance(mpProtoAction, pUnit->getPosition(), cZAxisVector, instigatorPlayerId, pUnit->getFlagVisibleToAll());
      BEntityIDArray killedUnits;
      BDamageHelper::doAreaEffectDamage(instigatorPlayerId, instigatorTeamId, pUnit->getID(), (BProtoAction*)mpProtoAction, pUnit->getPosition(), cInvalidObjectID, cOriginVector, &killedUnits, cInvalidObjectID);
   }

   // Make the ultimate sacrifice.
   #ifdef SYNC_UnitAction
   syncUnitActionData("BUnitActionDetonate::detonate ownerID", pUnit->getID().asLong());
   #endif

   if (pUnit->hasAnimation(cAnimTypeDetonated) && pUnit->getAnimationType(BObjectAnimationState::cAnimationStateMisc) != cAnimTypeDetonated)
      pUnit->setAnimation(mID, BObjectAnimationState::cAnimationStateMisc, cAnimTypeDetonated, true, true, -1, true);

   if (pUnit->getProtoObject() && pUnit->getProtoObject()->getFlagPhysicsDetonateOnDeath() && pUnit->getFlagIsPhysicsReplacement())
   {
      // turn back on physics kill on inactive... 
      BUnitActionPhysics* pAction = reinterpret_cast<BUnitActionPhysics*>(pUnit->getActionByType(BAction::cActionTypeUnitPhysics));
      pAction->setFlagCompleteOnInactivePhysics(true);

      // and send the object flying - data drive this!
      BVector currentVelocity = pUnit->getVelocity();
      float maxLateralOffset = getProtoAction()->getDetonateThrowHorizontalMax();
      float maxVertOffset = getProtoAction()->getDetonateThrowVerticalMax();
      BVector impulseOffset = cOriginVector;

      currentVelocity.y = 0.0f;
      float minVelocityToTakeFromObject = 0.1f;
      if (currentVelocity.length() > minVelocityToTakeFromObject && currentVelocity.safeNormalize())
      {
         impulseOffset = currentVelocity;
         impulseOffset.x *= getRandRangeFloat(cSimRand, 0.5 * maxLateralOffset, maxLateralOffset);
         impulseOffset.z *= getRandRangeFloat(cSimRand, 0.5 * maxLateralOffset, maxLateralOffset);
         impulseOffset.y = getRandRangeFloat(cSimRand, 0.5 * maxVertOffset, maxVertOffset);
      }
      else
      {
         impulseOffset = BVector(getRandRangeFloat(cSimRand, -maxLateralOffset, maxLateralOffset),
            getRandRangeFloat(cSimRand, 0.5f * maxVertOffset, maxVertOffset),
            getRandRangeFloat(cSimRand, -maxLateralOffset, maxLateralOffset));
      }

      pUnit->getPhysicsObject()->applyImpulse(impulseOffset);

      //-- Play our impact sound 
      BCueIndex cueIndex = cInvalidCueIndex;
      const BProtoObject* pProto = pUnit->getProtoObject();
      if(pProto)
         cueIndex = pProto->getSound(cObjectSoundImpactDeath);

      if(cueIndex != cInvalidCueIndex)
         gWorld->getWorldSoundManager()->addSound(pUnit->getPosition(), cueIndex, false, cInvalidCueIndex, false, true);

      // add a decal
      if(pProto && pProto->getImpactDecal())
         gWorld->createImpactTerrainDecal(pUnit->getPosition(), pProto->getImpactDecal());
   }
   else
   {
      //We can't KILL the things because the action still needs an owner for updates... doh.
      //pUnit->kill();
      pUnit->setHitpoints(0.0f);

      // Don't leave a corpse, because we blew up
      pUnit->setFlagNoCorpse(true);
   }

   mDetonated = true;
}

//==============================================================================
//==============================================================================
void BUnitActionDetonate::setProtoAction(const BProtoAction* pAction)
{
   BASSERT(pAction);

   BAction::setProtoAction(pAction);

   // [4/28/2008 xemu] adding support to set some flags based on the data in our protoaction
   if (pAction->getDetonateFromPhysics())
   {
      setPhysicsTrigger(pAction->getPhysicsDetonationThreshold());
   }

   if (pAction->getDuration() > 0)
   {
      float useDuration = pAction->getDuration();
      // [4/28/2008 xemu] for designer readability, duration is +/- spread (so the total variance is actually 2 * spread)
      float durationSpread = pAction->getDurationSpread();
      startTimer(useDuration, durationSpread);
   }

   if (pAction->getDetonateWhenInRange())
   {
      mFlagProximityTrigger = true;
   }

   if (pAction->getDetonateOnDeath())
   {
      mFlagDeathTrigger = true;
   }
}

//==============================================================================
//==============================================================================
void BUnitActionDetonate::startTimer(float useDuration, float durationSpread)
{
   if (durationSpread > 0)
   {
      float randVal = getRandRangeFloat(cSimRand, -durationSpread, durationSpread);
      useDuration = useDuration + randVal;
      if (useDuration <= 0)
         useDuration = 0;
   }
   setCountdownTrigger((DWORD)(useDuration * 1000.0f));
}

//==============================================================================
//==============================================================================
void BUnitActionDetonate::physicsCollision(float velocity)
{
   if (Math::fAbs(velocity) > mPhysicsTriggerThreshold)
   {
      mFlagPhysicsTriggerActivated = true; 


   }
}

//==============================================================================
//==============================================================================
void BDetonateCollisionListener::contactPointAddedCallback(hkpContactPointAddedEvent& event)
{
   if (mpAction != NULL) 
   {
      mpAction->physicsCollision(event.m_projectedVelocity);
   }
}


//==============================================================================
//==============================================================================
bool BUnitActionDetonate::save(BStream* pStream, int saveType) const
{
   if (!BAction::save(pStream, saveType))
      return false;

   bool listener = (mpDetonateCollisionListener != NULL);
   GFWRITEVAR(pStream, bool, listener);

   GFWRITEVAR(pStream, float, mPhysicsTriggerThreshold);
   GFWRITEVAR(pStream, DWORD, mCountdownRemaining);
   GFWRITEVAR(pStream, float, mProximityRadius);
   GFWRITEVAR(pStream, BUnitOppID, mOppID);
   GFWRITEVAR(pStream, PlayerID, mDetonationInstigator);
   GFWRITEBITBOOL(pStream, mFlagProximityTrigger);
   GFWRITEBITBOOL(pStream, mFlagCountdownTrigger);
   GFWRITEBITBOOL(pStream, mFlagImmediateTrigger);
   GFWRITEBITBOOL(pStream, mFlagDeathTrigger);
   GFWRITEBITBOOL(pStream, mFlagPhysicsTrigger);
   GFWRITEBITBOOL(pStream, mFlagPhysicsTriggerActivated);
   GFWRITEBITBOOL(pStream, mDetonated);
   return true;
}

//==============================================================================
//==============================================================================
bool BUnitActionDetonate::load(BStream* pStream, int saveType)
{
   if (!BAction::load(pStream, saveType))
      return false;

   bool listener;
   GFREADVAR(pStream, bool, listener);
   if (listener)
   {
      mpDetonateCollisionListener = BDetonateCollisionListener::getInstance();
      mpDetonateCollisionListener->setObject(mpOwner->getUnit());
      mpDetonateCollisionListener->setAction(this);
      if (mpOwner->getPhysicsObject() && mpOwner->getPhysicsObject()->getRigidBody())
         mpOwner->getPhysicsObject()->addHavokCollisionListener(mpDetonateCollisionListener);
   }

   GFREADVAR(pStream, float, mPhysicsTriggerThreshold);
   GFREADVAR(pStream, DWORD, mCountdownRemaining);
   GFREADVAR(pStream, float, mProximityRadius);
   GFREADVAR(pStream, BUnitOppID, mOppID);
   GFREADVAR(pStream, PlayerID, mDetonationInstigator);
   GFREADBITBOOL(pStream, mFlagProximityTrigger);
   GFREADBITBOOL(pStream, mFlagCountdownTrigger);
   GFREADBITBOOL(pStream, mFlagImmediateTrigger);
   GFREADBITBOOL(pStream, mFlagDeathTrigger);
   GFREADBITBOOL(pStream, mFlagPhysicsTrigger);
   GFREADBITBOOL(pStream, mFlagPhysicsTriggerActivated);
   GFREADBITBOOL(pStream, mDetonated);

   return true;
}

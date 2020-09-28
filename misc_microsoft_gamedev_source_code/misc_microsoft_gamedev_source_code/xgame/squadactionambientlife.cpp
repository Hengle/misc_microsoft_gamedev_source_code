//==============================================================================
// squadactionambientlife.cpp
// Copyright (c) 2008 Ensemble Studios
//==============================================================================

// Includes
#include "common.h"
#include "squad.h"
#include "squadactionambientlife.h"
#include "world.h"
#include "actionmanager.h"
#include "database.h"
#include "config.h"
#include "configsgame.h"
#include "SimOrderManager.h"
#include "simhelper.h"
#include "squadactionmove.h"
#include "unitquery.h"
#include "TerrainSimRep.h"
#include "ability.h"
#include "SimOrderManager.h"
#include "math.h"

//==============================================================================
//==============================================================================
IMPLEMENT_FREELIST(BSquadActionAmbientLife, 4, &gSimHeap);

//==============================================================================
//==============================================================================
bool BSquadActionAmbientLife::connect(BEntity* pOwner, BSimOrder* pOrder)
{
   return BAction::connect(pOwner, pOrder);
}
//==============================================================================
//==============================================================================
void BSquadActionAmbientLife::disconnect()
{
   BSquad* pSquad=reinterpret_cast<BSquad*>(mpOwner);
   BASSERT(pSquad);

   pSquad->removeActionByID(mChildActionID);

   BAction::disconnect();
}

//==============================================================================
//==============================================================================
bool BSquadActionAmbientLife::init()
{
   if (!BAction::init())
      return(false);

   mWanderTimer = getRandRangeDWORD(cSimRand, 0, gDatabase.getALMaxWanderFrequency());
   mDevourTimer = 10000;
   mMaxWanderDistance = gDatabase.getALMaxWanderDistance();
   mMinWanderDistance = gDatabase.getALMinWanderDistance();
   mPredatorCheckTimer = gDatabase.getALPredatorCheckFrequency();
   mCurrentPreyTimer = gDatabase.getALPreyCheckFrequency();
   mAmbientLifeState = cStateWander;
   mChildActionID = cInvalidActionID;
   mFlagFleeing = false;
   mFlagLeavingMap = false;

   mDangerousSquad = cInvalidObjectID;
   mPreySquad = cInvalidObjectID;
   mCurrentPreyUnit = cInvalidObjectID;

   return(true);
}

//==============================================================================
//==============================================================================
bool BSquadActionAmbientLife::setState(BActionState state)
{
   BSquad* pSquad = mpOwner->getSquad();
   BASSERT(pSquad);

   switch(state)
   {
      //Moving.
   case cStateMoving:
      {
         BVector position = mTarget.getPosition();
         gTerrainSimRep.clampWorld(position);
         mTarget.setPosition(position);

         mChildActionID = pSquad->doMove(mpOrder, this, &mTarget, false, false, false, true);
         if (mChildActionID == cInvalidActionID)
         {
            setState(cStateWorking);
            return (true);
         }
         break;
      }
   }

   return (BAction::setState(state));
}

//==============================================================================
//==============================================================================
bool BSquadActionAmbientLife::update(float elapsed)
{
//-- FIXING PREFIX BUG ID 1572
   const BSquad* pSquad = mpOwner->getSquad();
//--
   BASSERT(pSquad);

   switch (mState)
   {
   case cStateNone:
      {
         setAmbientLifeState(cStateWander);
         setState(cStateWorking);
      }
      break;

   case cStateAttacking:
   case cStateMoving:
   case cStateWorking:
      {
         updateAmbientLifeState(elapsed);
      }
      break;

   case cStateDone:
      {

      }
      break;
   }

   return (true);
}

//==============================================================================
//==============================================================================
void BSquadActionAmbientLife::updateAmbientLifeState(float elapsed)
{
//-- FIXING PREFIX BUG ID 1573
   const BSquad* pSquad = mpOwner->getSquad();
//--
   BASSERT(pSquad);

   DWORD lastUpdateLength = gWorld->getLastUpdateLength();

   // DMG REFACTOR THIS
   if (mAmbientLifeState != cStateDevour)
   {
      if ( mPredatorCheckTimer <= lastUpdateLength)
      {
         mPredatorCheckTimer = gDatabase.getALPredatorCheckFrequency();
         updateOpps();

         if (mDangerousSquad != cInvalidObjectID)
         {
            setAmbientLifeState(cStateFlee);
         }
         else if (mPreySquad != cInvalidObjectID)
         {
            setAmbientLifeState(cStateHunt);
         }
      }
      else
      {
         mPredatorCheckTimer -= lastUpdateLength;
      }
   }

   switch (mAmbientLifeState)
   {
   case cStateWander:
      {
         if (mWanderTimer <= lastUpdateLength)
         {
            mWanderTimer = getRandRangeDWORD(cSimRand, 0, gDatabase.getALMaxWanderFrequency());
            wanderMove();
         }
         else
         {
            mWanderTimer -= lastUpdateLength;
         }
      }
      break;
   case cStateFlee:
      {
         // DMG NOTE: Possibly do some sort of startle animation. 
         if (mDangerousSquad != cInvalidObjectID)
            flee();
      }
      break;
   case cStateHunt:
      {
         if (mCurrentPreyTimer <= lastUpdateLength)
         {
            if (mPreySquad != cInvalidObjectID)
            {
               attack();
               mCurrentPreyTimer = gDatabase.getALPreyCheckFrequency();
            }
         }
         else
         {
            mCurrentPreyTimer -= lastUpdateLength;
         }
      }
      break;
   case cStateDevour:
      {
         // DMG NOTE: When the art is available it would be nice if the hunter devoured the prey as long as the corpse remained
         if (mDevourTimer <= lastUpdateLength)
         {
            // DMG NOTE: MAAAGIC
            mDevourTimer = 10000;
            setAmbientLifeState(cStateWander);
         }
         else
         {
            mDevourTimer -= lastUpdateLength;
         }
      }
      break;
   case cStateIdle:
      {
         // Do nothing
      }
      break;
   }
}

//==============================================================================
//==============================================================================
void BSquadActionAmbientLife::updateOpps()
{
   if (mFlagLeavingMap)
      return;

//-- FIXING PREFIX BUG ID 1574
   const BSquad* pSquad = mpOwner->getSquad();
//--
   BASSERT(pSquad);

   // ensure updateAmbientLifeState does something
   // reasonable in this failure case
   if(!pSquad)
   {
      mDangerousSquad = cInvalidObjectID;
      mPreySquad = cInvalidObjectID;
      return;
   }

   // Need a new target...
   BUnitQuery query(pSquad->getPosition(), gDatabase.getALOppCheckRadius(), false);
   query.setFlagIgnoreDead(true);

   BEntityIDArray results(0, 100);
   long numResults = gWorld->getSquadsInArea(&query, &results);

   float minDistanceToDangerousSquad = Math::fNearlyInfinite;
   float minDistanceToPreySquad = Math::fNearlyInfinite;
   
   BSquad* pCurSquad = NULL;
   mDangerousSquad = cInvalidObjectID;
   mPreySquad = cInvalidObjectID;

   for (long i = 0; i < numResults; ++i)
   {
      pCurSquad = gWorld->getSquad(results[i]);

      if (pCurSquad->getID() == pSquad->getID())
         continue;

      float tempDistance = pSquad->getPosition().distance(pCurSquad->getPosition());

      if (!pCurSquad->getPlayer()->isGaia() || pCurSquad->canAttackTarget(pSquad->getID()))
      {
         if ( tempDistance < minDistanceToDangerousSquad)
         {
            mDangerousSquad = pCurSquad->getID();
         }
      }

      if (pSquad->canAttackTarget(pCurSquad->getID()))
      {
         if ( tempDistance < minDistanceToPreySquad)
         {
            mPreySquad = pCurSquad->getID();
         }
      }
   }
}

//==============================================================================
//==============================================================================
void BSquadActionAmbientLife::setAmbientLifeState(BAmbientLifeState newState)
{
   #ifdef SYNC_Squad
      syncSquadData("BSquadActionAmbientLife::setAmbientLifeState newState", newState);
   #endif
   mAmbientLifeState = newState;
}

//==============================================================================
//==============================================================================
void BSquadActionAmbientLife::wanderMove()
{
//-- FIXING PREFIX BUG ID 1575
   const BSquad* pSquad = mpOwner->getSquad();
//--
   BASSERT(pSquad);

   if (!pSquad->getLeaderUnit())
      return;

   // DMG NOTE: Should probably check to see if the random location is within the map and traversable terrain.  Is there an easy way to do that?
   BVector randomLoc = BSimHelper::randomCircularDistribution(pSquad->getLeaderUnit()->getPosition(), mMaxWanderDistance, mMinWanderDistance);
   
   mTarget.invalidateID();
   mTarget.invalidatePosition();
   mTarget.setPosition(randomLoc);
   setState(cStateMoving);
   setAmbientLifeState(cStateIdle);
}

//==============================================================================
//==============================================================================
void BSquadActionAmbientLife::flee()
{
   BSquad* pDangerousSquad = gWorld->getSquad(mDangerousSquad);
   if (!pDangerousSquad)
      return;

   BSquad* pSquad = mpOwner->getSquad();
   BASSERT(pSquad);  

   if (!pSquad->getLeaderUnit() || !pDangerousSquad->getLeaderUnit())
      return;

   BIntrinsicVector dir = (pSquad->getLeaderUnit()->getPosition() - pDangerousSquad->getLeaderUnit()->getPosition());
   dir.y = 0;
   dir.normalizeEstimate();

   // Add some random variation to flee vector
   BVector upVector(0.0f, 1.0f, 0.0f);
   BVector deviation = getRandRangeFloat(cSimRand, -4.0f, 4.0f) * dir.cross(upVector);
   dir += deviation;
   dir.normalizeEstimate();

   if (mFlagLeavingMap)
      dir *= 20000.0f;
   else
   {
      float fleeDist = gDatabase.getALFleeDistance();
      dir *= getRandRangeFloat(cSimRand, 0.5f * fleeDist, fleeDist);
   }

   if (!mFlagFleeing)
   {
      mFlagFleeing = true;
      pSquad->setAbilityMovementSpeedModifier(BAbility::cMovementModifierMode, gDatabase.getALFleeMovementModifier(), false);
   }

   mTarget.invalidateID();
   mTarget.invalidatePosition();
   mTarget.setPosition(pSquad->getLeaderUnit()->getPosition() + dir);
   setState(cStateMoving);
   setAmbientLifeState(cStateIdle);
}

//==============================================================================
//==============================================================================
void BSquadActionAmbientLife::attack()
{
   BSquad* pSquad = mpOwner->getSquad();
   BASSERT(pSquad);

   BSquad* pPreySquad = gWorld->getSquad(mPreySquad);
   if (!pPreySquad)
      return;

   mTarget.invalidateID();
   mTarget.invalidatePosition();

   BUnit* pCurrentPreyUnit = gWorld->getUnit(mCurrentPreyUnit);
//-- FIXING PREFIX BUG ID 1576
   const BUnit* pPreviousPreyUnit = pCurrentPreyUnit;
//--
   
   pCurrentPreyUnit = NULL;
   mCurrentPreyUnit = cInvalidObjectID;

   float bestDistance = Math::fNearlyInfinite;
   for (uint i = 0; i < pPreySquad->getNumberChildren(); i++)
   {
      BUnit* pUnit = gWorld->getUnit(pPreySquad->getChild(i));
      if (pUnit)
      {
         float tempDistance = pSquad->calculateXZDistance(pUnit);

         if (tempDistance < bestDistance)
         {
            bestDistance = tempDistance;
            pCurrentPreyUnit = pUnit;
            mCurrentPreyUnit = pUnit->getID();
         }
      }
   }

   if (pCurrentPreyUnit && pPreviousPreyUnit != pCurrentPreyUnit)
   {
      mTarget.setID(pCurrentPreyUnit->getID());

      // Create an order
      BSimOrder* pOrder = gSimOrderManager.createOrder();
      BASSERT(pOrder);
      pOrder->setTarget(mTarget);
      pOrder->setPriority(BSimOrder::cPrioritySim);

      #ifdef SYNC_Squad
         if (pPreviousPreyUnit)
            syncSquadData("BSquadActionAmbientLife::attack previous prey", pPreviousPreyUnit->getID());
         syncSquadData("BSquadActionAmbientLife::attack current prey", pCurrentPreyUnit->getID());
      #endif

      mChildActionID = pSquad->doAttack(pOrder, this, &mTarget);

      if (mChildActionID == cInvalidActionID)
      {
         gSimOrderManager.markForDelete(pOrder);
         setState(cStateWorking);
         return;
      }

      setState(cStateAttacking);
      setAmbientLifeState(cStateHunt);
   }
}

//==============================================================================
//==============================================================================
void BSquadActionAmbientLife::notify(DWORD eventType, BEntityID senderID, DWORD data1, DWORD data2)
{
   BSquad* pSquad = reinterpret_cast<BSquad*>(mpOwner);

   switch (eventType)
   {
   case BEntity::cEventActionFailed:
      if (data1 == mChildActionID)
      {
         if (mState == cStateMoving)
         {
            if (mFlagFleeing)
            {
               mFlagFleeing = false;
               pSquad->setAbilityMovementSpeedModifier(BAbility::cMovementModifierMode, gDatabase.getALFleeMovementModifier(), true);
            }
            else if (mFlagLeavingMap)
            {
               // BALETED!!!!
               pSquad->kill(false);
            }
            setState(cStateNone);
         }
         else if (mState == cStateAttacking)
            setState(cStateNone);
      }
      break;

   case BEntity::cEventActionDone:
      {
         if (data1 == mChildActionID)
         {
            if (mState == cStateMoving)
            {
               setState(cStateWorking);
               setAmbientLifeState(cStateWander);

               if (mFlagFleeing)
               {
                  mFlagFleeing = false;
                  pSquad->setAbilityMovementSpeedModifier(BAbility::cMovementModifierMode, gDatabase.getALFleeMovementModifier(), true);
               }
               else if (mFlagLeavingMap)
               {
                  // BALETED!!!!
                  pSquad->kill(false);
               }
            }
            else if (mState == cStateAttacking)
            {
               setState(cStateWorking);
               setAmbientLifeState(cStateWander);
            }
         }
      }
      break;

   case BEntity::cEventDamaged:
      {
         BEntityID attackerID = (BEntityID)data1;

         BUnit* pUnit = gWorld->getUnit(attackerID);
         
         if (pUnit && !mFlagFleeing)
         {
            mDangerousSquad = pUnit->getParentID();
            setAmbientLifeState(cStateFlee);
         }

//-- FIXING PREFIX BUG ID 1579
         const BPlatoon* pPlatoon = pSquad->getParentPlatoon();
//--

         if (pPlatoon)
         {
            for (uint i = 0; i < pPlatoon->getNumberChildren(); i++)
            {
               BSquad* pAllySquad = gWorld->getSquad(pPlatoon->getChild(i));

               if (pAllySquad)
               {
//-- FIXING PREFIX BUG ID 1577
                  const BUnit* pAllyLeader = pAllySquad->getLeaderUnit();
//--
//-- FIXING PREFIX BUG ID 1578
                  const BUnit* pLeader = pSquad->getLeaderUnit();
//--

                  if (pAllyLeader && pLeader && (pAllyLeader->getPosition() - pLeader->getPosition()).lengthEstimate() < gDatabase.getALOppCheckRadius())
                     pAllySquad->notify(BEntity::cEventAllyDamaged, pSquad->getID(), data1, data2);
               }
            }
         }
      }
      break;

   case BEntity::cEventAllyDamaged:
      {
         BEntityID attackerID = (BEntityID)data1;

         BUnit* pUnit = gWorld->getUnit(attackerID);

         if (pUnit && !mFlagFleeing)
         {
            mDangerousSquad = pUnit->getParentID();
            setAmbientLifeState(cStateFlee);
         }
      }
      break;

   case BEntity::cEventKilledUnit:
      {
         mPreySquad = cInvalidObjectID;
         mCurrentPreyUnit = cInvalidObjectID;
         mTarget.invalidatePosition();
         mTarget.invalidateID();
         pSquad->removeActionByID(mChildActionID);
         setState(cStateWorking);
         setAmbientLifeState(cStateDevour);
      }
      break;
   }
}

//==============================================================================
//==============================================================================
void BSquadActionAmbientLife::fleeMap(BSquad* pEnemySquad)
{
   mFlagLeavingMap = true;
   mDangerousSquad = (pEnemySquad ? pEnemySquad->getID() : cInvalidObjectID);
}

//==============================================================================
//==============================================================================
bool BSquadActionAmbientLife::save(BStream* pStream, int saveType) const
{
   if (!BAction::save(pStream, saveType))
      return false;
   GFWRITEVAR(pStream, BAmbientLifeState, mAmbientLifeState);
   GFWRITEVAR(pStream, float, mMaxWanderDistance);
   GFWRITEVAR(pStream, float, mMinWanderDistance);
   GFWRITEVAR(pStream, DWORD, mWanderTimer);  
   GFWRITEVAR(pStream, DWORD, mPredatorCheckTimer);
   GFWRITEVAR(pStream, DWORD, mDevourTimer);
   GFWRITEVAR(pStream, DWORD, mCurrentPreyTimer);
   GFWRITEVAL(pStream, BEntityID, mDangerousSquad);
   GFWRITEVAL(pStream, BEntityID, mPreySquad);
   GFWRITEVAL(pStream, BEntityID, mCurrentPreyUnit);
   GFWRITEVAR(pStream, BActionID, mChildActionID);
   GFWRITECLASS(pStream, saveType, mTarget);
   GFWRITEBITBOOL(pStream, mFlagFleeing);
   GFWRITEBITBOOL(pStream, mFlagLeavingMap);
   return true;
}

//==============================================================================
//==============================================================================
bool BSquadActionAmbientLife::load(BStream* pStream, int saveType)
{
   if (!BAction::load(pStream, saveType))
      return false;
   GFREADVAR(pStream, BAmbientLifeState, mAmbientLifeState);
   GFREADVAR(pStream, float, mMaxWanderDistance);
   GFREADVAR(pStream, float, mMinWanderDistance);
   GFREADVAR(pStream, DWORD, mWanderTimer);  
   GFREADVAR(pStream, DWORD, mPredatorCheckTimer);
   GFREADVAR(pStream, DWORD, mDevourTimer);
   GFREADVAR(pStream, DWORD, mCurrentPreyTimer);
   GFREADVAR(pStream, BEntityID, mDangerousSquad);
   GFREADVAR(pStream, BEntityID, mPreySquad);
   GFREADVAR(pStream, BEntityID, mCurrentPreyUnit);
   GFREADVAR(pStream, BActionID, mChildActionID);
   GFREADCLASS(pStream, saveType, mTarget);
   GFREADBITBOOL(pStream, mFlagFleeing);
   GFREADBITBOOL(pStream, mFlagLeavingMap);
   return true;
}

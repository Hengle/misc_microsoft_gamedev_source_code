//=================  =============================================================
// unitactiontentacledormant.cpp
// Copyright (c) 2006-2007 Ensemble Studios
//==============================================================================

#include "common.h"
#include "damagehelper.h"
#include "protoobject.h"
#include "tactic.h"
#include "unit.h"
#include "unitactiontentacledormant.h"
#include "unitquery.h"
#include "world.h"
#include "syncmacros.h"
#include "xvisual.h"

//==============================================================================
//==============================================================================
IMPLEMENT_FREELIST(BUnitActionTentacleDormant, 5, &gSimHeap);

//==============================================================================
//==============================================================================
bool BUnitActionTentacleDormant::connect(BEntity* pOwner, BSimOrder* pOrder)
{
   if (!BAction::connect(pOwner, pOrder))
      return(false);

   // Setup IK on the tentacle
   BASSERT(mpOwner);
   BUnit* pUnit = static_cast<BUnit*>(mpOwner);
   BASSERT(pUnit);

   const BVisual* pVisual = pUnit->getVisual();
   BASSERT(pVisual);

   return(true);
}

//==============================================================================
//==============================================================================
bool BUnitActionTentacleDormant::init()
{
   if (!BAction::init())
      return(false);

   mEnemies = false;
   mNextProximityCheck = 0.0f;
   mFlagForcedActive = false;
   mActiveTimeLeft = 0.0f;
   
   return(true);
}

//==============================================================================
//==============================================================================
bool BUnitActionTentacleDormant::setState(BActionState state)
{
   BUnit* pUnit=reinterpret_cast<BUnit*>(mpOwner);
   BASSERT(pUnit);
   BASSERT(mpProtoAction);

   syncUnitActionData("BUnitActionTentacleDormant::setState owner ID", pUnit->getID());
   syncUnitActionData("BUnitActionTentacleDormant::setState state", state);

   switch (state)
   {
      case cStateNone:
      {
         break;
      }

      case cStateWorking:
      {
         pUnit->setAnimation(mID, BObjectAnimationState::cAnimationStateWork, mpProtoAction->getAnimType());
         break;
      }
   }

   return (BAction::setState(state));
}

//==============================================================================
//==============================================================================
bool BUnitActionTentacleDormant::update(float elapsed)
{
   bool enemies = mEnemies;

   mNextProximityCheck += elapsed;

   if (mpProtoAction && mNextProximityCheck > mpProtoAction->getWorkRate())
   {
      mNextProximityCheck = 0.0f;
      enemies = isEnemyWithinProximity();
   }

   if (mFlagForcedActive)
   {
      mActiveTimeLeft -= elapsed;

      if (mActiveTimeLeft <= 0.0f)
      {
         mActiveTimeLeft = 0.0f;
         mFlagForcedActive = false;
      }
   }

   switch (mState)
   {
      // Underground
      case cStateNone:
      {
         if (mFlagForcedActive || (enemies && !mEnemies))
         {
            setState(cStateWorking);
         }
         break;
      }

      // Aboveground
      case cStateWorking:
      {
         if (!enemies && mEnemies && !mFlagForcedActive)
         {
            setState(cStateNone);
         }
         break;
      }
   }

   mEnemies = enemies;

   return (true);
}

//==============================================================================
//==============================================================================
void BUnitActionTentacleDormant::notify(DWORD eventType, BEntityID senderID, DWORD data1, DWORD data2)
{
   if (eventType == BEntity::cEventDamaged)
   {
      if (mState != cStateWorking)
         setState(cStateWorking);
   }
}
//==============================================================================
//==============================================================================
bool BUnitActionTentacleDormant::isEnemyWithinProximity()
{
   BASSERT(mpOwner);
//-- FIXING PREFIX BUG ID 4928
   const BUnit* pUnit = reinterpret_cast<BUnit*>(mpOwner);
//--

   BEntityIDArray results(0, 100);
   float checkDist = mpProtoAction->getWorkRange();
   BUnitQuery query(pUnit->getPosition(), checkDist + pUnit->getObstructionRadius(), true);
   gWorld->getSquadsInArea(&query, &results, false);

   for (uint i=0; i < results.getSize(); i++)
   {
      // check the leader unit of the squad
      BSquad* pTempSquad = gWorld->getSquad(results[i]);
      if (!pTempSquad)
         continue;

      BUnit* pLeaderUnit = pTempSquad->getLeaderUnit();
      if (!pLeaderUnit)
         continue;

      if (pLeaderUnit->getFlagFlying())
         continue;
      if (pLeaderUnit->getFlagGarrisoned())
         continue;
      if ((pLeaderUnit->getPlayer()->isEnemy(pUnit->getPlayerID()) == false) || (pLeaderUnit->getPlayerID() == 0))
         continue;
      if (!pLeaderUnit->isAlive())
         continue;
      if (pLeaderUnit->getProtoObject()->getFlagNeutral())
         continue;
      return (true);
   }

   return (false);
}


//==============================================================================
//==============================================================================
void BUnitActionTentacleDormant::forceActive(float timeActive)
{
   mFlagForcedActive = true;
   mActiveTimeLeft = timeActive;
}

//==============================================================================
//==============================================================================
bool BUnitActionTentacleDormant::save(BStream* pStream, int saveType) const
{
   if (!BAction::save(pStream, saveType))
      return false;
   GFWRITEVAR(pStream, BUnitOppID, mOppID);
   GFWRITEVAR(pStream, float, mNextProximityCheck);
   GFWRITEVAR(pStream, bool, mEnemies);
   GFWRITEVAR(pStream, bool, mFlagForcedActive);
   GFWRITEVAR(pStream, float, mActiveTimeLeft);
   return true;
}

//==============================================================================
//==============================================================================
bool BUnitActionTentacleDormant::load(BStream* pStream, int saveType)
{
   if (!BAction::load(pStream, saveType))
      return false;
   GFREADVAR(pStream, BUnitOppID, mOppID);
   GFREADVAR(pStream, float, mNextProximityCheck);
   GFREADVAR(pStream, bool, mEnemies);
   GFREADVAR(pStream, bool, mFlagForcedActive);
   GFREADVAR(pStream, float, mActiveTimeLeft);
   return true;
}

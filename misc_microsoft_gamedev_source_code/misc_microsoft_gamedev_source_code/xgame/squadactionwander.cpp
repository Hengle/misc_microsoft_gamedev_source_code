//==============================================================================
// squadactionwander.cpp
// Copyright (c) 2008 Ensemble Studios
//==============================================================================

// Includes
#include "common.h"
#include "squad.h"
#include "squadactionWander.h"
#include "squadactionmove.h"
#include "world.h"
#include "actionmanager.h"
#include "database.h"
#include "config.h"
#include "configsgame.h"
#include "tactic.h"
#include "visual.h"
#include "simhelper.h"
#include "commands.h"
#include "SimOrderManager.h"
#include "unitquery.h"

//==============================================================================
//==============================================================================
IMPLEMENT_FREELIST(BSquadActionWander, 4, &gSimHeap);

//==============================================================================
//==============================================================================
bool BSquadActionWander::connect(BEntity* pEntity, BSimOrder* pOrder)
{
   if (!BAction::connect(pEntity, pOrder))
   {
      return (false);
   }

   // Grab current position
   mOrigin = pEntity->getPosition();

   // Initialize forward vector
   mForward = cInvalidVector;

   return (true);
}

//==============================================================================
//==============================================================================
void BSquadActionWander::disconnect()
{
   BASSERT(mpOwner);

   removeChildActions();

   if (mOppID != cInvalidActionID)
   {
//-- FIXING PREFIX BUG ID 1728
      const BSquad* pSquad = reinterpret_cast<BSquad*>(mpOwner);
//--

      if (pSquad)
         pSquad->removeOppFromChildren(mOppID);
   }

   BAction::disconnect();
}

//==============================================================================
//==============================================================================
bool BSquadActionWander::init()
{
   if (!BAction::init())
   {
      return (false);
   }

   mFlagConflictsWithIdle = true;
   mChildActionID = cInvalidActionID;
   mOppID = cInvalidActionID;

   return (true);
}

//==============================================================================
//==============================================================================
bool BSquadActionWander::setState(BActionState state)
{
   BASSERT(mpOwner);
   BSquad* pSquad = reinterpret_cast<BSquad*>(mpOwner);
   
   switch (state)
   {
      case cStateMoving:
         BVector targetPos;
         BSimTarget target;

         // See if there are any other similar entities nearby
         BEntityIDArray results(0, 100);
         BUnitQuery query(pSquad->getAveragePosition(), 5.0f, true);
         gWorld->getSquadsInArea(&query, &results);

         int numHits = 0;
         BVector awayVector(0.0f);

         for (uint i=0; i < results.getSize(); i++)
         {
            BSquad* pTempSquad = gWorld->getSquad(results[i]);
            if (!pTempSquad)
               continue;

            if (pTempSquad == pSquad)
               continue;

            if (pTempSquad && pTempSquad->getProtoID() == pSquad->getProtoID())
            {
               ++numHits;
               awayVector += (pSquad->getAveragePosition() - pTempSquad->getPosition());
            }
         }

         if (numHits > 0)
         {
            awayVector /= static_cast<float>(numHits);
            awayVector = -awayVector;
            awayVector.normalize();

            float dist = (pSquad->getAveragePosition() - mOrigin).length();
            targetPos = pSquad->getAveragePosition() + (mpProtoAction->getWorkRange() - dist) * awayVector;
            target.setPosition(targetPos);
         }
         else
         {
            while (!target.isPositionValid())
            {
               targetPos = BSimHelper::randomCircularDistribution(mOrigin, mpProtoAction->getWorkRange(), 0.0f);
               target.setPosition(targetPos);
            }
         }
         mTarget = targetPos;
         target.setRange(5.0f);

         BPlatoon* pPlatoon = pSquad->getParentPlatoon();
         if (pPlatoon)
         {
            pPlatoon->removeAllOrders();
            pPlatoon->queueMove(target, BSimOrder::cPriorityUser);
         }
         break;
   }

   return (BAction::setState(state));
}

//==============================================================================
//==============================================================================
bool BSquadActionWander::update(float elapsed)
{
   BASSERT(mpOwner);

   switch (mState)
   {
      case cStateNone:
         setState(cStateMoving);
         break;

      case cStateMoving:
         mWaitTime = 0.0f;
         setState(cStateWait);
         break;

      case cStateWait:
         mWaitTime += elapsed;

         if (mWaitTime > 5.0f)
         {
            setState(cStateMoving);
         }
         break;
   }

   #if !defined (BUILD_FINAL)
      if (gConfig.isDefined(cConfigRenderSimDebug))
      {      
         gpDebugPrimitives->addDebugSphere(mOrigin, mpProtoAction->getWorkRange(), cDWORDBlue);
         BVector out = mTarget;
         out.y += 5.0f;
         gpDebugPrimitives->addDebugLine(mTarget, out, cDWORDRed, cDWORDRed);
      }
   #endif

   return (true);
}

//==============================================================================
//==============================================================================
void BSquadActionWander::notify(DWORD eventType, BEntityID senderID, DWORD data1, DWORD data2)
{
   if (eventType == BEntity::cEventActionDone)
   {
      if (mState == cStateWait)
      {
         // Move to a new position
         setState(cStateMoving);
      }
   }
}

//==============================================================================
// Remove cached child action
//==============================================================================
void BSquadActionWander::removeChildActions()
{
   BSquad* pSquad = reinterpret_cast<BSquad*>(mpOwner);
   BASSERT(pSquad);

   // Remove the child action.
   pSquad->removeActionByID(mChildActionID);

   mChildActionID = cInvalidActionID;
}

//==============================================================================
//==============================================================================
bool BSquadActionWander::save(BStream* pStream, int saveType) const
{
   if (!BAction::save(pStream, saveType))
      return false;
   GFWRITEVAR(pStream, BUnitOppID, mOppID);
   GFWRITEVAR(pStream, BActionID, mChildActionID);
   GFWRITEVECTOR(pStream, mOrigin);
   GFWRITEVAR(pStream, float, mWaitTime);
   GFWRITEVECTOR(pStream, mForward);
   GFWRITEVECTOR(pStream, mTarget);
   return true;
}

//==============================================================================
//==============================================================================
bool BSquadActionWander::load(BStream* pStream, int saveType)
{
   if (!BAction::load(pStream, saveType))
      return false;
   GFREADVAR(pStream, BUnitOppID, mOppID);
   GFREADVAR(pStream, BActionID, mChildActionID);
   GFREADVECTOR(pStream, mOrigin);
   GFREADVAR(pStream, float, mWaitTime);
   GFREADVECTOR(pStream, mForward);
   GFREADVECTOR(pStream, mTarget);
   return true;
}

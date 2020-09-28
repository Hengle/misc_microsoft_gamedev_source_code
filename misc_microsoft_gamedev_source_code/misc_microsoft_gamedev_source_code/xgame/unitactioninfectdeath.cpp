//==============================================================================
// unitactioninfectdeath.cpp
// Copyright (c) 2006-2007 Ensemble Studios
//==============================================================================

#include "common.h"
#include "unitactioninfectdeath.h"
#include "entity.h"
#include "protoobject.h"
#include "unitactionmove.h"
#include "unit.h"
#include "world.h"
#include "game.h"
#include "usermanager.h"
#include "user.h"
#include "syncmacros.h"
#include "visualitem.h"
#include "visual.h"
#include "corpsemanager.h"
#include "configsgame.h"
#include "worldsoundmanager.h"

//==============================================================================
//==============================================================================
IMPLEMENT_FREELIST(BUnitActionInfectDeath, 5, &gSimHeap);

//==============================================================================
//==============================================================================
bool BUnitActionInfectDeath::connect(BEntity* pOwner, BSimOrder* pOrder)
{
   if (!BAction::connect(pOwner, pOrder))
      return (false);

   bool controllers = grabControllers();
   BASSERT(controllers);

   BUnit* pUnit = reinterpret_cast<BUnit*>(mpOwner);
   BASSERT(pUnit);
   if (mInfectionPlayerID != 0) // This is the second incarnation of InfectDeath in which the playerID is NOT Gaia
   {
      // Do birth animation
      pUnit->setAnimation(mID, BObjectAnimationState::cAnimationStateTrain, cAnimTypeTrain, true, false, -1, true);
      pUnit->computeAnimation();
      BASSERT(pUnit->isAnimationSet(BObjectAnimationState::cAnimationStateTrain, cAnimTypeTrain));
   }

   BASSERT(pUnit->getParentSquad());
   BASSERT(!pUnit->getParentSquad()->isSquadAPhysicsVehicle());

#ifdef SYNC_UnitAction
   syncUnitActionData("BUnitActionInfectDeath::connect unitID", pOwner->getID().asLong());
   #endif
   return (true);
}

//==============================================================================
//==============================================================================
void BUnitActionInfectDeath::disconnect()
{
   BASSERT(mpOwner);
   #ifdef SYNC_UnitAction
   syncUnitActionData("BUnitActionInfectDeath::disconnect unitID", mpOwner->getID().asLong());
   #endif

   releaseControllers();

   //Remove the child action.
   BUnit* pUnit = reinterpret_cast<BUnit*>(mpOwner);
   BASSERT(pUnit);
   BSquad* pSquad=pUnit->getParentSquad();

   // [4/21/2008 CJS] Removed, as we may not have a squad if we're partway through our "being infected" routine
   //BASSERT(pSquad);
   if (pSquad)
      pSquad->removeActionByID(mChildActionID);

   BAction::disconnect();
}

//==============================================================================
//==============================================================================
bool BUnitActionInfectDeath::init()
{
   if (!BAction::init())
      return (false);

   mFlagConflictsWithIdle = true;
   mChildActionID = cInvalidActionID;
   mFormerSquad = cInvalidObjectID;

   return (true);
}

//==============================================================================
//==============================================================================
bool BUnitActionInfectDeath::setState(BActionState state)
{
   BUnit* pUnit = reinterpret_cast<BUnit*>(mpOwner);
   BASSERT(pUnit);

   BASSERT(pUnit->getParentSquad());
   BASSERT(!pUnit->getParentSquad()->isSquadAPhysicsVehicle());

#ifdef SYNC_UnitAction
   syncUnitData("BUnitActionInfectDeath::setState unitID", pUnit->getID().asLong());
   syncUnitData("BUnitActionInfectDeath::setState state", state);
   #endif

   switch (state)
   {
      case cStateMoving:
      {
         BUnitOpp* pNewOpp=BUnitOpp::getInstance();
         pNewOpp->init();
         pNewOpp->setSource(pUnit->getID());
         pNewOpp->setTarget(mTarget);
         pNewOpp->setType(BUnitOpp::cTypeMove);
         pNewOpp->setLeash(false);
         pNewOpp->setNotifySource(false);
         pNewOpp->setPriority(BUnitOpp::cPriorityCommand);
         pNewOpp->generateID();
         pUnit->addOpp(pNewOpp);

         break;
      }
      case cStateWait:
      {
         if (mInfectionPlayerID == 0)
         {
            if (pUnit->hasAnimation(cAnimTypeFloodDeath) && !mbSkipDeathAnim)
            {
               pUnit->setAnimation(mID, BObjectAnimationState::cAnimationStateMisc, cAnimTypeFloodDeath, false, false, -1, true);
               pUnit->computeAnimation();
               BASSERT(pUnit->isAnimationSet(BObjectAnimationState::cAnimationStateMisc, cAnimTypeFloodDeath));
            }
            else
            {
               pUnit->setFlagTakeInfectionForm(true);
               pUnit->setFlagFloodControl(true);
               mState = cStateDone;
            }
         }

         break;
      }
      case cStateDone:
      {
         pUnit->removeOpps();
         break;
      }
      case cStateFailed:
      {
         //Remove the child action.
         pUnit->removeActionByID(mChildActionID);
         break;
      }     
   }

   return (BAction::setState(state));
}

//==============================================================================
//==============================================================================
bool BUnitActionInfectDeath::update(float elapsed)
{
   BUnit* pUnit=reinterpret_cast<BUnit*>(mpOwner);
   BASSERT(pUnit);

   #ifdef SYNC_UnitAction
   syncUnitActionData("BUnitActionInfectDeath::update unitID", mpOwner->getID().asLong());
   syncUnitActionData("BUnitActionInfectDeath::update mState", mState);
   #endif

   switch (mState)
   {
      case cStateNone:
      {
         if (!grabControllers())
            break;

         if (pUnit->isAnimationLocked())
            break;

         if (mInfectionPlayerID == 0)
         {
            if (findSpotToDie())
               setState(cStateMoving);
            else
            {
               pUnit->setFlagTakeInfectionForm(true);
               setState(cStateDone);
            }
         }
         else
            setState(cStateWait);

         break;
      }
      case cStateMoving:
      {
         BSquad* pParentSquad = pUnit->getParentSquad();
         if (pParentSquad)
            pParentSquad->setPosition(pUnit->getPosition());
//-- FIXING PREFIX BUG ID 4907
         const BUnitActionMove* pMoveAction = reinterpret_cast<BUnitActionMove*>(pUnit->getActionByType(BAction::cActionTypeUnitMove));
//--
         if (!pMoveAction)
         {
            // we need to make sure we can safely set to the waiting state. try again on next update if we can't get controllers or animate
            if (!grabControllers())
               break;

            if (pUnit->isAnimationLocked())
               break;

            setState(cStateWait);
         }
         break;
      }
      default:
         break;
   }
   
   return (true);
} 

//==============================================================================
//==============================================================================
void BUnitActionInfectDeath::notify(DWORD eventType, BEntityID senderID, DWORD data, DWORD data2)
{
   if (!validateTag(eventType, senderID, data, data2))
      return;

   BUnit* pUnit=reinterpret_cast<BUnit*>(mpOwner);
   BASSERT(pUnit);

   switch (eventType)
   {
      case BEntity::cEventAnimEnd:
      case BEntity::cEventAnimChain:
      {
         if (pUnit->getAnimationType(cActionAnimationTrack) == cAnimTypeFloodDeath)
         {
            pUnit->setFlagTakeInfectionForm(true);
            setState(cStateDone);
         }
         else if (pUnit->getAnimationType(cActionAnimationTrack) == cAnimTypeTrain)
         {
            pUnit->setFlagFloodControl(true);
            setState(cStateDone);
         }
         break;
      }
      default:
      break;
   }

}

//==============================================================================
//==============================================================================
bool BUnitActionInfectDeath::findSpotToDie()
{
//-- FIXING PREFIX BUG ID 4908
   const BUnit* pUnit = reinterpret_cast<BUnit*>(mpOwner);
//--
   BASSERT(pUnit);
   BASSERT(mFormerSquad!=cInvalidObjectID);

   BSquad* pFormerSquad = gWorld->getSquad(mFormerSquad);
   if (!pFormerSquad)
      return false;

   BVector location = cInvalidVector;
   BVector squadPos = pFormerSquad->getPosition();
   BVector suggestion = cInvalidVector;
   DWORD   flags      = BWorld::cCPCheckObstructions | BWorld::cCPSetPlacementSuggestion | BWorld::cCPExpandHull | BWorld::cCPIgnoreNonSolid;
   float moveDistance = pFormerSquad->getObstructionRadius() + pUnit->getObstructionRadius() + 5.0f;

   float xOffset[8];
   float zOffset[8];
   xOffset[0]=0.0f;      zOffset[0]=1.0f;
   xOffset[1]=0.707f;    zOffset[1]=0.707f;
   xOffset[2]=1.0f;      zOffset[2]=0.0f;
   xOffset[3]=0.707f;    zOffset[3]=-0.707f;
   xOffset[4]=0.0f;      zOffset[4]=-1.0f;
   xOffset[5]=-0.707f;   zOffset[5]=-0.707f;
   xOffset[6]=-1.0f;     zOffset[6]=0.0f;
   xOffset[7]=-0.707f;   zOffset[7]=0.707f;

   // Randomize the offset direction
   int randomInc = getRandRange(cSimRand, 0, 7);
   for (int i=0; i<8; i++)
   {
      int offsetDir = i + randomInc;
      if (offsetDir > 7)
         offsetDir -= 7;
      
      BVector offset;
      offset.x = xOffset[offsetDir];
      offset.y = 0.0f;
      offset.z = zOffset[offsetDir];
      offset.scale(moveDistance);
      location = squadPos + offset;

      if( gWorld->checkPlacement( pUnit->getProtoID(), pUnit->getPlayerID(), location, suggestion, pUnit->getForward(), BWorld::cCPLOSDontCare, flags, 1 ) )
      {
         mTarget.setPosition(location);
         mTarget.setRange(0.1f);
         return (true);
      }
   }

   return (false);
}

//==============================================================================
//==============================================================================
bool BUnitActionInfectDeath::validateControllers() const
{
   //Return true if we have the controllers we need, false if not.
//-- FIXING PREFIX BUG ID 4909
   const BUnit* pUnit=reinterpret_cast<BUnit*>(mpOwner);
//--
   BASSERT(pUnit);

   if (pUnit->getController(BActionController::cControllerAnimation)->getActionID() != mID)
      return (false);

   return(true);
}

//==============================================================================
//==============================================================================
bool BUnitActionInfectDeath::grabControllers()
{
   BUnit* pUnit=reinterpret_cast<BUnit*>(mpOwner);
   BASSERT(pUnit);

   return pUnit->grabController(BActionController::cControllerAnimation, this, getOppID());
}

//==============================================================================
//==============================================================================
void BUnitActionInfectDeath::releaseControllers()
{
   BUnit* pUnit=reinterpret_cast<BUnit*>(mpOwner);
   BASSERT(pUnit);

   pUnit->releaseController(BActionController::cControllerAnimation, this);
}

//==============================================================================
//==============================================================================
void BUnitActionInfectDeath::setOppID(BUnitOppID oppID)
{
   if (mpOwner)
   {
      BUnit* pUnit=reinterpret_cast<BUnit*>(mpOwner);
      BASSERT(pUnit);
      pUnit->updateControllerOppIDs(mOppID, oppID);
   }
   mOppID=oppID;
}

//==============================================================================
//==============================================================================
uint BUnitActionInfectDeath::getPriority() const
{
//-- FIXING PREFIX BUG ID 4910
   const BUnit* pUnit=reinterpret_cast<BUnit*>(mpOwner);
//--
   BASSERT(pUnit);
   const BUnitOpp* pOpp=pUnit->getOppByID(mOppID);
   if (!pOpp)
      return (BUnitOpp::cPriorityNone);
   return (pOpp->getPriority());
}

//==============================================================================
//==============================================================================
bool BUnitActionInfectDeath::save(BStream* pStream, int saveType) const
{
   if (!BAction::save(pStream, saveType))
      return false;
   GFWRITECLASS(pStream, saveType, mTarget);
   //BDynamicSimArray<uint> mUnitOppIDs;
   GFWRITEVAR(pStream, uint8, mInfectionPlayerID);
   GFWRITEVAR(pStream, BActionID, mChildActionID);
   GFWRITEVAR(pStream, BEntityID, mFormerSquad);
   GFWRITEVAR(pStream, bool, mbSkipDeathAnim);
   GFWRITEVAR(pStream, BUnitOppID, mOppID);
   return true;
}

//==============================================================================
//==============================================================================
bool BUnitActionInfectDeath::load(BStream* pStream, int saveType)
{
   if (!BAction::load(pStream, saveType))
      return false;
   GFREADCLASS(pStream, saveType, mTarget);
   //BDynamicSimArray<uint> mUnitOppIDs;
   GFREADVAR(pStream, uint8, mInfectionPlayerID);
   GFREADVAR(pStream, BActionID, mChildActionID);
   GFREADVAR(pStream, BEntityID, mFormerSquad);
   GFREADVAR(pStream, bool, mbSkipDeathAnim);
   GFREADVAR(pStream, BUnitOppID, mOppID);
   return true;
}

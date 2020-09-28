//==============================================================================
// unitactionhotdrop.cpp
// Copyright (c) 2008 Ensemble Studios
//==============================================================================

#include "common.h"
#include "database.h"
#include "unit.h"
#include "unitactionhotdrop.h"
#include "squad.h"
#include "aimission.h"
#include "objectmanager.h"
#include "world.h"
#include "commands.h"
#include "squadplotter.h"
#include "unitquery.h"
#include "SimOrderManager.h"
#include "tactic.h"
#include "physics.h"
#include "physicsobject.h"
#include "simhelper.h"
#include "visualitem.h"
#include "visual.h"
#include "squadactionungarrison.h"
#include "squadactiongarrison.h"
#include "unitactiongarrison.h"
#include "pather.h"
#include "powermanager.h"
#include "selectionmanager.h"
#include "user.h"
#include "usermanager.h"
#include "configsgame.h"
#include "platoonactionmove.h"

//==============================================================================
//==============================================================================
IMPLEMENT_FREELIST(BUnitActionHotDrop, 4, &gSimHeap);


//==============================================================================
//==============================================================================
bool BUnitActionHotDrop::connect(BEntity* pOwner, BSimOrder* pOrder)
{
   BDEBUG_ASSERT(mpProtoAction);

   //Connect.
   if (!BAction::connect(pOwner, pOrder))
      return false;

   BUnit* pUnit = reinterpret_cast<BUnit*>(mpOwner);
   BASSERT(pUnit);

   if (!mpProtoAction->getUseTeleporter())
      pUnit->setFlagBlockContain(true);

   return true;
}

//==============================================================================
//==============================================================================
void BUnitActionHotDrop::disconnect()
{
   BAction* pAction = NULL;
   BSquad* pSquad = NULL;

   for (uint i = 0; i < mCurrentSquads.size(); i++)
   {
      pSquad = gWorld->getSquad(mCurrentSquads[i].mSquadID);

      if (!pSquad)
         continue;

      // Remove the hotdrop beam for any squads still going
      BEntityID id = mCurrentSquads[i].mHotdropBeam;
      if (id != cInvalidObjectID)
      {
         BObject* pObject = gWorld->getObject(id);
         BASSERT(pObject);
         if (pObject)
            pObject->kill(false);
         else
            mCurrentSquads[i].mHotdropBeam = cInvalidObjectID;
      }
      if (mCurrentSquads[i].mValidExit)
      {
         pSquad->doTeleport(mCurrentSquads[i].mExitLoc, 1, false, false );
      }
      completeTeleport(pSquad);
      removeSquad(i);
   }

   // Clear any remaining ungarrison actions
   int count = mSquadToUngarrison.size();
   for (int i = 0; i < count; ++i)
   {
      BActionID ungarrison = mSquadToUngarrison[i].second;
      BEntityID squad = mSquadToUngarrison[i].first;

      if (ungarrison != cInvalidActionID)
      {
         pSquad = gWorld->getSquad(squad);

         if (pSquad)
         {
            pAction = pSquad->findActionByID(ungarrison);

            if (pAction)
            {
               pAction->notify(BEntity::cEventActionDone, cInvalidObjectID, getID(), NULL);
            }
         }
      }
   }

   mSquadToUngarrison.clear();
   mCurrentSquads.clear();

   BAction::disconnect();
}

//==============================================================================
//==============================================================================
bool BUnitActionHotDrop::init()
{
   if (!BAction::init())
      return false;

   mHotdropTarget = cInvalidObjectID;
   mHotdropBeam = cInvalidObjectID;
   mCurrentSquads.clear();

   return true;
}

//==============================================================================
//==============================================================================
bool BUnitActionHotDrop::update(float elapsed)
{
   // Don't allow hot drop pad to set the garrisoned squad's positions to the pad's position.
   // This fixes a problem where the squad position is set wrong while the ungarrison anim is playing,
   // causing physics vehicles to drift back to the pad location.
   BUnit* pUnit = reinterpret_cast<BUnit*>(mpOwner);
   BASSERT(pUnit);
   BSquad* pSquad = pUnit->getParentSquad();
   BASSERT(pSquad);
   pSquad->setFlagUpdateGarrisonedSquadPositions(false);
   
   BASSERT(mpProtoAction);

   // Turn the pad's beam on and off with our leader unit if we're a hotdrop
   if (!mpProtoAction->getUseTeleporter())
   {
      BPlayerID pid = pUnit->getPlayerID();
      BPlayer* pPlayer = gWorld->getPlayer(pid);
      BPowerManager* pPowerManager = gWorld->getPowerManager();

      if (pPlayer)
      {
         bool enable = true;

         if (pPlayer->getLeaderUnit() == cInvalidObjectID)
            enable = false;
         else
         {
            // If leader is too close, disable anyway
            BUnit* pLeader = gWorld->getUnit(pPlayer->getLeaderUnit());
            float dist = cMaximumFloat;

            if (pLeader)
               dist = pLeader->getPosition().xzDistanceSqr(pUnit->getPosition());

            if (dist < (mpProtoAction->getProjectileSpread() * mpProtoAction->getProjectileSpread()))
               enable = false;
            else
               enable = true;

            // if the leader is in a location where powers are unavailable, disable
            if (pLeader && pPowerManager && !pPowerManager->isValidPowerLocation(pLeader->getPosition()))
               enable = false;
         }

         // if the hotdrop pad is in a location where powers are unavailable, disable
         if (pPowerManager && !pPowerManager->isValidPowerLocation(pUnit->getPosition()))
            enable = false;

         if (!enable && mHotdropBeam != cInvalidObjectID)
         {
            // Remove and destroy beam
            pUnit->removeAttachment(mHotdropBeam);
            mHotdropBeam = cInvalidObjectID;

            pUnit->setFlagBlockContain(true);

            // Refresh UI
            if (pUnit->getPlayerID() == gUserManager.getPrimaryUser()->getPlayerID())
            {
               gUserManager.getPrimaryUser()->setFlagCommandMenuRefresh(true);
            }
            else
            {
               gUserManager.getSecondaryUser()->setFlagCommandMenuRefresh(true);
            }
         }
         else if (enable && mHotdropBeam == cInvalidObjectID)
         {
            // Create and attach beam
            mHotdropBeam = pUnit->addAttachment(gDatabase.getOTIDHotdropPadBeam());

            pUnit->setFlagBlockContain(false);

            // Refresh UI
            if (pUnit->getPlayerID() == gUserManager.getPrimaryUser()->getPlayerID())
            {
               gUserManager.getPrimaryUser()->setFlagCommandMenuRefresh(true);
            }
            else
            {
               gUserManager.getSecondaryUser()->setFlagCommandMenuRefresh(true);
            }
         }
      }
   }


   switch (mState)
   {
      case cStateNone:
         setState(cStateWorking);
         break;
         
      case cStateWorking:
      {
         // If leader waiting to garrison, do that first
         //garrisonWaitingLeader();

         transportGarrisonedSquads();

         if (mpProtoAction->getUseTeleporter())
            break;

         // Move squads up and down the beam
         BSquad* pSquad;

         int count = mCurrentSquads.size();
         for (int i = 0; i < count; ++i)
         {
            pSquad = gWorld->getSquad(mCurrentSquads[i].mSquadID);
            if (pSquad && pSquad->getLeaderUnit())
            {
               BVector loc = pSquad->getPosition();

               if (mCurrentSquads[i].mGoingUp)
               {
                  mCurrentSquads[i].mHeight += mpProtoAction->getWorkRate() * elapsed;
                  loc.y = mCurrentSquads[i].mHeight;
               }
               else
               {
                  BASSERT(Math::fNearlyInfinite - mCurrentSquads[i].mGroundLevel > cFloatCompareEpsilon); // Make sure ground level is set (it and exitloc are set together)

                  // Offset height for terrain position
                  loc.x = mCurrentSquads[i].mExitLoc.x;
                  loc.z = mCurrentSquads[i].mExitLoc.z;

                  mCurrentSquads[i].mHeight -= mpProtoAction->getWorkRate() * elapsed;
                  loc.y = mCurrentSquads[i].mHeight;
                  
                  float ground = mCurrentSquads[i].mGroundLevel;
                  if (pSquad->getLeaderUnit()->getProtoObject() && pSquad->getLeaderUnit()->getFlagFlying())
                  {
                     ground += pSquad->getLeaderUnit()->getProtoObject()->getFlightLevel();
                  }
                  if (loc.y < ground)
                  {
                     loc.y = ground;

                     BASSERT(Math::fNearlyInfinite - mCurrentSquads[i].mGroundLevel > cFloatCompareEpsilon); // Make sure ground level is set (it and exitloc are set together)

                     // If squad target has hit the ground, complete the teleport
                     BVector targetPos = mCurrentSquads[i].mExitLoc;
                     completeTeleport(pSquad);
                  }
               }

               pSquad->setPosition(loc);
               pSquad->setLeashPosition(loc);

               uint childCount = pSquad->getNumberChildren();
               BVector ofs;
               for (uint j = 0; j < childCount; ++j)
               {
                  BUnit* pChild = gWorld->getUnit(pSquad->getChild(j));
                  if (pChild)
                  {
                     ofs = pSquad->getFormationPosition(pChild->getID()) - pSquad->getPosition();
                     ofs.y = 0.0f;
                     pChild->setPosition(loc + ofs);

                     if (!mCurrentSquads[i].mGoingUp && loc.y < mCurrentSquads[i].mGroundLevel)
                     {
                        pChild->setTieToGround(!pUnit->getProtoObject()->getFlagNoTieToGround());
                        pChild->tieToGround();
                     }
                  }
               }
            }
            // Something killed the squad, stop updating it and remove it
            else
            {
               BEntityID id = mCurrentSquads[i].mHotdropBeam;
               if (id != cInvalidObjectID)
               {
                  BObject* pObject = gWorld->getObject(id);
                  BASSERT(pObject);
                  if (pObject)
                     pObject->kill(false);
                  else
                     mCurrentSquads[i].mHotdropBeam = cInvalidObjectID;
               }

               mCurrentSquads[i].mValid = false;
            }
         }

         // Remove all dead entries
         count = mCurrentSquads.size();
         for (int i = count - 1; i >= 0; i--)
         {
            if (!mCurrentSquads[i].mValid)
            {
               // [11-24-08 CJS] We still want to wait for the ungarrison to complete, so don't remove us as its parent
               //removeSquad(i);

               // Ungarrison is still running, cache it off so we can clear it if need be
               if (mCurrentSquads[i].mUngarrisonID != cInvalidActionID)
               {
                  SquadUngarrisonPair su(mCurrentSquads[i].mSquadID, mCurrentSquads[i].mUngarrisonID);
                  mSquadToUngarrison.add(su);
               }
               mCurrentSquads.erase(i);
            }
         }
      }
      break;
   }

   return true;
}
//==============================================================================
//==============================================================================
void BUnitActionHotDrop::removeSquad(int index)
{
   if ( index < 0 || index >= mCurrentSquads.size())
      return;

   BAction* pAction = NULL;
   BSquad* pSquad = NULL;

   if (mCurrentSquads[index].mUngarrisonID != cInvalidActionID)
   {
      pSquad = gWorld->getSquad(mCurrentSquads[index].mSquadID);

      if (pSquad)
      {
         pAction = pSquad->findActionByID(mCurrentSquads[index].mUngarrisonID);

         if (pAction)
         {
            pAction->notify(BEntity::cEventActionDone, cInvalidObjectID, getID(), NULL);
            mCurrentSquads[index].mUngarrisonID = cInvalidActionID;
         }
      }
   }

}

//==============================================================================
//==============================================================================
int BUnitActionHotDrop::addGarrisonedSquad(BSquad& squad)
{
   // add the entry if there isn't already one
   int index = findSquad(squad.getID());
   if (index == -1)
   {
      SquadInfo si;

      si.mFlagGarrison = true;
      si.mHeight = squad.getPosition().y;
      si.mGoingUp = true;
      si.mValid = true;
      si.mUngarrisonID = cInvalidActionID;
      // DLM 11/25/08 - initialize mValidExit & mExitLoc & mHotdropbeam to something legit.
      si.mValidExit = false;
      si.mExitLoc = cOriginVector;
      si.mHotdropBeam = cInvalidObjectID;

      // NOTE: si.mExitLoc and si.mGroundLevel are not initialized here intentionally, as they are not used
      // until after the unit has been hotdropped (and are calculated when the hotdrop starts for more accurate
      // results).  Likewise for mHotdropBeam, since it's not created until we have the spot to hotdrop to...

      si.mGroundLevel = Math::fNearlyInfinite;

      // [8-19-08 CJS] If the squad has an air avoidance action, disable it so it won't warp us around
      BAction* pAction = (squad.getLeaderUnit() ? squad.getLeaderUnit()->getActionByType(BAction::cActionTypeUnitAvoidCollisionAir) : NULL);
      if (pAction && pAction->getFlagActive())
      {
         si.mReenableAvoidAir = true;
         pAction->setFlagActive(false);
      }
      else
         si.mReenableAvoidAir = false;

      si.mSquadID = squad.getID();
      index = mCurrentSquads.add(si);

      // Tell unit and its squad to ignore range check on garrison (so we can sneak it across the map)
      BSquadActionGarrison* pSquadAction = (BSquadActionGarrison*)squad.getActionByType(BAction::cActionTypeSquadGarrison);
      if (pSquadAction)
         pSquadAction->setIgnoreRange(true);

      // do the child specific things
      const BEntityIDArray& children = squad.getChildList();
      for (uint i = 0; i < children.getSize(); ++i)
      {
         BUnit* pChildUnit = gWorld->getUnit(children[i]);

         if (!pChildUnit)
            continue;

         BUnitActionGarrison* pUnitAction = (BUnitActionGarrison*)pChildUnit->getActionByType(BAction::cActionTypeUnitGarrison);
         if (pUnitAction)
            pUnitAction->setIgnoreRange(true);

         // Set unit to invulnerable so it doesn't die during hotdrop
         pChildUnit->setFlagInvulnerable(true);

         // Flag it so we don't accidentally camera track o nit
         pChildUnit->setFlagAllowStickyCam(false);

         // Disable tie to ground and physics so we can move it manually
         pChildUnit->setTieToGround(false);
         pChildUnit->setPhysicsKeyFramed(true);
      }
   }

   return index;
}

//==============================================================================
//==============================================================================
void BUnitActionHotDrop::notify(DWORD eventType, BEntityID senderID, DWORD data, DWORD data2)
{
   switch (eventType)
   {
      case BEntity::cEventActionDone:
      {
//-- FIXING PREFIX BUG ID 5014
         const BUnit* pUnit = reinterpret_cast<BUnit*>(mpOwner);
//--
         BASSERT(pUnit);
         BSquad* pSquad = gWorld->getSquad(senderID);
         BASSERT(pSquad);

         pSquad->setFlagTeleported(false);

         // Special case handling for squads in missions.
         applyAIMissionBookkeeping(*pSquad);
         
         // Clear the ungarrison action out of our list
         int count = mSquadToUngarrison.size();
         for (int i = 0; i < count; ++i)
         {
            if (mSquadToUngarrison[i].second == data)
            {
               mSquadToUngarrison.erase(i);
               break;
            }
         }
         
         int index = findSquad(pSquad->getID());
         if (index == -1)
            break;

         removeSquad(index);
      }
      break;

      case BEntity::cEventGarrisonStart:
      {
         if (mpProtoAction->getUseTeleporter())
            break;

         BUnit* pSender = gWorld->getUnit(senderID);
         if (!pSender)
            break;

         BSquad* pSquad = pSender->getParentSquad();
         if (pSquad)
            addGarrisonedSquad(*pSquad);
      }
      break;

      // This happens if the leader unit is killed 
      case BEntity::cEventGarrisonFail:
      {
         if (mpProtoAction->getUseTeleporter())
            break;

         BUnit* pSenderUnit = gWorld->getUnit(senderID);
         if (!pSenderUnit)
            break;

         BSquad* pSquad = pSenderUnit->getParentSquad();
         if (!pSquad)
            break;

         int index = findSquad(pSquad->getID());
         if (index == -1)
            break;

         mCurrentSquads[index].mGoingUp = false;
         completeTeleport(pSquad);
         removeSquad(index);
         mCurrentSquads.erase(index);

         // Clear the ungarrison action out of our list
         int count = mSquadToUngarrison.size();
         for (int i = 0; i < count; ++i)
         {
            if (mSquadToUngarrison[i].second == data)
            {
               mSquadToUngarrison.erase(i);
               break;
            }
         }
      }
      break;

      case BEntity::cEventSquadUngarrisonStart:
      {
         BSquad* pSquad = gWorld->getSquad(senderID);
         if (!pSquad)
            break;

         // Set teleported flag so protectors can jump over if need be
         pSquad->setFlagTeleported(true);

         if (mpProtoAction->getUseTeleporter())
         {
            break;
         }

         int index = findSquad(pSquad->getID());
         if (index == -1)
            break;

         // Offset height to match terrain
         mCurrentSquads[index].mGoingUp = false;

         // Get height at old position
         BVector loc = pSquad->getPosition();
         float startGroundPos;
         gTerrainSimRep.getHeightRaycast(loc, startGroundPos, true);

         // Offset start height by height difference
         mCurrentSquads[index].mHeight += (mCurrentSquads[index].mGroundLevel - startGroundPos);

         // set the new position and update location
         // make sure that our formations are valid, so the up / down interpolations work
         // Offset height for terrain position
         BVector newLoc(mCurrentSquads[index].mExitLoc.x, mCurrentSquads[index].mGroundLevel, mCurrentSquads[index].mExitLoc.z);
         pSquad->setPosition(newLoc);
         pSquad->setLeashPosition(newLoc);
         pSquad->updateFormation();

         /*BUnit *pCovLeaderUnit = getHotdropTarget();
         if (pCovLeaderUnit)
         {         
             BSquad* pCovLeaderSquad = pCovLeaderUnit->getParentSquad();
             if (pCovLeaderSquad)           
                pCovLeaderSquad->setFlagTeleported(true);  // Set teleported flag so protectors can jump over if need be
         }*/
      }
      break;
   }
}

//==============================================================================
//==============================================================================
void BUnitActionHotDrop::applyAIMissionBookkeeping(BSquad& rHotDroppedSquad)
{
   // If the hotdropped squad was in a mission, ungroup the squad.
   // The squad will re-group on the 'other side' of the teleport.
   BAIMission* pMission = gWorld->getAIMission(rHotDroppedSquad.getAIMissionID());
   if (pMission)
      pMission->ungroupSquad(rHotDroppedSquad.getID());
}


//==============================================================================
//==============================================================================
BUnit* BUnitActionHotDrop::getHotdropTarget()
{
   BASSERT(mpOwner);
   BASSERT(mpProtoAction);

//-- FIXING PREFIX BUG ID 5015
   const BUnit* pOwner = reinterpret_cast<BUnit*>(mpOwner);
//--
   BASSERT(pOwner);

   BUnit *pUnit;

   if (mHotdropTarget != cInvalidObjectID)
   {
      BSquad* pSquad = gWorld->getSquad(mHotdropTarget);      
      if (pSquad)
      {
         pUnit = pSquad->getLeaderUnit();
         return (pUnit);
      }
      // Halwes - 8/19/2008 - Our target has been destroyed look for a new one.
      else
      {
         mHotdropTarget = cInvalidObjectID;
      }
   }

   // Go through global unit list to find the hero unit
   BEntityHandle handle = cInvalidObjectID;
   while ((pUnit = gWorld->getNextUnit(handle)) != NULL)
   {
      if (pUnit == pOwner)
         continue;

      if (mpProtoAction->getUseTeleporter())
      {
         if (pUnit->getPlayerID() == cGaiaPlayer && pUnit->isType(gDatabase.getOTIDTeleportDropoff()))
         {
            if (pUnit->getParentSquad())
               mHotdropTarget = pUnit->getParentSquad()->getID();
            return pUnit;
         }
      }
      else
      {
         // Check type and player ID
         if (pUnit->isType(gDatabase.getOTIDHero()) && (pUnit->getPlayerID() == mpOwner->getPlayerID()))
         {
            if (pUnit->getParentSquad())
               mHotdropTarget = pUnit->getParentSquad()->getID();
            return pUnit;
         }
      }
   }
   return NULL;
}

//==============================================================================
//==============================================================================
void BUnitActionHotDrop::transportGarrisonedSquads()
{
   BUnit* pUnit = reinterpret_cast<BUnit*>(mpOwner);
   BASSERT(pUnit);
   BSquad* pSquad = pUnit->getParentSquad();
   BASSERT(pSquad);
   BASSERT(mpProtoAction);

   // Get garrisoned squad to transport
   if (pSquad->getGarrisonedSquads().getSize() == 0)
      return;
   BEntityID squadToTransportID = pSquad->getGarrisonedSquads()[0];
   BSquad* pSquadToTransport = gWorld->getSquad(squadToTransportID);
   if (pSquadToTransport == NULL)
      return;
   static BEntityIDArray squadsToTransport;
   squadsToTransport.resize(0);
   squadsToTransport.add(pSquad->getGarrisonedSquads()[0]);

   int index = findSquad(squadToTransportID);

   // Don't continue if the squad already has an ungarrison order
   const BAction* pAction = pSquadToTransport->getActionByTypeConst(BAction::cActionTypeSquadUngarrison);
   if (pAction != NULL)
      return;

   // Create a new army for the squads that are going to be ungarrisoned
   // TODO:  Is this necessary?
   BObjectCreateParms objectParms;
   objectParms.mPlayerID = pSquadToTransport->getPlayerID();
   BArmy* pArmy = gWorld->createArmy(objectParms);
   BASSERT(pArmy);
   pArmy->addSquads(squadsToTransport, !gConfig.isDefined(cConfigClassicPlatoonGrouping));
   
   BVector transportPos;
   if (mpProtoAction->getUseTeleporter())
   {
      bool bValidExit = false;
      calculateExitLocation(transportPos, pSquadToTransport, bValidExit);
   }
   else
   {
      // VAT: 12/02/08: if we get into this state, we have a squad that is garrisoned, 
      // but that the hotdrop pad doesn't know about - we need to make sure they are setup
      // correctly to be kicked out, otherwise the hotdrop pad backs up forever :( 
      if (index == -1)
         index = addGarrisonedSquad(*pSquadToTransport);

      // and if we STILL have an invalid index, we really are in a scary state :( 
      BASSERT(index != -1);
      if (index == -1)
         return;

      bool bValidExit = false;
      calculateExitLocation(mCurrentSquads[index].mExitLoc, pSquadToTransport, bValidExit);
      mCurrentSquads[index].mValidExit = bValidExit;

      // Get ground position
      gTerrainSimRep.getHeightRaycast(mCurrentSquads[index].mExitLoc, mCurrentSquads[index].mGroundLevel, true);

      // Create hotdrop beam
      BVector pos = mCurrentSquads[index].mExitLoc;
      pos.y = mCurrentSquads[index].mGroundLevel;
      BObjectCreateParms parms;
      parms.mPlayerID = pSquadToTransport->getPlayerID();
      parms.mPosition = pos;
      parms.mType = BEntity::cClassTypeObject;
      parms.mProtoObjectID = mpProtoAction->getProtoObject();

      BObject* pObject = gWorld->createObject(parms);

      if (pObject)
         mCurrentSquads[index].mHotdropBeam = pObject->getID();
      else
         mCurrentSquads[index].mHotdropBeam = cInvalidObjectID;

      transportPos = mCurrentSquads[index].mExitLoc;
   }

   // Unselect squad
   BPlayer* pPlayer = (BPlayer*)pSquadToTransport->getPlayer();
   if (pPlayer)
   {
      BUser* pUser = pPlayer->getUser();
      if (pUser)
      {
         BEntityID squadID = pSquadToTransport->getID();
         BSelectionManager* pSelectionManager = pUser->getSelectionManager();
         if (pSelectionManager && pSelectionManager->isSquadSelected(squadID))
         {
            pSelectionManager->unselectSquad(squadID);
         }
      }
   }

   // Create ungarrison order.
   BSimTarget target(mpOwner->getID());
   BSimOrder* pOrder = gSimOrderManager.createOrder();
   if (pOrder)  
   {
      pOrder->setPriority(BSimOrder::cPrioritySim);
   }

   float dist = mpProtoAction->getWorkRange();
   BVector rally = transportPos;

   BSquad* pLeaderSquad = gWorld->getSquad(mHotdropTarget);
   if (!pLeaderSquad)
   {
      BUnit* pUnit = gWorld->getUnit(mHotdropTarget);

      if (pUnit)
         pLeaderSquad = pUnit->getParentSquad();
   }

   // If hotdropping and leader is already moving, don't set a rally point
   BAction* pMove = NULL;
   
   if (pLeaderSquad)
      pMove = pLeaderSquad->getActionByType(BAction::cActionTypeSquadMove);

   if (pMove && !mpProtoAction->getUseTeleporter())
   {
      rally = cInvalidVector;
   }
   else
   {
      float angle = getRandRangeFloat(cSimRand, 0.0f, 2 * cPi);
      BVector ofs(sin(angle) * dist, 0.0f, cos(angle) * dist);
      rally += ofs;

      BVector suggestedRally;
      static BDynamicSimVectorArray instantiatePositions;
      instantiatePositions.setNumber(0);
      long closestDesiredPositionIndex;

      bool validRally = BSimHelper::findInstantiatePositions(1, instantiatePositions, 0.0f, rally,
                           cXAxisVector, cZAxisVector, pSquad->getObstructionRadius(), suggestedRally, closestDesiredPositionIndex, 4);

      if (validRally && instantiatePositions.size() > 0)
         rally = instantiatePositions[0];
   }

   if (mpProtoAction->getUseTeleporter())
   {
      BSimTarget destTarget;
      BUnit* pUnit = getHotdropTarget();

      if (pUnit && pUnit->getParentSquad())
         destTarget.setID(pUnit->getID());

      pOrder->setTarget(destTarget);
      pSquadToTransport->doUngarrison(pOrder, this, &destTarget, &target, rally, 0, cInvalidVector, transportPos, false, mpProtoAction->getFlagAlertWhenComplete(), true);
   }
   else
   {
      // Looks like the ungarrison action's target is used for notification when the action completes so it should be the
      // triggering unit.  That's kind of confusing.
      pOrder->setTarget(target);

      transportPos.y = pSquadToTransport->getPosition().y;
      mCurrentSquads[index].mUngarrisonID = pSquadToTransport->doUngarrison(pOrder, this, &target, &target, rally, 0, cInvalidVector, transportPos, false, mpProtoAction->getFlagAlertWhenComplete(), true);
      pSquadToTransport->setPosition(transportPos);
   }
}

//==============================================================================
//==============================================================================
void BUnitActionHotDrop::calculateExitLocation(BVector& loc, BSquad* pSquad, bool& rValidExit)
{
   BUnit* pUnit = reinterpret_cast<BUnit*>(mpOwner);
   BASSERT(pUnit);

   // TODO:  If leader unit doesn't exist then just pop squad outside the hot drop pad.
   // This is just temporary bullet proofing.  Eventually garrisoning should not be allowed
   // if there is no leader unit.  If the leader dies after garrisoning starts then the squad should
   // teleport to the last known leader unit position.  This action will probably need to cache the last
   // known position.
   BVector suggestedPos = cInvalidVector;
   loc = cInvalidVector;

   BUnit* pHotdropTarget = getHotdropTarget();

   // If we're recalling the leader to the hot drop pad, the exit location is the hot drop pad
   if (pHotdropTarget && pSquad && (pSquad->getLeaderUnit() == pHotdropTarget))
   {
      loc = pUnit->getPosition();
      rValidExit = true;
      return;
   }

   BVector startPos = cInvalidVector;

   if (pHotdropTarget && pHotdropTarget->getParentSquad())
   {
      float angle = getRandRangeFloat(cSimRand, 0.0f, 2 * cPi);
      BVector ofs(sin(angle), 0.0f, cos(angle));
      startPos = pHotdropTarget->getPosition() + ofs * (pHotdropTarget->getObstructionRadius() + pSquad->getObstructionRadius());
   }

   static BDynamicSimVectorArray instantiatePositions;
   instantiatePositions.setNumber(0);
   long closestDesiredPositionIndex;

   //bool validExit = false;
   rValidExit = false;
   
   if (pHotdropTarget && pSquad)
   {
      rValidExit = BSimHelper::findInstantiatePositions(1, instantiatePositions, pHotdropTarget->getObstructionRadius(), pHotdropTarget->getPosition(),
                     pHotdropTarget->getForward(), pHotdropTarget->getRight(), pSquad->getObstructionRadius(), suggestedPos, closestDesiredPositionIndex, 4);
   }

   if (rValidExit && instantiatePositions.size() > 0)
      suggestedPos = instantiatePositions[0];

   if (pHotdropTarget && rValidExit)
   {
      loc = suggestedPos;
   }
   else
   {
      loc = pUnit->getPosition();
   }
}

//==============================================================================
//==============================================================================
void BUnitActionHotDrop::completeTeleport(BSquad* pSquad)
{
   BASSERT(pSquad);

   int index = findSquad(pSquad->getID());
   BASSERT(index != -1);
   if (index == -1)
      return;

   // If there's a hotdrop beam, remove it
   BEntityID id = mCurrentSquads[index].mHotdropBeam;
   if (id != cInvalidObjectID)
   {
      BObject* pObject = gWorld->getObject(id);
      BASSERT(pObject);
      if (pObject)
         pObject->kill(false);
      else
         mCurrentSquads[index].mHotdropBeam = cInvalidObjectID;
   }

   // Were we able to exit to a valid position or did we pop back out the entrance?
   bool bValidExit = mCurrentSquads[index].mValidExit;

   mCurrentSquads[index].mValid = false; // Flag entry as invalid

   // Unflag units so they can be killed again
   uint count = pSquad->getNumberChildren();
   BUnit* pUnit;
   for (uint i = 0; i < count; ++i)
   {
      pUnit = gWorld->getUnit(pSquad->getChild(i));
      if (pUnit)
      {
         pUnit->setFlagInvulnerable(false);
         pUnit->setFlagAllowStickyCam(true);

         // Re-enable tie to ground and physics
         pUnit->setTieToGround(!pUnit->getProtoObject()->getFlagNoTieToGround());
         pUnit->tieToGround();
         pUnit->setPhysicsKeyFramed(false);

         // Re-enable air collision avoidance if we turned it off
         if (mCurrentSquads[index].mReenableAvoidAir)
         {
            BAction* pAction = pUnit->getActionByType(BAction::cActionTypeUnitAvoidCollisionAir);
            if (pAction)
               pAction->setFlagActive(true);
         }
      }
   }

   // If our exit loc was not valid, we don't want to acquire the leader unit action or do any of this
   if (!bValidExit)
      return;

   BUnit* pLeaderUnit = NULL;

   BSquad* pLeaderSquad = gWorld->getSquad(mHotdropTarget);
   if (!pLeaderSquad)
      pLeaderUnit = gWorld->getUnit(mHotdropTarget);
   else
      pLeaderUnit = pLeaderSquad->getLeaderUnit();

   if (!pLeaderUnit)
      return;

   if (!pLeaderSquad)
      pLeaderSquad = pLeaderUnit->getParentSquad();

   if (pLeaderSquad)
   {
      // Play hot drop chatter when the squad teleports
      BUnit *pCovLeaderUnit = getHotdropTarget();
      if (pCovLeaderUnit)
      {
         BSquad* pCovLeaderSquad = pCovLeaderUnit->getParentSquad();
         if (pCovLeaderSquad)
         {
            // Play it on the squad being hot dropped
            gWorld->createPowerAudioReactions(pSquad->getPlayerID(), cSquadSoundChatterReactHotDrop, pCovLeaderSquad->getPosition(), pSquad->getLOS(), pSquad->getID());
            // Play it on the covenant leader the squad is being hot dropped to
            gWorld->createPowerAudioReactions(pCovLeaderSquad->getPlayerID(), cSquadSoundChatterReactHotDrop, pCovLeaderSquad->getPosition(), pCovLeaderSquad->getLOS(), pCovLeaderSquad->getID());                                    
         }
      }

      BUnit* pOwnerUnit = static_cast<BUnit*>(mpOwner);
      BASSERT(pOwnerUnit);
      //BSquad* pOwnerSquad = pOwnerUnit->getParentSquad();

      // Leader shouldn't follow their own actions
      if (pLeaderSquad->getID() ==  mCurrentSquads[index].mSquadID)
         return;

      // Check for attack action on leader and setup same attack
      BAction* pAttack = pLeaderSquad->getActionByType(BAction::cActionTypeSquadAttack);

      if (pAttack && pAttack->getTarget()->isValid())
      {
         // Add an attack opportunity for ourselves
         pSquad->queueAttack(*pAttack->getTarget(), false);

         // Ensure we don't leash during the attack
         if (pAttack->getTarget()->isPositionValid())
            pSquad->setLeashPosition(pAttack->getTarget()->getPosition());
         else
         {
            BASSERT(pAttack->getTarget()->isIDValid());
            BSquad* pTarget = gWorld->getSquad(pAttack->getTarget()->getID());
            if (pTarget)
               pSquad->setLeashPosition(pTarget->getPosition());
         }
         return;
      }

      // Check for move action on leader and setup same move
      if (pLeaderSquad && pLeaderSquad->getParentPlatoon())
      {
         BPlatoonActionMove* pMove = (BPlatoonActionMove*)pLeaderSquad->getParentPlatoon()->getActionByType(BAction::cActionTypePlatoonMove);

         if (pMove && pMove->getTarget()->isValid())
         {
            // Put the squad in its own army
            BObjectCreateParms objectParms;
            objectParms.mPlayerID = pSquad->getPlayerID();
            BArmy* pArmy = gWorld->createArmy(objectParms);
            if (!pArmy)
               return;
            BEntityIDArray workingSquads;
            workingSquads.add(pSquad->getID());
            if (gConfig.isDefined(cConfigClassicPlatoonGrouping))
               pArmy->addSquads(workingSquads, false);
            else
               pArmy->addSquads(workingSquads, true);

            // Build the move command
            const BDynamicSimVectorArray& wp = pMove->getOrder()->getWaypoints();

            int32 ofs = pLeaderSquad->getParentPlatoon()->getCurrentUserWP_4();
            BSimTarget target;

            BWorkCommand tempCommand;
            if (wp.getNumber() == 1)
               tempCommand.setWaypoints(wp.getPtr(), 1);
            else if (ofs == 0)
               tempCommand.setWaypoints(wp.getPtr(), wp.getNumber());
            else
               tempCommand.setWaypoints(wp.getPtr() + ofs - 1, wp.getNumber() - ofs + 1);
            tempCommand.setRecipientType(BCommand::cArmy);
            tempCommand.setSenderType(BCommand::cGame);
            tempCommand.setFlag(BWorkCommand::cFlagAttackMove, true);
            tempCommand.setFlag(BCommand::cFlagAlternate, false);
            pArmy->queueOrder(&tempCommand);
            return;
         }
      }
   }
}

//==============================================================================
//==============================================================================
int BUnitActionHotDrop::findSquad(BEntityID squadID)
{
   int count = mCurrentSquads.size();
   for (int i = 0; i < count; ++i)
   {
      if (mCurrentSquads[i].mSquadID == squadID)
         return i;
   }

   return -1;
}


//==============================================================================
//==============================================================================
bool BUnitActionHotDrop::save(BStream* pStream, int saveType) const
{
   if (!BAction::save(pStream, saveType))
      return false;

   GFWRITEVAR(pStream, BEntityID, mHotdropTarget);
   GFWRITEVAR(pStream, BEntityID, mHotdropBeam);

   // mCurrentSquads
   int count = mCurrentSquads.size();
   GFWRITEVAR(pStream, int, count);
   GFVERIFYCOUNT(count, 100);
   for (long i = 0; i < count; i++)
   {
      const SquadInfo& squad = mCurrentSquads[i];
      GFWRITEVAR(pStream, BEntityID, squad.mSquadID);
      GFWRITEVECTOR(pStream, squad.mExitLoc);
      GFWRITEVAR(pStream, BEntityID, squad.mHotdropBeam);
      GFWRITEVAR(pStream, float, squad.mGroundLevel);
      GFWRITEVAR(pStream, float, squad.mHeight);
      GFWRITEBITBOOL(pStream, squad.mFlagGarrison);
      GFWRITEBITBOOL(pStream, squad.mGoingUp);
      GFWRITEBITBOOL(pStream, squad.mValid);
      GFWRITEBITBOOL(pStream, squad.mReenableAvoidAir);
      GFWRITEBITBOOL(pStream, squad.mValidExit);

      GFWRITEVAR(pStream, BActionID, squad.mUngarrisonID);
   }

   int suCount = mSquadToUngarrison.size();
   GFWRITEVAR(pStream, int, suCount);
   GFVERIFYCOUNT(suCount, 10);
   for (int i = 0; i < suCount; ++i)
   {
      const SquadUngarrisonPair& su = mSquadToUngarrison[i];

      GFWRITEVAR(pStream, BEntityID, su.first);
      GFWRITEVAR(pStream, BActionID, su.second);
   }

   return true;
}

//==============================================================================
//==============================================================================
bool BUnitActionHotDrop::load(BStream* pStream, int saveType)
{
   if (!BAction::load(pStream, saveType))
      return false;

   GFREADVAR(pStream, BEntityID, mHotdropTarget);
   GFREADVAR(pStream, BEntityID, mHotdropBeam);

   // mCurrentSquads
   int count;
   GFREADVAR(pStream, int, count);
   GFVERIFYCOUNT(count, 100);
   for (int i=0; i<count; i++)
   {
      BEntityID entityID;
      SquadInfo info;
      GFREADVAR(pStream, BEntityID, info.mSquadID);
      GFREADVECTOR(pStream, info.mExitLoc);
      GFREADVAR(pStream, BEntityID, info.mHotdropBeam);
      GFREADVAR(pStream, float, info.mGroundLevel);
      GFREADVAR(pStream, float, info.mHeight);
      GFREADBITBOOL(pStream, info.mFlagGarrison);
      GFREADBITBOOL(pStream, info.mGoingUp);
      GFREADBITBOOL(pStream, info.mValid);
      GFREADBITBOOL(pStream, info.mReenableAvoidAir);

      if (BAction::mGameFileVersion >= 47)
      {
         GFREADBITBOOL(pStream, info.mValidExit);
      }

      if( BAction::mGameFileVersion >= 46)
      {
         GFREADVAR(pStream, BActionID, info.mUngarrisonID);
      }

      mCurrentSquads.add(info);
   }

   
   if (BAction::mGameFileVersion >= 52)
   {
      int suCount;
      GFREADVAR(pStream, int, suCount);
      GFVERIFYCOUNT(suCount, 10);
      for (int i = 0; i < suCount; ++i)
      {
         SquadUngarrisonPair su;

         GFREADVAR(pStream, BEntityID, su.first);
         GFREADVAR(pStream, BActionID, su.second);

         mSquadToUngarrison.add(su);
      }
   }

   return true;
}

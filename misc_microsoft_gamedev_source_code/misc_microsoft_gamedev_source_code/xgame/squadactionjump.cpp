//==============================================================================
// squadactionjump.cpp
// Copyright (c) 2008 Ensemble Studios
//==============================================================================

// Includes
#include "common.h"
#include "squad.h"
#include "squadactionJump.h"
#include "squadactionmove.h"
#include "world.h"
#include "actionmanager.h"
#include "database.h"
#include "config.h"
#include "configsgame.h"
#include "tactic.h"
#include "visual.h"
#include "protosquad.h"
#include "formation2.h"
#include "pather.h"
#include "ability.h"
#include "simordermanager.h"
#include "simhelper.h"
#include "wwise_ids.h"
#include "worldsoundmanager.h"

//==============================================================================
//==============================================================================
IMPLEMENT_FREELIST(BSquadActionJump, 4, &gSimHeap);

//==============================================================================
//==============================================================================
bool BSquadActionJump::connect(BEntity* pEntity, BSimOrder* pOrder)
{
   // Validate jump target position
   BASSERT(pEntity);
   BSquad* pSquad = pEntity->getSquad();
   BASSERT(pSquad);
   
   if (!mTarget.isPositionValid())
   {
      BASSERT(mTarget.isIDValid());
      if (!mTarget.isIDValid())
         return false;

      BEntity* pEntity = gWorld->getEntity(mTarget.getID());
      BASSERT(pEntity);
      if (!pEntity)
         return false;

      mTarget.setPosition(pEntity->getPosition());
   }

   BVector startPos = pSquad->getAveragePosition();

   BPlatoon* pPlatoon = pSquad->getParentPlatoon();

   BVector ofs(0.0f);
   if (pPlatoon)
   {
      const BFormation2* pFormation = pPlatoon->getFormation();
      BASSERT(pFormation);
      const BFormationPosition2* pFormationPosition = pFormation->getFormationPosition(pSquad->getID());
      if (pFormationPosition)
         ofs = pFormationPosition->getOffset();

      startPos = pPlatoon->getPosition();
   }

   // Validate range
   // This action uses the duration field to store the max distance it can jump.  It does this because it was an available float field
   // that wasn't being used for other purposes.  It used to use work range, but that caused problems with real range checking.
   if (mpProtoAction && startPos.distanceSqr(mTarget.getPosition()) > mpProtoAction->getDuration() * mpProtoAction->getDuration())
   {
      setState(cStateFailed);
      return true;
      //return false;
   }

   float obstructionRadius = 0.0f;

   // If we're jumping to attack something, land a bit away from it
   if (mpProtoAction && mJumpType == BSimOrder::cJumpOrderJumpAttack)
   {
      BVector targetPos = mTarget.getPosition();

      BUnit* pUnit = pSquad->getLeaderUnit();
      BASSERT(pUnit);

      if (pUnit)
      {
         BTactic* pTactic = pUnit->getTactic();
         if (pTactic)
         {
            // Do weapon group range calculation if necessary
            const BWeapon* pWeapon = (BWeapon*)pTactic->getWeapon(mpProtoAction->getWeaponID());
            if (pWeapon)
            {
               BVector dir = mTarget.getPosition() - pSquad->getAveragePosition();
               dir.normalize();
               dir *= pWeapon->mMaxRange / 2.0f;
               targetPos -= dir;
            }
         }
      }

      mTarget.setPosition(targetPos);
   }

   if (mTarget.isIDValid())
   {
      BEntity* pTargetEntity = gWorld->getEntity(mTarget.getID());

      if (pTargetEntity)
      {
         obstructionRadius = pTargetEntity->getObstructionRadius();
      }
   }

   // Make target position reflect platoon position so that we don't all bunch up
   mTarget.setPosition(mTarget.getPosition() + ofs);

   // Adjust platoon position to a pathable spot
   BVector adjPosition(mTarget.getPosition());
   static BDynamicSimVectorArray instantiatePositions;
   instantiatePositions.setNumber(0);
   long closestDesiredPositionIndex = -1;

   bool validPos = BSimHelper::findInstantiatePositions(1, instantiatePositions, obstructionRadius, mTarget.getPosition(),
                        cXAxisVector, cZAxisVector, pSquad->getObstructionRadius(), adjPosition, closestDesiredPositionIndex, 4, true);

   if (!validPos)
      return false;

   if (instantiatePositions.size() > 0)
   {
      if ((0 <= closestDesiredPositionIndex) && (closestDesiredPositionIndex < instantiatePositions.getNumber()))
         adjPosition = instantiatePositions[closestDesiredPositionIndex];
      else
         adjPosition = instantiatePositions[0];
   }

   mTarget.setPosition(adjPosition);

   // Make sure jump target is in the world bounds
   if (gWorld->isOutsidePlayableBounds(mTarget.getPosition(), true))
   {
      BVector pos = mTarget.getPosition();

      if (pos.x < gWorld->getSimBoundsMinX()) pos.x = gWorld->getSimBoundsMinX() + 1.0f;
	   if (pos.z < gWorld->getSimBoundsMinZ()) pos.z = gWorld->getSimBoundsMinZ() + 1.0f;
	   if (pos.x > gWorld->getSimBoundsMaxX()) pos.x = gWorld->getSimBoundsMaxX() - 1.0f;
	   if (pos.z > gWorld->getSimBoundsMaxZ()) pos.z = gWorld->getSimBoundsMaxZ() - 1.0f;

      mTarget.setPosition(pos);

      //trace("Offset target is (%f %f %f)", mTarget.getPosition().x, mTarget.getPosition().y, mTarget.getPosition().z);

      // Readjust our final position
      adjPosition = mTarget.getPosition();
      instantiatePositions.setNumber(0);
      validPos = BSimHelper::findInstantiatePositions(1, instantiatePositions, 0.0f, mTarget.getPosition(),
                           cXAxisVector, cZAxisVector, pSquad->getObstructionRadius(), adjPosition, closestDesiredPositionIndex, 4, true);

      if (!validPos)
         return false;

      if (instantiatePositions.size() > 0)
      {
         if ((0 <= closestDesiredPositionIndex) && (closestDesiredPositionIndex < instantiatePositions.getNumber()))
            adjPosition = instantiatePositions[closestDesiredPositionIndex];
         else
            adjPosition = instantiatePositions[0];
      }

      mTarget.setPosition(adjPosition);
   }

   // Get correct ground height at final position
   BVector target = mTarget.getPosition();
   if (gTerrainSimRep.getHeight(target, true))
   {
      mTarget.setPosition(target);
   }

   // Stop any move actions we might be doing
   if (pSquad)
      pSquad->removeAllActionsOfType(BAction::cActionTypeSquadMove);

   // TRB 12/4/08 - Don't kill the platoon's move.  That will prevent it from moving other squads in the platoon that
   // aren't jumping.
   //if (pPlatoon)
   //   pPlatoon->removeAllActionsOfType(BAction::cActionTypePlatoonMove);

   // If our final position is really close to our current position, don't bother unless it's a pull
   if (mJumpType != BSimOrder::cJumpOrderJumpPull && mTarget.getPosition().distance(pSquad->getPosition()) < 10.0f)
      return false;

   if (!BAction::connect(pEntity, pOrder))
   {
      return (false);
   }

   // TRB 12/4/08 - Tell the platoon not to create a move action for this squad.  This action will move it to the destination.
   if (pPlatoon)
      pPlatoon->notifyThatSquadWillHandleItsOwnMovement_4(pSquad->getID());

   return (true);
}

//==============================================================================
//==============================================================================
void BSquadActionJump::disconnect()
{
   if (mOppID != cInvalidActionID)
   {
      BASSERT(mpOwner);
      BSquad* pSquad = mpOwner->getSquad();
      BASSERT(pSquad);

      pSquad->removeOppFromChildren(mOppID);

      pSquad->setFlagJumping(false);
   }

   PlayJumpSound(false);

   BAction::disconnect();
}

//==============================================================================
//==============================================================================
bool BSquadActionJump::init()
{
   if (!BAction::init())
   {
      return (false);
   }

   mJumping.clear();
   mTarget.reset();
   mOppID = cInvalidActionID;
   mJumpType = BSimOrder::cJumpOrderJump;
   mThrowerProtoActionID = cInvalidActionID;

   return (true);
}

//==============================================================================
//==============================================================================
bool BSquadActionJump::setState(BActionState state)
{
   if (state == cStateDone)
   {
      BASSERT(mpOwner);
      BSquad* pSquad = mpOwner->getSquad();
      BASSERT(pSquad);

      BASSERT(mTarget.isPositionValid());

      PlayJumpSound(false);

      // [6/3/08 CJS] Pulls are not volunatry, so don't reset our ability recover time :)
      if (mJumpType != BSimOrder::cJumpOrderJumpPull)
      {
         BAbilityID abilityID = gDatabase.getSquadAbilityID(pSquad, gDatabase.getAIDCommand());
         BAbility * pAbility = gDatabase.getAbilityFromID(abilityID);
         BASSERT(pAbility);

         pSquad->setRecover(cRecoverAbility, pSquad->getPlayer()->getAbilityRecoverTime(abilityID), abilityID);
      }

      pSquad->tieToGround();

      // Fix up the squad physics data
      pSquad->setTurnRadiusPos(pSquad->getPosition());
      pSquad->setTurnRadiusFwd(pSquad->getForward());
      pSquad->updateObstruction();
      pSquad->dirtyLOS();
      // TRB 12/1/08 - Don't set teleported flag for the squad because there is nothing that will set it back to false.
      // Protectors check this flag to see whether they should teleport to their target's position, but that's not necessary
      // in this case since the distance jumped should be short enough for protectors to move normally.
      //pSquad->setFlagTeleported(true);

      // If a unit is not leashed, give them a leash opp
      /*uint count = pSquad->getNumberChildren();
      for (uint i = 0; i < count; ++i)
      {
         BUnit* pUnit = gWorld->getUnit(pSquad->getChild(i));
         if (!pUnit)
            continue;

         if (!pSquad->isChildLeashed(pSquad->getChild(i)))
         {
            BUnitOpp* pNewOpp=BUnitOpp::getInstance();
            pNewOpp->init();
            pNewOpp->setSource(pSquad->getChild(i));
            pNewOpp->setTarget(pSquad->getLeashPosition());
            pNewOpp->setType(BUnitOpp::cTypeMove);
            pNewOpp->setLeash(true);
            pNewOpp->setForceLeash(true);
            pNewOpp->setNotifySource(false);
            pNewOpp->generateID();
            pNewOpp->setAllowComplete(true);
            pUnit->addOpp(pNewOpp);
         }
      }*/

      switch (mJumpType)
      {
         // If we are a jump gather move, gather from the target
         case BSimOrder::cJumpOrderJumpGather:
            {
               BSimOrder* pOrder=gSimOrderManager.createOrder();
               BASSERT(pOrder);
               if (!pOrder)
                  break;

               BSimTarget target = mTarget;
               BASSERT(mTarget.isPositionValid());
               BDynamicVectorArray waypoints;            
               waypoints.add(target.getPosition());

               if (target.isIDValid())
               {
                  const BSquad* pTargetSquad = gWorld->getSquad(target.getID());
                  if (pTargetSquad)
                     target.setID(pTargetSquad->getID());
               }

               target.setAbilityID(-1);
               pOrder->setTarget(target);
               pOrder->setOwnerID(pSquad->getID());
               if (mpOrder)
                  pOrder->setPriority(mpOrder->getPriority());
               else
                  pOrder->setPriority(BSimOrder::cPrioritySim);
               pOrder->setWaypoints(waypoints);

               pSquad->queueOrder(pOrder, BSimOrder::cTypeGather);
            }
            break;

         // Garrison in the target
         case BSimOrder::cJumpOrderJumpGarrison:
            {
               BSimOrder* pOrder=gSimOrderManager.createOrder();
               BASSERT(pOrder);
               if (!pOrder)
                  break;

               BSimTarget target = mTarget;
               BASSERT(mTarget.isPositionValid());
               BDynamicVectorArray waypoints;            
               waypoints.add(target.getPosition());

               if (target.isIDValid())
               {
                  BSquad* pTargetSquad = gWorld->getSquad(target.getID());
                  if (pTargetSquad && pTargetSquad->getLeaderUnit())
                     target.setID(pTargetSquad->getLeaderUnit()->getID());
               }

               target.setAbilityID(-1);
               pOrder->setTarget(target);
               pOrder->setOwnerID(pSquad->getID());
               if (mpOrder)
                  pOrder->setPriority(mpOrder->getPriority());
               else
                  pOrder->setPriority(BSimOrder::cPrioritySim);
               pOrder->setWaypoints(waypoints);

               pSquad->queueOrder(pOrder, BSimOrder::cTypeGarrison);
            }
            break;

         case BSimOrder::cJumpOrderJumpAttack:
            {
               // This is the wrong way to do this.  This was creating an orphaned opportunity, which
               // conflicted with future commands given to it.  For example, in a move order was given
               // the units would refuse to move with their squad until the unit attack action decided to end.
               /*
               BUnitOpp opp;
               opp.init();

               opp.setTarget(mTarget);
               opp.setType(BUnitOpp::cTypeAttack);
               opp.setSource(pSquad->getID());
               opp.setPriority(BUnitOpp::cPriorityCommand);
               opp.generateID();

               pSquad->addOppToChildren(opp);
               */
            }
            break;

         case BSimOrder::cJumpOrderJumpPull:
         case BSimOrder::cJumpOrderJump:
            // Do nothing
            break;

         default:
            BASSERT(0);
            break;
      }
   }

   return (BAction::setState(state));
}

//==============================================================================
//==============================================================================
bool BSquadActionJump::update(float elapsed)
{
   BASSERT(mpOwner);
   BSquad* pSquad = mpOwner->getSquad();
   BASSERT(pSquad);

   switch (mState)
   {
      case cStateNone:
         {
            // Queue jump action in units
            BUnitOpp opp;
            opp.init();

            pSquad->setFlagJumping(true);

            PlayJumpSound(true);

            switch (mJumpType)
            {
               case BSimOrder::cJumpOrderJump:
                  opp.setType(BUnitOpp::cTypeJump);
                  break;

               case BSimOrder::cJumpOrderJumpGather:
                  opp.setType(BUnitOpp::cTypeJumpGather);
                  break;

               case BSimOrder::cJumpOrderJumpGarrison:
                  opp.setType(BUnitOpp::cTypeJumpGarrison);
                  break;

               case BSimOrder::cJumpOrderJumpAttack:
                  opp.setType(BUnitOpp::cTypeJumpAttack);
                  break;

               case BSimOrder::cJumpOrderJumpPull:
                  BASSERT(mThrowerProtoActionID != cInvalidActionID); // If we are a pull, this should always be valid
                  opp.setUserData((uint16)mThrowerProtoActionID);
                  opp.setType(BUnitOpp::cTypeJumpPull);
                  break;

               default:
                  BASSERT(0);
            }
            opp.setTarget(mTarget);
            opp.setSource(pSquad->getID());
            opp.setPriority(BUnitOpp::cPriorityCritical);
            mOppID = opp.generateID();

            if (!pSquad->addOppToChildren(opp))
            {
               //trace("Failopp %d", mOppID);
               setState(cStateFailed);
               return true;
            }

            setState(cStateWorking);

            int count = pSquad->getNumberChildren();
            for (int i = 0; i < count; ++i)
            {
               BUnit* pUnit = gWorld->getUnit(pSquad->getChild(i));

               if (!pUnit)
                  continue;

               pUnit->doJump(&opp, this);

               // Add unit to list of units jumping
               mJumping.add(pUnit->getID());
            }

            pSquad->setPosition(mTarget.getPosition());

            // TRB 12/4/08 - Don't set the platoon's position.  Let it do that itself.
            //BPlatoon* pPlatoon = pSquad->getParentPlatoon();
            //if (pPlatoon)
            //   pPlatoon->setPosition(mTarget.getPosition());
         }
         break;

      case cStateWorking:
         BUnit *pLeaderUnit = pSquad->getLeaderUnit();
         if (!pLeaderUnit || !pLeaderUnit->getOppByID(mOppID))
         {
            // Opp isn't there, bail
            setState(cStateDone);
         }
         else
         {
            pSquad->dirtyLOS();
            pSquad->setLeashPosition(pSquad->getAveragePosition());
            pSquad->setPosition(pSquad->getAveragePosition());

            // Fix up the squad physics data
            if (pSquad->isSquadAPhysicsVehicle())
            {
               pSquad->setTurnRadiusPos(pSquad->getPosition());
               pSquad->setTurnRadiusFwd(pSquad->getForward());
            }
         }

         // See if any unit still jumping no longer have a jump action.  If so, remove them from the list
         uint count = mJumping.size();
         for (uint i = 0; i < count; ++i)
         {
            BUnit* pUnit = gWorld->getUnit(mJumping[i]);
            if (pUnit)
            {
               if (!pUnit->getActionByType(BAction::cActionTypeUnitJump))
               {
                  mJumping.removeIndex(i);
                  i = 0;
                  --count;
               }
            }
         }

         if (mJumping.isEmpty())
            setState(cStateDone);
         break;
   }

   //Done.
   return (true);
}

//==============================================================================
//==============================================================================
void BSquadActionJump::notify(DWORD eventType, BEntityID senderID, DWORD data1, DWORD data2)
{
   if ((eventType == BEntity::cEventActionDone || eventType == BEntity::cEventActionFailed) && mState == cStateWorking && data1 == mOppID)
   {
      BASSERT(mpOwner);
      BSquad* pSquad = mpOwner->getSquad();
      BASSERT(pSquad);

      mJumping.remove(senderID);
      if (mJumping.isEmpty())
         setState(cStateDone);
   }
}

//==============================================================================
//==============================================================================
bool BSquadActionJump::isInterruptible() const
{
   return false;
}

//==============================================================================
//==============================================================================
bool BSquadActionJump::save(BStream* pStream, int saveType) const
{
   if (!BAction::save(pStream, saveType))
      return false;
   GFWRITECLASS(pStream, saveType, mTarget);
   GFWRITEVAR(pStream, BUnitOppID, mOppID);
   GFWRITEVAR(pStream, BJumpOrderType, mJumpType);
   GFWRITEVAR(pStream, long, mThrowerProtoActionID);
   GFWRITEARRAY(pStream, BEntityID, mJumping, uint8, 20);  
   return true;
}

//==============================================================================
//==============================================================================
bool BSquadActionJump::load(BStream* pStream, int saveType)
{
   if (!BAction::load(pStream, saveType))
      return false;
   GFREADCLASS(pStream, saveType, mTarget);
   GFREADVAR(pStream, BUnitOppID, mOppID);
   GFREADVAR(pStream, BJumpOrderType, mJumpType);
   GFREADVAR(pStream, long, mThrowerProtoActionID);

   if (BAction::mGameFileVersion >= 39)
   {
      GFREADARRAY(pStream, BEntityID, mJumping, uint8, 20);  
   }
   return true;
}


//==============================================================================
//==============================================================================
bool BSquadActionJump::PlayJumpSound(bool startJump)
{
   BASSERT(mpOwner);
   BSquad* pSquad = mpOwner->getSquad();
   BASSERT(pSquad);

   //-- If there is a squad jump sound then we should play that one now.
   BCueIndex cueIndex = cInvalidCueIndex;
   const BProtoSquad* pProtoSquad = pSquad->getProtoSquad();
   if(pProtoSquad)
   {
      if(startJump)
         cueIndex = pProtoSquad->getSound(cSquadSoundStartJump);
      else
         cueIndex = pProtoSquad->getSound(cSquadSoundStopJump);
   }
   if(cueIndex != cInvalidCueIndex)
   {
      BRTPCInitArray rtpc;
      rtpc.add( BRTPCInit(AK::GAME_PARAMETERS::FOW_DISTANCE, 0.0f) );
      gWorld->getWorldSoundManager()->addSound(pSquad, -1, cueIndex, startJump, cInvalidCueIndex, startJump, startJump, &rtpc);   
      return true;
   }

   return false;
}

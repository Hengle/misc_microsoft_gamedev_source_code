//==============================================================================
// unitactiontowerwall.cpp
// Copyright (c) 2006-2007 Ensemble Studios
//==============================================================================

#include "common.h"
#include "squad.h"
#include "unitactiontowerwall.h"
#include "unit.h"
#include "world.h"
#include "database.h"
#include "tactic.h"
#include "unitquery.h"
#include "commands.h"
#include "configsgame.h"

static const double cMoveOffWaitTime = 3.0f;
static const float cDistanceMoveScalar = 1.0f;

//==============================================================================
//==============================================================================
IMPLEMENT_FREELIST(BUnitActionTowerWall, 4, &gSimHeap);

//==============================================================================
//==============================================================================
bool BUnitActionTowerWall::init(void)
{
   if (!BAction::init())
      return(false);

   mTarget.reset();
   mGateOpen = true;
   mObstructionUnitID = cInvalidObjectID;
   mBeamProjectileID = cInvalidObjectID;
   mWaitTimer = 0;
   mBeamStartPos = cOriginVector;
   mBeamEndPos = cOriginVector;

   return (true);
}

//==============================================================================
//==============================================================================
bool BUnitActionTowerWall::connect(BEntity* pOwner, BSimOrder* pOrder)
{
   if (!BAction::connect(pOwner, pOrder))
      return(false);

   setFlagFromTactic(false);

   return(true);
}

//==============================================================================
//==============================================================================
void BUnitActionTowerWall::disconnect()
{
   BAction::disconnect();
}


//==============================================================================
//==============================================================================
bool BUnitActionTowerWall::setState(BActionState state)
{
   switch(state)
   {
      case cStateWait:
      {       
         break;
      }      
      case cStateNone:
      {
         //-- Reset the timers
         mWaitTimer = 0;
         break;
      }
      case cStateWorking:
      {
         break;
      }
   }

   return BAction::setState(state);
}

//==============================================================================
//==============================================================================
bool BUnitActionTowerWall::update(float elapsed)
{
   BASSERT(mpOwner);
   if (!mpOwner)
      return (true);
   BASSERT(mpOwner->isClassType(BEntity::cClassTypeUnit) == true);
   if (!mpProtoAction)
      return (true);

   BUnit* pUnit = mpOwner->getUnit();
   BASSERT(pUnit);
   if (!pUnit)
      return (true);

   // just for sanity's sake, always update the wall, since we do nothing
   // in the case that we're in the right state already 
   updateGate();

   switch (mState)
   {
      case cStateWait:
      {      
         if(mWaitTimer > 0)
            mWaitTimer -= elapsed;

         if(mWaitTimer <= 0)
         {
            mWaitTimer = 0;
            if(mGateOpen == false)
            {
               createBeam();               
               BUnit* pObsUnit = gWorld->getUnit(mObstructionUnitID);
               if(pObsUnit)
               {
                  pObsUnit->setFlagCollidable(true);
                  pObsUnit->updateObstruction();
               }
            }

            setState(cStateWorking);
         }
         break;
         case cStateWorking:
         {
            updateBeam(elapsed);
         }
         break;
      }
   }

   if(!BAction::update(elapsed))
      return (false);

   return (true);
}

//==============================================================================
//==============================================================================
void BUnitActionTowerWall::setTarget(BSimTarget target)
{
   BASSERT(!target.getID().isValid() || (target.getID().getType() == BEntity::cClassTypeSquad));
   mTarget = target;

   BUnit* pUnit = mpOwner->getUnit();
   BASSERT(pUnit);
   BSquad* pTargetSquad = gWorld->getSquad(mTarget.getID());   
   if(!pTargetSquad)
      return;
   BUnit* pTargetUnit = pTargetSquad->getLeaderUnit();
   if(!pTargetUnit)
      return;
      
   mBeamStartPos = pUnit->getVisualCenter();
   mBeamEndPos = pTargetUnit->getVisualCenter();
   BVector sourceToDest = mBeamEndPos - mBeamStartPos;
   BVector destToSource = mBeamStartPos - mBeamEndPos;

   //-- Rotate the objects to face each other
   BVector forward = destToSource;
   forward.y = 0;
   forward.normalize();
   pUnit->setForward(forward);
   pUnit->setUp(cYAxisVector);
   pUnit->calcRight();

   forward = sourceToDest;
   forward.y = 0;
   forward.normalize();
   pTargetUnit->setForward(forward);
   pTargetUnit->setUp(cYAxisVector);
   pTargetUnit->calcRight();

   //-- Set the beam start and end positions
   sourceToDest.normalize();
   if(pUnit->getVisualBoundingBox())
   {
      const float* extents = pUnit->getVisualBoundingBox()->getExtents();
      float radius = max(extents[0], extents[2]);
      sourceToDest.scale(radius);
      mBeamStartPos += sourceToDest;
   }

   destToSource.normalize();
   if(pTargetUnit->getVisualBoundingBox())
   {
      const float* extents = pTargetUnit->getVisualBoundingBox()->getExtents();
      float radius = max(extents[0], extents[2]);
      destToSource.scale(radius);
      mBeamEndPos += destToSource;
   }
}

//==============================================================================
//==============================================================================
void BUnitActionTowerWall::notify(DWORD eventType, BEntityID senderID, DWORD data, DWORD data2)
{
   BASSERT(mpOwner);
   if (!mpOwner)
      return;
   BASSERT(mpOwner->isClassType(BEntity::cClassTypeUnit) == true);
   if (!mpProtoAction)
      return;

   BUnit* pUnit = mpOwner->getUnit();
   BASSERT(pUnit);

   switch(eventType)
   {
      case BEntity::cEventGarrisonEnd:
      case BEntity::cEventUngarrisonEnd:          
      case BEntity::cEventSquadModeChanaged:
      case BEntity::cEventSquadModeChanging:      
      case BEntity::cEventGarrisonFail:
      case BEntity::cEventUngarrisonFail:            
      {                  
         updateGate();
      }
      break;
   }
}

//==============================================================================
//==============================================================================
void BUnitActionTowerWall::updateGate()
{
   BUnit* pUnit = mpOwner->getUnit();
   BASSERT(pUnit);

   BEntityIDArray containedUnits = pUnit->getGarrisonedUnits();
   if(containedUnits.getSize() > 0)
   {
      closeGate();
   }
   else
   {
      openGate();
   }         
}

//==============================================================================
//==============================================================================
void BUnitActionTowerWall::closeGate()
{
   if(mGateOpen == false)
      return;

   BUnit* pUnit = mpOwner->getUnit();
   BASSERT(pUnit);

   BSquad* pTargetSquad = gWorld->getSquad(mTarget.getID());   
   if(!pTargetSquad)
      return;
   BUnit* pTargetUnit = pTargetSquad->getLeaderUnit();
   if(!pTargetUnit)
      return;

   //-- Make the obstruction    
   createObstruction(pUnit, pTargetUnit);

   //-- Pushoff units
   pushOffUnits();

   mGateOpen = false;
}

//==============================================================================
//==============================================================================
void BUnitActionTowerWall::openGate()
{
   if(mGateOpen == true)
      return;

   //-- Remove the obstruction
   BUnit* pObstruction = gWorld->getUnit(mObstructionUnitID);
   if(pObstruction)
      pObstruction->kill(true);

   BProjectile* pProjectile = gWorld->getProjectile(mBeamProjectileID);
   if(pProjectile)
   {
      pProjectile->kill(true);
      mBeamProjectileID = cInvalidObjectID;
   }

   setState(cStateNone);

   mGateOpen = true;
}

//==============================================================================
//==============================================================================
void BUnitActionTowerWall::pushOffUnits()
{
   if(mWaitTimer > 0)
      return; 

   BUnit* pObsUnit = gWorld->getUnit(mObstructionUnitID);
   if(pObsUnit)
   {  
      BOPQuadHull hull;
      BSquad* pObsSquad = pObsUnit->getParentSquad();
      if (pObsSquad)
      {
         pObsSquad->getObstructionHull(hull);

         // calculate what direction right is 
         BVector beamForward = mBeamEndPos - mBeamStartPos;
         float beamDistance = beamForward.length();
         float distanceToMoveAway = beamDistance * cDistanceMoveScalar;
         beamForward.normalize();      

         BVector beamRight;
         beamRight.assignCrossProduct(cYAxisVector, beamForward);
         beamRight.normalize();

         // manually move units to the right or left of the wall that's going to come up
         // modeled after moveSquadsFromObstruction in BWorld
         BEntityIDArray results(0, 50);
         BUnitQuery query1(pObsSquad->getPosition(), beamDistance, true);
         query1.setFlagIgnoreDead(true);

         int numTargets = gWorld->getSquadsInArea(&query1, &results, false);
         if (numTargets != 0)
         {
            BEntityIDArray moveLeftSquads(0, numTargets);
            BEntityIDArray moveRightSquads(0, numTargets);
            BSmallDynamicSimArray<BPlayerID> movePlayers(0, 1);

            for (int i = 0; i < numTargets; i++)
            {
               BEntityID squadID = results[i];

               // Skip over the obstruction's squad
               if (squadID == pObsSquad->getID())
                  continue;      

               const BSquad* pSquad = gWorld->getSquad(squadID);
               if (pSquad)
               {
                  const BProtoObject* pProtoObject = pSquad->getProtoObject();
                  if (!pProtoObject)
                     continue;

                  // Ignore permanently non-mobile units such as a socket 
                  if (pProtoObject->getFlagNonMobile())
                     continue;

                  // Ignore spartans inside a vehicle.  The vehicle itself should be in this list too and movement
                  // will be controlled through it.
                  if (pSquad->getFlagContainedSpartan())
                     continue;

                  // Temporary non-mobile units such as a locked down Cobra won't be able to be told to move
                  if (!pSquad->canMove(false))
                     continue;

                  // squads that are in a modal power won't be able to be told to move
                  if (pSquad->getSquadMode() == BSquadAI::cModePower)
                     continue;

                  // Hitched units or tow trucks won't move off obstructions
                  if (pSquad->getFlagHitched() || (const_cast<BSquad*>(pSquad))->hasHitchedSquad())
                     continue;

                  // we don't care about air units
                  if (pProtoObject->getMovementType() == cMovementTypeAir)
                     continue;

                  BVector dirFromObs = pSquad->getPosition() - pObsSquad->getPosition();
                  dirFromObs.normalize();
                  if (dirFromObs.dot(beamRight) > 0.0f)
                     moveRightSquads.add(squadID);
                  else
                     moveLeftSquads.add(squadID);

                  movePlayers.uniqueAdd(pSquad->getPlayerID());
               }
            }

            if (moveRightSquads.getNumber() > 0)
            {
               BVector targetPosition = pObsSquad->getPosition() + (beamRight * distanceToMoveAway);
               moveSquadsToPosition(movePlayers, moveRightSquads, targetPosition);
            }
            if (moveLeftSquads.getNumber() > 0)
            {
               BVector targetPosition = pObsSquad->getPosition() - (beamRight * distanceToMoveAway);
               moveSquadsToPosition(movePlayers, moveLeftSquads, targetPosition);
            }
         }
      }

      mWaitTimer = cMoveOffWaitTime;
      setState(cStateWait);
   }
}

//==============================================================================
//==============================================================================
void BUnitActionTowerWall::moveSquadsToPosition(const BSmallDynamicSimArray<BPlayerID>& movePlayers, const BEntityIDArray& squads, const BVector& position)
{
   BSimTarget targetPos(position);
   BWorkCommand tempCommand;
   tempCommand.setWaypoints(&position, 1);
   tempCommand.setRecipientType(BCommand::cArmy);
   tempCommand.setSenderType(BCommand::cGame);

   // Send a separate command per player.
   for (int i = 0; i < movePlayers.getNumber(); i++)
   {
      BPlayerID playerID = movePlayers[i];
      BEntityIDArray playerSquads;
      for (int j = 0; j < squads.getNumber(); j++)
      {
         BEntityID squadID = squads[j];
         BSquad* pSquad = gWorld->getSquad(squadID);
         if (pSquad && pSquad->getPlayerID() == playerID)
            playerSquads.add(squadID);
      }
      if (playerSquads.getNumber() > 0)
      {
         BObjectCreateParms objectParms;
         objectParms.mPlayerID = movePlayers[i];
         BArmy* pArmy = gWorld->createArmy(objectParms);
         if (pArmy)
         {
            pArmy->addSquads(playerSquads, !gConfig.isDefined(cConfigClassicPlatoonGrouping));
            pArmy->queueOrder(&tempCommand);
         }
      }               
   }

}

//==============================================================================
//==============================================================================
void BUnitActionTowerWall::createObstruction(BUnit* pSource, BUnit*pDest)
{
   BObjectCreateParms parms;
   parms.mProtoObjectID = gDatabase.getPOIDObstruction();
   parms.mPlayerID = 0;

   BVector sourceToDest = pDest->getPosition() - pSource->getPosition();
   parms.mPosition = sourceToDest;
   parms.mPosition.scale(0.5);
   parms.mPosition = pSource->getPosition() + parms.mPosition;

   BVector forward = sourceToDest;
   forward.y = 0;
   forward.normalize();
   forward.rotateXZ(cPiOver2);
   parms.mForward = forward;
   parms.mRight.assignCrossProduct(cYAxisVector, parms.mForward);
   parms.mRight.normalize();   

   mObstructionUnitID = gWorld->createEntity(gDatabase.getPOIDObstruction(), false, 0, parms.mPosition, parms.mForward, parms.mRight, true);
   BSquad* pSquad = gWorld->getSquad(mObstructionUnitID);
   BASSERT(pSquad);
   BUnit* pObsUnit = pSquad ? pSquad->getLeaderUnit() : NULL;
   mObstructionUnitID = pObsUnit ? pObsUnit->getID() : cInvalidObjectID;
   if (pObsUnit)
   {
      pObsUnit->setObstructionRadiusX(sourceToDest.length() / 2);
      pObsUnit->setObstructionRadiusY(pSource->getObstructionRadiusY());
      pObsUnit->setObstructionRadiusZ(pSource->getObstructionRadiusZ() / 2);
      pObsUnit->setFlagCollidable(false);
      pObsUnit->updateObstruction();
   }
}

//==============================================================================
//==============================================================================
void BUnitActionTowerWall::createBeam()
{ 

   BUnit* pUnit = mpOwner->getUnit();
   BASSERT(pUnit);
   BSquad* pTargetSquad = gWorld->getSquad(mTarget.getID());   
   if(!pTargetSquad)
      return;
   BUnit* pTargetUnit = pTargetSquad->getLeaderUnit();
   if(!pTargetUnit)
      return;

   BASSERT(mpProtoAction);
   if (mBeamProjectileID == cInvalidObjectID)
   {      
      // Get launch position
      BVector launchPosition = mBeamStartPos;
      
      // Get beam direction
      BVector beamForward = mBeamEndPos - launchPosition;
      beamForward.normalize();      
      
      BVector beamRight;
      beamRight.assignCrossProduct(cYAxisVector, beamForward);;
      beamRight.normalize();

      BVector beamUp;
      beamUp.assignCrossProduct(beamForward, beamRight);
      beamUp.normalize();

      // Create beam projectile
      BObjectCreateParms parms;
      parms.mPlayerID = 0;
      parms.mProtoObjectID = mpProtoAction->getProjectileID();
      parms.mPosition = mBeamStartPos;
      parms.mForward = mBeamEndPos;
      parms.mRight = beamRight;
      parms.mStartBuilt = true;
      parms.mNoTieToGround = true;

      BProjectile* pProjectile = gWorld->createProjectile(parms);
      if (pProjectile)
      {
         pProjectile->setFlagStaticPosition(true);
         mBeamProjectileID = pProjectile->getID();
         pProjectile->setFlagBeam(true);
         if (mpProtoAction)
         {
            pProjectile->setFlagBeamCollideWithUnits(mpProtoAction->doesBeamCollideWithUnits());
            pProjectile->setFlagBeamCollideWithTerrain(mpProtoAction->doesBeamCollideWithTerrain());
         }
         pProjectile->setParentID(pUnit->getID());
         pProjectile->setTargetLocation(mBeamEndPos);
         pProjectile->setForward(beamForward);
      }

      // kill squads stuck in the path
      killUnmovableSquads();
   }
}

//==============================================================================
//==============================================================================
void BUnitActionTowerWall::killUnmovableSquads()
{
   BUnit* pTowerUnit = mpOwner->getUnit();
   if (!pTowerUnit)
      return;
   BSquad* pTowerSquad = pTowerUnit->getParentSquad();
   if (!pTowerSquad)
      return;

   BEntityIDArray garrisonedSquads = pTowerSquad->getGarrisonedSquads();
   if (garrisonedSquads.isEmpty())
      return;

   BEntityID killer = garrisonedSquads[0];

   BUnit* pObsUnit = gWorld->getUnit(mObstructionUnitID);
   if(pObsUnit)
   {  
      BSquad* pObsSquad = pObsUnit->getParentSquad();
      if (pObsSquad)
      {
         BOPQuadHull hull;
         pObsSquad->getObstructionHull(hull);

         BEntityIDArray results(0, 100);
         BUnitQuery query1(&hull);
         query1.setFlagIgnoreDead(true);

         int numTargets = gWorld->getSquadsInArea(&query1, &results, false);
         for (int i = 0; i < numTargets; i++)
         {
            BEntityID squadID = results[i];
            BSquad* pSquad = gWorld->getSquad(squadID);

            // kill any squads in the obstruction 
            if (!pSquad || pSquad == pObsSquad || !pSquad->isEverMobile() || garrisonedSquads.contains(pSquad->getID()))
               continue;

            // we don't care about air units
            if (pSquad->getProtoObject() && pSquad->getProtoObject()->getMovementType() == cMovementTypeAir)
               continue;

            // is there seriously not a better way to kill with a valid killed by id? 
            uint numChildren = pSquad->getNumberChildren();
            for (uint j = 0; j < numChildren; ++j)
            {
               BUnit* pChildUnit = gWorld->getUnit(pSquad->getChild(j));
               if (pChildUnit)
                  pChildUnit->setKilledByID(killer);
            }
            pSquad->kill(false);
         }
      }
   }
}

//==============================================================================
//==============================================================================
void BUnitActionTowerWall::updateBeam(float elapsed)
{
   BUnit* pUnit = mpOwner->getUnit();
   BASSERT(pUnit);

   BProjectile* pProjectile = gWorld->getProjectile(mBeamProjectileID);
   if(pProjectile)
   {
      BVector currentPos = pProjectile->getTargetLocation();
      if(currentPos != mBeamEndPos)
      {
         BVector forward = mBeamEndPos - currentPos;
         float remainingDist = forward.length();
         forward.normalize();        

         if(!pProjectile->getProtoObject())
            return;
         
         forward.scale(elapsed * pProjectile->getProtoObject()->getMaxVelocity());
         float distToMove = forward.length();
         if(distToMove >= remainingDist)
         {
            pProjectile->setTargetLocation(mBeamEndPos);         
            setState(cStateNone);
         }
         else
         {  
            forward.scale(distToMove);
            currentPos += forward;
            pProjectile->setTargetLocation(currentPos);
         }
      }
   }
}

//==============================================================================
//==============================================================================
bool BUnitActionTowerWall::save(BStream* pStream, int saveType) const
{
   if (!BAction::save(pStream, saveType))
      return false;
   GFWRITECLASS(pStream, saveType, mTarget);
   GFWRITEVAR(pStream, bool, mGateOpen);
   GFWRITEVAR(pStream, BEntityID, mObstructionUnitID);
   GFWRITEVAR(pStream, BEntityID, mBeamProjectileID);
   GFWRITEVAR(pStream, double, mWaitTimer);
   GFWRITEVECTOR(pStream, mBeamStartPos);
   GFWRITEVECTOR(pStream, mBeamEndPos);
   return true;
}

//==============================================================================
//==============================================================================
bool BUnitActionTowerWall::load(BStream* pStream, int saveType)
{
   if (!BAction::load(pStream, saveType))
      return false;
   GFREADCLASS(pStream, saveType, mTarget);
   GFREADVAR(pStream, bool, mGateOpen);
   GFREADVAR(pStream, BEntityID, mObstructionUnitID);
   GFREADVAR(pStream, BEntityID, mBeamProjectileID);
   GFREADVAR(pStream, double, mWaitTimer);
   GFREADVECTOR(pStream, mBeamStartPos);
   GFREADVECTOR(pStream, mBeamEndPos);
   return true;
}

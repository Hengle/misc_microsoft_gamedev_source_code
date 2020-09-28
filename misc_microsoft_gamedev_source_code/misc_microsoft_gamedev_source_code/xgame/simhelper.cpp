//==============================================================================
// simhelper.cpp
//
// Copyright (c) 2008, Ensemble Studios
//==============================================================================
#include "common.h"
#include "ability.h"
#include "entity.h"
#include "kb.h"
#include "simhelper.h"
#include "team.h"
#include "world.h"
#include "unitquery.h"
#include "squadactionmove.h"
#include "squadplotter.h"
#include "commands.h"
#include "..\xsound\soundmanager.h"
#include "ui.h"
#include "gamesettings.h"
#include "powermanager.h"
#include "protopower.h"
#include "tactic.h"
#include "pather.h"
#include "alert.h"
#include "visual.h"
#include "grannyinstance.h"
#include "configsgame.h"

const float cCheckPlacementObstructionExpansionFactor = 0.7;
const float cCheckPlacementSearchDistanceFactor = 15.0f;

//==============================================================================
//==============================================================================
BVector BSimHelper::computeCentroid(const BEntityIDArray& entityIDs)
{
   BVector centroid = cOriginVector;
   float fCount = 0.0f;

   uint numEntities = entityIDs.getSize();
   for (uint i=0; i<numEntities; i++)
   {
      const BEntity* pEntity = gWorld->getEntity(entityIDs[i]);
      if (pEntity)
      {
         centroid += pEntity->getPosition();
         fCount += 1.0f;
      }
   }

   if (fCount > 0.0f)
      centroid /= fCount;

   return (centroid);
}


//==============================================================================
//==============================================================================
BVector BSimHelper::computeCentroid(const BKBSquadArray& kbSquads)
{
   BVector centroid = cOriginVector;
   float fCount = 0.0f;

   uint numKBSquads = kbSquads.getSize();
   for (uint i=0; i<numKBSquads; i++)
   {
      if (kbSquads[i])
      {
         centroid += kbSquads[i]->getPosition();
         fCount += 1.0f;
      }
   }

   if (fCount > 0.0f)
      centroid /= fCount;

   return (centroid);
}


//==============================================================================
//==============================================================================
BVector BSimHelper::computeCentroid(const BKBSquadIDArray& kbSquadIDs)
{
   BVector centroid = cOriginVector;
   float fCount = 0.0f;

   uint numKBSquadIDs = kbSquadIDs.getSize();
   for (uint i=0; i<numKBSquadIDs; i++)
   {
      const BKBSquad* pKBSquad = gWorld->getKBSquad(kbSquadIDs[i]);
      if (pKBSquad)
      {
         centroid += pKBSquad->getPosition();
         fCount += 1.0f;
      }
   }

   if (fCount > 0.0f)
      centroid /= fCount;

   return (centroid);
}


//==============================================================================
//==============================================================================
float BSimHelper::computeAssetValue(const BKBSquadIDArray& kbSquadIDs)
{
   float totalAssetValue = 0.0f;
   uint numKBSquadIDs = kbSquadIDs.getSize();
   for (uint i=0; i<numKBSquadIDs; i++)
   {
      const BKBSquad* pKBSquad = gWorld->getKBSquad(kbSquadIDs[i]);
      if (pKBSquad)
         totalAssetValue += pKBSquad->getAssetValue();
   }
   return (totalAssetValue);
}


//==============================================================================
//==============================================================================
float BSimHelper::computeTotalHP(const BKBSquadIDArray& kbSquadIDs)
{
   float totalHP = 0.0f;
   uint numKBSquadIDs = kbSquadIDs.getSize();
   for (uint i=0; i<numKBSquadIDs; i++)
   {
      const BKBSquad* pKBSquad = gWorld->getKBSquad(kbSquadIDs[i]);
      if (pKBSquad)
         totalHP += pKBSquad->getHitpoints();
   }
   return (totalHP);
}


//==============================================================================
//==============================================================================
float BSimHelper::computeTotalHP(const BEntityIDArray& squadIDs)
{
   float totalHP = 0.0f;
   uint numSquadIDs = squadIDs.getSize();
   for (uint i=0; i<numSquadIDs; i++)
   {
      const BSquad* pSquad = gWorld->getSquad(squadIDs[i]);
      if (pSquad)
      {
         totalHP += pSquad->getCurrentHP();
         continue;
      }
   }
   return (totalHP);
}


//==============================================================================
//==============================================================================
BEntityID BSimHelper::computeClosestToPoint(BVector pos, const BEntityIDArray& entityIDs)
{
   BEntityID closestEntityID = cInvalidObjectID;
   float closestDistSqr = cMaximumFloat;

   uint numEntities = entityIDs.getSize();
   for (uint i=0; i<numEntities; i++)
   {
      const BEntity* pEntity = gWorld->getEntity(entityIDs[i]);
      if (!pEntity)
         continue;

      float distSqr = pos.distanceSqr(pEntity->getPosition());
      if (distSqr < closestDistSqr)
      {
         closestDistSqr = distSqr;
         closestEntityID = entityIDs[i];
      }
   }

   return (closestEntityID);
}


//==============================================================================
//==============================================================================
void BSimHelper::computeMostNearAndFarToPoint(BVector pos, const BEntityIDArray& entityIDs, BEntityID* pNearID, float* pNearDist, BEntityID* pFarID, float* pFarDist)
{
   // Set up our initial values.
   BASSERT(pNearID || pNearDist || pFarID || pFarDist);
   BEntityID nearID = cInvalidObjectID;
   float nearDistSqr = cMaximumFloat;
   BEntityID farID = cInvalidObjectID;
   float farDistSqr = 0.0f;

   // Go through all the valid entities and get the near and far ones.
   uint numEntities = entityIDs.getSize();
   for (uint i=0; i<numEntities; i++)
   {
      const BEntity* pEntity = gWorld->getEntity(entityIDs[i]);
      if (pEntity)
      {
         float xzDistSqr = pos.xzDistanceSqr(pEntity->getPosition());
         if (xzDistSqr < nearDistSqr)
         {
            nearDistSqr = xzDistSqr;
            nearID = entityIDs[i];
         }
         if (xzDistSqr > farDistSqr)
         {
            farDistSqr = xzDistSqr;
            farID = entityIDs[i];
         }
      }
   }

   // Write out the results we care about.
   if (pNearID)
      *pNearID = nearID;
   if (pNearDist)
      *pNearDist = Math::fSqrt(nearDistSqr);
   if (pFarID)
      *pFarID = farID;
   if (pFarDist)
      *pFarDist = Math::fSqrt(farDistSqr);
}


//==============================================================================
//==============================================================================
BVector BSimHelper::randomCircularDistribution(BVector inputLoc, float outerRadius, float innerRadius)
{
   BASSERT(innerRadius >= 0.0f && innerRadius <= outerRadius);
   BVector randomLoc = inputLoc;
   float z = getRandRangeFloat(cSimRand, 0.0f, 1.0f);
   float r = (sqrt(z) * (outerRadius-innerRadius)) + innerRadius;
   randomLoc.x += r;
   float theta = getRandRangeFloat(cSimRand, 0.0f, cTwoPi);
   randomLoc.rotateRelativeXZ(inputLoc, theta);

   return (randomLoc);
}


//==============================================================================
//==============================================================================
void BSimHelper::calculateDebugRenderMatrix(BVector pos, BVector forward, BVector up, BVector right, float aboveGroundHeight, BMatrix& resultMatrix)
{
   BASSERT(aboveGroundHeight >= 0.0f);
   resultMatrix.makeOrient(forward, up, right);
   BVector offsetPosition = pos;
   offsetPosition.y += aboveGroundHeight;
   resultMatrix.setTranslation(offsetPosition);
}

//==============================================================================
// Group the contents of an entity list into smaller entity lists by player ID
//==============================================================================
BPlayerSpecificEntityIDsArray BSimHelper::groupEntityIDsByPlayer(const BEntityIDArray& entityIDs, BSimHelperFilterCallback filterCB)
{
   BPlayerSpecificEntityIDsArray playerEntityIDs;

   // Establish player IDs
   uint numPlayers = gWorld->getNumberPlayers();
   for (uint i = 0; i < numPlayers; i++)
   {
      const BPlayer* pPlayer = gWorld->getPlayer(i);
      if (pPlayer)
      {
         BPlayerSpecificEntityIDs tempPlayerEntityIDs;
         tempPlayerEntityIDs.mPlayerID = pPlayer->getID();
         playerEntityIDs.uniqueAdd(tempPlayerEntityIDs);
      }     
   }

   // Sort by player ID
   uint numEntityIDs = entityIDs.getSize();
   for (uint i = 0; i < numEntityIDs; i++)
   {
      uint numPlayers = playerEntityIDs.getSize();
      for (uint j = 0; j < numPlayers; j++)
      {
         BEntity* pEntity = gWorld->getEntity(entityIDs[i]);
         if (pEntity && (pEntity->getPlayerID() == playerEntityIDs[j].mPlayerID))
         {
            // Add entity if it passes the optional filter function
            if (!filterCB || filterCB(pEntity))
               playerEntityIDs[j].mEntityIDs.uniqueAdd(entityIDs[i]);
            break;
         }
      }
   }

   return (playerEntityIDs);
}

//======================================================================================================
// Move any squads around position
//======================================================================================================
bool BSimHelper::moveAllSquadsFromPosition(BVector position, float radius, const BEntityIDArray &ignoreSquads, bool& blocked, float testRadiusScale /*= 1.0f*/, int overrideSenderType /*= BCommand::cGame*/)
{      
   BUnitQuery query1(position, radius, true);
   BEntityIDArray results(0, 100);
   int numTargets = gWorld->getSquadsInArea(&query1, &results, false);
   int numTargetsToMove = 0;
   blocked = false;
   for (int i = 0; i < numTargets; i++)
   {
      // Skip over the ignore list
      if (ignoreSquads.contains(results[i]))
         continue;      

      BSquad* pSquad = gWorld->getSquad(results[i]);
      if (pSquad)
      {         
//-- FIXING PREFIX BUG ID 2948
         const BSquadActionMove* pMoveAction = (BSquadActionMove*)pSquad->getActionByTypeConst(BAction::cActionTypeSquadMove);
//--
         if (pMoveAction == NULL)
         {
            // Don't try to move squads that are in the process of loading/unloading, etc.
            if (pSquad->getFlagIsGarrisoning() || pSquad->getFlagIsUngarrisoning() || pSquad->getFlagIsHitching() || pSquad->getFlagIsUnhitching())
               continue;

            // Don't bother with squads that are garrisoned, attached, or hitched
            if (pSquad->getFlagGarrisoned() || pSquad->getFlagAttached() || pSquad->getFlagHitched())
               continue;

            // Can't move non-mobile units so completely blocked
            if (!pSquad->isMobile())
            {
               blocked = true;
               return (false);               
            }
         }

         const BProtoObject* pProtoObject = pSquad->getProtoObject();
         if (pProtoObject && (pProtoObject->getMovementType() != cMovementTypeAir))
            blocked = true;

         numTargetsToMove++;
      }
   }

   if (numTargetsToMove > 0)
   {
      float expandedObsRadius = testRadiusScale * radius;
      BUnitQuery query2(position, expandedObsRadius, true);
      results.clear();
      numTargets = gWorld->getSquadsInArea(&query2, &results, false);
      BEntityIDArray moveSquads;
      for (int i = 0; i < numTargets; i++)
      {
         // Skip over the ignore list
         if (ignoreSquads.contains(results[i]))
            continue;      

         BSquad* pSquad = gWorld->getSquad(results[i]);
         if (pSquad)
         {
            // Move all other squads that aren't moving
//-- FIXING PREFIX BUG ID 2949
            const BSquadActionMove* pMoveAction = (BSquadActionMove*)pSquad->getActionByTypeConst(BAction::cActionTypeSquadMove);
//--
            if (pMoveAction == NULL)
            {
               // Don't try to move squads that are in the process of loading/unloading, etc.
               if (pSquad->getFlagIsGarrisoning() || pSquad->getFlagIsUngarrisoning() || pSquad->getFlagIsHitching() || pSquad->getFlagIsUnhitching())
                  continue;

               // Don't bother moving non-mobile units
               if (!pSquad->isMobile())
                  continue;

               // Don't try to move air units
               const BProtoObject* pProtoObject = pSquad->getProtoObject();
               if(pProtoObject && (pProtoObject->getMovementType() == cMovementTypeAir))
                  continue;

               moveSquads.uniqueAdd(results[i]);
            }
         }
      }

      // Separate the squads by player
      BPlayerSpecificEntityIDsArray playerEntityIDs = BSimHelper::groupEntityIDsByPlayer(moveSquads);
      uint numPlayerEntityIDs = playerEntityIDs.getSize();
      for (uint i = 0; i < numPlayerEntityIDs; i++)
      {
         uint numEntities = playerEntityIDs[i].mEntityIDs.getSize();
         if (numEntities > 0)
         {
            BObjectCreateParms objectParms;
            objectParms.mPlayerID = playerEntityIDs[i].mPlayerID;
            BArmy* pArmy = gWorld->createArmy(objectParms);
            if (pArmy)
            {
               // Platoon the squads
               pArmy->addSquads(playerEntityIDs[i].mEntityIDs);

               // Move the squads to form around the blocking squad            
               BWorkCommand tempCommand;
               position.w = radius;
               tempCommand.setWaypoints(&position, 1);
               tempCommand.setRange(expandedObsRadius);
               tempCommand.setRecipientType(BCommand::cArmy);
               tempCommand.setSenderType(overrideSenderType);
               tempCommand.setFlagOverridePosition(true);
               tempCommand.setFlagOverrideRange(true);               
               pArmy->queueOrder(&tempCommand);
            }               
         }
      }
   }

   return (true);
}

//======================================================================================================
// Move any squads around obstruction squad
//======================================================================================================
bool BSimHelper::moveAllSquadsFromObstructionSquad(BEntityID obstructionSquad, bool& blocked, float testRadiusScale /*= 1.0f*/, int overrideSenderType /*= BCommand::cGame*/) //int overridePriority /*= BSimOrder::cPrioritySim*/)
{
   bool result = false;
   blocked = false;
   BSquad* pObsSquad = gWorld->getSquad(obstructionSquad);
   BDEBUG_ASSERT(pObsSquad);
   if (pObsSquad)
   {
      static BEntityIDArray ignoreSquads;
      ignoreSquads.resize(0);
      ignoreSquads.add(obstructionSquad);
      result = moveAllSquadsFromPosition(pObsSquad->getAveragePosition(), pObsSquad->getObstructionRadius(), ignoreSquads, blocked, testRadiusScale, overrideSenderType);   
   }
   
   return (result);
}

//======================================================================================================
// Calculate exit location from an ungarrsion target
//======================================================================================================
bool BSimHelper::calculateExitLocation(const BUnit* pContainerUnit, const BSquad* pExitSquad, BVector& result)
{
   bool success = false;
   result = cInvalidVector;   
   if (pContainerUnit && pExitSquad)
   {
      // Expand the obstruction to give it some extra spacing away from other squads
      float obstructionRadius = pExitSquad->getObstructionRadius();
      float radiusExpansion = obstructionRadius * cCheckPlacementObstructionExpansionFactor;

      // Set the search scale so it considers a large enough area for valid positions.  The search scale is a factor of tile size.
      float searchScaleAsFloat = obstructionRadius * cCheckPlacementSearchDistanceFactor * gTerrainSimRep.getReciprocalDataTileScale();
      int searchScale = Math::Max(1, Math::FloatToIntTrunc(searchScaleAsFloat));

      DWORD flags = BWorld::cCPCheckObstructions | BWorld::cCPCheckSquadObstructions | BWorld::cCPExpandHull | BWorld::cCPPushOffTerrain | 
                    BWorld::cCPLOSCenterOnly | BWorld::cCPIgnorePlacementRules | BWorld::cCPSetPlacementSuggestion;
      BVector suggestedPos = cInvalidVector;
      if (gWorld->checkPlacement(pExitSquad->getProtoObjectID(), pExitSquad->getPlayerID(), pContainerUnit->getPosition(), suggestedPos, cZAxisVector, BWorld::cCPLOSDontCare, 
                                 flags, searchScale, NULL, NULL, -1, pExitSquad->getProtoSquadID(), true, radiusExpansion))
      {
         result = suggestedPos;
         success = true;
      }      
   }

   return (success);
}

//==========================================================================================
// Determines if the exit location is blocked by non-mobile obstructions
//==========================================================================================
bool BSimHelper::findPosForAirborneSquad(BSquad* pHeroSquad, float validRadius, bool moveToRally, bool killOnFail, bool placeWhenUnobstructed)
{
   BASSERT(pHeroSquad);
   BASSERT(validRadius > 0.0f);

   if (!pHeroSquad)
      return false;

   // See if squad is in a pathable area
   if (gPather.isObstructedTile(pHeroSquad->getPosition()))
   {
      BVector newPos = pHeroSquad->getPosition();
      bool valid = false;

      BEntityIDArray ignoreSquads;
      valid = BSimHelper::evaluateExitLocation(pHeroSquad, ignoreSquads, pHeroSquad->getPosition(), newPos);

      if (pHeroSquad->isOutsidePlayableBounds(true))
      {
         if (newPos.x < gWorld->getSimBoundsMinX()) newPos.x = gWorld->getSimBoundsMinX() + 1.0f;
	      if (newPos.z < gWorld->getSimBoundsMinZ()) newPos.z = gWorld->getSimBoundsMinZ() + 1.0f;
	      if (newPos.x > gWorld->getSimBoundsMaxX()) newPos.x = gWorld->getSimBoundsMaxX() - 1.0f;
	      if (newPos.z > gWorld->getSimBoundsMaxZ()) newPos.z = gWorld->getSimBoundsMaxZ() - 1.0f;

         valid = false;
      }

      if (!valid || newPos == pHeroSquad->getPosition())
         valid = gPather.findClosestPassableTileEx(pHeroSquad->getPosition(), newPos);

      if (!valid)
         return false;

      float dist = pHeroSquad->getPosition().xzDistance(newPos);

      // Outside the specified radius
      if (dist > validRadius)
      {
         // Don't try to move to rally, just kill them in place
         if (!moveToRally)
         {
            if (killOnFail)
            {
               pHeroSquad->kill(false);
               return true;
            }
            else
            {
               return false;
            }
         }
         else
         {
            // Try to find a valid rally point to put the hero at
            BVector rally = cOriginVector;

            // Get the first base's rally point
            uint numBases = pHeroSquad->getPlayer()->getNumberGotoBases();
            if (numBases > 0)
            {
               BUnit* pBase = gWorld->getUnit(pHeroSquad->getPlayer()->getGotoBase(0));
               if (pBase)
                  rally = pBase->getRallyPoint();
            }

            // No valid rally point, try the global rally point
            if (rally.almostEqual(cOriginVector))
               rally = pHeroSquad->getPlayer()->getRallyPoint();

            // Nope, just teleport them out (yuck)
            if (rally.almostEqual(cOriginVector))
               rally = newPos;

            // Save off current rally point, just in case
            newPos = rally;
         }
      }

      // Find the closest valid point to the new position
      BVector finalPos;
      valid = BSimHelper::evaluateExitLocation(pHeroSquad, ignoreSquads, newPos, finalPos);

      if (!valid || newPos == pHeroSquad->getPosition())
         valid = gPather.findClosestPassableTileEx(newPos, finalPos);

      // Can't find a spot, just dump them at the original location
      if (!valid)
         finalPos = newPos;

      // Make sure we're at ground level
      gTerrainSimRep.getHeight(finalPos, true);

      pHeroSquad->doTeleport(finalPos);

      // Tell the player we moved the squad
      if (moveToRally)
         pHeroSquad->getPlayer()->getAlertManager()->createTransportCompleteAlert(finalPos, pHeroSquad->getID());
   }
   else if (placeWhenUnobstructed)
   {
      // Make sure hero is on the ground
      BVector pos = pHeroSquad->getPosition();
      gTerrainSimRep.getHeightRaycast(pos, pos.y, true);
      pos.y += 1.0f; // Push us above the ground so we will tie to it correctly
      pHeroSquad->doTeleport(pos);
      pHeroSquad->tieToGround();
   }

   return true;
}

//XXXHalwes - Query loops could be combined into one here.  Revisit if it is a perf problem.
//==========================================================================================
// Determines if the exit location is NOT blocked by non-mobile or locked-down obstructions
//==========================================================================================
bool BSimHelper::verifyExitLocation(const BSquad* pExitSquad, const BEntityIDArray &ignoreSquads, BVector exitLocation)
{
   if (!pExitSquad)
   {
      return (false);
   }

   long obstructionQuadTrees =
      BObstructionManager::cIsNewTypeCollidableNonMovableUnit | // Unit that can't move                                                       
      BObstructionManager::cIsNewTypeBlockLandUnits;            // Terrain that blocks any combo of movement that includes land-based movement
   BPlayerID playerID = pExitSquad->getPlayerID();
   float obsRadius = pExitSquad->getObstructionRadius();   
   float expansion = 0.0f; //obsRadius * 0.1f;
   bool result = gObsManager.testObstructions(exitLocation, obsRadius, expansion, obstructionQuadTrees, BObstructionManager::cObsNodeTypeAll, playerID);

   // Second pass to look for locked-down squads that are mobile
   if (!result)
   {
      BEntityID exitSquadID = pExitSquad->getID();            
      int numResults = 30;
      BUnitQuery query1(exitLocation, obsRadius, true);
      BEntityIDArray queryResults(0, numResults);
      int numTargets = gWorld->getSquadsInArea(&query1, &queryResults, false);
      for (int i = 0; i < numTargets; i++)
      {
//-- FIXING PREFIX BUG ID 2951
         const BSquad* pSquad = gWorld->getSquad(queryResults[i]);
//--
         if (!pSquad)
         {
            continue;
         }

         // Skip over the obstruction's squad
         if (queryResults[i] == exitSquadID)
         {
            continue;      
         }

         // Skip over squads in ignore list
         if (ignoreSquads.contains(queryResults[i]))
         {
            continue;
         }

         // Skip over squads in the process of garrisoning, ungarrisoning, hitching, or unhitching
         if (pSquad->getFlagIsGarrisoning() || pSquad->getFlagIsUngarrisoning() || pSquad->getFlagIsHitching() || pSquad->getFlagIsUnhitching())
         {
            continue;
         }
         
         // Skip over garrisoned, attached, and hitched squads
         if (pSquad->getFlagGarrisoned() || pSquad->getFlagAttached() || pSquad->getFlagHitched())
         {
            continue;
         }

         if (pSquad->isLockedDown())
         {         
            result = true;
            break;
         }
      }
   }

   return (!result);
}

//==============================================================================
// Clear the exit location of all mobile squads
//==============================================================================
bool BSimHelper::clearExitLocation(BSquad* pExitSquad, const BEntityIDArray &ignoreSquads, BVector exitLocation, bool& blocked)
{
   bool success = false;
   blocked = false;   
   if (pExitSquad)
   {      
      static BEntityIDArray newIgnoreSquads;
      newIgnoreSquads.resize(0);
      newIgnoreSquads.add(pExitSquad->getID());
      newIgnoreSquads.append(ignoreSquads);
      float obstructionRadius = pExitSquad->getObstructionRadius();
      success = BSimHelper::moveAllSquadsFromPosition(exitLocation, obstructionRadius, newIgnoreSquads, blocked, gDatabase.getTransportClearRadiusScale(), BCommand::cPlayer);
   }

   return (success);
}

//=======================================================================================
// Evaluate the exit location and determine a valid location utilizing the squad plotter
//=======================================================================================
bool BSimHelper::evaluateExitLocation(BSquad* pExitSquad, const BEntityIDArray &ignoreSquads, BVector exitLocation, BVector& suggestedExitLocation, float maxObsRadiusScale /*= 5.0f*/)
{
   if (!pExitSquad)
      return (false);

   bool validLocFound = BSimHelper::verifyExitLocation(pExitSquad, ignoreSquads, suggestedExitLocation);

   if(!validLocFound)
   {
      static BDynamicSimVectorArray instantiatePositions;
      instantiatePositions.setNumber(0);
      long closestDesiredPositionIndex = -1;
      const float cGrowMultiplier = 0.8f;
      bool success = BSimHelper::findInstantiatePositions(1, instantiatePositions, 0.5f, exitLocation,
         pExitSquad->getForward(), pExitSquad->getRight(), pExitSquad->getObstructionRadius() * 0.5f, suggestedExitLocation, closestDesiredPositionIndex, 7, true, cGrowMultiplier);

      if(success)
      {
         validLocFound = true;
         if(closestDesiredPositionIndex >= 0 && closestDesiredPositionIndex < instantiatePositions.getNumber())
            suggestedExitLocation = instantiatePositions[closestDesiredPositionIndex];
         else if(instantiatePositions.getNumber() > 0)
            suggestedExitLocation = instantiatePositions[0];
      }
   }

   return (validLocFound);
}


//==============================================================================
// BSimHelper::findInstantiatePositions
// Copied over from Age3.
//==============================================================================
bool BSimHelper::findInstantiatePositions(long numPositions, BDynamicSimVectorArray &instantiatePositions,
   float buildingObstructionRadius, BVector buildingPosition, const BVector& forward, const BVector& right,
   float squadObstructionRadius, const BVector &desiredPosition, long &closestDesiredPositionIndex, long growAttempts,
   bool tryInitialLocation, float obsRadiusGrowthMultiplierPerAttempt)
{
   closestDesiredPositionIndex = -1;
   float newUnitObstructionRadius = squadObstructionRadius;

   // jce 1/8/2002 -- if this is zero, the following stuff will hang, so fake out a minimum size of 0.2
   if(newUnitObstructionRadius<0.2f)
      newUnitObstructionRadius=0.2f;

   instantiatePositions.clear();

   float newUnitObstructionSize = newUnitObstructionRadius*2.0f;

   long quadtrees = 
      BObstructionManager::cIsNewTypeAllCollidableUnits |       // Unit that can and can't move                                                       
      BObstructionManager::cIsNewTypeBlockLandUnits;            // Terrain that blocks any combo of movement that includes land-based movement
   
   gObsManager.begin(BObstructionManager::cBeginUnit, newUnitObstructionRadius, quadtrees, BObstructionManager::cObsNodeTypeAll,
      0, cDefaultRadiusSofteningFactor, NULL, 0);

   BVector vPos = buildingPosition;

   // Set buildingObstructionSize just once.
   float buildingObstructionSize = buildingObstructionRadius*2.0f;

   // Aw for crying out loud just calculate the values once.  dlm 8/28/02
   float fDeltaState = newUnitObstructionSize * 1.1f;
   BVector vDeltaForward = forward * fDeltaState;
   BVector vDeltaRight = right * fDeltaState;

   long attempts = 0;

   if(tryInitialLocation)
   {
      bool anyObstructions = gObsManager.findObstructionsOnPoint(vPos, 0.0f, 0, 0, 0, false, gObsManager.getFoundObstructionResults());

/*
      DWORD color = cDWORDYellow;
      if(anyObstructions)
         color = cDWORDRed;
      gpDebugPrimitives->addDebugSphere(vPos, 3.0f, color, BDebugPrimitives::cCategoryNone, 5.0f);
*/

      if(!anyObstructions)
      {
         instantiatePositions.add(vPos);
         if (instantiatePositions.getNumber() == numPositions)
         {
            closestDesiredPositionIndex = 0;
            gObsManager.end();
            return true;
         }
      }

      attempts++;
   }

   // seed with our actual position
   BVector instantiatePosition;
   while (attempts <= growAttempts)
   {
      instantiatePosition = vPos;
      BVector vResetForward = forward * (buildingObstructionRadius+newUnitObstructionRadius) * 1.1f;
      BVector vResetRight = right * (buildingObstructionRadius+newUnitObstructionRadius) * 1.1f;

      long state=-1;
      float stateDistance=0.0f;
      float closestDesiredPositionDistance=cMaximumFloat;
      while (state < 4) 
      {
         switch (state)
         {
            case -1:
               //Set the initial pos to the "upper left".
               instantiatePosition += vResetForward;
               instantiatePosition -= vResetRight;
               state++;
               stateDistance=0.0f;
               break;
            case 0:
               //Do the "top" row.
               instantiatePosition += vDeltaRight;
               stateDistance += fDeltaState;
               if (stateDistance >= buildingObstructionSize)
               {
                  //Set the pos for the "right" side.
                  instantiatePosition = vPos;
                  instantiatePosition += vResetForward;
                  instantiatePosition += vResetRight;
                  state++;
                  stateDistance=0.0f;
               }
               break;
            case 1:
               //Do the "right" side.
               instantiatePosition -= vDeltaForward;
               stateDistance += fDeltaState;
               if (stateDistance >= buildingObstructionSize)
               {
                  //Set the pos for the "bottom" side.
                  instantiatePosition = vPos;
                  instantiatePosition -= vResetForward;
                  instantiatePosition += vResetRight;
                  state++;
                  stateDistance=0.0f;
               }
               break;
            case 2:
               //Do the "bottom" side.
               instantiatePosition -= vDeltaRight;
               stateDistance += fDeltaState;
               if (stateDistance >= buildingObstructionSize)
               {
                  //Set the pos for the "left" side.
                  instantiatePosition = vPos;
                  instantiatePosition -= vResetForward;
                  instantiatePosition -= vResetRight;
                  state++;
                  stateDistance=0.0f;
               }
               break;
            case 3:
               //So the "left" side.
               instantiatePosition += vDeltaForward;
               stateDistance += fDeltaState;
               if (stateDistance >= buildingObstructionSize)
               {
                  //We're done.
                  state++;
               }
               break;
         }
         
         // Before even checking the obmgr, check against the boundaries of the map.
         // if it's illegal, just continue.. dlm 1/25/01
         if (gWorld->isOutsidePlayableBounds(instantiatePosition, true))
            continue;

		   bool anyObstructions = gObsManager.findObstructionsOnPoint(instantiatePosition, 0.0f, 0, 0, 0, false, gObsManager.getFoundObstructionResults());

/*
         DWORD color = cDWORDYellow;
         if(anyObstructions)
            color = cDWORDRed;
         gpDebugPrimitives->addDebugSphere(instantiatePosition, 2.0f, color, BDebugPrimitives::cCategoryNone, 5.0f);
*/

         if (!anyObstructions)
         {
            instantiatePositions.add(instantiatePosition);

            //If we have a desired position, we are going to look all the way around
            //the building and track the closest index to the point.
            if (desiredPosition.x >= 0.0f)
            {
               BVector diff=desiredPosition-instantiatePosition;
               diff.y=0.0f;
               float diffLength=diff.length();
               if ((diffLength < closestDesiredPositionDistance) || (closestDesiredPositionIndex == -1))
               {
                  closestDesiredPositionDistance=diffLength;
                  closestDesiredPositionIndex=instantiatePositions.getNumber()-1;
               }
            }
            //Else, if we have found a sufficient number, we win
            else if (instantiatePositions.getNumber() == numPositions)
               break;
         }
      }
      // okay, if we didn't find enough on this pass, grow our radius
      if (instantiatePositions.getNumber() >= numPositions)
         break;

      // tick off one more try
      attempts++;
      
      // increase the search distance
      float grow = squadObstructionRadius * (1.0f + attempts * obsRadiusGrowthMultiplierPerAttempt);
      buildingObstructionRadius += grow;
      buildingObstructionSize += 2 * grow;
   }

   //Quit the ObMgr.
   gObsManager.end();

   //If we found enough positions, we're good.
   return(instantiatePositions.getNumber() >= numPositions);
}

//==============================================================================
//==============================================================================
bool BSimHelper::putSquadAtUnobstructedPosition(BSquad *pSquad)
{
   if (!pSquad)
      return false;

   // Find closest unobstructed position around containing squad.
   static BDynamicSimVectorArray instantiatePositions;
   instantiatePositions.setNumber(0);
   long closestDesiredPositionIndex;
   bool success = BSimHelper::findInstantiatePositions(1, instantiatePositions, pSquad->getObstructionRadius(), pSquad->getPosition(),
      pSquad->getForward(), pSquad->getRight(), pSquad->getObstructionRadius(), pSquad->getPosition(), closestDesiredPositionIndex, 4, false);
   if (success && (instantiatePositions.getSize() > 0))
   {
      // Position found
      BVector pos(pSquad->getPosition());
      if ((closestDesiredPositionIndex >= 0) && (closestDesiredPositionIndex < instantiatePositions.getNumber()))
         pos = instantiatePositions[closestDesiredPositionIndex];
      else
         pos = instantiatePositions[0];

      pSquad->setPosition(pos);
      pSquad->noStopSettle();
      pSquad->setTurnRadiusPos(pSquad->getPosition());
      pSquad->setLeashPosition(pSquad->getPosition());

      for (uint i = 0; i < pSquad->getNumberChildren(); i++)
      {
         BUnit *pUnit = gWorld->getUnit(pSquad->getChild(i));
         if (!pUnit)
            continue;

         // Settle will move units not controlled by physics.  Now move the physics controlled ones.
         if (pUnit->getFlagPhysicsControl())
         {
            BVector desiredLocation = pSquad->getFormationPosition(pUnit->getID());
            if (!desiredLocation.almostEqual(cInvalidVector))
            {
               if (pUnit->getProtoObject() && (pUnit->getProtoObject()->getMovementType() != cMovementTypeAir))
                  gTerrainSimRep.getHeightRaycast(desiredLocation, desiredLocation.y, true);

               pUnit->setPosition(desiredLocation, true);
               pUnit->setPhysicsKeyFramed(false, true);
            }
         }
 
         // If subupdating enabled then reset the interpolation matrices so it doesn't briefly render at its
         // old temporary location when it was contained.
         if (gEnableSubUpdating)
         {
            BMatrix matrix;
            pUnit->getWorldMatrix(matrix);
            pUnit->setInterpolationMatrices(matrix);
         }
      }
   }
   return true;
}

//==============================================================================
//==============================================================================
void BSimHelper::calculateSelectionAbility(const BEntityIDArray& squadIDs, BPlayerID playerID, BSelectionAbility& ability)
{
   ability.mAbilityID = -1;
   ability.mTargetType = -1;
   ability.mPlayer = false;

   uint numSquads = squadIDs.getSize();
   for (uint i=0; i<numSquads; i++)
   {
      const BSquad* pSquad = gWorld->getSquad(squadIDs[i]);
      if (!pSquad)
         continue;

      uint numChildren = pSquad->getNumberChildren();
      for (uint j=0; j<numChildren; j++)
      {
         const BUnit* pUnit = gWorld->getUnit(pSquad->getChild(j));
         if (!pUnit)
            continue;

         const BProtoObject* pProtoObject = pUnit->getProtoObject();
         if (!pProtoObject)
            continue;

         int abilityID = pProtoObject->getAbilityCommand();
         if (pProtoObject->getFlagAbilityDisabled())
            continue;

         if (abilityID != -1)
         {
//-- FIXING PREFIX BUG ID 2954
            const BAbility* pAbility = gDatabase.getAbilityFromID(abilityID);
//--
            ability.mPlayer = (pUnit->getPlayerID() == playerID);
            ability.mAbilityID = abilityID;
            ability.mTargetType = pAbility->getTargetType();
            ability.mAbilityType = pAbility->getType();
            return;
         }         
      }
   }

   // Didn't find anything.
   return;
}


//==============================================================================
//==============================================================================
void BSimHelper::updateSelectionAbility(const BEntityIDArray& squadIDs, BEntityID hoverObject, BVector hoverPoint, BSelectionAbility& ability)
{
   ability.mReverse = false;
   ability.mValid = false;
   ability.mTargetUnit = false;
   ability.mRecovering = true;

   // No squads.
   uint numSquads = squadIDs.getSize();
   if (numSquads == 0)
   {
      ability.mAbilityID = -1;
      ability.mTargetType = -1;
      return;
   }

   // Bad ability.
   if (ability.mAbilityID == -1)
      return;

   // Check for specific types of abilities
   if (ability.mTargetType == BAbility::cTargetNone || ability.mTargetType == BAbility::cTargetLocation || ability.mTargetType == BAbility::cTargetUnitOrLocation)
   {
      // Test the validity of a changeMode ability.
      if (ability.mAbilityType == cAbilityChangeMode)
      {
         BSimHelper::updateValidAbilityChangeMode(squadIDs, ability);
      }
      else if (ability.mAbilityType==cAbilityUnload)
      {
         BSimHelper::updateValidAbilityUnload(squadIDs, ability);
      }
      else if (ability.mAbilityType==cAbilityPower)
      {
         BSimHelper::updateValidAbilityPower(squadIDs, ability, hoverPoint);
      }
      else
         ability.mValid = true;
   }

   BAbilityID aidCommand = gDatabase.getAIDCommand();
   for (uint i=0; i<numSquads; i++)
   {
      const BSquad* pSquad = gWorld->getSquad(squadIDs[i]);
      if (!pSquad)
         continue;
      BAbilityID abilityID;
      const BProtoAction* pProtoAction = pSquad->getProtoActionForTarget(hoverObject, hoverPoint, aidCommand, false, NULL, false, &abilityID);
      if (!pProtoAction)
         continue;
      // Only accept the proto action if it's for the ability we're actually looking for
      if (abilityID != aidCommand)
         continue;

      ability.mTargetUnit = true;
      if (ability.mValid && ability.mReverse)
         ability.mReverse = false;
      else
         ability.mValid = true;

      // [8/27/2008 JRuediger] Special stasis hack.
      ability.mHideYSpecial = false;
      if(pProtoAction->getFlagStasis() || pProtoAction->getFlagStasisBomb() || pProtoAction->getFlagStasisDrain())
      {
         ability.mHideYSpecial = true;
      }
      break;
   }

   for (uint i=0; i<numSquads; i++)
   {
      const BSquad* pSquad = gWorld->getSquad(squadIDs[i]);
      if (!pSquad)
         continue;
      if (pSquad->getRecoverType() == cRecoverAbility)
         continue;
      if (pSquad->getFlagUsingTimedAbility())
         continue;
      ability.mRecovering = false;
      break;
   }
}


//==============================================================================
//==============================================================================
void BSimHelper::updateValidAbilityChangeMode(const BEntityIDArray& squadIDs, BSelectionAbility& ability)
{
   // This function only sets valid to false for ChangeMode ability type.
   BASSERT(ability.mAbilityType == cAbilityChangeMode);
   if (ability.mAbilityType != cAbilityChangeMode)
      return;

   // If the ability is already marked as mValid = false, this has no effect so bail.
   if (!ability.mValid)
      return;

   uint numSquads = squadIDs.getSize();
   bool allChanging = true;
   bool allChanged = true;
   bool anyChanging = false;

   for (uint i=0; i<numSquads; i++)
   {
      const BSquad* pSquad = gWorld->getSquad(squadIDs[i]);
      if (!pSquad)
         continue;

      if (!pSquad->getFlagChangingMode())
         allChanging = false;
      else
         anyChanging = true;

      if (pSquad->getCurrentOrChangingMode() == BSquadAI::cModeNormal)
         allChanged = false;
   }

   if (allChanging || (anyChanging && allChanged))
      ability.mValid = false;
}


//==============================================================================
//==============================================================================
void BSimHelper::updateValidAbilityUnload(const BEntityIDArray& squadIDs, BSelectionAbility& ability)
{
   BASSERT(ability.mAbilityType == cAbilityUnload);
   if (ability.mAbilityType != cAbilityUnload)
      return;

   uint numSquads = squadIDs.getSize();
   for (uint i=0; i<numSquads; i++)
   {
      const BSquad* pSquad = gWorld->getSquad(squadIDs[i]);
      if (!pSquad)
         continue;

      const BUnit* pUnit = pSquad->getLeaderUnit();
      if (!pUnit)
         continue;

      const BProtoObject* pProtoObject = pUnit->getProtoObject();
      if (!pProtoObject)
         continue;

      if (pProtoObject->getAbilityCommand() != ability.mAbilityID)
         continue;

      const BEntityRef* pContainUnitRef = pUnit->getFirstEntityRefByType(BEntityRef::cTypeContainUnit);
      const BEntityRef* pAttachObjectRef = pUnit->getFirstEntityRefByType(BEntityRef::cTypeAttachObject);
      if (pContainUnitRef || pAttachObjectRef)
      {
         ability.mValid = true;
         return;
      }
   }
}

//==============================================================================
//==============================================================================
void BSimHelper::updateValidAbilityPower(const BEntityIDArray& squadIDs, BSelectionAbility& ability, const BVector& hoverPoint)
{
   // This function only sets valid to false for Power ability type.
   BASSERT(ability.mAbilityType == cAbilityPower);
   if (ability.mAbilityType != cAbilityPower)
      return;

   const BAbility* pAbility = gDatabase.getAbilityFromID(ability.mAbilityID);
   if (!pAbility)
      return;

   uint numSquads = squadIDs.getSize();
   for (uint i=0; i<numSquads; i++)
   {
      const BSquad* pSquad = gWorld->getSquad(squadIDs[i]);
      if (!pSquad)
         continue;

      const BUnit* pUnit = pSquad->getLeaderUnit();
      if (!pUnit)
         continue;

      const BProtoObject* pProtoObject = pUnit->getProtoObject();
      if (!pProtoObject)
         continue;

      const BProtoPower* pProtoPower = gDatabase.getProtoPowerByID(pProtoObject->getProtoPowerID());
      if (!pProtoPower)
         continue;

      // sucks to have to do a string comparison here... *sigh* 
      if (pProtoPower->getName() != pAbility->getName())
         continue;

      BPlayer* pPlayer = gWorld->getPlayer(pSquad->getPlayerID());
      if (pPlayer && gWorld->getGametime() < pPlayer->getPowerAvailableTime(pProtoObject->getProtoPowerID()))
         continue;

      if (gWorld->getPowerManager() && gWorld->getPowerManager()->isSquadPowerValid(pSquad->getPlayerID(), *pSquad, *pProtoPower, hoverPoint))
      {
         ability.mValid = true;
         return;
      }
   }
}


//==============================================================================
//==============================================================================
void BSimHelper::diffEntityIDArrays(const BEntityIDArray& arrayA, const BEntityIDArray& arrayB, BEntityIDArray *pOnlyInA, BEntityIDArray *pOnlyInB, BEntityIDArray *pInBoth)
{
   BASSERT(pOnlyInA || pOnlyInB || pInBoth);
   BASSERT(!pOnlyInA || !pOnlyInB || pOnlyInA != pOnlyInB);
   BASSERT(!pOnlyInA || !pInBoth || pOnlyInA != pInBoth);
   BASSERT(!pOnlyInB || !pInBoth || pOnlyInB != pInBoth);
   if (!pOnlyInA && !pOnlyInB && !pInBoth)
      return;

   // Get local copies of our source data.
   BEntityIDArray listA = arrayA;
   BEntityIDArray listB = arrayB;
   uint sizeA = listA.getSize();
   uint sizeB = listB.getSize();

   // Clear our results lists.
   if (pOnlyInA)
      pOnlyInA->resize(0);
   if (pOnlyInB)
      pOnlyInB->resize(0);
   if (pInBoth)
      pInBoth->resize(0);

   // Easiest case:  Both lists empty.
   if (sizeA == 0 && sizeB == 0)
   {
      return;
   }
   // Easy case:  ListA empty.
   else if (sizeA == 0)
   {
      if (pOnlyInB || pInBoth)
      {
         for (uint i=0; i<sizeB; i++)
         {
            if (pOnlyInB)
               pOnlyInB->add(listB[i]);
            if (pInBoth)
               pInBoth->add(listB[i]);
         }
      }
   }
   // Easy case:  ListB empty.
   else if (sizeB == 0)
   {
      if (pOnlyInA || pInBoth)
      {
         for (uint i=0; i<sizeA; i++)
         {
            if (pOnlyInA)
               pOnlyInA->add(listA[i]);
            if (pInBoth)
               pInBoth->add(listA[i]);
         }
      }
   }
   // Both lists have something.
   else
   {
      // Check listA's stuff.
      if (pOnlyInA || pInBoth)
      {
         for (uint i=0; i<sizeA; i++)
         {
            // Couldn't find it, so it's only in A.
            if (!listB.contains(listA[i]))
            {
               if (pOnlyInA)
                  pOnlyInA->add(listA[i]);
            }
            // Otherwise, it's in both lists.
            else
            {
               if (pInBoth)
                  pInBoth->add(listA[i]);
            }
         }
      }

      // Check listB's stuff.
      if (pOnlyInB)
      {
         for (uint i=0; i<sizeB; i++)
         {
            // Couldn't find it, so it's only in B.
            if (!listA.contains(listB[i]))
            {
               if (pOnlyInB)
                  pOnlyInB->add(listB[i]);
            }
            // And we already handled the case of being in both lists.
         }
      }
   }
}

//==============================================================================
//==============================================================================
float BSimHelper::getCombatValueRepairCost(const BEntityIDArray& squadIDs)
{
   float combatValueRepairCost = 0.0f;
   uint numSquads = squadIDs.getSize();

   BCost squadCost;
   float squadCombatValueRepairCost = 0.0f;

   for (uint i=0; i<numSquads; i++)
   {
      const BSquad* pSquad = gWorld->getSquad(squadIDs[i]);
      if (!pSquad)
         continue;
      pSquad->getRepairCost(squadCost, squadCombatValueRepairCost);
      combatValueRepairCost += squadCombatValueRepairCost;
   }

   return (combatValueRepairCost);
}

//==============================================================================
//==============================================================================
void BSimHelper::repairByCombatValue(const BEntityIDArray& squadIDs, float combatValue, bool spreadAcrossSquads, bool allowReinforce)
{
   // Bomb check.
   if (combatValue <= 0.0f)
      return;
   uint numSquads = squadIDs.getSize();
   if (numSquads == 0)
      return;

   if (spreadAcrossSquads)
   {
      // Spread combat value repairing over each squad proportional to the 
      // combat value of that squad.  Excess combat value spills over and
      // is divided up to remaining squads.
      // Make list of squads that need repairing and total
      // up their combat values
      BDynamicSimFloatArray repairCosts(0, numSquads);
      BDynamicSimFloatArray baseCombatValues(0, numSquads);
      float totalCombatValue = 0.0f;

      BEntityIDArray squadsToRepair(0, numSquads);
      for (uint i=0; i<numSquads; i++)
      {
         const BSquad* pSquad = gWorld->getSquad(squadIDs[i]);
         if (!pSquad)
            continue;

         BCost squadCost;
         float squadCombatValueRepairCost = 0.0f;
         if (pSquad->getRepairCost(squadCost, squadCombatValueRepairCost))
         {
            float baseCV = pSquad->getCombatValue();
            squadsToRepair.add(squadIDs[i]);
            repairCosts.add(squadCombatValueRepairCost);
            baseCombatValues.add(baseCV);
            totalCombatValue += baseCV;
         }
      }

      // Nobody needs healing I guess...?
      uint numSquadsToRepair = squadsToRepair.getSize();
      if (numSquadsToRepair == 0)
         return;      
      if (totalCombatValue < cFloatCompareEpsilon)
         return;

      // Sort squads by excess repair cost (repair slice - repair cost
      BDynamicSimFloatArray sortByRepairDiff(0, numSquadsToRepair);
      BDynamicSimFloatArray sortedBaseCombatValues(0, numSquadsToRepair);
      BEntityIDArray sortedSquads(0, numSquadsToRepair);

      for (uint i=0; i<numSquadsToRepair; i++)
      {
         // Calculate amount of heal cost proportional to squad's combat value
         float squadRepairPortion = (baseCombatValues[i] / totalCombatValue) * combatValue;
         float repairDiff = squadRepairPortion - repairCosts[i];

         // Insert in sorted order
         long j = 0;
         while ((j < sortByRepairDiff.getNumber()) && (repairDiff < sortByRepairDiff[j]))
            j++;
         sortByRepairDiff.insertAtIndex(repairDiff, j);
         sortedBaseCombatValues.insertAtIndex(baseCombatValues[i], j);
         sortedSquads.insertAtIndex(squadsToRepair[i], j);
      }

      // Heal squads base on cost percentage
      float remainingRepair = combatValue;
      float remainingCombatValue = totalCombatValue;
      for (uint i=0; i<numSquadsToRepair; i++)
      {
         BSquad* pSquad = gWorld->getSquad(sortedSquads[i]);
         float squadHealCost = (sortedBaseCombatValues[i] / remainingCombatValue) * remainingRepair;
         float excessRepair = 0.0f;
         pSquad->repairCombatValue(squadHealCost, allowReinforce, excessRepair);
         remainingRepair -= (squadHealCost - excessRepair);
         remainingCombatValue -= sortedBaseCombatValues[i];
      }
   }
   // Just repair each squad by the specified combat value
   else
   {
      for (uint i=0; i<numSquads; i++)
      {
         BSquad* pSquad = gWorld->getSquad(squadIDs[i]);
         if (pSquad)
         {
            float excess;
            pSquad->repairCombatValue(combatValue, allowReinforce, excess);
         }
      }
   }
}

//==============================================================================
//==============================================================================
BVector BSimHelper::clampPosToAnchor(const BVector anchorPos, const BVector testPos, float maxDist)
{
   // It's the anchor pos.
   if (maxDist <= 0.0f)
      return (anchorPos);

   // It's the test pos.
   BVector dir = testPos - anchorPos;
   float lengthSqr = dir.lengthSquared();
   float maxDistSqr = maxDist * maxDist;
   if (lengthSqr <= maxDistSqr)
      return (testPos);

   // It needs to be clamped.
   dir.normalize();
   return (anchorPos + (dir * maxDist));
}

//==============================================================================
// Sort entities based on distance from reference point
//==============================================================================
void BSimHelper::sortEntitiesBasedOnDistance(const BEntityIDArray &entityList, BVector referencePos, BEntityIDArray &results)
{
   // This is usually called with entityList and results pointing to the same list so make a copy of it first.  :(
   static BEntityIDArray tempEntityList;
   tempEntityList.assignNoDealloc(entityList);

   static BDynamicSimLongArray positionSortOrderAll;
   static BDynamicSimFloatArray positionDist;
   positionSortOrderAll.resize(0);
   positionDist.resize(0);
   uint numEntities = tempEntityList.getSize();
   for (uint i = 0; i < numEntities; i++)
   {
      BEntity* pEntity = gWorld->getEntity(tempEntityList[i]);
      if (pEntity)
      {
         uint insertIndex = 0;
         float posDist = referencePos.distanceSqr(pEntity->getPosition());
         while ((insertIndex < positionDist.getSize()) && (posDist > positionDist[insertIndex]))
         {
            insertIndex++;
         }

         positionDist.insertAtIndex(posDist, insertIndex);
         positionSortOrderAll.insertAtIndex(i, insertIndex);
      }
   }

   results.resize(0);
   uint numPositionsSortOrderAll = positionSortOrderAll.getSize();
   for (uint i = 0; i < numPositionsSortOrderAll; i++)
   {
      results.add(tempEntityList[positionSortOrderAll[i]]);
   }
}

//==============================================================================
// Play one of the SPC VoG cost errors
//==============================================================================
void BSimHelper::playSPCCostErrorSound(BPlayerID playerID, const BCost* pCost, const BPopArray* pPop)
{
   BGameSettings* pSettings = gDatabase.getGameSettings();
   if (pSettings)
   {
      long gameType = -1;
      pSettings->getLong(BGameSettings::cGameType, gameType);
      if (gameType == BGameSettings::cGameTypeCampaign)
      {
         BPlayer* pPlayer = gWorld->getPlayer(playerID);
         if (pPlayer)
         {
            if (pPop && !pPlayer->checkPops(pPop))
            {
               gSoundManager.playSoundCueByEnum(BSoundManager::cSoundVOGNeedPop);
               return;
            }

            if (pCost)
            {
               BResourceID suppliesID = gDatabase.getRIDSupplies();
               BResourceID powerID = gDatabase.getRIDPower();
               float supplyCost = pCost->get(suppliesID);
               float powerCost = pCost->get(powerID);
               float supplyLevel = pPlayer->getResource(suppliesID);
               float powerLevel = pPlayer->getResource(powerID);
               bool supplyError = (supplyCost > supplyLevel);
               bool powerError = (powerCost > powerLevel);
               if (supplyError && powerError)
               {
                  float flip = getRandRangeFloat(cUnsyncedRand, 0.0f, 1.0f);
                  if (flip > 0.5f)
                  {
                     gSoundManager.playSoundCueByEnum(BSoundManager::cSoundVOGNeedSupplies);
                  }
                  else
                  {
                     gSoundManager.playSoundCueByEnum(BSoundManager::cSoundVOGNeedReactors);
                  }
               }
               else if (supplyError)
               {
                  gSoundManager.playSoundCueByEnum(BSoundManager::cSoundVOGNeedSupplies);
               }
               else
               {
                  gSoundManager.playSoundCueByEnum(BSoundManager::cSoundVOGNeedReactors);
               }

               return;
            }
         }
      }
   }

   gUI.playCantDoSound();
}

//==============================================================================
//==============================================================================
void BSimHelper::advanceToNewPosition(BObject* pObject, float elapsedTime, const BVector& newPosition, const BVector& newForward)
{
   BASSERT(pObject);
   if (!pObject)
      return;

   // Calculate SIM velocity
   BVisual* pVisual = pObject->getVisual();
   BVector simTranslation;
   simTranslation.assignDifference(newPosition, pObject->getPosition());
   simTranslation.y = 0.0f;
   BVector simVelocity;
   float oneOverTime = 1.0f / elapsedTime;
   simVelocity.assignProduct(oneOverTime, simTranslation);
   pObject->setVelocity(simVelocity);

   if ((elapsedTime > cFloatCompareEpsilon) && pVisual && (pVisual->mModelAsset.mType == cVisualAssetGrannyModel) && !pObject->getFlagPhysicsControl() && (gConfig.isDefined(cConfigEnableMotionExtraction) || gWorld->isPlayingCinematic()))
   {
      //-- FIXING PREFIX BUG ID 3381
      const BGrannyInstance* pGrannyInstance = (BGrannyInstance *) pVisual->mpInstance;
      //--

      // It is. Scale the animation appropriately.
      if (pGrannyInstance && pGrannyInstance->hasMotionExtraction())
      {
         float animationDuration = pObject->getAnimationDuration(cMovementAnimationTrack);
         if (animationDuration > cFloatCompareEpsilon)
         {
            // SLB: TODO precalc this
            // Calculate motion extraction velocity
            BMatrix extractedMotion;
            extractedMotion.makeIdentity();
            pGrannyInstance->getExtractedMotion(animationDuration, extractedMotion);
            BVector extractedTranslation;
            extractedMotion.getTranslation(extractedTranslation);
            syncUnitActionData("BUnitActionMove::advanceToNewPosition extractedMotion", extractedTranslation);
            extractedTranslation.y = 0.0f;
            BVector extractedVelocity;
            extractedVelocity.assignProduct(1.0f / (animationDuration * pObject->getAnimationRate()), extractedTranslation);
            float extractedScalarVelocity = extractedVelocity.dot(cZAxisVector); // project to forward vector

            if (extractedScalarVelocity > cFloatCompareEpsilon)
            {
               float simScalarVelocity = simVelocity.length();

               // Set animation rate
               float animationRate = simScalarVelocity / extractedScalarVelocity;
               float clampedAnimationRate = Math::Clamp(animationRate, 0.5f, 2.0f);
               pObject->setAnimationRate(clampedAnimationRate);

               // TRB 2/11/07  Don't do motion extraction collision check while moving.  The move action avoids collisions
               // so don't let the motion extraction code do it since float calculation accuracy could detect unwanted collisions.
               pObject->setFlagMotionCollisionChecked(true);

               if (clampedAnimationRate == animationRate)
                  return;
            }
         }
      }
   }

   // No motion extraction. Just set position.
   pObject->setFlagSkipMotionExtraction(true);
   pObject->setAnimationRate(1.0f);
   pObject->setPosition(newPosition);
}

//==============================================================================
//==============================================================================
void BSimHelper::getUnitsAlongSegment(BVector point1, BVector point2, BEntityIDArray &results)
{
   results.resize(0);

   static BObstructionNodePtrArray collisionObs;
   collisionObs.resize(0);

   getUnitObstructionsAlongSegment(point1, point2, collisionObs);

   // Add to results list
   uint obIndex;
   uint obCount = collisionObs.getNumber();
   for (obIndex = 0; obIndex < obCount; obIndex++)
   {
      const BOPObstructionNode* pObstructionNode = collisionObs[obIndex];
      if ((pObstructionNode == NULL) || (pObstructionNode->mObject == NULL))
         continue;
      results.add(pObstructionNode->mObject->getID());
   }
}

//==============================================================================
//==============================================================================
void BSimHelper::getUnitObstructionsAlongSegment(BVector point1, BVector point2, BObstructionNodePtrArray &collisionObs)
{
   collisionObs.resize(0);
   BVector intersectionPoint(0.0f);
   long lObOptions=
      BObstructionManager::cIsNewTypeCollidableMovingUnit |
      BObstructionManager::cIsNewTypeCollidableStationaryUnit |
      BObstructionManager::cIsNewTypeSolidNonMoveableUnits |
      BObstructionManager::cIsNewTypeBlockAirMovement |
      BObstructionManager::cIsNewTypeNonCollidableUnit;
   long lObNodeType = BObstructionManager::cObsNodeTypeUnit;

   gObsManager.begin(BObstructionManager::cBeginEntity, 0.0f, lObOptions, lObNodeType, 0, 0.0f, NULL, false);
   gObsManager.getObjectsIntersections(BObstructionManager::cGetAllIntersections, point1, point2, false, intersectionPoint, collisionObs);
   gObsManager.end();
}


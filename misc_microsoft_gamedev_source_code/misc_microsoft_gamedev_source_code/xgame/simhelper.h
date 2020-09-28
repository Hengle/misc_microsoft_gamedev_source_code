//==============================================================================
// simhelper.h
//
// Copyright (c) 2008 Ensemble Studios
//==============================================================================
#pragma once

#include "aitypes.h"
#include "simtypes.h"
#include "command.h"
#include "pop.h"
#include "obstructionmanager.h"

class BEntity;
class BObject;
typedef bool (*BSimHelperFilterCallback)(BEntity* pEntity);

//==============================================================================
// class BSimHelper
// This class is just a helpful place to put useful functions that may be used in
// multiple classes to avoid code duplication or cut & paste.
//==============================================================================
class BSimHelper
{
   public:

      // Compute the centroid of a list of things.
      static BVector computeCentroid(const BEntityIDArray& entityIDs);
      static BVector computeCentroid(const BKBSquadArray& kbSquads);
      static BVector computeCentroid(const BKBSquadIDArray& kbSquadIDs);

      static float computeAssetValue(const BKBSquadIDArray& kbSquadIDs);
      static float computeTotalHP(const BKBSquadIDArray& kbSquadIDs);
      static float computeTotalHP(const BEntityIDArray& squadIDs);

      // Return the closest entityID to the point.
      static BEntityID computeClosestToPoint(BVector pos, const BEntityIDArray& entityIDs);
      static void computeMostNearAndFarToPoint(BVector pos, const BEntityIDArray& entityIDs, BEntityID* pNearID, float* pNearDistSqr, BEntityID* pFarID, float* pFarDistSqr);

      // Get a random position about a point between an inner and outer radius.
      static BVector randomCircularDistribution(BVector inputLoc, float outerRadius, float innerRadius);

      // Calculate the matrix for rendering debug circles and boxes, given the position/orientation info and a offset height above the ground.
      static void calculateDebugRenderMatrix(BVector pos, BVector forward, BVector up, BVector right, float aboveGroundHeight, BMatrix& resultMatrix);

      // Group the contents of an entity list into smaller entity lists by player ID
      static BPlayerSpecificEntityIDsArray groupEntityIDsByPlayer(const BEntityIDArray& entityIDs, BSimHelperFilterCallback filterCB = NULL);

      // Move any squads around obstruction      
      static bool moveAllSquadsFromPosition(BVector position, float range, const BEntityIDArray &ignoreSquads, bool& blocked, float testRadiusScale = 1.0f, int overrideSenderType = BCommand::cGame);
      static bool moveAllSquadsFromObstructionSquad(BEntityID obstructionSquad, bool& blocked, float testRadiusScale = 1.0f, int overrideSenderType = BCommand::cGame);

      // Calculate exit location from an ungarrison target
      static bool calculateExitLocation(const BUnit* pContainerUnit, const BSquad* pExitSquad, BVector& result);

      // Calculate a place to put a squad that was in an airborne unit that was destroyed
      static bool findPosForAirborneSquad(BSquad* pHeroSquad, float validRadius, bool moveToRally, bool killOnFail = true, bool placeWhenUnobstructed = true);

      // Determines if the exit location is blocked by non-mobile obstructions
      static bool verifyExitLocation(const BSquad* pExitSquad, const BEntityIDArray &ignoreSquads, BVector exitLocation);

      // Clear the exit location of all mobile squads
      static bool clearExitLocation(BSquad* pExitSquad, const BEntityIDArray &ignoreSquads, BVector exitLocation, bool& blocked);

      // Evaluate the exit location and determine a valid location utilizing the squad plotter
      static bool evaluateExitLocation(BSquad* pExitSquad, const BEntityIDArray &ignoreSquads, BVector exitLocation, BVector& suggestedExitLocation, float maxObsRadiusScale = 5.0f);

      // Searches around a building obstruction to find an unobstructed position for a squad to be placed.  Much faster than BWorld::checkPlacement because this has
      // a lot fewer rules it checks.
      static bool findInstantiatePositions(long numPositions, BDynamicSimVectorArray &instantiatePositions,
         float buildingObstructionRadius, BVector buildingPosition, const BVector& forward, const BVector& right,
         float squadObstructionRadius, const BVector &desiredPosition, long &closestDesiredPositionIndex, long growAttempts, bool tryInitialLocation = false, float obsRadiusGrowthMultiplierPerAttempt = 0.0f);

      // Finds an unobstructed position and sets the squad to that position.  Used as failsafe if birth anim fails due to upgrade.
      // This was written for a very special case so be careful if you start calling this everywhere.
      static bool putSquadAtUnobstructedPosition(BSquad *pSquad);

      // Ability calculating type stuff...
      static void calculateSelectionAbility(const BEntityIDArray& squadIDs, BPlayerID playerID, BSelectionAbility& ability);
      static void updateSelectionAbility(const BEntityIDArray& squadIDs, BEntityID hoverObject, BVector hoverPoint, BSelectionAbility& ability);
      static void updateValidAbilityUnload(const BEntityIDArray& squadIDs, BSelectionAbility& ability);
      static void updateValidAbilityChangeMode(const BEntityIDArray& squadIDs, BSelectionAbility& ability);
      static void updateValidAbilityPower(const BEntityIDArray& squadIDs, BSelectionAbility& ability, const BVector& hoverPoint);

      // Diff entity ID arrays.
      static void diffEntityIDArrays(const BEntityIDArray& arrayA, const BEntityIDArray& arrayB, BEntityIDArray *pOnlyInA, BEntityIDArray *pOnlyInB, BEntityIDArray *pInBoth);

      static void repairByCombatValue(const BEntityIDArray& squadIDs, float combatValue, bool spreadAcrossSquads, bool allowReinforce);
      static float getCombatValueRepairCost(const BEntityIDArray& squadIDs);

      static BVector clampPosToAnchor(const BVector anchorPos, const BVector testPos, float maxDist);

      // Sort entities based on distance
      static void sortEntitiesBasedOnDistance(const BEntityIDArray &entityList, BVector referencePos, BEntityIDArray &results);

      // Halwes - 8/22/2008 - SPC VoG stuff
      static void playSPCCostErrorSound(BPlayerID playerID, const BCost* pCost, const BPopArray* pPop);

      static void advanceToNewPosition(BObject* pObject, float elapsedTime, const BVector& newPosition, const BVector& newForward);

      static void getUnitsAlongSegment(BVector point1, BVector point2, BEntityIDArray &results);
      static void getUnitObstructionsAlongSegment(BVector point1, BVector point2, BObstructionNodePtrArray &results);
};
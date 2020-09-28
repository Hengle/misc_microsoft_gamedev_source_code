//==============================================================================
// unitactiondodge.cpp
// Copyright (c) 2006-2008 Ensemble Studios
//==============================================================================

#include "common.h"
#include "unitactiondodge.h"
#include "unitactiondeflect.h"
#include "unit.h"
#include "config.h"
#include "configsgame.h"
#include "world.h"
#include "tactic.h"
#include "physics.h"
#include "physicsobject.h"

//==============================================================================
//==============================================================================
IMPLEMENT_FREELIST(BUnitActionDodge, 5, &gSimHeap);

//==============================================================================
//==============================================================================
BUnitActionDodge::BUnitActionDodge()
{
}

//==============================================================================
//==============================================================================
bool BUnitActionDodge::init()
{
   if (!BAction::init())
      return(false);

   mDodgeAnimOppID=BUnitOpp::cInvalidID;
   mDodgeeID=cInvalidObjectID;
   mDodgeCooldownDoneTime=0;
   mDodgeInitTime=0;

   return(true);
}


//==============================================================================
//==============================================================================
bool BUnitActionDodge::update(float elapsed)
{
   // Reset times to 0 if we've passed them
   if ((mDodgeCooldownDoneTime != 0) && (gWorld->getGametime() >= mDodgeCooldownDoneTime))
   {
      mDodgeAnimOppID = BUnitOpp::cInvalidID;
      mDodgeeID = cInvalidObjectID;
      mDodgeCooldownDoneTime = 0;
   }

   // Debugging
   #ifndef BUILD_FINAL
      if (gConfig.isDefined(cConfigDebugDodge))
      {
         if (mDodgeeID != cInvalidObjectID)
         {
            const BUnit* pUnit = getOwner()->getUnit();
            float radius = pUnit->getObstructionRadius();
            gTerrainSimRep.addDebugThickCircleOverTerrain(pUnit->getPosition(), radius, 0.5f, cDWORDGreen, 0.5f);
         }
      }
   #endif

   return true;
}

//==============================================================================
//==============================================================================
void BUnitActionDodge::notify(DWORD eventType, BEntityID senderID, DWORD data1, DWORD data2)
{
   // Dodge complete if the anim opp is done
   if ((eventType == BEntity::cEventOppComplete) && (data1 == mDodgeAnimOppID))
   {
      mDodgeAnimOppID = BUnitOpp::cInvalidID;
      mDodgeeID = cInvalidObjectID;

      // Set next available dodge time
      mDodgeCooldownDoneTime = gWorld->getGametime() + getProtoAction()->getDodgeCooldown();
   }
}

//==============================================================================
//==============================================================================
bool BUnitActionDodge::canDodge() const
{
   if (getProtoAction()->getFlagDisabled())
      return (false);

   // Already dodging
   if (mDodgeeID != cInvalidObjectID)
      return false;

   // Don't dodge if in cover
   if (mpOwner && mpOwner->isInCover())
      return false;

   // Cooling down from last dodge
   if (gWorld->getGametime() < mDodgeCooldownDoneTime)
      return false;

   BUnit* pUnit = reinterpret_cast<BUnit*>(mpOwner);
   BASSERT(pUnit);
   if (!pUnit)
      return false;

   if (getProtoAction()->getFlagWaitForDeflectCooldown())
   {
      BUnitActionDeflect* pAction = reinterpret_cast<BUnitActionDeflect*>(pUnit->getActionByType(BAction::cActionTypeUnitDeflect));
      if (pAction && pAction->getProtoAction())
      {
         if (gWorld->getGametime() < pAction->getDeflectCooldownDoneTime())
            return false;
      }
   }
   BSquad* pSquad = pUnit->getParentSquad();
   BASSERT(pSquad);
   if (!pSquad)
      return false;

   // No fair dodging under stasis
   if (pSquad->getNumStasisEffects() > 0)
      return false;

   return true;
}

//==============================================================================
//==============================================================================
bool BUnitActionDodge::tryDodge(BEntityID dodgeeID, BVector trajectory)
{
   BUnit* pUnit = reinterpret_cast<BUnit*>(mpOwner);
   BASSERT(pUnit);
   BProtoAction* pPA = const_cast<BProtoAction*>(getProtoAction());
   BASSERT(pPA);
   BSquad* pSquad = pUnit->getParentSquad();
   BASSERT(pSquad);

   // Check to see if can dodge
   if (!canDodge())
      return false;

   // Check angle between dodgee trajectory and forward for
   // calculating chance to dodge
   trajectory.y = 0.0f;
   trajectory.normalize();
   BVector fwd = pUnit->getForward();
   fwd.y = 0.0f;
   fwd.normalize();
   float angleBetween = fwd.angleBetweenVector(-trajectory);
   if (angleBetween >= pPA->getDodgeMaxAngle())
      return false;
   float chance = Math::Lerp(pPA->getDodgeChanceMax(), pPA->getDodgeChanceMin(), (angleBetween / pPA->getDodgeMaxAngle()));

   // Check chance to dodge or block
   float rand = getRandRangeFloat(cSimRand, 0.0f, 1.0f);
   if (rand > chance)
      return false;

   // Determine whether unit leashed
   BVector leashLocation(0.0f);
   float minDistToLeashLocation = FLT_MAX;
   uint bestAnimForLeashing = 0;
   bool isLeashed = pSquad->isChildLeashed(pUnit->getID(), 0.0f, leashLocation);

   // Anim or physics driven
   bool physicsDriven = pPA->getDodgePhysicsImpulse() > 0.0f;

   // List of valid anims (or positions for physics use)
   static BSmallDynamicSimArray<uint> validAnims;
   validAnims.clear();

   // Find unobstructed dodge positions
   long quadTreesToScan = /*BObstructionManager::cIsNewTypeAllCollidableUnits |
                          BObstructionManager::cIsNewTypeAllCollidableSquads |
                          */
                          BObstructionManager::cIsNewTypeCollidableStationaryUnit | 
                          BObstructionManager::cIsNewTypeCollidableNonMovableUnit | 
                          /*BObstructionManager::cIsNewTypeCollidableStationarySquad | */
                          BObstructionManager::cIsNewTypeBlockLandUnits |
                          BObstructionManager::cIsNewTypeDoppleganger;
   long nodeTypes = BObstructionManager::cObsNodeTypeAll;
   BVector unitPos = pUnit->getPosition();
   BVector right = pUnit->getRight();
   float radius = pUnit->getObstructionRadius();
   float dodgeDist = radius * 3.0f;
   BPlayerID playerID = pUnit->getPlayerID();

   static BEntityIDArray ignoreList;
   ignoreList.clear();
   ignoreList.add(pUnit->getID());
   gObsManager.begin(BObstructionManager::cBeginEntity, 0.0f, quadTreesToScan, nodeTypes, playerID, 0.0f, &ignoreList, pUnit->canJump());

   // Right
   BVector pos;
   if (pUnit->hasAnimation(cAnimTypeEvadeRight) || physicsDriven)
   {
      pos = unitPos + right * dodgeDist;
      bool obstructed = obstructedByTerrainBoundaries(pos, radius);
      if (!obstructed)
         obstructed = gObsManager.testObstructions(pos, radius, 0.0f, quadTreesToScan, nodeTypes, playerID);
      if (!obstructed)
      {
         // Weight left/right 2x of front/back
         validAnims.add(cAnimTypeEvadeRight);
         validAnims.add(cAnimTypeEvadeRight);

         // If not leashed then see if this is the best direction to get unit closest to leash location
         if (!isLeashed)
         {
            float distToLeash = pos.xzDistanceSqr(leashLocation);
            if (distToLeash < minDistToLeashLocation)
            {
               minDistToLeashLocation = distToLeash;
               bestAnimForLeashing = cAnimTypeEvadeRight;
            }
         }
      }

      #ifndef BUILD_FINAL
         if (gConfig.isDefined(cConfigDebugDodge))
            gTerrainSimRep.addDebugCircleOverTerrain(pos, radius, obstructed ? cDWORDRed : cDWORDGreen, 0.5f, BDebugPrimitives::cCategoryNone, 2.0f);
      #endif
   }

   // Left
   if (pUnit->hasAnimation(cAnimTypeEvadeLeft) || physicsDriven)
   {
      pos = unitPos - right * dodgeDist;
      bool obstructed = obstructedByTerrainBoundaries(pos, radius);
      if (!obstructed)
         obstructed = gObsManager.testObstructions(pos, radius, 0.0f, quadTreesToScan, nodeTypes, playerID);
      if (!obstructed)
      {
         // Weight left/right 2x of front/back
         validAnims.add(cAnimTypeEvadeLeft);
         validAnims.add(cAnimTypeEvadeLeft);

         // If not leashed then see if this is the best direction to get unit closest to leash location
         if (!isLeashed)
         {
            float distToLeash = pos.xzDistanceSqr(leashLocation);
            if (distToLeash < minDistToLeashLocation)
            {
               minDistToLeashLocation = distToLeash;
               bestAnimForLeashing = cAnimTypeEvadeLeft;
            }
         }
      }

      #ifndef BUILD_FINAL
         if (gConfig.isDefined(cConfigDebugDodge))
            gTerrainSimRep.addDebugCircleOverTerrain(pos, radius, obstructed ? cDWORDRed : cDWORDGreen, 0.5f, BDebugPrimitives::cCategoryNone, 2.0f);
      #endif
   }

   // Forward
   if (pUnit->hasAnimation(cAnimTypeEvadeFront) || physicsDriven)
   {
      pos = unitPos + fwd * dodgeDist;
      bool obstructed = obstructedByTerrainBoundaries(pos, radius);
      if (!obstructed)
         obstructed = gObsManager.testObstructions(pos, radius, 0.0f, quadTreesToScan, nodeTypes, playerID);
      if (!obstructed)
      {
         validAnims.add(cAnimTypeEvadeFront);

         // If not leashed then see if this is the best direction to get unit closest to leash location
         if (!isLeashed)
         {
            float distToLeash = pos.xzDistanceSqr(leashLocation);
            if (distToLeash < minDistToLeashLocation)
            {
               minDistToLeashLocation = distToLeash;
               bestAnimForLeashing = cAnimTypeEvadeFront;
            }
         }
      }

      #ifndef BUILD_FINAL
         if (gConfig.isDefined(cConfigDebugDodge))
            gTerrainSimRep.addDebugCircleOverTerrain(pos, radius, obstructed ? cDWORDRed : cDWORDGreen, 0.5f, BDebugPrimitives::cCategoryNone, 2.0f);
      #endif
   }

   // Back
   if (pUnit->hasAnimation(cAnimTypeEvadeBack) || physicsDriven)
   {
      pos = unitPos - fwd * dodgeDist;
      bool obstructed = obstructedByTerrainBoundaries(pos, radius);
      if (!obstructed)
         obstructed = gObsManager.testObstructions(pos, radius, 0.0f, quadTreesToScan, nodeTypes, playerID);
      if (!obstructed)
      {
         validAnims.add(cAnimTypeEvadeBack);

         // If not leashed then see if this is the best direction to get unit closest to leash location
         if (!isLeashed)
         {
            float distToLeash = pos.xzDistanceSqr(leashLocation);
            if (distToLeash < minDistToLeashLocation)
            {
               minDistToLeashLocation = distToLeash;
               bestAnimForLeashing = cAnimTypeEvadeBack;
            }
         }
      }

      #ifndef BUILD_FINAL
         if (gConfig.isDefined(cConfigDebugDodge))
            gTerrainSimRep.addDebugCircleOverTerrain(pos, radius, obstructed ? cDWORDRed : cDWORDGreen, 0.5f, BDebugPrimitives::cCategoryNone, 2.0f);
      #endif
   }

   gObsManager.end();

   // Setup opp to play dodge / block anim
   if (validAnims.getNumber() > 0)
   {
      uint8 animIndex = static_cast<uint8>(getRand(cSimRand)%validAnims.getSize());
      uint chosenAnim = validAnims[animIndex];

      // If not leashed, choose the anim that will get it closest to the leash position
      if (!isLeashed && (bestAnimForLeashing != 0))
         chosenAnim = bestAnimForLeashing;

      // Calculate and apply physics driven impulse if specified
      if (physicsDriven)
      {
         BPhysicsObject* pPO = pUnit->getPhysicsObject();
         if (!pPO || !pUnit->getFlagPhysicsControl())
            return false;
         BVector dir;
         switch (chosenAnim)
         {
            case cAnimTypeEvadeRight:
               dir = pUnit->getRight();
               break;
            case cAnimTypeEvadeLeft:
               dir = -pUnit->getRight();
               break;
            case cAnimTypeEvadeFront:
               dir = pUnit->getForward();
               break;
            case cAnimTypeEvadeBack:
               dir = -pUnit->getForward();
               break;
         }

         float force = pPA->getDodgePhysicsImpulse();
         BVector impulse;
         impulse = dir * force * pPO->getMass();
         pPO->applyImpulse(impulse);

         mDodgeCooldownDoneTime = gWorld->getGametime() + getProtoAction()->getDodgeCooldown();
      }
      // Otherwise to animation driven dodge
      else
      {
         BUnitOpp* pNewOpp=BUnitOpp::getInstance();
         pNewOpp->init();
         //pNewOpp->setTarget(BSimTarget());
         pNewOpp->setSource(pUnit->getID());
         pNewOpp->setNotifySource(true);
         pNewOpp->setUserData(static_cast<uint16>(chosenAnim));
         pNewOpp->setType(BUnitOpp::cTypeEvade);
         pNewOpp->setPreserveDPS(true);
         pNewOpp->setPriority(BUnitOpp::cPriorityCritical);
         pNewOpp->generateID();
         if (!pUnit->addOpp(pNewOpp))
         {
            BUnitOpp::releaseInstance(pNewOpp);
               return false;
         }
         setDodgeAnimOppID(pNewOpp->getID());
      }

      setDodgeeID(dodgeeID);
      mDodgeInitTime = gWorld->getGametime();

      return true;
   }

   return false;
}


//==============================================================================
//==============================================================================
bool BUnitActionDodge::save(BStream* pStream, int saveType) const
{
   if (!BAction::save(pStream, saveType))
      return false;
   GFWRITEVAR(pStream, BUnitOppID, mDodgeAnimOppID);
   GFWRITEVAR(pStream, BEntityID, mDodgeeID);
   GFWRITEVAR(pStream, DWORD, mDodgeCooldownDoneTime);
   GFWRITEVAR(pStream, DWORD, mDodgeInitTime);
   return true;
}

//==============================================================================
//==============================================================================
bool BUnitActionDodge::load(BStream* pStream, int saveType)
{
   if (!BAction::load(pStream, saveType))
      return false;
   GFREADVAR(pStream, BUnitOppID, mDodgeAnimOppID);
   GFREADVAR(pStream, BEntityID, mDodgeeID);
   GFREADVAR(pStream, DWORD, mDodgeCooldownDoneTime);
   GFREADVAR(pStream, DWORD, mDodgeInitTime);
   return true;
}

//==============================================================================
//==============================================================================
bool BUnitActionDodge::obstructedByTerrainBoundaries(BVector pos, float radius)
{
   float minBoundary = cFloatCompareEpsilon;
   float maxBoundary = gTerrainSimRep.getNumXDataTiles() * gTerrainSimRep.getDataTileScale() - cFloatCompareEpsilon;

   if ((pos.x - radius) < minBoundary)
      return true;
   if ((pos.z - radius) < minBoundary)
      return true;
   if ((pos.x + radius) > maxBoundary)
      return true;
   if ((pos.z + radius) > maxBoundary)
      return true;

   return false;
}
